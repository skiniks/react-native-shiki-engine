#include "IOSHighlightRenderer.h"
#include "../utils/WorkPrioritizer.h"
#import "LineNumberView.h"
#import "ShikiTextView.h"
#import <UIKit/UIKit.h>

namespace shiki {

IOSHighlightRenderer &IOSHighlightRenderer::getInstance() {
  static IOSHighlightRenderer instance;
  return instance;
}

IOSHighlightRenderer::IOSHighlightRenderer() {
  // Create serial queues for style and layout work
  styleQueue_ = dispatch_queue_create("com.shiki.style", DISPATCH_QUEUE_SERIAL);
  layoutQueue_ =
      dispatch_queue_create("com.shiki.layout", DISPATCH_QUEUE_SERIAL);
}

IOSHighlightRenderer::~IOSHighlightRenderer() {
  // No need to call dispatch_release with ARC
}

void IOSHighlightRenderer::applyLineNumberStyle(const ThemeStyle &style,
                                                void *lineNumberView) {
  LineNumberView *view = (__bridge LineNumberView *)lineNumberView;

  auto lineStyle = style.getLineNumberStyle();

  // Apply text color
  if (lineStyle.color) {
    auto color = ThemeColor::fromHex(*lineStyle.color);
    view.textColor = color->toUIColor();
  }

  // Apply background color
  if (lineStyle.backgroundColor) {
    auto bgColor = ThemeColor::fromHex(*lineStyle.backgroundColor);
    view.backgroundColor = bgColor->toUIColor();
  }

  // Apply font styling
  if (view.font) {
    UIFont *baseFont = view.font;
    UIFontDescriptor *descriptor = baseFont.fontDescriptor;
    UIFontDescriptorSymbolicTraits traits = descriptor.symbolicTraits;

    if (lineStyle.bold) {
      traits |= UIFontDescriptorTraitBold;
    }
    if (lineStyle.italic) {
      traits |= UIFontDescriptorTraitItalic;
    }

    UIFontDescriptor *newDescriptor =
        [descriptor fontDescriptorWithSymbolicTraits:traits];
    UIFont *styledFont = [UIFont fontWithDescriptor:newDescriptor
                                               size:baseFont.pointSize];
    if (styledFont) {
      view.font = styledFont;
    }
  }
}

void *IOSHighlightRenderer::createView(const ViewConfig &config) {
  ShikiTextView *textView = [[ShikiTextView alloc] initWithFrame:CGRectZero];
  configureTextView(textView, config);

  // Store config for later use
  viewConfigs_[(__bridge void *)textView] = config;

  return (__bridge void *)textView;
}

void IOSHighlightRenderer::destroyView(void *view) {
  if (!view)
    return;

  ShikiTextView *textView = (__bridge ShikiTextView *)view;
  [textView cancelPendingHighlights];
  [textView prepareForReuse];

  // Remove from configs
  viewConfigs_.erase(view);
}

void IOSHighlightRenderer::updateView(void *view,
                                      const std::vector<Token> &tokens,
                                      const std::string &text) {
  if (!view)
    return;

  // Cancel any pending work for this view
  cancelPendingWork(view);

  // Create work package
  StyleComputationWork work{
      tokens, text, view,
      false // not incremental
  };

  // Process in background
  processStylesInBackground(std::move(work));
}

void IOSHighlightRenderer::configureTextView(UITextView *textView,
                                             const ViewConfig &config) {
  textView.editable = NO;
  textView.selectable = config.selectable;
  textView.scrollEnabled = config.scrollEnabled;
  textView.backgroundColor = [UIColor clearColor];
  textView.textContainer.lineFragmentPadding = 0;
  textView.textContainerInset = *(const UIEdgeInsets *)&config.contentInset;

  // Set initial font
  FontConfig fontConfig;
  fontConfig.fontSize = config.fontSize;
  fontConfig.fontFamily = config.fontFamily;
  UIFont *font = FontManager::getInstance().createFont(fontConfig);
  if (font) {
    textView.font = font;
  }
}

void IOSHighlightRenderer::renderIncrementalHighlights(
    const std::vector<IncrementalUpdate> &updates, const std::string &text,
    void *nativeView) {

  if (!nativeView)
    return;

  // Cancel any pending work
  cancelPendingWork(nativeView);

  // Convert updates to tokens
  std::vector<Token> allTokens;
  for (const auto &update : updates) {
    allTokens.insert(allTokens.end(), update.tokens.begin(),
                     update.tokens.end());
  }

  // Create work package
  StyleComputationWork work{
      std::move(allTokens), text, nativeView,
      true // incremental update
  };

  // Process in background
  processStylesInBackground(std::move(work));
}

void IOSHighlightRenderer::processStylesInBackground(
    StyleComputationWork work) {
  pendingWorkCount_++;

  // Capture weak reference to view
  __weak ShikiTextView *weakView = (__bridge ShikiTextView *)work.targetView;

  // Create work item
  auto styleTask = [this, work = std::move(work), weakView]() {
    @autoreleasepool {
      // Check if view still exists
      ShikiTextView *strongView = weakView;
      if (!strongView) {
        pendingWorkCount_--;
        return;
      }

      // Create attributed string
      NSString *nsText = @(work.text.c_str());
      NSMutableAttributedString *attributedString;

      if (work.isIncremental) {
        // For incremental updates, start with existing text
        attributedString = strongView.attributedText.mutableCopy;
      } else {
        attributedString =
            [[NSMutableAttributedString alloc] initWithString:nsText];
      }

      UIFont *baseFont = getCurrentFont();

      // Apply styles in background
      for (const auto &token : work.tokens) {
        NSRange range = NSMakeRange(token.start, token.length);
        applyStyle(token.style, (__bridge void *)attributedString, range,
                   baseFont);
      }

      // Move to layout queue for final processing
      computeLayoutInBackground(attributedString, work.targetView);
    }
  };

  // Determine priority based on whether this is an incremental update
  WorkPriority priority =
      work.isIncremental ? WorkPriority::HIGH : WorkPriority::NORMAL;

  // Estimate work cost based on token count and text size
  size_t estimatedCost =
      work.tokens.size() * sizeof(Token) + work.text.length();

  // Submit to work prioritizer
  WorkPrioritizer::getInstance().submitWork(
      WorkItem(std::move(styleTask), priority, estimatedCost,
               "style_computation", work.isIncremental));
}

void IOSHighlightRenderer::computeLayoutInBackground(
    NSAttributedString *styledText, void *targetView) {

  dispatch_async(layoutQueue_, ^{
    @autoreleasepool {
      __weak ShikiTextView *weakView = (__bridge ShikiTextView *)targetView;
      ShikiTextView *strongView = weakView;
      if (!strongView) {
        pendingWorkCount_--;
        return;
      }

      // Create text container for layout
      NSTextContainer *container = [[NSTextContainer alloc] init];
      container.size = strongView.bounds.size;
      container.widthTracksTextView = YES;

      // Create layout manager
      NSLayoutManager *layoutManager = [[NSLayoutManager alloc] init];
      [layoutManager addTextContainer:container];

      // Create temporary storage
      NSTextStorage *storage = [[NSTextStorage alloc] init];
      [storage addLayoutManager:layoutManager];
      [storage setAttributedString:styledText];

      // Force layout computation
      [layoutManager ensureLayoutForTextContainer:container];

      // Apply to view on main thread
      applyStylesToViewOnMain(targetView, styledText);
    }
  });
}

void IOSHighlightRenderer::applyStylesToViewOnMain(void *view,
                                                   NSAttributedString *text) {

  dispatch_async(dispatch_get_main_queue(), ^{
    @autoreleasepool {
      __weak ShikiTextView *weakView = (__bridge ShikiTextView *)view;
      ShikiTextView *strongView = weakView;
      if (!strongView) {
        pendingWorkCount_--;
        return;
      }

      [strongView beginBatchUpdates];
      strongView.attributedText = text;
      [strongView endBatchUpdates];

      pendingWorkCount_--;
    }
  });
}

void IOSHighlightRenderer::cancelPendingWork(void *view) {
  ShikiTextView *textView = (__bridge ShikiTextView *)view;
  [textView cancelPendingHighlights];
}

void IOSHighlightRenderer::applyStyle(const ThemeStyle &style,
                                      void *attributedString, NSRange range,
                                      UIFont *baseFont) {
  NSMutableAttributedString *attrStr =
      (__bridge NSMutableAttributedString *)attributedString;

  // Apply text color
  if (!style.color.empty()) {
    auto color = ThemeColor::fromHex(style.color);
    [attrStr addAttribute:NSForegroundColorAttributeName
                    value:color->toUIColor()
                    range:range];
  }

  // Apply background color
  if (!style.backgroundColor.empty()) {
    auto bgColor = ThemeColor::fromHex(style.backgroundColor);
    [attrStr addAttribute:NSBackgroundColorAttributeName
                    value:bgColor->toUIColor()
                    range:range];
  }

  // Apply font styling
  if (baseFont) {
    UIFontDescriptor *descriptor = baseFont.fontDescriptor;
    UIFontDescriptorSymbolicTraits traits = descriptor.symbolicTraits;

    if (style.bold) {
      traits |= UIFontDescriptorTraitBold;
    }
    if (style.italic) {
      traits |= UIFontDescriptorTraitItalic;
    }

    UIFontDescriptor *newDescriptor =
        [descriptor fontDescriptorWithSymbolicTraits:traits];
    UIFont *styledFont = [UIFont fontWithDescriptor:newDescriptor
                                               size:baseFont.pointSize];
    if (styledFont) {
      [attrStr addAttribute:NSFontAttributeName value:styledFont range:range];
    }
  }
}

} // namespace shiki

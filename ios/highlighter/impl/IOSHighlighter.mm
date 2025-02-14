#include "IOSHighlighter.h"
#include "../../../cpp/highlighter/tokenizer/ShikiTokenizer.h"
#include "../../../cpp/highlighter/grammar/Grammar.h"
#include "../../../cpp/highlighter/theme/Theme.h"
#import "../../views/ShikiTextView.h"
#import "../../views/ShikiViewState.h"
#include <sstream>

namespace shiki {

namespace {
// Helper method to convert hex color string to UIColor
UIColor* colorFromHex(const std::string& hexStr) {
  NSString* hexString = [NSString stringWithUTF8String:hexStr.c_str()];
  if (![hexString hasPrefix:@"#"]) {
    hexString = [@"#" stringByAppendingString:hexString];
  }

  NSString* cleanString = [hexString stringByReplacingOccurrencesOfString:@"#" withString:@""];
  if([cleanString length] == 3) {
    cleanString = [NSString stringWithFormat:@"%@%@%@%@%@%@",
                  [cleanString substringWithRange:NSMakeRange(0, 1)],[cleanString substringWithRange:NSMakeRange(0, 1)],
                  [cleanString substringWithRange:NSMakeRange(1, 1)],[cleanString substringWithRange:NSMakeRange(1, 1)],
                  [cleanString substringWithRange:NSMakeRange(2, 1)],[cleanString substringWithRange:NSMakeRange(2, 1)]];
  }

  unsigned int baseValue;
  [[NSScanner scannerWithString:cleanString] scanHexInt:&baseValue];

  CGFloat red = ((baseValue >> 16) & 0xFF)/255.0f;
  CGFloat green = ((baseValue >> 8) & 0xFF)/255.0f;
  CGFloat blue = (baseValue & 0xFF)/255.0f;

  return [UIColor colorWithRed:red green:green blue:blue alpha:1.0];
}
} // namespace

IOSHighlighter& IOSHighlighter::getInstance() {
  static IOSHighlighter instance;
  return instance;
}

bool IOSHighlighter::setGrammar(const std::string& content) {
  try {
    grammar_ = Grammar::fromJson(content);
    auto& tokenizer = ShikiTokenizer::getInstance();
    tokenizer.setGrammar(grammar_);
    return true;
  } catch (const std::exception& e) {
    return false;
  }
}

bool IOSHighlighter::setTheme(const std::string& themeName) {
  try {
    theme_ = Theme::fromJson(themeName);
    auto& tokenizer = ShikiTokenizer::getInstance();
    tokenizer.setTheme(theme_);
    return true;
  } catch (const std::exception& e) {
    return false;
  }
}

std::vector<PlatformStyledToken> IOSHighlighter::highlight(const std::string& code) {
  try {
    auto tokens = ShikiTokenizer::getInstance().tokenize(code);
    std::vector<PlatformStyledToken> platformTokens;
    platformTokens.reserve(tokens.size());

    for (const auto& token : tokens) {
      PlatformStyledToken platformToken{
        token.start,
        token.length,
        token.style,
        token.getCombinedScope()
      };
      platformTokens.push_back(std::move(platformToken));
    }

    return platformTokens;
  } catch (const std::exception& e) {
    return {};
  }
}

void* IOSHighlighter::createNativeView(const PlatformViewConfig& config) {
  ShikiTextView* textView = [[ShikiTextView alloc] initWithFrame:CGRectZero];

  // Configure the text view
  textView.editable = NO;
  textView.selectable = config.selectable;
  textView.scrollEnabled = config.scrollEnabled;
  textView.backgroundColor = [UIColor clearColor];
  textView.textContainer.lineFragmentPadding = 0;

  // Set font
  UIFont* font = [UIFont fontWithName:@(config.fontFamily.c_str()) size:config.fontSize];
  if (!font) {
    font = [UIFont fontWithName:@"Menlo" size:config.fontSize];
  }
  textView.font = font;

  return (__bridge_retained void*)textView;
}

void IOSHighlighter::updateNativeView(void* view, const std::string& code) {
  if (!view) return;

  ShikiTextView* textView = (__bridge ShikiTextView*)view;
  auto platformTokens = highlight(code);

  // Update the view directly
  NSString* nsText = @(code.c_str());
  NSMutableAttributedString* attributedString = [[NSMutableAttributedString alloc] initWithString:nsText];

  // Apply styles
  for (const auto& token : platformTokens) {
    NSRange range = NSMakeRange(token.start, token.length);

    // Convert style to attributes
    NSMutableDictionary* attributes = [NSMutableDictionary dictionary];

    // Text color
    if (!token.style.color.empty()) {
      UIColor* color = colorFromHex(token.style.color);
      if (color) {
        [attributes setObject:color forKey:NSForegroundColorAttributeName];
      }
    }

    // Background color
    if (!token.style.backgroundColor.empty()) {
      UIColor* bgColor = colorFromHex(token.style.backgroundColor);
      if (bgColor) {
        [attributes setObject:bgColor forKey:NSBackgroundColorAttributeName];
      }
    }

    // Font style
    if (token.style.bold || token.style.italic) {
      UIFont* baseFont = textView.font;
      UIFontDescriptorSymbolicTraits traits = 0;
      if (token.style.bold) traits |= UIFontDescriptorTraitBold;
      if (token.style.italic) traits |= UIFontDescriptorTraitItalic;

      UIFontDescriptor* descriptor = [baseFont.fontDescriptor fontDescriptorWithSymbolicTraits:traits];
      if (descriptor) {
        UIFont* styledFont = [UIFont fontWithDescriptor:descriptor size:baseFont.pointSize];
        if (styledFont) {
          [attributes setObject:styledFont forKey:NSFontAttributeName];
        }
      }
    }

    [attributedString addAttributes:attributes range:range];
  }

  textView.attributedText = attributedString;

  if (updateCallback_) {
    updateCallback_(code, platformTokens);
  }
}

void IOSHighlighter::destroyNativeView(void* view) {
  if (!view) return;
  CFRelease(view);
}

void IOSHighlighter::setViewConfig(void* view, const PlatformViewConfig& config) {
  if (!view) return;

  ShikiTextView* textView = (__bridge ShikiTextView*)view;
  textView.selectable = config.selectable;
  textView.scrollEnabled = config.scrollEnabled;

  // Update font
  UIFont* font = [UIFont fontWithName:@(config.fontFamily.c_str()) size:config.fontSize];
  if (!font) {
    font = [UIFont fontWithName:@"Menlo" size:config.fontSize];
  }
  textView.font = font;
}

void IOSHighlighter::invalidateResources() {
  ShikiTokenizer::getInstance().setGrammar(nullptr);
  ShikiTokenizer::getInstance().setTheme(nullptr);
}

void IOSHighlighter::handleMemoryWarning() {
  invalidateResources();
}

void IOSHighlighter::setUpdateCallback(ViewUpdateCallback callback) {
  updateCallback_ = std::move(callback);
}

} // namespace shiki

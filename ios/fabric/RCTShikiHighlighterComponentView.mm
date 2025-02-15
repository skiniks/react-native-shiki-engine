#import "RCTShikiHighlighterComponentView.h"
#import "../../cpp/fabric/ShikiHighlighterComponentSpec.h"
#import "../../cpp/highlighter/renderer/IOSHighlightRenderer.h"
#import "../../cpp/highlighter/tokenizer/Token.h"
#import "../highlighter/impl/IOSHighlighter.h"
#import "../views/ShikiTextView.h"
#import "../views/ShikiViewState.h"
#import "LineNumberView.h"
#import <React/RCTBridge+Private.h>
#import <React/RCTConversions.h>
#import <React/RCTFabricComponentsPlugins.h>
#import <React/RCTFont.h>
#import <React/RCTImageResponseDelegate.h>
#import <React/RCTImageResponseObserverProxy.h>

using namespace facebook::react;

// Helper functions for bridging
static NSString *RCTBridgingToString(const std::string &str) {
  return @(str.c_str());
}

static std::string RCTBridgingFromString(NSString *str) {
  return str ? str.UTF8String : "";
}

static std::string RCTBridgingToString(NSDictionary *dict) {
  NSError *error = nil;
  NSData *jsonData = [NSJSONSerialization dataWithJSONObject:dict
                                                     options:0
                                                       error:&error];
  if (error) {
    return "";
  }
  NSString *jsonString = [[NSString alloc] initWithData:jsonData
                                               encoding:NSUTF8StringEncoding];
  return RCTBridgingFromString(jsonString);
}

@implementation RCTShikiHighlighterComponentView {
  ShikiTextView *_textView;
  LineNumberView *_lineNumberView;
  std::shared_ptr<shiki::IOSHighlighter> _highlighter;
  std::shared_ptr<shiki::IOSHighlightRenderer> _renderer;
  BOOL _isUpdating;
  BOOL _isVisible;
  BOOL _isPendingUpdate;
  NSOperationQueue *_updateQueue;
  NSUInteger _lastContentLength;
}

- (instancetype)initWithFrame:(CGRect)frame {
  if (self = [super initWithFrame:frame]) {
    static const auto defaultProps =
        std::make_shared<const ShikiHighlighterProps>();
    _props = defaultProps;

    // Get C++ singletons
    _highlighter = std::make_shared<shiki::IOSHighlighter>(
        shiki::IOSHighlighter::getInstance());

    // Create an empty shared_ptr and alias it to the singleton instance
    _renderer = std::shared_ptr<shiki::IOSHighlightRenderer>(
        std::shared_ptr<void>(), &shiki::IOSHighlightRenderer::getInstance());

    // Create view configuration
    shiki::ViewConfig viewConfig;
    viewConfig.fontSize = defaultProps->fontSize;
    viewConfig.fontFamily = defaultProps->fontFamily;
    viewConfig.showLineNumbers = defaultProps->showLineNumbers;
    viewConfig.scrollEnabled = YES;
    viewConfig.selectable = YES;

    // Create text view through C++
    void *nativeView = _renderer->createView(viewConfig);
    _textView = (__bridge ShikiTextView *)nativeView;

    // Configure selection handling
    __weak __typeof(self) weakSelf = self;
    _textView.onSelectionChange = ^(NSDictionary *selection) {
      [weakSelf handleSelectionChange:selection];
    };

    _textView.onCopy = ^(NSString *copiedText) {
      [weakSelf handleCopy:copiedText];
    };

    _lineNumberView = [[LineNumberView alloc] initWithFrame:CGRectZero];
    _lineNumberView.backgroundColor = [UIColor clearColor];
    [self addSubview:_lineNumberView];

    self.contentView = _textView;

    // Initialize update queue
    _updateQueue = [[NSOperationQueue alloc] init];
    _updateQueue.maxConcurrentOperationCount = 1;
  }
  return self;
}

- (void)dealloc {
  if (_textView) {
    _renderer->destroyView((__bridge void *)_textView);
    _textView = nil;
  }
}

#pragma mark - Selection Handling

- (void)handleSelectionChange:(NSDictionary *)selection {
  if (_eventEmitter) {
    std::dynamic_pointer_cast<const ShikiHighlighterEventEmitter>(_eventEmitter)
        ->dispatchEvent("selectionChange",
                        folly::dynamic::object("selection",
                                               RCTBridgingToString(selection)));
  }
}

- (void)handleCopy:(NSString *)copiedText {
  if (_eventEmitter) {
    std::dynamic_pointer_cast<const ShikiHighlighterEventEmitter>(_eventEmitter)
        ->dispatchEvent("copy", folly::dynamic::object(
                                    "text", RCTBridgingFromString(copiedText)));
  }
}

- (void)setSelection:(NSRange)range {
  _isUpdating = YES;
  _textView.selectedRange = range;
  _isUpdating = NO;
}

- (NSRange)getSelection {
  return _textView.selectedRange;
}

- (void)scrollToLine:(NSInteger)line {
  [_textView selectLine:line];
}

- (void)selectScope:(NSString *)scope {
  [_textView selectScope:scope];
}

- (void)selectWord:(NSInteger)location {
  [_textView selectWord:location];
}

#pragma mark - RCTComponentViewProtocol

+ (ComponentDescriptorProvider)componentDescriptorProvider {
  return concreteComponentDescriptorProvider<
      ShikiHighlighterComponentDescriptor>();
}

- (void)updateProps:(const Props::Shared &)props
           oldProps:(const Props::Shared &)oldProps {
  [super updateProps:props oldProps:oldProps];

  const auto &newHighlighterProps =
      *std::static_pointer_cast<const ShikiHighlighterProps>(props);
  const auto &oldHighlighterProps =
      *std::static_pointer_cast<const ShikiHighlighterProps>(oldProps);

  shiki::ViewConfig config;
  config.fontSize = newHighlighterProps.fontSize;
  config.fontFamily = newHighlighterProps.fontFamily;
  config.showLineNumbers = newHighlighterProps.showLineNumbers;
  config.scrollEnabled = YES;
  config.selectable = YES;

  // Update view with empty tokens and text for now - they'll be updated in
  // highlight
  std::vector<shiki::Token> emptyTokens;
  _renderer->updateView((__bridge void *)_textView, emptyTokens, "");

  // Handle highlighting updates
  if (newHighlighterProps.text != oldHighlighterProps.text ||
      newHighlighterProps.language != oldHighlighterProps.language ||
      newHighlighterProps.theme != oldHighlighterProps.theme) {

    [self updateHighlightingWithProps:newHighlighterProps];
  }
}

#pragma mark - Private

- (void)updateHighlightingWithProps:(const ShikiHighlighterProps &)props {
  NSLog(
      @"[ShikiDebug] Starting highlighting update with language: %s, theme: %s",
      props.language.c_str(), props.theme.c_str());

  if (props.text.empty() || props.language.empty() || props.theme.empty()) {
    NSLog(@"[ShikiDebug] Missing required properties - text: %d, language: %d, "
          @"theme: %d",
          !props.text.empty(), !props.language.empty(), !props.theme.empty());
    _textView.text = @(props.text.c_str());
    return;
  }

  // Cancel any pending updates
  [_updateQueue cancelAllOperations];

  if (!_isVisible) {
    NSLog(@"[ShikiDebug] View not visible, deferring update");
    _isPendingUpdate = YES;
    return;
  }

  [_textView suspendUpdates];

  // Optimize for large content
  NSUInteger contentLength = props.text.length();
  if (contentLength > 10000) {
    NSLog(@"[ShikiDebug] Large content detected (%lu chars), optimizing",
          (unsigned long)contentLength);
    [_textView optimizeForLargeContent];
  }

  _lastContentLength = contentLength;

  __weak __typeof(self) weakSelf = self;
  NSOperation *updateOperation = [NSBlockOperation blockOperationWithBlock:^{
    @try {
      __strong __typeof(weakSelf) strongSelf = weakSelf;
      if (!strongSelf) {
        NSLog(@"[ShikiDebug] Self was deallocated during update");
        return;
      }

      NSLog(@"[ShikiDebug] Setting grammar: %s", props.language.c_str());
      strongSelf->_highlighter->setGrammar(props.language);

      NSLog(@"[ShikiDebug] Setting theme: %s", props.theme.c_str());
      strongSelf->_highlighter->setTheme(props.theme);

      NSLog(@"[ShikiDebug] Starting token highlighting for %lu chars",
            (unsigned long)props.text.length());
      auto tokens = strongSelf->_highlighter->highlight(props.text);

      NSLog(@"[ShikiDebug] Generated %lu tokens", (unsigned long)tokens.size());

      // Log detailed token information for debugging
      for (size_t i = 0; i < tokens.size(); i++) {
        const auto &token = tokens[i];
        NSString *tokenText = [NSString
            stringWithUTF8String:props.text.substr(token.start, token.length)
                                     .c_str()];
        NSLog(@"[ShikiDebug] Token %zu: \n"
               "  Text: '%@'\n"
               "  Start: %zu\n"
               "  Length: %zu\n"
               "  Scope: %s\n"
               "  Color: %s\n"
               "  Background: %s\n"
               "  Font Style: %s\n"
               "  Bold: %d\n"
               "  Italic: %d\n"
               "  Underline: %d",
              i, tokenText, token.start, token.length, token.scope.c_str(),
              token.style.color.c_str(), token.style.backgroundColor.c_str(),
              token.style.fontStyle.c_str(), token.style.bold,
              token.style.italic, token.style.underline);
      }

      dispatch_async(dispatch_get_main_queue(), ^{
        if (!strongSelf) {
          NSLog(@"[ShikiDebug] Self was deallocated before applying styles");
          return;
        }

        NSLog(@"[ShikiDebug] Setting content without highlighting");
        [strongSelf->_textView
            setContentWithoutHighlighting:@(props.text.c_str())];

        NSLog(@"[ShikiDebug] Beginning batch updates");
        [strongSelf->_textView beginBatchUpdates];

        // Apply tokens to text view using NSAttributedString
        NSMutableAttributedString *attributedString =
            [[NSMutableAttributedString alloc]
                initWithString:@(props.text.c_str())];

        // Set background color for the entire text and default font
        UIColor *backgroundColor =
            [UIColor colorWithRed:0.157
                            green:0.165
                             blue:0.212
                            alpha:1.0]; // #282A36 Dracula background
        UIColor *defaultColor =
            [UIColor colorWithRed:0.973
                            green:0.973
                             blue:0.949
                            alpha:1.0]; // #F8F8F2 Dracula foreground
        UIFont *defaultFont = [UIFont fontWithName:@"Menlo" size:12.0];

        NSLog(@"[ShikiDebug] Setting default styles - bg: %@, fg: %@, font: %@",
              backgroundColor, defaultColor, defaultFont);

        [attributedString
            addAttributes:@{
              NSForegroundColorAttributeName : defaultColor,
              NSFontAttributeName : defaultFont
            }
                    range:NSMakeRange(0, attributedString.length)];

        strongSelf->_textView.backgroundColor = backgroundColor;

        for (const auto &token : tokens) {
          if (token.start + token.length > attributedString.length) {
            NSLog(@"Warning: Token range (%zu, %zu) exceeds string length %lu",
                  token.start, token.length,
                  (unsigned long)attributedString.length);
            continue;
          }

          NSRange range = NSMakeRange(token.start, token.length);
          UIColor *color = defaultColor;

          // Convert hex color to UIColor
          if (!token.style.color.empty()) {
            NSString *hexColor = @(token.style.color.c_str());
            NSLog(@"Processing token at range %@: color=%@",
                  NSStringFromRange(range), hexColor);

            // Remove '#' if present and standardize to 6 characters
            NSString *cleanHex = [hexColor hasPrefix:@"#"]
                                     ? [hexColor substringFromIndex:1]
                                     : hexColor;
            if (cleanHex.length == 3) {
              // Convert 3-digit hex to 6-digit
              cleanHex =
                  [NSString stringWithFormat:@"%C%C%C%C%C%C",
                                             [cleanHex characterAtIndex:0],
                                             [cleanHex characterAtIndex:0],
                                             [cleanHex characterAtIndex:1],
                                             [cleanHex characterAtIndex:1],
                                             [cleanHex characterAtIndex:2],
                                             [cleanHex characterAtIndex:2]];
            }

            if (cleanHex.length == 6) {
              unsigned int colorValue = 0;
              NSScanner *scanner = [NSScanner scannerWithString:cleanHex];
              if ([scanner scanHexInt:&colorValue]) {
                CGFloat red = ((colorValue & 0xFF0000) >> 16) / 255.0;
                CGFloat green = ((colorValue & 0x00FF00) >> 8) / 255.0;
                CGFloat blue = (colorValue & 0x0000FF) / 255.0;
                color = [UIColor colorWithRed:red
                                        green:green
                                         blue:blue
                                        alpha:1.0];
                NSLog(@"Successfully parsed color: r=%.2f g=%.2f b=%.2f", red,
                      green, blue);
              } else {
                NSLog(@"Failed to parse hex color: %@, using default",
                      cleanHex);
                color = defaultColor;
              }
            } else {
              NSLog(@"Invalid hex color length: %@, using default", cleanHex);
              color = defaultColor;
            }
          } else if (token.scope == "whitespace") {
            // Use default color for whitespace
            color = defaultColor;
          }

          UIFont *font = defaultFont;
          if (token.style.bold || token.style.italic) {
            UIFontDescriptorSymbolicTraits traits = 0;
            if (token.style.bold)
              traits |= UIFontDescriptorTraitBold;
            if (token.style.italic)
              traits |= UIFontDescriptorTraitItalic;

            UIFontDescriptor *descriptor =
                [font.fontDescriptor fontDescriptorWithSymbolicTraits:traits];
            if (descriptor) {
              font = [UIFont fontWithDescriptor:descriptor size:12.0];
            }
          }

          NSMutableDictionary *attributes = [NSMutableDictionary dictionary];
          [attributes setObject:color forKey:NSForegroundColorAttributeName];
          [attributes setObject:font forKey:NSFontAttributeName];

          // Only set background color if explicitly specified in the theme
          if (!token.style.backgroundColor.empty()) {
            NSString *bgHexColor = @(token.style.backgroundColor.c_str());
            NSString *cleanBgHex = [bgHexColor hasPrefix:@"#"]
                                       ? [bgHexColor substringFromIndex:1]
                                       : bgHexColor;
            if (cleanBgHex.length == 6) {
              unsigned int bgColorValue = 0;
              NSScanner *scanner = [NSScanner scannerWithString:cleanBgHex];
              if ([scanner scanHexInt:&bgColorValue]) {
                CGFloat red = ((bgColorValue & 0xFF0000) >> 16) / 255.0;
                CGFloat green = ((bgColorValue & 0x00FF00) >> 8) / 255.0;
                CGFloat blue = (bgColorValue & 0x0000FF) / 255.0;
                [attributes setObject:[UIColor colorWithRed:red
                                                      green:green
                                                       blue:blue
                                                      alpha:1.0]
                               forKey:NSBackgroundColorAttributeName];
              }
            }
          }

          [attributedString addAttributes:attributes range:range];
        }

        strongSelf->_textView.attributedText = attributedString;
        [strongSelf->_textView endBatchUpdates];
        [strongSelf->_textView resumeUpdates];
      });
    } @catch (NSException *exception) {
      NSLog(@"Error highlighting code: %@", exception);
    }
  }];

  [_updateQueue addOperation:updateOperation];
}

@end

Class<RCTComponentViewProtocol> RCTShikiHighlighterCls(void) {
  return RCTShikiHighlighterComponentView.class;
}

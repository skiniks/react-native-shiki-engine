#import "ShikiHighlighterView.h"
#import <React/RCTConversions.h>
#import <React/RCTFabricComponentsPlugins.h>
#import <React/RCTLog.h>
#import <react/renderer/components/RNShikiSpec/ComponentDescriptors.h>
#import <react/renderer/components/RNShikiSpec/Props.h>
#import <react/renderer/components/RNShikiSpec/RCTComponentViewHelpers.h>

using namespace facebook::react;

@interface ShikiHighlighterView () <RCTShikiHighlighterViewViewProtocol>
@end

@implementation ShikiHighlighterView {
  NSArray *_tokens;
  NSString *_text;
  double _fontSize;
  NSString *_fontFamily;
  BOOL _scrollEnabled;
  UIScrollView *_scrollView;
  UITextView *_textView;
}

+ (ComponentDescriptorProvider)componentDescriptorProvider {
  NSLog(@"ShikiHighlighterView: componentDescriptorProvider called");
  return concreteComponentDescriptorProvider<
      ShikiHighlighterViewComponentDescriptor>();
}

- (instancetype)initWithFrame:(CGRect)frame {
  NSLog(@"ShikiHighlighterView: initializing with frame %@",
        NSStringFromCGRect(frame));
  self = [super initWithFrame:frame];
  if (self) {
    static const auto defaultProps =
        std::make_shared<const ShikiHighlighterViewProps>();
    _props = defaultProps;

    self.clipsToBounds = YES;
    // TODO: temporarily match the dark background color from Dracula theme
    self.backgroundColor = [UIColor colorWithRed:40.0 / 255.0
                                           green:42.0 / 255.0
                                            blue:54.0 / 255.0
                                           alpha:1.0];
    _fontSize = 14.0;
    _scrollEnabled = YES;

    _scrollView = [[UIScrollView alloc] initWithFrame:self.bounds];
    _scrollView.autoresizingMask =
        UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    _scrollView.showsHorizontalScrollIndicator = YES;
    _scrollView.showsVerticalScrollIndicator = YES;
    _scrollView.alwaysBounceHorizontal = NO;
    _scrollView.alwaysBounceVertical = YES;
    _scrollView.contentInset =
        UIEdgeInsetsMake(16, 0, 16, 0); // Add top and bottom margin
    [self addSubview:_scrollView];

    _textView = [[UITextView alloc] initWithFrame:_scrollView.bounds];
    _textView.autoresizingMask =
        UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    _textView.backgroundColor = [UIColor clearColor];
    _textView.editable = NO;
    _textView.selectable = YES;
    _textView.scrollEnabled =
        NO; // Disable text view scrolling, let scroll view handle it
    _textView.showsHorizontalScrollIndicator = NO;
    _textView.showsVerticalScrollIndicator = NO;
    _textView.textContainer.lineBreakMode = NSLineBreakByCharWrapping;

    // Prevent text wrapping
    _textView.textContainer.widthTracksTextView = NO;
    _textView.textContainer.heightTracksTextView = NO;

    // Add padding
    _textView.textContainerInset = UIEdgeInsetsMake(12, 12, 12, 12);
    _textView.textContainer.lineFragmentPadding = 0;

    // Configure text view properties
    _textView.autocorrectionType = UITextAutocorrectionTypeNo;
    _textView.autocapitalizationType = UITextAutocapitalizationTypeNone;
    _textView.spellCheckingType = UITextSpellCheckingTypeNo;
    _textView.smartQuotesType = UITextSmartQuotesTypeNo;
    _textView.smartDashesType = UITextSmartDashesTypeNo;

    [_scrollView addSubview:_textView];
  }
  return self;
}

#pragma mark - Property Setters

- (void)setTokens:(NSArray *)tokens {
  NSLog(@"ShikiHighlighterView: setTokens called with %lu tokens",
        (unsigned long)tokens.count);
  if (_tokens != tokens) {
    _tokens = tokens;
    [self updateTokensDisplay];
  }
}

- (void)setText:(NSString *)text {
  NSLog(@"ShikiHighlighterView: setText called with length %lu",
        (unsigned long)text.length);
  if (_text != text) {
    _text = text;
    [self updateTokensDisplay];
  }
}

- (void)setFontSize:(double)fontSize {
  NSLog(@"ShikiHighlighterView: setFontSize called with %f", fontSize);
  if (_fontSize != fontSize) {
    _fontSize = fontSize;
    [self updateTokensDisplay];
  }
}

- (void)setFontFamily:(NSString *)fontFamily {
  NSLog(@"ShikiHighlighterView: setFontFamily called with %@", fontFamily);
  if (_fontFamily != fontFamily) {
    _fontFamily = fontFamily;
    [self updateTokensDisplay];
  }
}

- (void)setScrollEnabled:(BOOL)scrollEnabled {
  NSLog(@"ShikiHighlighterView: setScrollEnabled called with %d",
        scrollEnabled);
  if (_scrollEnabled != scrollEnabled) {
    _scrollEnabled = scrollEnabled;
    _scrollView.scrollEnabled = scrollEnabled;
  }
}

#pragma mark - Private Methods

- (UIColor *)colorFromHexString:(NSString *)hexString {
  if (![hexString hasPrefix:@"#"]) {
    return nil;
  }

  NSString *colorString = [hexString substringFromIndex:1];
  unsigned hexValue = 0;
  NSScanner *scanner = [NSScanner scannerWithString:colorString];
  [scanner scanHexInt:&hexValue];

  CGFloat r = ((hexValue & 0xFF0000) >> 16) / 255.0;
  CGFloat g = ((hexValue & 0x00FF00) >> 8) / 255.0;
  CGFloat b = (hexValue & 0x0000FF) / 255.0;

  return [UIColor colorWithRed:r green:g blue:b alpha:1.0];
}

- (void)updateTokensDisplay {
  NSLog(@"ShikiHighlighterView: updateTokensDisplay called");
  if (!_tokens || !_text) {
    return;
  }

  NSLog(@"ShikiHighlighterView: Have %lu tokens for text of length %lu",
        (unsigned long)_tokens.count, (unsigned long)_text.length);

  NSMutableAttributedString *attributedString =
      [[NSMutableAttributedString alloc] initWithString:_text];

  // Use the provided font family or fall back to system fonts
  UIFont *font = nil;

  if (_fontFamily && _fontFamily.length > 0) {
    NSLog(@"ShikiHighlighterView: Trying to use provided font family: %@",
          _fontFamily);
    font = [UIFont fontWithName:_fontFamily size:_fontSize];
  }

  // If the provided font family isn't available, fall back to system monospaced
  // font
  if (!font) {
    NSLog(@"ShikiHighlighterView: Falling back to system monospaced font");
    if (@available(iOS 13.0, *)) {
      font = [UIFont monospacedSystemFontOfSize:_fontSize
                                         weight:UIFontWeightRegular];
    } else {
      font = [UIFont systemFontOfSize:_fontSize];
    }
  }

  // Set paragraph style for line height
  NSMutableParagraphStyle *paragraphStyle =
      [[NSMutableParagraphStyle alloc] init];
  paragraphStyle.lineSpacing = 2.0;
  paragraphStyle.paragraphSpacing = 4.0;
  paragraphStyle.minimumLineHeight = _fontSize + 2.0;
  paragraphStyle.maximumLineHeight = _fontSize + 2.0;
  paragraphStyle.lineBreakMode = NSLineBreakByCharWrapping;

  // Default text attributes
  NSDictionary *defaultAttributes = @{
    NSFontAttributeName : font,
    NSForegroundColorAttributeName : [UIColor whiteColor],
    NSParagraphStyleAttributeName : paragraphStyle
  };
  [attributedString addAttributes:defaultAttributes
                            range:NSMakeRange(0, _text.length)];

  // Apply token styles
  NSLog(@"[VIEW DEBUG] Applying styles for %lu tokens to text of length %lu", (unsigned long)_tokens.count, (unsigned long)_text.length);
  for (NSDictionary *token in _tokens) {
    NSInteger start = [token[@"start"] integerValue];
    NSInteger length = [token[@"length"] integerValue];
    NSDictionary *style = token[@"style"];

    NSLog(@"[VIEW DEBUG] Token: start=%ld, length=%ld, style=%@", start, length, style);

    if (start + length <= _text.length) {
      NSRange range = NSMakeRange(start, length);
      NSMutableDictionary *attributes = [NSMutableDictionary dictionary];

      // Apply color
      if (style[@"color"]) {
        UIColor *color = [self colorFromHexString:style[@"color"]];
        if (color) {
          attributes[NSForegroundColorAttributeName] = color;
          NSLog(@"[VIEW DEBUG] Applied color %@ to range {%ld, %ld}", style[@"color"], range.location, range.length);
        }
      }

      // Apply font traits
      if ([style[@"bold"] boolValue]) {
        UIFont *boldFont = [UIFont
            fontWithDescriptor:
                [[font fontDescriptor]
                    fontDescriptorWithSymbolicTraits:UIFontDescriptorTraitBold]
                          size:_fontSize];
        if (boldFont) {
          attributes[NSFontAttributeName] = boldFont;
        }
      }

      if ([style[@"italic"] boolValue]) {
        UIFont *italicFont =
            [UIFont fontWithDescriptor:[[font fontDescriptor]
                                           fontDescriptorWithSymbolicTraits:
                                               UIFontDescriptorTraitItalic]
                                  size:_fontSize];
        if (italicFont) {
          attributes[NSFontAttributeName] = italicFont;
        }
      }

      if ([style[@"underline"] boolValue]) {
        attributes[NSUnderlineStyleAttributeName] = @(NSUnderlineStyleSingle);
      }

      if (attributes.count > 0) {
        [attributedString addAttributes:attributes range:range];
      }
    }
  }

  _textView.attributedText = attributedString;
  [self updateContentSize];
}

- (void)updateContentSize {
  // Get the text width by measuring the longest line
  NSArray *lines = [_textView.text componentsSeparatedByString:@"\n"];
  CGFloat maxWidth = 0;
  UIFont *font = _textView.font;

  for (NSString *line in lines) {
    CGFloat lineWidth =
        [line sizeWithAttributes:@{NSFontAttributeName : font}].width;
    maxWidth = MAX(maxWidth, lineWidth);
  }

  // Add padding
  maxWidth +=
      _textView.textContainerInset.left + _textView.textContainerInset.right;

  // Calculate the full content size
  CGSize textSize = [_textView sizeThatFits:CGSizeMake(maxWidth, CGFLOAT_MAX)];
  CGFloat contentHeight = textSize.height;

  // Set content size
  _scrollView.contentSize = CGSizeMake(maxWidth, contentHeight);

  // Update text view frame to match content size
  _textView.frame = CGRectMake(0, 0, maxWidth, contentHeight);

  NSLog(@"ShikiHighlighterView: Content size updated - width: %f, height: %f",
        maxWidth, contentHeight);
}

- (void)updateProps:(Props::Shared const &)props
           oldProps:(Props::Shared const &)oldProps {
  NSLog(@"ShikiHighlighterView: updateProps called");
  const auto &newProps =
      *std::static_pointer_cast<const ShikiHighlighterViewProps>(props);

  NSLog(@"ShikiHighlighterView: Received props - text length: %lu, tokens "
        @"count: %lu",
        newProps.text.length(), newProps.tokens.size());

  if (!newProps.tokens.empty()) {
    const auto &firstToken = newProps.tokens[0];
    NSLog(@"ShikiHighlighterView: First token - start: %d, length: %d, scopes: "
          @"%lu",
          firstToken.start, firstToken.length,
          (unsigned long)firstToken.scopes.size());
  }

  _scrollView.scrollEnabled = newProps.scrollEnabled;

  [super updateProps:props oldProps:oldProps];
}

- (void)layoutSubviews {
  [super layoutSubviews];
  NSLog(@"ShikiHighlighterView: layoutSubviews called");
  NSLog(@"ShikiHighlighterView: new layout - bounds: %@",
        NSStringFromCGRect(self.bounds));
  _scrollView.frame = self.bounds;
  [self updateContentSize];
}

@end

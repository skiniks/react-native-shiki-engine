#import "ShikiStyleManager.h"
#include "../../cpp/highlighter/cache/StyleCache.h"
#include "../../cpp/highlighter/error/HighlightError.h"
#include "../../cpp/highlighter/grammar/Grammar.h"
#include "../../cpp/highlighter/text/HighlightedText.h"
#include "../../cpp/highlighter/theme/Theme.h"
#include "../../cpp/highlighter/theme/ThemeColor.h"
#include "../../cpp/highlighter/tokenizer/ShikiTokenizer.h"
#include <string>

using namespace shiki;

NSString* const ShikiThemeDidChangeNotification = @"ShikiThemeDidChange";
NSString* const ShikiErrorDomain = @"com.shiki.highlighter";

@interface ShikiStyleManager ()
@end

@implementation ShikiStyleManager {
  NSMutableDictionary<NSString*, UIColor*>* _colorCache;
  NSUInteger _styleApplicationCount;
  NSTimeInterval _totalStyleApplicationTime;
  NSString* _currentThemeContent;
  NSString* _currentGrammarContent;
  BOOL _themeValid;
  BOOL _grammarValid;
  std::shared_ptr<shiki::Grammar> _currentGrammar;
  NSUInteger _maxTokenizationLength;
}

+ (instancetype)sharedInstance {
  static ShikiStyleManager* instance = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    instance = [[ShikiStyleManager alloc] init];
  });
  return instance;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    _colorCache = [NSMutableDictionary new];
    _styleApplicationCount = 0;
    _totalStyleApplicationTime = 0;
    _themeValid = NO;
    _grammarValid = NO;
    _maxTokenizationLength = 100000; // 100KB default

    self.defaultTextColor = [UIColor blackColor];
    self.defaultBackgroundColor = [UIColor whiteColor];
  }
  return self;
}

- (void)applyStyle:(const shiki::ThemeStyle&)style
    toAttributedString:(NSMutableAttributedString*)string
                 range:(NSRange)range {
  NSDate* start = [NSDate date];

  if (!string || range.location == NSNotFound)
    return;

  NSMutableDictionary* attributes = [NSMutableDictionary new];

  if (!style.color.empty()) {
    UIColor* textColor = [self colorFromHexString:@(style.color.c_str())];
    if (textColor) {
      attributes[NSForegroundColorAttributeName] = textColor;
    }
  }

  UIFont* currentFont = [string attribute:NSFontAttributeName
                                  atIndex:range.location
                           effectiveRange:nil];
  if (currentFont) {
    UIFont* styledFont = [self getFontWithStyle:style baseFont:currentFont];
    if (styledFont) {
      attributes[NSFontAttributeName] = styledFont;
    }
  }

  if (!style.backgroundColor.empty()) {
    UIColor* bgColor = [self colorFromHexString:@(style.backgroundColor.c_str())];
    if (bgColor) {
      attributes[NSBackgroundColorAttributeName] = bgColor;
    }
  }

  [string addAttributes:attributes range:range];

  NSTimeInterval duration = -[start timeIntervalSinceNow];
  _totalStyleApplicationTime += duration;
  _styleApplicationCount++;
}

- (NSTimeInterval)averageStyleApplicationTime {
  return _styleApplicationCount > 0 ? _totalStyleApplicationTime / _styleApplicationCount : 0;
}

- (void)clearMetrics {
  _styleApplicationCount = 0;
  _totalStyleApplicationTime = 0;
}

- (UIColor*)colorFromHexString:(NSString*)hexString {
  // Check color cache first
  UIColor* cachedColor = _colorCache[hexString];
  if (cachedColor) {
    return cachedColor;
  }

  // Parse and cache color
  if (hexString.length > 0) {
    unsigned int colorCode = 0;
    NSScanner* scanner = [NSScanner scannerWithString:hexString];
    if ([hexString hasPrefix:@"#"]) {
      [scanner setScanLocation:1];
    }
    if ([scanner scanHexInt:&colorCode]) {
      UIColor* color = [UIColor colorWithRed:((colorCode & 0xFF0000) >> 16) / 255.0
                                      green:((colorCode & 0x00FF00) >> 8) / 255.0
                                       blue:(colorCode & 0x0000FF) / 255.0
                                      alpha:1.0];

      // Cache the color
      if (_colorCache.count >= 1000) {
        [_colorCache removeAllObjects];
      }
      _colorCache[hexString] = color;
      return color;
    }
  }
  return nil;
}

- (void)clearStyleCache {
  [_colorCache removeAllObjects];
  [self clearMetrics];
}

- (UIFont*)getFontWithStyle:(const shiki::ThemeStyle&)style baseFont:(UIFont*)baseFont {
  UIFontDescriptor* descriptor = baseFont.fontDescriptor;
  NSMutableDictionary* traits = [NSMutableDictionary dictionaryWithDictionary:descriptor.fontAttributes];

  if (style.bold) {
    traits[UIFontDescriptorTraitsAttribute] = @{UIFontWeightTrait: @(UIFontWeightBold)};
  }

  if (style.italic) {
    traits[UIFontDescriptorTraitsAttribute] = @{UIFontSlantTrait: @(1.0)};
  }

  UIFontDescriptor* newDescriptor = [UIFontDescriptor fontDescriptorWithFontAttributes:traits];
  return [UIFont fontWithDescriptor:newDescriptor size:baseFont.pointSize];
}

// Helper method to convert HighlightError to NSError
- (void)setError:(NSError**)error withHighlightError:(const shiki::HighlightError&)highlightError {
  if (error) {
    NSString* message = @(highlightError.what());
    ShikiErrorCode code = static_cast<ShikiErrorCode>(highlightError.code());

    *error = [NSError errorWithDomain:ShikiErrorDomain
                                code:code
                            userInfo:@{NSLocalizedDescriptionKey : message}];
  }
}

- (BOOL)setThemeContent:(NSString*)content error:(NSError**)error {
  std::string cppContent = std::string([content UTF8String]);
  auto newTheme = shiki::Theme::fromJson(cppContent);
  if (!newTheme) {
    if (error) {
      *error = [NSError errorWithDomain:ShikiErrorDomain
                                  code:ShikiErrorCodeInvalidTheme
                              userInfo:@{NSLocalizedDescriptionKey : @"Failed to parse theme"}];
    }
    return NO;
  }

  self.currentTheme = newTheme;
  _currentThemeContent = content;
  _themeValid = YES;

  return YES;
}

- (BOOL)setGrammarContent:(NSString*)content error:(NSError**)error {
  if (!content) {
    if (error) {
      *error = [NSError errorWithDomain:ShikiErrorDomain
                                  code:ShikiErrorCodeInvalidInput
                              userInfo:@{NSLocalizedDescriptionKey : @"Grammar content is nil"}];
    }
    return NO;
  }

  try {
    std::string cppContent([content UTF8String]);

    // Use C++ grammar validation
    if (!Grammar::validateJson(cppContent)) {
      if (error) {
        *error = [NSError errorWithDomain:ShikiErrorDomain
                                     code:ShikiErrorCodeInvalidGrammar
                                 userInfo:@{NSLocalizedDescriptionKey : @"Invalid grammar format"}];
      }
      return NO;
    }

    // Parse and set the grammar
    auto grammar = Grammar::fromJson(cppContent);
    if (!grammar) {
      if (error) {
        *error =
            [NSError errorWithDomain:ShikiErrorDomain
                                code:ShikiErrorCodeInvalidGrammar
                            userInfo:@{NSLocalizedDescriptionKey : @"Failed to parse grammar"}];
      }
      return NO;
    }

    _currentGrammarContent = content;
    _grammarValid = YES;

    // Update tokenizer with new grammar
    ShikiTokenizer::getInstance().setGrammar(grammar);

    return YES;
  } catch (const HighlightError& e) {
    if (error) {
      [self setError:error withHighlightError:e];
    }
    return NO;
  } catch (const std::exception& e) {
    if (error) {
      *error = [NSError errorWithDomain:ShikiErrorDomain
                                  code:ShikiErrorCodeInvalidGrammar
                              userInfo:@{NSLocalizedDescriptionKey : @(e.what())}];
    }
    return NO;
  }
}

- (const Grammar*)getCurrentGrammar {
  return _currentGrammar.get();
}

- (nullable const ThemeStyle*)getCurrentTheme {
  if (!self.currentTheme) {
    return nullptr;
  }
  return self.currentTheme->findStyle(""); // Get default style
}

- (void)applyCurrentThemeToString:(NSMutableAttributedString*)string range:(NSRange)range {
  if (!string || range.location == NSNotFound) {
    return;
  }

  const ThemeStyle* theme = [self getCurrentTheme];
  if (!theme) {
    return;
  }

  NSDate* start = [NSDate date];
  [self applyStyle:*theme toAttributedString:string range:range];

  // Update metrics
  NSTimeInterval duration = -[start timeIntervalSinceNow];
  _totalStyleApplicationTime += duration;
  _styleApplicationCount++;
}

- (nullable UIColor*)currentForegroundColor {
  const ThemeStyle* theme = [self getCurrentTheme];
  if (!theme || theme->color.empty()) {
    return nil;
  }
  return [self colorFromHexString:@(theme->color.c_str())];
}

- (nullable UIColor*)currentBackgroundColor {
  const ThemeStyle* theme = [self getCurrentTheme];
  if (!theme || theme->backgroundColor.empty()) {
    return nil;
  }
  return [self colorFromHexString:@(theme->backgroundColor.c_str())];
}

- (void)clearCurrentTheme {
  self.currentTheme = nullptr;
  _currentThemeContent = nil;
  _themeValid = NO;

  // Clear related caches
  [self clearStyleCache];

  [[NSNotificationCenter defaultCenter] postNotificationName:ShikiThemeDidChangeNotification
                                                      object:self];
}

- (BOOL)highlightText:(NSString*)text
    toAttributedString:(NSMutableAttributedString*)attributedString
                error:(NSError**)error {
  // Check if we have valid grammar and theme
  if (!_grammarValid || !_currentGrammar) {
    if (error) {
      *error = [NSError errorWithDomain:ShikiErrorDomain
                                  code:ShikiErrorCodeInvalidGrammar
                              userInfo:@{NSLocalizedDescriptionKey : @"No valid grammar set"}];
    }
    return NO;
  }

  // Check text length
  if (text.length > _maxTokenizationLength) {
    if (error) {
      *error = [NSError errorWithDomain:ShikiErrorDomain
                                  code:ShikiErrorCodeInputTooLarge
                              userInfo:@{NSLocalizedDescriptionKey : @"Text too large to tokenize"}];
    }
    return NO;
  }

  try {
    std::string cppText = std::string(text.UTF8String);
    auto& tokenizer = ShikiTokenizer::getInstance();
    auto tokens = tokenizer.tokenize(cppText);

    for (const auto& token : tokens) {
      auto& styleCache = StyleCache::getInstance();
      auto cachedStyle = styleCache.getCachedStyle(token.getCombinedScope());
      ThemeStyle style;

      if (cachedStyle) {
        style = *cachedStyle;
      } else {
        style = tokenizer.resolveStyle(token.getCombinedScope());
        styleCache.cacheStyle(token.getCombinedScope(), style);
      }

      NSRange range = NSMakeRange(token.start, token.length);
      [self applyStyle:style toAttributedString:attributedString range:range];
    }

    return YES;
  } catch (const HighlightError& e) {
    if (error) {
      [self setError:error withHighlightError:e];
    }
    return NO;
  } catch (const std::exception& e) {
    if (error) {
      *error = [NSError errorWithDomain:ShikiErrorDomain
                                  code:ShikiErrorCodeNone
                              userInfo:@{NSLocalizedDescriptionKey : @(e.what())}];
    }
    return NO;
  }
}

- (void)setTheme:(std::shared_ptr<Theme>)theme {
  self.currentTheme = theme;
}

- (UIColor*)textColorForStyle:(const ThemeStyle&)style {
  if (!style.color.empty()) {
    UIColor* textColor = [self colorFromHexString:@(style.color.c_str())];
    return textColor ?: self.defaultTextColor;
  }
  return self.defaultTextColor;
}

- (UIColor*)backgroundColorForStyle:(const ThemeStyle&)style {
  if (!style.backgroundColor.empty()) {
    UIColor* bgColor = [self colorFromHexString:@(style.backgroundColor.c_str())];
    return bgColor ?: self.defaultBackgroundColor;
  }
  return self.defaultBackgroundColor;
}

- (NSDictionary*)attributesForStyle:(const ThemeStyle&)style {
  NSMutableDictionary* attributes = [NSMutableDictionary dictionary];

  // Set text color
  UIColor* textColor = [self textColorForStyle:style];
  if (textColor) {
    attributes[NSForegroundColorAttributeName] = textColor;
  }

  // Set background color
  UIColor* backgroundColor = [self backgroundColorForStyle:style];
  if (backgroundColor) {
    attributes[NSBackgroundColorAttributeName] = backgroundColor;
  }

  // Set font traits
  NSMutableDictionary* traits = [NSMutableDictionary dictionary];
  if (style.bold) {
    [traits setObject:@(YES) forKey:@(UIFontDescriptorTraitBold)];
  }
  if (style.italic) {
    [traits setObject:@(YES) forKey:@(UIFontDescriptorTraitItalic)];
  }

  if (traits.count > 0) {
    UIFontDescriptor* descriptor = [UIFont systemFontOfSize:12.0].fontDescriptor;
    descriptor =
        [descriptor fontDescriptorByAddingAttributes:@{UIFontDescriptorTraitsAttribute : traits}];
    attributes[NSFontAttributeName] = [UIFont fontWithDescriptor:descriptor size:12.0];
  }

  return attributes;
}

- (NSAttributedString*)styledAttributedStringFromText:(NSString*)text
                                             language:(NSString*)language
                                              options:(NSDictionary*)options {
  if (!text || !self.currentTheme) {
    return [[NSAttributedString alloc] initWithString:text ?: @""];
  }

  // Convert text to C++ string
  std::string cppText([text UTF8String]);

  // Tokenize the text
  std::vector<Token> tokens;
  try {
    tokens = ShikiTokenizer::getInstance().tokenize(cppText);
  } catch (const std::exception& e) {
    NSLog(@"Tokenization error: %s", e.what());
    return [[NSAttributedString alloc] initWithString:text];
  }

  // Create attributed string
  NSMutableAttributedString* attributedString =
      [[NSMutableAttributedString alloc] initWithString:text];

  // Apply styles to each token
  for (const auto& token : tokens) {
    if (token.start + token.length <= [text length]) {
      NSRange range = NSMakeRange(token.start, token.length);
      ThemeStyle style = self.currentTheme->resolveStyle(token.getCombinedScope());
      NSDictionary* attributes = [self attributesForStyle:style];
      [attributedString addAttributes:attributes range:range];
    }
  }

  return attributedString;
}

+ (instancetype)sharedManager {
  return [self sharedInstance];
}

@end

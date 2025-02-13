#import "ShikiStyleManager.h"
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
@property(nonatomic, strong) NSCache<NSString*, NSAttributedString*>* cache;
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
  BOOL _shouldCacheResults;
  NSCache<NSString*, NSArray<NSDictionary*>*>* _highlightCache;
}

static const NSUInteger kMaxColorCacheSize = 1000;

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
    _shouldCacheResults = NO;        // Disable caching for debugging
    _highlightCache = [[NSCache alloc] init];
    _highlightCache.countLimit = 0; // Disable cache

    _cache = [[NSCache alloc] init];
    _cache.countLimit = 0; // Disable cache

    self.defaultTextColor = [UIColor blackColor];
    self.defaultBackgroundColor = [UIColor whiteColor];

    NSLog(@"[ShikiDebug] Initialized ShikiStyleManager with caching disabled");
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
  NSLog(@"[ShikiDebug] colorFromHexString called with: %@", hexString);

  if (!hexString || ![hexString isKindOfClass:[NSString class]]) {
    NSLog(@"[ShikiDebug] Invalid hex string input");
    return nil;
  }

  // Clear color cache each time to ensure fresh colors
  [_colorCache removeAllObjects];

  ThemeColor themeColor = ThemeColor::fromHex(std::string([hexString UTF8String]));
  if (themeColor.getHexColor().empty()) {
    NSLog(@"[ShikiDebug] Invalid color conversion for hex: %@", hexString);
    return nil;
  }

  UIColor* color = themeColor.toUIColor();
  NSLog(@"[ShikiDebug] Converted %@ to UIColor: %@", hexString, color);
  return color;
}

- (void)clearStyleCache {
  [_colorCache removeAllObjects];
  [self clearMetrics];
}

- (UIFont*)getFontWithStyle:(const shiki::ThemeStyle&)style baseFont:(UIFont*)baseFont {
  if (!baseFont) {
    NSLog(@"[ShikiDebug] No base font provided");
    return nil;
  }

  NSLog(@"[ShikiDebug] Processing font style - Bold: %d, Italic: %d", style.bold, style.italic);
  UIFontDescriptor* descriptor = baseFont.fontDescriptor;
  UIFontDescriptorSymbolicTraits traits = descriptor.symbolicTraits;

  if (style.bold) {
    NSLog(@"[ShikiDebug] Adding bold trait");
    traits |= UIFontDescriptorTraitBold;
  }
  if (style.italic) {
    NSLog(@"[ShikiDebug] Adding italic trait");
    traits |= UIFontDescriptorTraitItalic;
  }

  UIFontDescriptor* newDescriptor = [descriptor fontDescriptorWithSymbolicTraits:traits];
  UIFont* newFont = [UIFont fontWithDescriptor:newDescriptor size:baseFont.pointSize];

  if (!newFont) {
    NSLog(@"[ShikiDebug] Failed to create font with traits - falling back to base font");
  } else {
    NSLog(@"[ShikiDebug] Successfully created styled font");
  }

  return newFont ?: baseFont;
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
      *error = [NSError errorWithDomain:ShikiErrorDomain
                                   code:static_cast<NSInteger>(e.code())
                               userInfo:@{NSLocalizedDescriptionKey : @(e.what())}];
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
                 range:(NSRange)range
              baseFont:(UIFont*)baseFont
                 error:(NSError**)error {

  if (!text || !attributedString || !baseFont) {
    if (error) {
      *error = [NSError errorWithDomain:ShikiErrorDomain
                                   code:ShikiErrorCodeInvalidInput
                               userInfo:@{NSLocalizedDescriptionKey : @"Invalid input parameters"}];
    }
    return NO;
  }

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
      *error =
          [NSError errorWithDomain:ShikiErrorDomain
                              code:ShikiErrorCodeInputTooLarge
                          userInfo:@{NSLocalizedDescriptionKey : @"Text too large to tokenize"}];
    }
    return NO;
  }

  // Check cache if enabled
  NSArray<NSDictionary*>* cachedTokens = nil;
  if (_shouldCacheResults) {
    cachedTokens = [_highlightCache objectForKey:text];
  }

  try {
    std::vector<Token> tokens;

    if (!cachedTokens) {
      // Tokenize the text
      std::string cppText([text UTF8String]);
      tokens = ShikiTokenizer::getInstance().tokenize(cppText);

      // Convert tokens to cached format if caching is enabled
      if (_shouldCacheResults) {
        NSMutableArray* tokenCache = [NSMutableArray new];
        for (const auto& token : tokens) {
          [tokenCache addObject:@{
            @"start" : @(token.start),
            @"length" : @(token.length),
            @"scope" : @(token.getCombinedScope().c_str())
          }];
        }
        [_highlightCache setObject:tokenCache forKey:text];
      }
    } else {
      // Convert cached tokens back to C++ format
      tokens.reserve(cachedTokens.count);
      for (NSDictionary* token in cachedTokens) {
        Token range([token[@"start"] unsignedIntegerValue],
                    [token[@"length"] unsignedIntegerValue]);
        range.addScope(std::string([token[@"scope"] UTF8String]));
        tokens.push_back(range);
      }
    }

    // Apply highlighting with token-specific styles
    for (const auto& token : tokens) {
      NSRange tokenRange = NSMakeRange(token.start, token.length);
      if (NSIntersectionRange(tokenRange, range).length > 0) {
        // Get theme style for specific token scope
        ThemeStyle tokenStyle;
        if (self.currentTheme) {
          // Use Theme's scope-based style resolution
          tokenStyle = ShikiTokenizer::getInstance().resolveStyle(token.getCombinedScope());
        } else {
          // Fallback to base theme if no current theme
          tokenStyle = *[self getCurrentTheme];
        }

        // Apply the resolved style
        [self applyStyle:tokenStyle toAttributedString:attributedString range:tokenRange];
      }
    }

    return YES;
  } catch (const HighlightError& e) {
    [self setError:error withHighlightError:e];
    return NO;
  } catch (const std::exception& e) {
    if (error) {
      *error = [NSError errorWithDomain:ShikiErrorDomain
                                   code:ShikiErrorCodeTokenizationFailed
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

- (BOOL)highlightText:(NSString*)text
    toAttributedString:(NSMutableAttributedString*)attributedString
              baseFont:(UIFont*)baseFont
                 error:(NSError**)error {
  return [self highlightText:text
          toAttributedString:attributedString
                       range:NSMakeRange(0, [text length])
                    baseFont:baseFont
                       error:error];
}

@end

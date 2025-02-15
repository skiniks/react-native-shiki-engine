#pragma once

#include "../../cpp/highlighter/error/HighlightError.h"
#include "../../cpp/highlighter/grammar/Grammar.h"
#include "../../cpp/highlighter/theme/Theme.h"
#include "../../cpp/highlighter/theme/ThemeStyle.h"
#include "../../cpp/highlighter/tokenizer/ShikiTokenizer.h"
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

NS_ASSUME_NONNULL_BEGIN

extern NSString *const ShikiThemeDidChangeNotification;
extern NSString *const ShikiErrorDomain;

// Error codes matching HighlightErrorCode
typedef NS_ENUM(NSInteger, ShikiErrorCode) {
  ShikiErrorCodeNone = 0,
  ShikiErrorCodeInvalidGrammar,
  ShikiErrorCodeInvalidTheme,
  ShikiErrorCodeRegexCompilationFailed,
  ShikiErrorCodeTokenizationFailed,
  ShikiErrorCodeInputTooLarge,
  ShikiErrorCodeResourceLoadFailed,
  ShikiErrorCodeOutOfMemory,
  ShikiErrorCodeInvalidInput
};

@interface ShikiStyleManager : NSObject

@property(nonatomic, readonly) BOOL themeValid;
@property(nonatomic, strong) UIColor *defaultTextColor;
@property(nonatomic, strong) UIColor *defaultBackgroundColor;
@property(nonatomic, assign) std::shared_ptr<shiki::Theme> currentTheme;

+ (instancetype)sharedManager;
+ (instancetype)sharedInstance;

- (BOOL)setThemeContent:(NSString *)content error:(NSError **)error;
- (NSAttributedString *)styledAttributedStringFromText:(NSString *)text
                                              language:(NSString *)language
                                               options:(NSDictionary *)options;

- (void)applyStyle:(const shiki::ThemeStyle &)style
    toAttributedString:(NSMutableAttributedString *)string
                 range:(NSRange)range;

- (void)clearStyleCache;
- (UIFont *)getFontWithStyle:(const shiki::ThemeStyle &)style
                    baseFont:(UIFont *)baseFont;

@property(nonatomic, assign, readonly)
    NSTimeInterval averageStyleApplicationTime;

- (void)clearMetrics;

@property(nonatomic, strong, readonly) NSString *currentThemeContent;
@property(nonatomic, strong, readonly) NSString *currentGrammarContent;

- (BOOL)setGrammarContent:(NSString *)content error:(NSError **)error;
- (nullable const shiki::ThemeStyle *)getCurrentTheme;
- (void)clearCurrentTheme;

- (void)applyCurrentThemeToString:(NSMutableAttributedString *)string
                            range:(NSRange)range;
- (nullable UIColor *)currentForegroundColor;
- (nullable UIColor *)currentBackgroundColor;

// Grammar-related methods
- (const shiki::Grammar *)getCurrentGrammar;

// Highlighting methods
- (BOOL)highlightText:(NSString *)text
    toAttributedString:(NSMutableAttributedString *)attributedString
              baseFont:(UIFont *)baseFont
                 error:(NSError **)error;

- (BOOL)highlightText:(NSString *)text
    toAttributedString:(NSMutableAttributedString *)attributedString
                 range:(NSRange)range
              baseFont:(UIFont *)baseFont
                 error:(NSError **)error;

// Optional configuration
@property(nonatomic, assign) NSUInteger maxTokenizationLength;
@property(nonatomic, assign) BOOL shouldCacheResults;

- (UIColor *)textColorForStyle:(const shiki::ThemeStyle &)style;
- (UIColor *)backgroundColorForStyle:(const shiki::ThemeStyle &)style;
- (NSDictionary *)attributesForStyle:(const shiki::ThemeStyle &)style;

@end

NS_ASSUME_NONNULL_END

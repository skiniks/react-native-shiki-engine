#import "ShikiHighlighter.h"
#import "ErrorManager.h"
#import <React/RCTBridge.h>
#import <React/RCTEventEmitter.h>
#import <React/RCTUIManager.h>
#import <React/RCTUtils.h>

#import <RNShikiSpec/RNShikiSpec.h>
#import <React/RCTConversions.h>
#import <React/RCTFabricComponentsPlugins.h>
#import <ReactCommon/CallInvoker.h>

using namespace facebook::react;

@interface RCTShikiHighlighterModule () <NativeShikiHighlighterSpec>
@end

#import "highlighter/theme/Theme.h"
#import "highlighter/theme/ThemeParser.h"
#import "highlighter/adapter/ShikiTextmateAdapter.h"

#include <memory>
#include <string>
#include <vector>

// Add adapter for shiki-textmate integration
static std::unique_ptr<shiki::ShikiTextmateAdapter> textmateAdapter;

@interface ShikiGrammarWrapper : NSObject
@property(nonatomic, assign) std::shared_ptr<shiki::Grammar> grammar;
- (instancetype)initWithGrammar:(std::shared_ptr<shiki::Grammar>)grammar;
@end

@implementation ShikiGrammarWrapper
- (instancetype)initWithGrammar:(std::shared_ptr<shiki::Grammar>)grammar {
  self = [super init];
  if (self) {
    _grammar = grammar;
  }
  return self;
}
@end

@interface ShikiThemeWrapper : NSObject
@property(nonatomic, assign) std::shared_ptr<shiki::Theme> theme;
- (instancetype)initWithTheme:(std::shared_ptr<shiki::Theme>)theme;
@end

@implementation ShikiThemeWrapper
- (instancetype)initWithTheme:(std::shared_ptr<shiki::Theme>)theme {
  self = [super init];
  if (self) {
    _theme = theme;
  }
  return self;
}
@end

static NSString *NSStringFromStdString(const std::string &str) {
  return [NSString stringWithUTF8String:str.c_str()];
}

@implementation RCTShikiHighlighterModule {
  __weak RCTBridge *_bridge;

  NSMutableDictionary<NSString *, ShikiGrammarWrapper *> *grammars_;
  NSMutableDictionary<NSString *, ShikiThemeWrapper *> *themes_;

  std::shared_ptr<shiki::Grammar> currentGrammar_;
  std::shared_ptr<shiki::Theme> currentTheme_;

  NSString *defaultLanguage_;
  NSString *defaultTheme_;

  dispatch_queue_t highlightQueue_;
}

@synthesize moduleRegistry = _moduleRegistry;
@synthesize viewRegistry_DEPRECATED = _viewRegistry_DEPRECATED;
@synthesize hasListeners = _hasListeners;

RCT_EXPORT_MODULE(ShikiHighlighter)

+ (BOOL)requiresMainQueueSetup {
  return NO;
}

- (instancetype)init {
  if (self = [super init]) {
    NSLog(@"[iOS DEBUG] ShikiHighlighter init called");

    // Initialize the textmate adapter
    if (!textmateAdapter) {
      NSLog(@"[iOS DEBUG] Creating new ShikiTextmateAdapter");
      textmateAdapter = std::make_unique<shiki::ShikiTextmateAdapter>();
      NSLog(@"[iOS DEBUG] ShikiTextmateAdapter created successfully");
    } else {
      NSLog(@"[iOS DEBUG] textmateAdapter already exists");
    }

    // Test if the adapter is actually working
    if (textmateAdapter) {
      NSLog(@"[iOS DEBUG] Testing adapter...");
      bool testResult = textmateAdapter->testMethod();
      NSLog(@"[iOS DEBUG] Adapter test result: %d", testResult);
    }

    highlightQueue_ =
        dispatch_queue_create("com.shiki.highlight", DISPATCH_QUEUE_SERIAL);

    grammars_ = [NSMutableDictionary dictionary];
    themes_ = [NSMutableDictionary dictionary];

    _hasListeners = NO;

    [self setupErrorHandling];
  }
  return self;
}

- (void)setupErrorHandling {
  shiki::ErrorManager::getInstance().setBridgeErrorCallback(
      [self](const std::string &errorJson) {
        if (self->_hasListeners) {
          [self sendEventWithName:@"ShikiError"
                             body:@{@"error" : @(errorJson.c_str())}];
        }
      });

  [self registerRecoveryStrategies];
}

- (void)registerRecoveryStrategies {
  auto &errorManager = shiki::ErrorManager::getInstance();

  errorManager.registerRecoveryStrategy(
      shiki::HighlightErrorCode::InvalidGrammar, []() { return true; });

  errorManager.registerRecoveryStrategy(shiki::HighlightErrorCode::InvalidTheme,
                                        []() { return true; });

  errorManager.registerRecoveryStrategy(shiki::HighlightErrorCode::OutOfMemory,
                                        []() { return true; });
}

- (std::shared_ptr<facebook::react::TurboModule>)getTurboModule:
    (const facebook::react::ObjCTurboModule::InitParams &)params {
  return std::make_shared<facebook::react::NativeShikiHighlighterSpecJSI>(
      params);
}

- (NSDictionary *)getConstants {
  return @{
    @"ErrorCodes" : @{
      @"InvalidGrammar" :
          @(static_cast<int>(shiki::HighlightErrorCode::InvalidGrammar)),
      @"InvalidTheme" :
          @(static_cast<int>(shiki::HighlightErrorCode::InvalidTheme)),
      @"OutOfMemory" :
          @(static_cast<int>(shiki::HighlightErrorCode::OutOfMemory))
    }
  };
}

- (NSArray<NSString *> *)supportedEvents {
  return @[ @"onHighlight", @"onError", @"onTelemetry" ];
}

- (void)startObserving {
  _hasListeners = YES;
}

- (void)stopObserving {
  _hasListeners = NO;
}

RCT_EXPORT_METHOD(addListener : (NSString *)eventName)
{
  // Required for RCTEventEmitter
}

RCT_EXPORT_METHOD(removeListeners : (double)count)
{
  // Required for RCTEventEmitter
}

RCT_EXPORT_METHOD(codeToTokens : (NSString *)code language : (NSString *)
                      language theme : (NSString *)
                          theme resolve : (RCTPromiseResolveBlock)
                              resolve reject : (RCTPromiseRejectBlock)reject)
{
  NSLog(@"[CRITICAL DEBUG] codeToTokens METHOD CALLED! code length: %lu", (unsigned long)(code ? code.length : 0));

  if (!code) {
    NSLog(@"[CRITICAL DEBUG] Code is nil, returning empty array");
    resolve(@[]);
    return;
  }

  __weak RCTShikiHighlighterModule *weakSelf = self;
  dispatch_async(self->highlightQueue_, ^{
    RCTShikiHighlighterModule *strongSelf = weakSelf;
    if (!strongSelf)
      return;

    NSLog(@"[iOS DEBUG] codeToTokens called with lang=%@ theme=%@ code length=%lu", language, theme, (unsigned long)code.length);
    @try {
      // Use specified language/theme or defaults
      NSString *lang = language ?: strongSelf->defaultLanguage_;
      NSString *thm = theme ?: strongSelf->defaultTheme_;
      NSLog(@"[iOS DEBUG] Using resolved lang=%@ theme=%@", lang, thm);

      // Check if language and theme are loaded
      NSLog(@"[iOS DEBUG] Checking grammar for lang='%@', available grammars: %@", lang, [strongSelf->grammars_ allKeys]);
      if (!lang || !strongSelf->grammars_[lang]) {
        NSLog(@"[iOS DEBUG] EARLY RETURN: Grammar check failed for lang='%@'", lang);
        dispatch_async(dispatch_get_main_queue(), ^{
          reject(@"invalid_language", @"Language not loaded", nil);
        });
        return;
      }

      NSLog(@"[iOS DEBUG] Checking theme for thm='%@', available themes: %@", thm, [strongSelf->themes_ allKeys]);
      if (!thm || !strongSelf->themes_[thm]) {
        NSLog(@"[iOS DEBUG] EARLY RETURN: Theme check failed for thm='%@'", thm);
        dispatch_async(dispatch_get_main_queue(), ^{
          reject(@"invalid_theme", @"Theme not loaded", nil);
        });
        return;
      }

      // Tokenize with the specified language and theme
      std::string codeStr = std::string([code UTF8String]);
      NSLog(@"[iOS DEBUG] About to tokenize code: %@ (length: %lu)", code, (unsigned long)code.length);
      NSLog(@"[iOS DEBUG] Using language: %@ theme: %@", lang, thm);
      NSLog(@"[DEBUG] Calling textmateAdapter->tokenizeWithStyles for code length: %lu", codeStr.length());
      NSLog(@"[CRITICAL DEBUG] textmateAdapter pointer: %p", textmateAdapter.get());

      std::vector<shiki::PlatformStyledToken> tokens;
      @try {
        NSLog(@"[CRITICAL DEBUG] About to call tokenizeWithStyles");
        tokens = textmateAdapter->tokenizeWithStyles(codeStr);
        NSLog(@"[CRITICAL DEBUG] tokenizeWithStyles returned %zu tokens", tokens.size());
      } @catch (NSException *e) {
        NSLog(@"[CRITICAL DEBUG] Exception in tokenizeWithStyles: %@", e.reason);
        throw;
      }
    NSLog(@"[DEBUG] Got %zu styled tokens from adapter", tokens.size());

    // Debug first few tokens to see their colors
    for (size_t i = 0; i < std::min(tokens.size(), (size_t)5); i++) {
        NSLog(@"[DEBUG] Token %zu: scope='%s', foreground='%s'",
              i, tokens[i].scope.c_str(), tokens[i].style.foreground.c_str());
    }
      NSLog(@"[iOS DEBUG] Generated %lu tokens", (unsigned long)tokens.size());

      // Convert to JS array
      NSMutableArray *result = [NSMutableArray array];
      int debugCount = 0;
      for (const auto &token : tokens) {
        NSMutableDictionary *tokenDict = [NSMutableDictionary dictionary];
        tokenDict[@"start"] = @(token.start);
        tokenDict[@"length"] = @(token.length);

        // Debug first few tokens
        if (debugCount < 5) {
          NSLog(@"[iOS DEBUG] Token %d: scope='%s', color='%s'", debugCount, token.scope.c_str(), token.style.foreground.c_str());
          debugCount++;
        }

        NSMutableArray *scopes = [NSMutableArray array];
        if (!token.scope.empty()) {
          [scopes addObject:NSStringFromStdString(token.scope)];
        }
        tokenDict[@"scopes"] = scopes;

        NSMutableDictionary *style = [NSMutableDictionary dictionary];
        style[@"color"] = token.style.foreground.empty()
                              ? [NSNull null]
                              : NSStringFromStdString(token.style.foreground);
        style[@"backgroundColor"] =
            token.style.background.empty()
                ? [NSNull null]
                : NSStringFromStdString(token.style.background);
        style[@"bold"] = @(token.style.bold);
        style[@"italic"] = @(token.style.italic);
        style[@"underline"] = @(token.style.underline);
        tokenDict[@"style"] = style;

        [result addObject:tokenDict];
      }

      dispatch_async(dispatch_get_main_queue(), ^{
        resolve(result);
      });
    } @catch (NSException *e) {
      dispatch_async(dispatch_get_main_queue(), ^{
        reject(@"tokenize_error", e.reason, nil);
      });
    }
  });
}

RCT_EXPORT_METHOD(loadLanguage : (NSString *)language grammarData : (NSString *)
                      grammarData resolve : (RCTPromiseResolveBlock)
                          resolve reject : (RCTPromiseRejectBlock)reject)
{
  if (!language || !grammarData) {
    reject(@"invalid_params", @"Language and grammar data are required", nil);
    return;
  }

  __weak RCTShikiHighlighterModule *weakSelf = self;
  dispatch_async(self->highlightQueue_, ^{
    RCTShikiHighlighterModule *strongSelf = weakSelf;
    if (!strongSelf)
      return;

    @try {
      std::string grammarStr = std::string([grammarData UTF8String]);

      // Load grammar using textmate adapter
      if (!textmateAdapter->loadGrammar([language UTF8String], grammarStr)) {
        dispatch_async(dispatch_get_main_queue(), ^{
          reject(@"grammar_load_error", @"Failed to load grammar with textmate adapter", nil);
        });
        return;
      }

      // Create a placeholder grammar wrapper for compatibility
      // Actual grammar is now managed by textmateAdapter
      // Note: Grammar now stored in textmateAdapter, creating minimal wrapper
      ShikiGrammarWrapper *grammarWrapper = [[ShikiGrammarWrapper alloc] init];
      strongSelf->grammars_[language] = grammarWrapper;

      // Set as default if it's the first one
      if (!strongSelf->defaultLanguage_) {
        strongSelf->defaultLanguage_ = language;
      }

      // Grammar reference removed - handled by textmateAdapter
      textmateAdapter->setGrammar([language UTF8String]);

      // Grammar now handled by textmate adapter

      dispatch_async(dispatch_get_main_queue(), ^{
        resolve(@(YES));
      });
    } @catch (NSException *e) {
      dispatch_async(dispatch_get_main_queue(), ^{
        reject(@"language_load_error", e.reason, nil);
      });
    }
  });
}

RCT_EXPORT_METHOD(loadTheme : (NSString *)theme themeData : (NSString *)
                      themeData resolve : (RCTPromiseResolveBlock)
                          resolve reject : (RCTPromiseRejectBlock)reject)
{
  if (!themeData) {
    reject(@"invalid_theme", @"Theme data is null", nil);
    return;
  }

  __weak RCTShikiHighlighterModule *weakSelf = self;
  dispatch_async(self->highlightQueue_, ^{
    RCTShikiHighlighterModule *strongSelf = weakSelf;
    if (!strongSelf)
      return;

    try {
      std::string themeStr = std::string([themeData UTF8String]);

      // Create the theme
      auto themeObj = std::make_shared<shiki::Theme>();
      shiki::ThemeParser parser(themeObj.get());
      parser.parseThemeStyle(themeStr);

      // Store in the map
      ShikiThemeWrapper *themeWrapper =
          [[ShikiThemeWrapper alloc] initWithTheme:themeObj];
      strongSelf->themes_[theme] = themeWrapper;

      // Set as default if it's the first one
      if (!strongSelf->defaultTheme_) {
        strongSelf->defaultTheme_ = theme;
      }

      strongSelf->currentTheme_ = themeObj;

      // Load theme into textmate adapter with both name and content
      textmateAdapter->loadTheme([theme UTF8String], themeStr);

      // Theme now handled by textmate adapter

      dispatch_async(dispatch_get_main_queue(), ^{
        resolve(@(true));
      });
    } catch (const std::exception &e) {
      dispatch_async(dispatch_get_main_queue(), ^{
        reject(@"theme_error", @(e.what()), nil);
      });
    }
  });
}

RCT_EXPORT_METHOD(setDefaultLanguage : (NSString *)language resolve : (
    RCTPromiseResolveBlock)resolve reject : (RCTPromiseRejectBlock)reject)
{
  if (!language) {
    reject(@"invalid_params", @"Language is required", nil);
    return;
  }

  __weak RCTShikiHighlighterModule *weakSelf = self;
  dispatch_async(self->highlightQueue_, ^{
    RCTShikiHighlighterModule *strongSelf = weakSelf;
    if (!strongSelf)
      return;

    @try {
      // Check if the language exists
      if (!strongSelf->grammars_[language]) {
        dispatch_async(dispatch_get_main_queue(), ^{
          reject(@"invalid_language", @"Language not loaded", nil);
        });
        return;
      }

      // Set as default
      strongSelf->defaultLanguage_ = language;

      // Grammar reference removed - handled by textmateAdapter
      textmateAdapter->setGrammar([language UTF8String]);

      // Default language now handled by textmate adapter

      dispatch_async(dispatch_get_main_queue(), ^{
        resolve(@(YES));
      });
    } @catch (NSException *e) {
      dispatch_async(dispatch_get_main_queue(), ^{
        reject(@"set_default_language_error", e.reason, nil);
      });
    }
  });
}

RCT_EXPORT_METHOD(setDefaultTheme : (NSString *)theme resolve : (
    RCTPromiseResolveBlock)resolve reject : (RCTPromiseRejectBlock)reject)
{
  if (!theme) {
    reject(@"invalid_params", @"Theme is required", nil);
    return;
  }

  __weak RCTShikiHighlighterModule *weakSelf = self;
  dispatch_async(self->highlightQueue_, ^{
    RCTShikiHighlighterModule *strongSelf = weakSelf;
    if (!strongSelf)
      return;

    @try {
      // Check if the theme exists
      if (!strongSelf->themes_[theme]) {
        dispatch_async(dispatch_get_main_queue(), ^{
          reject(@"invalid_theme", @"Theme not loaded", nil);
        });
        return;
      }

      // Set as default
      strongSelf->defaultTheme_ = theme;

      // Theme reference removed - handled by textmateAdapter
      textmateAdapter->setTheme([theme UTF8String]);

      // Default theme now handled by textmate adapter

      dispatch_async(dispatch_get_main_queue(), ^{
        resolve(@(YES));
      });
    } @catch (NSException *e) {
      dispatch_async(dispatch_get_main_queue(), ^{
        reject(@"set_default_theme_error", e.reason, nil);
      });
    }
  });
}

RCT_EXPORT_METHOD(getLoadedLanguages : (RCTPromiseResolveBlock)
                      resolve reject : (RCTPromiseRejectBlock)reject)
{
  __weak RCTShikiHighlighterModule *weakSelf = self;
  dispatch_async(self->highlightQueue_, ^{
    RCTShikiHighlighterModule *strongSelf = weakSelf;
    if (!strongSelf)
      return;

    @try {
      NSArray *languages = [strongSelf->grammars_ allKeys];

      dispatch_async(dispatch_get_main_queue(), ^{
        resolve(languages);
      });
    } @catch (NSException *e) {
      dispatch_async(dispatch_get_main_queue(), ^{
        reject(@"get_languages_error", e.reason, nil);
      });
    }
  });
}

RCT_EXPORT_METHOD(getLoadedThemes : (RCTPromiseResolveBlock)
                      resolve reject : (RCTPromiseRejectBlock)reject)
{
  __weak RCTShikiHighlighterModule *weakSelf = self;
  dispatch_async(self->highlightQueue_, ^{
    RCTShikiHighlighterModule *strongSelf = weakSelf;
    if (!strongSelf)
      return;

    @try {
      NSArray *themes = [strongSelf->themes_ allKeys];

      dispatch_async(dispatch_get_main_queue(), ^{
        resolve(themes);
      });
    } @catch (NSException *e) {
      dispatch_async(dispatch_get_main_queue(), ^{
        reject(@"get_themes_error", e.reason, nil);
      });
    }
  });
}

- (void)emitTelemetryEvent {
  if (!_hasListeners)
    return;

  // Cache metrics now handled by shikitori internally
  // Return placeholder metrics for compatibility

  NSDictionary *telemetryData = @{
    @"mainCache" : @{
      @"hitRate" : @(0.0),
      @"size" : @(0),
      @"memoryUsage" : @(0),
      @"evictions" : @(0)
    },
    @"patternCache" : @{
      @"hitRate" : @(0.0),
      @"size" : @(0),
      @"memoryUsage" : @(0),
      @"evictions" : @(0)
    },
    @"styleCache" : @{
      @"hitRate" : @(0.0),
      @"size" : @(0),
      @"memoryUsage" : @(0),
      @"evictions" : @(0)
    },
    @"syntaxTreeCache" : @{
      @"hitRate" : @(0.0),
      @"size" : @(0),
      @"memoryUsage" : @(0),
      @"evictions" : @(0)
    }
  };

  [self sendEventWithName:@"onTelemetry" body:telemetryData];
}

RCT_EXPORT_METHOD(getTelemetryData : (RCTPromiseResolveBlock)
                      resolve reject : (RCTPromiseRejectBlock)reject)
{
  dispatch_async(dispatch_get_main_queue(), ^{
    @try {
      [self emitTelemetryEvent];
      resolve(@(YES));
    } @catch (NSException *e) {
      reject(@"telemetry_error", e.reason, nil);
    }
  });
}

RCT_EXPORT_METHOD(enableCache : (BOOL)enabled resolve : (RCTPromiseResolveBlock)
                      resolve reject : (RCTPromiseRejectBlock)reject)
{
  dispatch_async(self->highlightQueue_, ^{
    @try {
      // Cache configuration now handled by shikitori internally
      // Note: Cache is always enabled in shikitori for optimal performance
      dispatch_async(dispatch_get_main_queue(), ^{
        resolve(@(YES));
      });
    } @catch (NSException *e) {
      dispatch_async(dispatch_get_main_queue(), ^{
        reject(@"cache_config_error", e.reason, nil);
      });
    }
  });
}

@end

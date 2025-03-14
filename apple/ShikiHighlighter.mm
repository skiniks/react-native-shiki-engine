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

#import "highlighter/cache/CacheManager.h"
#import "highlighter/core/Configuration.h"
#import "highlighter/grammar/Grammar.h"
#import "highlighter/theme/Theme.h"
#import "highlighter/theme/ThemeParser.h"
#import "highlighter/tokenizer/ShikiTokenizer.h"

@implementation RCTShikiHighlighterModule {
  __weak RCTBridge *_bridge;
  shiki::ShikiTokenizer *tokenizer_;

  // Maps to store multiple grammars and themes
  NSMutableDictionary<NSString *, std::shared_ptr<shiki::Grammar>> *grammars_;
  NSMutableDictionary<NSString *, std::shared_ptr<shiki::Theme>> *themes_;

  // Default language and theme
  NSString *defaultLanguage_;
  NSString *defaultTheme_;

  std::shared_ptr<shiki::Grammar> currentGrammar_;
  std::shared_ptr<shiki::Theme> currentTheme_;

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
    tokenizer_ = &shiki::ShikiTokenizer::getInstance();
    highlightQueue_ =
        dispatch_queue_create("com.shiki.highlight", DISPATCH_QUEUE_SERIAL);
    _hasListeners = NO;

    grammars_ = [NSMutableDictionary dictionary];
    themes_ = [NSMutableDictionary dictionary];

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
  if (!code) {
    resolve(@[]);
    return;
  }

  __weak RCTShikiHighlighterModule *weakSelf = self;
  dispatch_async(self->highlightQueue_, ^{
    RCTShikiHighlighterModule *strongSelf = weakSelf;
    if (!strongSelf)
      return;

    @try {
      // Use specified language/theme or defaults
      NSString *lang = language ?: strongSelf->defaultLanguage_;
      NSString *thm = theme ?: strongSelf->defaultTheme_;

      // Check if language and theme are loaded
      if (!lang || !strongSelf->grammars_[lang]) {
        dispatch_async(dispatch_get_main_queue(), ^{
          reject(@"invalid_language", @"Language not loaded", nil);
        });
        return;
      }

      if (!thm || !strongSelf->themes_[thm]) {
        dispatch_async(dispatch_get_main_queue(), ^{
          reject(@"invalid_theme", @"Theme not loaded", nil);
        });
        return;
      }

      // Tokenize with the specified language and theme
      std::string codeStr = std::string([code UTF8String]);
      std::vector<shiki::Token> tokens = strongSelf->tokenizer_->tokenize(
          codeStr, [lang UTF8String], [thm UTF8String]);

      // Convert to JS array
      NSMutableArray *result = [NSMutableArray array];
      for (const auto &token : tokens) {
        NSMutableDictionary *tokenDict = [NSMutableDictionary dictionary];
        tokenDict[@"start"] = @(token.start);
        tokenDict[@"length"] = @(token.length);

        NSMutableArray *scopes = [NSMutableArray array];
        for (const auto &scope : token.scopes) {
          [scopes addObject:@(scope.c_str())];
        }
        tokenDict[@"scopes"] = scopes;

        NSMutableDictionary *style = [NSMutableDictionary dictionary];
        style[@"color"] =
            token.style.color ? @(token.style.color) : [NSNull null];
        style[@"backgroundColor"] = token.style.backgroundColor
                                        ? @(token.style.backgroundColor)
                                        : [NSNull null];
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
      if (!shiki::Grammar::validateJson(grammarStr)) {
        dispatch_async(dispatch_get_main_queue(), ^{
          reject(@"invalid_grammar_json", @"Invalid grammar JSON format", nil);
        });
        return;
      }

      // Create the grammar
      auto grammar = shiki::Grammar::fromJson(grammarStr);
      if (!grammar) {
        dispatch_async(dispatch_get_main_queue(), ^{
          reject(@"grammar_parse_error", @"Failed to parse grammar", nil);
        });
        return;
      }

      // Store in the map
      strongSelf->grammars_[language] = grammar;

      // Set as default if it's the first one
      if (!strongSelf->defaultLanguage_) {
        strongSelf->defaultLanguage_ = language;
      }

      strongSelf->currentGrammar_ = grammar;
      strongSelf->tokenizer_->setGrammar(grammar);

      // Add to Configuration
      auto &config = shiki::Configuration::getInstance();
      config.addLanguage([language UTF8String], grammar);

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
      strongSelf->themes_[theme] = themeObj;

      // Set as default if it's the first one
      if (!strongSelf->defaultTheme_) {
        strongSelf->defaultTheme_ = theme;
      }

      strongSelf->currentTheme_ = themeObj;
      strongSelf->tokenizer_->setTheme(themeObj);

      // Add to Configuration
      auto &config = shiki::Configuration::getInstance();
      config.addTheme([theme UTF8String], themeObj);

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

      strongSelf->currentGrammar_ = strongSelf->grammars_[language];
      strongSelf->tokenizer_->setGrammar(strongSelf->currentGrammar_);

      // Update Configuration
      auto &config = shiki::Configuration::getInstance();
      config.setDefaultLanguage([language UTF8String]);

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

      strongSelf->currentTheme_ = strongSelf->themes_[theme];
      strongSelf->tokenizer_->setTheme(strongSelf->currentTheme_);

      // Update Configuration
      auto &config = shiki::Configuration::getInstance();
      config.setDefaultTheme([theme UTF8String]);

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

  auto &cacheManager = shiki::CacheManager::getInstance();
  auto mainMetrics = cacheManager.getCache().getMetrics();
  auto patternMetrics = cacheManager.getPatternCache().getMetrics();
  auto styleMetrics = cacheManager.getStyleCache().getMetrics();
  auto syntaxMetrics = cacheManager.getSyntaxTreeCache().getMetrics();

  NSDictionary *telemetryData = @{
    @"mainCache" : @{
      @"hitRate" : @(mainMetrics.hitRate()),
      @"size" : @(mainMetrics.size),
      @"memoryUsage" : @(mainMetrics.memoryUsage),
      @"evictions" : @(mainMetrics.evictions)
    },
    @"patternCache" : @{
      @"hitRate" : @(patternMetrics.hitRate()),
      @"size" : @(patternMetrics.size),
      @"memoryUsage" : @(patternMetrics.memoryUsage),
      @"evictions" : @(patternMetrics.evictions)
    },
    @"styleCache" : @{
      @"hitRate" : @(styleMetrics.hitRate()),
      @"size" : @(styleMetrics.size),
      @"memoryUsage" : @(styleMetrics.memoryUsage),
      @"evictions" : @(styleMetrics.evictions)
    },
    @"syntaxTreeCache" : @{
      @"hitRate" : @(syntaxMetrics.hitRate()),
      @"size" : @(syntaxMetrics.size),
      @"memoryUsage" : @(syntaxMetrics.memoryUsage),
      @"evictions" : @(syntaxMetrics.evictions)
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
      shiki::Configuration::getInstance().performance.enableCache = enabled;
      if (!enabled) {
        // Clear existing caches when disabling
        shiki::CacheManager::getInstance().clear();
      }
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

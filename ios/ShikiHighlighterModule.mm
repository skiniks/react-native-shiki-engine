#import "ShikiHighlighterModule.h"
#import "ErrorManager.h"
#import "RCTShikiHighlighterComponentView.h"
#import <React/RCTBridge.h>
#import <React/RCTEventEmitter.h>
#import <React/RCTUIManager.h>
#import <React/RCTUtils.h>

#if RCT_NEW_ARCH_ENABLED
#import <RNShikiHighlighter/RNShikiHighlighter.h>
#import <React/RCTConversions.h>
#import <React/RCTFabricComponentsPlugins.h>
#import <ReactCommon/CallInvoker.h>

using namespace facebook::react;

@interface RCTShikiHighlighterModule () <NativeShikiHighlighterSpec>
@end
#endif

#import "../../cpp/highlighter/cache/CacheManager.h"
#import "../../cpp/highlighter/grammar/Grammar.h"
#import "../../cpp/highlighter/theme/Theme.h"
#import "../../cpp/highlighter/theme/ThemeParser.h"
#import "../../cpp/highlighter/tokenizer/ShikiTokenizer.h"

@implementation RCTShikiHighlighterModule {
  __weak RCTBridge* _bridge;
  shiki::ShikiTokenizer* tokenizer_;
  std::shared_ptr<shiki::Grammar> currentGrammar_;
  std::shared_ptr<shiki::Theme> currentTheme_;
  dispatch_queue_t highlightQueue_;
  BOOL hasListeners;
}

@synthesize moduleRegistry = _moduleRegistry;
@synthesize viewRegistry_DEPRECATED = _viewRegistry_DEPRECATED;

RCT_EXPORT_MODULE(RNShikiHighlighterModule)

+ (BOOL)requiresMainQueueSetup {
  return NO;
}

- (instancetype)init {
  if (self = [super init]) {
    tokenizer_ = &shiki::ShikiTokenizer::getInstance();
    highlightQueue_ = dispatch_queue_create("com.shiki.highlight", DISPATCH_QUEUE_SERIAL);
    hasListeners = NO;

    [self setupErrorHandling];
  }
  return self;
}

- (void)setupErrorHandling {
  shiki::ErrorManager::getInstance().setBridgeErrorCallback([self](const std::string& errorJson) {
    if (self->hasListeners) {
      [self sendEventWithName:@"ShikiError" body:@{@"error" : @(errorJson.c_str())}];
    }
  });

  [self registerRecoveryStrategies];
}

- (void)registerRecoveryStrategies {
  auto& errorManager = shiki::ErrorManager::getInstance();

  errorManager.registerRecoveryStrategy(shiki::HighlightErrorCode::InvalidGrammar,
                                        []() { return true; });

  errorManager.registerRecoveryStrategy(shiki::HighlightErrorCode::InvalidTheme,
                                        []() { return true; });

  errorManager.registerRecoveryStrategy(shiki::HighlightErrorCode::OutOfMemory,
                                        []() { return true; });
}

#if RCT_NEW_ARCH_ENABLED
- (std::shared_ptr<facebook::react::TurboModule>)getTurboModule:
    (const facebook::react::ObjCTurboModule::InitParams&)params {
  return std::make_shared<facebook::react::NativeShikiHighlighterSpecJSI>(params);
}
#endif

- (NSDictionary*)getConstants {
  return @{
    @"ErrorCodes" : @{
      @"InvalidGrammar" : @(static_cast<int>(shiki::HighlightErrorCode::InvalidGrammar)),
      @"InvalidTheme" : @(static_cast<int>(shiki::HighlightErrorCode::InvalidTheme)),
      @"OutOfMemory" : @(static_cast<int>(shiki::HighlightErrorCode::OutOfMemory))
    }
  };
}

- (NSArray<NSString*>*)supportedEvents {
  return @[ @"onHighlight", @"onError" ];
}

- (void)startObserving {
  hasListeners = YES;
}

- (void)stopObserving {
  hasListeners = NO;
}

RCT_EXPORT_METHOD(addListener : (NSString*)eventName) {
  // Required for RCTEventEmitter
}

RCT_EXPORT_METHOD(removeListeners : (double)count) {
  // Required for RCTEventEmitter
}

RCT_EXPORT_METHOD(highlightCode : (NSString*)code language : (NSString*)language theme : (NSString*)
                      theme resolve : (RCTPromiseResolveBlock)
                          resolve reject : (RCTPromiseRejectBlock)reject) {
  if (!code || !language || !theme) {
    reject(@"invalid_params", @"Code, language, and theme are required", nil);
    return;
  }

  __weak RCTShikiHighlighterModule* weakSelf = self;
  dispatch_async(self->highlightQueue_, ^{
    RCTShikiHighlighterModule* strongSelf = weakSelf;
    if (!strongSelf)
      return;

    @try {
      std::string codeStr = std::string([code UTF8String]);
      auto tokens = strongSelf->tokenizer_->tokenize(codeStr);

      NSLog(@"Generated %lu tokens", (unsigned long)tokens.size());

      NSMutableArray* result = [NSMutableArray arrayWithCapacity:tokens.size()];
      for (const auto& token : tokens) {
        NSMutableDictionary* style = [NSMutableDictionary dictionary];
        [style setObject:@(token.style.color.c_str()) forKey:@"color"];

        if (!token.style.backgroundColor.empty()) {
          [style setObject:@(token.style.backgroundColor.c_str()) forKey:@"backgroundColor"];
        }
        if (token.style.bold) {
          [style setObject:@(YES) forKey:@"bold"];
        }
        if (token.style.italic) {
          [style setObject:@(YES) forKey:@"italic"];
        }
        if (token.style.underline) {
          [style setObject:@(YES) forKey:@"underline"];
        }

        NSMutableDictionary* tokenDict = @{
          @"start" : @(token.start),
          @"length" : @(token.length),
          @"scope" : [NSString stringWithUTF8String:token.getCombinedScope().c_str()],
          @"style" : style
        }
                                             .mutableCopy;

        [result addObject:tokenDict];
      }

      dispatch_async(dispatch_get_main_queue(), ^{
        resolve(result);
      });
    } @catch (NSException* e) {
      dispatch_async(dispatch_get_main_queue(), ^{
        reject(@"highlight_error", e.reason, nil);
      });
    }
  });
}

RCT_EXPORT_METHOD(loadLanguage : (NSString*)language grammarData : (NSString*)
                      grammarData resolve : (RCTPromiseResolveBlock)
                          resolve reject : (RCTPromiseRejectBlock)reject) {
  if (!language || !grammarData) {
    reject(@"invalid_params", @"Language and grammar data are required", nil);
    return;
  }

  __weak RCTShikiHighlighterModule* weakSelf = self;
  dispatch_async(self->highlightQueue_, ^{
    RCTShikiHighlighterModule* strongSelf = weakSelf;
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

      strongSelf->currentGrammar_ = shiki::Grammar::fromJson(grammarStr);
      if (!strongSelf->currentGrammar_) {
        dispatch_async(dispatch_get_main_queue(), ^{
          reject(@"grammar_parse_error", @"Failed to parse grammar", nil);
        });
        return;
      }

      strongSelf->tokenizer_->setGrammar(strongSelf->currentGrammar_);
      dispatch_async(dispatch_get_main_queue(), ^{
        resolve(@(YES));
      });
    } @catch (NSException* e) {
      dispatch_async(dispatch_get_main_queue(), ^{
        reject(@"language_load_error", e.reason, nil);
      });
    }
  });
}

RCT_EXPORT_METHOD(loadTheme : (NSString*)theme themeData : (NSString*)themeData resolve : (
    RCTPromiseResolveBlock)resolve reject : (RCTPromiseRejectBlock)reject) {
  if (!themeData) {
    reject(@"invalid_theme", @"Theme data is null", nil);
    return;
  }

  __weak RCTShikiHighlighterModule* weakSelf = self;
  dispatch_async(self->highlightQueue_, ^{
    RCTShikiHighlighterModule* strongSelf = weakSelf;
    if (!strongSelf)
      return;

    try {
      std::string themeStr = std::string([themeData UTF8String]);
      strongSelf->currentTheme_ = std::make_shared<shiki::Theme>();
      shiki::ThemeParser parser(strongSelf->currentTheme_.get());
      parser.parseThemeStyle(themeStr);
      strongSelf->tokenizer_->setTheme(strongSelf->currentTheme_);

      NSLog(@"Theme loaded with %lu rules", (unsigned long)strongSelf->currentTheme_->rules.size());
      for (size_t i = 0; i < std::min(size_t(5), strongSelf->currentTheme_->rules.size()); i++) {
        const auto& rule = strongSelf->currentTheme_->rules[i];
        NSLog(@"Rule %lu: scope=%s, color=%s", (unsigned long)i, rule.scope.c_str(),
              rule.style.color.c_str());
      }

      dispatch_async(dispatch_get_main_queue(), ^{
        resolve(@(true));
      });
    } catch (const std::exception& e) {
      dispatch_async(dispatch_get_main_queue(), ^{
        reject(@"theme_error", @(e.what()), nil);
      });
    }
  });
}

@end

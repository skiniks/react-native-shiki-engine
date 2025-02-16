#import <RNShikiHighlighter/RNShikiHighlighter.h>
#import <React/RCTBridgeModule.h>
#import <React/RCTEventEmitter.h>

NS_ASSUME_NONNULL_BEGIN

@interface RCTShikiHighlighterModule : RCTEventEmitter <RCTBridgeModule>

@property(nonatomic, assign) BOOL hasListeners;

- (void)codeToTokens:(NSString *)code
            language:(NSString *)language
               theme:(NSString *)theme
             resolve:(RCTPromiseResolveBlock)resolve
              reject:(RCTPromiseRejectBlock)reject;

- (void)loadLanguage:(NSString *)language
         grammarData:(NSString *)grammarData
             resolve:(RCTPromiseResolveBlock)resolve
              reject:(RCTPromiseRejectBlock)reject;

- (void)loadTheme:(NSString *)theme
        themeData:(NSString *)themeData
          resolve:(RCTPromiseResolveBlock)resolve
           reject:(RCTPromiseRejectBlock)reject;

- (void)enableCache:(BOOL)enabled
            resolve:(RCTPromiseResolveBlock)resolve
             reject:(RCTPromiseRejectBlock)reject;

@end

NS_ASSUME_NONNULL_END

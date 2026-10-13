#import "NativeShikiEngineModule.h"

#include "onig_regex.h"

#import <React/RCTBridgeModule.h>
#import <ReactCommon/RCTTurboModule.h>
#import <UIKit/UIKit.h>

#if __has_include(<ReactCommon/CxxTurboModuleUtils.h>)
#import <ReactCommon/CxxTurboModuleUtils.h>

@interface NativeShikiEngineCxxLoader : NSObject
@end

@implementation NativeShikiEngineCxxLoader

+ (void)load {
  facebook::react::registerCxxModuleToGlobalModuleMap(
      std::string(facebook::react::NativeShikiEngineModule::kModuleName),
      [](std::shared_ptr<facebook::react::CallInvoker> jsInvoker) {
        return std::make_shared<facebook::react::NativeShikiEngineModule>(
            jsInvoker);
      });
}

@end

#endif

/**
 * Bridgeless RN (0.86+ / Expo 57) resolves modules via RCT_EXPORT_MODULE, not
 * the C++ global map. Return the existing NativeShikiEngineModule unchanged.
 */
@interface ShikiEngine : NSObject <RCTBridgeModule, RCTTurboModule>
@end

@implementation ShikiEngine

RCT_EXPORT_MODULE(ShikiEngine)

+ (void)initialize {
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    [[NSNotificationCenter defaultCenter]
        addObserverForName:UIApplicationDidReceiveMemoryWarningNotification
                    object:nil
                     queue:[NSOperationQueue mainQueue]
                usingBlock:^(__unused NSNotification *note) {
                  clear_unused_pattern_cache();
                }];
  });
}

- (std::shared_ptr<facebook::react::TurboModule>)getTurboModule:
    (const facebook::react::ObjCTurboModule::InitParams &)params {
  return std::make_shared<facebook::react::NativeShikiEngineModule>(
      params.jsInvoker);
}

@end

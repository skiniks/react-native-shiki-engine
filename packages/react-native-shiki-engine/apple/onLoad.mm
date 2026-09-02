#import "NativeShikiEngineModule.h"

#include "onig_regex.h"

#if __has_include(<ReactCommon/CxxTurboModuleUtils.h>)
#import <ReactCommon/CxxTurboModuleUtils.h>
#if !TARGET_OS_OSX
#import <UIKit/UIKit.h>
#else
#import <AppKit/AppKit.h>
#endif

@interface OnLoad : NSObject
@end

@implementation OnLoad

+ (void)load {
  facebook::react::registerCxxModuleToGlobalModuleMap(
      std::string(facebook::react::NativeShikiEngineModule::kModuleName),
      [](std::shared_ptr<facebook::react::CallInvoker> jsInvoker) {
        return std::make_shared<facebook::react::NativeShikiEngineModule>(
            jsInvoker);
      });
	#if !TARGET_OS_OSX
  [[NSNotificationCenter defaultCenter]
      addObserverForName:UIApplicationDidReceiveMemoryWarningNotification
                  object:nil
                   queue:[NSOperationQueue mainQueue]
              usingBlock:^(__unused NSNotification *note) {
                clear_unused_pattern_cache();
              }];
	#endif
}

@end

#else
#import <React/RCTBridgeModule.h>
#import <ReactCommon/RCTTurboModule.h>
#if !TARGET_OS_OSX
#import <UIKit/UIKit.h>
#else
#import <AppKit/AppKit.h>
#endif

@interface ShikiEngine : NSObject <RCTBridgeModule, RCTTurboModule>
@end

@implementation ShikiEngine

RCT_EXPORT_MODULE(ShikiEngine)

+ (void)initialize {
	#if !TARGET_OS_OSX
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
  #endif
}

- (std::shared_ptr<facebook::react::TurboModule>)getTurboModule:
    (const facebook::react::ObjCTurboModule::InitParams &)params {
  return std::make_shared<facebook::react::NativeShikiEngineModule>(
      params.jsInvoker);
}

@end

#endif

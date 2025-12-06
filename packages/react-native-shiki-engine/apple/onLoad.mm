#import "NativeShikiEngineModule.h"
#import <Foundation/Foundation.h>
#import <ReactCommon/CxxTurboModuleUtils.h>

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
}

@end

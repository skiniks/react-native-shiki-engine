/**
 * ObjC registration shim for the pure-C++ TurboModule.
 *
 * onLoad.mm registers NativeShikiEngineModule into the global exported Cxx
 * module map (registerCxxModuleToGlobalModuleMap). That map is only read by
 * -[RCTTurboModuleManager provideTurboModule:runtime:], which the bridgeless
 * JS binding (global.__turboModuleProxy -> TurboModuleBinding provider chain)
 * never reaches on RN 0.86 / Expo SDK 57: verified with lldb against an Expo
 * 57 app — the registration breakpoint fires with name="ShikiEngine" at
 * process start, while every RCTTurboModuleManager entry point
 * (provideTurboModule:runtime:, moduleForName:, _provideObjCModule:) stays at
 * zero hits across a full JS boot that ends in
 * "TurboModuleRegistry.getEnforcing(...): 'ShikiEngine' could not be found".
 *
 * Modules that expose an RCT_EXPORT_MODULE class resolve fine in the same
 * app (every community pod does), so this shim makes ShikiEngine
 * discoverable through that conventional path and returns the existing
 * pure-C++ implementation unchanged. The ObjC protocol methods from the
 * generated spec are never invoked because getTurboModule: hands the
 * resolver the C++ module directly; JSI calls then go straight to
 * NativeShikiEngineModule.
 */
#import <Foundation/Foundation.h>

#import <React/RCTBridgeModule.h>
#import <ReactCommon/TurboModule.h>

#include "../cpp/NativeShikiEngineModule.h"

#if __has_include(<ReactCodegen/NativeShikiEngineSpec.h>)
#import <ReactCodegen/NativeShikiEngineSpec.h>
#elif __has_include(<NativeShikiEngineSpec/NativeShikiEngineSpec.h>)
#import <NativeShikiEngineSpec/NativeShikiEngineSpec.h>
#elif __has_include("NativeShikiEngineSpec.h")
#import "NativeShikiEngineSpec.h"
#else
#error "Could not find NativeShikiEngineSpec.h - ensure codegen has run and the podspec's header search paths are applied"
#endif

@interface ShikiEngine : NativeShikiEngineSpecBase <RCTBridgeModule>
@end

@implementation ShikiEngine

RCT_EXPORT_MODULE(ShikiEngine)

- (std::shared_ptr<facebook::react::TurboModule>)getTurboModule:
    (const facebook::react::ObjCTurboModule::InitParams &)params
{
  return std::make_shared<facebook::react::NativeShikiEngineModule>(
      params.jsInvoker);
}

@end

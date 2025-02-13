#import <React/RCTBridgeModule.h>

#if RCT_NEW_ARCH_ENABLED
#import <RNShikiEngine/RNShikiEngine.h>
#endif

@interface RCTShikiEngineModule : NSObject <RCTBridgeModule>
@end

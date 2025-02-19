#import <React/RCTLog.h>
#import <React/RCTUIManager.h>
#import <React/RCTViewManager.h>
#import "ShikiHighlighterView.h"
#import <React/RCTFabricComponentsPlugins.h>
#import <ReactCommon/RCTTurboModule.h>
#import <react/renderer/components/RNShikiSpec/ComponentDescriptors.h>

@interface ShikiHighlighterViewManager : RCTViewManager
@end

@implementation ShikiHighlighterViewManager

RCT_EXPORT_MODULE(ShikiHighlighterView)

RCT_EXPORT_VIEW_PROPERTY(tokens, NSArray)
RCT_EXPORT_VIEW_PROPERTY(text, NSString)
RCT_EXPORT_VIEW_PROPERTY(fontSize, double)
RCT_EXPORT_VIEW_PROPERTY(fontFamily, NSString)
RCT_EXPORT_VIEW_PROPERTY(scrollEnabled, BOOL)

- (UIView *)view
{
  return [[ShikiHighlighterView alloc] init];
}

+ (BOOL)requiresMainQueueSetup
{
  return NO;
}

@end

Class<RCTComponentViewProtocol> ShikiHighlighterViewCls(void)
{
  return ShikiHighlighterView.class;
}

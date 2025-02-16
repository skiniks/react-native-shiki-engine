#import "ShikiClipboard.h"
#import <UIKit/UIKit.h>
#import <mach/mach.h>

#import <RNShikiSpec/RNShikiSpec.h>

using namespace facebook;

@implementation RCTShikiClipboardModule

RCT_EXPORT_MODULE(ShikiClipboard)

- (std::shared_ptr<facebook::react::TurboModule>)getTurboModule:
    (const facebook::react::ObjCTurboModule::InitParams &)params {
  return std::make_shared<facebook::react::NativeShikiClipboardSpecJSI>(params);
}

- (void)setString:(NSString *)text
          resolve:(RCTPromiseResolveBlock)resolve
           reject:(RCTPromiseRejectBlock)reject {
  @autoreleasepool {
    @try {
      [[UIPasteboard generalPasteboard] setString:text];
      resolve(nil);
    } @catch (NSException *exception) {
      reject(@"clipboard_error", @"Failed to set clipboard content", nil);
    }
  }
}

- (void)getString:(RCTPromiseResolveBlock)resolve
           reject:(RCTPromiseRejectBlock)reject {
  @autoreleasepool {
    @try {
      NSString *content = [[UIPasteboard generalPasteboard] string];
      resolve(content ?: @"");
    } @catch (NSException *exception) {
      reject(@"clipboard_error", @"Failed to get clipboard content", nil);
    }
  }
}

+ (BOOL)requiresMainQueueSetup {
  return NO;
}

@end

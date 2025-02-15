#import "ShikiClipboard.h"
#import <UIKit/UIKit.h>

@implementation ShikiClipboard

RCT_EXPORT_MODULE()

RCT_EXPORT_METHOD(setString : (NSString *)text resolver : (
    RCTPromiseResolveBlock)resolve rejecter : (RCTPromiseRejectBlock)reject)
{
  @try {
    [[UIPasteboard generalPasteboard] setString:text];
    resolve(nil);
  } @catch (NSException *exception) {
    reject(@"clipboard_error", @"Failed to set clipboard content", nil);
  }
}

RCT_EXPORT_METHOD(getString : (RCTPromiseResolveBlock)
                      resolve rejecter : (RCTPromiseRejectBlock)reject)
{
  @try {
    NSString *content = [[UIPasteboard generalPasteboard] string];
    resolve(content ?: @"");
  } @catch (NSException *exception) {
    reject(@"clipboard_error", @"Failed to get clipboard content", nil);
  }
}

+ (BOOL)requiresMainQueueSetup {
  return NO;
}

@end

#import "ShikiEngineModule.h"
#import "../cpp/engine/OnigurumaRegex.h"
#import <RNShikiEngine/RNShikiEngine.h>
#import <React/RCTBridge.h>

#if RCT_NEW_ARCH_ENABLED
using namespace facebook::react;
#endif

@implementation RCTShikiEngineModule {
  std::unordered_map<double, OnigurumaContext *> scanners_;
  double nextScannerId_;
}

RCT_EXPORT_MODULE(RNShikiEngine)

- (instancetype)init {
  if (self = [super init]) {
    nextScannerId_ = 1;
  }
  return self;
}

- (void)dealloc {
  for (const auto &pair : scanners_) {
    free_scanner(pair.second);
  }
  scanners_.clear();
}

+ (BOOL)requiresMainQueueSetup {
  return NO;
}

#if RCT_NEW_ARCH_ENABLED
- (std::shared_ptr<facebook::react::TurboModule>)getTurboModule:
    (const facebook::react::ObjCTurboModule::InitParams &)params {
  return std::make_shared<facebook::react::NativeShikiEngineSpecJSI>(params);
}
#endif

RCT_EXPORT_METHOD(createScanner : (NSArray *)patterns maxCacheSize : (double)
                      maxCacheSize resolver : (RCTPromiseResolveBlock)
                          resolve rejecter : (RCTPromiseRejectBlock)reject)
{
  @try {
    std::vector<std::string> patternStrings;
    std::vector<const char *> patternPtrs;

    patternStrings.reserve([patterns count]);
    patternPtrs.reserve([patterns count]);

    for (NSString *pattern in patterns) {
      patternStrings.push_back(std::string([pattern UTF8String]));
      patternPtrs.push_back(patternStrings.back().c_str());
    }

    OnigurumaContext *context =
        create_scanner(patternPtrs.data(), static_cast<int>(patternPtrs.size()),
                       static_cast<size_t>(maxCacheSize));

    if (!context) {
      reject(@"create_scanner_error", @"Failed to create scanner", nil);
      return;
    }

    double scannerId = nextScannerId_++;
    scanners_[scannerId] = context;
    resolve(@(scannerId));
  } @catch (NSException *e) {
    reject(@"create_scanner_error", e.reason, nil);
  }
}

RCT_EXPORT_METHOD(findNextMatchSync : (double)scannerId text : (NSString *)
                      text startPosition : (double)
                          startPosition resolver : (RCTPromiseResolveBlock)
                              resolve rejecter : (RCTPromiseRejectBlock)reject)
{
  @try {
    auto it = scanners_.find(scannerId);
    if (it == scanners_.end()) {
      reject(@"invalid_scanner", @"Invalid scanner ID", nil);
      return;
    }

    std::string textStr = std::string([text UTF8String]);
    OnigurumaResult *result = find_next_match(it->second, textStr.c_str(),
                                              static_cast<int>(startPosition));

    if (!result) {
      resolve(nil);
      return;
    }

    NSMutableArray *captureIndices =
        [NSMutableArray arrayWithCapacity:result->capture_count];
    for (int i = 0; i < result->capture_count; i++) {
      int start = result->capture_indices[i * 2];
      int end = result->capture_indices[i * 2 + 1];

      [captureIndices addObject:@{
        @"start" : @(start),
        @"end" : @(end),
        @"length" : @(end - start)
      }];
    }

    NSDictionary *match = @{
      @"index" : @(result->pattern_index),
      @"captureIndices" : captureIndices
    };

    free_result(result);
    resolve(match);
  } @catch (NSException *e) {
    reject(@"find_match_error", e.reason, nil);
  }
}

RCT_EXPORT_METHOD(destroyScanner : (double)scannerId resolver : (
    RCTPromiseResolveBlock)resolve rejecter : (RCTPromiseRejectBlock)reject)
{
  @try {
    auto it = scanners_.find(scannerId);
    if (it != scanners_.end()) {
      free_scanner(it->second);
      scanners_.erase(it);
    }
    resolve(nil);
  } @catch (NSException *e) {
    reject(@"destroy_scanner_error", e.reason, nil);
  }
}

@end

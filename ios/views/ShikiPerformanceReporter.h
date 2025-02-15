#import "ShikiPerformanceMonitor.h"
#import <Foundation/Foundation.h>
#import <React/RCTBridge.h>
#import <React/RCTEventEmitter.h>

NS_ASSUME_NONNULL_BEGIN

@interface ShikiPerformanceReporter : RCTEventEmitter

@property(nonatomic, weak) RCTBridge *bridge;
@property(nonatomic, assign) BOOL autoReportEnabled;
@property(nonatomic, assign) NSTimeInterval reportingInterval;

+ (instancetype)sharedReporter;

- (void)startReporting;
- (void)stopReporting;
- (void)reportMetrics;
- (void)clearMetrics;

@end

NS_ASSUME_NONNULL_END

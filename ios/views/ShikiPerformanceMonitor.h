#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, ShikiPerformanceMetric) {
  ShikiPerformanceMetricStateTransition,
  ShikiPerformanceMetricUpdateProcessing,
  ShikiPerformanceMetricHighlighting,
  ShikiPerformanceMetricSelectionChange
};

@interface ShikiPerformanceEvent : NSObject
@property(nonatomic, assign) ShikiPerformanceMetric metric;
@property(nonatomic, assign) NSTimeInterval startTime;
@property(nonatomic, assign) NSTimeInterval duration;
@property(nonatomic, copy) NSString *description;
@property(nonatomic, strong) NSDictionary *metadata;
@end

@interface ShikiPerformanceMonitor : NSObject

+ (instancetype)sharedMonitor;

- (void)startMeasuring:(ShikiPerformanceMetric)metric
           description:(NSString *)description;
- (void)stopMeasuring:(ShikiPerformanceMetric)metric;
- (void)recordEvent:(ShikiPerformanceMetric)metric
           duration:(NSTimeInterval)duration;
- (void)addMetadata:(NSDictionary *)metadata
          forMetric:(ShikiPerformanceMetric)metric;

- (NSArray<ShikiPerformanceEvent *> *)getEventsForMetric:
    (ShikiPerformanceMetric)metric;
- (void)clearEvents;

@end

NS_ASSUME_NONNULL_END

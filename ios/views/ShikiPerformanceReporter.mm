#import "ShikiPerformanceReporter.h"
#import <React/RCTBridge+Private.h>
#import <React/RCTPerformanceLogger.h>

@implementation ShikiPerformanceReporter {
  dispatch_source_t _reportingTimer;
  dispatch_queue_t _reporterQueue;
  BOOL _hasListeners;
}

RCT_EXPORT_MODULE()

+ (instancetype)sharedReporter {
  static ShikiPerformanceReporter *reporter = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    reporter = [[ShikiPerformanceReporter alloc] init];
  });
  return reporter;
}

+ (BOOL)requiresMainQueueSetup {
  return NO;
}

- (NSArray<NSString *> *)supportedEvents {
  return @[ @"ShikiPerformanceReport" ];
}

- (void)startObserving {
  _hasListeners = YES;
}

- (void)stopObserving {
  _hasListeners = NO;
}

- (instancetype)init {
  if (self = [super init]) {
    _reportingInterval = 5.0; // 5 seconds default
    _reporterQueue = dispatch_queue_create("com.shiki.performancereporter",
                                           DISPATCH_QUEUE_SERIAL);
  }
  return self;
}

- (void)startReporting {
  if (_reportingTimer)
    return;

  _reportingTimer =
      dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, _reporterQueue);

  uint64_t interval = (uint64_t)(_reportingInterval * NSEC_PER_SEC);
  dispatch_source_set_timer(_reportingTimer,
                            dispatch_time(DISPATCH_TIME_NOW, interval),
                            interval, NSEC_PER_SEC / 10);

  dispatch_source_set_event_handler(_reportingTimer, ^{
    [self reportMetrics];
  });

  dispatch_resume(_reportingTimer);
}

- (void)stopReporting {
  if (_reportingTimer) {
    dispatch_source_cancel(_reportingTimer);
    _reportingTimer = nil;
  }
}

- (void)reportMetrics {
  if (!_hasListeners)
    return;

  ShikiPerformanceMonitor *monitor = [ShikiPerformanceMonitor sharedMonitor];

  // Collect metrics for each type
  NSMutableDictionary *metrics = [NSMutableDictionary new];

  // State transitions
  NSArray *transitions =
      [monitor getEventsForMetric:ShikiPerformanceMetricStateTransition];
  if (transitions.count > 0) {
    NSMutableArray *transitionMetrics = [NSMutableArray new];
    for (ShikiPerformanceEvent *event in transitions) {
      [transitionMetrics addObject:@{
        @"description" : event.description ?: @"",
        @"duration" : @(event.duration * 1000), // Convert to ms
        @"metadata" : event.metadata ?: @{},
        @"timestamp" : @(event.startTime)
      }];
    }
    metrics[@"stateTransitions"] = transitionMetrics;
  }

  // Update processing
  NSArray *updates =
      [monitor getEventsForMetric:ShikiPerformanceMetricUpdateProcessing];
  if (updates.count > 0) {
    NSMutableArray *updateMetrics = [NSMutableArray new];
    for (ShikiPerformanceEvent *event in updates) {
      [updateMetrics addObject:@{
        @"description" : event.description ?: @"",
        @"duration" : @(event.duration * 1000),
        @"metadata" : event.metadata ?: @{},
        @"timestamp" : @(event.startTime)
      }];
    }
    metrics[@"updates"] = updateMetrics;
  }

  // Send to JS using RCTEventEmitter
  [self sendEventWithName:@"ShikiPerformanceReport" body:metrics];

  // Clear recorded events
  [monitor clearEvents];
}

- (void)clearMetrics {
  [[ShikiPerformanceMonitor sharedMonitor] clearEvents];
}

@end

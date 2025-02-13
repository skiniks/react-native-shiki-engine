#import "ShikiPerformanceMonitor.h"

@implementation ShikiPerformanceEvent
@end

@implementation ShikiPerformanceMonitor {
  NSMutableDictionary<NSNumber*, NSMutableArray<ShikiPerformanceEvent*>*>* _events;
  NSMutableDictionary<NSNumber*, NSDate*>* _startTimes;
  NSMutableDictionary<NSNumber*, NSDictionary*>* _pendingMetadata;
  dispatch_queue_t _monitorQueue;
}

+ (instancetype)sharedMonitor {
  static ShikiPerformanceMonitor* monitor = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    monitor = [[ShikiPerformanceMonitor alloc] init];
  });
  return monitor;
}

- (instancetype)init {
  if (self = [super init]) {
    _events = [NSMutableDictionary new];
    _startTimes = [NSMutableDictionary new];
    _pendingMetadata = [NSMutableDictionary new];
    _monitorQueue = dispatch_queue_create("com.shiki.performancemonitor", DISPATCH_QUEUE_SERIAL);
  }
  return self;
}

- (void)startMeasuring:(ShikiPerformanceMetric)metric description:(NSString*)description {
  dispatch_async(_monitorQueue, ^{
    _startTimes[@(metric)] = [NSDate date];

    ShikiPerformanceEvent* event = [[ShikiPerformanceEvent alloc] init];
    event.metric = metric;
    event.startTime = [_startTimes[@(metric)] timeIntervalSinceReferenceDate];
    event.description = description;

    if (!_events[@(metric)]) {
      _events[@(metric)] = [NSMutableArray new];
    }
    [_events[@(metric)] addObject:event];
  });
}

- (void)stopMeasuring:(ShikiPerformanceMetric)metric {
  dispatch_async(_monitorQueue, ^{
    NSDate* startTime = _startTimes[@(metric)];
    if (!startTime)
      return;

    NSTimeInterval duration = [[NSDate date] timeIntervalSinceDate:startTime];
    ShikiPerformanceEvent* event = [_events[@(metric)] lastObject];
    event.duration = duration;
    event.metadata = _pendingMetadata[@(metric)];

    [_startTimes removeObjectForKey:@(metric)];
    [_pendingMetadata removeObjectForKey:@(metric)];
  });
}

- (void)recordEvent:(ShikiPerformanceMetric)metric duration:(NSTimeInterval)duration {
  dispatch_async(_monitorQueue, ^{
    ShikiPerformanceEvent* event = [[ShikiPerformanceEvent alloc] init];
    event.metric = metric;
    event.duration = duration;
    event.startTime = [[NSDate date] timeIntervalSinceReferenceDate];
    event.metadata = _pendingMetadata[@(metric)];

    if (!_events[@(metric)]) {
      _events[@(metric)] = [NSMutableArray new];
    }
    [_events[@(metric)] addObject:event];

    [_pendingMetadata removeObjectForKey:@(metric)];
  });
}

- (void)addMetadata:(NSDictionary*)metadata forMetric:(ShikiPerformanceMetric)metric {
  dispatch_async(_monitorQueue, ^{
    _pendingMetadata[@(metric)] = metadata;
  });
}

- (NSArray<ShikiPerformanceEvent*>*)getEventsForMetric:(ShikiPerformanceMetric)metric {
  __block NSArray* result = nil;
  dispatch_sync(_monitorQueue, ^{
    result = [_events[@(metric)] copy];
  });
  return result;
}

- (void)clearEvents {
  dispatch_async(_monitorQueue, ^{
    [_events removeAllObjects];
    [_startTimes removeAllObjects];
    [_pendingMetadata removeAllObjects];
  });
}

@end

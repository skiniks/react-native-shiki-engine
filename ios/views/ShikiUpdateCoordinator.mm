#import "ShikiUpdateCoordinator.h"

@implementation ShikiUpdateOperation
@end

@implementation ShikiUpdateCoordinator {
  NSMutableArray<ShikiUpdateOperation*>* _pendingUpdates;
  BOOL _isBatchUpdating;
  NSUInteger _batchUpdateCount;
}

- (instancetype)initWithViewState:(ShikiViewState*)viewState {
  self = [super init];
  if (self) {
    _viewState = viewState;
    _pendingUpdates = [NSMutableArray new];
    _isBatchUpdating = NO;
    _batchUpdateCount = 0;
    _updateCoalescingInterval = 0.1; // 100ms default
  }
  return self;
}

- (void)beginBatchUpdates {
  _isBatchUpdating = YES;
  _batchUpdateCount++;
  [_viewState beginUpdate];
}

- (void)endBatchUpdates {
  if (_batchUpdateCount > 0) {
    _batchUpdateCount--;
    if (_batchUpdateCount == 0) {
      _isBatchUpdating = NO;
      [_viewState endUpdate];
      [self processUpdates];
    }
  }
}

- (void)cancelPendingUpdates {
  [self cancelAllUpdates];
  _isBatchUpdating = NO;
  _batchUpdateCount = 0;
  [_viewState cancelUpdate];
}

- (void)scheduleUpdate:(void (^)(void))updateBlock {
  if (!updateBlock)
    return;

  ShikiUpdateOperation* operation = [[ShikiUpdateOperation alloc] init];
  operation.updateBlock = updateBlock;
  operation.timestamp = CACurrentMediaTime();

  [_pendingUpdates addObject:operation];

  if (!_isBatchUpdating) {
    [self processUpdates];
  }
}

- (void)cancelAllUpdates {
  for (ShikiUpdateOperation* operation in _pendingUpdates) {
    operation.isCancelled = YES;
  }
  [_pendingUpdates removeAllObjects];
}

- (void)processUpdates {
  if (!_viewState.canPerformUpdates || _isBatchUpdating) {
    return;
  }

  NSArray<ShikiUpdateOperation*>* updates = [_pendingUpdates copy];
  [_pendingUpdates removeAllObjects];

  for (ShikiUpdateOperation* operation in updates) {
    if (operation.isCancelled)
      continue;

    @try {
      operation.updateBlock();
    } @catch (NSException* exception) {
      if (self.onUpdateComplete) {
        self.onUpdateComplete(NO);
      }
      return;
    }
  }

  if (self.onUpdateComplete) {
    self.onUpdateComplete(YES);
  }
}

- (void)pauseUpdates {
  [_viewState pauseUpdates];
}

- (void)resumeUpdates {
  [_viewState resumeUpdates];
  if (!_isBatchUpdating) {
    [self processUpdates];
  }
}

- (NSUInteger)pendingUpdateCount {
  return _pendingUpdates.count;
}

@end

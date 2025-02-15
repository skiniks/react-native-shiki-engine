#import "ShikiViewState.h"
#import "ShikiPerformanceMonitor.h"

@implementation ShikiViewState {
  NSMutableArray *_stateHistory;
  NSUInteger _retryAttempts;
  NSUInteger _maxRetryAttempts;
  BOOL _isPerformingBatchUpdates;
  BOOL _isBatchUpdateInProgress;
  NSMutableArray *_pendingUpdates;
  NSUInteger _pendingUpdateCount;
  void (^_onStateChange)(ShikiViewLifecycleState oldState,
                         ShikiViewLifecycleState newState);
  BOOL _isInTransition;
  ShikiViewLifecycleState _pendingState;
  BOOL _enableStatePersistence;
  NSError *_lastError;
  BOOL _isUpdating;
  BOOL _isBatchUpdating;
  BOOL _isTransitioning;
  BOOL _isPaused;
  NSMutableArray<NSDictionary *> *_history;
}

static NSString *const kStateStorageKey = @"com.shiki.viewstate";

- (instancetype)init {
  if (self = [super init]) {
    _lifecycleState = ShikiViewLifecycleStateInactive;
    _stateHistory = [NSMutableArray new];
    _retryAttempts = 0;
    _maxRetryAttempts = 3; // Default max retry attempts
    _isPerformingBatchUpdates = NO;
    _isBatchUpdateInProgress = NO;
    _pendingUpdates = [NSMutableArray new];
    _pendingUpdateCount = 0;
    _isUpdating = NO;
    _isBatchUpdating = NO;
    _isTransitioning = NO;
    _isPaused = NO;
    _history = [NSMutableArray new];
    _lastError = nil;
  }
  return self;
}

- (void)beginUpdate {
  _isUpdating = YES;
  _hasPendingUpdates = YES;
}

- (void)endUpdate {
  _isUpdating = NO;
  _hasPendingUpdates = NO;
}

- (void)cancelUpdate {
  _isUpdating = NO;
  _hasPendingUpdates = NO;
  if (_isBatchUpdating) {
    [self cancelBatchUpdate];
  }
}

- (void)pauseUpdates {
  _isPaused = YES;
}

- (void)resumeUpdates {
  _isPaused = NO;
}

- (NSDictionary *)currentState {
  return @{
    @"lifecycleState" : @(_lifecycleState),
    @"stateHistory" : _stateHistory ?: @[],
    @"isPerformingBatchUpdates" : @(_isPerformingBatchUpdates),
    @"hasPendingUpdates" : @(_pendingUpdates.count > 0),
    @"retryAttempts" : @(_retryAttempts)
  };
}

- (void)transitionToState:(ShikiViewLifecycleState)newState {
  if (_isInTransition) {
    NSLog(@"Warning: State transition already in progress");
    return;
  }

  ShikiViewLifecycleState oldState = _lifecycleState;

  // Start measuring transition
  [[ShikiPerformanceMonitor sharedMonitor]
      startMeasuring:ShikiPerformanceMetricStateTransition
         description:[NSString stringWithFormat:@"State transition: %ld -> %ld",
                                                (long)oldState,
                                                (long)newState]];

  // Add metadata
  [[ShikiPerformanceMonitor sharedMonitor]
      addMetadata:@{
        @"fromState" : @(oldState),
        @"toState" : @(newState),
        @"hasPendingUpdates" : @(_pendingUpdates.count > 0),
        @"isPerformingBatchUpdates" : @(_isPerformingBatchUpdates)
      }
        forMetric:ShikiPerformanceMetricStateTransition];

  // Validate and perform transition
  if (![self canTransitionFromState:oldState toState:newState]) {
    [[ShikiPerformanceMonitor sharedMonitor]
        stopMeasuring:ShikiPerformanceMetricStateTransition];
    NSLog(@"Invalid state transition from %ld to %ld", (long)oldState,
          (long)newState);
    [self handleInvalidTransition:oldState toState:newState];
    return;
  }

  [self beginStateTransition];
  _pendingState = newState;

  // Record state change
  _lifecycleState = newState;
  [_stateHistory addObject:@{
    @"from" : @(oldState),
    @"to" : @(newState),
    @"timestamp" : @(CACurrentMediaTime())
  }];

  // Trim history
  if (_stateHistory.count > 10) {
    [_stateHistory removeObjectAtIndex:0];
  }

  if (_onStateChange) {
    _onStateChange(oldState, newState);
  }

  [self completeStateTransition];

  // Stop measuring
  [[ShikiPerformanceMonitor sharedMonitor]
      stopMeasuring:ShikiPerformanceMetricStateTransition];
}

- (void)beginStateTransition {
  _isTransitioning = YES;
}

- (void)completeStateTransition {
  _isTransitioning = NO;
  _pendingState = _lifecycleState;
}

- (void)cancelStateTransition {
  _isTransitioning = NO;
}

- (void)handleInvalidTransition:(ShikiViewLifecycleState)fromState
                        toState:(ShikiViewLifecycleState)toState {
  // Try to recover from invalid transition
  switch (fromState) {
  case ShikiViewLifecycleStateUpdating:
    // Force complete any pending updates
    [self completeStateTransition];
    // Then retry transition
    [self transitionToState:toState];
    break;

  case ShikiViewLifecycleStateBackground:
    if (toState == ShikiViewLifecycleStateActive) {
      // Need to go through intermediate state
      [self transitionToState:ShikiViewLifecycleStateInactive];
      [self transitionToState:toState];
    }
    break;

  case ShikiViewLifecycleStateInactive:
    // Force reset if trying to transition from Inactive incorrectly
    [self reset];
    break;

  case ShikiViewLifecycleStateError:
    // Must go through reset
    [self reset];
    if (toState != ShikiViewLifecycleStateInactive) {
      [self transitionToState:ShikiViewLifecycleStateInactive];
      [self transitionToState:toState];
    }
    break;

  case ShikiViewLifecycleStateOffscreen:
    if (toState == ShikiViewLifecycleStateUpdating) {
      // Queue update for when view becomes visible
      if (!_pendingUpdates) {
        _pendingUpdates = [NSMutableArray new];
      }
      _pendingUpdateCount++;
    }
    break;

  default:
    [self recoverFromInvalidState];
    break;
  }
}

- (void)recoverFromInvalidState {
  // Reset to a known good state
  _lifecycleState = ShikiViewLifecycleStateInactive;
  _isInTransition = NO;
  _pendingState = _lifecycleState;
  [self reset];
}

- (BOOL)needsRecovery {
  return _isInTransition || _lifecycleState == ShikiViewLifecycleStateError ||
         (_stateHistory.count > 0 &&
          [[_stateHistory lastObject][@"to"] integerValue] != _lifecycleState);
}

- (BOOL)canTransitionFromState:(ShikiViewLifecycleState)fromState
                       toState:(ShikiViewLifecycleState)toState {
  // Already implemented transitions...

  // Add more validation rules
  switch (fromState) {
  case ShikiViewLifecycleStateInactive:
    // Can transition to any state except Error
    if (toState == ShikiViewLifecycleStateError) {
      // Add update to pending queue instead of direct transition
      if (!_pendingUpdates) {
        _pendingUpdates = [NSMutableArray new];
      }
      _pendingUpdateCount++;
      return NO;
    }
    return YES;

  case ShikiViewLifecycleStateActive:
    // Can transition to any state except Inactive from Active
    return toState != ShikiViewLifecycleStateInactive;

  case ShikiViewLifecycleStateBackground:
    // Must go through Inactive before becoming Active
    if (toState == ShikiViewLifecycleStateActive)
      return NO;
    return YES;

  case ShikiViewLifecycleStateOffscreen:
    // Can't transition directly to Updating or Error
    return (toState != ShikiViewLifecycleStateUpdating &&
            toState != ShikiViewLifecycleStateError);

  case ShikiViewLifecycleStateUpdating:
    // Can only complete or error out from Updating
    return (toState == ShikiViewLifecycleStateActive ||
            toState == ShikiViewLifecycleStateError);

  case ShikiViewLifecycleStateError:
    // Must reset to Inactive from Error
    return toState == ShikiViewLifecycleStateInactive;

  default:
    return NO;
  }
}

- (BOOL)canPerformUpdates {
  return !_isPaused && !_isUpdating &&
         self.lifecycleState == ShikiViewLifecycleStateActive;
}

- (BOOL)isActive {
  return _lifecycleState == ShikiViewLifecycleStateActive;
}

- (BOOL)canAcceptUpdates {
  return !_isPaused && self.lifecycleState == ShikiViewLifecycleStateActive;
}

- (void)handleError:(NSError *)error {
  _lastError = error;
  _retryAttempts = 0;
  _lifecycleState = ShikiViewLifecycleStateError;
  [_stateHistory addObject:@{
    @"from" : @(_lifecycleState),
    @"to" : @(ShikiViewLifecycleStateError),
    @"timestamp" : @(CACurrentMediaTime())
  }];
}

- (void)reset {
  [_stateHistory removeAllObjects];
}

- (void)saveStateToDisk {
  if (!_enableStatePersistence)
    return;

  NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
  NSDictionary *state = @{
    @"lifecycleState" : @(_lifecycleState),
    @"stateHistory" : _stateHistory,
    @"hasPendingUpdates" : @(_pendingUpdates.count > 0)
  };

  [defaults setObject:state forKey:kStateStorageKey];
  [defaults synchronize];
}

- (void)restoreStateFromDisk {
  if (!_enableStatePersistence)
    return;

  NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
  NSDictionary *state = [defaults objectForKey:kStateStorageKey];

  if (state) {
    _lifecycleState =
        (ShikiViewLifecycleState)[state[@"lifecycleState"] integerValue];
    _stateHistory = [state[@"stateHistory"] mutableCopy];
    if ([state[@"hasPendingUpdates"] boolValue]) {
      if (!_pendingUpdates) {
        _pendingUpdates = [NSMutableArray new];
      }
    }
  }
}

- (void)clearPersistedState {
  [[NSUserDefaults standardUserDefaults] removeObjectForKey:kStateStorageKey];
  [[NSUserDefaults standardUserDefaults] synchronize];
}

- (void)beginBatchUpdate {
  _isBatchUpdating = YES;
  _hasPendingUpdates = NO;
}

- (void)endBatchUpdate {
  _isBatchUpdating = NO;
  _pendingUpdateCount = 0;
  [_pendingUpdates removeAllObjects];

  if (self.isActive) {
    [self transitionToState:ShikiViewLifecycleStateActive];
  }
}

- (void)cancelBatchUpdate {
  _isBatchUpdating = NO;
  _hasPendingUpdates = YES;
}

- (void)schedulePendingUpdate:(void (^)(void))update {
  _pendingUpdateCount++;
  [_pendingUpdates addObject:update];

  if (!_isBatchUpdateInProgress && self.canAcceptUpdates) {
    [self processPendingUpdates];
  }
}

- (void)processPendingUpdates {
  if (_pendingUpdates.count == 0)
    return;

  [self beginBatchUpdate];

  @try {
    for (void (^update)(void) in _pendingUpdates) {
      update();
    }
    [self endBatchUpdate];
  } @catch (NSException *exception) {
    [self cancelBatchUpdate];
    @throw exception;
  }
}

- (void)attemptRecoveryWithCompletion:(void (^)(BOOL succeeded))completion {
  // Simple recovery: just clear the error and return to active state
  [self clearError];
  completion(YES);
}

- (void)resetWithCompletion:(void (^)(void))completion {
  // Clear all state
  [self clearPersistedState];
  [self reset];

  // Reset to initial state
  _lifecycleState = ShikiViewLifecycleStateInactive;
  _lastError = nil;
  _retryAttempts = 0;

  if (completion) {
    completion();
  }
}

// Add custom getter for stateHistory
- (NSArray<NSDictionary *> *)stateHistory {
  return [_stateHistory copy];
}

- (BOOL)hasPendingUpdates {
  return _pendingUpdates.count > 0;
}

- (NSError *)lastError {
  return _lastError;
}

- (void)setError:(NSError *)error {
  _lastError = error;
  if (error) {
    _lifecycleState = ShikiViewLifecycleStateError;
  }
}

- (void)clearError {
  _lastError = nil;
  if (_lifecycleState == ShikiViewLifecycleStateError) {
    _lifecycleState = ShikiViewLifecycleStateActive;
  }
}

@end

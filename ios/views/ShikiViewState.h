#pragma once

#import <Foundation/Foundation.h>

typedef NS_ENUM(NSInteger, ShikiViewLifecycleState) {
  ShikiViewLifecycleStateActive,
  ShikiViewLifecycleStateBackground,
  ShikiViewLifecycleStateOffscreen,
  ShikiViewLifecycleStateError,
  ShikiViewLifecycleStateInactive,
  ShikiViewLifecycleStateUpdating
};

NS_ASSUME_NONNULL_BEGIN

@interface ShikiViewState : NSObject

@property(nonatomic, assign) ShikiViewLifecycleState lifecycleState;
@property(nonatomic, assign) BOOL hasPendingUpdates;
@property(nonatomic, readonly) BOOL canAcceptUpdates;
@property(nonatomic, readonly) BOOL canPerformUpdates;
@property(nonatomic, strong, readonly, nullable) NSError *lastError;
@property(nonatomic, readonly, copy, nonnull)
    NSArray<NSDictionary *> *stateHistory;

// Update management
- (void)beginUpdate;
- (void)endUpdate;
- (void)beginBatchUpdate;
- (void)endBatchUpdate;
- (void)cancelBatchUpdate;
- (void)cancelUpdate;
- (void)pauseUpdates;
- (void)resumeUpdates;

// State management
- (void)beginStateTransition;
- (void)completeStateTransition;
- (void)cancelStateTransition;
- (void)transitionToState:(ShikiViewLifecycleState)newState;
- (void)reset;

// Error handling
- (void)setError:(nullable NSError *)error;
- (void)clearError;
- (void)attemptRecoveryWithCompletion:
    (void (^_Nullable)(BOOL succeeded))completion;

// State persistence
- (void)saveStateToDisk;
- (void)restoreStateFromDisk;
- (void)clearPersistedState;

// Helper methods
- (BOOL)isActive;
- (BOOL)needsRecovery;
- (nonnull NSDictionary *)currentState;
- (void)handleError:(nonnull NSError *)error;
- (void)resetWithCompletion:(void (^_Nullable)(void))completion;

@end

NS_ASSUME_NONNULL_END

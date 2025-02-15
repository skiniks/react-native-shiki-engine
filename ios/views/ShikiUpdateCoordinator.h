#import "ShikiViewState.h"
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface ShikiUpdateOperation : NSObject
@property(nonatomic, copy) void (^updateBlock)(void);
@property(nonatomic, assign) NSTimeInterval timestamp;
@property(nonatomic, assign) BOOL isCancelled;
@end

@interface ShikiUpdateCoordinator : NSObject

@property(nonatomic, weak) ShikiViewState *viewState;
@property(nonatomic, readonly) NSUInteger pendingUpdateCount;
@property(nonatomic, assign) NSTimeInterval updateCoalescingInterval;
@property(nonatomic, copy, nullable) void (^onUpdateComplete)(BOOL success);

- (instancetype)initWithViewState:(ShikiViewState *)viewState;
- (void)scheduleUpdate:(void (^)(void))updateBlock;
- (void)cancelAllUpdates;
- (void)processUpdates;
- (void)pauseUpdates;
- (void)resumeUpdates;

// Batch update support
- (void)beginBatchUpdates;
- (void)endBatchUpdates;
- (void)cancelPendingUpdates;

@end

NS_ASSUME_NONNULL_END

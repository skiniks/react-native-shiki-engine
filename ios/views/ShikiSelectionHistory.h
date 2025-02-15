#import "ShikiSelectionTypes.h"
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface ShikiSelectionState : NSObject

@property(nonatomic, assign) NSRange range;
@property(nonatomic, assign) ShikiSelectionMode mode;
@property(nonatomic, copy, nullable) NSString *scope;

+ (instancetype)stateWithRange:(NSRange)range
                          mode:(ShikiSelectionMode)mode
                         scope:(nullable NSString *)scope;

@end

@interface ShikiSelectionHistory : NSObject

@property(nonatomic, readonly) BOOL canUndo;
@property(nonatomic, readonly) BOOL canRedo;
@property(nonatomic, assign) NSUInteger maxHistorySize;

- (void)pushState:(ShikiSelectionState *)state;
- (nullable ShikiSelectionState *)undo;
- (nullable ShikiSelectionState *)redo;
- (void)clear;

@end

NS_ASSUME_NONNULL_END

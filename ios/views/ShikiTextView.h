#import "ShikiSelectionManager.h"
#import "ShikiUpdateCoordinator.h"
#import "ShikiViewState.h"
#import "ShikiVirtualizedContentManager.h"
#import <React/RCTComponent.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

extern NSString* const ShikiTextViewSelectionChangedNotification;
extern NSString* const ShikiTextViewDidCopyNotification;

@interface ShikiTextView : UITextView <UITextViewDelegate>

// Event callbacks
@property(nonatomic, copy, nullable) RCTDirectEventBlock onSelectionChange;
@property(nonatomic, copy, nullable) RCTDirectEventBlock onVisibleRangeChanged;
@property(nonatomic, copy, nullable) void (^onCopy)(NSString* copiedText);
@property(nonatomic, copy, nullable) void (^onSyntaxScopeSelected)(NSString* scope, NSRange range);
@property(nonatomic, copy, nullable) void (^onKeyCommandTriggered)(NSString* command);

// State flags
@property(nonatomic, assign) BOOL shouldNotifySelectionChanges;
@property(nonatomic, assign) BOOL preserveSelectionOnUpdate;
@property(nonatomic, assign) BOOL allowsDragSelection;
@property(nonatomic, assign) BOOL enableKeyboardShortcuts;
@property(nonatomic, assign) BOOL shouldPreserveStateOnMemoryWarning;
@property(nonatomic, assign) CGFloat lineHeight;
@property(nonatomic, assign) BOOL isScrolling;

// Core components
@property(nonatomic, strong, readonly) ShikiViewState* viewState;
@property(nonatomic, strong, readonly) ShikiSelectionManager* selectionManager;
@property(nonatomic, strong, readonly) ShikiVirtualizedContentManager* virtualizer;
@property(nonatomic, strong, readonly) ShikiUpdateCoordinator* updateCoordinator;
@property(nonatomic, strong, readonly) NSTextStorage* optimizedStorage;
@property(nonatomic, assign) NSTimeInterval updateDebounceInterval;

// Selection handling
@property(nonatomic, copy, nullable) void (^onSelectionDragBegin)(CGPoint location);
@property(nonatomic, copy, nullable) void (^onSelectionDragMove)(CGPoint location);
@property(nonatomic, copy, nullable) void (^onSelectionDragEnd)(CGPoint location);

// Error handling
@property(nonatomic, copy, nullable) void (^onError)(NSError* error);
@property(nonatomic, copy, nullable) void (^onLifecycleStateChange)(ShikiViewLifecycleState state);

// Selection methods
- (void)selectLine:(NSInteger)lineNumber;
- (void)selectWord:(NSInteger)location;
- (void)selectScope:(NSString*)scope;
- (NSArray<NSValue*>*)getSelectableRanges;

// Search methods
- (NSArray<NSValue*>*)findText:(NSString*)text options:(NSRegularExpressionOptions)options;
- (void)selectNextMatch;
- (void)selectPreviousMatch;
- (void)selectAllMatches;
- (void)clearSearch;

// Undo/redo
- (void)undoSelection;
- (void)redoSelection;

// View lifecycle
- (void)cleanup;
- (void)resetTextStorage;
- (void)cancelPendingHighlights;
- (void)optimizeForLargeContent;
- (void)setContentWithoutHighlighting:(NSString*)content;
- (void)handleError:(NSError*)error;

// Update management
- (void)suspendUpdates;
- (void)resumeUpdates;
- (void)resetView;

// Viewport tracking
- (void)updateViewport;
- (void)scrollViewDidScroll:(UIScrollView*)scrollView;
- (void)scrollViewWillBeginDragging:(UIScrollView*)scrollView;
- (void)scrollViewDidEndDragging:(UIScrollView*)scrollView willDecelerate:(BOOL)decelerate;

// Batch updates
- (void)beginBatchUpdates;
- (void)endBatchUpdates;
- (void)prepareForReuse;

@end

NS_ASSUME_NONNULL_END

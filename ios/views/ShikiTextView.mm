#import "ShikiTextView.h"
#import "ShikiPerformanceReporter.h"
#import "ShikiSelectionManager.h"
#import "ShikiUpdateCoordinator.h"
#import "ShikiViewState.h"
#import "ShikiVirtualizedContentManager.h"
#import <React/RCTBridge.h>
#import <React/RCTBridgeModule.h>

NSString* const ShikiTextViewSelectionChangedNotification =
    @"ShikiTextViewSelectionChangedNotification";
NSString* const ShikiTextViewDidCopyNotification = @"ShikiTextViewDidCopyNotification";

@interface ShikiTextView ()
@property(nonatomic, strong, readwrite) NSTextStorage* optimizedStorage;
@end

@implementation ShikiTextView {
  NSOperationQueue* _highlightQueue;
  BOOL _isHighlighting;
  NSMutableArray<NSValue*>* _selectableRanges;
  NSTimer* _updateTimer;
  ShikiViewState* _viewState;
  NSTimeInterval _updateDebounceInterval;
  ShikiSelectionManager* _selectionManager;
  BOOL _enableKeyboardShortcuts;
  BOOL _shouldPreserveStateOnMemoryWarning;
  ShikiUpdateCoordinator* _updateCoordinator;
  ShikiVirtualizedContentManager* _virtualizer;
  CGFloat _lineHeight;
  BOOL _isScrolling;
  __weak ShikiTextView* _weakSelf;
  NSArray<UIKeyCommand*>* _customKeyCommands;
}

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    _virtualizer = [[ShikiVirtualizedContentManager alloc] init];
    _lineHeight = 0;
    _isScrolling = NO;
    self.delegate = self;
    [self commonInit];
  }
  return self;
}

- (void)commonInit {
  self.editable = NO;
  self.selectable = YES;
  self.delegate = self;

  _highlightQueue = [[NSOperationQueue alloc] init];
  _highlightQueue.maxConcurrentOperationCount = 1;
  _shouldNotifySelectionChanges = YES;

  // Initialize selection support
  _selectableRanges = [NSMutableArray new];
  _selectionManager = [[ShikiSelectionManager alloc] init];

  // Set up selection callback with proper weak reference
  ShikiTextView* __weak weakSelf = self;
  _selectionManager.onScopeSelected = ^(NSString* scope, NSRange range) {
    ShikiTextView* strongSelf = weakSelf;
    if (!strongSelf)
      return;

    if (strongSelf.onSyntaxScopeSelected) {
      strongSelf.onSyntaxScopeSelected(scope, range);
    }
  };

  // Set up view state
  _viewState = [[ShikiViewState alloc] init];

  // Add observers for lifecycle events
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(applicationWillResignActive:)
                                               name:UIApplicationWillResignActiveNotification
                                             object:nil];

  // Add copy menu item observer
  [self addCopyMenuObserver];

  _updateDebounceInterval = 0.1; // 100ms default

  // Add long press gesture for scope selection
  UILongPressGestureRecognizer* longPress =
      [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(handleLongPress:)];
  [self addGestureRecognizer:longPress];

  // Add drag gesture recognizer
  UIPanGestureRecognizer* dragGesture =
      [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(handleDragGesture:)];
  dragGesture.minimumNumberOfTouches = 1;
  dragGesture.maximumNumberOfTouches = 1;
  [self addGestureRecognizer:dragGesture];

  _enableKeyboardShortcuts = YES;
  [self registerKeyCommands];

  // Configure view state callbacks
  [_viewState addObserver:self
               forKeyPath:@"lifecycleState"
                  options:NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld
                  context:nil];

  _shouldPreserveStateOnMemoryWarning = NO;

  // Add memory pressure observer
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(handleMemoryWarning:)
                                               name:UIApplicationDidReceiveMemoryWarningNotification
                                             object:nil];

  // Initialize update coordinator
  _updateCoordinator = [[ShikiUpdateCoordinator alloc] initWithViewState:_viewState];

  // Set up update coordinator callback with proper weak reference
  _updateCoordinator.onUpdateComplete = ^(BOOL success) {
    ShikiTextView* strongSelf = weakSelf;
    if (!strongSelf)
      return;

    if (!success) {
      [strongSelf handleViewError];
    }
  };

  // Configure performance reporting with strong reference
  ShikiPerformanceReporter* reporter = [ShikiPerformanceReporter sharedReporter];
  RCTBridge* bridge = [[RCTBridge alloc] init];
  [reporter setBridge:bridge]; // Use setter method instead of direct assignment
  reporter.autoReportEnabled = YES;
  [reporter startReporting];
}

- (void)observeValueForKeyPath:(NSString*)keyPath
                      ofObject:(id)object
                        change:(NSDictionary<NSKeyValueChangeKey, id>*)change
                       context:(void*)context {
  if ([keyPath isEqualToString:@"lifecycleState"] && object == _viewState) {
    ShikiViewLifecycleState oldState =
        (ShikiViewLifecycleState)[[change objectForKey:NSKeyValueChangeOldKey] integerValue];
    ShikiViewLifecycleState newState =
        (ShikiViewLifecycleState)[[change objectForKey:NSKeyValueChangeNewKey] integerValue];
    [self handleLifecycleStateChange:oldState toState:newState];
  }
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [self cancelPendingHighlights];

  if ([ShikiPerformanceReporter sharedReporter].autoReportEnabled) {
    [[ShikiPerformanceReporter sharedReporter] reportMetrics];
  }

  _virtualizer = nil;
}

#pragma mark - Public Methods

- (void)cancelPendingHighlights {
  [_highlightQueue cancelAllOperations];
  _isHighlighting = NO;
}

- (void)cleanup {
  // Clear selection state
  [_selectionManager clearSelection];

  // Cancel any pending operations
  [self cancelPendingHighlights];
  [_updateTimer invalidate];
  _updateTimer = nil;

  // Reset view state
  self.attributedText = nil;
  _isHighlighting = NO;

  // Clear optimized storage
  [self resetTextStorage];

  [_updateCoordinator cancelAllUpdates];
}

- (void)resetTextStorage {
  if (_optimizedStorage) {
    [_optimizedStorage beginEditing];
    [_optimizedStorage deleteCharactersInRange:NSMakeRange(0, _optimizedStorage.length)];
    [_optimizedStorage endEditing];

    // Get the layout manager and text container
    NSLayoutManager* layoutManager = self.layoutManager;
    NSTextContainer* textContainer = self.textContainer;

    // On iOS, UITextView's NSLayoutManager generally only uses one text container
    // so we don't need to remove any containers, just ensure our text container is attached
    if (![layoutManager.textContainers containsObject:textContainer]) {
      [layoutManager addTextContainer:textContainer];
    }
    [_optimizedStorage addLayoutManager:layoutManager];
  }
}

#pragma mark - UITextViewDelegate

- (void)textViewDidChangeSelection:(UITextView*)textView {
  if (!_shouldNotifySelectionChanges)
    return;

  NSRange selectedRange = textView.selectedRange;

  // Notify via block if set
  if (_onSelectionChange) {
    _onSelectionChange(
        @{@"range" : @{@"start" : @(selectedRange.location), @"length" : @(selectedRange.length)}});
  }

  // Post notification
  [[NSNotificationCenter defaultCenter]
      postNotificationName:ShikiTextViewSelectionChangedNotification
                    object:self
                  userInfo:@{@"selectedRange" : [NSValue valueWithRange:selectedRange]}];
}

#pragma mark - Notifications

- (void)applicationWillResignActive:(NSNotification*)notification {
  // Cancel any pending highlights when app goes to background
  [self cancelPendingHighlights];
}

#pragma mark - Selection Enhancement

- (void)selectLine:(NSInteger)lineNumber {
  [_selectionManager selectLineAtLocation:lineNumber];
}

- (void)selectWord:(NSInteger)location {
  [_selectionManager selectWordAtLocation:location];
}

- (void)selectScope:(NSString*)scope {
  [_selectionManager selectScope:scope];
}

#pragma mark - Performance Optimizations

- (void)optimizeForLargeContent {
  if (!_optimizedStorage) {
    _optimizedStorage = [[NSTextStorage alloc] init];
    [_optimizedStorage addLayoutManager:self.layoutManager];
    self.layoutManager.allowsNonContiguousLayout = YES;
    self.textContainer.widthTracksTextView = YES;
    self.textContainer.heightTracksTextView = NO;
  }
}

- (void)beginBatchUpdates {
  [self.updateCoordinator beginBatchUpdates];
}

- (void)endBatchUpdates {
  [self.updateCoordinator endBatchUpdates];
}

- (void)prepareForReuse {
  [self.updateCoordinator cancelPendingUpdates];
  [self resetTextStorage];
  [self cleanup];
}

- (void)invalidateContent {
  [self cancelPendingHighlights];
  [_updateTimer invalidate];
  _updateTimer = nil;

  // Clear optimized storage if needed
  [self resetTextStorage];
}

- (void)setContentWithoutHighlighting:(NSString*)content {
  __weak ShikiTextView* weakSelf = self;
  [_updateCoordinator scheduleUpdate:^{
    ShikiTextView* strongSelf = weakSelf;
    if (!strongSelf)
      return;

    NSRange previousRange = strongSelf.selectedRange;
    strongSelf.text = content;

    if (strongSelf.preserveSelectionOnUpdate) {
      strongSelf.selectedRange = previousRange;
    }
  }];
}

- (void)processQueuedUpdates {
  if (!_viewState.canPerformUpdates) {
    return;
  }

  [_viewState beginBatchUpdate];

  @try {
    // Process updates
    if (_highlightQueue.operationCount > 0) {
      [self cancelPendingHighlights];
      // Re-highlight if needed
    }

    [_viewState endBatchUpdate];
  } @catch (NSException* exception) {
    [_viewState cancelBatchUpdate];
    [self handleViewError];
  }
}

#pragma mark - Copy Support

- (void)addCopyMenuObserver {
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(handleMenuControllerWillShow:)
                                               name:UIMenuControllerWillShowMenuNotification
                                             object:nil];
}

- (void)handleMenuControllerWillShow:(NSNotification*)notification {
  // Just call setMenuItems directly without creating unused variable
  [[UIMenuController sharedMenuController] setMenuItems:nil];
}

- (void)copy:(id)sender {
  [super copy:sender];

  NSString* copiedText = [[UIPasteboard generalPasteboard] string];
  if (self.onCopy) {
    self.onCopy(copiedText);
  }

  [[NSNotificationCenter defaultCenter] postNotificationName:ShikiTextViewDidCopyNotification
                                                      object:self
                                                    userInfo:@{@"text" : copiedText}];
}

#pragma mark - Text Storage Optimization

- (void)layoutSubviews {
  [super layoutSubviews];

  // Optimize text container size
  self.textContainer.size = CGSizeMake(self.bounds.size.width, CGFLOAT_MAX);
}

#pragma mark - Gesture Recognizers

- (void)addGestureRecognizers {
  UITapGestureRecognizer* doubleTap =
      [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleDoubleTap:)];
  doubleTap.numberOfTapsRequired = 2;
  [self addGestureRecognizer:doubleTap];

  UITapGestureRecognizer* tripleTap =
      [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTripleTap:)];
  tripleTap.numberOfTapsRequired = 3;
  [self addGestureRecognizer:tripleTap];

  [doubleTap requireGestureRecognizerToFail:tripleTap];
}

- (void)handleDoubleTap:(UITapGestureRecognizer*)recognizer {
  CGPoint location = [recognizer locationInView:self];
  NSUInteger index = [self.layoutManager characterIndexForPoint:location
                                                inTextContainer:self.textContainer
                       fractionOfDistanceBetweenInsertionPoints:NULL];

  [_selectionManager selectWordAtLocation:index];
}

- (void)handleTripleTap:(UITapGestureRecognizer*)recognizer {
  CGPoint location = [recognizer locationInView:self];
  NSUInteger index = [self.layoutManager characterIndexForPoint:location
                                                inTextContainer:self.textContainer
                       fractionOfDistanceBetweenInsertionPoints:NULL];

  [_selectionManager selectLineAtLocation:index];
}

- (void)handleLongPress:(UILongPressGestureRecognizer*)recognizer {
  if (recognizer.state != UIGestureRecognizerStateBegan)
    return;

  CGPoint location = [recognizer locationInView:self];
  NSUInteger index = [self.layoutManager characterIndexForPoint:location
                                                inTextContainer:self.textContainer
                       fractionOfDistanceBetweenInsertionPoints:NULL];

  [_selectionManager expandToSyntaxScope];
}

- (void)handleDragGesture:(UIPanGestureRecognizer*)recognizer {
  if (!_allowsDragSelection)
    return;

  CGPoint location = [recognizer locationInView:self];

  switch (recognizer.state) {
    case UIGestureRecognizerStateBegan: {
      if (_onSelectionDragBegin) {
        _onSelectionDragBegin(location);
      }
      break;
    }

    case UIGestureRecognizerStateChanged: {
      if (_onSelectionDragMove) {
        _onSelectionDragMove(location);
      }
      NSRange currentSelection = _selectionManager.primarySelectedRange;
      NSUInteger characterIndex = [self.layoutManager characterIndexForPoint:location
                                                             inTextContainer:self.textContainer
                                    fractionOfDistanceBetweenInsertionPoints:NULL];
      NSRange newRange;

      if (characterIndex < currentSelection.location) {
        newRange = NSMakeRange(characterIndex, NSMaxRange(currentSelection) - characterIndex);
      } else {
        newRange =
            NSMakeRange(currentSelection.location, characterIndex - currentSelection.location);
      }

      [_selectionManager selectRange:newRange];
      break;
    }

    case UIGestureRecognizerStateEnded: {
      if (_onSelectionDragEnd) {
        _onSelectionDragEnd(location);
      }

      if ([self isCommandKeyPressed]) {
        [_selectionManager endMultiSelection];
      }
      break;
    }

    default:
      break;
  }
}

- (BOOL)isCommandKeyPressed {
#if TARGET_OS_OSX
  if (@available(iOS 13.0, *)) {
    UIScene* scene = UIApplication.sharedApplication.connectedScenes.anyObject;
    if ([scene isKindOfClass:[UIWindowScene class]]) {
      UIWindowScene* windowScene = (UIWindowScene*)scene;
      UIWindow* window = windowScene.windows.firstObject;
      return (window.undoManager.modifierFlags & UIKeyModifierCommand) != 0;
    }
  }
#endif
  // On iOS we don't need to check modifier flags manually
  // since UIKeyCommand handles this for us
  return NO;
}

#pragma mark - Syntax Scopes

- (void)updateSyntaxScopes:(NSArray<NSString*>*)scopes {
  [_selectionManager updateSyntaxScopes:scopes];
}

- (NSArray<NSString*>*)getSyntaxScopesAtLocation:(NSUInteger)location {
  return [_selectionManager getSyntaxScopesAtLocation:location];
}

- (void)willMoveToWindow:(UIWindow*)newWindow {
  [super willMoveToWindow:newWindow];

  if (!newWindow) {
    // Save state before moving offscreen
    [_viewState saveStateToDisk];
    [_selectionManager saveSelectionState];
    [_updateCoordinator pauseUpdates];
  }

  ShikiViewLifecycleState newState =
      newWindow ? ShikiViewLifecycleStateActive : ShikiViewLifecycleStateOffscreen;

  [_viewState transitionToState:newState];
}

- (void)didMoveToWindow {
  [super didMoveToWindow];

  if (self.window) {
    // Restore state when coming onscreen
    [_viewState restoreStateFromDisk];

    // Resume updates if needed
    if (_viewState.hasPendingUpdates) {
      [_updateCoordinator resumeUpdates];
    }
  }
}

#pragma mark - Keyboard Command Support

- (void)registerKeyCommands {
  if (!_enableKeyboardShortcuts)
    return;

  UIKeyCommand* selectAll = [UIKeyCommand keyCommandWithInput:@"a"
                                                modifierFlags:UIKeyModifierCommand
                                                       action:@selector(selectAll:)];
  selectAll.discoverabilityTitle = @"Select All";

  UIKeyCommand* copy = [UIKeyCommand keyCommandWithInput:@"c"
                                           modifierFlags:UIKeyModifierCommand
                                                  action:@selector(copy:)];
  copy.discoverabilityTitle = @"Copy";

  _customKeyCommands = @[ selectAll, copy ];
}

- (NSArray<UIKeyCommand*>*)keyCommands {
  return _customKeyCommands;
}

#pragma mark - Keyboard Command Handlers

- (void)handleSelectAll:(UIKeyCommand*)command {
  _selectionManager.selectionMode = ShikiSelectionModeNormal;
  [self selectAll:nil];
}

- (void)handleExpandSelection:(UIKeyCommand*)command {
  [_selectionManager expandSelection];
}

- (void)handleSelectNextOccurrence:(UIKeyCommand*)command {
  [_selectionManager selectNextOccurrence];
}

- (void)handleUndo:(UIKeyCommand*)command {
  [_selectionManager undo];
}

- (void)handleRedo:(UIKeyCommand*)command {
  [_selectionManager redo];
}

- (void)handleFind {
  if (self.onKeyCommandTriggered) {
    self.onKeyCommandTriggered(@"find");
  }
}

- (void)handleFindNext:(UIKeyCommand*)command {
  [_selectionManager selectNextMatch];
}

- (void)handleFindPrevious:(UIKeyCommand*)command {
  [_selectionManager selectPreviousMatch];
}

- (void)handleLifecycleStateChange:(ShikiViewLifecycleState)oldState
                           toState:(ShikiViewLifecycleState)newState {
  // Begin state transition
  [_viewState beginStateTransition];

  @try {
    switch (newState) {
      case ShikiViewLifecycleStateActive:
        [self handleActiveState];
        break;

      case ShikiViewLifecycleStateBackground:
        [self handleBackgroundState];
        break;

      case ShikiViewLifecycleStateOffscreen:
        [self handleOffscreenState];
        break;

      case ShikiViewLifecycleStateError:
        [self handleErrorState];
        break;

      case ShikiViewLifecycleStateUpdating:
        [self handleUpdatingState];
        break;

      default:
        break;
    }

    // Complete transition if successful
    [_viewState completeStateTransition];

  } @catch (NSException* exception) {
    // Cancel transition on error
    [_viewState cancelStateTransition];
    [self handleViewError];
  }

  if (self.onLifecycleStateChange) {
    self.onLifecycleStateChange(newState);
  }
}

- (void)handleActiveState {
  [_updateCoordinator resumeUpdates];
  [_selectionManager restoreSelectionState];
}

- (void)handleBackgroundState {
  [_updateCoordinator pauseUpdates];
  [_selectionManager saveSelectionState];
}

- (void)handleOffscreenState {
  [_updateCoordinator cancelAllUpdates];
  [_updateTimer invalidate];
  _updateTimer = nil;
}

- (void)handleUpdatingState {
  // Prepare for update
  [self beginBatchUpdates];
}

- (void)handleErrorState {
  // Clean up and notify
  [self cancelPendingHighlights];
  [_updateTimer invalidate];
  _updateTimer = nil;

  if (self.onError) {
    NSError* error =
        [NSError errorWithDomain:@"ShikiTextView"
                            code:-1
                        userInfo:@{NSLocalizedDescriptionKey : @"View entered error state"}];
    self.onError(error);
  }
}

- (void)handleAppDidEnterBackground:(NSNotification*)notification {
  [_viewState transitionToState:ShikiViewLifecycleStateBackground];
}

- (void)handleAppWillEnterForeground:(NSNotification*)notification {
  if (self.window) {
    [_viewState transitionToState:ShikiViewLifecycleStateActive];
  }
}

- (void)suspendUpdates {
  [self cancelPendingHighlights];
  [_updateTimer invalidate];
  _updateTimer = nil;

  // Save any pending state
  [_selectionManager saveSelectionState];
}

- (void)resumeUpdates {
  if (_viewState.canAcceptUpdates) {
    [self processQueuedUpdates];
    [_selectionManager restoreSelectionState];
  }
}

- (void)handleViewError {
  [self handleError:_viewState.lastError];
}

- (void)handleMemoryWarning:(NSNotification*)notification {
  if (!_shouldPreserveStateOnMemoryWarning) {
    [self clearMemoryHeavyResources];
  } else {
    [self preserveStateAndReduceMemory];
  }
}

- (void)clearMemoryHeavyResources {
  // Clear optimized storage
  if (_optimizedStorage) {
    [_optimizedStorage beginEditing];
    [_optimizedStorage deleteCharactersInRange:NSMakeRange(0, _optimizedStorage.length)];
    [_optimizedStorage endEditing];
    _optimizedStorage = nil;
  }

  // Cancel pending operations
  [self cancelPendingHighlights];
  [_updateTimer invalidate];
  _updateTimer = nil;

  // Clear search results
  [_selectionManager clearSearch];

  // Save state before clearing
  [_viewState saveStateToDisk];
}

- (void)preserveStateAndReduceMemory {
  // Save current state
  [_viewState saveStateToDisk];
  [_selectionManager saveSelectionState];

  // Clear only non-essential resources
  [self cancelPendingHighlights];
  [_updateTimer invalidate];
  _updateTimer = nil;

  // Reduce memory usage of text storage
  if (_optimizedStorage) {
    [_optimizedStorage beginEditing];
    [_optimizedStorage endEditing];
  }
}

- (void)updateViewport {
  CGRect visibleRect = self.bounds;
  CGPoint contentOffset = self.contentOffset;

  // Calculate visible lines
  NSUInteger firstLine = [_virtualizer getEstimatedLineAtPosition:contentOffset.y
                                                       lineHeight:_lineHeight];
  NSUInteger lastLine =
      [_virtualizer getEstimatedLineAtPosition:(contentOffset.y + visibleRect.size.height)
                                    lineHeight:_lineHeight];

  [_virtualizer updateViewport:firstLine
               lastVisibleLine:lastLine
                 contentHeight:self.contentSize.height
                viewportHeight:visibleRect.size.height
                   isScrolling:_isScrolling];

  // Get visible range
  NSRange visibleRange = [_virtualizer getVisibleRangeForText:self.text];

  // Request update for visible range
  [self requestUpdateForRange:visibleRange];
}

- (void)scrollViewDidScroll:(UIScrollView*)scrollView {
  [self updateViewport];
}

- (void)scrollViewWillBeginDragging:(UIScrollView*)scrollView {
  _isScrolling = YES;
  [self updateViewport];
}

- (void)scrollViewDidEndDragging:(UIScrollView*)scrollView willDecelerate:(BOOL)decelerate {
  if (!decelerate) {
    _isScrolling = NO;
    [self updateViewport];
  }
}

- (void)scrollViewDidEndDecelerating:(UIScrollView*)scrollView {
  _isScrolling = NO;
  [self updateViewport];
}

- (void)requestUpdateForRange:(NSRange)range {
  // Notify highlighter of visible range change
  if (self.onVisibleRangeChanged) {
    self.onVisibleRangeChanged(@{
      @"range" : @{@"start" : @(range.location), @"length" : @(range.length)},
      @"isScrolling" : @(_isScrolling)
    });
  }
}

- (void)handleError:(NSError*)error {
  if (self.onError) {
    self.onError(error);
  }
}

- (NSArray<NSValue*>*)getSelectableRanges {
  return [_selectableRanges copy];
}

- (NSArray<NSValue*>*)findText:(NSString*)text options:(NSRegularExpressionOptions)options {
  NSMutableArray<NSValue*>* ranges = [NSMutableArray new];
  NSError* error = nil;
  NSRegularExpression* regex = [NSRegularExpression regularExpressionWithPattern:text
                                                                         options:options
                                                                           error:&error];
  if (error) {
    if (self.onError) {
      self.onError(error);
    }
    return @[];
  }

  NSString* content = self.text;
  NSArray<NSTextCheckingResult*>* matches = [regex matchesInString:content
                                                           options:0
                                                             range:NSMakeRange(0, content.length)];

  for (NSTextCheckingResult* match in matches) {
    [ranges addObject:[NSValue valueWithRange:match.range]];
  }

  return ranges;
}

- (void)selectNextMatch {
  [_selectionManager selectNextMatch];
}

- (void)selectPreviousMatch {
  [_selectionManager selectPreviousMatch];
}

- (void)selectAllMatches {
  [_selectionManager selectAllMatches];
}

- (void)clearSearch {
  [_selectionManager clearSearch];
}

- (void)undoSelection {
  [_selectionManager undo];
}

- (void)redoSelection {
  [_selectionManager redo];
}

- (void)resetView {
  [self cleanup];
  [_viewState reset];
  [_viewState transitionToState:ShikiViewLifecycleStateInactive];
}

@end

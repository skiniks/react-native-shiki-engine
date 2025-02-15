#import "ShikiSelectionHistory.h"

@implementation ShikiSelectionState

+ (instancetype)stateWithRange:(NSRange)range
                          mode:(ShikiSelectionMode)mode
                         scope:(nullable NSString *)scope {
  ShikiSelectionState *state = [[ShikiSelectionState alloc] init];
  state.range = range;
  state.mode = mode;
  state.scope = scope;
  return state;
}

@end

@implementation ShikiSelectionHistory {
  NSMutableArray<ShikiSelectionState *> *_undoStack;
  NSMutableArray<ShikiSelectionState *> *_redoStack;
}

- (instancetype)init {
  if (self = [super init]) {
    _undoStack = [NSMutableArray new];
    _redoStack = [NSMutableArray new];
    _maxHistorySize = 50;
  }
  return self;
}

- (void)pushState:(ShikiSelectionState *)state {
  [_undoStack addObject:state];
  [_redoStack removeAllObjects];

  if (_undoStack.count > _maxHistorySize) {
    [_undoStack removeObjectAtIndex:0];
  }
}

- (nullable ShikiSelectionState *)undo {
  if (_undoStack.count == 0)
    return nil;

  ShikiSelectionState *state = [_undoStack lastObject];
  [_undoStack removeLastObject];
  [_redoStack addObject:state];

  return state;
}

- (nullable ShikiSelectionState *)redo {
  if (_redoStack.count == 0)
    return nil;

  ShikiSelectionState *state = [_redoStack lastObject];
  [_redoStack removeLastObject];
  [_undoStack addObject:state];

  return state;
}

- (void)clear {
  [_undoStack removeAllObjects];
  [_redoStack removeAllObjects];
}

- (BOOL)canUndo {
  return _undoStack.count > 0;
}

- (BOOL)canRedo {
  return _redoStack.count > 0;
}

@end

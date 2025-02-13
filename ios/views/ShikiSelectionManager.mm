#import "ShikiSelectionManager.h"
#import "ShikiPerformanceMonitor.h"
#import "ShikiSelectionHistory.h"
#import "ShikiTextView.h"

@interface ShikiSelectionManager ()

@property(nonatomic, strong) NSMutableArray<NSValue*>* mutableSelectableRanges;
@property(nonatomic, strong) NSMutableArray<NSValue*>* mutableSelectedRanges;

- (void)expandWordSelection:(NSValue*)selection;
- (void)expandLineSelection:(NSValue*)selection;
- (void)expandScopeSelection:(NSValue*)selection;

@end

@implementation ShikiSelectionManager {
  NSRange _lastSelection;
  NSMutableDictionary<NSString*, NSArray<NSValue*>*>* _scopedRanges;
  NSMutableDictionary<NSString*, NSArray<NSValue*>*>* _syntaxScopeRanges;
  NSArray<NSString*>* _syntaxScopes;
  void (^_onScopeSelected)(NSString* scope, NSRange range);
  NSRange _primarySelection;
  BOOL _allowsMultipleSelection;
  ShikiSelectionHistory* _selectionHistory;
  NSArray<NSValue*>* _searchResults;
  NSInteger _currentSearchIndex;
  BOOL _allowsDiscontiguousSelection;
  void (^_onMultiSelectionChange)(NSArray<NSValue*>* ranges);
}

- (instancetype)initWithTextView:(ShikiTextView*)textView {
  if (self = [super init]) {
    _textView = textView;
    _selectionMode = ShikiSelectionModeNormal;
    _mutableSelectableRanges = [NSMutableArray new];
    _scopedRanges = [NSMutableDictionary new];
    _preserveSelectionOnUpdate = YES;
    _syntaxScopeRanges = [NSMutableDictionary new];
    _mutableSelectedRanges = [NSMutableArray new];
    _allowsMultipleSelection = NO;
    _selectionHistory = [[ShikiSelectionHistory alloc] init];
    _searchResults = @[];
    _currentSearchIndex = -1;
    _allowsDiscontiguousSelection = NO;
  }
  return self;
}

#pragma mark - Property Getters

- (NSArray<NSValue*>*)selectableRanges {
  return [_mutableSelectableRanges copy];
}

- (NSArray<NSValue*>*)selectedRanges {
  return [_mutableSelectedRanges copy];
}

#pragma mark - Private Selection Methods

- (void)expandWordSelection:(NSValue*)selection {
  if (!_textView)
    return;

  NSRange range = [selection rangeValue];
  NSString* text = _textView.text;

  // Find word boundaries
  NSCharacterSet* wordSet = [NSCharacterSet alphanumericCharacterSet];

  // Search backward for start
  NSInteger start = range.location;
  while (start > 0 && [wordSet characterIsMember:[text characterAtIndex:start - 1]]) {
    start--;
  }

  // Search forward for end
  NSInteger end = NSMaxRange(range);
  while (end < text.length && [wordSet characterIsMember:[text characterAtIndex:end]]) {
    end++;
  }

  [self selectRange:NSMakeRange(start, end - start)];
}

- (void)expandLineSelection:(NSValue*)selection {
  if (!_textView)
    return;

  NSRange range = [selection rangeValue];
  NSString* text = _textView.text;

  // Get the line range containing the selection
  NSRange lineRange = [text lineRangeForRange:range];

  // If already selecting full line(s), expand to include next line
  if (NSEqualRanges(lineRange, range)) {
    NSUInteger nextLineStart = NSMaxRange(lineRange);
    if (nextLineStart < text.length) {
      NSRange nextLineRange = [text lineRangeForRange:NSMakeRange(nextLineStart, 0)];
      lineRange = NSUnionRange(lineRange, nextLineRange);
    }
  }

  [self selectRange:lineRange];
}

- (void)expandScopeSelection:(NSValue*)selection {
  if (!_textView)
    return;

  NSRange range = [selection rangeValue];
  NSUInteger location = range.location;

  // Get all scopes at the current location
  NSArray<NSString*>* scopes = [self getSyntaxScopesAtLocation:location];

  // Find the next larger scope that contains the current selection
  NSRange bestRange = range;
  NSString* bestScope = nil;

  for (NSString* scope in scopes) {
    NSMutableArray* ranges = [NSMutableArray arrayWithArray:_syntaxScopeRanges[scope]];
    for (NSValue* rangeValue in ranges) {
      NSRange scopeRange = [rangeValue rangeValue];
      if (NSLocationInRange(range.location, scopeRange) &&
          NSLocationInRange(NSMaxRange(range), scopeRange) && scopeRange.length > range.length) {
        if (!bestScope || scopeRange.length < bestRange.length) {
          bestRange = scopeRange;
          bestScope = scope;
        }
      }
    }
  }

  if (bestScope && !NSEqualRanges(bestRange, range)) {
    [self selectRange:bestRange];
    if (_onScopeSelected) {
      _onScopeSelected(bestScope, bestRange);
    }
  }
}

#pragma mark - Public Methods

- (void)selectAllOccurrences {
  if (!_textView)
    return;

  NSRange currentSelection = _textView.selectedRange;
  NSString* selectedText = [_textView.text substringWithRange:currentSelection];

  if (selectedText.length == 0)
    return;

  // Find all occurrences
  NSMutableArray<NSValue*>* occurrences = [NSMutableArray array];
  NSString* text = _textView.text;
  NSRange searchRange = NSMakeRange(0, text.length);

  while (searchRange.location < text.length) {
    NSRange foundRange = [text rangeOfString:selectedText options:0 range:searchRange];

    if (foundRange.location == NSNotFound)
      break;

    [occurrences addObject:[NSValue valueWithRange:foundRange]];
    searchRange.location = NSMaxRange(foundRange);
    searchRange.length = text.length - searchRange.location;
  }

  // Enable multi-selection and select all occurrences
  _allowsMultipleSelection = YES;
  [_mutableSelectedRanges removeAllObjects];
  [_mutableSelectedRanges addObjectsFromArray:occurrences];
  [self updateTextViewSelection];
}

- (void)selectPreviousOccurrence {
  if (!_textView)
    return;

  NSRange currentSelection = _textView.selectedRange;
  NSString* selectedText = [_textView.text substringWithRange:currentSelection];

  if (selectedText.length == 0)
    return;

  // Search backward from current selection
  NSRange searchRange = NSMakeRange(0, currentSelection.location);
  NSRange previousRange = [_textView.text rangeOfString:selectedText
                                                options:NSBackwardsSearch
                                                  range:searchRange];

  if (previousRange.location != NSNotFound) {
    [self selectRange:previousRange];
  } else {
    // Wrap around to last occurrence
    searchRange =
        NSMakeRange(currentSelection.location, _textView.text.length - currentSelection.location);
    previousRange = [_textView.text rangeOfString:selectedText
                                          options:NSBackwardsSearch
                                            range:searchRange];
    if (previousRange.location != NSNotFound) {
      [self selectRange:previousRange];
    }
  }
}

- (void)selectNextOccurrence {
  if (!_textView)
    return;

  NSRange currentSelection = _textView.selectedRange;
  NSString* selectedText = [_textView.text substringWithRange:currentSelection];

  if (selectedText.length == 0)
    return;

  // Search forward from end of current selection
  NSRange searchRange = NSMakeRange(NSMaxRange(currentSelection),
                                    _textView.text.length - NSMaxRange(currentSelection));
  NSRange nextRange = [_textView.text rangeOfString:selectedText options:0 range:searchRange];

  if (nextRange.location != NSNotFound) {
    [self selectRange:nextRange];
  } else {
    // Wrap around to first occurrence
    searchRange = NSMakeRange(0, currentSelection.location);
    nextRange = [_textView.text rangeOfString:selectedText options:0 range:searchRange];
    if (nextRange.location != NSNotFound) {
      [self selectRange:nextRange];
    }
  }
}

#pragma mark - Selection Methods

- (void)selectRange:(NSRange)range {
  if (!_textView)
    return;

  [[ShikiPerformanceMonitor sharedMonitor] startMeasuring:ShikiPerformanceMetricSelectionChange
                                              description:@"Selection range change"];

  [[ShikiPerformanceMonitor sharedMonitor] addMetadata:@{
    @"range" : [NSValue valueWithRange:range],
    @"mode" : @(_selectionMode),
    @"isMultiSelect" : @(_allowsMultipleSelection)
  }
                                             forMetric:ShikiPerformanceMetricSelectionChange];

  // Save state before changing selection
  [_selectionHistory pushState:[ShikiSelectionState stateWithRange:_textView.selectedRange
                                                              mode:_selectionMode
                                                             scope:nil]];

  _lastSelection = range;
  _textView.selectedRange = range;
  [_textView scrollRangeToVisible:range];

  [[ShikiPerformanceMonitor sharedMonitor] stopMeasuring:ShikiPerformanceMetricSelectionChange];
}

- (void)expandSelection {
  if (!_textView)
    return;

  NSRange currentSelection = _textView.selectedRange;

  switch (_selectionMode) {
    case ShikiSelectionModeWord:
      [self expandWordSelection:[NSValue valueWithRange:currentSelection]];
      break;
    case ShikiSelectionModeLine:
      [self expandLineSelection:[NSValue valueWithRange:currentSelection]];
      break;
    case ShikiSelectionModeScope:
      [self expandScopeSelection:[NSValue valueWithRange:currentSelection]];
      break;
    default:
      break;
  }
}

- (void)shrinkSelection {
  if (!_textView)
    return;

  NSRange currentSelection = _textView.selectedRange;
  NSRange newRange = currentSelection;

  // Find the largest contained range
  for (NSValue* rangeValue in _mutableSelectableRanges) {
    NSRange range = [rangeValue rangeValue];
    if (NSLocationInRange(range.location, currentSelection) &&
        NSLocationInRange(NSMaxRange(range), currentSelection) &&
        range.length < currentSelection.length) {
      newRange = range;
    }
  }

  if (!NSEqualRanges(newRange, currentSelection)) {
    [self selectRange:newRange];
  }
}

#pragma mark - Scope Selection

- (void)selectScope:(NSString*)scope {
  NSArray<NSValue*>* ranges = _scopedRanges[scope];
  if (ranges.count > 0) {
    [self selectRange:[ranges.firstObject rangeValue]];
  }
}

- (void)selectEnclosingScope {
  if (!_textView)
    return;

  NSRange currentSelection = _textView.selectedRange;
  NSRange enclosingRange = currentSelection;

  // Find the smallest enclosing range
  for (NSValue* rangeValue in _mutableSelectableRanges) {
    NSRange range = [rangeValue rangeValue];
    if (NSLocationInRange(currentSelection.location, range) &&
        NSLocationInRange(NSMaxRange(currentSelection), range) &&
        range.length > currentSelection.length) {
      if (enclosingRange.length == currentSelection.length ||
          range.length < enclosingRange.length) {
        enclosingRange = range;
      }
    }
  }

  if (!NSEqualRanges(enclosingRange, currentSelection)) {
    [self selectRange:enclosingRange];
  }
}

#pragma mark - Word and Line Selection

- (void)selectWordAtLocation:(NSUInteger)location {
  if (!_textView || location >= _textView.text.length)
    return;

  NSString* text = _textView.text;

  // Find word boundaries
  NSCharacterSet* wordSet = [NSCharacterSet alphanumericCharacterSet];

  // Search backward for start
  NSInteger start = location;
  while (start > 0 && [wordSet characterIsMember:[text characterAtIndex:start - 1]]) {
    start--;
  }

  // Search forward for end
  NSInteger end = location;
  while (end < text.length && [wordSet characterIsMember:[text characterAtIndex:end]]) {
    end++;
  }

  [self selectRange:NSMakeRange(start, end - start)];
}

- (void)selectLineAtLocation:(NSUInteger)location {
  if (!_textView || location >= _textView.text.length)
    return;

  NSString* text = _textView.text;
  NSRange lineRange = [text lineRangeForRange:NSMakeRange(location, 0)];
  [self selectRange:lineRange];
}

#pragma mark - Selection State

- (void)saveSelectionState {
  if (_textView) {
    _lastSelection = _textView.selectedRange;
  }
}

- (void)restoreSelectionState {
  if (_textView && _preserveSelectionOnUpdate) {
    [self selectRange:_lastSelection];
  }
}

- (void)clearSelectionState {
  _lastSelection = NSMakeRange(0, 0);
  [_mutableSelectableRanges removeAllObjects];
  [_scopedRanges removeAllObjects];
}

- (void)updateSyntaxScopes:(NSArray<NSString*>*)scopes {
  _syntaxScopes = [scopes copy];
  [self updateSyntaxScopeRanges];
}

- (void)updateSyntaxScopeRanges {
  [_syntaxScopeRanges removeAllObjects];

  if (!_textView || !_syntaxScopes)
    return;

  NSAttributedString* text = _textView.attributedText;
  if (!text)
    return;

  [text enumerateAttributesInRange:NSMakeRange(0, text.length)
                           options:0
                        usingBlock:^(NSDictionary<NSAttributedStringKey, id>* _Nonnull attrs,
                                     NSRange range, BOOL* _Nonnull stop) {
                          NSString* scope = attrs[@"syntax_scope"];
                          if (scope) {
                            NSMutableArray* ranges =
                                [_syntaxScopeRanges[scope] mutableCopy] ?: [NSMutableArray new];
                            _syntaxScopeRanges[scope] = ranges;
                            [ranges addObject:[NSValue valueWithRange:range]];
                          }
                        }];
}

- (void)selectSyntaxScope:(NSString*)scope atLocation:(NSUInteger)location {
  NSArray<NSValue*>* ranges = _syntaxScopeRanges[scope];
  for (NSValue* rangeValue in ranges) {
    NSRange range = [rangeValue rangeValue];
    if (NSLocationInRange(location, range)) {
      [self selectRange:range];
      if (_onScopeSelected) {
        _onScopeSelected(scope, range);
      }
      break;
    }
  }
}

- (void)expandToSyntaxScope {
  if (!_textView)
    return;

  NSRange currentSelection = _textView.selectedRange;
  NSUInteger location = currentSelection.location;

  NSArray<NSString*>* scopes = [self getSyntaxScopesAtLocation:location];
  if (scopes.count == 0)
    return;

  // Find the smallest enclosing syntax scope
  NSRange bestRange = currentSelection;
  NSString* bestScope = nil;

  for (NSString* scope in scopes) {
    NSArray<NSValue*>* ranges = _syntaxScopeRanges[scope];
    for (NSValue* rangeValue in ranges) {
      NSRange range = [rangeValue rangeValue];
      if (NSLocationInRange(location, range) && (!bestScope || range.length < bestRange.length)) {
        bestRange = range;
        bestScope = scope;
      }
    }
  }

  if (bestScope && !NSEqualRanges(bestRange, currentSelection)) {
    [self selectRange:bestRange];
    if (_onScopeSelected) {
      _onScopeSelected(bestScope, bestRange);
    }
  }
}

- (NSArray<NSString*>*)getSyntaxScopesAtLocation:(NSUInteger)location {
  if (!_textView)
    return @[];

  NSMutableArray<NSString*>* scopes = [NSMutableArray new];

  [_syntaxScopeRanges
      enumerateKeysAndObjectsUsingBlock:^(NSString* scope, NSArray<NSValue*>* ranges, BOOL* stop) {
        for (NSValue* rangeValue in ranges) {
          if (NSLocationInRange(location, [rangeValue rangeValue])) {
            [scopes addObject:scope];
            break;
          }
        }
      }];

  return scopes;
}

- (void)addSelectionRange:(NSRange)range {
  if (!_allowsMultipleSelection) {
    [self selectRange:range];
    return;
  }

  // Add new range if it doesn't overlap with existing ones
  BOOL canAdd = YES;
  for (NSValue* existingRange in _mutableSelectedRanges) {
    if (NSIntersectionRange([existingRange rangeValue], range).length > 0) {
      canAdd = NO;
      break;
    }
  }

  if (canAdd) {
    [_mutableSelectedRanges addObject:[NSValue valueWithRange:range]];
    [self updateTextViewSelection];
  }
}

- (void)removeSelectionRange:(NSRange)range {
  [_mutableSelectedRanges removeObject:[NSValue valueWithRange:range]];
  [self updateTextViewSelection];
}

- (void)clearSelection {
  [_mutableSelectedRanges removeAllObjects];
  _primarySelection = NSMakeRange(0, 0);
  [self updateTextViewSelection];
}

- (BOOL)hasSelection {
  return _mutableSelectedRanges.count > 0;
}

- (NSRange)primarySelectedRange {
  return _primarySelection;
}

- (void)updateTextViewSelection {
  if (!_textView)
    return;

  if (_mutableSelectedRanges.count == 0) {
    _textView.selectedRange = NSMakeRange(0, 0);
    return;
  }

  if (!_allowsMultipleSelection) {
    _textView.selectedRange = [[_mutableSelectedRanges firstObject] rangeValue];
    return;
  }

  // For multiple selection, handle discontiguous selection
  if (_allowsMultipleSelection && _allowsDiscontiguousSelection) {
    NSMutableAttributedString* text =
        [[NSMutableAttributedString alloc] initWithAttributedString:_textView.attributedText];

    // Clear existing selection attributes
    [text removeAttribute:NSBackgroundColorAttributeName range:NSMakeRange(0, text.length)];

    // Apply selection highlighting
    UIColor* selectionColor = [UIColor colorWithRed:0.0 green:0.0 blue:1.0 alpha:0.2];
    UIColor* primarySelectionColor = [UIColor colorWithRed:0.0 green:0.0 blue:1.0 alpha:0.4];

    for (NSValue* rangeValue in _mutableSelectedRanges) {
      NSRange range = [rangeValue rangeValue];
      UIColor* color =
          NSEqualRanges(range, _primarySelection) ? primarySelectionColor : selectionColor;

      [text addAttribute:NSBackgroundColorAttributeName value:color range:range];
    }

    _textView.attributedText = text;
  }
}

#pragma mark - Search Support

- (NSArray<NSValue*>*)findAllOccurrences:(NSString*)text
                                 options:(NSRegularExpressionOptions)options {
  if (!_textView || !text.length)
    return @[];

  NSMutableArray<NSValue*>* results = [NSMutableArray new];
  NSString* content = _textView.text;

  NSError* error = nil;
  NSRegularExpression* regex = [NSRegularExpression regularExpressionWithPattern:text
                                                                         options:options
                                                                           error:&error];
  if (error) {
    NSLog(@"Search regex error: %@", error);
    return @[];
  }

  [regex
      enumerateMatchesInString:content
                       options:0
                         range:NSMakeRange(0, content.length)
                    usingBlock:^(NSTextCheckingResult* result, NSMatchingFlags flags, BOOL* stop) {
                      [results addObject:[NSValue valueWithRange:result.range]];
                    }];

  _searchResults = results;
  _currentSearchIndex = results.count > 0 ? 0 : -1;

  return results;
}

- (void)selectNextMatch {
  if (_searchResults.count == 0)
    return;

  _currentSearchIndex = (_currentSearchIndex + 1) % _searchResults.count;
  [self selectRange:[_searchResults[_currentSearchIndex] rangeValue]];
}

- (void)selectPreviousMatch {
  if (_searchResults.count == 0)
    return;

  _currentSearchIndex = (_currentSearchIndex - 1 + _searchResults.count) % _searchResults.count;
  [self selectRange:[_searchResults[_currentSearchIndex] rangeValue]];
}

- (void)selectAllMatches {
  if (_searchResults.count == 0)
    return;

  _allowsMultipleSelection = YES;
  [_mutableSelectedRanges removeAllObjects];
  [_mutableSelectedRanges addObjectsFromArray:_searchResults];
  [self updateTextViewSelection];
}

- (void)clearSearch {
  _searchResults = @[];
  _currentSearchIndex = -1;
}

#pragma mark - Undo/Redo Support

- (void)undo {
  ShikiSelectionState* state = [_selectionHistory undo];
  if (state) {
    _selectionMode = state.mode;
    _textView.selectedRange = state.range;
    [_textView scrollRangeToVisible:state.range];
  }
}

- (void)redo {
  ShikiSelectionState* state = [_selectionHistory redo];
  if (state) {
    _selectionMode = state.mode;
    _textView.selectedRange = state.range;
    [_textView scrollRangeToVisible:state.range];
  }
}

- (void)beginMultiSelection {
  _allowsMultipleSelection = YES;
  [self saveSelectionState];
}

- (void)endMultiSelection {
  _allowsMultipleSelection = NO;
  [self updateTextViewSelection];
}

- (void)toggleSelectionAtRange:(NSRange)range {
  if (!_allowsMultipleSelection) {
    [self selectRange:range];
    return;
  }

  NSValue* rangeValue = [NSValue valueWithRange:range];
  NSUInteger index = [_mutableSelectedRanges indexOfObject:rangeValue];

  if (index != NSNotFound) {
    [_mutableSelectedRanges removeObjectAtIndex:index];
  } else {
    [_mutableSelectedRanges addObject:rangeValue];
  }

  [self updateTextViewSelection];

  if (_onMultiSelectionChange) {
    _onMultiSelectionChange(_mutableSelectedRanges);
  }
}

- (BOOL)isRangeSelected:(NSRange)range {
  NSValue* rangeValue = [NSValue valueWithRange:range];
  return [_mutableSelectedRanges containsObject:rangeValue];
}

- (NSArray<NSString*>*)getScopesAtLocation:(NSUInteger)location {
  if (!_textView || location >= _textView.text.length) {
    return @[];
  }

  NSMutableArray<NSString*>* scopes = [NSMutableArray new];
  NSAttributedString* text = _textView.attributedText;

  [text enumerateAttributesInRange:NSMakeRange(location, 1)
                           options:0
                        usingBlock:^(NSDictionary<NSAttributedStringKey, id>* _Nonnull attrs,
                                     NSRange range, BOOL* _Nonnull stop) {
                          NSString* scope = attrs[@"syntax_scope"];
                          if (scope) {
                            [scopes addObject:scope];
                          }
                        }];

  return [scopes copy];
}

- (void)expandSelectionToLine {
  if (!_textView)
    return;

  NSRange currentSelection = _textView.selectedRange;
  NSString* text = _textView.text;

  // Get the line range containing the selection
  NSRange lineRange = [text lineRangeForRange:currentSelection];

  // If already selecting full line(s), expand to include next line
  if (NSEqualRanges(lineRange, currentSelection)) {
    NSUInteger nextLineStart = NSMaxRange(lineRange);
    if (nextLineStart < text.length) {
      NSRange nextLineRange = [text lineRangeForRange:NSMakeRange(nextLineStart, 0)];
      lineRange = NSUnionRange(lineRange, nextLineRange);
    }
  }

  [self selectRange:lineRange];
}

@end

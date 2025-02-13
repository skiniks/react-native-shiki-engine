#import "ShikiSelectionTypes.h"
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@class ShikiTextView;
@class ShikiSelectionHistory;

@interface ShikiSelectionManager : NSObject

@property(nonatomic, weak) ShikiTextView* textView;
@property(nonatomic, assign) ShikiSelectionMode selectionMode;
@property(nonatomic, strong, readonly) NSArray<NSValue*>* selectableRanges;
@property(nonatomic, assign) BOOL preserveSelectionOnUpdate;
@property(nonatomic, strong) NSArray<NSString*>* syntaxScopes;
@property(nonatomic, copy, nullable) void (^onScopeSelected)(NSString* scope, NSRange range);
@property(nonatomic, assign) BOOL allowsMultipleSelection;
@property(nonatomic, strong, readonly) NSArray<NSValue*>* selectedRanges;
@property(nonatomic, strong, readonly) ShikiSelectionHistory* selectionHistory;
@property(nonatomic, assign) BOOL allowsDiscontiguousSelection;
@property(nonatomic, copy, nullable) void (^onMultiSelectionChange)(NSArray<NSValue*>* ranges);

- (instancetype)initWithTextView:(ShikiTextView*)textView;

// Selection methods
- (void)selectRange:(NSRange)range;
- (void)expandSelection;
- (void)shrinkSelection;
- (void)selectNextOccurrence;
- (void)selectPreviousOccurrence;
- (void)selectAllOccurrences;

// Scope-based selection
- (void)selectScope:(NSString*)scope;
- (void)selectEnclosingScope;
- (NSArray<NSString*>*)getScopesAtLocation:(NSUInteger)location;

// Word and line selection
- (void)selectWordAtLocation:(NSUInteger)location;
- (void)selectLineAtLocation:(NSUInteger)location;
- (void)expandSelectionToLine;

// Selection state
- (void)saveSelectionState;
- (void)restoreSelectionState;
- (void)clearSelectionState;

// Syntax-aware selection
- (void)updateSyntaxScopes:(NSArray<NSString*>*)scopes;
- (void)selectSyntaxScope:(NSString*)scope atLocation:(NSUInteger)location;
- (void)expandToSyntaxScope;
- (NSArray<NSString*>*)getSyntaxScopesAtLocation:(NSUInteger)location;

// Multi-selection methods
- (void)addSelectionRange:(NSRange)range;
- (void)removeSelectionRange:(NSRange)range;
- (void)clearSelection;
- (BOOL)hasSelection;
- (NSRange)primarySelectedRange;

// Search methods
- (NSArray<NSValue*>*)findAllOccurrences:(NSString*)text
                                 options:(NSRegularExpressionOptions)options;
- (void)selectNextMatch;
- (void)selectPreviousMatch;
- (void)selectAllMatches;
- (void)clearSearch;

// Undo/Redo methods
- (void)undo;
- (void)redo;

// New methods
- (void)beginMultiSelection;
- (void)endMultiSelection;
- (void)toggleSelectionAtRange:(NSRange)range;
- (BOOL)isRangeSelected:(NSRange)range;

@end

NS_ASSUME_NONNULL_END

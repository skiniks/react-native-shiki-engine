#pragma once

#import "../views/ShikiViewState.h"
#import <React/RCTViewComponentView.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@class ShikiViewState;
@class ShikiUpdateCoordinator;
@class VirtualizedContentManager;

@interface RCTShikiHighlighterComponentView : RCTViewComponentView

- (void)setSelection:(NSRange)range;
- (NSRange)getSelection;
- (void)scrollToLine:(NSInteger)line;
- (void)selectScope:(NSString *)scope;
- (void)selectWord:(NSInteger)location;

@end

NS_ASSUME_NONNULL_END

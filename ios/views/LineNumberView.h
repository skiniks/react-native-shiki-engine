#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface LineNumberView : UIView

@property(nonatomic, strong) UIFont* font;
@property(nonatomic, strong) UIColor* textColor;
@property(nonatomic, strong) UIColor* backgroundColor;
@property(nonatomic, assign) CGFloat lineHeight;
@property(nonatomic, assign) NSUInteger numberOfLines;
@property(nonatomic, assign) CGFloat padding;

- (void)updateLineNumbers;
- (CGFloat)requiredWidth;

@end

NS_ASSUME_NONNULL_END

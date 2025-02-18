#import <React/RCTLog.h>
#import <React/RCTUIManager.h>
#import <React/RCTViewManager.h>

@interface RTNTestView : UIView

@property (nonatomic, copy) NSString *text;
@property (nonatomic, strong) UILabel *label;

@end

@implementation RTNTestView

- (instancetype)init
{
    self = [super init];
    if (self) {
        self.backgroundColor = [UIColor clearColor];

        _label = [[UILabel alloc] init];
        _label.text = @"RTNTestView is working!";
        [self addSubview:_label];

        _label.translatesAutoresizingMaskIntoConstraints = false;
        [NSLayoutConstraint activateConstraints:@[
            [_label.leadingAnchor constraintEqualToAnchor:self.leadingAnchor],
            [_label.topAnchor constraintEqualToAnchor:self.topAnchor],
            [_label.trailingAnchor constraintEqualToAnchor:self.trailingAnchor],
            [_label.bottomAnchor constraintEqualToAnchor:self.bottomAnchor],
        ]];

        _label.textAlignment = NSTextAlignmentCenter;
    }
    return self;
}

- (void)setText:(NSString *)text
{
    _text = [text copy];
    _label.text = _text;
}

@end

@interface RTNTestViewManager : RCTViewManager
@end

@implementation RTNTestViewManager

RCT_EXPORT_MODULE(RTNTestView)

RCT_EXPORT_VIEW_PROPERTY(text, NSString)

- (UIView *)view
{
    return [[RTNTestView alloc] init];
}

@end

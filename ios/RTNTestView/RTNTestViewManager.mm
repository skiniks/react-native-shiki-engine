#import "RTNTestView.h"
#import <React/RCTLog.h>
#import <React/RCTUIManager.h>
#import <React/RCTViewManager.h>

@interface RTNTestView : UIView

@property(nonatomic, copy) NSString *text;
@property(nonatomic, strong) UIColor *textColor;
@property(nonatomic, copy) NSString *fontStyle;
@property(nonatomic, strong) UILabel *label;

@end

@implementation RTNTestView

- (instancetype)init {
  self = [super init];
  if (self) {
    _label = [[UILabel alloc] init];
    [self addSubview:_label];

    _label.translatesAutoresizingMaskIntoConstraints = false;
    [NSLayoutConstraint activateConstraints:@[
      [_label.leadingAnchor constraintEqualToAnchor:self.leadingAnchor],
      [_label.topAnchor constraintEqualToAnchor:self.topAnchor],
      [_label.trailingAnchor constraintEqualToAnchor:self.trailingAnchor],
      [_label.bottomAnchor constraintEqualToAnchor:self.bottomAnchor],
    ]];

    _label.textAlignment = NSTextAlignmentCenter;
    _label.textColor = [UIColor blackColor];
  }
  return self;
}

- (void)setText:(NSString *)text {
  _text = [text copy];
  _label.text = _text;
}

- (void)setTextColor:(UIColor *)textColor {
  _textColor = textColor;
  _label.textColor = _textColor;
}

- (void)setFontStyle:(NSString *)fontStyle {
  _fontStyle = [fontStyle copy];
  UIFont *currentFont = _label.font;
  CGFloat fontSize = currentFont.pointSize;
  UIFont *newFont = currentFont;

  if ([_fontStyle isEqualToString:@"italic"]) {
    UIFontDescriptor *descriptor = [currentFont.fontDescriptor
        fontDescriptorWithSymbolicTraits:UIFontDescriptorTraitItalic];
    if (descriptor) {
      newFont = [UIFont fontWithDescriptor:descriptor size:fontSize];
    }
  }

  _label.font = newFont;
}

@end

@interface RTNTestViewManager : RCTViewManager
@end

@implementation RTNTestViewManager

RCT_EXPORT_MODULE(RTNTestView)

- (UIView *)view {
  return [[RTNTestView alloc] init];
}

RCT_EXPORT_VIEW_PROPERTY(text, NSString)
RCT_EXPORT_VIEW_PROPERTY(textColor, UIColor)
RCT_EXPORT_VIEW_PROPERTY(fontStyle, NSString)

@end

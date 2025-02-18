#import "RTNTestView.h"

#import <React/RCTConversions.h>

#import <react/renderer/components/RNShikiSpec/ComponentDescriptors.h>
#import <react/renderer/components/RNShikiSpec/EventEmitters.h>
#import <react/renderer/components/RNShikiSpec/Props.h>
#import <react/renderer/components/RNShikiSpec/RCTComponentViewHelpers.h>

#import "RCTFabricComponentsPlugins.h"

using namespace facebook::react;

@interface RTNTestViewComponentView () <RCTRTNTestViewViewProtocol>
@end

@implementation RTNTestViewComponentView {
  UILabel *_label;
}

+ (ComponentDescriptorProvider)componentDescriptorProvider {
  return concreteComponentDescriptorProvider<RTNTestViewComponentDescriptor>();
}

- (instancetype)initWithFrame:(CGRect)frame {
  if (self = [super initWithFrame:frame]) {
    static const auto defaultProps = std::make_shared<const RTNTestViewProps>();
    _props = defaultProps;

    _label = [[UILabel alloc] init];
    _label.text = @"Initial value";
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

- (void)updateProps:(Props::Shared const &)props
           oldProps:(Props::Shared const &)oldProps {
  const auto &oldViewProps =
      *std::static_pointer_cast<RTNTestViewProps const>(_props);
  const auto &newViewProps =
      *std::static_pointer_cast<RTNTestViewProps const>(props);

  if (oldViewProps.text != newViewProps.text) {
    NSString *text =
        [[NSString alloc] initWithUTF8String:newViewProps.text.c_str()];
    _label.text = text;
  }

  if (oldViewProps.textColor != newViewProps.textColor) {
    if (newViewProps.textColor) {
      _label.textColor = RCTUIColorFromSharedColor(newViewProps.textColor);
    } else {
      _label.textColor = [UIColor blackColor];
    }
  }

  if (oldViewProps.fontStyle != newViewProps.fontStyle) {
    UIFontDescriptor *descriptor = _label.font.fontDescriptor;
    if (newViewProps.fontStyle == RTNTestViewFontStyle::Italic) {
      descriptor = [descriptor
          fontDescriptorWithSymbolicTraits:UIFontDescriptorTraitItalic];
    } else {
      descriptor = [descriptor fontDescriptorWithSymbolicTraits:0];
    }
    if (descriptor) {
      _label.font = [UIFont fontWithDescriptor:descriptor
                                          size:_label.font.pointSize];
    }
  }

  [super updateProps:props oldProps:oldProps];
}

@end

Class<RCTComponentViewProtocol> RTNTestViewCls(void) {
  return RTNTestViewComponentView.class;
}

#pragma mark - RCTFabricComponentsPlugins

@implementation RTNTestViewComponentView (RCTFabricComponentsPlugins)

- (NSString *)componentViewName {
  return @"RTNTestView";
}

@end

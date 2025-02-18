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

+ (ComponentDescriptorProvider)componentDescriptorProvider
{
    return concreteComponentDescriptorProvider<RTNTestViewComponentDescriptor>();
}

- (instancetype)initWithFrame:(CGRect)frame
{
    if (self = [super initWithFrame:frame]) {
        static const auto defaultProps = std::make_shared<const RTNTestViewProps>();
        _props = defaultProps;

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

- (void)updateProps:(Props::Shared const &)props oldProps:(Props::Shared const &)oldProps
{
    const auto &oldViewProps = *std::static_pointer_cast<RTNTestViewProps const>(_props);
    const auto &newViewProps = *std::static_pointer_cast<RTNTestViewProps const>(props);

    if (oldViewProps.text != newViewProps.text) {
        _label.text = [[NSString alloc] initWithCString:newViewProps.text.c_str() encoding:NSASCIIStringEncoding];
    }

    [super updateProps:props oldProps:oldProps];
}

@end

Class<RCTComponentViewProtocol> RTNTestViewCls(void)
{
    return RTNTestViewComponentView.class;
}

#pragma mark - RCTFabricComponentsPlugins

@implementation RTNTestViewComponentView (RCTFabricComponentsPlugins)

- (NSString *)componentViewName
{
    return @"RTNTestView";
}

@end

#import "../cpp/fabric/ShikiHighlighterComponent.h"
#import "RCTShikiHighlighterComponentView.h"

using namespace facebook::react;

Class<RCTComponentViewProtocol> RCTShikiHighlighterCls(void) {
  return RCTShikiHighlighterComponentView.class;
}

ComponentDescriptorProvider RCTShikiHighlighterDescriptorProvider(void) {
  return concreteComponentDescriptorProvider<
      ShikiHighlighterComponentDescriptor>();
}

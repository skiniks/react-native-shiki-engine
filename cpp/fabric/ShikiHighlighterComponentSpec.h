#pragma once

#include <react/renderer/components/view/ConcreteViewShadowNode.h>
#include <react/renderer/components/view/ViewEventEmitter.h>
#include <react/renderer/components/view/ViewProps.h>
#include <react/renderer/core/ConcreteComponentDescriptor.h>
#include <react/renderer/core/PropsParserContext.h>
#include <string>

namespace facebook::react {

// Define component name as a constexpr char array
extern const char ShikiHighlighterComponentName[];

class ShikiHighlighterProps final : public ViewProps {
public:
  ShikiHighlighterProps() = default;
  ShikiHighlighterProps(const PropsParserContext& context, const ShikiHighlighterProps& sourceProps,
                        const RawProps& rawProps);
  ShikiHighlighterProps(const ShikiHighlighterProps& sourceProps, const RawProps& rawProps);

  float fontSize{14.0f};
  std::string fontFamily{"Menlo"};
  std::string fontWeight{"normal"};
  std::string fontStyle{"normal"};
  bool showLineNumbers{true};
  std::string text{""};
  std::string language{"plaintext"};
  std::string theme{"github-light"};
};

using ShikiHighlighterEventEmitter = ViewEventEmitter;

class ShikiHighlighterShadowNode;

using ConcreteShikiHighlighterShadowNode =
    ConcreteViewShadowNode<ShikiHighlighterComponentName, ShikiHighlighterProps>;

class ShikiHighlighterShadowNode final : public ConcreteShikiHighlighterShadowNode {
  using ConcreteShikiHighlighterShadowNode::ConcreteShikiHighlighterShadowNode;

public:
  using Shared = std::shared_ptr<const ShikiHighlighterShadowNode>;
};

class JSI_EXPORT ShikiHighlighterComponentDescriptor final
    : public ConcreteComponentDescriptor<ShikiHighlighterShadowNode> {
  using ConcreteComponentDescriptor::ConcreteComponentDescriptor;

public:
  ComponentHandle getComponentHandle() const {
    return reinterpret_cast<ComponentHandle>(this);
  }

  ComponentName getComponentName() const {
    return ShikiHighlighterComponentName;
  }
};

} // namespace facebook::react

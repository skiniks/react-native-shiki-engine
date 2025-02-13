#include "ShikiHighlighterComponent.h"

namespace facebook::react {

ComponentHandle ShikiHighlighterComponentDescriptor::getComponentHandle() const {
  return reinterpret_cast<ComponentHandle>(this);
}

ComponentName ShikiHighlighterComponentDescriptor::getComponentName() const {
  return ShikiHighlighterComponentName;
}

} // namespace facebook::react

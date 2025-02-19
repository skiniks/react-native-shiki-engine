#pragma once
#include <string>

namespace shiki {

struct PlatformViewConfig {
  std::string language;
  std::string theme;
  std::string code;
  float fontSize{14.0f};
  bool showLineNumbers{false};
  std::string fontFamily;
  std::string fontWeight;
  std::string fontStyle;
  bool scrollEnabled{true};
  bool selectable{true};
};

}  // namespace shiki

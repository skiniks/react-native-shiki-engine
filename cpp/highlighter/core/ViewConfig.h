#pragma once

#include <string>

namespace shiki {

struct ViewConfig {
  float fontSize = 14.0f;
  std::string fontFamily = "Menlo";
  std::string fontWeight = "regular";
  std::string fontStyle = "normal";
  bool showLineNumbers = false;
  bool scrollEnabled = true;
  bool selectable = true;
};

} // namespace shiki

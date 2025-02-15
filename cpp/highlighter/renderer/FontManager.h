#pragma once
#include <memory>
#include <string>

#include "../core/Constants.h"

#ifdef __APPLE__
#  import <UIKit/UIKit.h>
#endif

namespace shiki {

struct FontConfig {
  float fontSize{14.0f};
  std::string fontFamily;
  std::string fontWeight;
  std::string fontStyle;
  bool bold{false};
  bool italic{false};
};

class FontManager {
 public:
  static FontManager& getInstance();

#ifdef __APPLE__
  UIFont* createFont(const FontConfig& config);
  UIFont* createFontWithStyle(UIFont* baseFont, bool bold, bool italic);
#endif

 private:
  FontManager() = default;
  FontManager(const FontManager&) = delete;
  FontManager& operator=(const FontManager&) = delete;

  // Platform-specific font name mapping
  std::string mapFontWeight(const std::string& weight);
  std::string mapFontStyle(const std::string& style);
};

}  // namespace shiki

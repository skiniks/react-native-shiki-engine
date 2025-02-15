#pragma once
#include <rapidjson/document.h>

#include <memory>
#include <string>
#include <vector>

#include "../text/TokenRange.h"
#include "Theme.h"
#include "ThemeStyle.h"

namespace shiki {

class Theme;

class ThemeParser {
 public:
  explicit ThemeParser(Theme* theme) : theme_(theme) {}

  // Parse simple theme style
  ThemeStyle parseThemeStyle(const std::string& themeContent);

  // Parse complete theme
  static std::shared_ptr<Theme> parseTheme(const std::string& name, const std::string& content);
  static std::shared_ptr<Theme> parseThemeFromObject(const rapidjson::Value& themeJson);

  // Other parsing methods
  std::vector<TokenRange> tokenize(const std::string& content);
  static ThemeStyle parseStyle(const std::string& json);
  static std::vector<ThemeStyle> parseStyles(const std::string& json);

 private:
  Theme* theme_;
  friend class Theme;
};

}  // namespace shiki

#pragma once
#include <rapidjson/document.h>

#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../core/Constants.h"
#include "../tokenizer/Token.h"
#include "ThemeColor.h"
#include "ThemeStyle.h"

namespace shiki {

// Forward declarations
class ThemeParser;

struct ThemeRule {
  std::string scope;
  std::vector<std::string> additionalScopes;
  ThemeStyle style;
};

class Theme {
 public:
  explicit Theme(const std::string& name = "") : name(name), background_(""), foreground_("") {}

  std::string name;
  std::string type = "dark";  // "dark" or "light"
  std::vector<ThemeRule> rules;
  std::vector<ThemeColor> colors;  // Editor colors (e.g. background, foreground)
  std::unordered_map<std::string, std::string> colorReplacements;

  // Style management
  void addStyle(const ThemeStyle& style);
  const ThemeStyle* findStyle(const std::string& scope) const;
  ThemeStyle resolveStyle(const std::string& scope) const;
  bool applyStyle(Token& token) const;

  // Color management
  void setBackground(const ThemeColor& color) {
    background_ = color;
  }
  void setForeground(const ThemeColor& color) {
    foreground_ = color;
  }
  const ThemeColor& getBackground() const {
    return background_;
  }
  const ThemeColor& getForeground() const {
    return foreground_;
  }

  // Style utilities
  static uint32_t parseColor(const std::string& hexColor);
  static uint32_t getFontStyle(const std::string& fontStyle);

  // Cache management
  void clearCache();

  ThemeColor getColor(const std::string& name) const;

  // Add semantic token support
  void addSemanticTokenRule(const std::string& token, const ThemeStyle& style);
  ThemeStyle resolveSemanticToken(const std::string& token) const;

  ThemeStyle getLineNumberStyle() const;

  static std::shared_ptr<Theme> fromJson(const std::string& content);

 private:
  friend class ThemeParser;

  // Find most specific matching rule for a scope
  const ThemeRule* findBestMatchingRule(const std::string& scope) const;

  // Get all parent scopes for inheritance
  std::vector<std::string> getParentScopes(const std::string& scope) const;

  // Merge styles from parent scopes
  ThemeStyle mergeParentStyles(const std::vector<std::string>& scopes) const;

  ThemeColor background_;
  ThemeColor foreground_;
  std::vector<ThemeStyle> styles_;
  mutable std::unordered_map<std::string, ThemeStyle> styleCache_;
  std::map<std::string, ThemeStyle> semanticTokens_;
};

}  // namespace shiki

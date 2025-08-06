#pragma once
#include <rapidjson/document.h>

#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "ThemeColor.h"
#include "ThemeStyle.h"
// Configuration now handled by shikitori
#include "highlighter/core/Constants.h"
// Grammar and Token dependencies removed

namespace shiki {

class ThemeParser;

struct ThemeRule {
  std::string scope;
  std::vector<std::string> additionalScopes;
  ThemeStyle style;
};

class Theme {
 public:
  enum class MatchMode { Strict, Standard, Relaxed };

  Theme() {}
  explicit Theme(const std::string& name) : name(name) {}
  Theme(const Theme& other);
  Theme& operator=(const Theme& other);
  virtual ~Theme() = default;

  void addStyle(const ThemeStyle& style);
  const ThemeStyle* findStyle(const std::string& scope) const;
  ThemeStyle resolveStyle(const std::string& scope) const;
  // applyStyle method removed - tokens handled by shikitori

  // Pattern management now handled by shikitori

  const std::string& getName() const {
    return name;
  }
  const std::string& getScopeName() const {
    return scopeName;
  }
  // Pattern access methods removed - handled by shikitori

  size_t getRuleCount() const {
    return rules.size();
  }
  const ThemeRule& getRuleAt(size_t index) const {
    return rules[index];
  }
  const std::vector<ThemeRule>& getRules() const {
    return rules;
  }

  uint32_t getFontStyle(const std::string& scope) const;
  ThemeStyle getStyle(const std::string& scope) const;

  // Configuration now handled by shikitori

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

  static uint32_t parseColor(const std::string& hexColor);

  void clearCache();

  ThemeColor getColor(const std::string& name) const;

  void addSemanticTokenRule(const std::string& token, const ThemeStyle& style);
  ThemeStyle resolveSemanticToken(const std::string& token) const;

  ThemeStyle getLineNumberStyle() const;

  static std::shared_ptr<Theme> fromJson(const std::string& content);

  static std::vector<std::string> splitScope(const std::string& scope);
  static std::vector<std::string> getSegments(const std::string& scope);
  static bool
  isScopeMatch(const std::string& ruleScope, const std::string& tokenScope, MatchMode mode = MatchMode::Standard);
  static size_t calculateScopeSpecificity(const std::string& ruleScope, const std::string& tokenScope);

  // Helper function to match a selector against a scope path using TextMate rules
  static bool isPathMatch(const std::string& selector, const std::string& scopePath, MatchMode mode);

  // Helper function to calculate specificity for a path match
  static size_t calculatePathSpecificity(const std::string& selector, const std::string& scopePath);

  std::string name;
  std::string type{"dark"};
  std::string scopeName;
  // Patterns now handled by shikitori

 private:
  friend class ThemeParser;

  const ThemeRule* findBestMatchingRule(const std::string& scope) const;

  std::vector<std::string> getParentScopes(const std::string& scope) const;

  ThemeStyle mergeParentStyles(const std::vector<std::string>& scopes) const;

  ThemeColor background_;
  ThemeColor foreground_;
  std::vector<ThemeRule> rules;
  std::vector<ThemeColor> colors;
  std::unordered_map<std::string, std::string> colorReplacements;
  std::unordered_map<std::string, ThemeStyle> styles_;
  mutable std::unordered_map<std::string, ThemeStyle> styleCache_;
  std::map<std::string, ThemeStyle> semanticTokens_;
  // Configuration removed - handled by shikitori
};

}  // namespace shiki

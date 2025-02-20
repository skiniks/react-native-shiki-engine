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
#include "highlighter/core/Configuration.h"
#include "highlighter/core/Constants.h"
#include "highlighter/grammar/Grammar.h"
#include "highlighter/tokenizer/Token.h"

namespace shiki {

class ThemeParser;

struct ThemeRule {
  std::string scope;
  std::vector<std::string> additionalScopes;
  ThemeStyle style;
};

class Theme {
 public:
  Theme() : configuration_(&Configuration::getInstance()) {}
  explicit Theme(const std::string& name) : name(name), configuration_(&Configuration::getInstance()) {}
  virtual ~Theme() = default;

  void addStyle(const ThemeStyle& style);
  const ThemeStyle* findStyle(const std::string& scope) const;
  ThemeStyle resolveStyle(const std::string& scope) const;
  bool applyStyle(Token& token) const;

  void addPattern(const GrammarPattern& pattern) {
    patterns.push_back(pattern);
  }

  const std::string& getName() const {
    return name;
  }
  const std::string& getScopeName() const {
    return scopeName;
  }
  const std::vector<GrammarPattern>& getPatterns() const {
    return patterns;
  }
  std::vector<GrammarPattern>& getPatterns() {
    return patterns;
  }

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

  void setConfiguration(std::shared_ptr<Configuration> config) {
    configuration_ = std::move(config);
  }

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

  std::string name;
  std::string type{"dark"};
  std::string scopeName;
  std::vector<GrammarPattern> patterns;

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
  std::shared_ptr<Configuration> configuration_;
};

}  // namespace shiki

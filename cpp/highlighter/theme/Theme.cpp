#include "Theme.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "ScopeMatcher.h"
#include "ThemeParser.h"
#include "highlighter/tokenizer/Token.h"

namespace shiki {

Theme::Theme(const Theme& other)
  : name(other.name),
    type(other.type),
    scopeName(other.scopeName),
    patterns(other.patterns),
    background_(other.background_),
    foreground_(other.foreground_),
    rules(other.rules),
    colors(other.colors),
    colorReplacements(other.colorReplacements),
    styles_(other.styles_),
    styleCache_(other.styleCache_),
    semanticTokens_(other.semanticTokens_),
    configuration_(other.configuration_) {}

Theme& Theme::operator=(const Theme& other) {
  if (this != &other) {
    name = other.name;
    type = other.type;
    scopeName = other.scopeName;
    patterns = other.patterns;
    background_ = other.background_;
    foreground_ = other.foreground_;
    rules = other.rules;
    colors = other.colors;
    colorReplacements = other.colorReplacements;
    styles_ = other.styles_;
    styleCache_ = other.styleCache_;
    semanticTokens_ = other.semanticTokens_;
    configuration_ = other.configuration_;
  }
  return *this;
}

std::shared_ptr<Theme> Theme::fromJson(const std::string& content) {
  try {
    rapidjson::Document doc;
    if (doc.Parse(content.c_str()).HasParseError()) {
      return nullptr;
    }
    return ThemeParser::parseThemeFromObject(doc);
  } catch (...) {
    return nullptr;
  }
}

uint32_t Theme::parseColor(const std::string& hexColor) {
  if (hexColor.empty() || hexColor[0] != '#') {
    return 0;
  }

  std::string hex = hexColor.substr(1);
  uint32_t color = 0;

  try {
    std::stringstream ss;
    ss << std::hex << hex;
    ss >> color;

    if (hex.length() == 6) {
      color = (0xFF << 24) | color;
    } else if (hex.length() == 8) {
      uint32_t alpha = color & 0xFF;
      color = color >> 8;
      color = (alpha << 24) | color;
    }
  } catch (...) {
    return 0;
  }

  return color;
}

uint32_t Theme::getFontStyle(const std::string& scope) const {
  auto style = getStyle(scope);
  if (!style.hasProperties()) {
    return configuration_->getDefaults().defaultFontStyle == "normal" ? style::FONT_NORMAL : style::FONT_BOLD;
  }

  uint32_t fontStyle = style::FONT_NORMAL;
  if (style.bold) fontStyle |= style::FONT_BOLD;
  if (style.italic) fontStyle |= style::FONT_ITALIC;
  if (style.underline) fontStyle |= style::FONT_UNDERLINE;
  return fontStyle;
}

ThemeStyle Theme::getStyle(const std::string& scope) const {
  auto it = styles_.find(scope);
  if (it == styles_.end()) {
    if (configuration_->getDefaults().throwOnMissingColors) {
      throw std::runtime_error("No style found for scope: " + scope);
    }
    return ThemeStyle();
  }
  return it->second;
}

void Theme::addSemanticTokenRule(const std::string& token, const ThemeStyle& style) {
  semanticTokens_[token] = style;
}

ThemeStyle Theme::resolveSemanticToken(const std::string& token) const {
  auto it = semanticTokens_.find(token);
  if (it != semanticTokens_.end()) {
    return it->second;
  }
  return ThemeStyle();  // Return default style
}

void Theme::addStyle(const ThemeStyle& style) {
  ThemeRule rule;
  rule.style = style;
  rule.scope = style.scope;
  rules.push_back(rule);
}

const ThemeStyle* Theme::findStyle(const std::string& scope) const {
  for (const auto& rule : rules) {
    if (rule.scope == scope) {
      return &rule.style;
    }
  }
  return nullptr;
}

void Theme::clearCache() {
  styleCache_.clear();
}

ThemeColor Theme::getColor(const std::string& name) const {
  // If it's a hex color, create a ThemeColor directly
  if (!name.empty() && name[0] == '#') {
    return *ThemeColor::fromHex(name);
  }

  // If it's a named color from the theme, find it in the colors array
  for (const auto& color : colors) {
    if (color.hexColor == name) {
      return color;
    }
  }

  // If not found, return foreground color
  return foreground_;
}

ThemeStyle Theme::getLineNumberStyle() const {
  // Look for line number specific style in theme
  for (const auto& rule : rules) {
    if (rule.scope == "line-number") {
      return rule.style;
    }
  }

  // If no line number style specified, use foreground color with reduced opacity
  ThemeStyle style;
  style.color = foreground_.toHex();
  return style;
}

ThemeStyle Theme::resolveStyle(const std::string& scope) const {
  for (const auto& rule : rules) {
    if (rule.scope == scope) {
      return rule.style;
    }
  }

  const ThemeRule* bestMatch = nullptr;
  size_t bestSpecificity = 0;

  for (const auto& rule : rules) {
    if (ScopeMatcher::isScopeMatch(rule.scope, scope)) {
      size_t specificity = ScopeMatcher::calculateScopeSpecificity(rule.scope, scope);
      if (!bestMatch || specificity > bestSpecificity) {
        bestMatch = &rule;
        bestSpecificity = specificity;
      }
    }
  }

  if (bestMatch) {
    return bestMatch->style;
  }

  auto scopeParts = ScopeMatcher::splitScope(scope);
  for (const auto& scopePart : scopeParts) {
    for (const auto& rule : rules) {
      if (ScopeMatcher::isScopeMatch(rule.scope, scopePart)) {
        return rule.style;
      }
    }
  }

  ThemeStyle defaultStyle;
  defaultStyle.color = foreground_.toHex();
  return defaultStyle;
}

std::vector<std::string> Theme::getParentScopes(const std::string& scope) const {
  std::vector<std::string> parents;

  // Split scope into parts
  std::vector<std::string> parts;
  std::istringstream stream(scope);
  std::string part;

  while (std::getline(stream, part, ' ')) {
    // For each space-separated part, get dot-separated parents
    std::string current;
    std::istringstream dotStream(part);
    std::string segment;

    while (std::getline(dotStream, segment, '.')) {
      if (!current.empty()) {
        current += ".";
      }
      current += segment;
      parents.push_back(current);
    }
  }

  // Sort from least to most specific
  std::sort(parents.begin(), parents.end(), [](const std::string& a, const std::string& b) {
    return a.length() < b.length();
  });

  // Add the full scope at the end (most specific)
  if (!scope.empty() && (parents.empty() || parents.back() != scope)) {
    parents.push_back(scope);
  }

  return parents;
}

ThemeStyle Theme::mergeParentStyles(const std::vector<std::string>& scopes) const {
  ThemeStyle result;

  // Start with default foreground color
  result.color = foreground_.toHex();

  // Apply styles from least to most specific
  for (const auto& scope : scopes) {
    const ThemeRule* rule = findBestMatchingRule(scope);
    if (rule) {
      // Only override non-empty properties
      if (!rule->style.color.empty()) {
        result.color = rule->style.color;
      }
      if (!rule->style.backgroundColor.empty()) {
        result.backgroundColor = rule->style.backgroundColor;
      }
      if (!rule->style.fontStyle.empty()) {
        result.fontStyle = rule->style.fontStyle;
      }
      result.bold |= rule->style.bold;
      result.italic |= rule->style.italic;
      result.underline |= rule->style.underline;
    }
  }

  return result;
}

const ThemeRule* Theme::findBestMatchingRule(const std::string& scope) const {
  const ThemeRule* bestMatch = nullptr;
  size_t bestSpecificity = 0;

  for (const auto& rule : rules) {
    if (ScopeMatcher::isScopeMatch(rule.scope, scope)) {
      size_t specificity = ScopeMatcher::calculateScopeSpecificity(rule.scope, scope);
      if (!bestMatch || specificity > bestSpecificity) {
        bestMatch = &rule;
        bestSpecificity = specificity;
      }
    }
  }

  return bestMatch;
}

bool Theme::applyStyle(Token& token) const {
  const ThemeRule* bestMatch = nullptr;
  size_t bestSpecificity = 0;

  for (const auto& rule : rules) {
    // Try each scope in the token's scope list
    for (const auto& tokenScope : token.scopes) {
      if (ScopeMatcher::isScopeMatch(rule.scope, tokenScope)) {
        size_t specificity = ScopeMatcher::calculateScopeSpecificity(rule.scope, tokenScope);
        if (!bestMatch || specificity > bestSpecificity) {
          bestMatch = &rule;
          bestSpecificity = specificity;
        }
      }
    }
  }

  if (bestMatch) {
    token.style = bestMatch->style;
    return true;
  }

  return false;
}

}  // namespace shiki

#include "Theme.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>

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
  if (style.scope.empty()) {
    return;
  }

  ThemeRule rule;
  rule.style = style;
  rule.scope = style.scope;

  for (auto& existingRule : rules) {
    if (existingRule.scope == style.scope) {
      existingRule.style.applySettings(style);
      return;
    }
  }

  rules.push_back(rule);

  styles_[style.scope] = style;

  clearCache();
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
  style.foreground = foreground_.toHex();
  return style;
}

std::vector<std::string> Theme::splitScope(const std::string& scope) {
  std::vector<std::string> parts;
  std::istringstream stream(scope);
  std::string part;

  // Split by spaces (for composite scopes)
  while (std::getline(stream, part, ' ')) {
    if (!part.empty()) {
      parts.push_back(part);
    }
  }

  return parts;
}

std::vector<std::string> Theme::getSegments(const std::string& scope) {
  std::vector<std::string> segments;
  std::istringstream stream(scope);
  std::string segment;

  // Split by dots (for hierarchical scopes)
  while (std::getline(stream, segment, '.')) {
    if (!segment.empty()) {
      segments.push_back(segment);
    }
  }

  return segments;
}

bool Theme::isScopeMatch(const std::string& ruleScope, const std::string& tokenScope, MatchMode mode) {
  // Exact match is always true
  if (ruleScope == tokenScope) return true;

  // Split scopes into their components
  auto ruleParts = splitScope(ruleScope);
  auto tokenParts = splitScope(tokenScope);

  // Handle composite scopes (space-separated)
  if (ruleParts.size() > 1) {
    // All rule parts must match some token part
    for (const auto& rulePart : ruleParts) {
      bool foundMatch = false;
      for (const auto& tokenPart : tokenParts) {
        if (isScopeMatch(rulePart, tokenPart, mode)) {
          foundMatch = true;
          break;
        }
      }
      if (!foundMatch) return false;
    }
    return true;
  }

  // Direct prefix match (e.g., "keyword" matches "keyword.control")
  if (tokenScope.find(ruleScope + ".") == 0) return true;

  // Special case for specific language highlighting
  // "keyword.rust" should match "keyword" in Rust files
  auto ruleSegments = getSegments(ruleScope);
  auto tokenSegments = getSegments(tokenScope);

  if (mode == MatchMode::Relaxed) {
    // Check if rule is a subset of token segments in any order
    bool allSegmentsFound = true;
    for (const auto& ruleSeg : ruleSegments) {
      if (std::find(tokenSegments.begin(), tokenSegments.end(), ruleSeg) == tokenSegments.end()) {
        allSegmentsFound = false;
        break;
      }
    }
    if (allSegmentsFound) return true;
  }

  // Handle special cases for common scopes
  bool isCommentRule = false;
  bool isCommentToken = false;
  for (const auto& seg : ruleSegments) {
    if (seg == "comment") {
      isCommentRule = true;
      break;
    }
  }
  for (const auto& seg : tokenSegments) {
    if (seg == "comment") {
      isCommentToken = true;
      break;
    }
  }
  if (isCommentRule && isCommentToken) return true;

  return false;
}

size_t Theme::calculateScopeSpecificity(const std::string& ruleScope, const std::string& tokenScope) {
  // Direct match gets highest specificity
  if (ruleScope == tokenScope) {
    return 1000 + ruleScope.length();
  }

  // Parent scope match (e.g. "keyword" matches "keyword.control.rust")
  if (tokenScope.find(ruleScope + ".") == 0) {
    return 500 + ruleScope.length();
  }

  // Component match (e.g. "rust" in "keyword.control.rust")
  auto ruleParts = splitScope(ruleScope);
  auto tokenParts = splitScope(tokenScope);

  size_t matchCount = 0;
  for (const auto& rulePart : ruleParts) {
    for (const auto& tokenPart : tokenParts) {
      if (rulePart == tokenPart) {
        matchCount++;
      }
    }
  }

  // Calculate specificity based on segment depth
  auto ruleSegments = getSegments(ruleScope);
  auto tokenSegments = getSegments(tokenScope);

  size_t segmentMatchCount = 0;
  for (const auto& ruleSeg : ruleSegments) {
    if (std::find(tokenSegments.begin(), tokenSegments.end(), ruleSeg) != tokenSegments.end()) {
      segmentMatchCount++;
    }
  }

  return (matchCount * 100) + (segmentMatchCount * 10) + ruleScope.length();
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
    if (isScopeMatch(rule.scope, scope)) {
      size_t specificity = calculateScopeSpecificity(rule.scope, scope);
      if (!bestMatch || specificity > bestSpecificity) {
        bestMatch = &rule;
        bestSpecificity = specificity;
      }
    }
  }

  if (bestMatch) {
    return bestMatch->style;
  }

  auto scopeParts = splitScope(scope);
  for (const auto& scopePart : scopeParts) {
    for (const auto& rule : rules) {
      if (isScopeMatch(rule.scope, scopePart)) {
        return rule.style;
      }
    }
  }

  ThemeStyle defaultStyle;
  defaultStyle.foreground = foreground_.toHex();
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
  result.foreground = foreground_.toHex();

  // Apply styles from least to most specific
  for (const auto& scope : scopes) {
    const ThemeRule* rule = findBestMatchingRule(scope);
    if (rule) {
      // Only override non-empty properties
      if (!rule->style.foreground.empty()) {
        result.foreground = rule->style.foreground;
      }
      if (!rule->style.background.empty()) {
        result.background = rule->style.background;
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
    if (isScopeMatch(rule.scope, scope)) {
      size_t specificity = calculateScopeSpecificity(rule.scope, scope);
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
      if (isScopeMatch(rule.scope, tokenScope)) {
        size_t specificity = calculateScopeSpecificity(rule.scope, tokenScope);
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

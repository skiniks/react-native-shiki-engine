#include "Theme.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "../tokenizer/Token.h"
#include "ThemeParser.h"

namespace shiki {

namespace {
  // Split a scope into its component parts
  std::vector<std::string> splitScope(const std::string& scope) {
    std::vector<std::string> parts;
    std::istringstream stream(scope);
    std::string part;

    // First split by spaces
    while (std::getline(stream, part, ' ')) {
      parts.push_back(part);
    }
    return parts;
  }

  // Check if a rule scope matches a token scope following TextMate scope selector rules
  bool isScopeMatch(const std::string& ruleScope, const std::string& tokenScope) {
    // Exact match
    if (ruleScope == tokenScope) return true;

    // Check if rule scope is a parent scope of token scope
    // e.g. "source.rust" matches "source.rust.comment"
    if (tokenScope.find(ruleScope + ".") == 0) return true;

    // For compound scopes (those with spaces), all parts must match
    auto ruleParts = splitScope(ruleScope);
    auto tokenParts = splitScope(tokenScope);

    if (ruleParts.size() > 1) {
      for (const auto& rulePart : ruleParts) {
        bool found = false;
        for (const auto& tokenPart : tokenParts) {
          if (isScopeMatch(rulePart, tokenPart)) {
            found = true;
            break;
          }
        }
        if (!found) return false;
      }
      return true;
    }

    return false;
  }
}  // namespace

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

uint32_t Theme::getFontStyle(const std::string& fontStyle) {
  if (fontStyle.empty()) {
    return style::FONT_NORMAL;
  }

  uint32_t style = style::FONT_NORMAL;
  if (fontStyle.find("bold") != std::string::npos) {
    style |= style::FONT_BOLD;
  }
  if (fontStyle.find("italic") != std::string::npos) {
    style |= style::FONT_ITALIC;
  }
  if (fontStyle.find("underline") != std::string::npos) {
    style |= style::FONT_UNDERLINE;
  }
  return style;
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
  styles_.push_back(style);
}

const ThemeStyle* Theme::findStyle(const std::string& scope) const {
  for (const auto& style : styles_) {
    if (style.fontStyle == scope) {
      return &style;
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
    return ThemeColor::fromHex(name);
  }

  // If it's a named color from the theme, find it in the colors array
  for (const auto& color : colors) {
    if (color.getHexColor() == name) {
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

  // Fall back to default line number style
  ThemeStyle style;
  style.color = "#6272A4";  // Dracula theme default
  return style;
}

ThemeStyle Theme::resolveStyle(const std::string& scope) const {
  // Check cache first
  auto cacheIt = styleCache_.find(scope);
  if (cacheIt != styleCache_.end()) {
    return cacheIt->second;
  }

  // Split the scope into individual components
  auto scopeParts = splitScope(scope);

  // Find all matching rules and sort by specificity
  std::vector<std::pair<const ThemeRule*, size_t>> matchingRules;

  for (const auto& rule : rules) {
    // Try to match each scope part
    for (const auto& scopePart : scopeParts) {
      if (isScopeMatch(rule.scope, scopePart)) {
        size_t specificity = rule.scope.length();
        // Add extra specificity for exact matches
        if (rule.scope == scopePart) {
          specificity += 1000;
        }
        matchingRules.push_back({&rule, specificity});
        break;  // Only count each rule once
      }
    }
  }

  // Sort rules by specificity (highest first)
  std::sort(matchingRules.begin(), matchingRules.end(), [](const auto& a, const auto& b) {
    return a.second > b.second;
  });

  ThemeStyle result;
  // Start with default foreground color
  result.color = foreground_.toHex();

  // Apply styles from most specific to least specific
  for (const auto& [rule, specificity] : matchingRules) {
    if (!rule->style.color.empty() && result.color == foreground_.toHex()) {
      result.color = rule->style.color;
    }
    if (!rule->style.backgroundColor.empty() && result.backgroundColor.empty()) {
      result.backgroundColor = rule->style.backgroundColor;
    }
    if (!rule->style.fontStyle.empty() && result.fontStyle.empty()) {
      result.fontStyle = rule->style.fontStyle;
    }
    result.bold |= rule->style.bold;
    result.italic |= rule->style.italic;
    result.underline |= rule->style.underline;
  }

  // Cache and return the result
  styleCache_[scope] = result;
  return result;
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

  for (const auto& rule : rules) {
    if (isScopeMatch(rule.scope, scope)) {
      // If we haven't found a match yet, or this rule is more specific
      if (!bestMatch || rule.scope.length() > bestMatch->scope.length()) {
        bestMatch = &rule;
      }
    }
  }

  return bestMatch;
}

ThemeColor ThemeColor::fromHex(const std::string& hex) {
  ThemeColor color;
  color.hexColor = hex[0] == '#' ? hex : "#" + hex;
  return color;
}

std::string ThemeColor::toHex() const {
  return hexColor;
}

bool Theme::applyStyle(Token& token) const {
  std::cout << "[DEBUG] Applying style for token scopes: '" << token.getCombinedScope() << "'" << std::endl;

  // Find the most specific matching rule
  const ThemeRule* bestMatch = nullptr;
  size_t bestSpecificity = 0;

  for (const auto& rule : rules) {
    // Try each scope in the token's scope list
    for (const auto& tokenScope : token.scopes) {
      if (isScopeMatch(rule.scope, tokenScope)) {
        size_t specificity = rule.scope.length();
        if (!bestMatch || specificity > bestSpecificity) {
          bestMatch = &rule;
          bestSpecificity = specificity;
          std::cout << "[DEBUG] Found better matching rule: '" << rule.scope << "' (specificity: " << specificity << ")"
                    << std::endl;
        }
      }
    }
  }

  if (bestMatch) {
    std::cout << "[DEBUG] Applying style from rule: '" << bestMatch->scope << "' color: " << bestMatch->style.color
              << std::endl;
    token.style = bestMatch->style;
    return true;
  }

  std::cout << "[DEBUG] No matching style rule found" << std::endl;
  return false;
}

}  // namespace shiki

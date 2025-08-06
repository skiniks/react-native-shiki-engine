#include "Theme.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "ThemeParser.h"

namespace shiki {

Theme::Theme(const Theme& other)
  : name(other.name),
    type(other.type),
    scopeName(other.scopeName),
    background_(other.background_),
    foreground_(other.foreground_),
    rules(other.rules),
    colors(other.colors),
    colorReplacements(other.colorReplacements),
    styles_(other.styles_),
    styleCache_(other.styleCache_),
    semanticTokens_(other.semanticTokens_) {
    // Configuration and patterns removed - handled by shikitori
}

Theme& Theme::operator=(const Theme& other) {
  if (this != &other) {
    name = other.name;
    type = other.type;
    scopeName = other.scopeName;
    background_ = other.background_;
    foreground_ = other.foreground_;
    rules = other.rules;
    colors = other.colors;
    colorReplacements = other.colorReplacements;
    styles_ = other.styles_;
    styleCache_ = other.styleCache_;
    semanticTokens_ = other.semanticTokens_;
    // Configuration and patterns removed - handled by shikitori
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
    return style::FONT_NORMAL; // Default to normal font style
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
    // Return default style instead of throwing
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
  static int logCounter = 0;
  static const int LOG_LIMIT = 100;  // Limit total log entries
  bool shouldLog = logCounter < LOG_LIMIT;

  // Exact match is always true
  if (ruleScope == tokenScope) {
    if (shouldLog) {
      std::cout << "[SCOPE_MATCH] #" << logCounter << " EXACT: " << ruleScope << " == " << tokenScope << std::endl;
      logCounter++;
    }
    return true;
  }

  // TextMate scope matching is complex - we need to handle several cases

  // Split scopes into their space-separated components
  auto ruleParts = splitScope(ruleScope);
  auto tokenParts = splitScope(tokenScope);

  // Case 1: Handle composite scopes (space-separated)
  if (ruleParts.size() > 1) {
    // All rule parts must match some token part
    for (const auto& rulePart : ruleParts) {
      bool foundMatch = false;
      for (const auto& tokenPart : tokenParts) {
        if (isPathMatch(rulePart, tokenPart, mode)) {
          foundMatch = true;
          break;
        }
      }
      if (!foundMatch) {
        if (shouldLog) {
          std::cout << "[SCOPE_MATCH] #" << logCounter << " COMPOSITE-FAIL: " << ruleScope << " !~ " << tokenScope << std::endl;
          logCounter++;
        }
        return false;
      }
    }
    if (shouldLog) {
      std::cout << "[SCOPE_MATCH] #" << logCounter << " COMPOSITE-MATCH: " << ruleScope << " ~ " << tokenScope << std::endl;
      logCounter++;
    }
    return true;
  }

  // Case 2: For single-part scopes, we use path matching on each token part
  for (const auto& tokenPart : tokenParts) {
    if (isPathMatch(ruleScope, tokenPart, mode)) {
      if (shouldLog) {
        std::cout << "[SCOPE_MATCH] #" << logCounter << " PATH-MATCH: " << ruleScope << " matches path in " << tokenScope << std::endl;
        logCounter++;
      }
      return true;
    }
  }

  // No match found
  if (shouldLog) {
    std::cout << "[SCOPE_MATCH] #" << logCounter << " NO-MATCH: " << ruleScope << " !~ " << tokenScope << std::endl;
    logCounter++;
  }
  return false;
}

// Helper function for TextMate path-like scope matching
// This implements the actual TextMate scope selector matching logic
bool Theme::isPathMatch(const std::string& selector, const std::string& scopePath, MatchMode mode) {
  // Case 1: Exact match
  if (selector == scopePath) {
    return true;
  }

  // Case 2: Direct prefix match (e.g., "keyword" matches "keyword.control")
  if (scopePath.find(selector + ".") == 0) {
    return true;
  }

  // Get dot-separated segments
  auto selectorSegments = getSegments(selector);
  auto scopeSegments = getSegments(scopePath);

  // Case 3: Prefix match within path component
  // In TextMate, "string" should match "source.ts string.quoted"
  if (mode == MatchMode::Relaxed) {
    // Try to find the selector at any position in the scope path
    std::string current;
    for (size_t i = 0; i < scopeSegments.size(); i++) {
      if (current.empty()) {
        current = scopeSegments[i];
      } else {
        current += "." + scopeSegments[i];
      }

      // Check if this partial scope matches the selector
      if (current == selector ||
          current.find(selector + ".") == 0 ||
          selector.find(current + ".") == 0) {
        return true;
      }
    }

    // Case 4: Check if any segment in scopePath exactly matches selector
    // This handles cases like "string" matching "source.ts.string.quoted"
    for (const auto& segment : scopeSegments) {
      if (segment == selectorSegments[0]) {
        return true;
      }
    }

    // Case 5: Sequential segment matching
    // "a.b" should match "a.x.b" but not "a.b.c"
    if (selectorSegments.size() > 1) {
      for (size_t i = 0; i <= scopeSegments.size() - selectorSegments.size(); i++) {
        bool matchesAllSegments = true;
        for (size_t j = 0; j < selectorSegments.size(); j++) {
          if (i + j >= scopeSegments.size() || selectorSegments[j] != scopeSegments[i + j]) {
            matchesAllSegments = false;
            break;
          }
        }
        if (matchesAllSegments) {
          return true;
        }
      }
    }
  } else { // Strict mode
    // Only match if selector is a proper prefix of scopePath
    if (selectorSegments.size() <= scopeSegments.size()) {
      bool isPrefixMatch = true;
      for (size_t i = 0; i < selectorSegments.size(); i++) {
        if (selectorSegments[i] != scopeSegments[i]) {
          isPrefixMatch = false;
          break;
        }
      }
      return isPrefixMatch;
    }
  }

  return false;
}

size_t Theme::calculateScopeSpecificity(const std::string& ruleScope, const std::string& tokenScope) {
  static int logCounter = 0;
  static const int LOG_LIMIT = 50;  // Limit total log entries
  bool shouldLog = logCounter < LOG_LIMIT;
  size_t specificity = 0;

  // Direct match gets highest specificity
  if (ruleScope == tokenScope) {
    specificity = 1000 + ruleScope.length();
    if (shouldLog) {
      std::cout << "[SCOPE_SPECIFICITY] #" << logCounter << " EXACT: " << ruleScope << " == " << tokenScope << " = " << specificity << std::endl;
      logCounter++;
    }
    return specificity;
  }

  // Split into parts (for composite scopes)
  auto ruleParts = splitScope(ruleScope);
  auto tokenParts = splitScope(tokenScope);

  // For composite scopes, we need to calculate specificity based on matching parts
  if (ruleParts.size() > 1) {
    size_t matchingParts = 0;
    size_t totalPathSpecificity = 0;

    for (const auto& rulePart : ruleParts) {
      size_t bestPartSpecificity = 0;
      for (const auto& tokenPart : tokenParts) {
        if (isPathMatch(rulePart, tokenPart, MatchMode::Relaxed)) {
          // Calculate specificity for this part match
          size_t partSpecificity = calculatePathSpecificity(rulePart, tokenPart);
          if (partSpecificity > bestPartSpecificity) {
            bestPartSpecificity = partSpecificity;
          }
          matchingParts++;
          break;
        }
      }
      totalPathSpecificity += bestPartSpecificity;
    }

    if (matchingParts == ruleParts.size()) {
      // All parts matched
      specificity = 900 + totalPathSpecificity + ruleScope.length();
      if (shouldLog) {
        std::cout << "[SCOPE_SPECIFICITY] #" << logCounter << " COMPOSITE-FULL: " << ruleScope << " ~= " << tokenScope << " = " << specificity << std::endl;
        logCounter++;
      }
      return specificity;
    } else if (matchingParts > 0) {
      // Some parts matched
      specificity = 700 + totalPathSpecificity + ruleScope.length();
      if (shouldLog) {
        std::cout << "[SCOPE_SPECIFICITY] #" << logCounter << " COMPOSITE-PARTIAL: " << ruleScope << " ~= " << tokenScope << " = " << specificity << std::endl;
        logCounter++;
      }
      return specificity;
    }
  }

  // For single part scopes, find the best matching token part
  size_t bestPathSpecificity = 0;
  for (const auto& tokenPart : tokenParts) {
    if (isPathMatch(ruleScope, tokenPart, MatchMode::Relaxed)) {
      size_t pathSpecificity = calculatePathSpecificity(ruleScope, tokenPart);
      if (pathSpecificity > bestPathSpecificity) {
        bestPathSpecificity = pathSpecificity;
      }
    }
  }

  if (bestPathSpecificity > 0) {
    specificity = 800 + bestPathSpecificity + ruleScope.length();
    if (shouldLog) {
      std::cout << "[SCOPE_SPECIFICITY] #" << logCounter << " PATH-MATCH: " << ruleScope << " path match in " << tokenScope << " = " << specificity << std::endl;
      logCounter++;
    }
    return specificity;
  }

  // Fallback minimal specificity based on length
  specificity = ruleScope.length();
  if (shouldLog) {
    std::cout << "[SCOPE_SPECIFICITY] #" << logCounter << " MINIMAL: " << ruleScope << " minimal match with " << tokenScope << " = " << specificity << std::endl;
    logCounter++;
  }
  return specificity;
}

// Helper function to calculate path-specificity between a selector and scope path
size_t Theme::calculatePathSpecificity(const std::string& selector, const std::string& scopePath) {
  // Exact match gets highest specificity
  if (selector == scopePath) {
    return 500 + selector.length();
  }

  // Direct prefix match
  if (scopePath.find(selector + ".") == 0) {
    return 400 + selector.length();
  }

  // Get segments for more detailed analysis
  auto selectorSegments = getSegments(selector);
  auto scopeSegments = getSegments(scopePath);

  // Path that contains all selector segments in order
  if (selectorSegments.size() <= scopeSegments.size()) {
    bool allSegmentsFound = true;
    size_t lastFoundIndex = 0;

    for (const auto& selSeg : selectorSegments) {
      bool foundSegment = false;
      for (size_t i = lastFoundIndex; i < scopeSegments.size(); i++) {
        if (selSeg == scopeSegments[i]) {
          lastFoundIndex = i + 1;
          foundSegment = true;
          break;
        }
      }
      if (!foundSegment) {
        allSegmentsFound = false;
        break;
      }
    }

    if (allSegmentsFound) {
      return 300 + (selectorSegments.size() * 10);
    }
  }

  // Count matching segments regardless of order
  size_t matchingSegments = 0;
  for (const auto& selSeg : selectorSegments) {
    if (std::find(scopeSegments.begin(), scopeSegments.end(), selSeg) != scopeSegments.end()) {
      matchingSegments++;
    }
  }

  if (matchingSegments > 0) {
    return 200 + (matchingSegments * 10);
  }

  // Minimal match
  return 100;
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

// applyStyle method removed - token styling now handled by shikitori
/*
bool Theme::applyStyle(Token& token) const {
  static int logCounter = 0;
  static const int LOG_LIMIT = 100;  // Limit total log entries
  bool shouldLog = logCounter < LOG_LIMIT;

  const ThemeRule* bestMatch = nullptr;
  size_t bestSpecificity = 0;

  if (shouldLog && !token.scopes.empty()) {
    std::cout << "[APPLY_STYLE] #" << logCounter << " Token scopes: ";
    for (size_t i = 0; i < token.scopes.size(); ++i) {
      std::cout << token.scopes[i];
      if (i < token.scopes.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
  }

  for (const auto& rule : rules) {
    // Try each scope in the token's scope list
    for (const auto& tokenScope : token.scopes) {
      if (isScopeMatch(rule.scope, tokenScope)) {
        size_t specificity = calculateScopeSpecificity(rule.scope, tokenScope);
        if (!bestMatch || specificity > bestSpecificity) {
          bestMatch = &rule;
          bestSpecificity = specificity;

          if (shouldLog) {
            std::cout << "[APPLY_STYLE] #" << logCounter << " Match found: Rule '" << rule.scope
                      << "' for token scope '" << tokenScope << "' with specificity " << specificity << std::endl;
          }
        }
      }
    }
  }

  if (bestMatch) {
    token.style = bestMatch->style;

    if (shouldLog) {
      std::cout << "[APPLY_STYLE] #" << logCounter << " Final style applied from rule: " << bestMatch->scope
                << " (fg: " << bestMatch->style.foreground << ")" << std::endl;
      logCounter++;
    }

    return true;
  }

  if (shouldLog) {
    std::cout << "[APPLY_STYLE] #" << logCounter << " No matching style found for token" << std::endl;
    logCounter++;
  }

  return false;
}
*/

}  // namespace shiki

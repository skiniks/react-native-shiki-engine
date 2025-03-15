#include "ScopeMatcher.h"

#include <algorithm>
#include <sstream>

namespace shiki {

std::vector<std::string> ScopeMatcher::splitScope(const std::string& scope) {
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

std::vector<std::string> ScopeMatcher::getSegments(const std::string& scope) {
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

bool ScopeMatcher::isScopeMatch(const std::string& ruleScope, const std::string& tokenScope, MatchMode mode) {
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

  // Handle hierarchical scopes (dot-separated)

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

size_t ScopeMatcher::calculateScopeSpecificity(const std::string& ruleScope, const std::string& tokenScope) {
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

}  // namespace shiki

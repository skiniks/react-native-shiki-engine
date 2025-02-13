#pragma once
#include "../theme/ThemeStyle.h"
#include <string>
#include <vector>

namespace shiki {

struct Token {
  size_t start;
  size_t length;
  std::vector<std::string> scopes; // Changed from single scope to vector of scopes
  ThemeStyle style;                // The resolved style from the theme

  // Helper method to add a scope
  void addScope(const std::string& scope) {
    if (!scope.empty()) {
      scopes.push_back(scope);
    }
  }

  // Helper method to get combined scope string
  std::string getCombinedScope() const {
    std::string combined;
    for (size_t i = 0; i < scopes.size(); ++i) {
      if (i > 0)
        combined += " ";
      combined += scopes[i];
    }
    return combined;
  }

  // Comparison operator for sorting
  bool operator<(const Token& other) const {
    return start < other.start;
  }
};
} // namespace shiki

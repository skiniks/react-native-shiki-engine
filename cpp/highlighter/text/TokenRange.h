#pragma once
#include "../theme/ThemeStyle.h"
#include "TextRange.h"
#include <string>
#include <vector>

namespace shiki {

class TokenRange : public TextRange {
public:
  std::vector<std::string> scopes;

  TokenRange() = default;
  TokenRange(size_t s, size_t l) : TextRange(s, l) {}
  TokenRange(size_t s, size_t l, std::vector<std::string> sc)
      : TextRange(s, l), scopes(std::move(sc)) {}

  void addScope(const std::string& scope) {
    if (!scope.empty()) {
      scopes.push_back(scope);
    }
  }

  std::string getCombinedScope() const {
    std::string combined;
    for (size_t i = 0; i < scopes.size(); ++i) {
      if (i > 0)
        combined += " ";
      combined += scopes[i];
    }
    return combined;
  }

  // Override measure to handle token-specific measurements if needed
  void measure() override {}
};

} // namespace shiki

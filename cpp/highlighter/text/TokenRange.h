#pragma once
#include <memory>
#include <string>
#include <vector>

#include "TextRange.h"
#include "highlighter/theme/ThemeStyle.h"

namespace shiki {

class TokenRange : public TextRange {
 public:
  TokenRange() = default;
  TokenRange(size_t s, size_t l) : TextRange(s, l) {}
  TokenRange(size_t s, size_t l, const ThemeStyle& st) : TextRange(s, l, st) {}
  TokenRange(size_t s, size_t l, std::vector<std::string> sc) : TextRange(s, l), scopes(std::move(sc)) {}
  TokenRange(size_t s, size_t l, const ThemeStyle& st, std::vector<std::string> sc)
    : TextRange(s, l, st), scopes(std::move(sc)) {}
  std::vector<std::string> scopes;

  void addScope(const std::string& scope) {
    if (!scope.empty()) {
      scopes.push_back(scope);
    }
  }

  std::string getCombinedScope() const {
    std::string combined;
    for (size_t i = 0; i < scopes.size(); ++i) {
      if (i > 0) combined += " ";
      combined += scopes[i];
    }
    return combined;
  }

  void measure(const std::string& text, const std::shared_ptr<PlatformMeasurement>& measurer) override {
    TextRange::measure(text, measurer);
  }

  bool hasScope(const std::string& scope) const {
    return std::find(scopes.begin(), scopes.end(), scope) != scopes.end();
  }

  bool hasScopePrefix(const std::string& prefix) const {
    for (const auto& scope : scopes) {
      if (scope.compare(0, prefix.length(), prefix) == 0) {
        return true;
      }
    }
    return false;
  }

  TokenRange mergeWithScopes(const TokenRange& other) const {
    const TextRange& mergedBase = merge(other);
    TokenRange merged(mergedBase.start, mergedBase.length, mergedBase.style);
    merged.scopes = scopes;
    for (const auto& scope : other.scopes) {
      if (std::find(merged.scopes.begin(), merged.scopes.end(), scope) == merged.scopes.end()) {
        merged.scopes.push_back(scope);
      }
    }

    return merged;
  }
};

}  // namespace shiki

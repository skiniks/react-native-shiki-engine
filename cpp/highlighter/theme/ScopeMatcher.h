#pragma once
#include <string>
#include <vector>

namespace shiki {

class ScopeMatcher {
 public:
  enum class MatchMode {
    Strict,  // Exact match or direct parent-child relationship
    Standard,  // TextMate standard matching rules
    Relaxed  // More lenient matching for better compatibility
  };

  // Split a scope into its component parts (space-separated)
  static std::vector<std::string> splitScope(const std::string& scope);

  // Get segments of a scope (dot-separated)
  static std::vector<std::string> getSegments(const std::string& scope);

  // Check if a rule scope matches a token scope
  static bool
  isScopeMatch(const std::string& ruleScope, const std::string& tokenScope, MatchMode mode = MatchMode::Standard);

  // Calculate specificity of a scope match (higher is more specific)
  static size_t calculateScopeSpecificity(const std::string& ruleScope, const std::string& tokenScope);
};

}  // namespace shiki

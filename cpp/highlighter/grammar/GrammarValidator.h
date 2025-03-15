#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Grammar.h"

namespace shiki {

struct ValidationResult {
  bool isValid{true};
  std::vector<std::string> warnings;
  std::vector<std::string> errors;

  void addWarning(const std::string& warning) {
    warnings.push_back(warning);
  }

  void addError(const std::string& error) {
    isValid = false;
    errors.push_back(error);
  }
};

class GrammarValidator {
 public:
  static ValidationResult validate(const Grammar& grammar);

  static ValidationResult validatePattern(const GrammarPattern& pattern);

  static ValidationResult checkCommonIssues(const Grammar& grammar);

  static ValidationResult checkRepositoryReferences(const Grammar& grammar);

  static ValidationResult checkCircularIncludes(const Grammar& grammar);

  static ValidationResult checkRegexPatterns(const Grammar& grammar);

  static ValidationResult checkCaptureGroups(const Grammar& grammar);

  static ValidationResult checkScopeNames(const Grammar& grammar);

  static ValidationResult checkEssentialPatterns(const Grammar& grammar);

 private:
  static bool isValidRegex(const std::string& pattern, std::string& errorMsg);

  static bool isValidScopeName(const std::string& scopeName, std::string& errorMsg);

  static bool hasEssentialCaptures(const GrammarPattern& pattern);

  static bool repositoryReferenceExists(const std::string& reference, const Grammar& grammar);

  static bool
  detectCircularIncludes(const Grammar& grammar, const std::string& include, std::vector<std::string>& includeStack);
};

}  // namespace shiki

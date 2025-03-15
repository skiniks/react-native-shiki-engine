#include "GrammarValidator.h"

#include <regex>
#include <sstream>
#include <unordered_set>

#include "highlighter/core/Configuration.h"

namespace shiki {

ValidationResult GrammarValidator::validate(const Grammar& grammar) {
  ValidationResult result;

  auto commonIssues = checkCommonIssues(grammar);
  auto repoRefs = checkRepositoryReferences(grammar);
  auto circularIncludes = checkCircularIncludes(grammar);
  auto regexPatterns = checkRegexPatterns(grammar);
  auto captureGroups = checkCaptureGroups(grammar);
  auto scopeNames = checkScopeNames(grammar);
  auto essentialPatterns = checkEssentialPatterns(grammar);

  result.warnings.insert(result.warnings.end(), commonIssues.warnings.begin(), commonIssues.warnings.end());
  result.warnings.insert(result.warnings.end(), repoRefs.warnings.begin(), repoRefs.warnings.end());
  result.warnings.insert(result.warnings.end(), circularIncludes.warnings.begin(), circularIncludes.warnings.end());
  result.warnings.insert(result.warnings.end(), regexPatterns.warnings.begin(), regexPatterns.warnings.end());
  result.warnings.insert(result.warnings.end(), captureGroups.warnings.begin(), captureGroups.warnings.end());
  result.warnings.insert(result.warnings.end(), scopeNames.warnings.begin(), scopeNames.warnings.end());
  result.warnings.insert(result.warnings.end(), essentialPatterns.warnings.begin(), essentialPatterns.warnings.end());

  result.errors.insert(result.errors.end(), commonIssues.errors.begin(), commonIssues.errors.end());
  result.errors.insert(result.errors.end(), repoRefs.errors.begin(), repoRefs.errors.end());
  result.errors.insert(result.errors.end(), circularIncludes.errors.begin(), circularIncludes.errors.end());
  result.errors.insert(result.errors.end(), regexPatterns.errors.begin(), regexPatterns.errors.end());
  result.errors.insert(result.errors.end(), captureGroups.errors.begin(), captureGroups.errors.end());
  result.errors.insert(result.errors.end(), scopeNames.errors.begin(), scopeNames.errors.end());
  result.errors.insert(result.errors.end(), essentialPatterns.errors.begin(), essentialPatterns.errors.end());

  result.isValid = commonIssues.isValid && repoRefs.isValid && circularIncludes.isValid && regexPatterns.isValid &&
    captureGroups.isValid && scopeNames.isValid && essentialPatterns.isValid;

  return result;
}

ValidationResult GrammarValidator::validatePattern(const GrammarPattern& pattern) {
  ValidationResult result;

  if (pattern.match.empty() && pattern.begin.empty() && pattern.include.empty()) {
    result.addWarning("Pattern has no match, begin, or include property: " + pattern.name);
  }

  if (!pattern.begin.empty() && pattern.end.empty()) {
    result.addError("Pattern with 'begin' must also have 'end' property: " + pattern.name);
  }

  if (!pattern.match.empty()) {
    std::string errorMsg;
    if (!isValidRegex(pattern.match, errorMsg)) {
      result.addError("Invalid match regex in pattern '" + pattern.name + "': " + errorMsg);
    }
  }

  if (!pattern.begin.empty()) {
    std::string errorMsg;
    if (!isValidRegex(pattern.begin, errorMsg)) {
      result.addError("Invalid begin regex in pattern '" + pattern.name + "': " + errorMsg);
    }
  }

  if (!pattern.end.empty()) {
    std::string errorMsg;
    if (!isValidRegex(pattern.end, errorMsg)) {
      result.addError("Invalid end regex in pattern '" + pattern.name + "': " + errorMsg);
    }
  }

  if (!pattern.name.empty()) {
    std::string errorMsg;
    if (!isValidScopeName(pattern.name, errorMsg)) {
      result.addError("Invalid scope name in pattern: " + errorMsg);
    }
  }

  if (!pattern.contentName.empty()) {
    std::string errorMsg;
    if (!isValidScopeName(pattern.contentName, errorMsg)) {
      result.addError("Invalid content name in pattern: " + errorMsg);
    }
  }

  if (!pattern.match.empty() && pattern.captures.empty()) {
    result.addWarning("Match pattern has no captures: " + pattern.name);
  }

  for (const auto& nestedPattern : pattern.patterns) {
    auto nestedResult = validatePattern(nestedPattern);
    result.warnings.insert(result.warnings.end(), nestedResult.warnings.begin(), nestedResult.warnings.end());
    result.errors.insert(result.errors.end(), nestedResult.errors.begin(), nestedResult.errors.end());
    result.isValid = result.isValid && nestedResult.isValid;
  }

  return result;
}

ValidationResult GrammarValidator::checkCommonIssues(const Grammar& grammar) {
  ValidationResult result;

  if (grammar.name.empty()) {
    result.addWarning("Grammar has no name");
  }

  if (grammar.scopeName.empty()) {
    result.addWarning("Grammar has no scope name");
  }

  if (grammar.patterns.empty()) {
    result.addError("Grammar has no patterns");
  }

  return result;
}

ValidationResult GrammarValidator::checkRepositoryReferences(const Grammar& grammar) {
  ValidationResult result;
  std::unordered_set<std::string> referencedRepos;

  auto collectRepoRefs = [&referencedRepos](const GrammarPattern& pattern) {
    if (pattern.include.empty()) return;

    if (pattern.include[0] == '#') {
      referencedRepos.insert(pattern.include.substr(1));
    }

    for (const auto& nestedPattern : pattern.patterns) {
      if (!nestedPattern.include.empty() && nestedPattern.include[0] == '#') {
        referencedRepos.insert(nestedPattern.include.substr(1));
      }
    }
  };

  for (const auto& pattern : grammar.patterns) {
    collectRepoRefs(pattern);

    for (const auto& nestedPattern : pattern.patterns) {
      collectRepoRefs(nestedPattern);
    }
  }

  for (const auto& repoRef : referencedRepos) {
    if (grammar.repository.find(repoRef) == grammar.repository.end()) {
      result.addError("Referenced repository not found: " + repoRef);
    }
  }

  return result;
}

ValidationResult GrammarValidator::checkCircularIncludes(const Grammar& grammar) {
  ValidationResult result;

  for (const auto& pattern : grammar.patterns) {
    if (!pattern.include.empty()) {
      std::vector<std::string> includeStack;
      if (detectCircularIncludes(grammar, pattern.include, includeStack)) {
        std::stringstream ss;
        ss << "Circular include detected: ";
        for (const auto& inc : includeStack) {
          ss << inc << " -> ";
        }
        ss << pattern.include;
        result.addError(ss.str());
      }
    }
  }

  return result;
}

ValidationResult GrammarValidator::checkRegexPatterns(const Grammar& grammar) {
  ValidationResult result;

  auto checkPatternRegex = [&result](const GrammarPattern& pattern) {
    if (!pattern.match.empty()) {
      std::string errorMsg;
      if (!isValidRegex(pattern.match, errorMsg)) {
        result.addError("Invalid match regex in pattern '" + pattern.name + "': " + errorMsg);
      }
    }

    if (!pattern.begin.empty()) {
      std::string errorMsg;
      if (!isValidRegex(pattern.begin, errorMsg)) {
        result.addError("Invalid begin regex in pattern '" + pattern.name + "': " + errorMsg);
      }
    }

    if (!pattern.end.empty()) {
      std::string errorMsg;
      if (!isValidRegex(pattern.end, errorMsg)) {
        result.addError("Invalid end regex in pattern '" + pattern.name + "': " + errorMsg);
      }
    }
  };

  for (const auto& pattern : grammar.patterns) {
    checkPatternRegex(pattern);

    for (const auto& nestedPattern : pattern.patterns) {
      checkPatternRegex(nestedPattern);
    }
  }

  for (const auto& [name, pattern] : grammar.repository) {
    for (const auto& grammarPattern : pattern.patterns) {
      checkPatternRegex(grammarPattern);

      for (const auto& nestedPattern : grammarPattern.patterns) {
        checkPatternRegex(nestedPattern);
      }
    }
  }

  return result;
}

ValidationResult GrammarValidator::checkCaptureGroups(const Grammar& grammar) {
  ValidationResult result;

  auto checkPatternCaptures = [&result](const GrammarPattern& pattern) {
    if (!pattern.match.empty() && pattern.captures.empty()) {
      result.addWarning("Match pattern has no captures: " + pattern.name);
    }

    if (!pattern.begin.empty() && pattern.beginCaptures.empty()) {
      result.addWarning("Begin pattern has no captures: " + pattern.name);
    }

    if (!pattern.end.empty() && pattern.endCaptures.empty()) {
      result.addWarning("End pattern has no captures: " + pattern.name);
    }

    for (const auto& [index, _] : pattern.captures) {
      if (index < 0) {
        result.addError("Capture index must be non-negative: " + std::to_string(index));
      }
    }

    for (const auto& [index, _] : pattern.beginCaptures) {
      if (index < 0) {
        result.addError("Begin capture index must be non-negative: " + std::to_string(index));
      }
    }

    for (const auto& [index, _] : pattern.endCaptures) {
      if (index < 0) {
        result.addError("End capture index must be non-negative: " + std::to_string(index));
      }
    }
  };

  for (const auto& pattern : grammar.patterns) {
    checkPatternCaptures(pattern);

    for (const auto& nestedPattern : pattern.patterns) {
      checkPatternCaptures(nestedPattern);
    }
  }

  for (const auto& [name, pattern] : grammar.repository) {
    for (const auto& grammarPattern : pattern.patterns) {
      checkPatternCaptures(grammarPattern);
    }
  }

  return result;
}

ValidationResult GrammarValidator::checkScopeNames(const Grammar& grammar) {
  ValidationResult result;

  auto checkPatternScopeName = [&result](const GrammarPattern& pattern) {
    if (!pattern.name.empty()) {
      std::string errorMsg;
      if (!isValidScopeName(pattern.name, errorMsg)) {
        result.addError("Invalid scope name in pattern: " + errorMsg);
      }
    }

    for (const auto& [_, scopeName] : pattern.captures) {
      if (!scopeName.empty()) {
        std::string errorMsg;
        if (!isValidScopeName(scopeName, errorMsg)) {
          result.addError("Invalid capture scope name: " + errorMsg);
        }
      }
    }

    for (const auto& [_, scopeName] : pattern.beginCaptures) {
      if (!scopeName.empty()) {
        std::string errorMsg;
        if (!isValidScopeName(scopeName, errorMsg)) {
          result.addError("Invalid begin capture scope name: " + errorMsg);
        }
      }
    }

    for (const auto& [_, scopeName] : pattern.endCaptures) {
      if (!scopeName.empty()) {
        std::string errorMsg;
        if (!isValidScopeName(scopeName, errorMsg)) {
          result.addError("Invalid end capture scope name: " + errorMsg);
        }
      }
    }
  };

  if (!grammar.scopeName.empty()) {
    std::string errorMsg;
    if (!isValidScopeName(grammar.scopeName, errorMsg)) {
      result.addError("Invalid grammar scope name: " + errorMsg);
    }
  }

  for (const auto& pattern : grammar.patterns) {
    checkPatternScopeName(pattern);

    for (const auto& nestedPattern : pattern.patterns) {
      checkPatternScopeName(nestedPattern);
    }
  }

  for (const auto& [name, pattern] : grammar.repository) {
    for (const auto& grammarPattern : pattern.patterns) {
      checkPatternScopeName(grammarPattern);
    }
  }

  return result;
}

ValidationResult GrammarValidator::checkEssentialPatterns(const Grammar& grammar) {
  ValidationResult result;

  if (grammar.scopeName.find("source.") == 0) {
    bool hasCommentPattern = false;
    bool hasStringPattern = false;
    bool hasKeywordPattern = false;

    auto checkEssentialCategories = [&](const GrammarPattern& pattern) {
      if (pattern.name.find("comment") != std::string::npos) {
        hasCommentPattern = true;
      } else if (pattern.name.find("string") != std::string::npos) {
        hasStringPattern = true;
      } else if (pattern.name.find("keyword") != std::string::npos) {
        hasKeywordPattern = true;
      }

      for (const auto& nestedPattern : pattern.patterns) {
        if (nestedPattern.name.find("comment") != std::string::npos) {
          hasCommentPattern = true;
        } else if (nestedPattern.name.find("string") != std::string::npos) {
          hasStringPattern = true;
        } else if (nestedPattern.name.find("keyword") != std::string::npos) {
          hasKeywordPattern = true;
        }
      }
    };

    for (const auto& pattern : grammar.patterns) {
      checkEssentialCategories(pattern);
    }

    for (const auto& [name, pattern] : grammar.repository) {
      for (const auto& grammarPattern : pattern.patterns) {
        checkEssentialCategories(grammarPattern);
      }
    }

    if (!hasCommentPattern) {
      result.addWarning("Grammar is missing comment patterns");
    }

    if (!hasStringPattern) {
      result.addWarning("Grammar is missing string patterns");
    }

    if (!hasKeywordPattern) {
      result.addWarning("Grammar is missing keyword patterns");
    }
  }

  return result;
}

bool GrammarValidator::isValidRegex(const std::string& pattern, std::string& errorMsg) {
  try {
    std::regex re(pattern);
    return true;
  } catch (const std::regex_error& e) {
    errorMsg = e.what();
    return false;
  }
}

bool GrammarValidator::isValidScopeName(const std::string& scopeName, std::string& errorMsg) {
  static const std::regex scopeNameRegex("^[a-zA-Z0-9]+(\\.[a-zA-Z0-9]+)*$");

  if (!std::regex_match(scopeName, scopeNameRegex)) {
    errorMsg = "Scope name should be dot-separated alphanumeric segments: " + scopeName;
    return false;
  }

  return true;
}

bool GrammarValidator::hasEssentialCaptures(const GrammarPattern& pattern) {
  return !pattern.captures.empty() || !pattern.beginCaptures.empty() || !pattern.endCaptures.empty();
}

bool GrammarValidator::repositoryReferenceExists(const std::string& reference, const Grammar& grammar) {
  if (reference.empty() || reference[0] != '#') {
    return false;
  }

  std::string repoName = reference.substr(1);
  return grammar.repository.find(repoName) != grammar.repository.end();
}

bool GrammarValidator::detectCircularIncludes(
  const Grammar& grammar,
  const std::string& include,
  std::vector<std::string>& includeStack
) {
  if (std::find(includeStack.begin(), includeStack.end(), include) != includeStack.end()) {
    return true;
  }

  includeStack.push_back(include);

  if (include[0] == '#') {
    std::string repoName = include.substr(1);
    auto repoIt = grammar.repository.find(repoName);

    if (repoIt != grammar.repository.end()) {
      const auto& pattern = repoIt->second;

      for (const auto& grammarPattern : pattern.patterns) {
        if (!grammarPattern.include.empty()) {
          if (detectCircularIncludes(grammar, grammarPattern.include, includeStack)) {
            return true;
          }
        }
      }

      for (const auto& repoPattern : pattern.patterns) {
        for (const auto& nestedPattern : repoPattern.patterns) {
          if (!nestedPattern.include.empty()) {
            if (detectCircularIncludes(grammar, nestedPattern.include, includeStack)) {
              return true;
            }
          }
        }
      }
    }
  }

  includeStack.pop_back();

  return false;
}

}  // namespace shiki

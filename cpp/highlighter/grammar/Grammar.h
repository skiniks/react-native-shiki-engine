#pragma once
#include <map>
#include <memory>
#include <rapidjson/document.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "../error/HighlightError.h"

namespace shiki {

enum class GrammarErrorCode {
  InvalidPattern,         // Pattern is malformed or invalid
  InvalidInclude,         // Include reference is invalid or not found
  IncludeResolutionError, // Error while resolving an include
  CircularInclude,        // Detected circular include dependency
  InvalidRepository,      // Repository reference is invalid
  ValidationError         // General pattern validation error
};

inline std::string grammarErrorToString(GrammarErrorCode code) {
  switch (code) {
    case GrammarErrorCode::InvalidPattern:
      return "Invalid pattern";
    case GrammarErrorCode::InvalidInclude:
      return "Invalid include reference";
    case GrammarErrorCode::IncludeResolutionError:
      return "Include resolution error";
    case GrammarErrorCode::CircularInclude:
      return "Circular include dependency detected";
    case GrammarErrorCode::InvalidRepository:
      return "Invalid repository reference";
    case GrammarErrorCode::ValidationError:
      return "Pattern validation error";
    default:
      return "Unknown grammar error";
  }
}

class GrammarError : public HighlightError {
public:
  GrammarError(GrammarErrorCode code, const std::string& details)
      : HighlightError(HighlightErrorCode::GrammarError,
                      grammarErrorToString(code) + ": " + details),
        grammarCode_(code) {}

  GrammarErrorCode getGrammarCode() const { return grammarCode_; }

private:
  GrammarErrorCode grammarCode_;
};

struct GrammarPattern {
  std::string name;
  std::string match;
  std::string begin;
  std::string end;
  std::string include;
  std::unordered_map<int, std::string> captures;
  std::unordered_map<int, std::string> beginCaptures;
  std::unordered_map<int, std::string> endCaptures;
  std::vector<GrammarPattern> patterns;
  int index{-1};

  bool hasInclude() const {
    return !include.empty();
  }
};

struct GrammarRule {
  std::vector<GrammarPattern> patterns;
};

class Grammar {
public:
  Grammar() = default;
  explicit Grammar(const std::string& name);
  virtual ~Grammar() = default;

  void addPattern(const GrammarPattern& pattern);
  std::string getScopeForMatch(size_t index, const std::vector<std::string>& captures) const;

  const std::string& getName() const {
    return name;
  }
  const std::string& getScopeName() const {
    return scopeName;
  }
  const std::vector<GrammarPattern>& getPatterns() const {
    return patterns;
  }

  std::vector<GrammarPattern>& getPatterns() {
    return patterns;
  }

  void processIncludePattern(GrammarPattern& pattern, const std::string& repository);
  std::vector<GrammarPattern> processPatterns(const rapidjson::Value& patterns,
                                            const std::string& repository);

  static std::shared_ptr<Grammar> fromJson(const std::string& content);
  static bool validateJson(const std::string& content);
  static std::shared_ptr<Grammar> loadByScope(const std::string& scope);

  std::string name;
  std::string scopeName;
  std::vector<GrammarPattern> patterns;
  std::unordered_map<std::string, GrammarRule> repository;

private:
  std::unordered_map<int, size_t> patternIndexMap_;
  std::unordered_set<std::string> includeStack_;

  void validatePattern(const GrammarPattern& pattern) const;
  void validateInclude(const std::string& include) const;
  void checkCircularDependency(const std::string& include);

  friend class GrammarParser;
};

} // namespace shiki

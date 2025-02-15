#pragma once
#include <rapidjson/document.h>

#include <map>
#include <memory>
#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../error/HighlightError.h"

namespace shiki {

enum class GrammarErrorCode {
  InvalidPattern,  // Pattern is malformed or invalid
  InvalidInclude,  // Include reference is invalid or not found
  IncludeResolutionError,  // Error while resolving an include
  CircularInclude,  // Detected circular include dependency
  InvalidRepository,  // Repository reference is invalid
  ValidationError  // General pattern validation error
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
    : HighlightError(HighlightErrorCode::GrammarError, grammarErrorToString(code) + ": " + details),
      grammarCode_(code) {}

  GrammarErrorCode getGrammarCode() const {
    return grammarCode_;
  }

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

// Pattern validation constants
struct PatternValidationRules {
  static constexpr size_t MAX_PATTERN_LENGTH = 10000;  // Maximum length for regex patterns
  static constexpr size_t MAX_NESTED_DEPTH = 50;  // Maximum depth for nested patterns
  static constexpr size_t MAX_CAPTURES = 100;  // Maximum number of captures
};

class GrammarPatternValidator {
 public:
  // Validate a complete pattern structure
  static void validatePattern(const GrammarPattern& pattern, size_t depth = 0) {
    validateBasicStructure(pattern);
    validateRegexPatterns(pattern);
    validateCaptures(pattern);
    validateNestedPatterns(pattern, depth);
  }

  // Validate JSON schema for grammar
  static void validateGrammarSchema(const rapidjson::Document& doc) {
    validateRequiredFields(doc);
    validatePatternArray(doc);
    validateRepository(doc);
  }

 private:
  static void validateBasicStructure(const GrammarPattern& pattern) {
    // Check for valid pattern structure
    if (pattern.match.empty() && pattern.begin.empty() && pattern.include.empty()) {
      throw GrammarError(
        GrammarErrorCode::InvalidPattern,
        "Pattern must have either match, begin, or include property"
      );
    }

    // Validate begin/end pairs
    if (!pattern.begin.empty() && pattern.end.empty()) {
      throw GrammarError(GrammarErrorCode::InvalidPattern, "Pattern with 'begin' must also have 'end' property");
    }

    // Validate name if present
    if (!pattern.name.empty() && !isValidScopeName(pattern.name)) {
      throw GrammarError(GrammarErrorCode::InvalidPattern, "Invalid scope name format: " + pattern.name);
    }
  }

  static void validateRegexPatterns(const GrammarPattern& pattern) {
    // Validate regex patterns
    auto validateRegex = [](const std::string& pattern, const std::string& type) {
      if (pattern.length() > PatternValidationRules::MAX_PATTERN_LENGTH) {
        throw GrammarError(GrammarErrorCode::InvalidPattern, type + " pattern exceeds maximum length");
      }
      try {
        std::regex re(pattern);
      } catch (const std::regex_error& e) {
        throw GrammarError(GrammarErrorCode::InvalidPattern, "Invalid " + type + " regex pattern: " + e.what());
      }
    };

    if (!pattern.match.empty()) {
      validateRegex(pattern.match, "match");
    }
    if (!pattern.begin.empty()) {
      validateRegex(pattern.begin, "begin");
    }
    if (!pattern.end.empty()) {
      validateRegex(pattern.end, "end");
    }
  }

  static void validateCaptures(const GrammarPattern& pattern) {
    // Validate capture maps
    auto validateCaptureMap = [](const std::unordered_map<int, std::string>& captures, const std::string& type) {
      if (captures.size() > PatternValidationRules::MAX_CAPTURES) {
        throw GrammarError(GrammarErrorCode::InvalidPattern, type + " captures exceed maximum limit");
      }
      for (const auto& [index, name] : captures) {
        if (index < 0) {
          throw GrammarError(GrammarErrorCode::InvalidPattern, type + " capture index must be non-negative");
        }
        if (!isValidScopeName(name)) {
          throw GrammarError(GrammarErrorCode::InvalidPattern, "Invalid " + type + " capture scope name: " + name);
        }
      }
    };

    validateCaptureMap(pattern.captures, "pattern");
    validateCaptureMap(pattern.beginCaptures, "begin");
    validateCaptureMap(pattern.endCaptures, "end");
  }

  static void validateNestedPatterns(const GrammarPattern& pattern, size_t depth) {
    if (depth > PatternValidationRules::MAX_NESTED_DEPTH) {
      throw GrammarError(GrammarErrorCode::InvalidPattern, "Pattern nesting depth exceeds maximum limit");
    }
    for (const auto& nested : pattern.patterns) {
      validatePattern(nested, depth + 1);
    }
  }

  static bool isValidScopeName(const std::string& name) {
    // Scope names should follow the format: component(.component)*
    static const std::regex scopeNameRegex(R"(^[a-zA-Z0-9_-]+(\.[a-zA-Z0-9_-]+)*$)");
    return std::regex_match(name, scopeNameRegex);
  }

  static void validateRequiredFields(const rapidjson::Document& doc) {
    if (!doc.IsObject()) {
      throw GrammarError(GrammarErrorCode::ValidationError, "Grammar must be a JSON object");
    }

    const char* requiredFields[] = {"name", "scopeName"};
    for (const char* field : requiredFields) {
      if (!doc.HasMember(field) || !doc[field].IsString()) {
        throw GrammarError(
          GrammarErrorCode::ValidationError,
          std::string("Missing or invalid required field: ") + field
        );
      }
    }
  }

  static void validatePatternArray(const rapidjson::Document& doc) {
    if (!doc.HasMember("patterns") || !doc["patterns"].IsArray()) {
      throw GrammarError(GrammarErrorCode::ValidationError, "Grammar must have a valid patterns array");
    }
  }

  static void validateRepository(const rapidjson::Document& doc) {
    if (doc.HasMember("repository")) {
      if (!doc["repository"].IsObject()) {
        throw GrammarError(GrammarErrorCode::ValidationError, "Repository must be an object if present");
      }
    }
  }
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
  std::vector<GrammarPattern> processPatterns(const rapidjson::Value& patterns, const std::string& repository);

  static std::shared_ptr<Grammar> fromJson(const std::string& content);
  static bool validateJson(const std::string& content);

  std::string name;
  std::string scopeName;
  std::vector<GrammarPattern> patterns;
  std::unordered_map<std::string, GrammarRule> repository;

 private:
  // Pattern indexing
  std::unordered_map<int, size_t> patternIndexMap_;

  // Include resolution tracking
  std::unordered_set<std::string> includeStack_;

  // Include resolution cache
  struct IncludeKey {
    std::string include;
    std::string repository;

    bool operator==(const IncludeKey& other) const {
      return include == other.include && repository == other.repository;
    }
  };

  struct IncludeKeyHash {
    size_t operator()(const IncludeKey& key) const {
      return std::hash<std::string>()(key.include) ^ std::hash<std::string>()(key.repository);
    }
  };

  // Cache resolved includes to avoid redundant processing
  mutable std::unordered_map<IncludeKey, std::vector<GrammarPattern>, IncludeKeyHash> includeCache_;

  std::vector<GrammarPattern> resolveInclude(const std::string& include, const std::string& repositoryKey);
  std::vector<GrammarPattern> resolveRepositoryReference(const std::string& repoName);
  std::vector<GrammarPattern> resolveSelfReference();

  void cacheResolvedInclude(const IncludeKey& key, const std::vector<GrammarPattern>& patterns);
  std::vector<GrammarPattern>* findCachedInclude(const IncludeKey& key) const;
  void clearIncludeCache();

  void validatePattern(const GrammarPattern& pattern) const;
  void validateInclude(const std::string& include) const;
  void checkCircularDependency(const std::string& include);

  friend class GrammarParser;
};

}  // namespace shiki

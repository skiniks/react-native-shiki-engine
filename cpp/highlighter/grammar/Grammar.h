#pragma once
#include <map>
#include <memory>
#include <rapidjson/document.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace shiki {

struct ScopePattern {
  std::string name;                  // The scope name (e.g., "keyword.control")
  std::string match;                 // The regex pattern
  std::vector<std::string> captures; // Named capture groups
  int index{-1};                     // Pattern index in Oniguruma scanner
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

  bool hasInclude() const {
    return !include.empty();
  }
};

struct GrammarRule {
  std::vector<GrammarPattern> patterns;
};

class Grammar {
public:
  Grammar() = default; // Default constructor
  explicit Grammar(const std::string& name);
  virtual ~Grammar() = default;

  void addPattern(const ScopePattern& pattern);
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

  // Add methods for pattern processing
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
  std::string name_;
  std::vector<ScopePattern> patterns_;
  std::unordered_map<int, size_t> patternIndexMap_; // Maps Oniguruma index to pattern
  // Add repository support
  std::map<std::string, std::vector<GrammarPattern>> repository_;

  friend class GrammarParser;
};

} // namespace shiki

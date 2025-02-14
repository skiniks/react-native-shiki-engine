#pragma once
#include "../grammar/Grammar.h"
#include "../renderer/LineNumbers.h"
#include "../text/TokenRange.h"
#include "../theme/Theme.h"
#include "../utils/ConcurrencyUtil.h"
#include "../cache/StyleCache.h"
#include "Token.h"
#include "oniguruma.h"
#include <future>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace shiki {

enum class WorkPriority;
class WorkPrioritizer;

// Default batch size (32KB)
constexpr size_t DEFAULT_BATCH_SIZE = 32 * 1024;

// Deleter types for RAII management of Oniguruma resources
struct OnigRegexDeleter {
  void operator()(regex_t* r) const;
};

struct OnigRegionDeleter {
  void operator()(OnigRegion* r) const;
};

// Helper class for cleaning and organizing tokens
class TokenCleaner {
public:
  std::vector<Token> clean(std::vector<Token>&& tokens, const std::string& code);

private:
  Token createWhitespaceToken(size_t start, size_t length);
  bool shouldAddToken(const Token& token, const std::vector<Token>& existingTokens);
};

struct CompiledPattern {
  std::string name;
  GrammarPattern pattern;
  std::unique_ptr<regex_t, OnigRegexDeleter> regex;
  std::unique_ptr<regex_t, OnigRegexDeleter> beginRegex;
  std::unique_ptr<regex_t, OnigRegexDeleter> endRegex;
};

class ShikiTokenizer {
public:
  static ShikiTokenizer& getInstance();
  ~ShikiTokenizer();

  void setGrammar(std::shared_ptr<Grammar> grammar);
  void setTheme(std::shared_ptr<Theme> theme);
  std::shared_ptr<Theme> getTheme() const {
    return theme_;
  }
  std::vector<Token> tokenize(const std::string& code);
  std::vector<Token> tokenizeParallel(const std::string& code,
                                      size_t batchSize = DEFAULT_BATCH_SIZE);
  ThemeStyle resolveStyle(const std::string& scope) const;

private:
  ShikiTokenizer() = default;

  // Thread-local storage for temporary buffers
  static thread_local std::vector<std::string> scopeStack_;
  static thread_local std::unordered_set<size_t> processedPositions_;

  // Mutex for general state changes
  mutable std::shared_mutex mutex_;

  // Mutex specifically for pattern compilation
  std::mutex pattern_mutex_;

  // Mutex for external grammar loading
  mutable std::shared_mutex grammar_mutex_;

  std::shared_ptr<Grammar> grammar_;
  std::shared_ptr<Theme> theme_;
  std::vector<CompiledPattern> compiledPatterns_;
  std::unique_ptr<ConcurrencyUtil> concurrencyUtil_;
  std::unique_ptr<LineNumbers> lineNumbers_;

  // Cache for external grammars
  std::unordered_map<std::string, std::shared_ptr<Grammar>> externalGrammars_;

  // External grammar loading
  std::shared_ptr<Grammar> loadExternalGrammar(const std::string& scope);

  void clearCompiledPatterns();
  void compilePatterns();

  // Pattern processing helpers
  void processPatterns(const std::vector<GrammarPattern>& patterns, const std::string& code,
                       std::vector<Token>& tokens, std::vector<std::string>& scopeStack);

  void processCompiledPatterns(const std::string& code, std::vector<Token>& tokens,
                               std::vector<std::string>& scopeStack,
                               std::unordered_set<size_t>& processedPositions);

  bool tryMatchPattern(regex_t* regex, const std::string& code, size_t pos, OnigRegion* region,
                       size_t& matchStart, size_t& matchLength);

  bool tryMatchBeginEndPattern(const CompiledPattern& pattern, const std::string& code, size_t pos,
                               OnigRegion* region, size_t& matchStart, size_t& matchLength);

  std::string buildFullScope(const std::vector<std::string>& scopeStack);

  void processPattern(const GrammarPattern& pattern, const std::string& code,
                      std::vector<Token>& tokens, std::vector<std::string>& scopeStack);

  // Remove redundant methods that are no longer used
  void processBeginEndPattern(const GrammarPattern& pattern, const std::string& code,
                              std::vector<Token>& tokens) = delete;

  std::vector<Token> tokenizeBatch(const std::string& code, size_t start, size_t length);
  std::vector<Token> mergeTokens(std::vector<std::vector<Token>>& batchResults);
  bool isPatternBoundary(const std::string& code, size_t pos);
  bool findBestMatch(const std::string& code, size_t pos, const std::vector<OnigRegex>& regexes,
                     OnigRegion* bestRegion, size_t& bestRegexIndex);

  void resolveStyles(std::vector<Token>& tokens);

  // Style resolution helpers
  void resolveStylesParallel(std::vector<Token>& tokens);
  void resolveStylesBatch(
      std::vector<Token>::iterator start,
      std::vector<Token>::iterator end,
      shiki::StyleCache& styleCache,
      size_t& resolvedCount,
      size_t& cacheHits);
};

} // namespace shiki

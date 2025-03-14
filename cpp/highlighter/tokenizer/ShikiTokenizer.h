#pragma once
#include <xxhash.h>

#include <future>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Token.h"
#include "highlighter/cache/StyleCache.h"
#include "highlighter/core/Configuration.h"
#include "highlighter/grammar/Grammar.h"
#include "highlighter/renderer/LineNumbers.h"
#include "highlighter/text/TokenRange.h"
#include "highlighter/theme/Theme.h"
#include "highlighter/utils/ConcurrencyUtil.h"
#include "oniguruma.h"

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

// Token cache entry that stores tokens for a region of text
struct TokenCacheEntry {
  std::vector<Token> tokens;
  std::string textHash;  // Hash of the text region
  size_t lastUsed;  // Timestamp for LRU

  TokenCacheEntry() = default;
  TokenCacheEntry(std::vector<Token> t, std::string hash, size_t time)
    : tokens(std::move(t)), textHash(std::move(hash)), lastUsed(time) {}
};

struct CompiledPatternsKey {
  std::string language;

  bool operator==(const CompiledPatternsKey& other) const {
    return language == other.language;
  }

  struct Hash {
    size_t operator()(const CompiledPatternsKey& key) const {
      return std::hash<std::string>()(key.language);
    }
  };
};

struct TokenCacheKey {
  std::string language;
  std::string theme;
  std::string codeHash;

  bool operator==(const TokenCacheKey& other) const {
    return language == other.language && theme == other.theme && codeHash == other.codeHash;
  }

  struct Hash {
    size_t operator()(const TokenCacheKey& key) const {
      size_t h1 = std::hash<std::string>()(key.language);
      size_t h2 = std::hash<std::string>()(key.theme);
      size_t h3 = std::hash<std::string>()(key.codeHash);
      return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
  };
};

class ShikiTokenizer {
 public:
  static ShikiTokenizer& getInstance();
  ~ShikiTokenizer();

  void setGrammar(std::shared_ptr<Grammar> grammar);
  void setTheme(std::shared_ptr<Theme> theme);
  std::shared_ptr<Grammar> getGrammar() const {
    return grammar_;
  }
  std::shared_ptr<Theme> getTheme() const {
    return theme_;
  }
  std::vector<Token> tokenize(const std::string& code);
  std::vector<Token> tokenizeParallel(const std::string& code, size_t batchSize = DEFAULT_BATCH_SIZE);

  std::vector<Token> tokenize(const std::string& code, const std::string& language, const std::string& theme);
  std::vector<Token> tokenizeParallel(
    const std::string& code,
    const std::string& language,
    const std::string& theme,
    size_t batchSize = DEFAULT_BATCH_SIZE
  );

  ThemeStyle resolveStyle(const std::string& scope) const;
  ThemeStyle resolveStyle(const std::string& scope, const std::string& themeName) const;

 private:
  ShikiTokenizer() = default;

  // Thread-local storage for temporary buffers
  static thread_local std::vector<std::string> scopeStack_;
  static thread_local std::unordered_set<size_t> processedPositions_;

  // Mutex for general state changes
  mutable std::shared_mutex mutex_;

  // Mutex specifically for pattern compilation
  std::mutex pattern_mutex_;
  std::shared_ptr<Grammar> grammar_;
  std::shared_ptr<Theme> theme_;
  std::string defaultLanguage_;
  std::string defaultTheme_;
  std::unordered_map<CompiledPatternsKey, std::vector<CompiledPattern>, CompiledPatternsKey::Hash>
    compiledPatternsCache_;

  std::vector<CompiledPattern> compiledPatterns_;

  std::unique_ptr<ConcurrencyUtil> concurrencyUtil_;
  std::unique_ptr<LineNumbers> lineNumbers_;

  // Token cache
  static constexpr size_t MIN_CACHE_REGION_SIZE = 1024;  // 1KB minimum for caching
  static constexpr size_t MAX_TOKEN_CACHE_ENTRIES = 100;
  mutable std::mutex tokenCacheMutex_;
  mutable std::unordered_map<TokenCacheKey, TokenCacheEntry, TokenCacheKey::Hash> tokenCache_;
  mutable size_t cacheTimestamp_ = 0;

  // Private methods
  void clearCompiledPatterns();
  void clearCompiledPatterns(const std::string& language);
  void compilePatterns();
  void compilePatterns(const std::string& language);
  std::vector<CompiledPattern>& getCompiledPatterns(const std::string& language);

  std::vector<Token> tokenizeBatch(const std::string& code, size_t start, size_t length);
  std::vector<Token> tokenizeBatch(
    const std::string& code,
    size_t start,
    size_t length,
    const std::string& language,
    const std::string& theme
  );
  std::vector<Token> mergeTokens(std::vector<std::vector<Token>>& batchResults);
  bool isPatternBoundary(const std::string& code, size_t pos);
  bool findBestMatch(
    const std::string& code,
    size_t pos,
    const std::vector<OnigRegex>& regexes,
    OnigRegion* bestRegion,
    size_t& bestRegexIndex
  );

  // Cache management
  std::string computeTextHash(const std::string& text) const;
  std::optional<std::vector<Token>> tryGetCachedTokens(const std::string& text);
  std::optional<std::vector<Token>>
  tryGetCachedTokens(const std::string& text, const std::string& language, const std::string& theme);
  void cacheTokens(const std::string& text, const std::vector<Token>& tokens);
  void cacheTokens(
    const std::string& text,
    const std::vector<Token>& tokens,
    const std::string& language,
    const std::string& theme
  );
  void evictOldestTokenCache();

  // Style resolution
  void resolveStyles(std::vector<Token>& tokens);
  void resolveStyles(std::vector<Token>& tokens, const std::string& themeName);
  void resolveStylesParallel(std::vector<Token>& tokens);
  void resolveStylesParallel(std::vector<Token>& tokens, const std::string& themeName);
  void resolveStylesBatch(
    std::vector<Token>::iterator start,
    std::vector<Token>::iterator end,
    StyleCache& styleCache,
    size_t& resolvedCount,
    size_t& cacheHits
  );
  void resolveStylesBatch(
    std::vector<Token>::iterator start,
    std::vector<Token>::iterator end,
    StyleCache& styleCache,
    size_t& resolvedCount,
    size_t& cacheHits,
    const std::string& themeName
  );

  // Pattern processing
  void processPatterns(
    const std::vector<GrammarPattern>& patterns,
    const std::string& code,
    std::vector<Token>& tokens,
    std::vector<std::string>& scopeStack
  );
  void processPatterns(
    const std::vector<CompiledPattern>& patterns,
    const std::string& code,
    std::vector<Token>& tokens,
    std::vector<std::string>& scopeStack
  );

  friend class TokenCleaner;
};

}  // namespace shiki

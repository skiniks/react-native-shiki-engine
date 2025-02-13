#pragma once
#include "../grammar/Grammar.h"
#include "../tokenizer/ShikiTokenizer.h"
#include <list>
#include <memory>
#include <string>
#include <unordered_map>

namespace shiki {

struct SyntaxTreeNode {
  std::string scope;
  size_t start;
  size_t length;
  std::vector<std::shared_ptr<SyntaxTreeNode>> children;
};

class SyntaxTreeCache {
public:
  static SyntaxTreeCache& getInstance();

  // Cache a syntax tree for a given text and grammar
  void cacheTree(const std::string& text, const std::string& grammarId,
                 std::shared_ptr<SyntaxTreeNode> tree);

  // Retrieve cached tree if available
  std::shared_ptr<SyntaxTreeNode> getCachedTree(const std::string& text,
                                                const std::string& grammarId);

  // Clear cache entries
  void clearCache();

  // Configure cache size
  void setMaxCacheSize(size_t size) {
    maxCacheSize_ = size;
  }

  // Get cache metrics
  size_t getCacheSize() const {
    return cache_.size();
  }
  size_t getCacheHits() const {
    return cacheHits_;
  }
  size_t getCacheMisses() const {
    return cacheMisses_;
  }
  void resetMetrics() {
    cacheHits_ = cacheMisses_ = 0;
  }

private:
  SyntaxTreeCache() = default;

  struct CacheKey {
    std::string text;
    std::string grammarId;

    bool operator==(const CacheKey& other) const {
      return text == other.text && grammarId == other.grammarId;
    }
  };

  struct CacheKeyHash {
    size_t operator()(const CacheKey& key) const {
      return std::hash<std::string>()(key.text) ^ std::hash<std::string>()(key.grammarId);
    }
  };

  using LRUList = std::list<CacheKey>;
  std::unordered_map<CacheKey, std::pair<std::shared_ptr<SyntaxTreeNode>, LRUList::iterator>,
                     CacheKeyHash>
      cache_;
  LRUList lruList_;
  size_t maxCacheSize_ = 100; // Default cache size

  // Metrics
  size_t cacheHits_ = 0;
  size_t cacheMisses_ = 0;

  void evictLRU();
};

} // namespace shiki

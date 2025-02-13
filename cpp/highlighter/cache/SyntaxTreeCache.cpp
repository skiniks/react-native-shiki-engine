#include "SyntaxTreeCache.h"
#include <list>

namespace shiki {

SyntaxTreeCache& SyntaxTreeCache::getInstance() {
  static SyntaxTreeCache instance;
  return instance;
}

void SyntaxTreeCache::evictLRU() {
  if (!lruList_.empty()) {
    auto lru = lruList_.back();
    cache_.erase(lru);
    lruList_.pop_back();
  }
}

void SyntaxTreeCache::cacheTree(const std::string& text, const std::string& grammarId,
                                std::shared_ptr<SyntaxTreeNode> tree) {
  if (!tree)
    return;

  CacheKey key{text, grammarId};

  // Remove existing entry if present
  auto it = cache_.find(key);
  if (it != cache_.end()) {
    lruList_.erase(it->second.second);
    cache_.erase(it);
  }

  // Implement LRU eviction if cache is full
  while (cache_.size() >= maxCacheSize_) {
    evictLRU();
  }

  // Add to front of LRU list
  lruList_.push_front(key);
  cache_[key] = std::make_pair(tree, lruList_.begin());
}

std::shared_ptr<SyntaxTreeNode> SyntaxTreeCache::getCachedTree(const std::string& text,
                                                               const std::string& grammarId) {

  CacheKey key{text, grammarId};
  auto it = cache_.find(key);

  if (it != cache_.end()) {
    // Move to front of LRU list
    lruList_.erase(it->second.second);
    lruList_.push_front(key);
    it->second.second = lruList_.begin();

    cacheHits_++;
    return it->second.first;
  }

  cacheMisses_++;
  return nullptr;
}

void SyntaxTreeCache::clearCache() {
  cache_.clear();
  lruList_.clear();
}

} // namespace shiki

#pragma once
#include <xxhash.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "Cache.h"
#include "CacheConfig.h"
#include "highlighter/theme/ThemeStyle.h"

namespace shiki {

class StyleCache {
 public:
  static StyleCache& getInstance() {
    static StyleCache instance;
    return instance;
  }

  void cacheStyle(const std::string& scope, const ThemeStyle& style) {
    size_t styleSize = estimateStyleSize(style);
    uint64_t scopeHash = computeScopeHash(scope);

    // Common scopes like 'source', 'keyword', 'string', etc. get high priority
    Cache<uint64_t, ThemeStyle>::Priority priority;
    if (isCommonScope(scopeHash)) {
      priority = Cache<uint64_t, ThemeStyle>::Priority::HIGH;
    } else {
      auto usageIt = scopeUsage_.find(scopeHash);
      priority = (usageIt != scopeUsage_.end() && usageIt->second > 5) ? Cache<uint64_t, ThemeStyle>::Priority::HIGH
                                                                       : Cache<uint64_t, ThemeStyle>::Priority::NORMAL;
    }

    cache_.add(scopeHash, style, styleSize, priority);
  }

  std::optional<ThemeStyle> getCachedStyle(const std::string& scope) {
    uint64_t scopeHash = computeScopeHash(scope);
    auto result = cache_.get(scopeHash);
    if (result) {
      // Track usage for priority decisions using hash
      scopeUsage_[scopeHash]++;
    }
    return result;
  }

  void clear() {
    cache_.clear();
    scopeUsage_.clear();
  }

  Cache<uint64_t, ThemeStyle>::CacheMetrics getMetrics() const {
    return cache_.getMetrics();
  }

 private:
  StyleCache() : cache_(cache::StyleCacheConfig::MEMORY_LIMIT, cache::StyleCacheConfig::ENTRY_LIMIT) {
    // Initialize common scopes with their hashes
    const std::vector<std::string> commonScopesList = {
      "source",
      "keyword",
      "string",
      "comment",
      "constant",
      "variable",
      "function",
      "storage",
      "entity.name",
      "punctuation",
      "meta"
    };

    for (const auto& scope : commonScopesList) {
      commonScopeHashes_.insert(computeScopeHash(scope));
    }
  }

  uint64_t computeScopeHash(const std::string& scope) const {
    XXH3_state_t* const state = XXH3_createState();
    if (!state) return 0;

    XXH3_64bits_reset(state);
    XXH3_64bits_update(state, scope.data(), scope.size());
    const uint64_t hash = XXH3_64bits_digest(state);
    XXH3_freeState(state);

    return hash;
  }

  bool isCommonScope(uint64_t scopeHash) const {
    return commonScopeHashes_.find(scopeHash) != commonScopeHashes_.end();
  }

  size_t estimateStyleSize(const ThemeStyle& style) const {
    // Base size + color strings + font info
    return sizeof(ThemeStyle) + style.foreground.capacity() + style.background.capacity();
  }

  Cache<uint64_t, ThemeStyle> cache_;
  std::unordered_map<uint64_t, size_t> scopeUsage_;
  std::unordered_set<uint64_t> commonScopeHashes_;
};

}  // namespace shiki

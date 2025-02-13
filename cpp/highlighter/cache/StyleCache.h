#pragma once
#include "Cache.h"
#include "CacheConfig.h"
#include "../theme/ThemeStyle.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace shiki {

class StyleCache {
public:
  static StyleCache& getInstance() {
    static StyleCache instance;
    return instance;
  }

  void cacheStyle(const std::string& scope, const ThemeStyle& style) {
    size_t styleSize = estimateStyleSize(style);

    // Common scopes like 'source', 'keyword', 'string', etc. get high priority
    Cache<std::string, ThemeStyle>::Priority priority;
    if (isCommonScope(scope)) {
      priority = Cache<std::string, ThemeStyle>::Priority::HIGH;
    } else {
      auto usageIt = scopeUsage_.find(scope);
      priority = (usageIt != scopeUsage_.end() && usageIt->second > 5)
          ? Cache<std::string, ThemeStyle>::Priority::HIGH
          : Cache<std::string, ThemeStyle>::Priority::NORMAL;
    }

    cache_.add(scope, style, styleSize, priority);
  }

  std::optional<ThemeStyle> getCachedStyle(const std::string& scope) {
    auto result = cache_.get(scope);
    if (result) {
      // Track usage for priority decisions
      scopeUsage_[scope]++;
    }
    return result;
  }

  void clear() {
    cache_.clear();
    scopeUsage_.clear();
  }

  Cache<std::string, ThemeStyle>::CacheMetrics getMetrics() const {
    return cache_.getMetrics();
  }

private:
  StyleCache()
      : cache_(cache::StyleCacheConfig::MEMORY_LIMIT,
               cache::StyleCacheConfig::ENTRY_LIMIT) {
    // Initialize common scopes
    commonScopes_ = {
      "source", "keyword", "string", "comment", "constant", "variable",
      "function", "storage", "entity.name", "punctuation", "meta"
    };
  }

  bool isCommonScope(const std::string& scope) const {
    return commonScopes_.find(scope) != commonScopes_.end();
  }

  size_t estimateStyleSize(const ThemeStyle& style) const {
    // Base size + color strings + font info
    return sizeof(ThemeStyle) + style.color.capacity() + style.backgroundColor.capacity();
  }

  Cache<std::string, ThemeStyle> cache_;
  std::unordered_map<std::string, size_t> scopeUsage_;  // Track scope usage frequency
  std::unordered_set<std::string> commonScopes_;  // Store common scopes as member
};

} // namespace shiki

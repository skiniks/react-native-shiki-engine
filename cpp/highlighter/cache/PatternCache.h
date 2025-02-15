#pragma once
#include <oniguruma.h>
#include <xxhash.h>

#include <memory>
#include <string>
#include <unordered_map>

#include "Cache.h"
#include "CacheConfig.h"

namespace shiki {

struct CompiledPattern {
  regex_t* regex;
  time_t timestamp;
  size_t usageCount{0};
  uint64_t hash{0};

  CompiledPattern() : regex(nullptr), timestamp(0) {}

  explicit CompiledPattern(regex_t* r, uint64_t h = 0) : regex(r), timestamp(time(nullptr)), usageCount(0), hash(h) {}

  // Prevent copying
  CompiledPattern(const CompiledPattern&) = delete;
  CompiledPattern& operator=(const CompiledPattern&) = delete;

  // Allow moving
  CompiledPattern(CompiledPattern&& other) noexcept
    : regex(other.regex), timestamp(other.timestamp), usageCount(other.usageCount), hash(other.hash) {
    other.regex = nullptr;
  }

  CompiledPattern& operator=(CompiledPattern&& other) noexcept {
    if (this != &other) {
      if (regex) onig_free(regex);
      regex = other.regex;
      timestamp = other.timestamp;
      usageCount = other.usageCount;
      hash = other.hash;
      other.regex = nullptr;
    }
    return *this;
  }

  ~CompiledPattern() {
    if (regex) {
      onig_free(regex);
      regex = nullptr;
    }
  }
};

class PatternCache {
 public:
  static PatternCache& getInstance() {
    static PatternCache instance;
    return instance;
  }

  void cachePattern(const std::string& pattern, regex_t* regex) {
    if (!regex) return;

    uint64_t patternHash = computePatternHash(pattern);
    auto compiledPattern = std::make_shared<CompiledPattern>(regex, patternHash);
    size_t patternSize = estimatePatternSize(pattern, regex);

    // Check if this pattern was previously used frequently
    auto priorityIt = patternPriorities_.find(patternHash);
    Cache<uint64_t, std::shared_ptr<CompiledPattern>>::Priority priority =
      (priorityIt != patternPriorities_.end() && priorityIt->second > 10)
      ? Cache<uint64_t, std::shared_ptr<CompiledPattern>>::Priority::HIGH
      : Cache<uint64_t, std::shared_ptr<CompiledPattern>>::Priority::NORMAL;

    cache_.add(patternHash, compiledPattern, patternSize, priority);
  }

  regex_t* getCachedPattern(const std::string& pattern) {
    uint64_t patternHash = computePatternHash(pattern);
    auto result = cache_.get(patternHash);
    if (result && *result) {
      auto& compiledPattern = *result;
      // Check if pattern has expired
      if (time(nullptr) - compiledPattern->timestamp > cache::PatternCacheConfig::EXPIRY_SECONDS) {
        return nullptr;
      }
      compiledPattern->timestamp = time(nullptr);  // Update timestamp

      // Update usage count and priority tracking
      compiledPattern->usageCount++;
      patternPriorities_[patternHash] = compiledPattern->usageCount;

      return compiledPattern->regex;
    }
    return nullptr;
  }

  void clear() {
    cache_.clear();
    patternPriorities_.clear();
  }

  Cache<uint64_t, std::shared_ptr<CompiledPattern>>::CacheMetrics getMetrics() const {
    return cache_.getMetrics();
  }

 private:
  PatternCache() : cache_(cache::PatternCacheConfig::MEMORY_LIMIT, cache::PatternCacheConfig::ENTRY_LIMIT) {}

  uint64_t computePatternHash(const std::string& pattern) const {
    XXH3_state_t* const state = XXH3_createState();
    if (!state) return 0;

    XXH3_64bits_reset(state);
    XXH3_64bits_update(state, pattern.data(), pattern.size());
    const uint64_t hash = XXH3_64bits_digest(state);
    XXH3_freeState(state);

    return hash;
  }

  size_t estimatePatternSize(const std::string& pattern, const regex_t* regex) const {
    // Base size + pattern size + estimated regex size
    return sizeof(CompiledPattern) + pattern.capacity() + 1024;
  }

  Cache<uint64_t, std::shared_ptr<CompiledPattern>> cache_;
  std::unordered_map<uint64_t, size_t> patternPriorities_;
};

}  // namespace shiki

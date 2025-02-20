#pragma once
#include <xxhash.h>

#include <atomic>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "Cache.h"
#include "highlighter/core/CacheEntry.h"
#include "highlighter/core/Configuration.h"
#include "highlighter/core/Constants.h"

namespace shiki {

class CacheManager {
 public:
  static CacheManager& getInstance() {
    static CacheManager instance(std::shared_ptr<Configuration>(&Configuration::getInstance(), [](Configuration*) {}));
    return instance;
  }

  explicit CacheManager(std::shared_ptr<Configuration> config)
    : config_(std::move(config)),
      cache_(config_->getDefaults().memoryLimit, config_->getDefaults().entryLimit),
      patternCache_(config_->getDefaults().memoryLimit, config_->getDefaults().entryLimit),
      styleCache_(config_->getDefaults().memoryLimit, config_->getDefaults().entryLimit),
      syntaxTreeCache_(config_->getDefaults().memoryLimit, config_->getDefaults().entryLimit) {
    startTelemetryLogging();
  }

  void add(const std::string& code, const std::string& language, CacheEntry entry) {
    if (!config_->performance.enableCache) return;

    size_t entrySize = entry.getMemoryUsage();
    uint64_t hash = computeHash(code, language);

    Cache<uint64_t, CacheEntry>::Priority priority =
      entry.hitCount > 5 ? Cache<uint64_t, CacheEntry>::Priority::HIGH : Cache<uint64_t, CacheEntry>::Priority::NORMAL;

    cache_.add(hash, std::move(entry), entrySize, priority);
  }

  std::optional<CacheEntry> get(const std::string& code, const std::string& language) {
    if (!config_->performance.enableCache) return std::nullopt;

    uint64_t hash = computeHash(code, language);
    return cache_.get(hash);
  }

  void clear() {
    cache_.clear();
  }

  size_t getCurrentSize() const {
    return cache_.memoryUsage();
  }

  size_t getEntryCount() const {
    return cache_.size();
  }

  Cache<uint64_t, CacheEntry>::CacheMetrics getMetrics() const {
    return cache_.getMetrics();
  }

  const Cache<uint64_t, CacheEntry>& getCache() const {
    return cache_;
  }
  const Cache<uint64_t, CacheEntry>& getPatternCache() const {
    return patternCache_;
  }
  const Cache<uint64_t, CacheEntry>& getStyleCache() const {
    return styleCache_;
  }
  const Cache<uint64_t, CacheEntry>& getSyntaxTreeCache() const {
    return syntaxTreeCache_;
  }

  void logTelemetry() const {
    std::cout << "\n=== Cache Telemetry Report ===\n";
    cache_.logMetrics("Main Cache");
    patternCache_.logMetrics("Pattern Cache");
    styleCache_.logMetrics("Style Cache");
    syntaxTreeCache_.logMetrics("Syntax Tree Cache");
    std::cout << "===========================\n\n";
  }

 private:
  void startTelemetryLogging() {
    telemetryThread_ = std::thread([this]() {
      while (!shouldStopTelemetry_) {
        std::this_thread::sleep_for(std::chrono::minutes(5));
        if (!shouldStopTelemetry_) {
          logTelemetry();
        }
      }
    });
    telemetryThread_.detach();
  }

  void stopTelemetryLogging() {
    shouldStopTelemetry_ = true;
  }

  uint64_t computeHash(const std::string& code, const std::string& language) const {
    XXH3_state_t* const state = XXH3_createState();
    if (!state) return 0;

    XXH3_64bits_reset(state);
    XXH3_64bits_update(state, code.data(), code.size());
    XXH3_64bits_update(state, language.data(), language.size());
    const uint64_t hash = XXH3_64bits_digest(state);
    XXH3_freeState(state);

    return hash;
  }

  std::shared_ptr<Configuration> config_;
  Cache<uint64_t, CacheEntry> cache_;
  Cache<uint64_t, CacheEntry> patternCache_;
  Cache<uint64_t, CacheEntry> styleCache_;
  Cache<uint64_t, CacheEntry> syntaxTreeCache_;
  std::atomic<bool> shouldStopTelemetry_{false};
  std::thread telemetryThread_;

  ~CacheManager() {
    stopTelemetryLogging();
  }
};

}  // namespace shiki

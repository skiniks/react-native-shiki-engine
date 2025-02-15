#pragma once
#include <xxhash.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace shiki {

class ConfigurationValidator;

struct Configuration {
  struct Core {
    std::string language;
    std::string theme;
  } core;

  struct View {
    float fontSize{14.0f};
    std::string fontFamily{"Menlo"};
    std::string fontWeight{"regular"};
    std::string fontStyle{"normal"};
    bool showLineNumbers{false};
    bool scrollEnabled{true};
    bool selectable{true};

    struct Insets {
      float top{0};
      float right{0};
      float bottom{0};
      float left{0};
    } contentInset;
  } view;

  struct Performance {
    size_t maxCacheSize{50 * 1024 * 1024};  // 50MB
    size_t maxCacheEntries{1000};
    size_t batchSize{32 * 1024};  // 32KB
    bool enableIncrementalUpdates{true};
    bool enableBackgroundProcessing{true};
  } performance;

  struct Memory {
    size_t lowMemoryThreshold{50 * 1024 * 1024};  // 50MB
    size_t criticalMemoryThreshold{100 * 1024 * 1024};  // 100MB
    bool preserveStateOnMemoryWarning{false};
  } memory;

  struct Defaults {
    size_t memoryLimit = 50 * 1024 * 1024;  // 50MB
    size_t entryLimit = 1000;
    size_t batchSize = 32 * 1024;  // 32KB
    std::string defaultFontStyle = "normal";
    bool throwOnMissingColors = true;
  };

  void setDefaults(const Defaults& defaults) {
    defaults_ = defaults;
  }

  const Defaults& getDefaults() const {
    return defaults_;
  }

  bool validate(std::string& error) const;
  std::string toJson() const;
  static std::optional<Configuration> fromJson(const std::string& json);

  template <typename T>
  T toPlatformConfig() const;

  ~Configuration() = default;
  Configuration(Configuration&&) = default;
  Configuration& operator=(Configuration&&) = default;

  static Configuration& getInstance() {
    static Configuration instance;
    return instance;
  }

  struct ConfigKey {
    std::string language;
    std::string theme;
    std::string fontFamily;
    float fontSize;
    bool showLineNumbers;

    bool operator==(const ConfigKey& other) const {
      return language == other.language && theme == other.theme && fontFamily == other.fontFamily &&
        fontSize == other.fontSize && showLineNumbers == other.showLineNumbers;
    }

    uint64_t hash() const {
      XXH3_state_t* const state = XXH3_createState();
      if (!state) return 0;

      XXH3_64bits_reset(state);
      XXH3_64bits_update(state, language.data(), language.size());
      XXH3_64bits_update(state, theme.data(), theme.size());
      XXH3_64bits_update(state, fontFamily.data(), fontFamily.size());
      XXH3_64bits_update(state, &fontSize, sizeof(fontSize));
      XXH3_64bits_update(state, &showLineNumbers, sizeof(showLineNumbers));

      const uint64_t hash = XXH3_64bits_digest(state);
      XXH3_freeState(state);

      return hash;
    }
  };

  struct ConfigKeyHash {
    size_t operator()(const ConfigKey& key) const {
      return key.hash();
    }
  };

  struct ConfigValue {
    std::string resolvedTheme;
    std::string resolvedGrammar;
    size_t memoryUsage{0};
    time_t timestamp{0};
    size_t hitCount{0};
  };

  void cacheConfig(const ConfigKey& key, const ConfigValue& value) {
    auto hashKey = key.hash();
    configCache_[hashKey] = value;
  }

  std::optional<ConfigValue> getCachedConfig(const ConfigKey& key) {
    auto hashKey = key.hash();
    auto it = configCache_.find(hashKey);
    if (it != configCache_.end()) {
      it->second.hitCount++;
      it->second.timestamp = time(nullptr);
      return it->second;
    }
    return std::nullopt;
  }

  void clearCache() {
    configCache_.clear();
  }

  size_t getCacheSize() const {
    return configCache_.size();
  }

  size_t getMemoryUsage() const {
    size_t total = 0;
    for (const auto& [_, value] : configCache_) {
      total += value.memoryUsage;
    }
    return total;
  }

 private:
  friend class ConfigurationValidator;
  static ConfigurationValidator& getValidator();
  Configuration() = default;
  Configuration(const Configuration&) = delete;
  Configuration& operator=(const Configuration&) = delete;

  std::unordered_map<uint64_t, ConfigValue> configCache_;
  Defaults defaults_;
};

class ConfigurationValidator {
 public:
  using ValidationRule = std::function<bool(const Configuration&, std::string&)>;

  void addRule(const std::string& name, ValidationRule rule) {
    rules_[name] = std::move(rule);
  }

  bool validate(const Configuration& config, std::string& error) const {
    for (const auto& [name, rule] : rules_) {
      if (!rule(config, error)) {
        return false;
      }
    }
    return true;
  }

 private:
  std::unordered_map<std::string, ValidationRule> rules_;
};

}  // namespace shiki

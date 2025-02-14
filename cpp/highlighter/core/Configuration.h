#pragma once
#include <string>
#include <memory>
#include <optional>
#include <unordered_map>
#include <functional>

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
    size_t maxCacheSize{50 * 1024 * 1024}; // 50MB
    size_t maxCacheEntries{1000};
    size_t batchSize{32 * 1024}; // 32KB
    bool enableIncrementalUpdates{true};
    bool enableBackgroundProcessing{true};
  } performance;

  struct Memory {
    size_t lowMemoryThreshold{50 * 1024 * 1024}; // 50MB
    size_t criticalMemoryThreshold{100 * 1024 * 1024}; // 100MB
    bool preserveStateOnMemoryWarning{false};
  } memory;

  bool validate(std::string& error) const;
  std::string toJson() const;
  static std::optional<Configuration> fromJson(const std::string& json);

  template<typename T>
  T toPlatformConfig() const;

private:
  friend class ConfigurationValidator;
  static ConfigurationValidator& getValidator();
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

} // namespace shiki

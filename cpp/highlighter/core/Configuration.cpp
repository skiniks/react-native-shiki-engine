#include "Configuration.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <sstream>

#include "highlighter/grammar/Grammar.h"
#include "highlighter/platform/PlatformHighlighter.h"
#include "highlighter/theme/Theme.h"

namespace shiki {

ConfigurationValidator& Configuration::getValidator() {
  static ConfigurationValidator validator;
  static bool initialized = false;

  if (!initialized) {
    validator.addRule("core.defaultLanguage", [](const Configuration& config, std::string& error) {
      if (config.core.languages.empty()) {
        error = "At least one language must be loaded";
        return false;
      }
      if (config.core.defaultLanguage.empty()) {
        error = "Default language must be specified";
        return false;
      }
      if (config.core.languages.find(config.core.defaultLanguage) == config.core.languages.end()) {
        error = "Default language must be loaded";
        return false;
      }
      return true;
    });

    validator.addRule("core.defaultTheme", [](const Configuration& config, std::string& error) {
      if (config.core.themes.empty()) {
        error = "At least one theme must be loaded";
        return false;
      }
      if (config.core.defaultTheme.empty()) {
        error = "Default theme must be specified";
        return false;
      }
      if (config.core.themes.find(config.core.defaultTheme) == config.core.themes.end()) {
        error = "Default theme must be loaded";
        return false;
      }
      return true;
    });

    validator.addRule("view.fontSize", [](const Configuration& config, std::string& error) {
      if (config.view.fontSize <= 0) {
        error = "Font size must be positive";
        return false;
      }
      return true;
    });

    validator.addRule("view.fontFamily", [](const Configuration& config, std::string& error) {
      if (config.view.fontFamily.empty()) {
        error = "Font family must be specified";
        return false;
      }
      return true;
    });

    validator.addRule("performance.maxCacheSize", [](const Configuration& config, std::string& error) {
      if (config.performance.maxCacheSize == 0) {
        error = "Cache size cannot be zero";
        return false;
      }
      return true;
    });

    validator.addRule("memory.thresholds", [](const Configuration& config, std::string& error) {
      if (config.memory.criticalMemoryThreshold <= config.memory.lowMemoryThreshold) {
        error = "Critical memory threshold must be greater than low memory threshold";
        return false;
      }
      return true;
    });

    initialized = true;
  }

  return validator;
}

bool Configuration::validate(std::string& error) const {
  return getValidator().validate(*this, error);
}

std::string Configuration::toJson() const {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  writer.StartObject();

  writer.Key("core");
  writer.StartObject();

  writer.Key("defaultLanguage");
  writer.String(core.defaultLanguage.c_str());

  writer.Key("defaultTheme");
  writer.String(core.defaultTheme.c_str());

  writer.Key("languages");
  writer.StartArray();
  for (const auto& [name, _] : core.languages) {
    writer.String(name.c_str());
  }
  writer.EndArray();

  writer.Key("themes");
  writer.StartArray();
  for (const auto& [name, _] : core.themes) {
    writer.String(name.c_str());
  }
  writer.EndArray();

  writer.EndObject();

  writer.Key("view");
  writer.StartObject();
  writer.Key("fontSize");
  writer.Double(view.fontSize);
  writer.Key("fontFamily");
  writer.String(view.fontFamily.c_str());
  writer.Key("fontWeight");
  writer.String(view.fontWeight.c_str());
  writer.Key("fontStyle");
  writer.String(view.fontStyle.c_str());
  writer.Key("showLineNumbers");
  writer.Bool(view.showLineNumbers);
  writer.Key("scrollEnabled");
  writer.Bool(view.scrollEnabled);
  writer.Key("selectable");
  writer.Bool(view.selectable);

  writer.Key("contentInset");
  writer.StartObject();
  writer.Key("top");
  writer.Double(view.contentInset.top);
  writer.Key("right");
  writer.Double(view.contentInset.right);
  writer.Key("bottom");
  writer.Double(view.contentInset.bottom);
  writer.Key("left");
  writer.Double(view.contentInset.left);
  writer.EndObject();

  writer.EndObject();

  writer.Key("performance");
  writer.StartObject();
  writer.Key("maxCacheSize");
  writer.Uint64(performance.maxCacheSize);
  writer.Key("maxCacheEntries");
  writer.Uint64(performance.maxCacheEntries);
  writer.Key("batchSize");
  writer.Uint64(performance.batchSize);
  writer.Key("enableIncrementalUpdates");
  writer.Bool(performance.enableIncrementalUpdates);
  writer.Key("enableBackgroundProcessing");
  writer.Bool(performance.enableBackgroundProcessing);
  writer.EndObject();

  writer.Key("memory");
  writer.StartObject();
  writer.Key("lowMemoryThreshold");
  writer.Uint64(memory.lowMemoryThreshold);
  writer.Key("criticalMemoryThreshold");
  writer.Uint64(memory.criticalMemoryThreshold);
  writer.Key("preserveStateOnMemoryWarning");
  writer.Bool(memory.preserveStateOnMemoryWarning);
  writer.EndObject();

  writer.EndObject();
  return buffer.GetString();
}

std::optional<Configuration> Configuration::fromJson(const std::string& json) {
  rapidjson::Document doc;
  if (doc.Parse(json.c_str()).HasParseError()) {
    return std::nullopt;
  }

  Configuration config;

  if (doc.HasMember("core") && doc["core"].IsObject()) {
    const auto& core = doc["core"];
    if (core.HasMember("defaultLanguage")) config.core.defaultLanguage = core["defaultLanguage"].GetString();
    if (core.HasMember("defaultTheme")) config.core.defaultTheme = core["defaultTheme"].GetString();
  }

  if (doc.HasMember("view") && doc["view"].IsObject()) {
    const auto& view = doc["view"];
    if (view.HasMember("fontSize")) config.view.fontSize = view["fontSize"].GetFloat();
    if (view.HasMember("fontFamily")) config.view.fontFamily = view["fontFamily"].GetString();
    if (view.HasMember("fontWeight")) config.view.fontWeight = view["fontWeight"].GetString();
    if (view.HasMember("fontStyle")) config.view.fontStyle = view["fontStyle"].GetString();
    if (view.HasMember("showLineNumbers")) config.view.showLineNumbers = view["showLineNumbers"].GetBool();
    if (view.HasMember("scrollEnabled")) config.view.scrollEnabled = view["scrollEnabled"].GetBool();
    if (view.HasMember("selectable")) config.view.selectable = view["selectable"].GetBool();

    if (view.HasMember("contentInset") && view["contentInset"].IsObject()) {
      const auto& inset = view["contentInset"];
      if (inset.HasMember("top")) config.view.contentInset.top = inset["top"].GetFloat();
      if (inset.HasMember("right")) config.view.contentInset.right = inset["right"].GetFloat();
      if (inset.HasMember("bottom")) config.view.contentInset.bottom = inset["bottom"].GetFloat();
      if (inset.HasMember("left")) config.view.contentInset.left = inset["left"].GetFloat();
    }
  }

  if (doc.HasMember("performance") && doc["performance"].IsObject()) {
    const auto& perf = doc["performance"];
    if (perf.HasMember("maxCacheSize")) config.performance.maxCacheSize = perf["maxCacheSize"].GetUint64();
    if (perf.HasMember("maxCacheEntries")) config.performance.maxCacheEntries = perf["maxCacheEntries"].GetUint64();
    if (perf.HasMember("batchSize")) config.performance.batchSize = perf["batchSize"].GetUint64();
    if (perf.HasMember("enableIncrementalUpdates"))
      config.performance.enableIncrementalUpdates = perf["enableIncrementalUpdates"].GetBool();
    if (perf.HasMember("enableBackgroundProcessing"))
      config.performance.enableBackgroundProcessing = perf["enableBackgroundProcessing"].GetBool();
  }

  if (doc.HasMember("memory") && doc["memory"].IsObject()) {
    const auto& mem = doc["memory"];
    if (mem.HasMember("lowMemoryThreshold")) config.memory.lowMemoryThreshold = mem["lowMemoryThreshold"].GetUint64();
    if (mem.HasMember("criticalMemoryThreshold"))
      config.memory.criticalMemoryThreshold = mem["criticalMemoryThreshold"].GetUint64();
    if (mem.HasMember("preserveStateOnMemoryWarning"))
      config.memory.preserveStateOnMemoryWarning = mem["preserveStateOnMemoryWarning"].GetBool();
  }

  return std::optional<Configuration>(std::move(config));
}

template <>
PlatformViewConfig Configuration::toPlatformConfig<PlatformViewConfig>() const {
  PlatformViewConfig config;
  config.language = core.defaultLanguage;
  config.theme = core.defaultTheme;
  config.fontSize = view.fontSize;
  config.fontFamily = view.fontFamily;
  config.fontWeight = view.fontWeight;
  config.fontStyle = view.fontStyle;
  config.showLineNumbers = view.showLineNumbers;
  config.scrollEnabled = view.scrollEnabled;
  config.selectable = view.selectable;
  return config;
}

}  // namespace shiki

#include "ThemeParser.h"

#include <rapidjson/document.h>

#include <cctype>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "Theme.h"
#include "highlighter/memory/TextRangePool.h"

class ThemeColor;

namespace shiki {

using Document = rapidjson::Document;

std::vector<std::string> ThemeParser::processScopes(const rapidjson::Value& scopeValue) {
  std::vector<std::string> scopes;

  if (scopeValue.IsString()) {
    std::string scopeStr = scopeValue.GetString();
    std::istringstream scopeStream(scopeStr);
    std::string scope;
    while (std::getline(scopeStream, scope, ',')) {
      scope.erase(0, scope.find_first_not_of(" \t"));
      scope.erase(scope.find_last_not_of(" \t") + 1);
      if (!scope.empty()) {
        scopes.push_back(scope);
      }
    }
  } else if (scopeValue.IsArray()) {
    for (const auto& scope : scopeValue.GetArray()) {
      if (scope.IsString()) {
        std::string scopeStr = scope.GetString();
        std::istringstream scopeStream(scopeStr);
        std::string subScope;
        while (std::getline(scopeStream, subScope, ',')) {
          subScope.erase(0, subScope.find_first_not_of(" \t"));
          subScope.erase(subScope.find_last_not_of(" \t") + 1);
          if (!subScope.empty()) {
            scopes.push_back(subScope);
          }
        }
      }
    }
  }

  return scopes;
}

ThemeStyle ThemeParser::processTokenSettings(const rapidjson::Value& settings) {
  ThemeStyle style;

  if (settings.HasMember("foreground")) {
    std::string color = settings["foreground"].GetString();
    style.foreground = color[0] == '#' ? color : "#" + color;
  }

  if (settings.HasMember("background")) {
    std::string color = settings["background"].GetString();
    style.background = color[0] == '#' ? color : "#" + color;
  }

  if (settings.HasMember("fontStyle")) {
    std::string fontStyle = settings["fontStyle"].GetString();
    style.fontStyle = fontStyle;
    style.bold = fontStyle.find("bold") != std::string::npos;
    style.italic = fontStyle.find("italic") != std::string::npos;
    style.underline = fontStyle.find("underline") != std::string::npos;
  }

  return style;
}

void ThemeParser::processTokenColor(std::shared_ptr<Theme> theme, const rapidjson::Value& tokenColor) {
  if (!tokenColor.HasMember("scope") || !tokenColor.HasMember("settings")) {
    return;
  }

  ThemeStyle style;
  if (tokenColor.HasMember("settings") && tokenColor["settings"].IsObject()) {
    style = processTokenSettings(tokenColor["settings"]);
  }

  if (tokenColor.HasMember("scope")) {
    std::vector<std::string> scopes = processScopes(tokenColor["scope"]);

    for (const auto& scope : scopes) {
      style.scope = scope;
      theme->addStyle(style);
    }
  }
}

void ThemeParser::processColors(std::shared_ptr<Theme> theme, const rapidjson::Value& colors) {
  if (colors.HasMember("editor.background")) {
    std::string colorStr = colors["editor.background"].GetString();
    theme->setBackground(*ThemeColor::fromHex(colorStr));
  }

  if (colors.HasMember("editor.foreground")) {
    std::string colorStr = colors["editor.foreground"].GetString();
    theme->setForeground(*ThemeColor::fromHex(colorStr));
  }
}

ThemeStyle ThemeParser::parseThemeStyle(const std::string& themeContent) {
  ThemeStyle style;
  Document doc;
  doc.Parse(themeContent.c_str());

  if (doc.HasParseError()) {
    throw std::runtime_error("Invalid theme JSON");
  }

  if (doc.HasMember("type")) {
    theme_->type = doc["type"].GetString();
  } else {
    theme_->type = "dark";
  }

  if (doc.HasMember("colors") && doc["colors"].IsObject()) {
    const auto& colors = doc["colors"];
    if (colors.HasMember("editor.foreground")) {
      std::string color = colors["editor.foreground"].GetString();
      theme_->setForeground(*ThemeColor::fromHex(color));
      style.foreground = color[0] == '#' ? color : "#" + color;
    }
    if (colors.HasMember("editor.background")) {
      std::string color = colors["editor.background"].GetString();
      theme_->setBackground(*ThemeColor::fromHex(color));
      style.background = color[0] == '#' ? color : "#" + color;
    }
  }

  if (doc.HasMember("settings") && doc["settings"].IsArray()) {
    const auto& settings = doc["settings"].GetArray();
    for (const auto& setting : settings) {
      if (!setting.HasMember("scope") && !setting.HasMember("name") && setting.HasMember("settings")) {
        const auto& settingsObj = setting["settings"];
        ThemeStyle baseStyle = processTokenSettings(settingsObj);

        if (!baseStyle.foreground.empty()) {
          style.foreground = baseStyle.foreground;
          theme_->setForeground(*ThemeColor::fromHex(style.foreground));
        }
        if (!baseStyle.background.empty()) {
          style.background = baseStyle.background;
          theme_->setBackground(*ThemeColor::fromHex(style.background));
        }
        break;
      }
    }
  }

  if (style.foreground.empty() || style.background.empty()) {
    throw std::runtime_error("Theme must specify both foreground and background colors");
  }

  if (doc.HasMember("tokenColors") && doc["tokenColors"].IsArray()) {
    const auto& tokenColors = doc["tokenColors"].GetArray();
    for (const auto& tokenColor : tokenColors) {
      if (!tokenColor.HasMember("scope") || !tokenColor.HasMember("settings")) {
        continue;
      }

      ThemeStyle tokenStyle = processTokenSettings(tokenColor["settings"]);

      std::vector<std::string> scopes = processScopes(tokenColor["scope"]);

      for (const auto& scope : scopes) {
        ThemeRule rule;
        rule.scope = scope;
        rule.style = tokenStyle;
        theme_->rules.push_back(rule);
      }
    }
  }

  std::unordered_map<std::string, std::string> replacementMap;
  int replacementCount = 0;

  auto getReplacementColor = [&](const std::string& value) -> std::string {
    auto it = replacementMap.find(value);
    if (it != replacementMap.end()) return it->second;
    replacementCount++;
    std::stringstream ss;
    ss << "#" << std::setfill('0') << std::setw(8) << std::hex << replacementCount;
    std::string hex = ss.str();
    replacementMap[value] = hex;
    theme_->colorReplacements[hex] = value;
    return hex;
  };

  for (auto& rule : theme_->rules) {
    if (!rule.style.foreground.empty() && rule.style.foreground[0] != '#')
      rule.style.foreground = getReplacementColor(rule.style.foreground);
    if (!rule.style.background.empty() && rule.style.background[0] != '#')
      rule.style.background = getReplacementColor(rule.style.background);
  }

  if (doc.HasMember("colors") && doc["colors"].IsObject()) {
    const auto& colors = doc["colors"];
    for (auto it = colors.MemberBegin(); it != colors.MemberEnd(); ++it) {
      std::string key = it->name.GetString();
      if (key == "editor.foreground" || key == "editor.background" || key.find("terminal.ansi") == 0) {
        if (it->value.IsString()) {
          std::string color = it->value.GetString();
          if (!color.empty() && color[0] != '#') {
            std::string replacement = getReplacementColor(color);
            theme_->colors.push_back(*ThemeColor::fromHex(replacement));
          }
        }
      }
    }
  }

  return style;
}

std::vector<TokenRange> ThemeParser::tokenize(const std::string& content) {
  std::vector<TokenRange> ranges;
  ranges.reserve(content.length() / 8);
  size_t pos = 0;

  while (pos < content.length()) {
    // Skip whitespace
    while (pos < content.length() && std::isspace(content[pos])) {
      pos++;
    }

    if (pos >= content.length()) break;

    size_t start = pos;

    // Handle string literals
    if (content[pos] == '"' || content[pos] == '\'') {
      char quote = content[pos++];
      while (pos < content.length() && content[pos] != quote) {
        if (content[pos] == '\\') pos++;  // Skip escaped character
        if (pos < content.length()) pos++;
      }
      if (pos < content.length()) pos++;  // Include closing quote

      TokenRange range;
      range.start = start;
      range.length = pos - start;
      range.scopes.push_back("string");
      ranges.push_back(std::move(range));
    }
    // Handle identifiers and keywords
    else if (std::isalpha(content[pos]) || content[pos] == '_') {
      while (pos < content.length() && (std::isalnum(content[pos]) || content[pos] == '_')) {
        pos++;
      }

      TokenRange range;
      range.start = start;
      range.length = pos - start;
      range.scopes.push_back("identifier");
      ranges.push_back(std::move(range));
    }
    // Handle numbers
    else if (std::isdigit(content[pos])) {
      while (pos < content.length() && (std::isdigit(content[pos]) || content[pos] == '.')) {
        pos++;
      }

      TokenRange range;
      range.start = start;
      range.length = pos - start;
      range.scopes.push_back("constant.numeric");
      ranges.push_back(std::move(range));
    }
    // Handle operators and punctuation
    else {
      pos++;
      TokenRange range;
      range.start = start;
      range.length = pos - start;
      range.scopes.push_back("punctuation");
      ranges.push_back(std::move(range));
    }
  }

  return ranges;
}

ThemeStyle ThemeParser::parseStyle(const std::string& json) {
  Document doc;
  doc.Parse(json.c_str());

  if (doc.HasParseError()) {
    throw std::runtime_error("Invalid style JSON");
  }

  ThemeStyle style;
  if (doc.HasMember("scope")) {
    style.fontStyle = doc["scope"].GetString();
  }
  if (doc.HasMember("foreground")) {
    style.foreground = doc["foreground"].GetString();
  }
  if (doc.HasMember("background")) {
    style.background = doc["background"].GetString();
  }
  if (doc.HasMember("fontStyle")) {
    std::string fontStyle = doc["fontStyle"].GetString();
    if (fontStyle.find("bold") != std::string::npos) {
      style.bold = true;
    }
    if (fontStyle.find("italic") != std::string::npos) {
      style.italic = true;
    }
  }

  return style;
}

std::vector<ThemeStyle> ThemeParser::parseStyles(const std::string& json) {
  Document doc;
  doc.Parse(json.c_str());

  if (doc.HasParseError()) {
    throw std::runtime_error("Invalid styles JSON");
  }

  std::vector<ThemeStyle> styles;

  if (!doc.IsArray()) {
    return styles;
  }

  for (const auto& styleJson : doc.GetArray()) {
    ThemeStyle style;

    if (styleJson.HasMember("scope")) {
      if (styleJson["scope"].IsString()) {
        style.fontStyle = styleJson["scope"].GetString();
      } else if (styleJson["scope"].IsArray() && styleJson["scope"].Size() > 0) {
        style.fontStyle = styleJson["scope"][0].GetString();
      }
    }

    if (styleJson.HasMember("foreground")) {
      style.foreground = styleJson["foreground"].GetString();
    }
    if (styleJson.HasMember("background")) {
      style.background = styleJson["background"].GetString();
    }
    if (styleJson.HasMember("fontStyle")) {
      std::string fontStyle = styleJson["fontStyle"].GetString();
      if (fontStyle.find("bold") != std::string::npos) {
        style.bold = true;
      }
      if (fontStyle.find("italic") != std::string::npos) {
        style.italic = true;
      }
    }

    if (!style.fontStyle.empty()) {
      styles.push_back(style);
    }
  }

  return styles;
}

std::shared_ptr<Theme> ThemeParser::parseTheme(const std::string& name, const std::string& content) {
  Document doc;
  doc.Parse(content.c_str());

  if (doc.HasParseError()) {
    throw std::runtime_error("Invalid theme JSON");
  }

  auto theme = std::make_shared<Theme>(name);

  if (doc.HasMember("colors") && doc["colors"].IsObject()) {
    processColors(theme, doc["colors"]);
  }

  if (doc.HasMember("tokenColors") && doc["tokenColors"].IsArray()) {
    for (const auto& tokenColor : doc["tokenColors"].GetArray()) {
      processTokenColor(theme, tokenColor);
    }
  }

  return theme;
}

std::shared_ptr<Theme> ThemeParser::parseThemeFromObject(const rapidjson::Value& themeJson) {
  if (!themeJson.IsObject()) {
    throw std::runtime_error("Theme JSON must be an object");
  }

  auto theme = std::make_shared<Theme>(themeJson["name"].GetString());

  if (themeJson.HasMember("colors") && themeJson["colors"].IsObject()) {
    processColors(theme, themeJson["colors"]);
  }

  if (themeJson.HasMember("tokenColors") && themeJson["tokenColors"].IsArray()) {
    for (const auto& tokenColor : themeJson["tokenColors"].GetArray()) {
      processTokenColor(theme, tokenColor);
    }
  }

  return theme;
}

}  // namespace shiki

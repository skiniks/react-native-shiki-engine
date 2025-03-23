#include "GrammarLoader.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "GrammarParser.h"
#include "highlighter/core/Configuration.h"

namespace shiki {

std::shared_ptr<Grammar> GrammarLoader::loadFromFile(const std::string& filePath) {
  if (!std::filesystem::exists(filePath)) {
    throw HighlightError(HighlightErrorCode::ResourceLoadFailed, "Grammar file not found: " + filePath);
  }

  std::ifstream file(filePath);
  if (!file.is_open()) {
    throw HighlightError(HighlightErrorCode::ResourceLoadFailed, "Failed to open grammar file: " + filePath);
  }

  std::stringstream buffer;
  buffer << file.rdbuf();

  return loadFromJavaScriptModule(buffer.str());
}

void GrammarLoader::registerGrammar(const std::string& scopeName, std::shared_ptr<Grammar> grammar) {
  grammarCache_[scopeName] = std::move(grammar);
}

std::shared_ptr<Grammar> GrammarLoader::getGrammar(const std::string& scopeName) {
  auto it = grammarCache_.find(scopeName);
  if (it != grammarCache_.end()) {
    return it->second;
  }

  return loadGrammarByScopeName(scopeName);
}

std::string GrammarLoader::findGrammarFile(const std::string& scopeName) {
  std::string fileName = scopeName;
  std::replace(fileName.begin(), fileName.end(), '.', '-');
  fileName += ".mjs";

  for (const auto& path : searchPaths_) {
    std::string fullPath = path + "/" + fileName;
    if (std::filesystem::exists(fullPath)) {
      return fullPath;
    }
  }

  size_t lastDot = scopeName.find_last_of('.');
  if (lastDot != std::string::npos) {
    std::string shortName = scopeName.substr(lastDot + 1);

    for (const auto& path : searchPaths_) {
      std::string fullPath = path + "/" + shortName + ".mjs";
      if (std::filesystem::exists(fullPath)) {
        return fullPath;
      }
    }
  }

  return "";
}

std::shared_ptr<Grammar> GrammarLoader::loadGrammarByScopeName(const std::string& scopeName) {
  std::string filePath = findGrammarFile(scopeName);
  if (filePath.empty()) {
    auto& config = Configuration::getInstance();
    config.log(Configuration::LogLevel::Warning, "Grammar file not found for scope name: %s", scopeName.c_str());
    return nullptr;
  }

  try {
    auto grammar = loadFromFile(filePath);

    grammarCache_[scopeName] = grammar;

    return grammar;
  } catch (const HighlightError& e) {
    auto& config = Configuration::getInstance();
    config
      .log(Configuration::LogLevel::Error, "Failed to load grammar for scope name %s: %s", scopeName.c_str(), e.what());
    return nullptr;
  }
}

std::shared_ptr<Grammar> GrammarLoader::loadFromJavaScriptModule(const std::string& jsContent) {
  auto& config = Configuration::getInstance();

  size_t startPos = jsContent.find("export default");
  if (startPos == std::string::npos) {
    throw HighlightError(HighlightErrorCode::InvalidGrammar, "Could not find 'export default' in JavaScript module");
  }

  startPos = jsContent.find("{", startPos);
  if (startPos == std::string::npos) {
    throw HighlightError(HighlightErrorCode::InvalidGrammar, "Could not find JSON object in JavaScript module");
  }

  size_t endPos = jsContent.rfind("}");
  if (endPos == std::string::npos || endPos <= startPos) {
    throw HighlightError(HighlightErrorCode::InvalidGrammar, "Could not find end of JSON object in JavaScript module");
  }

  std::string jsonContent = jsContent.substr(startPos, endPos - startPos + 1);

  std::string cleanJson = jsonContent;
  size_t pos = 0;
  while ((pos = cleanJson.find("'", pos)) != std::string::npos) {
    cleanJson.replace(pos, 1, "\"");
    pos += 1;
  }

  std::regex trailingCommaRegex(R"(,\s*([}\]]))", std::regex::ECMAScript);
  cleanJson = std::regex_replace(cleanJson, trailingCommaRegex, "$1");

  try {
    if (config.isDebugMode()) {
      config.log(Configuration::LogLevel::Debug, "[DEBUG] Loading grammar, content length: %zu", cleanJson.length());
      if (cleanJson.length() > 100) {
        config.log(Configuration::LogLevel::Debug, "[DEBUG] Content preview: %.100s...", cleanJson.c_str());
      }
    }

    auto grammar = Grammar::fromJson(cleanJson);

    if (!grammar->scopeName.empty()) {
      grammarCache_[grammar->scopeName] = grammar;
    }

    return grammar;
  } catch (const HighlightError& e) {
    throw HighlightError(HighlightErrorCode::InvalidGrammar, "Failed to load grammar: " + std::string(e.what()));
  } catch (const std::exception& e) {
    throw HighlightError(HighlightErrorCode::InvalidGrammar, "Failed to parse grammar: " + std::string(e.what()));
  }
}

}  // namespace shiki

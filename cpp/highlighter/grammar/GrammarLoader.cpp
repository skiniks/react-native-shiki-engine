#include "GrammarLoader.h"

#include <iostream>
#include <regex>
#include <sstream>

#include "GrammarParser.h"
#include "highlighter/core/Configuration.h"

namespace shiki {

void GrammarLoader::registerGrammar(const std::string& scopeName, std::shared_ptr<Grammar> grammar) {
  if (!scopeName.empty() && grammar) {
    grammarCache_[scopeName] = std::move(grammar);
  }
}

std::shared_ptr<Grammar> GrammarLoader::getGrammar(const std::string& scopeName) {
  auto it = grammarCache_.find(scopeName);
  if (it != grammarCache_.end()) {
    return it->second;
  }

  logWarning("Grammar not found for scope name: %s", scopeName.c_str());
  return nullptr;
}

std::string GrammarLoader::extractJsonFromModule(const std::string& jsContent) {
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

  return jsContent.substr(startPos, endPos - startPos + 1);
}

std::string GrammarLoader::cleanJsonString(const std::string& jsonContent) {
  std::string cleanJson = jsonContent;
  size_t pos = 0;
  while ((pos = cleanJson.find("'", pos)) != std::string::npos) {
    cleanJson.replace(pos, 1, "\"");
    pos += 1;
  }

  std::regex trailingCommaRegex(R"(,\s*([}\]]))", std::regex::ECMAScript);
  return std::regex_replace(cleanJson, trailingCommaRegex, "$1");
}

std::shared_ptr<Grammar> GrammarLoader::loadFromJavaScriptModule(const std::string& jsContent) {
  try {
    std::string jsonContent = extractJsonFromModule(jsContent);

    std::string cleanJson = cleanJsonString(jsonContent);

    logDebug("Loading grammar, content length: %zu", cleanJson.length());
    if (cleanJson.length() > 100) {
      logDebug("Content preview: %.100s...", cleanJson.c_str());
    }

    auto grammar = Grammar::fromJson(cleanJson);

    if (!grammar->scopeName.empty()) {
      registerGrammar(grammar->scopeName, grammar);
    }

    return grammar;
  } catch (const HighlightError& e) {
    throw HighlightError(HighlightErrorCode::InvalidGrammar, "Failed to load grammar: " + std::string(e.what()));
  } catch (const std::exception& e) {
    throw HighlightError(HighlightErrorCode::InvalidGrammar, "Failed to parse grammar: " + std::string(e.what()));
  }
}

void GrammarLoader::log(Configuration::LogLevel level, const char* format, va_list args) {
  auto& config = Configuration::getInstance();

  char buffer[1024];
  vsnprintf(buffer, sizeof(buffer), format, args);

  config.log(level, "%s", buffer);
}

void GrammarLoader::logDebug(const char* format, ...) {
  auto& config = Configuration::getInstance();
  if (!config.isDebugMode()) {
    return;
  }

  va_list args;
  va_start(args, format);
  log(Configuration::LogLevel::Debug, (std::string("[DEBUG] ") + format).c_str(), args);
  va_end(args);
}

void GrammarLoader::logWarning(const char* format, ...) {
  va_list args;
  va_start(args, format);
  log(Configuration::LogLevel::Warning, format, args);
  va_end(args);
}

void GrammarLoader::logError(const char* format, ...) {
  va_list args;
  va_start(args, format);
  log(Configuration::LogLevel::Error, format, args);
  va_end(args);
}

}  // namespace shiki

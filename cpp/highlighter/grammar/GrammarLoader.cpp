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

std::shared_ptr<Grammar> GrammarLoader::loadFromJavaScriptModule(const std::string& jsContent) {
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

void GrammarLoader::logDebug(const char* format, ...) {
  auto& config = Configuration::getInstance();
  if (!config.isDebugMode()) {
    return;
  }

  va_list args;
  va_start(args, format);
  char buffer[1024];
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  config.log(Configuration::LogLevel::Debug, "[DEBUG] %s", buffer);
}

void GrammarLoader::logWarning(const char* format, ...) {
  auto& config = Configuration::getInstance();

  va_list args;
  va_start(args, format);
  char buffer[1024];
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  config.log(Configuration::LogLevel::Warning, "%s", buffer);
}

void GrammarLoader::logError(const char* format, ...) {
  auto& config = Configuration::getInstance();

  va_list args;
  va_start(args, format);
  char buffer[1024];
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  config.log(Configuration::LogLevel::Error, "%s", buffer);
}

}  // namespace shiki

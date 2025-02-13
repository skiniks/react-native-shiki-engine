#pragma once
#include "../error/HighlightError.h"
#include "Grammar.h"
#include <memory>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <string>

namespace shiki {

class GrammarParser {
public:
  static std::shared_ptr<Grammar> parse(const std::string& jsonStr);
  static std::shared_ptr<Grammar> parseFromFile(const std::string& filePath);
  static bool validateGrammarJson(const std::string& content);

private:
  static void parsePatterns(const rapidjson::Value& patterns, Grammar& grammar);
  static void parsePatterns(const rapidjson::Value& patterns,
                            std::vector<GrammarPattern>& patternList);
  static void parseRepository(const rapidjson::Value& repository, Grammar& grammar);
  static void validateRequiredFields(const rapidjson::Document& document);
};

} // namespace shiki

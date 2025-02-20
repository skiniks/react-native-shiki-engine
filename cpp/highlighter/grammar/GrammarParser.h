#pragma once
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <memory>
#include <string>

#include "Grammar.h"
#include "highlighter/error/HighlightError.h"

namespace shiki {

class GrammarParser {
 public:
  static std::shared_ptr<Grammar> parse(const std::string& jsonStr);
  static std::shared_ptr<Grammar> parseFromFile(const std::string& filePath);
  static bool validateGrammarJson(const std::string& content);

 private:
  static void parsePatterns(const rapidjson::Value& patterns, Grammar& grammar);
  static void parsePatterns(const rapidjson::Value& patterns, std::vector<GrammarPattern>& patternList);
  static void parseRepository(const rapidjson::Value& repository, Grammar& grammar);
  static void validateRequiredFields(const rapidjson::Document& document);
};

}  // namespace shiki

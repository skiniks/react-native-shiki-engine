#include "GrammarParser.h"
#include <fstream>
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <sstream>

namespace shiki {

std::shared_ptr<Grammar> GrammarParser::parse(const std::string& jsonStr) {
  rapidjson::Document document;
  if (document.Parse(jsonStr.c_str()).HasParseError()) {
    throw HighlightError(HighlightErrorCode::InvalidGrammar,
                         "Failed to parse grammar JSON: " +
                             std::string(rapidjson::GetParseError_En(document.GetParseError())));
  }

  try {
    validateRequiredFields(document);

    auto grammar = std::make_shared<Grammar>();

    // Parse basic properties
    grammar->name = document["name"].GetString();
    grammar->scopeName = document["scopeName"].GetString();

    // Parse patterns
    if (document.HasMember("patterns") && document["patterns"].IsArray()) {
      parsePatterns(document["patterns"], *grammar);
    }

    // Parse repository
    if (document.HasMember("repository") && document["repository"].IsObject()) {
      parseRepository(document["repository"], *grammar);
    }

    return grammar;
  } catch (const std::runtime_error& e) {
    throw HighlightError(HighlightErrorCode::InvalidGrammar,
                         "Invalid grammar structure: " + std::string(e.what()));
  }
}

std::shared_ptr<Grammar> GrammarParser::parseFromFile(const std::string& filePath) {
  std::ifstream file(filePath);
  if (!file.is_open()) {
    throw HighlightError(HighlightErrorCode::ResourceLoadFailed,
                         "Failed to open grammar file: " + filePath);
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  return parse(buffer.str());
}

bool GrammarParser::validateGrammarJson(const std::string& content) {
  try {
    rapidjson::Document document;
    if (document.Parse(content.c_str()).HasParseError()) {
      throw HighlightError(HighlightErrorCode::InvalidGrammar,
                           "JSON parse error: " +
                               std::string(rapidjson::GetParseError_En(document.GetParseError())) +
                               " at offset " + std::to_string(document.GetErrorOffset()));
    }

    if (!document.IsObject()) {
      throw HighlightError(HighlightErrorCode::InvalidGrammar, "Grammar must be a JSON object");
    }

    // Check required fields
    const char* requiredFields[] = {"name", "scopeName", "patterns"};
    for (const char* field : requiredFields) {
      if (!document.HasMember(field)) {
        throw HighlightError(HighlightErrorCode::InvalidGrammar,
                             std::string("Missing required field: ") + field);
      }

      if (field == std::string("patterns") && !document[field].IsArray()) {
        throw HighlightError(HighlightErrorCode::InvalidGrammar,
                             "The 'patterns' field must be an array");
      } else if ((field == std::string("name") || field == std::string("scopeName")) &&
                 !document[field].IsString()) {
        throw HighlightError(HighlightErrorCode::InvalidGrammar,
                             std::string("The '") + field + "' field must be a string");
      }
    }

    return true;
  } catch (const HighlightError& e) {
    // Re-throw the error to be caught by the caller
    throw;
  } catch (...) {
    throw HighlightError(HighlightErrorCode::InvalidGrammar,
                         "Unknown error while validating grammar JSON");
  }
}

void GrammarParser::validateRequiredFields(const rapidjson::Document& document) {
  if (!document.IsObject()) {
    throw HighlightError(HighlightErrorCode::InvalidGrammar, "Grammar must be a JSON object");
  }

  // Check required fields
  const char* requiredFields[] = {"name", "scopeName"};
  for (const char* field : requiredFields) {
    if (!document.HasMember(field) || !document[field].IsString()) {
      throw HighlightError(HighlightErrorCode::InvalidGrammar,
                           std::string("Missing or invalid required field: ") + field);
    }
  }
}

void GrammarParser::parsePatterns(const rapidjson::Value& patterns,
                                  std::vector<GrammarPattern>& patternList) {
  for (const auto& pattern : patterns.GetArray()) {
    GrammarPattern grammarPattern;

    // Parse name
    if (pattern.HasMember("name") && pattern["name"].IsString()) {
      grammarPattern.name = pattern["name"].GetString();
    }

    // Parse match pattern
    if (pattern.HasMember("match") && pattern["match"].IsString()) {
      grammarPattern.match = pattern["match"].GetString();

      // Special handling for comment patterns
      if (grammarPattern.name.find("comment") != std::string::npos) {
        std::cout << "[DEBUG] Found comment pattern: " << grammarPattern.match << std::endl;
      }
    }

    // Parse begin/end patterns
    if (pattern.HasMember("begin") && pattern["begin"].IsString()) {
      grammarPattern.begin = pattern["begin"].GetString();
    }
    if (pattern.HasMember("end") && pattern["end"].IsString()) {
      grammarPattern.end = pattern["end"].GetString();
    }

    // Parse captures
    if (pattern.HasMember("captures") && pattern["captures"].IsObject()) {
      for (auto it = pattern["captures"].MemberBegin(); it != pattern["captures"].MemberEnd();
           ++it) {
        if (it->value.HasMember("name") && it->value["name"].IsString()) {
          grammarPattern.captures[std::stoi(it->name.GetString())] = it->value["name"].GetString();
        }
      }
    }

    // Parse begin/end captures
    if (pattern.HasMember("beginCaptures") && pattern["beginCaptures"].IsObject()) {
      for (auto it = pattern["beginCaptures"].MemberBegin();
           it != pattern["beginCaptures"].MemberEnd(); ++it) {
        if (it->value.HasMember("name") && it->value["name"].IsString()) {
          grammarPattern.beginCaptures[std::stoi(it->name.GetString())] =
              it->value["name"].GetString();
        }
      }
    }
    if (pattern.HasMember("endCaptures") && pattern["endCaptures"].IsObject()) {
      for (auto it = pattern["endCaptures"].MemberBegin(); it != pattern["endCaptures"].MemberEnd();
           ++it) {
        if (it->value.HasMember("name") && it->value["name"].IsString()) {
          grammarPattern.endCaptures[std::stoi(it->name.GetString())] =
              it->value["name"].GetString();
        }
      }
    }

    // Parse include
    if (pattern.HasMember("include") && pattern["include"].IsString()) {
      grammarPattern.include = pattern["include"].GetString();
    }

    // Parse nested patterns
    if (pattern.HasMember("patterns") && pattern["patterns"].IsArray()) {
      parsePatterns(pattern["patterns"], grammarPattern.patterns);
    }

    patternList.push_back(std::move(grammarPattern));
  }
}

void GrammarParser::parsePatterns(const rapidjson::Value& patterns, Grammar& grammar) {
  if (patterns.IsArray()) {
    parsePatterns(patterns, grammar.getPatterns());
  }
}

void GrammarParser::parseRepository(const rapidjson::Value& repository, Grammar& grammar) {
  for (auto it = repository.MemberBegin(); it != repository.MemberEnd(); ++it) {
    const std::string key = it->name.GetString();
    const auto& value = it->value;

    GrammarRule rule;
    if (value.HasMember("patterns") && value["patterns"].IsArray()) {
      parsePatterns(value["patterns"], grammar);
    }

    grammar.repository[key] = std::move(rule);
  }
}

} // namespace shiki

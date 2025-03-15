#include "Grammar.h"

#include <sstream>

#include "GrammarParser.h"
#include "highlighter/core/Configuration.h"

namespace shiki {

Grammar::Grammar(const std::string& name) : name(name) {
  // Initialize with default patterns if needed
  if (name == "text") {
    // Add basic text patterns
    GrammarPattern pattern;
    pattern.name = "text";
    pattern.match = ".*";
    addPattern(pattern);
  }
}

void Grammar::validatePattern(const GrammarPattern& pattern) const {
  GrammarPatternValidator::validatePattern(pattern);
}

void Grammar::validateInclude(const std::string& include) const {
  if (include.empty()) {
    throw GrammarError(GrammarErrorCode::InvalidInclude, "Include reference cannot be empty");
  }

  if (include[0] == '#') {
    // Repository reference
    std::string repoName = include.substr(1);
    if (repository.find(repoName) == repository.end()) {
      throw GrammarError(GrammarErrorCode::InvalidInclude, "Repository reference not found: " + repoName);
    }
  } else if (include != "$self") {
    throw GrammarError(
      GrammarErrorCode::InvalidInclude,
      "Only repository (#) and self ($self) references are supported"
    );
  }
}

void Grammar::checkCircularDependency(const std::string& include) {
  if (includeStack_.find(include) != includeStack_.end()) {
    std::stringstream ss;
    ss << "Circular dependency detected in include chain: ";
    for (const auto& inc : includeStack_) {
      ss << inc << " -> ";
    }
    ss << include;
    throw GrammarError(GrammarErrorCode::CircularInclude, ss.str());
  }
}

void Grammar::addPattern(const GrammarPattern& pattern) {
  try {
    validatePattern(pattern);
    patterns.push_back(pattern);
    if (pattern.index >= 0) {
      patternIndexMap_[pattern.index] = patterns.size() - 1;
    }
  } catch (const GrammarError& e) {
    throw GrammarError(e.getGrammarCode(), "Failed to add pattern '" + pattern.name + "': " + e.what());
  }
}

std::string Grammar::getScopeForMatch(size_t patternIndex, const std::vector<std::string>& captures) const {
  auto it = patternIndexMap_.find(static_cast<int>(patternIndex));
  if (it != patternIndexMap_.end()) {
    const auto& pattern = patterns[it->second];

    // If we have captures and they match the pattern's capture groups,
    // return the appropriate scope
    if (!captures.empty() && patternIndex < captures.size()) {
      return captures[patternIndex];
    }

    return pattern.name;
  }
  return "";
}

void Grammar::cacheResolvedInclude(const IncludeKey& key, const std::vector<GrammarPattern>& patterns) {
  includeCache_[key] = patterns;
}

std::vector<GrammarPattern>* Grammar::findCachedInclude(const IncludeKey& key) const {
  auto it = includeCache_.find(key);
  if (it != includeCache_.end()) {
    return &it->second;
  }
  return nullptr;
}

void Grammar::clearIncludeCache() {
  includeCache_.clear();
}

std::vector<GrammarPattern> Grammar::resolveRepositoryReference(const std::string& repoName) {
  auto it = repository.find(repoName);
  if (it == repository.end()) {
    throw GrammarError(GrammarErrorCode::InvalidRepository, "Repository not found: " + repoName);
  }
  return it->second.patterns;
}

std::vector<GrammarPattern> Grammar::resolveSelfReference() {
  return patterns;
}

std::vector<GrammarPattern> Grammar::resolveInclude(const std::string& include, const std::string& repositoryKey) {
  IncludeKey key{include, repositoryKey};

  // Check cache first
  if (auto cached = findCachedInclude(key)) {
    return *cached;
  }

  std::vector<GrammarPattern> resolvedPatterns;
  auto& config = Configuration::getInstance();

  // Resolve based on include type
  if (include[0] == '#') {
    // Repository reference
    resolvedPatterns = resolveRepositoryReference(include.substr(1));
  } else if (include == "$self") {
    resolvedPatterns = resolveSelfReference();
  } else if (include[0] == '$') {
    // Special built-in rule (TextMate specific)
    if (include == "$base") {
      // Base grammar patterns (top-level patterns)
      resolvedPatterns = patterns;
    } else {
      // Other special rules not yet supported
      if (config.isDebugMode()) {
        config.log(Configuration::LogLevel::Warning, "Unsupported built-in rule: %s", include.c_str());
      }
    }
  } else {
    // External grammar reference (TextMate specific)
    // This would require loading another grammar file
    if (config.isDebugMode()) {
      config.log(Configuration::LogLevel::Warning, "External grammar reference not supported: %s", include.c_str());
    }

    // Try to find the grammar in the loaded languages
    auto externalGrammar = config.getLanguage(include);
    if (externalGrammar) {
      resolvedPatterns = externalGrammar->getPatterns();
    } else {
      throw GrammarError(GrammarErrorCode::InvalidInclude, "Unsupported include type: " + include);
    }
  }

  // Cache the result
  cacheResolvedInclude(key, resolvedPatterns);

  return resolvedPatterns;
}

void Grammar::processIncludePattern(GrammarPattern& pattern, const std::string& repositoryKey) {
  try {
    if (!pattern.hasInclude()) {
      return;
    }

    validateInclude(pattern.include);
    checkCircularDependency(pattern.include);

    includeStack_.insert(pattern.include);

    // Resolve the include with caching
    pattern.patterns = resolveInclude(pattern.include, repositoryKey);

    includeStack_.erase(pattern.include);
  } catch (const GrammarError& e) {
    throw GrammarError(e.getGrammarCode(), "Failed to process include '" + pattern.include + "': " + e.what());
  }
}

std::vector<GrammarPattern>
Grammar::processPatterns(const rapidjson::Value& patternsJson, const std::string& repositoryKey) {
  std::vector<GrammarPattern> result;

  if (!patternsJson.IsArray()) {
    throw GrammarError(GrammarErrorCode::InvalidPattern, "Patterns must be an array");
  }

  try {
    for (const auto& pattern : patternsJson.GetArray()) {
      GrammarPattern p;

      // Handle includes
      if (pattern.HasMember("include")) {
        if (!pattern["include"].IsString()) {
          throw GrammarError(GrammarErrorCode::InvalidInclude, "Include must be a string");
        }
        p.include = pattern["include"].GetString();
        processIncludePattern(p, repositoryKey);
        result.push_back(std::move(p));
        continue;
      }

      // Basic pattern properties
      if (pattern.HasMember("match")) {
        if (!pattern["match"].IsString()) {
          throw GrammarError(GrammarErrorCode::InvalidPattern, "Match must be a string");
        }
        p.match = pattern["match"].GetString();
      }
      if (pattern.HasMember("name")) {
        if (!pattern["name"].IsString()) {
          throw GrammarError(GrammarErrorCode::InvalidPattern, "Name must be a string");
        }
        p.name = pattern["name"].GetString();
      }
      if (pattern.HasMember("contentName")) {
        if (!pattern["contentName"].IsString()) {
          throw GrammarError(GrammarErrorCode::InvalidPattern, "ContentName must be a string");
        }
        p.contentName = pattern["contentName"].GetString();
      }
      if (pattern.HasMember("begin")) {
        if (!pattern["begin"].IsString()) {
          throw GrammarError(GrammarErrorCode::InvalidPattern, "Begin must be a string");
        }
        p.begin = pattern["begin"].GetString();
      }
      if (pattern.HasMember("end")) {
        if (!pattern["end"].IsString()) {
          throw GrammarError(GrammarErrorCode::InvalidPattern, "End must be a string");
        }
        p.end = pattern["end"].GetString();
      }
      if (pattern.HasMember("applyEndPatternLast")) {
        if (!pattern["applyEndPatternLast"].IsBool()) {
          throw GrammarError(GrammarErrorCode::InvalidPattern, "ApplyEndPatternLast must be a boolean");
        }
        p.applyEndPatternLast = pattern["applyEndPatternLast"].GetBool();
      }

      // Process captures
      auto processCaptureObject = [](const rapidjson::Value& obj) -> std::unordered_map<int, std::string> {
        std::unordered_map<int, std::string> captures;
        if (!obj.IsObject()) {
          throw GrammarError(GrammarErrorCode::InvalidPattern, "Captures must be an object");
        }
        for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it) {
          // TextMate format: captures can be either an object with a name property or a direct string
          if (it->value.IsObject() && it->value.HasMember("name") && it->value["name"].IsString()) {
            captures[std::stoi(it->name.GetString())] = it->value["name"].GetString();
          } else if (it->value.IsString()) {
            captures[std::stoi(it->name.GetString())] = it->value.GetString();
          } else {
            throw GrammarError(
              GrammarErrorCode::InvalidPattern,
              "Capture must have a string name property or be a string"
            );
          }
        }
        return captures;
      };

      if (pattern.HasMember("captures")) {
        p.captures = processCaptureObject(pattern["captures"]);
      }
      if (pattern.HasMember("beginCaptures")) {
        p.beginCaptures = processCaptureObject(pattern["beginCaptures"]);
      } else if (!p.begin.empty() && !p.captures.empty()) {
        // If no beginCaptures but we have captures and a begin pattern, use captures for beginCaptures (TextMate
        // behavior)
        p.beginCaptures = p.captures;
      }
      if (pattern.HasMember("endCaptures")) {
        p.endCaptures = processCaptureObject(pattern["endCaptures"]);
      } else if (!p.end.empty() && !p.captures.empty()) {
        // If no endCaptures but we have captures and an end pattern, use captures for endCaptures (TextMate behavior)
        p.endCaptures = p.captures;
      }

      // Process nested patterns
      if (pattern.HasMember("patterns")) {
        p.patterns = processPatterns(pattern["patterns"], repositoryKey);
      }

      validatePattern(p);
      result.push_back(std::move(p));
    }
  } catch (const GrammarError& e) {
    throw GrammarError(
      e.getGrammarCode(),
      "Error processing patterns in repository '" + repositoryKey + "': " + e.what()
    );
  }

  return result;
}

std::shared_ptr<Grammar> Grammar::fromJson(const std::string& content) {
  auto& config = Configuration::getInstance();

  try {
    if (config.isDebugMode()) {
      config.log(
        Configuration::LogLevel::Debug,
        "[DEBUG] Grammar::fromJson - Parsing JSON content of length %zu",
        content.length()
      );
    }

    rapidjson::Document doc;
    if (doc.Parse(content.c_str()).HasParseError()) {
      auto error = std::string(rapidjson::GetParseError_En(doc.GetParseError()));
      auto offset = doc.GetErrorOffset();

      if (config.isDebugMode()) {
        config.log(Configuration::LogLevel::Error, "[DEBUG] JSON parse error at offset %zu: %s", offset, error.c_str());

        // Show the context around the error
        size_t start = offset > 20 ? offset - 20 : 0;
        size_t end = start + 40 < content.length() ? start + 40 : content.length();
        std::string context = content.substr(start, end - start);
        config.log(Configuration::LogLevel::Error, "[DEBUG] Error context: %s", context.c_str());
      }

      throw GrammarError(GrammarErrorCode::ValidationError, "Invalid JSON syntax in grammar definition: " + error);
    }

    // Validate the overall grammar schema
    GrammarPatternValidator::validateGrammarSchema(doc);

    auto grammar = GrammarParser::parse(content);

    if (config.isDebugMode()) {
      config.log(
        Configuration::LogLevel::Debug,
        "[DEBUG] Successfully parsed grammar '%s' with %zu patterns",
        grammar->getName().c_str(),
        grammar->getPatterns().size()
      );
    }

    return grammar;
  } catch (const HighlightError& e) {
    if (config.isDebugMode()) {
      config.log(Configuration::LogLevel::Error, "[DEBUG] Failed to parse grammar JSON: %s", e.what());
    }
    throw GrammarError(GrammarErrorCode::ValidationError, "Failed to parse grammar JSON: " + std::string(e.what()));
  }
}

bool Grammar::validateJson(const std::string& content) {
  try {
    rapidjson::Document doc;
    if (doc.Parse(content.c_str()).HasParseError()) {
      return false;
    }
    GrammarPatternValidator::validateGrammarSchema(doc);
    return true;
  } catch (...) {
    return false;
  }
}

}  // namespace shiki

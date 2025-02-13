#include "Grammar.h"
#include "GrammarParser.h"

namespace shiki {

Grammar::Grammar(const std::string& name) : name_(name) {
  // Initialize with default patterns if needed
  if (name == "text") {
    // Add basic text patterns
    ScopePattern pattern;
    pattern.name = "text";
    pattern.match = ".*";
    addPattern(pattern);
  }
}

void Grammar::addPattern(const ScopePattern& pattern) {
  patterns_.push_back(pattern);
  if (pattern.index >= 0) {
    patternIndexMap_[pattern.index] = patterns_.size() - 1;
  }
}

std::string Grammar::getScopeForMatch(size_t patternIndex,
                                      const std::vector<std::string>& captures) const {
  auto it = patternIndexMap_.find(static_cast<int>(patternIndex));
  if (it != patternIndexMap_.end()) {
    const auto& pattern = patterns_[it->second];

    // If we have captures and they match the pattern's capture groups,
    // return the appropriate scope
    if (!captures.empty() && patternIndex < captures.size()) {
      return captures[patternIndex];
    }

    return pattern.name;
  }
  return "";
}

void Grammar::processIncludePattern(GrammarPattern& pattern, const std::string& repository) {
  // Handle different types of includes:
  // #<scope> - repository reference
  // $self - self reference
  // $base - base grammar reference
  // source.xxx - external grammar reference

  if (pattern.include[0] == '#') {
    // Repository reference - look up in current grammar's repository
    std::string repoName = pattern.include.substr(1);
    auto it = repository_.find(repoName);
    if (it != repository_.end()) {
      pattern.patterns = it->second;
    }
  } else if (pattern.include == "$self") {
    // Self reference - use all patterns from current grammar
    for (const auto& p : patterns_) {
      GrammarPattern converted;
      converted.match = p.match;
      converted.name = p.name;
      // Convert captures from vector to map
      for (size_t i = 0; i < p.captures.size(); i++) {
        if (!p.captures[i].empty()) {
          converted.captures[i] = p.captures[i];
        }
      }
      pattern.patterns.push_back(converted);
    }
  }
  // Note: $base and external grammar references would require additional infrastructure
}

std::vector<GrammarPattern> Grammar::processPatterns(const rapidjson::Value& patterns,
                                                     const std::string& repository) {

  std::vector<GrammarPattern> result;

  if (!patterns.IsArray()) {
    return result;
  }

  for (const auto& pattern : patterns.GetArray()) {
    GrammarPattern p;

    // Handle includes
    if (pattern.HasMember("include")) {
      p.include = pattern["include"].GetString();
      processIncludePattern(p, repository);
      result.push_back(std::move(p));
      continue;
    }

    // Basic pattern properties
    if (pattern.HasMember("match")) {
      p.match = pattern["match"].GetString();
    }
    if (pattern.HasMember("name")) {
      p.name = pattern["name"].GetString();
    }
    if (pattern.HasMember("begin")) {
      p.begin = pattern["begin"].GetString();
    }
    if (pattern.HasMember("end")) {
      p.end = pattern["end"].GetString();
    }

    // Process captures
    auto processCaptureObject =
        [](const rapidjson::Value& obj) -> std::unordered_map<int, std::string> {
      std::unordered_map<int, std::string> captures;
      if (obj.IsObject()) {
        for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it) {
          if (it->value.HasMember("name")) {
            captures[std::stoi(it->name.GetString())] = it->value["name"].GetString();
          }
        }
      }
      return captures;
    };

    if (pattern.HasMember("captures")) {
      p.captures = processCaptureObject(pattern["captures"]);
    }
    if (pattern.HasMember("beginCaptures")) {
      p.beginCaptures = processCaptureObject(pattern["beginCaptures"]);
    }
    if (pattern.HasMember("endCaptures")) {
      p.endCaptures = processCaptureObject(pattern["endCaptures"]);
    }

    // Process nested patterns
    if (pattern.HasMember("patterns")) {
      p.patterns = processPatterns(pattern["patterns"], repository);
    }

    result.push_back(std::move(p));
  }

  return result;
}

std::shared_ptr<Grammar> Grammar::fromJson(const std::string& content) {
  try {
    return GrammarParser::parse(content);
  } catch (...) {
    return nullptr;
  }
}

std::shared_ptr<Grammar> Grammar::loadByScope(const std::string& scope) {
  // TODO: Implement grammar loading from file system or embedded resources
  // For now, return nullptr to indicate grammar not found
  return nullptr;
}

bool Grammar::validateJson(const std::string& content) {
  return GrammarParser::validateGrammarJson(content);
}

} // namespace shiki

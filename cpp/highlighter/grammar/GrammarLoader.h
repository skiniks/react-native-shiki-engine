#pragma once
#include <memory>
#include <string>
#include <unordered_map>

#include "Grammar.h"

namespace shiki {

class GrammarLoader {
 public:
  static GrammarLoader& getInstance() {
    static GrammarLoader instance;
    return instance;
  }

  std::shared_ptr<Grammar> loadFromFile(const std::string& filePath);

  std::shared_ptr<Grammar> loadFromJavaScriptModule(const std::string& jsContent);

  void registerGrammar(const std::string& scopeName, std::shared_ptr<Grammar> grammar);

  std::shared_ptr<Grammar> getGrammar(const std::string& scopeName);

  void setSearchPaths(const std::vector<std::string>& paths) {
    searchPaths_ = paths;
  }

  void addSearchPath(const std::string& path) {
    searchPaths_.push_back(path);
  }

  void clearCache() {
    grammarCache_.clear();
  }

 private:
  GrammarLoader() = default;
  GrammarLoader(const GrammarLoader&) = delete;
  GrammarLoader& operator=(const GrammarLoader&) = delete;

  std::string findGrammarFile(const std::string& scopeName);

  std::shared_ptr<Grammar> loadGrammarByScopeName(const std::string& scopeName);

  std::unordered_map<std::string, std::shared_ptr<Grammar>> grammarCache_;
  std::vector<std::string> searchPaths_;
};

}  // namespace shiki

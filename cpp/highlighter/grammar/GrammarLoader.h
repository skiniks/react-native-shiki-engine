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

  std::shared_ptr<Grammar> loadFromJavaScriptModule(const std::string& jsContent);

  void registerGrammar(const std::string& scopeName, std::shared_ptr<Grammar> grammar);
  std::shared_ptr<Grammar> getGrammar(const std::string& scopeName);
  void clearCache() {
    grammarCache_.clear();
  }

 private:
  GrammarLoader() = default;
  GrammarLoader(const GrammarLoader&) = delete;
  GrammarLoader& operator=(const GrammarLoader&) = delete;

  void logDebug(const char* format, ...);
  void logWarning(const char* format, ...);
  void logError(const char* format, ...);

  std::unordered_map<std::string, std::shared_ptr<Grammar>> grammarCache_;
};

}  // namespace shiki

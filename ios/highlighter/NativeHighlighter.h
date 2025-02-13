#pragma once

#ifdef __OBJC__
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#endif

#include "../../cpp/highlighter/text/TextRange.h"
#include "../../cpp/highlighter/theme/Theme.h"
#include "../../cpp/highlighter/theme/ThemeStyle.h"
#include <functional>
#include <memory>
#include <sstream>
#include <string>

namespace shiki {

// Forward declarations
class Grammar;
class Theme;
class ThemeStyle;
class SyntaxTreeNode;

// Constants
static constexpr size_t MAX_CODE_LENGTH = 1024 * 1024; // 1MB

struct StyledToken {
  size_t start;
  size_t length;
  ThemeStyle style;
  std::string scope;
};

struct HighlighterViewConfig {
  std::string language;
  std::string theme;
  bool showLineNumbers{false};
  float fontSize{14.0f};
  std::string fontFamily;
  std::string fontWeight;
  std::string fontStyle;
};

// Callback for view updates
using ViewUpdateCallback = std::function<void(const std::string&, const std::vector<StyledToken>&)>;

class NativeHighlighter {
public:
  static NativeHighlighter& getInstance();

  // Returns false if grammar/theme couldn't be loaded but can continue with defaults
  bool setGrammar(const std::string& content);
  bool setTheme(const std::string& themeName);

  // Core highlighting method that returns tokens with resolved styles
  std::vector<StyledToken> highlight(const std::string& code, bool parallel = true);

  // Native view creation and management
  void* createNativeView(const HighlighterViewConfig& config);
  void updateNativeView(void* view, const std::string& code);
  void destroyNativeView(void* view);

  // Register for view updates
  void setViewUpdateCallback(ViewUpdateCallback callback);

  // Platform-specific style resolution
  ThemeStyle getDefaultStyle() const;
  ThemeStyle getLineNumberStyle() const;

  bool validateGrammar(const std::string& content);

  // Incremental highlighting support
  struct IncrementalUpdate {
    TextRange range;
    std::vector<StyledToken> tokens;
  };

  // Returns regions that need updating with their new tokens
  std::vector<IncrementalUpdate> highlightIncremental(const std::string& newText,
                                                      size_t contextLines = 1);

protected:
  void validateState() const;

private:
  NativeHighlighter() = default;

  std::shared_ptr<Grammar> loadGrammar(const std::string& language);
  std::shared_ptr<Theme> loadTheme(const std::string& themeName);

  std::string currentLanguage_;
  std::string currentTheme_;
  bool initialized_{false};
  std::shared_ptr<Grammar> currentGrammar_;

  ViewUpdateCallback viewUpdateCallback_;

  std::string lastText_;
  std::shared_ptr<SyntaxTreeNode> lastTree_;

  std::shared_ptr<SyntaxTreeNode> buildSyntaxTree(const std::string& code);
  std::vector<StyledToken> tokenizeFromTree(const std::shared_ptr<SyntaxTreeNode>& tree);

  // Helper methods
  std::vector<StyledToken> highlightRange(const std::string& text, const TextRange& range);

  std::shared_ptr<SyntaxTreeNode> buildTreeForRange(const std::string& text,
                                                    const TextRange& range);
};

} // namespace shiki

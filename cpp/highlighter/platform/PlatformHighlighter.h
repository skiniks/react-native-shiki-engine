#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../theme/ThemeStyle.h"

namespace shiki {

// Forward declarations
class Grammar;
class Theme;

struct PlatformViewConfig {
  std::string language;
  std::string theme;
  bool showLineNumbers{false};
  float fontSize{14.0f};
  std::string fontFamily;
  std::string fontWeight;
  std::string fontStyle;
  bool scrollEnabled{true};
  bool selectable{true};
};

struct PlatformStyledToken {
  size_t start;
  size_t length;
  ThemeStyle style;
  std::string scope;
};

class PlatformHighlighter {
 public:
  virtual ~PlatformHighlighter() = default;

  // Core functionality
  virtual bool setGrammar(const std::string& content) = 0;
  virtual bool setTheme(const std::string& themeName) = 0;
  virtual std::vector<PlatformStyledToken> highlight(const std::string& code) = 0;

  // View management
  virtual void* createNativeView(const PlatformViewConfig& config) = 0;
  virtual void updateNativeView(void* view, const std::string& code) = 0;
  virtual void destroyNativeView(void* view) = 0;

  // Configuration
  virtual void setViewConfig(void* view, const PlatformViewConfig& config) = 0;

  // Resource management
  virtual void invalidateResources() = 0;
  virtual void handleMemoryWarning() = 0;

 protected:
  // Shared resources
  std::shared_ptr<Grammar> grammar_;
  std::shared_ptr<Theme> theme_;
};

}  // namespace shiki

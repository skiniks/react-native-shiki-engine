#pragma once

#include <functional>
#include <memory>
#include <string>

#include "../../../cpp/highlighter/platform/PlatformHighlighter.h"

#ifdef __OBJC__
@class ShikiHighlighterView;
#else
typedef void ShikiHighlighterView;
#endif

namespace shiki {

using ViewUpdateCallback = std::function<void(const std::string&, const std::vector<PlatformStyledToken>&)>;

class IOSHighlighter : public PlatformHighlighter {
 public:
  static IOSHighlighter& getInstance();
  virtual ~IOSHighlighter() = default;

  // PlatformHighlighter implementation
  bool setGrammar(const std::string& content) override;
  bool setTheme(const std::string& themeName) override;
  std::vector<PlatformStyledToken> highlight(const std::string& code) override;
  void* createNativeView(const PlatformViewConfig& config) override;
  void updateNativeView(void* view, const std::string& code) override;
  void destroyNativeView(void* view) override;
  void setViewConfig(void* view, const PlatformViewConfig& config) override;
  void invalidateResources() override;
  void handleMemoryWarning() override;

  // iOS-specific methods
  void setUpdateCallback(ViewUpdateCallback callback);

 private:
  IOSHighlighter() = default;
  ViewUpdateCallback updateCallback_;
};

}  // namespace shiki

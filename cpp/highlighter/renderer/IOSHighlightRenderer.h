#pragma once
#include "../../ios/highlighter/NativeHighlighter.h"
#include "../theme/ThemeStyle.h"
#include "FontManager.h"
#include <dispatch/dispatch.h>
#include <memory>
#include <unordered_map>

@class ShikiTextView;

namespace shiki {

// Forward declarations
class VirtualizedContentManager;

struct ViewConfig {
  std::string language;
  std::string theme;
  float fontSize{14.0f};
  std::string fontFamily;
  std::string fontWeight;
  std::string fontStyle;
  bool showLineNumbers{false};
  UIEdgeInsets contentInset{0, 0, 0, 0};
  bool scrollEnabled{true};
  bool selectable{true};
};

// Add new struct for background work
struct StyleComputationWork {
  std::vector<StyledToken> tokens;
  std::string text;
  void* targetView;
  bool isIncremental{false};
};

class IOSHighlightRenderer {
public:
  static IOSHighlightRenderer& getInstance();

  // View lifecycle
  void* createView(const ViewConfig& config);
  void destroyView(void* view);
  void updateView(void* view, const std::vector<StyledToken>& tokens, const std::string& text);

  // Styling
  void setFontConfig(const FontConfig& config) {
    fontConfig_ = config;
  }

  UIFont* getCurrentFont() const {
    return FontManager::getInstance().createFont(fontConfig_);
  }

  void applyLineNumberStyle(const ThemeStyle& style, void* lineNumberView);

  // Incremental highlighting support
  void renderIncrementalHighlights(const std::vector<NativeHighlighter::IncrementalUpdate>& updates,
                                   const std::string& text, void* nativeView);

private:
  IOSHighlightRenderer();
  ~IOSHighlightRenderer();

  FontConfig fontConfig_;
  std::unordered_map<void*, ViewConfig> viewConfigs_;

  // Background queue management
  dispatch_queue_t styleQueue_;
  dispatch_queue_t layoutQueue_;

  // Track pending work
  std::atomic<size_t> pendingWorkCount_{0};

  // Helper methods for background processing
  void processStylesInBackground(StyleComputationWork work);
  void computeLayoutInBackground(NSAttributedString* styledText, void* targetView);
  void applyStylesToViewOnMain(void* view, NSAttributedString* text);

  // Cancel pending work for a view
  void cancelPendingWork(void* view);

  void applyStyle(const ThemeStyle& style, void* attributedString, NSRange range, UIFont* baseFont);

  void configureTextView(UITextView* textView, const ViewConfig& config);

  std::shared_ptr<VirtualizedContentManager> virtualizer_;
};

} // namespace shiki

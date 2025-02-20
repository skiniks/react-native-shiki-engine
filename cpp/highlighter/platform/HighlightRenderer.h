#pragma once

#include <string>
#include <vector>

#include "PlatformHighlighter.h"
#include "highlighter/core/ViewConfig.h"

namespace shiki {

class HighlightRenderer {
 public:
  virtual ~HighlightRenderer() = default;

  virtual void* createView(const ViewConfig& config) = 0;
  virtual void destroyView(void* view) = 0;
  virtual void updateView(void* view, const ViewConfig& config) = 0;
  virtual void renderTokens(void* view, const std::vector<StyledToken>& tokens, const std::string& theme) = 0;

 protected:
  HighlightRenderer() = default;
};

}  // namespace shiki

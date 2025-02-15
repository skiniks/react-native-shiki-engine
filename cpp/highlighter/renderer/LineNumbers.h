#pragma once
#include <string>
#include <vector>

#include "../theme/ThemeStyle.h"

namespace shiki {

class LineNumbers {
 public:
  struct Config {
    bool show{false};
    float fontSize{12.0f};
    std::string fontFamily;
    float padding{8.0f};
    ThemeStyle style;
  };

  struct LineInfo {
    size_t number;
    float y;
    float width;
  };

  explicit LineNumbers(const Config& config);

  // Calculate line positions and dimensions
  void calculateLines(const std::string& code, float contentWidth, float lineHeight);

  // Get line number info for rendering
  const std::vector<LineInfo>& getLines() const {
    return lines_;
  }

  // Get total width needed for line numbers
  float getTotalWidth() const {
    return totalWidth_;
  }

  // Get style for line numbers
  const ThemeStyle& getStyle() const {
    return config_.style;
  }

 private:
  Config config_;
  std::vector<LineInfo> lines_;
  float totalWidth_{0.0f};

  // Helper methods
  size_t countLines(const std::string& code) const;
  float calculateMaxLineWidth(size_t maxLineNumber) const;
};

}  // namespace shiki

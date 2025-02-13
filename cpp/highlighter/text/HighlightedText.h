#pragma once
#include "../core/Constants.h"
#include "../error/HighlightError.h"
#include "../memory/MemoryManager.h"
#include "TextRange.h"
#include <functional>
#include <string>
#include <vector>

namespace shiki {
namespace text {

  class HighlightedText {
  public:
    virtual ~HighlightedText() = default;

    HighlightedText(const std::string& text) {
      if (!MemoryManager::isTextSizeValid(text)) {
        throw HighlightError(HighlightErrorCode::InputTooLarge, "Text size exceeds maximum limit");
      }
      text_ = text;
      initializeHighlighting();
    }

    void setText(const std::string& text) {
      if (!MemoryManager::isTextSizeValid(text)) {
        throw std::runtime_error("Text too large");
      }
      text_ = text;
      clearHighlighting();
    }

    void setRanges(std::vector<TextRange> ranges) {
      ranges_ = std::move(ranges);
      MemoryManager::limitRanges(ranges_);
      updateNativeView();
    }

    const std::string& getText() const {
      return text_;
    }
    const std::vector<TextRange>& getRanges() const {
      return ranges_;
    }

    // Platform-specific implementations
    virtual void updateNativeView() = 0;
    virtual void clearHighlighting() = 0;
    virtual void measureRange(TextRange& range) = 0;

  protected:
    std::string text_;
    std::vector<TextRange> ranges_;

    virtual void initializeHighlighting() {
      ranges_.clear();
    }

    void processInBatches(std::function<void(size_t, size_t)> processor) {
      const size_t length = text_.length();
      size_t position = 0;
      while (position < length) {
        size_t end = std::min(position + text::DEFAULT_BATCH_SIZE, length);
        processor(position, end);
        position = end;
      }
    }
  };

} // namespace text
} // namespace shiki

#pragma once
#include <cstddef>
#include <cstdint>

namespace shiki {

namespace memory {
  static constexpr size_t LOW_THRESHOLD = 50 * 1024 * 1024;  // 50MB
  static constexpr size_t CRITICAL_THRESHOLD = 100 * 1024 * 1024;  // 100MB
  static constexpr float HIGH_PRESSURE = 0.85f;
  static constexpr float CRITICAL_PRESSURE = 0.95f;
}  // namespace memory

namespace cache {
  static constexpr size_t SIZE_LIMIT = 50 * 1024 * 1024;  // 50MB
  static constexpr size_t MAX_ENTRIES = 1000;
}  // namespace cache

namespace text {
  static constexpr size_t MAX_SIZE = 10 * 1024 * 1024;  // 10MB
  static constexpr size_t MAX_RANGES = 100000;
  static constexpr size_t DEFAULT_BATCH_SIZE = 32 * 1024;  // 32KB
}  // namespace text

namespace style {
  static constexpr uint32_t FONT_NORMAL = 0;
  static constexpr uint32_t FONT_BOLD = 1;
  static constexpr uint32_t FONT_ITALIC = 2;
  static constexpr uint32_t FONT_BOLD_ITALIC = FONT_BOLD | FONT_ITALIC;
  static constexpr uint32_t FONT_UNDERLINE = 4;
}  // namespace style

}  // namespace shiki

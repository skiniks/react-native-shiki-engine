#pragma once
#include <cstddef>

namespace shiki {
namespace cache {

// General cache limits
static constexpr size_t DEFAULT_MEMORY_LIMIT = 50 * 1024 * 1024;  // 50MB
static constexpr size_t DEFAULT_ENTRY_LIMIT = 1000;

// Specific cache configurations
struct SyntaxTreeCacheConfig {
  static constexpr size_t MEMORY_LIMIT = 25 * 1024 * 1024;  // 25MB
  static constexpr size_t ENTRY_LIMIT = 500;
};

struct PatternCacheConfig {
  static constexpr size_t MEMORY_LIMIT = 10 * 1024 * 1024;  // 10MB
  static constexpr size_t ENTRY_LIMIT = 200;
  static constexpr size_t EXPIRY_SECONDS = 3600;  // 1 hour
};

struct StyleCacheConfig {
  static constexpr size_t MEMORY_LIMIT = 5 * 1024 * 1024;  // 5MB
  static constexpr size_t ENTRY_LIMIT = 1000;
};

struct TokenCacheConfig {
  static constexpr size_t MEMORY_LIMIT = 10 * 1024 * 1024;  // 10MB
  static constexpr size_t ENTRY_LIMIT = 300;
};

} // namespace cache
} // namespace shiki

#include "pch.h"
#include "ShikiEngine.h"

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace winrt::ShikiEngine
{

namespace {

  std::mutex g_scanners_mutex;
  std::unordered_map<double, OnigContext*> g_scanners;
  double g_next_scanner_id = 1;

  thread_local std::string g_last_utf8;
  thread_local std::vector<int> g_last_b2u;

  bool is_ascii_only(const std::string& utf8) {
    for (unsigned char c : utf8) {
      if (c >= 0x80) {
        return false;
      }
    }
    return true;
  }

  std::vector<int> build_byte_to_utf16_table(const std::string& utf8) {
    std::vector<int> table(utf8.size() + 1);
    int u16 = 0;
    size_t i = 0;
    const size_t n = utf8.size();
    while (i < n) {
      const unsigned char c = static_cast<unsigned char>(utf8[i]);
      size_t len = 1;
      int units = 1;
      if (c < 0x80) {
        len = 1;
      }
      else if ((c & 0xE0) == 0xC0) {
        len = 2;
      }
      else if ((c & 0xF0) == 0xE0) {
        len = 3;
      }
      else if ((c & 0xF8) == 0xF0) {
        len = 4;
        units = 2;
      }
      for (size_t k = 0; k < len && i + k < n; k++) {
        table[i + k] = u16;
      }
      u16 += units;
      i += len;
    }
    table[n] = u16;
    return table;
  }

  int utf16_to_byte_offset(const std::vector<int>& table, int utf16_offset) {
    if (utf16_offset <= 0) {
      return 0;
    }
    const int n = static_cast<int>(table.size()) - 1;
    int lo = 0;
    int hi = n;
    while (lo < hi) {
      const int mid = lo + (hi - lo) / 2;
      if (table[mid] < utf16_offset) {
        lo = mid + 1;
      }
      else {
        hi = mid;
      }
    }
    return lo;
  }

  int byte_to_utf16_offset(const std::vector<int>& table, int byte_offset) {
    if (byte_offset < 0) {
      return byte_offset;
    }
    const int n = static_cast<int>(table.size()) - 1;
    if (byte_offset > n) {
      return table[n];
    }
    return table[byte_offset];
  }

} // namespace

// See https://microsoft.github.io/react-native-windows/docs/native-platform for help writing native modules

void ShikiEngine::Initialize(React::ReactContext const &reactContext) noexcept {
  m_context = reactContext;
}

double ShikiEngine::createScanner(std::vector<std::string> patterns, double maxCacheSize) noexcept {
  const size_t pattern_count = patterns.size();
  
  std::vector<const char*> pattern_ptrs;
  pattern_ptrs.reserve(pattern_count);

  for (const auto& pattern: patterns) {
    pattern_ptrs.push_back(pattern.c_str());
  }

  const size_t cache_hint = maxCacheSize > 0 ? static_cast<size_t>(maxCacheSize) : 0;
  OnigContext* context = create_scanner(pattern_ptrs.data(), static_cast<int>(pattern_count), cache_hint);

  if (!context) {
    _error.Message = "Failed to create scanner";
<<<<<<< HEAD
<<<<<<< HEAD
    return -1;
=======
>>>>>>> a406944 (add Windows support)
=======
    return -1;
>>>>>>> 8ae31ef (cleanup messes which made while testings)
  }

  std::lock_guard<std::mutex> lock(g_scanners_mutex);
  const double scanner_id = g_next_scanner_id++;
  g_scanners[scanner_id] = context;
  owned_scanner_ids_.insert(scanner_id);
  return scanner_id;
}

std::optional<ShikiEngineCodegen::ShikiEngineSpec_findNextMatchSync_returnType> ShikiEngine::findNextMatchSync(double scannerId, std::string text, double startPosition) noexcept {
  OnigContext* context = nullptr;
  {
    std::lock_guard<std::mutex> lock(g_scanners_mutex);
    auto it = g_scanners.find(scannerId);
<<<<<<< HEAD
<<<<<<< HEAD
    if (it == g_scanners.end() || !it->second) {
      _error.Message = "Invalid scanner ID";
      return std::nullopt;
=======
    if (it == g_scanners.end()) {
      _error.Message = "Invalid scanner ID";
>>>>>>> a406944 (add Windows support)
=======
    if (it == g_scanners.end() || !it->second) {
      _error.Message = "Invalid scanner ID";
      return std::nullopt;
>>>>>>> 8ae31ef (cleanup messes which made while testings)
    }
    context = it->second;
  }

  int start_byte = 0;
  const bool ascii = is_ascii_only(text);

  if (ascii) {
    g_last_utf8.clear();
    g_last_b2u.clear();
    start_byte = static_cast<int>(startPosition);
    if (start_byte < 0) {
      start_byte = 0;
    }
  }
  else {
    if (text != g_last_utf8) {
      g_last_utf8 = text;
      g_last_b2u = build_byte_to_utf16_table(text);
    }
    start_byte = utf16_to_byte_offset(g_last_b2u, static_cast<int>(startPosition));
  }

  OnigResult* result = find_next_match(context, text.c_str(), start_byte);
  if (!result) {
    return std::nullopt;
  }

  ShikiEngineCodegen::ShikiEngineSpec_findNextMatchSync_returnType match_obj;
  match_obj.index = static_cast<double>(result->pattern_index);

  std::vector<ShikiEngineCodegen::ShikiEngineSpec_findNextMatchSync_returnType_captureIndices_element> capture_indices;
  capture_indices.reserve(result->capture_count);
  for (int i = 0; i < result->capture_count; i++) {
    ShikiEngineCodegen::ShikiEngineSpec_findNextMatchSync_returnType_captureIndices_element capture;
    int start = result->capture_indices[i * 2];
    int end = result->capture_indices[i * 2 + 1];

    if (!ascii) {
      if (start >= 0) {
        start = byte_to_utf16_offset(g_last_b2u, start);
      }
      if (end >= 0) {
        end = byte_to_utf16_offset(g_last_b2u, end);
      }
    }

    capture.start = static_cast<double>(start);
    capture.end = static_cast<double>(end);
    capture.length = static_cast<double>(end - start);
    capture_indices.push_back(std::move(capture));
  }

  match_obj.captureIndices = std::move(capture_indices);

  free_result(result);
  return match_obj;
}

void ShikiEngine::destroyScanner(double scannerId) noexcept {
  OnigContext* context = nullptr;
  {
    std::lock_guard<std::mutex> lock(g_scanners_mutex);
    auto it = g_scanners.find(scannerId);
    if (it == g_scanners.end()) {
      return;
    }
    context = it->second;
    g_scanners.erase(it);
    owned_scanner_ids_.erase(scannerId);
  }
  free_scanner(context);
}

void ShikiEngine::configureCache(double maxEntries, double maxMemoryBytes) noexcept {
  const size_t entries = maxEntries > 0 ? static_cast<size_t>(maxEntries) : 0;
  const size_t bytes = maxMemoryBytes > 0 ? static_cast<size_t>(maxMemoryBytes) : 0;
  configure_pattern_cache(entries, bytes);
}

void ShikiEngine::clearPatternCache() noexcept {
  clear_unused_pattern_cache();
  g_last_utf8.clear();
  g_last_b2u.clear();
}

void ShikiEngine::trimMemory() noexcept {
  trim_pattern_cache();
  g_last_utf8.clear();
  g_last_b2u.clear();
}

ShikiEngineCodegen::ShikiEngineSpec_getCacheStats_returnType ShikiEngine::getCacheStats() noexcept {
  const OnigCacheStats stats = get_pattern_cache_stats();
  ShikiEngineCodegen::ShikiEngineSpec_getCacheStats_returnType obj;
  obj.entryCount = static_cast<double>(stats.entry_count);
  obj.estimatedBytes = static_cast<double>(stats.estimated_bytes);
  obj.scannerCount = static_cast<double>(stats.scanner_count);
  obj.maxEntries = static_cast<double>(stats.max_entries);
  obj.maxBytes = static_cast<double>(stats.max_bytes);
  return obj;
}

} // namespace winrt::ShikiEngine
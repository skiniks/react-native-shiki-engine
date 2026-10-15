#include "NativeShikiEngineModule.h"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace facebook::react {

namespace {

  std::mutex g_scanners_mutex;
  std::unordered_map<double, OnigContext*> g_scanners;
  double g_next_scanner_id = 1;

  struct InternedString {
    std::string utf8;
    std::vector<int> b2u;
    bool ascii = true;
  };

  std::mutex g_strings_mutex;
  std::unordered_map<double, std::shared_ptr<InternedString>> g_strings;
  double g_next_string_id = 1;

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
      } else if ((c & 0xE0) == 0xC0) {
        len = 2;
      } else if ((c & 0xF0) == 0xE0) {
        len = 3;
      } else if ((c & 0xF8) == 0xF0) {
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
      } else {
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

  std::shared_ptr<InternedString> make_interned_string(std::string utf8) {
    auto interned = std::make_shared<InternedString>();
    interned->ascii = is_ascii_only(utf8);
    interned->utf8 = std::move(utf8);
    if (!interned->ascii) {
      interned->b2u = build_byte_to_utf16_table(interned->utf8);
    }
    return interned;
  }

  jsi::Array pack_match(jsi::Runtime& rt, const OnigResult& result, bool ascii, const std::vector<int>& b2u) {
    const int capture_count = result.capture_count;
    jsi::Array packed(rt, 1 + static_cast<size_t>(capture_count) * 2);
    packed.setValueAtIndex(rt, 0, result.pattern_index);

    for (int i = 0; i < capture_count; i++) {
      int start = result.capture_indices[i * 2];
      int end = result.capture_indices[i * 2 + 1];
      if (!ascii) {
        if (start >= 0) {
          start = byte_to_utf16_offset(b2u, start);
        }
        if (end >= 0) {
          end = byte_to_utf16_offset(b2u, end);
        }
      }
      packed.setValueAtIndex(rt, 1 + i * 2, start);
      packed.setValueAtIndex(rt, 2 + i * 2, end);
    }
    return packed;
  }

  std::optional<jsi::Array>
  search_with_string(jsi::Runtime& rt, OnigContext* context, const InternedString& text, double startPosition) {
    int start_byte = 0;
    if (text.ascii) {
      start_byte = static_cast<int>(startPosition);
      if (start_byte < 0) {
        start_byte = 0;
      }
    } else {
      start_byte = utf16_to_byte_offset(text.b2u, static_cast<int>(startPosition));
    }

    OnigResult* result = find_next_match(context, text.utf8.c_str(), start_byte);
    if (!result) {
      return std::nullopt;
    }

    jsi::Array packed = pack_match(rt, *result, text.ascii, text.b2u);
    free_result(result);
    return packed;
  }

  OnigContext* require_scanner(jsi::Runtime& rt, double scannerId) {
    std::lock_guard<std::mutex> lock(g_scanners_mutex);
    auto it = g_scanners.find(scannerId);
    if (it == g_scanners.end()) {
      throw jsi::JSError(rt, "Invalid scanner ID");
    }
    return it->second;
  }

}  // namespace

NativeShikiEngineModule::NativeShikiEngineModule(std::shared_ptr<CallInvoker> jsInvoker)
  : NativeShikiEngineCxxSpec<NativeShikiEngineModule>(std::move(jsInvoker)) {}

NativeShikiEngineModule::~NativeShikiEngineModule() {
  {
    std::lock_guard<std::mutex> lock(g_scanners_mutex);
    for (const double scanner_id : owned_scanner_ids_) {
      auto it = g_scanners.find(scanner_id);
      if (it == g_scanners.end()) {
        continue;
      }
      free_scanner(it->second);
      g_scanners.erase(it);
    }
    owned_scanner_ids_.clear();
  }

  {
    std::lock_guard<std::mutex> lock(g_strings_mutex);
    for (const double string_id : owned_string_ids_) {
      g_strings.erase(string_id);
    }
    owned_string_ids_.clear();
  }
}

jsi::Object NativeShikiEngineModule::getConstants(jsi::Runtime& rt) {
  return jsi::Object(rt);
}

double NativeShikiEngineModule::createScanner(jsi::Runtime& rt, jsi::Array patterns, double maxCacheSize) {
  const size_t pattern_count = patterns.length(rt);
  std::vector<std::string> pattern_strings;
  std::vector<const char*> pattern_ptrs;
  pattern_strings.reserve(pattern_count);
  pattern_ptrs.reserve(pattern_count);

  for (size_t i = 0; i < pattern_count; i++) {
    jsi::String pattern = patterns.getValueAtIndex(rt, i).asString(rt);
    pattern_strings.push_back(pattern.utf8(rt));
    pattern_ptrs.push_back(pattern_strings.back().c_str());
  }

  const size_t cache_hint = maxCacheSize > 0 ? static_cast<size_t>(maxCacheSize) : 0;
  OnigContext* context = create_scanner(pattern_ptrs.data(), static_cast<int>(pattern_count), cache_hint);

  if (!context) {
    throw jsi::JSError(rt, "Failed to create scanner");
  }

  std::lock_guard<std::mutex> lock(g_scanners_mutex);
  const double scanner_id = g_next_scanner_id++;
  g_scanners[scanner_id] = context;
  owned_scanner_ids_.insert(scanner_id);
  return scanner_id;
}

double NativeShikiEngineModule::internString(jsi::Runtime& rt, jsi::String text) {
  auto interned = make_interned_string(text.utf8(rt));
  std::lock_guard<std::mutex> lock(g_strings_mutex);
  const double string_id = g_next_string_id++;
  g_strings.emplace(string_id, std::move(interned));
  owned_string_ids_.insert(string_id);
  return string_id;
}

void NativeShikiEngineModule::releaseString(jsi::Runtime& rt, double stringId) {
  std::lock_guard<std::mutex> lock(g_strings_mutex);
  if (owned_string_ids_.erase(stringId) == 0) {
    return;
  }
  g_strings.erase(stringId);
}

std::optional<jsi::Array>
NativeShikiEngineModule::findNextMatchSync(jsi::Runtime& rt, double scannerId, double stringId, double startPosition) {
  OnigContext* context = require_scanner(rt, scannerId);

  std::shared_ptr<InternedString> text;
  {
    std::lock_guard<std::mutex> lock(g_strings_mutex);
    auto it = g_strings.find(stringId);
    if (it == g_strings.end()) {
      throw jsi::JSError(rt, "Invalid string ID");
    }
    text = it->second;
  }

  return search_with_string(rt, context, *text, startPosition);
}

void NativeShikiEngineModule::destroyScanner(jsi::Runtime& rt, double scannerId) {
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

void NativeShikiEngineModule::configureCache(jsi::Runtime& rt, double maxEntries, double maxMemoryBytes) {
  const size_t entries = maxEntries > 0 ? static_cast<size_t>(maxEntries) : 0;
  const size_t bytes = maxMemoryBytes > 0 ? static_cast<size_t>(maxMemoryBytes) : 0;
  configure_pattern_cache(entries, bytes);
}

void NativeShikiEngineModule::clearPatternCache(jsi::Runtime& rt) {
  clear_unused_pattern_cache();
}

void NativeShikiEngineModule::trimMemory(jsi::Runtime& rt) {
  trim_pattern_cache();
}

jsi::Object NativeShikiEngineModule::getCacheStats(jsi::Runtime& rt) {
  const OnigCacheStats stats = get_pattern_cache_stats();
  jsi::Object obj(rt);
  obj.setProperty(rt, "entryCount", static_cast<double>(stats.entry_count));
  obj.setProperty(rt, "estimatedBytes", static_cast<double>(stats.estimated_bytes));
  obj.setProperty(rt, "scannerCount", static_cast<double>(stats.scanner_count));
  obj.setProperty(rt, "maxEntries", static_cast<double>(stats.max_entries));
  obj.setProperty(rt, "maxBytes", static_cast<double>(stats.max_bytes));
  return obj;
}

}  // namespace facebook::react

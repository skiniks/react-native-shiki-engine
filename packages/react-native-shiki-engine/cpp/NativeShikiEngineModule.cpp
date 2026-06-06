#include "NativeShikiEngineModule.h"

#include <unordered_map>
#include <vector>

namespace facebook::react {

// Store scanner contexts with their IDs
static std::unordered_map<double, OnigContext*> g_scanners;
static double g_nextScannerId = 1;

// ---- UTF-8 <-> UTF-16 offset conversion ----
//
// vscode-textmate passes/expects offsets in UTF-16 code units (JS string
// indexing), but oniguruma scans the UTF-8 encoded buffer and reports byte
// offsets. For pure-ASCII text the two coincide; any multi-byte character
// (CJK, emoji, ...) shifts every following offset and corrupts token scopes.
// Build a byte->UTF-16 offset table per call and convert in both directions,
// mirroring what the official vscode-oniguruma binding does.

// Returns table of size (bytes + 1): table[byteOffset] = utf16Offset.
// Continuation bytes map to the UTF-16 offset of the code point they belong to.
static std::vector<int> buildByteToUtf16Table(const std::string& utf8) {
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
      units = 2;  // surrogate pair in UTF-16
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

// Converts a UTF-16 code unit offset to the corresponding UTF-8 byte offset.
static int utf16ToByteOffset(const std::vector<int>& table, int utf16Offset) {
  if (utf16Offset <= 0) {
    return 0;
  }
  // table is monotonically non-decreasing; find first byte whose utf16 >= target
  const int n = static_cast<int>(table.size()) - 1;
  for (int b = 0; b <= n; b++) {
    if (table[b] >= utf16Offset) {
      return b;
    }
  }
  return n;
}

static inline int byteToUtf16Offset(const std::vector<int>& table, int byteOffset) {
  if (byteOffset < 0) {
    return byteOffset;
  }
  const int n = static_cast<int>(table.size()) - 1;
  if (byteOffset > n) {
    return table[n];
  }
  return table[byteOffset];
}

NativeShikiEngineModule::NativeShikiEngineModule(std::shared_ptr<CallInvoker> jsInvoker)
  : NativeShikiEngineCxxSpec<NativeShikiEngineModule>(std::move(jsInvoker)) {}

NativeShikiEngineModule::~NativeShikiEngineModule() {
  // Clean up any remaining scanners
  for (const auto& pair : g_scanners) {
    free_scanner(pair.second);
  }
  g_scanners.clear();
}

jsi::Object NativeShikiEngineModule::getConstants(jsi::Runtime& rt) {
  // Create empty constants object as shown in JS implementation
  return jsi::Object(rt);
}

double NativeShikiEngineModule::createScanner(jsi::Runtime& rt, jsi::Array patterns, double maxCacheSize) {
  // Convert JSI array to C string array
  size_t patternCount = patterns.length(rt);
  std::vector<std::string> patternStrings;
  std::vector<const char*> patternPtrs;

  patternStrings.reserve(patternCount);
  patternPtrs.reserve(patternCount);

  for (size_t i = 0; i < patternCount; i++) {
    jsi::String pattern = patterns.getValueAtIndex(rt, i).asString(rt);
    patternStrings.push_back(pattern.utf8(rt));
    patternPtrs.push_back(patternStrings.back().c_str());
  }

  // Create scanner with the provided patterns
  OnigContext* context =
    create_scanner(patternPtrs.data(), static_cast<int>(patternCount), static_cast<size_t>(maxCacheSize));

  if (!context) {
    throw jsi::JSError(rt, "Failed to create scanner");
  }

  // Store scanner and return its ID
  double scannerId = g_nextScannerId++;
  g_scanners[scannerId] = context;
  return scannerId;
}

std::optional<jsi::Object>
NativeShikiEngineModule::findNextMatchSync(jsi::Runtime& rt, double scannerId, jsi::String text, double startPosition) {
  auto it = g_scanners.find(scannerId);
  if (it == g_scanners.end()) {
    throw jsi::JSError(rt, "Invalid scanner ID");
  }

  std::string textStr = text.utf8(rt);

  // JS side (vscode-textmate) speaks UTF-16 offsets; oniguruma speaks UTF-8
  // byte offsets. Convert startPosition in, and all capture indices out.
  const std::vector<int> b2u = buildByteToUtf16Table(textStr);
  const int startByte = utf16ToByteOffset(b2u, static_cast<int>(startPosition));

  OnigResult* result = find_next_match(it->second, textStr.c_str(), startByte);

  if (!result) {
    return std::nullopt;
  }

  // Convert OnigResult to JSI object matching JS implementation
  jsi::Object matchObj(rt);
  matchObj.setProperty(rt, "index", result->pattern_index);

  jsi::Array captureIndices(rt, result->capture_count);
  for (int i = 0; i < result->capture_count; i++) {
    jsi::Object capture(rt);
    int start = result->capture_indices[i * 2];
    int end = result->capture_indices[i * 2 + 1];

    // Unmatched optional groups report negative offsets; pass them through.
    if (start >= 0) {
      start = byteToUtf16Offset(b2u, start);
    }
    if (end >= 0) {
      end = byteToUtf16Offset(b2u, end);
    }

    capture.setProperty(rt, "start", start);
    capture.setProperty(rt, "end", end);
    capture.setProperty(rt, "length", end - start);

    captureIndices.setValueAtIndex(rt, i, std::move(capture));
  }
  matchObj.setProperty(rt, "captureIndices", std::move(captureIndices));

  free_result(result);
  return matchObj;
}

void NativeShikiEngineModule::destroyScanner(jsi::Runtime& rt, double scannerId) {
  auto it = g_scanners.find(scannerId);
  if (it != g_scanners.end()) {
    free_scanner(it->second);
    g_scanners.erase(it);
  }
}

}  // namespace facebook::react

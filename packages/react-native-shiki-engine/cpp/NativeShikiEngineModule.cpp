#include "NativeShikiEngineModule.h"
#include <unordered_map>

namespace facebook::react {

// Store scanner contexts with their IDs
static std::unordered_map<double, OnigContext*> g_scanners;
static double g_nextScannerId = 1;

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

double NativeShikiEngineModule::createScanner(jsi::Runtime& rt, jsi::Array patterns,
                                              double maxCacheSize) {
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
  OnigContext* context = create_scanner(patternPtrs.data(), static_cast<int>(patternCount),
                                        static_cast<size_t>(maxCacheSize));

  if (!context) {
    throw jsi::JSError(rt, "Failed to create scanner");
  }

  // Store scanner and return its ID
  double scannerId = g_nextScannerId++;
  g_scanners[scannerId] = context;
  return scannerId;
}

std::optional<jsi::Object> NativeShikiEngineModule::findNextMatchSync(jsi::Runtime& rt,
                                                                      double scannerId,
                                                                      jsi::String text,
                                                                      double startPosition) {
  auto it = g_scanners.find(scannerId);
  if (it == g_scanners.end()) {
    throw jsi::JSError(rt, "Invalid scanner ID");
  }

  std::string textStr = text.utf8(rt);
  OnigResult* result =
      find_next_match(it->second, textStr.c_str(), static_cast<int>(startPosition));

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

} // namespace facebook::react

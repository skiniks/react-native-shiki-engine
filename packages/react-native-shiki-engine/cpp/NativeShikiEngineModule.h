#pragma once

#include <ReactCommon/CallInvoker.h>

#include <jsi/jsi.h>

#include <memory>
#include <optional>
#include <unordered_set>

#if __has_include(<react/renderer/components/NativeShikiEngineSpec/NativeShikiEngineSpecJSI.h>)
#  include <react/renderer/components/NativeShikiEngineSpec/NativeShikiEngineSpecJSI.h>
#elif __has_include(<React-Codegen/NativeShikiEngineSpecJSI.h>)
#  include <React-Codegen/NativeShikiEngineSpecJSI.h>
#elif __has_include(<ReactCodegen/NativeShikiEngineSpecJSI.h>)
#  include <ReactCodegen/NativeShikiEngineSpecJSI.h>
#elif __has_include(<NativeShikiEngineSpecJSI.h>)
#  include <NativeShikiEngineSpecJSI.h>
#else
#  error "Could not find NativeShikiEngineSpecJSI.h - ensure codegen has run and New Architecture is enabled"
#endif

#if __has_include("onig_regex.h")
#  include "onig_regex.h"
#endif

namespace facebook::react {

class NativeShikiEngineModule : public NativeShikiEngineCxxSpec<NativeShikiEngineModule> {
 public:
  NativeShikiEngineModule(std::shared_ptr<CallInvoker> jsInvoker);
  ~NativeShikiEngineModule();

  jsi::Object getConstants(jsi::Runtime& rt);
  double createScanner(jsi::Runtime& rt, jsi::Array patterns, double maxCacheSize);
  double internString(jsi::Runtime& rt, jsi::String text);
  void releaseString(jsi::Runtime& rt, double stringId);
  std::optional<jsi::Array>
  findNextMatchSync(jsi::Runtime& rt, double scannerId, double stringId, double startPosition);
  void destroyScanner(jsi::Runtime& rt, double scannerId);
  void configureCache(jsi::Runtime& rt, double maxEntries, double maxMemoryBytes);
  void clearPatternCache(jsi::Runtime& rt);
  void trimMemory(jsi::Runtime& rt);
  jsi::Object getCacheStats(jsi::Runtime& rt);

 private:
  std::unordered_set<double> owned_scanner_ids_;
  std::unordered_set<double> owned_string_ids_;
};

}  // namespace facebook::react

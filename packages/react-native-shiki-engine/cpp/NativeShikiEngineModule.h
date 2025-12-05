#pragma once

#if __has_include(<react/renderer/components/NativeShikiEngineSpec/NativeShikiEngineSpecJSI.h>)
#include <react/renderer/components/NativeShikiEngineSpec/NativeShikiEngineSpecJSI.h>
#elif __has_include(<React-Codegen/NativeShikiEngineSpecJSI.h>)
#include <React-Codegen/NativeShikiEngineSpecJSI.h>
#elif __has_include(<ReactCodegen/NativeShikiEngineSpecJSI.h>)
#include <ReactCodegen/NativeShikiEngineSpecJSI.h>
#elif __has_include(<NativeShikiEngineSpecJSI.h>)
#include <NativeShikiEngineSpecJSI.h>
#else
#error "Could not find NativeShikiEngineSpecJSI.h - ensure codegen has run and New Architecture is enabled"
#endif

#if __has_include("onig_regex.h")
#include "onig_regex.h"
#endif

namespace facebook::react {

class NativeShikiEngineModule : public NativeShikiEngineCxxSpec<NativeShikiEngineModule> {
public:
  NativeShikiEngineModule(std::shared_ptr<CallInvoker> jsInvoker);
  ~NativeShikiEngineModule();

  jsi::Object getConstants(jsi::Runtime& rt);
  double createScanner(jsi::Runtime& rt, jsi::Array patterns, double maxCacheSize);
  std::optional<jsi::Object> findNextMatchSync(jsi::Runtime& rt, double scannerId, jsi::String text,
                                               double startPosition);
  void destroyScanner(jsi::Runtime& rt, double scannerId);
};

} // namespace facebook::react

#pragma once

#include <ReactCommon/CallInvoker.h>

#include <jsi/jsi.h>

#include <memory>
#include <optional>
#include <unordered_set>

#include "pch.h"
#include "resource.h"

#if __has_include("codegen/NativeShikiEngineDataTypes.g.h")
  #include "codegen/NativeShikiEngineDataTypes.g.h"
#endif
// Note: The following lines use Mustache template syntax which will be processed during
// project generation to produce standard C++ code. If existing codegen spec files are found,
// use the actual filename; otherwise use conditional includes.
#if __has_include("codegen/NativeShikiEngineSpec.g.h")
  #include "codegen/NativeShikiEngineSpec.g.h"
#endif

#include "NativeModules.h"

namespace winrt::ShikiEngine
{

// See https://microsoft.github.io/react-native-windows/docs/native-platform for help writing native modules

REACT_MODULE(ShikiEngine)
struct ShikiEngine
{
  // Note: Mustache template syntax below will be processed during project generation
  // to produce standard C++ code based on detected codegen files.
#if __has_include("codegen/NativeShikiEngineSpec.g.h")
  using ModuleSpec = ShikiEngineCodegen::ShikiEngineSpec;
#endif

  REACT_INIT(Initialize)
  void Initialize(React::ReactContext const &reactContext) noexcept;

  REACT_SYNC_METHOD(createScanner)
  double createScanner(std::vector<std::string> patterns, double maxCacheSize) noexcept;

  REACT_SYNC_METHOD(findNextMatchSync)
  std::optional<ShikiEngineCodegen::ShikiEngineSpec_findNextMatchSync_returnType> findNextMatchSync(double scannerId, std::string text, double startPosition) noexcept;

  REACT_METHOD(destroyScanner)
  void destroyScanner(double scannerId) noexcept;

  REACT_METHOD(configureCache)
  void configureCache(double maxEntries, double maxMemoryBytes) noexcept;

  REACT_METHOD(clearPatternCache)
  void clearPatternCache() noexcept;

  REACT_METHOD(trimMemory)
  void trimMemory() noexcept;

  REACT_SYNC_METHOD(getCacheStats)
  ShikiEngineCodegen::ShikiEngineSpec_getCacheStats_returnType getCacheStats() noexcept;

private:
  React::ReactContext m_context;
  std::unordered_set<double> owned_scanner_ids_;
  React::ReactError _error;
};

} // namespace winrt::ShikiEngine
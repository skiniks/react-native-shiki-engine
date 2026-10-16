
/*
 * This file is auto-generated from a NativeModule spec file in js.
 *
 * This is a C++ Spec class that should be used with MakeTurboModuleProvider to register native modules
 * in a way that also verifies at compile time that the native module matches the interface required
 * by the TurboModule JS spec.
 */
#pragma once
// clang-format off

// #include "NativeShikiEngineDataTypes.g.h" before this file to use the generated type definition
#include <NativeModules.h>
#include <tuple>

namespace ShikiEngineCodegen {

inline winrt::Microsoft::ReactNative::FieldMap GetStructInfo(ShikiEngineSpec_findNextMatchSync_returnType_captureIndices_element*) noexcept {
    winrt::Microsoft::ReactNative::FieldMap fieldMap {
        {L"start", &ShikiEngineSpec_findNextMatchSync_returnType_captureIndices_element::start},
        {L"end", &ShikiEngineSpec_findNextMatchSync_returnType_captureIndices_element::end},
        {L"length", &ShikiEngineSpec_findNextMatchSync_returnType_captureIndices_element::length},
    };
    return fieldMap;
}

inline winrt::Microsoft::ReactNative::FieldMap GetStructInfo(ShikiEngineSpec_findNextMatchSync_returnType*) noexcept {
    winrt::Microsoft::ReactNative::FieldMap fieldMap {
        {L"index", &ShikiEngineSpec_findNextMatchSync_returnType::index},
        {L"captureIndices", &ShikiEngineSpec_findNextMatchSync_returnType::captureIndices},
    };
    return fieldMap;
}

inline winrt::Microsoft::ReactNative::FieldMap GetStructInfo(ShikiEngineSpec_getCacheStats_returnType*) noexcept {
    winrt::Microsoft::ReactNative::FieldMap fieldMap {
        {L"entryCount", &ShikiEngineSpec_getCacheStats_returnType::entryCount},
        {L"estimatedBytes", &ShikiEngineSpec_getCacheStats_returnType::estimatedBytes},
        {L"scannerCount", &ShikiEngineSpec_getCacheStats_returnType::scannerCount},
        {L"maxEntries", &ShikiEngineSpec_getCacheStats_returnType::maxEntries},
        {L"maxBytes", &ShikiEngineSpec_getCacheStats_returnType::maxBytes},
    };
    return fieldMap;
}

struct ShikiEngineSpec : winrt::Microsoft::ReactNative::TurboModuleSpec {
  static constexpr auto methods = std::tuple{
      SyncMethod<double(std::vector<std::string>, double) noexcept>{0, L"createScanner"},
      SyncMethod<std::optional<ShikiEngineSpec_findNextMatchSync_returnType>(double, std::string, double) noexcept>{1, L"findNextMatchSync"},
      Method<void(double) noexcept>{2, L"destroyScanner"},
      Method<void(double, double) noexcept>{3, L"configureCache"},
      Method<void() noexcept>{4, L"clearPatternCache"},
      Method<void() noexcept>{5, L"trimMemory"},
      SyncMethod<ShikiEngineSpec_getCacheStats_returnType() noexcept>{6, L"getCacheStats"},
  };

  template <class TModule>
  static constexpr void ValidateModule() noexcept {
    constexpr auto methodCheckResults = CheckMethods<TModule, ShikiEngineSpec>();

    REACT_SHOW_METHOD_SPEC_ERRORS(
          0,
          "createScanner",
          "    REACT_SYNC_METHOD(createScanner) double createScanner(std::vector<std::string> const & patterns, double maxCacheSize) noexcept { /* implementation */ }\n"
          "    REACT_SYNC_METHOD(createScanner) static double createScanner(std::vector<std::string> const & patterns, double maxCacheSize) noexcept { /* implementation */ }\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
          1,
          "findNextMatchSync",
          "    REACT_SYNC_METHOD(findNextMatchSync) std::optional<ShikiEngineSpec_findNextMatchSync_returnType> findNextMatchSync(double scannerId, std::string text, double startPosition) noexcept { /* implementation */ }\n"
          "    REACT_SYNC_METHOD(findNextMatchSync) static std::optional<ShikiEngineSpec_findNextMatchSync_returnType> findNextMatchSync(double scannerId, std::string text, double startPosition) noexcept { /* implementation */ }\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
          2,
          "destroyScanner",
          "    REACT_METHOD(destroyScanner) void destroyScanner(double scannerId) noexcept { /* implementation */ }\n"
          "    REACT_METHOD(destroyScanner) static void destroyScanner(double scannerId) noexcept { /* implementation */ }\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
          3,
          "configureCache",
          "    REACT_METHOD(configureCache) void configureCache(double maxEntries, double maxMemoryBytes) noexcept { /* implementation */ }\n"
          "    REACT_METHOD(configureCache) static void configureCache(double maxEntries, double maxMemoryBytes) noexcept { /* implementation */ }\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
          4,
          "clearPatternCache",
          "    REACT_METHOD(clearPatternCache) void clearPatternCache() noexcept { /* implementation */ }\n"
          "    REACT_METHOD(clearPatternCache) static void clearPatternCache() noexcept { /* implementation */ }\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
          5,
          "trimMemory",
          "    REACT_METHOD(trimMemory) void trimMemory() noexcept { /* implementation */ }\n"
          "    REACT_METHOD(trimMemory) static void trimMemory() noexcept { /* implementation */ }\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
          6,
          "getCacheStats",
          "    REACT_SYNC_METHOD(getCacheStats) ShikiEngineSpec_getCacheStats_returnType getCacheStats() noexcept { /* implementation */ }\n"
          "    REACT_SYNC_METHOD(getCacheStats) static ShikiEngineSpec_getCacheStats_returnType getCacheStats() noexcept { /* implementation */ }\n");
  }
};

} // namespace ShikiEngineCodegen

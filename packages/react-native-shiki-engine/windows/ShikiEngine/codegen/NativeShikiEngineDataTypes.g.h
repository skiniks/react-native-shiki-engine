
/*
 * This file is auto-generated from a NativeModule spec file in js.
 *
 * This is a C++ Spec class that should be used with MakeTurboModuleProvider to register native modules
 * in a way that also verifies at compile time that the native module matches the interface required
 * by the TurboModule JS spec.
 */
#pragma once
// clang-format off

#include <string>
#include <optional>
#include <functional>
#include <vector>

namespace ShikiEngineCodegen {

struct ShikiEngineSpec_findNextMatchSync_returnType_captureIndices_element {
    double start;
    double end;
    double length;
};

struct ShikiEngineSpec_findNextMatchSync_returnType {
    double index;
    std::vector<ShikiEngineSpec_findNextMatchSync_returnType_captureIndices_element> captureIndices;
};

struct ShikiEngineSpec_getCacheStats_returnType {
    double entryCount;
    double estimatedBytes;
    double scannerCount;
    double maxEntries;
    double maxBytes;
};

} // namespace ShikiEngineCodegen

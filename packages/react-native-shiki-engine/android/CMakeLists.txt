cmake_minimum_required(VERSION 3.13)
project(ReactNativeShikiEngine)

set(CMAKE_VERBOSE_MAKEFILE ON)

# Find prebuilt oniguruma library
find_library(LOG_LIB log)
find_library(ONIG_LIB onig
    PATHS ${CMAKE_CURRENT_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}
    NO_CMAKE_FIND_ROOT_PATH
    REQUIRED
)

# Main library
add_library(react-native-shiki-engine SHARED
    src/main/cpp/cpp-adapter.cpp
    ../cpp/NativeShikiEngineModule.cpp
    ../cpp/onig_regex.cpp
)

# Include directories for our code
target_include_directories(react-native-shiki-engine
    PUBLIC
        ../cpp
        src/main/cpp/include
)

# Set C++ standard and properties
set_target_properties(react-native-shiki-engine PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

# Tell Folly not to look for folly-config.h
target_compile_definitions(react-native-shiki-engine
    PRIVATE
    -DFOLLY_NO_CONFIG=1
)

# Add codegen files
set(CODEGEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/generated/source/codegen/jni")
target_sources(react-native-shiki-engine PRIVATE
    ${CODEGEN_DIR}/NativeShikiEngineSpec-generated.cpp
    ${CODEGEN_DIR}/react/renderer/components/NativeShikiEngineSpec/NativeShikiEngineSpecJSI-generated.cpp
)

target_include_directories(react-native-shiki-engine PRIVATE
    ${CODEGEN_DIR}
    ${CODEGEN_DIR}/react/renderer/components/NativeShikiEngineSpec
)

# Use prefab to find React Native libraries
find_package(fbjni REQUIRED CONFIG)
find_package(ReactAndroid REQUIRED CONFIG)

# Link libraries
target_link_libraries(react-native-shiki-engine
    ${ONIG_LIB}
    ${LOG_LIB}
    android
    fbjni::fbjni
    ReactAndroid::jsi
    ReactAndroid::reactnative
)

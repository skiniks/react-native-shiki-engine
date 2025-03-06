# set C++ standard
cmake_minimum_required(VERSION 3.13)
set(CMAKE_VERBOSE_MAKEFILE on)

# Define the library name here.
project(appmodules)

# Set paths
set(CPP_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../cpp)
set(THIRDPARTY_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty)
set(NODE_MODULES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../node_modules)
set(REACT_ANDROID_DIR ${NODE_MODULES_DIR}/react-native/ReactAndroid)

# This file includes all the necessary to let you build your application with the New Architecture.
include(${REACT_ANDROID_DIR}/cmake-utils/ReactNative-application.cmake)

# Add xxhash library
add_library(
  xxhash
  STATIC
  ${THIRDPARTY_ROOT}/xxHash/xxhash.c
)

target_include_directories(
  xxhash
  PUBLIC
  ${THIRDPARTY_ROOT}/xxHash
)

# Collect all C++ source files
file(GLOB_RECURSE CPP_SOURCES
  "${CPP_ROOT}/highlighter/**/*.cpp"
  "${CPP_ROOT}/highlighter/**/*.h"
)

# Android platform-specific sources
file(GLOB PLATFORM_SOURCES
  "src/main/cpp/*.cpp"
  "src/main/cpp/*.h"
)

# Add shikibridge library
add_library(
  shikibridge
  SHARED
  ${CPP_SOURCES}
  ${PLATFORM_SOURCES}
  src/main/cpp/ShikiHighlighter.cpp
)

target_include_directories(
  shikibridge
  PUBLIC
  ${CPP_ROOT}
  ${CPP_ROOT}/highlighter
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/include
  ${THIRDPARTY_ROOT}/rapidjson/include
  ${THIRDPARTY_ROOT}/xxHash
  ${NODE_MODULES_DIR}/react-native/ReactCommon
  ${NODE_MODULES_DIR}/react-native/ReactCommon/jsi
  ${REACT_ANDROID_DIR}/src/main/jni/react/turbomodule
)

# Link libraries
target_link_libraries(
  ${CMAKE_PROJECT_NAME}
  xxhash
  shikibridge
)

# Also link directly to shikibridge library
target_link_libraries(
  shikibridge
  xxhash
  android
  log
  jsi
  fbjni
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/libonig.so
)

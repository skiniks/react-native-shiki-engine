cmake_minimum_required(VERSION 3.13)
project(ReactNativeShikiEngine)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 17)

find_library(ONIG_LIB onig
    PATHS ${CMAKE_CURRENT_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}
    NO_CMAKE_FIND_ROOT_PATH
    REQUIRED
)

add_library(react-native-shiki-engine SHARED
    src/main/cpp/cpp-adapter.cpp
    ../cpp/NativeShikiEngineModule.cpp
    ../cpp/onig_regex.cpp
)

target_include_directories(react-native-shiki-engine PUBLIC src/main/cpp/include)
target_include_directories(react-native-shiki-engine PUBLIC ../cpp)

find_library(log-lib log)

target_link_libraries(react-native-shiki-engine
    ${ONIG_LIB}
    android
    ${log-lib}
    jsi
    fbjni
    react_codegen_NativeShikiEngineSpec
)

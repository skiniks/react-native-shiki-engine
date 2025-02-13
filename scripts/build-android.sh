#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"
ONIGURUMA_DIR="$PROJECT_DIR/thirdparty/oniguruma"
BUILD_DIR="$PROJECT_DIR/build-onig-android"
ANDROID_DIR="$PROJECT_DIR/android"

# Get Oniguruma version
if [ -f "$ONIGURUMA_DIR/src/oniguruma.h" ]; then
    MAJOR=$(grep "ONIGURUMA_VERSION_MAJOR" "$ONIGURUMA_DIR/src/oniguruma.h" | awk '{print $3}')
    MINOR=$(grep "ONIGURUMA_VERSION_MINOR" "$ONIGURUMA_DIR/src/oniguruma.h" | awk '{print $3}')
    TEENY=$(grep "ONIGURUMA_VERSION_TEENY" "$ONIGURUMA_DIR/src/oniguruma.h" | awk '{print $3}')
    ONIG_VERSION="$MAJOR.$MINOR.$TEENY"
    echo "Building Oniguruma version $ONIG_VERSION"
else
    echo "Error: Could not find oniguruma.h"
    exit 1
fi

# Android architectures to build for
ARCHS="arm64-v8a armeabi-v7a x86 x86_64"
MIN_SDK=21

# Make sure ANDROID_NDK_HOME is set
if [ -z "$ANDROID_NDK_HOME" ]; then
    echo "Error: ANDROID_NDK_HOME environment variable is not set"
    exit 1
fi

# Create the license bundle
create_license_bundle() {
    echo "Creating license bundle..."
    local LICENSE_DIR="$BUILD_DIR/license"
    mkdir -p "$LICENSE_DIR"

    if [ -f "$ONIGURUMA_DIR/COPYING" ]; then
        cp "$ONIGURUMA_DIR/COPYING" "$LICENSE_DIR/LICENSE"
    else
        echo "Warning: Could not find Oniguruma license file"
        echo "Please ensure the license file exists at $ONIGURUMA_DIR/COPYING"
        exit 1
    fi
}

build_arch() {
    local arch=$1
    local build_dir="$BUILD_DIR/$arch"
    local host_os
    case "$(uname -s)" in
        Darwin*)  host_os="darwin-x86_64" ;;
        Linux*)   host_os="linux-x86_64" ;;
        MINGW*)   host_os="windows-x86_64" ;;
        *)        echo "Unknown operating system"; exit 1 ;;
    esac
    local toolchain="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/$host_os"

    echo "Building for $arch..."

    # Set up architecture-specific variables
    case $arch in
        "arm64-v8a")
            TARGET=aarch64-linux-android
            ;;
        "armeabi-v7a")
            TARGET=armv7a-linux-androideabi
            ;;
        "x86")
            TARGET=i686-linux-android
            ;;
        "x86_64")
            TARGET=x86_64-linux-android
            ;;
        *)
            echo "Unknown architecture: $arch"
            exit 1
            ;;
    esac

    mkdir -p "$build_dir"
    cd "$ONIGURUMA_DIR" || exit 1

    make distclean >/dev/null 2>&1 || true

    if [ ! -f "configure" ]; then
        ./autogen.sh || exit 1
    fi

    # Set up the build environment
    export CC="$toolchain/bin/$TARGET$MIN_SDK-clang"
    export CXX="$toolchain/bin/$TARGET$MIN_SDK-clang++"
    export AR="$toolchain/bin/llvm-ar"
    export RANLIB="$toolchain/bin/llvm-ranlib"
    export STRIP="$toolchain/bin/llvm-strip"

    # Configure flags for Android
    export CFLAGS="-fPIC -O2 -DANDROID -D__ANDROID_API__=$MIN_SDK -DONIGURUMA_VERSION=\"$ONIG_VERSION\""
    export LDFLAGS="-fPIC"

    ./configure --host="$TARGET" \
                --prefix="$build_dir" \
                --enable-shared \
                --disable-static \
                --disable-dependency-tracking || exit 1

    make clean
    if command -v nproc >/dev/null 2>&1; then
        make -j$(nproc)
    else
        make -j$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
    fi
    make install || exit 1

    # Copy the shared library to the jniLibs directory
    mkdir -p "$ANDROID_DIR/src/main/jniLibs/$arch"
    cp "$build_dir/lib/libonig.so" "$ANDROID_DIR/src/main/jniLibs/$arch/"

    # Copy headers to the include directory
    mkdir -p "$ANDROID_DIR/src/main/cpp/include"
    cp -R "$build_dir/include/"* "$ANDROID_DIR/src/main/cpp/include/"

    cd - > /dev/null
}

# Create build metadata
create_build_metadata() {
    # Create resources directory first
    mkdir -p "$ANDROID_DIR/src/main/resources"

    # Create version properties file
    cat > "$ANDROID_DIR/src/main/resources/oniguruma-version.properties" << EOF
version=$ONIG_VERSION
buildDate=$(date -u +"%Y-%m-%d")
minSdkVersion=$MIN_SDK
EOF

    # Copy license to resources
    cp "$BUILD_DIR/license/LICENSE" "$ANDROID_DIR/src/main/resources/"
}

echo "Building Oniguruma for Android..."

# Clean previous builds
rm -rf "$BUILD_DIR"
rm -rf "$ANDROID_DIR/src/main/jniLibs"
rm -rf "$ANDROID_DIR/src/main/cpp/include"
rm -rf "$ANDROID_DIR/src/main/resources"

# Create build directory
mkdir -p "$BUILD_DIR"

# Create license bundle first
create_license_bundle

# Build for each architecture
for arch in $ARCHS; do
    build_arch "$arch"
done

# Create build metadata and copy resources
create_build_metadata

echo "Build completed successfully!"
echo "Libraries created at: $ANDROID_DIR/src/main/jniLibs/"

# Verify the builds
echo "Verifying builds..."
build_success=true
for arch in $ARCHS; do
    echo "Checking $arch:"
    if [ -f "$ANDROID_DIR/src/main/jniLibs/$arch/libonig.so" ]; then
        ls -l "$ANDROID_DIR/src/main/jniLibs/$arch/libonig.so"
        file "$ANDROID_DIR/src/main/jniLibs/$arch/libonig.so"
    else
        echo "ERROR: Library for $arch not found!"
        build_success=false
    fi
done

if [ "$build_success" = true ]; then
    # Clean up build directories after successful completion
    rm -rf "$BUILD_DIR"
    echo "Build directory cleaned up"
fi

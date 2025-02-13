#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"
ONIGURUMA_DIR="$PROJECT_DIR/thirdparty/oniguruma"
BUILD_DIR="$PROJECT_DIR/build-onig"
IOS_DIR="$PROJECT_DIR/ios"

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

SIMULATOR_ARCHS="x86_64 arm64"  # arm64 for M1/M2 simulator
DEVICE_ARCHS="arm64"
MIN_IOS_VERSION="11.0"

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
    local platform=$2
    local build_dir="$BUILD_DIR/${platform}-${arch}"

    echo "Building for $arch ($platform)..."

    mkdir -p "$build_dir"
    cd "$ONIGURUMA_DIR" || exit 1

    make distclean >/dev/null 2>&1 || true

    if [ ! -f "configure" ]; then
        ./autogen.sh || exit 1
    fi

    if [ "$platform" == "simulator" ]; then
        export SDK="iphonesimulator"
        export PLATFORM="iPhoneSimulator"
    else
        export SDK="iphoneos"
        export PLATFORM="iPhoneOS"
    fi

    export SDKROOT="$(xcrun --sdk $SDK --show-sdk-path)"
    export DEVELOPER_DIR="$(xcode-select -p)"
    export CC="$(xcrun -find -sdk $SDK clang)"
    export CXX="$(xcrun -find -sdk $SDK clang++)"

    # Add flags for deterministic builds
    export ZERO_AR_DATE=1
    export ZERO_TIMESTAMP=1

    if [ "$platform" == "simulator" ]; then
        export CFLAGS="-arch $arch -isysroot $SDKROOT -mios-simulator-version-min=$MIN_IOS_VERSION -I$SDKROOT/usr/include -fembed-bitcode -DONIGURUMA_VERSION=\"$ONIG_VERSION\""
        export LDFLAGS="-arch $arch -isysroot $SDKROOT"
    else
        export CFLAGS="-arch $arch -isysroot $SDKROOT -miphoneos-version-min=$MIN_IOS_VERSION -I$SDKROOT/usr/include -fembed-bitcode -DONIGURUMA_VERSION=\"$ONIG_VERSION\""
        export LDFLAGS="-arch $arch -isysroot $SDKROOT"
    fi

    ./configure --host="$arch-apple-darwin" \
                --prefix="$build_dir" \
                --enable-static \
                --disable-shared \
                --disable-dependency-tracking || exit 1

    make clean
    make -j$(sysctl -n hw.ncpu) || exit 1
    make install || exit 1

    cd - > /dev/null
}

create_fat_libraries() {
    echo "Creating fat libraries..."

    mkdir -p "$IOS_DIR/lib"

    local simulator_libs=()
    for arch in $SIMULATOR_ARCHS; do
        simulator_libs+=("$BUILD_DIR/simulator-$arch/lib/libonig.a")
    done
    if [ ${#simulator_libs[@]} -gt 1 ]; then
        echo "Creating simulator fat library..."
        xcrun lipo -create "${simulator_libs[@]}" -output "$BUILD_DIR/simulator/libonig.a"
    else
        cp "${simulator_libs[0]}" "$BUILD_DIR/simulator/libonig.a"
    fi

    mkdir -p "$BUILD_DIR/device"
    cp "$BUILD_DIR/device-arm64/lib/libonig.a" "$BUILD_DIR/device/libonig.a"

    mkdir -p "$IOS_DIR/lib/simulator" "$IOS_DIR/lib/device"
    cp "$BUILD_DIR/simulator/libonig.a" "$IOS_DIR/lib/simulator/"
    cp "$BUILD_DIR/device/libonig.a" "$IOS_DIR/lib/device/"
}

create_xcframework() {
    echo "Creating XCFramework..."
    local FRAMEWORK_NAME="Oniguruma"

    rm -rf "$IOS_DIR/$FRAMEWORK_NAME.xcframework"

    # Create Info.plist with version metadata
    cat > "$BUILD_DIR/Info.plist" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>OnigurumaVersion</key>
    <string>$ONIG_VERSION</string>
    <key>BuildDate</key>
    <string>$(date -u +"%Y-%m-%d")</string>
</dict>
</plist>
EOF

    # Create a Resources bundle with the license
    local RESOURCES_DIR="$BUILD_DIR/resources"
    mkdir -p "$RESOURCES_DIR"
    cp "$BUILD_DIR/license/LICENSE" "$RESOURCES_DIR/"
    cp "$BUILD_DIR/Info.plist" "$RESOURCES_DIR/"

    xcrun xcodebuild -create-xcframework \
        -library "$IOS_DIR/lib/simulator/libonig.a" \
        -headers "$BUILD_DIR/simulator-x86_64/include" \
        -library "$IOS_DIR/lib/device/libonig.a" \
        -headers "$BUILD_DIR/simulator-x86_64/include" \
        -output "$IOS_DIR/$FRAMEWORK_NAME.xcframework"

    # Copy the license and Info.plist into the XCFramework
    for platform in ios-arm64 ios-arm64_x86_64-simulator; do
        mkdir -p "$IOS_DIR/$FRAMEWORK_NAME.xcframework/$platform/Resources"
        cp "$BUILD_DIR/license/LICENSE" "$IOS_DIR/$FRAMEWORK_NAME.xcframework/$platform/Resources/"
        cp "$BUILD_DIR/Info.plist" "$IOS_DIR/$FRAMEWORK_NAME.xcframework/$platform/Resources/"
    done

    echo "XCFramework created at: $IOS_DIR/$FRAMEWORK_NAME.xcframework"
}

echo "Building Oniguruma for iOS..."

rm -rf "$BUILD_DIR"
rm -rf "$IOS_DIR/lib"

mkdir -p "$BUILD_DIR/simulator" "$BUILD_DIR/device"

# Create license bundle first
create_license_bundle

for arch in $SIMULATOR_ARCHS; do
    build_arch "$arch" "simulator"
done

for arch in $DEVICE_ARCHS; do
    build_arch "$arch" "device"
done

create_fat_libraries
create_xcframework

echo "Build completed successfully!"
echo "Libraries created at:"
echo "  Simulator: $IOS_DIR/lib/simulator/libonig.a"
echo "  Device: $IOS_DIR/lib/device/libonig.a"

echo "Verifying architectures..."
echo "Simulator library:"
xcrun lipo -info "$IOS_DIR/lib/simulator/libonig.a"
echo "Device library:"
xcrun lipo -info "$IOS_DIR/lib/device/libonig.a"

echo "Verifying XCFramework..."
if [ -d "$IOS_DIR/Oniguruma.xcframework" ]; then
    echo "XCFramework structure:"
    ls -R "$IOS_DIR/Oniguruma.xcframework"

    # Clean up build directory after successful completion
    echo "Cleaning up build directory..."
    rm -rf "$BUILD_DIR"
    rm -rf "$IOS_DIR/lib"
else
    echo "XCFramework was not created successfully"
fi

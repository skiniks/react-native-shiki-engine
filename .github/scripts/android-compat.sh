#!/usr/bin/env bash
set -euo pipefail

RN_VERSION="${1:?Usage: android-compat.sh <react-native-version>}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

source "$SCRIPT_DIR/bootstrap-rn-compat.sh"
bootstrap_rn_compat_app "$RN_VERSION"

echo "Building react-native-shiki-engine on Android for React Native ${RN_VERSION}"

cd "$RN_SHIKI_COMPAT_ANDROID_DIR"
./gradlew :react-native-shiki-engine:assembleDebug --no-daemon --stacktrace

echo "Successfully built react-native-shiki-engine on Android with React Native ${RN_VERSION}"

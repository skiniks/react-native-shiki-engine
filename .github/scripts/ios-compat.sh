#!/usr/bin/env bash
set -euo pipefail

RN_VERSION="${1:?Usage: ios-compat.sh <react-native-version>}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

source "$SCRIPT_DIR/bootstrap-rn-compat.sh"
bootstrap_rn_compat_app "$RN_VERSION"

echo "Building react-native-shiki-engine on iOS for React Native ${RN_VERSION}"

export RCT_NEW_ARCH_ENABLED=1
export NO_FLIPPER=1
export LANG=en_US.UTF-8
export LC_ALL=en_US.UTF-8

# Xcode 26+ / Apple Clang rejects fmt 11.x consteval (RN 0.81 and earlier).
# Force those pods to C++17 + FMT_USE_CONSTEVAL=0.
patch_podfile_fmt_xcode26() {
  local podfile="$1"
  python3 - "$podfile" <<'PY'
from pathlib import Path
import re
import sys

path = Path(sys.argv[1])
text = path.read_text()
if "FMT_USE_CONSTEVAL=0" in text:
    print(f"Podfile already patched for fmt/Xcode 26: {path}")
    sys.exit(0)

snippet = """
    # Workaround: Xcode 26 / Apple Clang rejects fmt 11.x consteval
    installer.pods_project.targets.each do |target|
      next unless ['fmt', 'RCT-Folly'].include?(target.name)
      target.build_configurations.each do |cfg|
        cfg.build_settings['CLANG_CXX_LANGUAGE_STANDARD'] = 'c++17'
        defs = cfg.build_settings['GCC_PREPROCESSOR_DEFINITIONS'] || ['$(inherited)']
        defs = [defs] unless defs.is_a?(Array)
        defs << 'FMT_USE_CONSTEVAL=0' unless defs.include?('FMT_USE_CONSTEVAL=0')
        cfg.build_settings['GCC_PREPROCESSOR_DEFINITIONS'] = defs
      end
    end
"""

match = re.search(r"react_native_post_install\(\s*[\s\S]*?\)\n", text)
if match is None:
    raise SystemExit(f"Could not find react_native_post_install(...) in {path}")

text = text[: match.end()] + snippet + text[match.end() :]
path.write_text(text)
print(f"Patched {path} for fmt/Xcode 26 workaround")
PY
}

cd "$RN_SHIKI_COMPAT_APP_DIR"
bundle install
cd "$RN_SHIKI_COMPAT_IOS_DIR"
patch_podfile_fmt_xcode26 "$RN_SHIKI_COMPAT_IOS_DIR/Podfile"
bundle exec pod install

xcodebuild \
  -workspace "${RN_SHIKI_COMPAT_SCHEME}.xcworkspace" \
  -scheme "$RN_SHIKI_COMPAT_SCHEME" \
  -sdk iphonesimulator \
  -configuration Debug \
  -destination 'generic/platform=iOS Simulator' \
  SKIP_BUNDLING=1 \
  build

echo "Successfully built react-native-shiki-engine on iOS with React Native ${RN_VERSION}"

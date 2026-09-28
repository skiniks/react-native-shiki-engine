#!/usr/bin/env bash
set -euo pipefail

_BOOTSTRAP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

bootstrap_rn_compat_app() {
  local rn_version="$1"
  local project_dir package_dir app_dir

  project_dir="$(cd "$_BOOTSTRAP_DIR/../.." && pwd)"
  package_dir="$project_dir/packages/react-native-shiki-engine"
  app_dir="${RUNNER_TEMP:-${TMPDIR:-/tmp}}/rn-shiki-compat-${rn_version//./-}"

  echo "Bootstrapping React Native ${rn_version} app at ${app_dir}"

  rm -rf "$app_dir"

  npx --yes @react-native-community/cli init ShikiCompat \
    --directory "$app_dir" \
    --version "$rn_version" \
    --pm npm \
    --skip-git-init \
    --install-pods false \
    --replace-directory true

  if grep -q '^newArchEnabled=' "$app_dir/android/gradle.properties"; then
    sed -i.bak 's/^newArchEnabled=.*/newArchEnabled=true/' "$app_dir/android/gradle.properties"
    rm -f "$app_dir/android/gradle.properties.bak"
  else
    echo 'newArchEnabled=true' >> "$app_dir/android/gradle.properties"
  fi

  cd "$app_dir"
  npm install "$package_dir"

  export RN_SHIKI_COMPAT_VERSION="$rn_version"
  export RN_SHIKI_COMPAT_APP_DIR="$app_dir"
  export RN_SHIKI_COMPAT_IOS_DIR="$app_dir/ios"
  export RN_SHIKI_COMPAT_ANDROID_DIR="$app_dir/android"
  export RN_SHIKI_COMPAT_SCHEME="ShikiCompat"
}

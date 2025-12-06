#!/bin/bash

set -e

if ! which clang-format >/dev/null; then
  echo "error: clang-format not installed"
  echo "Install with: brew install clang-format"
  exit 1
fi

find packages/react-native-shiki-engine/android packages/react-native-shiki-engine/cpp packages/react-native-shiki-engine/apple \
  \! -path '*/Oniguruma.xcframework/*' \
  \! -path '*/android/.cxx/*' \
  \! -path '*/android/src/main/cpp/include/*' \
  \! -path '*/generated/*' \
  \! -path '*/build/*' \
  -type f \( -name "*.h" -o -name "*.hpp" -o -name "*.cpp" -o -name "*.m" -o -name "*.mm" \) \
  -print0 | while read -d $'\0' file; do
  echo "-> Formatting $file"
  clang-format -i -style=file "$file"
done

echo "Verifying formatting..."
find packages/react-native-shiki-engine/android packages/react-native-shiki-engine/cpp packages/react-native-shiki-engine/apple \
  \! -path '*/Oniguruma.xcframework/*' \
  \! -path '*/android/.cxx/*' \
  \! -path '*/android/src/main/cpp/include/*' \
  \! -path '*/generated/*' \
  \! -path '*/build/*' \
  -type f \( -name "*.h" -o -name "*.hpp" -o -name "*.cpp" -o -name "*.m" -o -name "*.mm" \) \
  -print0 | while read -d $'\0' file; do
  if ! clang-format -style=file --dry-run -Werror "$file"; then
    echo "error: $file is not properly formatted"
    exit 1
  fi
done

echo "All files formatted successfully!"

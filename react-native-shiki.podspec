require "json"

package = JSON.parse(File.read(File.join(__dir__, "package.json")))

Pod::Spec.new do |s|
  s.name         = "react-native-shiki"
  s.version      = package["version"]
  s.summary      = package["description"]
  s.homepage     = package["homepage"]
  s.license      = package["license"]
  s.authors      = package["author"]
  s.platforms    = { :ios => min_ios_version_supported }
  s.source       = { :git => "https://github.com/skiniks/react-native-shiki.git", :tag => "#{s.version}" }

  s.vendored_frameworks = "apple/Oniguruma.xcframework"

  s.source_files = [
    "apple/**/*.{h,mm}",
    "cpp/**/*.{cpp,h,mm,hpp}",
    "thirdparty/xxHash/xxhash.{c,h}",
    "thirdparty/shiki-textmate/src/**/*.{cpp,cc,h,hpp}"
  ]

  s.pod_target_xcconfig = {
    'HEADER_SEARCH_PATHS' => [
      '$(PODS_TARGET_SRCROOT)/cpp',
      '$(PODS_TARGET_SRCROOT)/thirdparty/rapidjson/include',
      '$(PODS_TARGET_SRCROOT)/thirdparty/xxHash',
      '$(PODS_TARGET_SRCROOT)/thirdparty/shiki-textmate/include',
      '$(PODS_TARGET_SRCROOT)/apple/Oniguruma.xcframework/ios-arm64/Headers',
      '$(PODS_ROOT)/Headers/Public/react-native-shiki',
      '$(PODS_ROOT)/Headers/Public/ReactCodegen',
    ].join(' '),
    'FRAMEWORK_SEARCH_PATHS' => [
      '$(PODS_TARGET_SRCROOT)/apple'
    ].join(' '),
    'OTHER_LDFLAGS' => '-framework Oniguruma'
  }

  s.exclude_files = [
    "cpp/highlighter/platform/android/**/*",
    "thirdparty/shiki-textmate/tests/**/*",
    "thirdparty/shiki-textmate/third_party/**/*"
  ]

  install_modules_dependencies(s)
end

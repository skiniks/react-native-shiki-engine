require "json"

package = JSON.parse(File.read(File.join(__dir__, "package.json")))

Pod::Spec.new do |s|
  s.name         = "react-native-shiki-engine"
  s.version      = package["version"]
  s.summary      = package["description"]
  s.homepage     = package["homepage"]
  s.license      = package["license"]
  s.authors      = package["author"]
  s.platforms    = { :ios => min_ios_version_supported }
  s.source       = { :git => "https://github.com/skiniks/react-native-shiki-engine.git", :tag => "#{s.version}" }

  s.vendored_frameworks = "ios/Oniguruma.xcframework"

  s.source_files = [
    "cpp/**/*.{cpp,h,mm,hpp}",
    "ios/**/*.{h,mm}",
    "cpp/highlighter/platform/ios/**/*.{h,mm}",
    "cpp/highlighter/assets/ios/**/*.{h,mm}"
  ]

  s.exclude_files = [
    "cpp/highlighter/platform/android/**/*",
    "**/android/**/*",
    "ios/onLoad.mm",
    "cpp/NativeShikiEngineModule.cpp"
  ]

  s.pod_target_xcconfig = {
    'HEADER_SEARCH_PATHS' => [
      '$(PODS_TARGET_SRCROOT)/thirdparty/rapidjson/include',
      '$(PODS_TARGET_SRCROOT)/cpp/fabric',
      '$(PODS_TARGET_SRCROOT)/cpp',
      '$(PODS_TARGET_SRCROOT)/cpp/highlighter',
      '$(PODS_TARGET_SRCROOT)/cpp/highlighter/tokenizer',
      '$(PODS_ROOT)/Headers/Public/react-native-shiki-engine',
      '$(PODS_ROOT)/Headers/Public/ReactCodegen',
      '$(PODS_TARGET_SRCROOT)/ios/Oniguruma.xcframework/ios-arm64/Headers'
    ].join(' '),
    'FRAMEWORK_SEARCH_PATHS' => [
      '$(PODS_TARGET_SRCROOT)/ios'
    ].join(' ')
  }

  install_modules_dependencies(s)
end

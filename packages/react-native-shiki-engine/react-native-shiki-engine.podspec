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

  s.vendored_frameworks = "apple/Oniguruma.xcframework"

  s.source_files = [
    "cpp/**/*.{cpp,h}",
    "apple/**/*.{h,mm}",
  ]

  if ENV['RCT_NEW_ARCH_ENABLED'] == '1'
    s.pod_target_xcconfig = {
      "HEADER_SEARCH_PATHS" => [
        # RN 0.74+
        "\"$(PODS_ROOT)/Headers/Private/ReactCodegen/react/renderer/components/NativeShikiEngineSpec\"",
        "\"$(PODS_ROOT)/Headers/Public/ReactCodegen/react/renderer/components/NativeShikiEngineSpec\"",
        # RN 0.71-0.73
        "\"$(PODS_ROOT)/Headers/Private/React-Codegen/react/renderer/components/NativeShikiEngineSpec\"",
        "\"$(PODS_ROOT)/Headers/Public/React-Codegen/react/renderer/components/NativeShikiEngineSpec\"",
        # Fallback to root codegen headers
        "\"$(PODS_ROOT)/Headers/Private/React-Codegen\"",
        "\"$(PODS_ROOT)/Headers/Public/React-Codegen\"",
        "\"$(PODS_ROOT)/Headers/Private/ReactCodegen\"",
        "\"$(PODS_ROOT)/Headers/Public/ReactCodegen\""
      ].join(" ")
    }
  end

  install_modules_dependencies(s)
end

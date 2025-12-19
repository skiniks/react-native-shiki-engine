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

  react_native_path = nil
  [
    File.join(Pod::Config.instance.installation_root, "..", "node_modules", "react-native"),
    File.join(__dir__, "..", "..", "node_modules", "react-native"),
    File.join(__dir__, "node_modules", "react-native")
  ].each do |path|
    if File.exist?(File.join(path, "package.json"))
      react_native_path = path
      break
    end
  end

  react_native_package = nil
  if react_native_path
    begin
      react_native_package = JSON.parse(File.read(File.join(react_native_path, "package.json")))
    rescue
      react_native_package = nil
    end
  end
  react_native_version = react_native_package ? react_native_package["version"] : nil

  version_parts = react_native_version ? react_native_version.split('.').map(&:to_i) : [0, 0, 0]
  is_rn_73_or_higher = version_parts[0] > 0 || (version_parts[0] == 0 && version_parts[1] >= 73)

  if is_rn_73_or_higher
    s.source_files = [
      "cpp/**/*.{cpp,h}",
      "apple/**/*.{h,mm}",
    ]
  else
    s.source_files = [
      "cpp/**/*.{cpp,h}",
      "apple/**/*.{h,mm}",
    ]
    s.exclude_files = "apple/onLoad.mm"
  end

  if ENV['RCT_NEW_ARCH_ENABLED'] == '1'
    s.pod_target_xcconfig = {
      "HEADER_SEARCH_PATHS" => [
        "\"$(PODS_ROOT)/Headers/Private/ReactCodegen/react/renderer/components/NativeShikiEngineSpec\"",
        "\"$(PODS_ROOT)/Headers/Public/ReactCodegen/react/renderer/components/NativeShikiEngineSpec\"",
        "\"$(PODS_ROOT)/Headers/Private/React-Codegen/react/renderer/components/NativeShikiEngineSpec\"",
        "\"$(PODS_ROOT)/Headers/Public/React-Codegen/react/renderer/components/NativeShikiEngineSpec\"",
        "\"$(PODS_ROOT)/../build/shiki-codegen/build/generated/ios\""
      ].join(" "),
      "OTHER_CPLUSPLUSFLAGS" => "$(inherited) -DRCT_NEW_ARCH_ENABLED=1"
    }

    s.dependency "React-Codegen"
    s.dependency "ReactCommon/turbomodule/core"
  end

  install_modules_dependencies(s)
end

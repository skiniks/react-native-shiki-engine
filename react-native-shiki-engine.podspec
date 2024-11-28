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
    "cpp/**/*.{cpp,h}",
    "ios/**/*.{h,mm}",
  ]

  install_modules_dependencies(s)
end

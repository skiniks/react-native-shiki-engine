#include "IOSAssetLoader.h"
#import <Foundation/Foundation.h>
#include <rapidjson/error/en.h>

namespace shiki {

IOSAssetLoader& IOSAssetLoader::getInstance() {
  static IOSAssetLoader instance;
  return instance;
}

rapidjson::Document IOSAssetLoader::loadAsset(const std::string& name) {
  NSString* path = findAssetPath(name);
  if (!path) {
    throw std::runtime_error("Asset not found: " + name);
  }

  NSData* data = [NSData dataWithContentsOfFile:path];
  if (!data) {
    throw std::runtime_error("Failed to read asset: " + name);
  }

  std::string jsonStr = std::string(static_cast<const char*>([data bytes]), [data length]);

  rapidjson::Document document;
  rapidjson::ParseResult ok = document.Parse(jsonStr.c_str());

  if (!ok) {
    throw std::runtime_error(
        "JSON parse error: " + std::string(rapidjson::GetParseError_En(ok.Code())) + " at offset " +
        std::to_string(ok.Offset()));
  }

  return document;
}

NSString* IOSAssetLoader::findAssetPath(const std::string& name) {
  NSString* nsName = @(name.c_str());

  // Look in main bundle first
  NSString* path = [[NSBundle mainBundle] pathForResource:nsName ofType:@"json"];
  if (path)
    return path;

  // Then check framework bundle
  NSBundle* frameworkBundle =
      [NSBundle bundleForClass:NSClassFromString(@"RNShikiHighlighterView")];
  return [frameworkBundle pathForResource:nsName ofType:@"json"];
}

} // namespace shiki

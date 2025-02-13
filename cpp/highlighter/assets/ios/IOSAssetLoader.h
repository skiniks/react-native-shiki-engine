#pragma once
#include "../AssetLoader.h"

namespace shiki {

class IOSAssetLoader : public AssetLoader {
public:
  static IOSAssetLoader& getInstance();
  rapidjson::Document loadAsset(const std::string& name) override;

private:
  IOSAssetLoader() = default;
  NSString* findAssetPath(const std::string& name);
};

} // namespace shiki

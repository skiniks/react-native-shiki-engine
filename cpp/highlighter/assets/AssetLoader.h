#pragma once
#include <memory>
#include <rapidjson/document.h>
#include <string>

namespace shiki {

class AssetLoader {
public:
  virtual ~AssetLoader() = default;
  virtual rapidjson::Document loadAsset(const std::string& name) = 0;

protected:
  AssetLoader() = default;
};

} // namespace shiki

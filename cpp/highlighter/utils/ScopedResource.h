#pragma once
#include <functional>

namespace shiki {

template <typename T>
class ScopedResource {
 public:
  explicit ScopedResource(T* resource, std::function<void(T*)> deleter)
    : resource_(resource), deleter_(std::move(deleter)) {}

  ~ScopedResource() {
    if (resource_ && deleter_) {
      deleter_(resource_);
    }
  }

  T* get() {
    return resource_;
  }
  T* release() {
    T* temp = resource_;
    resource_ = nullptr;
    return temp;
  }

  // Prevent copying
  ScopedResource(const ScopedResource&) = delete;
  ScopedResource& operator=(const ScopedResource&) = delete;

  // Allow moving
  ScopedResource(ScopedResource&& other) noexcept : resource_(other.release()), deleter_(std::move(other.deleter_)) {}

 private:
  T* resource_;
  std::function<void(T*)> deleter_;
};

}  // namespace shiki

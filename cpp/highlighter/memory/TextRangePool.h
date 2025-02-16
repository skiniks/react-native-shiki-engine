#pragma once
#include <memory>

#include "../text/TextRange.h"
#include "MemoryPool.h"

namespace shiki {

class TextRangePool {
 public:
  static TextRangePool& getInstance() {
    static TextRangePool instance;
    return instance;
  }

  template <typename... Args>
  TextRange* allocate(Args&&... args) {
    return pool_.allocate(std::forward<Args>(args)...);
  }

  void deallocate(TextRange* ptr) {
    pool_.deallocate(ptr);
  }

  size_t getActiveAllocations() const {
    return pool_.getActiveAllocations();
  }

  size_t getTotalCapacity() const {
    return pool_.getTotalCapacity();
  }

 private:
  TextRangePool() = default;
  ~TextRangePool() = default;

  TextRangePool(const TextRangePool&) = delete;
  TextRangePool& operator=(const TextRangePool&) = delete;

  static constexpr size_t BLOCK_SIZE = 2048;
  MemoryPool<TextRange, BLOCK_SIZE> pool_;
};

}  // namespace shiki

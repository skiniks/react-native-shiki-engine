#pragma once
#include "MemoryPool.h"
#include "highlighter/tokenizer/Token.h"

namespace shiki {

class TokenPool {
 public:
  static TokenPool& getInstance() {
    static TokenPool instance;
    return instance;
  }

  Token* allocate() {
    return pool_.allocate();
  }

  void deallocate(Token* ptr) {
    pool_.deallocate(ptr);
  }

  size_t getActiveAllocations() const {
    return pool_.getActiveAllocations();
  }

  size_t getTotalCapacity() const {
    return pool_.getTotalCapacity();
  }

 private:
  TokenPool() = default;
  ~TokenPool() = default;

  TokenPool(const TokenPool&) = delete;
  TokenPool& operator=(const TokenPool&) = delete;

  static constexpr size_t BLOCK_SIZE = 1024;
  MemoryPool<Token, BLOCK_SIZE> pool_;
};

}  // namespace shiki

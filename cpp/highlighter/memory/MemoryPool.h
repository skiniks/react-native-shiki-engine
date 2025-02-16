#pragma once
#include <array>
#include <cassert>
#include <memory>
#include <mutex>
#include <new>
#include <unordered_set>
#include <vector>

namespace shiki {

template <typename T, size_t BlockSize = 4096>
class MemoryPool {
 public:
  MemoryPool() {
    allocateNewBlock();
  }

  ~MemoryPool() {
    // Ensure all allocated objects are destroyed
    for (auto& block : blocks_) {
      T* typedBlock = reinterpret_cast<T*>(block.get());
      for (size_t i = 0; i < BlockSize; i++) {
        T* obj = &typedBlock[i];
        if (isAllocated(obj)) {
          obj->~T();
        }
      }
    }
  }

  MemoryPool(const MemoryPool&) = delete;
  MemoryPool& operator=(const MemoryPool&) = delete;

  template <typename... Args>
  T* allocate(Args&&... args) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (freeList_.empty()) {
      allocateNewBlock();
    }

    T* ptr = freeList_.back();
    freeList_.pop_back();

    new (ptr) T(std::forward<Args>(args)...);

    allocatedObjects_.insert(ptr);
    activeAllocations_++;
    return ptr;
  }

  void deallocate(T* ptr) {
    if (!ptr) return;

    std::lock_guard<std::mutex> lock(mutex_);

    // Verify the pointer belongs to our pool
    bool found = false;
    for (auto& block : blocks_) {
      T* typedBlock = reinterpret_cast<T*>(block.get());
      if (ptr >= typedBlock && ptr < (typedBlock + BlockSize)) {
        found = true;
        break;
      }
    }

    assert(found && "Attempted to deallocate pointer not from this pool");
    assert(isAllocated(ptr) && "Double free or invalid pointer");

    // Destroy the object
    ptr->~T();

    allocatedObjects_.erase(ptr);
    freeList_.push_back(ptr);
    activeAllocations_--;
  }

  size_t getActiveAllocations() const {
    return activeAllocations_;
  }

  size_t getTotalCapacity() const {
    return blocks_.size() * BlockSize;
  }

 private:
  void allocateNewBlock() {
    static_assert(alignof(AlignedStorage) >= alignof(T), "Insufficient alignment");
    static_assert(sizeof(AlignedStorage) >= sizeof(T), "Insufficient storage");

    auto newBlock = std::make_unique<AlignedStorage[]>(BlockSize);
    T* typedBlock = reinterpret_cast<T*>(newBlock.get());

    for (size_t i = 0; i < BlockSize; i++) {
      freeList_.push_back(&typedBlock[i]);
    }

    blocks_.push_back(std::move(newBlock));
  }

  bool isAllocated(T* ptr) const {
    return allocatedObjects_.find(ptr) != allocatedObjects_.end();
  }

  struct alignas(alignof(T)) AlignedStorage {
    std::byte storage[sizeof(T)];
  };

  std::vector<std::unique_ptr<AlignedStorage[]>> blocks_;
  std::vector<T*> freeList_;
  std::unordered_set<T*> allocatedObjects_;
  std::mutex mutex_;
  size_t activeAllocations_{0};
};

}  // namespace shiki

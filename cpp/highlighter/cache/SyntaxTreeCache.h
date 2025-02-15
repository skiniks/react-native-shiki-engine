#pragma once
#include <xxhash.h>

#include <memory>
#include <string>
#include <unordered_map>

#include "../grammar/Grammar.h"
#include "../tokenizer/ShikiTokenizer.h"
#include "Cache.h"
#include "CacheConfig.h"

namespace shiki {

struct SyntaxTreeNode {
  std::string scope;
  size_t start;
  size_t length;
  std::vector<std::shared_ptr<SyntaxTreeNode>> children;
};

class SyntaxTreeCache {
 public:
  static SyntaxTreeCache& getInstance() {
    static SyntaxTreeCache instance;
    return instance;
  }

  void cacheTree(const std::string& text, const std::string& grammarId, std::shared_ptr<SyntaxTreeNode> tree) {
    size_t treeSize = estimateTreeSize(tree);
    uint64_t hash = computeHash(text, grammarId);

    // Determine priority based on text size
    Cache<uint64_t, std::shared_ptr<SyntaxTreeNode>>::Priority priority;
    if (text.length() < 1000) {  // Small files get high priority
      priority = Cache<uint64_t, std::shared_ptr<SyntaxTreeNode>>::Priority::HIGH;
    } else {
      priority = Cache<uint64_t, std::shared_ptr<SyntaxTreeNode>>::Priority::NORMAL;
    }

    cache_.add(hash, tree, treeSize, priority);
  }

  std::shared_ptr<SyntaxTreeNode> getCachedTree(const std::string& text, const std::string& grammarId) {
    uint64_t hash = computeHash(text, grammarId);
    return cache_.get(hash).value_or(nullptr);
  }

  void clearCache() {
    cache_.clear();
  }

  Cache<uint64_t, std::shared_ptr<SyntaxTreeNode>>::CacheMetrics getMetrics() const {
    return cache_.getMetrics();
  }

 private:
  SyntaxTreeCache() : cache_(cache::SyntaxTreeCacheConfig::MEMORY_LIMIT, cache::SyntaxTreeCacheConfig::ENTRY_LIMIT) {}

  uint64_t computeHash(const std::string& text, const std::string& grammarId) const {
    XXH3_state_t* const state = XXH3_createState();
    if (!state) return 0;

    XXH3_64bits_reset(state);
    XXH3_64bits_update(state, text.data(), text.size());
    XXH3_64bits_update(state, grammarId.data(), grammarId.size());
    const uint64_t hash = XXH3_64bits_digest(state);
    XXH3_freeState(state);

    return hash;
  }

  size_t estimateTreeSize(const std::shared_ptr<SyntaxTreeNode>& tree) const {
    if (!tree) return 0;

    // Base size for the node
    size_t size = sizeof(SyntaxTreeNode);

    // Add string capacity
    size += tree->scope.capacity();

    // Add size of children vector
    size += tree->children.capacity() * sizeof(std::shared_ptr<SyntaxTreeNode>);

    // Recursively add sizes of children
    for (const auto& child : tree->children) {
      size += estimateTreeSize(child);
    }

    return size;
  }

  Cache<uint64_t, std::shared_ptr<SyntaxTreeNode>> cache_;
};

}  // namespace shiki

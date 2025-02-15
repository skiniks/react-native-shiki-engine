#pragma once
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

struct SyntaxTreeKey {
  std::string text;
  std::string grammarId;

  bool operator==(const SyntaxTreeKey& other) const {
    return text == other.text && grammarId == other.grammarId;
  }
};

}  // namespace shiki

namespace std {
template <>
struct hash<shiki::SyntaxTreeKey> {
  size_t operator()(const shiki::SyntaxTreeKey& key) const {
    return hash<string>()(key.text) ^ hash<string>()(key.grammarId);
  }
};
}  // namespace std

namespace shiki {

class SyntaxTreeCache {
 public:
  static SyntaxTreeCache& getInstance() {
    static SyntaxTreeCache instance;
    return instance;
  }

  void cacheTree(const std::string& text, const std::string& grammarId, std::shared_ptr<SyntaxTreeNode> tree) {
    SyntaxTreeKey key{text, grammarId};
    size_t treeSize = estimateTreeSize(tree);

    // Determine priority based on text size
    Cache<SyntaxTreeKey, std::shared_ptr<SyntaxTreeNode>>::Priority priority;
    if (text.length() < 1000) {  // Small files get high priority
      priority = Cache<SyntaxTreeKey, std::shared_ptr<SyntaxTreeNode>>::Priority::HIGH;
    } else {
      priority = Cache<SyntaxTreeKey, std::shared_ptr<SyntaxTreeNode>>::Priority::NORMAL;
    }

    cache_.add(key, tree, treeSize, priority);
  }

  std::shared_ptr<SyntaxTreeNode> getCachedTree(const std::string& text, const std::string& grammarId) {
    SyntaxTreeKey key{text, grammarId};
    return cache_.get(key).value_or(nullptr);
  }

  void clearCache() {
    cache_.clear();
  }

  Cache<SyntaxTreeKey, std::shared_ptr<SyntaxTreeNode>>::CacheMetrics getMetrics() const {
    return cache_.getMetrics();
  }

 private:
  SyntaxTreeCache() : cache_(cache::SyntaxTreeCacheConfig::MEMORY_LIMIT, cache::SyntaxTreeCacheConfig::ENTRY_LIMIT) {}

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

  Cache<SyntaxTreeKey, std::shared_ptr<SyntaxTreeNode>> cache_;
};

}  // namespace shiki

#include "ShikiTokenizer.h"

#include <oniguruma.h>
#include <xxhash.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "highlighter/cache/CacheManager.h"
#include "highlighter/cache/StyleCache.h"
#include "highlighter/core/Configuration.h"
#include "highlighter/memory/TokenPool.h"
#include "highlighter/utils/ScopedResource.h"
#include "highlighter/utils/WorkPrioritizer.h"

namespace shiki {

namespace {
  // Helper function to delete OnigRegion without the int parameter
  void onigRegionDeleter(OnigRegion* region) {
    if (region) {
      onig_region_free(region, 1);
    }
  }
}  // anonymous namespace

ShikiTokenizer& ShikiTokenizer::getInstance() {
  static ShikiTokenizer instance;
  return instance;
}

ShikiTokenizer::~ShikiTokenizer() {
  clearCompiledPatterns();
}

void ShikiTokenizer::setGrammar(std::shared_ptr<Grammar> grammar) {
  clearCompiledPatterns();
  grammar_ = std::move(grammar);
  if (grammar_) {
    compilePatterns();
    // Initialize concurrency util with hardware threads
    concurrencyUtil_ = std::make_unique<ConcurrencyUtil>(std::thread::hardware_concurrency());
  }
}

void ShikiTokenizer::setTheme(std::shared_ptr<Theme> theme) {
  theme_ = std::move(theme);

  // Configure line numbers if theme has line number styles
  if (theme_) {
    LineNumbers::Config config;
    config.show = true;  // Can be made configurable
    config.style = theme_->getLineNumberStyle();
    config.fontSize = 12.0f;  // Can be made configurable
    lineNumbers_ = std::make_unique<LineNumbers>(config);
  }
}

void ShikiTokenizer::clearCompiledPatterns() {
  compiledPatterns_.clear();
}

void ShikiTokenizer::compilePatterns() {
  if (!grammar_) return;

  const auto& patterns = grammar_->getPatterns();
  compiledPatterns_.reserve(patterns.size());

  OnigErrorInfo einfo;
  for (const auto& pattern : patterns) {
    CompiledPattern compiled;

    // Skip patterns without a match
    if (pattern.match.empty()) continue;

    // Special handling for comment patterns
    bool isComment = pattern.name.find("comment") != std::string::npos;
    OnigOptionType options = ONIG_OPTION_CAPTURE_GROUP;
    if (isComment) {
      // Add multiline option for comments to handle line endings properly
      options |= ONIG_OPTION_MULTILINE;
    }

    regex_t* regex = nullptr;
    int result = onig_new(
      &regex,
      (OnigUChar*)pattern.match.c_str(),
      (OnigUChar*)(pattern.match.c_str() + pattern.match.length()),
      options,
      ONIG_ENCODING_UTF8,
      ONIG_SYNTAX_DEFAULT,
      &einfo
    );

    if (result == ONIG_NORMAL) {
      compiled.regex.reset(regex);
      compiled.name = pattern.name;
      compiledPatterns_.push_back(std::move(compiled));
    } else {
      char s[ONIG_MAX_ERROR_MESSAGE_LEN];
      onig_error_code_to_str((OnigUChar*)s, result, &einfo);
      std::cerr << "Failed to compile pattern: " << s << std::endl;
    }
  }

  std::cout << "[INFO] Compiled " << compiledPatterns_.size() << " patterns" << std::endl;
}

bool isMoreSpecific(const std::string& scope1, const std::string& scope2) {
  return scope1.find(scope2) != std::string::npos && scope1.length() > scope2.length();
}

std::vector<Token> convertRangesToTokens(const std::vector<TextRange>& ranges, const std::string& code) {
  std::vector<Token> tokens;
  tokens.reserve(ranges.size());

  for (size_t i = 0; i < ranges.size(); i++) {
    const auto& range = ranges[i];
    Token token;
    token.start = range.start;
    token.length = range.length;

    // Cast to TokenRange if possible to get scopes
    if (const TokenRange* tokenRange = dynamic_cast<const TokenRange*>(&range)) {
      for (const auto& scope : tokenRange->scopes) {
        token.addScope(scope);
      }
    } else {
      token.addScope("text");
    }

    // Verify token boundaries
    if (token.start + token.length > code.length()) {
      std::cout << "[ERROR] Token boundary exceeds code length!" << std::endl;
      continue;
    }

    tokens.push_back(token);
  }

  return tokens;
}

std::string ShikiTokenizer::computeTextHash(const std::string& text) const {
  const uint64_t hash = XXH64(text.data(), text.size(), 0);
  return std::to_string(hash);
}

std::optional<std::vector<Token>> ShikiTokenizer::tryGetCachedTokens(const std::string& text) {
  if (text.length() < MIN_CACHE_REGION_SIZE) {
    return std::nullopt;
  }

  std::string hash = computeTextHash(text);
  std::lock_guard<std::mutex> lock(tokenCacheMutex_);

  auto it = tokenCache_.find(hash);
  if (it != tokenCache_.end() && it->second.textHash == hash) {
    it->second.lastUsed = ++cacheTimestamp_;
    return it->second.tokens;
  }

  return std::nullopt;
}

void ShikiTokenizer::cacheTokens(const std::string& text, const std::vector<Token>& tokens) {
  if (text.length() < MIN_CACHE_REGION_SIZE) {
    return;
  }

  std::string hash = computeTextHash(text);
  std::lock_guard<std::mutex> lock(tokenCacheMutex_);

  // Evict if needed
  if (tokenCache_.size() >= MAX_TOKEN_CACHE_ENTRIES) {
    evictOldestTokenCache();
  }

  // Add to cache
  tokenCache_.insert_or_assign(hash, TokenCacheEntry(tokens, hash, ++cacheTimestamp_));
}

void ShikiTokenizer::evictOldestTokenCache() {
  if (tokenCache_.empty()) return;

  auto oldest = tokenCache_.begin();
  for (auto it = tokenCache_.begin(); it != tokenCache_.end(); ++it) {
    if (it->second.lastUsed < oldest->second.lastUsed) {
      oldest = it;
    }
  }
  tokenCache_.erase(oldest);
}

std::vector<Token> ShikiTokenizer::tokenize(const std::string& code) {
  if (!grammar_ || !theme_) {
    std::cerr << "Missing grammar or theme, skipping tokenization" << std::endl;
    return {};
  }

  // Try to get tokens from cache first
  if (auto cachedTokens = tryGetCachedTokens(code)) {
    return *cachedTokens;
  }

  std::vector<Token> tokens;
  tokens.reserve(code.length() / 16);
  std::vector<std::string> scopeStack;

  // Start with base scope from grammar
  if (!grammar_->scopeName.empty()) {
    scopeStack.push_back(grammar_->scopeName);
  }

  // Process patterns and build token tree
  processPatterns(grammar_->getPatterns(), code, tokens, scopeStack);

  // Sort tokens by position
  std::sort(tokens.begin(), tokens.end(), [](const Token& a, const Token& b) { return a.start < b.start; });

  // Resolve theme styles for all tokens
  resolveStyles(tokens);

  // Cache the tokens for future use
  cacheTokens(code, tokens);

  // Return processed tokens
  return tokens;
}

void ShikiTokenizer::processPatterns(
  const std::vector<GrammarPattern>& patterns,
  const std::string& code,
  std::vector<Token>& tokens,
  std::vector<std::string>& scopeStack
) {
  const char* str = code.c_str();
  const char* end = str + code.length();
  const char* start = str;
  auto& tokenPool = TokenPool::getInstance();

  // Only add base scope once at the start if not already present
  if (grammar_ && !grammar_->scopeName.empty() && (scopeStack.empty() || scopeStack[0] != grammar_->scopeName)) {
    scopeStack.insert(scopeStack.begin(), grammar_->scopeName);
  }

  while (start < end) {
    const GrammarPattern* bestPattern = nullptr;
    std::unique_ptr<OnigRegion, decltype(&onigRegionDeleter)> bestRegion(nullptr, onigRegionDeleter);
    size_t bestPosition = std::string::npos;

    // Try each pattern at the current position
    for (const auto& pattern : patterns) {
      if (pattern.match.empty()) continue;

      OnigErrorInfo einfo;
      regex_t* regex;
      int r = onig_new(
        &regex,
        (const UChar*)pattern.match.c_str(),
        (const UChar*)(pattern.match.c_str() + pattern.match.length()),
        ONIG_OPTION_CAPTURE_GROUP,
        ONIG_ENCODING_UTF8,
        ONIG_SYNTAX_DEFAULT,
        &einfo
      );

      if (r != ONIG_NORMAL) {
        continue;
      }

      std::unique_ptr<regex_t, void (*)(regex_t*)> regexGuard(regex, onig_free);
      std::unique_ptr<OnigRegion, decltype(&onigRegionDeleter)> region(onig_region_new(), onigRegionDeleter);

      r = onig_search(
        regex,
        (const UChar*)str,
        (const UChar*)end,
        (const UChar*)start,
        (const UChar*)end,
        region.get(),
        ONIG_OPTION_NONE
      );

      if (r >= 0) {  // Match found
        size_t position = region->beg[0];
        if (bestPosition == std::string::npos || position < bestPosition) {
          bestPosition = position;
          bestPattern = &pattern;
          std::unique_ptr<OnigRegion, decltype(&onigRegionDeleter)> newRegion(onig_region_new(), onigRegionDeleter);
          onig_region_copy(newRegion.get(), region.get());
          bestRegion = std::move(newRegion);
        }
      }
    }

    if (bestPattern && bestRegion) {
      // Validate match boundaries
      if (bestRegion->beg[0] > code.length() || bestRegion->end[0] > code.length()) {
        start++;
        continue;
      }

      // If there's text before the match, create a basic token
      if (start - str < bestRegion->beg[0]) {
        Token* textToken = tokenPool.allocate();
        textToken->start = start - str;
        textToken->length = bestRegion->beg[0] - (start - str);
        for (const auto& scope : scopeStack) {
          textToken->addScope(scope);
        }
        textToken->addScope("text");
        tokens.push_back(*textToken);
        tokenPool.deallocate(textToken);
      }

      // Create a map of positions to tokens to handle overlapping regions
      std::map<std::pair<size_t, size_t>, Token*> positionTokens;

      // Check if this is a comment pattern
      bool isComment = bestPattern->name.find("comment") != std::string::npos;
      bool isLineComment = isComment && bestPattern->name.find("line") != std::string::npos;

      // For line comments, we want to handle them specially to avoid duplicating the comment
      // markers
      if (isLineComment) {
        // Create token for the comment marker (//)
        Token* markerToken = tokenPool.allocate();
        markerToken->start = bestRegion->beg[0];
        markerToken->length = 2;  // Length of "//"
        for (const auto& scope : scopeStack) {
          markerToken->addScope(scope);
        }
        markerToken->addScope(bestPattern->name);
        markerToken->addScope("punctuation.definition.comment");
        positionTokens[{markerToken->start, markerToken->length}] = markerToken;

        // Create token for the comment text
        if (bestRegion->end[0] - bestRegion->beg[0] > 2) {
          Token* textToken = tokenPool.allocate();
          textToken->start = bestRegion->beg[0] + 2;  // Start after "//"
          textToken->length = bestRegion->end[0] - bestRegion->beg[0] - 2;
          for (const auto& scope : scopeStack) {
            textToken->addScope(scope);
          }
          textToken->addScope(bestPattern->name);
          positionTokens[{textToken->start, textToken->length}] = textToken;
        }
      } else {
        // Handle captures first as they are more specific
        if (!bestPattern->captures.empty()) {
          for (const auto& capture_entry : bestPattern->captures) {
            int index = capture_entry.first;
            const std::string& name = capture_entry.second;
            if (index < bestRegion->num_regs && !name.empty() && bestRegion->beg[index] < code.length() &&
                bestRegion->end[index] <= code.length()) {
              Token* captureToken = tokenPool.allocate();
              captureToken->start = bestRegion->beg[index];
              captureToken->length = bestRegion->end[index] - bestRegion->beg[index];

              // Add base scopes
              for (const auto& scope : scopeStack) {
                captureToken->addScope(scope);
              }
              captureToken->addScope(name);

              // Store in position map
              positionTokens[{captureToken->start, captureToken->length}] = captureToken;
            }
          }
        }

        // Create token for the full match, but only for positions not covered by captures
        if (!bestPattern->name.empty()) {
          size_t matchStart = bestRegion->beg[0];
          size_t matchLength = bestRegion->end[0] - bestRegion->beg[0];

          // Check if this position is already covered by a capture
          bool covered = false;
          for (const auto& [pos, token] : positionTokens) {
            if (pos.first == matchStart && pos.second == matchLength) {
              covered = true;
              break;
            }
          }

          if (!covered) {
            Token* token = tokenPool.allocate();
            token->start = matchStart;
            token->length = matchLength;

            // Add scopes from the stack first
            for (const auto& scope : scopeStack) {
              token->addScope(scope);
            }
            token->addScope(bestPattern->name);

            // Store in position map
            positionTokens[{token->start, token->length}] = token;
          }
        }
      }

      // Add all tokens from the position map to the final token list
      for (const auto& [pos, token] : positionTokens) {
        tokens.push_back(*token);
        tokenPool.deallocate(token);
      }

      start = str + bestRegion->end[0];
    } else {
      // No match found, create a basic token for this character
      Token* token = tokenPool.allocate();
      token->start = start - str;
      token->length = 1;
      for (const auto& scope : scopeStack) {
        token->addScope(scope);
      }
      token->addScope("text");
      tokens.push_back(*token);
      tokenPool.deallocate(token);
      start++;
    }
  }

  // If there's any remaining text, create a token for it
  if (start < end) {
    Token* token = tokenPool.allocate();
    token->start = start - str;
    token->length = end - start;
    for (const auto& scope : scopeStack) {
      token->addScope(scope);
    }
    token->addScope("text");
    tokens.push_back(*token);
    tokenPool.deallocate(token);
  }
}

void ShikiTokenizer::resolveStyles(std::vector<Token>& tokens) {
  if (!theme_) return;

  // Use parallel resolution for large documents (>1000 tokens)
  if (tokens.size() > 1000 && concurrencyUtil_) {
    resolveStylesParallel(tokens);
    return;
  }

  auto& styleCache = StyleCache::getInstance();
  size_t resolvedCount = 0;
  size_t cacheHits = 0;

  resolveStylesBatch(tokens.begin(), tokens.end(), styleCache, resolvedCount, cacheHits);

  // Log only essential metrics
  auto metrics = styleCache.getMetrics();
  auto config = Configuration::getInstance().getDefaults();
  std::cout << "[INFO] Style resolution complete - " << cacheHits * 100.0 / tokens.size() << "% session hit rate ("
            << cacheHits << "/" << tokens.size() << " tokens), " << metrics.size << " cached entries" << std::endl;

  std::cout << "\n[TELEMETRY] Cache Performance:" << std::endl;
  std::cout << "  Style Cache:" << std::endl;
  std::cout << "    Lifetime Hit Rate: " << std::fixed << std::setprecision(2) << (metrics.hitRate() * 100) << "% ("
            << metrics.hits << "/" << (metrics.hits + metrics.misses) << " total requests)" << std::endl;
  std::cout << "    Size: " << metrics.size << " entries" << std::endl;
  std::cout << "    Memory Usage: " << metrics.memoryUsage / 1024 << "KB" << std::endl;
  std::cout << "    Evictions: " << metrics.evictions << std::endl;

  std::cout << "  Token Cache:" << std::endl;
  std::cout << "    Size: " << tokenCache_.size() << " entries" << std::endl;
  std::cout << "    Max Size: " << MAX_TOKEN_CACHE_ENTRIES << " entries" << std::endl;

  std::cout << "  Pattern Stats:" << std::endl;
  std::cout << "    Compiled Patterns: " << compiledPatterns_.size() << std::endl;
  std::cout << "    Total Tokens: " << tokens.size() << std::endl;
}

void ShikiTokenizer::resolveStylesParallel(std::vector<Token>& tokens) {
  const size_t batchSize = 250;  // Process 250 tokens per batch
  const size_t numBatches = (tokens.size() + batchSize - 1) / batchSize;

  std::vector<std::promise<std::pair<size_t, size_t>>> promises(numBatches);
  std::vector<std::future<std::pair<size_t, size_t>>> futures;
  futures.reserve(numBatches);

  auto& styleCache = StyleCache::getInstance();
  size_t totalResolved = 0;
  size_t totalCacheHits = 0;

  // Create batches of work
  for (size_t i = 0; i < numBatches; i++) {
    auto start = tokens.begin() + i * batchSize;
    auto end = (i == numBatches - 1) ? tokens.end() : start + batchSize;

    futures.push_back(promises[i].get_future());

    // Create work item for this batch
    auto processFunction = [this, start, end, &styleCache, promise = &promises[i]]() mutable {
      size_t resolvedCount = 0;
      size_t cacheHits = 0;

      try {
        resolveStylesBatch(start, end, styleCache, resolvedCount, cacheHits);
        promise->set_value({resolvedCount, cacheHits});
      } catch (...) {
        promise->set_exception(std::current_exception());
      }
    };

    // Determine work priority based on visibility (first 1000 tokens get high priority)
    bool isVisible = std::distance(tokens.begin(), start) < 1000;
    WorkPriority priority = isVisible ? WorkPriority::HIGH : WorkPriority::NORMAL;

    // Estimate work cost based on number of tokens and their scopes
    size_t estimatedCost = std::distance(start, end) * sizeof(Token) * 2;

    // Submit work item
    WorkPrioritizer::getInstance().submitWork(
      WorkItem(std::move(processFunction), priority, estimatedCost, "style_resolution", isVisible)
    );
  }

  // Process pending work while waiting for results
  while (WorkPrioritizer::getInstance().getMetrics().pendingHigh > 0 ||
         WorkPrioritizer::getInstance().getMetrics().pendingNormal > 0) {
    WorkPrioritizer::getInstance().processPendingWork();
  }

  // Collect results
  for (auto& future : futures) {
    auto [resolved, hits] = future.get();
    totalResolved += resolved;
    totalCacheHits += hits;
  }

  // Log only cache performance in production
  std::cout << "[INFO] Parallel style resolution complete - " << std::fixed << std::setprecision(2)
            << (styleCache.getMetrics().hitRate() * 100) << "% cache hit rate" << std::endl;
}

void ShikiTokenizer::resolveStylesBatch(
  std::vector<Token>::iterator start,
  std::vector<Token>::iterator end,
  shiki::StyleCache& styleCache,
  size_t& resolvedCount,
  size_t& cacheHits
) {
  for (auto it = start; it != end; ++it) {
    auto& token = *it;
    // Try combined scope first from cache
    std::string combinedScope = token.getCombinedScope();
    if (auto cachedStyle = styleCache.getCachedStyle(combinedScope)) {
      token.style = *cachedStyle;
      resolvedCount++;
      cacheHits++;
      continue;
    }

    // Try individual scopes from cache
    ThemeStyle bestStyle;
    int bestSpecificity = -1;
    bool foundInCache = false;

    for (const auto& scope : token.scopes) {
      if (auto cachedStyle = styleCache.getCachedStyle(scope)) {
        int specificity = scope.length();
        if (specificity > bestSpecificity) {
          bestStyle = *cachedStyle;
          bestSpecificity = specificity;
          foundInCache = true;
        }
      }
    }

    if (foundInCache) {
      token.style = bestStyle;
      resolvedCount++;
      cacheHits++;
      continue;
    }

    // If not in cache, resolve from theme and cache the result
    auto combinedStyle = theme_->resolveStyle(combinedScope);
    if (!combinedStyle.color.empty()) {
      styleCache.cacheStyle(combinedScope, combinedStyle);
      token.style = combinedStyle;
      bestSpecificity = combinedScope.length();
      resolvedCount++;
    } else {
      // Try individual scopes
      for (const auto& scope : token.scopes) {
        auto style = theme_->resolveStyle(scope);
        if (!style.color.empty()) {
          styleCache.cacheStyle(scope, style);
          int specificity = scope.length();
          if (specificity > bestSpecificity) {
            bestStyle = style;
            bestSpecificity = specificity;
          }
        }
      }

      token.style = bestStyle.color.empty() ? ThemeStyle{theme_->getForeground().toHex()} : bestStyle;
    }
  }
}

std::vector<Token> ShikiTokenizer::tokenizeParallel(const std::string& code, size_t batchSize) {
  std::vector<std::promise<std::vector<Token>>> promises;
  std::vector<std::future<std::vector<Token>>> futures;
  promises.reserve((code.length() + batchSize - 1) / batchSize);
  futures.reserve(promises.size());

  size_t pos = 0;
  while (pos < code.length()) {
    size_t length = std::min(batchSize, code.length() - pos);
    if (pos + length < code.length()) {
      // Find next safe boundary
      while (length > 0 && !isPatternBoundary(code, pos + length)) {
        length--;
      }
    }

    // Create promise and future
    promises.emplace_back();
    futures.push_back(promises.back().get_future());

    // Create work item
    auto processFunction = [this, code, pos, length, promise = &promises.back()]() mutable {
      try {
        auto tokens = tokenizeBatch(code, pos, length);
        promise->set_value(std::move(tokens));
      } catch (...) {
        promise->set_exception(std::current_exception());
      }
    };

    // Determine work priority based on visibility
    bool isVisible = pos < code.length() && pos + length <= std::min(code.length(), size_t(1000));
    WorkPriority priority = isVisible ? WorkPriority::HIGH : WorkPriority::NORMAL;

    // Estimate work cost based on text length and pattern complexity
    size_t estimatedCost = length * sizeof(Token) * compiledPatterns_.size();

    // Submit work item
    WorkPrioritizer::getInstance().submitWork(
      WorkItem(std::move(processFunction), priority, estimatedCost, "tokenization", isVisible)
    );

    pos += length;
  }

  // Process pending work while waiting for results
  while (WorkPrioritizer::getInstance().getMetrics().pendingHigh > 0 ||
         WorkPrioritizer::getInstance().getMetrics().pendingNormal > 0) {
    WorkPrioritizer::getInstance().processPendingWork();
  }

  // Collect results
  std::vector<std::vector<Token>> results;
  results.reserve(futures.size());
  for (auto& future : futures) {
    results.push_back(future.get());
  }

  return mergeTokens(results);
}

std::vector<Token> ShikiTokenizer::tokenizeBatch(const std::string& code, size_t start, size_t length) {
  std::vector<Token> tokens;
  size_t end = start + length;
  size_t pos = start;

  while (pos < end) {
    bool matched = false;
    size_t bestMatchLength = 0;
    const CompiledPattern* bestPattern = nullptr;
    OnigRegion* bestRegion = onig_region_new();
    std::unique_ptr<OnigRegion, decltype(&onigRegionDeleter)> bestRegionGuard(bestRegion, onigRegionDeleter);

    // Find the best matching pattern at this position
    for (const auto& compiled : compiledPatterns_) {
      OnigRegion* region = onig_region_new();
      std::unique_ptr<OnigRegion, decltype(&onigRegionDeleter)> regionGuard(region, onigRegionDeleter);

      if (onig_search(
            compiled.regex.get(),
            (OnigUChar*)code.c_str(),
            (OnigUChar*)(code.c_str() + code.length()),
            (OnigUChar*)(code.c_str() + pos),
            (OnigUChar*)(code.c_str() + code.length()),
            region,
            ONIG_OPTION_NONE
          ) >= 0) {
        size_t matchLength = region->end[0] - region->beg[0];
        if (!matched || matchLength > bestMatchLength) {
          matched = true;
          bestMatchLength = matchLength;
          bestPattern = &compiled;
          onig_region_copy(bestRegion, region);
        }
      }
    }

    if (matched && bestPattern) {
      Token token;
      token.start = bestRegion->beg[0];
      token.length = bestRegion->end[0] - bestRegion->beg[0];
      token.addScope(bestPattern->name);
      tokens.push_back(token);
      pos = bestRegion->end[0];
    } else {
      pos++;
    }
  }

  return tokens;
}

std::vector<Token> ShikiTokenizer::mergeTokens(std::vector<std::vector<Token>>& batchResults) {
  std::vector<Token> merged;
  size_t totalSize = 0;

  // Calculate total size for reserve
  for (const auto& batch : batchResults) {
    totalSize += batch.size();
  }
  merged.reserve(totalSize);

  // Merge all tokens
  for (auto& batch : batchResults) {
    merged.insert(merged.end(), std::make_move_iterator(batch.begin()), std::make_move_iterator(batch.end()));
  }

  // Sort by position
  std::sort(merged.begin(), merged.end());

  return merged;
}

bool ShikiTokenizer::isPatternBoundary(const std::string& code, size_t pos) {
  if (pos >= code.length()) return true;

  // Simple heuristic: whitespace or common delimiters are safe boundaries
  static const std::string delimiters = " \t\n\r.,;(){}[]<>\"'";
  return delimiters.find(code[pos]) != std::string::npos;
}

ThemeStyle ShikiTokenizer::resolveStyle(const std::string& scope) const {
  if (!theme_) return ThemeStyle();
  return theme_->resolveStyle(scope);
}

bool ShikiTokenizer::findBestMatch(
  const std::string& code,
  size_t pos,
  const std::vector<OnigRegex>& regexes,
  OnigRegion* bestRegion,
  size_t& bestRegexIndex
) {
  int bestPosition = -1;
  bestRegexIndex = 0;

  OnigRegion* region = onig_region_new();
  std::unique_ptr<OnigRegion, OnigRegionDeleter> regionGuard(region);

  for (size_t i = 0; i < regexes.size(); ++i) {
    if (onig_search(
          regexes[i],
          (OnigUChar*)code.c_str(),
          (OnigUChar*)(code.c_str() + code.length()),
          (OnigUChar*)(code.c_str() + pos),
          (OnigUChar*)(code.c_str() + code.length()),
          region,
          ONIG_OPTION_NONE
        ) >= 0) {
      if (bestPosition == -1 || region->beg[0] < static_cast<size_t>(bestPosition)) {
        bestPosition = region->beg[0];
        bestRegexIndex = i;
        onig_region_copy(bestRegion, region);
      }
    }
  }

  return bestPosition >= 0;
}

// Helper function to split a scope string into its component parts
std::vector<std::string> splitScope(const std::string& scope) {
  std::vector<std::string> parts;
  std::istringstream stream(scope);
  std::string part;

  // Split by spaces
  while (std::getline(stream, part, ' ')) {
    if (!part.empty()) {
      parts.push_back(part);
    }
  }
  return parts;
}

bool isScopeMatch(const std::string& ruleScope, const std::string& tokenScope) {
  // Remove debug logging and keep core logic
  auto ruleParts = splitScope(ruleScope);
  auto tokenParts = splitScope(tokenScope);

  bool isCommentRule = false;
  bool isCommentToken = false;
  for (const auto& part : ruleParts) {
    if (part == "comment" || part.find("comment.") == 0) {
      isCommentRule = true;
      break;
    }
  }
  for (const auto& part : tokenParts) {
    if (part == "comment" || part.find("comment.") == 0) {
      isCommentToken = true;
      break;
    }
  }

  if (isCommentRule && isCommentToken) return true;
  if (ruleScope == tokenScope) return true;
  if (tokenScope.find(ruleScope + ".") == 0) return true;

  if (ruleParts.size() > 1) {
    for (const auto& rulePart : ruleParts) {
      bool found = false;
      for (const auto& tokenPart : tokenParts) {
        if (isScopeMatch(rulePart, tokenPart)) {
          found = true;
          break;
        }
      }
      if (!found) return false;
    }
    return true;
  }

  return false;
}

void shiki::OnigRegexDeleter::operator()(regex_t* r) const {
  if (r) {
    onig_free(r);
  }
}

void shiki::OnigRegionDeleter::operator()(OnigRegion* r) const {
  if (r) {
    onig_region_free(r, 1);  // 1 means free the region itself too
  }
}

}  // namespace shiki

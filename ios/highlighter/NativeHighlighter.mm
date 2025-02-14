#import "NativeHighlighter.h"
#include "../../cpp/highlighter/assets/ios/IOSAssetLoader.h"
#include "../../cpp/highlighter/cache/StyleCache.h"
#include "../../cpp/highlighter/cache/SyntaxTreeCache.h"
#include "../../cpp/highlighter/error/HighlightError.h"
#include "../../cpp/highlighter/grammar/Grammar.h"
#include "../../cpp/highlighter/renderer/IOSHighlightRenderer.h"
#include "../../cpp/highlighter/text/TextDiff.h"
#include "../../cpp/highlighter/text/TextRange.h"
#include "../../cpp/highlighter/theme/Theme.h"
#include "../../cpp/highlighter/tokenizer/ShikiTokenizer.h"
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace shiki {

NativeHighlighter& NativeHighlighter::getInstance() {
  static NativeHighlighter instance;
  return instance;
}

bool NativeHighlighter::setGrammar(const std::string& content) {
  try {
    auto grammar = Grammar::fromJson(content);
    if (!grammar) {
      return false;
    }

    currentGrammar_ = std::move(grammar);
    currentLanguage_ = currentGrammar_->getName();
    return true;
  } catch (const HighlightError& e) {
    // Log error if needed
    return false;
  }
}

bool NativeHighlighter::validateGrammar(const std::string& content) {
  return Grammar::validateJson(content);
}

bool NativeHighlighter::setTheme(const std::string& themeName) {
  if (themeName.empty()) {
    return false;
  }

  if (themeName == currentTheme_)
    return true;

  currentTheme_ = themeName;

  try {
    auto theme = loadTheme(themeName);
    ShikiTokenizer::getInstance().setTheme(theme);
    return true;
  } catch (const std::exception& e) {
    // Log error but continue with previous/default theme
    return false;
  }
}

void NativeHighlighter::validateState() const {
  if (!initialized_) {
    throw HighlightError(HighlightErrorCode::InvalidGrammar,
                         "Highlighter not initialized with grammar and theme");
  }
}

std::vector<StyledToken> NativeHighlighter::highlight(const std::string& code, bool parallel) {
  if (code.empty()) {
    return {};
  }

  if (code.length() > MAX_CODE_LENGTH) {
    std::ostringstream oss;
    oss << "Code length (" << code.length() << ") exceeds maximum allowed size (" << MAX_CODE_LENGTH
        << ")";
    throw HighlightError(HighlightErrorCode::InputTooLarge, oss.str());
  }

  validateState();

  // Try to get cached tree
  auto& cache = SyntaxTreeCache::getInstance();
  auto tree = cache.getCachedTree(code, currentLanguage_);

  if (!tree) {
    // Build and cache new tree
    tree = buildSyntaxTree(code);
    cache.cacheTree(code, currentLanguage_, tree);
  }

  // Convert tree to styled tokens
  return tokenizeFromTree(tree);
}

ThemeStyle NativeHighlighter::getDefaultStyle() const {
  ThemeStyle style;
  auto& tokenizer = ShikiTokenizer::getInstance();
  auto theme = tokenizer.getTheme();
  if (theme) {
    style.color = theme->getForeground().toHex();
    // Background color is handled by the view
  } else {
    style.color = "#F8F8F2"; // Dracula default as fallback
  }
  return style;
}

ThemeStyle NativeHighlighter::getLineNumberStyle() const {
  auto& tokenizer = ShikiTokenizer::getInstance();
  auto theme = tokenizer.getTheme();
  if (theme) {
    return theme->getLineNumberStyle();
  }
  // Fallback style if no theme is set
  ThemeStyle style;
  style.color = "#6E7681"; // A neutral gray that works with most themes
  return style;
}

std::shared_ptr<Grammar> NativeHighlighter::loadGrammar(const std::string& language) {
  rapidjson::Document doc = IOSAssetLoader::getInstance().loadAsset(language);
  if (doc.IsNull() || doc.HasParseError()) {
    return nullptr;
  }

  // Convert document to string
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  doc.Accept(writer);
  std::string jsonStr(buffer.GetString(), buffer.GetSize());

  return Grammar::fromJson(jsonStr);
}

std::shared_ptr<Theme> NativeHighlighter::loadTheme(const std::string& themeName) {
  rapidjson::Document doc = IOSAssetLoader::getInstance().loadAsset(themeName);
  if (doc.IsNull() || doc.HasParseError()) {
    return nullptr;
  }

  // Convert document to string
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  doc.Accept(writer);
  std::string jsonStr(buffer.GetString(), buffer.GetSize());

  return Theme::fromJson(jsonStr);
}

void* NativeHighlighter::createNativeView(const HighlighterViewConfig& config) {
  ViewConfig viewConfig;
  viewConfig.fontSize = config.fontSize;
  viewConfig.fontFamily = config.fontFamily;
  viewConfig.showLineNumbers = config.showLineNumbers;
  viewConfig.language = config.language;
  viewConfig.theme = config.theme;
  viewConfig.fontWeight = config.fontWeight;
  viewConfig.fontStyle = config.fontStyle;

  return IOSHighlightRenderer::getInstance().createView(viewConfig);
}

void NativeHighlighter::updateNativeView(void* view, const std::string& code) {
  if (!view)
    return;

  auto tokens = highlight(code);
  IOSHighlightRenderer::getInstance().updateView(view, tokens, code);
}

void NativeHighlighter::destroyNativeView(void* view) {
  if (!view)
    return;
  IOSHighlightRenderer::getInstance().destroyView(view);
}

void NativeHighlighter::setViewUpdateCallback(ViewUpdateCallback callback) {
  viewUpdateCallback_ = std::move(callback);
}

std::shared_ptr<SyntaxTreeNode> NativeHighlighter::buildSyntaxTree(const std::string& code) {
  auto& tokenizer = ShikiTokenizer::getInstance();
  auto tokens = tokenizer.tokenize(code);

  auto root = std::make_shared<SyntaxTreeNode>();
  root->start = 0;
  root->length = code.length();

  // Build tree from tokens
  for (const auto& token : tokens) {
    auto node = std::make_shared<SyntaxTreeNode>();
    node->scope = token.getCombinedScope();
    node->start = token.start;
    node->length = token.length;
    root->children.push_back(node);
  }

  return root;
}

std::vector<StyledToken>
NativeHighlighter::tokenizeFromTree(const std::shared_ptr<SyntaxTreeNode>& tree) {
  std::vector<StyledToken> tokens;
  if (!tree)
    return tokens;

  // Get theme and default foreground color once
  auto& tokenizer = ShikiTokenizer::getInstance();
  auto theme = tokenizer.getTheme();
  std::string defaultForeground = theme ? theme->getForeground().toHex() : "#F8F8F2";

  // Convert tree nodes to styled tokens
  for (const auto& child : tree->children) {
    StyledToken token;
    token.start = child->start;
    token.length = child->length;
    token.scope = child->scope;
    token.style = tokenizer.resolveStyle(child->scope);

    // Always ensure we have a foreground color
    if (token.style.color.empty()) {
      token.style.color = defaultForeground;
    }

    tokens.push_back(token);
  }

  return tokens;
}

std::vector<NativeHighlighter::IncrementalUpdate>
NativeHighlighter::highlightIncremental(const std::string& newText, size_t contextLines) {

  ErrorUtils::throwIfEmpty(newText, "text");
  validateState();

  std::vector<IncrementalUpdate> updates;

  // If no previous text, do full highlight
  if (lastText_.empty()) {
    TextRange fullRange{0, newText.length()};
    updates.push_back({fullRange, highlightRange(newText, fullRange)});

    lastText_ = newText;
    lastTree_ = buildSyntaxTree(newText);
    return updates;
  }

  // Find changed regions
  auto changedRegions = TextDiff::findChangedRegions(lastText_, newText, contextLines);

  // Process each changed region
  for (const auto& region : changedRegions) {
    updates.push_back({region, highlightRange(newText, region)});
  }

  // Update state
  lastText_ = newText;
  lastTree_ = buildSyntaxTree(newText);

  return updates;
}

std::vector<StyledToken> NativeHighlighter::highlightRange(const std::string& text,
                                                           const TextRange& range) {

  // Extract the text range
  std::string rangeText = text.substr(range.start, range.length);

  // Build syntax tree for range
  auto rangeTree = buildTreeForRange(text, range);

  // Convert to styled tokens
  auto tokens = tokenizeFromTree(rangeTree);

  // Adjust token positions to be relative to full text
  for (auto& token : tokens) {
    token.start += range.start;
  }

  return tokens;
}

std::shared_ptr<SyntaxTreeNode> NativeHighlighter::buildTreeForRange(const std::string& text,
                                                                     const TextRange& range) {

  auto& tokenizer = ShikiTokenizer::getInstance();

  // Extract context before/after for accurate tokenization
  size_t contextSize = 1000; // Adjust based on grammar needs

  size_t start = (range.start > contextSize) ? range.start - contextSize : 0;
  size_t end = std::min(range.start + range.length + contextSize, text.length());

  std::string contextText = text.substr(start, end - start);

  // Tokenize with context
  auto tokens = tokenizer.tokenize(contextText);

  // Build tree
  auto root = std::make_shared<SyntaxTreeNode>();
  root->start = range.start;
  root->length = range.length;

  // Filter tokens to just the requested range
  for (const auto& token : tokens) {
    size_t tokenStart = start + token.start;
    if (tokenStart >= range.start && tokenStart < (range.start + range.length)) {

      auto node = std::make_shared<SyntaxTreeNode>();
      node->scope = token.getCombinedScope();
      node->start = tokenStart;
      node->length = token.length;
      root->children.push_back(node);
    }
  }

  return root;
}

} // namespace shiki

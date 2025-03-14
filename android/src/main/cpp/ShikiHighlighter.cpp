#include <jsi/jsi.h>

#include <android/log.h>
#include <fbjni/fbjni.h>
#include <jni.h>

#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include "highlighter/core/Configuration.h"
#include "highlighter/grammar/Grammar.h"
#include "highlighter/theme/Theme.h"
#include "highlighter/theme/ThemeParser.h"
#include "highlighter/theme/ThemeStyle.h"
#include "highlighter/tokenizer/ShikiTokenizer.h"
#include "highlighter/tokenizer/Token.h"

#define TAG       "ShikiHighlighter"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)

bool isValidHexColor(const std::string& color) {
  // Check if the color is a valid hex color (without #)
  if (color.empty()) return false;

  // Valid hex color should be 6 characters (RGB) or 8 characters (ARGB)
  if (color.length() != 6 && color.length() != 8) return false;

  // All characters should be valid hex digits
  for (char c : color) {
    if (!isxdigit(c)) return false;
  }

  return true;
}

using namespace facebook;
using namespace jni;

class ErrorCallback {
 public:
  virtual ~ErrorCallback() = default;
  virtual void onError(const std::string& message) = 0;
};

class JCallbackImpl : public jni::JavaClass<JCallbackImpl> {
 public:
  static constexpr auto kJavaDescriptor = "Lcom/shiki/JCallback;";

  void invoke(const std::string& message) {
    static const auto method = javaClassStatic()->getMethod<void(jstring)>("invoke");
    method(self(), jni::make_jstring(message).get());
  }
};

std::string themesPath;
std::string languagesPath;

std::mutex themeMutex;

static std::unordered_map<std::string, std::shared_ptr<shiki::Theme>> loadedThemes;

jni::local_ref<jni::JClass> findClassLocal(const char* name) {
  return jni::findClassLocal(name);
}

struct ShikiHighlighterImpl : public jni::HybridClass<ShikiHighlighterImpl> {
  static constexpr auto kJavaDescriptor = "Lcom/shiki/ShikiHighlighter;";

  static void registerNatives() {
    javaClassStatic()->registerNatives({
      makeNativeMethod("initHybrid", ShikiHighlighterImpl::initHybrid),
      makeNativeMethod("setupErrorCallback", ShikiHighlighterImpl::setupErrorCallback),
      makeNativeMethod("registerRecoveryStrategies", ShikiHighlighterImpl::registerRecoveryStrategies),
      makeNativeMethod("getStyleCacheNative", ShikiHighlighterImpl::getStyleCacheNative),
      makeNativeMethod(
        "loadGrammarNative",
        "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)J",
        ShikiHighlighterImpl::loadGrammarNative
      ),
      makeNativeMethod(
        "loadThemeNative",
        "(Ljava/lang/String;Ljava/lang/String;)J",
        ShikiHighlighterImpl::loadThemeNative
      ),
      makeNativeMethod(
        "tokenizeNative",
        "(Ljava/lang/String;Ljava/lang/String;)Ljava/util/ArrayList;",
        ShikiHighlighterImpl::tokenizeNative
      ),
      makeNativeMethod("enableCacheNative", ShikiHighlighterImpl::enableCacheNative),
      makeNativeMethod("handleMemoryWarningNative", ShikiHighlighterImpl::handleMemoryWarningNative),
      makeNativeMethod("hasRecoveryStrategyNative", ShikiHighlighterImpl::hasRecoveryStrategyNative),
      makeNativeMethod("addRecoveryStrategyNative", ShikiHighlighterImpl::addRecoveryStrategyNative),
      makeNativeMethod("setGrammarNative", ShikiHighlighterImpl::setGrammarNative),
      makeNativeMethod("setThemeNative", "(Ljava/lang/String;)V", ShikiHighlighterImpl::setThemeNative),
      makeNativeMethod("getThemeNative", ShikiHighlighterImpl::getThemeNative),
      makeNativeMethod(
        "tokenizeParallelNative",
        "(Ljava/lang/String;I)Ljava/util/ArrayList;",
        ShikiHighlighterImpl::tokenizeParallelNative
      ),
      makeNativeMethod(
        "resolveStyleNative",
        "(Ljava/lang/String;)Lcom/shiki/ThemeStyle;",
        ShikiHighlighterImpl::resolveStyleNative
      ),
      makeNativeMethod("releaseGrammarNative", ShikiHighlighterImpl::releaseGrammarNative),
      makeNativeMethod("releaseThemeNative", ShikiHighlighterImpl::releaseThemeNative),
      makeNativeMethod("resetNative", ShikiHighlighterImpl::resetNative),
    });
  }

  ShikiHighlighterImpl() {
    LOGD("ShikiHighlighterImpl created");
  }

  static jni::local_ref<jhybriddata> initHybrid(jni::alias_ref<jclass>) {
    LOGD("initHybrid called");
    return makeCxxInstance();
  }

  static jni::local_ref<jhybriddata> getStyleCacheNative(jni::alias_ref<jhybridobject> jThis) {
    LOGD("getStyleCacheNative called");
    return makeCxxInstance();
  }

  void setupErrorCallback(jni::alias_ref<JCallbackImpl> callback) {
    LOGD("setupErrorCallback called");
    // Store callback for later use
  }

  void registerRecoveryStrategies() {
    LOGD("registerRecoveryStrategies called");
  }

  jlong
  loadGrammarNative(jni::alias_ref<jstring> name, jni::alias_ref<jstring> scopeName, jni::alias_ref<jstring> json) {
    if (!name || !scopeName || !json) {
      LOGE("loadGrammarNative: One or more parameters are null");
      return 0;
    }

    try {
      std::string nameStr = name->toString();
      std::string scopeNameStr = scopeName->toString();
      std::string jsonStr = json->toString();

      LOGD("loadGrammarNative: Loading grammar: %s with scope: %s", nameStr.c_str(), scopeNameStr.c_str());

      try {
        // Parse the grammar JSON properly using the fromJson method
        if (!shiki::Grammar::validateJson(jsonStr)) {
          LOGE("Invalid grammar JSON format for %s", nameStr.c_str());
          return 0;
        }

        auto grammar = shiki::Grammar::fromJson(jsonStr);
        if (!grammar) {
          LOGE("Failed to parse grammar JSON for %s", nameStr.c_str());
          return 0;
        }

        // Set the name and scope name
        grammar->name = nameStr;
        grammar->scopeName = scopeNameStr;

        // Set the grammar in the tokenizer
        auto& tokenizer = shiki::ShikiTokenizer::getInstance();
        tokenizer.setGrammar(grammar);

        // Store the grammar in the class member
        currentGrammar = grammar;
        LOGD("Grammar loaded successfully: %s with %zu patterns", nameStr.c_str(), grammar->getPatterns().size());

        // Return the pointer to the grammar object as a long value
        return reinterpret_cast<jlong>(grammar.get());
      } catch (const std::exception& e) {
        LOGE("Error creating grammar object: %s", e.what());
        // Don't rethrow, just log the error and return
        return 0;
      }
    } catch (const std::exception& e) {
      LOGE("Error in loadGrammarNative: %s", e.what());
      return 0;
    } catch (...) {
      LOGE("Unknown error in loadGrammarNative");
      return 0;
    }
  }

  jlong loadThemeNative(jni::alias_ref<jstring> name, jni::alias_ref<jstring> jsonStr) {
    if (!name || !jsonStr) {
      LOGE("loadThemeNative: name or jsonStr is null");
      return 0;
    }

    try {
      std::string nameStr = name->toString();
      std::string jsonContent = jsonStr->toString();

      LOGD("Loading theme: %s", nameStr.c_str());

      try {
        // Check if theme is already loaded
        {
          std::lock_guard<std::mutex> lock(themeMutex);
          auto it = loadedThemes.find(nameStr);
          if (it != loadedThemes.end()) {
            LOGD("Theme already loaded, reusing: %s", nameStr.c_str());
            // Store the theme in the class member
            currentTheme = it->second;

            // Set the theme in the tokenizer
            auto& tokenizer = shiki::ShikiTokenizer::getInstance();
            tokenizer.setTheme(currentTheme);

            LOGD("Theme reused successfully: %s", nameStr.c_str());

            // Return the pointer to the theme object as a long value
            return reinterpret_cast<jlong>(currentTheme.get());
          }
        }

        // Create a new theme object
        auto theme = std::make_shared<shiki::Theme>(nameStr);

        // Parse the theme JSON
        shiki::ThemeParser parser(theme.get());
        parser.parseThemeStyle(jsonContent);

        // Log theme information
        LOGD("Theme loaded successfully: %s with %zu rules", nameStr.c_str(), theme->getRuleCount());

        // Log some sample rules for debugging
        if (theme->getRuleCount() > 0) {
          LOGD("Sample theme rules:");
          size_t sampleCount = std::min(theme->getRuleCount(), size_t(10));
          for (size_t i = 0; i < sampleCount; i++) {
            const auto& rule = theme->getRuleAt(i);
            LOGD("  Rule %zu: scope='%s', color='%s'", i, rule.scope.c_str(), rule.style.color.c_str());
          }
        } else {
          LOGD("No theme rules found!");
        }

        // Store the theme in the static map to prevent destruction
        {
          std::lock_guard<std::mutex> lock(themeMutex);
          loadedThemes[nameStr] = theme;
        }

        // Store the theme in the class member
        currentTheme = theme;

        // Set the theme in the tokenizer
        auto& tokenizer = shiki::ShikiTokenizer::getInstance();
        tokenizer.setTheme(currentTheme);

        LOGD("Theme loaded successfully: %s", nameStr.c_str());

        // Return the pointer to the theme object as a long value
        return reinterpret_cast<jlong>(theme.get());
      } catch (const std::exception& e) {
        LOGE("Error creating theme object: %s", e.what());
        // Don't rethrow, just log the error and return
        return 0;
      }
    } catch (const std::exception& e) {
      LOGE("Error in loadThemeNative: %s", e.what());
      return 0;
    }
  }

  jni::local_ref<jni::JArrayList<jobject>>
  tokenizeNative(jni::alias_ref<jstring> code, jni::alias_ref<jstring> language) {
    try {
      auto& tokenizer = shiki::ShikiTokenizer::getInstance();

      // Check if tokenizer has a theme
      if (!tokenizer.getTheme()) {
        LOGE("No theme set in tokenizer when tokenizing");
        return jni::JArrayList<jobject>::create(0);
      }

      std::string codeStr = code->toString();
      std::string langStr = language->toString();
      LOGD("Tokenizing code of length %zu for language %s", codeStr.length(), langStr.c_str());

      try {
        std::vector<shiki::Token> tokens = tokenizer.tokenize(codeStr);
        LOGD("Tokenization complete, got %zu tokens", tokens.size());

        // Get the theme for direct rule matching
        auto theme = tokenizer.getTheme();
        const auto& rules = theme->getRules();
        LOGD("Using theme with %zu rules for token styling", rules.size());

        // Log some sample tokens for debugging
        size_t sampleCount = std::min(tokens.size(), size_t(5));
        for (size_t i = 0; i < sampleCount; i++) {
          const auto& token = tokens[i];
          std::string scopesStr;
          for (const auto& scope : token.scopes) {
            if (!scopesStr.empty()) scopesStr += ", ";
            scopesStr += scope;
          }
          LOGD(
            "Sample token %zu: start=%d, length=%d, scopes=[%s], color=%s",
            i,
            token.start,
            token.length,
            scopesStr.c_str(),
            token.style.color.c_str()
          );
        }

        // Ensure each token has a style with a color
        int coloredTokens = 0;
        int directMatchTokens = 0;
        int resolvedStyleTokens = 0;
        int defaultColorTokens = 0;

        for (auto& token : tokens) {
          // Force style resolution for all tokens, even if they already have a color
          bool foundMatch = false;

          // Only attempt to resolve styles if the token has scopes
          if (!token.scopes.empty()) {
            // Try each scope against all theme rules
            for (const auto& scope : token.scopes) {
              for (const auto& rule : rules) {
                // Simple scope matching - could be improved with proper TextMate scope matching
                if (scope == rule.scope || scope.find(rule.scope + ".") == 0 || rule.scope.find(scope + ".") == 0) {
                  if (!rule.style.color.empty()) {
                    token.style.color = rule.style.color;
                    token.style.bold = rule.style.bold;
                    token.style.italic = rule.style.italic;
                    token.style.underline = rule.style.underline;
                    LOGD(
                      "Direct rule match: scope '%s' matched rule '%s' with color '%s'",
                      scope.c_str(),
                      rule.scope.c_str(),
                      rule.style.color.c_str()
                    );
                    foundMatch = true;
                    directMatchTokens++;
                    break;
                  }
                }
              }
              if (foundMatch) break;
            }

            // If no direct match, try resolveStyle
            if (!foundMatch) {
              std::string combinedScope = token.getCombinedScope();
              try {
                shiki::ThemeStyle resolvedStyle = theme->resolveStyle(combinedScope);
                if (!resolvedStyle.color.empty()) {
                  token.style = resolvedStyle;
                  LOGD("Resolved style for scope %s: color=%s", combinedScope.c_str(), token.style.color.c_str());
                  resolvedStyleTokens++;
                  foundMatch = true;
                } else {
                  LOGD("Resolved style for scope %s but no color was set", combinedScope.c_str());
                }
              } catch (const std::exception& e) {
                LOGE("Error resolving style for scope %s: %s", combinedScope.c_str(), e.what());
              }
            }
          }

          // If we didn't find a match, use the default color
          if (!foundMatch) {
            // If no match was found, use theme's foreground color
            std::string oldColor = token.style.color;
            token.style.color = theme->getForeground().toHex();
            defaultColorTokens++;

            // Log if we're changing from a non-default color
            if (!oldColor.empty() && oldColor != token.style.color) {
              LOGD("Changed token color from %s to %s (default)", oldColor.c_str(), token.style.color.c_str());
            }
          } else {
            coloredTokens++;
          }
        }

        LOGD(
          "Token color stats: %d colored tokens, %d direct matches, %d resolved styles, %d default colors",
          coloredTokens,
          directMatchTokens,
          resolvedStyleTokens,
          defaultColorTokens
        );

        return createTokenArray(tokens);
      } catch (const std::exception& e) {
        LOGE("Error in tokenize: %s", e.what());
        return jni::JArrayList<jobject>::create(0);
      }
    } catch (const std::exception& e) {
      LOGE("Error in tokenizeNative: %s", e.what());
      return jni::JArrayList<jobject>::create(0);
    } catch (...) {
      LOGE("Unknown error in tokenizeNative");
      return jni::JArrayList<jobject>::create(0);
    }
  }

  void enableCacheNative(jboolean enabled) {
    LOGD("enableCacheNative called with: %d", enabled);
  }

  void handleMemoryWarningNative(jni::alias_ref<jhybriddata> hybridData) {
    LOGD("handleMemoryWarningNative called");
  }

  jboolean hasRecoveryStrategyNative(jint code) {
    LOGD("hasRecoveryStrategyNative called with code: %d", code);
    return JNI_FALSE;
  }

  void addRecoveryStrategyNative(jint code, jboolean strategy) {
    LOGD("addRecoveryStrategyNative called with code: %d, strategy: %d", code, strategy);
  }

  void setGrammarNative(jni::alias_ref<jstring> grammar) {
    if (!grammar) {
      LOGE("setGrammarNative: grammar is null");
      return;
    }

    try {
      std::string grammarName = grammar->toString();
      LOGD("setGrammarNative: Setting grammar: %s", grammarName.c_str());

      // Check if we already have this grammar loaded
      if (currentGrammar && currentGrammar->name == grammarName) {
        LOGD("Grammar %s is already set, skipping", grammarName.c_str());
        return;
      }

      try {
        auto grammarObj = std::make_shared<shiki::Grammar>(grammarName);

        // Store the grammar in the class member
        currentGrammar = grammarObj;

        auto& tokenizer = shiki::ShikiTokenizer::getInstance();
        tokenizer.setGrammar(currentGrammar);

        LOGD("Grammar set successfully: %s", grammarName.c_str());
      } catch (const std::exception& e) {
        LOGE("Error creating grammar object: %s", e.what());
        // Don't rethrow, just log the error and return
      }
    } catch (const std::exception& e) {
      LOGE("Error in setGrammarNative: %s", e.what());
    } catch (...) {
      LOGE("Unknown error in setGrammarNative");
    }
  }

  void setThemeNative(jni::alias_ref<jstring> theme) {
    if (!theme) {
      LOGE("setThemeNative: theme is null");
      return;
    }

    try {
      std::string themeName = theme->toString();
      LOGD("setThemeNative: Setting theme: %s", themeName.c_str());

      auto& tokenizer = shiki::ShikiTokenizer::getInstance();

      // Check if we already have this theme loaded and set
      if (currentTheme && currentTheme->name == themeName && tokenizer.getTheme() == currentTheme) {
        LOGD("Theme %s is already set, skipping", themeName.c_str());
        return;
      }

      // Look for the theme in our static map
      std::shared_ptr<shiki::Theme> themeToSet;
      {
        std::lock_guard<std::mutex> lock(themeMutex);
        auto it = loadedThemes.find(themeName);
        if (it != loadedThemes.end()) {
          themeToSet = it->second;
        }
      }

      // If we found the theme, set it
      if (themeToSet) {
        LOGD("Using already loaded theme from map: %s", themeName.c_str());
        currentTheme = themeToSet;
        tokenizer.setTheme(currentTheme);
        LOGD("Theme set successfully: %s", themeName.c_str());
        return;
      }

      // If we have the theme loaded but not set in the tokenizer
      if (currentTheme && currentTheme->name == themeName) {
        LOGD("Using already loaded theme: %s", themeName.c_str());
        tokenizer.setTheme(currentTheme);
        LOGD("Theme set successfully: %s", themeName.c_str());
        return;
      }

      // If we don't have the theme loaded, log a warning
      LOGW("Theme %s not found or not loaded yet", themeName.c_str());
      // We could potentially load the theme here if it's not loaded yet
    } catch (const std::exception& e) {
      LOGE("setThemeNative: Exception: %s", e.what());
    } catch (...) {
      LOGE("setThemeNative: Unknown exception");
    }
  }

  jni::local_ref<jstring> getThemeNative() {
    try {
      auto& tokenizer = shiki::ShikiTokenizer::getInstance();
      if (auto theme = tokenizer.getTheme()) {
        return jni::make_jstring(theme->name);
      }
      return jni::make_jstring("");
    } catch (const std::exception& e) {
      LOGE("Error in getThemeNative: %s", e.what());
      return jni::make_jstring("");
    }
  }

  jni::local_ref<jni::JArrayList<jobject>> tokenizeParallelNative(jni::alias_ref<jstring> code, jint batchSize) {
    try {
      auto& tokenizer = shiki::ShikiTokenizer::getInstance();

      // Check if tokenizer has a theme
      if (!tokenizer.getTheme()) {
        LOGE("No theme set in tokenizer when tokenizing in parallel");
        return jni::JArrayList<jobject>::create(0);
      }

      std::string codeStr = code->toString();
      LOGD("Tokenizing code in parallel with batch size %d, code length %zu", batchSize, codeStr.length());

      try {
        std::vector<shiki::Token> tokens = tokenizer.tokenizeParallel(codeStr, batchSize);
        LOGD("Parallel tokenization complete, got %zu tokens", tokens.size());
        return createTokenArray(tokens);
      } catch (const std::exception& e) {
        LOGE("Error in tokenizeParallel: %s", e.what());
        return jni::JArrayList<jobject>::create(0);
      }
    } catch (const std::exception& e) {
      LOGE("Error in tokenizeParallelNative: %s", e.what());
      return jni::JArrayList<jobject>::create(0);
    } catch (...) {
      LOGE("Unknown error in tokenizeParallelNative");
      return jni::JArrayList<jobject>::create(0);
    }
  }

  jni::local_ref<jobject> resolveStyleNative(jni::alias_ref<jstring> scope) {
    try {
      auto& tokenizer = shiki::ShikiTokenizer::getInstance();
      std::string scopeStr = scope->toString();

      LOGD("Resolving style for scope: %s", scopeStr.c_str());

      // Check if tokenizer has a theme
      auto theme = tokenizer.getTheme();
      if (!theme) {
        LOGE("No theme set in tokenizer when resolving style for: %s", scopeStr.c_str());
        // Create a default style with default color
        shiki::ThemeStyle defaultStyle;
        defaultStyle.color = "#ffffff";  // Default to white if no theme is available
        return createThemeStyle(defaultStyle);
      }

      try {
        // Resolve the style with proper error handling
        shiki::ThemeStyle style = theme->resolveStyle(scopeStr);
        LOGD("Style resolved for scope %s: color=%s", scopeStr.c_str(), style.color.c_str());
        return createThemeStyle(style);
      } catch (const std::exception& e) {
        LOGE("Error resolving style for scope %s: %s", scopeStr.c_str(), e.what());
        // Create a default style with the theme's foreground color
        shiki::ThemeStyle defaultStyle;
        defaultStyle.color = theme->getForeground().toHex();
        if (defaultStyle.color.empty()) {
          defaultStyle.color = "#ffffff";  // Default to white if foreground color is not set
        }
        return createThemeStyle(defaultStyle);
      }
    } catch (const std::exception& e) {
      LOGE("Error in resolveStyleNative: %s", e.what());
      // Create a default style with default color
      shiki::ThemeStyle defaultStyle;
      defaultStyle.color = "#ffffff";  // Default to white
      return createThemeStyle(defaultStyle);
    } catch (...) {
      LOGE("Unknown error in resolveStyleNative");
      // Create a default style with default color
      shiki::ThemeStyle defaultStyle;
      defaultStyle.color = "#ffffff";  // Default to white
      return createThemeStyle(defaultStyle);
    }
  }

  void releaseGrammarNative() {
    LOGD("releaseGrammarNative called");
    currentGrammar.reset();
  }

  void releaseThemeNative() {
    LOGD("releaseThemeNative called");
    currentTheme.reset();
  }

  void resetNative() {
    LOGD("resetNative called");
    currentGrammar.reset();
    currentTheme.reset();
  }

  static jni::local_ref<jobject> createThemeStyle(const shiki::ThemeStyle& style) {
    try {
      JNIEnv* env = jni::Environment::current();
      if (!env) {
        LOGE("Failed to get JNI environment in createThemeStyle");
        return nullptr;
      }

      jclass themeStyleClass = env->FindClass("com/shiki/ThemeStyle");
      if (!themeStyleClass) {
        LOGE("Failed to find ThemeStyle class");
        return nullptr;
      }

      jmethodID constructor = env->GetMethodID(themeStyleClass, "<init>", "()V");
      if (!constructor) {
        LOGE("Failed to find ThemeStyle constructor");
        return nullptr;
      }

      jobject themeStyle = env->NewObject(themeStyleClass, constructor);
      if (!themeStyle) {
        LOGE("Failed to create ThemeStyle object");
        return nullptr;
      }

      jfieldID colorField = env->GetFieldID(themeStyleClass, "color", "Ljava/lang/String;");
      jfieldID bgColorField = env->GetFieldID(themeStyleClass, "backgroundColor", "Ljava/lang/String;");
      jfieldID boldField = env->GetFieldID(themeStyleClass, "bold", "Z");
      jfieldID italicField = env->GetFieldID(themeStyleClass, "italic", "Z");
      jfieldID underlineField = env->GetFieldID(themeStyleClass, "underline", "Z");

      if (!colorField || !bgColorField || !boldField || !italicField || !underlineField) {
        LOGE("Failed to find one or more ThemeStyle fields");
        return nullptr;
      }

      try {
        if (!style.color.empty()) {
          // Ensure color begins with '#' if it's a valid hex color
          std::string colorStr = style.color;
          if (colorStr[0] != '#' && isValidHexColor(colorStr)) {
            colorStr = "#" + colorStr;
          }

          // Only set the color if it's a valid format
          if (colorStr[0] == '#' && isValidHexColor(colorStr.substr(1))) {
            LOGD(
              "Setting color: %s (original: %s) for scope: %s",
              colorStr.c_str(),
              style.color.c_str(),
              style.scope.c_str()
            );
            auto jColorStr = jni::make_jstring(colorStr);
            if (jColorStr) {
              env->SetObjectField(themeStyle, colorField, jColorStr.get());
            }
          } else {
            LOGD("Invalid color format: %s for scope: %s", style.color.c_str(), style.scope.c_str());
            // Don't set a default color here - leave it null to be handled by the Kotlin side
          }
        } else {
          // Don't provide a default color for tokens that don't have one
          // This will allow the Kotlin side to handle default colors appropriately
          LOGD("No color in style for scope: %s", style.scope.c_str());
        }
      } catch (const std::exception& e) {
        LOGE("Error setting foreground color: %s", e.what());
      }

      // Handle background color
      try {
        if (!style.backgroundColor.empty()) {
          // Ensure backgroundColor begins with '#' if it's a valid hex color
          std::string bgColorStr = style.backgroundColor;
          if (bgColorStr[0] != '#' && isValidHexColor(bgColorStr)) {
            bgColorStr = "#" + bgColorStr;
          }

          // Only set the background color if it's a valid format
          if (bgColorStr[0] == '#' && isValidHexColor(bgColorStr.substr(1))) {
            LOGD(
              "Setting backgroundColor: %s (original: %s) for scope: %s",
              bgColorStr.c_str(),
              style.backgroundColor.c_str(),
              style.scope.c_str()
            );
            auto jBgColorStr = jni::make_jstring(bgColorStr);
            if (jBgColorStr) {
              env->SetObjectField(themeStyle, bgColorField, jBgColorStr.get());
            }
          } else {
            LOGD(
              "Skipping invalid background color format: %s for scope: %s",
              style.backgroundColor.c_str(),
              style.scope.c_str()
            );
          }
        }
      } catch (const std::exception& e) {
        LOGE("Error setting background color: %s", e.what());
      }

      try {
        env->SetBooleanField(themeStyle, boldField, style.bold ? JNI_TRUE : JNI_FALSE);
        env->SetBooleanField(themeStyle, italicField, style.italic ? JNI_TRUE : JNI_FALSE);
        env->SetBooleanField(themeStyle, underlineField, style.underline ? JNI_TRUE : JNI_FALSE);
      } catch (const std::exception& e) {
        LOGE("Error setting font style properties: %s", e.what());
      }

      return jni::adopt_local(themeStyle);
    } catch (const std::exception& e) {
      LOGE("Error in createThemeStyle: %s", e.what());
      return nullptr;
    } catch (...) {
      LOGE("Unknown error in createThemeStyle");
      return nullptr;
    }
  }

 private:
  friend HybridBase;
  jni::alias_ref<jhybridobject> jSelf_;
  std::shared_ptr<ErrorCallback> errorCallback_;

  std::shared_ptr<shiki::Grammar> currentGrammar;
  std::shared_ptr<shiki::Theme> currentTheme;

  jni::local_ref<jni::JArrayList<jobject>> createTokenArray(const std::vector<shiki::Token>& tokens) {
    try {
      LOGD("Creating token array with %zu tokens", tokens.size());
      auto resultArray = jni::JArrayList<jobject>::create(tokens.size());
      JNIEnv* env = jni::Environment::current();

      if (!env) {
        LOGE("Failed to get JNI environment in createTokenArray");
        return jni::JArrayList<jobject>::create(0);
      }

      jclass tokenClass = env->FindClass("com/shiki/Token");
      if (!tokenClass) {
        LOGE("Failed to find Token class");
        return jni::JArrayList<jobject>::create(0);
      }

      jmethodID tokenConstructor = env->GetMethodID(tokenClass, "<init>", "()V");
      if (!tokenConstructor) {
        LOGE("Failed to find Token constructor");
        return jni::JArrayList<jobject>::create(0);
      }

      jfieldID startField = env->GetFieldID(tokenClass, "start", "I");
      jfieldID lengthField = env->GetFieldID(tokenClass, "length", "I");
      jfieldID styleField = env->GetFieldID(tokenClass, "style", "Lcom/shiki/ThemeStyle;");

      if (!startField || !lengthField || !styleField) {
        LOGE("Failed to find Token fields");
        return jni::JArrayList<jobject>::create(0);
      }

      int processedTokens = 0;
      for (const auto& token : tokens) {
        try {
          jobject tokenObj = env->NewObject(tokenClass, tokenConstructor);
          if (!tokenObj) {
            LOGE("Failed to create Token object");
            continue;
          }

          env->SetIntField(tokenObj, startField, token.start);
          env->SetIntField(tokenObj, lengthField, token.length);

          try {
            auto styleObj = createThemeStyle(token.style);
            if (styleObj) {
              env->SetObjectField(tokenObj, styleField, styleObj.get());
            }
          } catch (const std::exception& e) {
            LOGE("Error creating theme style: %s", e.what());
            // Continue with the token, just without a style
          }

          auto javaToken = jni::adopt_local(tokenObj);
          resultArray->add(javaToken);
          processedTokens++;
        } catch (const std::exception& e) {
          LOGE("Error processing token at index %d: %s", processedTokens, e.what());
          // Continue with the next token
        }
      }

      LOGD("Successfully processed %d out of %zu tokens", processedTokens, tokens.size());
      return resultArray;
    } catch (const std::exception& e) {
      LOGE("Error in createTokenArray: %s", e.what());
      return jni::JArrayList<jobject>::create(0);
    } catch (...) {
      LOGE("Unknown error in createTokenArray");
      return jni::JArrayList<jobject>::create(0);
    }
  }
};

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void*) {
  return jni::initialize(vm, [] { ShikiHighlighterImpl::registerNatives(); });
}

extern "C" JNIEXPORT jobjectArray JNICALL Java_com_shiki_view_ShikiHighlighterView_processBatchNative(
  JNIEnv* env,
  jobject thiz,
  jstring text,
  jint start,
  jint length
) {
  // Find the Token class
  jclass tokenClass = env->FindClass("com/shiki/Token");
  if (tokenClass == nullptr) {
    LOGE("processBatchNative: Failed to find Token class");
    return nullptr;
  }

  if (text == nullptr) {
    LOGE("processBatchNative: text is null");
    return env->NewObjectArray(0, tokenClass, nullptr);
  }

  try {
    const char* textChars = env->GetStringUTFChars(text, nullptr);
    if (textChars == nullptr) {
      LOGE("processBatchNative: GetStringUTFChars returned null");
      return env->NewObjectArray(0, tokenClass, nullptr);
    }

    std::string textStr(textChars);
    env->ReleaseStringUTFChars(text, textChars);

    LOGD("processBatchNative: Processing text of length %zu", textStr.length());

    auto& tokenizer = shiki::ShikiTokenizer::getInstance();

    // Check if tokenizer has a theme
    if (!tokenizer.getTheme()) {
      LOGE("processBatchNative: No theme set in tokenizer");
      try {
        // Create a default theme with basic styles
        auto defaultTheme = std::make_shared<shiki::Theme>("default");

        // Set default foreground and background colors
        defaultTheme->setForeground(shiki::ThemeColor("#ffffff"));
        defaultTheme->setBackground(shiki::ThemeColor("#000000"));

        // Add a default style for the entire source
        shiki::ThemeStyle defaultStyle;
        defaultStyle.color = "#ffffff";
        defaultStyle.scope = "source";
        defaultTheme->addStyle(defaultStyle);

        // Important: Store a strong reference to the theme object
        // This prevents it from being destroyed prematurely
        static std::shared_ptr<shiki::Theme> savedDefaultTheme;
        savedDefaultTheme = defaultTheme;

        // Set the new theme
        tokenizer.setTheme(savedDefaultTheme);
        LOGD("processBatchNative: Created and set default theme");
      } catch (const std::exception& e) {
        LOGE("processBatchNative: Error creating default theme: %s", e.what());
        return env->NewObjectArray(0, tokenClass, nullptr);
      } catch (...) {
        LOGE("processBatchNative: Unknown error creating default theme");
        return env->NewObjectArray(0, tokenClass, nullptr);
      }
    }

    // Tokenize the text - this will throw an exception if no grammar is set
    std::vector<shiki::Token> tokens;
    try {
      LOGD("processBatchNative: Starting tokenization");
      tokens = tokenizer.tokenize(textStr);
      LOGD("processBatchNative: Generated %zu tokens", tokens.size());
    } catch (const std::exception& e) {
      LOGE("processBatchNative: Exception during tokenization: %s", e.what());
      return env->NewObjectArray(0, tokenClass, nullptr);
    }

    // Create the Java array of tokens
    jobjectArray result = env->NewObjectArray(tokens.size(), tokenClass, nullptr);
    if (result == nullptr) {
      LOGE("processBatchNative: Failed to create result array");
      return env->NewObjectArray(0, tokenClass, nullptr);
    }

    jmethodID tokenConstructor = env->GetMethodID(tokenClass, "<init>", "()V");
    if (tokenConstructor == nullptr) {
      LOGE("processBatchNative: Failed to find Token constructor");
      return env->NewObjectArray(0, tokenClass, nullptr);
    }

    jfieldID startField = env->GetFieldID(tokenClass, "start", "I");
    jfieldID lengthField = env->GetFieldID(tokenClass, "length", "I");
    jfieldID styleField = env->GetFieldID(tokenClass, "style", "Lcom/shiki/ThemeStyle;");

    if (startField == nullptr || lengthField == nullptr || styleField == nullptr) {
      LOGE("processBatchNative: Failed to find Token fields");
      return env->NewObjectArray(0, tokenClass, nullptr);
    }

    int tokensWithColor = 0;
    int tokensWithDefaultColor = 0;

    // Get the theme's foreground color for default styling
    std::string defaultColor = "#ffffff";  // Default to white if no theme is available
    if (auto theme = tokenizer.getTheme()) {
      defaultColor = theme->getForeground().toHex();
      if (defaultColor.empty()) {
        defaultColor = "#ffffff";  // Fallback to white if foreground color is not set
      }
    }

    // Fill the array with Token objects
    for (size_t i = 0; i < tokens.size(); i++) {
      const auto& token = tokens[i];

      jobject tokenObj = env->NewObject(tokenClass, tokenConstructor);
      if (tokenObj == nullptr) {
        LOGE("processBatchNative: Failed to create Token object for token %zu", i);
        continue;
      }

      env->SetIntField(tokenObj, startField, token.start);
      env->SetIntField(tokenObj, lengthField, token.length);

      // Get the style for this token
      shiki::ThemeStyle style;
      if (!token.scopes.empty()) {
        // Use the first scope for style resolution
        std::string scope = token.scopes[0];
        try {
          LOGD("processBatchNative: Resolving style for token %zu with scope: %s", i, scope.c_str());

          // Create a local copy of the style to ensure memory safety
          style = tokenizer.resolveStyle(scope);

          // If no color was resolved, set the theme's default foreground color
          if (style.color.empty()) {
            LOGD("processBatchNative: No color resolved for scope: %s, using theme's default color", scope.c_str());
            style.color = defaultColor;
            tokensWithDefaultColor++;
          } else {
            tokensWithColor++;
            LOGD("processBatchNative: Resolved color %s for scope: %s", style.color.c_str(), scope.c_str());
          }
        } catch (const std::exception& e) {
          LOGE("processBatchNative: Exception resolving style for scope %s: %s", scope.c_str(), e.what());
          style.color = defaultColor;
          tokensWithDefaultColor++;
        }
      } else {
        LOGD("processBatchNative: Token %zu has no scopes, using default style", i);
        style.color = defaultColor;
        tokensWithDefaultColor++;
      }

      // Create the ThemeStyle object and set it on the token
      try {
        // Use a local variable to hold the reference
        jni::local_ref<jobject> styleObjRef = ShikiHighlighterImpl::createThemeStyle(style);
        jobject styleObj = styleObjRef.get();

        if (styleObj != nullptr) {
          env->SetObjectField(tokenObj, styleField, styleObj);
        } else {
          LOGE("processBatchNative: Failed to create style object for token %zu", i);
          // Create a default style with the theme's default color
          shiki::ThemeStyle defaultStyle;
          defaultStyle.color = defaultColor;

          // Use a local variable to hold the reference
          jni::local_ref<jobject> defaultStyleObjRef = ShikiHighlighterImpl::createThemeStyle(defaultStyle);
          jobject defaultStyleObj = defaultStyleObjRef.get();

          if (defaultStyleObj != nullptr) {
            env->SetObjectField(tokenObj, styleField, defaultStyleObj);
          } else {
            LOGE("processBatchNative: Failed to create default style object for token %zu", i);
          }
        }
      } catch (const std::exception& e) {
        LOGE("processBatchNative: Exception creating style object: %s", e.what());
        // Create a default style with the theme's default color
        try {
          shiki::ThemeStyle defaultStyle;
          defaultStyle.color = defaultColor;

          // Use a local variable to hold the reference
          jni::local_ref<jobject> defaultStyleObjRef = ShikiHighlighterImpl::createThemeStyle(defaultStyle);
          jobject defaultStyleObj = defaultStyleObjRef.get();

          if (defaultStyleObj != nullptr) {
            env->SetObjectField(tokenObj, styleField, defaultStyleObj);
          }
        } catch (...) {
          LOGE("processBatchNative: Failed to create fallback style object for token %zu", i);
        }
      }

      env->SetObjectArrayElement(result, i, tokenObj);
      env->DeleteLocalRef(tokenObj);
    }

    LOGD(
      "processBatchNative: Applied styles to all tokens. Total: %zu, With color: %d, With default color: %d",
      tokens.size(),
      tokensWithColor,
      tokensWithDefaultColor
    );

    return result;
  } catch (const std::exception& e) {
    LOGE("processBatchNative: Exception: %s", e.what());
    return env->NewObjectArray(0, tokenClass, nullptr);
  } catch (...) {
    LOGE("processBatchNative: Unknown exception");
    return env->NewObjectArray(0, tokenClass, nullptr);
  }
}

#include "AndroidHighlighter.h"

#include <sstream>

#include "../../../../cpp/highlighter/grammar/Grammar.h"
#include "../../../../cpp/highlighter/theme/Theme.h"
#include "../../../../cpp/highlighter/tokenizer/ShikiTokenizer.h"

namespace shiki {

AndroidHighlighter& AndroidHighlighter::getInstance() {
  static AndroidHighlighter instance;
  return instance;
}

AndroidHighlighter::~AndroidHighlighter() {
  if (auto env = getEnv()) {
    clearJNIReferences(env);
  }
}

bool AndroidHighlighter::setGrammar(const std::string& content) {
  try {
    grammar_ = Grammar::fromJson(content);
    ShikiTokenizer::getInstance().setGrammar(grammar_);
    return true;
  } catch (const std::exception& e) {
    // TODO: Add proper error handling/logging
    return false;
  }
}

bool AndroidHighlighter::setTheme(const std::string& themeName) {
  try {
    theme_ = Theme::fromJson(themeName);
    ShikiTokenizer::getInstance().setTheme(theme_);
    return true;
  } catch (const std::exception& e) {
    // TODO: Add proper error handling/logging
    return false;
  }
}

void* AndroidHighlighter::createNativeView(const PlatformViewConfig& config) {
  JNIEnv* env = getEnv();
  if (!env) return nullptr;

  jobject view = createJavaView(env, config);
  if (!view) return nullptr;

  // Create a global reference to keep the view alive
  return env->NewGlobalRef(view);
}

void AndroidHighlighter::updateNativeView(void* view, const std::string& code) {
  if (!view) return;

  JNIEnv* env = getEnv();
  if (!env) return;

  updateJavaView(env, static_cast<jobject>(view), code);
}

void AndroidHighlighter::destroyNativeView(void* view) {
  if (!view) return;

  JNIEnv* env = getEnv();
  if (!env) return;

  env->DeleteGlobalRef(static_cast<jobject>(view));
}

void AndroidHighlighter::setViewConfig(void* view, const PlatformViewConfig& config) {
  if (!view) return;
  // TODO: Implement view configuration update
}

void AndroidHighlighter::invalidateResources() {
  grammar_.reset();
  theme_.reset();
  ShikiTokenizer::getInstance().setGrammar(nullptr);
  ShikiTokenizer::getInstance().setTheme(nullptr);
}

void AndroidHighlighter::handleMemoryWarning() {
  // TODO: Implement memory warning handling
  // Could clear caches, release non-essential resources, etc.
}

void AndroidHighlighter::setUpdateCallback(ViewUpdateCallback callback) {
  updateCallback_ = std::move(callback);
}

void AndroidHighlighter::setJavaVM(JavaVM* vm) {
  javaVM_ = vm;
  if (auto env = getEnv()) {
    cacheJNIReferences(env);
  }
}

void AndroidHighlighter::attachCurrentThread() {
  if (!javaVM_) return;

  JNIEnv* env = nullptr;
  if (javaVM_->AttachCurrentThread(&env, nullptr) != JNI_OK) {
    // TODO: Handle error
  }
}

void AndroidHighlighter::detachCurrentThread() {
  if (!javaVM_) return;

  javaVM_->DetachCurrentThread();
}

JNIEnv* AndroidHighlighter::getEnv() const {
  if (!javaVM_) return nullptr;

  JNIEnv* env = nullptr;
  if (javaVM_->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
    return nullptr;
  }
  return env;
}

jobject AndroidHighlighter::createJavaView(JNIEnv* env, const PlatformViewConfig& config) {
  if (!viewClass_ || !createViewMethod_) return nullptr;

  // Convert config to Java objects
  jstring language = env->NewStringUTF(config.language.c_str());
  jstring theme = env->NewStringUTF(config.theme.c_str());
  jstring fontFamily = env->NewStringUTF(config.fontFamily.c_str());

  // Call Java method to create view
  jobject view = env->CallStaticObjectMethod(
    viewClass_,
    createViewMethod_,
    language,
    theme,
    config.showLineNumbers,
    static_cast<jfloat>(config.fontSize),
    fontFamily
  );

  // Cleanup local references
  env->DeleteLocalRef(language);
  env->DeleteLocalRef(theme);
  env->DeleteLocalRef(fontFamily);

  return view;
}

void AndroidHighlighter::updateJavaView(JNIEnv* env, jobject view, const std::string& code) {
  if (!view || !updateViewMethod_ || !updateCallback_) return;

  try {
    auto tokens = ShikiTokenizer::getInstance().tokenize(code);
    std::vector<PlatformStyledToken> platformTokens;
    platformTokens.reserve(tokens.size());

    for (const auto& token : tokens) {
      PlatformStyledToken platformToken{token.start, token.length, token.style, token.getCombinedScope()};
      platformTokens.push_back(std::move(platformToken));
    }

    // Convert tokens to Java array/objects and update view
    // TODO: Implement Java-side token handling

    updateCallback_(code, platformTokens);
  } catch (const std::exception& e) {
    // TODO: Add proper error handling/logging
  }
}

void AndroidHighlighter::cacheJNIReferences(JNIEnv* env) {
  // Find and cache ShikiHighlighterView class and methods
  jclass localClass = env->FindClass("com/shiki/ShikiHighlighterView");
  if (localClass) {
    viewClass_ = static_cast<jclass>(env->NewGlobalRef(localClass));
    createViewMethod_ = env->GetStaticMethodID(
      viewClass_,
      "create",
      "(Ljava/lang/String;Ljava/lang/String;ZFLjava/lang/String;)Lcom/shiki/ShikiHighlighterView;"
    );
    updateViewMethod_ = env->GetMethodID(viewClass_, "updateContent", "(Ljava/lang/String;[Lcom/shiki/StyledToken;)V");
    env->DeleteLocalRef(localClass);
  }
}

void AndroidHighlighter::clearJNIReferences(JNIEnv* env) {
  if (viewClass_) {
    env->DeleteGlobalRef(viewClass_);
    viewClass_ = nullptr;
  }
  createViewMethod_ = nullptr;
  updateViewMethod_ = nullptr;
}

}  // namespace shiki

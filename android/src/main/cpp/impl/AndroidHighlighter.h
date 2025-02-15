#pragma once

#include <jni.h>

#include <functional>

#include "../../../../cpp/highlighter/platform/PlatformHighlighter.h"

namespace shiki {

using ViewUpdateCallback = std::function<void(const std::string&, const std::vector<PlatformStyledToken>&)>;

class AndroidHighlighter : public PlatformHighlighter {
 public:
  static AndroidHighlighter& getInstance();

  // PlatformHighlighter implementation
  bool setGrammar(const std::string& content) override;
  bool setTheme(const std::string& themeName) override;
  void* createNativeView(const PlatformViewConfig& config) override;
  void updateNativeView(void* view, const std::string& code) override;
  void destroyNativeView(void* view) override;
  void setViewConfig(void* view, const PlatformViewConfig& config) override;
  void invalidateResources() override;
  void handleMemoryWarning() override;

  // Android-specific methods
  void setUpdateCallback(ViewUpdateCallback callback);
  void setJavaVM(JavaVM* vm);
  void attachCurrentThread();
  void detachCurrentThread();

  // JNI helpers
  JNIEnv* getEnv() const;
  jobject createJavaView(JNIEnv* env, const PlatformViewConfig& config);
  void updateJavaView(JNIEnv* env, jobject view, const std::string& code);

 private:
  AndroidHighlighter() = default;
  ~AndroidHighlighter();

  ViewUpdateCallback updateCallback_;
  JavaVM* javaVM_{nullptr};
  jclass viewClass_{nullptr};
  jmethodID createViewMethod_{nullptr};
  jmethodID updateViewMethod_{nullptr};

  // JNI reference management
  void cacheJNIReferences(JNIEnv* env);
  void clearJNIReferences(JNIEnv* env);
};

}  // namespace shiki

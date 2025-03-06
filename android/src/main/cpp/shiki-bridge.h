#ifndef SHIKI_BRIDGE_H
#define SHIKI_BRIDGE_H

#include <jsi/jsi.h>

#include <jni.h>

#include <string>

using namespace facebook;

extern "C" {
JNIEXPORT void JNICALL Java_com_shiki_ShikiBridge_nativeInstall(JNIEnv* env, jobject thiz, jobject jsiContextHolder);
}

#endif  // SHIKI_BRIDGE_H

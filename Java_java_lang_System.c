/*
 * Java_java_lang_System.c
 *
 *  Created on: Jan 11, 2013
 *      Author: hammer
 */

#include "jni.h"
#include "vmtime.h"

JNIEXPORT jlong JNICALL Java_java_lang_System_nanoTime(JNIEnv *env, jclass cls) {
    return vtNanoTime();
}

JNIEXPORT jlong JNICALL Java_java_lang_System_currentTimeMillis(JNIEnv *env, jclass cls) {
    return vtCurrentTimeMillis();
}

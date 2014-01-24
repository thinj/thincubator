/*
 * Java_java_lang_Thread.c
 *
 *  Created on: Jan 25, 2013
 *      Author: hammer
 */
#include "jni.h"
#include "vmids.h"
#include "frame.h"
#include "jarray.h"
#include "console.h"
#include "objectaccess.h"
#include "thread.h"
#include "constantpool.h"

//#define DEBUG_ENABLE
#include "debug.h"

JNIEXPORT void JNICALL Java_java_lang_Thread_attach(JNIEnv *env, jobject this) {
    thAttach(env, this);
}

JNIEXPORT void JNICALL Java_java_lang_Thread_yield(JNIEnv *env, jclass cls) {
    thYield(env);
}

JNIEXPORT void JNICALL Java_java_lang_Object_notifyAll(JNIEnv *env, jobject lockedObject) {
    thNotify(env, lockedObject, TRUE);
}

JNIEXPORT void JNICALL Java_java_lang_Object_notify(JNIEnv *env, jobject lockedObject) {
    thNotify(env, lockedObject, FALSE);
}

JNIEXPORT void JNICALL Java_java_lang_Thread_sleep(JNIEnv *env, jclass cls, jlong millis) {
    thSleep(env, millis);
}

JNIEXPORT void JNICALL Java_java_lang_Thread_interrupt(JNIEnv *env, jobject this) {
    thInterrupt(env, this);
}

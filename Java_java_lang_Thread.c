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

JNIEXPORT jobject JNICALL Java_java_lang_Thread_getMainStack(JNIEnv *env, jclass cls) {
    return NULL;//osGetCurrentStack();
}

JNIEXPORT jobject JNICALL Java_java_lang_Thread_allocStack(JNIEnv *env, jobject this) {
//    // Allocate an object without protecting it:
//    jarray stackObject = osAllocateStack();
//
//    if (stackObject != NULL) {
//        // Prepare the stack so it contains 'this' as its first element:
//        stackable* stack = jaGetArrayPayLoad(stackObject);
//
//        const methodInClass* mic = getVirtualMethodEntryByLinkId(this, M_java_lang_Thread_runFromNative);
//        __DEBUG("start addr = 0x%04x", mic->codeOffset);
//        // Prepare the VM context so it will start running the method leading way to Thread.run():
//        u2 sp = 0;
//
//        // Push 'this':
//        stack[sp].operand.jref = this;
//        stack[sp++].type = OBJECTREF;
//    }
//    // else: An out-of-mem exception has been thrown
//
//    return stackObject;
    return NULL;
}

JNIEXPORT void JNICALL Java_java_lang_Thread_yield(JNIEnv *env, jclass cls) {
    thYield(env);
}

JNIEXPORT void JNICALL Java_java_lang_Thread_startScheduling(JNIEnv *env, jclass cls) {
    // The initial stack shall no longer be protected:
    osUnprotectStack();
    consoutli("Unprotect othe rstuff ?\n");
    thStartScheduling();
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

JNIEXPORT void JNICALL Java_java_lang_Thread_addToThreadCollection(JNIEnv *env, jclass threadCls, jobject thread) {
    thAddToThreadCollection(env, threadCls, thread);
}

JNIEXPORT jobject JNICALL Java_java_lang_Thread_getMainNativeContext(JNIEnv *env, jclass cls) {
    return NULL;//thGetMainNativeContext();
}

JNIEXPORT jobject JNICALL Java_java_lang_Thread_getMainNativeStack(JNIEnv *env, jclass cls) {
    return thGetMainNativeStack();
}

JNIEXPORT jobject JNICALL Java_java_lang_Thread_allocNativeContext(JNIEnv *env, jobject this, jobject stack) {
    return thAllocNativeContext(env, this, stack);
}

JNIEXPORT jobject JNICALL Java_java_lang_Thread_allocNativeStack(JNIEnv *env, jobject this) {
    return thAllocNativeStack();
}

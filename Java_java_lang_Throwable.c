/*
 * Java_java_lang_Throwable.c
 *
 *  Created on: Mar 2, 2012
 *      Author: hammer
 */

#include "jni.h"

JNIEXPORT jint JNICALL Java_java_lang_Throwable_getStackTraceElement(JNIEnv *env,
        jobject exception, jint index) {
    // This might be quite inefficient, but it saves memory...

    contextDef savedContext = *env;
    int count = 0;
    jint pc = -1;
    while (TRUE) {
        if (count >= index || env->framePointer == 0) {
            pc = env->programCounter - 1;
            break;
        }
        count++;
        pop_frame(env);
    }
    *env = savedContext;

    return pc;
}

JNIEXPORT jint JNICALL Java_java_lang_Throwable_getStackTraceDepth(JNIEnv *env, jobject exception) {
    contextDef savedContext = *env;
    jint count = 0;
    while (TRUE) {
        count++;
        if (env->framePointer == 0) {
            break;
        }
        pop_frame(env);
    }
    *env = savedContext;

    return count;
}

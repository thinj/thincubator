/*
 * Java_java_lang_Object.c
 *
 *  Created on: Mar 18, 2011
 *      Author: hammer
 */

#include <stdlib.h>

#include "console.h"
#include "heap.h"
#include "heaplist.h"

#include "constantpool.h"
#include "objectaccess.h"
#include "thread.h"
#include "vmids.h"
#include "jni.h"
#include "debug.h"

jint Java_java_lang_Object_hashCode(void *context, jobject this) {
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
    return (jint) this;
#pragma GCC diagnostic warning "-Wpointer-to-int-cast"
}

jobject JNICALL Java_java_lang_Object_getClass(JNIEnv *env, jobject this) {
    u2 classId = oaGetClassIdFromObject(this);

    // Find Class instance that has classId 'classId':
    int i;
    jobject class = NULL;
    int len = GetArrayLength((jarray) cpJavaLangClassArray);
    u2 aClassIdLinkId = A_java_lang_Class_aClassId;
    for (i = 0; i < len && class == NULL; i++) {
        jobject classObject = GetObjectArrayElement(env, cpJavaLangClassArray, i);

        jint classObjectDotaClassId = GetIntField(env, classObject, aClassIdLinkId);

        if (classObjectDotaClassId == (jint) classId) {
            class = classObject;
        }
    }

    if (class == NULL) {
        __DEBUG("NULL class!!!\n");
    }
    return class;
}

JNIEXPORT void JNICALL Java_java_lang_Object_wait(JNIEnv *env, jobject lockedObject, jlong timeout) {
    thWait(env, lockedObject, timeout);
}


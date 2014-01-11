/*
 * jni.c
 *
 *  Created on: Feb 20, 2012
 *      Author: hammer
 */

#include <string.h>
#include "jni.h"
#include "trace.h"
#include "heaplist.h"
#include "instructions.h"
#include "objectaccess.h"
#include "vmids.h"
#include "simplelist.h"
#include "constantpool.h"
#include "exceptions.h"

void call_static_method(contextDef* context, u2 referencedClassId, u2 linkId) {
    BEGIN
            ;

    const methodInClass* mic;

    CALL(mic = getStaticMethodEntryByLinkId(referencedClassId, linkId));

    cpInvokeCommon(context, mic, TRUE);

    inExecute(context);

    END;
}

void call_instance_method(contextDef* context, jobject referencedClassObject, u2 linkId) {
    BEGIN
            ;

    const methodInClass* mic;

    CALL(mic = getVirtualMethodEntryByLinkId(referencedClassObject, linkId));

    cpInvokeCommon(context, mic, TRUE);

    inExecute(context);

    END;
}

/**
 * This function returns a pointer to the stackable within the static memory part of the
 * class identified by cls and linkId.
 * \param cls The class containing the static field
 * \param linkId The id of the member
 * \param type The expected type of the static field (for validation only)
 * \return A pointer to the stackable
 */
static stackable* GetStaticFieldStackable(contextDef* context, jclass cls, u2 linkId, stackType type) {
    u2 classId = GetIntField(context, (jobject) cls, A_java_lang_Class_aClassId);
    const fieldInClass* fic = getFieldInClassbyLinkId(classId, linkId);

    stackable* val = GetStaticField(fic->address);

#ifdef VALIDATE_TYPE
    if (cls != NULL) {
        VALIDATE_TYPE(val->type, type);
    }
#endif

    return val;
}

void SetBooleanField(contextDef* context, jobject obj, u2 linkId, jboolean value) {
    u2 classId = oaGetClassIdFromObject(obj);
    const fieldInClass* fic = getFieldInClassbyLinkId(classId, linkId);

    stackable val;
    val.operand.jrenameint = (value == 0 ? 0 : 1);
    val.type = JAVAINT;
    PutField(context, obj, fic->address, &val);
}

void SetIntField(contextDef* context, jobject obj, u2 linkId, jint value) {
    u2 classId = oaGetClassIdFromObject(obj);
    const fieldInClass* fic = getFieldInClassbyLinkId(classId, linkId);

    stackable val;
    val.operand.jrenameint = value;
    val.type = JAVAINT;
    PutField(context, obj, fic->address, &val);
}

void SetLongField(contextDef* context, jobject obj, u2 linkId, jlong value) {
    u2 classId = oaGetClassIdFromObject(obj);
    const fieldInClass* fic = getFieldInClassbyLinkId(classId, linkId);

    stackable val;
    val.operand.jrenameint = value & 0xffffffff;
    val.type = JAVAINT;
    PutField(context, obj, fic->address, &val);

    val.operand.jrenameint = (value >> 32) & 0xffffffff;
    PutField(context, obj, fic->address + 1, &val);
}

void SetObjectField(contextDef* context, jobject obj, u2 linkId, jobject value) {
    u2 classId = oaGetClassIdFromObject(obj);
    const fieldInClass* fic = getFieldInClassbyLinkId(classId, linkId);

    stackable val;
    val.operand.jref = value;
    val.type = OBJECTREF;
    PutField(context, obj, fic->address, &val);
}

void SetStaticObjectField(contextDef* context, jclass cls, u2 linkId, jobject value) {
    u2 classId = GetIntField(context, (jobject) cls, A_java_lang_Class_aClassId);
    const fieldInClass* fic = getFieldInClassbyLinkId(classId, linkId);

    stackable val;
    val.operand.jref = value;
    val.type = OBJECTREF;
    PutStaticField(fic->address, &val);
}

static stackable* GetFieldStackable(contextDef* context, jobject obj, u2 linkId, stackType type, int offset) {
    if (obj != NULL) {
        u2 classId = oaGetClassIdFromObject(obj);
        const fieldInClass* fic = getFieldInClassbyLinkId(classId, linkId);

        stackable* val = GetField(context, obj, fic->address + offset);

#ifdef VALIDATE_TYPE
        if (obj != NULL) {
            VALIDATE_TYPE(val->type, type);
        }
#endif

        return val;
    } else {
        throwNullPointerException(context);
    }
    
    return NULL;
}

jobject GetStaticObjectField(contextDef* context, jclass cls, u2 linkId) {
    stackable* val = GetStaticFieldStackable(context, cls, linkId, OBJECTREF);

    return val == NULL ? NULL : val->operand.jref;
}

jobject GetObjectField(contextDef* context, jobject obj, u2 linkId) {
    stackable* val = GetFieldStackable(context, obj, linkId, OBJECTREF, 0);

    return val == NULL ? NULL : val->operand.jref;
}

jint GetIntField(contextDef* context, jobject obj, u2 linkId) {
    stackable* val = GetFieldStackable(context, obj, linkId, JAVAINT, 0);

    return val == NULL ? 0 : val->operand.jrenameint;
}

jboolean GetBooleanField(contextDef* context, jobject obj, u2 linkId) {
    stackable* val = GetFieldStackable(context, obj, linkId, JAVAINT, 0);

    return val == NULL ? 0 : (jboolean) (val->operand.jrenameint == 0 ? 0 : 1);
}

jlong GetLongField(contextDef* context, jobject obj, u2 linkId) {
    stackable* val = GetFieldStackable(context, obj, linkId, JAVAINT, 0);

    jlong res;
    if (val != NULL) {
        res = val->operand.jrenameint;
        val = GetFieldStackable(context, obj, linkId, JAVAINT, 1);
        if (val != NULL) {
            res += ((jlong) (val->operand.jrenameint)) << 32;
        } else {
            res = 0;
        }
    } else {
        res = 0;
    }

    return res;
}

BOOL ExceptionCheck(contextDef* context) {
    return context->exceptionThrown;
}

jstring NewString(contextDef* context, const jchar *value) {
    u2 size;
    getClassSize(C_java_lang_String, &size); // size of java.lang.String

    jstring stringObject = (jstring) heapAllocObjectByStackableSize(context, size, C_java_lang_String);

    if (stringObject != NULL) {
        // Allocate char[]:
        size_t len = strlen((char*) value);

        // Avoid garbage collection of the newly allocated String object (there is no reference to it yet,
        // so a protection is necessary):
        heapProtect((jobject) stringObject, TRUE);
        jcharArray charArray = NewCharArray(context, len);
        heapProtect((jobject) stringObject, FALSE);

        if (charArray != NULL) {
            // memcpy into charArray (length and type are set):
            int i;
            for (i = 0; i < len; i++) {
                SetCharArrayElement(context, charArray, i, value[i]);
            }

            u2 linkId = A_java_lang_String_value;
            SetObjectField(context, (jobject) stringObject, linkId, (jobject) charArray);
        }
        // else: out of mem has been thrown
    }
    return stringObject;

}

jobject AllocObject(contextDef* context, jclass cls) {
    u2 classId = GetIntField(context, (jobject) cls, A_java_lang_Class_aClassId);
    u2 size;
    getClassSize(classId, &size);
    return heapAllocObjectByStackableSize(context, size, classId);
}

jboolean IsSameObject(jobject ref1, jobject ref2) {
    return (jboolean) (ref1 == ref2);
}

jobject NewGlobalRef(contextDef* context, jobject ref) {
    jclass cls = getJavaLangClass(context, C_java_lang_Class);
    slAdd(context, GetStaticObjectField(context, cls, A_java_lang_Class_aGlobalRefList), ref);
    // TODO Replace this check with pointer-test:
    if (ExceptionCheck(context)) {
        // Out of mem:
        ref = NULL;
    }
    return ref;
}

void DeleteGlobalRef(contextDef* context, jobject ref) {
    jclass cls = getJavaLangClass(context, C_java_lang_Class);
    slRemove(context, GetStaticObjectField(context, cls, A_java_lang_Class_aGlobalRefList), ref);
}

/*
 * jarray.c
 *
 *  Created on: Sep 14, 2012
 *      Author: hammer
 */

#include "types.h"
#include "jarray.h"
#include "heap.h"
#include "heaplist.h"
#include "constantpool.h"
#include "jni.h"
#include "exceptions.h"

static size_t GetAlignedArrayHeaderSize() {
    return ToAlignedSize(sizeof (array_t));
}

/**
 * \return A void* the to the memory area where the array elements resides. The length field is not part of
 * the pointed-to area
 * \throws NullPointerException if array == null
 */
void* jaGetArrayPayLoad(contextDef* context, jarray array) {
    align_t* at = (align_t*) GetObjectPayload(context, (jobject) array);
    if (at != NULL) {
        at += GetAlignedArrayHeaderSize();
    }

    return (void*) at;
}

/**
 * This method validates the array pointer, the index value and returns a pointer to
 * the position in memory where index 0 is positioned (the first element in the array)
 *
 * \throws NullPointerException if array == NULL
 * \throws ArrayIndexOutOfBoundsException if ix < 0 or ix >= length of array
 * \return NULL, if an exception has been thrown; otherwise a valid pointer is returned.
 */
static void* GetPointerToArrayPosition(contextDef* context, jarray array, jint ix) {
    void* p = jaGetArrayPayLoad(context, array);
    if (p != NULL) {
        if (ix < 0 || ix >= GetArrayLength(array)) {
            throwArrayIndexOutOfBoundsException(context, ix, GetArrayLength(array));
            p = NULL;
        }
    }

    return p;
}

/**
 * This method returns the array control struct from the supplied array. Goes for both primitive array
 * and object arrays.
 * \param array Assumed to be a jarray
 * \return The array control struct for the array. It is the responsibility of the caller to ensure
 * that the pointer indeed is an array
 */
static array_t* GetArrayHeader(jarray array) {
    return ((array_t*) GetObjectPayloadNoThrow((jobject) array));
}

void InitArray(jarray array, size_t length) { //, u2 elementClassId) {
    array_t* a = GetArrayHeader(array);
    a->length = length;
    //a->elementClassId = elementClassId;
}

size_t GetAlignedArraySize(size_t payloadSize) {
    return GetAlignedArrayHeaderSize() + ToAlignedSize(payloadSize);
}

// Common macro for returning a value in an array. All exception checks etc. are done,
// and the macro will 'return' (so don't place any code after use of this macro):
#define GET_ARRAY_ELEMENT(array, index, TYPE) \
		TYPE* p = (TYPE*) GetPointerToArrayPosition(context, array, index); \
		if (p != NULL) { \
			p += index; \
			return *p; \
		} \
		return (TYPE) 0;

// Common macro for setting a value in an array. All exception checks etc. are done:
#define SET_ARRAY_ELEMENT(array, index, value, TYPE) \
		HEAP_VALIDATE; \
		TYPE* p = (TYPE*) GetPointerToArrayPosition(context, array, index); \
		if (p != NULL) { \
			p += index; \
			*p = value; \
		} \
		HEAP_VALIDATE;

jobject GetObjectArrayElement(contextDef* context, jobjectArray array, size_t index) {
    GET_ARRAY_ELEMENT((jarray) array, index, jobject);
}

void SetObjectArrayElement(contextDef* context, jobjectArray array, size_t index, jobject value) {
    HEAP_VALIDATE;

    SET_ARRAY_ELEMENT((jarray) array, index, value, jobject);
}

size_t GetArrayLength(jarray array) {
    return GetArrayHeader(array)->length;
}

void SetByteArrayRegion(contextDef* context, jbyteArray array, jsize start, jsize len, jbyte *buf) {
    HEAP_VALIDATE;

    jbyte* p = (jbyte*) GetPointerToArrayPosition(context, (jarray) array, start);
    if (p == NULL) {
        return;
    }

    // Validate end also:
    GetPointerToArrayPosition(context, (jarray) array, start + len - 1);

    if (array == NULL) {
        // By testing the array after the above call, an NPE will be thrown automatically.
        return;
    }

    if (p != NULL && buf != NULL) {
        p += start;
        while (len-- > 0) {
            *p = *buf;
            p++;
            buf++;
        }
    }
    HEAP_VALIDATE;
}

void GetByteArrayRegion(contextDef* context, jbyteArray array, jsize start, jsize len, jbyte *buf) {
    HEAP_VALIDATE;

    jbyte* p = (jbyte*) GetPointerToArrayPosition(context, (jarray) array, start);
    if (p == NULL) {
        return;
    }

    // Validate end also:
    GetPointerToArrayPosition(context, (jarray) array, start + len - 1);

    if (array == NULL) {
        return;
    }

    if (p != NULL && buf != NULL) {
        p += start;
        while (len-- > 0) {
            *buf = *p;
            p++;
            buf++;
        }
    }
    HEAP_VALIDATE;
}

jobjectArray NewObjectArray(contextDef* context, jint count, u2 elementClassId, jobject init) {
    HEAP_VALIDATE;
    u2 arrayClassId = getArrayClassIdForElementClassId(elementClassId);
    size_t size = sizeof (jobject);
    // The payload size:
    u2 payloadSize = count * size;

    size_t alignedSizeInBytes = GetAlignedArraySize(payloadSize) * sizeof (align_t);

    jobjectArray array = (jobjectArray) heapAllocObjectByByteSize(context, alignedSizeInBytes, arrayClassId);

    if (array == NULL) {
        throwOutOfMemoryError(context);
    } else {
        InitArray((jarray) array, count); //, elementClassId);
        jint i;
        for (i = 0; i < count; i++) {
            SetObjectArrayElement(context, (jobjectArray) array, i, init);
        }
    }

    HEAP_VALIDATE;
    return array;
}

jboolean GetBooleanArrayElement(contextDef* context, jbooleanArray array, size_t index) {
    GET_ARRAY_ELEMENT((jarray) array, index, jboolean);
}

jbyte GetByteArrayElement(contextDef* context, jbyteArray array, size_t index) {
    GET_ARRAY_ELEMENT((jarray) array, index, jbyte);
}

jchar GetCharArrayElement(contextDef* context, jcharArray array, size_t index) {
    GET_ARRAY_ELEMENT((jarray) array, index, jchar);
}

jint GetIntArrayElement(contextDef* context, jintArray array, size_t index) {
    GET_ARRAY_ELEMENT((jarray) array, index, jint);
}

jint GetLongArrayElement(contextDef* context, jlongArray array, size_t index) {
    GET_ARRAY_ELEMENT((jarray) array, index, jlong);
}

static jarray NewPrimitiveArray(contextDef* context, CLASS_TYPE classType, size_t len, size_t elementSize) {
    HEAP_VALIDATE;
    u2 arrayClassId = getClassIdForClassType(classType);

    u2 payloadSize = len * elementSize;

    size_t alignedSizeInBytes = GetAlignedArraySize(payloadSize) * sizeof (align_t);

    jarray array = (jarray) heapAllocObjectByByteSize(context, alignedSizeInBytes, arrayClassId);

    if (array != NULL) {
        InitArray(array, len); //, 0);
    }
    //else: out of mem has been thrown
    HEAP_VALIDATE;

    return array;
}

jcharArray NewCharArray(contextDef* context, size_t len) {
    return (jcharArray) NewPrimitiveArray(context, CT_CHAR_ARRAY, len, sizeof (jchar));
}

jbooleanArray NewBooleanArray(contextDef* context, size_t len) {
    return (jbooleanArray) NewPrimitiveArray(context, CT_BOOLEAN_ARRAY, len, sizeof (jboolean));
}

jbyteArray NewByteArray(contextDef* context, size_t len) {
    return (jbyteArray) NewPrimitiveArray(context, CT_BYTE_ARRAY, len, sizeof (jbyte));
}

jintArray NewIntArray(contextDef* context, size_t len) {
    return (jintArray) NewPrimitiveArray(context, CT_INT_ARRAY, len, sizeof (jint));
}

jlongArray NewLongArray(contextDef* context, size_t len) {
    return (jlongArray) NewPrimitiveArray(context, CT_LONG_ARRAY, len, sizeof (jlong));
}

void SetBooleanArrayElement(contextDef* context, jbooleanArray array, size_t index, BOOL value) {
    SET_ARRAY_ELEMENT((jarray) array, index, value, jboolean);
}

void SetByteArrayElement(contextDef* context, jbyteArray array, size_t index, jbyte value) {
    SET_ARRAY_ELEMENT((jarray) array, index, value, jbyte);
}

void SetIntArrayElement(contextDef* context, jintArray array, size_t index, jint value) {
    SET_ARRAY_ELEMENT((jarray) array, index, value, jint);
}

void SetLongArrayElement(contextDef* context, jlongArray array, size_t index, jlong value) {
    SET_ARRAY_ELEMENT((jarray) array, index, value, jlong);
}

void SetCharArrayElement(contextDef* context, jcharArray array, size_t index, jchar value) {
    SET_ARRAY_ELEMENT((jarray) array, index, value, jchar);
}

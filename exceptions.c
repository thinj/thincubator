/*
 * exceptions.c
 *
 *  Created on: Sep 23, 2012
 *      Author: hammer
 */

#include "constantpool.h"
#include "frame.h"
#include "heap.h"
#include "jni.h"
#include "objectaccess.h"
#include "vmids.h"
#include "heaplist.h"

/**
 * This method looks up the exception handler for the exception. Note that program counter is used when
 * looking up the handler.
 *
 * \param containingClassId The class containing the code throwing the exception
 * \param exceptionClassId The exception thrown
 *
 * \return The found handler or NULL, if no handler for the exception.
 */
const exceptionHandler* getExceptionHandler(contextDef* context, u2 containingClassId, u2 exceptionClassId) {
    int i;
    const exceptionHandler* hit = NULL;
    do {
        containingClassId = context->classIndex;
        // The tables expect PC to be incremented after the instruction:
        codeIndex pc = context->programCounter - 1;
        for (i = 0; i < numberOfAllExceptionHandersInAllClasses; i++) {
            const exceptionHandler* p = NULL;
            p = &(allExceptionHandersInAllClasses[i]);

            BOOL classOk = p->exceptionClassId == exceptionClassId;
            if (!classOk) {
                classOk = is_S_SubClassing_T(exceptionClassId, p->exceptionClassId);
            }
            if (p->classId == containingClassId && classOk && p->startPC <= pc && p->endPC > pc) {
                hit = p;
                break;
            }
        }
        if (hit == NULL) {
            if (context->framePointer > 0) {
                pop_frame(context);
            } else {
                // No exception handler; return null:
                break;
            }
        }
    } while (hit == NULL);

    return hit;
}

/**
 * This method throws an exception
 *
 * \param exception The exception to throw
 */
void throwException(contextDef* context, jobject exception) {
    // See VM Spec section 3.10:file:///tools/vmspec/Overview.doc.html#15494
    // Get class id of exception:
    u2 exceptionClassId = oaGetClassIdFromObject(exception);
    //__DEBUG("containing classId: %d\n", context->classIndex);
    //__DEBUG("class id of exception: %d\n", (int) exceptionClassId);

    contextDef contextCopy = *context;
    const exceptionHandler* p = getExceptionHandler(context, context->classIndex, exceptionClassId);

    if (p != NULL) {
        //__DEBUG("Found handler: %04x\n", p->handlerPC);
        operandStackPushObjectRef(context, exception);
        context->programCounter = p->handlerPC;
    } else {
        HEAP_VALIDATE;
        //heap_dump();
        *context = contextCopy;
        consoutli("Uncaught exception (%d), terminating thread: \n", (int) exceptionClassId);
        consoutli("Terminate thread; create method for this in thread.c\n");
        consoutli("lav om saa det er method returns...\n");
        consoutli("vent til sammensmeltning af native kald og java kald...\n");
        jvmexit(1);
    }
    context->exceptionThrown = TRUE;
}

/**
 * This method throws a Null Pointer Exception
 */
void throwNullPointerException(contextDef* context) {
    jobject npe = newObject(context, C_java_lang_NullPointerException);

    operandStackPushObjectRef(context, npe);

    call_instance_method(context, npe, M_java_lang_NullPointerException__init);

    throwException(context, npe);
}

/**
 * This method throws an Interrupted Exception
 */
void throwInterruptedException(contextDef* context) {
    jobject npe = newObject(context, C_java_lang_InterruptedException);

    operandStackPushObjectRef(context, npe);

    call_instance_method(context, npe, M_java_lang_InterruptedException__init);

    throwException(context, npe);
}

/**
 * This method throws a Negative Array Size Exception
 */
void throwNegativeArraySizeException(contextDef* context) {
    jobject npe = newObject(context, C_java_lang_NegativeArraySizeException);

    operandStackPushObjectRef(context, npe);

    call_instance_method(context, npe, M_java_lang_NegativeArraySizeException__init);

    throwException(context, npe);
}

void throwArrayIndexOutOfBoundsException(contextDef* context, jint index, jint arrayLength) {
    jobject except = newObject(context, C_java_lang_ArrayIndexOutOfBoundsException);
    operandStackPushObjectRef(context, except);
    operandStackPushJavaInt(context, index);

    call_instance_method(context, except, M_java_lang_ArrayIndexOutOfBoundsException__init_I);

    throwException(context, except);
}

void throwArithmeticException(contextDef* context, const char* cause) {
    jobject except = newObject(context, C_java_lang_ArithmeticException);

    operandStackPushObjectRef(context, except);
    jobject jstr = (jobject) NewString(context, (jchar*) cause);
    operandStackPushObjectRef(context, jstr);

    call_instance_method(context, except, M_java_lang_ArithmeticException__init_Ljava_lang_String);

    throwException(context, except);
}

void throwIllegalMonitorStateException(contextDef* context, const char* cause) {
    jobject except = newObject(context, C_java_lang_IllegalMonitorStateException);

    operandStackPushObjectRef(context, except);
    jstring jstr = NewString(context, (jchar*) cause);
    operandStackPushObjectRef(context, (jobject) jstr);

    call_instance_method(context, except, M_java_lang_IllegalMonitorStateException__init_Ljava_lang_String);

    throwException(context, except);
}

void throwClassCastException(contextDef* context, u2 classId_S, u2 classId_T) {
    jobject exception = newObject(context, C_java_lang_ClassCastException);

    operandStackPushObjectRef(context, exception);

    call_instance_method(context, exception, M_java_lang_ClassCastException__init);

    throwException(context, exception);
}

void throwOutOfMemoryError(contextDef* context) {
    u2 classId = C_java_lang_OutOfMemoryError;
    u2 memberId = M_java_lang_OutOfMemoryError_getInstance;
    call_static_method(context, classId, memberId);

    // Pop exception:
    jobject exception = operandStackPopObjectRef(context);

    throwException(context, exception);
}

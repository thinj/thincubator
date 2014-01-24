/*
 * operandstack.c
 *
 *  Created on: Aug 22, 2010
 *      Author: hammer
 */

#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "console.h"

#include "jarray.h"
#include "operandstack.h"
#include "frame.h"
#include "heap.h"
#include "heaplist.h"
#include "jni.h"
#include "vmids.h"
#include "constantpool.h"


// TODO Make the current stack part of context -or, even better: Let there be one and only one combined c/java stack!
// The current stack :
stackable* stack;



void osSetStack(contextDef* context, jbyteArray jStack) {
    // Avoid dereferencing the java type each time the stack is going to be referenced:
    stack = jaGetArrayPayLoad(context, (jarray) jStack);
}

stackable* getStack() {
    return stack;
}

void pop(contextDef* context, stackable* ret) {
    if (context->stackPointer > 0) {
        --context->stackPointer;
        *ret = *(stack + context->stackPointer);
    } else {
        consout("Operand Stack underrun: %04x\n", context->stackPointer);
        jvmexit(1);
    }
}

void push(contextDef* context, stackableOperand OP, stackType TYPE) {
    if (context->stackPointer >= STACK_SIZE) {
        consoutli("stack overflow: %d\n", (int) (context->stackPointer));
        jvmexit(1);
    }
    stack[context->stackPointer].operand = OP;
    stack[context->stackPointer].type = TYPE;
    context->stackPointer++;
}

jint POP_VERIFY_INT(contextDef* context) {
    if (context->stackPointer > 0) {
        --context->stackPointer;
        VALIDATE_TYPE((stack + context->stackPointer)->type, JAVAINT);
        return (stack + context->stackPointer)->operand.jrenameint;
    } else {
        consout("Operand Stack underrun: %04x\n", context->stackPointer);
        jvmexit(1);
    }

    return 0;
}

jobject POP_VERIFY_JREF(contextDef* context) {
    if (context->stackPointer > 0) {
        --context->stackPointer;
        VALIDATE_TYPE((stack + context->stackPointer)->type, OBJECTREF);
        return (stack + context->stackPointer)->operand.jref;
    } else {
        consout("Operand Stack underrun: %04x\n", context->stackPointer);
        jvmexit(1);
    }

    return 0;
}

u2 POP_VERIFY_U2(contextDef* context) {
    if (context->stackPointer > 0) {
        --context->stackPointer;
        VALIDATE_TYPE((stack + context->stackPointer)->type, U2);
        return (stack + context->stackPointer)->operand.u2val;
    } else {
        consout("Operand Stack underrun: %04x\n", context->stackPointer);
        jvmexit(1);
    }

    return 0;
}

jint PEEK_VERIFY_INT(contextDef* context, int OFFSET) {
    int __delta = context->framePointer + (OFFSET);
    VALIDATE_TYPE((stack + __delta)->type, JAVAINT);
    return (stack + __delta)->operand.jrenameint;
}

jobject PEEK_VERIFY_JREF(contextDef* context, int OFFSET) {
    int __delta = context->framePointer + (OFFSET);
    VALIDATE_TYPE((stack + __delta)->type, OBJECTREF);
    return (stack + __delta)->operand.jref;
}

jint operandStackPopJavaInt(contextDef* context) {
    jint value;

    value = POP_VERIFY_INT(context);

    return value;
}

jint operandStackPeekJavaInt(contextDef* context, int offset) {
    jint value;

    value = PEEK_VERIFY_INT(context, offset);

    return value;
}

jlong operandStackPeekJavaLong(contextDef* context, int offset) {
    jlong j;
    jint value;

    value = PEEK_VERIFY_INT(context, offset);
    j = value;
    j <<= 32;
    //	j = ((jlong) value) << 32;
    value = PEEK_VERIFY_INT(context, offset + 1);
    j = j | (value & 0xffffffff);

    return j;
}

jobject operandStackPeekObjectRef(contextDef* context, int offset) {
    jobject value;

    value = PEEK_VERIFY_JREF(context, offset);

    return value;
}

void operandStackPushJavaLong(contextDef* context, jlong j) {
    stackableOperand op;
    op.jrenameint = (jint) (j & 0xffffffff);
    push(context, op, JAVAINT);
    op.jrenameint = (jint) ((j >> 32) & 0xffffffff);
    push(context, op, JAVAINT);
}

jlong operandStackPopJavaLong(contextDef* context) {
    jlong j;
    jint value;

    value = POP_VERIFY_INT(context);
    j = value;
    j <<= 32;
    //	j = ((jlong) value) << 32;
    value = POP_VERIFY_INT(context);
    j = j | (value & 0xffffffff);

    return j;
}

jobject operandStackPopObjectRef(contextDef* context) {
    jobject value;

    value = POP_VERIFY_JREF(context);

    return value;
}

u2 operandStackPopU2(contextDef* context) {
    u2 value;

    value = POP_VERIFY_U2(context);

    return value;
}

void operandStackPushObjectRef(contextDef* context, jobject jref) {
    stackableOperand op;
    op.jref = jref;
    push(context, op, OBJECTREF);
}

void operandStackPushJavaInt(contextDef* context, jint jrenameint) {
    stackableOperand op;
    op.jrenameint = jrenameint;
    push(context, op, JAVAINT);
}

void operandStackPushU2(contextDef* context, u2 u2) {
    stackableOperand op;
    op.u2val = u2;
    push(context, op, U2);
}

void operandStackPushVariableJavaInt(contextDef* context, u1 varnum) {
    stackable op = stack[varnum + context->framePointer];

    VALIDATE_TYPE(op.type, JAVAINT);

    push(context, op.operand, JAVAINT);
}

void operandStackPushVariableJavaLong(contextDef* context, u1 varnum) {
    stackable op = stack[varnum + context->framePointer];

    VALIDATE_TYPE(op.type, JAVAINT);

    push(context, op.operand, JAVAINT);

    op = stack[varnum + 1 + context->framePointer];

    VALIDATE_TYPE(op.type, JAVAINT);

    push(context, op.operand, JAVAINT);
}

void operandStackPushVariableObjectRef(contextDef* context, u1 varnum) {
    stackable op = stack[varnum + context->framePointer];
    VALIDATE_TYPE(op.type, OBJECTREF);

    push(context, op.operand, OBJECTREF);
}

BOOL operandStackIsVariableObjectRefNull(contextDef* context, u1 varnum) {
    stackable op = stack[varnum + context->framePointer];

    VALIDATE_TYPE(op.type, OBJECTREF);

    return op.operand.jref == NULL ? TRUE : FALSE;
}

BOOL osIsObjectRefAtOffsetNull(contextDef* context, u2 offset) {
    stackable op = stack[context->stackPointer - offset];

    VALIDATE_TYPE(op.type, OBJECTREF);

    return op.operand.jref == NULL ? TRUE : FALSE;
}

void getOperandRelativeToStackPointer(contextDef* context, s1 offset, stackable* st) {
    *st = stack[context->stackPointer + offset];
}

void operandStackIncrementVariableJavaInt(contextDef* context, u1 varnum, jint delta) {
    stackType type = stack[varnum + context->framePointer].type;
    VALIDATE_TYPE(type, JAVAINT);

    stack[varnum + context->framePointer].operand.jrenameint += delta;
}

void operandStackPopVariableJavaInt(contextDef* context, u1 varnum) {
    jint jrenameint = operandStackPopJavaInt(context);
    stackable* dst = &stack[varnum + context->framePointer];
    dst->operand.jrenameint = jrenameint;
    dst->type = JAVAINT;
}

void operandStackPopVariableJavaLong(contextDef* context, u1 varnum) {
    jint ms = operandStackPopJavaInt(context);
    jint ls = operandStackPopJavaInt(context);
    stackable* dst = &stack[varnum + context->framePointer];
    dst->operand.jrenameint = ls;
    dst->type = JAVAINT;
    dst++;
    dst->operand.jrenameint = ms;
    dst->type = JAVAINT;
}

void operandStackPopVariableObjectRef(contextDef* context, u1 varnum) {
    jobject jref = operandStackPopObjectRef(context);
    stack[varnum + context->framePointer].operand.jref = jref;
    stack[varnum + context->framePointer].type = OBJECTREF;
}

const char* StackTypeToString(stackType type) {
    switch (type) {
        case JAVAINT:
            return "JAVAINT";
        case OBJECTREF:
            return "OBJECTREF";
        case U2:
            return "U2";
        default:
            return "(unknown)";
    }
}

void operandStackPop(contextDef* context, int count) {
    if (context->stackPointer > count) {
        context->stackPointer -= count;
    } else {
        consout("Operand Stack underrun: %04x\n", context->stackPointer - count);
        jvmexit(1);
    }
}

//void osDumpStack() {
//	u2 i;
//	for (i = 0; i < context->stackPointer; i++) {
//		stackable* se = &stack[i];
//
//		char* reg;
//		if (i == context->framePointer) {
//			reg = "FP";
//		} else if (i == context->contextPointer) {
//			reg = "CP";
//		} else {
//			reg = "  ";
//		}
//
//		consout("%s %04x: ", reg, i);
//		if (se->type == JAVAINT) {
//			consout("I %d\n", se->operand.jrenameint);
//		} else if (se->type == OBJECTREF) {
//			consout("O %d\n", se->operand.jref);
//		} else if (se->type == U2) {
//			consout("U %d\n", se->operand.u2val);
//		} else {
//			consout("? %d\n", se->operand.jrenameint);
//		}
//	}
//}


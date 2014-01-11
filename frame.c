/*
 * frame.c
 *
 *  Created on: Aug 22, 2010
 *      Author: hammer
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "heaplist.h"
#include "heap.h"
#include "frame.h"
#include "console.h"
#include "constantpool.h"
#include "instructions.h"
#include "vmids.h"
#include "thread.h"

//#define DEBUG_ENABLE
#include "debug.h"

/*
 * Frame layout and registers:
 * sp->
 * cp-> (same address as sp, or in other words sp == cp)
 *      flags (returnFromVM)
 *      prev cp
 *      prev ci
 *      retu addr
 *      prev fp
 *      local var 3
 *      local var 2
 *      local var 1
 *      local var 0
 *      arg
 *      arg
 * fp-> arg
 */
void pop_frame(contextDef* context) {
    // set stack pointer to fixed point at stack:
    context->stackPointer = context->contextPointer;

    // pop context including return address:
    context->returnFromVM = operandStackPopU2(context) != 0;
    context->contextPointer = operandStackPopU2(context);
    context->classIndex = operandStackPopU2(context);
    context->programCounter = operandStackPopU2(context); // This is the return
    u2 framePointer = operandStackPopU2(context);
    context->stackPointer = context->framePointer;
    context->framePointer = framePointer;
    // TODO: When swapping thread contexts the exceptionThrown shall also be pushed/popped
    // Don't touch context->exceptionThrown
}

void push_frame(contextDef* context, u1 localVariableCount, u2 dstClassIndex, codeIndex dest, BOOL returnFromVM) {
    __DEBUG("call: 0x%04x", dest);
    // Push previous frame pointer:
    operandStackPushU2(context, context->framePointer);
    // Push return address:
    operandStackPushU2(context, context->programCounter);
    // Push current class index:
    operandStackPushU2(context, context->classIndex);
    // Push current context pointer:
    operandStackPushU2(context, context->contextPointer);
    operandStackPushU2(context, returnFromVM ? 1 : 0);
    // Clear return - flag:
    context->returnFromVM = FALSE;
    // Save address of context:
    context->contextPointer = context->stackPointer;

    size_t numberOfPushedRegisters = 5;

    context->framePointer = context->stackPointer - localVariableCount - numberOfPushedRegisters;
    context->classIndex = dstClassIndex;
    context->programCounter = dest;
}

u1 getU1FromCode(contextDef* context) {
    u1 bytecode = code[context->programCounter];
    context->programCounter++;

    return bytecode;
}

u2 getU2FromCode(contextDef* context) {
    int msb = getU1FromCode(context) & 0xff;
    int lsb = getU1FromCode(context) & 0xff;

    return (u2) msb * 256 + (u2) lsb;
}

s1 getS1FromCode(contextDef* context) {
    return (s1) getU1FromCode(context);
}

s2 getS2FromCode(contextDef* context) {
    int msb = getS1FromCode(context) & 0xff;
    int lsb = getS1FromCode(context) & 0xff;

    return (s2) msb * 256 + (s2) lsb;
}

void dumpStackTrace(contextDef* context) {
    contextDef savedContext = *context;
    BOOL addComma = FALSE;

    // TODO Fix long problem
    int tid = (int) thGetCurrentThreadId(context);
    consout("context: TID=0x%04x PC=0x%04x  CID=0x%04x  SP=0x%04x\n", tid, context->programCounter,
            context->classIndex, context->stackPointer);
    consout("Internal trace:\n");
    exit(1);
    while (1) {
        if (addComma) {
            consout(",");
        } else {
            consout("  at: ");
            addComma = TRUE;
        }
        consout("%d", context->programCounter - 1);
        if (context->framePointer == 0) {
            break;
        }
        pop_frame(context);
    }
    *context = savedContext;
    consout("\n");
}





/* 
 * File:   nativethreads.c
 * Author: hej
 *
 * Created on December 27, 2013, 4:03 PM
 */

//#include <stdio.h>

#include "nativethreads.h"
#include "console.h"
#include "jarray.h"

#define DEBUG_ENABLE
#include "debug.h"


// During thread switching we are not able to use variables on a stack, so these are used as scratch variables:
static ntReg_t RSP;
static ntThreadContext_t* cThr;
static ntThreadContext_t* nThr;
//static size_t sCStackSize;
static void* stack;
static void (*nextFunc)(ntThreadContext_t*);

#ifdef DEBUG_ENABLE

/**
 * This function checks for stack growth and prints any increasing value to stdout
 * For debugging purpose only.
 * \param ntc The thread context to test
 */
static void sCheckStack(contextDef* context, ntThreadContext_t* ntc) {
    static size_t maxSize = 0;
    char* cp = jaGetArrayPayLoad(context, (jarray) nThr->stack);
    size_t i;
    size_t size = 0;

    for (i = 0; i < ntc->stackSize; i++) {
        if (cp[i] != 0) {
            size = ntc->stackSize - i;
            break;
        }
    }

    if (size > maxSize) {
        maxSize = size;
        consoutli("New max stack usage = %d/%d\n", maxSize, ntc->stackSize);
    }
}
#else
#define sCheckStack(X)
#endif

void ntYield(contextDef* context, ntThreadContext_t* currThr, ntThreadContext_t* nextThr) {
    cThr = currThr;
    nThr = nextThr;

    if (cThr != NULL) {
        sCheckStack(context, cThr);
        // Save registers:
        asm("pushq  %rbp");
        asm("movq   %rsp, RSP(%rip);");

        cThr->rsp = RSP;
    }

    if (nThr->func != NULL) {
        // Start new thread:
        stack = jaGetArrayPayLoad(context, (jarray) nThr->stack) + nThr->stackSize;
        asm("movq  stack(%rip), %rbp");
        asm("movq  %rbp, %rsp");
        nextFunc = nThr->func;
        nThr->func = NULL;
        nextFunc(nThr);
    } else {
        // Resume thread:
        RSP = nThr->rsp;

        asm("movq  RSP(%rip), %rsp;");
        asm("popq  %rbp");

        sCheckStack(context, nThr);
    }
}

void ntInitContext(ntThreadContext_t* ctx, jbyteArray stack, size_t stackSize, void (*func)(ntThreadContext_t*), u2 classId, codeIndex startAddress) {
    ctx->func = func;
    ctx->stack = stack;
    ctx->stackSize = stackSize;

    ctx->context.programCounter = startAddress;
    ctx->context.classIndex = classId;
    ctx->context.stackPointer = 1;
    ctx->context.framePointer = 0;
    ctx->context.contextPointer = 0;
    ctx->context.returnFromVM = FALSE;
    ctx->context.exceptionThrown = FALSE;
}

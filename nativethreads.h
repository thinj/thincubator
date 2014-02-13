/* 
 * File:   nativethreads.h
 * Author: hej
 *
 * Created on December 27, 2013, 4:04 PM
 */

#ifndef NATIVETHREADS_H
#define	NATIVETHREADS_H

#include "types.h"

#ifdef	__cplusplus
extern "C" {
#endif

    // The type able to hold a SP value:
    typedef long ntReg_t;

    // Thread context:

    typedef struct __ntThreadContext_t {
        // The native stack pointer. Only valid when thread is not running:
        ntReg_t rsp;

        // Size of the C stack:
        size_t stackSize;

        // Function pointer to the function to run. Only used when starting thread:
        void (*func)(struct __ntThreadContext_t*);

        // JVM Context:
        contextDef context;

        // Java stack:
        jbyteArray javaStack;

        // Pointer to the memory holding the C stack. Only used when starting thread:
        jbyteArray stack;

        // Pointer to encapsulating java object:
        jbyteArray javaSelf;

        // Simple thread collection; use a chained list:
        struct __ntThreadContext_t* next;
    } ntThreadContext_t;


    void ntYield(contextDef* context, ntThreadContext_t* currThr, ntThreadContext_t* nextThr);
    /**
     * this function initializes a native thread context
     * 
     * \param stack Pointer to the memory holding the stack. Only used when starting thread
     * \param stackSize Size of the stack
     * \param func Function pointer to the function to run. Set to NULL after start
     */
    void ntInitContext(ntThreadContext_t* ctx, size_t stackSize, void (*func)(ntThreadContext_t*), u2 classId, codeIndex startAddress);
#ifdef	__cplusplus
}
#endif

#endif	/* NATIVETHREADS_H */


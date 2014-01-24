/*
 * thread.h
 *
 *  Created on: Mar 8, 2013
 *      Author: hammer
 */

#ifndef THREAD_H_
#define THREAD_H_

#include "types.h"
#include "nativethreads.h"

// Iteration type:
typedef struct {
    // Thread:
    jobject thread;
    
    // Pointer to stack
    stackable* stack;
    
    // The current stack pointer in the stack:
    u2 stackPointer;
} thStackInfo_t;

/**
 * \return true if scheduling is enabled
 */
BOOL thIsSchedulingEnabled(void);

/**
 * This function attaches the thread to the kernel at sets its state to StateRunnable
 * @param 
 * \param this The thread to attach to kernel. If its the first thread, it is assumed
 * to be the main thread
 */

void thAttach(contextDef* context, jobject this);

/**
 * This function reschedules.
 */
void thYield(contextDef* context);

/**
 * This method Locks the monitor. Follows the semantics of the JVM instruction of same name.
 *
 * \throws NullPointerException if arg == null
 * \throws IllegalMonitorStateException if monitor is in illegal state
 */
void thMonitorEnter(contextDef* context, jobject objectToLock);

/**
 * This method unlocks the monitor. Follows the semantics of the JVM instruction of same name.
 *
 * \throws NullPointerException if arg == null
 * \throws IllegalMonitorStateException if monitor is in illegal state
 */
void thMonitorExit(contextDef* context, jobject objectToLock);

/**
 * This function notifies one or all waiting threads from the wait set on the locked object.
 *
 * \param lockedObject The object to notify on
 * \param notifyAll true, if all threads shall be notified; false if only a single thread shall be notified
 * \throws IllegalMonitorStateException If the lockedObject is not owned by calling thread,
 */
void thNotify(contextDef* context, jobject lockedObject, BOOL notifyAll);

/**
 * This function waits for a notification on an object
 *
 * \param lockedObject The object to wait on
 * \throws IllegalMonitorStateException If the lockedObject is not owned by calling thread,
 */
void thWait(contextDef* context, jobject lockedObject);

/**
 * This function puts the current thread into sleep state for at least the specified time
 *
 * \param millis The number of milli seconds to sleep
 * \throws InterruptedException If interrupted
 */
void thSleep(contextDef* context, jlong millis);

/**
 * This function interrupts the thread defined by 'thread'
 *
 * \param thread The thread to interrupt
 */
void thInterrupt(contextDef* context, jobject thread);


/**
 * Simple count based yield method
 */
void thTryYield(contextDef* context);

/**
 * This function adds the thread to the collection of all threads
 *
 * \param threadCls the java.lang.Thread class
 * \param thread The thread to add
 */
void thAddToThreadCollection(contextDef* context, jclass threadCls, jobject thread);

/**
 * This method returns the thread id of the current thread.
 * Note that unpredictable results might occur if called before Thread class has been class loaded.
 */
jlong thGetCurrentThreadId(contextDef* context);

/**
 * This method returns the native thread context for the main thread.
 *
 * \return The native thread context for the main thread.
 */
jobject thGetMainNativeContext(void);

/**
 * This method returns the native stack for the main thread.
 *
 * \return The native stack for the main thread.
 */
jobject thGetMainNativeStack(void);

/**
 * This function starts the VM and will never return.
 * \param heap A pointer to the memory area where the heap will be placed
 * \param heapSize The size of the heap area (in chunks of align_t)
 * \param javaStackSize The size of the java stack (in bytes)
 * \param cStackSize The size of the c stack (in bytes)
 */
void thStartVM(align_t* heap, size_t heapSize, size_t javaStackSize, size_t cStackSize);

/**
 * This function allocates a native stack for the calling thread.
 * \return The native stack as a java object (byte[])
 */
jobject thAllocNativeStack(void);

/**
 * This function returns the current stack pointer value for the indicated thread
 * \param thread The thread to get a Java Stack pointer value from
 * \return  The stack pointer value
 */
int thGetJavaStackPointer(jobject thread);

/**
 * This function returns a pointer to thread context for the main thread
 * \return A pointer to native thread context for the main thread
 */
ntThreadContext_t* thGetJavaMainContext(void);


/**
 * This function shall be used for iteration over the stack info for all threads.
 * 
 * \param current If the 'thread' field is set to NULL prior to this call the first thread
 * is read; if set to a non-NULL value it is assumed to point at the last thread read, 
 * and the returned value will be the next thread and its stack info
 * \return The stack info for the requested thread
 */
thStackInfo_t* nextStackInfo(thStackInfo_t* current);

/**
 * This function shall exit the vm
 */
void thinjvm_exit(int exit_code);


#endif /* THREAD_H_ */

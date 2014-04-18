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
 * \param timeout The timeout in millis. If == 0; wait until notified == never timeout.
 * \throws IllegalMonitorStateException If the lockedObject is not owned by calling thread,
 */
void thWait(contextDef* context, jobject lockedObject, jlong timeout);

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
 * This function returns current thread
 * \param context
 * \return The current thread
 */
jobject thGetCurrentThread(contextDef* context);

/**
 * Simple count based yield method
 */
void thTryYield(contextDef* context);

/**
 * This method returns the thread id of the current thread.
 * Note that unpredictable results might occur if called before Thread class has been class loaded.
 */
jlong thGetCurrentThreadId(contextDef* context);

/**
 * This function starts the VM and will never return.
 * \param heap A pointer to the memory area where the heap will be placed
 * \param heapSize The size of the heap area (in chunks of align_t)
 * \param javaStackSize The size of the java stack (in bytes)
 * \param cStackSize The size of the c stack (in bytes)
 * \return 0 if succ
 */
int thStartVM(align_t* heap, size_t heapSize, size_t javaStackSize, size_t cStackSize);


typedef void (*thCallback_t)(contextDef* context, stackable* memory, size_t size);

/**
 * This function iterates over all threads and for each thread the callback is called.
 * The context
 * \param context The context supplied by the caller of this method. Will be used 
 * as 1st argument to callback function
 * @param callback The callback function pointer
 */
void thForeachThread(contextDef* context, thCallback_t callback);

/**
 * This function shall exit the vm
 */
void thinjvm_exit(int exit_code);


#endif /* THREAD_H_ */

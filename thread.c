/*
 * thread.c
 *
 *  Created on: Mar 8, 2013
 *      Author: hammer
 */

#include <string.h>

#include "types.h"
#include "thread.h"
#include "constantpool.h"
#include "vmids.h"
#include "jni.h"
#include "exceptions.h"
#include "vmtime.h"
#include "nativethreads.h"
#include "jarray.h"
#include "instructions.h"

#define DEBUG_ENABLE
#include "debug.h"
#include "alltests.h"

// Collection of all threads:
static ntThreadContext_t* sAllThreads = NULL;
ntThreadContext_t* thCurrentThread = NULL;

// Native Thread context for the 'thread' calling the 'main' method of this executable:
static ntThreadContext_t sCMainContext;

// Native thread context for the main thread:
static ntThreadContext_t* sJavaMainContext;

BOOL nissehat = FALSE;

// The size of a Native C stack:
static size_t sCStackSize;

// The native C stack for the java main thread:
static jbyteArray sMainThreadNativeStack;

// The java stack for the main thread:
static jbyteArray sMainThreadJavaStack;

/**
 *  Flag for enabling scheduling.
 *  Shall be set true when java.lang.Thread has changed the way stacks and context shall be accessed.
 */
static BOOL aSchedulingEnabled = FALSE;

static void sDumpContext(contextDef* context, char* prefix) {
    jobject thr = GetStaticObjectField(context, getJavaLangClass(context, C_java_lang_Thread), A_java_lang_Thread_aCurrentThread);
    consoutli("%s"
            "pc = 0x%04x "
            "sp = 0x%04x "
            "clid = %03d "

            "cp = 0x%04x "
            "ET = %s "
            "fp = 0x%04x "
            "RV = %s "

            "thr = %p\n",

            prefix,
            context->programCounter,
            context->stackPointer,
            context->classIndex,

            context->contextPointer,
            context->exceptionThrown ? "T" : "F",
            context->framePointer,
            context->returnFromVM ? "T" : "F",

            thr);
}


///**
// * This function returns the context of a thread. It is the callers responsibility to
// * ensure that the thread != NULL
// * 
// * \param thread The thread to get a context from 
// * \return The execution context of the thread
// */
//static ntThreadContext_t* sGetContext(contextDef* context, jobject thread) {
//    jobject contextObject = GetObjectField(context, thread, A_java_lang_Thread_aNativeContext);
//    return (ntThreadContext_t*) jaGetArrayPayLoad(contextObject);
//}

/**
 * \return true if scheduling is enabled
 */
BOOL thIsSchedulingEnabled() {
    return aSchedulingEnabled;
}

/**
 * This method enables scheduling.
 */
void thStartScheduling() {
    aSchedulingEnabled = TRUE;
}

//static void sDumpAllThreads() {
//	jclass threadClass = getJavaLangClass(C_java_lang_Thread);
//	jobject stackThread = GetStaticObjectField(threadClass, A_java_lang_Thread_aAllThreads);
//	__DEBUG("A_java_lang_Thread_aAllThreads: %p", stackThread);
//
//	while (stackThread != NULL ) {
//		__DEBUG("    thread: %p", stackThread);
//		// Next thread:
//		stackThread = GetObjectField(stackThread, A_java_lang_Thread_aNextThread);
//	}
//}


// TODO Make time-based!
static int aYieldCount = 0;

void thTryYield(contextDef* context) {
    if (aSchedulingEnabled) {
        if (aYieldCount++ >= 100) {
            aYieldCount = 0;
            __DEBUG("*** Before calling yield... pc = 0x%04x, sp = 0x%04x, cp = 0x%04x, stack = %p",
                    context->programCounter, context->stackPointer, context->contextPointer, getStack());
            if (nissehat) consoutli("*** Before calling yield... pc = 0x%04x, sp = 0x%04x, cp = 0x%04x, stack = %p\n",
                    context->programCounter, context->stackPointer, context->contextPointer, getStack());
            thYield(context);
            __DEBUG("*** After calling yield... pc = 0x%04x, sp = 0x%04x, cp = 0x%04x, stack = %p",
                    context->programCounter, context->stackPointer, context->contextPointer, getStack());
            if (nissehat) consoutli("*** After calling yield... pc = 0x%04x, sp = 0x%04x, cp = 0x%04x, stack = %p\n",
                    context->programCounter, context->stackPointer, context->contextPointer, getStack());
        }
    }
}

jlong thGetCurrentThreadId(contextDef* context) {
    jobject currentThread = thCurrentThread->javaThread;
    return GetLongField(context, currentThread, A_java_lang_Thread_aId);
}

static ntThreadContext_t* sGetNextThread(ntThreadContext_t* curr) {
    ntThreadContext_t* next = curr->next;
    if (next == NULL) {
        // Start all over:
        next = sAllThreads->next;
    }

    return next;
}

static void sRemoveThread(ntThreadContext_t* curr) {
    ntThreadContext_t* prev = NULL;
    ntThreadContext_t* t = sAllThreads;

    while (t != curr && t != NULL) {
        prev = t;
        t = t->next;
    }

    if (prev == NULL) {
        // Didn't enter the loop at all:
        if (t != NULL) {
            // It's the first thread and there is at least a single thread left:
            sAllThreads = t->next;
        }
    } else {
        ntThreadContext_t* next;
        if (t != NULL) {
            next = t->next;
        } else {
            // It is the last element:
            next = NULL;
        }
        prev->next = next;
    }
}

void thYield(contextDef* context) {
    jclass cls = getJavaLangClass(context, C_java_lang_Thread);
    //sDumpContext(context, "thYield before: ");
    __DEBUG("before: pc = 0x%04x, sp = 0x%04x", context->programCounter, context->stackPointer);

    if (nissehat) consoutli("foobar %p\n", context);
    consoutli("foobar\n");
    ////////////////////////////////////////////////////////////////////////////
    // Find next runnable thread:
    ////////////////////////////////////////////////////////////////////////////
    ntThreadContext_t* nextThread = thCurrentThread;
    BOOL interrupted = FALSE;
    while ((nextThread = sGetNextThread(nextThread)) != NULL) {
        int state = GetIntField(context, nextThread->javaThread, A_java_lang_Thread_aState);
        if (state == StateTerminated) {
            // TODO Use enum value in stead!
            // Thread is terminated, get rid of it:
            sRemoveThread(nextThread);

            // Start all over searching for a runnable thread:
            nextThread = sAllThreads;
        } else if (state == StateTimedWaiting) {
            jobject javaThread = nextThread->javaThread;
            jlong when = GetLongField(context, nextThread->javaThread, A_java_lang_Thread_aWakeUpAt);
            if (when < vtCurrentTimeMillis()) {
                SetIntField(context, javaThread, A_java_lang_Thread_aState, StateRunnable);
                break;
            } else {
                // Check for interrupt:
                if (GetBooleanField(context, javaThread, A_java_lang_Thread_aInterrupted)) {
                    SetIntField(context, javaThread, A_java_lang_Thread_aState, StateRunnable);
                    interrupted = TRUE;
                    break;
                }
                // else: Not interrupted, stay asleep...
            }
        } else if (state == StateRunnable) {
            break;
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // Switch to next thread:
    ////////////////////////////////////////////////////////////////////////////
    if (nextThread != NULL) {
        SetStaticObjectField(context, cls, A_java_lang_Thread_aCurrentThread, nextThread->javaThread);

        // Let the other thread run:
        if (thCurrentThread != nextThread) {
            __DEBUG("nt yield %p %p\n", thCurrentThread, nextThread);
            if (nissehat) consoutli("nt yield %p %p\n", &nextThread->context, &nextThread->context);
            // Set stack to point at next Thread's stack:
            osSetStack(context, nextThread->javaStack);

            ntThreadContext_t* currNtc = thCurrentThread;
            thCurrentThread = nextThread;
            ntYield(context, currNtc, nextThread);
        }
        if (interrupted) {
            consoutli("interrupted: not impl!!!\n");
            jvmexit(7435);
            // throwInterruptedException();
        }
    } else {
        // All threads are terminated:
        jvmexit(0);
    }

    __DEBUG("after");
    //sDumpContext(context, "thYield after : ");
    if (nissehat) consoutli("foobar %p\n", context);
}

/**
 * This function (re-) competes for a lock on an object. Before calling this method the object to lock and then
 * the lockCount shall be set in the thread object. It is the responsibility of the caller to ensure
 * that the object to lock has a monitor != NULL.
 * \param thread The competing thread
 * \return TRUE, if the lock was granted to the thread; FALSE if the thread is blocked and should wait
 */
BOOL sTryWaitForLock(contextDef* context, jobject thread) {
    consoutli("sTryWait enter:%p\n", context);
    BOOL lockGranted = TRUE;

    consoutli("sTryWait enter:%p\n", context);
    jint threadLockCount = GetIntField(context, thread, A_java_lang_Thread_aLockCount);
    consoutli("sTryWait enter:%p\n", context);
    jobject objectToLock = GetObjectField(context, thread, A_java_lang_Thread_aBlockingObject);
    if (objectToLock == NULL) {
        // ups! obkectToLock er null ????!!!!
        consoutli("sTryWait enter:%p %p\n", context, objectToLock);
        jvmexit(56);
    }
    jobject monitor = GetObjectField(context, objectToLock, A_java_lang_Object_aMonitor);

    consoutli("sTryWait enter:%p\n", context);
    // If monitorLockCount == 0 then the monitor isn't locked:
    jint monitorLockCount = GetIntField(context, monitor, A_java_lang_Object_Monitor_aLockCount);
    if (monitorLockCount == 0) {
        consoutli("sTryWait enter:%p\n", context);
        // thread is not owner and the monitor isn't locked, re-use monitor and set the thread as owner:
        SetObjectField(context, monitor, A_java_lang_Object_Monitor_aOwner, thread);
        SetIntField(context, monitor, A_java_lang_Object_Monitor_aLockCount, threadLockCount);
        // thread is not blocking:
        SetObjectField(context, thread, A_java_lang_Thread_aBlockingObject, NULL);
        __DEBUG("%p got lock %p\n", thread, objectToLock);
        SetIntField(context, thread, A_java_lang_Thread_aState, StateRunnable);
        consoutli("sTryWait enter:%p\n", context);
    } else {
        consoutli("sTryWait enter:%p\n", context);
        jobject lockOwnerThread = GetObjectField(context, monitor, A_java_lang_Object_Monitor_aOwner);
        if (lockOwnerThread == thread) {
            consoutli("sTryWait enter:%p\n", context);
            // Current thread is already owner; increment lock counter:
            threadLockCount += monitorLockCount;
            SetIntField(context, monitor, A_java_lang_Object_Monitor_aLockCount, threadLockCount);
        } else {
            consoutli("sTryWait enter:%p\n", context);
            // Change state on thread to BLOCKED on object:
            SetObjectField(context, thread, A_java_lang_Thread_aBlockingObject, objectToLock);
            SetIntField(context, thread, A_java_lang_Thread_aState, StateBlocked);
            SetIntField(context, thread, A_java_lang_Thread_aLockCount, threadLockCount);

            lockGranted = FALSE;
        }
        consoutli("sTryWait enter:%p\n", context);
    }

    consoutli("sTryWait leave:%p\n", context);
    return lockGranted;
}

/**
 * This function (re-) competes for a lock on an object. Before calling this method the object to lock and then
 * the lockCount shall be set in the thread object. It is the responsibility of the caller to ensure
 * that the object to lock has a monitor != NULL.
 * This method blocks until the lock has been granted
 * \param thread The competing thread
 */
void sWaitForLock(contextDef* context, jobject thread) {
    while (!sTryWaitForLock(context, thread)) {
        thYield(context);
    }
}

/**
 * This function unlocks a monitor. It is the callers responsibility to ensure that current thread owns the
 * monitor
 * \param monitor The monitor to unlock
 * \param lockedObject The object encapsulating the monitor
 * \param unlockCount The number of unlocks to perform
 * \throws IllegalMonitorStateException if the monitors lockCount < unlockCount
 */
static void sUnlockOwnedMonitor(contextDef* context, jobject monitor, jobject lockedObject, jint unlockCount) {
    // Unlock monitor:
    jint lockCount = GetIntField(context, monitor, A_java_lang_Object_Monitor_aLockCount);
    if (lockCount >= unlockCount) {
        lockCount -= unlockCount;
        SetIntField(context, monitor, A_java_lang_Object_Monitor_aLockCount, lockCount);
        if (lockCount == 0) {
            // No longer locking the monitor, test if some other thread is blocking on the monitor:
            ntThreadContext_t* thread = sAllThreads;
            //            jclass threadClass = getJavaLangClass(C_java_lang_Thread);
            //            jobject thread = GetStaticObjectField(threadClass, A_java_lang_Thread_aAllThreads);
            while (thread != NULL) {
                jint state = GetIntField(context, thread->javaThread, A_java_lang_Thread_aState);
                if (state == StateBlocked) {
                    jobject blockingObject = GetObjectField(context, thread->javaThread, A_java_lang_Thread_aBlockingObject);
                    if (blockingObject == lockedObject) {
                        if (sTryWaitForLock(context, thread->javaThread)) {
                            // Got the lock; no yield here, let the unlocking thread continue
                            break;
                        }
                    }
                }
                thread = thread->next;
            }
        }
    } else {
        __DEBUG("throwIllegalMonitorStateException");
        throwIllegalMonitorStateException("Monitor is not locked");
    }
}

void thMonitorEnter(contextDef* context, jobject objectToLock) {
    __DEBUG("begin %p %04x\n",
            GetStaticObjectField(context, getJavaLangClass(context, C_java_lang_Thread), A_java_lang_Thread_aCurrentThread),
            context->programCounter);

    if (objectToLock != NULL) {
        jobject monitor = GetObjectField(context, objectToLock, A_java_lang_Object_aMonitor);
        jobject currentThread = GetStaticObjectField(context, getJavaLangClass(context, C_java_lang_Thread),
                A_java_lang_Thread_aCurrentThread);

        if (monitor == NULL) {
            // Lazy monitor allocation:
            monitor = AllocObject(context, getJavaLangClass(context, C_java_lang_Object_Monitor));
            if (monitor != NULL) {
                SetObjectField(context, objectToLock, A_java_lang_Object_aMonitor, monitor);
                // Monitor is now all set - and not locked.
            }
            // else: OutOfMem has been thrown
        }

        if (monitor != NULL) {
            SetObjectField(context, currentThread, A_java_lang_Thread_aBlockingObject, objectToLock);
            SetIntField(context, currentThread, A_java_lang_Thread_aLockCount, 1);

            // Monitor is already owned by somebody unless lockCount == 0
            sWaitForLock(context, currentThread);
        }
    } else {
        // Can't lock on null:
        __DEBUG("throw NPE");
        throwNullPointerException(context);
    }
    __DEBUG("end %04x\n", context->programCounter);
}

void thMonitorExit(contextDef* context, jobject objectToLock) {
    consoutli("thMonitorExit enter %p\n", context);
    if (objectToLock != NULL) {
        jobject monitor = GetObjectField(context, objectToLock, A_java_lang_Object_aMonitor);
        jobject currentThread = GetStaticObjectField(context, getJavaLangClass(context, C_java_lang_Thread),
                A_java_lang_Thread_aCurrentThread);
        jobject lockOwnerThread = GetObjectField(context, monitor, A_java_lang_Object_Monitor_aOwner);

        if (lockOwnerThread == currentThread) {
            // Unlock monitor:
            sUnlockOwnedMonitor(context, monitor, objectToLock, 1);
        } else {
            __DEBUG("throwIllegalMonitorStateException");
            throwIllegalMonitorStateException("Thread not owner");
        }
    } else {
        // Can't lock on null:
        __DEBUG("throwNullPointerException");
        throwNullPointerException(context);
    }
    consoutli("thMonitorExit leave %p\n", context);
    nissehat = TRUE;
}

/**
 * This function notifies one or all threads waiting on a mutex
 *
 * \param lockedObject The mutex
 * \param notifyAll true, if all threads shall be notified; false if only a single thread shall be notified
 */
void thNotify(contextDef* context, jobject lockedObject, BOOL notifyAll) {
    __DEBUG("%p thNotify begin %04x\n",
            GetStaticObjectField(context, getJavaLangClass(context, C_java_lang_Thread), A_java_lang_Thread_aCurrentThread),
            context->programCounter);
    if (lockedObject != NULL) {
        jobject monitor = GetObjectField(context, lockedObject, A_java_lang_Object_aMonitor);
        if (monitor != NULL) {
            jobject currentThread = GetStaticObjectField(context, getJavaLangClass(context, C_java_lang_Thread),
                    A_java_lang_Thread_aCurrentThread);
            jobject lockOwnerThread = GetObjectField(context, monitor, A_java_lang_Object_Monitor_aOwner);
            if (currentThread == lockOwnerThread) {
                // Build a list of threads that changes from WAITING to RUNNABLE:
                jobject runnableList = NULL;

                // Find all threads that are in Wait Set WAITING on the lockedObject:
                jobject waitElement = GetObjectField(context, monitor, A_java_lang_Object_Monitor_aWaitSet);

                jobject previousWaitElement = NULL;
                while (waitElement != NULL) {
                    jobject waitingThread = GetObjectField(context, waitElement, A_java_lang_Object_WaitElement_aWaiting);
                    if (waitingThread == NULL) {
                        // This should never happen, but handle it gracefully:
                        __DEBUG("throwNullPointerException");
                        throwNullPointerException(context);
                        break;
                    }

                    jobject runnableElement = NULL;
                    BOOL removeFromWaitSet = TRUE;
                    int threadState = GetIntField(context, waitingThread, A_java_lang_Thread_aState);
                    if (threadState == StateWaiting) {
                        SetIntField(context, waitingThread, A_java_lang_Thread_aState, StateRunnable);
                        // Add thread to list of RUNNABLE threads (and remove from wait set):
                        runnableElement = waitElement;
                    } else if (threadState == StateTerminated) {
                        // Remove from wait set
                    } else {
                        // Arggh, mismatch between wait set and thread state?
                        consoutli("Unexpected state: %d\n", threadState);
                        removeFromWaitSet = FALSE;
                    }

                    // Save a copy of the next - field:
                    jobject next = GetObjectField(context, waitElement, A_java_lang_Object_WaitElement_aNext);

                    // Build the list of threads that have changed state to RUNNABLE:
                    if (runnableElement != NULL) {
                        SetObjectField(context, runnableElement, A_java_lang_Object_WaitElement_aNext, runnableList);
                        runnableList = runnableElement;
                    }
                    if (removeFromWaitSet) {
                        if (previousWaitElement == NULL) {
                            // It's the first element:
                            SetObjectField(context, monitor, A_java_lang_Object_Monitor_aWaitSet, next);
                        } else {
                            SetObjectField(context, previousWaitElement, A_java_lang_Object_WaitElement_aNext, next);
                        }
                    } else {
                        previousWaitElement = waitElement;
                    }
                    waitElement = next;

                    // notifyAll / notify ?
                    if (!notifyAll) {
                        if (runnableElement != NULL) {
                            break;
                        }
                    }
                }
                // The runnableList now contains all threads that have changed state to RUNNABLE:
                // All threads in the runnableList should try to enter the monitor again:
                for (waitElement = runnableList; waitElement != NULL;
                        waitElement = GetObjectField(context, waitElement, A_java_lang_Object_WaitElement_aNext)) {
                    jobject thread = GetObjectField(context, waitElement, A_java_lang_Object_WaitElement_aWaiting);
                    // The object to lock and the lock count is already (shall be!) on the stack:
                    sWaitForLock(context, thread);
                }
                // The thread T is then removed from the wait set and re-enabled for
                // thread scheduling. It then locks the object again (which may involve
                // competing in the usual manner with other threads); once it has gained
                // control of the lock, it performs N - 1 additional lock operations on
                // that same object and then returns from the invocation of the wait method.
                // Thus, on return from the wait method, the state of the object's lock is
                // exactly as it was when the wait method was invoked.
            } else {
                __DEBUG("throwIllegalMonitorStateException");
                throwIllegalMonitorStateException("Thread not owner");
            }
        } else {
            __DEBUG("throwIllegalMonitorStateException");
            throwIllegalMonitorStateException("Monitor is not locked");
        }
    } else {
        // Can't notify on null:
        __DEBUG("throwNullPointerException");
        throwNullPointerException(context);
    }

    __DEBUG("end %04x\n", context->programCounter);
}

void thWait(contextDef* context, jobject lockedObject) {
    __DEBUG("Begin PC = %04x\n", context->programCounter);
    sDumpContext(context, "thWait  begin:  ");
    if (lockedObject != NULL) {
        jobject monitor = GetObjectField(context, lockedObject, A_java_lang_Object_aMonitor);
        if (monitor != NULL) {
            jobject currentThread = GetStaticObjectField(context, getJavaLangClass(context, C_java_lang_Thread),
                    A_java_lang_Thread_aCurrentThread);
            jobject lockOwnerThread = GetObjectField(context, monitor, A_java_lang_Object_Monitor_aOwner);
            if (currentThread == lockOwnerThread) {
                /*
                 * Add current thread to (end of) wait set for locked object:
                 */
                jobject waitElement = GetObjectField(context, monitor, A_java_lang_Object_Monitor_aWaitSet);
                jobject previos = waitElement;
                while (waitElement != NULL) {
                    previos = waitElement;
                    waitElement = GetObjectField(context, waitElement, A_java_lang_Object_WaitElement_aNext);
                }

                jobject element = AllocObject(context, getJavaLangClass(context, C_java_lang_Object_WaitElement));
                if (element != NULL) {
                    SetObjectField(context, element, A_java_lang_Object_WaitElement_aWaiting, currentThread);
                    if (previos == NULL) {
                        // Element is the one and only waiting thread:
                        SetObjectField(context, monitor, A_java_lang_Object_Monitor_aWaitSet, element);
                    } else {
                        // There are more waiting threads:
                        SetObjectField(context, previos, A_java_lang_Object_WaitElement_aNext, element);
                    }

                    /*
                     * Set thread state to WAITING:
                     */
                    SetIntField(context, currentThread, A_java_lang_Thread_aState, StateWaiting);

                    /*
                     * Unlock locked object as many times as current thread has locked it (and save lock count):
                     */
                    jint lockCount = GetIntField(context, monitor, A_java_lang_Object_Monitor_aLockCount);
                    SetIntField(context, currentThread, A_java_lang_Thread_aLockCount, lockCount);
                    SetObjectField(context, currentThread, A_java_lang_Thread_aBlockingObject, lockedObject);

                    // Relinquish all locks:
                    sUnlockOwnedMonitor(context, monitor, lockedObject, lockCount);

                    // Find another thread to run:
                    thYield(context);
                }
                // else: Out Of Mem has been thrown
            } else {
                __DEBUG("throwIllegalMonitorStateException");
                throwIllegalMonitorStateException("Thread not owner");
            }
        } else {
            __DEBUG("throwIllegalMonitorStateException");
            throwIllegalMonitorStateException("Monitor is not locked");
        }
    } else {
        // Can't notify on null:
        __DEBUG("throwNullPointerException");
        throwNullPointerException(context);
    }

    __DEBUG("ended\n");
    sDumpContext(context, "thWait  after : ");
    context->stackPointer += 5;
    context->contextPointer += 6;
    context->framePointer += 10;
    sDumpContext(context, "thWait  after : ");
}

void thSleep(contextDef* context, jlong millis) {
    jobject currentThread = GetStaticObjectField(context, getJavaLangClass(context, C_java_lang_Thread),
            A_java_lang_Thread_aCurrentThread);

    // Fall asleep:
    jlong wakeup = vtCurrentTimeMillis() + millis;
    SetIntField(context, currentThread, A_java_lang_Thread_aState, StateTimedWaiting);
    SetLongField(context, currentThread, A_java_lang_Thread_aWakeUpAt, wakeup);

    // Switch thread:
    thYield(context);
}

jbyteArray sAllocStack(contextDef* context, jobject thread) {
    // Allocate an object without protecting it:
    jbyteArray stackObject = osAllocateStack(context);

    if (stackObject != NULL) {
        // Avoid garbage collection of our one and only stack (at this point).
        // The stack shall be unprotected when the stack has been referenced from the
        // first thread, see osUnprotectStack():
        heapProtect((jobject) stackObject, TRUE);

        if (thread != NULL) {
            // Prepare the stack so it contains 'this' as its first element:
            stackable* stack = jaGetArrayPayLoad(context, (jarray) stackObject);

            // Push 'this':
            stack[0].operand.jref = thread;
            stack[0].type = OBJECTREF;
        }
    }
    // else: An out-of-mem exception has been thrown

    return stackObject;
}

/**
 * Definition of function for starting the run-method
 */
static void sRunFunction(ntThreadContext_t* ntc) {
    consoutli("Creating thread: %p\n", &ntc->context);
    inExecute(&ntc->context);

    // Unreachable; runFromNative will never return to this point
}

/**
 * This function allocates a native thread context for the calling thread.
 * \param context Java context
 * \param this The thread instance
 * \param stack The native stack
 * \return The native thread context as a java object (byte[])
 */
ntThreadContext_t* thAllocNativeContext(contextDef* context, jobject this, jobject stack) {
    jbyteArray ntContextObject = NewByteArray(context, sizeof (ntThreadContext_t));
    ntThreadContext_t* nt = NULL;
    if (ntContextObject != NULL) {
        heapProtect((jobject) ntContextObject, TRUE);
        nt = (ntThreadContext_t*) jaGetArrayPayLoad(context, (jarray) ntContextObject);
        const methodInClass* mic = getVirtualMethodEntryByLinkId(this, M_java_lang_Thread_runFromNative);

        ntInitContext(nt, (jbyteArray) stack, sCStackSize, sRunFunction, C_java_lang_Thread, mic->codeOffset);
        nt->javaSelf = ntContextObject;
    }
    // else: Out of mem has been thrown

    return nt;
}

void thAddToThreadCollection(contextDef* context, jclass threadCls, jobject thread) {
    __DEBUG("**** BEGIN adding: %p", thread);
    jbyteArray javaStack = NULL;
    jbyteArray nativeStack = NULL;
    ntThreadContext_t* nativeContext = NULL;

    if (sAllThreads == NULL) {
        // Use Main thread context
        javaStack = sMainThreadJavaStack;
        nativeStack = sMainThreadNativeStack;
        nativeContext = sJavaMainContext;
    } else {
        // Allocate and prepare the java stack for this thread:
        javaStack = sAllocStack(context, thread);
        if (javaStack != NULL) {
            // Allocate and prepare the native C stack for this thread:
            nativeStack = NewByteArray(context, sCStackSize);
        }
        if (nativeStack != NULL) {
            // Allocate and prepare a native thread context:
            nativeContext = thAllocNativeContext(context, thread, (jobject) nativeStack);
        }
    }

    if (nativeContext != NULL) {
        nativeContext->next = sAllThreads;
        nativeContext->javaStack = javaStack;

        sAllThreads = nativeContext;
    }
    // else: Out of mem thrown

    //sDumpAllThreads();
    __DEBUG("**** END adding: %p", thread);
}

void thInterrupt(contextDef* context, jobject thread) {
    // How difficult can it be ?!
    SetBooleanField(context, thread, A_java_lang_Thread_aInterrupted, TRUE);
}



//jobject thGetMainNativeContext(void) {
//    return (jobject) sJavaMainContextObject;
//}

///**
// * This function returns a pointer to native thread context for the main thread
// * \return A pointer to native thread context for the main thread
// */
//ntThreadContext_t* thGetJavaMainContext(void) {
//    return (ntThreadContext_t*) jaGetArrayPayLoad((jarray) sJavaMainContextObject);
//}

/**
 * Definition of main function for VM. Has to be compatible with the native scheduling mechanism
 */
static void sMainFunction(ntThreadContext_t* ntc) {
    // First thread running:
    sAllThreads = ntc;

    // initialize java stack:
    sMainThreadJavaStack = sAllocStack(&ntc->context, NULL);

    if (sMainThreadJavaStack == NULL) {
        consoutli("Premature out of memory; can't alloc a stack\n");
        jvmexit(1);
    }

    // Set as current stack:
    osSetStack(&ntc->context, sMainThreadJavaStack);

    // Load all java.lang.Class instances:
    cpGenerateJavaLangClassInstances(&ntc->context);

    push_frame(&ntc->context, 0, ntc->context.classIndex, ntc->context.programCounter, TRUE);

    inExecute(&ntc->context);

    // Return to C Main thread:
    ntYield(&ntc->context, sJavaMainContext, &sCMainContext);
}

void thStartVM(align_t* heap, size_t heapSize, size_t javaStackSize, size_t cStackSize) {
    // Initialize java heap:
    heapInit(heap, heapSize);

    // Record the size of a C stack:
    sCStackSize = cStackSize;

    // Record the size of a java stack:
    osSetJavaStackSize(javaStackSize);

    // Clear static area:
    memset(&staticMemory[0], staticMemorySize, sizeof (stackable));

    // Turn this 'thread' into C Main thread. The C Main thread shall remain 
    // passive until termination of VM:
    memset(&sCMainContext, 0, sizeof (sCMainContext));
    sCMainContext.func = NULL;

    jbyteArray jmc = NewByteArray(&sCMainContext.context, sizeof (ntThreadContext_t));

    if (jmc == NULL) {
        consoutli("Premature out of memory; can't alloc thread context for main thread\n");
        jvmexit(1);
    }

    sJavaMainContext = (ntThreadContext_t*) jaGetArrayPayLoad(&sCMainContext.context, (jarray) jmc);

    // Establish self-pointing pointer (thus enabling later references to the java object
    // containing the context):
    sJavaMainContext->javaSelf = jmc;

    // Protect it so GC won't eat it:
    heapProtect((jobject) jmc, TRUE);

    // Create native C stack for the Java main thread execution:
    sMainThreadNativeStack = NewByteArray(&sCMainContext.context, cStackSize);
    if (sMainThreadNativeStack == NULL) {
        consoutli("Premature out of memory; can't alloc a stack\n");
        jvmexit(1);
    }

    // Protect it so GC won't eat it:
    heapProtect((jobject) sMainThreadNativeStack, TRUE);

    // Initialize context:
    ntInitContext(sJavaMainContext, sMainThreadNativeStack, sCStackSize, sMainFunction,
            startClassIndex, startAddress);

    // Start Java Main Thread:
    ntYield(&sJavaMainContext->context, &sCMainContext, sJavaMainContext);

    // will only return to this point when the main thread has terminated
}

jobject thAllocNativeStack(void) {
    return (jobject) NULL; //osAllocateNativeStack(sCStackSize);
}

jobject thGetMainNativeStack(void) {
    return (jobject) sMainThreadNativeStack;
}

int thGetJavaStackPointer(jobject thread) {
    return 0;
}

//thStackInfo_t* nextStackInfo(thStackInfo_t* current) {
//
//}

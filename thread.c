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

//#define DEBUG_ENABLE
#include "debug.h"
#include "alltests.h"

// Collection of all threads:
static ntThreadContext_t* sAllThreads = NULL;
ntThreadContext_t* thCurrentThread = NULL;

// Native Thread context for the 'thread' calling the 'main' method of this executable:
static ntThreadContext_t sCMainContext;

// The size of a Native C stack:
static size_t sCStackSize;

/**
 * The size of a thread stack in count of stackables. Default is 100, but this is changed when the main stack is initialized
 */
size_t STACK_SIZE = 100;

/**
 * This function returns the size of a stack in number of bytes
 */
static size_t sGetStackSizeInBytes() {
    return STACK_SIZE * sizeof (stackable);
}

/**
 * This function sets the java thread reference as the first element in the java stack
 * \param javaStack  The java stack to modify
 * \param javaThread The thread reference to write
 */
static void sSetJavaThread(contextDef* context, jbyteArray javaStack, jobject javaThread) {
    stackable* st = jaGetArrayPayLoad(context, (jarray) javaStack);
    st[0].operand.jref = javaThread;
    st[0].type = OBJECTREF;
}

/**
 * This function returns the java thread object associated with native thread ntc
 * \param ntc The native thread 
 * \return  The associated java thread object or NULL, if none associated or ntc == NULL
 */
static jobject sGetJavaThread(contextDef* context, ntThreadContext_t* ntc) {
    jobject retval = NULL;
    if (ntc != NULL) {
        if (ntc->javaStack != NULL) {
            stackable* stack = jaGetArrayPayLoad(context, (jarray) ntc->javaStack);

            retval = stack[0].operand.jref;
        }
    }

    return retval;
}

/**
 * This function returns the current java thread object
 * 
 * \return  The current thread object or NULL, if no current
 */
static jobject sGetCurrentJavaThread(contextDef* context) {
    return sGetJavaThread(context, thCurrentThread);
}

static void sDumpContext(contextDef* context, char* prefix) {
    jobject thr = sGetCurrentJavaThread(context);
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

// TODO Make time-based!
static int aYieldCount = 0;

void thTryYield(contextDef* context) {
    if (aYieldCount++ >= 100) {
        aYieldCount = 0;
        thYield(context);
    }
}

jlong thGetCurrentThreadId(contextDef* context) {
    jobject currentThread = sGetCurrentJavaThread(context);
    return GetLongField(context, currentThread, A_java_lang_Thread_aId);
}

static ntThreadContext_t* sGetNextThread(ntThreadContext_t* curr) {
    ntThreadContext_t* next = curr->next;
    if (next == NULL) {
        // Start all over:
        next = sAllThreads;
    }

    return next;
}

/**
 * This function removes the thread and makes the allocated resources available
 * for garbage collection
 * \param curr The thread to remove
 */

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

    // Make resources available for GC:
    heapProtect((jobject) curr->stack, FALSE);
    heapProtect((jobject) curr->javaStack, FALSE);
    heapProtect((jobject) curr->javaSelf, FALSE);
}

/**
 * This function returns the next thread to switch to. If the Main Java thread
 * has not yet been attached, this method returns the contents of thCurrentThread
 * \param interrupted Pointer to flag to set if returned thread is interrupted
 * @return the next thread to switch to. 
 */

ntThreadContext_t* sGetNextRunnableThread(contextDef* context) {
    ntThreadContext_t* nextThread = thCurrentThread;
    if (sGetJavaThread(context, nextThread) != NULL) {
        while ((nextThread = sGetNextThread(nextThread)) != NULL) {
            jobject nextJavaThread = sGetJavaThread(context, nextThread);
            int state = GetIntField(context, nextJavaThread, A_java_lang_Thread_aState);
            if (state == StateTerminated) {
                // TODO Use enum value in stead!
                // Thread is terminated, get rid of it:
                sRemoveThread(nextThread);

                // Start all over searching for a runnable thread:
                nextThread = sAllThreads;
            } else if (state == StateTimedWaiting) {
                jlong when = GetLongField(context, nextJavaThread, A_java_lang_Thread_aWakeUpAt);
                if (when < vtCurrentTimeMillis()) {
                    SetIntField(context, nextJavaThread, A_java_lang_Thread_aState, StateRunnable);
                    break;
                } else {
                    // Check for interrupt:
                    if (GetBooleanField(context, nextJavaThread, A_java_lang_Thread_aInterrupted)) {
                        SetIntField(context, nextJavaThread, A_java_lang_Thread_aState, StateRunnable);
                        break;
                    }
                    // else: Not interrupted, stay asleep...
                }
            } else if (state == StateRunnable) {
                break;
            }
        }
    } else {
        nextThread = thCurrentThread;
    }

    return nextThread;
}

void thYield(contextDef* context) {
    // sDumpContext(context, "thYield before: ");
    __DEBUG("before: pc = 0x%04x, sp = 0x%04x", context->programCounter, context->stackPointer);

    ////////////////////////////////////////////////////////////////////////////
    // Find next runnable thread:
    ////////////////////////////////////////////////////////////////////////////
    ntThreadContext_t* nextThread = sGetNextRunnableThread(context);

    ////////////////////////////////////////////////////////////////////////////
    // Switch to next thread:
    ////////////////////////////////////////////////////////////////////////////
    if (nextThread != NULL) {
        // Let the other thread run:
        if (thCurrentThread != nextThread) {
            jclass cls = getJavaLangClass(context, C_java_lang_Thread);
            if (sGetJavaThread(context, nextThread) == NULL) {
                jvmexit(88);
            }

            __DEBUG("nt yield %p %p\n", thCurrentThread, nextThread);
            // Set stack to point at next Thread's stack:
            osSetStack(context, nextThread->javaStack);

            ntThreadContext_t* currNtc = thCurrentThread;
            thCurrentThread = nextThread;
            ntYield(context, currNtc, nextThread);
        }
    } else {
        // All threads are terminated:
        jvmexit(0);
    }

    //sDumpContext(context, "thYield after : ");
}

/**
 * This function (re-) competes for a lock on an object. Before calling this method the object to lock and then
 * the lockCount shall be set in the thread object. It is the responsibility of the caller to ensure
 * that the object to lock has a monitor != NULL.
 * \param thread The competing thread
 * \return TRUE, if the lock was granted to the thread; FALSE if the thread is blocked and should wait
 */
BOOL sTryWaitForLock(contextDef* context, jobject thread) {
    BOOL lockGranted = TRUE;

    jint threadLockCount = GetIntField(context, thread, A_java_lang_Thread_aLockCount);
    jobject objectToLock = GetObjectField(context, thread, A_java_lang_Thread_aBlockingObject);
    if (objectToLock == NULL) {
        // ups! obkectToLock er null ????!!!!
        consoutli("sTryWait enter:%p %p\n", context, objectToLock);
        jvmexit(56);
    }
    jobject monitor = GetObjectField(context, objectToLock, A_java_lang_Object_aMonitor);

    // If monitorLockCount == 0 then the monitor isn't locked:
    jint monitorLockCount = GetIntField(context, monitor, A_java_lang_Object_Monitor_aLockCount);
    if (monitorLockCount == 0) {
        // thread is not owner and the monitor isn't locked, re-use monitor and set the thread as owner:
        SetObjectField(context, monitor, A_java_lang_Object_Monitor_aOwner, thread);
        SetIntField(context, monitor, A_java_lang_Object_Monitor_aLockCount, threadLockCount);
        // thread is not blocking:
        SetObjectField(context, thread, A_java_lang_Thread_aBlockingObject, NULL);
        __DEBUG("%p got lock %p\n", thread, objectToLock);
        SetIntField(context, thread, A_java_lang_Thread_aState, StateRunnable);
    } else {
        jobject lockOwnerThread = GetObjectField(context, monitor, A_java_lang_Object_Monitor_aOwner);
        if (lockOwnerThread == thread) {
            // Current thread is already owner; increment lock counter:
            threadLockCount += monitorLockCount;
            SetIntField(context, monitor, A_java_lang_Object_Monitor_aLockCount, threadLockCount);
        } else {
            // Change state on thread to BLOCKED on object:
            SetObjectField(context, thread, A_java_lang_Thread_aBlockingObject, objectToLock);
            SetIntField(context, thread, A_java_lang_Thread_aState, StateBlocked);
            SetIntField(context, thread, A_java_lang_Thread_aLockCount, threadLockCount);

            lockGranted = FALSE;
        }
    }

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
 * This function searches for a thread blocked on 'lockedObject', and if any found
 * the first found is set to state 'StateRunnable'
 * @param context
 * @param lockedObject The object that has been unlocked
 */
static void sUnlockThread(contextDef* context, jobject lockedObject) {
    jobject monitor = GetObjectField(context, lockedObject, A_java_lang_Object_aMonitor);

    // No longer locking the monitor, test if some other thread is blocking on the monitor:
    ntThreadContext_t* thread = sAllThreads;
    while (thread != NULL) {
        jobject javaThread = sGetJavaThread(context, thread);
        jint state = GetIntField(context, javaThread, A_java_lang_Thread_aState);
        if (state == StateBlocked) {
            jobject blockingObject = GetObjectField(context, javaThread, A_java_lang_Thread_aBlockingObject);
            if (blockingObject == lockedObject) {
                // If monitorLockCount == 0 then the monitor isn't locked:
                jint monitorLockCount = GetIntField(context, monitor, A_java_lang_Object_Monitor_aLockCount);
                if (monitorLockCount == 0) {
                    // Make this thread runnable; as long as it has a blocking object ref it will gain the 
                    // lock before continuing execution:
                    SetIntField(context, javaThread, A_java_lang_Thread_aState, StateRunnable);
                    break;
                }
            }
        }
        thread = thread->next;
    }
}

/**
 * This function unlocks a monitor. It is the callers responsibility to ensure that current thread owns the
 * monitor
 * \param lockedObject The object encapsulating the monitor
 * \param unlockCount The number of unlocks to perform
 * \throws IllegalMonitorStateException if the monitors lockCount < unlockCount
 */
static void sUnlockOwnedMonitor(contextDef* context, jobject lockedObject, jint unlockCount) {
    // Unlock monitor:
    jobject monitor = GetObjectField(context, lockedObject, A_java_lang_Object_aMonitor);

    jint lockCount = GetIntField(context, monitor, A_java_lang_Object_Monitor_aLockCount);
    if (lockCount >= unlockCount) {
        lockCount -= unlockCount;
        SetIntField(context, monitor, A_java_lang_Object_Monitor_aLockCount, lockCount);
        if (lockCount == 0) {
            // No longer locking the monitor, test if some other thread is blocking on the monitor:
            sUnlockThread(context, lockedObject);
        }
    } else {
        __DEBUG("throwIllegalMonitorStateException");
        throwIllegalMonitorStateException(context, "Monitor is not locked");
    }
}

void thMonitorEnter(contextDef* context, jobject objectToLock) {
    __DEBUG("begin %p %04x\n",
            GetStaticObjectField(context, getJavaLangClass(context, C_java_lang_Thread), A_java_lang_Thread_aCurrentThread),
            context->programCounter);

    if (objectToLock != NULL) {
        jobject monitor = GetObjectField(context, objectToLock, A_java_lang_Object_aMonitor);
        jobject currentThread = sGetCurrentJavaThread(context);

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
    __DEBUG("thMonitorExit enter %p\n", context);
    if (objectToLock != NULL) {
        jobject monitor = GetObjectField(context, objectToLock, A_java_lang_Object_aMonitor);
        jobject currentThread = sGetCurrentJavaThread(context);
        jobject lockOwnerThread = GetObjectField(context, monitor, A_java_lang_Object_Monitor_aOwner);

        if (lockOwnerThread == currentThread) {
            // Unlock monitor:
            sUnlockOwnedMonitor(context, objectToLock, 1);
        } else {
            __DEBUG("throwIllegalMonitorStateException");
            throwIllegalMonitorStateException(context, "Thread not owner");
        }
    } else {
        // Can't lock on null:
        __DEBUG("throwNullPointerException");
        throwNullPointerException(context);
    }
    __DEBUG("thMonitorExit leave %p\n", context);
}

/**
 * This function validates the object to lock on; sets monitor and current thread
 *
 * \param lockedObject The mutex that the thread hopefully is locked on
 * \param monitor The monitor instance is returned in this pointer
 * \param currentThread The current thread is returned in this pointer
 * \return true, if no exceptions have been thrown and all is ok
 */
BOOL sCheckWaitNotify(contextDef* context, jobject lockedObject, jobject* monitor, jobject* currentThread) {
    BOOL retval = FALSE;
    if (lockedObject != NULL) {
        *monitor = GetObjectField(context, lockedObject, A_java_lang_Object_aMonitor);
        if (*monitor != NULL) {
            *currentThread = sGetCurrentJavaThread(context);
            jobject lockOwnerThread = GetObjectField(context, *monitor, A_java_lang_Object_Monitor_aOwner);
            if (*currentThread == lockOwnerThread) {
                retval = TRUE;
            } else {
                __DEBUG("throwIllegalMonitorStateException");
                throwIllegalMonitorStateException(context, "Thread not owner");
            }
        } else {
            __DEBUG("throwIllegalMonitorStateException");
            throwIllegalMonitorStateException(context, "Monitor is not locked");
        }
    } else {
        // Can't notify on null:
        __DEBUG("throwNullPointerException");
        throwNullPointerException(context);
    }

    return retval;
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

    jobject monitor, currentThread;
    if (sCheckWaitNotify(context, lockedObject, &monitor, &currentThread)) {
        // Find all threads that are in Wait Set WAITING on the lockedObject:
        jobject waitElement = GetObjectField(context, monitor, A_java_lang_Object_Monitor_aWaitSet);
        while (waitElement != NULL) {
            jobject waitingThread = GetObjectField(context, waitElement, A_java_lang_Object_WaitElement_aWaiting);
            if (waitingThread == NULL) {
                // This should never happen, but handle it gracefully:
                __DEBUG("throwNullPointerException");
                throwNullPointerException(context);
                break;
            }

            // Set state to RUNNABLE:
            SetIntField(context, waitingThread, A_java_lang_Thread_aState, StateRunnable);

            // Remove thread from the list of WAITING threads:
            jobject next = GetObjectField(context, waitElement, A_java_lang_Object_WaitElement_aNext);
            SetObjectField(context, monitor, A_java_lang_Object_Monitor_aWaitSet, next);

            waitElement = next;

            // notifyAll / notify ?
            if (!notifyAll) {
                break;
            }
        }
    }

    __DEBUG("end %04x\n", context->programCounter);
}

void thWait(contextDef* context, jobject lockedObject) {
    // file:///tools/vmspec/Threads.doc.html#21294

    __DEBUG("Begin PC = %04x\n", context->programCounter);
    // sDumpContext(context, "thWait  begin:  ");
    jobject monitor, currentThread;
    if (sCheckWaitNotify(context, lockedObject, &monitor, &currentThread)) {
        /*
         * Add current thread to (end of) wait set for locked object:
         */
        jobject waitElement = GetObjectField(context, monitor, A_java_lang_Object_Monitor_aWaitSet);
        jobject lastElement = waitElement;
        while (waitElement != NULL) {
            lastElement = waitElement;
            waitElement = GetObjectField(context, waitElement, A_java_lang_Object_WaitElement_aNext);
        }

        jobject newElement = AllocObject(context, getJavaLangClass(context, C_java_lang_Object_WaitElement));
        if (newElement != NULL) {
            SetObjectField(context, newElement, A_java_lang_Object_WaitElement_aWaiting, currentThread);
            if (lastElement == NULL) {
                // Element is the one and only waiting thread:
                SetObjectField(context, monitor, A_java_lang_Object_Monitor_aWaitSet, newElement);
            } else {
                // There are more waiting threads:
                SetObjectField(context, lastElement, A_java_lang_Object_WaitElement_aNext, newElement);
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
            sUnlockOwnedMonitor(context, lockedObject, lockCount);

            // Wait for a notification changing state from WAITING to RUNNABLE:
            while (GetIntField(context, currentThread, A_java_lang_Thread_aState) == StateWaiting) {
                thYield(context);
            }

            // Compete for lock:
            sWaitForLock(context, currentThread);

            // Restore lock count:
            SetIntField(context, currentThread, A_java_lang_Thread_aLockCount, lockCount);
        }
        // else: Out Of Mem has been thrown
    }

    __DEBUG("ended\n");
    //    sDumpContext(context, "thWait  after : ");
}

void thSleep(contextDef* context, jlong millis) {
    jobject currentThread = sGetCurrentJavaThread(context);

    // Fall asleep:
    jlong wakeup = vtCurrentTimeMillis() + millis;
    SetIntField(context, currentThread, A_java_lang_Thread_aState, StateTimedWaiting);
    SetLongField(context, currentThread, A_java_lang_Thread_aWakeUpAt, wakeup);

    // Switch thread:
    thYield(context);

    // Check for interrupt:
    if (GetBooleanField(context, currentThread, A_java_lang_Thread_aInterrupted)) {
        throwInterruptedException(context);
    }
}

jbyteArray sAllocStack(contextDef* context) {
    // Allocate an object without protecting it:
    jbyteArray stackObject = NewByteArray(context, sGetStackSizeInBytes());

    if (stackObject != NULL) {
        // Avoid garbage collection of our one and only stack (at this point).
        // The stack shall be unprotected when the stack has been referenced from the
        // first thread, see osUnprotectStack():
        heapProtect((jobject) stackObject, TRUE);
    }
    // else: An out-of-mem exception has been thrown

    return stackObject;
}

/**
 * Definition of function for starting the run-method
 */
static void sRunFunction(ntThreadContext_t* ntc) {
    __DEBUG("Creating thread: %p\n", &ntc->context);
    inExecute(&ntc->context);

    // Unreachable; runFromNative will never return to this point
}

/**
 * Definition of main function for VM. Has to be compatible with the native scheduling mechanism
 */
static void sMainFunction(ntThreadContext_t* ntc) {
    // First thread running:
    sAllThreads = ntc;

    // Set as current stack:
    osSetStack(&ntc->context, ntc->javaStack);

    // Load all java.lang.Class instances:
    cpGenerateJavaLangClassInstances(&ntc->context);

    push_frame(&ntc->context, 0, ntc->context.classIndex, ntc->context.programCounter, TRUE);

    inExecute(&ntc->context);

    __DEBUG("Java Main thread has terminated");
    // Return to C Main thread:
    ntYield(&ntc->context, NULL, &sCMainContext);
}

/**
 * This function allocates a native thread context for the calling thread.
 * \param context Java context
 * \param this The thread instance
 * \param stack The native stack
 * \return The native thread context as a java object (byte[])
 */
ntThreadContext_t* thAllocNativeContext(contextDef* context, jobject this, jbyteArray stack) {
    jbyteArray ntContextObject = NewByteArray(context, sizeof (ntThreadContext_t));
    ntThreadContext_t* nt = NULL;
    if (ntContextObject != NULL) {
        heapProtect((jobject) ntContextObject, TRUE);
        nt = (ntThreadContext_t*) jaGetArrayPayLoad(context, (jarray) ntContextObject);
        nt->stack = stack;
        if (this != NULL) {
            const methodInClass* mic = getVirtualMethodEntryByLinkId(this, M_java_lang_Thread_runFromNative);
            ntInitContext(nt, sCStackSize, sRunFunction, C_java_lang_Thread, mic->codeOffset);
        } else {
            // It's the main thread
            ntInitContext(nt, sCStackSize, sMainFunction, startClassIndex, startAddress);
        }
        nt->javaSelf = ntContextObject;
    }
    // else: Out of mem has been thrown

    return nt;
}

static ntThreadContext_t* sAllocContext(contextDef* context, jobject thread) {
    jbyteArray javaStack = NULL;
    jbyteArray nativeStack = NULL;
    ntThreadContext_t* nativeContext = NULL;

    // Allocate and prepare the java stack for this thread:
    javaStack = sAllocStack(context);
    if (javaStack != NULL) {
        // Allocate and prepare the native C stack for this thread:
        nativeStack = NewByteArray(context, sCStackSize);
    }
    if (nativeStack != NULL) {
        heapProtect((jobject) nativeStack, TRUE);
        // Allocate and prepare a native thread context:
        nativeContext = thAllocNativeContext(context, thread, nativeStack);
    }
    if (nativeContext != NULL) {
        nativeContext->next = NULL;
        nativeContext->javaStack = javaStack;
    }

    return nativeContext;
}

void thInterrupt(contextDef* context, jobject thread) {
    // How difficult can it be ?!
    SetBooleanField(context, thread, A_java_lang_Thread_aInterrupted, TRUE);
}

void thStartVM(align_t* heap, size_t heapSize, size_t javaStackSize, size_t cStackSize) {
    // Initialize java heap:
    heapInit(heap, heapSize);

    // Record the size of a C stack:
    sCStackSize = cStackSize;

    // STACK_SIZE is in count of stackables, not bytes:
    STACK_SIZE = javaStackSize / sizeof (stackable);

    // Clear static area:
    memset(&staticMemory[0], staticMemorySize, sizeof (stackable));

    // Turn this 'thread' into C Main thread. The C Main thread shall remain 
    // passive until termination of VM:
    memset(&sCMainContext, 0, sizeof (sCMainContext));
    sCMainContext.func = NULL;

    // -------------------------------------------------------------------
    // TODO set no-throw flag 
    ntThreadContext_t* javaMainContext = sAllocContext(&sCMainContext.context, NULL);

    // Start Java Main Thread:
    ntYield(&javaMainContext->context, &sCMainContext, javaMainContext);

    __DEBUG("VM is terminating");
    // will only return to this point when the main thread has terminated
}

void thAttach(contextDef* context, jobject thread) {
    __DEBUG("**** BEGIN adding: %p", thread);

    // Set current state to StateRunnable:
    SetIntField(context, thread, A_java_lang_Thread_aState, StateRunnable);

    ntThreadContext_t* nativeContext;
    if (sGetJavaThread(context, sAllThreads) != NULL) {
        nativeContext = sAllocContext(context, thread);

        if (nativeContext != NULL) {
            nativeContext->next = sAllThreads;
            sAllThreads = nativeContext;
        }
        // else: Out of mem thrown
    } else {
        // It's the main thread; the context has already been alloc'ed, however
        // at the alloc time the java Thread object did not exist:
        // At the top of the java stack there shall be a reference to own thread;
        // otherwise the Main Thread will be garbage collected:
        nativeContext = sAllThreads;
    }
    sSetJavaThread(context, nativeContext->javaStack, thread);

    __DEBUG("**** END adding: %p", thread);
}

void thForeachThread(contextDef* context, thCallback_t callback) {
    ntThreadContext_t* ntc;
    for (ntc = sAllThreads; ntc != NULL; ntc = ntc->next) {
        stackable* stack = jaGetArrayPayLoad(context, (jarray) ntc->javaStack);

        callback(context, stack, ntc->context.stackPointer);
    }
}

jobject thGetCurrentThread(contextDef* context) {
    return sGetCurrentJavaThread(context);
}

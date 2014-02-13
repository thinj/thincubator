/*
 * heap.c
 *
 *  Created on: Oct 13, 2010
 *      Author: hammer
 */

#include "config.h"
#include "jni.h"
#include "jarray.h"
#include "heap.h"
#include "heaplist.h"
#include "constantpool.h"
#include "frame.h"
#include "exceptions.h"
#include "objectaccess.h"
#include "vmids.h"
#include "thread.h"

//#define DEBUG_ENABLE
#include "debug.h"
#include "nativethreads.h"

// The current mark value:
static unsigned int markValue;

size_t HEAP_HEADER_SIZE;

void heapInit(align_t* heap, size_t size) {
    HEAP_HEADER_SIZE = ToAlignedSize(sizeof (header_t));

    heap_init(heap, size);
    // Never let markValue == 0:
    markValue = 1;
}

jobject heapAllocObjectByByteSize(contextDef* context, u2 size, u2 classId) {
    header_t* h = heap_alloc(ToAlignedSize(size));
    if (h == NULL && context != NULL) {
        markAndSweep(context);
        h = heap_alloc(ToAlignedSize(size));
    }

    if (h != NULL) {
        h->e.classId = classId;
        // The mark - field is already init to 0 => it is always != markValue as long it isn't marked
        // => it can be garbage collected, and it will not by incident look like it is marked
    } else {
        //		consout("out of mem friends!\n");
        throwOutOfMemoryError(context);
    }

    return (jobject) h;
}

jobject heapAllocObjectByStackableSize(contextDef* context, u2 size, u2 classId) {
    return heapAllocObjectByByteSize(context, size * sizeof (stackable), classId);
}

void heapProtect(jobject jref, BOOL protected) {
    header_t* h = (header_t*) jref;
    set_type(h, protected ? HT_PROTECTED : HT_USED);
}

/**
 * This method marks all objects referenced from the 'memory'
 * \param memory The arary of stackable to search
 * \size The number of elements in 'memory'
 */
static void mark2(contextDef* context, stackable* memory, size_t size);

static void markObject3(contextDef* context, jobject obj) {
    HEAP_VALIDATE;
    header_t* curList;
    header_t* nxtList;
    if (obj != NULL) {
        curList = getHeader(obj);
        curList->e.next = NULL;
    }

    nxtList = NULL;
    while (curList != NULL) {
        HEAP_VALIDATE;
        header_t* h = curList;
        curList = curList->e.next;
        h->e.next = NULL;
        obj = getObjectFromHeader(h);

        if (h->e.mark != markValue) {
            // Mark now to avoid future visits:
            h->e.mark = markValue;

            if (isObjectArray(h->e.classId)) {
                jobjectArray arr = (jobjectArray) obj;
                size_t length = GetArrayLength((jarray) arr);
                size_t i;
                for (i = 0; i < length; i++) {
                    jobject element = GetObjectArrayElement(context, arr, i);
                    if (element != NULL) {
                        // Insert into next list;
                        h = getHeader(element);
                        // TODO hvis to elementer peger på samme object, risikerer vi så at collecte live objecsts?
                        if (h->e.next == NULL) {
                            if (nxtList == NULL) {
                                h->e.next = h;
                                nxtList = h;
                            } else {
                                h->e.next = nxtList->e.next;
                                nxtList->e.next = h;
                            }
                        }
                        // else: Already in list
                    }
                }
            } else if (isPrimitiveValueArray(h->e.classId)) {
                // Ignore
            } else {
                // A 'normal' object with stackables:
                stackable* memory = GetObjectPayloadNoThrow(obj);
                u2 size;
                getClassSize(h->e.classId, &size);
                size_t i;
                for (i = 0; i < size; i++) {
                    if (memory[i].type == OBJECTREF) {
                        if (memory[i].operand.jref != NULL) {
                            h = getHeader(memory[i].operand.jref);

                            // TODO hvis to elementer peger på samme object, risikerer vi så at collecte live objecsts?
                            if (h->e.next == NULL) {
                                if (nxtList == NULL) {
                                    h->e.next = h;
                                    nxtList = h;
                                } else {
                                    h->e.next = nxtList->e.next;
                                    nxtList->e.next = h;
                                }
                            }
                            // else: Already in list

                        }
                    }
                }
            }
        }
        // else: Already marked

        if (curList == NULL) {
            curList = nxtList;
            nxtList = NULL;
        }

        HEAP_VALIDATE;
    }
}

static void mark2(contextDef* context, stackable* memory, size_t size) {
    HEAP_VALIDATE;
    size_t i;
    for (i = 0; i < size; i++) {
        if (memory[i].type == OBJECTREF) {
            if (memory[i].operand.jref != NULL) {
                markObject3(context, memory[i].operand.jref);
            }
        }
    }
    HEAP_VALIDATE;
}

void markAndSweep(contextDef* context) {
    __DEBUG("Mark & Sweep BEGIN");
    //	heap_dump();
    HEAP_VALIDATE;
    // The mark field in header_t is MARK_BIT_SIZE bits only. Avoiding the value '0' will
    // ensure that a newly created, not protected object will always != markValue
    // => it can be collected:
    markValue = (markValue + 2) & ((1 << MARK_BIT_SIZE) - 1);

    // Mark static area:
    mark2(context, staticMemory, staticMemorySize);

    // Mark all stacks:
    thForeachThread(context, mark2);
    
    // Sweep heap:
    heap_sweep(markValue);

    //	heap_dump();
    __DEBUG("Mark & Sweep END");

    HEAP_VALIDATE;
}
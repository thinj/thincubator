/*
 * types.c
 *
 *  Created on: Sep 14, 2012
 *      Author: hammer
 */

#include <stddef.h>
#include "types.h"
#include "exceptions.h"

//#define TO_ALIGNED_SIZE(X) (size_in_bytes + sizeof(align_t) - 1) / sizeof(align_t)

/**
 * This method converts a size in bytes into a size in count of align_t
 */
size_t ToAlignedSize(size_t size_in_bytes) {
    return (size_in_bytes + sizeof (align_t) - 1) / sizeof (align_t);
}

/**
 * This function returns a pointer to the instance payload of the object
 * \param obj The object for which the payload is returned
 * \return The instance payload of the object or null obj == NULL
 * \throws NullPointerException if obj == null
 */
void* GetObjectPayload(contextDef* context, jobject obj) {
    align_t* align;
    if (obj != NULL) {
        align = ((align_t*) obj) + HEAP_HEADER_SIZE;
    } else {
        align = NULL;
        throwNullPointerException(context);
    }
    return (void*) align;
}

void* GetObjectPayloadNoThrow(jobject obj) {
    align_t* align = NULL;
    if (obj != NULL) {
        align = ((align_t*) obj) + HEAP_HEADER_SIZE;
    }
    return (void*) align;
}


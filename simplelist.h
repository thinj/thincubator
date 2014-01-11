/*
 * simplelist.h
 *
 *  Created on: May 24, 2013
 *      Author: hej
 */

#ifndef SIMPLELIST_H_
#define SIMPLELIST_H_

#include "jni.h"
#include "vmids.h"

/**
 * This function allocates an empty SimpleList.
 * \throws OutOfMemoryError if out of mem
 * \return The allocated list or NULL, if out of mem
 */
jobject slCreateList(contextDef* context);

/**
 * \param list An instance of SimpleList
 * \param obj The element to add. NULL is accepted, but use with care (see slRemove)
 * \throws NullPointerException if list == NULL
 */
void slAdd(contextDef* context, jobject list, jobject obj);

/**
 * \param list An instance of SimpleList
 * \param obj The element to remove. Null is accepted, but only the first element where the value is null is removed
 * \return true if an element has been removed
 * \throws NullPointerException if list == NULL
 */
jboolean slRemove(contextDef* context, jobject list, jobject obj);

#endif /* SIMPLELIST_H_ */

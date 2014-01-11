/*
 *  Java_thinj_util_SimpleList.c
 */

#include "jni.h"
#include "vmids.h"
#include "exceptions.h"
#include "constantpool.h"

jobject slCreateList(contextDef* context) {
    return AllocObject(context, getJavaLangClass(context, C_thinj_util_SimpleList));
}

void slAdd(contextDef* context, jobject list, jobject obj) {
    if (list == NULL) {
        throwNullPointerException(context);
    } else {
        jobject aList = GetObjectField(context, list, A_thinj_util_SimpleList_aList);
        jobject element = AllocObject(context, getJavaLangClass(context, C_thinj_util_SimpleList_Element));
        if (element != NULL) {
            // Insert first in list:
            SetObjectField(context, element, A_thinj_util_SimpleList_Element_aNext, aList);
            SetObjectField(context, element, A_thinj_util_SimpleList_Element_aValue, obj);
            SetObjectField(context, list, A_thinj_util_SimpleList_aList, element);
        }
        // else: OutOfMem has been thrown
    }
}

jboolean slRemove(contextDef* context, jobject list, jobject obj) {
    jboolean found = FALSE;
    if (list == NULL) {
        throwNullPointerException(context);
    } else {
        jobject prev = NULL;
        jobject next = GetObjectField(context, list, A_thinj_util_SimpleList_aList);

        // Search for object in list:
        while (next != NULL) {
            if (IsSameObject(GetObjectField(context, next, A_thinj_util_SimpleList_Element_aValue), obj)) {
                // Got it!
                break;
            }
            prev = next;
            next = GetObjectField(context, next, A_thinj_util_SimpleList_Element_aNext);
        }

        if (next != NULL) {
            // Object found in list:
            jobject nextNext = GetObjectField(context, next, A_thinj_util_SimpleList_Element_aNext);
            if (prev == NULL) {
                // It's the first object in the list:
                SetObjectField(context, list, A_thinj_util_SimpleList_aList, nextNext);
            } else {
                SetObjectField(context, prev, A_thinj_util_SimpleList_Element_aNext, nextNext);
            }
            found = TRUE;
        }
        // else: Object not in list
    }

    return found;
}

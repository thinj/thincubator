/*
 * constantpool.c
 *
 *  Created on: Sep 29, 2010
 *      Author: hammer
 */

#include <stdlib.h>
#include "console.h"
#include "constantpool.h"
#include "objectaccess.h"
#include "exceptions.h"
#include "instructions.h"
#include "frame.h"
#include "heap.h"
#include "trace.h"
#include "jni.h"
#include "vmids.h"
#include "simplelist.h"

static void sValidateClassId(u2 classId) {
    if (classId >= cpNumberOfAllClassInstanceInfo) {
        consout("Class id = %d out of range (0-%d)\n", classId, cpNumberOfAllClassInstanceInfo);
        DUMP_STACKTRACE("class id");
        jvmexit(1);
    }
}

static void sDumpStack(contextDef* context, int lineNo) {
    consoutli("%d: dumping 0x%04x 0x%04x...\n", lineNo, context->programCounter, context->stackPointer);
    int i;
    for (i = 0; i < context->stackPointer; i++) {
        stackable* stp;
        stp = stack + i;
        const char* typ = StackTypeToString(stp->type);
        consoutli("%04x %s\n", i, typ);
    }

}

jobjectArray cpJavaLangClassArray;

/**
 * This method looks up the class instance info for the class identified by classId
 * \param classId Identifies the class to lookup
 * \return A non-null struct with class instance info. If unable to find the matching info, the vm exits.
 */
static const classInstanceInfoDef* getClassInfo(u2 classId) {
    BEGIN
            ;
    sValidateClassId(classId);

    const classInstanceInfoDef* classInfo = &allClassInstanceInfo[classId];

    END;
    return classInfo;
}

/**
 * This method looks up the super class id for a class
 * \param classId The id of the class for which the super class id shall be found.
 */
static u2 getSuperClass(u2 classId) {
    return getClassInfo(classId)->superClassId;
}

u2 getArrayClassIdForElementClassId(u2 elementClassId) {
    BEGIN
            ;
    int i;
    for (i = 0; i < cpNumberOfAllClassInstanceInfo; i++) {
        if (allClassInstanceInfo[i].elementClassId == elementClassId
                && allClassInstanceInfo[i].type == CT_OBJECT_ARRAY) {
            break;
        }
    }

    if (i >= cpNumberOfAllClassInstanceInfo) {
        consout("Array Class for element id = %d not found\n", elementClassId);
        DUMP_STACKTRACE("class element id");
        jvmexit(1);
    }

    END;
    return i;

}

/**
 * This method looks up the size of an instance of a class
 */
void getClassSize(u2 classId, u2* size) {
    *size = getClassInfo(classId)->instanceSize;
}

/**
 * This function tests if class S implements interface T
 * \param classId_S The class id to test
 * \param classId_T The interface to test for implementation of
 * \return true, if implementing; false otherwise
 */
BOOL is_S_implementing_T(u2 classId_S, u2 classId_T) {
    BEGIN
            ;
    int interface_start;
    int interface_count;
    const classInstanceInfoDef* classInfo = getClassInfo(classId_S);

    interface_start = classInfo->interface_start;
    interface_count = classInfo->interface_count;

    BOOL implements = FALSE;
    int i;
    for (i = 0; i < interface_count && !implements; i++) {
        implements = implementedInterfaces[i + interface_start] == classId_T;
    }

    END;
    return implements;
}

/**
 * This function tests if class S is sub class of class T
 * \param classId_S The (sub-) class id to test
 * \param classId_T The class id to test against
 * \return true, if sub classing; false otherwise
 */
BOOL is_S_SubClassing_T(u2 classId_S, u2 classId_T) {
    BEGIN
            ;
    BOOL subclassing = FALSE;

    while (!subclassing && classId_S != 0) {
        const classInstanceInfoDef* class_S = getClassInfo(classId_S);
        subclassing = class_S->superClassId == classId_T;
        if (!subclassing) {
            // Recurse up through super classes:
            classId_S = class_S->superClassId;
        }
    }

    END;
    return subclassing;
}

/**
 * This method looks up the type (interface/class) of a class
 * \param classId the class id to lookup with
 * \return The resulting type. If not match, the method will exit the vm.
 */
CLASS_TYPE getClassType(u2 classId) {
    return getClassInfo(classId)->type;
}

#if 0

/**
 * This function converts a CLASS_TYPE to a readable string
 * \param ctyp The value to convert
 * \return a non-null char*
 */
char* getClassTypeString(CLASS_TYPE ctyp) {
    if (ctyp == CT_CLASS) {
        return "CT_CLASS";
    } else if (ctyp == CT_INTERFACE) {
        return "CT_INTERFACE";
    } else if (ctyp == CT_OBJECT_ARRAY) {
        return "CT_OBJECT_ARRAY";
    } else if (ctyp == CT_BYTE_ARRAY) {
        return "CT_BYTE_ARRAY";
    } else if (ctyp == CT_CHAR_ARRAY) {
        return "CT_CHAR_ARRAY";
    } else if (ctyp == CT_DOUBLE_ARRAY) {
        return "CT_DOUBLE_ARRAY";
    } else if (ctyp == CT_FLOAT_ARRAY) {
        return "CT_FLOAT_ARRAY";
    } else if (ctyp == CT_INT_ARRAY) {
        return "CT_INT_ARRAY";
    } else if (ctyp == CT_LONG_ARRAY) {
        return "CT_LONG_ARRAY";
    } else if (ctyp == CT_SHORT_ARRAY) {
        return "CT_SHORT_ARRAY";
    } else {
        return "unknown type";
    }
}
#endif

CLASS_TYPE convertArrayType(ARRAY_TYPE atyp) {
    CLASS_TYPE ct;
    if (atyp == T_REFERENCE) {
        ct = CT_OBJECT_ARRAY;
    } else if (atyp == (ARRAY_TYPE) CT_BOOLEAN_ARRAY) {
        ct = CT_BOOLEAN_ARRAY;
    } else if (atyp == (ARRAY_TYPE) CT_CHAR_ARRAY) {
        ct = CT_CHAR_ARRAY;
    } else if (atyp == (ARRAY_TYPE) CT_FLOAT_ARRAY) {
        ct = CT_FLOAT_ARRAY;
    } else if (atyp == (ARRAY_TYPE) CT_DOUBLE_ARRAY) {
        ct = CT_DOUBLE_ARRAY;
    } else if (atyp == (ARRAY_TYPE) CT_BYTE_ARRAY) {
        ct = CT_BYTE_ARRAY;
    } else if (atyp == (ARRAY_TYPE) CT_SHORT_ARRAY) {
        ct = CT_SHORT_ARRAY;
    } else if (atyp == (ARRAY_TYPE) CT_INT_ARRAY) {
        ct = CT_INT_ARRAY;
    } else if (atyp == (ARRAY_TYPE) CT_LONG_ARRAY) {
        ct = CT_LONG_ARRAY;
    } else {
        consout("Unknown ARRAY_TYPE: %d\n", atyp);
        jvmexit(1);
    }

    return ct;
}

/**
 * This method invokes a native method identified by the parameter
 * \param nativeIndex The index (+1) into the native jump table
 */
void invokeNativeMethod(contextDef* context, u2 nativeIndex) {
//    if (nativeIndex == 1) {
//    sDumpStack(context, __LINE__);
//    }
    nativeJumpTableEntry entry = nativeJumpTable[nativeIndex - 1];
    entry(context);
}

/**
 * This method will open a new frame and call the method indicated in the parameters.
 *
 * \param mic A pointer to a method definition to be invoked.
 * \param returnFromVM if TRUE: When a frame is popped (using pop_frame) and this is true,
 * the execute() - function will return; if FALSE: No return will happen from the execute()
 * function. Shall be set to TRUE when a native function is calling a java method.
 */
void cpInvokeCommon(contextDef* context, const methodInClass* mic, BOOL returnFromVM) {
    int localVariables;
    if (mic->nativeIndex > 0) {
        localVariables = 0;
    } else {
        //CALL(validateStackables(stack, context->operandStackPointer));
        // Allocate space for local variables (arguments are already allocated):
        // Need to initialize stack space otherwise the marking phase of mark & sweep will see
        // some the data on the stack as object refs:
        localVariables = mic->numberOfLocalVariables - mic->numberOfArguments;
        //CALL(validateStackables(stack, context->operandStackPointer));
    }

    while (localVariables-- > 0) {
        // Push some non-GC critical dummy data:
        operandStackPushJavaInt(context, 0);
    }
    // This is a no-go: context->operandStackPointer += localVariableCount - argumentCount;
    // since it might leave jref - lookalikes on the stack, which might confuse GC

//    Søg på FIX 1 (her); FIX 2 & 3 i thinj og få taget højde for +1 fænomenet.
    int numLocVar = mic->numberOfLocalVariables;
//    if (mic->nativeIndex == 1) {
//        consoutli("FIX 1 native: %d %d\n", mic->nativeIndex, numLocVar);
//        sDumpStack(context, __LINE__);
//        //numLocVar++;
//    }
    push_frame(context, numLocVar, mic->classId, mic->codeOffset, returnFromVM);
    if (mic->nativeIndex > 0) {
        //		consoutli("localVariableCount: %d, native method: %d\n", mic->numberOfLocalVariables, mic->nativeIndex - 1);
        //		consoutli("context: PC=0x%04x  CID=0x%04x  SP=0x%04x  FP=0x%04x EX.THROWN=%s\n", context->programCounter,
        //				context->classIndex, context->stackPointer, context->framePointer,
        //				context->exceptionThrown ? "Yes" : "No");
        //		osDumpStack();
        invokeNativeMethod(context, mic->nativeIndex);
        //		consoutli("context: PC=0x%04x  CID=0x%04x  SP=0x%04x  FP=0x%04x EX.THROWN=%s\n", context->programCounter,
        //				context->classIndex, context->stackPointer, context->framePointer,
        //				context->exceptionThrown ? "Yes" : "No");
        //		consoutli("returned...\n");
        // There is no return from a native method, so the frame is popped now:
        //		pop_frame();
    }
}

/**
 * This method returns the methodInClass with link id matching 'linkId' for the class identified
 * by class id 'classId'
 * \param classId identifies the class
 * \param linkId The link identification
 * \return the corresponding methodInClass entry or NULL, if no match.
 */
static const methodInClass* getMethodInClass(u2 classId, u2 linkId) {
    const methodInClass* res = NULL;
    sValidateClassId(classId);
    const methodInClass* mic = allConstantPools[classId].methods;
    size_t len = allConstantPools[classId].numberOfMethods;

    // Search using bi-section; indexing using linkId is not possible
    int l = 0;
    int r = len;
    int center;
    // Invariants: center < r => It's always safe to use center as index, and r shall never be used
    // as index
    while (res == NULL && l != r) {
        if (r - l == 1) {
            // Test with l only - r is not a legal index:
            if (linkId == mic[l].linkId) {
                // Found:
                res = &mic[l];
            }
            // No more iterations:
            break;
        } else {
            center = (r + l) / 2;
            int diff = (int) linkId - (int) mic[center].linkId;
            if (diff < 0) {
                // Go left:
                r = center;
            } else if (diff > 0) {
                // Go right:
                l = center;
            } else {
                // Found:
                res = &mic[center];
            }
        }
    }

    return res;
}

const methodInClass* getVirtualMethodEntryByLinkId(jobject jref, u2 linkId) {
    BEGIN
            ;

    int referencedClassId = oaGetClassIdFromObject(jref);
    //printf("classId = %d, obj= %p\n", (int) referencedClassId, jref);
    //jvmexit(1);
    // Lookup method in referenced class or in it's super-classes:
    const methodInClass* mic = NULL;
    while (mic == NULL) {
        //		for (i = 0; i < numberOfAllMethodsInAllClasses && !found; i++) {
        //			if (allMethodsInAllClasses[i].classId == referencedClassId
        //					&& allMethodsInAllClasses[i].linkId == linkId) {
        //				found = TRUE;
        //				methodId = i;
        //			}
        //		}
        mic = getMethodInClass(referencedClassId, linkId);
        if (mic == NULL) {
            // Not found:
            if (referencedClassId != JAVA_LANG_OBJECT_CLASS_ID) {
                // Look in super class:
                CALL(referencedClassId = getSuperClass(referencedClassId))
                        ;
            } else {
                // Already in java.lang.Object; symbol not found.
                break;
            }
        }
    }

    //printf("mic found: %d, refclassid = %d\n", found, referencedClassId);
    if (mic == NULL) {
        consout("Failed to look up virtual method entry: referencedClass = %d, linkId=%d\n",
                oaGetClassIdFromObject(jref), linkId);
        jvmexit(1);
    }
    END;

    return mic;
}

/**
 * This method looks up a reference to a method
 * \param classId the id of the referencing class
 * \param constantPoolIndex The index into the constant pool
 * \return The matching method reference. No return is made if no match
 */
static const memberReference* lookupMethodReference(u2 classId, u2 constantPoolIndex) {
    sValidateClassId(classId);

    const memberReference * const methodReferences = allConstantPools[classId].methodReferences;
    size_t length = allConstantPools[classId].numberOfMethodReferences;

    const memberReference* res = NULL;
    if (constantPoolIndex < length) {
        res = &methodReferences[constantPoolIndex];
    }

    return res;
}

/**
 * This method looks up an instance method of a class. This will recurse up through super classes
 * until a match is found.
 * \param index The index identifying the static method within constant pool identified by classIndex
 * \param codeIndex A pointer to where the destination address shall be set
 * \param dstClassIndex A pointer to where the resulting class index shall be set
 * \param localVariableCount The number of local variables, including the arguments
 * \param argumentCount The number of arguments
 * \param nativeIndex The native index. If > 0, the method is a native method.
 * \return The method or NULL, if a NPE has been thrown
 */
const methodInClass* cpGetVirtualMethodEntry(contextDef* context, u2 cp_index) {
    BEGIN
            ;
    int linkId;

    const memberReference* mref = lookupMethodReference(context->classIndex, cp_index);
    linkId = mref->linkId;
    u2 argCount = mref->numberOfArguments;

    // Find the class (-id) on which the referenced method shall be invoked:
    stackable st;

//    consoutli("dumping 0x%04x 0x%04x...\n", context->programCounter, context->stackPointer);
//    if (context->programCounter == 0x008d) {
//        sDumpStack(context, __LINE__);
//    }
    
    s1 ac = (s1) (-argCount);
//    consoutli("argcount = %d %04x, index = %04x\n", argCount, context->programCounter, context->stackPointer + ac);
    getOperandRelativeToStackPointer(context, (s1) (-argCount), &st);
//    consoutli("type = %s\n", StackTypeToString(st.type));
    VALIDATE_TYPE(st.type, OBJECTREF);

    jobject jref = st.operand.jref;
    const methodInClass* mic;
    if (jref == 0) {
        mic = NULL;
        throwNullPointerException(context);
    } else {
        mic = getVirtualMethodEntryByLinkId(jref, linkId);
    }

    END;

    return mic;
}

const methodInClass* getStaticMethodEntryByLinkId(u2 referencedClassId, u2 linkId) {
    const methodInClass* mic = NULL;

    while (mic == NULL) {
        mic = getMethodInClass(referencedClassId, linkId);
        if (mic == NULL) {
            if (referencedClassId != 0) {
                // try super class:
                const classInstanceInfoDef* cl = getClassInfo(referencedClassId);
                referencedClassId = cl->superClassId;
            } else {
                // Not found in any super class including Object => not found
                break;
            }
        }
    }

    if (mic == NULL) {
        consout("Failed to look up static method entry: referencedClass = %d, linkId=%d\n", referencedClassId, linkId);
        jvmexit(1);
    }

    return mic;
}

/**
 * This method looks up a static method of a class.
 *
 * \param classId The referencing class (typically  == context->classIndex)
 * \param cp_index The constant pool index identifying the static method within constant pool
 *  identified by context->classIndex
 * \return A method definition pointer. Points into the autogenerated table
 */
const methodInClass* getStaticMethodEntry(u2 classId, u2 cp_index) {
    int linkId;
    int referencedClassId;

    const memberReference* mref = lookupMethodReference(classId, cp_index);
    linkId = mref->linkId;
    referencedClassId = mref->referencedClassId;

    return getStaticMethodEntryByLinkId(referencedClassId, linkId);

    //	// TODO Super class lookup missing!
    //	int methodId;
    //	if (found) {
    //		found = FALSE;
    //		for (i = 0; i < numberOfAllMethodsInAllClasses && !found; i++) {
    //			if (allMethodsInAllClasses[i].classId == referencedClassId
    //					&& allMethodsInAllClasses[i].linkId == linkId) {
    //				found = TRUE;
    //				methodId = i;
    //			}
    //		}
    //	}
    //
    //	const methodInClass* mic;
    //	if (found) {
    //		mic = &allMethodsInAllClasses[methodId];
    //	} else {
    //		consout("Failed to look up static method entry: index = %d, classId=%d\n", cp_index,
    //				classId);
    //		jvmexit(1);
    //	}
    //
    //	return mic;
}

const fieldInClass* getFieldInClassbyLinkId(u2 classId, u2 linkId) {
    u2 referencedClassId = classId;
    int i;
    const fieldInClass* fic = NULL;
    while (fic == NULL) {
        const constantPool* cpool = &allConstantPools[referencedClassId];
        const fieldInClass* allFields = cpool->fields;
        size_t numberOfAllFields = cpool->numberOfFields;
        for (i = 0; i < numberOfAllFields && fic == NULL; i++) {
            if (allFields[i].linkId == linkId) {
                fic = &allFields[i];
            }
        }
        if (fic == NULL) {
            if (referencedClassId != JAVA_LANG_OBJECT_CLASS_ID) {
                // Look in super class:
                referencedClassId = getSuperClass(referencedClassId);
            } else {
                // Already in java.lang.Object; symbol not found.
                break;
            }
        }
    }

    if (fic == NULL) {
        consout("Failed to look up field entry: classId = %d, linkId=%d\n", classId, linkId);
        jvmexit(1);
    }

    return fic;
}

static void getFieldEntry(contextDef* context, u2 index, u2* address, u1* size) {
    int linkId;
    int referencedClassId;

    const constantPool* cpool = &allConstantPools[context->classIndex];
    const memberReference* mref = &cpool->fieldReferences[index];

    linkId = mref->linkId;
    referencedClassId = mref->referencedClassId;

    // Now the linkId is established...
    const fieldInClass* fic = getFieldInClassbyLinkId(referencedClassId, linkId);
    //	const fieldInClass* fic = NULL;
    //	while (fic == NULL) {
    //		cpool = &allConstantPools[referencedClassId];
    //		const fieldInClass* allFields = cpool->fields;
    //		size_t numberOfAllFields = cpool->numberOfFields;
    //		for (i = 0; i < numberOfAllFields && fic == NULL; i++) {
    //			// TODO Optimization candidate:
    //			if (allFields[i].linkId == linkId) {
    //				fic = &allFields[i];
    //			}
    //		}
    //		if (fic == NULL) {
    //			if (referencedClassId != JAVA_LANG_OBJECT_CLASS_ID) {
    //				// Look in super class:
    //				referencedClassId = getSuperClass(referencedClassId);
    //			} else {
    //				// Already in java.lang.Object; symbol not found.
    //				break;
    //			}
    //		}
    //	}

    if (fic != NULL) {
        *address = fic->address;
        *size = fic->size;
        //				printf("addr = %d, size = %d\n", *address, *size);
    } else {
        consout("Failed to look up field entry: index = %d, classId=%d\n", index, context->classIndex);
        jvmexit(1);
    }
}

//static void obsolete_getFieldEntry(u2 index, u2* address, u1* size) {
//	int i;
//	BOOL found = FALSE;
//	int linkId;
//	int referencedClassId;
//
//	for (i = 0; i < numberOfAllFieldReferences && !found; i++) {
//		if (allFieldReferences[i].index == index && allFieldReferences[i].referencingClassId
//				== context->classIndex) {
//			linkId = allFieldReferences[i].linkId;
//			referencedClassId = allFieldReferences[i].referencedClassId;
//			found = TRUE;
//		}
//	}
//	//	stdPrintfRepaint("Found %s, linkid=%d", found ? "Found" : "Not found", linkId);
//
//	// Now the linkId is established...
//	int fieldId;
//	if (found) {
//		found = FALSE;
//		while (!found) {
//			for (i = 0; i < numberOfAllFields && !found; i++) {
//				if (allFields[i].classId == referencedClassId && allFields[i].linkId == linkId) {
//					found = TRUE;
//					fieldId = i;
//				}
//			}
//			if (!found) {
//				if (referencedClassId != JAVA_LANG_OBJECT_CLASS_ID) {
//					// Look in super class:
//					referencedClassId = getSuperClass(referencedClassId);
//				} else {
//					// Already in java.lang.Object; symbol not found.
//					break;
//				}
//			}
//		}
//	}
//
//	if (found) {
//		const fieldInClass* fic = &allFields[fieldId];
//		*address = fic->address;
//		*size = fic->size;
//		//				printf("addr = %d, size = %d\n", *address, *size);
//	} else {
//		consout("Failed to look up field entry: index = %d, classId=%d\n", index,
//				context->classIndex);
//		jvmexit(1);
//	}
//}

void getStaticFieldEntry(contextDef* context, u2 index, u2* address, u1* size) {
    getFieldEntry(context, index, address, size);
}

void getInstanceFieldEntry(contextDef* context, u2 index, u2* address, u1* size) {
    getFieldEntry(context, index, address, size);
}

/**
 * This method looks up a constant within current class' constant pool
 */
void cpGetConstant(contextDef* context, u2 constantPoolIndex, constantDef* constant) {
    int i;
    BOOL found = FALSE;
    for (i = 0; i < numberOfAllIntegerConstantReferences && !found; i++) {
        if (allIntegerConstantReferences[i].classId == context->classIndex
                && allIntegerConstantReferences[i].constantPoolIndex == constantPoolIndex) {
            found = TRUE;
            constant->type = CONSTANT_INTEGER;
            constant->value.jrenameint = allIntegerConstantReferences[i].value;
        }
    }

    for (i = 0; i < numberOfAllLongConstantReferences && !found; i++) {
        if (allLongConstantReferences[i].classId == context->classIndex
                && allLongConstantReferences[i].constantPoolIndex == constantPoolIndex) {
            found = TRUE;
            constant->type = CONSTANT_LONG;
            constant->value.jlong = allLongConstantReferences[i].value;
        }
    }

    if (!found) {
        for (i = 0; i < numberOfAllStringConstantReferences && !found; i++) {
            if (allStringConstantReferences[i].classId == context->classIndex
                    && allStringConstantReferences[i].constantPoolIndex == constantPoolIndex) {
                found = TRUE;
                constant->type = CONSTANT_STRING;
                constant->value.string = (jchar*) allStringConstantReferences[i].value;
            }
        }
    }

    if (!found) {
        for (i = 0; i < numberOfAllClassReferences && !found; i++) {
            if (allClassReferences[i].classId == context->classIndex
                    && allClassReferences[i].constantPoolIndex == constantPoolIndex) {
                found = TRUE;
                constant->type = CONSTANT_CLASS;
                constant->value.classId = allClassReferences[i].targetClassId;
            }
        }
    }

    if (!found) {
        // TODO support float & String!
        consout("Missing support for float - or wrong index: %d\n", constantPoolIndex);
        jvmexit(1);
    }
}

/**
 * This method looks up a class reference in list of class references
 * \param classId The class reference / id
 */
void cpGetClassReference(contextDef* context, u2 constantPoolIndex, u2* classId) {
    int i;
    BOOL found = FALSE;
    for (i = 0; i < numberOfAllClassReferences && !found; i++) {
        if (allClassReferences[i].constantPoolIndex == constantPoolIndex
                && allClassReferences[i].classId == context->classIndex) {
            found = TRUE;
            *classId = allClassReferences[i].targetClassId;
        }
    }

    if (!found) {
        consout("Failed to look up class reference: index = %d, classId=%d\n", constantPoolIndex, context->classIndex);
        jvmexit(1);
    }
}

///**
// * This method looks up an array type and retrieves the array info
// * \param classId The class id for the array type class
// * \param size The size in bytes of a single array element
// * \param elementClassId The class id of a single array element
// */
//void getArrayInfo(u2 classId, u2* elementClassId, size_t* size) {
//	BEGIN;
//
//	const classInstanceInfoDef* classInfo = getClassInfo(classId);
//
//	*size = classInfo->elementSize;
//	//*elementClassId = classInfo->classId;
//	*elementClassId = classId;
//	END;
//}

BOOL isObjectArray(u2 classId) {
    return getClassInfo(classId)->type == CT_OBJECT_ARRAY ? TRUE : FALSE;
}

BOOL isPrimitiveValueArray(u2 classId) {
    BEGIN
            ;

    CLASS_TYPE type = getClassInfo(classId)->type;

    BOOL isPrimitiveValueArray =
            type == CT_BOOLEAN_ARRAY || type == CT_BYTE_ARRAY || type == CT_CHAR_ARRAY || type == CT_DOUBLE_ARRAY
            || type == CT_FLOAT_ARRAY || type == CT_INT_ARRAY || type == CT_LONG_ARRAY
            || type == CT_SHORT_ARRAY ? TRUE : FALSE;

    END;
    return isPrimitiveValueArray;
}

u2 getArrayElementClassId(u2 classId) {
    return getClassInfo(classId)->elementClassId;
}

u2 getClassIdForClassType(CLASS_TYPE type) {
    int i;
    u2 classId;
    for (i = 0; i < cpNumberOfAllClassInstanceInfo; i++) {
        if (type == allClassInstanceInfo[i].type) {
            classId = i;
            break;
        }
    }

    if (i >= cpNumberOfAllClassInstanceInfo) {
        consoutli("Failed to look up type=%d\n", type);
        jvmexit(1);
    }

    return classId;
}

void cpGenerateJavaLangClassInstances(contextDef* context) {
    // Allocate Class[] containing only those classes that are referenced:
    cpJavaLangClassArray = NewObjectArray(context, (jint) cpNumberOfAllClassInstanceInfo, C_java_lang_Class, NULL);

    if (cpJavaLangClassArray != NULL) {
        // Don't GC our array of classes:
        heapProtect((jobject) cpJavaLangClassArray, TRUE);

        jint i;
        for (i = 0; i < cpNumberOfAllClassInstanceInfo; i++) {
            // Simulate: Class cl  = new Class();
            // TODO NewObject !!!
            // TODO Lazy load?
//            u2 size;
//            getClassSize(C_java_lang_Class, &size);
//            jobject jc = heapAllocObjectByStackableSize(size, C_java_lang_Class);
            jobject jc = newObject(context, C_java_lang_Class);
            if (jc == NULL) {
                // Out of mem
                break;
            }
            SetIntField(context, jc, A_java_lang_Class_aClassId, i);
            SetObjectArrayElement(context, cpJavaLangClassArray, i, jc);
        }

        // Set the aAllClasses in java.lang.Class:
        jclass classInstance = getJavaLangClass(context, C_java_lang_Class);
        SetStaticObjectField(context, classInstance, A_java_lang_Class_aAllClasses, (jobject) cpJavaLangClassArray);

        // Create the list holding global refs:
        jobject globalRefList = slCreateList(context);
        SetStaticObjectField(context, classInstance, A_java_lang_Class_aGlobalRefList, globalRefList);

        // No need of protection:
        heapProtect((jobject) cpJavaLangClassArray, FALSE);
    }
}

jclass getJavaLangClass(contextDef* context, u2 requestedClassId) {
    // TODO Lazy load ?
    return (jclass) GetObjectArrayElement(context, cpJavaLangClassArray, requestedClassId);
}

//void deprecated_generateJavaLangClassInstances() {
//	int i;
//	int j;
//	int arrayLength = 0;
//
//	// Calculate the length of the Class[] to generate:
//	for (i = 0; i < numberOfAllClassReferences; i++) {
//		// The candidate for java.lang.Class generation:
//		u2 classId = allClassReferences[i].targetClassId;
//
//		// Verify that it hasn't been generated before:
//		BOOL found = FALSE;
//		for (j = 0; j < i - 1 && !found; j++) {
//			found = allClassReferences[j].targetClassId == classId;
//		}
//
//		if (!found) {
//			arrayLength++;
//		}
//	}
//
//	consoutli("arrayLength: %d\n", arrayLength);
//	// Allocate Class[] containing only those classes that are referenced:
//	cpJavaLangClassArray = NewObjectArray((jint) arrayLength, C_java_lang_Class, NULL );
//
//	if (cpJavaLangClassArray != NULL ) {
//		// Don't GC our array of classes:
//		heapProtect(cpJavaLangClassArray, TRUE);
//
//		int arrayIndex = 0;
//		u2 aClassIdLinkId = A_java_lang_Class_aClassId;
//
//		jclass classInstance = NULL;
//
//		for (i = 0; i < numberOfAllClassReferences; i++) {
//			// The candidate for java.lang.Class generation:
//			u2 classId = allClassReferences[i].targetClassId;
//
//			// Verify that it hasn't been generated before:
//			BOOL found = FALSE;
//			for (j = 0; j < i - 1 && !found; j++) {
//				found = allClassReferences[j].targetClassId == classId;
//			}
//
//			if (!found) {
//				// The class instance does not exist yet:
//				// Simulate: Class cl  = new Class();
//				u2 size;
//				getClassSize(C_java_lang_Class, &size);
//				jobject jc = heapAllocObjectByStackableSize(size, C_java_lang_Class);
//				SetIntField(jc, aClassIdLinkId, classId);
//				if (ExceptionCheck()) {
//					break;
//				}
//
//				consoutli("class id: %d\n", classId);
//				if (classId == C_java_lang_Class) {
//					classInstance = jc;
//					consoutli("class: \n");
//				}
//				SetObjectArrayElement(cpJavaLangClassArray, arrayIndex, jc);
//				arrayIndex++;
//			}
//		}
//
//		// Set the aAllClasses in java.lang.Class:
//		SetStaticObjectField(classInstance, A_java_lang_Class_aAllClasses, cpJavaLangClassArray);
//	}
//	// consout("class init done\n");
//}

//jclass deprecated_getJavaLangClass(u2 requestedClassId) {
//    BEGIN
//            ;
//    int i;
//    int j;
//    int index = 0;
//
//    // Calculate the index into the array 'javaLangClassArray':
//    for (i = 0; i < numberOfAllClassReferences; i++) {
//        // The candidate for java.lang.Class generation:
//        u2 classId = allClassReferences[i].targetClassId;
//        if (requestedClassId == classId) {
//            break;
//        }
//
//        // Verify that it hasn't been generated before:
//        BOOL found = FALSE;
//        for (j = 0; j < i - 1 && !found; j++) {
//            found = allClassReferences[j].targetClassId == classId;
//        }
//
//        if (!found) {
//            index++;
//        }
//    }
//
//    return (jclass) GetObjectArrayElement(cpJavaLangClassArray, index);
//}

void cpCommonLDC(contextDef* context, u2 constantPoolIndex) {
    // Look up value (int, float or String) from within const pool:
    constantDef constant;
    cpGetConstant(context, constantPoolIndex, &constant);
    if (constant.type == CONSTANT_INTEGER) {
        operandStackPushJavaInt(context, constant.value.jrenameint);
    } else if (constant.type == CONSTANT_LONG) {
        operandStackPushJavaLong(context, constant.value.jlong);
    } else if (constant.type == CONSTANT_STRING) {
        //		consout("%s:%d ldc string: %d\n", __FILE__, __LINE__, constantPoolIndex);
        jobject str = (jobject) NewString(context, constant.value.string);
        operandStackPushObjectRef(context, str);
    } else if (constant.type == CONSTANT_CLASS) {
        //registerNatives skal bygge det Class[], der skal foretages lookup i:
        jclass jc = getJavaLangClass(context, constant.value.classId);
        operandStackPushObjectRef(context, (jobject) jc);
    } else {
        // TODO Support float
        consout("Unsupported type: %d\n", constant.type);
        jvmexit(1);
    }
}

/**
 * This method test if S is 'instanceof' T
 * \param classId_S The class id of S
 * \param classId_T The class id of T
 * \return true, if S is 'instanceof' T; false otherwise.
 */
BOOL CP_IsInstanceOf(u2 classId_S, u2 classId_T) {
    CLASS_TYPE type_T = getClassType(classId_T);
    CLASS_TYPE type_S = getClassType(classId_S);

    BOOL instanceOf;
    if (isObjectArray(classId_S) || isPrimitiveValueArray(classId_S)) {
        if (isObjectArray(classId_T)) {
            if (isObjectArray(classId_S)) {
                // Both are object arrays:
                u2 classId_SC = getArrayElementClassId(classId_S);
                u2 classId_TC = getArrayElementClassId(classId_T);
                instanceOf = CP_IsInstanceOf(classId_SC, classId_TC);
            } else {
                instanceOf = FALSE;
            }
        } else if (isPrimitiveValueArray(classId_T)) {
            instanceOf = (classId_T == classId_S) ? TRUE : FALSE;
        } else if (type_T == CT_CLASS) {
            instanceOf = classId_T == 0 ? TRUE : FALSE;
        } else {
            // S shall implement T:
            if (!is_S_implementing_T(classId_S, classId_T)) {
                instanceOf = FALSE;
            } else {
                instanceOf = TRUE;
            }
            consout("not tested, since arrays does not implement any interfaces ... %d\n", instanceOf);
            jvmexit(1);
        }
    } else if (type_S == CT_CLASS) {
        if (type_T == CT_CLASS) {
            instanceOf = classId_S == classId_T ? TRUE : FALSE;
            if (!instanceOf) {
                instanceOf = is_S_SubClassing_T(classId_S, classId_T);
            }
        } else {
            // S shall implement T:
            if (!is_S_implementing_T(classId_S, classId_T)) {
                instanceOf = FALSE;
            } else {
                instanceOf = TRUE;
            }
        }
    } else if (type_S == CT_INTERFACE) {
        // All objects in this VM has a class reference; so we don't support any classes having a class id which
        // is an interface.
        // This piece of code is not testable nor tested ;-)
        consout("not implemented\n");
        jvmexit(1);
    } else {
        consout("not implemented\n");
        jvmexit(1);
    }

    /*
     *i If S is an ordinary (nonarray) class, then:
     *i    o If T is a class type, then S must be the same class (§2.8.1) as T or a subclass of T.
     *i    o If T is an interface type, then S must implement (§2.13) interface T.
     *i If S is an interface type, then:
     *-    o If T is a class type, then T must be Object (§2.4.7).
     *-    o If T is an interface type, then T must be the same interface as S, or a superinterface of S (§2.13.2).
     *  If S is a class representing the array type SC[], that is, an array of components of type SC, then:
     *i    o If T is a class type, then T must be Object (§2.4.7).
     *i    o If T is an array type TC[], that is, an array of components of type TC, then one of the following must be true:
     *i        + TC and SC are the same primitive type (§2.4.1).
     *i        + TC and SC are reference types (§2.4.6), and type SC can be cast to TC by these runtime rules.
     *(i)  o If T is an interface type, T must be one of the interfaces implemented by arrays (§2.15).
     */
    return instanceOf;
}

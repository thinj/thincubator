/*
 * vmids.h
 *
 *  Created on: Feb 4, 2013
 *      Author: hammer
 */

#ifndef VMIDS_H_
#define VMIDS_H_

//------------------------------------------------------------------------------
// Class referenced by VM
//------------------------------------------------------------------------------
extern const u2 C_java_lang_ArithmeticException;
extern const u2 C_java_lang_ArrayIndexOutOfBoundsException;
extern const u2 C_java_lang_Class;
extern const u2 C_java_lang_ClassCastException;
extern const u2 C_java_lang_IllegalMonitorStateException;
extern const u2 C_java_lang_InterruptedException;
extern const u2 C_java_lang_NegativeArraySizeException;
extern const u2 C_java_lang_NullPointerException;
extern const u2 C_java_lang_Object;
extern const u2 C_java_lang_Object_Monitor;
extern const u2 C_java_lang_Object_WaitElement;
extern const u2 C_java_lang_OutOfMemoryError;
extern const u2 C_java_lang_String;
extern const u2 C_java_lang_Thread;
extern const u2 C_thinj_util_SimpleList;
extern const u2 C_thinj_util_SimpleList_Element;

//------------------------------------------------------------------------------
// Link Ids referenced by VM
//------------------------------------------------------------------------------
extern const u2 M_java_lang_ArithmeticException__init_Ljava_lang_String;
extern const u2 M_java_lang_ArrayIndexOutOfBoundsException__init_I;
extern const u2 A_java_lang_Class_aAllClasses;
extern const u2 A_java_lang_Class_aClassId;
extern const u2 A_java_lang_Class_aGlobalRefList;
extern const u2 M_java_lang_ClassCastException__init;
extern const u2 M_java_lang_IllegalMonitorStateException__init_Ljava_lang_String;
extern const u2 M_java_lang_InterruptedException__init;
extern const u2 M_java_lang_NegativeArraySizeException__init;
extern const u2 M_java_lang_NullPointerException__init;
extern const u2 A_java_lang_Object_aMonitor;
extern const u2 A_java_lang_Object_Monitor_aLockCount;
extern const u2 A_java_lang_Object_Monitor_aOwner;
extern const u2 A_java_lang_Object_Monitor_aWaitSet;
extern const u2 A_java_lang_Object_WaitElement_aNext;
extern const u2 A_java_lang_Object_WaitElement_aWaiting;
extern const u2 M_java_lang_OutOfMemoryError__init;
extern const u2 M_java_lang_OutOfMemoryError_getInstance;
extern const u2 A_java_lang_String_value;
extern const u2 A_java_lang_Thread_aBlockingObject;
extern const u2 A_java_lang_Thread_aCurrentThread;
extern const u2 A_java_lang_Thread_aId;
extern const u2 A_java_lang_Thread_aInterrupted;
extern const u2 A_java_lang_Thread_aLockCount;
extern const u2 A_java_lang_Thread_aState;
extern const u2 A_java_lang_Thread_aWakeUpAt;
extern const u2 M_java_lang_Thread_runFromNative;
extern const u2 A_thinj_util_SimpleList_aList;
extern const u2 A_thinj_util_SimpleList_Element_aNext;
extern const u2 A_thinj_util_SimpleList_Element_aValue;


#endif /* VMIDS_H_ */

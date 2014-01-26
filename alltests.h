
//------------------------------------------------------------------------------
// Class Instance Information
//------------------------------------------------------------------------------
#define C__B 1
#define C__C 2
#define C__I 3
#define C__J 4
#define C__Ljava_lang_Class 5
#define C__Ljava_lang_Object 6
#define C_java_io_PrintStream 7
#define C_java_lang_Error 12
#define C_java_lang_Exception 13
#define C_java_lang_IndexOutOfBoundsException 15
#define C_java_lang_Runnable 22
#define C_java_lang_RuntimeException 23
#define C_java_lang_StringBuilder 25
#define C_java_lang_System 26
#define C_java_lang_Throwable 28
#define C_java_lang_VirtualMachineError 29
#define C_java_util_Arrays 30
#define C_thinj_regression_AllTests 31
#define C_thinj_regression_ThreadTest 32

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
#define A_java_lang_OutOfMemoryError_INSTANCE 26
#define A_java_lang_StringBuilder_aValue 122
#define A_java_lang_System_err 33
#define A_java_lang_System_out 71
#define A_java_lang_Thread_aRunnable 46
#define A_java_lang_Thread_aThreadIdGenerator 47
#define A_java_lang_Throwable_aCause 29
#define A_java_lang_Throwable_aStackTrace 31
#define A_thinj_regression_ThreadTest_aFlag 85
#define A_thinj_regression_ThreadTest_aMutex 87
#define A_thinj_regression_ThreadTest_aCount1 88
#define A_thinj_regression_ThreadTest_aDone 90

//------------------------------------------------------------------------------
// Method Attributes
//------------------------------------------------------------------------------
#define M_java_lang_Object__init 1
#define M_java_lang_Object_getClass 2
#define M_java_lang_Object_toString 6
#define M_java_lang_Object_hashCode 15
#define M_java_io_PrintStream__init 1
#define M_java_io_PrintStream_printLjava_lang_Object 34
#define M_java_io_PrintStream_printlnLjava_lang_String 35
#define M_java_io_PrintStream_printLjava_lang_String 36
#define M_java_io_PrintStream_printI 37
#define M_java_io_PrintStream_outStringLfLjava_lang_String 75
#define M_java_io_PrintStream_outStringLjava_lang_String 76
#define M_java_lang_Class_getName 3
#define M_java_lang_Class_toString 6
#define M_java_lang_Error__init 1
#define M_java_lang_Exception__init 1
#define M_java_lang_Exception__init_Ljava_lang_String 24
#define M_java_lang_IndexOutOfBoundsException__init_Ljava_lang_String 24
#define M_java_lang_NullPointerException__init_Ljava_lang_String 24
#define M_java_lang_OutOfMemoryError__clinit 28
#define M_java_lang_Runnable_run 44
#define M_java_lang_RuntimeException__init 1
#define M_java_lang_RuntimeException__init_Ljava_lang_String 24
#define M_java_lang_String_toString 6
#define M_java_lang_String__init__CII 9
#define M_java_lang_String__init__C 10
#define M_java_lang_String_length 11
#define M_java_lang_String_getCharsII_CI 14
#define M_java_lang_String_hashCode 15
#define M_java_lang_StringBuilder__init 1
#define M_java_lang_StringBuilder_appendLjava_lang_String 4
#define M_java_lang_StringBuilder_toString 6
#define M_java_lang_StringBuilder_appendI 17
#define M_java_lang_System__clinit 28
#define M_java_lang_Thread__init 1
#define M_java_lang_Thread__clinit 28
#define M_java_lang_Thread_run 44
#define M_java_lang_Thread__init_Ljava_lang_Runnable 45
#define M_java_lang_Thread_attach 51
#define M_java_lang_Thread_yield 52
#define M_java_lang_Throwable__init 1
#define M_java_lang_Throwable_toString 6
#define M_java_lang_Throwable__init_Ljava_lang_String 24
#define M_java_lang_Throwable_getStackTraceDepth 30
#define M_java_lang_Throwable_getStackTraceElementI 32
#define M_java_lang_Throwable_printStackTrace 38
#define M_java_lang_VirtualMachineError__init 1
#define M_java_util_Arrays_hashCode_C 7
#define M_thinj_regression_AllTests_main_Ljava_lang_String 72
#define M_thinj_regression_AllTests__jvminit 124
#define M_thinj_regression_ThreadTest__clinit 28
#define M_thinj_regression_ThreadTest_main_Ljava_lang_String 72
#define M_thinj_regression_ThreadTest_simpleYield 91
#define M_thinj_regression_ThreadTest_yieldAndGC 92

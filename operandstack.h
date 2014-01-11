/*
 * operandstack.h
 *
 *  Created on: Aug 22, 2010
 *      Author: hammer
 */

#ifndef OPERANDSTACK_H_
#define OPERANDSTACK_H_

#include <stdlib.h>
#include "types.h"
#include "config.h"
#include "frame.h"


extern stackable* stack;

/**
 * This is the size of the stack counted in stackable entries, not in bytes:
 */
extern size_t STACK_SIZE;

/**
 * \param type
 * \returns The type as a human readable string
 */
const char* StackTypeToString(stackType type);

void pop(contextDef* context, stackable* op);
void push(contextDef* context, stackableOperand OP, stackType TYPE);

//void push(stackableOperand op, stackType type);

void operandStackPushJavaInt(contextDef* context, jint);
void operandStackPushJavaLong(contextDef* context, jlong j);
void operandStackPushJavaString(contextDef* context, char* string);

jlong operandStackPopJavaLong(contextDef* context);
jint operandStackPopJavaInt(contextDef* context);
jobject operandStackPopObjectRef(contextDef* context);


/**
 * \param offset The offset relative to frame pointer
 * \return The jint value at position frame pointer + offset
 */
jint operandStackPeekJavaInt(contextDef* context, int offset);

/**
 * \param offset The offset relative to frame pointer
 * \return The jlong value at position frame pointer + offset
 */
jlong operandStackPeekJavaLong(contextDef* context, int offset);

/**
 * \param offset The offset relative to frame pointer
 * \return The jobject value at position frame pointer + offset
 */
jobject operandStackPeekObjectRef(contextDef* context, int offset);

void operandStackPushU2(contextDef* context, u2 u2);
u2 operandStackPopU2(contextDef* context);

/**
 * This functions pops off (and ignores) 'count' elements from the stack
 */
void operandStackPop(contextDef* context, int count);

void operandStackPushVariableJavaInt(contextDef* context, u1 varnum);
void operandStackPushVariableJavaLong(contextDef* context, u1 varnum);
void operandStackPopVariableJavaLong(contextDef* context, u1 varnum);
void operandStackPushVariableObjectRef(contextDef* context, u1 varnum);

void operandStackPushObjectRef(contextDef* context, jobject jref);

void operandStackPopVariableJavaInt(contextDef* context, u1 varnum);
void operandStackPopVariableJavaLong(contextDef* context, u1 varnum);
void operandStackPopVariableObjectRef(contextDef* context, u1 varnum);

/**
 * This method increments the local variable identified by 'varnum' with
 * the signed value given by 'delta'
 * \param varnum The local variable index
 * \param delta The signed amount to increase with
 */
void operandStackIncrementVariableJavaInt(contextDef* context, u1 varnum, jint delta);

/**
 * This function looks up a stackable relative to the stack pointer
 * \param offset The offset to be added to the stack pointer
 * \param st The address wherein the result shall go
 */
void getOperandRelativeToStackPointer(contextDef* context, s1 offset, stackable* st);

/**
 * This method marks all objects on the current stack
 */
void markStackObjects(u1 mark);

/**
 * This method initializes the stack by writing integer values at all locations.
 * \return The allocated main Java stack
 */
jbyteArray osMainStackInit(contextDef* context, size_t stackSize);

/**
 * This method returns true if and only if the jobject variable # varnum is null
 * \param varnum The number of the variable at the stack. E.g. 'this' is typically #0
 * \return true if and only if the jobject variable # varnum is null
 */
BOOL operandStackIsVariableObjectRefNull(contextDef* context, u1 varnum);

/**
 * This method returns true if and only if the jobject variable at address Sp - offset is null
 * \param offset The offset of the variable at the stack. The offset is subtracted from SP
 * \return true if and only if the jobject is null
 */
BOOL osIsObjectRefAtOffsetNull(contextDef* context, u2 offset);

/**
 * This method returns a pointer to the first element at the stack
 * \return A pointer to the first element at the stack
 */
stackable* getStack();

/**
 * This function allocates a stack on heap and return the allocated stack.
 * \return The allocated stack (as a byte[])
 */
jbyteArray osAllocateStack(contextDef* context);

/**
 * This function returns the current stack. Is only guaranteed to be valid during Thread#clinit().
 * \return A byte[] with the current stack
 */
jbyteArray osGetCurrentStack();

/**
 * This function sets the current stack
 * \param The new jstack
 */
void osSetStack(contextDef* context, jbyteArray jStack);

/**
 * This method unprotects the current stack. Shall only be used to fix a problem with the first stack:
 * The first stack is allocated on the heap, but not referenced from any objects / classes. In order of
 * avoiding GC og this stack, it is initially protected. However, when the java.lang.Thread.<clinit> has
 * been run, this initial stack is begin referenced from Thread.currentThread => it shall no longer be
 * protected. This serves this function.
 */
void osUnprotectStack();

/**
 * This function dumps the current operand stack no stdout
 */
void osDumpStack();

#endif /* OPERANDSTACK_H_ */

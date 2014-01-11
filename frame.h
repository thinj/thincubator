/*
 * frame.h
 *
 *  Created on: Aug 22, 2010
 *      Author: hammer
 */

#ifndef FRAME_H_
#define FRAME_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>
#include "types.h"
#include "operandstack.h"

    /**
     * Flags for context:
     */
    /**
     * When the RETURN_FROM_VM flag is set (happens during pop_frame, if RETURN_FROM_VM_PENDING is set)
     * the execute() - function shall return
     */
#define RETURN_FROM_VM 0x01


    /**
     * This method pops the current frame off the stack
     */
    void pop_frame(contextDef* context);

    /**
     * This method pushes a frame on stack (more or less same as an invoke<address>)
     *
     * \param context A pointer to the contextDef to take frame info
     * \param localVariableCount The number of local variables including method arguments
     * \param dstClassIndex The class id of the class containing the method to call
     * \param dest The address of the method to call
     * \param returnFromVM if TRUE: When a frame is popped (using pop_frame) and this is true,
     * the execute() - function will return; if FALSE: No return will happen from the execute()
     * function
     */
    void push_frame(contextDef* context, u1 localVariableCount, u2 dstClassIndex, codeIndex dest, BOOL returnFromVM);

    u1 getU1FromCode(contextDef* context);
    u2 getU2FromCode(contextDef* context);
    s1 getS1FromCode(contextDef* context);
    s2 getS2FromCode(contextDef* context);

    /**
     * This method single step the program
     */
    void singleStep(void);

    /*
     * This method dumps a stack trace of current (thread)..
     */
    void dumpStackTrace();

    /**
     * This method adds or removes the breakpoint defined by the parameter 'addr'
     * \param addr The address where the breakpoint shall be added or removed
     */
    void toggleBreakpoint(codeIndex addr);

    /**
     * This method returns true if 'addr' is a breakpoint address
     * \param addr The address to test
     */
    BOOL isBreakpoint(codeIndex addr);


    /**
     * This function clears the supplied context and set the class id and start address in the context
     * \param context A pointer to the instance to be initialized
     * \param classId Identification of the class to start execution in
     * \param startAddress Start address of byte code to execute
     */
    void clearContext(contextDef* context, u2 classId, codeIndex startAddress);


#ifdef  __cplusplus
}
#endif

#endif /* FRAME_H_ */

/*
 * config.h
 *
 *  Created on: Jun 23, 2011
 *      Author: hammer
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <stdio.h>
#include "console.h"

#include "thread.h"

// For dumpStackTrace prototype:
#include "frame.h"

#define jvmexit(X) do {if (X) {/*dumpStackTrace();*/}  consout("jvmexit %s %d\n", __FILE__, __LINE__); thinjvm_exit(X);} while (0)



#endif /* CONFIG_H_ */

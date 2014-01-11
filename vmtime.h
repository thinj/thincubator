/*
 * vmtime.h
 *
 *  Created on: Jun 4, 2013
 *      Author: hej
 */

#ifndef VMTIME_H_
#define VMTIME_H_

#include <time.h>
#include "jni.h"

/**
 * \return The uptime measured in nano seconds
 */
jlong vtNanoTime();

/**
 * \returns The time since 1970 in milli seconds
 */
jlong vtCurrentTimeMillis();


#endif /* VMTIME_H_ */

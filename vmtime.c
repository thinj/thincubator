/*
 * vmtime.c
 *
 *  Created on: Jun 4, 2013
 *      Author: hej
 */


#include "vmtime.h"

#include "jni.h"
#include "architecture.h"
//#if ARCH == ARCH_ARM
//#include "blueboard.h"
//#endif


#if ARCH == ARCH_ARM
unsigned long long readNanoTimer();
#endif

jlong vtNanoTime() {
#if ARCH == ARCH_ARM
    return (jlong) readNanoTimer();
#elif ARCH == ARCH_NATIVE
    // see: http://linux.die.net/man/3/clock_gettime
    jlong j = 123456789012LL;
    struct timespec tp;
    // Link using librt:
    if (clock_gettime(CLOCK_MONOTONIC, &tp) == 0) {
        j = 1000000000LL * (jlong) (tp.tv_sec) + (jlong) (tp.tv_nsec);
    }
    return j;
#endif
}

jlong vtCurrentTimeMillis() {
#if ARCH == ARCH_ARM
    // TODO Change to real wall clock
    return (jlong) readNanoTimer() / 1000000;
#elif ARCH == ARCH_NATIVE
    // see: http://linux.die.net/man/3/clock_gettime
    jlong j = 123456789012LL;
    struct timespec tp;
    // Link using librt:
    if (clock_gettime(CLOCK_REALTIME, &tp) == 0) {
        j = 1000LL * (jlong) (tp.tv_sec) + (jlong) (tp.tv_nsec) / 1000000;
    }
    return j;
#endif
}

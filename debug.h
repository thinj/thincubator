/*
 * debug.h
 *
 *  Created on: Jun 28, 2013
 *      Author: hej
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#ifdef DEBUG_ENABLE
#define __DEBUG(format, args...) consout("(%s:%4d) %s: " format "\n",  __FILE__, __LINE__, __FUNCTION__, ## args);
#else
#define __DEBUG(x ...)
#endif

#endif /* DEBUG_H_ */

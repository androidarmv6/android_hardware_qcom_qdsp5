#ifndef _VENC_DEBUG_H
#define _VENC_DEBUG_H
/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/common/inc/venc_debug.h#2 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2009-09-30   as     Corrected Debug message
2009-07-14   bc     Initial revision

 ========================================================================== */

/*========================================================================

                     INCLUDE FILES FOR MODULE

==========================================================================*/
#include <stdio.h>
#include <string.h>

#ifdef _ANDROID_LOG_
#include <utils/Log.h>
#include <utils/threads.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#define LOG_NDEBUG 0
#define LOG_TAG "VENC_ENC"

#define VENC_MSG_HIGH(fmt, a, b, c) LOGE("VENC_MSG_HIGH %s::%d "fmt"\n",__FUNCTION__, __LINE__,(a), (b), (c))
#define VENC_MSG_PROFILE(fmt, a, b, c) LOGE("VENC_MSG_PROFILE %s::%d "fmt"\n",__FUNCTION__, __LINE__,(a), (b), (c))
#define VENC_MSG_ERROR(fmt, a, b, c) LOGE("VENC_ERROR %s::%d "fmt"\n",__FUNCTION__, __LINE__,(a), (b), (c))
#define VENC_MSG_FATAL(fmt, a, b, c) LOGE("VENC_ERROR %s::%d "fmt"\n",__FUNCTION__, __LINE__,(a), (b), (c))

#ifdef _ANDROID_LOG_DEBUG
#define VENC_MSG_MEDIUM(fmt, a, b, c) LOGE("VENC_MSG_MEDIUM %s::%d "fmt"\n",__FUNCTION__, __LINE__,(a), (b), (c))
#else
#define VENC_MSG_MEDIUM(fmt, a, b, c)
#endif

#ifdef _ANDROID_LOG_DEBUG_LOW
#define VENC_MSG_LOW(fmt, a, b, c) LOGE("VENC_MSG_LOW %s::%d "fmt"\n",__FUNCTION__, __LINE__,(a), (b), (c))
#else
#define VENC_MSG_LOW(fmt, a, b, c) 
#endif

#else

#define VENC_MSG_HIGH(fmt, ...) fprintf(stderr, "VENC_MSG_HIGH %s::%d "fmt"\n",__FUNCTION__, __LINE__,## __VA_ARGS__)
#define VENC_MSG_PROFILE(fmt, ...) fprintf(stderr, "VENC_MSG_PROFILE %s::%d "fmt"\n",__FUNCTION__, __LINE__,## __VA_ARGS__)
#define VENC_MSG_ERROR(fmt, ...) fprintf(stderr, "VENC_ERROR %s::%d "fmt"\n",__FUNCTION__, __LINE__,## __VA_ARGS__)
#define VENC_MSG_FATAL(fmt, ...) fprintf(stderr, "VENC_ERROR %s::%d "fmt"\n",__FUNCTION__, __LINE__,## __VA_ARGS__)

#ifdef  VENC_MSG_LOG_DEBUG
#define VENC_MSG_LOW(fmt, ...) fprintf(stderr, "VENC_MSG_LOW %s::%d "fmt"\n",__FUNCTION__, __LINE__,## __VA_ARGS__)
#define VENC_MSG_MEDIUM(fmt, ...) fprintf(stderr, "VENC_MSG_MEDIUM %s::%d "fmt"\n",__FUNCTION__, __LINE__,## __VA_ARGS__)
#else
#define VENC_MSG_LOW(fmt, a, b, c) 
#define VENC_MSG_MEDIUM(fmt, a, b, c)
#endif

#endif //#ifdef _ANDROID_LOG_

#endif // #ifndef _VENC_DEBUG_H

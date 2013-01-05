#ifndef OMX_VENC_DEBUG_H
#define OMX_VENC_DEBUG_H
/*============================================================================
@file OMX_VencDebug.h

Debug message macros

                     Copyright (c) 2008 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
============================================================================*/

/*----------------------------------------------------------------------------
 *
 * $Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/omx/inc/OMX_VencDebug.h#5 $ 
 *
 * when         who     what, where, why
 * ----------   ---     ----------------------------------------------------
 * 2009-09-30   as      Corrected Debug message
 * 2009-28-09   as      Changed the debug TAG 
 * 2009-21-05   as      Changed message to LOGE
 * 2008-12-03   bc      Enabled diag
 * 2007-10-10   bc      Created file.
 *
 * -------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * Include Files
 * -------------------------------------------------------------------------*/

#include <stdio.h>


#ifdef _ANDROID_LOG_
#define LOG_NDEBUG 0
#include <utils/Log.h>
#include <utils/threads.h>
#include <sys/prctl.h>
#include <sys/resource.h>

#define OMX_VENC_MSG_HIGH(fmt, a, b, c) LOGE("OMX_VENC_MSG_HIGH %s::%d "fmt"\n",__FUNCTION__, __LINE__,(a), (b), (c))
#define OMX_VENC_MSG_ERROR(fmt, a, b, c) LOGE("OMX_VENC_MSG_ERROR %s::%d "fmt"\n",__FUNCTION__, __LINE__,(a), (b), (c))

#ifdef _ANDROID_LOG_DEBUG
#define OMX_VENC_MSG_MEDIUM(fmt, a, b, c) LOGE("OMX_VENC_MSG_MEDIUM %s::%d "fmt"\n",__FUNCTION__, __LINE__,(a), (b), (c))
#else 
#define OMX_VENC_MSG_MEDIUM(fmt, a, b, c)
#endif

#ifdef _ANDROID_LOG_DEBUG_LOW
#define OMX_VENC_MSG_LOW(fmt, a, b, c) LOGE("OMX_VENC_MSG_LOW %s::%d "fmt"\n",__FUNCTION__, __LINE__,(a), (b), (c))
#else 
#define OMX_VENC_MSG_LOW(fmt, a, b, c)
#endif

#else

#define OMX_VENC_MSG_HIGH(fmt, ...) fprintf(stderr, "OMX_VENC_MSG_HIGH %s::%d "fmt"\n",__FUNCTION__, __LINE__,## __VA_ARGS__)
#define OMX_VENC_MSG_ERROR(fmt, ...) fprintf(stderr, "OMX_VENC_MSG_ERROR %s::%d "fmt"\n",__FUNCTION__, __LINE__,## __VA_ARGS__)

#ifdef VENC_MSG_LOG_DEBUG 
#define OMX_VENC_MSG_MEDIUM(fmt, ...) fprintf(stderr, "OMX_VENC_MSG_MEDIUM %s::%d "fmt"\n",__FUNCTION__, __LINE__,## __VA_ARGS__)
#define OMX_VENC_MSG_LOW(fmt, ...) fprintf(stderr, "OMX_VENC_MSG_LOW %s::%d "fmt"\n",__FUNCTION__, __LINE__,## __VA_ARGS__)
#else
#define OMX_VENC_MSG_MEDIUM(fmt, a, b, c)
#define OMX_VENC_MSG_LOW(fmt, a, b, c)

#endif
#endif
#endif // #ifndef OMX_VENC_MSG_Q_H

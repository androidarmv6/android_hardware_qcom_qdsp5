/* Copyright (c) 2007 by QUALCOMM, Incorporated.  All Rights Reserved. 
** QUALCOMM Proprietary and Confidential.
*/

#ifndef QTV_MSG_H
#define QTV_MSG_H

#include <string.h>
#include <stdio.h>


/* Usage of QTV_MSG, QTV_MSG_PRIO, etc
 * QTV_MSG(<QTV Message Sub-Group Id>, <message>)
 * QTV_MSG_PRIOx(<QTV Message Sub-Group Id>, <QTV Message Priorities>, <message>, args)
 */

/* QTV Message Sub-Group Ids */
#define QTVDIAG_GENERAL           0 
#define QTVDIAG_DEBUG             1 
#define QTVDIAG_STATISTICS        2
#define QTVDIAG_UI_TASK           3
#define QTVDIAG_MP4_PLAYER        4
#define QTVDIAG_AUDIO_TASK        5
#define QTVDIAG_VIDEO_TASK        6
#define QTVDIAG_STREAMING         7
#define QTVDIAG_MPEG4_TASK        8
#define QTVDIAG_FILE_OPS          9
#define QTVDIAG_RTP               10
#define QTVDIAG_RTCP              11
#define QTVDIAG_RTSP              12
#define QTVDIAG_SDP_PARSE         13
#define QTVDIAG_ATOM_PARSE        14
#define QTVDIAG_TEXT_TASK         15
#define QTVDIAG_DEC_DSP_IF        16
#define QTVDIAG_STREAM_RECORDING  17
#define QTVDIAG_CONFIGURATION     18
#define QTVDIAG_BCAST_FLO         19
#define QTVDIAG_GENERIC_BCAST     20


/* QTV Message Priorities */
#ifdef _ANDROID_
    	#define LOG_TAG "QCvdec"
    	#include <utils/Log.h>
	#ifdef VDEC_LOG_DEBUG    // should be LOGI but LOGI is suppressed in some builds so keeping LOGW
		#define LOG_QTVDIAG_PRIO_LOW(a...)		LOGW(a)
		#define LOG_QTVDIAG_PRIO_MED(a...)     		LOGW(a)
		#define LOG_QTVDIAG_PRIO_DEBUG(a...)  		LOGW(a)
  		#define DEBUG_PRINT(a...)       		LOGW(a)
	#else
		#define LOG_QTVDIAG_PRIO_LOW(a...)  	while(0) {}   
		#define LOG_QTVDIAG_PRIO_MED(a...)  	while(0) {}   
		#define LOG_QTVDIAG_PRIO_DEBUG(a...)	while(0) {}
  		#define DEBUG_PRINT(a...)       	while(0) {}
	#endif

	#ifdef VDEC_LOG_HIGH
		#define LOG_QTVDIAG_PRIO_HIGH(a...)   	LOGW(a)
	#else
		#define LOG_QTVDIAG_PRIO_HIGH(a...)	while(0) {}
	#endif

	#ifdef VDEC_LOG_ERROR
		#define LOG_QTVDIAG_PRIO_ERROR(a...)   	LOGE(a)
		#define LOG_QTVDIAG_PRIO_FATAL(a...)   	LOGE(a)
		#define DEBUG_PRINT_ERROR(a...)		LOGE(a)
	#else
		#define LOG_QTVDIAG_PRIO_ERROR(a...)	while(0) {}
		#define LOG_QTVDIAG_PRIO_FATAL(a...)	while(0) {}
		#define DEBUG_PRINT_ERROR(a...)		while(0) {}
	#endif

	#ifdef PROFILE_DECODER       //should be LOGI but keeping LOGE
		#define QTV_PERF(a)
		#define QTV_PERF_MSG_PRIO(a,b,c...)     LOGW(c)
		#define QTV_PERF_MSG_PRIO1(a,b,c...)    LOGW(c)
		#define QTV_PERF_MSG_PRIO2(a,b,c...)    LOGW(c)
		#define QTV_PERF_MSG_PRIO3(a,b,c...)    LOGW(c)
	#else
		#define QTV_PERF(a)
		#define QTV_PERF_MSG_PRIO(a,b,c)		while(0){}
		#define QTV_PERF_MSG_PRIO1(a,b,c,d) 		while(0){}
		#define QTV_PERF_MSG_PRIO2(a,b,c,d,e) 		while(0){}
		#define QTV_PERF_MSG_PRIO3(a,b,c,d,e,f) 	while(0){}
	#endif

#else
	#ifdef VDEC_LOG_DEBUG
		#define LOG_QTVDIAG_PRIO_LOW(a...)	printf(a)  	   
		#define LOG_QTVDIAG_PRIO_MED(a...)  	printf(a)   
		#define LOG_QTVDIAG_PRIO_DEBUG(a...)	printf(a)
  		#define DEBUG_PRINT(a...)       	printf(a)
	#else
		#define LOG_QTVDIAG_PRIO_LOW(a...)  	while(0) {}   
		#define LOG_QTVDIAG_PRIO_MED(a...)  	while(0) {}   
		#define LOG_QTVDIAG_PRIO_DEBUG(a...)	while(0) {}
  		#define DEBUG_PRINT(a...)       	while(0) {}
	#endif
	
	#ifdef VDEC_LOG_HIGH
		#define LOG_QTVDIAG_PRIO_HIGH(a...)	printf(a) 
	#else
		#define LOG_QTVDIAG_PRIO_HIGH(a...)	while(0) {}
	#endif
	
	#ifdef VDEC_LOG_ERROR
		#define LOG_QTVDIAG_PRIO_ERROR(a...)	printf(a)
		#define LOG_QTVDIAG_PRIO_FATAL(a...)	printf(a)
		#define DEBUG_PRINT_ERROR(a...)		printf(a)
	#else
		#define LOG_QTVDIAG_PRIO_ERROR(a...)	while(0) {}
		#define LOG_QTVDIAG_PRIO_FATAL(a...)	while(0) {}
		#define DEBUG_PRINT_ERROR(a...)		while(0) {}
	#endif	
		
	#ifdef PROFILE_DECODER
		#define QTV_PERF(a)
		#define QTV_PERF_MSG_PRIO(a,b,c...)           printf(c)
		#define QTV_PERF_MSG_PRIO1(a,b,c...)          printf(c)
		#define QTV_PERF_MSG_PRIO2(a,b,c...)          printf(c)
		#define QTV_PERF_MSG_PRIO3(a,b,c...)          printf(c)
	#else
		#define QTV_PERF(a)
		#define QTV_PERF_MSG_PRIO(a,b,c)		while(0){}
		#define QTV_PERF_MSG_PRIO1(a,b,c,d) 		while(0){}
		#define QTV_PERF_MSG_PRIO2(a,b,c,d,e) 		while(0){}
		#define QTV_PERF_MSG_PRIO3(a,b,c,d,e,f) 	while(0){}
	#endif
#endif 

#define QTV_MSG_PRIO(a,b,c)                   LOG_##b(c)
#define QTV_MSG_PRIO1(a,b,c,d)                LOG_##b((c),(d))
#define QTV_MSG_PRIO2(a,b,c,d,e)              LOG_##b((c),(d),(e))
#define QTV_MSG_PRIO3(a,b,c,d,e,f)            LOG_##b((c),(d),(e),(f))
#define QTV_MSG_PRIO4(a,b,c,d,e,f,g)          LOG_##b((c),(d),(e),(f),(g))
#define QTV_MSG_PRIO5(a,b,c,d,e,f,g,h)        LOG_##b((c),(d),(e),(f),(g),(h))
#define QTV_MSG_PRIO6(a,b,c,d,e,f,g,h,i)      LOG_##b((c),(d),(e),(f),(g),(h),(i))
#define QTV_MSG_PRIO7(a,b,c,d,e,f,g,h,i,j)    LOG_##b((c),(d),(e),(f),(g),(h),(i),(j))
#define QTV_MSG_PRIO8(a,b,c,d,e,f,g,h,i,j,k)  LOG_##b((c),(d),(e),(f),(g),(h),(i),(j),(k))

#define ERR(a, b, c, d)  				while(0){}
#define ERR_FATAL(a, b, c, d) 				while(0){}

#define MSG_ERROR(a, b) 				while(0){}
#define MSG_ERROR4(a, b, c, d) 				while(0){}

#define QTV_MSG(a,b)                   while(0) {}
#define QTV_MSG1(a,b,c)                while(0) {}
#define QTV_MSG2(a,b,c,d)              while(0) {}
#define QTV_MSG3(a,b,c,d,e)            while(0) {}
#define QTV_MSG4(a,b,c,d,e,f)          while(0) {}

#if !defined(ASSERT)
	#define ASSERT(x) if (!(x)) {char *pp=0; QTV_MSG_PRIO2(QTVDIAG_VIDEO_TASK,QTVDIAG_PRIO_FATAL,"%s:%d *** ERROR ASSERT(0)\n", __FILE__, __LINE__); *pp=0;}  /*use this to crash at assert*/
#endif

#if !defined(ASSERT_FATAL)
	#define ASSERT_FATAL(x) if (!(x)) {char *pp=0; QTV_MSG_PRIO2(QTVDIAG_VIDEO_TASK,QTVDIAG_PRIO_FATAL,"%s:%d *** ERROR ASSERT(0)\n", __FILE__, __LINE__); *pp=0;}
#endif

#endif // QTV_MSG_H

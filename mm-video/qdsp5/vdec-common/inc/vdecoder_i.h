#ifndef _VDECODER_I_H_
#define _VDECODER_I_H_
/* =======================================================================
                               vdecoder_i.h
DESCRIPTION
  Here we define the base class for all media-specific decoders.


Copyright (c) 2007 by QUALCOMM, Incorporated.  All Rights Reserved. 
QUALCOMM Proprietary and Confidential.
========================================================================== */

/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/VdecCommon/main/latest/src/vdecoder_i.h#4 $
$DateTime: 2008/10/15 10:15:15 $
$Change: 763219 $


                     INCLUDE FILES FOR MODULE

========================================================================== */

#include "vdecoder_types.h"

/* ==========================================================================

                        DATA DECLARATIONS

========================================================================== */
/* -----------------------------------------------------------------------
** Constant / Define Declarations
** ----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
** Type Declarations
** ----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
** Global Constant Data Declarations
** ----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
** Global Data Declarations
** ----------------------------------------------------------------------- */

/* =======================================================================
**                          Macro Definitions
** ======================================================================= */

/* =======================================================================
MACRO MYOBJ

ARGS
  xx_obj - this is the xx argument

DESCRIPTION:
  Complete description of what this macro does
========================================================================== */

/* =======================================================================
**                        Function Declarations
** ======================================================================= */

/* ======================================================================
FUNCTION
  SAMPLE_FUNC

DESCRIPTION
  Thorough, meaningful description of what this function does.

DEPENDENCIES
  List any dependencies for this function, global variables, state,
  resource availability, etc.

RETURN VALUE
  Enumerate possible return values

SIDE EFFECTS
  Detail any side effects.

========================================================================== */

/* =======================================================================
**                        Class Declarations
** ======================================================================= */

/* ======================================================================
CLASS
  VideoDecoder

DESCRIPTION
  Abstract base class for all video decoders.  
========================================================================== */
class VideoDecoder
{
public:
	typedef VDEC_ERROR (VideoDecoder::*FN_PTR_INITIALIZE)(
							const unsigned short,
							VDEC_MALLOC_FN,
							VDEC_FREE_FN,
							VDEC_EVENT_CB_FN,
							void * const,
							VDEC_DIMENSIONS &,
							VDEC_CONCURRENCY_CONFIG,
							VDEC_SLICE_BUFFER_INFO*,
							unsigned,
							int
							);
 typedef VDEC_ERROR (VideoDecoder::*FN_PTR_REUSE_FRAME_BUFFER)(VDEC_FRAME * const);
 typedef VDEC_ERROR (VideoDecoder::*FN_PTR_FLUSH)(int*);
 typedef VDEC_ERROR (VideoDecoder::*FN_PTR_EOS)(void);
 typedef VDEC_ERROR (VideoDecoder::*FN_PTR_SUSPEND)(void);
 typedef VDEC_ERROR (VideoDecoder::*FN_PTR_RESUME)(VDEC_CONCURRENCY_CONFIG);
 typedef VDEC_ERROR (VideoDecoder::*FN_PTR_DECODE)(VDEC_INPUT_BUFFER*, bool);
 typedef VDEC_ERROR (VideoDecoder::*FN_PTR_SET_PARAMETER)(VDEC_PARAMETER_ID,VDEC_PARAMETER_DATA* const);
 typedef VDEC_ERROR (VideoDecoder::*FN_PTR_GET_PARAMETER)(VDEC_PARAMETER_ID,VDEC_PARAMETER_DATA*);
 typedef void (VideoDecoder::*FN_PTR_DESTRUCTOR)(void);

protected:
  FN_PTR_INITIALIZE fnPtrInitialize;
  FN_PTR_REUSE_FRAME_BUFFER fnPtrReuseFrameBuffer;
  FN_PTR_FLUSH fnPtrFlush;
  FN_PTR_EOS fnPtrEOS;
  FN_PTR_SUSPEND fnPtrSuspend;
  FN_PTR_RESUME fnPtrResume;
  FN_PTR_DECODE fnPtrDecode;
  FN_PTR_SET_PARAMETER fnPtrSetParameter;
  FN_PTR_GET_PARAMETER fnPtrGetParameter;
  FN_PTR_DESTRUCTOR fnPtrDestructor;
  //=========================================================================

  /* Information about the 'frame-available' callback.  All subclasses
  ** use this, so it's stored centrally here.
  */
  VDEC_EVENT_CB_FN         m_pEventCb;
  void *                   m_pEventCbData;
  unsigned short           m_nLayers;
  VDEC_MALLOC_FN           m_pfnFrameBufferMalloc;
  VDEC_FREE_FN             m_pfnFrameBufferFree;
  VDEC_DIMENSIONS          m_frameSize;
  VDEC_CONCURRENCY_CONFIG  m_concurrencyConfig;
  unsigned char                  m_bDeblockingEnable;
  unsigned char				   m_bExternalClock;	
  bool                     m_bDecoderInitialized;

public:
 typedef
 VideoDecoder* (*VDEC_CREATION_FUNC)(VDEC_ERROR * const  pErr_out);
 //Constructor
 VideoDecoder( void );
 //Destructor
 ~VideoDecoder( void );
  // Subclasses are required to implement Following Functions;
 // currently there is no common behavior for the base class
 VDEC_ERROR Initialize
 (
    const unsigned short            nLayers,
    VDEC_MALLOC_FN                  pfnFrameBufferMalloc,
    VDEC_FREE_FN                    pfnFrameBufferFree,
    VDEC_EVENT_CB_FN                pEventCb,
    void * const                    pEventCbData,
    VDEC_DIMENSIONS                &frameSize,
    VDEC_CONCURRENCY_CONFIG         concurrencyConfig,
    VDEC_SLICE_BUFFER_INFO *        slice_buffer_info,
    unsigned			    numOutBuffers,
    int 			    adsp_fd
	);
 VDEC_ERROR ReuseFrameBuffer( VDEC_FRAME * const pFrame );
 VDEC_ERROR Flush( int *nFlushedFrames );
 VDEC_ERROR EOS( void );
 VDEC_ERROR Suspend( void );
 VDEC_ERROR Resume( VDEC_CONCURRENCY_CONFIG concurrencyConfig);
 VDEC_ERROR Decode( VDEC_INPUT_BUFFER * input , bool);
 VDEC_ERROR SetParameter( VDEC_PARAMETER_ID     parameter,
						  VDEC_PARAMETER_DATA * const info );
  VDEC_ERROR GetParameter( VDEC_PARAMETER_ID parameter,
	                       VDEC_PARAMETER_DATA *info ); 
};

#endif /* _VDECODER_I_H_ */

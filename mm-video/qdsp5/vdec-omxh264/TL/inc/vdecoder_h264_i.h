#ifndef _VDECODER_H264_I_H_
#define _VDECODER_H264_I_H_
/* =======================================================================
                               vdecoder_h264_i.h
DESCRIPTION
  Definition of the H264-specific VideoDecoder.

Copyright (c) 2007 by QUALCOMM, Incorporated.  All Rights Reserved. 
QUALCOMM Proprietary and Confidential.
========================================================================== */

/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/H264Vdec/main/latest/inc/vdecoder_h264_i.h#6 $
$DateTime: 2009/10/14 18:22:58 $
$Change: 1053988 $

========================================================================== */

/* =======================================================================
**               Includes and Public Data Declarations
** ======================================================================= */

/* ==========================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */
#include "vdecoder_i.h"
#include "h264_pal.h"
#include "h264_types.h"
#include "frame_buffer_pool.h"

/* ==========================================================================

                        DATA DECLARATIONS

========================================================================== */
/* -----------------------------------------------------------------------
** Constant / Define Declarations
** ----------------------------------------------------------------------- */
//#define OMM_PERFORMANCE_STATS

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
  H264Decoder

DESCRIPTION
  Decoder for H264-derivatives.

========================================================================== */
class H264Decoder : public VideoDecoder
{
public:
  // VDEC-registry stuff...
  //
  static const char* const        sm_ftypsSupported;

  static VideoDecoder* Create
  (
   VDEC_ERROR * const              pErr_out
  );


  H264Decoder
  (
  VDEC_ERROR * const              pErr_out
  );

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
  unsigned			  numOutBuffers,
  int 				  adsp_fd
  );


  VDEC_ERROR SetParameter( VDEC_PARAMETER_ID           parameter,
                                   VDEC_PARAMETER_DATA * const info );
  VDEC_ERROR GetParameter( VDEC_PARAMETER_ID           parameter,
                                   VDEC_PARAMETER_DATA *       info );

  ~H264Decoder( void );

  void DestroyH264Decoder(void);
  // Serialization functions
  //
  
  // We override Flush to allow us to clean up the H264 layer beneath.
  //
  VDEC_ERROR Flush( int *nFlushedFrames );

  VDEC_ERROR EOS( void );

  VDEC_ERROR Decode( VDEC_INPUT_BUFFER* , bool);

  VDEC_ERROR ReuseFrameBuffer(VDEC_FRAME * const pFrame);


  // We override Suspend and Resume.
  VDEC_ERROR Suspend( void );

  VDEC_ERROR Resume( VDEC_CONCURRENCY_CONFIG concurrencyConfig);

private:
  /* Malloc and free routines to be passed into H264 implementation.  These
   * routines should allocate from the system heap.
   */
  static void* malloc(unsigned int bytes);
  static void free(void *p);

  /* Callbacks which allow us to handle callbacks from the H264 decoder
  ** which notify us of DSP events.  The lack of any user-data to these
  ** callbacks forces us to be a singleton, so that'll have to be fixed.
  */
  static void DecodeDoneCallback(
                                VDL_Decode_Return_Type returnCode,    /* return status code */
                                void *pDecodeStats,  /* return decode frame statistics */
                                void *pUserData      /* user data.  Unused by us */
                                );


  /* The DecodeDoneCallback just turns its user data pointer into a pThis
  ** pointer and calls our member handler, which does all the real work.
  */
  void DecodeDoneHandler( VDL_Decode_Return_Type returnCode,    /* return status code */
                          void*                 pDecodeStats );


  /* Because we have static members that need explicit initialization and
  ** destruction, we need to keep a counter of the number of H264Decoders
  ** floating around out there.  Not useful initially, but when we add
  ** support for multiple simultaneous decoders this'll matter.
  */
  static unsigned int sm_numDecoders;

  /* Member functions to initialize said statics when the first decoder
  ** starts and destroy them after the last decoder quits.
  */
  void DoStaticConstruction( void );
  void DoStaticDestruction( void );

  /* An internal state which helps us keep track of the DSP.
  */
  enum
  {
    VDEC_STATE_ERROR         = -1,
    VDEC_STATE_UNINITIALIZED,
    VDEC_STATE_INITIALIZED,
    VDEC_STATE_DESTRUCTING
  }
  m_state;

  /* Video stream configuration information
  */

private:

  //
  //=========================================================================
  // Frame buffer allocation
  //
private:

  //
  //=========================================================================

  // Stuff that used to be allocated in MP4CodecInfo, but which is only used by
  // this decoder (and so shouldn't be as 'global' as MP4CodecInfo...)

#ifndef FEATURE_QTV_MDP
  /* This YUV buffer pointer is used to send YUV buffer to the decoder in
  ** the case of a Post Processing only DSP (currently used for H.264)
  */
  unsigned char** m_YUVPtrs;

#endif /* ifndef FEATURE_QTV_MDP */

private:
  VDEC_ERROR InitializeDecInternal(void);
  unsigned long int m_numFramesDecodeDone;
  void* m_TL;                              // TL Instance 
  TL_ERROR m_TLErrorCode;
  void*  m_pDL;                            // DL Instance
  H264_PAL *m_pH264PL_Instance;            // PL Instance


#ifdef OMM_PERFORMANCE_STATS
  omm::CmSmartPtr<omm::ImMediaClock> m_pClock;
#endif /* OMM_PERFORMANCE_STATS */
};

#endif /* _VDECODER_H264_I_H_ */


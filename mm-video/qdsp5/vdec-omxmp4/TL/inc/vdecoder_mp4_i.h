#ifndef _VDECODER_MP4_I_H_
#define _VDECODER_MP4_I_H_
/* =======================================================================
                               vdecoder_mp4_i.h
DESCRIPTION
  Definition of the MP4-specific VideoDecoder.

Copyrighted by QUALCOMM Incorporated;
Copyright 2003 QUALCOMM Incorporated, All Rights Reserved
========================================================================== */

/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/MPEG4Vdec/main/latest/inc/vdecoder_mp4_i.h#2 $
$DateTime: 2009/03/03 14:14:44 $
$Change: 853763 $

========================================================================== */

/* =======================================================================
**               Includes and Public Data Declarations
** ======================================================================= */

/* ==========================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */
#include "vdecoder_i.h"
#include "MP4_TL.h"

/* ==========================================================================

                        DATA DECLARATIONS

========================================================================== */
/* -----------------------------------------------------------------------
** Constant / Define Declarations
** ----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
** Type Declarations
** ----------------------------------------------------------------------- */
typedef MP4VOLType VDEC_MP4_VOL;

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
  MP4Decoder

DESCRIPTION
  Decoder for MP4-derivatives.

========================================================================== */
class MP4Decoder : public VideoDecoder
{
public:

  static VideoDecoder * Create
  (
    VDEC_ERROR * const              pErr_out
  );

  MP4Decoder
  (
    VDEC_ERROR * const              pErr_out
  );
MP4Decoder();

  ~MP4Decoder( void );

  // Serialization functions
  //
  // We override Flush to allow us to clean up the MP4 layer beneath.
  //
  void DestroyMP4Decoder(void);
  VDEC_ERROR Flush( int *nFlushedFrames );
  #ifdef FEATURE_MPEG4_VLD_DSP
  VDEC_ERROR Queue_eOS(void);
  #endif

  VDEC_ERROR Decode( VDEC_INPUT_BUFFER * input, bool);

  // We override Suspend and Resume & Scale.
  VDEC_ERROR Suspend( void );

  VDEC_ERROR Resume( VDEC_CONCURRENCY_CONFIG concurrencyConfig);

#ifdef FEATURE_QTV_DECODER_XSCALE_VIDEO
  VDEC_ERROR Scale( VDEC_DIMENSIONS outFrameSize,
                            VDEC_SCALE_CB_FN scaleCbFn,
                            void * const     pCbData );
#endif //FEATURE_QTV_DECODER_XSCALE_VIDEO

  VDEC_ERROR Initialize
  (
  const unsigned short     nLayers,
  VDEC_MALLOC_FN           pfnFrameBufferMalloc,
  VDEC_FREE_FN             pfnFrameBufferFree,
  VDEC_EVENT_CB_FN         pEventCb,
  void * const             pEventCbData,
  VDEC_DIMENSIONS          &frameSize,
  VDEC_CONCURRENCY_CONFIG   concurrencyConfig,
  VDEC_SLICE_BUFFER_INFO   *slice_buffer_info,
  unsigned 		    numOutBuffers,
  int 			    adsp_fd
  );
  VDEC_ERROR SetParameter( VDEC_PARAMETER_ID           parameter,
                                   VDEC_PARAMETER_DATA * const info );
  VDEC_ERROR GetParameter( VDEC_PARAMETER_ID           parameter,
                                   VDEC_PARAMETER_DATA *       info );

  VDEC_ERROR ReuseFrameBuffer(VDEC_FRAME * const pFrame);
  VDEC_ERROR EOS (void); 

protected:
  /* Callbacks which allow us to handle callbacks from the MP4 decoder
  ** which notify us of DSP events.  The lack of any user-data to these
  ** callbacks forces us to be a singleton, so that'll have to be fixed.
  */
  static void DecodeDoneCallback(   VDL_Decode_Return_Type returnCode,    /* return status code */
  void *pDecodeStats,  /* return decode frame statistics */
  void *pUserData      /* user data.  Unused by us */ );

  /* The DecodeDoneCallback just turns its user data pointer into a pThis
  ** pointer and calls our member handler, which does all the real work.
  */
  void DecodeDoneHandler(   VDL_Decode_Return_Type returnCode,    /* return status code */
  void*                 pDecodeStats );

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
  //=========================================================================
  // Frame buffer allocation
  //
protected:
  MP4_TL *m_pTL;
  signed short m_ErrorCode;
  //
  //=========================================================================

};

#endif /* _VDECODER_MP4_I_H_ */


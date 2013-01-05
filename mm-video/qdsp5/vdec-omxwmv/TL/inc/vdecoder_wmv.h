#ifndef _VDECODER_WMV_H_
#define _VDECODER_WMV_H_
/* =======================================================================
                               vdecoder_wmv.h
DESCRIPTION
  Definition of the WMV-specific VideoDecoder.

Copyrighted by QUALCOMM Incorporated;
Copyright 2003 QUALCOMM Incorporated, All Rights Reserved
========================================================================== */

/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/Wmv9Vdec/main/latest/inc/vdecoder_wmv.h#2 $
$DateTime: 2009/04/14 19:05:45 $
$Change: 887518 $

========================================================================== */

/* =======================================================================
**               Includes and Public Data Declarations
** ======================================================================= */

/* ==========================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */
#include "vdecoder_i.h"
#include "frame_buffer_pool.h"
#include "VDL_Types.h"
#include "TL_types.h"
#include "MP4_Types.h"
//#include "QCUtils.h"

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

#ifndef MAKEFOURCC_WMV
#define MAKEFOURCC_WMV(ch0, ch1, ch2, ch3) \
        ((U32_WMV)(U8_WMV)(ch0) | ((U32_WMV)(U8_WMV)(ch1) << 8) |   \
        ((U32_WMV)(U8_WMV)(ch2) << 16) | ((U32_WMV)(U8_WMV)(ch3) << 24 ))

#define mmioFOURCC_WMV(ch0, ch1, ch2, ch3)  MAKEFOURCC_WMV(ch0, ch1, ch2, ch3)
#endif

/* The number of VOL-header bytes we'll encapsulate in a BLOB and hand
** to the windows-init code
*/
#define WMV_SEQUENCE_HEADER_BYTES   4
#define FOURCC_WMV3     mmioFOURCC_WMV('W','M','V','3')
#define FOURCC_WMV2     mmioFOURCC_WMV('W','M','V','2')
#define FOURCC_WMV1     mmioFOURCC_WMV('W','M','V','1')
#define FOURCC_I420     0x30323449

typedef enum tagWMCDecodeDispRotateDegree
{
    WMC_DispRotate0 = 0,
    WMC_DispRotate90,
    WMC_DispFlip,
    WMC_DispRotate270
} tWMCDecodeDispRotateDegree;

/* =======================================================================
**                        Class Declarations
** ======================================================================= */

/* ======================================================================
CLASS
  WMVDecoder

DESCRIPTION
  Decoder for WMV-derivatives.

========================================================================== */
class WMVDecoder : public VideoDecoder
{
public:
  /* VDEC-registry stuff
  */
  static const char* const        sm_ftypsSupported;

  /*static VDEC_ERROR ParseHeader( const char * const      fourcc,
                                 const VDEC_BYTE * const pBuf,
                                 const unsigned long     bufSize,
                                 unsigned long * const   pBufConsumed_out,
                                 VDEC_BLOB ** const      ppConfig_out );
  */
  static VideoDecoder* Create
  (
    VDEC_ERROR * const              pErr_out
  );
    
  WMVDecoder
  (
    VDEC_ERROR * const              pErr_out
  );

  VDEC_ERROR Initialize
  (
    // VDEC_BLOB * const               pConfig,
    const unsigned short            nLayers,
    VDEC_MALLOC_FN                  pfnFrameBufferMalloc,
    VDEC_FREE_FN                    pfnFrameBufferFree,
    VDEC_EVENT_CB_FN                pEventCb,
    void * const                    pEventCbData,
    VDEC_DIMENSIONS                &frameSize,
    VDEC_CONCURRENCY_CONFIG         concurrencyConfig,
    VDEC_SLICE_BUFFER_INFO *        slice_buffer_info,
    unsigned			    numOutBuffers,
    int                             adsp_fd
  );


  VDEC_ERROR SetParameter( VDEC_PARAMETER_ID           parameter,
                                   VDEC_PARAMETER_DATA * const info );
  VDEC_ERROR GetParameter( VDEC_PARAMETER_ID           parameter,
                                   VDEC_PARAMETER_DATA *       info );

  ~WMVDecoder( void );

  void DestroyWMVDecoder(void);

  /* Serialization functions
  */
  VDEC_ERROR Dump( VDEC_BYTE ** const ppBuf_ret,
                           unsigned long * const pBufSize_ret );

  /* We override Flush to allow us to clean up the WMV layer beneath.
  */
  VDEC_ERROR Flush( int *nFlushedFrames );

  VDEC_ERROR EOS( void );

  VDEC_ERROR Decode( VDEC_INPUT_BUFFER* , bool);

  VDEC_ERROR ReuseFrameBuffer(VDEC_FRAME * const pFrame);


  // We override Suspend and Resume.
  VDEC_ERROR Suspend( void );

  VDEC_ERROR Resume( VDEC_CONCURRENCY_CONFIG concurrencyConfig);

private:
  /* Callbacks which allow us to handle callbacks from the H264 decoder
  ** which notify us of DSP events.  The lack of any user-data to these
  ** callbacks forces us to be a singleton, so that'll have to be fixed.
  */
  static void DecodeDoneCallback( VDL_Decode_Return_Type returnCode,    /* return status code */
                                  void *pDecodeStats,  /* return decode frame statistics */
                                  void *pUserData      /* user data.  Unused by us */ 
								 );

  DecodeStatsType* GetStatsBuffer (void);
  void FreeStatsBuffer(DecodeStatsType* pDecodeStats);

  /* The DecodeDoneCallback just turns its user data pointer into a pThis
  ** pointer and calls our member handler, which does all the real work.
  */
  void DecodeDoneHandler( VDL_Decode_Return_Type returnCode,    /* return status code */
                          void *pDecodeStats  /* return decode frame statistics */
            );

//  static QCConditionType sm_videoSyncEvent;

  /* Because we have static members that need explicit initialization and
  ** destruction, we need to keep a counter of the number of WMVDecoders
  ** floating around out there.  Not useful initially, but when we add
  ** support for multiple simultaneous decoders this'll matter.
  */
  static unsigned int sm_numDecoders;

  /* Member functions to initialize static members when the decoder starts
  ** and destroy them after the decoder quits.
  */
  void DoStaticConstruction( void );
  void DoStaticDestruction( void );

  /* An internal state which helps us keep track of the state of the decoder.
  */
  enum
  {
    VDEC_STATE_ERROR = -1,
    VDEC_STATE_UNINITIALIZED,
    VDEC_STATE_INITIALIZED,
    VDEC_STATE_DESTRUCTING
  }
  m_state;

  /* Post processing filter options.  If it is left uninitialized, the decoder
  ** determine the post-filtering level. Level -1 is the default.
  */
  enum
  {
    PP_UNINITIALIZED = -1,
    PP_NO_PP_FILTER,
    PP_QUICK_DEBLOCKING,
    PP_FULL_DEBLOCKING,
    PP_QUICK_DEBLOCKING_DERINGING,
    PP_FULL_DEBLOCKING_DERINGING
  }
  m_pp_filter_option;

  enum
  {
    HURRYUP_DEFAULT = 0,
    HURRYUP_LEVEL1,
    HURRYUP_LEVEL2,
    HURRYUP_LEVEL3,
    HURRYUP_LEVEL4
  }
  m_hurryup_level;

  FrameBufferPool *m_pBuffers;

private:
  bool    m_bHeaderParsed;
  unsigned long int  m_iFourcc;
  unsigned long int  m_numFramesDecodeDone;
  char    m_WMVHeaderData[MAX_MP4_LAYERS][WMV_SEQUENCE_HEADER_BYTES];
  unsigned long int  m_SrcHeight,m_SrcWidth;
  char    m_fourcc[5];
  VDEC_SLICE_BUFFER_INFO  *m_pSliceBufferInfo;

  VDEC_ERROR InitializeDecInternal(void);
  int GetLayerIDWithEarliestTimeStamp(unsigned long int NumBytes[MAX_MP4_LAYERS],
                                      unsigned long long TimeStamp[MAX_MP4_LAYERS], unsigned char  nlayers);
 };

#endif /* _VDECODER_WMV_H_ */


/* =======================================================================

                          vdecoder_utils.cpp

DESCRIPTION
  Definitions of 'utility' functions used on both sides of the VDEC API.

INITIALIZATION AND SEQUENCING REQUIREMENTS
  None.

Copyright (c) 2007 by QUALCOMM, Incorporated.  All Rights Reserved. 
QUALCOMM Proprietary and Confidential.
========================================================================== */
/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/VdecCommon/main/latest/src/vdecoder_utils.cpp#2 $
$DateTime: 2008/08/19 16:50:05 $
$Change: 726228 $

========================================================================== */

/* ==========================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */
#include "vdecoder_utils.h"
extern "C"
{
#include "TL_types.h"
#include "VDL_Types.h"
}
#include "MP4_Types.h"

/* ==========================================================================

                DEFINITIONS AND DECLARATIONS FOR MODULE

This section contains definitions for constants, macros, types, variables
and other items needed by this module.

========================================================================== */

/* -----------------------------------------------------------------------
** Constant / Define Declarations
** ----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
** Type Declarations
** ----------------------------------------------------------------------- */
struct VDEC2DSP_AUDIO_EQUIV
{
  QtvQDSPAudioType  dsp;
  VDEC_CONCURRENCY_CONFIG vdec;
};

struct VDEC2VDL_CONCURRENCY_EQUIV
{
  VDL_Concurrency_Type  dlAudio;
  VDEC_CONCURRENCY_CONFIG vdec;
};

/*
struct VDEC2MEDIA_AUDIO_EQUIV
{
  Media::CodecType  media;
  VDEC_CONCURRENCY_CONFIG vdec;
};
*/
/* -----------------------------------------------------------------------
** Local Object Definitions
** ----------------------------------------------------------------------- */
const VDEC2DSP_AUDIO_EQUIV s_dspAudioMap[] =
{
  { QTVQDSP_AUDIO_EVRC8K,      VDEC_CONCURRENT_AUDIO_EVRC },
  { QTVQDSP_AUDIO_QCELP13K,    VDEC_CONCURRENT_AUDIO_QCELP },
  { QTVQDSP_AUDIO_AMR,         VDEC_CONCURRENT_AUDIO_AMR },
  { QTVQDSP_AUDIO_AAC,         VDEC_CONCURRENT_AUDIO_AAC },
  { QTVQDSP_AUDIO_MP3,         VDEC_CONCURRENT_AUDIO_MP3 },
  { QTVQDSP_AUDIO_CONC,        VDEC_CONCURRENT_AUDIO_CONC },
  { QTVQDSP_AUDIO_REAL,        VDEC_CONCURRENT_AUDIO_REAL },
  { QTVQDSP_AUDIO_AMR_WB,      VDEC_CONCURRENT_AUDIO_AMR_WB },    
  { QTVQDSP_AUDIO_AMR_WB_PLUS, VDEC_CONCURRENT_AUDIO_AMR_WB_PLUS },
  { QTVQDSP_VIDEO_MPEG4_ENCODER,   VDEC_CONCURRENT_VIDEO_MPEG4_ENCODER },
  { QTVQDSP_VIDEO_H264_ENCODER,    VDEC_CONCURRENT_VIDEO_H264_ENCODER },
  { QTVQDSP_AUD_OVER_ENC_QCELP13K, VDEC_CONCURRENT_AUD_OVER_ENC_QCELP13K },
  { QTVQDSP_AUD_OVER_ENC_AMR,      VDEC_CONCURRENT_AUD_OVER_ENC_AMR },
  { QTVQDSP_AUD_OVER_ENC_EVRC,     VDEC_CONCURRENT_AUD_OVER_ENC_EVRC },
  { QTVQDSP_AUDIO_NONE,            VDEC_CONCURRENT_NONE }
};

/* ----------------------------------------------------------------------- */

/*
const VDEC2MEDIA_AUDIO_EQUIV s_mediaAudioMap[] =
{
  { Media::UNKNOWN_CODEC,     VDEC_CONCURRENT_NONE },
  { Media::EVRC_CODEC,        VDEC_CONCURRENT_AUDIO_EVRC },
  { Media::QCELP_CODEC,       VDEC_CONCURRENT_AUDIO_QCELP },
  { Media::AAC_CODEC,         VDEC_CONCURRENT_AUDIO_AAC },
//  { Media::BSAC_CODEC,        VDEC_CONCURRENT_AUDIO_AAC },
  { Media::GSM_AMR_CODEC,     VDEC_CONCURRENT_AUDIO_AMR },
  { Media::MP3_CODEC,         VDEC_CONCURRENT_AUDIO_MP3 },
  { Media::WMA_CODEC,         VDEC_CONCURRENT_NONE }, // Right now, no WMA support. This will change soon.
  { Media::CONC_CODEC,        VDEC_CONCURRENT_AUDIO_CONC },
  { Media::AMR_WB_CODEC,      VDEC_CONCURRENT_AUDIO_AMR_WB },
  { Media::AMR_WB_PLUS_CODEC, VDEC_CONCURRENT_AUDIO_AMR_WB_PLUS }
};
*/

const VDEC2VDL_CONCURRENCY_EQUIV s_dlAudioMap[] =
{
  { VDL_AUDIO_EVRC8K,          VDEC_CONCURRENT_AUDIO_EVRC },
  { VDL_AUDIO_QCELP13K,        VDEC_CONCURRENT_AUDIO_QCELP },
  { VDL_AUDIO_AMR,             VDEC_CONCURRENT_AUDIO_AMR },
  { VDL_AUDIO_AAC,             VDEC_CONCURRENT_AUDIO_AAC },
  { VDL_AUDIO_MP3,             VDEC_CONCURRENT_AUDIO_MP3 },
  { VDL_AUDIO_CONC,            VDEC_CONCURRENT_AUDIO_CONC },
  { VDL_AUDIO_REAL,            VDEC_CONCURRENT_AUDIO_REAL },
  { VDL_AUDIO_AMR_WB,          VDEC_CONCURRENT_AUDIO_AMR_WB },    
  { VDL_AUDIO_AMR_WB_PLUS,     VDEC_CONCURRENT_AUDIO_AMR_WB_PLUS },
  { VDL_VIDEO_MPEG4_ENCODER,   VDEC_CONCURRENT_VIDEO_MPEG4_ENCODER },
  { VDL_VIDEO_H264_ENCODER,    VDEC_CONCURRENT_VIDEO_H264_ENCODER },
  { VDL_AUD_OVER_ENC_QCELP13K, VDEC_CONCURRENT_AUD_OVER_ENC_QCELP13K },
  { VDL_AUD_OVER_ENC_AMR,      VDEC_CONCURRENT_AUD_OVER_ENC_AMR },
  { VDL_AUD_OVER_ENC_EVRC,     VDEC_CONCURRENT_AUD_OVER_ENC_EVRC },
  { VDL_AUDIO_NONE,            VDEC_CONCURRENT_NONE }
};

/* ----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
** Forward Declarations
** ----------------------------------------------------------------------- */

/* =======================================================================
**                          Macro Definitions
** ======================================================================= */

/* =======================================================================
**                            Function Definitions
** ======================================================================= */
/* ======================================================================
FUNCTION:

DESCRIPTION:

PARAMETERS:

RETURN VALUE:

SIDE EFFECTS:
===========================================================================*/

/* ======================================================================
FUNCTION:
  ConvertDSPAudioToVdec

DESCRIPTION:
  Converts a QTVQDSP_AUDIO_ audio codec #define (from mp4qdsp_drv.h) into a
  VDEC_AUDIO_CONFIG (from vdecoder_types.h)

PARAMETERS:
  const int dspAudio
    The QTVQDSP_AUDIO_ #define.

RETURN VALUE:
  VDEC_CONCURRENCY_CONFIG
    The corresponding vdec type.

SIDE EFFECTS:
  None.
===========================================================================*/

#if !defined(ARR_SIZE)
#define  ARR_SIZE( a )  ( sizeof( (a) ) / sizeof( (a[0]) ) )
#endif

VDEC_CONCURRENCY_CONFIG ConvertDSPAudioToVdec( const int dspAudio )
{
  VDEC_CONCURRENCY_CONFIG vdecAudio = VDEC_CONCURRENT_NONE;

  for ( int i = 0; i < static_cast<int>( ARR_SIZE(s_dspAudioMap) ); ++i )
  {
    if ( s_dspAudioMap[ i ].dsp == (QtvQDSPAudioType) dspAudio )
    {
      vdecAudio = s_dspAudioMap[ i ].vdec;
      break;
    }
  }

  return vdecAudio;
}

/* ======================================================================
FUNCTION:
  ConvertMediaAudioToVdec

DESCRIPTION:
  Converts a Media::CodecType (from media.h) into a
  VDEC_AUDIO_CONFIG (from vdecoder_types.h)

PARAMETERS:
  const Media::CodecType mediaAudio
    The audio type to convert.

RETURN VALUE:
  VDEC_CONCURRENCY_CONFIG
    The corresponding vdec type.

SIDE EFFECTS:
  None.
===========================================================================*/
/* 
VDEC_CONCURRENCY_CONFIG ConvertMediaAudioToVdec( const Media::CodecType mediaAudio )
{
  VDEC_CONCURRENCY_CONFIG vdecAudio = VDEC_CONCURRENT_NONE;

  for ( int i = 0; i < ARR_SIZE(s_mediaAudioMap); ++i )
  {
    if ( s_mediaAudioMap[ i ].media == mediaAudio )
    {
      vdecAudio = s_mediaAudioMap[ i ].vdec;
      break;
    }
  }

  return vdecAudio;
}
*/

/* ======================================================================
FUNCTION:
  ConvertVdecAudioToDSP

DESCRIPTION:
  Converts a VDEC_AUDIO_CONFIG (from vdecoder_types.h) audio codec
  into a QTVQDSP_AUDIO_ #define (from mp4qdsp_drv.h)

PARAMETERS:
  const VDEC_CONCURRENCY_CONFIG vdecAudio
    The vdec audio type.

RETURN VALUE:
  int
    The QTVQDSP_AUDIO_ #define corresponding to the vdec type.

SIDE EFFECTS:
  none.
===========================================================================*/
int ConvertVdecAudioToDSP( const VDEC_CONCURRENCY_CONFIG vdecAudio )
{
  int dspAudio = (int) QTVQDSP_AUDIO_NONE;

  for ( int i = 0; i < static_cast<int>( ARR_SIZE(s_dspAudioMap) ); ++i )
  {
    if ( s_dspAudioMap[ i ].vdec == vdecAudio )
    {
      dspAudio = (int) s_dspAudioMap[ i ].dsp;
      break;
    }
  }

  return dspAudio;
}

/* ======================================================================
FUNCTION:
  ConvertVDLConcurrencyTypeToDSP

DESCRIPTION:
  Converts a VDEC_AUDIO_CONFIG (from vdecoder_types.h) audio codec
  into a VDL_AUDIO_TYPE #define (from VDL_types.h)

PARAMETERS:
  const VDEC_CONCURRENCY_CONFIG vdecAudio
    The vdec audio type.

RETURN VALUE:
  int
    The VDEC2VDL_CONCURRENCY_EQUIV #define corresponding to the vdec type.

SIDE EFFECTS:
  none.
===========================================================================*/
int ConvertVDLConcurrencyTypeToDSP( const VDEC_CONCURRENCY_CONFIG vdecAudio )
{
  int dlAudio = (int) VDL_AUDIO_NONE;

  for ( int i = 0; i < static_cast<int>( ARR_SIZE(s_dlAudioMap) ); ++i )
  {
    if ( s_dlAudioMap[ i ].vdec == vdecAudio )
    {
      dlAudio = (int) s_dlAudioMap[ i ].dlAudio;
      break;
    }
  }

  return dlAudio;
}

/* ======================================================================
FUNCTION:
  ConvertStatsToVdecFrame

DESCRIPTION:
  Converts a DecodeStatsType (from mp4codec.h) into a VDEC_FRAME
  (from vdecoder_types.h)

PARAMETERS:
   const DecodeStatsType &stats_in
     The stats structure to convert
   VDEC_FRAME               &frame_out
     The VDEC_FRAME to populate!

RETURN VALUE:
  none.

SIDE EFFECTS:
  none.
===========================================================================*/
void ConvertStatsToVdecFrame
(
  const DecodeStatsType &stats_in,
  VDEC_FRAME               &frame_out
)
{
  // Copy data from the stats buffer to a shiny VDEC_FRAME.
  //
  // Easy stuff first
  //
  frame_out.stream       = 0; // Caller has to set this one...
  frame_out.stride    = stats_in.Width;

  frame_out.dimensions.height = stats_in.SrcHeight;
  frame_out.dimensions.width = stats_in.SrcWidth;

  frame_out.timestamp     = stats_in.FrameDisplayTime;
  frame_out.layer         = stats_in.LayerID;
  frame_out.extFrame.numConcealedMbs = stats_in.NumConcealedMB;
  frame_out.extFrame.timeIncrement = stats_in.TimeIncrement;
  frame_out.extFrame.numIntraMbs = stats_in.NumIntraMBs;
  frame_out.extFrame.streamStatus = (VDEC_STREAM_STATUS)stats_in.StreamStatus;
#ifdef FEATURE_MPEG4_DEBLOCKER
  //frame_out.extFrame.pPostFilterMbInfo = (VDEC_POST_FILTER_MB_INFO *) stats_in.pMbInfo;
#endif // FEATURE_MPEG4_DEBLOCKER
  frame_out.extFrame.pPostFilterMbInfo = NULL;

  /* reserved for adaptive backlight feature */
  frame_out.extFrame.pPostFilterFrameInfo = NULL;

  frame_out.frameType     = (VDEC_FRAMETYPE)stats_in.FrameType;

  // Which buffer we return and the color format depend on hardware...
  // If we're using the MDP, we need a YUV buffer.  If we're not
  // we want the RGB buffers.
  //
#ifdef FEATURE_QTV_MDP
#if defined( FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A )
  frame_out.pBuf            = ( VDEC_BYTE * )stats_in.pOutputYUVBuf;
#elif defined( FEATURE_VIDEO_PLAYER_INTERFACE_REV_C )
#ifdef FEATURE_VIDEO_PLAYER_INTERFACE_REV_C1
  if( MP4QDSPInfo.VideoDecodeCmd.VideoDecoderType == QTVQDSP_VIDEO_WMV)
  {
    frame_out.pBuf            = ( VDEC_BYTE * )stats_in.pYUVBuf;
  }
  else
  {
    frame_out.pBuf            = ( VDEC_BYTE * )stats_in.pOutputYUVBuf;
  }
#else
  frame_out.pBuf            = ( VDEC_BYTE * )stats_in.pYUVBuf;
#endif /* FEATURE_VIDEO_PLAYER_INTERFACE_REV_C1 */
#else
  #error DSP hardware rev not supported
#endif
  frame_out.format.fourcc    = VDEC_FOURCC_I420   /*VDEC_COLOR_YUV_12BPP*/;
  frame_out.format.bbp       = 12;
#else // FEATURE_QTV_MDP
#ifdef PLATFORM_CSIM
  frame_out.pBuf            = ( VDEC_BYTE * )stats_in.pOutputYUVBuf;
  frame_out.format.fourcc   = VDEC_FOURCC_I420  /* VDEC_COLOR_RGB_16BPP*/;
  frame_out.format.bbp      = 12;
#else
  frame_out.pBuf            = ( VDEC_BYTE * )stats_in.pRGBBuf;
  frame_out.format.fourcc   = VDEC_FOURCC_BI_RGB  /* VDEC_COLOR_RGB_16BPP*/;
  frame_out.format.bbp      = 16;
#endif /* PLATFORM_CSIM */
#endif // FEATURE_QTV_MDP

}


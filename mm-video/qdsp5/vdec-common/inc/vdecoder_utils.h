#ifndef _VDECODER_UTILS_H_
#define _VDECODER_UTILS_H_
/* =======================================================================

                          vdecoder_utils.h

DESCRIPTION
  Definitions of 'utility' functions used on both sides of the VDEC API.

INITIALIZATION AND SEQUENCING REQUIREMENTS
  None.

Copyright (c) 2007 by QUALCOMM, Incorporated.  All Rights Reserved. 
QUALCOMM Proprietary and Confidential.
========================================================================== */
/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/VdecCommon/main/latest/src/vdecoder_utils.h#1 $
$DateTime: 2008/08/05 12:05:56 $
$Change: 717059 $

========================================================================== */

/* =======================================================================
**               Includes and Public Data Declarations
** ======================================================================= */

/* ==========================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */
#include "vdecoder_types.h"
#include "TL_types.h"

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
**                        Macro Definitions
** ======================================================================= */

/* =======================================================================
**                        Function Declarations
** ======================================================================= */
/* ======================================================================
FUNCTION:
  ConvertDSPAudioToVdec

DESCRIPTION:
  Converts a MP4QDSP_ audio codec #define (from mp4codec.h) into a
  VDEC_AUDIO_CONFIG (from vdecoder_types.h)

PARAMETERS:
  const int mp4audio
    The MP4QDSP_ #define.

RETURN VALUE:
  VDEC_CONCURRENCY_CONFIG
    The corresponding vdec type.

SIDE EFFECTS:
  None.
===========================================================================*/
VDEC_CONCURRENCY_CONFIG ConvertDSPAudioToVdec( const int mp4audio );

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
//VDEC_CONCURRENCY_CONFIG ConvertMediaAudioToVdec( const Media::CodecType mediaAudio );

/* ======================================================================
FUNCTION:
  ConvertVdecAudioToDSP

DESCRIPTION:
  Converts a VDEC_AUDIO_CONFIG (from vdecoder_types.h) audio codec
  into a MP4QDSP_ #define (from mp4codec.h)

PARAMETERS:
  const VDEC_CONCURRENCY_CONFIG vdecAudio
    The vdec audio type.

RETURN VALUE:
  int
    The MP4QDSP_ #define corresponding to the vdec type.

SIDE EFFECTS:
  none.
===========================================================================*/
int ConvertVdecAudioToDSP( const VDEC_CONCURRENCY_CONFIG vdecAudio );

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
    The VDL_CONCURRENCY_TYPE #define corresponding to the vdec type.

SIDE EFFECTS:
  none.
===========================================================================*/
int ConvertVDLConcurrencyTypeToDSP( const VDEC_CONCURRENCY_CONFIG vdecAudio );

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
  None.
SIDE EFFECTS:
  none.
===========================================================================*/
void ConvertStatsToVdecFrame
(
  const DecodeStatsType &stats_in,
  VDEC_FRAME               &frame_out
);

#endif /* _VDECODER_UTILS_H_ */

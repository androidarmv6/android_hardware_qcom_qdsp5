#ifndef VDECODER_TYPES_I_H
#define VDECODER_TYPES_I_H
/* =======================================================================
                               vdecoder_types.h
DESCRIPTION
  This file defines all the types used by the QTV video decoder API
  which don't need to be seen by the outside world.

Copyright (c) 2007 by QUALCOMM, Incorporated.  All Rights Reserved. 
QUALCOMM Proprietary and Confidential.
========================================================================== */

/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/VdecCommon/main/latest/src/vdecoder_types_i.h#2 $
$DateTime: 2008/09/16 11:30:25 $
$Change: 744302 $

========================================================================== */

/* =======================================================================
**               Includes and Public Data Declarations
** ======================================================================= */

/* ==========================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */
#include "queue.h"
#include "assert.h"

extern "C"
{
  #include "MP4_Types.h"    /* for MP4 config params */
}

/* ==========================================================================

                        DATA DECLARATIONS

========================================================================== */
/* -----------------------------------------------------------------------
** Constant / Define Declarations
** ----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
** Type Declarations
** ----------------------------------------------------------------------- */
class VideoDecoder; /* Forward declaration */

/* file extension specified in the file. This is used to figure out what
** decoder to use
*/
static const int FTYP_SIZE = 4;

/* Type values for VDEC_BLOBs.
*/

//typedef enum
//{
//  VDEC_BLOBTYPE_EMPTY,       /* An empty BLOB                   */
//  VDEC_BLOBTYPE_MP4VOL,      /* MPEG-4 VOL header               */
//  VDEC_BLOBTYPE_H264INIT,    /* BLOB containing H.264 init info */
//  VDEC_BLOBTYPE_JPEGINIT,    /* SKT-MOD JPEG init info          */
//  VDEC_BLOBTYPE_WMVINIT,     /* Windows media init info         */
//  VDEC_BLOBTYPE_OSCARINIT,   /* Oscar media init info           */
//  VDEC_BLOBTYPE_REALINIT     /* Real Video media init info      */
//}
//VDEC_BLOBTYPE;

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

#endif /* VDECODER_TYPES_I_H */

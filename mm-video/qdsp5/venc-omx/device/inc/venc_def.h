#ifndef _VENC_DEF_H
#define _VENC_DEF_H
/*===========================================================================

                   S T A N D A R D    D E C L A R A T I O N S

DESCRIPTION
  This header file contains general types and macros that are of use
  to the video encoder device layer
       
Copyright (c) 2009 by Qualcomm, Incorporated. All Rights Reserved. 
Qualcomm Proprietary and Confidential.
===========================================================================*/


/*===========================================================================

                      EDIT HISTORY FOR FILE

This section contains comments describing changes made to this file.
Notice that changes are listed in reverse chronological order.

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/device/inc/venc_def.h#1 $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
2009/08/07  bc      Initial revision

===========================================================================*/


/*===========================================================================

                            Data Declarations

===========================================================================*/

/* ------------------------------------------------------------------------
** Constants
** ------------------------------------------------------------------------ */

#ifdef TRUE
#undef TRUE
#endif

#ifdef FALSE
#undef FALSE
#endif

#define TRUE   1   /* Boolean true value. */
#define FALSE  0   /* Boolean false value. */

/* -----------------------------------------------------------------------
** Standard Types
** ----------------------------------------------------------------------- */

typedef  unsigned char      boolean;     /* Boolean value type. */
typedef  unsigned char      byte;        /* Unsigned 8  bit value type. */

typedef  unsigned long int  uint32;      /* Unsigned 32 bit value */
typedef  unsigned short     uint16;      /* Unsigned 16 bit value */
typedef  unsigned char      uint8;       /* Unsigned 8  bit value */

typedef  signed long int    int32;       /* Signed 32 bit value */
typedef  signed short       int16;       /* Signed 16 bit value */
typedef  signed char        int8;        /* Signed 8  bit value */


  typedef long long           int64;
  typedef unsigned long long  uint64;

#endif
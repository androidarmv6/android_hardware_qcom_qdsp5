#ifndef MP4BITSTREAM_H
#define MP4BITSTREAM_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
			mp4bitstream.h
DESCRIPTION
  This header file contains all the definitions necessary to interface to the
  MP4 bitstream slice and synchronization functions.  The functions in this
  module provide 3 types of services.

  The slice show/read/flush functions are used to look at and/or extract a
  number of bits from either the forward or reverse ends of an MPEG slice.

  mp4bitstream_seek_resync_marker examines the slice for either the resync
  marker, the DC marker, or the motion marker.

  Finally, debug trace routines are provided which are used by the mp4
  modules to write information to an external file when used in the LTK
  environment.

EXTERNALIZED FUNCTIONS
  mp4bitstream_slice_show_bits  
  mp4bitstream_slice_flush_bits
  mp4bitstream_slice_read_bits
  mp4bitstream_slice_show_bits_reverse
  mp4bitstream_slice_flush_bits_reverse
  mp4bitstream_slice_read_bits_reverse
  mp4bitstream_seek_resync_marker
  mp4bitstream_btos
  mp4bitstream_slice_trace_routine
  mp4bitstream_slice_trace_routine_rev
  mp4bitstream_seek_263k_resync_marker

INITIALIZATION AND SEQUENCING REQUIREMENTS
  Detail how to initialize and use this service.  The sequencing aspect
  is only needed if the order of operations is important.


Copyright(c) 2002 by QUALCOMM, Incorporated. All Rights Reserved.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/MPEG4Vdec/main/latest/inc/mp4bitstream.h#2 $
$DateTime: 2009/03/03 14:14:44 $
$Change: 853763 $

========================================================================== */

/* =======================================================================
**               Includes and Public Data Declarations
** ======================================================================= */

/* ==========================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */
#include "MP4_Types.h"
#include "MP4Errors.h"

#ifdef NO_ARM_CLZ
  #define CLZ(reg, val) { reg = 0; while( reg < 32 && ((val & (1 << (31 - reg))) == 0) ) reg ++;  }
#else
#ifndef CLZ
  #define CLZ(reg,val) __asm { CLZ reg, val }
#endif  
#endif

/* <EJECT> */
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
**                        Class Declarations
** ======================================================================= */

unsigned long int TL_CLZ(unsigned long int val);

/* <EJECT> */
MP4_ERROR_TYPE mp4bitstream_slice_show_bits(
  mp4_slice_type *pSlice,                /* pointer to slice */
  unsigned long int          NumberOfBitsToRead,    /* number of bits to read (<= 24)*/
  unsigned long int         *pResult                /* output target */
  );

/* <EJECT> */
MP4_ERROR_TYPE mp4bitstream_slice_show_bits_reverse(
  mp4_slice_type *pSlice,              /* pointer to slice */
  unsigned long int          NumberOfBitsToRead,  /* number of bits to read (<= 24)*/
  unsigned long int         *pResult              /* output target */
  );


/* <EJECT> */
MP4_ERROR_TYPE mp4bitstream_slice_flush_bits(
  mp4_slice_type *pSlice,                /* Input Slice pointer */
  unsigned long int          NumberOfBitsToFlush    /* number of bits to flush (<= 24)*/
  );


/* <EJECT> */
MP4_ERROR_TYPE mp4bitstream_slice_read_bits(
  mp4_slice_type *pSlice,                /* pointer to slice */
  unsigned long int          NumberOfBitsToRead,    /* number of bits to read (<= 24)*/
  unsigned long int         *pResult                /* Value of the data. Output */
  );


/* <EJECT> */
MP4_ERROR_TYPE mp4bitstream_slice_flush_bits_reverse(
  mp4_slice_type *pSlice,              /* pointer to slice */
  unsigned long int          NumberOfBitsToFlush  /* number of bits to flush (<= 24)*/
  );

/* <EJECT> */
MP4_ERROR_TYPE mp4bitstream_slice_read_bits_reverse(
  mp4_slice_type *pSlice,               /* pointer to slice */
  unsigned long int          NumberOfBitsToRead,   /* number of bits to read (<= 24)*/
  unsigned long int         *pResult               /* Value of the data. Output */
  );


/* <EJECT> */
MP4_ERROR_TYPE mp4bitstream_gob_resync_marker(
    mp4_slice_type *pSlice
    );


/* <EJECT> */
MP4_ERROR_TYPE mp4bitstream_seek_resync_marker(
  mp4_slice_type  *pSlice,
  VOP_type        *pVop,        /* updated by reading Frame header */
  MP4VOLType      *pVol         /* for data_partitioned flag */
  );

#ifdef FEATURE_MPEG4_VLD_DSP
MP4_ERROR_TYPE mp4bitstream_seek_resync_marker_vld_dsp
(
   mp4_slice_type  *pSlice,
   VOP_type        *pVop,        /* updated by reading Frame header */
   MP4VOLType      *pVol         /* for data_partitioned flag */
);
#endif

/* <EJECT> */
#ifdef FEATURE_MP4_H263_ANNEX_K
MP4_ERROR_TYPE mp4bitstream_seek_263k_resync_marker(
  mp4_slice_type  *pSlice,
  VOP_type        *pVop,        /* updated by reading Frame header */
  MP4VOLType      *pVol         /* for data_partitioned flag */
  );
#endif /* FEATURE_MP4_H263_ANNEX_K */

#endif /* MP4BITSTREAM_H */


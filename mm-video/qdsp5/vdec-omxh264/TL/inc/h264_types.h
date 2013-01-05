#ifndef _H264_TYPES_H_
#define _H264_TYPES_H_
/* =======================================================================
 *  h264_types.h
 *  DESCRIPTION
 *  H.264 type definitions
 *
 *  Copyright (c) 2007 by QUALCOMM, Incorporated.  All Rights
 *  Reserved. 
 *  QUALCOMM Proprietary and Confidential.
 * ==========================================================================
 **/

// Slice type
// // slice type 5~9 enforce that all slices in a picture are of the same type
#define SLICE_TYPE_P1               0
#define SLICE_TYPE_B1               1
#define SLICE_TYPE_I1               2
#define SLICE_TYPE_SP1              3
#define SLICE_TYPE_SI1              4
#define SLICE_TYPE_P2               5
#define SLICE_TYPE_B2               6
#define SLICE_TYPE_I2               7
#define SLICE_TYPE_SP2              8
#define SLICE_TYPE_SI2              9

// macros for slice type manipulation
#define IS_SLICE_P(slice_type)      ( (slice_type) == SLICE_TYPE_P1 || (slice_type) == SLICE_TYPE_P2 )
#define IS_SLICE_B(slice_type)      ( (slice_type) == SLICE_TYPE_B1 || (slice_type) == SLICE_TYPE_B2 )
#define IS_SLICE_I(slice_type)      ( (slice_type) == SLICE_TYPE_I1 || (slice_type) == SLICE_TYPE_I2 )

typedef enum
{
  TL_ERR_NONE = 0,
  TL_ERR_MYSTERY_ERROR,
  TL_ERR_OUT_OF_MEMORY,
  TL_ERR_FATAL_ERROR,
  TL_ERR_OUT_OF_BUFFERS,
  TL_ERR_NO_INPUT_AVAILABLE,
  TL_ERR_RESOURCE_UNAVAILABLE,
  TL_ERR_UNSUPPORTED_DIMENSIONS
}TL_ERROR;

#endif /* _H264_TYPES_H_ */

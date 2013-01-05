#ifndef TL_TYPES_H
#define TL_TYPES_H
/* =======================================================================
                               tl_types.h
DESCRIPTION
  This file defines all the types used by the video transform layer 
  which don't need to be seen by the outside world.

Copyright (c) 2007 by QUALCOMM, Incorporated.  All Rights Reserved. 
QUALCOMM Proprietary and Confidential.
========================================================================== */

/* =======================================================================
                             Edit History

$Header: //depot/asic/msmshared/services/Qtv/dec\TL\TL_types.h#1
$DateTime: 2008/08/19 16:50:05 $
$Change: 726228 $

========================================================================== */

/* =======================================================================
**               Includes and Public Data Declarations
** ======================================================================= */

/* ==========================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */
#include "queue.h"
#include "assert.h"
#include "VDL_Types.h"

/* ==========================================================================

                        DATA DECLARATIONS

========================================================================== */
/* -----------------------------------------------------------------------
** Constant / Define Declarations
** ----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
** Type Declarations
** ----------------------------------------------------------------------- */

#ifdef FEATURE_QTV_WVGA_ENABLE
  #define VIDEO_WVGA_WIDTH            800
  #define VIDEO_WVGA_HEIGHT           480
  #define VIDEO_MAX_DSP_WIDTH         VIDEO_WVGA_WIDTH
  #define VIDEO_MAX_DECODE_WIDTH      VIDEO_WVGA_WIDTH
  #define VIDEO_MAX_DECODE_HEIGHT     VIDEO_WVGA_HEIGHT
#elif defined FEATURE_QTV_VGA_ENABLE
  #define VIDEO_VGA_WIDTH             640
  #define VIDEO_VGA_HEIGHT            480
  #define VIDEO_MAX_DSP_WIDTH         VIDEO_VGA_WIDTH
  #define VIDEO_MAX_DECODE_WIDTH      VIDEO_VGA_WIDTH
  #define VIDEO_MAX_DECODE_HEIGHT     VIDEO_VGA_HEIGHT
#else 
  #define VIDEO_QVGA_WIDTH             320
  #define VIDEO_QVGA_HEIGHT            240
  #define VIDEO_MAX_DSP_WIDTH         VIDEO_QVGA_WIDTH
  #define VIDEO_MAX_DECODE_WIDTH      VIDEO_QVGA_WIDTH
  #define VIDEO_MAX_DECODE_HEIGHT     VIDEO_QVGA_HEIGHT
#endif /* FEATURE_QTV_WVGA_ENABLE */

#define VIDEO_DECODE_H264_MIN_WIDTH	48
#define VIDEO_DECODE_H264_MIN_HEIGHT	32

#ifdef FEATURE_QTV_WVGA_ENABLE
  #define MAX_MB    1500
#elif defined FEATURE_QTV_VGA_ENABLE
  #define MAX_MB    1200
#else
  #define MAX_MB    300  
#endif /* FEATURE_QTV_WVGA_ENABLE */

#define VIDEO_YUV_BITS_PER_PIXEL  12

// Frame types
#define UNKNOWN_FRAME_TYPE   0
#define I_FRAME_TYPE         1
#define P_FRAME_TYPE         2
#define B_FRAME_TYPE         3
#define QFRE_FRAME_TYPE      4

/*typedef struct
{
  unsigned char    *cbp; // 6 of these per macroblock
  signed short    *mvx; // Motion Vector X, 4 per macroblock
  signed short    *mvy; // Motion Vector Y, 4 per macroblock
} MP4DeblockerInfoType;


typedef struct
{
  unsigned short  quant;
  unsigned char   mbType; 
  signed short    mvx[4];
  signed short    mvy[4];
  unsigned char   blockTypeConfig;
  unsigned char   codedBlockConfig;
  unsigned char   subBlockConfig;
}MP4PostFilterMBInfo;
*/
typedef struct
{
  unsigned long long   FrameDisplayTime;    /* time at which to display this frame */
                                /* = if time is unknown or bad */
                                /* = frame time stamp + delta */
  unsigned long int   TimeStampValidityField;
  void     *pYUVBuf;            /* pointer to the output video YUV buffer */
#if defined FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A || \
    defined FEATURE_VIDEO_PLAYER_INTERFACE_REV_C1
  void     *pOutputYUVBuf;      /* pointer to the output video YUV buffer */
#endif /* FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A || 
          FEATURE_VIDEO_PLAYER_INTERFACE_REV_C1 */
  void     *pRefBuf[2];         /* an array of the reference frames */

  unsigned long int   NumBytes;            /* number of bytes returned */
                                /* Note: these bytes shall be contiguous in */
                                /* memory */
  unsigned long int   NumBytesProcessed;   /* # of bytes processed in decoding frame */

  unsigned char  fIsIntraVOP;         /* TRUE if I-VOP. Used for random access */
  unsigned char  fIsBFrame;           /* TRUE if B-Frame */

  unsigned long int   Height;              /* Height and width of the current frame */
  unsigned long int   Width;
  unsigned long int   SrcHeight;           /* Unpadded Source Height and width of the current frame */
  unsigned long int   SrcWidth;
#ifdef FEATURE_QTV_DECODER_XSCALE_VIDEO
  unsigned long int   ScaledHeight;        /* Scaled sizes for use with xScaler */
  unsigned long int   ScaledWidth;
#endif /* FEATURE_QTV_DECODER_XSCALE_VIDEO */
  unsigned long int   LayerID;
  unsigned long int   NumConcealedMB;      /* number of MBs concealed per frame */

  /* Luma Quantization parameter for each macroblock of the frame
  ** This is passed up to the app layer, in case the customer wants
  ** to do their own de-blocking on the YUV buffer                       */
  unsigned short   QPBuf[MAX_MB];

  /* Indicates whether each MB in frame is INTER or INTRA */
  unsigned char  IsIntraMBBuf[MAX_MB];/* pointer to buffer containing Boolean     */
                                /* for each MB in frame to indicate whether */
                                /* it's INTRA or INTER                      */

  unsigned long int NumIntraMBs;          /* number of intra MBs                      */
  unsigned long int TimeIncrement;        /* TimeIncrement info for each frame. */
  unsigned long long derivedTimestamp;     /* Derived timestamp calculated by decoder */
  unsigned long int StreamStatus;         /* Indicates the stream status */
                               /* 0 - Indicates there is no change in the header information for the stream*/
                               /* 1 - Indicates that we have received a new header (e.g VOL for MP4) before
							         this frame.                                                          */

  
  unsigned long int   FrameType;           /* 0 - unknown frame type */
                                /* 1 - I frame            */
                                /* 2 - P frame            */
                                /* 3 - B frame            */
#ifdef FEATURE_MPEG4_DEBLOCKER
  //MP4PostFilterMBInfo  *pMbInfo; // pMbInfo[MAX_MB];
#endif //FEATURE_MPEG4_DEBLOCKER

  //MP4TransformType    rotation; /* frame rotation                        */
  //MP4TransformType    scaling;  /* frame scaling                         */
  //MP4DecodeReturnType ReturnCode;  /* code to say if standard frame, error, or not coded VOP */
  VDL_Decode_Return_Type ReturnCode;    /* return status code */

  unsigned char bIsFlushed; /* TRUE if this frame is flushed and should not
                      ** be returned to the application when complete */
  
  void     *pMetaData;          /* Pointer to metadata that is malloc'd and */ 
                                /* freed by the decoder. This pointer is    */
                                /* transported to upper layers in a         */
                                /* a VDEC_FRAME. It is freed when the frame */
                                /* buffer is released.                      */
  //MP4DeblockerInfoType DeblockerInfo;

#if defined(FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A) && defined(FEATURE_MP4_H263_ANNEX_J) 
  //PostProcessingType PPMode;  /* Describes the post-processing performed on a frame */
#endif /* FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A && FEATURE_MP4_H263_ANNEX_J */ 

 unsigned char OutBufId;            /* This field will contain the value of FrameId which goes in to the*/
                              /* frame header. The purpose of this variable is to enable us to    */
                              /* set the proper value of NextInFrameId in mp4_process_flush when  */
                              /* we flush the stats Q and slice Q during repositioning and ensure */
                              /* that the ARM and DSP remain in sync. */
 VDL_Frame_Stats_Pkt_Type *pFrameStatistics; /*Used in case of VLD in DSP */
} DecodeStatsType;

/* Define a decode frame statistics structure */
//typedef struct
//{
//  q_link_type          Link;
//  DecodeStatsType   DecodeStats;
//} DecodeStatsPacketType;

#ifdef FEATURE_QTV_OSCAR_DECODER
  /*-------------------------------------------------------------------------
    For OSCAR, we only have a maximum of 2 layers of scalability, and
    FEATURE_MP4_TEMPORAL_SCALABILITY actually adds code that causes problems
    for the media & decoder, so we manually override the layer count.
  -------------------------------------------------------------------------*/
  #define MAX_DECODE_LAYERS 2
#elif defined(FEATURE_MP4_TEMPORAL_SCALABILITY)
/*
**  Max Nr. of Layers
**
**  Must be >= 1. If it equals 1, then no enhancement layer may be
**  present.
*/
# define MAX_DECODE_LAYERS 4
#else
/*
**  Max Nr. of Layers
**
**  No temporal scalability, so no enhancement layers
*/
# define MAX_DECODE_LAYERS 1
#endif /* FEATURE_MP4_TEMPORAL_SCALABILITY */


//sks


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

#endif /* TL_TYPES_I_H */

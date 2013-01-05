#ifndef DL_TYPES_H
#define DL_TYPES_H
/* =======================================================================
                               VDLTypes.h
DESCRIPTION
  This file has the data structures that will not only be exposed outside DL (along
  with DL-API) but will also be used within DL

Copyright (c) 2007 by QUALCOMM, Incorporated.  All Rights Reserved. 
QUALCOMM Proprietary and Confidential.
========================================================================== */

/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/QDSP5VdecDriver/main/latest/src/VDL_Types.h#2 $
$DateTime: 2009/03/03 14:14:44 $
$Change: 853763 $

========================================================================== */

/*==========================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */
#include "queue.h"

/*==========================================================================

                       DATA DECLARATIONS

========================================================================== */

typedef enum
{
  VDL_ERR_NONE = 0,
  VDL_ERR_NULL_STREAM_ID,
  VDL_ERR_INVALID_STREAM_ID,
  VDL_ERR_INIT_FAILURE,
  VDL_ERR_INVALID_STATE,
  VDL_ERR_OUT_OF_MEMORY,
  VDL_ERR_TERMINATE_FAILURE,
  VDL_ERR_MYSTERY

}VDL_ERROR;

typedef struct
{
  unsigned short PacketID;
  unsigned short Num_MB_Concealed;
  unsigned short Num_Intra_MB;
  unsigned short Frame_Decode_Time_High;
  unsigned short Frame_Decode_Time_Low;
  unsigned short EndofStatsMarker;
} VDL_Frame_Stats_Pkt_Type;     /* Stats packet structure */

typedef struct
{
  q_link_type   link;
  VDL_Frame_Stats_Pkt_Type *pStats_pkt;
}VDL_FrameStats_Pkt_Type;

typedef struct
{
  q_link_type   link;             /* buffer queue link    */
  unsigned short        BufferID;         /* buffer identification number */
  unsigned short        SliceSeqNum; /* Queue counter comon between slice and PP buffers*/
  unsigned short        *pNextMacroBlock; /* pointer to the next macro block to send */

  unsigned short        CodecType;

  unsigned short        NumMacroBlocks;   /* number of macro blocks in this buffer */
  unsigned short        NumMacroBlocksToSend;
  unsigned short        fFirstMB;         /* indicates first MacroBlock in a frame */
  unsigned short        fLastMB;          /* indicates last MacroBlock in a frame */
  void          *pSliceData;      /* slice data */
  void          *pSliceDataPhysical;
  int           sliceRegionId;
  unsigned short        SubframePktSeqNum;/* indicates subframe packet sequence number */
  unsigned long int        SliceDataSize;     /* indicates size of subframe in bytes */
  unsigned long int        SliceBufferSize;   /* indicates size of slice buffer allocated */
  unsigned char       fAllocateYUVBuf;  /* If true, lower levels should allocate a new
                                   video frame to hold output from this slice */
  unsigned char isFlushMarker;           /* flush marker for processing flush */
  unsigned char isSubFrameDone;
  VDL_Frame_Stats_Pkt_Type *pFrameStatsInPtr; /*Frame Statictics holder*/
  VDL_FrameStats_Pkt_Type *pFrameStatsOutPtr; /*Frame Statictics ptr.filed used by DL*/
  unsigned char isStatsFromL2;           /*used by DL to figure from where the stats cam from*/
} VDL_Slice_Pkt_Type;     /* slice packet structure */

typedef struct
{
  q_link_type   link;             /* buffer queue link    */
  unsigned short        BufferID;         /* buffer identification number */
  unsigned short        SliceSeqNum; /* Queue counter comon between slice and PP buffers*/
  unsigned short        CodecType; 
  void          *PP_Output_ptr;  /*PP output address info*/
  unsigned short        postproc_info_1; /*Load Sharing Details*/
  unsigned long int        LoadSharingPktSize; /*Load Sharing pakcet size*/
  void          *LoadSharingPktAddr; /*Load sharing data pkt addr*/
  unsigned short        pp_Param_0; /*This is a post-processing command specific input */
  unsigned short        pp_Param_1; /*This is a post-processing command specific input */
  unsigned short        pp_Param_2; /*This is a post-processing command specific input */
  unsigned short        pp_Param_3; /*This is a post-processing command specific input */
  unsigned char isFlushMarker;    /* flush marker for processing flush */
  unsigned char isPPDone;         /*Like SubframeDone */
} VDL_PP_cmd_Info_Type; //PP Cammand information paramaters*/

typedef enum
{
  VDL_FRAME_FLUSHED = -1,   /* frame flushed */
  VDL_DECODE_NO_ERROR  = 0,   /* no frame error*/
  VDL_DECODE_FRAME_ERROR = 1, /* frame error */    

  VDL_FATAL_ERROR = 2,     /* fatal error, restart decoder */    
  VDL_DECODE_FRAME_NOT_CODED = 3,   /* the frame is a not coded frame */
  VDL_OUT_OF_MEMORY = 4,         /* out of memory */
  VDL_DECODE_FRAME_FRE = 5,      
  VDL_SUSPEND_NO_ERROR = 6,
  VDL_FRAME_FRE_FAILED = 7,  /*used in VLD in DSP interface only*/
  VDL_END_OF_SEQUENCE = 8,
  VDL_END_OF_SEQUENCE_FOR_FLUSH = 9

} VDL_Decode_Return_Type;           /* return codes used in the decoder callback function*/


typedef enum
{
  VDL_AUDIO_NONE = 0,
  VDL_AUDIO_EVRC8K,
  VDL_AUDIO_QCELP13K,
  VDL_AUDIO_AMR,
  VDL_AUDIO_AAC,
  VDL_AUDIO_BSAC,
  VDL_AUDIO_AAC_PLUS,
  VDL_AUDIO_MP3,
  VDL_AUDIO_WMA,
  VDL_AUDIO_CONC,
  VDL_AUDIO_REAL,
  VDL_AUDIO_AMR_WB,
  VDL_AUDIO_AMR_WB_PLUS,
  VDL_VIDEO_MPEG4_ENCODER,
  VDL_VIDEO_H264_ENCODER,
  VDL_AUD_OVER_ENC_QCELP13K,
  VDL_AUD_OVER_ENC_AMR,
  VDL_AUD_OVER_ENC_EVRC,
  VDL_AUDIO_MAX
} VDL_Concurrency_Type;

typedef enum
{
  VDL_VIDEO_NONE = 0,
  VDL_VIDEO_MPEG4,
  VDL_VIDEO_H263,
  VDL_VIDEO_H264,
  VDL_VIDEO_H263P3,
  VDL_VIDEO_WMV,
  VDL_VIDEO_REAL,
  VDL_VIDEO_OSCAR,
  VDL_VIDEO_MAX
} VDL_Video_Type;

typedef enum
{
  VDL_PROFILE_SIMPLE = 0,
  VDL_PROFILE_MAIN,
  VDL_PROFILE_EXTENDED,
  VDL_PROFILE_MAX
}VDL_Profile_Type;

typedef enum
{
  VDL_CODEC_MPEG4 = 0,
  VDL_CODEC_H263_P0,
  VDL_CODEC_H264,
  VDL_CODEC_H263_P3,
  VDL_CODEC_RV9,
  VDL_CODEC_WMV9,
  VDL_CODEC_FRUC,
  VDL_CODEC_H264_VLD = 0x8,
  VDL_CODEC_MP4_VLD = 0xa
}VDL_Codec_Type;


typedef void (* VDL_Decoder_Cb_Type)     /* decoder callback function definition*/
(
VDL_Decode_Return_Type ReturnCode,    /* return status code */
void *pDecodeStats,  /* return decode frame statistics */
void               *pUserData      /* user data */
);

typedef enum
{
  VDL_INTERFACE_NONE = 0,
  VDL_INTERFACE_ARM,
  VDL_INTERFACE_REV_C,
  VDL_INTERFACE_REV2A_DMA,
  VDL_INTERFACE_REV2A_DME,
  VDL_INTERFACE_RTOS,
  VDL_INTERFACE_RTOS_VLD_DSP,
  VDL_INTERFACE_MAX
}VDL_Interface_Type;

#endif

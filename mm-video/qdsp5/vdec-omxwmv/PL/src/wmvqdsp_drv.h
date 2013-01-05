
#ifndef WMVQDSP_DRV_H
#define WMVQDSP_DRV_H
/*=============================================================================
                                WMVQDSP_DRV.H
        
DESCRIPTION:
   This file provides necessary interface for the WMV QDSP.

ABSTRACT:
   This file provides the interface specification for the WMV video macro
   block decoder implemented in the QDSP 5 in the MSM7xxx family of ASICs.
   Applications utilize these WMV services through the VDEC API.
   
   Video decoding and encoding functionality is accomplished by
   several modules including but not limited to the: media player
   application, MPEG 4 engine, video codec, & audio codec
   as outlined below:

   -------------------------------------------------
   \     QCT BREWapi Media Player Application      \
   -------------------------------------------------
   \         BREWapi IMedia Interface              \
   -------------------------------------------------
   \               MPEG 4 engine                   \
   -------------------------------------------------
   \  Video codec API    \   \   Audio codec API   \
   -----------------------   -----------------------
   \  Video codec        \   \   QCT Audio codec   \
   -----------------------   -----------------------

   The MPEG 4 video codec is then futher deposed into that portion that runs
   on the ARM processors and that which runs on the QDSP 4 as follows:

   -----------------------
   \  Video codec API    \
   ------------------------
   \   ARM Video codec   \
   -----------------------
   \    Codec QDSP I/F   \
   -----------------------
   \   QDSP Video codec  \
   -----------------------

   This header file describes the Codec QDSP I/F module in the diagram above.

   The ARM video codec shall call mp4qdsp_initialize() prior to any other
   function in order to instruct the QDSP driver to download and initialize
   the QDSP HW.  This function shall be called only once or directly following
   a call to mp4qdsp_terminate().  The ARM video codec shall call
   mp4qdsp_terminate() in order to instruct the QDSP driver to shutdown the
   QDSP HW and terminate these services. The complete initialization and
   configuration sequence is as follows:


   ARM-Video-Codec               QDSP-drv                QDSP HW
   ------------------------------------------------------------------
   QDSPState=RESET                                     State = Unknown

   mp4qdsp_initialize()  --->
                               RESET QDSP              State = Reset
                               Download DSP image      State = Init
                                      .
                                      .
                                      .
                  <---------- mp4qdsp_process_qdsp_msg(MP4QDSP_DOWNLOAD_DONE)

     QDSPState = DOWNLOAD_STATE

   mp4qdsp_command       MP4QDSP_VIDEO_INIT_CMD ------->
                                                       State = Init
                  <---------- mp4qdsp_process_qdsp_msg(MP4QDSP_INIT_STATUS)
     QDSPState = INIT_STATE
                                   .
                                   .
                                   .

  mp4qdsp_command        MP4QDSP_DECODE_CMD ----------->

  TBD-ANR fill in comments here....... 
                                                      State = Decode
EXTERNALIZED FUNCTIONS
  List functions and a brief description that are exported from this file

INITIALIZATION AND SEQUENCING REQUIREMENTS
  Detail how to initialize and use this service.  The sequencing aspect
  is only needed if the order of operations is important.


        Copyright © 1999-2002 QUALCOMM Incorporated.
               All Rights Reserved.
            QUALCOMM Proprietary

============================================================================*/
/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/Wmv9Vdec/main/latest/src/PL/wmvqdsp_drv.h#2 $
$DateTime: 2009/04/14 19:05:45 $
$Change: 887518 $

========================================================================== */

/* =======================================================================
**               Includes and Public Data Declarations
** ======================================================================= */
#include "VDL_API.h"
#include "TL_types.h"
#include "mp4buf.h"
#include <pthread.h>

/* ==========================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */
#define QTVQDSP_PKT_END_MARKER_HI (0x7FFF)
#define QTVQDSP_PKT_END_MARKER_LO (0xFFFF)

#define WMV_FRAME_HEADER_PKT_TYPE     (0x0001)
#define WMV_MB_HEADER_PKT_TYPE        (0x0002)

#define WMV_SLICE_BUFFER_SIZE         (6000) // Size derived from MP4 empirical data
#define WMV_MAX_MB                    (300)  // Based on max frame size of QVA
#define WMV_NOT_CODED_MB_PKT_SIZE     (12*4) // Excluding the end of packet marker
#define WMV_MAX_NOT_CODED_SLICE_SIZE  (WMV_MAX_MB * (WMV_NOT_CODED_MB_PKT_SIZE + \
                                                     sizeof(QTVQDSP_PKT_END_MARKER_HI) + \
                                                     sizeof(QTVQDSP_PKT_END_MARKER_LO)))
#define WMV_MAX_RESIDUALS_PER_MB      (64*6) // 384
#define WMV_MAX_MOTION_VECS_PER_MB    (4)    // Defined in WMV standard

#define WMV_MAX_MB_SIZE               (sizeof(WMVQDSP_HdrSPktType) + \
                                       sizeof(WMVQDSP_ModesSPktType) + \
                                       WMV_MAX_RESIDUALS_PER_MB * \
                                         sizeof(WMVQDSP_ResidualsSPktType) + \
                                       sizeof(QTVQDSP_PKT_END_MARKER_HI) + \
                                       sizeof(QTVQDSP_PKT_END_MARKER_LO))
#define WMV_MAX_GUARD_SIZE            (WMV_MAX_MB_SIZE)

extern const  unsigned short WmvNumYUVBuffers;
#ifndef FEATURE_QDSP_RTOS
/* The frame header packet structure.  A frame header must be sent before the 
** very first video MB packet--it must be the first packet inserted into the 
** buffer.
*/
typedef struct
{
  unsigned long int NumWords;
  unsigned long int FrameInfo;   /* Frame information */
  unsigned long int Dimensions;  /* Width in pixels, must be a multiple of 16 */
  unsigned long int FrameQuant;  /* Parameters having to do with Picture Quantization */
  unsigned long int FrameBuf0;   /* Address of frame buffer 0 */
  unsigned long int FrameBuf1;   /* Address of frame buffer 1 */
  unsigned long int FrameBuf2;   /* Address of frame buffer 2 */
  unsigned long int FrameBuf3;   /* Address of frame buffer 3 */
  unsigned long int EndOfPacket; /* End of packet markder: 0x7FFFFFFF */
} WMVQDSP_FrameHeaderPktType;

/* The header sub-packet structure */
typedef struct
{
  unsigned long int MBInfo;
} WMVQDSP_HdrSPktType;

/* The mode sub-packet structure */
typedef struct
{
  unsigned long int Luma0SubBlockInfo;
  unsigned long int Luma1SubBlockInfo;
  unsigned long int Luma2SubBlockInfo;
  unsigned long int Luma3SubBlockInfo;
  unsigned long int CbSubBlockInfo;
  unsigned long int CrSubBlockInfo;
} WMVQDSP_ModesSPktType;
/* The motion vector element type */
typedef struct
{
  unsigned long int MotVec;
} WMVQDSP_MotionVecsSPktType;
#else
/* The frame header packet structure.  A frame header must be sent before the 
** very first video MB packet--it must be the first packet inserted into the 
** buffer.
*/
typedef struct
{
  unsigned short PacketID;
  unsigned short xDimension; /*The Horizal resolution should be multiple of 16 */
  unsigned short yDimension; /*The Horizal resolution should be multiple of 16 */
  unsigned short LineWidth;  /* Line Width, Horizontal dimension rounded to multiple of 32 bits */
  unsigned short FrameInfo;   /* Frame information */
  unsigned short FrameBuf0Hi; /* The High 16 bits of Address of frame buffer 0 */
  unsigned short FrameBuf0Lo; /* The Low 16 bits of Address of frame buffer 0 */
  unsigned short FrameBuf1Hi; /* The High 16 bits of Address of frame buffer 1 */
  unsigned short FrameBuf1Lo; /* The Low 16 bits of Address of frame buffer 1 */
  unsigned short FrameBuf2Hi; /* The High 16 bits of Address of frame buffer 2 */
  unsigned short FrameBuf2Lo;   /* The Low 16 bits of Address of frame buffer 2 */
  unsigned short EndOfPacket; /* End of packet markder: 0x7FFFFFFF */
} WMVQDSP_FrameHeaderPktType;

/* The header sub-packet structure */
typedef struct
{
  unsigned short packet_ID;
  unsigned short MBInfo;
  
} WMVQDSP_HdrSPktType; // Packet size is not added since it is copied in a different way 

/* The mode sub-packet structure */
typedef struct
{
  unsigned short Luma0SubBlockInfo_Lo;
  unsigned short Luma0SubBlockInfo_Hi;
  unsigned short Luma1SubBlockInfo_Lo;
  unsigned short Luma1SubBlockInfo_Hi;
  unsigned short Luma2SubBlockInfo_Lo;
  unsigned short Luma2SubBlockInfo_Hi;
  unsigned short Luma3SubBlockInfo_Lo;
  unsigned short Luma3SubBlockInfo_Hi;
  unsigned short CbSubBlockInfo_Lo;
  unsigned short CbSubBlockInfo_Hi;
  unsigned short CrSubBlockInfo_Lo;
  unsigned short CrSubBlockInfo_Hi;
} WMVQDSP_ModesSPktType;

/* The motion vector element type */
typedef struct
{
  unsigned short MotVecX;
  unsigned short MotVecY;
} WMVQDSP_MotionVecsSPktType;

#endif

/* The residual element type */
typedef struct
{
  unsigned long int RunLevel;
} WMVQDSP_ResidualsSPktType;

/* Define the MB packet type to hold all the MB sub-packet information */
typedef struct
{
  WMVQDSP_HdrSPktType Header;
  WMVQDSP_MotionVecsSPktType MotionVecs[WMV_MAX_MOTION_VECS_PER_MB];
  WMVQDSP_ModesSPktType Modes;
  WMVQDSP_ResidualsSPktType Residuals[WMV_MAX_RESIDUALS_PER_MB];
} WMVQDSP_MBPktType;

typedef struct {
  unsigned char  pending_release;          /* TRUE if this YUV is with the client and pending release. */
  unsigned long long   pts;                      /* Presentation timestamp for this frame */
  unsigned long int   reference_cnt;            /* For each frame which references this YUV we increment the count. */
} wmv_dec_yuv_buf_info_type;

typedef struct {
  /* YUV buffer info array. There is a 1:1 mapping between this info
     and the actual buf pointer stored inside MP4CodecInfo.OutputBufInfo */
  wmv_dec_yuv_buf_info_type  bufs[8];
  /* Critical section for thread safety */
  pthread_mutex_t            cs;
  /* Timestamp from last released YUV buffer */
  unsigned long long                    prev_pts_released;

} wmv_dec_yuv_info_type;

extern wmv_dec_yuv_info_type wmv_dec_yuv_info;
extern MP4BufType *wmv_OutputBufInfo;
extern signed short wmv_NextInFrameId;




/* Globally accessible frame and MB packet structures */
extern WMVQDSP_FrameHeaderPktType WMVQDSP_CurFrameHeaderPkt;
extern WMVQDSP_MBPktType          WMVQDSP_CurMBPkt;

/* Globally accessible pointers to the sub-packet structures within the 
** MB packet.
*/
extern WMVQDSP_HdrSPktType        *WMVQDSP_HdrSPkt;
extern WMVQDSP_MotionVecsSPktType *WMVQDSP_MotionVecsSPkt;
extern WMVQDSP_ModesSPktType      *WMVQDSP_ModesSPkt;
extern WMVQDSP_ResidualsSPktType  *WMVQDSP_ResidualsSPkt;

/* Source and target YUV buffer indexes */
extern unsigned long int WMVQDSP_CurrentTargetIndex;
extern unsigned long int WMVQDSP_CurrentSourceIndex;
extern DecodeStatsType *wmv_cur_frame_stats;
extern void* pWmvDL;
//extern unsigned char qtv_cfg_enable_wmv3_optimizations;
extern unsigned char qtv_cfg_enable_wmv3_smcdb_support;
/*===========================================================================

FUNCTION:
  WMVQDSPWriteMBPacketToSliceBuffer

DESCRIPTION:

DEPENDENCIES

INPUT/OUTPUT PARAMETERS:

RETURN VALUE:

SIDE EFFECTS:

===========================================================================*/
void WMVQDSP_MBPacketToSliceBuffer
(
  unsigned short mb_num,
  unsigned short total_num_mbs
);
void PAL_WMVQueueStatsBuffer(void *pDecodeStats );
void PAL_WMVFreeCurrentSliceBuffer(void);
VDL_ERROR PAL_WMVQueueEOSStatsBuffer( DecodeStatsType* pDecoderStatsEOS);
/***************************************************************************/
/*
 WMVDecReleaseYUVBuf()
   Releases a YUV buffer  
<In>
   YUV buffer pointer, presentation time stamp
<Out>
   None
 <Return> 
   None
<Note> 
   None
*/
/***************************************************************************/
void WMVDecReleaseYUVBuf(unsigned char *yuv_buf, unsigned long long  pts);
/*************************************************************************/
/*     
  WMVDecIsYUVBufAvailableForDecode()
    returns whether a buffer is available for decode. in the list of YUV buffers, 
    check if there is any buffer available that is not pending release.
 <In>
    None
 <Out>
    None
 <Return>
    TRUE: if a buffer is available else FALSE. 
 <Note>
    None
*/
/**************************************************************************/
unsigned char WMVDecIsYUVBufAvailableForDecode(void);
/*************************************************************************/
/*     
  WMVDecIsSliceBufAvailableForDecode
    Query the DL layer to see if a slice buffer is available in the free slice
    buffer pool.  
 <In>
    None
 <Out>
    None
 <Return>
    TRUE: if a buffer is available else FALSE. 
 <Note>
    None 
*/
/**************************************************************************/
unsigned char WMVDecIsSliceBufAvailableForDecode(void);
/***********************************************************************/
/*
  WMVDecInitPendingRelease() 
     Clears the pending release. This function must be called whenever we 
     reposition too to clear all the pending release info..
  <In>
     None 
  <Out>
     None
  <Return>
     None 
  <Notes>
     None
 ===========================================================================*/
void WMVDecInitPendingRelease(void);
/***********************************************************************/
/*
  UpdateYUVRefCnt() 
     Increments the reference associated with the particular YUV Buffer.
     We will release the YUV buffer back to the free pool only when the 
     reference count becomes zero. This ensures proper management of the 
     YUV buffers when referenced by more than one frames (e.g one coded VOP
     and one or many non-coded VOPs.)
  <In>
     None 
  <Out>
     None
  <Return>
     None 
  <Notes>
     None
 ===========================================================================*/
void WMVUpdateYUVRefCnt(unsigned long long timeStamp);
/**************************************************************************/
/*
  WMVDecGetNextFreeYUVBufIdx()
    Gets the next YUV buffer index into m_OutputBufInfo 
    that is not pending release.
 <In>
     None
 <Out>
     YUV buffer index.
 <Return>
     YUV buffer index if available else -1.
 <Note>
     None
*/
 /*************************************************************************/
signed short WMVDecGetNextFreeYUVBufIdx(void);

/**************************************************************************/
/*
  WMVDecReleaseCurrYUVBufIdx()
    Releases the current YUV buffer index 
 <In>
     pts
 <Out>
     void.
 <Return>
     void
 <Note>
     None
*/
 /*************************************************************************/
void WMVDecReleaseCurrYUVBufIdx(unsigned long long timeStamp);


#endif //WMVQDSP_DRV_H

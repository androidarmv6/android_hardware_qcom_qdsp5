/*===========================================================================
                              WMVQDSP_DRV.C

DESCRIPTION:
   This file provides necessary interface for the WMV QDSP.

ABSTRACT:

        Copyright © 1999-2002 QUALCOMM Incorporated.
               All Rights Reserved.
            QUALCOMM Proprietary

===========================================================================*/
/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/Wmv9Vdec/main/latest/src/PL/wmvqdsp_drv.c#2 $
$DateTime: 2009/04/14 19:05:45 $
$Change: 887518 $

========================================================================== */

/* =======================================================================
** Includes and Public Data Declarations
** ======================================================================= */
#include "wmvqdsp_drv.h"
#include "qtv_msg.h"

#ifdef FEATURE_QTV_WINDOWS_MEDIA_SMCDB_SUPPORT
extern unsigned char qtv_cfg_enable_wmv3_smcdb_support;
#endif

/* =======================================================================
** Local Definitions and Declarations
** ======================================================================= */

/* Local defintion for boolean variables */
#ifdef TRUE
    #undef TRUE
#endif
#ifdef FALSE
    #undef FALSE
#endif
#define TRUE   1   /* Boolean true value. */
#define FALSE  0   /* Boolean false value. */

/* Block and sub-block constants */
#define WMV_NUM_BLOCKS       6
#define WMV_NUM_SUB_BLOCKS   4

/* MB type mask and values */
#define WMV_MB_TYPE_MASK     0x0006
#define WMV_NO_MV_MB_TYPE    0
#define WMV_1MV_MB_TYPE      1
#define WMV_4MV_MB_TYPE      3

/* Coded block pattern mask */
#define WMV_MB_CBP_MASK      0x03F0

/* MB sub-block configurations mask and values */
#ifndef FEATURE_QDSP_RTOS
#define WMV_SBLK_CONFIG_MASK 0x30000000
#else
#define WMV_SBLK_CONFIG_MASK 0x3000
#endif
#define WMV_8x8_SBLK_CONFIG  0
#define WMV_4x8_SBLK_CONFIG  1
#define WMV_8x4_SBLK_CONFIG  2
#define WMV_4x4_SBLK_CONFIG  3

#ifndef FEATURE_QDSP_RTOS
/* Sub-block coefficient count masks */
#define WMV_SBLK_3_COEFF_COUNT_MASK 0x0FE00000
#define WMV_SBLK_2_COEFF_COUNT_MASK 0x001FC000
#define WMV_SBLK_1_COEFF_COUNT_MASK 0x00003F80
#define WMV_SBLK_0_COEFF_COUNT_MASK 0x0000007F

#else

/* Sub-block coefficient count masks */
#define WMV_SBLK_2A_COEFF_COUNT_MASK 0x3
#define WMV_SBLK_2B_COEFF_COUNT_MASK 0x1F
#define WMV_SBLK_COEFF_COUNT_MASK 0x7F

#endif


/* Globally accessible frame and MB packet structures */
WMVQDSP_FrameHeaderPktType  WMVQDSP_CurFrameHeaderPkt;
WMVQDSP_MBPktType           WMVQDSP_CurMBPkt;

/* Globally accessible pointers to the sub-packet structures within the
** MB packet.
*/
WMVQDSP_HdrSPktType        *WMVQDSP_HdrSPkt        = &WMVQDSP_CurMBPkt.Header;
WMVQDSP_MotionVecsSPktType *WMVQDSP_MotionVecsSPkt =  WMVQDSP_CurMBPkt.MotionVecs;
WMVQDSP_ModesSPktType      *WMVQDSP_ModesSPkt      = &WMVQDSP_CurMBPkt.Modes;
WMVQDSP_ResidualsSPktType  *WMVQDSP_ResidualsSPkt  =  WMVQDSP_CurMBPkt.Residuals;

/* Source and target YUV buffer indexes */
unsigned long int WMVQDSP_CurrentTargetIndex = 0;
unsigned long int WMVQDSP_CurrentSourceIndex = 1;
static VDL_Slice_Pkt_Type *pSliceBuf;
//handle to the DL layer to be used by this instance
void* pWmvDL;

DecodeStatsType *wmv_cur_frame_stats;

wmv_dec_yuv_info_type wmv_dec_yuv_info;
MP4BufType *wmv_OutputBufInfo;
signed short wmv_NextInFrameId = -1;
const  unsigned short WmvNumYUVBuffers = 8;
/* =======================================================================
** Function Definitions
** ======================================================================= */
/*===========================================================================

FUNCTION:
  WMVQDSP_WriteMBSubPackets

DESCRIPTION:
  This function writes the current MB sub-packets into the given buffer.

DEPENDENCIES
  None.

INPUT/OUTPUT PARAMETERS:
  unsigned short **ppBuf
    The pointer to the buffer in which the sub packet data should be written.

RETURN VALUE:
  None.

SIDE EFFECTS:
  None.

===========================================================================*/
void WMVQDSP_WriteMBSubPackets(unsigned short **ppBuf)
{
  /* Used in the two loops when writing out the residuals */
  unsigned char i, j;

  /* Sub-packet header location */
#ifndef FEATURE_QDSP_RTOS  
  unsigned short *pHdrSPkt;
#endif

  /* The following local variables will hold information extracted from the
  ** sub-packet fields to aid in determining how to write out the MB data
  ** to the slice buffer.
  */
  unsigned short mb_type;
  unsigned short mb_cbp;
  unsigned long int mb_sub_block_config;
  unsigned short sub_block_coeff_count[WMV_NUM_SUB_BLOCKS];
  unsigned short residual_count = 0;
#ifdef FEATURE_QDSP_RTOS
  WMVQDSP_HdrSPkt->packet_ID = 0x0002;
#endif
  /* ---------------------------------------------------------------------
  ** Write the sub-packet header
  ** --------------------------------------------------------------------- */
  memcpy ((unsigned short*)*ppBuf, (unsigned short*)WMVQDSP_HdrSPkt,
          sizeof(WMVQDSP_HdrSPktType));
#ifndef FEATURE_QDSP_RTOS  
  pHdrSPkt = (unsigned short*)*ppBuf;
#endif  
  *ppBuf += sizeof(WMVQDSP_HdrSPktType) / sizeof(unsigned short);

  /* ---------------------------------------------------------------------
  ** Write the sub-packet motion vectors
  ** --------------------------------------------------------------------- */
  /* Extract the MB type from bits 1 and 2 of the MB info field */
  mb_type = (WMVQDSP_HdrSPkt->MBInfo & WMV_MB_TYPE_MASK) >> 1;
  switch (mb_type)
  {
    case WMV_NO_MV_MB_TYPE:
      /* No MV - no motion vector data */
      break;

    case WMV_1MV_MB_TYPE:
      /* 1-MV */
      memcpy ((unsigned short*)*ppBuf, (unsigned short*)WMVQDSP_MotionVecsSPkt,
              sizeof(WMVQDSP_MotionVecsSPktType));
      *ppBuf += sizeof(WMVQDSP_MotionVecsSPktType) / sizeof(unsigned short);
      break;

    case WMV_4MV_MB_TYPE:
      /* 4-MV */
      memcpy ((unsigned short*)*ppBuf, (unsigned short*)WMVQDSP_MotionVecsSPkt,
              sizeof(WMVQDSP_MotionVecsSPktType)*4);
      *ppBuf += (sizeof(WMVQDSP_MotionVecsSPktType)*4) / sizeof(unsigned short);
      break;

    default:
      QTV_MSG_PRIO1(QTVDIAG_GENERAL, QTVDIAG_PRIO_ERROR, "Invalid MB type: %d", mb_type);
      break;
  }

  /* ---------------------------------------------------------------------
  ** Write the sub-packet modes
  ** --------------------------------------------------------------------- */
#ifndef FEATURE_QDSP_RTOS
 /* ---------------------------------------------------------------------
  ** Write the sub-packet modes
  ** --------------------------------------------------------------------- */
  memcpy ((unsigned short*)*ppBuf, (unsigned short*)WMVQDSP_ModesSPkt,
          sizeof(WMVQDSP_ModesSPktType));
  *ppBuf += sizeof(WMVQDSP_ModesSPktType) / sizeof(unsigned short);

  /* ---------------------------------------------------------------------
  ** Write the subpacket residuals, for coded blocks
  ** --------------------------------------------------------------------- */
  /* Extract the coded block pattern from the MB info field */
  mb_cbp = (WMVQDSP_HdrSPkt->MBInfo & WMV_MB_CBP_MASK) >> 4;

  /* Loop through the blocks and write the block information */
  for (i = 0; i < WMV_NUM_BLOCKS; i++)
  {
    /* Determine if block is coded.
    ** We should not rely on the check: "if (!(mb_cbp & (1 << i)))" because
    ** it can be inconsistent with the residual count due to the current
    ** limitations in the MS code, and this should change.
    */
    if (1)
    {
      /* Block is coded */
      /* Determine the sub-block configuration */
      mb_sub_block_config = (WMV_SBLK_CONFIG_MASK
                             & ((unsigned long int*)WMVQDSP_ModesSPkt)[i]) >> 28;

      /* We rely on the coeff count for a sub-block to be 0 if it is not
      ** used.
      */
      memset(sub_block_coeff_count, 0, sizeof(sub_block_coeff_count));

      switch (mb_sub_block_config)
      {
        case WMV_4x4_SBLK_CONFIG:
          sub_block_coeff_count[3] = (WMV_SBLK_3_COEFF_COUNT_MASK
                                      & ((unsigned long int*)WMVQDSP_ModesSPkt)[i]) >> 21;
          sub_block_coeff_count[2] = (WMV_SBLK_2_COEFF_COUNT_MASK
                                      & ((unsigned long int*)WMVQDSP_ModesSPkt)[i]) >> 14;
          /* fall through */
        case WMV_8x4_SBLK_CONFIG:
        case WMV_4x8_SBLK_CONFIG:
          sub_block_coeff_count[1] = (WMV_SBLK_1_COEFF_COUNT_MASK
                                      & ((unsigned long int*)WMVQDSP_ModesSPkt)[i]) >> 7;
          /* fall through */
        case WMV_8x8_SBLK_CONFIG:
          sub_block_coeff_count[0] = (WMV_SBLK_0_COEFF_COUNT_MASK
                                      & ((unsigned long int*)WMVQDSP_ModesSPkt)[i]);
          break;

        default:
          QTV_MSG_PRIO1(QTVDIAG_GENERAL, QTVDIAG_PRIO_ERROR, "Invalid sub-block config %d",
                   mb_sub_block_config);
          break;
      }

      /* If MB is flagged "skipped", and it has non-zero residuals, there is
      ** is an inconsistency. In this case we mark the MB as coded. This is
      ** due to a limitation in the MS code. This should eventually change so
      ** that we do not have to check for this inconsistency.
      */
      if ((mb_cbp & (1 << i)) && (((unsigned long int*)WMVQDSP_ModesSPkt)[i]) & 0xFFFFFFF)
      {
        QTV_MSG_PRIO3(QTVDIAG_GENERAL, QTVDIAG_PRIO_ERROR, "Skipped MB w/ residuals: %x %x, block: %d",
                  mb_cbp, ((unsigned long int*)WMVQDSP_ModesSPkt)[i], i );
        WMVQDSP_HdrSPkt->MBInfo &= ~(0x1 << (4 + i));
        QTV_MSG_PRIO2(QTVDIAG_GENERAL, QTVDIAG_PRIO_ERROR, "Bitwise AND MBInfo with: %x, %x",
                  ~(0x1 << (4 + i)), WMVQDSP_HdrSPkt->MBInfo );

        /* Re-write the sub-packet header */
        memcpy ((unsigned short*)pHdrSPkt, (unsigned short*)WMVQDSP_HdrSPkt,
                sizeof(WMVQDSP_HdrSPktType));
      }

      /* Traverse through 4x4 sub-blocks and write out residuals */
      for (j = 0; j < WMV_NUM_SUB_BLOCKS; j++)
      {
        memcpy ((unsigned short*)*ppBuf, (unsigned short*)&WMVQDSP_ResidualsSPkt[residual_count],
                sub_block_coeff_count[j] * sizeof(WMVQDSP_ResidualsSPktType));
        *ppBuf += (sub_block_coeff_count[j]
                   * sizeof(WMVQDSP_ResidualsSPktType) / sizeof(unsigned short));
        residual_count += sub_block_coeff_count[j];
      }
    }
  }

  /* Write the end of packet marker */
  *(*ppBuf)++ = QTVQDSP_PKT_END_MARKER_LO;
  *(*ppBuf)++ = QTVQDSP_PKT_END_MARKER_HI;
#else

  

   /* ---------------------------------------------------------------------
  ** Write the subpacket residuals, for coded blocks
  ** --------------------------------------------------------------------- */
  /* Extract the coded block pattern from the MB info field */
  mb_cbp = (WMVQDSP_HdrSPkt->MBInfo & WMV_MB_CBP_MASK) >> 4;



   /* Loop through the blocks and write the block information */
  for(i = 0; i < WMV_NUM_BLOCKS; i++)
  {
    if(mb_type == WMV_NO_MV_MB_TYPE || (!(mb_cbp & (1 << i))))
    {
      /* --------------------------------------------------------------------
      ** Write the sub-packet modes
      ** ------------------------------------------------------------------*/
      memcpy(*ppBuf, &(((unsigned short*)WMVQDSP_ModesSPkt)[i<<1]), sizeof(unsigned long int));
      *ppBuf += 2;
      //memcpy(*ppBuf, &(((unsigned short*)WMVQDSP_ModesSPkt)[2*i  ]), sizeof(unsigned short));
      //*ppBuf += 1;
    }
  }
 

  /* Loop through the blocks and write the block information */
  for (i = 0; i < WMV_NUM_BLOCKS; i++)
  {
    /* Determine if block is coded.
    ** We should not rely on the check: "if (!(mb_cbp & (1 << i)))" because
    ** it can be inconsistent with the residual count due to the current
    ** limitations in the MS code, and this should change.
    */
    if (!(mb_cbp & (1 << i)))
    {
      /* Block is coded */
      /* Determine the sub-block configuration */
      mb_sub_block_config = (WMV_SBLK_CONFIG_MASK
                             & ((unsigned long int*)WMVQDSP_ModesSPkt)[i]) >> 12;

      /* We rely on the coeff count for a sub-block to be 0 if it is not
      ** used.
      */
      memset(sub_block_coeff_count, 0, sizeof(sub_block_coeff_count));

      switch(mb_sub_block_config)
      {
        case WMV_4x4_SBLK_CONFIG:
          sub_block_coeff_count[3] = (WMV_SBLK_COEFF_COUNT_MASK
                                      & (((unsigned long int*)WMVQDSP_ModesSPkt)[i] >> 5));
          sub_block_coeff_count[2] = (WMV_SBLK_2A_COEFF_COUNT_MASK
                                      & (((unsigned long int*)WMVQDSP_ModesSPkt)[i] >> 30)) |
                                     ((WMV_SBLK_2B_COEFF_COUNT_MASK
                                       & (((unsigned long int*)WMVQDSP_ModesSPkt)[i])) << 2);
          /* fall through */
        case WMV_8x4_SBLK_CONFIG:
        case WMV_4x8_SBLK_CONFIG:
          sub_block_coeff_count[1] = (WMV_SBLK_COEFF_COUNT_MASK
                                      & (((unsigned long int*)WMVQDSP_ModesSPkt)[i] >> 23));
          /* fall through */
        case WMV_8x8_SBLK_CONFIG:
          sub_block_coeff_count[0] = (WMV_SBLK_COEFF_COUNT_MASK
                                      & (((unsigned long int*)WMVQDSP_ModesSPkt)[i] >> 16));
          break;

        default:
          QTV_MSG_PRIO1(QTVDIAG_GENERAL, QTVDIAG_PRIO_ERROR, "Invalid sub-block config %d",
                        mb_sub_block_config);
          break;
      }


      /* Traverse through 4x4 sub-blocks and write out residuals */
      for(j = 0; j < WMV_NUM_SUB_BLOCKS; j++)
      {
        int numSubBlkCoeffs = (sub_block_coeff_count[j] == 0x7F ? 32 : 
                               sub_block_coeff_count[j]);

        memcpy ((unsigned short*)*ppBuf,
                (unsigned short*)&WMVQDSP_ResidualsSPkt[residual_count],
                numSubBlkCoeffs * sizeof(WMVQDSP_ResidualsSPktType));
        *ppBuf += (numSubBlkCoeffs
                   * sizeof(WMVQDSP_ResidualsSPktType) / sizeof(unsigned short));
        residual_count += numSubBlkCoeffs;
      }
    }
  }

*(*ppBuf)++ = QTVQDSP_PKT_END_MARKER_HI;
#endif

}

/*===========================================================================

FUNCTION:
  WMVQDSP_WriteFrameHeader

DESCRIPTION:
  This function writes the current frame header into the given buffer.

DEPENDENCIES
  None.

INPUT/OUTPUT PARAMETERS:
  unsigned short **ppBuf
    The pointer to the buffer in which the sub packet data should be written.

RETURN VALUE:
  None.

SIDE EFFECTS:
  None.

===========================================================================*/
void WMVQDSP_WriteFrameHeader(unsigned short **ppBuf)
{
  unsigned short numBytes = sizeof(WMVQDSP_FrameHeaderPktType);
#ifndef FEATURE_QDSP_RTOS
  WMVQDSP_CurFrameHeaderPkt.NumWords = (numBytes-4)>>2;
#ifdef FEATURE_QTV_WINDOWS_MEDIA_SMCDB_SUPPORT
  if (qtv_cfg_enable_wmv3_smcdb_support) 
  {
    /* Set SMCDB bit */
    WMVQDSP_CurFrameHeaderPkt.FrameQuant |= 1 << 7;
  }
#endif /* FEATURE_QTV_WINDOWS_MEDIA_SMCDB_SUPPORT */
#endif
  memcpy ((unsigned short*)*ppBuf, (unsigned short*)&WMVQDSP_CurFrameHeaderPkt, numBytes);
  *ppBuf += numBytes>>1;
}

/*===========================================================================

FUNCTION:
  WMVQDSPWriteMBPacketToSliceBuffer

DESCRIPTION:
  The function handles the transfer of data from the MB packets to the slice
  buffer.

DEPENDENCIES
  Should be called only after a complete MB has been decoded.

INPUT/OUTPUT PARAMETERS:
  unsigned short mb_num
    The current MB number.

  unsigned short total_num_mbs
    The total number of MBs in the frame.

RETURN VALUE:
  None.

SIDE EFFECTS:
  None.

===========================================================================*/
void WMVQDSP_MBPacketToSliceBuffer
(
  unsigned short mb_num,
  unsigned short total_num_mbs
)
{

#ifndef FEATURE_QDSP_RTOS
  unsigned long int *pFirstWord;
#else
  unsigned short *pFirstWord;
#endif
  unsigned long int numBytesWritten;
  signed short WMVNextInFrameId = -1;
  unsigned char* p;

  if (mb_num == 0)
  {
    /* This is the start of a new frame. Use a new slice buffer */
    pSliceBuf = VDL_Get_Slice_Buffer(pWmvDL);
    if (pSliceBuf == NULL)
    {
      QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_FATAL, "Out of memory");
      return;
    }

    pSliceBuf->fFirstMB        = TRUE;
    pSliceBuf->fAllocateYUVBuf = TRUE;
    pSliceBuf->CodecType       = VDL_CODEC_WMV9;

    WMVNextInFrameId = WMVDecGetNextFreeYUVBufIdx();
    ASSERT( WMVNextInFrameId >= 0 );

 #ifndef FEATURE_QDSP_RTOS
    /* Set the frame ID to equal the target index */
    WMVQDSP_CurFrameHeaderPkt.FrameInfo |= WMVNextInFrameId<<8;

    WMVQDSP_CurFrameHeaderPkt.FrameInfo |= WMVQDSP_CurrentTargetIndex<<2;
    WMVQDSP_CurFrameHeaderPkt.FrameInfo |= WMVQDSP_CurrentSourceIndex<<4;
 #else
    WMVQDSP_CurFrameHeaderPkt.FrameInfo |= WMVQDSP_CurrentTargetIndex<<2;
    WMVQDSP_CurFrameHeaderPkt.FrameInfo |= WMVQDSP_CurrentSourceIndex;
 #endif
#ifdef FEATURE_QTV_WINDOWS_MEDIA_SMCDB_SUPPORT
   if(qtv_cfg_enable_wmv3_smcdb_support)
   {
     p = (unsigned char*)wmv_OutputBufInfo->pOutputYUVBuf[WMVNextInFrameId];
   }
   else
#endif
   {
     p = (unsigned char*)wmv_OutputBufInfo->pYUVBuf[WMVNextInFrameId];
   }


   wmv_cur_frame_stats->pOutputYUVBuf = p;

     /* Store the value of NextInFrameId which we set in mp4_set_frame_header in the
	  ** stats buffer. This value will be used by us when we want to rewind to the correct
	  ** value of NextInFrameId after we flush out the stats Q and slice Q during repositioning
	  ** in mp4_process_flush. If the value of NextInFrameId is incorrectly set then the
	  ** ARM-DSP sync will be disturbed and we could encounter video freeze after repositioning.
	  */

	 wmv_cur_frame_stats->OutBufId = WMVNextInFrameId;


	/*
	There is a 1:1 mapping between the index of MP4CodecInfo.OutputBufInfo.pOutputYUVBuf
	(output display buffers, if deblocking is turned on) and the index of 
	MP4CodecInfo.OutputBufInfo.pYUVBuf (output internal buffers, also used for referencing by DSP)..
	So the address from one of the MP4CodecInfo.OutputBufInfo.pYUVBuf's should be stored in 
	the frame header: Mpeg4FrameHdr.VopBuf'X'Hi/Lo [X=0,1,2]..
	
	Now lets say there are 3 frames with successive time stamps and each predicted
	from the previous one i.e. Timestamp of f1 < Timestamp of f2 < Timestamp of f3
	The reference frame for f2 is f1 and the reference frame for f3 is f2.
	The OutputVOPIndex for f1 shall become the sourceVOPIndex (MP4CodecInfo.currentSourceLayer)
	for f2. Similarly, the OutputVOPIndex for f2 shall become the sourceVOPIndex
	for f3. So all we need to make sure is that DSP knows about the output YUV buffer and its 
	corresponding source YUV buffer for the current frame. 
	
	MP4DecGetNextFreeYUVBufIdx() gets the index of the next output YUV buffer that can 
	be used. We use this value to index into MP4CodecInfo.OutputBufInfo.pYUVBuf and 
	write the buffers address into the Mpeg4FrameHdr based on the value of the OutputVOPIndex
	as done below:
	*/
	
	/*
	WMV special: The address of reference buffers are specified to the DSP using
	WMVQDSP_CurFrameHeaderPkt.FrameBuf0 and WMVQDSP_CurFrameHeaderPkt.FrameBuf1 and the 
	deblocked Output YUV buffer are specified using
	WMVQDSP_CurFrameHeaderPkt.FrameBuf2 and WMVQDSP_CurFrameHeaderPkt.FrameBuf3.
	If deblocking is off output will be the same as the Reference/Source Buffer. 
	
	Also there is a correlation between Reference/Source YUV and deblocked Output buffer.
	For reference/source buffer WMVQDSP_CurFrameHeaderPkt.FrameBuf0 deblocked Output is put in
	WMVQDSP_CurFrameHeaderPkt.FrameBuf2 and for reference/source buffer WMVQDSP_CurFrameHeaderPkt.FrameBuf1
	deblocked Output is put in WMVQDSP_CurFrameHeaderPkt.FrameBuf3.
	*/
#ifndef FEATURE_QDSP_RTOS
#ifdef FEATURE_QTV_WINDOWS_MEDIA_SMCDB_SUPPORT
   if(qtv_cfg_enable_wmv3_smcdb_support)
   {
     if(WMVQDSP_CurrentTargetIndex == 0)
	 {
	   WMVQDSP_CurFrameHeaderPkt.FrameBuf2 = (unsigned long int)p;
	 }
	 else if(WMVQDSP_CurrentTargetIndex == 1)
	 {
	   WMVQDSP_CurFrameHeaderPkt.FrameBuf3 = (unsigned long int)p;
	 }/* although we could ahve 3 but we are toggling b/w 0 nad 1..so 2 never happens*/
	 else
	 {
	    ASSERT(0);
	 }
   }
#endif
#else
   if(WMVQDSP_CurrentTargetIndex == 0)
   {
     WMVQDSP_CurFrameHeaderPkt.FrameBuf0Hi = (unsigned long int)p >> 16;
     WMVQDSP_CurFrameHeaderPkt.FrameBuf0Lo = (unsigned long int)p & 0xFFFF;
   }
   else if(WMVQDSP_CurrentTargetIndex == 1)
   {
     WMVQDSP_CurFrameHeaderPkt.FrameBuf1Hi = (unsigned long int)p >> 16;
     WMVQDSP_CurFrameHeaderPkt.FrameBuf1Lo = (unsigned long int)p & 0xFFFF;
   }/* although we could ahve 3 but we are toggling b/w 0 nad 1..so 2 never happens*/
   else
   {
     ASSERT(0);
   }
#endif
 	
    QTV_MSG_PRIO2(QTVDIAG_GENERAL, QTVDIAG_PRIO_HIGH,
                "WMVQDSP_CurrentTargetIndex = %d ,WMVNextInFrameId being sent to DSP = %d",
				WMVQDSP_CurrentTargetIndex,
                WMVNextInFrameId
				);
 #ifndef FEATURE_QDSP_RTOS
    QTV_MSG_PRIO4(QTVDIAG_GENERAL, QTVDIAG_PRIO_HIGH,
                ",OutputYUV: FrameBuf2=0x%x, FrameBuf3=0x%x,||| Reference YUV: FrameHdr: FrameBuf0=0x%x, FrameBuf1=0x%x",
				WMVQDSP_CurFrameHeaderPkt.FrameBuf2,
				WMVQDSP_CurFrameHeaderPkt.FrameBuf3,
                WMVQDSP_CurFrameHeaderPkt.FrameBuf0,
                WMVQDSP_CurFrameHeaderPkt.FrameBuf1
                );
 #else
    QTV_MSG_PRIO4(QTVDIAG_GENERAL, QTVDIAG_PRIO_HIGH,
                ",OutputYUV: FrameBuf2Hi=0x%x,FrameBuf2Lo=0x%x, ||| Reference YUV0: FrameHdr: FrameBuf1Hi=0x%x, FrameBuf1Lo=0x%x",
				WMVQDSP_CurFrameHeaderPkt.FrameBuf2Hi,
				WMVQDSP_CurFrameHeaderPkt.FrameBuf2Lo,
                WMVQDSP_CurFrameHeaderPkt.FrameBuf1Hi,
                WMVQDSP_CurFrameHeaderPkt.FrameBuf1Lo
                );
    QTV_MSG_PRIO2(QTVDIAG_GENERAL, QTVDIAG_PRIO_HIGH,
                ", Reference YUV1: FrameHdr: FrameBuf0Hi=0x%x, FrameBuf0Lo=0x%x",
                WMVQDSP_CurFrameHeaderPkt.FrameBuf0Hi,
                WMVQDSP_CurFrameHeaderPkt.FrameBuf0Lo
                );
 #endif

    WMVQDSP_WriteFrameHeader(&pSliceBuf->pNextMacroBlock);

#ifndef FEATURE_QDSP_RTOS
    pSliceBuf->NumMacroBlocks++;
#endif
    pSliceBuf->SliceDataSize = sizeof( WMVQDSP_CurFrameHeaderPkt );
  }
  else if( mb_num >= total_num_mbs )
  {
    QTV_MSG_PRIO1(QTVDIAG_GENERAL, QTVDIAG_PRIO_FATAL, "Past MB boundary MB count %d", mb_num);
    return;
  }

  if (pSliceBuf)
  {
    /* Save the first word so we can write the size into it after we process
    ** the packet and determine the size of the packet.
    */
#ifndef FEATURE_QDSP_RTOS
    pFirstWord = (unsigned long int*)pSliceBuf->pNextMacroBlock;
    pSliceBuf->pNextMacroBlock = pSliceBuf->pNextMacroBlock + 2;
#else
    pFirstWord = (unsigned short*)pSliceBuf->pNextMacroBlock;
    pSliceBuf->pNextMacroBlock = pSliceBuf->pNextMacroBlock + 1;
#endif

    WMVQDSP_WriteMBSubPackets(&pSliceBuf->pNextMacroBlock);
 #ifndef FEATURE_QDSP_RTOS
    if(*(pSliceBuf->pNextMacroBlock-2) != QTVQDSP_PKT_END_MARKER_LO
       || *(pSliceBuf->pNextMacroBlock-1) != QTVQDSP_PKT_END_MARKER_HI)
    {
      QTV_MSG_PRIO2(QTVDIAG_GENERAL, QTVDIAG_PRIO_FATAL, "No end of packet marker %x %x",
               *(pSliceBuf->pNextMacroBlock-2),
               *(pSliceBuf->pNextMacroBlock-1));
    }
 #else
    if(*(pSliceBuf->pNextMacroBlock-1) != QTVQDSP_PKT_END_MARKER_HI)
      {
      QTV_MSG_PRIO1(QTVDIAG_GENERAL, QTVDIAG_PRIO_FATAL, "No end of packet marker %x",
                              *(pSliceBuf->pNextMacroBlock-1));
    }
 #endif
   
    /* Number of bytes written including the size word
    */
    numBytesWritten = (unsigned long int)pSliceBuf->pNextMacroBlock - (unsigned long int)pFirstWord;

    pSliceBuf->SliceDataSize += numBytesWritten;
    pSliceBuf->NumMacroBlocks++;
 #ifndef FEATURE_QDSP_RTOS
    *pFirstWord = (numBytesWritten - 4) >> 2;
 #else
    *pFirstWord = (numBytesWritten) >> 1;
 #endif

    if(mb_num == (total_num_mbs - 1))
    {
       pSliceBuf->fLastMB = TRUE;
#ifdef FEATURE_QTV_LOG_MP4_QDSP_SLICE_BUFFER
      mp4_log_slice_buffer(pSliceBuf,3);
#endif
      
      /* We are about to queue the dequeued slice buffer onto the mp4_slice_q.
      */
      /* Queue the slice to the graph task and dont signal it to be picked up
      ** yet.
      */
      (void)VDL_Queue_Slice_Buffer(pWmvDL,pSliceBuf);

      pSliceBuf = NULL;
    }
    else if((pSliceBuf->SliceBufferSize - pSliceBuf->SliceDataSize) < WMV_MAX_GUARD_SIZE)
    {
      pSliceBuf->fLastMB = FALSE;
#ifdef FEATURE_QTV_LOG_MP4_QDSP_SLICE_BUFFER
      mp4_log_slice_buffer(pSliceBuf,3);
#endif
      /* We are about to queue the dequeued slice buffer onto the mp4_slice_q.
      */
      /* Queue the slice to the graph task and dont signal it to be picked up
      ** yet
      */
      (void)VDL_Queue_Slice_Buffer(pWmvDL,pSliceBuf);

      pSliceBuf = NULL;
      /* Get another slice buffer for the next packet */
      pSliceBuf = VDL_Get_Slice_Buffer(pWmvDL);
      if (pSliceBuf == NULL)
      {
        QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_FATAL, "Out of memory");
        return;
      }

      pSliceBuf->fFirstMB        = FALSE;
      pSliceBuf->fAllocateYUVBuf = FALSE;
      pSliceBuf->CodecType       = VDL_CODEC_WMV9;
    }
    /* Else wait for next MB to pack into the slice buffer and keep packing
    ** until slice buffer is full before putting it on the slice queue.
    */
  }
}
/*===========================================================================
FUNCTION:
  PAL_WMVQueueStatsBuffer

DESCRIPTION:
  Queue the stats Buffer
  

INPUT/OUTPUT PARAMETERS:
 Stats buffer

RETURN VALUE:
  None

SIDE EFFECTS: 
==============================================================================*/
void PAL_WMVQueueStatsBuffer(void *pDecodeStats )
{
   DecodeStatsType* pStats_temp = (DecodeStatsType*)pDecodeStats;
  (void)VDL_Queue_Stats_Buffer( pWmvDL,pDecodeStats,pStats_temp->ReturnCode );
}

/*===========================================================================
FUNCTION:
  PAL_WMVQueueEOSStatsBuffer

DESCRIPTION:
  Queue the EOS stats Buffer
  

INPUT/OUTPUT PARAMETERS:
 Stats buffer

RETURN VALUE:
  None

SIDE EFFECTS: 
==============================================================================*/
VDL_ERROR PAL_WMVQueueEOSStatsBuffer( DecodeStatsType* pDecoderStatsEOS)
{
	return VDL_Queue_Stats_Buffer( pWmvDL,(void*) pDecoderStatsEOS, VDL_END_OF_SEQUENCE );
}

/*===========================================================================
FUNCTION:
  PAL_WMVFreeCurrentSliceBuffer

DESCRIPTION:
  Frees the current slice buffer in use
  

INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE:
  None

SIDE EFFECTS: 
==============================================================================*/
void PAL_WMVFreeCurrentSliceBuffer(void)
{
  if(pSliceBuf != NULL)
  {
    (void)VDL_Free_Slice_Buffer(pWmvDL,pSliceBuf);
    pSliceBuf = NULL;
  }
  return;
}

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
void WMVDecReleaseYUVBuf(unsigned char *yuv_buf, unsigned long long  pts)
{
  signed long int  yuv_buf_idx;
  
  pthread_mutex_lock(&wmv_dec_yuv_info.cs);


  /*
   Convert the address to a YUV buffer index
  */
  for (yuv_buf_idx = 0; yuv_buf_idx < WmvNumYUVBuffers; yuv_buf_idx++)
  {
    if (yuv_buf == wmv_OutputBufInfo->pYUVBuf[yuv_buf_idx])
    {
      break;
    }
  }


  if (yuv_buf_idx < WmvNumYUVBuffers)
  {
    if (FALSE == wmv_dec_yuv_info.bufs[yuv_buf_idx].pending_release)
    {       
    
      QTV_MSG_PRIO1(QTVDIAG_GENERAL, QTVDIAG_PRIO_HIGH,"MP4DecReleaseYUVBuf: Unexpected double release on YUV buf index %d (same "
                    "frame released twice?)", 
                    yuv_buf_idx);      
    }
    else
    {
      /* Releasing a pending buffer (normal case)  */
      if (wmv_dec_yuv_info.bufs[yuv_buf_idx].pts != pts)
      {        
        /*
         PTS copied down during buffer allocation does not equal the PTS
         returned after render.  Perhaps this was caused by 2 DSP done's
         being rxd out of order, and so the YUV bufs got their TS's mixed
         up.  mp4 ISR problem.
         Another Reason could be we are freeing a non-Coded VOP which refers
         the same YUV buffer as the immediately previous Coded VOP. The stats
         buffer entries of the above two VOPs are same, except the FrameTimeStamp
         field. So in this case it is perfectly alright for us to get a 
         time stamp mis-match.
        */
        QTV_MSG_PRIO2(QTVDIAG_GENERAL, QTVDIAG_PRIO_FATAL,"MP4DecReleaseYUVBuf:Stats buf TS = %dms does not match allocd TS = "
                     "%dms (2 DSP done's rxd out of order?)",
                     pts, wmv_dec_yuv_info.bufs[yuv_buf_idx].pts);
      
      }
      else if ( wmv_dec_yuv_info.prev_pts_released >= 
                wmv_dec_yuv_info.bufs[yuv_buf_idx].pts) 
      {
        /* YUV bufs released out of order   */
        if (wmv_dec_yuv_info.prev_pts_released == 
           wmv_dec_yuv_info.bufs[yuv_buf_idx].pts)
        {
         /* Both TS's are equal */
         QTV_MSG_PRIO1(QTVDIAG_GENERAL, QTVDIAG_PRIO_FATAL,"MP4DecReleaseYUVBuf:Prev and curr released frame bufs have same "
                      "TS = %dms",
                      wmv_dec_yuv_info.bufs[yuv_buf_idx].pts);
        }
        else
        {
         /*
           TS's out of order.  This is bad, and will (did) result in out
           of order rendering.  Unless the previous frame was dropped 
           during this frame's render, which happens a lot when the MDP
           takes too long.  Since we have no way to tell at this layer why
           the YUV buf is being released, we leave it up to the user to
         verify.
         */
         QTV_MSG_PRIO2(QTVDIAG_GENERAL, QTVDIAG_PRIO_FATAL,"MP4DecReleaseYUVBuf:Frame bufs released out of order!  "
                       "%dms -> %dms (ignore if either was dropped)",
                       wmv_dec_yuv_info.prev_pts_released, 
                       wmv_dec_yuv_info.bufs[yuv_buf_idx].pts);
        }
      }    
    }

    /* Release the buffer */
    if(wmv_dec_yuv_info.bufs[yuv_buf_idx].reference_cnt > 0)
    {
      wmv_dec_yuv_info.bufs[yuv_buf_idx].reference_cnt--;
    }

    if(wmv_dec_yuv_info.bufs[yuv_buf_idx].reference_cnt == 0)
    {
      QTV_MSG_PRIO2(QTVDIAG_GENERAL, QTVDIAG_PRIO_LOW,
                   "Releasing YUV buf. Index: %d PTS: %dms",
                   yuv_buf_idx, wmv_dec_yuv_info.bufs[yuv_buf_idx].pts);
      wmv_dec_yuv_info.bufs[yuv_buf_idx].pending_release = FALSE;
    }
    else
    {
      QTV_MSG_PRIO3(QTVDIAG_GENERAL, QTVDIAG_PRIO_LOW,
                 "YUV buf. Index: %d PTS: %dms Not Released - ref count: %d",
                  yuv_buf_idx, wmv_dec_yuv_info.bufs[yuv_buf_idx].pts,
                  wmv_dec_yuv_info.bufs[yuv_buf_idx].reference_cnt);
    }
    
    /*
    Record this released buffer's PTS for use in determining if the next buffer was
     release in order.
    */
    wmv_dec_yuv_info.prev_pts_released = wmv_dec_yuv_info.bufs[yuv_buf_idx].pts;            
  }
  else
  {
    QTV_MSG_PRIO1(QTVDIAG_GENERAL, QTVDIAG_PRIO_FATAL,"Unknown mp4 YUV buf address = 0x%x, cannot release", 
             yuv_buf);
  }
  pthread_mutex_unlock(&wmv_dec_yuv_info.cs);

}

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
unsigned char WMVDecIsYUVBufAvailableForDecode(void)
{
  int index;
  pthread_mutex_lock(&wmv_dec_yuv_info.cs);
  
  for (index = 0; index < WmvNumYUVBuffers; index++)
  {
    if ( FALSE == wmv_dec_yuv_info.bufs[index].pending_release )
    {
      break;
    }
  }
  
  pthread_mutex_unlock(&wmv_dec_yuv_info.cs); 
  
  if (index == WmvNumYUVBuffers)
  {
    QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_HIGH,"mp4:no YUV buffer available for decode");
    return FALSE;
  }      

  return TRUE;
}

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
unsigned char WMVDecIsSliceBufAvailableForDecode(void)
{
  unsigned char sliceBufAvailable = FALSE;
  VDL_IsSliceBufAvailableForDecode( pWmvDL, &sliceBufAvailable );
  return sliceBufAvailable;
}

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
void WMVDecInitPendingRelease(void)
{
  unsigned int index;

  pthread_mutex_lock(&wmv_dec_yuv_info.cs);
  for (index = 0; index < WmvNumYUVBuffers; index++)
  {
     wmv_dec_yuv_info.bufs[index].pending_release = TRUE;
     wmv_dec_yuv_info.bufs[index].pts = 0;
     wmv_dec_yuv_info.bufs[index].reference_cnt = 0;
  }
  
  wmv_dec_yuv_info.prev_pts_released = 0;
  pthread_mutex_unlock(&wmv_dec_yuv_info.cs);

}

/***********************************************************************/
/*
  WMVUpdateYUVRefCnt() 
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
void WMVUpdateYUVRefCnt(unsigned long long timeStamp)
{
  unsigned long int index = 0;

  pthread_mutex_lock(&wmv_dec_yuv_info.cs);
  for(index = 0; index < WmvNumYUVBuffers; index++)
  {
    if(wmv_dec_yuv_info.bufs[index].pts == timeStamp)
    {
      wmv_dec_yuv_info.bufs[index].reference_cnt++;
    }
  }
  pthread_mutex_unlock(&wmv_dec_yuv_info.cs);
}

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
void WMVDecReleaseCurrYUVBufIdx(unsigned long long timeStamp)
{
  unsigned long int index = 0;

  pthread_mutex_lock(&wmv_dec_yuv_info.cs);
  for(index = 0; index < WmvNumYUVBuffers; index++)
  {
    if(wmv_dec_yuv_info.bufs[index].pts == timeStamp)
    {
      wmv_dec_yuv_info.bufs[index].pending_release = FALSE;
      wmv_dec_yuv_info.bufs[index].pts = 0 ;
      wmv_dec_yuv_info.bufs[index].reference_cnt = 0;
      break;
    }
  }
  if (index == WmvNumYUVBuffers)
  {
   QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_ERROR, "No buffer with this timeStamp!!!" );
  }
  pthread_mutex_unlock(&wmv_dec_yuv_info.cs);

}

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
signed short WMVDecGetNextFreeYUVBufIdx(void)
{
  unsigned long int  index = 0;
  signed short currFrameId = -1;

  pthread_mutex_lock(&wmv_dec_yuv_info.cs);

  currFrameId = wmv_NextInFrameId;
  for (index = 0; index < WmvNumYUVBuffers; index++)
  {
    currFrameId = (currFrameId + 1) % wmv_OutputBufInfo->numYUVBuffers;

    if ( FALSE == wmv_dec_yuv_info.bufs[currFrameId].pending_release )
    {          
     break;
    }
  }     
  
  if (index == WmvNumYUVBuffers)
  {
    /*
      All YUV buffers are occupied by the client and pending release.  
    */
    QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_ERROR, "All YUV buffers pending release!" );
    currFrameId = -1;
  }
  else
  {
    wmv_dec_yuv_info.bufs[currFrameId].pending_release = TRUE;
    wmv_dec_yuv_info.bufs[currFrameId].pts = wmv_cur_frame_stats->FrameDisplayTime ;
    wmv_dec_yuv_info.bufs[currFrameId].reference_cnt = 1;
  }

  pthread_mutex_unlock(&wmv_dec_yuv_info.cs);
  wmv_NextInFrameId = currFrameId;
  return currFrameId;
}


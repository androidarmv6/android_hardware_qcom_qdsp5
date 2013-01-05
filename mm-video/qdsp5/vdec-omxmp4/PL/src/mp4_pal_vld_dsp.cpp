/*
=============================================================================
        MP4_PAL_VLD_DSP.cpp
  
  DESCRIPTION:
   This file provides the interface specification for the MPEG4 PAL supporting
   VLD in DSP interface.

  ABSTRACT:

        Copyright © 1999-2002 QUALCOMM Incorporated. 
               All Rights Reserved.
            QUALCOMM Proprietary

============================================================================*/
/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/MPEG4Vdec/main/latest/src/PL/mp4_pal_vld_dsp.cpp#3 $
$DateTime: 2009/04/09 12:01:33 $
$Change: 883980 $

========================================================================== */

/* =======================================================================
**               Includes and Public Data Declarations
** ======================================================================= */
#include "mp4_pal_vld_dsp.h"

#include "qtv_msg.h"

#if defined (T_MSM7500) && !defined (PLATFORM_LTK)
  #include "pmem.h"
#else
#define PMEM_GET_PHYS_ADDR(x) x
#endif

extern "C" {
#include "VDL_API.h"
#include "assert.h"
}

#define MP4_VLD_DSP_FRAME_HEADER_PACKET_TYPE (0xBD01)
#define MP4_VLD_DSP_SLICE_HEADER_PACKET_TYPE (0xBD02)
#define MP4_VLD_DSP_END_MARKER            (0x7FFF)
#define MP4_VLD_DSP_VGA_PART1_OFFSET    60 * 1024
#define MP4_VLD_DSP_WVGA_PART1_OFFSET   75 * 1024

/*===========================================================================
FUNCTION:
  MP4_PAL_VLD_DSP constructor

DESCRIPTION:
  Constructor
  
INPUT/OUTPUT PARAMETERS:
  pointer to VDL instance

RETURN VALUE:
  None
==============================================================================*/
MP4_PAL_VLD_DSP::MP4_PAL_VLD_DSP(void *pDL)
{
  //assign VDL instance pointer to the VDL pointer obtained from PAL
  //initialize all pointers to NULL
  pDLPtr = pDL;
  pSliceWrite = NULL;
  pCurrentSliceBuf_VLD_DSP = NULL;
  pDP1StartAddress = NULL;
  pSliceWriteForPart1 = NULL;
  pSliceHeader = NULL;
  pFirstSliceHeader = NULL;
  QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH, "MP4_PAL_VLD_DSP created\n");
}

/*===========================================================================
FUNCTION:
  Destructor

DESCRIPTION:
  Destroys PAL VLD in DSP instance.
  
INPUT/OUTPUT PARAMETERS:
 None

RETURN VALUE:
  None
==============================================================================*/
MP4_PAL_VLD_DSP::~MP4_PAL_VLD_DSP()
{
  pDLPtr = NULL;
  pSliceWrite = NULL;
  pCurrentSliceBuf_VLD_DSP = NULL;
  pDP1StartAddress = NULL;
  pSliceWriteForPart1 = NULL;
  pSliceHeader = NULL;  
  QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_DEBUG, "MP4_PAL_VLD destructed");
}

/*===========================================================================
FUNCTION:     
  InitFrameHeader

DESCRIPTION:  
  Fills the default frame header 
  
INPUT/OUTPUT PARAMETERS:
  pointer to array of buffers of type MP4BufType,
  width of the frame, height of the frame
  
RETURN VALUE: 
  None
===========================================================================*/
void MP4_PAL_VLD_DSP::InitFrameHeader(MP4BufType* pMP4Buf,unsigned short Width,unsigned short Height)
{
  unsigned char *p;

  if (Width * Height <= MP4_VGA_WIDTH * MP4_VGA_HEIGHT) 
  {
    offsetSliceDataPart1 = MP4_VLD_DSP_VGA_PART1_OFFSET;
  }
  else
  {
    offsetSliceDataPart1 = MP4_VLD_DSP_WVGA_PART1_OFFSET;
  }

  QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH, "offsetSliceDataPart1 %d\n", offsetSliceDataPart1);

  MP4_PL_VLD_DSP_Frame_Header.PacketId  = MP4_VLD_DSP_FRAME_HEADER_PACKET_TYPE;  
  MP4_PL_VLD_DSP_Frame_Header.Width = Width;    
  MP4_PL_VLD_DSP_Frame_Header.Height = Height;
  MP4_PL_VLD_DSP_Frame_Header.LineWidth = Width;

  /* Init the YUV buffer pointers. For Mpeg4 this address will be 
     reset on a per frame basis as-n-when display buffers become available
     from the upper client layer. There should be a 1:1 mapping between 
     the reference buffers and the display buffers */

  p = (unsigned char *)PMEM_GET_PHYS_ADDR((unsigned char *)pMP4Buf->pYUVBuf[0]);
  MP4_PL_VLD_DSP_Frame_Header.VopBuf0Hi = (((unsigned long int)p) >> 16) & 0xFFFF;
  MP4_PL_VLD_DSP_Frame_Header.VopBuf0Lo = ((unsigned long int)p) & 0xFFFF;

  p = (unsigned char *)PMEM_GET_PHYS_ADDR((unsigned char *)pMP4Buf->pYUVBuf[1]);
  MP4_PL_VLD_DSP_Frame_Header.VopBuf1Hi = (((unsigned long int)p) >> 16) & 0xFFFF;
  MP4_PL_VLD_DSP_Frame_Header.VopBuf1Lo = ((unsigned long int)p) & 0xFFFF;

  p = (unsigned char *)PMEM_GET_PHYS_ADDR((unsigned char *)pMP4Buf->pYUVBuf[2]);
  MP4_PL_VLD_DSP_Frame_Header.VopBuf2Hi = (((unsigned long int)p) >> 16) & 0xFFFF;
  MP4_PL_VLD_DSP_Frame_Header.VopBuf2Lo = ((unsigned long int)p) & 0xFFFF;

  MP4_PL_VLD_DSP_Frame_Header.EndMarker = MP4_VLD_DSP_END_MARKER;

}

/*===========================================================================
FUNCTION:    
  GetNewSliceBuf()

DESCRIPTION:  
  This function is called to get a new slice buffer to send to the DSP

INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE: 
  PL_ERROR
===========================================================================*/
PL_ERROR MP4_PAL_VLD_DSP::GetNewSliceBuf()
{
  pSliceWrite = GetNewSliceBufferDataPtr();
  if (pSliceWrite == NULL) return PL_ERR_OUT_OF_MEMORY;
  return PL_ERR_NONE;
}

/*===========================================================================
FUNCTION:    
  SetFrameHeaderAddresses()

DESCRIPTION:  
  This function is called to set frame header addresses

INPUT/OUTPUT PARAMETERS:
  pointer to array of buffers of type MP4BufType,
  frame index, previous frame index, frame index before previous one,
  pointer to stats buffer

RETURN VALUE: 
  None
===========================================================================*/
void MP4_PAL_VLD_DSP::SetFrameHeaderAddresses(MP4BufType* pMP4Buf, signed short NextInFrameId, 
                                              signed short prevRefFrameIndex, signed short prevPrevRefFrameIndex,
                                              DecodeStatsType *pDecodeStats)
{
  unsigned char* pOutBuf;
  unsigned char* pVopBuf;
  unsigned char* pBackwardRefVopBuf;
  unsigned char* pForwardRefVopBuf;

  /* Format the global instance of the frame header */

  ASSERT( NextInFrameId >= 0 );
  ASSERT( NextInFrameId < pMP4Buf->numOutputYUV_buffers );
  ASSERT( pMP4Buf->numOutputYUV_buffers == pMP4Buf->numYUVBuffers );

  /* Format the correct output YUV buffer's */
  // Convert to physical before passing to DSP
  pOutBuf = (unsigned char*)PMEM_GET_PHYS_ADDR(pMP4Buf->pOutputYUVBuf[NextInFrameId]);  
  MP4_PL_VLD_DSP_Frame_Header.OutBufHi = (((unsigned long int)pOutBuf) >> 16) & 0xFFFF;
  MP4_PL_VLD_DSP_Frame_Header.OutBufLo = ((unsigned long int)pOutBuf) & 0xFFFF;

  pVopBuf = (unsigned char*)PMEM_GET_PHYS_ADDR(pMP4Buf->pYUVBuf[NextInFrameId]);
 
  /*
  There is a 1:1 mapping between the index of pMP4Buf->pOutputYUVBuf
  (output display buffers, if deblocking is turned on) and the index of 
  pMP4Buf->pYUVBuf (output internal buffers, also used for referencing by DSP)..
  So the address from one of the pMP4Buf->pYUVBuf's should be stored in 
  the frame header: m_Mpeg4FrameHdr.VopBuf'X'Hi/Lo [X=0,1,2]..

  Now lets say there are 3 frames with successive time stamps and each predicted
  from the previous one i.e. Timestamp of f1 < Timestamp of f2 < Timestamp of f3
  The reference frame for f2 is f1 and the reference frame for f3 is f2.
  The OutputVOPIndex for f1 shall become the sourceVOPIndex (m_currentSourceLayer)
  for f2. Similarly, the OutputVOPIndex for f2 shall become the sourceVOPIndex
  for f3. So all we need to make sure is that DSP knows about the output YUV buffer and its 
  corresponding source YUV buffer for the current frame. 

  MP4DecGetNextFreeYUVBufIdx() gets the index of the next output YUV buffer that can 
  be used. We use this value to index into pMP4Buf->pYUVBuf and 
  write the buffers address into the m_Mpeg4FrameHdr based on the value of the OutputVOPIndex
  as done below:
  */
  if ( prevRefFrameIndex >= 0 )
  {
    // Convert to physical before passing to DSP
    pBackwardRefVopBuf = (unsigned char*)PMEM_GET_PHYS_ADDR(pMP4Buf->pYUVBuf[prevRefFrameIndex]);
    MP4_PL_VLD_DSP_Frame_Header.VopBuf0Hi = (((unsigned long int)pBackwardRefVopBuf) >> 16) & 0xFFFF;
    MP4_PL_VLD_DSP_Frame_Header.VopBuf0Lo = ((unsigned long int)pBackwardRefVopBuf) & 0xFFFF;
  }
  if ( prevPrevRefFrameIndex >= 0 )
  {
    // Convert to physical before passing to DSP
    pForwardRefVopBuf = (unsigned char*)PMEM_GET_PHYS_ADDR(pMP4Buf->pYUVBuf[prevPrevRefFrameIndex]);    
    MP4_PL_VLD_DSP_Frame_Header.VopBuf1Hi = (((unsigned long int)pForwardRefVopBuf) >> 16) & 0xFFFF;
    MP4_PL_VLD_DSP_Frame_Header.VopBuf1Lo = ((unsigned long int)pForwardRefVopBuf) & 0xFFFF;
  }

  MP4_PL_VLD_DSP_Frame_Header.VopBuf2Hi = (((unsigned long int)pVopBuf) >> 16) & 0xFFFF;
  MP4_PL_VLD_DSP_Frame_Header.VopBuf2Lo = ((unsigned long int)pVopBuf) & 0xFFFF;

  if ( qtv_cfg_DSPDeblockingEnable )
    pDecodeStats->pOutputYUVBuf = (void*) pOutBuf;
  else
    pDecodeStats->pOutputYUVBuf = (void*) pVopBuf;
  
}

/*===========================================================================
FUNCTION:     
  SetFrameHeader

DESCRIPTION:  
  sets the members of the frame header
  
INPUT/OUTPUT PARAMETERS:
  pointer to VOL, pointer to VOP, pointer to buffer array of type MP4BufType,
  current frame index, previous frame index, frame index before the previous one,
  pointer to decoder stats, error concealment type, index of current frame, 
  index of reference VOP

RETURN VALUE: 
  None
===========================================================================*/
void MP4_PAL_VLD_DSP::SetFrameHeader(MP4VOLType *pVOL,VOP_type *pVOP, MP4BufType* pMP4Buf, 
                                         signed short NextInFrameId, signed short prevRefFrameIndex, 
                                         signed short prevPrevRefFrameIndex,DecodeStatsType *pDecodeStats,
					 unsigned short errorConcealmentType, unsigned short OutputVOPIndex,
					 unsigned short sourceVOPIndex)
{  
  MP4_PL_VLD_DSP_Frame_Header.FrameInfo0 = (
                                   ((sourceVOPIndex << 5) & 0x0060) | 
                                   ((errorConcealmentType << 7) & 0x00c0) |
                                   ((pVOP->fRoundingType << 4) & 0x0010) | 
                                   ((qtv_cfg_DSPDeblockingEnable << 3) & 0x0008 ) | 
                                   (OutputVOPIndex & 0x3) );

  MP4_PL_VLD_DSP_Frame_Header.FrameInfo1 = ( ((pVOP->cFcodeForward << 13) & 0xE000) |
                                   ((pVOP->uiQuant << 8) & 0x1F00) |
                                   ((pVOP->cIntraDCVLCThr << 5) & 0x00E0) |
                                   ((pVOL->fRVLC << 4) & 0x0010) |
                                   ((pVOP->cPredType << 2) & 0x000C) |
                                   ((pVOL->fDataPartition << 1) & 0x0002) |
                                   (pVOL->fIsShortHeader & 0x0001) );


  SetFrameHeaderAddresses(pMP4Buf,NextInFrameId,prevRefFrameIndex,prevPrevRefFrameIndex,pDecodeStats);  

}

/*===========================================================================
FUNCTION:     
  AddFrameHeader

DESCRIPTION:  
  This function does the following:
    1. Gets a new slice buffer
    2. Fills up the frame header
    3. Increments the slice buffer pointer just beyond the frame header

INPUT/OUTPUT PARAMETERS:
  pointer to statistics to be filled by DSP, Boolean indicating whether the mode 
  is DP or not

RETURN VALUE: 
  PL_ERROR
===========================================================================*/
PL_ERROR MP4_PAL_VLD_DSP::AddFrameHeader(void *StatsPtr, unsigned char isDP)
{
  unsigned short numBytes = 0;   
 
  /* if there is a valid slice buffer pointer */
  if(pCurrentSliceBuf_VLD_DSP)
  {
    if(isDP)
	{
	 unsigned long int totNumBytesInPart0 = 0;

	 /* for multiple slices in a subframe, need to calculate the slice data size here */
	 pCurrentSliceBuf_VLD_DSP->SliceDataSize = (pSliceWriteForPart1 - (unsigned short *)pCurrentSliceBuf_VLD_DSP->pSliceData) << 1;	 
	 
	 totNumBytesInPart0 = (unsigned long int)((pSliceWrite - (unsigned short *)pCurrentSliceBuf_VLD_DSP->pSliceData) << 1);

	 /* now update the total partition0 size in the slice header */
	 pFirstSliceHeader->NumBytesInSliceDataPart0SizeHigh = (totNumBytesInPart0) >> 16;
	 pFirstSliceHeader->NumBytesInSliceDataPart0SizeLow = (totNumBytesInPart0) & 0xFFFF;		 

	}       
    
    /* if there is already a slice pending with us..queue it. */
    (void)FillSubPacketsToDSPSliceBuffer(TRUE);
  }
  /*  Get a new slice buffer pointer */
  if (GetNewSliceBuf() == PL_ERR_OUT_OF_MEMORY)
     return PL_ERR_OUT_OF_MEMORY;

  /* in the case of data partition mode, set the start address for writing partition1 data */
  if(isDP)
  {    
    pDP1StartAddress = (unsigned short *)pCurrentSliceBuf_VLD_DSP->pSliceData + (offsetSliceDataPart1 >> 1);
    pSliceWriteForPart1 = pDP1StartAddress;
  }
  
  /* Copy the entire frame header into the slice buf */
  numBytes = sizeof( MP4_PL_VLD_DSP_Frame_Header_Type );
  memcpy (pSliceWrite, &MP4_PL_VLD_DSP_Frame_Header, numBytes);
  pSliceWrite += numBytes >> 1;

  QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_HIGH, "MPEG4 New Frame");
  pCurrentSliceBuf_VLD_DSP->fFirstMB = TRUE;
  
  /* in the case of data partition mode, we will update the slice just before sending the slice
   because the total size must equal to the sum of the sizes of the individual partitions and also
   the offset */
  if(!isDP)
  {    
    pCurrentSliceBuf_VLD_DSP->SliceDataSize += numBytes; 
  }

  pCurrentSliceBuf_VLD_DSP->pFrameStatsInPtr = (VDL_Frame_Stats_Pkt_Type *)StatsPtr;  
  pCurrentSliceBuf_VLD_DSP->fFirstMB = TRUE;
  pCurrentSliceBuf_VLD_DSP->fAllocateYUVBuf = TRUE;


  return PL_ERR_NONE;
}

/* ======================================================================
FUNCTION
  EOS

DESCRIPTION
  Sends notification to decoder that the stream has ended. This function 
  is useful for error concealment so that the decoder 
  does not have to wait indefinitely for the last chunk of data.

INPUT/OUTPUT PARAMETERS:
  unsigned char indicating data partition mode or not

RETURN VALUE
  PL_ERROR
 ========================================================================== */
PL_ERROR MP4_PAL_VLD_DSP::EOS(unsigned char isDP)
{
  /* if there is a valid slice buffer pointer,update the slice data size incase of DP mode and
  send the packet to DSP */
  if(pCurrentSliceBuf_VLD_DSP)
  {
   if(isDP)
   {
     unsigned long int totNumBytesInPart0 = 0;

     /* for multiple slices in a subframe, need to calculate the slice data size here */
     pCurrentSliceBuf_VLD_DSP->SliceDataSize = (pSliceWriteForPart1 - (unsigned short *)pCurrentSliceBuf_VLD_DSP->pSliceData) << 1;

     totNumBytesInPart0 = (unsigned long int)((pSliceWrite - (unsigned short *)pCurrentSliceBuf_VLD_DSP->pSliceData) << 1);

     /* now update the total partition0 size in the slice header */
     pFirstSliceHeader->NumBytesInSliceDataPart0SizeHigh = (totNumBytesInPart0) >> 16;
     pFirstSliceHeader->NumBytesInSliceDataPart0SizeLow = (totNumBytesInPart0) & 0xFFFF;	     

   }
    //if there is already a slice pending with us..queue it.
   (void)FillSubPacketsToDSPSliceBuffer(TRUE);
  }

  if (GetNewSliceBuf() == PL_ERR_OUT_OF_MEMORY)
    return PL_ERR_OUT_OF_MEMORY;

  QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_FATAL, "Queueing stats for EOS frame");  
  (void)VDL_Queue_Stats_Buffer( pDLPtr,NULL,VDL_END_OF_SEQUENCE);
  pCurrentSliceBuf_VLD_DSP->fAllocateYUVBuf = TRUE;
  pCurrentSliceBuf_VLD_DSP->NumMacroBlocks++;
  pCurrentSliceBuf_VLD_DSP->fFirstMB = TRUE;

  pCurrentSliceBuf_VLD_DSP->fLastMB = TRUE;
  pCurrentSliceBuf_VLD_DSP->CodecType = VDL_CODEC_MP4_VLD; 
  
  MP4_VLD_EOS_frame_header.PacketId = 0xBD03;
  MP4_VLD_EOS_frame_header.EndMarker = 0x7FFF;

  pEOS = (MP4_PL_VLD_DSP_EOS_FrameHeaderPacketType*) pSliceWrite;
  memcpy (pSliceWrite, &MP4_VLD_EOS_frame_header, sizeof( MP4_VLD_EOS_frame_header ));
  pCurrentSliceBuf_VLD_DSP->SliceDataSize = sizeof( MP4_VLD_EOS_frame_header );
  QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_FATAL, "Queieng EOS buffer to VDL");
  VDL_Queue_Slice_Buffer(pDLPtr, pCurrentSliceBuf_VLD_DSP); 

  pCurrentSliceBuf_VLD_DSP = NULL;
  pSliceWrite = NULL;
  pSliceWriteForPart1 = NULL;

  return PL_ERR_NONE;
}
/*===========================================================================
FUNCTION:     AddSliceHeaderForCombinedMode

DESCRIPTION:  
  Adds a slice header describing the VCL data for the DSP

INPUT/OUTPUT PARAMETERS:
  Number of bytes in slice, first macroblock number in slice, 
  quantization scale, number of bits consumed in slice

RETURN VALUE: 
  PL_ERROR
===========================================================================*/
PL_ERROR MP4_PAL_VLD_DSP::AddSliceHeaderForCombinedMode(unsigned int numBytesInSliceData,
                                         unsigned short MBNum, 
                                         unsigned short quant_scale,                                         
                                         unsigned short numBitsConsumedInSlice)
{
  unsigned short numBytes;
  unsigned long int totalSliceSize;

  /* We know the slice size that we are going to fill..see if we have enough space */
  totalSliceSize = sizeof( MP4_PL_VLD_DSP_Slice_Header_Packet_Type ) + numBytesInSliceData + numBytesInSliceData%2;

  if((pCurrentSliceBuf_VLD_DSP->SliceBufferSize - pCurrentSliceBuf_VLD_DSP->SliceDataSize) < totalSliceSize)
  {
    QTV_MSG_PRIO2(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH,"SliceBufferFull..sending to VDL BytesWritten = %d,CurrentRequest = %d",
                   (pCurrentSliceBuf_VLD_DSP->SliceDataSize),(totalSliceSize) );

    (void)FillSubPacketsToDSPSliceBuffer(FALSE);

   /*  Get a new slice buffer pointer */
    if (GetNewSliceBuf() == PL_ERR_OUT_OF_MEMORY)
      return PL_ERR_OUT_OF_MEMORY;
  }

  /* update ths members of the slice header */
  MP4_PL_VLD_DSP_Slice_Header_Packet_Type *ptr = (MP4_PL_VLD_DSP_Slice_Header_Packet_Type*)pSliceWrite;
  ptr->PacketId = MP4_VLD_DSP_SLICE_HEADER_PACKET_TYPE;
  ptr->SliceInfo0 = MBNum;
  ptr->SliceInfo1 = ((((numBytesInSliceData % 2) << 5) & 0x0020) |
                         (quant_scale & 0x001F));

  numBytesInSliceData += (numBytesInSliceData % 2);

  ptr->NumBytesInSliceDataHigh = (unsigned short) (numBytesInSliceData >> 16);
  ptr->NumBytesInSliceDataLow = (unsigned short) (numBytesInSliceData & 0xFFFF);
  ptr->NumBitsInSliceConsumed = numBitsConsumedInSlice;
  ptr->EndMarker = MP4_VLD_DSP_END_MARKER;  

  numBytes = sizeof( MP4_PL_VLD_DSP_Slice_Header_Packet_Type);
  pSliceWrite += numBytes >> 1;
  pCurrentSliceBuf_VLD_DSP->SliceDataSize += numBytes;
    
  return PL_ERR_NONE;
}

/*===========================================================================
FUNCTION:     
  AddSliceForCombinedMode

DESCRIPTION:  
  Adds a slice for combined mode 

INPUT/OUTPUT PARAMETERS:
  pointer to bitstream buffer, number of bytes in the buffer

RETURN VALUE: 
  PL_ERROR
===========================================================================*/
PL_ERROR MP4_PAL_VLD_DSP::AddSliceForCombinedMode(unsigned char *pBitstream, unsigned long int numBytes)
{
  unsigned long int *p32SliceWrite = (unsigned long int*) pSliceWrite;
  unsigned char  *p8SliceWrite =  (unsigned char*) pSliceWrite;
  unsigned char *src = pBitstream;
  unsigned long int temp32BS = 0;
  unsigned long int numBytesIn = numBytes;

  /* check if there is enough space to write the slice data */
  if((pCurrentSliceBuf_VLD_DSP->SliceBufferSize - pCurrentSliceBuf_VLD_DSP->SliceDataSize) < numBytes)
  {
    QTV_MSG_PRIO2(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_ERROR,"Small Slice Buffer.Available Size = %d,Bytes to write = %d",
                   (pCurrentSliceBuf_VLD_DSP->SliceBufferSize - pCurrentSliceBuf_VLD_DSP->SliceDataSize),(numBytes+ numBytes%2) );

    return PL_ERR_OUT_OF_MEMORY;
  }

  if (((unsigned long int)p8SliceWrite % 4) && (numBytes >= 2))
  {
    *p8SliceWrite++ = *(src+1);
    *p8SliceWrite++ = *src;
    p32SliceWrite = (unsigned long int*)p8SliceWrite;
    src+=2;
    numBytes -= 2;
  }

  /* write the bitstream data 32 bits at a time into the slice buffer */
  for (int i=0; i < (numBytes >> 2); ++i)
  {
    temp32BS = *(src+1);
    temp32BS |= (*src) << 8;    
    temp32BS |= (*(src+2)) << 24;
    temp32BS |= (*(src+3)) << 16;   
    src+=4;

    *p32SliceWrite++ = temp32BS;
  }

  /* align to a 16 bit word boundary by padding 0xFF if the bitstream is not 
    aligned at a 16 bit boundary */
  p8SliceWrite = (unsigned char*) p32SliceWrite;
  if (numBytes & 0x2) 
  {
    *p8SliceWrite++ = *(src+1);
    *p8SliceWrite++ = *src;
    src +=2;
  }
  
  if (numBytes & 0x1)
  {
    *p8SliceWrite++ = 0xFF;
    *p8SliceWrite++ = *src;
  }
  pSliceWrite = (unsigned short*)p8SliceWrite;
  pCurrentSliceBuf_VLD_DSP->SliceDataSize += numBytesIn + (numBytesIn % 2);

  return PL_ERR_NONE;
}

/*===========================================================================
FUNCTION:     
  AddSliceHeaderForDPMode

DESCRIPTION:  
  Adds a slice header describing the VCL data for the DSP

INPUT/OUTPUT PARAMETERS:
  number of bits in partition0 data of the slice, number of bits in partition1 data
  of the slice, first macroblock number in slice, unsigned char indicating whether the
  slice is the first in the frame, first macroblock number of the next slice,
  quantization scale, number of bits consumed in partition0 data of the slice,
  number of bits consumed in partition1 data of the slice

RETURN VALUE: 
  PL_ERROR
===========================================================================*/
PL_ERROR MP4_PAL_VLD_DSP::AddSliceHeaderForDPMode(unsigned int numBitsInSliceData_Part0,
		                         unsigned int numBitsInSliceData_Part1,
                                         unsigned short MBNum,
					 unsigned short firstSliceOfFrame,
					 unsigned short nextMBNum,
                                         unsigned short quant_scale,                                         
                                         unsigned short numBitsConsumedInSlice_Part0,
					 unsigned short numBitsConsumedInSlice_Part1)
{
  unsigned short numBytes,numBytesInSliceDataPart0=0,numBytesInSliceDataPart1=0;  
  unsigned long int totalSliceSize_Part0;
  unsigned long int totalSliceSize_Part1;
  unsigned long int totalBufferSize_Part0;
  unsigned long int totalBufferSize_Part1;
  unsigned long int totalSliceSizeWritten_Part0;
  unsigned long int totalSliceSizeWritten_Part1;

  /* calculate the number of bytes in both the partitions */
  numBytesInSliceDataPart0 = (numBitsInSliceData_Part0 >> 3) + (numBitsInSliceData_Part0%8?1:0) + (numBitsInSliceData_Part0%16?1:0);
  numBytesInSliceDataPart1 = (numBitsInSliceData_Part1 >> 3)  + (numBitsInSliceData_Part1%8?1:0) + (numBitsInSliceData_Part1%16?1:0);
  
  /* We know the slice size that we are going to fill..see if we have enough space */
  totalSliceSize_Part0 = sizeof( MP4_PL_VLD_DSP_Slice_Header_Packet_Type_DP ) + numBytesInSliceDataPart0 ;

  /* for the first slice of frame, add the frame header size as well */
  if(firstSliceOfFrame)
  {
    totalSliceSize_Part0 += sizeof(MP4_PL_VLD_DSP_Frame_Header_Type);
  }

  /* compute the total buffer size and the size of the data written into the individual partitions */
  totalBufferSize_Part0 = (pDP1StartAddress - (unsigned short *)pCurrentSliceBuf_VLD_DSP->pSliceData)<<1;
  totalSliceSizeWritten_Part0 = (pSliceWrite - (unsigned short *)pCurrentSliceBuf_VLD_DSP->pSliceData)<<1;  
  totalSliceSizeWritten_Part1 = (pSliceWriteForPart1 - pDP1StartAddress)<<1;

  totalBufferSize_Part1 = (((unsigned short *)pCurrentSliceBuf_VLD_DSP->pSliceData + 
	                      (pCurrentSliceBuf_VLD_DSP->SliceBufferSize>>1)
                          ) - pDP1StartAddress)<<1;  

  totalSliceSize_Part1 = numBytesInSliceDataPart1;
  
  /* check if there is space to write the current slice */
  if(((totalBufferSize_Part0 - totalSliceSizeWritten_Part0) < totalSliceSize_Part0) ||
	  ((totalBufferSize_Part1 - totalSliceSizeWritten_Part1) < totalSliceSize_Part1))
  {
    unsigned long int totNumBytesInPart0=0;

    if(firstSliceOfFrame)
    {
      QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH,"SliceBufferFull...even before writing the first slice!!");
      return PL_ERR_OUT_OF_MEMORY;
    }

    QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH,"SliceBufferFull...");
                  
    /* for multiple slices in a subframe, need to calculate the slice data size here*/
    pCurrentSliceBuf_VLD_DSP->SliceDataSize = (pSliceWriteForPart1 - (unsigned short *)pCurrentSliceBuf_VLD_DSP->pSliceData) << 1;
	

    totNumBytesInPart0 = (unsigned long int)((pSliceWrite - (unsigned short *)pCurrentSliceBuf_VLD_DSP->pSliceData) << 1);

    /* now update the total partition0 size in the slice header */
    pFirstSliceHeader->NumBytesInSliceDataPart0SizeHigh = (totNumBytesInPart0) >> 16;
    pFirstSliceHeader->NumBytesInSliceDataPart0SizeLow = (totNumBytesInPart0) & 0xFFFF;	
  
    (void)FillSubPacketsToDSPSliceBuffer(FALSE);

    /* Get a new slice buffer pointer */
    if (GetNewSliceBuf() == PL_ERR_OUT_OF_MEMORY)
      return PL_ERR_OUT_OF_MEMORY;
	
    pDP1StartAddress = (unsigned short *)pCurrentSliceBuf_VLD_DSP->pSliceData + (offsetSliceDataPart1 >> 1);
    pSliceWriteForPart1 = pDP1StartAddress;

    pFirstSliceHeader = (MP4_PL_VLD_DSP_Slice_Header_Packet_Type_DP*)pSliceWrite;
	
  }
  
  MP4_PL_VLD_DSP_Slice_Header_Packet_Type_DP *ptr;

  pSliceHeader = (MP4_PL_VLD_DSP_Slice_Header_Packet_Type_DP*)pSliceWrite;

  /* set the pointer to start writing the slice header */
  ptr = pSliceHeader;

  /* for the first slice of the frame, maintain a pointer to the slice header to
   update the total partition0 size just before sending the packet */
  if(firstSliceOfFrame)
  {
    totalSliceSize_Part0 += sizeof(MP4_PL_VLD_DSP_Frame_Header_Type);
    pFirstSliceHeader = pSliceHeader;
  }
  ptr->PacketId = MP4_VLD_DSP_SLICE_HEADER_PACKET_TYPE;
  ptr->SliceInfo0 = MBNum;
  ptr->NextMBNum = nextMBNum;
  ptr->SliceInfo1 = (quant_scale & 0x001F);

  ptr->NumBitsInSliceDataPart0Consumed = numBitsConsumedInSlice_Part0;
  
  ptr->EndMarker = MP4_VLD_DSP_END_MARKER;  

  numBytes = sizeof( MP4_PL_VLD_DSP_Slice_Header_Packet_Type_DP);
  pSliceWrite += numBytes >> 1;  
  
  return PL_ERR_NONE;
}

/*===========================================================================
FUNCTION:     
  AddSliceForDPMode

DESCRIPTION:  
  Adds a slice for DP mode

INPUT/OUTPUT PARAMETERS:
  pointer to partition0 data, number of bits in partition0, number of bits in
  partition1, number of bits of marker to flush, pointer to bitstream, starting
  macroblock number of the slice

RETURN VALUE: 
  PL_ERROR
===========================================================================*/
PL_ERROR MP4_PAL_VLD_DSP::AddSliceForDPMode(unsigned char *pDP0, unsigned long int numBitsPart0,							
					    unsigned long int numBitsPart1,unsigned long int numBitsOfMarkerToFlush,
					    unsigned char **pBS, unsigned long int iStartMB)
{
  unsigned long int numBytesPart0 = (numBitsPart0 >> 3);  
  unsigned char stuffingByte=0,numStuffingBits;  
  int startPos = -1;  
  unsigned char *p8BS;
  unsigned short *p16BS = (unsigned short*) pDP0;
  unsigned char *p8SliceWrite = (unsigned char*) pSliceWrite;
  unsigned char *p8Part0 = p8SliceWrite;    
  unsigned long int numBytesWritten,numBytesInSliceDataPart0,numBytesInSliceDataPart1;
  unsigned long int sliceDataPart0Padded = 0,sliceDataPart1Padded = 0;

#if 0
  /* write the bits of partition0 data */  
  for (int i = 0; i < ( numBitsPart0 >> 4); ++i)
  {
    *p8SliceWrite++ = (*p16BS >> 8);
    *p8SliceWrite++ = (*p16BS++ & 0xFF);
  }

  /* compute number of bytes written */
  numBytesWritten = (p8SliceWrite - p8Part0);

#else
  unsigned long int *p32SliceWrite = (unsigned long int*) pSliceWrite;
  unsigned long int temp32BS = 0;
  unsigned long int updatedNumBitsPart0 = numBitsPart0;
  unsigned long int updatedTotalBitsPart1;

  p8BS = pDP0;
  /* write the bits of partition0 data */ 
  if (((unsigned long int)p8SliceWrite % 4) && (updatedNumBitsPart0 >= 16))
  {
    *p8SliceWrite++ = *(p8BS+1);
    *p8SliceWrite++ = *p8BS;
    p32SliceWrite = (unsigned long int*)p8SliceWrite;
    p8BS+=2;
    updatedNumBitsPart0 -= 16;
  }

  /* write the bitstream data 32 bits at a time into the slice buffer */
  for (int i=0; i < ( updatedNumBitsPart0 >> 5); ++i)
  {
    temp32BS = *(p8BS+1);
    temp32BS |= (*p8BS) << 8;    
    temp32BS |= (*(p8BS+2)) << 24;
    temp32BS |= (*(p8BS+3)) << 16;   
    p8BS+=4;

    *p32SliceWrite++ = temp32BS;
  }

  /* align to a 16 bit word boundary */
  p8SliceWrite = (unsigned char*) p32SliceWrite;
  if ((updatedNumBitsPart0%32)/16) 
  {
    *p8SliceWrite++ = *(p8BS+1);
    *p8SliceWrite++ = *p8BS;
    p8BS +=2;
  }

  p16BS = (unsigned short*) p8BS;
  /* compute number of bytes written */
  numBytesWritten = (p8SliceWrite - p8Part0);
#endif

  /* copy the bits till the start of the DC marker and 
   then pad stuffing bits to make it byte aligned */
    
  numStuffingBits = 8 - (numBitsPart0%8);

  if(numBitsPart0%8)
  {
    /* since starts from 0, the next bit where the stuffing bit should start */
    startPos = numStuffingBits - 1; 

    /* if the marker does not start at a byte aligned boundary, then create the stuffing byte */    
    if(startPos == 0)
    {
      stuffingByte=0;
    }
    else
    {
      /* stuffing bits is 0 followed by 1s */
      for(int j=startPos - 1;j>=0;j--)
        stuffingByte |= ((1<<j) & 0xFF); //to add the 1 s following the 0 for the stuffing bits
    }
  }

  /* the following condition below implies we have one more byte to be written before getting to the byte having the 
    dc or motion marker, so we add the next byte and then replace the byte having the marker with the stuffing byte */
  if(numBytesPart0 > numBytesWritten)
  {
    p8BS = (unsigned char *)p16BS;	  
    p8BS++;

    /* if the marker does not start at a byte aligned boundary, then extract the byte and replace with the 
      stuffing bits */
    if(startPos >= 0)
    {
      *p8SliceWrite++ = (((*p8BS >> (startPos + 1)) << (startPos + 1))& 0xFF) | stuffingByte;		  
      *p8SliceWrite++ = (*p16BS & 0xFF);
    }
    else
    {
      /* if the marker starts at a byte aligned boundary, then add 0x7F as the stuffing byte */
      *p8SliceWrite++ = 0x7F;
      *p8SliceWrite++ = (*p16BS & 0xFF);
    }	  
  }
  else
  {
    /* if we are here, it implies that the data before the dc or motion marker ended at a byte aligned boundary, so in this
    case, replace the marker with the stuffing byte and pad it with 0xFF to make it 16 bit word aligned */

    p8BS = (unsigned char *)p16BS;
    if(startPos >= 0)
    {
      *(p8SliceWrite+1) = (((*p8BS >> (startPos + 1)) << (startPos + 1))& 0xFF) | stuffingByte;	
    }
    else
    {
      *(p8SliceWrite+1) = 0x7F;
    }
    *p8SliceWrite++ = 0xFF;
    p8SliceWrite++;

    /* this flag indicates whether the slice data was padded or not to make it word aligned */
    sliceDataPart0Padded = 1;
  }    
  p8BS++;

  numBytesInSliceDataPart0 = (p8SliceWrite - p8Part0);
  pSliceHeader->NumBytesInSliceDataPart0High = (unsigned short) (numBytesInSliceDataPart0 >> 16);
  pSliceHeader->NumBytesInSliceDataPart0Low = (unsigned short) (numBytesInSliceDataPart0 & 0xFFFF);  
        
  /* since p8BS is already at a byte boundary due to addition of stuffing bits,
    just flush the remaining bytes of the marker and then continue */  

  p8BS += ((numBitsOfMarkerToFlush - numStuffingBits) >> 3);

  unsigned char numRemainingMarkerBits = (numBitsOfMarkerToFlush  - numStuffingBits) % 8;
  unsigned long int totalBitsPart1 = numBitsPart1 + numRemainingMarkerBits;
    
  /******************* now write data partition1 info into slice buffer *************/  
  pSliceWrite = (unsigned short*)p8SliceWrite;
  p8SliceWrite = (unsigned char *)pSliceWriteForPart1;  
    
#if 0
  p16BS = (unsigned short*) p8BS;
  for (int i = 0; i < (totalBitsPart1 >> 4); ++i)
  {
    *p8SliceWrite++ = (*p16BS >> 8);
    *p8SliceWrite++ = (*p16BS++ & 0xFF);
  }  
  
  p8BS = (unsigned char *)p16BS;

  /* make the partition1 data 16 bit word aligned */
  if(totalBitsPart1 % 16)
  {  
    *p8SliceWrite++ = 0xFF;
    *p8SliceWrite++ = (*p8BS++ & 0xFF);
    sliceDataPart1Padded = 1;
  }  
#else
  p32SliceWrite = (unsigned long int*) p8SliceWrite;
  updatedTotalBitsPart1 = totalBitsPart1;

  /* write the bits of partition1 data */ 
  if (((unsigned long int)p8SliceWrite % 4) && (updatedTotalBitsPart1 >= 16))
  {
    *p8SliceWrite++ = *(p8BS+1);
    *p8SliceWrite++ = *p8BS;
    p32SliceWrite = (unsigned long int*)p8SliceWrite;
    p8BS+=2;
    updatedTotalBitsPart1 -= 16;
  }

  /* write the bitstream data 32 bits at a time into the slice buffer */
  for (int i=0; i < ( updatedTotalBitsPart1 >> 5); ++i)
  {
    temp32BS = *(p8BS+1);
    temp32BS |= (*p8BS) << 8;    
    temp32BS |= (*(p8BS+2)) << 24;
    temp32BS |= (*(p8BS+3)) << 16;   
    p8BS+=4;

    *p32SliceWrite++ = temp32BS;
  }

  /* align to a 16 bit word boundary */
  p8SliceWrite = (unsigned char*) p32SliceWrite;
  if ((updatedTotalBitsPart1%32)/16) 
  {
    *p8SliceWrite++ = *(p8BS+1);
    *p8SliceWrite++ = *p8BS;
    p8BS +=2;
  }

  /* make the partition1 data 16 bit word aligned */
  if(totalBitsPart1 % 16)
  {  
    *p8SliceWrite++ = 0xFF;
    *p8SliceWrite++ = *p8BS++;
    sliceDataPart1Padded = 1;
  }
#endif

  /*************for multiple slices in a subframe,start writing partition1 data from an offset *********/
  *pBS = p8BS;
  pSliceWriteForPart1 = (unsigned short *)p8SliceWrite;
  
  pSliceHeader->SliceInfo1 |= (((sliceDataPart1Padded << 6) & 0x0040) |
	                          ((sliceDataPart0Padded << 5) & 0x0020));

  numBytesInSliceDataPart1 = (totalBitsPart1 >> 3) + ((totalBitsPart1%16)?1:0);
  pSliceHeader->NumBytesInSliceDataPart1High = (unsigned short) (numBytesInSliceDataPart1 >> 16);
  pSliceHeader->NumBytesInSliceDataPart1Low = (unsigned short) (numBytesInSliceDataPart1 & 0xFFFF);
  pSliceHeader->NumBitsInSliceDataPart1Consumed = numRemainingMarkerBits;

  pSliceHeader->NumBytesInSliceDataPart1OffsetHigh = (unsigned short)(offsetSliceDataPart1 >> 16);
  pSliceHeader->NumBytesInSliceDataPart1OffsetLow = (unsigned short)(offsetSliceDataPart1 & 0xFFFF);

  /* no need to set it here, can set it just before sending the subframe */
  pSliceHeader->NumBytesInSliceDataPart0SizeHigh = (unsigned short)(offsetSliceDataPart1 >> 16);
  pSliceHeader->NumBytesInSliceDataPart0SizeLow = (unsigned short)(offsetSliceDataPart1 & 0xFFFF);
    
  return PL_ERR_NONE;
}


/*===========================================================================
FUNCTION:
  FillSubPacketsToDSPSliceBuffer

DESCRIPTION:
  This function queues the slice buffer to DSP. Packet ready for DSP 
  consumption is generated and is ready to be queued

INPUT/OUTPUT PARAMETERS:
  unsigned char indicating whether slice has last MB

RETURN VALUE:
  PL_ERROR
==============================================================================*/
PL_ERROR MP4_PAL_VLD_DSP::FillSubPacketsToDSPSliceBuffer(unsigned char fLastMB)
{  
  pCurrentSliceBuf_VLD_DSP->NumMacroBlocks++;

  pCurrentSliceBuf_VLD_DSP->fLastMB = fLastMB;
  pCurrentSliceBuf_VLD_DSP->CodecType = VDL_CODEC_MP4_VLD; 

  (void)VDL_Queue_Slice_Buffer(pDLPtr,pCurrentSliceBuf_VLD_DSP);
  pCurrentSliceBuf_VLD_DSP = NULL;
  pSliceWrite = NULL;

  return PL_ERR_NONE;
}


/*===========================================================================
FUNCTION:
  GetNewSliceBufferDataPtr

DESCRIPTION:
  Gets the pointer to a new slice data buffer

INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE:
  16 bit Pointer to slice data
==============================================================================*/
unsigned short* MP4_PAL_VLD_DSP::GetNewSliceBufferDataPtr(void)
{
  pCurrentSliceBuf_VLD_DSP = VDL_Get_Slice_Buffer(pDLPtr);

  if (pCurrentSliceBuf_VLD_DSP == NULL)
  {
    QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_FATAL, "Out of memory");
    return NULL;
  }
  pCurrentSliceBuf_VLD_DSP->SliceDataSize = 0;
  
  pCurrentSliceBuf_VLD_DSP->fFirstMB = FALSE;
  
/* It slow down ARM decode time. It's not a cache memory. Need more investigation. 
  memset(pCurrentSliceBuf_VLD_DSP->pSliceData,0,pCurrentSliceBuf_VLD_DSP->SliceBufferSize);
*/
  return(unsigned short*) pCurrentSliceBuf_VLD_DSP->pSliceData;
}

/*===========================================================================
FUNCTION:
  FreeSliceData

DESCRIPTION:
  Frees the current slice buffer in use
  
INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE:
  None
==============================================================================*/
void MP4_PAL_VLD_DSP::FreeSliceData(void)
{
  /* reset all variables and pointers here */
  if (pCurrentSliceBuf_VLD_DSP != NULL)
  {
    VDL_Free_Slice_Buffer(pDLPtr,pCurrentSliceBuf_VLD_DSP);
    pCurrentSliceBuf_VLD_DSP = NULL;
    pSliceWrite = NULL;
    pCurrentSliceBuf_VLD_DSP = NULL;
    pDP1StartAddress = NULL;
    pSliceWriteForPart1 = NULL;
    pSliceHeader = NULL;
    pFirstSliceHeader = NULL;
  }
  return;
}

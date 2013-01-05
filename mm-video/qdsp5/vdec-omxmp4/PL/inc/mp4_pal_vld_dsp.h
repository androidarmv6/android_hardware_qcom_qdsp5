#ifndef MP4_PAL_VLD_DSP_H
#define MP4_PAL_VLD_DSP_H
/*=============================================================================
        mp4_pal_vld_dsp.h

DESCRIPTION:     
   This file provides the interface specification for the MPEG4 PAL supporting
   VLD in DSP interface.
    
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

$Header: //source/qcom/qct/multimedia2/Video/Dec/MPEG4Vdec/main/latest/src/PL/mp4_pal_vld_dsp.h#1 $
$DateTime: 2009/03/03 14:14:44 $
$Change: 853763 $

========================================================================== */

/* =======================================================================
**               Includes and Public Data Declarations
** ======================================================================= */
#include "MP4_PAL.h"

/*PAL error types */
typedef enum
{
  PL_ERR_NONE = 0,   
  PL_ERR_INIT_FAILURE,      
  PL_ERR_INVALID_STATE,      
  PL_ERR_OUT_OF_MEMORY,
  PL_ERR_VDL_UNINTIALIZED,
  PL_ERR_VDL_LAYER,
  PL_ERR_MYSTERY
}PL_ERROR;


class MP4_PAL_VLD_DSP
{
public:
  MP4_PAL_VLD_DSP(void *pDL);
  ~MP4_PAL_VLD_DSP();
  void InitFrameHeader(MP4BufType* pMP4Buf,unsigned short Width,unsigned short Height);

  PL_ERROR AddFrameHeader(void *outAddr, unsigned char isDP);
  void SetFrameHeader(MP4VOLType *pVOL,VOP_type *pVOP, MP4BufType* pMP4Buf, 
                      signed short NextInFrameId, signed short prevRefFrameIndex, 
                      signed short prevPrevRefFrameIndex,DecodeStatsType *pDecodeStats,
		      unsigned short errorConcealmentType, unsigned short OutputVOPIndex,
		      unsigned short sourceVOPIndex);

  void SetFrameHeaderAddresses(MP4BufType* pMP4Buf, signed short NextInFrameId, 
                               signed short prevRefFrameIndex, signed short prevPrevRefFrameIndex,
                               DecodeStatsType *pDecodeStats);

  PL_ERROR AddSliceHeaderForCombinedMode(unsigned int NumBytesInSliceData,
                                         unsigned short MBNum, 
                                         unsigned short quant_scale,                                         
                                         unsigned short NumBitsConsumedInSlice);
  PL_ERROR AddSliceForCombinedMode(unsigned char *pBitstream, unsigned long int numBytes);

  PL_ERROR AddSliceHeaderForDPMode(unsigned int NumBitsInSliceData_Part0,
				   unsigned int NumBitsInSliceData_Part1,
                                   unsigned short MBNum, 
				   unsigned short firstSliceOfFrame,
				   unsigned short NextMBNum,
                                   unsigned short quant_scale,                                         
                                   unsigned short NumBitsConsumedInSlice_Part0,
				   unsigned short NumBitsConsumedInSlice_Part1);

  PL_ERROR AddSliceForDPMode(unsigned char *pDP0, unsigned long int numBits_Part0,							
			     unsigned long int numBits_Part1,unsigned long int numBitsOfMarkerToFlush,
			     unsigned char **pBS, unsigned long int iStartMB);

  PL_ERROR FillSubPacketsToDSPSliceBuffer(unsigned char fLastMB);

  PL_ERROR GetNewSliceBuf();
  unsigned short* GetNewSliceBufferDataPtr(void);
  void FreeSliceData(void);
  PL_ERROR EOS(unsigned char isDP);
  void *pDLPtr; 

private:

  /* slice header packet type for combined mode */
  typedef struct
  {
    unsigned short PacketId;
    unsigned short SliceInfo0;
    unsigned short SliceInfo1;
    unsigned short NumBytesInSliceDataHigh;
    unsigned short NumBytesInSliceDataLow;
    unsigned short NumBitsInSliceConsumed;
    unsigned short EndMarker;
  }MP4_PL_VLD_DSP_Slice_Header_Packet_Type;

  MP4_PL_VLD_DSP_Slice_Header_Packet_Type MP4_PL_VLD_DSP_Slice_Header;
  
  /* slice header packet type for data partition mode */
  typedef struct
  {
    unsigned short PacketId;
    unsigned short SliceInfo0;
    unsigned short NextMBNum;
    unsigned short SliceInfo1;
    unsigned short NumBytesInSliceDataPart0High;
    unsigned short NumBytesInSliceDataPart0Low;
    unsigned short NumBitsInSliceDataPart0Consumed;
    unsigned short NumBytesInSliceDataPart1High;
    unsigned short NumBytesInSliceDataPart1Low;
    unsigned short NumBitsInSliceDataPart1Consumed;
    unsigned short NumBytesInSliceDataPart1OffsetHigh;
    unsigned short NumBytesInSliceDataPart1OffsetLow;	
    unsigned short NumBytesInSliceDataPart0SizeHigh;
    unsigned short NumBytesInSliceDataPart0SizeLow;
    unsigned short EndMarker;
  }MP4_PL_VLD_DSP_Slice_Header_Packet_Type_DP;

  MP4_PL_VLD_DSP_Slice_Header_Packet_Type_DP MP4_PL_VLD_DSP_Slice_Header_DP;
  
  /*  EOS packet structure */
  typedef struct
  {
    unsigned short PacketId;            /* Should be MP4_FRAME_HEADER_PACKET_TYPE */
    unsigned short EndMarker;           /* Should be MP4_MB_END_MARKER            */
  } MP4_PL_VLD_DSP_EOS_FrameHeaderPacketType;
  MP4_PL_VLD_DSP_EOS_FrameHeaderPacketType MP4_VLD_EOS_frame_header;
  
  MP4_PL_VLD_DSP_EOS_FrameHeaderPacketType *pEOS;

  /* Frame header packet type */
  typedef struct
  {
    unsigned short PacketId;            /* Should be MP4_FRAME_HEADER_PACKET_TYPE */
    unsigned short Width;               /* Width of frame in pixels               */
    unsigned short Height;              /* Height of frame in pixels              */
    unsigned short LineWidth;           /* Used for interlaced support            */
    unsigned short FrameInfo0;           /* FRAME_INFO bits                        */
    unsigned short FrameInfo1;
    unsigned short VopBuf0Hi;           /* High 16 bits of VOP buffer 0, byte-addr*/
    unsigned short VopBuf0Lo;           /* Low 16 bits...                         */
    unsigned short VopBuf1Hi;           
    unsigned short VopBuf1Lo;
    unsigned short VopBuf2Hi;
    unsigned short VopBuf2Lo;
    unsigned short OutBufHi;            /* High 16 bits of output buffer          */
    unsigned short OutBufLo;            /* Low 16 bits of output buffer           */  
    unsigned short EndMarker;           /* Should be MP4_VLD_DSP_END_MARKER            */
  }MP4_PL_VLD_DSP_Frame_Header_Type;

  MP4_PL_VLD_DSP_Frame_Header_Type MP4_PL_VLD_DSP_Frame_Header;

  MP4_PL_VLD_DSP_Slice_Header_Packet_Type_DP *pSliceHeader, *pFirstSliceHeader;

  unsigned short *pSliceWrite;  /* current address for writing slice data */
  unsigned long int offsetSliceDataPart1; /* offset for writing partition1 data */
  unsigned short *pDP1StartAddress;   /* starting address for writing partition1 data */
  unsigned short *pSliceWriteForPart1; /* current address for writing partition1 data */

  VDL_Slice_Pkt_Type* pCurrentSliceBuf_VLD_DSP; /* current slice buffer pointer */
};
#endif 

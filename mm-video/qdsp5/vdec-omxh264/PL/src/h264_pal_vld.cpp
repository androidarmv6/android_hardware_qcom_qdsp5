/*=============================================================================
        H264_PAL_VLD.cpp
DESCRIPTION:
   This file provides necessary interface for the MPEG 4 QDSP.

  ABSTRACT:

Copyright (c) 2007 by QUALCOMM, Incorporated.  All Rights Reserved. 
QUALCOMM Proprietary and Confidential.

============================================================================*/
/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/H264Vdec/main/latest/src/PL/h264_pal_vld.cpp#1 $
$DateTime: 2008/08/05 12:05:56 $
$Change: 717059 $

========================================================================== */

/* =======================================================================
**               Includes and Public Data Declarations
** ======================================================================= */
#include "h264_pal_vld.h"
#include "h264_types.h"
#include "qtv_msg.h"
#ifndef FEATURE_H264_CONSOLE_APP
  #ifndef _CFP
extern "C" {
//#include "rex.h"
}
  #endif
#endif

extern "C" {
#include "VDL_API.h"
#include "assert.h"
}

#ifndef T_MSM7500
  #define H264_FRUC_MB_HEADER_PACKET_TYPE   (0x8203)
#else
  #define H264_FRUC_MB_HEADER_PACKET_TYPE   (0xA202)
  #define H264_FRUC_MAX_MBS_IN_FRUC_SUB_FRAME     300
  #define H264_FRUC_FRAME_HEADER_PACKET_TYPE (0xA201)
#endif

#define H264_MB_HEADER_PACKET_TYPE    (0x8202)
#define H264_FRAME_HEADER_PACKET_TYPE (0x8201)
#define H264_VLD_FRAME_HEADER_PACKET_TYPE (0xB201)
#define H264_VLD_SLICE_HEADER_PACKET_TYPE (0xB202)
#define H264_MB_END_MARKER            (0x7FFF)

#ifndef T_MSM7500
  #define MAX_H264_GUARD_SIZE    1800 // Based on Max 854 words
// Spec dated Oct 25, 2005
#else
  #define MAX_H264_GUARD_SIZE    1000 // ANR-TBD Recompute 
#endif

// IPCM helper members
#define H264_IPCM_NUM_LUMA   128
#define H264_IPCM_NUM_CHROMA  32
unsigned char qtv_dbg_wait_for_SB_Full = 1;
/*===========================================================================
FUNCTION:
  H264_PAL_VLD constructor

DESCRIPTION:
  Default constructor
  

INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE:
  None

SIDE EFFECTS: 
==============================================================================*/
H264_PAL_VLD::H264_PAL_VLD(void *pDL):H264_PAL(pDL)
{
  //VDL instance pointer
  pDLPtr = pDL;
  slice_write_ptr_VLD = NULL;
  pCurrentSliceBuf_VLD = NULL;
  QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH, "H264_PAL_VLD created");
}
/*===========================================================================
FUNCTION:
  Destructor

DESCRIPTION:
  Destroys PAL instance.
  

INPUT/OUTPUT PARAMETERS:
 None

RETURN VALUE:
  PL_ERR_NONE when succesfull

SIDE EFFECTS: 
==============================================================================*/
H264_PAL_VLD::~H264_PAL_VLD()
{
  pDLPtr = NULL;
  QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_DEBUG, "H264_PAL_VLD destructed");
}
/*===========================================================================
FUNCTION:     
  FillFrameHeaderConstants

DESCRIPTION:  
  Fills the default frame header details
  
INPUT/OUTPUT PARAMETERS:
  FrameBuffAddr - buffer buffer array address
  num_frame_buffers - number of frame buffers
  
RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
void H264_PAL_VLD::FillFrameHeaderConstants(unsigned char** FrameBuffAddr,unsigned int num_frame_buffers, H264PL_FrameHeaderPacketType* p_h264_frame_header)
{
  h264_VLD_frame_header.PacketId  = H264_VLD_FRAME_HEADER_PACKET_TYPE;
  if (FrameBuffAddr)
  {
    memset(&(h264_frame_header.FrameBuf0High), 64, 0);
    switch (num_frame_buffers)
    {
      case 18:
      case 17:
      case 16:
        {
          h264_VLD_frame_header.FrameBuf15Low  = p_h264_frame_header->FrameBuf15Low;
          h264_VLD_frame_header.FrameBuf15High = p_h264_frame_header->FrameBuf15High;
        }
        /*lint -fallthrough*/
      case 15:
        {
          h264_VLD_frame_header.FrameBuf14Low  = p_h264_frame_header->FrameBuf14Low;
          h264_VLD_frame_header.FrameBuf14High = p_h264_frame_header->FrameBuf14High;
        }
        /*lint -fallthrough*/
      case 14:
        {
          h264_VLD_frame_header.FrameBuf13Low  = p_h264_frame_header->FrameBuf13Low;
          h264_VLD_frame_header.FrameBuf13High = p_h264_frame_header->FrameBuf13High;
        }
        /*lint -fallthrough*/
      case 13:
        {
          h264_VLD_frame_header.FrameBuf12Low  = p_h264_frame_header->FrameBuf12Low;
          h264_VLD_frame_header.FrameBuf12High = p_h264_frame_header->FrameBuf12High;
        }
        /*lint -fallthrough*/
      case 12:
        {
          h264_VLD_frame_header.FrameBuf11Low  = p_h264_frame_header->FrameBuf11Low;
          h264_VLD_frame_header.FrameBuf11High = p_h264_frame_header->FrameBuf11High;
        }
        /*lint -fallthrough*/
      case 11:
        {
          h264_VLD_frame_header.FrameBuf10Low  = p_h264_frame_header->FrameBuf10Low;
          h264_VLD_frame_header.FrameBuf10High = p_h264_frame_header->FrameBuf10High;
        }
        /*lint -fallthrough*/
      case 10:
        {
          h264_VLD_frame_header.FrameBuf9Low  = p_h264_frame_header->FrameBuf9Low;
          h264_VLD_frame_header.FrameBuf9High = p_h264_frame_header->FrameBuf9High;
        }
        /*lint -fallthrough*/
      case 9:
        {
          h264_VLD_frame_header.FrameBuf8Low  = p_h264_frame_header->FrameBuf8Low;
          h264_VLD_frame_header.FrameBuf8High = p_h264_frame_header->FrameBuf8High;
        }
        /*lint -fallthrough*/
      case 8:
        {
          h264_VLD_frame_header.FrameBuf7Low  = p_h264_frame_header->FrameBuf7Low;
          h264_VLD_frame_header.FrameBuf7High = p_h264_frame_header->FrameBuf7High;
        }
        /*lint -fallthrough*/
      case 7:
        {
          h264_VLD_frame_header.FrameBuf6Low  = p_h264_frame_header->FrameBuf6Low;
          h264_VLD_frame_header.FrameBuf6High = p_h264_frame_header->FrameBuf6High;
        }
        /*lint -fallthrough*/
      case 6:
        {
          h264_VLD_frame_header.FrameBuf5Low  = p_h264_frame_header->FrameBuf5Low;
          h264_VLD_frame_header.FrameBuf5High = p_h264_frame_header->FrameBuf5High;
        }
        /*lint -fallthrough*/
      case 5:
        {
          h264_VLD_frame_header.FrameBuf4Low  = p_h264_frame_header->FrameBuf4Low;
          h264_VLD_frame_header.FrameBuf4High = p_h264_frame_header->FrameBuf4High;
        }
        /*lint -fallthrough*/
      case 4:
        {
          h264_VLD_frame_header.FrameBuf3Low  = p_h264_frame_header->FrameBuf3Low;
          h264_VLD_frame_header.FrameBuf3High = p_h264_frame_header->FrameBuf3High;
        }
        /*lint -fallthrough*/
      case 3:
        {
          h264_VLD_frame_header.FrameBuf2Low  = p_h264_frame_header->FrameBuf2Low;
          h264_VLD_frame_header.FrameBuf2High = p_h264_frame_header->FrameBuf2High;
        }
        /*lint -fallthrough*/
      case 2:
        {
          h264_VLD_frame_header.FrameBuf1Low  = p_h264_frame_header->FrameBuf1Low;
          h264_VLD_frame_header.FrameBuf1High = p_h264_frame_header->FrameBuf1High;
        }
        /*lint -fallthrough*/
      case 1:
        {
          h264_VLD_frame_header.FrameBuf0Low  = p_h264_frame_header->FrameBuf0Low;
          h264_VLD_frame_header.FrameBuf0High = p_h264_frame_header->FrameBuf0High;
          break;
        }
        /*lint -fallthrough*/

      default:
        {
          /* Although this code mallocs a buffer if we dont provide one, we will never
          ** use it that way so if someone does, assert 
          */
          ASSERT(0);
        }
    }

  }

  h264_VLD_frame_header.EndMarker = H264_MB_END_MARKER;
}
/*===========================================================================
FUNCTION:     
  UpdateFrameHeader

DESCRIPTION:  
  updates the frame header
  
INPUT/OUTPUT PARAMETERS:
  X_Dimension - x dimesion of frame
  Y_Dimension - y dimesion of frame
  FrameInfo_0 
  FrameInfo_1
  Output_frame_buffer - output frame buffer address

RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
void H264_PAL_VLD::UpdateFrameHeader(unsigned short X_Dimension,unsigned short Y_Dimension,
                                     unsigned char ErrConcealType, unsigned char ChromaQPIndexOffset,
                                     unsigned char intraPredFlag, unsigned char num_ref_frames,
                                     unsigned char* Output_frame_buffer, unsigned char** ref_frames_arr)
{
  pFrameHdr->X_Dimension = X_Dimension;
  pFrameHdr->Y_Dimension = Y_Dimension;
  pFrameHdr->FrameInfo_0 = ( ((ErrConcealType << 11) & 0x1800) |
                             (((ChromaQPIndexOffset + 12) << 6) & 0x07C0)|
                             ((intraPredFlag << 5) & 0x0020)|
                             (num_ref_frames & 0x001F));
  pFrameHdr->PacketId = H264_VLD_FRAME_HEADER_PACKET_TYPE;
#ifdef T_MSM7500
  pFrameHdr->LineWidth     = pFrameHdr->X_Dimension;
#endif
#ifdef FEATURE_VDL_TEST_PC
  //Fill the output frame buffer address information
  pFrameHdr->OutputFrameBufHigh = (unsigned long int)Output_frame_buffer >> 16;
  pFrameHdr->OutputFrameBufLow  = (unsigned long int)Output_frame_buffer & 0xFFFF;
#else
  unsigned int pAddr = (unsigned long int)Output_frame_buffer;

  pFrameHdr->OutputFrameBufHigh = (unsigned long int)pAddr >> 16;
  pFrameHdr->OutputFrameBufLow  = (unsigned long int)pAddr & 0xFFFF;
#endif //FEATURE_VDL_TEST_PC
  pCurrentSliceBuf_VLD->SliceDataSize += sizeof( h264_VLD_frame_header );

  if(ref_frames_arr) {
	pFrameHdr->FrameBuf15Low  = (unsigned short)(((unsigned long int)ref_frames_arr[15]) & 0xFFFF);
        pFrameHdr->FrameBuf15High = (unsigned short)(((unsigned long int)ref_frames_arr[15]) >> 16);
	
	pFrameHdr->FrameBuf14Low  = (unsigned short)(((unsigned long int)ref_frames_arr[14]) & 0xFFFF);
        pFrameHdr->FrameBuf14High = (unsigned short)(((unsigned long int)ref_frames_arr[14]) >> 16);

	pFrameHdr->FrameBuf13Low  = (unsigned short)(((unsigned long int)ref_frames_arr[13]) & 0xFFFF);
        pFrameHdr->FrameBuf13High = (unsigned short)(((unsigned long int)ref_frames_arr[13]) >> 16);
	
	pFrameHdr->FrameBuf12Low  = (unsigned short)(((unsigned long int)ref_frames_arr[12]) & 0xFFFF);
        pFrameHdr->FrameBuf12High = (unsigned short)(((unsigned long int)ref_frames_arr[12]) >> 16);
	
	pFrameHdr->FrameBuf11Low  = (unsigned short)(((unsigned long int)ref_frames_arr[11]) & 0xFFFF);
        pFrameHdr->FrameBuf11High = (unsigned short)(((unsigned long int)ref_frames_arr[11]) >> 16);
	
	pFrameHdr->FrameBuf10Low  = (unsigned short)(((unsigned long int)ref_frames_arr[10]) & 0xFFFF);
        pFrameHdr->FrameBuf10High = (unsigned short)(((unsigned long int)ref_frames_arr[10]) >> 16);

	pFrameHdr->FrameBuf9Low  = (unsigned short)(((unsigned long int)ref_frames_arr[9]) & 0xFFFF);
        pFrameHdr->FrameBuf9High = (unsigned short)(((unsigned long int)ref_frames_arr[9]) >> 16);

	pFrameHdr->FrameBuf8Low  = (unsigned short)(((unsigned long int)ref_frames_arr[8]) & 0xFFFF);
        pFrameHdr->FrameBuf8High = (unsigned short)(((unsigned long int)ref_frames_arr[8]) >> 16);

	pFrameHdr->FrameBuf7Low  = (unsigned short)(((unsigned long int)ref_frames_arr[7]) & 0xFFFF);
        pFrameHdr->FrameBuf7High = (unsigned short)(((unsigned long int)ref_frames_arr[7]) >> 16);

	pFrameHdr->FrameBuf6Low  = (unsigned short)(((unsigned long int)ref_frames_arr[6]) & 0xFFFF);
        pFrameHdr->FrameBuf6High = (unsigned short)(((unsigned long int)ref_frames_arr[6]) >> 16);

	pFrameHdr->FrameBuf5Low  = (unsigned short)(((unsigned long int)ref_frames_arr[5]) & 0xFFFF);
        pFrameHdr->FrameBuf5High = (unsigned short)(((unsigned long int)ref_frames_arr[5]) >> 16);

	pFrameHdr->FrameBuf4Low  = (unsigned short)(((unsigned long int)ref_frames_arr[4]) & 0xFFFF);
        pFrameHdr->FrameBuf4High = (unsigned short)(((unsigned long int)ref_frames_arr[4]) >> 16);

	pFrameHdr->FrameBuf3Low  = (unsigned short)(((unsigned long int)ref_frames_arr[3]) & 0xFFFF);
        pFrameHdr->FrameBuf3High = (unsigned short)(((unsigned long int)ref_frames_arr[3]) >> 16);

	pFrameHdr->FrameBuf2Low  = (unsigned short)(((unsigned long int)ref_frames_arr[2]) & 0xFFFF);
        pFrameHdr->FrameBuf2High = (unsigned short)(((unsigned long int)ref_frames_arr[2]) >> 16);

	pFrameHdr->FrameBuf1Low  = (unsigned short)(((unsigned long int)ref_frames_arr[1]) & 0xFFFF);
        pFrameHdr->FrameBuf1High = (unsigned short)(((unsigned long int)ref_frames_arr[1]) >> 16);

	pFrameHdr->FrameBuf0Low  = (unsigned short)(((unsigned long int)ref_frames_arr[0]) & 0xFFFF);
        pFrameHdr->FrameBuf0High = (unsigned short)(((unsigned long int)ref_frames_arr[0]) >> 16);
  }
}

/*===========================================================================
FUNCTION:    GetNewSliceBuf()

DESCRIPTION:  
  This function is called to get a new slice buffer to send to the DSP

RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
PL_ERROR H264_PAL_VLD::GetNewSliceBuf()
{
  slice_write_ptr_VLD = GetNewSliceBufferDataPtr();
  if (slice_write_ptr_VLD == NULL) return PL_ERR_OUT_OF_MEMORY;
  return PL_ERR_NONE;
}
/*===========================================================================
FUNCTION:     AddFrameHeader

DESCRIPTION:  
  This fn MUST be called up every frame before adding MBs. 
  It does the following to setup for MB packing into slicebuf
    1. Get a new slice buffer from the MP4 layer
    2. Fills up the frame header
    3. Increments the slice buffer pointer just beyond the frame header, 
       ready for filling the MBs

RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
PL_ERROR H264_PAL_VLD::AddFrameHeader(void *StatsPtr)
{
  unsigned short numBytes = 0;

  if(qtv_dbg_wait_for_SB_Full)
  {
    if(pCurrentSliceBuf_VLD)
    {
      //if there is already a slice pending with us..queue it.
     (void)FillSubPacketsToDSPSliceBuffer();
    }
  }
  /*-------------------------------------------------------------------------
    Get a new slice buffer pointer
  -------------------------------------------------------------------------*/
  if (GetNewSliceBuf() == PL_ERR_OUT_OF_MEMORY)
    return PL_ERR_OUT_OF_MEMORY;

  pFrameHdr = (H264PL_VLD_FrameHeaderPacketType*) slice_write_ptr_VLD;
  /*-------------------------------------------------------------------------
    Copy the entire frame header into the slice buf
  -------------------------------------------------------------------------*/
  numBytes = sizeof( H264PL_VLD_FrameHeaderPacketType );
  memcpy (slice_write_ptr_VLD, &h264_VLD_frame_header, numBytes);
  slice_write_ptr_VLD += numBytes >> 1;

  QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_MED, "H264 New Frame");
  pCurrentSliceBuf_VLD->fFirstMB = 1;
  //update the stats pointer in the frame
  pCurrentSliceBuf_VLD->pFrameStatsInPtr = (VDL_Frame_Stats_Pkt_Type *)StatsPtr;

  return PL_ERR_NONE;
}

/* ======================================================================
FUNCTION
  H264_PAL_VLD::EOS

DESCRIPTION
  Sends notification to decoder that the stream has ended.

PARAMETERS
  None 
RETURN VALUE

NOTES
  This function is useful for error concealment, and so that the decoder 
  doesn't wait indefinitely for the last chunk of data.
========================================================================== */
PL_ERROR H264_PAL_VLD::EOS(void *StatsPtr)
{
  /*-------------------------------------------------------------------------
    Get a new slice buffer pointer
  -------------------------------------------------------------------------*/
  if(qtv_dbg_wait_for_SB_Full)
  {
    if(pCurrentSliceBuf_VLD)
    {
      //if there is already a slice pending with us..queue it.
     (void)FillSubPacketsToDSPSliceBuffer();
    }
  }

  if (GetNewSliceBuf() == PL_ERR_OUT_OF_MEMORY)
    return PL_ERR_OUT_OF_MEMORY;

  pCurrentSliceBuf_VLD->fAllocateYUVBuf = 1;
  pCurrentSliceBuf_VLD->NumMacroBlocks++;
  pCurrentSliceBuf_VLD->fFirstMB = 1;

  pCurrentSliceBuf_VLD->fLastMB = 1;
  pCurrentSliceBuf_VLD->CodecType = VDL_CODEC_H264_VLD; 
  //update the stats pointer in the frame
  pCurrentSliceBuf_VLD->pFrameStatsInPtr = (VDL_Frame_Stats_Pkt_Type *)StatsPtr;

  h264_VLD_EOS_frame_header.PacketId = 0xB203;
  h264_VLD_EOS_frame_header.EndMarker = 0x7FFF;

  pEOS = (H264PL_VLD_EOS_FrameHeaderPacketType*) slice_write_ptr_VLD;
  memcpy (slice_write_ptr_VLD, &h264_VLD_EOS_frame_header, sizeof( h264_VLD_EOS_frame_header ));
  pCurrentSliceBuf_VLD->SliceDataSize = sizeof( h264_VLD_EOS_frame_header );

  VDL_Queue_Slice_Buffer(pDLPtr, pCurrentSliceBuf_VLD);

  pCurrentSliceBuf_VLD = NULL;
  slice_write_ptr_VLD = NULL;

  return PL_ERR_NONE;
}
/*===========================================================================
FUNCTION:     AddSliceHeader

DESCRIPTION:  
  Adds a slice header describing the VCL data for the DSP
RETURN VALUE: 
  PL_ERROR

SIDE EFFECTS: 
  None. 
===========================================================================*/
PL_ERROR H264_PAL_VLD::AddSliceHeader(unsigned char *pBitstream_org, unsigned char slice_type, 
                                      unsigned short first_mb_in_slice, unsigned char disable_deblocking_filter_idc, signed char slice_qp, 
                                      signed short filter_offset_a, signed short filter_offset_b,
                                      unsigned char num_ref_idx_l0_active_minus_one, 
                                      unsigned int bitstream_len, unsigned char bits_read,
                                      unsigned char   *h264_map_ref_tab_L0,unsigned long int SliceSizeinBytes)
{
  unsigned short numBytes;
  unsigned long int TotalSliceSize;
  if(qtv_dbg_wait_for_SB_Full)
  {
    //We know the slice size that we are going to fill..see if we have enough space
    TotalSliceSize = sizeof( H264PL_VLD_P_SliceHeaderPacketType ) + SliceSizeinBytes + SliceSizeinBytes%2;
    if((pCurrentSliceBuf_VLD->SliceBufferSize - pCurrentSliceBuf_VLD->SliceDataSize) < TotalSliceSize)
    {
      QTV_MSG_PRIO2(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH,"SliceBufferFull..sending to VDL BytesWritten = %ld,CurrentRequest = %ld",
                     (pCurrentSliceBuf_VLD->SliceDataSize),(TotalSliceSize) );

     (void)FillSubPacketsToDSPSliceBuffer();

     /*-------------------------------------------------------------------------
        Get a new slice buffer pointer
      -------------------------------------------------------------------------*/
      if (GetNewSliceBuf() == PL_ERR_OUT_OF_MEMORY)
        return PL_ERR_OUT_OF_MEMORY;
    }
  }
  if (IS_SLICE_I(slice_type))
  {
    /*-------------------------------------------------------------------------
    Copy the entire frame header into the slice buf
    -------------------------------------------------------------------------*/
    H264PL_VLD_I_SliceHeaderPacketType *ptr = (H264PL_VLD_I_SliceHeaderPacketType*) slice_write_ptr_VLD;
    ptr->PacketId = H264_VLD_SLICE_HEADER_PACKET_TYPE;
    ptr->Slice_Info_0 = first_mb_in_slice;
    ptr->Slice_Info_1 = ( (((bitstream_len % 2) << 11) & 0x0800)|
                          ( ((slice_type % 5) << 8) & 0x0700) |
                          ( slice_qp & 0x00FF) );
    ptr->Slice_Info_2 = ( ( (disable_deblocking_filter_idc << 10) & 0x0C00) |
                          ( ((filter_offset_a + 12) << 5) & 0x03E0) |
                          ( (filter_offset_b + 12) & 0x001F) );

    bitstream_len += (bitstream_len % 2);
    ptr->NumBytesInRBSPHigh = (unsigned short) (bitstream_len >> 16);
    ptr->NumBytesInRBSPLow = (unsigned short) (bitstream_len & 0xFFFF);
    ptr->NumBitsInRBSPConsumed = bits_read;
    ptr->EndMarker = 0x7FFF;

    numBytes = sizeof( H264PL_VLD_I_SliceHeaderPacketType );
  }
  else if (IS_SLICE_P(slice_type))
  {
    /*-------------------------------------------------------------------------
    Copy the entire frame header into the slice buf
    -------------------------------------------------------------------------*/
    H264PL_VLD_P_SliceHeaderPacketType *ptr = (H264PL_VLD_P_SliceHeaderPacketType*) slice_write_ptr_VLD;
    ptr->PacketId = 0xB202;
    ptr->Slice_Info_0 = first_mb_in_slice;
    ptr->Slice_Info_1 = ( (((bitstream_len % 2) << 11) & 0x0800)|
                          ( ((slice_type % 5) << 8) & 0x0700) |
                          ( slice_qp & 0x00FF) );
    ptr->Slice_Info_2 = ( ( (disable_deblocking_filter_idc << 10) & 0x0C00) |
                          ( ((filter_offset_a + 12) << 5) & 0x03E0) |
                          ( (filter_offset_b + 12) & 0x001F) );
    ptr->Slice_Info_3 = (num_ref_idx_l0_active_minus_one & 0x001F);
    ptr->RefIdxMapInfo_0 = ( ( (h264_map_ref_tab_L0[0] << 12) & 0xF000) |
                             ( (h264_map_ref_tab_L0[1] << 8) & 0x0F00) |
                             ( (h264_map_ref_tab_L0[2] << 4)  & 0x00F0) |
                             ( (h264_map_ref_tab_L0[3])  & 0x000F ) );
    ptr->RefIdxMapInfo_1 = ( ( (h264_map_ref_tab_L0[4] << 12) & 0xF000) |
                             ( (h264_map_ref_tab_L0[5] << 8) & 0x0F00) |
                             ( (h264_map_ref_tab_L0[6] << 4)  & 0x00F0) |
                             ( (h264_map_ref_tab_L0[7])  & 0x000F ) );
    ptr->RefIdxMapInfo_2 = ( ( (h264_map_ref_tab_L0[8] << 12) & 0xF000) |
                             ( (h264_map_ref_tab_L0[9] << 8) & 0x0F00) |
                             ( (h264_map_ref_tab_L0[10] << 4)  & 0x00F0) |
                             ( (h264_map_ref_tab_L0[11])  & 0x000F ) );
    ptr->RefIdxMapInfo_3 = ( ( (h264_map_ref_tab_L0[12] << 12) & 0xF000) |
                             ( (h264_map_ref_tab_L0[13] << 8) & 0x0F00) |
                             ( (h264_map_ref_tab_L0[14] << 4)  & 0x00F0) |
                             ( (h264_map_ref_tab_L0[15])  & 0x000F ) );

    bitstream_len += (bitstream_len % 2);
    ptr->NumBytesInRBSPHigh = (unsigned short) (bitstream_len >> 16);
    ptr->NumBytesInRBSPLow = (unsigned short) (bitstream_len & 0xFFFF);
    ptr->NumBitsInRBSPConsumed = bits_read;
    ptr->EndMarker = 0x7FFF;

    numBytes = sizeof( H264PL_VLD_P_SliceHeaderPacketType );
  }
  else
  {
    return PL_ERR_INVALID_STATE;
  }
  slice_write_ptr_VLD += numBytes >> 1;
  pCurrentSliceBuf_VLD->SliceDataSize += numBytes;
    

  return PL_ERR_NONE;
}
/*===========================================================================
FUNCTION:     AddSlice

DESCRIPTION:  

RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
PL_ERROR H264_PAL_VLD::AddSlice(unsigned char *pBitstream, unsigned long int numBytes,void *outAddr)
{
  unsigned long int *p32SliceWrite = (unsigned long int*) slice_write_ptr_VLD;
  unsigned char  *p8SliceWrite =  (unsigned char*) slice_write_ptr_VLD;
  unsigned char *src = pBitstream;
  unsigned long int temp32BS = 0;
  unsigned long int numBytesIn = numBytes;

  if((pCurrentSliceBuf_VLD->SliceBufferSize - pCurrentSliceBuf_VLD->SliceDataSize) < numBytes)
  {
    QTV_MSG_PRIO2(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_ERROR,"Small Slice Buffer.Available Size = %ld,Bytes to write = %ld",
                   (pCurrentSliceBuf_VLD->SliceBufferSize - pCurrentSliceBuf_VLD->SliceDataSize),(numBytes+ numBytes%2) );

    return PL_ERR_OUT_OF_MEMORY;
  }

  if ((unsigned long int)p8SliceWrite % 4)
  {
    *p8SliceWrite++ = *(src+1);
    *p8SliceWrite++ = *src;
    p32SliceWrite = (unsigned long int*)p8SliceWrite;
    src+=2;
    numBytes -= 2;
  }

  /* write the bitstream data 32 bits at a time into the slice buffer */
  for (int i = 0; i < (numBytes >> 2); ++i)
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
    *p8SliceWrite++ = 0x0;
    *p8SliceWrite++ = *src;
  }
  slice_write_ptr_VLD = (unsigned short*)p8SliceWrite;
  pCurrentSliceBuf_VLD->SliceDataSize += numBytesIn + (numBytesIn % 2);

  if(qtv_dbg_wait_for_SB_Full == 0)
  {
    QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH,"Sending Slice Immediately");
    pCurrentSliceBuf_VLD->pFrameStatsInPtr = (VDL_Frame_Stats_Pkt_Type *)outAddr;

    FillSubPacketsToDSPSliceBuffer();
  }
  return PL_ERR_NONE;
}
/*===========================================================================
FUNCTION:
  FillSubPacketsToDSPSliceBuffer

DESCRIPTION:
  This function is called from the H.264 library decode MB function and is 
  passed a H264QDSP_MBPacketType which contains decoded sub packets. This 
  function is reponsible for filling a slice buffer if it already exists or
  getting a new slice buffer if it does not or if the packet does not fit 
  in the existing slice buffer.

INPUT/OUTPUT PARAMETERS:
  *pkt    : Source of data (ARM VLC decoded) to be used to pack the slice buffer

RETURN VALUE:
  None

SIDE EFFECTS: 
  Packet ready for DSP consumption is generated and is ready to be queued

==============================================================================*/
PL_ERROR H264_PAL_VLD::FillSubPacketsToDSPSliceBuffer()
{
  pCurrentSliceBuf_VLD->fAllocateYUVBuf = 1;
  pCurrentSliceBuf_VLD->NumMacroBlocks++;

  pCurrentSliceBuf_VLD->fLastMB = 1;
  pCurrentSliceBuf_VLD->CodecType = VDL_CODEC_H264_VLD; 

  (void)VDL_Queue_Slice_Buffer(pDLPtr,pCurrentSliceBuf_VLD);
  pCurrentSliceBuf_VLD = NULL;
  slice_write_ptr_VLD = NULL;

  return PL_ERR_NONE;
}

/*===========================================================================
FUNCTION:
  FillPPCommandPacketsToDL

DESCRIPTION:
  This function is called from the H.264 library to send a PP cmd to the
  DL.

INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE:
  None

SIDE EFFECTS: 
  Packet ready for DSP consumption is generated and is ready to be queued

==============================================================================*/
PL_ERROR H264_PAL_VLD::FillPPCommandPacketsToDL(void *Output_frame_ptr)
{
  VDL_PP_cmd_Info_Type   *p_PP_cmd_buf = NULL;

  p_PP_cmd_buf = VDL_Get_PP_Pkt_Buffer(pDLPtr);
  if(p_PP_cmd_buf == NULL)
  {
    QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_FATAL, "Out of memory");
    return PL_ERR_OUT_OF_MEMORY;
  }
  p_PP_cmd_buf->CodecType = 0x0; 
  p_PP_cmd_buf->PP_Output_ptr = Output_frame_ptr;
  p_PP_cmd_buf->pp_Param_0 = 0x4000;
  
  (void)VDL_Queue_PP_Pkt_Buffer(pDLPtr,p_PP_cmd_buf);

  return PL_ERR_NONE;
}

/*===========================================================================
FUNCTION:
  GetNewSliceBufferDataPtr

DESCRIPTION:
  This function is called to get the pointer to a new slice data buffer, so that
  the decoded MB information can be written to it. 

INPUT/OUTPUT PARAMETERS:
  Ptr to Slice Data

RETURN VALUE:
  None

SIDE EFFECTS: 
  Note: Also sets the pCurrentSliceBuf_VLD which is of static scope to this file
==============================================================================*/
unsigned short* H264_PAL_VLD::GetNewSliceBufferDataPtr(void)
{
  pCurrentSliceBuf_VLD = VDL_Get_Slice_Buffer(pDLPtr);

  if (pCurrentSliceBuf_VLD == NULL)
  {
    QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_FATAL, "Out of memory");
    return NULL;
  }
  pCurrentSliceBuf_VLD->SliceDataSize = 0;
  //Defaulting to false
  pCurrentSliceBuf_VLD->fFirstMB = 0;
  return(unsigned short*) pCurrentSliceBuf_VLD->pSliceData;
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

SIDE EFFECTS: 
==============================================================================*/
void H264_PAL_VLD::FreeSliceData(void)
{
  if (pCurrentSliceBuf_VLD != NULL)
  {
    VDL_Free_Slice_Buffer(pDLPtr,pCurrentSliceBuf_VLD);
    pCurrentSliceBuf_VLD = NULL;
  }
  return;
}


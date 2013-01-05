#ifndef H264_PAL_VLD_H
#define H264_PAL_VLD_H
/*=============================================================================
        h264_vld_pal.h
DESCRIPTION:
   This file provides packetization functionality to abstract regular decoding
   with packetization specific to DSP.

  ABSTRACT:
   This file provides the interface specification for the H264 video macro
   block decoder implemented in the QDSP.
   Applications utilize these H264 services through a high-level MPEG 4
   player API and shall never call either the video codec API {mp4_codec.h}
   nor these services directly.

   Video decoding and encoding functionality is accomplished by
   several modules including but not limited to the: media player
   application, MPEG 4 engine, video codec, & audio codec.
                                                      State = Decode
EXTERNALIZED FUNCTIONS
  List functions and a brief description that are exported from this file

INITIALIZATION AND SEQUENCING REQUIREMENTS
  Detail how to initialize and use this service.  The sequencing aspect
  is only needed if the order of operations is important.


Copyright (c) 2007 by QUALCOMM, Incorporated.  All Rights Reserved. 
QUALCOMM Proprietary and Confidential.

============================================================================*/
/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/H264Vdec/main/latest/src/PL/h264_pal_vld.h#1 $
$DateTime: 2008/08/05 12:05:56 $
$Change: 717059 $

========================================================================== */

/* =======================================================================
**               Includes and Public Data Declarations
** ======================================================================= */
#include "h264_pal.h"


class H264_PAL_VLD:public H264_PAL
{
public:
  H264_PAL_VLD(void *pDL);
  ~H264_PAL_VLD();
  void FillFrameHeaderConstants(unsigned char** FrameBuffAddr,unsigned int num_frame_buffers, H264PL_FrameHeaderPacketType* p_h264_frame_header);
  void UpdateFrameHeader(unsigned short X_Dimension,unsigned short Y_Dimension,
                         unsigned char ErrConcealType, unsigned char ChromaQPIndexOffset,
                         unsigned char intraPredFlag, unsigned char num_ref_frames,
                         unsigned char* Output_frame_buffer, unsigned char** ref_frames_arr);
  PL_ERROR AddFrameHeader(void *outAddr);
  PL_ERROR AddSliceHeader(unsigned char *pBitstream_org, unsigned char slice_type, 
                          unsigned short first_mb_in_slice, unsigned char disable_deblocking_filter_idc, signed char slice_qp, 
                          signed short filter_offset_a, signed short filter_offset_b,
                          unsigned char num_ref_idx_l0_active_minus_one, 
                          unsigned int bitstream_len, unsigned char bits_read,
                          unsigned char   *h264_map_ref_tab_L0,unsigned long int SliceSizeinBytes);
  PL_ERROR AddSlice(unsigned char *pBitstream, unsigned long int numBytes,void *outAddr);

  /* Fills the DSp slice buffer and sends it to DL */
  PL_ERROR FillSubPacketsToDSPSliceBuffer();

  /*Fill and send a PP Cmd to DL*/
  PL_ERROR FillPPCommandPacketsToDL(void *outputAddr);

  PL_ERROR GetNewSliceBuf();
  unsigned short* GetNewSliceBufferDataPtr(void);
  void FreeSliceData(void);
  PL_ERROR EOS(void *outputAddr);

  unsigned char RefIdxReorderFlag;
  void *pDLPtr; //DL Instance Pointer
private:
  /*
  ** Define a structure for the Frame Header packet that goes to the DSP
  */
  typedef struct
  {
    unsigned short PacketId;            /* Should be MP4_FRAME_HEADER_PACKET_TYPE */
    unsigned short Slice_Info_0;           
    unsigned short Slice_Info_1;           
    unsigned short Slice_Info_2;           
    unsigned short NumBytesInRBSPHigh;           
    unsigned short NumBytesInRBSPLow;           
    unsigned short NumBitsInRBSPConsumed;           
    unsigned short EndMarker;           /* Should be MP4_MB_END_MARKER            */
  } H264PL_VLD_I_SliceHeaderPacketType;
  H264PL_VLD_I_SliceHeaderPacketType h264_VLD_I_slice_header;

  /*
** Define a structure for the Frame Header packet that goes to the DSP
*/
  typedef struct
  {
    unsigned short PacketId;            /* Should be MP4_FRAME_HEADER_PACKET_TYPE */
    unsigned short Slice_Info_0;           
    unsigned short Slice_Info_1;           
    unsigned short Slice_Info_2;           
    unsigned short Slice_Info_3;        
    unsigned short RefIdxMapInfo_0;
    unsigned short RefIdxMapInfo_1;
    unsigned short RefIdxMapInfo_2;
    unsigned short RefIdxMapInfo_3;
    unsigned short NumBytesInRBSPHigh;           
    unsigned short NumBytesInRBSPLow;           
    unsigned short NumBitsInRBSPConsumed;           
    unsigned short EndMarker;           /* Should be MP4_MB_END_MARKER            */
  } H264PL_VLD_P_SliceHeaderPacketType;
  H264PL_VLD_P_SliceHeaderPacketType h264_VLD_P_slice_header;

  /*
  ** Define a structure for the Frame Header packet that goes to the DSP
  */
  typedef struct
  {
    unsigned short PacketId;            /* Should be MP4_FRAME_HEADER_PACKET_TYPE */
    unsigned short EndMarker;           /* Should be MP4_MB_END_MARKER            */
  } H264PL_VLD_EOS_FrameHeaderPacketType;
  H264PL_VLD_EOS_FrameHeaderPacketType h264_VLD_EOS_frame_header;
  H264PL_VLD_EOS_FrameHeaderPacketType *pEOS;

  /*
  ** Define a structure for the Frame Header packet that goes to the DSP
  */
  typedef struct
  {
    unsigned short PacketId;            /* Should be MP4_FRAME_HEADER_PACKET_TYPE */
    unsigned short X_Dimension;         /* Width of frame in pixels               */
    unsigned short Y_Dimension;         /* Height of frame in pixels              */
#ifndef T_MSM7500
    unsigned short Reserved;            
#else
    unsigned short LineWidth;           /* Line width for MSM7500                  */
#endif
    unsigned short FrameInfo_0;         /* FRAME_INFO_0 bits                       */
    unsigned short FrameBuf0High;       /* High 16 bits of VOP buffer 0, byte-addr*/
    unsigned short FrameBuf0Low;        /* Low 16 bits...                         */
    unsigned short FrameBuf1High;       /* High 16 bits of VOP buffer 1, byte-addr*/
    unsigned short FrameBuf1Low;        /* Low 16 bits...                         */
    unsigned short FrameBuf2High;       /* High 16 bits of VOP buffer 2, byte-addr*/
    unsigned short FrameBuf2Low;        /* Low 16 bits...                         */
    unsigned short FrameBuf3High;       /* High 16 bits of VOP buffer 3, byte-addr*/
    unsigned short FrameBuf3Low;        /* Low 16 bits...                         */
    unsigned short FrameBuf4High;       /* High 16 bits of VOP buffer 4, byte-addr*/
    unsigned short FrameBuf4Low;        /* Low 16 bits...                         */
    unsigned short FrameBuf5High;       /* High 16 bits of VOP buffer 5, byte-addr*/
    unsigned short FrameBuf5Low;        /* Low 16 bits...                         */
    unsigned short FrameBuf6High;       /* High 16 bits of VOP buffer 6, byte-addr*/
    unsigned short FrameBuf6Low;        /* Low 16 bits...                         */
    unsigned short FrameBuf7High;       /* High 16 bits of VOP buffer 7, byte-addr*/
    unsigned short FrameBuf7Low;        /* Low 16 bits...                         */
    unsigned short FrameBuf8High;       /* High 16 bits of VOP buffer 8, byte-addr */
    unsigned short FrameBuf8Low;        /* Low 16 bits...                          */
    unsigned short FrameBuf9High;       /* High 16 bits of VOP buffer 9, byte-addr */
    unsigned short FrameBuf9Low;        /* Low 16 bits...                          */
    unsigned short FrameBuf10High;      /* High 16 bits of VOP buffer 10, byte-addr*/
    unsigned short FrameBuf10Low;       /* Low 16 bits...                          */
    unsigned short FrameBuf11High;      /* High 16 bits of VOP buffer 11, byte-addr*/
    unsigned short FrameBuf11Low;       /* Low 16 bits...                          */
    unsigned short FrameBuf12High;      /* High 16 bits of VOP buffer 12, byte-addr*/
    unsigned short FrameBuf12Low;       /* Low 16 bits...                          */
    unsigned short FrameBuf13High;      /* High 16 bits of VOP buffer 13, byte-addr*/
    unsigned short FrameBuf13Low;       /* Low 16 bits...                          */
    unsigned short FrameBuf14High;      /* High 16 bits of VOP buffer 14, byte-addr*/
    unsigned short FrameBuf14Low;       /* Low 16 bits...                          */
    unsigned short FrameBuf15High;      /* High 16 bits of VOP buffer 15, byte-addr*/
    unsigned short FrameBuf15Low;       /* Low 16 bits...                          */
    unsigned short OutputFrameBufHigh;  /* High 16 bits of Output frame,  byte-addr*/
    unsigned short OutputFrameBufLow;   /* Low 16 bits...                          */
    unsigned short EndMarker;           /* Should be MP4_MB_END_MARKER            */
  } H264PL_VLD_FrameHeaderPacketType;
  H264PL_VLD_FrameHeaderPacketType h264_VLD_frame_header;

  H264PL_VLD_FrameHeaderPacketType *pFrameHdr;
  unsigned short *slice_write_ptr_VLD;
  VDL_Slice_Pkt_Type* pCurrentSliceBuf_VLD;
};
#endif //H264_PAL_VLD_H

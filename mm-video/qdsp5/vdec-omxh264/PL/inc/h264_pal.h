#ifndef H264_PAL_H
#define H264_PAL_H
/*=============================================================================
        h264_pal.h
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

$Header: //source/qcom/qct/multimedia2/Video/Dec/H264Vdec/main/latest/src/PL/h264_pal.h#2 $
$DateTime: 2008/10/03 16:17:12 $
$Change: 756557 $

========================================================================== */

/* =======================================================================
**               Includes and Public Data Declarations
** ======================================================================= */
#ifndef FEATURE_H264_CONSOLE_APP
#else
  #include "qc_def.h"
#endif
#include <string.h>

#include "vdecoder_types.h"
#include "VDL_API.h"

// Define compact packet feature
#ifdef FEATURE_QDSP_RTOS
  #define COMPACT_PKT
#endif /* FEATURE_QDSP_RTOS */

#define XSCALAR_CMD_LENGTH 19

/*************************************************************************
*  Below few enums are still used in TL. To be cleaned                   *
*************************************************************************/

/* enumeration for type of prediction mode (same as the MSM Video Player
** Interface Specification
*/
typedef enum
{
  H264PL_PREDICTION_MODE_IPCM,
  H264PL_PREDICTION_MODE_INTRA_4x4,
  H264PL_PREDICTION_MODE_INTRA_16x16,
  H264PL_PREDICTION_MODE_SKIP_MB,
  H264PL_PREDICTION_MODE_INTER,
  H264PL_PREDICTION_MODE_RESERVED_1,
  H264PL_PREDICTION_MODE_RESERVED_2,
  H264PL_PREDICTION_MODE_RESERVED_3
} H264PL_PredictionModeEnum;

/* enumeration for type of prediction mode (same as the MSM Video Player
** Interface Specification
*/
typedef enum
{
  H264PL_LUMA_4x4_DC1,
  H264PL_LUMA_4x4_DC2,
  H264PL_LUMA_4x4_DC3,
  H264PL_LUMA_4x4_DC4,
  H264PL_LUMA_4x4_VERTICAL,
  H264PL_LUMA_4x4_HORIZONTAL,
  H264PL_LUMA_4x4_DIAG_DWN_LEFT,
  H264PL_LUMA_4x4_DIAG_DWN_RIGHT,
  H264PL_LUMA_4x4_VERTICAL_RIGHT,
  H264PL_LUMA_4x4_HORIZONTAL_DWN,
  H264PL_LUMA_4x4_VERTICAL_LEFT,
  H264PL_LUMA_4x4_HORIZONTAL_UP,
  H264PL_LUMA_4x4_RESERVED
} H264PL_Luma4x4ModeEnum;

/* enumeration for type of prediction mode (same as the MSM Video Player
** Interface Specification
*/
typedef enum
{
  H264PL_CHROMA8x8_PRED_MODE_DC0,
  H264PL_CHROMA8x8_PRED_MODE_DC1,
  H264PL_CHROMA8x8_PRED_MODE_DC2,
  H264PL_CHROMA8x8_PRED_MODE_DC3,
  H264PL_CHROMA8x8_PRED_MODE_VERTICAL,
  H264PL_CHROMA8x8_PRED_MODE_HORIZONTAL,
  H264PL_CHROMA8x8_PRED_MODE_PLANAR,
  H264PL_CHROMA8x8_PRED_MODE_RESERVED
} H264PL_CHROMA8x8ModeEnum;

/* enumeration for type of prediction mode (same as the MSM Video Player
** Interface Specification
*/
typedef enum
{
  H264PL_LUMA16x16_PRED_MODE_DC1,
  H264PL_LUMA16x16_PRED_MODE_DC2,
  H264PL_LUMA16x16_PRED_MODE_DC3,
  H264PL_LUMA16x16_PRED_MODE_DC4,
  H264PL_LUMA16x16_PRED_MODE_VERTICAL,
  H264PL_LUMA16x16_PRED_MODE_HORIZONTAL,
  H264PL_LUMA16x16_PRED_MODE_PLANAR,
  H264PL_LUMA16x16_PRED_MODE_RESERVED
} H264PL_LUMA16x16ModeEnum;

/*enumeration specifying color info */
typedef enum
{
  LUMA,
  CHROMA_CB,
  CHROMA_CR
}H264PL_COLORINFO;

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

class H264_PAL
{
public:
/******************************************************************************
*                Functions to interact with Driver layer                      *
******************************************************************************/
  H264_PAL(void *pDL);
  ~H264_PAL();

  void *pDLPtr; //DL Instance Pointer

  //VLD interface type 
  VDL_Interface_Type VLDInterfaceType;
  //Current Slice buffer pointer
  VDL_Slice_Pkt_Type* pCurrentSliceBuf;

  PL_ERROR Initialize(VDL_Decoder_Cb_Type   DecoderCB,  /* decode done call back function */
                      void *ddUserData,                /* user data for DecoderCB */
                      VDL_Concurrency_Type concurrencyConfig,                  /* Encoded audio format */ 
		      int adsp_fd
                       );

  PL_ERROR ConfigureHW( unsigned short height,
                        unsigned short width, 
                        unsigned char **frame_buffers, 
                        unsigned int num_frame_buffers,
                        VDEC_SLICE_BUFFER_INFO * m_pSliceBufferInfo );
  PL_ERROR Terminate();
  PL_ERROR Flush();
  PL_ERROR Resume(VDL_Concurrency_Type concurrencyConfig);
  PL_ERROR Suspend();
  void QueueStatsBuffer(void *,unsigned char isFruc);
  void QueueEOSStatsBuffer(void *pDecodeStats);
  PL_ERROR EOS(void* outputAddr);

  unsigned char IsSliceBufAvailableForDecode();

  /******************************************************************************
  * Functions to update Frame header Info                                       *
  ******************************************************************************/
  void FillFrameHeaderConstants(unsigned char** FrameBuffAddr,unsigned int num_frame_buffers);
  void UpdateFrameHeader(unsigned short X_Dimension,unsigned short Y_Dimension,
                         unsigned short FrameInfo_0,unsigned short FrameInfo_1,
                         unsigned char* Output_frame_buffer,unsigned char iSFrucFrame,
			 unsigned char** ref_frames_arr);
  PL_ERROR AddFrameHeader();

  /******************************************************************************
  * Functions to update MB header Info                                       *
  ******************************************************************************/
  void FillMBHeader(unsigned short  num_mb_pkt_words,unsigned char isFrucMb);
  void UpdateMBQP_Values(unsigned short Value);
  void UpdateMBFilter_Offsets(unsigned short Value);
  void UpdateMbInfo(unsigned short Value);


/*===========================================================================
  data declarations to support inline functions
===========================================================================*/
  unsigned short *slice_cur_mb_start;
  unsigned short *slice_write_ptr;
  unsigned short *pH264NumMV;
  unsigned short *pH264NumCoeff;
  unsigned short *pH264IPCMLuma;
  unsigned short *pH264IPCMCb;
  unsigned short *pH264IPCMCr;
  unsigned short *pH264Coeff;

/*===========================================================================
 ------------------------------------
 SLICE BUFFER DIRECT PACKING EXAMPLES
 ------------------------------------
 Example 1: I4x4 MB
 ------------------
 ---------------------------------------------------------------------------
  # |API to invoke                 | Slice Buffer Packing (Words) 
 ---------------------------------------------------------------------------
  1 |StartMBDecoding                | 
    |                              | 
    |Set hdr->MB_Info , hdr->QP_Values, hdr->Filter_Offsets
    |                              | 
  2 |FillMBHeader(hdr)| 0008 8202 ZZZ1 QPQP FILT
    |                              | 
  3 |FillIntraModes(M[]) | 0000 8202 ZZZ1 QPQP FILT MOD1 MOD2 MOD3
    |                              | MOD4 MOD5 MOD6
    |                              | 
  4 |Lum:ParserDecResidualBlock->  | 
  4a|InitResid(tot_coeff NNNN) | 0000 8202 ZZZ1 QPQP FILT MOD1 MOD2 MOD3
    |                              | MOD4 MOD5 MOD6 NNNN
  4b|FillResid until all resids| 0000 8202 ZZZ1 QPQP FILT MOD1 MOD2 MOD3
    |are packed                    | MOD4 MOD5 MOD6 NNNN COEF COEF COEF COEF
    |                              | 
  5 |Cr DC:ParserDecResidualBlock->|
  5a|InitResid(tot_coeff NNNN) | 0000 8202 ZZZ1 QPQP FILT MOD1 MOD2 MOD3
    |                              | MOD4 MOD5 MOD6 NNNN COEF COEF COEF COEF
    |                              | NNNN 
  5b|FillResid until all resids| 0000 8202 ZZZ1 QPQP FILT MOD1 MOD2 MOD3
    |are packed                    | MOD4 MOD5 MOD6 NNNN COEF COEF COEF COEF
    |                              | NNNN COEF COEF COEF COEF
    |                              | 
  6 |Cr AC:ParserDecResidualBlock->|
  6a|InitResid(tot_coeff NNNN) | 0000 8202 ZZZ1 QPQP FILT MOD1 MOD2 MOD3
    |                              | MOD4 MOD5 MOD6 NNNN COEF COEF COEF COEF
    |                              | NNNN COEF COEF COEF COEF NNNN
  6b|FillResid until all resids| 0000 8202 ZZZ1 QPQP FILT MOD1 MOD2 MOD3
    |are packed                    | MOD4 MOD5 MOD6 NNNN COEF COEF COEF COEF
    |                              | NNNN COEF COEF COEF COEF NNNN COEF COEF 
    |                              | 
  7 |QDSPFillEndMkr            | 0000 8202 ZZZ1 QPQP FILT MOD1 MOD2 MOD3
    |                              | MOD4 MOD5 MOD6 NNNN COEF COEF COEF COEF
    |                              | NNNN COEF COEF COEF COEF NNNN COEF COEF 
    |                              | 7FFF
    |                              |
  8 |QDSPFillMBLen             | LENG 8202 ZZZ1 QPQP FILT MOD1 MOD2 MOD3
    |                              | MOD4 MOD5 MOD6 NNNN COEF COEF COEF COEF
    |                              | NNNN COEF COEF COEF COEF NNNN COEF COEF 
    |                              | 7FFF
    |                              |
  9 |FillSubPacketsToDSPSliceBuffer
 ---------------------------------------------------------------------------

  
 Example 2: Concealed MB
 -----------------------
 ---------------------------------------------------------------------------
  # |API to invoke                 | Slice Buffer Packing (Words) 
 ---------------------------------------------------------------------------
  1 |StartMBDecoding                | 
    |Set hdr->MB_Info , hdr->QP_Values, hdr->Filter_Offsets
  2 |FillMBHeader(hdr)| 0008 8202 0003 QPQP FILT
  3 |FillSkipMV(0,0)       | 0008 8202 0003 QPQP FILT 0000 0000 
  4 |H264QDSPFillEndMkr            | 0008 8202 0003 QPQP FILT 0000 0000 7FFF
  5 |FillSubPacketsToDSPSliceBuffer

 ---------------------------------------------------------------------------
===========================================================================*/
/*===========================================================================
FUNCTION:
  StartMBDecoding

DESCRIPTION:
  This function MUST be called at the beggining of MB decode for each MB.
  It initializes the slice write Pointer to the start of next MB position 
  in the slice buffer

INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE:
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
  void StartMBDecoding()
  {
    slice_write_ptr = slice_cur_mb_start;
  }

/*===========================================================================
FUNCTION:
  NoteCurrentMBStart

DESCRIPTION:
  This function saves the pointer to current MB in the slice, so that 
  data like SPKT_HEADER can be written after rest of the
  decoded data for the MB is written into the slice buffer 


INPUT/OUTPUT PARAMETERS:
  Ptr to Current MB Start location in Slice Data buffer 

RETURN VALUE:
  None

SIDE EFFECTS: 
==============================================================================*/
  void NoteCurrentMBStart(unsigned short* slice_current_mb_start)
  {
    slice_cur_mb_start = slice_current_mb_start;
  }

/*===========================================================================
FUNCTION:     
  InitializeMV

DESCRIPTION:  
  This function initializes the Num MV pointer by pointing it to the current 
  slicebuf position. This function MUST be called before filling the motion 
  vectors using the H264FillMotionVectorPacket
  
INPUT/OUTPUT PARAMETERS:
  numMVs: Number of motion vectors for the current MB. This can be set to 
          zero if the value is unknown and can be set later using 
          SetNumMV
  
RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
  void InitializeMV(unsigned short numMVs)
  {
    pH264NumMV = slice_write_ptr++;
    *pH264NumMV = numMVs;
  }

/*===========================================================================
FUNCTION:     
  SetNumMV

DESCRIPTION:  
  Set the number of motion vectors for this macro block
  
INPUT/OUTPUT PARAMETERS:
  numMVs: Number of motion vectors for the current MB. 
  
RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
  void SetNumMV(unsigned short numMVs)
  {
    *pH264NumMV = numMVs;
  }

/*===========================================================================
FUNCTION:     
  FillSkipMV

DESCRIPTION:  
  This function fills the Skip MB Motion vector values
  
INPUT/OUTPUT PARAMETERS:
  mvX: Skip MVs X motion vector 
  mvY: Skip MVs Y motion vector 
  
RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
  void FillSkipMV(signed short mvX, signed short mvY)
  {
#ifndef T_MSM7500
    *slice_write_ptr ++ = mvX;
    *slice_write_ptr ++ = mvY;
#else
    *slice_write_ptr ++ = mvX  << 1;
    *slice_write_ptr ++ = mvY  << 1;
#endif
  }

/*===========================================================================
FUNCTION:     
  InitResidual

DESCRIPTION:  
  This function initializes the Num residual pointer by pointing it to the 
  current slicebuf position. This function MUST be called before filling 
  the residuals using H264FillResid
  
INPUT/OUTPUT PARAMETERS:
  numCoeff: Number of motion vectors for the current MB. This can be set to 
            zero if the value is unknown and can be set later using 
            H264SetNumResid
  
RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
  void InitResidual(unsigned short numCoeff)
  {
    pH264NumCoeff = slice_write_ptr++;
    *pH264NumCoeff = numCoeff;
  }

/*===========================================================================
FUNCTION:     
  FillResidual

DESCRIPTION:  
  This function packs the next residual in the slice buffer
  
INPUT/OUTPUT PARAMETERS:
  resid: The next residual to be packed into the slice buffer
  
RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
  void FillResidual(unsigned short resid)
  {
    *slice_write_ptr++ = resid;
  }

/*===========================================================================
FUNCTION:     
  FillCoeffiecients

DESCRIPTION:  
  This function packs the number of coeeficients
  
INPUT/OUTPUT PARAMETERS:
  coeff: number of coeeficients
  
RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
  void FillCoeffiecients(unsigned short coeff)
  {
    *slice_write_ptr++ = coeff;
  }

#ifdef FEATURE_ARM_ONLY
  void FillRunLevelPair(unsigned int coeff,short *runlevels);
#endif

/*===========================================================================
FUNCTION:     
  FillZeroResidual

DESCRIPTION:  
  Pack a zero residual into the slice buffer
  
INPUT/OUTPUT PARAMETERS:
  None
  
RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
  void FillZeroResidual()
  {
    *slice_write_ptr++ = 0;
  }

/*===========================================================================
FUNCTION:     
  SetNumResidual

DESCRIPTION:  
  Set the number of residuals packed
  
INPUT/OUTPUT PARAMETERS:
  numCoeff: Number of residuals packed
  
RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
  void SetNumResidual(unsigned short numCoeff)
  {
    *pH264NumCoeff = numCoeff;
  }

  void FillEndMarker();

/*===========================================================================
FUNCTION:     
  FillMBLength

DESCRIPTION:  
  Fill the length of the current MB in the slice buffer
  
INPUT/OUTPUT PARAMETERS:
  None
  
RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
  void FillMBLength()
  {
    *slice_cur_mb_start = (unsigned short) (slice_write_ptr -
                                    slice_cur_mb_start);
  }

#ifdef COMPACT_PKT
  unsigned short *pH264CodecBlockFlags;
/*===========================================================================
FUNCTION:     
  InitCodedBlockFlags

DESCRIPTION:  
  Initialize the dsp driver to pack coded block flags
  
INPUT/OUTPUT PARAMETERS:
  None
  
RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
  void InitCodedBlockFlags()
  {
    pH264CodecBlockFlags = slice_write_ptr;
    slice_write_ptr += 2;
  }

/*===========================================================================
FUNCTION:     
  FillCodedBlockFlags

DESCRIPTION:  
  Fill the coded block flags into the slice buffer
  
INPUT/OUTPUT PARAMETERS:
  coded_block_flags: Coded block flags to be packed into the slice buffer
  
RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
  void FillCodedBlockFlags(unsigned long int coded_block_flags)
  {
    // store coded_block_flags
    pH264CodecBlockFlags[0] = (unsigned short) (coded_block_flags & 0xFFFF);
    pH264CodecBlockFlags[1] = (unsigned short) (coded_block_flags >> 16);
  }
#endif

  void InitIPCMMb(void);
  void FillIPCMPkt(H264PL_COLORINFO Ptr2assign, unsigned short loPix, unsigned short hiPix);

  void FillIntraModes( unsigned short *pkt);
  void FillMotionVectors (signed short    mv_x,    
                          signed short mv_y,
                          unsigned short   ref_idx,
                          unsigned short   x_pos,
                          unsigned short   y_pos,
                          unsigned short   width,
                          unsigned short   height,
                          unsigned char   *h264_map_ref_tab_L0
#ifdef FEATURE_H264_B_FRAMES
                          ,unsigned char   *h264_map_ref_tab_L1
                          ,unsigned char    b_vop_indicator
                          ,unsigned char    listidx
#endif
                         );

/*===========================================================================
FUNCTION:     
  PackFrucInfo

DESCRIPTION:  
  Helper func to pack the fruc mv info (dx, dy) and nz info (nz_ref_mode) 
  into the slice buffer
    
INPUT/OUTPUT PARAMETERS:
  dx: Horizontal MV component for Fruc MB
  dy: Vertical MV component for Fruc MB
  nz_ref_mode: nz component of fruc //7:2 nz; 
                                    //2:1 all_block_ref==0? 1:0
                                    //1:0 intra?0:1
  slicebuf: Slice buffer pointer location where fruc data is to filled
  
RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
  void PackFrucInfo(int dx, int dy, int nz_ref_mode, unsigned short* slicebuf)
  {
    *slicebuf = 0; 
    *slicebuf |= (((-dx) & 0xff) <<8);
    slicebuf++;
    *slicebuf = ((nz_ref_mode & 0xff));
    *slicebuf |= (((-dy) & 0xff) <<8);
  }

/*===========================================================================
FUNCTION:     
  PackFrucMB

DESCRIPTION:  
  Pack the current fruc macroblock to the slice buffer. This function 
  overgrids the data as needed
    
INPUT/OUTPUT PARAMETERS:
  dx: Horizontal MV component for Fruc MB
  dy: Vertical MV component for Fruc MB
  nz_ref_mode: nz component of fruc //7:2 nz; 
                                    //2:1 all_block_ref==0? 1:0
                                    //1:0 intra?0:1
  row: The horizontal position of the current macroblock
  column: The vertical position of the current macroblock
  numCols: The frame width in macroblocks
  
RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
  void PackFrucMB(int dx, int dy, int nz_ref_mode, int row,
                  int column, int numCols)
  {
    unsigned short* slice_buf = slice_write_ptr;

    /*-------------------------------------------------------------------------
      Need to format the fruc_info data in to a format specified in the 
      ARM<-->DSP FRUC interface. so loop over all rows and columns to do 
      this. Also, pad one row on each side prior to giving to the dsp.
      e.g. for QVGA, 20x15 --> 22x17.            
    -------------------------------------------------------------------------*/

    // Pack to left edge if MB is in first column
    if (column == 0)
      PackFrucInfo(dx, dy, nz_ref_mode, &slice_buf[((row+1)*(numCols+2))*2]);

    // Pack the MB to the inner grid location
    PackFrucInfo(dx, dy, nz_ref_mode, &slice_buf[((row+1)*(numCols+2)+column +1)*2]);

    // Pack to right edge if MB is in last column
    if (column == (numCols-1))
      PackFrucInfo(dx, dy, nz_ref_mode, &slice_buf[((row+1)*(numCols+2)+column +2)*2]);

  }

/*===========================================================================
FUNCTION:     
  DoneFrucMB

DESCRIPTION:  
  The slice completion function for the Fruc frame. This function moves the 
  slice buffer 
    
INPUT/OUTPUT PARAMETERS:
  dx: Horizontal MV component for Fruc MB
  dy: Vertical MV component for Fruc MB
  nz_ref_mode: nz component of fruc //7:2 nz; 
                                    //2:1 all_block_ref==0? 1:0
                                    //1:0 intra?0:1
  row: The horizontal position of the current macroblock
  column: The vertical position of the current macroblock
  numCols: The frame width in MBs
  
RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
  void DoneFrucMB(unsigned char numRows, unsigned char numCols)
  {
    unsigned short* slice_buf = slice_write_ptr;

    /*-------------------------------------------------------------------------
      Finally duplicate the top and bottom rows for the top and bottom 
      padding respectively
    -------------------------------------------------------------------------*/
    memcpy(&slice_buf[0], 
           &slice_buf[1*(numCols+2)*2], 
           sizeof(int)*(numCols+2));

    memcpy(&slice_buf[(numRows+1)*(numCols+2)*2], 
           &slice_buf[(numRows)*(numCols+2)*2], 
           sizeof(int)*(numCols+2));

    slice_write_ptr += (numCols+2) * (numRows+2) * 4 / sizeof(unsigned short);
  }

#ifdef T_MSM7500
  void InitACResid(unsigned short numCoeff);
  void FillACResid(unsigned short run, unsigned short lvl);
  void DoneACResid(unsigned short numCoeff);
  void InitDCResid(void);
  void InitDCCoeff(unsigned short numCoeff, unsigned short zerosLeft);
  void FillDCResid(unsigned short coeff, unsigned short numzeros);
#endif // T_MSM7500

  /* Fills the DSp slice buffer and sends it to DL */
  PL_ERROR FillSubPacketsToDSPSliceBuffer( unsigned short mb_cnt,
                                           unsigned short num_mbs,
                                           unsigned char isFruc,
                                           unsigned short frameSizeInMbs);
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
    unsigned short FrameInfo_1;         /* FRAME_INFO_1 bits                       */
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
#ifdef FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A_XSCALE
    unsigned short xScalarBuffer[XSCALAR_CMD_LENGTH];   /* In band xScaling*/
#endif //FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A_XSCALE
    unsigned short EndMarker;           /* Should be MP4_MB_END_MARKER            */
  } H264PL_FrameHeaderPacketType;
  H264PL_FrameHeaderPacketType h264_frame_header;
private:
  /*
  ** Define a structure for the Frame Header sub packet that goes to the DSP
  */
  typedef struct
  {
    unsigned short Packet_Id;            /* Should be H264_FRAME_HEADER_PACKET_TYPE */
    unsigned short MB_Info;             /* Width of frame in pixels                */
    unsigned short QP_Values;           /* Height of frame in pixels               */
    unsigned short Filter_Offsets;      /* FRAME_INFO bits                         */
  } H264PL_HeaderSubPacketType;

  H264PL_HeaderSubPacketType mb_hdr_spkt;

  /* Generic motion vector type 
  */
  typedef struct
  {
    unsigned short Mot_Vec_X_0;
    unsigned short Mot_Vec_Y_0;
    unsigned short Block_Configuration;
  } H264PL_MVType; //Not used right now

  /*
  ** Define a structure for the Motion Vector sub packet that goes to the DSP
  */
  typedef struct
  {
    unsigned short          Num_Blocks;
    H264PL_MVType MV[16];
  } H264PL_MVSubPacketType; //Not Used Right now

  /* Blk Height and Width table (according to MSM Video Player Iface spec
  ** Figure 4.6 */
  unsigned short blk_ht_wd[5];

  /*Functions to interact with DL to get a new slice buffer */
  unsigned short* GetNewSliceBufferDataPtr(void);

  /*Free the current slice buffer to..return it to DL */
  void FreeCurrentSliceBuffer(void);

};
#endif //H264_PAL_H

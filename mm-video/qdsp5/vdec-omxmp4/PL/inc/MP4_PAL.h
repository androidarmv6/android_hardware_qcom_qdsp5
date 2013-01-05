#ifndef MP4_PAL_H
#define MP4_PAL_H
/*=============================================================================
      MP4_PAL.h

DESCRIPTION:
  This file defines the "Packet Abstraction Layer" for the MPEG-4 decoder.  It
  is an interface to the Driver Layer from the "Transform Layer", and abstracts
  all the data packing and driver-communication details from the decoder's 
  algorithms.

   LIMITATIONS:

   ABSTRACT:
   MPEG 4 video decoding and encoding functionality is accomplished by
   several modules including but not limited to the: media player
   application, MPEG 4 engine, MPEG 4 video codec, & MPEG 4 audio codec
   as outlined below:

EXTERNALIZED FUNCTIONS
  List functions and a brief description that are exported from this file

INITIALIZATION AND SEQUENCING REQUIREMENTS
  is only needed if the order of operations is important.

        Copyright ?1999-2002 QUALCOMM Incorporated.
               All Rights Reserved.
            QUALCOMM Proprietary

============================================================================*/
/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/MPEG4Vdec/main/latest/src/PL/MP4_PAL.h#8 $
$DateTime: 2009/03/17 17:15:26 $
$Change: 865323 $

========================================================================== */

/* =======================================================================
**               Includes and Public Data Declarations
** ======================================================================= */

/* ==========================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */
#include "MP4_Types.h"
#include "TL_types.h"
#include "qtv_msg.h"
#include "assert.h"
#include "vdecoder_types.h"
#include "vdecoder_utils.h"
extern "C" {
#include "mp4bitstream.h"
}

#include "VDL_API.h"

/* Local defintion for boolean variables */
#ifdef TRUE
    #undef TRUE
#endif
#ifdef FALSE
    #undef FALSE
#endif
#define TRUE   1   /* Boolean true value. */
#define FALSE  0   /* Boolean false value. */

#define MAX_RUN_LEVEL_PAIRS 32
#define UNPACKED_RUN_LEVEL_ID 0xFFFF


#ifdef FEATURE_QDSP_RTOS
#define MAX_NUM_RVLC 224                //224=32(64pixels)*6(blocks)+32(guard buffer)
#else
#define MAX_NUM_RVLC 384                
#endif /* FEATURE_QDSP_RTOS */
#ifdef FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A
#define MAX_MB_SIZE   (sizeof(MP4QDSPMBHeaderType) + \
                       sizeof(MP4QDSPFrameHeaderType) + \
                       6 * sizeof(unsigned short) + \
                       MAX_NUM_RVLC * sizeof(rvlc_run_level_type) + \
                       2)
#else /* FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A */
#define MAX_MB_SIZE   (sizeof(MP4QDSPMBHeaderType) + \
                       6 * sizeof(unsigned short) + \
                       MAX_NUM_RVLC * sizeof(rvlc_run_level_type) + \
                       2)
#endif /* FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A */


#ifdef FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A
/* Define packet types for MPEG 4 */
#define MP4_FRAME_HEADER_PACKET_TYPE   (0x4D01)
#define MP4_MB_PACKET_TYPE       (0x4D02)
#define MP4_B_MB_PACKET_TYPE     (0x4D03)

#ifdef FEATURE_MP4_H263_PROFILE3 
#define H263P3_FRAME_HEADER_PACKET_TYPE (0x2801)
#define H263P3_MB_PACKET_TYPE           (0x2802)
#endif /* FEATURE_MP4_H263_PROFILE3 */
#endif /* FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A */

/* Define not-coded packet size */
#ifdef FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A
#define MP4_NOT_CODED_PACKET_SIZE      (3)
#else /* FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A */
#define MP4_NOT_CODED_PACKET_SIZE      (2)
#endif /* FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A */


#define BACKWARD_REF_VOP_INDEX      0
#define FORWARD_REF_VOP_INDEX       1
#define UNFILTERED_OUTPUT_VOP_INDEX 2
extern unsigned char qtv_cfg_DSPDeblockingEnable;

void* PMEM_GET_PHYS_ADDR (void* abc);

class MP4_PAL {
public:
  DecodeStatsType *m_pDecodeStats;
	MP4QDSPMBHeaderType *m_pHdr;
  /*******************************************
   * This variable is used to indicate if the 
   * slice is queued to the vdl layer to be 
   * sent to dsp or not. If due to some error
   * slice is not sent to dsp, then this 
   * variable will be false. If it is false 
   * then we need to clear the statsbuffer, 
   * slice buffer and pending release flag.
   * This check is done in TL layer.
   *******************************************/	
  unsigned char sliceSentToVdlLayer;
  #ifdef FEATURE_MPEG4_VLD_DSP
    VDL_Interface_Type VLDInterfaceType;
  #endif
protected:
  #ifdef FEATURE_MPEG4_VLD_DSP  
    unsigned char m_VLDinDSPflag;
  #endif
 void* m_pDecodeBuffer;
 unsigned short* m_pSliceMB;
 unsigned long int m_BufferSize;
 rvlc_run_level_type *m_pRVLC, *m_pRVLCSave, *m_pMaxRVLC, *m_pGuardRVLC;
unsigned short  *m_pRVLCCount;
#if defined (FEATURE_QDSP_RTOS) && defined (FEATURE_MPEG4_COMPACT_PACKET)
rvlc_count_type *m_pCount;
#endif /* defined (FEATURE_QDSP_RTOS) && defined (FEATURE_MPEG4_COMPACT_PACKET) */


  /*
  ** These variables keep track of YUV buffer indexes
  ** that correspond to the base and enhanced layers.
  */

  unsigned short m_subframePacketSeqNum;

#ifdef FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A
MP4QDSPFrameHeaderType m_Mpeg4FrameHdr;
#endif /* FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A */

#ifdef FEATURE_QTV_DECODER_XSCALE_VIDEO
  VDEC_DIMENSIONS  m_outFrameSize;
  VDEC_SCALE_CB_FN m_scaleCbFn;
  void *           m_scaleCbData;
#endif //FEATURE_QTV_DECODER_XSCALE_VIDEO

unsigned short m_SrcWidth,m_SrcHeight;        /* Dimensions of the source clip */
VDL_Video_Type m_CodecType;

public:
unsigned char m_fReuseCurSliceBuf;
unsigned char m_fPartialSliceDecoded;
void* m_pMP4DL;

  unsigned char             m_fShortVideoHeader; /* short header flag */
#if ( defined(FEATURE_VIDEO_AUDIO_DUBBING) || defined(FEATURE_MP4_TRANSCODER) )
  /* Flag indicating whether we are audio dubbing or transcoding */  
  unsigned char m_fProfile3Disable; 
#endif /* FEATURE_VIDEO_AUDIO_DUBBING */

#ifdef FEATURE_QTV_DECODER_LCDB_ENABLE
  /*LCDB related parameters*/
  VDEC_LCDB_STRENGTH LCDBParamSettings;
  unsigned char lcdbParamsChanged;
#endif
dsp_packet_type         m_QDSPPacket;      // for YUV buffers

/*===========================================================================
FUNCTION:
  FreeCurrentSliceBuffer
DESCRIPTION:
Frees the current slice buffer (Done at time of error)  
  
RETURN VALUE:
NONE
  
===========================================================================*/
void FreeCurrentSliceBuffer();

/* <EJECT> */
/*===========================================================================

FUNCTION:
  MP4_PAL

DESCRIPTION:
  Constructor for MPEG-4's packet abstraction layer
  
RETURN VALUE:
  None.

===========================================================================*/
  MP4_PAL();
  MP4_PAL(signed short* pError);
  VDL_ERROR QueueEOSStatsBuffer( DecodeStatsType* pDecoderStatsEOS);
/* <EJECT> */
/*===========================================================================

FUNCTION:
  ~MP4_PAL

DESCRIPTION:
  Destructor for MPEG-4's packet abstraction layer
  
RETURN VALUE:
  None.
===========================================================================*/
~MP4_PAL();

/* <EJECT> */
/*===========================================================================

FUNCTION:
  Initialize 

DESCRIPTION:
  Initialization for MPEG-4's packet abstraction layer
  
RETURN VALUE:
  None.

===========================================================================*/
unsigned char Initialize(unsigned short height, unsigned short width, VDL_Concurrency_Type ConcurrencyFormat, unsigned int streamID, void* DecoderCB, int adsp_fd);

#ifdef FEATURE_MPEG4_VLD_DSP
/*===========================================================================

FUNCTION:
  SetVLDInDSPFlag

DESCRIPTION:
  Configures the driver layer HW
  
RETURN VALUE:
  VDL_ERROR.

===========================================================================*/
unsigned char SetVLDinDSPflag(unsigned char bEnableVLDinDSP);
#endif
/*===========================================================================

FUNCTION:
  Configure_HW 

DESCRIPTION:
  Configures the driver layer HW
  
RETURN VALUE:
  VDL_ERROR.

===========================================================================*/
VDL_ERROR Configure_HW(unsigned short DecHeight, unsigned short DecWidth,
                       unsigned short SrcHeight,unsigned short SrcWidth,VDL_Video_Type VideocodecType,VDEC_SLICE_BUFFER_INFO * m_pSliceBufferInfo);

/*===========================================================================
FUNCTION:
  IsSliceBufAvailableForDecode

DESCRIPTION:
  Query the DL layer to see if a slice buffer is available in the free slice
  buffer pool.  

INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE:
  None

SIDE EFFECTS: 
==============================================================================*/
unsigned char IsSliceBufAvailableForDecode();

/* <EJECT> */
/*===========================================================================

FUNCTION:
  Flush  

DESCRIPTION:
  Flush the MPEG-4 packet abstraction layer
  
RETURN VALUE:
  TRUE if successful, FALSE otherwise.

===========================================================================*/
VDL_ERROR Flush();

/* <EJECT> */
/*===========================================================================

FUNCTION:
  Suspend 

DESCRIPTION:
  Suspend the MPEG-4 packet abstraction layer
  
RETURN VALUE:
  TRUE if successful, FALSE otherwise.

===========================================================================*/
VDL_ERROR Suspend();

/* <EJECT> */
/*===========================================================================

FUNCTION:
  Resume

DESCRIPTION:
  Resume the MPEG-4 packet abstraction layer
  
RETURN VALUE:
  TRUE if successful, FALSE otherwise.

===========================================================================*/
VDL_ERROR Resume(unsigned long concurrencyConfig);

/* <EJECT> */
/*===========================================================================

FUNCTION:
  Scale

DESCRIPTION:
  to Enable disable xScaling
  
RETURN VALUE:
  TRUE if successful, FALSE otherwise.

===========================================================================*/
#ifdef FEATURE_QTV_DECODER_XSCALE_VIDEO
VDEC_ERROR Scale( unsigned short width, unsigned short height,
                  VDEC_DIMENSIONS outFrameSize,
                  VDEC_SCALE_CB_FN scaleCbFn,
                  void * const     pCbData );
#endif //FEATURE_QTV_DECODER_XSCALE_VIDEO
       
/*===========================================================================

FUNCTION:
  SetLCDBStrength

DESCRIPTION:
  Set the LCDB parameters.
  
RETURN VALUE:
  void
===========================================================================*/
void SetLCDBParams(void *lcdbStrength);
       
void set_output_yuv_address( unsigned char *pOutBuf, unsigned char *pVopBuf );
#ifdef FEATURE_MPEG4_VLD_DSP
/*===========================================================================

FUNCTION:
  queue_stats_buffer

DESCRIPTION:
  queues the stats buffer
  
RETURN VALUE:
  void
===========================================================================*/
void queue_stats_buffer(void *pCurrentDecodeStats);
#endif
/* <EJECT> */
/*===========================================================================

FUNCTION:
  init_frame_hdr_default

DESCRIPTION:
  Initialize the standardized portion of the MP4 frame packet header.
INPUT/OUTPUT PARAMETERS:
  pMP4Buf - pointer to the MP4BufType structure containing the YUV buffers

RETURN VALUE:
  None.
  
SIDE EFFECTS:
  None.
===========================================================================*/
#ifdef FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A
void update_frame_header(unsigned short Width, unsigned short Height);
void init_frame_hdr_default(MP4BufType* pMP4Buf, unsigned short Width, unsigned short Height);
#endif /* FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A */

/* <EJECT> */
/*===========================================================================

FUNCTION:
  mp4_conceal_macroblocks

DESCRIPTION:
  Helper function to conceal all MBs from fristMB to lastMB-1 (inclusive).

INPUT/OUTPUT PARAMETERS:
  firstMB
  lastMB
  pCodecInfo
  m_QDSPPacket
  nConcealed

RETURN VALUE:
  None.

SIDE EFFECTS:
  None.

===========================================================================*/
void conceal_macroblocks(
  unsigned long int            firstMB,
  unsigned long int            lastMB,
  dsp_packet_type  *pQDSPPacket,
  unsigned long int           *nConcealed,
  error_status_type          *pErrStat
);

/* <EJECT> */
/*===========================================================================

FUNCTION:
  mp4huffman_decode_RVLC_forward

DESCRIPTION:
  Huffman decoding of RVLC in forward direction

INPUT/OUTPUT PARAMETERS:
  fIntra
  pSlice
  ppRVLC
  nDCT
  run
  level

RETURN VALUE:
  MP4ERROR_SUCCESS
  MP4ERROR_ILL_RVLC_I or MP4ERROR_ILL_RVLC_P
  MP4ERROR_MISSING_MARKERBIT_RVLC_I or MP4ERROR_MISSING_MARKERBIT_RVLC_P

SIDE EFFECTS:
  None.
===========================================================================*/
MP4_ERROR_TYPE mp4huffman_decode_RVLC_forward(
  unsigned char         fIntra,        /* 1 for IVOP, 0 for PVOP
                                  * based on MB_TYPE in MCBPC */
  mp4_slice_type *pSlice,        /* Pointer to slice structure */
  unsigned long int nDCT,                    /* Number of coefficients so far */
  unsigned char        *pLast          /* TRUE if last */
);

/* <EJECT> */
/*===========================================================================

FUNCTION:
  mp4huffman_decode_AC_terms

DESCRIPTION:
  Huffman decoding of DCT coefficients, with escape codes

INPUT/OUTPUT PARAMETERS:
  pSlice
  ppRVLC
  Intra
  dummy
  nDCT

RETURN VALUE:
  MP4ERROR_SUCCESS
  MP4ERROR_ILL_ACTERM_SHORT_HEADER

SIDE EFFECTS:
  None.
===========================================================================*/
signed long int mp4huffman_decode_AC_terms(
  mp4_slice_type *pSlice,              /* Pointer to slice structure */
  unsigned char Intra,                         /* 1 if IVOP, 0 if PVOP */
  unsigned char dummy,                       /* Just make the last parameter consistent with h263 decode func */
  unsigned long int nDCT                          /* Number of coefficients so far */
  );

/*===========================================================================

FUNCTION:
  h263huffman_decode_AC_terms

DESCRIPTION:
  Huffman decoding of DCT coefficients, with escape codes

INPUT/OUTPUT PARAMETERS:
  pSlice
  ppRVLC
  Intra
  fextendedLevel
  nDCT

RETURN VALUE:
  MP4ERROR_SUCCESS
  MP4ERROR_ILL_ACTERM_SHORT_HEADER

SIDE EFFECTS:
  None.
===========================================================================*/
MP4_ERROR_TYPE h263huffman_decode_AC_terms(
  mp4_slice_type *pSlice,              /* Pointer to slice structure */
  unsigned char Intra,                         /* 1 if IVOP, 0 if PVOP */
  unsigned char fextendedLevel,              /* if TRUE check for extended level */
  unsigned long int nDCT                          /* Number of coefficients so far */
  );

/* <EJECT> */
/*===========================================================================

FUNCTION:
  mp4huffman_decode_RVLC_reverse

DESCRIPTION:
  Huffman decoding of RVLC in reverse direction

INPUT/OUTPUT PARAMETERS:
  fIntra
  pSlice
  pRVLC
  pLast
  pLength

RETURN VALUE:
  MP4ERROR_SUCCESS
  MP4ERROR_ILL_RVLC_I or MP4ERROR_ILL_RVLC_P
  MP4ERROR_SLICE_BUFFER_OVERRUN (from read_bits_reverse)
  MP4ERROR_MISSING_MARKERBIT_RVLC_I or MP4ERROR_MISSING_MARKERBIT_RVLC_P

SIDE EFFECTS:
  None.


===========================================================================*/
MP4_ERROR_TYPE mp4huffman_decode_RVLC_reverse(
  unsigned char fIntra,                /* 1 for IVOP, 0 for PVOP
                                  * based on MB_TYPE in MCBPC*/
  mp4_slice_type      *pSlice,   /* Pointer to slice structure */
  unsigned char        *pLast,         /* TRUE if last */
  unsigned long int *pLength                /* Length of decoded data */
  );

/* <EJECT> */
/*===========================================================================

FUNCTION:
  InitSliceDecoding

DESCRIPTION:
  Prepares the PAL to decode an MPEG-4 bitstream slice
  
RETURN VALUE:
  MP4ERROR_SUCCESS     - if success
  Any other Error Code - if failure
===========================================================================*/
MP4_ERROR_TYPE InitSliceDecoding(unsigned short *pszMpegHdr, mp4_slice_type *pSlice, DecodeStatsType  *pCurrentDecodeStats);
/* <EJECT> */
/*===========================================================================

FUNCTION:
  EndSliceDecoding

DESCRIPTION:
  Signals the PAL to end the packing of an MPEG-4 bitstream slice
  
RETURN VALUE:
  None.
===========================================================================*/
void EndSliceDecoding(unsigned long int uiMBCount, unsigned long int uiTotalMB, unsigned long int uiFirstMB, 
                               MP4BufType* pMP4Buf, signed short NextInFrameId,
                               signed short prevRefFrameIndex, signed short prevPrevRefFrameIndex,
                               VOP_type *pVOP, 
                               unsigned char bCBP[MAX_MB], unsigned char fScalability, VDL_Video_Type VideoDecoderType, DecodeStatsType **ppCurrentDecodeStats);
/* <EJECT> */
/*===========================================================================

FUNCTION:
  StartMBdecoding

DESCRIPTION:
  Prepares the PAL to decode a macroblock
  
RETURN VALUE:
  None.
===========================================================================*/
void StartMBDecoding(
#if defined (FEATURE_QDSP_RTOS) && defined (FEATURE_MPEG4_COMPACT_PACKET)
					 unsigned char cMBType
#endif
					 );
#ifdef FEATURE_MPEG4_B_FRAMES
/* <EJECT> */
/*===========================================================================

FUNCTION:
  StartMBdecoding

DESCRIPTION:
  Prepares the PAL to decode a macroblock
  
RETURN VALUE:
  None.
===========================================================================*/
void StartMBDecoding(unsigned char cMBType, unsigned char bCBP);
#endif

/* <EJECT> */
/*===========================================================================

FUNCTION:
  EndMBDecoding

DESCRIPTION:
  Signals the PAL to end the decoding of a macroblock
  
RETURN VALUE:
  FALSE if no more MBs can be packed into the current packet buffer
  TRUE otherwise.
  (We should eventually complete abstract knowledge of the packet buffer from
  the Transform Layer (TL)).
===========================================================================*/
unsigned char EndMBDecoding(unsigned long int *pulMBNum, mp4_slice_type *pBitstream, unsigned long int NextMBnum, unsigned char bNotCoded, VOP_type *pVOP );
/* <EJECT> */
/*===========================================================================

FUNCTION:
  AbortMBDecoding

DESCRIPTION:
  Signals the PAL that the current MB decode was aborted
  
RETURN VALUE:
  None.
===========================================================================*/
void AbortMBDecoding();

/* <EJECT> */
/*===========================================================================

FUNCTION:
  SaveRVLCPosition

DESCRIPTION:
  Signals the PAL that the current MB decode was aborted
  
RETURN VALUE:
  None.
===========================================================================*/
void SaveRVLCPosition();

/* <EJECT> */
/*===========================================================================

FUNCTION:
  StartBlockDecoding

DESCRIPTION:
  Prepares the PAL to start decoding a block
  
RETURN VALUE:
  None.
===========================================================================*/
void StartBlockDecoding();
/* <EJECT> */
/*===========================================================================

FUNCTION:
  EndBlockDecoding

DESCRIPTION:
  Signals the PAL that a block has finished decoding
  
RETURN VALUE:
  None.
===========================================================================*/
void EndBlockDecoding();
/* <EJECT> */
/*===========================================================================

FUNCTION:
  PackNotCodedMB

DESCRIPTION:
  Signals the PAL that a not-coded MB was encountered.  The PAL packs the data
  appropriately
  
RETURN VALUE:
  None.
===========================================================================*/
void PackNotCodedMB();
/* <EJECT> */
/*===========================================================================

FUNCTION:
  AddResiduals

DESCRIPTION:
  Packs a run-level pair
  
RETURN VALUE:
  None.
===========================================================================*/
void AddResiduals( unsigned short run, unsigned short level );
/* <EJECT> */
/*===========================================================================

FUNCTION:
  Pack MBInfo

DESCRIPTION:
  Packs MB-specific info for a single macroblock
  
RETURN VALUE:
  None.
===========================================================================*/
void PackMBInfo(unsigned char fACPred, /*unsigned char fReverse,*/
               unsigned char cMBType, unsigned char fRoundingType, unsigned char fNotCoded, unsigned char fStartSlice, signed long int cMBNum, unsigned char bCBP[MAX_MB]);

#ifdef FEATURE_MPEG4_B_FRAMES
/* <EJECT> */
/*===========================================================================

FUNCTION:
  Pack MBInfo

DESCRIPTION:
  Packs MB-specific info for a single macroblock
  
RETURN VALUE:
  None.
===========================================================================*/
void PackMBInfoBvop(unsigned char cMBType, unsigned char bCBP);
#endif

/* <EJECT> */
/*===========================================================================

FUNCTION:
 PackCBP

DESCRIPTION:
  Packs Coded Block Parameter for a single macroblock
  
RETURN VALUE:
  None.
===========================================================================*/
#if defined (FEATURE_QDSP_RTOS) && defined (FEATURE_MPEG4_COMPACT_PACKET)
void PackCBP(signed long int cMBNum, unsigned char bCBP[MAX_MB]);
#endif /* defined (FEATURE_QDSP_RTOS) && defined (FEATURE_MPEG4_COMPACT_PACKET) */
/* <EJECT> */
/*===========================================================================

FUNCTION:
  Pack MBInfo

DESCRIPTION:
  Packs source VOP buffer index for a macroblock
  
RETURN VALUE:
  None.
===========================================================================*/
void PackSourceVOP();
/* <EJECT> */
/*===========================================================================

FUNCTION:
  PackMBType

DESCRIPTION:
  Packs MB-type info

RETURN VALUE:
  None.
===========================================================================*/
void PackMBType(unsigned char cMBType);
/* <EJECT> */
/*===========================================================================

FUNCTION:
  PackACPred

DESCRIPTION:
  Packs ACPred info for a single macroblock
  
RETURN VALUE:
  None.
===========================================================================*/
void PackACPred(unsigned char fACPred);
/* <EJECT> */
/*===========================================================================

FUNCTION:
  PackRoundingType

DESCRIPTION:
  Packs rounding type info for a single macroblock
  
RETURN VALUE:
  None.
===========================================================================*/
void PackRoundingType(unsigned char RoundingType);
/* <EJECT> */
/*===========================================================================

FUNCTION:
  PackNotCodedBit

DESCRIPTION:
  Packs the NOT_CODED bit for a single macroblock
  
RETURN VALUE:
  None.
===========================================================================*/
void PackNotCodedBit(unsigned char fNotCoded);
/* <EJECT> */
/*===========================================================================

FUNCTION:
  PackStartSliceBit

DESCRIPTION:
  Packs the start slice bit for the MB.  (True if MB is the start of a slice)
  
RETURN VALUE:
  None.
===========================================================================*/
void PackStartSliceBit(unsigned char fStartSlice);
/* <EJECT> */
/*===========================================================================

FUNCTION:
  PackMBNumber

DESCRIPTION:
  Packs current MB number of the frame

RETURN VALUE:
  None.
===========================================================================*/
void PackMBNumber(unsigned long int cMBNum);
/* <EJECT> */
/*===========================================================================

FUNCTION:
  Pack DCScaler

DESCRIPTION:
  Packs DCScaler values for a single macroblock
  
RETURN VALUE:
  None.
===========================================================================*/
void PackDCScaler(unsigned short uiDCScalerLum, unsigned short uiDCScalerChr);

/* <EJECT> */
/*===========================================================================

FUNCTION:
  PackVOPQuant

DESCRIPTION:
  Packs VOP Quantization Scale data
  
RETURN VALUE:
  None.
===========================================================================*/
void PackVOPQuant(unsigned short uiVOPQuant, unsigned char cMBType);

#ifdef FEATURE_MPEG4_B_FRAMES
/* <EJECT> */
/*===========================================================================

FUNCTION:
  PackVOPQuantBvop

DESCRIPTION:
  Packs VOP Quantization Scale data
  
RETURN VALUE:
  None.
===========================================================================*/
void PackVOPQuantBvop(unsigned short uiVOPQuant, unsigned char cMBType);
#endif

/* <EJECT> */
/*===========================================================================

FUNCTION:
  PackMotionVectors

DESCRIPTION:
  Packs the motion vectors for a single MB
  
RETURN VALUE:
  None.
===========================================================================*/
void PackMotionVectors(MotionVectorType MV, unsigned char cMBType);

#ifdef FEATURE_MPEG4_B_FRAMES
/* <EJECT> */
/*===========================================================================

FUNCTION:
  PackMotionVectors

DESCRIPTION:
  Packs the motion vectors for a single MB
  
RETURN VALUE:
  None.
===========================================================================*/
void PackMotionVectorsBvop(MotionVectorType FMV, MotionVectorType BMV, unsigned char cMBType, unsigned char bCBP); 
#endif

protected:
void init_member_variables();
  /* <EJECT> */
/*===========================================================================

FUNCTION:
  InitSliceBuf

DESCRIPTION:
  Allocates a new slice buffer if necessary, or reuses one for a new slice.
  
RETURN VALUE:
  MP4ERROR_SUCCESS     - if success
  Any other Error Code - if failure
===========================================================================*/
MP4_ERROR_TYPE InitSliceBuf(unsigned short *pszMpegHdr, mp4_slice_type *pSlice, DecodeStatsType *pCurrentDecodeStats);

#ifdef FEATURE_QDSP_RTOS

/*===========================================================================
FUNCTION:
  mp4_unpack_DCT_coef

DESCRIPTION:
  This function takes the nPairs of run/levels and writes them out as DCT
  coefficients.  It is assumed that this would be done for space efficiency
  when there are more than 32 pairs of coef, although this need not be the case.  

  See MSM process-DSP interface specification.

INPUT/OUTPUT PARAMETERS:
  pDecodeBuffer
  nPairs

RETURN VALUE:
  None.

SIDE EFFECTS:
  None.

============================================================================*/
void mp4_unpack_DCT_coef(void *pDecodeBuffer, unsigned char nPairs); 
#endif /* FEATURE_QDSP_RTOS */

/* <EJECT> */
/*===========================================================================

FUNCTION:
  MBSubPacketsToDSPSliceBuffer

DESCRIPTION:
  Send the slice buffer to the DSP
  
RETURN VALUE:
  None.
===========================================================================*/
void MBSubPacketsToDSPSliceBuffer(unsigned long int uiMBCount, unsigned long int uiFirstMB, 
                                           MP4BufType* pMP4Buf, signed short NextInFrameId, 
                                           signed short prevRefFrameIndex, signed short prevPrevRefFrameIndex,
                                           VOP_type *pVOP, unsigned char bCBP[MAX_MB], 
                                           unsigned char fScalability, VDL_Video_Type VideoDecoderType,
                                           DecodeStatsType  **ppCurrentDecodeStats);

/* <EJECT> */
/*===========================================================================

FUNCTION:
  mp4huffman_decode_VLC

DESCRIPTION:
  Huffman decoding of VLC (DCT coefficients)
  Search the entry for the VLC item
  If success
  1. Fill tcoeff (but not the sign)
  2. return CodeLength (positive number):
  (Does not actually move the SlicePointer)
  on failure, return 0 if escape, -1 for error.

INPUT/OUTPUT PARAMETERS:
  pSlice
  Intra
  pLast
  pCodeLength

RETURN VALUE
  MP4ERROR_SUCCESS     - if success
  Any other Error Code - if failure

SIDE EFFECTS:
  None.
===========================================================================*/
MP4_ERROR_TYPE mp4huffman_decode_VLC(
  mp4_slice_type      *pSlice,         /* Pointer to slice structure */
  unsigned char Intra,                         /* 1 if IVOP, 0 if PVOP */
  signed char *pLast,                  /* TRUE if this is last level */
  unsigned long int *pCodeLength,                 /* length of the decoded Code */
  unsigned long int *pNextCode                    
);
/* <EJECT> */
/*===========================================================================

FUNCTION:
  GetSliceBuf

DESCRIPTION:
  Allocate a new slice buffer
  
RETURN VALUE:
  TRUE if successful, FALSE otherwise
===========================================================================*/
unsigned char GetSliceBuf();

#ifdef FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A
/* <EJECT> */
/*===========================================================================

FUNCTION:
  set_frame_header_addresses

DESCRIPTION:
  This function writes the MPEG4 frame header to the QDSP

INPUT/OUTPUT PARAMETERS:
  pData

RETURN VALUE:
  None.

SIDE EFFECTS:
  None.
===========================================================================*/
void set_frame_header_addresses( 					
  VOP_type *pVOP,  
  MP4BufType* pMP4Buf, 
  signed short NextInFrameId,
  signed short prevRefFrameIndex, 
  signed short prevPrevRefFrameIndex
);
/* <EJECT> */
/*===========================================================================

FUNCTION:
  set_frame_header

DESCRIPTION:
  This function writes the MPEG4 frame header to the QDSP

INPUT/OUTPUT PARAMETERS:
  pData

RETURN VALUE:
  None.

SIDE EFFECTS:
  None.
===========================================================================*/
void set_frame_header( 					
  VOP_type *pVOP,  
  MP4BufType* pMP4Buf, 
  signed short NextInFrameId, 
  signed short prevRefFrameIndex, 
  signed short prevPrevRefFrameIndex
);
void set_LCDB_params();
#endif /* FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A */

};

/* <EJECT> */
/*===========================================================================

FUNCTION:
  EndBlockDecoding

DESCRIPTION:
  Signals the PAL that a block has finished decoding
  
RETURN VALUE:
  None.
===========================================================================*/
inline void MP4_PAL::EndBlockDecoding()
{
#if defined (FEATURE_QDSP_RTOS) && defined (FEATURE_MPEG4_COMPACT_PACKET)
    m_pCount->count = (unsigned char) (m_pRVLC - (rvlc_run_level_type*)m_pCount);
  if (m_pCount->count > MAX_RUN_LEVEL_PAIRS) {
    mp4_unpack_DCT_coef(m_pCount, m_pCount->count);
    *(unsigned short*)m_pCount = UNPACKED_RUN_LEVEL_ID;
    m_pRVLC = ((rvlc_run_level_type*)(m_pCount+1)) + MAX_RUN_LEVEL_PAIRS;
  }
#else
  *m_pRVLCCount = (unsigned short) (m_pRVLC - (rvlc_run_level_type*)(m_pRVLCCount + 1));
#if defined (FEATURE_QDSP_RTOS) && !defined (PLATFORM_LTK)
  if (*m_pRVLCCount > MAX_RUN_LEVEL_PAIRS) {
    mp4_unpack_DCT_coef((m_pRVLCCount+1), *m_pRVLCCount);
    *m_pRVLCCount = UNPACKED_RUN_LEVEL_ID;
    m_pRVLC = ((rvlc_run_level_type*)(m_pRVLCCount+1)) + MAX_RUN_LEVEL_PAIRS;
  }
#endif /* FEATURE_QDSP_RTOS && !defined (PLATFORM_LTK) */
#endif /* defined (FEATURE_QDSP_RTOS) && defined (FEATURE_MPEG4_COMPACT_PACKET) */

/* Make sure that we have not exceeded the maximum buffer size.
  * This should have been caught below (outside of the loop)
  */
  ASSERT( m_pRVLC < m_pMaxRVLC );  
}

/* <EJECT> */
/*===========================================================================

FUNCTION:
  StartBlockDecoding

DESCRIPTION:
  Prepares the PAL to start decoding a block
  
RETURN VALUE:
  None.
===========================================================================*/
inline void MP4_PAL::StartBlockDecoding()
{
#if defined (FEATURE_QDSP_RTOS) && defined (FEATURE_MPEG4_COMPACT_PACKET)
  m_pCount = (rvlc_count_type *)m_pRVLC;
#else
  m_pRVLCCount = (unsigned short *)m_pRVLC;
  m_pRVLC = (rvlc_run_level_type*)(m_pRVLCCount + 1);
#endif /* defined (FEATURE_QDSP_RTOS) && defined (FEATURE_MPEG4_COMPACT_PACKET) */
}

/* <EJECT> */
/*===========================================================================

FUNCTION:
  AbortMBDecoding

DESCRIPTION:
  Signals the PAL that the current MB decode was aborted
  
RETURN VALUE:
  None.
===========================================================================*/
inline void MP4_PAL::AbortMBDecoding()
{
  m_pRVLC = (rvlc_run_level_type *)m_pRVLCSave;
}

/* <EJECT> */
/*===========================================================================

FUNCTION:
  SaveRVLCPosition

DESCRIPTION:
  Signals the PAL that the current MB decode was aborted
  
RETURN VALUE:
  None.
===========================================================================*/
inline void MP4_PAL::SaveRVLCPosition()
{
  m_pRVLCSave = m_pRVLC;
}

/* <EJECT> */
/*===========================================================================

FUNCTION:
  EndMBDecoding

DESCRIPTION:
  Signals the PAL to end the decoding of a macroblock
  
RETURN VALUE:
  FALSE if no more MBs can be packed into the current packet buffer
  TRUE otherwise.
  (We should eventually complete abstract knowledge of the packet buffer from
  the Transform Layer (TL)).
===========================================================================*/
inline unsigned char MP4_PAL::EndMBDecoding(unsigned long int *pulMBNum, mp4_slice_type *pBitstream, unsigned long int NextMBnum, unsigned char bNotCoded, VOP_type *pVOP )
{
  //QTV_USE_ARG2(pBitstream, pVOP);
  if(bNotCoded)
  {
    *((unsigned short*)m_pHdr + MP4_NOT_CODED_PACKET_SIZE) = MP4_MB_END_MARKER;
    m_pRVLC = (rvlc_run_level_type *) ((unsigned short *) m_pHdr + MP4_NOT_CODED_PACKET_SIZE + 1);
  }
  else /* coded */
  {
    *((unsigned short*)m_pRVLC) = MP4_MB_END_MARKER;
    m_pRVLC = (rvlc_run_level_type*)((unsigned short*)m_pRVLC + 1);
  }
  m_pHdr->NumWords = (unsigned short*) m_pRVLC - (unsigned short *) m_pHdr;

  /* See if we have enough room for another macroblock */
  if ( m_pRVLC > m_pGuardRVLC && *pulMBNum < NextMBnum-1 )
  {
    QTV_MSG_PRIO3(QTVDIAG_GENERAL, QTVDIAG_PRIO_ERROR, "CMB P-VOP overrun guard: %d bytes %d %d",
              (unsigned long int)m_pRVLC - (unsigned long int)m_pGuardRVLC, *pulMBNum, NextMBnum );
    /* this happened *after* last mb completed*/
    *pulMBNum = *pulMBNum + 1;
    /* to tell others we still have more to do */
    m_fPartialSliceDecoded = TRUE;
#ifdef FEATURE_MP4_H263_ANNEX_I
    if (pVOP->fAdvancedIntraCodingMode)
    {
	  signed long int ErrCode = MP4ERROR_SUCCESS;
      /* Check to see if a GOB resync marker follows */
      ErrCode = mp4bitstream_gob_resync_marker(pBitstream);
      if ( ErrCode == MP4ERROR_SUCCESS )
      {
        /* A GOB resync marker follows, so this is the end of the GOB. Set the 
        ** NextNBnum pointer to point to the start of the new GOB.
        */
        pBitstream->NextMBnum = *pulMBNum;
        /* The slice is no longer partially decoded, so set it to FALSE */ 
        m_fPartialSliceDecoded = FALSE;
      }
      else
      {
        ErrCode = MP4ERROR_SUCCESS;   /* reset the error flag */
      }
    }
#endif /* FEATURE_MP4_H263_ANNEX_I */
    /* so that we break a long slice into smaller chunks */
    return FALSE;
  }  /* check for overrun guard */
  return TRUE;
}
/* <EJECT> */
/*===========================================================================

FUNCTION:
  StartMBdecoding

DESCRIPTION:
  Prepares the PAL to decode a macroblock
  
RETURN VALUE:
  None.
===========================================================================*/
inline void MP4_PAL::StartMBDecoding(
#if defined (FEATURE_QDSP_RTOS) && defined (FEATURE_MPEG4_COMPACT_PACKET)
									 unsigned char cMBType
#endif
									 )
{
  m_pRVLCSave = m_pRVLC;
  m_pHdr           = (MP4QDSPMBHeaderType *) m_pRVLC;
#if defined (FEATURE_QDSP_RTOS) && defined (FEATURE_MPEG4_COMPACT_PACKET)
  unsigned char nHeaderSize = 0;
    if (cMBType == MODE_INTRA || cMBType == MODE_INTRAQ)
      nHeaderSize = sizeof(MP4QDSPIntraMBHeaderType);
    else if (cMBType == MODE_INTER || cMBType == MODE_INTERQ)
      nHeaderSize = sizeof(MP4QDSPInter1MVMBHeaderType);
    else if (cMBType == MODE_INTER4V || cMBType == MODE_INTER4VQ)
      nHeaderSize = sizeof(MP4QDSPInter4MVMBHeaderType);

    m_pRVLC = (rvlc_run_level_type*) ((unsigned char*)m_pHdr + nHeaderSize);
#else
    m_pRVLC = (rvlc_run_level_type*) (m_pHdr + 1);
#endif /* defined (FEATURE_QDSP_RTOS) && defined (FEATURE_MPEG4_COMPACT_PACKET) */
#ifdef FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A
#ifdef FEATURE_MP4_H263_PROFILE3
  if( m_fShortVideoHeader 
#if ( defined(FEATURE_VIDEO_AUDIO_DUBBING) || defined(FEATURE_MP4_TRANSCODER) )
      && !m_fProfile3Disable
#endif /* #ifdef FEATURE_VIDEO_AUDIO_DUBBING || FEATURE_MP4_TRANSCODER */
    )
  {
	  ((MP4QDSPMBHeaderType*)m_pHdr)->PacketId = H263P3_MB_PACKET_TYPE;
  }
  else
#endif
    ((MP4QDSPMBHeaderType*)m_pHdr)->PacketId = MP4_MB_PACKET_TYPE;

#endif /* FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A */
    ((MP4QDSPMBHeaderType*)m_pHdr)->MBInfo = 0;
}
#ifdef FEATURE_MPEG4_B_FRAMES
/* <EJECT> */
/*===========================================================================

FUNCTION:
  StartMBdecoding

DESCRIPTION:
  Prepares the PAL to decode a macroblock
  
RETURN VALUE:
  None.
===========================================================================*/
inline void MP4_PAL::StartMBDecoding(unsigned char cMBType, unsigned char bCBP)
{
  m_pRVLCSave = m_pRVLC;
  m_pHdr           = (MP4QDSPMBHeaderType *) m_pRVLC;
  unsigned char nHeaderSize = 0;
  if (cMBType == FORWARD || cMBType == BACKWARD) {
      m_pRVLC = (rvlc_run_level_type*) (m_pHdr + 1);
#ifdef FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A
  ((MP4QDSPMBHeaderType*)m_pHdr)->PacketId = MP4_MB_PACKET_TYPE;
#endif /* FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A */
  }
  else 
  {
   if (bCBP) {
    if (cMBType == BIDIRECTIONAL || cMBType == DIRECT)
      nHeaderSize = sizeof(MP4QDSPB1MVQuantMBHeaderType);  
    else if (cMBType == DIRECT4MV)
      nHeaderSize = sizeof(MP4QDSPB4MVQuantMBHeaderType);           
   } else {
    if (cMBType == BIDIRECTIONAL || cMBType == DIRECT)
      nHeaderSize = sizeof(MP4QDSPB1MVMBHeaderType);  
    else if (cMBType == DIRECT4MV)
      nHeaderSize = sizeof(MP4QDSPB4MVMBHeaderType);           
   }
   m_pRVLC = (rvlc_run_level_type*) ((unsigned char*)m_pHdr + nHeaderSize);
#ifdef FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A
   ((MP4QDSPMBHeaderType*)m_pHdr)->PacketId = MP4_B_MB_PACKET_TYPE;
#endif /* FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A */
  }
}
#endif
/* <EJECT> */
/*===========================================================================

FUNCTION:
  AddResiduals

DESCRIPTION:
  Packs a run-level pair
  
RETURN VALUE:
  None.
===========================================================================*/
inline void MP4_PAL::AddResiduals( unsigned short run, unsigned short level )
{
  m_pRVLC->run = run;
  m_pRVLC->level = level;
  m_pRVLC++;
}

/* <EJECT> */
/*===========================================================================

FUNCTION:
  Pack MBInfo

DESCRIPTION:
  Packs MB-specific info for a single macroblock
  
RETURN VALUE:
  None.
===========================================================================*/
inline void MP4_PAL::PackMBInfo(unsigned char fACPred, /*unsigned char fReverse,*/
               unsigned char cMBType, unsigned char fRoundingType, unsigned char fNotCoded, unsigned char fStartSlice, signed long int cMBNum, unsigned char bCBP[MAX_MB])
  
{
  /* build the MB-info word according to p. 4-25 of Interface Spec
  ** (80-V2983-1; RevA */

  ((MP4QDSPMBHeaderType*)m_pHdr)->MBInfo |=         
#if defined (FEATURE_QDSP_RTOS) && defined (FEATURE_MPEG4_COMPACT_PACKET)
      (bCBP[cMBNum] << 10) |
#endif /* defined (FEATURE_QDSP_RTOS) && defined (FEATURE_MPEG4_COMPACT_PACKET) */
  ((fACPred != 0) << 7) |
  ((cMBType == MODE_INTER4V || cMBType == MODE_INTER4VQ) << 5) |
  ((cMBType == MODE_INTERQ || cMBType == MODE_INTER ||
    cMBType == MODE_INTER4V || cMBType == MODE_INTER4VQ) << 4) |
  ((fRoundingType != 0) << 3) |
  ((fNotCoded != 0) << 2) |
  ((fStartSlice == TRUE ) << 1) |
  (cMBNum == 0);
  bCBP[0]=bCBP[0]; //Used to remove the warning
}
#ifdef FEATURE_MPEG4_B_FRAMES
/* <EJECT> */
/*===========================================================================

FUNCTION:
  Pack MBInfo

DESCRIPTION:
  Packs MB-specific info for a single macroblock
  
RETURN VALUE:
  None.
===========================================================================*/
inline void MP4_PAL::PackMBInfoBvop(unsigned char cMBType, unsigned char bCBP)  
{
  // since B frame has no ACDC prediction don bother about 
  // sending start of slice flag to the DSP
  if (cMBType == FORWARD) {
      ((MP4QDSPMBHeaderType*)m_pHdr)->MBInfo =         
          (FORWARD_REF_VOP_INDEX << 10) |
          (1 << 4); // inter MB
  } else if (cMBType == BACKWARD) {
      ((MP4QDSPMBHeaderType*)m_pHdr)->MBInfo =         
          (BACKWARD_REF_VOP_INDEX << 10) |
          (1 << 4); // inter MB
  } else {
      ((MP4QDSPMBHeaderType*)m_pHdr)->MBInfo =         
          (FORWARD_REF_VOP_INDEX << 12) |
          (BACKWARD_REF_VOP_INDEX << 10) |
          ((cMBType == DIRECT4MV) << 5) |
          ((bCBP ? 0:1) << 2);
  }
}
#endif
/* <EJECT> */
/*===========================================================================

FUNCTION:
 PackCBP

DESCRIPTION:
  Packs Coded Block Parameter for a single macroblock
  
RETURN VALUE:
  None.
===========================================================================*/
#if defined (FEATURE_QDSP_RTOS) && defined (FEATURE_MPEG4_COMPACT_PACKET)
inline void MP4_PAL::PackCBP(signed long int cMBNum, unsigned char bCBP[MAX_MB])
{
  ((MP4QDSPMBHeaderType*)m_pHdr)->MBInfo |=  
      (bCBP[cMBNum] << 10);
}
#endif /* defined (FEATURE_QDSP_RTOS) && defined (FEATURE_MPEG4_COMPACT_PACKET) */
/* <EJECT> */
/*===========================================================================

FUNCTION:
  PackSourceVOP

DESCRIPTION:
  Packs source VOP buffer index for a macroblock
  
RETURN VALUE:
  None.
===========================================================================*/
inline void MP4_PAL::PackSourceVOP()
{
  ((MP4QDSPMBHeaderType*)m_pHdr)->MBInfo |=         
      (BACKWARD_REF_VOP_INDEX << 10);
}
/* <EJECT> */
/*===========================================================================

FUNCTION:
  PackMBType

DESCRIPTION:
  Packs MB-type info for a single macroblock
  
RETURN VALUE:
  None.
===========================================================================*/
inline void MP4_PAL::PackMBType(unsigned char cMBType)
{
  ((MP4QDSPMBHeaderType*)m_pHdr)->MBInfo |=    
  ((cMBType == MODE_INTER4V || cMBType == MODE_INTER4VQ) << 5) |
  ((cMBType == MODE_INTERQ || cMBType == MODE_INTER ||
    cMBType == MODE_INTER4V || cMBType == MODE_INTER4VQ) << 4);
}
/* <EJECT> */
/*===========================================================================

FUNCTION:
  PackACPred

DESCRIPTION:
  Packs MB-type info for a single macroblock
  
RETURN VALUE:
  None.
===========================================================================*/
inline void MP4_PAL::PackACPred(unsigned char fACPred)
{
  ((MP4QDSPMBHeaderType*)m_pHdr)->MBInfo |=    
  ((fACPred != 0) << 7);
}
/* <EJECT> */
/*===========================================================================

FUNCTION:
  PackRoundingType

DESCRIPTION:
  Packs rounding-type bit for a single macroblock
  
RETURN VALUE:
  None.
===========================================================================*/
inline void MP4_PAL::PackRoundingType(unsigned char fRoundingType)
{
  ((MP4QDSPMBHeaderType*)m_pHdr)->MBInfo |=    
    ((fRoundingType != 0) << 3);
}
/* <EJECT> */
/*===========================================================================

FUNCTION:
  PackNotCodedBit

DESCRIPTION:
  Packs the NOT_CODED bit for a single macroblock
  
RETURN VALUE:
  None.
===========================================================================*/
inline void MP4_PAL::PackNotCodedBit(unsigned char fNotCoded)
{
  ((MP4QDSPMBHeaderType*)m_pHdr)->MBInfo |=    
  ((fNotCoded != 0) << 2);
}
/* <EJECT> */
/*===========================================================================

FUNCTION:
  PackStartSliceBit

DESCRIPTION:
  Packs the start slice bit for the MB.  (True if MB is the start of a slice)
  
RETURN VALUE:
  None.
===========================================================================*/
inline void MP4_PAL::PackStartSliceBit(unsigned char fStartSlice)
{
  ((MP4QDSPMBHeaderType*)m_pHdr)->MBInfo |=    
   (fStartSlice << 1);
}
/* <EJECT> */
/*===========================================================================

FUNCTION:
  PackMBNumber

DESCRIPTION:
  Packs current MB number of the frame
  
RETURN VALUE:
  None.
===========================================================================*/
inline void MP4_PAL::PackMBNumber(unsigned long int cMBNum)
{
  ((MP4QDSPMBHeaderType*)m_pHdr)->MBInfo |=    
    (cMBNum == 0);
}

/* <EJECT> */
/*===========================================================================

FUNCTION:
  Pack DCScaler

DESCRIPTION:
  Packs DCScaler values for a single macroblock
  
RETURN VALUE:
  None.
===========================================================================*/
inline void MP4_PAL::PackDCScaler(unsigned short uiDCScalerLum, unsigned short uiDCScalerChr)
{
  ((MP4QDSPMBHeaderType*)m_pHdr)->DCScalerLum = uiDCScalerLum;
  ((MP4QDSPMBHeaderType*)m_pHdr)->DCScalerChr = uiDCScalerChr;
}

/* <EJECT> */
/*===========================================================================

FUNCTION:
  PackVOPQuant

DESCRIPTION:
  Packs VOP Quantization Scale data
  
RETURN VALUE:
  None.
===========================================================================*/
inline void MP4_PAL::PackVOPQuant(unsigned short uiVOPQuant, unsigned char cMBType)
{
#if defined (FEATURE_QDSP_RTOS) && defined (FEATURE_MPEG4_COMPACT_PACKET)
  if( (cMBType == MODE_INTRA) || (cMBType == MODE_INTRAQ) ) 
    ((MP4QDSPIntraMBHeaderType*) m_pHdr)->VOPQuantScale = uiVOPQuant;
  if( (cMBType == MODE_INTER) || (cMBType == MODE_INTERQ) ) 
    ((MP4QDSPInter1MVMBHeaderType*) m_pHdr)->VOPQuantScale = uiVOPQuant;
  else if ( (cMBType == MODE_INTER4V) || (cMBType == MODE_INTER4VQ) )
    ((MP4QDSPInter4MVMBHeaderType*) m_pHdr)->VOPQuantScale = uiVOPQuant;
#else
  cMBType = cMBType; // remove compiler warning
  ((MP4QDSPMBHeaderType*)m_pHdr)->VOPQuantScale = uiVOPQuant;
#endif /* defined (FEATURE_QDSP_RTOS) && defined (FEATURE_MPEG4_COMPACT_PACKET) */
}

#ifdef FEATURE_MPEG4_B_FRAMES
/* <EJECT> */
/*===========================================================================

FUNCTION:
  PackVOPQuantBvop

DESCRIPTION:
  Packs VOP Quantization Scale data for B VOPs
  
RETURN VALUE:
  None.
===========================================================================*/
inline void MP4_PAL::PackVOPQuantBvop(unsigned short uiVOPQuant, unsigned char cMBType)
{
  if ((cMBType == DIRECT) || (cMBType == BIDIRECTIONAL))
    ((MP4QDSPB1MVQuantMBHeaderType*) m_pHdr)->VOPQuantScale = uiVOPQuant;
  else if (cMBType == DIRECT4MV)
    ((MP4QDSPB4MVQuantMBHeaderType*) m_pHdr)->VOPQuantScale = uiVOPQuant;
  else
    // this part need to change for compact pkt case
    ((MP4QDSPMBHeaderType*)m_pHdr)->VOPQuantScale = uiVOPQuant;
}
#endif
/* <EJECT> */
/*===========================================================================

FUNCTION:
  PackMotionVectors

DESCRIPTION:
  Packs the motion vectors for a single MB
  
RETURN VALUE:
  None.
===========================================================================*/
inline void MP4_PAL::PackMotionVectors(MotionVectorType MV, unsigned char cMBType)
{
#if defined (FEATURE_QDSP_RTOS) && defined (FEATURE_MPEG4_COMPACT_PACKET)
  if( (cMBType == MODE_INTER) || (cMBType == MODE_INTERQ) ) 
    memcpy( ((MP4QDSPInter1MVMBHeaderType*) m_pHdr)->MV, MV, sizeof(OneMotionVectorType) );
  else if ( (cMBType == MODE_INTER4V) || (cMBType == MODE_INTER4VQ) )
    memcpy( ((MP4QDSPInter4MVMBHeaderType*) m_pHdr)->MV, MV, sizeof(MotionVectorType) );
#else
  cMBType = cMBType;//Used to remove the warning
  memcpy( m_pHdr->MV, MV, sizeof(MotionVectorType) );
#endif /* defined (FEATURE_QDSP_RTOS) && defined (FEATURE_MPEG4_COMPACT_PACKET) */
}
#ifdef FEATURE_MPEG4_B_FRAMES
/* <EJECT> */
/*===========================================================================

FUNCTION:
  PackMotionVectorsBvop

DESCRIPTION:
  Packs the motion vectors for a single MB
  
RETURN VALUE:
  None.
===========================================================================*/
inline void MP4_PAL::PackMotionVectorsBvop(MotionVectorType FMV, MotionVectorType BMV, unsigned char cMBType, unsigned char bCBP) { 
  unsigned char blockNum;

  if (cMBType == FORWARD)
      memcpy( m_pHdr->MV, FMV, sizeof(MotionVectorType) );
  else if (cMBType == BACKWARD)
      memcpy( m_pHdr->MV, BMV, sizeof(MotionVectorType) );
  else if (bCBP) {
      if ((cMBType == DIRECT) || (cMBType == BIDIRECTIONAL)) {
          ((MP4QDSPB1MVQuantMBHeaderType*) m_pHdr)->MV.Fx = FMV[0].x;
          ((MP4QDSPB1MVQuantMBHeaderType*) m_pHdr)->MV.Fy = FMV[0].y;
          ((MP4QDSPB1MVQuantMBHeaderType*) m_pHdr)->MV.Bx = BMV[0].x;
          ((MP4QDSPB1MVQuantMBHeaderType*) m_pHdr)->MV.By = BMV[0].y;
      }
      else {
          for (blockNum=0; blockNum<NYBLOCKS;blockNum++) {
              ((MP4QDSPB4MVQuantMBHeaderType*) m_pHdr)->MV[blockNum].Fx = FMV[blockNum].x;
              ((MP4QDSPB4MVQuantMBHeaderType*) m_pHdr)->MV[blockNum].Fy = FMV[blockNum].y;
              ((MP4QDSPB4MVQuantMBHeaderType*) m_pHdr)->MV[blockNum].Bx = BMV[blockNum].x;
              ((MP4QDSPB4MVQuantMBHeaderType*) m_pHdr)->MV[blockNum].By = BMV[blockNum].y; 
          }
      }
  } else {
      if ((cMBType == DIRECT) || (cMBType == BIDIRECTIONAL)) {
          ((MP4QDSPB1MVMBHeaderType*) m_pHdr)->MV.Fx = FMV[0].x;
          ((MP4QDSPB1MVMBHeaderType*) m_pHdr)->MV.Fy = FMV[0].y;
          ((MP4QDSPB1MVMBHeaderType*) m_pHdr)->MV.Bx = BMV[0].x;
          ((MP4QDSPB1MVMBHeaderType*) m_pHdr)->MV.By = BMV[0].y;
      }
      else {
          for (blockNum=0; blockNum<NYBLOCKS;blockNum++) {
              ((MP4QDSPB4MVMBHeaderType*) m_pHdr)->MV[blockNum].Fx = FMV[blockNum].x;
              ((MP4QDSPB4MVMBHeaderType*) m_pHdr)->MV[blockNum].Fy = FMV[blockNum].y;
              ((MP4QDSPB4MVMBHeaderType*) m_pHdr)->MV[blockNum].Bx = BMV[blockNum].x;
              ((MP4QDSPB4MVMBHeaderType*) m_pHdr)->MV[blockNum].By = BMV[blockNum].y; 
          }
      }
  }
}
#endif

#endif /* MP4_PAL_H */

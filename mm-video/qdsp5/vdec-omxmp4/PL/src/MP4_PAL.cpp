/*=============================================================================
      MP4_PAL.cpp

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

$Header: //source/qcom/qct/multimedia2/Video/Dec/MPEG4Vdec/main/latest/src/PL/MP4_PAL.cpp#8 $
$DateTime: 2009/03/17 17:15:26 $
$Change: 865323 $

========================================================================== */

#include "MP4_PAL.h"
#ifdef T_MSM7500
  #include "pmem.h"
#endif
#include "mp4tables.h"

void* PMEM_GET_PHYS_ADDR (void* abc){
	return abc;
}

extern "C" {
#include "mp4bitstream.h"
#ifdef FEATURE_QTV_DECODER_XSCALE_VIDEO
#include "vppXScalar.h"
#endif //FEATURE_QTV_DECODER_XSCALE_VIDEO
}

#define MAX_DCT_COEF 64

#define MPEG4_GET_FRAME_HDR_USERDATA(p)  (((p)->FrameInfo & 0xFF00) >> 8)
#define MPEG4_GET_FRAME_HDR_VOP(p)  ((p)->FrameInfo & 0x3)

#define H263P3_PACK_FRAME_HDR_INFO(userdata,vop)  (((userdata) << 8) | (vop & 0x3))
#ifdef FEATURE_QDSP_RTOS
/* H263P3 Frame Header's FRAME_INFO field as per 7500 Decoder I/F Spec */
  #define H263P3_QDSP_RTOS_PACK_FRAME_HDR_INFO(dbFilterMode, fAdvancedIntraCodingMode, vopRoundingType, OutputVOPIndex) \
  ( \
    ((dbFilterMode)<<4) | \
    ((fAdvancedIntraCodingMode)<<3) | \
    (((vopRoundingType != 0))<<2) | \
    ((OutputVOPIndex) & 0x3) \
  )
#endif  /* FEATURE_QDSP_RTOS */

#define MPEG4_PACK_FRAME_HDR_INFO(userdata, vop_rounding_type, deblock, vop) \
  ( \
    ((userdata) << 8) | \
    (((vop_rounding_type) & 0x01 ) << 4) | \
    (((deblock) & 0x01 ) << 3) | \
    ((vop) & 0x3) \
  )
//LCDB default strength provided by systems team
#define MPEG4_LCDB_DEFAULT_STRENGTH 29

unsigned char qtv_cfg_DSPDeblockingEnable;

/* <EJECT> */
/*===========================================================================

FUNCTION:
  init_member_variables

DESCRIPTION:
  called by constructor
  
RETURN VALUE:
  None.

===========================================================================*/
void MP4_PAL::init_member_variables()
{
  m_pHdr = NULL;
  m_pDecodeBuffer = NULL;
  m_BufferSize = 0;
  m_pRVLC = NULL;
  m_pRVLCSave = NULL;
  m_pMaxRVLC = NULL;
  m_pGuardRVLC = NULL;
  m_pRVLCCount = NULL;
#if defined (FEATURE_QDSP_RTOS) && defined (FEATURE_MPEG4_COMPACT_PACKET)
  m_pCount = NULL;
#endif /* defined (FEATURE_QDSP_RTOS) && defined (FEATURE_MPEG4_COMPACT_PACKET) */
  m_pDecodeStats = NULL;
  sliceSentToVdlLayer = false;
  m_fReuseCurSliceBuf = FALSE;
  m_fPartialSliceDecoded = FALSE;
  m_QDSPPacket.pSlice = NULL;

  m_subframePacketSeqNum = 0;
  m_pMP4DL = NULL;
  m_SrcWidth = 0;
  m_SrcHeight = 0;
  m_BufferSize = 0;
  m_fShortVideoHeader = FALSE;
#ifdef FEATURE_QTV_DECODER_XSCALE_VIDEO
  /*deafualt to NO xScaling */
  vpp_xScale_frames(FALSE,0,0,0,0);
  m_scaleCbFn = 0;
  m_scaleCbData = 0;
#endif //FEATURE_QTV_DECODER_XSCALE_VIDEO
#ifdef FEATURE_QTV_DECODER_LCDB_ENABLE
  lcdbParamsChanged  = FALSE;
  /* default to default LCDB settings*/
  memset(&LCDBParamSettings,0,sizeof(VDEC_LCDB_STRENGTH));
  LCDBParamSettings.LCDB_INTRA_MB_QP_THD0 = MPEG4_LCDB_DEFAULT_STRENGTH;
#endif
}
/* <EJECT> */
/*===========================================================================

FUNCTION:
  MP4_PAL

DESCRIPTION:
  Constructor for MPEG-4's packet abstraction layer
  
RETURN VALUE:
  None.

===========================================================================*/
MP4_PAL::MP4_PAL()
{
  init_member_variables();
  #ifdef FEATURE_MPEG4_VLD_DSP
  m_VLDinDSPflag = false;
  #endif
}
/* <EJECT> */
/*===========================================================================

FUNCTION:
  MP4_PAL

DESCRIPTION:
  Constructor for MPEG-4's packet abstraction layer
  
RETURN VALUE:
  None.

===========================================================================*/
MP4_PAL::MP4_PAL(signed short* pError)
{
  init_member_variables();
  //Create and Initialize DL layer
  VDL_ERROR pVDLError;
  m_pMP4DL = VDL_Create(&pVDLError); 
  if ( pVDLError != VDL_ERR_NONE || m_pMP4DL == NULL )
  {
    QTV_MSG_PRIO1(QTVDIAG_GENERAL, QTVDIAG_PRIO_FATAL,
                  "VDL_Create errored with return code = %d",pVDLError);     

    *pError = MP4ERROR_INIT_FAILED;
  }
  else
  {
    *pError = MP4ERROR_SUCCESS;
  }
}

/*===========================================================================
FUNCTION:
  FreeCurrentSliceBuffer
DESCRIPTION:
Frees the current slice buffer (Done at time of error)  
  
RETURN VALUE:
NONE
  
===========================================================================*/
void MP4_PAL::FreeCurrentSliceBuffer()
{
  m_fReuseCurSliceBuf = FALSE;
  if (m_QDSPPacket.pSlice != NULL)
  {
    VDL_Free_Slice_Buffer(m_pMP4DL,m_QDSPPacket.pSlice);
    m_QDSPPacket.pSlice = NULL;
  }
}
/* <EJECT> */
/*===========================================================================

FUNCTION:
  ~MP4_PAL

DESCRIPTION:
  Destructor for MPEG-4's packet abstraction layer
  
RETURN VALUE:
  None.
===========================================================================*/
MP4_PAL::~MP4_PAL()
{
  VDL_ERROR DLErrorCode;
  if(m_QDSPPacket.pSlice != NULL)
  {
    VDL_Free_Slice_Buffer(m_pMP4DL,m_QDSPPacket.pSlice);
    m_QDSPPacket.pSlice = NULL;
  }

  DLErrorCode = VDL_Terminate(m_pMP4DL);

  if ( DLErrorCode != VDL_ERR_NONE )
  {

    QTV_MSG_PRIO1(QTVDIAG_GENERAL, QTVDIAG_PRIO_FATAL,

                  "VDL_terminate failed with error code %d",DLErrorCode);

  }


  DLErrorCode = VDL_Destroy(m_pMP4DL);
  if ( DLErrorCode != VDL_ERR_NONE )
  {
    QTV_MSG_PRIO1(QTVDIAG_GENERAL, QTVDIAG_PRIO_FATAL,
                  "VDL_Destroy failed with error code %d",DLErrorCode);
  }

  m_pMP4DL  = NULL;
}
/* <EJECT> */
/*===========================================================================

FUNCTION:
  Initialize 

DESCRIPTION:
  Initialization for MPEG-4's packet abstraction layer
  
RETURN VALUE:
  None.

===========================================================================*/
unsigned char MP4_PAL::Initialize(unsigned short height, unsigned short width, VDL_Concurrency_Type ConcurrencyFormat,
                            unsigned int streamID, void* DecoderCB, int adsp_fd)
{
  //Create and Initialize DL layer
  VDL_ERROR pError;
  //Save the dimentions for later use(currently only for xscaler
  m_SrcHeight = height;
  m_SrcWidth = width;

  //Initialize the DL layer for this instance's use
  pError = VDL_Initialize(m_pMP4DL,
                          (VDL_Decoder_Cb_Type)DecoderCB,
                          (void*)streamID,
                          VDL_VIDEO_MPEG4,
                          ConcurrencyFormat,
			  adsp_fd);
  if ( pError != VDL_ERR_NONE )
  {
    QTV_MSG_PRIO1(QTVDIAG_GENERAL, QTVDIAG_PRIO_FATAL,"VDL_Initialize failed with err code = %d",pError);
    return FALSE;
  }
  return TRUE;
}

#ifdef FEATURE_MPEG4_VLD_DSP
/*===========================================================================
FUNCTION:
  SetVLDinDSPflag

DESCRIPTION:
  Enables/diables VLD in DSP flag
  
INPUT/OUTPUT PARAMETERS:
  unsigned char to enable or disable vld in dsp

RETURN VALUE:
  unsigned char indicating whether vld in dsp can be set or not

SIDE EFFECTS: 
==============================================================================*/
unsigned char MP4_PAL::SetVLDinDSPflag(unsigned char bEnableVLDinDSP)
{
  if(VLDInterfaceType == VDL_INTERFACE_RTOS_VLD_DSP)
  {
    //already VDL initialized cannot change run time
    QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_FATAL,
                 "Cannot change to VLD in DSP run time");
     return FALSE;
  }
  else
  {
    //The user option will be considered
    m_VLDinDSPflag = bEnableVLDinDSP;
    QTV_MSG_PRIO1(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_FATAL,
                 "VLD in DSP flag = %d set",m_VLDinDSPflag);
    return TRUE;
  }
}
#endif
/*===========================================================================

FUNCTION:
  Configure_HW 

DESCRIPTION:
  Configures the driver layer HW
  
RETURN VALUE:
  VDL_ERROR.

===========================================================================*/
VDL_ERROR MP4_PAL::Configure_HW(unsigned short DecHeight, unsigned short DecWidth,
                                unsigned short SrcHeight,unsigned short SrcWidth,VDL_Video_Type VideocodecType, VDEC_SLICE_BUFFER_INFO * pSliceBufferInfo)
{
  //Save the dimentions for later use
  m_SrcHeight = SrcHeight;
  m_SrcWidth = SrcWidth;
  m_CodecType = VideocodecType;
#ifdef FEATURE_QTV_DECODER_XSCALE_VIDEO
  //Now we have proper width and height..Convey xScaler details to PAL
  if ( m_scaleCbFn )
  {
    (void)Scale(DecWidth,DecHeight,m_outFrameSize,m_scaleCbFn,m_scaleCbData);
    m_scaleCbFn = 0;
  }
#endif //FEATURE_QTV_DECODER_XSCALE_VIDEO  
  VDL_ERROR err;
#ifdef FEATURE_MPEG4_VLD_DSP
  if((err=VDL_Configure_HW(m_pMP4DL, DecHeight, DecWidth, VDL_PROFILE_SIMPLE,VideocodecType,NULL,8,NULL,pSliceBufferInfo,8)))
  {
	QTV_MSG_PRIO1(QTVDIAG_VIDEO_TASK,QTVDIAG_PRIO_FATAL,
                 "Driver Layer hardware config failed with error code %d",
                 err);
    return err;
  }
  else
  {
    QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_HIGH,"MPEG4 HardwareConfig Done Successfully");
  }
  //Configuration successfull. Check the DSP interface type.
  VLDInterfaceType = VDL_Get_Interface_Type(m_pMP4DL,DecHeight,DecWidth,VideocodecType,VDL_PROFILE_SIMPLE);
  if(VLDInterfaceType == VDL_INTERFACE_RTOS_VLD_DSP)
  {
  QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_HIGH,"MPEG4 HardwareConfig VLD in DSP Interface");
  }
  else
  {
    QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_HIGH,"MPEG4 HardwareConfig VLD in ARM Interface");
  }
#else
  return VDL_Configure_HW(m_pMP4DL, DecHeight, DecWidth, VDL_PROFILE_SIMPLE,VideocodecType,NULL,8,NULL,pSliceBufferInfo,8);
#endif

  #ifdef FEATURE_MPEG4_VLD_DSP
   return err;
  #endif
}

/* <EJECT> */
/*===========================================================================

FUNCTION:
  Flush  

DESCRIPTION:
  Flush the MPEG-4 packet abstraction layer
  
RETURN VALUE:
  TRUE if successful, FALSE otherwise.

===========================================================================*/
VDL_ERROR MP4_PAL::Flush()
{
  VDL_ERROR DLErrorCode;
  if(m_QDSPPacket.pSlice != NULL)
  {
     QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_FATAL,
                               "PAL having a slice..Check for leak");
  }

  DLErrorCode = VDL_Flush(m_pMP4DL);
  if ( DLErrorCode != VDL_ERR_NONE )
  {
    QTV_MSG_PRIO1(QTVDIAG_GENERAL, QTVDIAG_PRIO_FATAL,
                  "VDL_Flush errored with return code = %d",DLErrorCode);
  }
  return DLErrorCode;
}

/* <EJECT> */
/*===========================================================================

FUNCTION:
  Suspend 

DESCRIPTION:
  Suspend the MPEG-4 packet abstraction layer
  
RETURN VALUE:
  TRUE if successful, FALSE otherwise.

===========================================================================*/
VDL_ERROR MP4_PAL::Suspend()
{
  VDL_ERROR DLErrorCode;

  DLErrorCode = VDL_Suspend(m_pMP4DL);
  if ( DLErrorCode != VDL_ERR_NONE )
  {
    QTV_MSG_PRIO1(QTVDIAG_GENERAL, QTVDIAG_PRIO_FATAL,
                  "VDL_Suspend errored with return code = %d",DLErrorCode);
  }
  return DLErrorCode;
}

/* <EJECT> */
/*===========================================================================

FUNCTION:
  Resume

DESCRIPTION:
  Resume the MPEG-4 packet abstraction layer
  
RETURN VALUE:
  TRUE if successful, FALSE otherwise.

===========================================================================*/
VDL_ERROR MP4_PAL::Resume(unsigned long concurrencyConfig)
{
  VDL_ERROR DLErrorCode;

  DLErrorCode = VDL_Resume(m_pMP4DL, (VDL_Concurrency_Type) ConvertVDLConcurrencyTypeToDSP(concurrencyConfig));
  if ( DLErrorCode != VDL_ERR_NONE )
  {
    QTV_MSG_PRIO1(QTVDIAG_GENERAL, QTVDIAG_PRIO_FATAL,
                  "VDL_Resume errored with return code = %d",DLErrorCode);
  }
  return DLErrorCode;
}
#ifdef FEATURE_QTV_DECODER_XSCALE_VIDEO
/* <EJECT> */
/*===========================================================================

FUNCTION:
  Scale

DESCRIPTION:
  Enable/Disable xScaling
  
RETURN VALUE:
  

===========================================================================*/
VDEC_ERROR MP4_PAL::Scale(  unsigned short width, unsigned short height,
                            VDEC_DIMENSIONS outFrameSize,
                            VDEC_SCALE_CB_FN scaleCbFn,
                            void * const     pCbData )
{
  unsigned char bScale = TRUE;
  VDEC_ERROR vdec_err = VDEC_ERR_OPERATION_UNSUPPORTED;

  if ( height && width )
  {
    QTV_MSG_PRIO2( QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_HIGH,"MP4_PAL::Scale SourceDim = %d x %d",width,height);
    QTV_MSG_PRIO2( QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_HIGH,"MP4_PAL::Scale Requested Scaled Dim = %d x %d",outFrameSize.width,outFrameSize.height);

    VPP_XSCALE_ERROR xScalarErr = vpp_xScale_frames(TRUE,
                                                    width,
                                                    height,
                                                    outFrameSize.width,
                                                    outFrameSize.height);

    if ( xScalarErr != VPP_XSCALE_ERR_EVERYTHING_FINE )
    {
      QTV_MSG_PRIO1(QTVDIAG_GENERAL, QTVDIAG_PRIO_HIGH,
                    "MP4_PAL::Scale err (%d)",
                    xScalarErr );

      if ( scaleCbFn )
      {
        (* scaleCbFn)(this,FALSE,pCbData);
      }
      return VDEC_ERR_OPERATION_UNSUPPORTED;
    }
    else
    {
      if ( scaleCbFn )
      {
        (* scaleCbFn)(this,TRUE,pCbData);
      }
    }
  }
  else
  {
    //store for later use when we know the frame size of the 
    //original frame after decoding the seq para.
    m_scaleCbFn = scaleCbFn;
    m_outFrameSize = outFrameSize;
    m_scaleCbData = pCbData;
  }
  return VDEC_ERR_EVERYTHING_FINE;
}
#endif //FEATURE_QTV_DECODER_XSCALE_VIDEO

/*===========================================================================

FUNCTION:
  SetLCDBStrength

DESCRIPTION:
  Set the LCDB parameters.
  
RETURN VALUE:
  void
===========================================================================*/
void MP4_PAL::SetLCDBParams(void *lcdbStrength)
{
#ifdef FEATURE_QTV_DECODER_LCDB_ENABLE
  lcdbParamsChanged = TRUE;
  memcpy(&LCDBParamSettings,(VDEC_LCDB_STRENGTH*)lcdbStrength,sizeof(VDEC_LCDB_STRENGTH));
  QTV_MSG_PRIO2( QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_HIGH,
                 "LCDB threshold setting: %d,LCDB_INTRA_MB_QP_THD0: %d,",LCDBParamSettings.bLCDBThreshold,LCDBParamSettings.LCDB_INTRA_MB_QP_THD0);
  QTV_MSG_PRIO3( QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_HIGH,
                 "LCDB_INTER_MB_QP_THD_BND0 = %d,LCDB_INTER_MB_QP_THD_INSIDE0 = %d,LCDB_INTER_MB_QP_THD_BND4 = %d",
                 LCDBParamSettings.LCDB_INTER_MB_QP_THD_BND0,LCDBParamSettings.LCDB_INTER_MB_QP_THD_INSIDE0,LCDBParamSettings.LCDB_INTER_MB_QP_THD_BND4);

  QTV_MSG_PRIO3( QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_HIGH,
                 "LCDB_INTER_MB_QP_THD_INSIDE4 = %d,LCDB_NUM_NC_BLKS_THD = %d,LCDB_QP_JUMP_NOTCODED_BLKS = %d",
                 LCDBParamSettings.LCDB_INTER_MB_QP_THD_INSIDE4,LCDBParamSettings.LCDB_NUM_NC_BLKS_THD,LCDBParamSettings.LCDB_QP_JUMP_NOTCODED_BLKS);

#else
  lcdbStrength = lcdbStrength; // remove compiler warning
  QTV_MSG_PRIO( QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_HIGH,"LCDB not supported");
#endif
}


#ifdef FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A
/* <EJECT> */
/*===========================================================================

FUNCTION:
  set_output_yuv_address

DESCRIPTION:
  This function writes all the frame header info except for the FrameInfo word

INPUT/OUTPUT PARAMETERS:
  pOutBuf, pVopBuf

RETURN VALUE:
  None.

SIDE EFFECTS:
  None.
===========================================================================*/
void MP4_PAL::set_output_yuv_address( unsigned char *pOutBuf, unsigned char *pVopBuf )
{
#ifdef FEATURE_MP4_H263_PROFILE3
  if ( m_fShortVideoHeader
#if ( defined(FEATURE_VIDEO_AUDIO_DUBBING) || defined(FEATURE_MP4_TRANSCODER) )
       && !m_fProfile3Disable
#endif /* #ifdef FEATURE_VIDEO_AUDIO_DUBBING || FEATURE_MP4_TRANSCODER */
     )
  {
#ifdef FEATURE_QDSP_RTOS
    /* when we start handling Post loop deblocking we will need to set *pOutputYUVBuf to pOutBuf as per  i/f spec*/
    m_pDecodeStats->pOutputYUVBuf = (void*) pVopBuf;
#else
    if ( m_VOP.fDeblockingFilter )
    {
      m_pDecodeStats->pOutputYUVBuf = (void*) pVopBuf;
    }
    else
    {
      m_pDecodeStats->pOutputYUVBuf = (void*) pOutBuf;
    }
#endif  /* FEATURE_QDSP_RTOS */
  }
  else
#endif /* FEATURE_MP4_H263_PROFILE3 */
  {
    if ( qtv_cfg_DSPDeblockingEnable )
      m_pDecodeStats->pOutputYUVBuf = (void*) pOutBuf;
    else
      m_pDecodeStats->pOutputYUVBuf = (void*) pVopBuf;
  }
}
/* <EJECT> */
/*===========================================================================

FUNCTION:
  set_frame_header_addresses

DESCRIPTION:
  This function writes all the frame header info except for the FrameInfo word

INPUT/OUTPUT PARAMETERS:
  pData

RETURN VALUE:
  None.

SIDE EFFECTS:
  None.
===========================================================================*/
void MP4_PAL::set_frame_header_addresses( VOP_type *pVOP, MP4BufType* pMP4Buf, signed short NextInFrameId, signed short prevRefFrameIndex, signed short prevPrevRefFrameIndex)
{
  unsigned char* pOutBuf;
  unsigned char* pVopBuf;
#ifdef FEATURE_QTV_DECODER_XSCALE_VIDEO
  VPP_XSCALE_ERROR xScaleErr;
  unsigned short ScaledWidth,ScaledHeight;
#endif /* FEATURE_QTV_DECODER_XSCALE_VIDEO */
  unsigned char* pBackwardRefVopBuf;
  unsigned char* pForwardRefVopBuf;

  /* Format the global instance of the frame header */

  ASSERT( NextInFrameId >= 0 );
  ASSERT( NextInFrameId < pMP4Buf->numOutputYUV_buffers );
  ASSERT( pMP4Buf->numOutputYUV_buffers == pMP4Buf->numYUVBuffers );

  /* Format the correct output YUV buffer's */
#ifndef FEATURE_QDSP_RTOS
  pOutBuf = (unsigned char*)pMP4Buf->pOutputYUVBuf[NextInFrameId];
#else
// Convert to physical before passing to DSP
  pOutBuf = (unsigned char*)PMEM_GET_PHYS_ADDR(pMP4Buf->pOutputYUVBuf[NextInFrameId]);
#endif
  m_Mpeg4FrameHdr.OutBufHi = (((unsigned long int)pOutBuf) >> 16) & 0xFFFF;
  m_Mpeg4FrameHdr.OutBufLo = ((unsigned long int)pOutBuf) & 0xFFFF;

//#ifdef PLATFORM_LTK
//  unsigned char* pRGBBuf;
//#ifndef FEATURE_QDSP_RTOS
//  pRGBBuf = (unsigned char*)pMP4Buf->pRGBBuf[NextInFrameId];
//#else
//// Convert to physical before passing to DSP
//  pRGBBuf = (unsigned char*)PMEM_GET_PHYS_ADDR(pMP4Buf->pRGBBuf[NextInFrameId]);
//#endif
//  m_Mpeg4FrameHdr.OutRGBBufHi = (((unsigned long int)pRGBBuf) >> 16) & 0xFFFF;
//  m_Mpeg4FrameHdr.OutRGBBufLo = ((unsigned long int)pRGBBuf) & 0xFFFF;
//  m_pDecodeStats->pRGBBuf = (void*) pRGBBuf;
//#endif /* PLATFORM_LTK */

#ifndef FEATURE_QDSP_RTOS
  pVopBuf = (unsigned char*)pMP4Buf->pYUVBuf[NextInFrameId]; 
#else
  pVopBuf = (unsigned char*)PMEM_GET_PHYS_ADDR(pMP4Buf->pYUVBuf[NextInFrameId]);
#endif

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
    pBackwardRefVopBuf = (unsigned char*)PMEM_GET_PHYS_ADDR(pMP4Buf->pYUVBuf[prevRefFrameIndex]);
    m_Mpeg4FrameHdr.VopBuf0Hi = (((unsigned long int)pBackwardRefVopBuf) >> 16) & 0xFFFF;
    m_Mpeg4FrameHdr.VopBuf0Lo = ((unsigned long int)pBackwardRefVopBuf) & 0xFFFF;
  }
  if ( prevPrevRefFrameIndex >= 0 )
  {
    pForwardRefVopBuf = (unsigned char*)PMEM_GET_PHYS_ADDR(pMP4Buf->pYUVBuf[prevPrevRefFrameIndex]);
    m_Mpeg4FrameHdr.VopBuf1Hi = (((unsigned long int)pForwardRefVopBuf) >> 16) & 0xFFFF;
    m_Mpeg4FrameHdr.VopBuf1Lo = ((unsigned long int)pForwardRefVopBuf) & 0xFFFF;
  }

  m_Mpeg4FrameHdr.VopBuf2Hi = (((unsigned long int)pVopBuf) >> 16) & 0xFFFF;
  m_Mpeg4FrameHdr.VopBuf2Lo = ((unsigned long int)pVopBuf) & 0xFFFF;

  QTV_MSG_PRIO6(QTVDIAG_GENERAL, QTVDIAG_PRIO_HIGH,
                "FrameHdr: VopBuf0=0x%x%x, VopBuf1=0x%x%x, VopBuf2=0x%x%x",
                m_Mpeg4FrameHdr.VopBuf0Hi,m_Mpeg4FrameHdr.VopBuf0Lo,
                m_Mpeg4FrameHdr.VopBuf1Hi,m_Mpeg4FrameHdr.VopBuf1Lo,
                m_Mpeg4FrameHdr.VopBuf2Hi,m_Mpeg4FrameHdr.VopBuf2Lo
               );

  set_output_yuv_address(pOutBuf, pVopBuf);

#ifdef FEATURE_QTV_DECODER_XSCALE_VIDEO
  if ( vpp_xScalar_is_active() )
  {
    xScaleErr = vpp_xScalar_pack_cmd(m_Mpeg4FrameHdr.xScalarBuffer,
                                     sizeof(m_Mpeg4FrameHdr.xScalarBuffer),
                                     pVopBuf,
                                     pOutBuf,
                                     TRUE,
                                     TRUE,
                                     NextInFrameId,
                                     &ScaledWidth,
                                     &ScaledHeight);

    if ( xScaleErr == VPP_XSCALE_ERR_EVERYTHING_FINE )
    {
#ifndef FEATURE_QDSP_RTOS
      //Always use output buffer if XScaler is used.
      m_pDecodeStats->pOutputYUVBuf = (void*) pOutBuf;
#endif
      /* enable xScale in the frame header */
      m_Mpeg4FrameHdr.FrameInfo = m_Mpeg4FrameHdr.FrameInfo | 0x80;;

      /* TEMPORARY ARM SIDE WORKAROUND FOR DSP BUG */
      /* due to a problem with non-mod 16 clips and the DSP, we need to adjust the clipping region
      ** to clip off some non-bit exact pixels.
      **
      ** Once this DSP bug is fixed, we can remove all of this save the else cases.
      */
      if ( m_SrcWidth & 15 )
      {
        vpp_xScalar_adjust_scaled_dim( ScaledWidth,
                                       m_SrcWidth,
                                       &m_pDecodeStats->Width,
                                       &m_pDecodeStats->ScaledWidth,
                                       (VPP_XScalarCmdType*)m_Mpeg4FrameHdr.xScalarBuffer,
                                       TRUE );
      }
      else
      {
        m_pDecodeStats->ScaledWidth = ScaledWidth;
        m_pDecodeStats->Width = ScaledWidth;
      }

      if ( m_SrcHeight & 15 )
      {
        vpp_xScalar_adjust_scaled_dim( ScaledHeight,
                                       m_SrcHeight,
                                       &m_pDecodeStats->Height,
                                       &m_pDecodeStats->ScaledHeight,
                                       (VPP_XScalarCmdType*)m_Mpeg4FrameHdr.xScalarBuffer,
                                       FALSE );
      }
      else
      {
        m_pDecodeStats->ScaledHeight = ScaledHeight;
        m_pDecodeStats->Height = ScaledHeight;
      }
      QTV_MSG_PRIO2( QTVDIAG_VIDEO_TASK,QTVDIAG_PRIO_MED,"Scaling dimensions %d x %d",m_pDecodeStats->ScaledWidth,m_pDecodeStats->ScaledHeight);

      /* TEMPORARY ARM SIDE WORKAROUND FOR DSP BUG */
    }
    else
    {
      /* something isn't right, just don't attempt to scale. */
      QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_ERROR,
                    "Scale command failed: %d",xScaleErr);
      m_pDecodeStats->Width = ScaledWidth = 0;
      m_pDecodeStats->Height  = ScaledHeight = 0;
    }
  }
  else
  {
    QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_LOW,"xScaler Not active");
  }
#endif /* FEATURE_QTV_DECODER_XSCALE_VIDEO */
}
/* <EJECT> */
/*===========================================================================

FUNCTION:
  set_LCDB_params

DESCRIPTION:
  This function writes the LCDB parameters for the MPEG4 frame header

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
  None.

SIDE EFFECTS:
  None.
===========================================================================*/
  #ifdef FEATURE_QTV_DECODER_LCDB_ENABLE
void MP4_PAL::set_LCDB_params()
{
  //LCDB bit needs to be set to 0..set first wors of LCDB word to default
  //default value provided by systems team.
  //for QTV LCDB setting is ignored by DSP for now.
  m_Mpeg4FrameHdr.FrameInfo = m_Mpeg4FrameHdr.FrameInfo | 0x00;
  m_Mpeg4FrameHdr.lcdbwords[0] = MPEG4_LCDB_DEFAULT_STRENGTH;
  if ( LCDBParamSettings.bLCDBThreshold  == 0 && lcdbParamsChanged )
  {
    m_Mpeg4FrameHdr.lcdbwords[0] = LCDBParamSettings.LCDB_INTRA_MB_QP_THD0;
  }
  else if ( LCDBParamSettings.bLCDBThreshold  == 1 && lcdbParamsChanged )
  {
    m_Mpeg4FrameHdr.FrameInfo = m_Mpeg4FrameHdr.FrameInfo | 0x04;
    m_Mpeg4FrameHdr.lcdbwords[0] = LCDBParamSettings.LCDB_INTRA_MB_QP_THD0;
    m_Mpeg4FrameHdr.lcdbwords[1] = LCDBParamSettings.LCDB_INTER_MB_QP_THD_BND0;
    m_Mpeg4FrameHdr.lcdbwords[2] = LCDBParamSettings.LCDB_INTER_MB_QP_THD_INSIDE0;
    m_Mpeg4FrameHdr.lcdbwords[3] = LCDBParamSettings.LCDB_INTER_MB_QP_THD_BND4;
    m_Mpeg4FrameHdr.lcdbwords[4] = LCDBParamSettings.LCDB_INTER_MB_QP_THD_INSIDE4;
    m_Mpeg4FrameHdr.lcdbwords[5] = LCDBParamSettings.LCDB_NUM_NC_BLKS_THD;
    m_Mpeg4FrameHdr.lcdbwords[6] = LCDBParamSettings.LCDB_QP_JUMP_NOTCODED_BLKS;
  }
}
  #endif /* FEATURE_QTV_DECODER_LCDB_ENABLE */

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
void MP4_PAL::set_frame_header( VOP_type *pVOP, MP4BufType* pMP4Buf, signed short NextInFrameId
                                , signed short prevRefFrameIndex, signed short prevPrevRefFrameIndex
                              )
{
  char vopRoundingType = pVOP->fRoundingType;
  set_frame_header_addresses(pVOP, pMP4Buf, NextInFrameId, prevRefFrameIndex, prevPrevRefFrameIndex);
/* Here we also set the YUV buf that the DSP will decode the present frame into (*pOutputYUVBuf).
** For case of  H263P3 && FEATURE_QDSP_RTOS and no post deblocking,we decode into the VOP Buf
** : Refer ARM DSP interface spec for 7X.
** Also for Mpeg4 and H263 P0 in case deblocking was switched off as per spec, 
** we should have had output buffer for DSP to be VOP buffer. But here in call 
** to MPEG4_PACK_FRAME_HDR_INFO we hard code deblocking to TRUE. I don't know the reasons,
**  but at some point we may want to look at that.
*/

#ifdef FEATURE_MP4_H263_PROFILE3
  if ( m_fShortVideoHeader
#if ( defined(FEATURE_VIDEO_AUDIO_DUBBING) || defined(FEATURE_MP4_TRANSCODER) )
       && !m_fProfile3Disable
#endif /* #ifdef FEATURE_VIDEO_AUDIO_DUBBING || FEATURE_MP4_TRANSCODER */
     )
  {
    unsigned char dbFilterMode = MP4_DISABLE_DB_FILTER;
    /* H263P3 Frame Header's FRAME_INFO field as per 7500 Decoder I/F Spec */
#ifdef FEATURE_MP4_H263_ANNEX_J
    if ( pVOP->fDeblockingFilter )
    {
      dbFilterMode = MP4_IN_LOOP_DB_FILTER;
    }
#endif /* FEATURE_MP4_H263_ANNEX_J */

#ifdef FEATURE_QDSP_RTOS
#ifdef FEATURE_MP4_H263_ANNEX_I
    m_Mpeg4FrameHdr.FrameInfo = H263P3_QDSP_RTOS_PACK_FRAME_HDR_INFO(dbFilterMode, pVOP->fAdvancedIntraCodingMode,
                                                                     vopRoundingType, UNFILTERED_OUTPUT_VOP_INDEX);
#else    
    m_Mpeg4FrameHdr.FrameInfo = H263P3_QDSP_RTOS_PACK_FRAME_HDR_INFO(dbFilterMode, 0,
                                                                     vopRoundingType, UNFILTERED_OUTPUT_VOP_INDEX);
#endif   /* FEATURE_MP4_H263_ANNEX_I */
#else
    if ( dbFilterMode == MP4_IN_LOOP_DB_FILTER )
    {
      m_pDecodeStats->pOutputYUVBuf = (void*) pVopBuf;
    }
    else
    {
      m_pDecodeStats->pOutputYUVBuf = (void*) pOutBuf;
    }
    m_Mpeg4FrameHdr.FrameInfo = H263P3_PACK_FRAME_HDR_INFO(NextInFrameId, UNFILTERED_OUTPUT_VOP_INDEX);
#endif  /* FEATURE_QDSP_RTOS */
  }
  else
#endif /* FEATURE_MP4_H263_PROFILE3 */
  {
    m_Mpeg4FrameHdr.FrameInfo = MPEG4_PACK_FRAME_HDR_INFO(NextInFrameId, vopRoundingType, qtv_cfg_DSPDeblockingEnable, UNFILTERED_OUTPUT_VOP_INDEX);
#ifdef FEATURE_QTV_DECODER_LCDB_ENABLE
    if ( !m_fShortVideoHeader )
      set_LCDB_params();
#endif
  }
}

/* <EJECT> */
/*===========================================================================

FUNCTION:
  update_frame_header

DESCRIPTION:
  This function writes the updates the frame header if there are
  any changes in the frame header formed initially.

INPUT/OUTPUT PARAMETERS:
  Width, Height

RETURN VALUE:
  None.

SIDE EFFECTS:
  None.
===========================================================================*/
void MP4_PAL::update_frame_header(unsigned short Width, unsigned short Height)
{
	if(Width < m_Mpeg4FrameHdr.Width)
		m_Mpeg4FrameHdr.Width = Width;
	if(Height < m_Mpeg4FrameHdr.Height)
		m_Mpeg4FrameHdr.Height = Height;
}

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
void MP4_PAL::init_frame_hdr_default(MP4BufType* pMP4Buf, unsigned short Width, unsigned short Height)
{
  unsigned char* p;
#ifdef FEATURE_MP4_H263_PROFILE3
  if ( m_fShortVideoHeader
#if ( defined(FEATURE_VIDEO_AUDIO_DUBBING) || defined(FEATURE_MP4_TRANSCODER) )
       && !m_fProfile3Disable
#endif /* #ifdef FEATURE_VIDEO_AUDIO_DUBBING || FEATURE_MP4_TRANSCODER */
     )
  {
    m_Mpeg4FrameHdr.PacketId  = H263P3_FRAME_HEADER_PACKET_TYPE;
  }
  else
#endif /* FEATURE_MP4_H263_PROFILE3 */
  {
    m_Mpeg4FrameHdr.PacketId  = MP4_FRAME_HEADER_PACKET_TYPE;
  }
  m_Mpeg4FrameHdr.EndMarker = MP4_MB_END_MARKER;

  /* The line width in pixels. It can be used to support interlaced frames. This
   * parameter may find other usage in video applications in the future.
   *  1. Not interlaced frame, LINE_WIDTH = X_DIMENSION
   *  2. Interlaced frame, LINE_WIDTH = 2 * X_DIMENSION
   * Because of hardware limitations, LINE_WIDTH must be a multiple of 32
   * bytes. For example, for QCIF, LINE_WIDTH=176+16, while the X_DIMENSION =
   * 176
   */
//  m_Mpeg4FrameHdr.LineWidth = (pMP4Buf->Width + 0x1F) & 0xFFE0;
  m_Mpeg4FrameHdr.LineWidth = (pMP4Buf->Width);

  /* Init the YUV buffer pointers. For Mpeg4 this address will be 
     reset on a per frame basis as-n-when display buffers become available
     from the upper client layer. There should be a 1:1 mapping between 
     the reference buffers and the display buffers */

#ifndef FEATURE_QDSP_RTOS
  p = (unsigned char *)pMP4Buf->pYUVBuf[0]; 
#else
  p = (unsigned char *)PMEM_GET_PHYS_ADDR((unsigned char *)pMP4Buf->pYUVBuf[0]);
#endif
  m_Mpeg4FrameHdr.VopBuf0Hi = (((unsigned long int)p) >> 16) & 0xFFFF;
  m_Mpeg4FrameHdr.VopBuf0Lo = ((unsigned long int)p) & 0xFFFF;

#ifndef FEATURE_QDSP_RTOS
  p = (unsigned char *)pMP4Buf->pYUVBuf[1]; 
#else
  p = (unsigned char *)PMEM_GET_PHYS_ADDR((unsigned char *)pMP4Buf->pYUVBuf[1]);
#endif
  m_Mpeg4FrameHdr.VopBuf1Hi = (((unsigned long int)p) >> 16) & 0xFFFF;
  m_Mpeg4FrameHdr.VopBuf1Lo = ((unsigned long int)p) & 0xFFFF;

#ifndef FEATURE_QDSP_RTOS
  p = (unsigned char *)(unsigned long int)pMP4Buf->pYUVBuf[2]; 
#else
  p = (unsigned char *)PMEM_GET_PHYS_ADDR((unsigned char *)pMP4Buf->pYUVBuf[2]);
#endif
  m_Mpeg4FrameHdr.VopBuf2Hi = (((unsigned long int)p) >> 16) & 0xFFFF;
  m_Mpeg4FrameHdr.VopBuf2Lo = ((unsigned long int)p) & 0xFFFF;

  m_Mpeg4FrameHdr.Width = Width;
  m_Mpeg4FrameHdr.Height = Height;
  /* The line width in pixels. It can be used to support interlaced frames. This
     * parameter may find other usage in video applications in the future.
     *  1. Not interlaced frame, LINE_WIDTH = X_DIMENSION
     *  2. Interlaced frame, LINE_WIDTH = 2 * X_DIMENSION
     * Because of hardware limitations, LINE_WIDTH must be a multiple of 32
     * bytes. For example, for QCIF, LINE_WIDTH=176+16, while the X_DIMENSION =
     * 176
  */
  //  m_Mpeg4FrameHdr.LineWidth = (DecWidth + 0x1F) & 0xFFE0;
  m_Mpeg4FrameHdr.LineWidth = Width;
#ifdef FEATURE_QTV_DECODER_LCDB_ENABLE
  memset(m_Mpeg4FrameHdr.lcdbwords,0,LCDB_CMD_LENGTH*sizeof(unsigned short));
#endif
#ifdef FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A_XSCALE
  /*Initialize the xScalar body*/
  memset(m_Mpeg4FrameHdr.xScalarBuffer,0,XSCALAR_CMD_LENGTH*sizeof(unsigned short));
#endif
}
#endif /* FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A */

/* <EJECT> */
/*===========================================================================

FUNCTION:
  GetSliceBuf

DESCRIPTION:
  Allocate a new slice buffer
  
RETURN VALUE:
  TRUE if successful, FALSE otherwise
===========================================================================*/
unsigned char MP4_PAL::GetSliceBuf()
{
  /* We are about to get a free slice buffer off the mp4_slice_free_q.
  ** Keep track of it so that we can free it if termination occurs before
  ** the transition to the mp4_slice_q is complete. A TASKLOCK() cannot
  ** be done here because mp4_get_slice_buffer() uses a critical section.
  ** This is an urgent short term solution to prevent fatal errors that
  ** are seen during termination.
  */
  if (m_QDSPPacket.pSlice != NULL )
  {
    QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_FATAL,
                 "PAL already has a slice..check for leak!");
  }

  m_QDSPPacket.pSlice = VDL_Get_Slice_Buffer(m_pMP4DL);
  if ( !m_QDSPPacket.pSlice )
  {
    QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_FATAL,
                 "mp4_get_slice_buffer out of memory!");
    return FALSE;
  }
  return TRUE;
}

#ifdef FEATURE_MPEG4_VLD_DSP
/*===========================================================================

FUNCTION:
  queue_stats_buffer

DESCRIPTION:
  Queues the stats buffer to DL
  
RETURN VALUE:
  None.
===========================================================================*/
void MP4_PAL::queue_stats_buffer(void *pCurrentDecodeStats)
{
  VDL_Queue_Stats_Buffer( m_pMP4DL,pCurrentDecodeStats, VDL_DECODE_NO_ERROR );
}
#endif

/* <EJECT> */
/*===========================================================================

FUNCTION:
  MBSubPacketsToDSPSliceBuffer

DESCRIPTION:
  Send the slice buffer to the DSP
  
RETURN VALUE:
  None.
===========================================================================*/
void MP4_PAL::MBSubPacketsToDSPSliceBuffer(unsigned long int uiMBCount, unsigned long int uiFirstMB, 
                                           MP4BufType* pMP4Buf, signed short NextInFrameId, 
                                           signed short prevRefFrameIndex, signed short prevPrevRefFrameIndex,
                                           VOP_type *pVOP, unsigned char bCBP[MAX_MB], 
                                           unsigned char fScalability, VDL_Video_Type VideoDecoderType,
                                           DecodeStatsType  **ppCurrentDecodeStats)
{
  uiFirstMB = uiFirstMB;  /* to handle compiler warning */
  /* need to adjust sQDESPPacket.MBlast for the
  ** correct number decoded because mp4_prepare_send_video_slice()
  ** is driven by this element
  */
  m_QDSPPacket.MBlast = uiMBCount;

  /* SEND THE SLICE TO THE QDSP */
  if ( !m_fReuseCurSliceBuf )
  {
    unsigned long int totnBytes;       /* number of bytes remaining in the payload */

    m_QDSPPacket.pSlice->NumMacroBlocks = 0;
    m_QDSPPacket.pSlice->NumMacroBlocks = (unsigned short) (m_QDSPPacket.MBlast - m_QDSPPacket.MBnum);
    m_QDSPPacket.MBnum = m_QDSPPacket.MBlast;
    totnBytes = (unsigned char*) m_pRVLC - (unsigned char*) m_QDSPPacket.pSlice->pNextMacroBlock;
    ASSERT( m_QDSPPacket.MBnum == (signed long int) uiMBCount );
    m_QDSPPacket.pSlice->fLastMB = (m_QDSPPacket.MBnum == pVOP->TotalMB);

    /* Add MPEG4 header to slice queue */
#ifdef FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A
    if ( m_QDSPPacket.pSlice->fFirstMB )
    {
      unsigned short *pSliceMB = (unsigned short*)m_QDSPPacket.pSlice->pSliceData;

      set_frame_header( pVOP, pMP4Buf, NextInFrameId, prevRefFrameIndex, prevPrevRefFrameIndex);

      /* Store the value of NextInFrameId which we set in set_frame_header in the 
      ** stats buffer. This value will be used by us when we want to rewind to the correct 
      ** value of NextInFrameId after we flush out the stats Q and slice Q during repositioning
      ** in mp4_process_flush. If the value of NextInFrameId is incorrectly set then the  
      ** ARM-DSP sync will be disturbed and we could encounter video freeze after repositioning.
      */
#ifdef FEATURE_QDSP_RTOS
      if ( VideoDecoderType == VDL_VIDEO_H263P3 )
      {
        m_pDecodeStats->OutBufId = (NextInFrameId & 0xFF);
      }
      else
#endif /* FEATURE_QDSP_RTOS */        
      {
        m_pDecodeStats->OutBufId = (m_Mpeg4FrameHdr.FrameInfo & 0xFF00) >> 8;
      }

      if ( VideoDecoderType == VDL_VIDEO_H263P3 )
      {
#ifdef FEATURE_QDSP_RTOS
        memcpy((void*)pSliceMB, (void*)&m_Mpeg4FrameHdr, sizeof(MP4QDSPFrameHeaderType));
#else
        unsigned short *pFrameHeader = (unsigned short*) &m_Mpeg4FrameHdr;
#ifndef FEATURE_QTV_DECODER_LCDB_ENABLE 
        pSliceMB[0] = sizeof(MP4QDSPFrameHeaderType)/sizeof(unsigned short);
#else 
        pSliceMB[0] = (sizeof(MP4QDSPFrameHeaderType)/sizeof(unsigned short)) - LCDB_CMD_LENGTH;
#endif 
        //memcpy((void*)&pSliceMB[1], (void*)&m_Mpeg4FrameHdr, sizeof(MP4QDSPFrameHeaderType));
        /* This is a hack...H.263P3 DSP image only expects 13 words for the 
         * frame header.  It's 14 words for MPEG-4 and H.263.  We're going
         * to take this out when it's fixed in the DSP */
        memcpy((void*)&pSliceMB[1], (void*)pFrameHeader, sizeof(unsigned short) * 3);
        //Skip LineWidth
#ifndef FEATURE_QTV_DECODER_LCDB_ENABLE        
        memcpy((void*)&pSliceMB[4], (void*)(pFrameHeader + 4),  
               sizeof(MP4QDSPFrameHeaderType) - (sizeof(unsigned short) * 4) ); 
#else
        //once again frame header is different for h263 and h263p3(no lcdb words for p3
        //so we have to skip those as well.
        //write from frame_info to OUTPUT_BUFFER_LOW (9 words)
        memcpy((void*)&pSliceMB[4], (void*)(pFrameHeader + 4),sizeof(unsigned short) * 9);
        //now skip lcdb words 7*sizeof(unsigned short) and write from xscalar buffer..
        //20 is xscalar buffer lenght + end of marker bit
        memcpy((void*)&pSliceMB[13],(void*)(pFrameHeader + 20),sizeof(unsigned short)*20);
#endif
        m_QDSPPacket.pSlice->NumMacroBlocks++;
#endif /* FEATURE_QDSP_RTOS */
      }
      else
      {
        memcpy((void*)pSliceMB, (void*)&m_Mpeg4FrameHdr, sizeof(MP4QDSPFrameHeaderType));
#ifdef T_MSM7500
        /* Clear the userdata from the FrameInfo since it doesn't exist on
         * 7500
         */
        ((MP4QDSPFrameHeaderType*)pSliceMB)->FrameInfo &= 0x00FF;
#endif /* T_MSM7500 */
      }
    }
    m_QDSPPacket.pSlice->SliceDataSize = totnBytes;
#ifdef T_MSM7500
    m_QDSPPacket.pSlice->CodecType = VideoDecoderType;
//jlk - refactor this codec type assignment
    if ( VideoDecoderType == VDL_VIDEO_MPEG4 )
    {
      m_QDSPPacket.pSlice->CodecType = VDL_CODEC_MPEG4; 
    }
    else if ( VideoDecoderType == VDL_VIDEO_H263 )
    {
      m_QDSPPacket.pSlice->CodecType = VDL_CODEC_H263_P0;
    }
    else if ( VideoDecoderType == VDL_VIDEO_H263P3 )
    {
      m_QDSPPacket.pSlice->CodecType = VDL_CODEC_H263_P3;
    }

#endif /* T_MSM7500 */
#endif /* FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A */
    /* We are about to queue the dequeued slice buffer onto the mp4_slice_q.
     */
    if ( m_QDSPPacket.pSlice->fLastMB )
    {
      VDL_Queue_Stats_Buffer( m_pMP4DL,(void*) *ppCurrentDecodeStats, VDL_DECODE_NO_ERROR );
      sliceSentToVdlLayer = true;
    }
    VDL_Queue_Slice_Buffer(m_pMP4DL, m_QDSPPacket.pSlice);
    m_QDSPPacket.pSlice = NULL;
  }
}

/* <EJECT> */
/*===========================================================================

FUNCTION:
  EndSliceDecoding

DESCRIPTION:
  Signals the PAL to end the packing of an MPEG-4 bitstream slice
  
RETURN VALUE:
  None.
===========================================================================*/
void MP4_PAL::EndSliceDecoding(unsigned long int uiMBCount, unsigned long int uiTotalMB, unsigned long int uiFirstMB, 
                               MP4BufType* pMP4Buf, signed short NextInFrameId, 
                               signed short prevRefFrameIndex, signed short prevPrevRefFrameIndex,
                               VOP_type *pVOP, 
                               unsigned char bCBP[MAX_MB], unsigned char fScalability, VDL_Video_Type VideoDecoderType, DecodeStatsType **ppCurrentDecodeStats)
{
  m_BufferSize -= ((unsigned long int)m_pRVLC - (unsigned long int)m_pDecodeBuffer);
  if ( (m_BufferSize > MAX_MB_SIZE) && (uiMBCount != uiTotalMB) )
  {
    m_fReuseCurSliceBuf = TRUE; 
  }
  else
  {
    if ( uiMBCount == uiTotalMB )
    {
      //we're done the frame.  Re-init
    }
    m_fReuseCurSliceBuf = FALSE;
  }  
  MBSubPacketsToDSPSliceBuffer(uiMBCount, uiFirstMB, pMP4Buf, NextInFrameId, 
                               prevRefFrameIndex, prevPrevRefFrameIndex,
                               pVOP, bCBP, fScalability, VideoDecoderType, ppCurrentDecodeStats);
}

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
MP4_ERROR_TYPE MP4_PAL::InitSliceDecoding(unsigned short *pszMpegHdr, mp4_slice_type *pSlice, DecodeStatsType  *pCurrentDecodeStats)
{
  MP4_ERROR_TYPE iErrCode = MP4ERROR_SUCCESS;

  iErrCode = InitSliceBuf(pszMpegHdr, pSlice, pCurrentDecodeStats);
  if ( iErrCode != MP4ERROR_SUCCESS )
  {
    return iErrCode;
  }

  m_pDecodeBuffer = (void*)m_pSliceMB;
  //m_BufferSize = BufferSize;
  m_pRVLC  = (rvlc_run_level_type *) m_pSliceMB;

  /* Calculate pointer to the end of the slice buffer.  We don't actually want
  * to fill the slice buffer completely since we pickup concurrency with the
  * QDSP with smaller slices.
  */
  m_pMaxRVLC   = (rvlc_run_level_type*) ((unsigned long int)m_pRVLC + m_BufferSize);
  m_pGuardRVLC = (rvlc_run_level_type*) ((unsigned long int)m_pRVLC + 
                                         m_BufferSize - MAX_MB_SIZE);
  m_pRVLCSave = m_pRVLC;
  return MP4ERROR_SUCCESS;
}


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
MP4_ERROR_TYPE MP4_PAL::InitSliceBuf(unsigned short *pszMpegHdr, mp4_slice_type *pSlice, DecodeStatsType  *pCurrentDecodeStats)
{
  m_pDecodeStats = pCurrentDecodeStats;
  if ( !m_fReuseCurSliceBuf )
  {
    if ( FALSE == GetSliceBuf() )
      return MP4ERROR_OUT_OF_MEMORY;
  }
  m_pHdr = (MP4QDSPMBHeaderType*)m_QDSPPacket.pSlice->pSliceData;
  /*
  ** need to initialize sQSDPPacket here because
  ** MotionVectorPrediction needs it.
  **  Fixed length portion of the slice.
  */
  m_QDSPPacket.FirstMB = pSlice->FirstMB;
  m_QDSPPacket.MBlast  = pSlice->NextMBnum;

  m_pSliceMB = (unsigned short*)m_QDSPPacket.pSlice->pSliceData;
  if ( m_fReuseCurSliceBuf )
  {
    *pszMpegHdr = 0;
    m_pSliceMB = (unsigned short*)((unsigned char*)m_pSliceMB + (m_QDSPPacket.pSlice->SliceBufferSize - m_BufferSize));
  }
  else
  {
    if ( m_QDSPPacket.MBnum == 0 )
    {
      m_QDSPPacket.pSlice->fFirstMB = TRUE;
      m_QDSPPacket.pSlice->fAllocateYUVBuf = TRUE;
#ifdef FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A
      /* set szMpegHdr if we need room for a header, otherwise it will be 0 */
#ifndef FEATURE_QTV_DECODER_LCDB_ENABLE
      *pszMpegHdr = sizeof(MP4QDSPFrameHeaderType)/sizeof(unsigned short);
#else
      if ( m_CodecType == VDL_VIDEO_H263P3 )
      {
        *pszMpegHdr = (sizeof(MP4QDSPFrameHeaderType)/sizeof(unsigned short)) - LCDB_CMD_LENGTH;
      }
      else
      {
        *pszMpegHdr = (sizeof(MP4QDSPFrameHeaderType)/sizeof(unsigned short)); 
      }
#endif
#endif /* FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A */
    }
    else
    {
      m_QDSPPacket.pSlice->fFirstMB = FALSE;
      m_QDSPPacket.pSlice->fAllocateYUVBuf = FALSE;
      *pszMpegHdr = 0;   /* No header necessary if this is not the first slice of a frame */
    }
    m_pSliceMB += *pszMpegHdr;
    m_BufferSize = m_QDSPPacket.pSlice->SliceBufferSize - *pszMpegHdr*sizeof(unsigned short);

  }/* !fReuseCurSliceBuf */
  return MP4ERROR_SUCCESS;
}

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
#ifdef FEATURE_QDSP_RTOS
void MP4_PAL::mp4_unpack_DCT_coef(void *pDecodeBuffer, unsigned char nPairs) 
{
  int i, j, nDCT=0;
  unsigned short tDCT[MAX_DCT_COEF];
#ifdef FEATURE_MPEG4_COMPACT_PACKET
  unsigned short run;
  unsigned short *pCoeffs = (unsigned short*)pDecodeBuffer + 1;
  rvlc_run_level_type *pDCT_buf = (rvlc_run_level_type*)(pDecodeBuffer);

  for ( i=0; i<nPairs; i++ )
  {
    run = pDCT_buf[i].run & 0x00ff;
    for ( j=0; j<run; j++ )
    {
#else
  rvlc_run_level_type *pDCT_buf = (rvlc_run_level_type*)pDecodeBuffer;
  for ( i=0; i<nPairs; i++ )
  {
    for ( j=0; j<pDCT_buf[i].run; j++ )
    {
#endif /* FEATURE_MPEG4_COMPACT_PACKET */
      if ( nDCT >= MAX_DCT_COEF )
      {
        QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_ERROR, "mp4_unpack_DCT_coef() error: exceeded MAX_DCT_COEF");
        break;
      }
      tDCT[nDCT] = 0;
      nDCT++;
    }
    if ( nDCT >= MAX_DCT_COEF )
    {
      QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_ERROR, "mp4_unpack_DCT_coef() error: exceeded MAX_DCT_COEF");
      break;
    }
    tDCT[nDCT] = pDCT_buf[i].level;
    nDCT++;
  }
  for ( i=nDCT; i<MAX_DCT_COEF; i++ )
  {
    tDCT[i] = 0;
  }
#ifdef FEATURE_MPEG4_COMPACT_PACKET
  memcpy((void*)pCoeffs, (void*)tDCT, sizeof(unsigned short)*MAX_DCT_COEF);
#else
  memcpy(pDecodeBuffer, (void*)tDCT, sizeof(unsigned short)*MAX_DCT_COEF);
#endif /* FEATURE_MPEG4_COMPACT_PACKET */
}
#endif /* FEATURE_QDSP_RTOS */
/* <EJECT> */
/*===========================================================================

FUNCTION:
  conceal_macroblocks

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
void MP4_PAL::conceal_macroblocks(
                                 unsigned long int            FirstMB,
                                 unsigned long int            LastMB,
                                 dsp_packet_type  *pm_QDSPPacket,
                                 unsigned long int           *nConcealed,
                                 error_status_type          *pErrStat
                                 )
{
  unsigned long int               i;
  unsigned short              *ptrRVLC = m_pSliceMB;
  MP4QDSPMBHeaderType *pHdr;
  const unsigned char          bytesPerWord = sizeof( unsigned short );
  const unsigned char          wordsPerMB = MP4_NOT_CODED_PACKET_SIZE + 1;
  /* +1 for the end marker */

  QTV_MSG_PRIO2(QTVDIAG_GENERAL, QTVDIAG_PRIO_ERROR, "conceal mblock %d to %d", FirstMB, LastMB );
  ASSERT( ptrRVLC != NULL );

  /* we need to generate (fake) the Slice memory needed to
  ** for fNotCoded blocks in PrepareSendVideoSlice and
  ** mp4qdsp_send_video_pkt().
  */


  /* if there is not enough space to conceal all macro blocks, conceal as many
     as possible */
  if ( (m_BufferSize / (wordsPerMB * bytesPerWord)) < LastMB - FirstMB )
  {
    QTV_MSG_PRIO3(QTVDIAG_GENERAL, QTVDIAG_PRIO_ERROR, "conceal mblock %d to %d, buffersize is %d",
                  FirstMB, LastMB, m_BufferSize );
    /* wordsPerMB is the amount of space used by concealing
       a macro block, but it is done in words, not bytes, so * 2 */
    *nConcealed = (m_BufferSize / (wordsPerMB * bytesPerWord)) - 1;
  }
  else
  {
    *nConcealed = LastMB - FirstMB;
  }

  for ( i = FirstMB; i < FirstMB + *nConcealed; i++ )
  {
    /* Sanity check: make sure we have enough room in the buffer for the number
    ** of words we plan on writing.  We can do this easily by looking at the
    ** element of ptrRVLC one past the end of the current chunk - does it line
    ** up with the end of the buffer as we expect? 
    ** The magic with BufferSize is due to the fact that pDecodeBuffer
    ** is a word pointer but BufferSize is counting bytes. */
    ASSERT( ptrRVLC + wordsPerMB <= m_pSliceMB + ( ( m_BufferSize + 1 ) / bytesPerWord ) );

    /* Decode a "Not Coded" Macroblock into the DSP decode buffer */
    pHdr = (MP4QDSPMBHeaderType *) ptrRVLC; /*lint !e740 !e826 */
    pHdr->NumWords = wordsPerMB;
    pHdr->PacketId = MP4_MB_PACKET_TYPE;
    pHdr->MBInfo = ((1<<4) | (1<<2)); //Set "interMB" bit and "NotCoded" bit
    ptrRVLC[ wordsPerMB - 1 ] = MP4_MB_END_MARKER;
    ptrRVLC += wordsPerMB;
  }

  /* All other ErrStats will be generated at the point of failure but the 
  ** number of concealed MBs is generated here.
  */
  pErrStat->iConcealed = *nConcealed; /*lint !e713 */

  m_pRVLC = (rvlc_run_level_type *)ptrRVLC;
  return;
}

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
MP4_ERROR_TYPE MP4_PAL::mp4huffman_decode_VLC
(
mp4_slice_type      *pSlice,         /* Pointer to slice structure */
unsigned char Intra,                         /* 1 if IVOP, 0 if PVOP */
signed char *pLast,                  /* TRUE if this is last level */
unsigned long int *pCodeLength,                 /* length of the decoded Code */
unsigned long int *pNextCode
)
{
  unsigned long int MaxCode;                       /* Maximum Code Sequence */
  unsigned long int CodeIndex;
  Mp4TCoeffTableInfo  *pMp4TCoeffTableInfo;
  Mp4TCoeffDecodeType *pDecodedTCoefficient;

  /* Read 13 bits, which is maximum code size                    */
  (void)mp4bitstream_slice_show_bits( pSlice, 13, &MaxCode );

  /* Get the leading zeros in the bitstream code word            */
  CodeIndex =  TL_CLZ( MaxCode);

  if ( CodeIndex >= (19 + MAX_AC_TCOEFF_INFO_VLD_TAB_SIZE) )
  {
    return(Intra ? MP4ERROR_ILL_RVLC_I : MP4ERROR_ILL_RVLC_P );
  }
  /* Select the VLC table base address using the leading zeros   */
  pMp4TCoeffTableInfo = (Mp4TCoeffTableInfo *)&Mp4TCoeffTableInfoPtrs[Intra][CodeIndex-19];
  /* Calculate the code offset based on trailing bits following  */
  /* the first 1 in the code                                     */
  CodeIndex = (MaxCode >> pMp4TCoeffTableInfo->shift) & pMp4TCoeffTableInfo->mask;
  pDecodedTCoefficient = (Mp4TCoeffDecodeType *) &(pMp4TCoeffTableInfo->vlcLUTptr)[CodeIndex];
  *pLast = pDecodedTCoefficient->last;

  if ( *pLast == -1 )
  {
    /* Escape Code, return 2 bits following the 7 escape code bits  */
    *pNextCode = (MaxCode >> (13-(7+2))); 
  }
  else
  {
    /* Copy the code length, level, run information to caller space */
    *pCodeLength = pDecodedTCoefficient->codeLength;
    m_pRVLC->level = pDecodedTCoefficient->level;
    m_pRVLC->run = pDecodedTCoefficient->run;
    /* return the sign bit following the VLC code  */
    *pNextCode = (MaxCode >> (13-pDecodedTCoefficient->codeLength)); 
  }
  return MP4ERROR_SUCCESS;
}  /* mp4huffman_decode_VLC  */

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
MP4_ERROR_TYPE MP4_PAL::mp4huffman_decode_AC_terms
(
mp4_slice_type *pSlice,              /* Pointer to slice structure */
unsigned char Intra,                         /* 1 if IVOP, 0 if PVOP */
unsigned char dummy,                       /* Just make the last parameter consistent with h263 decode func */
unsigned long int nDCT                          /* Number of coefficients so far */
)
{
  dummy = dummy; //Supress warning  
  unsigned long int  nextCode;             /* Trailing bits after VLC decode      */
  MP4_ERROR_TYPE iErrCode;
  unsigned long int CodeLength;
  signed char fLast = 0;        /* TRUE if this is last level */

  do
  {
    iErrCode = mp4huffman_decode_VLC(pSlice, Intra, &fLast, &CodeLength, &nextCode);
    if ( iErrCode != MP4ERROR_SUCCESS )
    {
      return iErrCode;
    }

    if ( fLast == -1 ) //Escape Code
    {
      /* Flush out the escape code (7 bits) and 1 first bit following it */
      iErrCode = mp4bitstream_slice_flush_bits(pSlice, 7+1);
      if ( iErrCode != MP4ERROR_SUCCESS )
      {
        return iErrCode;
      }

      /* In case of escape code, "nextCode" has the 2 bits following escape code*/
      if ( !(nextCode & 2) ) /* Esc+ "0" */
      {
        /* Type 1 Escape (Page 168, Section 7.4.1.3) */
        iErrCode =  mp4huffman_decode_VLC(pSlice, Intra, &fLast, &CodeLength, &nextCode);
        if ( iErrCode != MP4ERROR_SUCCESS )
        {
          return iErrCode;
        }
        /* implement Table B-19 and B-20 using if statements*/
        if((fLast != -1) && (m_pRVLC->run < Mp4LMAX[Intra][fLast][0]))
        {
          m_pRVLC->level += Mp4LMAX[Intra][fLast][m_pRVLC->run + 1];
        }
        else
        {
          return MP4ERROR_ILL_RVLC_I;
        }
      }
      else
      {
        /* Flush out the second bit following escape code */
        iErrCode = mp4bitstream_slice_flush_bits(pSlice, 1);
        if ( !(nextCode & 1) )
        {
          /* Type 2 Escape (P. 168, sec. 7.4.1.3) */
          iErrCode = mp4huffman_decode_VLC(pSlice, Intra, &fLast, &CodeLength, &nextCode);
          if ( iErrCode != MP4ERROR_SUCCESS )
          {
            return iErrCode;
          }
          m_pRVLC->run += Mp4RMAX[Intra][fLast][m_pRVLC->level-1];
        }
        else
        {
          /* Type 3 Escape (P. 168, Sec. 7.4.1.3) */
          iErrCode = mp4bitstream_slice_read_bits(pSlice, 21, &nextCode);
          fLast  = (signed char) ((nextCode >> 20) & 0x1);
          m_pRVLC->run   = (signed char)   ((nextCode >> 14) & 0x3F);
          m_pRVLC->level = (signed short) ((nextCode   >> 1) & 0xFFF);
          /* Fixup the sign of level */
          if ( m_pRVLC->level > 2048 )
          {
            m_pRVLC->level = (signed short) ((signed long int)m_pRVLC->level - 4096);
          }
          /* Error checking */
          if ( !((nextCode >> 13) &   0x1) )
          { /* bad message this is not short header */
            return MP4ERROR_ILL_ACTERM_SHORT_HEADER;
          }
          if ( !(nextCode &   0x1) )
          { /* bad message this is not short header */
            return MP4ERROR_ILL_ACTERM_SHORT_HEADER;
          }
          nDCT += m_pRVLC->run+1;
          ++m_pRVLC;
          continue;
        }
      }
    }

    /* Flush the bitstream word by the VLC Code Length                     */
    iErrCode = mp4bitstream_slice_flush_bits(pSlice, CodeLength );
    /* "nextCode" now has the sign bit for the decoded level in bit 0      */
    if ( nextCode & 0x1 )
    {
      m_pRVLC->level = -m_pRVLC->level;
    }

    nDCT += m_pRVLC->run+1;
    ++m_pRVLC;

    if ( iErrCode  || (nDCT > 64) )
    {
      ERR("CMB AC coeff. error %d", iErrCode, 0, 0);
      return MP4ERROR_ILL_RVLC_I;
    }

  } while ( !fLast );   /* AC-terms decoding loop */

  return(MP4ERROR_SUCCESS);

}   /* mp4huffman_decode_AC_terms */

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
MP4_ERROR_TYPE MP4_PAL::h263huffman_decode_AC_terms
(
mp4_slice_type *pSlice,              /* Pointer to slice structure */
unsigned char Intra,                         /* 1 if IVOP, 0 if PVOP */
unsigned char fextendedLevel,              /* if TRUE check for extended level */
unsigned long int nDCT                          /* Number of coefficients so far */
)
{
  unsigned long int nextCode;             /* Trailing bits after VLC decode      */
  MP4_ERROR_TYPE  iErrCode;
  unsigned long int CodeLength;
  signed char fLast = 0;

  do
  {
    iErrCode = mp4huffman_decode_VLC(pSlice, Intra, &fLast, &CodeLength, &nextCode);
    if ( iErrCode != MP4ERROR_SUCCESS )
    {
      return iErrCode;
    }

    if ( fLast == -1 ) //Escape Code
    {
      /* Flush out the escape code (7 bits) */
      iErrCode = mp4bitstream_slice_flush_bits(pSlice, 7);
      /* Short header FLC */
      iErrCode = mp4bitstream_slice_read_bits(pSlice, 15, &nextCode);

      if ( iErrCode ) return iErrCode;
      fLast  = (signed char) ((nextCode >> 14) & 0x1);
      m_pRVLC->run   = (signed char) ((nextCode >>  8) & 0x3F);
      /* Sign extend to 16 bits */
      m_pRVLC->level = (signed short) ((signed char) (nextCode & 0xFF));

      /* Check to see if modified quant is turned on. 
      ** if LUM or Chr quant if >=8 dont use extended Level
      ** since if Lum Quant is < 8 => Chr Quant will be <8 
      ** check only Lum quant
      */ 
      if ( fextendedLevel )
      {
        /* in case of modified quant, this level filed with the bit
        ** sequence == 128 implies that an extended level field of
        ** 11 bits follows
        */
        if ( m_pRVLC->level == 128 /* bit seq 1000 0000 */ )
        {
          unsigned short extended_level = 0;

          iErrCode = mp4bitstream_slice_read_bits
                     (pSlice, 11, &nextCode);

          if ( iErrCode ) return iErrCode;

          /* Extended level is to be interpreted thus:
          ** Take the 2s complement of the original level. Take the 
          ** 11 least significant bits. Rotate cyclically by 5 bits 
          ** to the right.
          */

          /* Step 1: Cyclically rotate Left by 5 bits */
          extended_level |= ( nextCode & 0x003F ) << 5;
          extended_level |= ( nextCode & 0x07C0 ) >> 6;

          /* Step 2: To get real level 2s complement of above value */
          extended_level = (~extended_level | 0x07FF) + 1;
          m_pRVLC->level = (signed short) extended_level;

          /* Constarint 4 Pg. 149 H.263 Annex T */
          if ( extended_level < 128 )
          {
            return MP4ERROR_ILL_ACTERM_SHORT_HEADER;
          }
        }
      }
      else
      {
        /* Check for invalid values */
        if ( m_pRVLC->level == 0 || m_pRVLC->level == 128 )
        {
          return MP4ERROR_ILL_ACTERM_SHORT_HEADER;
        }
      }
    }
    else
    {
      /* Flush the bitstream word by the VLC Code Length                     */
      iErrCode = mp4bitstream_slice_flush_bits(pSlice, CodeLength );
      /* "nextCode" now has the sign bit for the decoded level in bit 0      */
      if ( nextCode & 0x1 )
      {
        m_pRVLC->level = -m_pRVLC->level;
      }
    }
    nDCT += m_pRVLC->run+1;
    ++m_pRVLC;

    if ( iErrCode  || (nDCT > 64) )
    {
      ERR("CMB AC coeff. error %d", iErrCode, 0, 0);
      return MP4ERROR_ILL_RVLC_I;
    }

  } while ( !fLast );   /* AC-terms decoding loop */

  return(MP4ERROR_SUCCESS);

}   /* h263huffman_decode_AC_terms */

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
MP4_ERROR_TYPE MP4_PAL::mp4huffman_decode_RVLC_forward
(
unsigned char         fIntra,        /* 1 for IVOP, 0 for PVOP
                                * based on MB_TYPE in MCBPC */
mp4_slice_type *pSlice,        /* Pointer to slice structure */
unsigned long int nDCT,                    /* Number of coefficients so far */
unsigned char        *pLast          /* TRUE if last */
)
{
  unsigned long int Code;
  unsigned long int Mask;
  unsigned long int Data;
  unsigned long int Length;
  rvlc_last_run_level_type const *pTable;
  int ZPosition = 0;
  int Sign;
  MP4_ERROR_TYPE iErrCode = MP4ERROR_SUCCESS;
  unsigned char fLast;
  pLast = pLast; //Supress warning
  do
  {
    /* Max. RVLC code length, without the Sign bit.*/
    mp4bitstream_slice_show_bits( pSlice, 15, &Code);
    Length = 2;
    Mask = 0x4000;

    if ( Code & Mask )            /* code starts with 1 */
    {
      /* Check for error condition (i.e. no 1 bits) */
      if ( !(Code & (Mask-1)) )
      {
        return(fIntra ? MP4ERROR_ILL_RVLC_I : MP4ERROR_ILL_RVLC_P);
      }
      /* Look for next '1' and count the bits in between
      * Error condition was checked outside the loop above.
      */
      /* Mask >>= 1;
      while( !(Code & Mask) )
      {
      Mask >>= 1;
      Length++;
      }
      Length++; */

      /* Optimize above logic using CLZ's */
      Code ^= Mask;
      Length = TL_CLZ(Code);
      Length -= 15; 

      ZPosition = 0;
    }  /* if ( Code & Mask ) */
    else                    /* code starts with a 0. */
    {
      /* Look for two or more 0s and count the bits in between */
      /* Need to Optimize later using CLZ's                    */
      Mask >>= 1;
      while ( Code & Mask )    /* Look for the first '0' bit */
      {
        Mask >>= 1;
        Length++;
      }

      /* Record the position */
      ZPosition = Length - 1;
      Length++;
      Mask >>= 1;
      while ( Code & Mask )    /* Look for the second '0' bit */
      {
        Mask >>= 1;
        Length++;
      }
      Length++;
    }  /* if ( ! Code & Mask )  */

    /* Make sure that the length is valid */
    if ( Length < 3 || Length > 15 )
    {
      return(fIntra ? MP4ERROR_ILL_RVLC_I : MP4ERROR_ILL_RVLC_P );
    }

    /* Read the appropriate number of bits */
    iErrCode = mp4bitstream_slice_read_bits( pSlice, Length, &Code );
    if ( iErrCode != MP4ERROR_SUCCESS )
    {
      return iErrCode;
    }

    if ( Code == 0 )                                /* ESCAPE*/
    {
      /* Read 8 bits */
      mp4bitstream_slice_show_bits( pSlice, 8, &Data);
      if ( !(Data & 0x80) )
      {
        return(fIntra ? MP4ERROR_MISSING_MARKERBIT_RVLC_I :
               MP4ERROR_MISSING_MARKERBIT_RVLC_P);
      }
      fLast      = ((Data >> 6) & 0x1);
      m_pRVLC->run = (Data & 0x3F);
      /* Flush the 8 bits */
      iErrCode = mp4bitstream_slice_flush_bits( pSlice, 8);
      if ( iErrCode ) return iErrCode;

      /* Read 18 bits */
      mp4bitstream_slice_show_bits( pSlice, 18, &Data);
      if ( !(Data & 0x20000) )
      {
        return(fIntra ? MP4ERROR_MISSING_MARKERBIT_RVLC_I :
               MP4ERROR_MISSING_MARKERBIT_RVLC_P);
      }
      m_pRVLC->level = (signed short) ((Data >> 6) & 0x7FF);
      if ( !m_pRVLC->level )            /* Page422, E.1.4.1.1, item 1 */
      {
        return(fIntra ? MP4ERROR_ILL_RVLC_I : MP4ERROR_ILL_RVLC_P );
      }
      if ( !(Data & 0x20) )
      {
        return(fIntra ? MP4ERROR_MISSING_MARKERBIT_RVLC_I :
               MP4ERROR_MISSING_MARKERBIT_RVLC_P);
      }
      if ( Data & 0x1E )  /* Escape sequence of 0000 */
      {
        return(fIntra ? MP4ERROR_ILL_RVLC_I : MP4ERROR_ILL_RVLC_P );
      }
      Sign = (Data & 0x1);
      /* Flush the 18 bits */
      iErrCode = mp4bitstream_slice_flush_bits( pSlice, 18 );
      if ( iErrCode ) return iErrCode;


    }   /* if ( Code == 0 ) */
    else
    {
      /* form the Indexing mechanism for RVLC*/
      ZPosition = (ZPosition << 1) | (Code & 0x1);
      pTable = &Mp4DecodeRVLC[!fIntra][Length-3][ZPosition];

      /*
      ** Check for holes in the Huffman Table entries; coded as Level = -1.
      ** We have bad Codeword if level is -1.
      */
      if ( pTable->level == -1 )
      {
        return( fIntra ? MP4ERROR_ILL_RVLC_I : MP4ERROR_ILL_RVLC_P );
      }

      m_pRVLC->run   = pTable->run;
      m_pRVLC->level = pTable->level;
      fLast  = pTable->last;

      iErrCode = mp4bitstream_slice_read_bits(pSlice,1, &Code);
      if ( iErrCode != MP4ERROR_SUCCESS )
      {
        return iErrCode;
      }

      Sign = Code;
    }  /* if ( Code != 0 ) */

    if ( Sign == 1 )
    {
      m_pRVLC->level = -m_pRVLC->level;
    }


    nDCT += m_pRVLC->run+1;
    ++m_pRVLC;

    if ( iErrCode  || (nDCT > 64) )
    {
      ERR("RVLC AC coeff. error %d", iErrCode, 0, 0);
      return MP4ERROR_ILL_RVLC_I;
    }

  } while ( !fLast );   /* AC-terms decoding loop */


  return(MP4ERROR_SUCCESS);
}  /* mp4huffman_decode_RVLC_forward */


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
MP4_ERROR_TYPE MP4_PAL::mp4huffman_decode_RVLC_reverse
(
unsigned char fIntra,                /* 1 for IVOP, 0 for PVOP
                                * based on MB_TYPE in MCBPC*/
mp4_slice_type      *pSlice,   /* Pointer to slice structure */
unsigned char        *pLast,         /* TRUE if last */
unsigned long int *pLength                /* Length of decoded data */
)
{
  unsigned long int Code, Mask = 0x0001;
  unsigned long int Length, Count;
  rvlc_last_run_level_type const *pTable;
  int ZPosition = 0, Sign;
  MP4_ERROR_TYPE Error;
  unsigned long int Data;

  Error = mp4bitstream_slice_read_bits_reverse(pSlice, 1, &Data);
  if ( Error )
  {
    return Error;
  }

  Sign = Data;

  *pLength = 1;

  /* Max. RVLC code length, without the Sign bit is 15.*/
  if ( (Error =
        mp4bitstream_slice_show_bits_reverse(pSlice, 15, &Code)) !=
       MP4ERROR_SUCCESS )
  {
    return Error;
  }

  Code >>= 1; /* ignore the last bit for finding length */
  Length = 2;

  if ( Code & Mask )            /* code ends with 1? */
  {
    /* Look for next 1 and count bits */
    Code >>= 1;
    while ( !(Code & Mask) )
    {
      Code >>= 1;
      Length++;
    }
    ZPosition = 0;
    Length++;
  }
  else                             /* code ends with a 0?. */
  {
    /* Look for two or more 0s and count bits */
    Count = 2;
    while ( Count > 0 )
    {
      Code >>= 1;
      if ( !(Code & Mask) )
      {
        if ( Count == 2 )
        {
          ZPosition = Length-1;
        }
        Count--;
      }
      Length++;
      if ( Length >= 16 )    /* Max RVLC Len=16. Prevent run away */
      {
        return( fIntra ? MP4ERROR_ILL_RVLC_I : MP4ERROR_ILL_RVLC_P );
      }
    }
    /* ZPosition needs to be verified here. !!! */
    ZPosition = Length - ZPosition - 2;
  }

  if ( Length < 3 || Length > 15 )
  {
    return(fIntra ? MP4ERROR_ILL_RVLC_I : MP4ERROR_ILL_RVLC_P );
  }

  if ( (Error=mp4bitstream_slice_show_bits_reverse (pSlice, (unsigned char) Length,
                                                    &Code)) != MP4ERROR_SUCCESS )
  {
    return Error;
  }
  if ( (Error=mp4bitstream_slice_flush_bits_reverse (pSlice, (unsigned char) Length)) !=
       MP4ERROR_SUCCESS )
  {
    return Error;
  }

  *pLength += Length;

  if ( !Code )            /* ESCAPE. Check for Length == 4 also here */
  {
    if ( (Error = mp4bitstream_slice_read_bits_reverse 
          (pSlice, 1, &Data)) !=
         MP4ERROR_SUCCESS )
    {
      return Error;
    }
    if ( !Data )
    {
      return(fIntra ? MP4ERROR_MISSING_MARKERBIT_RVLC_I :
             MP4ERROR_MISSING_MARKERBIT_RVLC_P );
    }

    if ( (Error = mp4bitstream_slice_read_bits_reverse
          (pSlice,11, &Data)) !=
         MP4ERROR_SUCCESS )
    {
      return Error;
    }

    m_pRVLC->level = (char) Data;
    if ( !m_pRVLC->level )            /* Page422, E.1.4.1.1, item 1 */
    {
      return(fIntra ? MP4ERROR_ILL_RVLC_I : MP4ERROR_ILL_RVLC_P );
    }

    if ( (Error=mp4bitstream_slice_read_bits_reverse
          (pSlice, 1, &Data)) !=
         MP4ERROR_SUCCESS )
    {
      return Error;
    }
    if ( !Data )
    {
      return(fIntra ? MP4ERROR_MISSING_MARKERBIT_RVLC_I :
             MP4ERROR_MISSING_MARKERBIT_RVLC_P );
    }

    if ( (Error=mp4bitstream_slice_read_bits_reverse(pSlice,6, &Data)) !=
         MP4ERROR_SUCCESS )
    {
      return Error;
    }
    m_pRVLC->run = (char) Data;

    if ( (Error=mp4bitstream_slice_read_bits_reverse(pSlice,1, &Data)) !=
         MP4ERROR_SUCCESS )
    {
      return Error;
    }
    *pLast = (unsigned char) Data;

    if ( (Error=mp4bitstream_slice_read_bits_reverse
          (pSlice, 5, &Data)) !=
         MP4ERROR_SUCCESS )
    {
      return Error;
    }
    if ( !Data )
    {
      return(fIntra ? MP4ERROR_MISSING_MARKERBIT_RVLC_I :
             MP4ERROR_MISSING_MARKERBIT_RVLC_P );
    }

    *pLength += 25;
  }
  else
  {
    /* form the Indexing mechanism for RVLC*/
    ZPosition = (ZPosition << 1) | (Code & 1);

    pTable = &Mp4DecodeRVLC[!fIntra][Length-3][ZPosition];

    m_pRVLC->run   = pTable->run;
    m_pRVLC->level = pTable->level;
    *pLast  = pTable->last;
    if ( Sign == 1 )
    {
      m_pRVLC->level = -m_pRVLC->level;
    }
    /*
    ** Check for holes in the Huffman Table entries; coded as Level = -1.
    ** We have bad Codeword if level is -1.
    */
    if ( pTable->level == -1 )
    {
      return( fIntra ? MP4ERROR_ILL_RVLC_I : MP4ERROR_ILL_RVLC_P );
    }
  }
  return(MP4ERROR_SUCCESS);
}  /* mp4huffman_decode_RVLC_reverse */

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
unsigned char MP4_PAL::IsSliceBufAvailableForDecode()
{
  unsigned char sliceBufAvailable = FALSE;
  VDL_IsSliceBufAvailableForDecode( m_pMP4DL, &sliceBufAvailable );
  return sliceBufAvailable;
}

VDL_ERROR MP4_PAL::QueueEOSStatsBuffer( DecodeStatsType* pDecoderStatsEOS)
{
	return VDL_Queue_Stats_Buffer( m_pMP4DL,(void*) pDecoderStatsEOS, VDL_END_OF_SEQUENCE );
}

/*============================================================================
  FILE: OMX_Venc.cpp
  
  OVERVIEW: Mpeg4 video encoder component class. Interfaces with device to 
  perform Mpeg4 encoding capabilities. Component uses a single thread to
  process commands from the client. Thread also processes status and 
  frame callbacks from the device layer.
  
                       Copyright (c) 2008 QUALCOMM Incorporated.
                                All Rights Reserved.
                          Qualcomm Confidential and Proprietary
============================================================================*/

/*============================================================================
  EDIT HISTORY FOR MODULE
  
  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order. Please
  use ISO format for dates.
  
  $Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/omx/src/OMX_Venc.cpp#9 $$DateTime: 2010/04/19 11:39:16 $$Author: c_ashish $
  
  when        who  what, where, why
  ----------  ---  -----------------------------------------------------------
  2010-04-02  as   Added support for TimeStamp scale factor.
  2010-03-22  as   Added OMX extension support
  2010-02-18  as   Added Profile and level as per resolution & changed level
                   for some debug messages.
  2010-02-18  as   Added support for OMX_IndexConfigVideoIntraVOPRefresh
  2009-10-02  as   Added support for change quality,intra refresh,intra period
                   out of band vol.
  2009-09-28  as   Fixed qp range for Vt pass and changed some debug msg level
  2009-09-28  as   Fixed for use buffer and allocate buffer if offset is non-zero
  2009-08-27  bc   Fixed OMX IL state machine bugs for Khronos compliance
  2009-08-27  bc   Added OMX IL support for zero sized input frames with EOS flag set
  2009-08-24  bc   On-target integration updates
  2009-08-13  bc   Resolved action items from code reviews 61288 and 61438
  2009-08-13  bc   cleanup
  2009-08-13  bc   OMX component failure case bug fixes
  2009-05-21  as   Changes some debug message &condition in use buffer
  2009-04-20  as   Adding support for OMX_IndexParamStandardComponentRole
  2009-04-20  as   checking portindex in get parameter & 
                   Add no support for OMX_IndexConfigCommonRotate 
  2009-04-09   as  Typo error pInput instead of pBuffer 
  2009-04-05   as  sync with andriod release 
  2008-12-29    ak   Added bobby's config file parser instead of the 
                   command line arguments. Added Rotation support, MP4 
                   error Correction H263_GOB, H263_SLICE minor bug fixes.
  2008-12-19  bc   Error checking enhancements
  2008-12-15  bc   Updates for single thread design
  2008-12-04  bc   Cleaned up diag initialization
  2008-08-11  bc   Fixed multisession bug where number of sessions 
                   is greater than 1
  2008-08-11  bc   Warning removal
  2008-08-14  bc   Fixed single-session limitation. 
                   Fixed pmem buffer deallocation crash
                   Added support for OMX_BUFFERFLAG_EOS flag.
                   Release all free buffers in IDLE state transition.
  2008-07-15  bc   Buffer management bug fix
  2008-07-15  bc   Added support for h263
  2008-07-08  bc   Aligned to new OMX core
  2008-06-13  bc   Fixed bitstream buffer overflow for i-frame
  2008-06-12  bc   Bug fixes device layer configuration.
  2008-06-11  bc   Bug fixes for initial target integration.
  2008-06-03  bc   Created file.
  
============================================================================*/

/*----------------------------------------------------------------------------
* Include Files
* -------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "OMX_Venc.h"
#include "OMX_VencDebug.h"
#include "OMX_VencBufferManager.h"
#include "OMX_VencMsgQ.h"
#include "venc_dev_api.h"
#include "venc_semaphore.h"
#include "venc_thread.h"
#include "venc_signal.h"
#include "qc_omx_common.h"
#include "venc_file.h"

#define LOG_TAG "VENC_OMX"
/*----------------------------------------------------------------------------
* Preprocessor Definitions and Constants
* -------------------------------------------------------------------------*/

#define OMX_SPEC_VERSION 0x00000101
#define OMX_INIT_STRUCT(_s_, _name_)            \
   memset((_s_), 0x0, sizeof(_name_));          \
   (_s_)->nSize = sizeof(_name_);               \
   (_s_)->nVersion.nVersion = OMX_SPEC_VERSION


#define BITSTREAM_BUFF_SIZE(nBitrate, xFramerate) (4*8 * ((nBitrate) / 8) / ((xFramerate) >> 16))
// BitMask Management logic
#define BITS_SHIFT(mIndex) 1 << mIndex
#define BITMASK_SET(mArray,mIndex) (mArray |= BITS_SHIFT(mIndex))
#define BITMASK_CLEAR(mArray,mIndex) (mArray &= (OMX_U32) ~BITS_SHIFT(mIndex))
#define BITMASK_PRESENT(mArray,mIndex) (mArray & BITS_SHIFT(mIndex))

static const char pRoleMPEG4[] = "video_encoder.mpeg4";
static const char pRoleH263[] = "video_encoder.h263";
static const char pRoleAVC[] = "video_encoder.avc";


/*----------------------------------------------------------------------------
* Type Declarations
* -------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
* Global Data Definitions
* -------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
* Static Variable Definitions
* -------------------------------------------------------------------------*/
Venc* Venc::g_pVencInstance = NULL;

/*----------------------------------------------------------------------------
* Static Function Declarations and Definitions
* -------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
* Externalized Function Definitions
* -------------------------------------------------------------------------*/
extern "C" {
void* get_omx_component_factory_fn(void)
{
   return Venc::get_instance();
}
}
int roundingup( double val )
{
   int ret = (int) val;
   val -= ret;
   if(val >= 0.5)
      return (int) ++ret;
   else
      return ret;
}

typedef struct OMXComponentCapabilityFlagsType
{
    ////////////////// OMX COMPONENT CAPABILITY RELATED MEMBERS
    OMX_BOOL iIsOMXComponentMultiThreaded;
    OMX_BOOL iOMXComponentSupportsExternalOutputBufferAlloc;
    OMX_BOOL iOMXComponentSupportsExternalInputBufferAlloc;
    OMX_BOOL iOMXComponentSupportsMovableInputBuffers;
    OMX_BOOL iOMXComponentSupportsPartialFrames;
    OMX_BOOL iOMXComponentUsesNALStartCodes;
    OMX_BOOL iOMXComponentCanHandleIncompleteFrames;
    OMX_BOOL iOMXComponentUsesFullAVCFrames;

} OMXComponentCapabilityFlagsType;
#define OMX_COMPONENT_CAPABILITY_TYPE_INDEX 0xFF7A347

///////////////////////////////////////////////////////////////////////////////
// Refer to the header file for detailed method descriptions
///////////////////////////////////////////////////////////////////////////////
Venc::Venc() :
   m_thread(NULL),
   m_cmd_sem(NULL),
   m_loadUnloadSignal(NULL),
   m_bFlushingIndex_in(OMX_FALSE),
   m_bFlushingIndex_out(OMX_FALSE),
   m_pFreeInBM(NULL),
   m_pFreeOutBM(NULL),
   m_pMsgQ(NULL),
   m_nDLHandle(VENC_INVAL_HANDLE),
   m_eState(OMX_StateInvalid),
   m_pAppData(NULL),
   m_hSelf(NULL),
   m_bPanic(OMX_FALSE),
   m_bIsQcomPvt(OMX_FALSE),
   m_pComponentName(NULL),
   m_sInBuffHeaders(NULL),
   m_sOutBuffHeaders(NULL),
   m_sPrivateInPortData(NULL),
   m_sPrivateOutPortData(NULL),
   m_finput(NULL),
   m_foutput(NULL),
   m_bDeviceInit_done(OMX_FALSE),
   m_bDeviceStart(OMX_FALSE),
   m_bDeviceExit_done(OMX_FALSE),
   m_flags(0),
   m_in_alloc_cnt(0),
   m_out_alloc_cnt(0),
   m_nInputDown(0),
   m_nOutputDown(0),
   m_Mark_command(OMX_FALSE),
   m_Mark_port(OMX_FALSE),
   m_bGetSyntaxHdr(OMX_TRUE),
   m_bComponentInitialized(OMX_FALSE),
   m_bPendingChangeQuality(OMX_FALSE)
{
   OMX_VENC_MSG_HIGH("OMX-ENC:-constructor", 0, 0, 0);
   if (venc_semaphore_create(&m_cmd_sem, 0, 1) != 0)
   {
      OMX_VENC_MSG_ERROR("error creating semaphore", 0, 0, 0);
   }
   if (venc_signal_create(&m_loadUnloadSignal) != 0)
   {
      OMX_VENC_MSG_ERROR("error creating signal", 0, 0, 0);
   }
   memset(&m_sPrivateInPortData, 0, sizeof(m_sPrivateInPortData));
   memset(&m_sPrivateOutPortData, 0, sizeof(m_sPrivateOutPortData));
   memset(&m_sErrorCorrection, 0, sizeof(m_sErrorCorrection));   
}

Venc::~Venc()
{
   //since we don't have release instance need to set it to null in the destructor
   (void) venc_semaphore_destroy(m_cmd_sem);
   (void) venc_signal_destroy(m_loadUnloadSignal);
   g_pVencInstance = NULL;
   OMX_VENC_MSG_HIGH("OMX-ENC:-destructor", 0, 0, 0);
}

void Venc:: mem_dump(char* data, int n_bytes,int input)
{
	if(input)
{
		venc_file_write(m_finput, (void*) data, (int) n_bytes);
}
	else
{
	   venc_file_write(m_foutput, (void*) data, (int) n_bytes);
		
}
}

/**********************************************************************//**
 * @brief Release all input and output buffers
 *
 *************************************************************************/
void Venc::free_port_inout()
{
   int  i;
   for (i = 0; i < (int) m_sOutPortDef.nBufferCountActual; i++)
   {
      OMX_VENC_MSG_LOW("free output buffer", 0, 0, 0);
      if (m_sOutBuffHeaders[i].pBuffer)
      {
         if(OMX_FreeBuffer(m_hSelf,
                           PORT_INDEX_OUT,
                           &m_sOutBuffHeaders[i]) != OMX_ErrorNone)
         {
            OMX_VENC_MSG_ERROR("Error OMX_FreeBuffer failed %d",i, 0, 0);
         }
         else
         {
            OMX_VENC_MSG_ERROR("buffer %d is null", i, 0, 0);
         }
      }
   }
   
   for (i = 0; i < (int) m_sInPortDef.nBufferCountActual; i++)
   {
      OMX_VENC_MSG_LOW("free input buffer", 0, 0, 0);
      if (m_sInBuffHeaders[i].pBuffer)
      {
         if(OMX_FreeBuffer(m_hSelf,
                           PORT_INDEX_IN,
                           &m_sInBuffHeaders[i]) != OMX_ErrorNone)
         {
            OMX_VENC_MSG_ERROR("Error OMX_FreeBuffer failed %d",i, 0, 0);
         }
      }
      else
      {
         OMX_VENC_MSG_ERROR("buffer %d is null", i, 0, 0);
      }
   }
}

OMX_ERRORTYPE Venc::component_init(OMX_IN OMX_STRING pComponentName)
{
   OMX_ERRORTYPE result = OMX_ErrorNone;
   OMX_BOOL bThread = OMX_FALSE;
   OMX_VIDEO_CODINGTYPE eCodec;
   int nNameLen = (int) strlen(pComponentName);

   OMX_VENC_MSG_LOW("initializing component", 0, 0, 0);

   if (pComponentName && nNameLen < OMX_MAX_STRINGNAME_SIZE)
   {
      if (strcmp(pComponentName, "OMX.qcom.video.encoder.mpeg4") == 0)
      {
         m_pComponentName = (OMX_STRING) malloc(OMX_MAX_STRINGNAME_SIZE);
         memcpy(m_cRole, pRoleMPEG4, strlen(pRoleMPEG4) + 1);
         memcpy(m_pComponentName, pComponentName, nNameLen);
         eCodec =  OMX_VIDEO_CodingMPEG4;
         OMX_VENC_MSG_HIGH("Initializing component OMX.qcom.video.encoder.mpeg4", 0, 0, 0);
      }
      else if (strcmp(pComponentName, "OMX.qcom.video.encoder.h263") == 0)
      {
         m_pComponentName = (OMX_STRING) malloc(OMX_MAX_STRINGNAME_SIZE);
         memcpy(m_cRole, pRoleH263, strlen(pRoleH263) + 1);
         memcpy(m_pComponentName, pComponentName, nNameLen);
         eCodec =  OMX_VIDEO_CodingH263;
         OMX_VENC_MSG_HIGH("Initializing component OMX.qcom.video.encoder.h263", 0, 0, 0);
      }
      else if (strcmp(pComponentName, "OMX.qcom.video.encoder.avc") == 0)
      {
         m_pComponentName = (OMX_STRING) malloc(OMX_MAX_STRINGNAME_SIZE);
         memcpy(m_cRole, pRoleAVC, strlen(pRoleAVC) + 1);
         memcpy(m_pComponentName, pComponentName, nNameLen);
         eCodec =  OMX_VIDEO_CodingAVC;
         OMX_VENC_MSG_HIGH("Initializing component OMX.qcom.video.encoder.avc",0,0,0);
      }
      else
      {
         return OMX_ErrorBadParameter;
      }
   }
   else
   {
      return OMX_ErrorBadParameter;
   }

   OMX_VENC_MSG_LOW("Initializing component", 0, 0, 0);
   /// allocate message queue
   m_pMsgQ = new VencMsgQ;
   if (m_pMsgQ == NULL)
   {
      OMX_VENC_MSG_ERROR("failed to allocate message queue", 0, 0, 0);
      result = OMX_ErrorInsufficientResources;
      goto bail;
   }
   m_eState = OMX_StateInvalid;

   memset(&m_sCallbacks,0,sizeof(OMX_CALLBACKTYPE));

   OMX_INIT_STRUCT(&m_sPortParam, OMX_PORT_PARAM_TYPE);
   m_sPortParam.nPorts = 0x2;
   m_sPortParam.nStartPortNumber = (OMX_U32) PORT_INDEX_IN;
   
   OMX_INIT_STRUCT(&m_sPortParam_audio, OMX_PORT_PARAM_TYPE);
   m_sPortParam_audio.nPorts = 0;
   m_sPortParam_audio.nStartPortNumber = 0;

   OMX_INIT_STRUCT(&m_sPortParam_img, OMX_PORT_PARAM_TYPE);
   m_sPortParam_img.nPorts = 0;
   m_sPortParam_img.nStartPortNumber = 0;

   OMX_INIT_STRUCT(&m_sParamBitrate, OMX_VIDEO_PARAM_BITRATETYPE);
   m_sParamBitrate.nPortIndex = (OMX_U32) PORT_INDEX_OUT;
   m_sParamBitrate.eControlRate = OMX_Video_ControlRateVariable; // no frame skip
   m_sParamBitrate.nTargetBitrate = 64000;

   OMX_INIT_STRUCT(&m_sConfigEncoderModeType,QOMX_VIDEO_PARAM_ENCODERMODETYPE);
   m_sConfigEncoderModeType.nMode = QOMX_VIDEO_EncoderModeDefault;
   
   #ifndef FEATURE_LINUX_A
   OMX_INIT_STRUCT(&m_sTimeStampScale,QOMX_TIMESTAMPSCALETYPE);
   m_sTimeStampScale.nTimestampScale = 1 << 16;           
   #endif
   
   OMX_INIT_STRUCT(&m_sConfigBitrate, OMX_VIDEO_CONFIG_BITRATETYPE);
   m_sConfigBitrate.nPortIndex = (OMX_U32) PORT_INDEX_OUT;
   m_sConfigBitrate.nEncodeBitrate = 64000;

   OMX_INIT_STRUCT(&m_sConfigFramerate, OMX_CONFIG_FRAMERATETYPE);
   m_sConfigFramerate.nPortIndex = (OMX_U32) PORT_INDEX_OUT;
   m_sConfigFramerate.xEncodeFramerate = 15 << 16;

   OMX_INIT_STRUCT(&m_sConfigIntraRefreshVOP, OMX_CONFIG_INTRAREFRESHVOPTYPE);
   m_sConfigIntraRefreshVOP.nPortIndex = (OMX_U32) PORT_INDEX_OUT;
   m_sConfigIntraRefreshVOP.IntraRefreshVOP = OMX_FALSE;

   OMX_INIT_STRUCT(&m_sParamProfileLevel, OMX_VIDEO_PARAM_PROFILELEVELTYPE);
   m_sParamProfileLevel.nPortIndex = (OMX_U32) PORT_INDEX_OUT;
   switch (eCodec)
   {
      case OMX_VIDEO_CodingMPEG4 :   
         m_sParamProfileLevel.eProfile = (OMX_U32) OMX_VIDEO_MPEG4ProfileSimple;
         m_sParamProfileLevel.eLevel = (OMX_U32) OMX_VIDEO_MPEG4Level0;
         break;   
      case OMX_VIDEO_CodingAVC :  
         m_sParamProfileLevel.eProfile = (OMX_U32)OMX_VIDEO_AVCProfileBaseline; 
         m_sParamProfileLevel.eLevel = (OMX_U32)OMX_VIDEO_AVCLevel1 ;
         break;   
      case OMX_VIDEO_CodingH263 :
         m_sParamProfileLevel.eProfile = (OMX_U32) OMX_VIDEO_H263ProfileBaseline;
         m_sParamProfileLevel.eLevel = (OMX_U32) OMX_VIDEO_H263Level10;
         break;
      default:
         OMX_VENC_MSG_ERROR("Error in CodecType", 0, 0, 0);
         break;
   }

   // Initialize the video parameters for input port 
   OMX_INIT_STRUCT(&m_sInPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
   m_sInPortDef.nPortIndex = (OMX_U32) PORT_INDEX_IN;
   m_sInPortDef.bEnabled = OMX_TRUE;
   m_sInPortDef.bPopulated = OMX_FALSE;
   m_sInPortDef.eDomain = OMX_PortDomainVideo;
   m_sInPortDef.eDir = OMX_DirInput;
   m_sInPortDef.nBufferCountMin = MIN_IN_BUFFERS;
   m_sInPortDef.nBufferCountActual = MIN_IN_BUFFERS;
   m_sInPortDef.nBufferSize =  (OMX_U32)(176*144*3/2);
   m_sInPortDef.format.video.cMIMEType = "YUV420";
   m_sInPortDef.format.video.nFrameWidth = 176;
   m_sInPortDef.format.video.nFrameHeight = 144;
   m_sInPortDef.format.video.nBitrate = 64000;
   m_sInPortDef.format.video.xFramerate = 15 << 16;
   m_sInPortDef.format.video.eColorFormat =  OMX_COLOR_FormatYUV420SemiPlanar;
   m_sInPortDef.format.video.eCompressionFormat =  OMX_VIDEO_CodingUnused;

   // Initialize the video parameters for output port
   OMX_INIT_STRUCT(&m_sOutPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
   m_sOutPortDef.nPortIndex = (OMX_U32) PORT_INDEX_OUT;
   m_sOutPortDef.bEnabled = OMX_TRUE;
   m_sOutPortDef.bPopulated = OMX_FALSE;
   m_sOutPortDef.eDomain = OMX_PortDomainVideo;
   m_sOutPortDef.eDir = OMX_DirOutput;
   m_sOutPortDef.nBufferCountMin = MIN_OUT_BUFFERS; 
   m_sOutPortDef.nBufferCountActual = MIN_OUT_BUFFERS;

   m_sOutPortDef.nBufferSize = BITSTREAM_BUFF_SIZE(m_sConfigBitrate.nEncodeBitrate,
                                                   m_sConfigFramerate.xEncodeFramerate);
   OMX_VENC_MSG_LOW("Default output buffer size %ld",m_sOutPortDef.nBufferSize, 0, 0);
   switch (eCodec)
   {
      case OMX_VIDEO_CodingMPEG4: 
         m_sOutPortDef.format.video.cMIMEType = "MPEG4";
         break;
      case OMX_VIDEO_CodingAVC:
         m_sOutPortDef.format.video.cMIMEType = "H264";
         break;
      case OMX_VIDEO_CodingH263:
         m_sOutPortDef.format.video.cMIMEType = "H263";
         break;
   }
      m_sOutPortDef.format.video.cMIMEType = "H263";
   m_sOutPortDef.format.video.nFrameWidth = 176;
   m_sOutPortDef.format.video.nFrameHeight = 144;
   m_sOutPortDef.format.video.nBitrate = 64000;
   m_sOutPortDef.format.video.xFramerate = 15 << 16;
   m_sOutPortDef.format.video.eColorFormat =  OMX_COLOR_FormatUnused;
   switch (eCodec)
   {
      case OMX_VIDEO_CodingMPEG4: 
         m_sOutPortDef.format.video.eCompressionFormat =  OMX_VIDEO_CodingMPEG4;
         break;
      case OMX_VIDEO_CodingAVC:
         m_sOutPortDef.format.video.eCompressionFormat =  OMX_VIDEO_CodingAVC;
         break;
      case OMX_VIDEO_CodingH263:
         m_sOutPortDef.format.video.eCompressionFormat =  OMX_VIDEO_CodingH263;
         break;
   }

   // Initialize video parameters for input qcom port 
   OMX_INIT_STRUCT(&m_sInQcomPortDef, OMX_QCOM_PARAM_PORTDEFINITIONTYPE);
   m_sInQcomPortDef.nPortIndex = (OMX_U32) PORT_INDEX_IN;
   m_sInQcomPortDef.nMemRegion = OMX_QCOM_MemRegionEBI1;
   m_sInQcomPortDef.nCacheAttr = OMX_QCOM_CacheAttrNone;
   m_sInQcomPortDef.nFramePackingFormat = 3;

   OMX_INIT_STRUCT(&m_sQcomPlatformPvt, OMX_QCOM_PLATFORMPRIVATE_EXTN);
   m_sQcomPlatformPvt.nPortIndex = (OMX_U32) PORT_INDEX_IN;
   m_sQcomPlatformPvt.type = OMX_QCOM_PLATFORM_PRIVATE_PMEM;

   // Initialize the video color format for input port 
   OMX_INIT_STRUCT(&m_sInPortFormat, OMX_VIDEO_PARAM_PORTFORMATTYPE);
   m_sInPortFormat.nPortIndex = (OMX_U32) PORT_INDEX_IN;
   m_sInPortFormat.nIndex = 0;
   m_sInPortFormat.eColorFormat =  OMX_COLOR_FormatYUV420SemiPlanar;
   m_sInPortFormat.eCompressionFormat = OMX_VIDEO_CodingUnused;

   // Initialize the compression format for output port
   OMX_INIT_STRUCT(&m_sOutPortFormat, OMX_VIDEO_PARAM_PORTFORMATTYPE);
   m_sOutPortFormat.nPortIndex = (OMX_U32) PORT_INDEX_OUT;
   m_sOutPortFormat.nIndex = 0;
   m_sOutPortFormat.eColorFormat = OMX_COLOR_FormatUnused;
   switch (eCodec)
   {
      case OMX_VIDEO_CodingMPEG4: 
         m_sOutPortFormat.eCompressionFormat =  OMX_VIDEO_CodingMPEG4;
         break;
      case OMX_VIDEO_CodingAVC:
         m_sOutPortFormat.eCompressionFormat =  OMX_VIDEO_CodingAVC;
         break;
      case OMX_VIDEO_CodingH263:
         m_sOutPortFormat.eCompressionFormat =  OMX_VIDEO_CodingH263;
         break;
   }

   OMX_INIT_STRUCT(&m_sPriorityMgmt, OMX_PRIORITYMGMTTYPE);

   OMX_INIT_STRUCT(&m_sInBufSupplier, OMX_PARAM_BUFFERSUPPLIERTYPE);
   m_sInBufSupplier.nPortIndex = (OMX_U32) PORT_INDEX_IN;

   OMX_INIT_STRUCT(&m_sOutBufSupplier, OMX_PARAM_BUFFERSUPPLIERTYPE);
   m_sOutBufSupplier.nPortIndex = (OMX_U32) PORT_INDEX_OUT;

   // mp4 specific init
   OMX_INIT_STRUCT(&m_sParamMPEG4, OMX_VIDEO_PARAM_MPEG4TYPE);
   m_sParamMPEG4.nPortIndex = (OMX_U32) PORT_INDEX_OUT;
   m_sParamMPEG4.eProfile = OMX_VIDEO_MPEG4ProfileSimple;
   m_sParamMPEG4.eLevel = OMX_VIDEO_MPEG4Level0;
   m_sParamMPEG4.nSliceHeaderSpacing = 0;
   m_sParamMPEG4.bSVH = OMX_FALSE;
   m_sParamMPEG4.bGov = OMX_FALSE;
   m_sParamMPEG4.nPFrames = 29; // 2 second intra period for default 15 fps
   m_sParamMPEG4.bACPred = OMX_TRUE;
   m_sParamMPEG4.nTimeIncRes = 30; // delta = 2 @ 15 fps
   m_sParamMPEG4.nAllowedPictureTypes = 2; // pframe and iframe
   
   m_sParamMPEG4.nHeaderExtension = 1; // number of video packet headers per vop
   m_sParamMPEG4.bReversibleVLC = OMX_FALSE;

   // h263 specific init
   OMX_INIT_STRUCT(&m_sParamH263, OMX_VIDEO_PARAM_H263TYPE);
   m_sParamH263.nPortIndex = (OMX_U32) PORT_INDEX_OUT;
   m_sParamH263.nPFrames = 29;
   m_sParamH263.nBFrames = 0;
   m_sParamH263.eProfile = OMX_VIDEO_H263ProfileBaseline;
   m_sParamH263.eLevel = OMX_VIDEO_H263Level10;
   m_sParamH263.bPLUSPTYPEAllowed = OMX_FALSE;
   m_sParamH263.nAllowedPictureTypes = 2;
   m_sParamH263.bForceRoundingTypeToZero = OMX_TRUE;
   m_sParamH263.nPictureHeaderRepetition = 0;
   m_sParamH263.nGOBHeaderInterval = 0;

  // h264 specific init
   OMX_INIT_STRUCT(&m_sParamAVC, OMX_VIDEO_PARAM_AVCTYPE);
   m_sParamAVC.nPortIndex = (OMX_U32) PORT_INDEX_OUT;
   m_sParamAVC.nSliceHeaderSpacing = 0;
   m_sParamAVC.nPFrames = 29;
   m_sParamAVC.nBFrames = 0;
   m_sParamAVC.eProfile = OMX_VIDEO_AVCProfileBaseline;
   m_sParamAVC.eLevel = OMX_VIDEO_AVCLevel1;
   m_sParamAVC.bEntropyCodingCABAC = OMX_FALSE;
   m_sParamAVC.nAllowedPictureTypes = 2;

   // QPs
   OMX_INIT_STRUCT(&m_sParamQPs, OMX_VIDEO_PARAM_QUANTIZATIONTYPE);
   m_sParamQPs.nPortIndex = (OMX_U32) PORT_INDEX_OUT;
   switch (eCodec)
   {
      case OMX_VIDEO_CodingMPEG4:
      case OMX_VIDEO_CodingH263:
         m_sParamQPs.nQpI = 10;
         m_sParamQPs.nQpP = 10;
         m_sParamQPs.nQpB = 0;
         break;
      case OMX_VIDEO_CodingAVC:
         m_sParamQPs.nQpI = 30;
         m_sParamQPs.nQpP = 30;
         m_sParamQPs.nQpB = 0;
         break;
   }
//Initialize the quality parameter
   	quality.target_bitrate = m_sOutPortDef.format.video.nBitrate;
	quality.max_frame_rate = m_sOutPortDef.format.video.xFramerate >> 16;
	quality.scaling_factor = 0 ;
// Initialize the min/max QP
   switch (eCodec)
   {
      case OMX_VIDEO_CodingMPEG4:
      case OMX_VIDEO_CodingH263:
         quality.min_qp = 2;
         quality.max_qp  = 31;
         break;
      case OMX_VIDEO_CodingAVC:
         quality.min_qp = 2;
         quality.max_qp  = 51;
         break;
   }

   // Intra refresh
   OMX_INIT_STRUCT(&m_sParamIntraRefresh, OMX_VIDEO_PARAM_INTRAREFRESHTYPE);
   m_sParamIntraRefresh.nPortIndex = (OMX_U32) PORT_INDEX_OUT;
   m_sParamIntraRefresh.eRefreshMode = OMX_VIDEO_IntraRefreshMax;
   m_sParamIntraRefresh.nAirMBs = 0;
   m_sParamIntraRefresh.nAirRef = 0;
   m_sParamIntraRefresh.nCirMBs = 0;

   // Error resilience
   OMX_INIT_STRUCT(&m_sErrorCorrection, OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE);
   m_sErrorCorrection.nPortIndex = (OMX_U32) PORT_INDEX_OUT;
   m_sErrorCorrection.bEnableDataPartitioning = OMX_FALSE;
   m_sErrorCorrection.bEnableHEC = OMX_FALSE;
   m_sErrorCorrection.bEnableResync = OMX_FALSE;
   m_sErrorCorrection.bEnableRVLC = OMX_FALSE;
   m_sErrorCorrection.nResynchMarkerSpacing = 0;

   /// free buffer managers
   OMX_VENC_MSG_LOW("creating free output bm", 0, 0, 0);
   m_pFreeOutBM = new VencBufferManager(&result);
   if (result != OMX_ErrorNone)
   {
      OMX_VENC_MSG_ERROR("failed to create free output buffer manager", 0, 0, 0);
      goto bail;
   }
   else if (m_pFreeOutBM == NULL)
   {
      OMX_VENC_MSG_ERROR("failed to create free output buffer manager", 0, 0, 0);
      result = OMX_ErrorInsufficientResources;
      goto bail;
   }

   m_pFreeInBM = new VencBufferManager(&result);
   if (result != OMX_ErrorNone)
   {
      OMX_VENC_MSG_ERROR("failed to create free input buffer manager", 0, 0, 0);
      goto bail;
   }
   else if (m_pFreeInBM == NULL)
   {
      OMX_VENC_MSG_ERROR("failed to create free input buffer manager", 0, 0, 0);
      result = OMX_ErrorInsufficientResources;
      goto bail;
   }
   
   OMX_VENC_MSG_LOW("creating thread", 0, 0, 0);
   if (venc_thread_create(&m_thread,
                          component_thread,
                          (void*) this,
                          0) != 0)
   {
      OMX_VENC_MSG_ERROR("failed to create thread", 0, 0, 0);
      result = OMX_ErrorInsufficientResources;
      goto bail;
   }
   else
   {
      bThread = OMX_TRUE;
   }

   // kick off the device layer threads
   venc_sys_up(NULL);

   if (venc_load(DL_status_callback, (void*) this) != VENC_STATUS_SUCCESS)
   {
      OMX_VENC_MSG_ERROR("failed to load device layer", 0, 0, 0);
      result = OMX_ErrorUndefined;
   }
   else
   {
      // wait for two seconds
      (void) venc_signal_wait(m_loadUnloadSignal, 2000);

      if (m_eState != OMX_StateLoaded)
      {
         OMX_VENC_MSG_ERROR("failed to go to loaded state", 0, 0, 0);
         goto bail;
      }
      else
      {
      OMX_VENC_MSG_LOW("we are now in the OMX_StateLoaded state", 0, 0, 0);
      }
   }

   if (result == OMX_ErrorNone)
   {
      m_bComponentInitialized = OMX_TRUE;
   }
   else
   {
      m_bComponentInitialized = OMX_FALSE;
   }

   return result;

bail:
   if (bThread)
   {
      if (m_pMsgQ->PushMsg(VencMsgQ::MSG_ID_EXIT, NULL) == OMX_ErrorNone)
      {
         if (venc_thread_destroy(m_thread, NULL) != 0)
         {
            OMX_VENC_MSG_ERROR("failed to destroy thread", 0, 0, 0);
         }
      }
      else
      {
         OMX_VENC_MSG_ERROR("failed to send thread exit msg", 0, 0, 0);
      }
      
   }
   if (m_pMsgQ != NULL)
   {
      delete m_pMsgQ;
   }
   if (m_pFreeOutBM)
   {
      delete m_pFreeOutBM;
   }
   if (m_pFreeInBM)
   {
      delete m_pFreeInBM;
   }
   if (m_pComponentName)
   {
      free(m_pComponentName);
   }
   return result;
}

Venc* Venc::get_instance()
{
   if (g_pVencInstance) 
   {
      OMX_VENC_MSG_ERROR("Singleton Class can't created more than one instance", 0, 0, 0);
      return NULL;
   }
   g_pVencInstance = new Venc();
   return g_pVencInstance;
}

OMX_ERRORTYPE Venc::get_component_version(OMX_IN  OMX_HANDLETYPE hComponent,
                                          OMX_OUT OMX_STRING pComponentName,
                                          OMX_OUT OMX_VERSIONTYPE* pComponentVersion,
                                          OMX_OUT OMX_VERSIONTYPE* pSpecVersion,
                                          OMX_OUT OMX_UUIDTYPE* pComponentUUID)
{
   if (hComponent == NULL ||
       pComponentName == NULL || 
       pComponentVersion == NULL ||
       pSpecVersion == NULL || 
       pComponentUUID == NULL)
   {
      return OMX_ErrorBadParameter;
   }

   memcpy(pComponentName, m_pComponentName, OMX_MAX_STRINGNAME_SIZE);
   pSpecVersion->nVersion = OMX_SPEC_VERSION;
   return OMX_ErrorNone;
}
OMX_ERRORTYPE Venc::send_command(OMX_IN  OMX_HANDLETYPE hComponent,
                                 OMX_IN  OMX_COMMANDTYPE Cmd,
                                 OMX_IN  OMX_U32 nParam1,
                                 OMX_IN  OMX_PTR pCmdData)
{

   VencMsgQ::MsgIdType msgId;
   VencMsgQ::MsgDataType msgData;
   
   if(m_eState == OMX_StateInvalid)
   {
      return OMX_ErrorInvalidState;
   }

   switch (Cmd)
   {
   case OMX_CommandStateSet:
      OMX_VENC_MSG_LOW("sending command MSG_ID_STATE_CHANGE", 0, 0, 0);
      msgId = VencMsgQ::MSG_ID_STATE_CHANGE;
      msgData.eState = (OMX_STATETYPE) nParam1;
      break;
   case OMX_CommandFlush:
   
      // If OEMs wish to flush, they need to be aware that ports must be
      // flushed simultaneously. We don't support flushing of individual ports.

      if (nParam1 != OMX_ALL)
      {
         OMX_VENC_MSG_ERROR("Flush must occur on input and output ports simulaneously", 0, 0, 0);
         return OMX_ErrorBadPortIndex;
      }
      else
      {
         OMX_VENC_MSG_LOW("sending command MSG_ID_FLUSH", 0, 0, 0);
         msgId = VencMsgQ::MSG_ID_FLUSH;
         msgData.nPortIndex = nParam1;
      }
      break;
   case OMX_CommandPortDisable:
      if((nParam1 != (OMX_U32)PORT_INDEX_IN) && (nParam1 != (OMX_U32)PORT_INDEX_OUT) && 
         (nParam1 != (OMX_U32)PORT_INDEX_BOTH))
      {
         OMX_VENC_MSG_ERROR("bad port index to call OMX_CommandPortDisable", 0, 0, 0);
         return OMX_ErrorBadPortIndex;
      }
      OMX_VENC_MSG_LOW("sending command MSG_ID_PORT_DISABLE", 0, 0, 0);
      msgId = VencMsgQ::MSG_ID_PORT_DISABLE;
      msgData.nPortIndex = nParam1;
      break;
   case OMX_CommandPortEnable:
      if((nParam1 != (OMX_U32)PORT_INDEX_IN) && (nParam1 != (OMX_U32)PORT_INDEX_OUT) && 
         (nParam1 != (OMX_U32)PORT_INDEX_BOTH))
      {
         OMX_VENC_MSG_ERROR("bad port index to call OMX_CommandPortEnable", 0, 0, 0);
         return OMX_ErrorBadPortIndex;
      }
      OMX_VENC_MSG_LOW("sending command MSG_ID_PORT_ENABLE", 0, 0, 0);
      msgId = VencMsgQ::MSG_ID_PORT_ENABLE;
      msgData.nPortIndex = nParam1;
      break;
   case OMX_CommandMarkBuffer:
      if(nParam1 != (OMX_U32)PORT_INDEX_IN)
      {
         OMX_VENC_MSG_ERROR("bad port index to call OMX_CommandMarkBuffer", 0, 0, 0);
         return OMX_ErrorBadPortIndex;
      }
      if(!pCmdData)
      {
         OMX_VENC_MSG_ERROR("param is null", 0, 0, 0);
         return OMX_ErrorBadParameter;
      }
      OMX_VENC_MSG_LOW("sending command MSG_ID_MARK_BUFFER", 0, 0, 0);
      msgId = VencMsgQ::MSG_ID_MARK_BUFFER;
      msgData.sMarkBuffer.nPortIndex = nParam1;
      memcpy(&msgData.sMarkBuffer.sMarkData,
             pCmdData,
             sizeof(OMX_MARKTYPE));
      break;
   default:
      OMX_VENC_MSG_ERROR("invalid command %d", (int) Cmd, 0, 0);
      return OMX_ErrorBadParameter;
   }

   m_pMsgQ->PushMsg(msgId, &msgData);
   if (venc_semaphore_wait(m_cmd_sem, 0) != 0)
   {
      OMX_VENC_MSG_ERROR("error waiting for semaphore", 0, 0, 0);
      return OMX_ErrorUndefined;
   }

   return OMX_ErrorNone; 
}

OMX_ERRORTYPE Venc::get_parameter(OMX_IN  OMX_HANDLETYPE hComponent, 
                                  OMX_IN  OMX_INDEXTYPE nParamIndex,  
                                  OMX_INOUT OMX_PTR pCompParam)
{
   ///////////////////////////////////////////////////////////////////////////////
   // Supported Param Index                         Type
   // ============================================================================
   // OMX_IndexParamVideoPortFormat                 OMX_VIDEO_PARAM_PORTFORMATTYPE
   // OMX_IndexParamPortDefinition                  OMX_PARAM_PORTDEFINITIONTYPE
   // OMX_IndexParamVideoInit                       OMX_PORT_PARAM_TYPE
   // OMX_IndexParamVideoBitrate                    OMX_VIDEO_PARAM_BITRATETYPE
   // OMX_IndexParamVideoMpeg4                      OMX_VIDEO_PARAM_MPEG4TYPE
   // OMX_IndexParamVideoProfileLevelQuerySupported OMX_VIDEO_PARAM_PROFILELEVEL
   // OMX_IndexParamVideoProfileLevelCurrent        OMX_VIDEO_PARAM_PROFILELEVEL
   // OMX_IndexParamVideoErrorCorrection            OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE 
   // QOMX_IndexParamVideoSyntaxHdr                 QOMX_VIDEO_PARAM_SYNTAXHDRTYPE
   // QOMX_IndexParamVideoEncoderMode               QOMX_VIDEO_PARAM_ENCODERMODETYPE
   // QOMX_IndexConfigTimestampScale                QOMX_TIMESTAMPSCALETYPE
   ///////////////////////////////////////////////////////////////////////////////
   if (pCompParam == NULL)
   {
      OMX_VENC_MSG_ERROR("param is null", 0, 0, 0);
      return OMX_ErrorBadParameter;
   }

   if(m_eState == OMX_StateInvalid)
   {
      OMX_VENC_MSG_ERROR("Get Param in Invalid State", 0, 0, 0); 
      return OMX_ErrorInvalidState;
   }

   OMX_ERRORTYPE result = OMX_ErrorNone;
   switch (nParamIndex)
   {
   case OMX_IndexParamVideoPortFormat:
   {
      OMX_VIDEO_PARAM_PORTFORMATTYPE* pParam = reinterpret_cast<OMX_VIDEO_PARAM_PORTFORMATTYPE*>(pCompParam);
      if (pParam->nPortIndex == (OMX_U32) PORT_INDEX_IN)
      {
         memcpy(pParam, &m_sInPortFormat, sizeof(m_sInPortFormat));
      }
      else if(pParam->nPortIndex == (OMX_U32) PORT_INDEX_OUT)
      {
         memcpy(pParam, &m_sOutPortFormat, sizeof(m_sOutPortFormat));
      }
      else
      {
         OMX_VENC_MSG_ERROR("GetParameter called on Bad Port Index", 0, 0, 0);
         result = OMX_ErrorBadPortIndex;
      }
      break;

   }
   case OMX_IndexParamPortDefinition:
   {
      OMX_PARAM_PORTDEFINITIONTYPE* pParam = reinterpret_cast<OMX_PARAM_PORTDEFINITIONTYPE*>(pCompParam);
      if (pParam->nPortIndex == (OMX_U32) PORT_INDEX_IN)
      {
         memcpy(pParam, &m_sInPortDef, sizeof(m_sInPortDef));
      }
      else if(pParam->nPortIndex == (OMX_U32) PORT_INDEX_OUT)
      {
         memcpy(pParam, &m_sOutPortDef, sizeof(m_sOutPortDef));
      }
      else
      {
         OMX_VENC_MSG_ERROR("GetParameter called on Bad Port Index", 0, 0, 0);
         result = OMX_ErrorBadPortIndex;
      }
      break;
   }
   case OMX_IndexParamAudioInit:
   {
      OMX_PORT_PARAM_TYPE *audioPortParamType = reinterpret_cast<OMX_PORT_PARAM_TYPE *> (pCompParam);
      OMX_VENC_MSG_LOW("get_parameter: OMX_IndexParamAudioInit", 0, 0, 0);
      memcpy(audioPortParamType, &m_sPortParam_audio, sizeof(m_sPortParam_audio));
      break;
   }

   case OMX_IndexParamImageInit:
   {
      OMX_PORT_PARAM_TYPE *imagePortParamType = reinterpret_cast<OMX_PORT_PARAM_TYPE *> (pCompParam);
      OMX_VENC_MSG_LOW("get_parameter: OMX_IndexParamImageInit", 0, 0, 0);
      memcpy(imagePortParamType, &m_sPortParam_img, sizeof(m_sPortParam_img));
      break;
   }
   case OMX_IndexParamOtherInit:
   {
      OMX_VENC_MSG_LOW("get_parameter: OMX_IndexParamOtherInit", 0, 0, 0);
      result = OMX_ErrorUnsupportedIndex;
      break;
   }

   case OMX_IndexParamPriorityMgmt:
   {
      OMX_PRIORITYMGMTTYPE *priorityMgmType = reinterpret_cast<OMX_PRIORITYMGMTTYPE *> (pCompParam);
      OMX_VENC_MSG_LOW("get_parameter: OMX_IndexParamPriorityMgmt", 0, 0, 0);
      memcpy(priorityMgmType, &m_sPriorityMgmt, sizeof(m_sPriorityMgmt));
      break;
   }
   case OMX_IndexParamCompBufferSupplier:
   {
      OMX_PARAM_BUFFERSUPPLIERTYPE *bufferSupplierType = reinterpret_cast<OMX_PARAM_BUFFERSUPPLIERTYPE*> (pCompParam);
      OMX_VENC_MSG_LOW("get_parameter: OMX_IndexParamCompBufferSupplier", 0, 0, 0);
      if(bufferSupplierType->nPortIndex ==(OMX_U32) PORT_INDEX_IN)
      {
         memcpy(bufferSupplierType, &m_sInBufSupplier, sizeof(m_sInBufSupplier));
      }
      else if(bufferSupplierType->nPortIndex ==(OMX_U32) PORT_INDEX_OUT)
      {
         memcpy(bufferSupplierType, &m_sOutBufSupplier, sizeof(m_sOutBufSupplier));
      }
      else
      {
         OMX_VENC_MSG_ERROR("GetParameter called on Bad Port Index", 0, 0, 0);
         result = OMX_ErrorBadPortIndex;
      }
      break;
   }
   case OMX_IndexParamVideoInit:
   {
      OMX_PORT_PARAM_TYPE* pParam = reinterpret_cast<OMX_PORT_PARAM_TYPE*>(pCompParam);
      memcpy(pParam, &m_sPortParam, sizeof(m_sPortParam));
      break;
   }
   case OMX_IndexParamVideoBitrate:
   {
      OMX_VIDEO_PARAM_BITRATETYPE* pParam = reinterpret_cast<OMX_VIDEO_PARAM_BITRATETYPE*>(pCompParam);
      memcpy(pParam, &m_sParamBitrate, sizeof(m_sParamBitrate));
      break;
   }
   case OMX_IndexParamVideoMpeg4:
   {
      OMX_VIDEO_PARAM_MPEG4TYPE* pParam = reinterpret_cast<OMX_VIDEO_PARAM_MPEG4TYPE*>(pCompParam);
      memcpy(pParam, &m_sParamMPEG4, sizeof(m_sParamMPEG4));
      break;
   }
   case OMX_IndexParamVideoProfileLevelCurrent:
   {
      OMX_VIDEO_PARAM_PROFILELEVELTYPE* pParam = reinterpret_cast<OMX_VIDEO_PARAM_PROFILELEVELTYPE*>(pCompParam);
      memcpy(pParam, &m_sParamProfileLevel, sizeof(m_sParamProfileLevel));

      break;
   }
   case OMX_IndexParamVideoH263:
   {
      OMX_VIDEO_PARAM_H263TYPE* pParam = reinterpret_cast<OMX_VIDEO_PARAM_H263TYPE*>(pCompParam);
      memcpy(pParam, &m_sParamH263, sizeof(m_sParamH263));
      break;
   }
   case OMX_IndexParamVideoAvc:
   {
      OMX_VIDEO_PARAM_AVCTYPE* pParam = reinterpret_cast<OMX_VIDEO_PARAM_AVCTYPE*>(pCompParam);
      memcpy(pParam, &m_sParamAVC, sizeof(m_sParamAVC));
      break;
   }
   case OMX_IndexParamVideoQuantization:
   {
      OMX_VIDEO_PARAM_QUANTIZATIONTYPE* pParam = reinterpret_cast<OMX_VIDEO_PARAM_QUANTIZATIONTYPE*>(pCompParam);
      memcpy(pParam, &m_sParamQPs, sizeof(m_sParamQPs));
      break;
   }
   case OMX_IndexParamVideoIntraRefresh:
   {
      OMX_VIDEO_PARAM_INTRAREFRESHTYPE* pParam = reinterpret_cast<OMX_VIDEO_PARAM_INTRAREFRESHTYPE*>(pCompParam);
      memcpy(pParam, &m_sParamIntraRefresh, sizeof(m_sParamIntraRefresh));
      break;
   }
   case OMX_IndexParamVideoProfileLevelQuerySupported:
   {
      
      OMX_VIDEO_PARAM_PROFILELEVELTYPE* pParam = reinterpret_cast<OMX_VIDEO_PARAM_PROFILELEVELTYPE*>(pCompParam);


      if (m_sOutPortDef.format.video.eCompressionFormat == OMX_VIDEO_CodingMPEG4)
      {
		   static const OMX_U32 MPEG4Profile[][2] = {
		   { (OMX_U32) OMX_VIDEO_MPEG4ProfileSimple, (OMX_U32) OMX_VIDEO_MPEG4Level0 },
              { (OMX_U32) OMX_VIDEO_MPEG4ProfileSimple, (OMX_U32) OMX_VIDEO_MPEG4Level0b },
              { (OMX_U32) OMX_VIDEO_MPEG4ProfileSimple, (OMX_U32) OMX_VIDEO_MPEG4Level1 },
              { (OMX_U32) OMX_VIDEO_MPEG4ProfileSimple, (OMX_U32) OMX_VIDEO_MPEG4Level2 },
		   { (OMX_U32) OMX_VIDEO_MPEG4ProfileSimple, (OMX_U32) OMX_VIDEO_MPEG4Level3 },
		   { (OMX_U32) OMX_VIDEO_MPEG4ProfileSimple, (OMX_U32) OMX_VIDEO_MPEG4Level4a } };
         static const OMX_U32 nSupport = sizeof(MPEG4Profile) / (sizeof(OMX_U32) * 2);
         if ( pParam->nProfileIndex < nSupport)
         {
            pParam->eProfile = (OMX_VIDEO_MPEG4PROFILETYPE) MPEG4Profile[pParam->nProfileIndex][0];
            pParam->eLevel = (OMX_VIDEO_MPEG4LEVELTYPE) MPEG4Profile[pParam->nProfileIndex][1];
         }
         else
         {
            result = OMX_ErrorNoMore;
         }
      }
	  else if (m_sOutPortDef.format.video.eCompressionFormat == OMX_VIDEO_CodingAVC)
	  {
		   static const OMX_U32 H264Profile[][2] = {
		   { (OMX_U32) OMX_VIDEO_AVCProfileBaseline, (OMX_U32) OMX_VIDEO_AVCLevel1 },
              { (OMX_U32) OMX_VIDEO_AVCProfileBaseline, (OMX_U32) OMX_VIDEO_AVCLevel1b },
              { (OMX_U32) OMX_VIDEO_AVCProfileBaseline, (OMX_U32) OMX_VIDEO_AVCLevel11 },
              { (OMX_U32) OMX_VIDEO_AVCProfileBaseline, (OMX_U32) OMX_VIDEO_AVCLevel12 },
		   { (OMX_U32) OMX_VIDEO_AVCProfileBaseline, (OMX_U32) OMX_VIDEO_AVCLevel13 },
		   { (OMX_U32) OMX_VIDEO_AVCProfileBaseline, (OMX_U32) OMX_VIDEO_AVCLevel2 },
		   { (OMX_U32) OMX_VIDEO_AVCProfileBaseline, (OMX_U32) OMX_VIDEO_AVCLevel21 },
		   { (OMX_U32) OMX_VIDEO_AVCProfileBaseline, (OMX_U32) OMX_VIDEO_AVCLevel22 }};
         static const OMX_U32 nSupport = sizeof(H264Profile) / (sizeof(OMX_U32) * 2);
         if (pParam->nProfileIndex < nSupport)
         {
            pParam->eProfile = (OMX_VIDEO_AVCPROFILETYPE) H264Profile[pParam->nProfileIndex][0];
            pParam->eLevel = (OMX_VIDEO_AVCLEVELTYPE) H264Profile[pParam->nProfileIndex][1];
         }
         else
         {
            result = OMX_ErrorNoMore;
         }

	  }
      else
      {
		   static const OMX_U32 H263Profile[][2] = {
		   { (OMX_U32) OMX_VIDEO_H263ProfileBaseline, (OMX_U32) OMX_VIDEO_H263Level10 },
              { (OMX_U32) OMX_VIDEO_H263ProfileBaseline, (OMX_U32) OMX_VIDEO_H263Level20 },
              { (OMX_U32) OMX_VIDEO_H263ProfileBaseline, (OMX_U32) OMX_VIDEO_H263Level30 },
           { (OMX_U32) OMX_VIDEO_H263ProfileBaseline, (OMX_U32) OMX_VIDEO_H263Level40 },
              { (OMX_U32) OMX_VIDEO_H263ProfileBaseline, (OMX_U32) OMX_VIDEO_H263Level45 },
		   { (OMX_U32) OMX_VIDEO_H263ProfileBaseline, (OMX_U32) OMX_VIDEO_H263Level50 },
           { (OMX_U32) OMX_VIDEO_H263ProfileBaseline, (OMX_U32) OMX_VIDEO_H263Level60 },
		   { (OMX_U32) OMX_VIDEO_H263ProfileBaseline, (OMX_U32) OMX_VIDEO_H263Level70 }};
         static const OMX_U32 nSupport = sizeof(H263Profile) / (sizeof(OMX_U32) * 2);
         if (pParam->nProfileIndex < nSupport)
         {
            pParam->eProfile = (OMX_VIDEO_H263PROFILETYPE) H263Profile[pParam->nProfileIndex][0];
            pParam->eLevel = (OMX_VIDEO_H263LEVELTYPE) H263Profile[pParam->nProfileIndex][1];
         }
         else
         {
            result = OMX_ErrorNoMore;
         }
      }
      break;
   }
   case OMX_IndexParamVideoErrorCorrection:
   {
      OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE* pParam = reinterpret_cast<OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE*>(pCompParam);
      memcpy(pParam, &m_sErrorCorrection, sizeof(m_sErrorCorrection));
      break;
   }
  case OMX_QcomIndexPortDefn:
  {
      OMX_QCOM_PARAM_PORTDEFINITIONTYPE* pParam = reinterpret_cast<OMX_QCOM_PARAM_PORTDEFINITIONTYPE*>(pCompParam);
      if (pParam->nPortIndex == (OMX_U32) PORT_INDEX_IN)
      {
         memcpy(pParam, &m_sInQcomPortDef, sizeof(m_sInQcomPortDef));
      }
      else
      {
   result = OMX_ErrorNoMore;
      }
      break;
   }
 case OMX_IndexConfigCommonRotate:
   { 
      OMX_VENC_MSG_ERROR("unsupported index %d", (int) nParamIndex, 0, 0);
      result = OMX_ErrorUnsupportedIndex; 
      break; 
   }
 case OMX_IndexParamStandardComponentRole:
   {
        OMX_PARAM_COMPONENTROLETYPE *comp_role;
        comp_role = (OMX_PARAM_COMPONENTROLETYPE *) pCompParam;
        comp_role->nVersion.nVersion = OMX_SPEC_VERSION;
        comp_role->nSize = sizeof(*comp_role); 
        OMX_VENC_MSG_LOW("OMX_IndexParamStandardComponentRole %d",nParamIndex, 0, 0);
        if(NULL != comp_role->cRole)
        {
           memcpy(comp_role->cRole, m_cRole, strlen((char*) m_cRole) + 1);
        }
        else
        {
            OMX_VENC_MSG_ERROR("OMX_IndexParamStandardComponentRole %d is passed with NULL parameter for role",nParamIndex, 0, 0);
      result = OMX_ErrorBadParameter;
        }
        break;
   }
case QOMX_IndexParamVideoSyntaxHdr:
    {  
      /* Retrive the required information from the IL component */
	  venc_status_type venc_status;
      QOMX_VIDEO_SYNTAXHDRTYPE* pParam = reinterpret_cast<QOMX_VIDEO_SYNTAXHDRTYPE*>(pCompParam);
      translate_config(&m_sConfigParam);
      venc_status = venc_get_syntax_hdr(m_nDLHandle,
	                                (uint32*)&pParam->data,
	                                pParam->nBytes,
					(int32*)&(pParam->nBytes),
                                        &m_sConfigParam);

	  if(venc_status == VENC_STATUS_SUCCESS)
	  {
         memcpy(&m_sSyntaxhdr, pParam, sizeof(m_sSyntaxhdr));
	  }
      else
	  {
         OMX_VENC_MSG_ERROR("Error returned from GetSyntaxHeader()",0,0,0);
	  }
      break;
      }
case QOMX_IndexParamVideoEncoderMode:
    {
      QOMX_VIDEO_PARAM_ENCODERMODETYPE* pParam = reinterpret_cast<QOMX_VIDEO_PARAM_ENCODERMODETYPE*>(pCompParam);
      memcpy(pParam, &m_sConfigEncoderModeType, sizeof(m_sConfigEncoderModeType));
      OMX_VENC_MSG_MEDIUM("m_sConfigEncoderModeType.nMode get = 0x%x", m_sConfigEncoderModeType.nMode, 0, 0);
      break;
    }
#ifndef FEATURE_LINUX_A
case QOMX_IndexConfigTimestampScale:
     {
         QOMX_TIMESTAMPSCALETYPE* pParam = reinterpret_cast<QOMX_TIMESTAMPSCALETYPE*>(pCompParam);
         memcpy(pParam, &m_sTimeStampScale, sizeof(m_sTimeStampScale));
         OMX_VENC_MSG_MEDIUM("m_sTimeStampScale.nTimestampScale is set to 0x%x", (int) m_sTimeStampScale.nTimestampScale >> 16, 0, 0);
         break;
     }
#endif
case OMX_COMPONENT_CAPABILITY_TYPE_INDEX:
   {
        OMXComponentCapabilityFlagsType *pParam = reinterpret_cast<OMXComponentCapabilityFlagsType*>(pCompParam);
        pParam->iIsOMXComponentMultiThreaded = OMX_TRUE;
        pParam->iOMXComponentSupportsExternalOutputBufferAlloc = OMX_FALSE;
        pParam->iOMXComponentSupportsExternalInputBufferAlloc = OMX_TRUE;
        pParam->iOMXComponentSupportsMovableInputBuffers = OMX_TRUE;
        pParam->iOMXComponentUsesNALStartCodes = OMX_TRUE;
        pParam->iOMXComponentSupportsPartialFrames = OMX_FALSE;
        pParam->iOMXComponentCanHandleIncompleteFrames = OMX_FALSE;
        pParam->iOMXComponentUsesFullAVCFrames = OMX_FALSE;
        OMX_VENC_MSG_HIGH("Supporting capability index in encoder node", 0, 0, 0);
        break;
   }


   default:
      OMX_VENC_MSG_ERROR("unsupported index %d", (int) nParamIndex, 0, 0);
      result = OMX_ErrorUnsupportedIndex;
      break;
   }
   return result; 
}


OMX_ERRORTYPE Venc::set_parameter(OMX_IN  OMX_HANDLETYPE hComponent, 
                                  OMX_IN  OMX_INDEXTYPE nIndex,
                                  OMX_IN  OMX_PTR pCompParam)
{
   ///////////////////////////////////////////////////////////////////////////////
   // Supported Param Index                         Type
   // ============================================================================
   // OMX_IndexParamVideoPortFormat                 OMX_VIDEO_PARAM_PORTFORMATTYPE
   // OMX_IndexParamPortDefinition                  OMX_PARAM_PORTDEFINITIONTYPE
   // OMX_IndexParamVideoInit                       OMX_PORT_PARAM_TYPE
   // OMX_IndexParamVideoBitrate                    OMX_VIDEO_PARAM_BITRATETYPE
   // OMX_IndexParamVideoMpeg4                      OMX_VIDEO_PARAM_MPEG4TYPE
   // OMX_IndexParamVideoProfileLevelCurrent        OMX_VIDEO_PARAM_PROFILELEVEL
   // QOMX_IndexParamVideoEncoderMode               QOMX_VIDEO_PARAM_ENCODERMODETYPE 
   // QOMX_IndexConfigTimestampScale                QOMX_TIMESTAMPSCALETYPE
   ///////////////////////////////////////////////////////////////////////////////
   OMX_VIDEO_PARAM_PORTFORMATTYPE* pParamCheck = reinterpret_cast<OMX_VIDEO_PARAM_PORTFORMATTYPE*>(pCompParam);
   if (pCompParam == NULL)
   {
      OMX_VENC_MSG_ERROR("param is null", 0, 0, 0);
      return OMX_ErrorBadParameter;
   }

   if((m_eState == OMX_StateLoaded)
      ||((pParamCheck->nPortIndex == (OMX_U32) PORT_INDEX_IN && !m_sInPortDef.bEnabled)||
      (pParamCheck->nPortIndex == (OMX_U32) PORT_INDEX_OUT && !m_sOutPortDef.bEnabled)))
   {
      OMX_VENC_MSG_LOW("Set Parameter called in valid state", 0, 0, 0); 
   }
   else
   {
      OMX_VENC_MSG_ERROR("Set Parameter called in Incorrect state", 0, 0, 0);
      return OMX_ErrorIncorrectStateOperation;
   }

   switch (nIndex)
   {
   case OMX_IndexParamVideoPortFormat:
   {
      OMX_VIDEO_PARAM_PORTFORMATTYPE* pParam = reinterpret_cast<OMX_VIDEO_PARAM_PORTFORMATTYPE*>(pCompParam);
      if (pParam->nPortIndex == (OMX_U32) PORT_INDEX_IN)
      {
         if (pParam->eColorFormat != OMX_COLOR_FormatYUV420SemiPlanar)
      {
            OMX_VENC_MSG_ERROR("color format must be YUV420 semi-planar", 0, 0, 0);
            return OMX_ErrorBadParameter;
         }
           m_sInPortFormat.eColorFormat = pParam->eColorFormat ;
           m_sInPortFormat.eCompressionFormat = pParam->eCompressionFormat ;
           m_sInPortDef.format.video.xFramerate = pParam->xFramerate;
           m_sInPortFormat.xFramerate = pParam->xFramerate;
        }
      else if (pParam->nPortIndex == (OMX_U32) PORT_INDEX_OUT )
      {
           m_sOutPortFormat.eColorFormat = pParam->eColorFormat ;
           m_sOutPortFormat.eCompressionFormat = pParam->eCompressionFormat ;
           m_sOutPortDef.format.video.xFramerate = pParam->xFramerate;     
           m_sOutPortFormat.xFramerate = pParam->xFramerate;
      }
      else
      {
          OMX_VENC_MSG_ERROR("set_parameter: OMX IndexParamVidePortFormat: OMX_ErrorUnsupportedIndex", 0, 0, 0); 
          return OMX_ErrorBadPortIndex;
      }
      m_sConfigFramerate.xEncodeFramerate = pParam->xFramerate;
      break;
   }
   case OMX_IndexParamPortDefinition:
   {
      OMX_PARAM_PORTDEFINITIONTYPE* pParam = reinterpret_cast<OMX_PARAM_PORTDEFINITIONTYPE*>(pCompParam);
      /* When the component is in Loaded state and IDLE Pending 
         Or while the I/P or the O/P port or disabled */
      if(!BITMASK_PRESENT(m_flags,OMX_COMPONENT_IDLE_PENDING))
      {
         OMX_VENC_MSG_LOW("Set Parameter called in valid state", 0, 0, 0); 
      }
      else
      {
         OMX_VENC_MSG_ERROR("Set Parameter called in Incorrect state", 0, 0, 0);
         return OMX_ErrorIncorrectStateOperation;
      }

      if (pParam->nPortIndex == (OMX_U32) PORT_INDEX_IN)
      {
         if(pParam->nBufferCountActual < m_sInPortDef.nBufferCountMin)
         {
            OMX_VENC_MSG_ERROR("SetParemeter - called with Buf cnt less than minimum buf count", 0, 0, 0);
            return OMX_ErrorBadParameter;
         }
         memcpy(&m_sInPortDef, pParam, sizeof(m_sInPortDef));
         m_sInPortDef.nBufferSize = m_sInPortDef.format.video.nFrameWidth * 
                            m_sInPortDef.format.video.nFrameHeight * 3 / 2;
         m_sInPortDef.format.video.nBitrate = pParam->format.video.nBitrate;
         m_sInPortDef.format.video.xFramerate = pParam->format.video.xFramerate;
         m_sInPortFormat.xFramerate = pParam->format.video.xFramerate;
      }
      else if(pParam->nPortIndex == (OMX_U32) PORT_INDEX_OUT)
      {
         memcpy(&m_sOutPortDef, pParam, sizeof(m_sOutPortDef));
         m_sOutPortDef.format.video.nBitrate = pParam->format.video.nBitrate;
         m_sOutPortDef.format.video.xFramerate = pParam->format.video.xFramerate;     
         m_sOutPortFormat.xFramerate = pParam->format.video.xFramerate;
         m_sConfigBitrate.nEncodeBitrate = pParam->format.video.nBitrate;
         m_sParamBitrate.nTargetBitrate = pParam->format.video.nBitrate;       
         m_sConfigFramerate.xEncodeFramerate = pParam->format.video.xFramerate;
      }
      else
      {
         OMX_VENC_MSG_ERROR("SetParameter called on Bad Port Index", 0, 0, 0);
         return OMX_ErrorBadPortIndex;
      }

      if (m_sParamBitrate.eControlRate == OMX_Video_ControlRateDisable)
      {
       m_sOutPortDef.nBufferSize = m_sInPortDef.nBufferSize;
      }
      else if(pParam->nBufferSize >= m_sOutPortDef.nBufferSize)
       {
       m_sOutPortDef.nBufferSize = m_sInPortDef.format.video.nFrameWidth * m_sInPortDef.format.video.nFrameHeight ; 
       OMX_VENC_MSG_LOW("set_parameter : output buffer size = %d",m_sOutPortDef.nBufferSize, 0, 0);
       }
       else
       {
        OMX_VENC_MSG_ERROR("ERROR in output buffer size:- Not meeting requirement", 0, 0, 0);
        return OMX_ErrorUnsupportedIndex; // need to check if we need to return bad parameter.
       }
      break;
   }
   case OMX_IndexParamVideoInit:
   {
      OMX_PORT_PARAM_TYPE* pParam = reinterpret_cast<OMX_PORT_PARAM_TYPE*>(pCompParam);
      memcpy(&m_sPortParam, pParam, sizeof(m_sPortParam));
      break;
   }
   case OMX_IndexParamVideoBitrate:
   {
      OMX_VIDEO_PARAM_BITRATETYPE* pParam = reinterpret_cast<OMX_VIDEO_PARAM_BITRATETYPE*>(pCompParam);
      m_sParamBitrate.nSize = pParam->nSize;
      m_sParamBitrate.nVersion = pParam->nVersion;
      m_sParamBitrate.nPortIndex = pParam->nPortIndex;
      m_sParamBitrate.nTargetBitrate = pParam->nTargetBitrate;
      if (m_sConfigEncoderModeType.nMode != QOMX_VIDEO_EncoderModeMMS )
      {
         m_sParamBitrate.eControlRate = pParam->eControlRate;
       
      }
      else
      {
         OMX_VENC_MSG_ERROR("OMX_IndexParamVideoBitrate can not be set if EncoderMode is already set to QOMX_VIDEO_EncoderModeMMS", 0, 0, 0);
      }
     if (pParam->nPortIndex == (OMX_U32) PORT_INDEX_IN)
     {
      m_sInPortDef.format.video.nBitrate = pParam->nTargetBitrate;
     }
     else if(pParam->nPortIndex == (OMX_U32) PORT_INDEX_OUT)
     {
      m_sOutPortDef.format.video.nBitrate = pParam->nTargetBitrate;
     }
     else
     {
        OMX_VENC_MSG_ERROR("SetParameter called on Bad Port Index", 0, 0, 0);
        return OMX_ErrorBadPortIndex;
     }
      m_sConfigBitrate.nEncodeBitrate = pParam->nTargetBitrate;
      break;
   }
   case OMX_IndexParamVideoMpeg4:
   {
      OMX_VIDEO_PARAM_MPEG4TYPE* pParam = reinterpret_cast<OMX_VIDEO_PARAM_MPEG4TYPE*>(pCompParam);
      memcpy(&m_sParamMPEG4, pParam, sizeof(m_sParamMPEG4));
      m_sParamProfileLevel.eProfile = pParam->eProfile;
      m_sParamProfileLevel.eLevel = pParam->eLevel;
      break;
   }
   case OMX_IndexParamVideoProfileLevelCurrent:
   {
      OMX_VIDEO_PARAM_PROFILELEVELTYPE* pParam = reinterpret_cast<OMX_VIDEO_PARAM_PROFILELEVELTYPE*>(pCompParam);
	 if (m_sOutPortDef.format.video.eCompressionFormat == OMX_VIDEO_CodingMPEG4)
	  {
            memcpy(&m_sParamProfileLevel, pParam, sizeof(m_sParamProfileLevel));
            m_sParamMPEG4.eProfile = (OMX_VIDEO_MPEG4PROFILETYPE) pParam->eProfile;
            m_sParamMPEG4.eLevel = (OMX_VIDEO_MPEG4LEVELTYPE) pParam->eLevel;
	  }
	 else if (m_sOutPortDef.format.video.eCompressionFormat == OMX_VIDEO_CodingH263)
	  {
            memcpy(&m_sParamProfileLevel, pParam, sizeof(m_sParamProfileLevel));
            m_sParamH263.eProfile = (OMX_VIDEO_H263PROFILETYPE) pParam->eProfile;
	    m_sParamH263.eLevel = (OMX_VIDEO_H263LEVELTYPE) pParam->eLevel;
	  }
	  else if(m_sOutPortDef.format.video.eCompressionFormat == OMX_VIDEO_CodingAVC)
	  {
	    memcpy(&m_sParamProfileLevel, pParam, sizeof(m_sParamProfileLevel));
            m_sParamAVC.eProfile = (OMX_VIDEO_AVCPROFILETYPE) pParam->eProfile;
            m_sParamAVC.eLevel = (OMX_VIDEO_AVCLEVELTYPE) pParam->eLevel;
	  }
      break;
   }
   case OMX_IndexParamVideoErrorCorrection:
   {
      OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE* pParam = reinterpret_cast<OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE*>(pCompParam);
      memcpy(&m_sErrorCorrection, pParam, sizeof(m_sErrorCorrection));
      break;
   }   
   case OMX_IndexParamVideoQuantization:
   {
      OMX_VIDEO_PARAM_QUANTIZATIONTYPE* pParam = reinterpret_cast<OMX_VIDEO_PARAM_QUANTIZATIONTYPE*>(pCompParam);
      memcpy(&m_sParamQPs, pParam, sizeof(m_sParamQPs));
      break;
   }
   case OMX_IndexParamVideoIntraRefresh:
   {
      OMX_VIDEO_PARAM_INTRAREFRESHTYPE* pParam = reinterpret_cast<OMX_VIDEO_PARAM_INTRAREFRESHTYPE*>(pCompParam);
      memcpy(&m_sParamIntraRefresh, pParam, sizeof(m_sParamIntraRefresh));
      break;
   }
   case OMX_IndexParamVideoH263:
   {
      OMX_VIDEO_PARAM_H263TYPE* pParam = reinterpret_cast<OMX_VIDEO_PARAM_H263TYPE*>(pCompParam);
      memcpy(&m_sParamH263, pParam, sizeof(m_sParamH263));
      m_sParamProfileLevel.eProfile = pParam->eProfile;
      m_sParamProfileLevel.eLevel = pParam->eLevel;
      break;
   }
   case OMX_IndexParamVideoAvc:
   {
      OMX_VIDEO_PARAM_AVCTYPE* pParam = reinterpret_cast<OMX_VIDEO_PARAM_AVCTYPE*>(pCompParam);
      memcpy(&m_sParamAVC, pParam, sizeof(m_sParamAVC));
      m_sParamProfileLevel.eProfile = pParam->eProfile;
      m_sParamProfileLevel.eLevel = pParam->eLevel;
      break;
   }
   case OMX_IndexParamPriorityMgmt:
   {
      OMX_PRIORITYMGMTTYPE *priorityMgmtype = reinterpret_cast<OMX_PRIORITYMGMTTYPE*> (pCompParam);
      if(m_eState != OMX_StateLoaded)
      {
         OMX_VENC_MSG_ERROR("Set Parameter called in Invalid State", 0, 0, 0); 
         return OMX_ErrorIncorrectStateOperation;
      }
      OMX_VENC_MSG_LOW("set_parameter: OMX_IndexParamPriorityMgmt %d", priorityMgmtype->nGroupID, 0, 0);
      OMX_VENC_MSG_LOW("set_parameter: priorityMgmtype %d", priorityMgmtype->nGroupPriority, 0, 0);
      m_sPriorityMgmt.nGroupID = priorityMgmtype->nGroupID;
      m_sPriorityMgmt.nGroupPriority = priorityMgmtype->nGroupPriority;
      break;
   }

   case OMX_IndexParamCompBufferSupplier:
   {
      OMX_PARAM_BUFFERSUPPLIERTYPE *bufferSupplierType = reinterpret_cast<OMX_PARAM_BUFFERSUPPLIERTYPE*> (pCompParam);
      OMX_VENC_MSG_LOW("set_parameter: OMX_IndexParamCompBufferSupplier %d", bufferSupplierType->eBufferSupplier, 0, 0);
      if(bufferSupplierType->nPortIndex ==(OMX_U32)PORT_INDEX_IN)
      {
         m_sInBufSupplier.eBufferSupplier = bufferSupplierType->eBufferSupplier;
      }
      else if(bufferSupplierType->nPortIndex == (OMX_U32)PORT_INDEX_OUT)
      {
         m_sOutBufSupplier.eBufferSupplier = bufferSupplierType->eBufferSupplier;
      }
      else
      {
         OMX_VENC_MSG_ERROR("SetParameter called on Bad Port Index", 0, 0, 0);
         return OMX_ErrorBadPortIndex;
      }
      break;
   }
   case OMX_QcomIndexPortDefn:
   {
      OMX_QCOM_PARAM_PORTDEFINITIONTYPE* pParam = reinterpret_cast<OMX_QCOM_PARAM_PORTDEFINITIONTYPE*>(pCompParam);
      if (pParam->nPortIndex == (OMX_U32) PORT_INDEX_IN)
      {
         memcpy(&m_sInQcomPortDef, pParam, sizeof(m_sInQcomPortDef));
      }
      else
      {
         return OMX_ErrorNoMore;
      }
      break;
   }
   case OMX_QcomIndexPlatformPvt:
   {
      OMX_QCOM_PLATFORMPRIVATE_EXTN* pParam = reinterpret_cast<OMX_QCOM_PLATFORMPRIVATE_EXTN*>(pCompParam);
      OMX_VENC_MSG_LOW("Inside OMX_QcomIndexPlatformPvt", 0, 0, 0);

      if(pParam->nPortIndex == (OMX_U32)PORT_INDEX_IN)
      {
          if(pParam->type == OMX_QCOM_PLATFORM_PRIVATE_PMEM)
              m_bIsQcomPvt = OMX_TRUE;
          else
              m_bIsQcomPvt = OMX_FALSE;
      }
      else
      {
         OMX_VENC_MSG_ERROR("SetParameter called on unsupported Port Index for QcomPvt", 0, 0, 0);
         return OMX_ErrorBadPortIndex;
      }
      break;
   }

   case OMX_IndexConfigCommonRotate:
   { 
      OMX_VENC_MSG_ERROR("unsupported index in set_parameter %d", (int)nIndex, 0, 0);
      return OMX_ErrorUnsupportedIndex; 
   }

    case OMX_IndexParamStandardComponentRole:
     {
          OMX_PARAM_COMPONENTROLETYPE *comp_role;
          comp_role = (OMX_PARAM_COMPONENTROLETYPE *) pCompParam;
          OMX_VENC_MSG_LOW("OMX_IndexParamStandardComponentRole", 0, 0, 0);  
          if(!strncmp(m_pComponentName,"OMX.qcom.video.encoder.mpeg4",strlen("OMX.qcom.video.encoder.mpeg4")))
          {
              if(!strcmp((const char*)comp_role->cRole, pRoleMPEG4))
              {
                 memcpy(comp_role->cRole, m_cRole, strlen((char*) m_cRole) + 1);
              }
              else
              {
                  OMX_VENC_MSG_ERROR("unknown Index", 0, 0, 0);
                  return OMX_ErrorUnsupportedSetting;
              }
          }
          else if(!strncmp(m_pComponentName,"OMX.qcom.video.encoder.h263",strlen("OMX.qcom.video.encoder.h263")))
          {
              if(!strcmp((const char*)comp_role->cRole, pRoleH263))
              {
                 memcpy(comp_role->cRole, m_cRole, strlen((char*) m_cRole) + 1);
              }
              else
              {
                  OMX_VENC_MSG_ERROR("unknown Index", 0, 0, 0);
                  return OMX_ErrorUnsupportedSetting;
              }
          }
          else if(!strncmp(m_pComponentName,"OMX.qcom.video.encoder.avc",strlen("OMX.qcom.video.encoder.avc")))
          {
              if(!strcmp((const char*)comp_role->cRole, pRoleAVC))
              {
                 memcpy(comp_role->cRole, m_cRole, strlen((char*) m_cRole) + 1);
              }
              else
              {
                  OMX_VENC_MSG_ERROR("unknown Index", 0, 0, 0);
                  return OMX_ErrorUnsupportedSetting;
              }
          }
          else
          {
               OMX_VENC_MSG_ERROR("unknown param", 0, 0, 0);
               return OMX_ErrorInvalidComponentName;
          }
          break;
     }
     case QOMX_IndexParamVideoEncoderMode:
     {
         QOMX_VIDEO_PARAM_ENCODERMODETYPE* pParam = reinterpret_cast<QOMX_VIDEO_PARAM_ENCODERMODETYPE*>(pCompParam);
         memcpy(&m_sConfigEncoderModeType, pParam, sizeof(m_sConfigEncoderModeType));
         OMX_VENC_MSG_MEDIUM("m_sConfigEncoderModeType.nMode is set to 0x%x", (int) m_sConfigEncoderModeType.nMode, 0, 0);
         break;
     }
#ifndef FEATURE_LINUX_A
     case QOMX_IndexConfigTimestampScale:
     {
         QOMX_TIMESTAMPSCALETYPE* pParam = reinterpret_cast<QOMX_TIMESTAMPSCALETYPE*>(pCompParam);
         memcpy(&m_sTimeStampScale, pParam, sizeof(m_sTimeStampScale));
	 OMX_VENC_MSG_MEDIUM("m_sTimeStampScale.nTimestampScale is set to 0x%x", (int) m_sTimeStampScale.nTimestampScale >> 16, 0, 0);
         break;
     }
#endif
   default:
      OMX_VENC_MSG_ERROR("unsupported index %d", (int) nIndex, 0, 0);
      return OMX_ErrorUnsupportedIndex;
   }
   return OMX_ErrorNone; 
}


OMX_ERRORTYPE Venc::get_config(OMX_IN  OMX_HANDLETYPE hComponent,
                               OMX_IN  OMX_INDEXTYPE nIndex, 
                               OMX_INOUT OMX_PTR pCompConfig)
{
   ////////////////////////////////////////////////////////////////
   // Supported Config Index           Type
   // =============================================================
   // OMX_IndexConfigVideoBitrate      OMX_VIDEO_CONFIG_BITRATETYPE
   // OMX_IndexConfigVideoFramerate    OMX_CONFIG_FRAMERATETYPE
   // OMX_IndexConfigCommonRotate      OMX_CONFIG_ROTATIONTYPE      
   // QOMX_IndexConfigVideoIntraRefresh    OMX_VIDEO_PARAM_INTRAREFRESHTYPE
   // QOMX_IndexConfigVideoIntraperiod  QOMX_VIDEO_INTRAPERIODTYPE
   // OMX_IndexConfigVideoIntraVOPRefresh  OMX_CONFIG_INTRAREFRESHVOPTYPE
   ////////////////////////////////////////////////////////////////

   if (pCompConfig == NULL)
   {
      OMX_VENC_MSG_ERROR("param is null", 0, 0, 0);
      return OMX_ErrorBadParameter;
   }

   if (m_eState == OMX_StateInvalid)
   {
      OMX_VENC_MSG_ERROR("can't be in invalid state", 0, 0, 0);
      return OMX_ErrorIncorrectStateOperation;
   }

   switch (nIndex)
   {
   case OMX_IndexConfigVideoBitrate:
   {
      OMX_VIDEO_CONFIG_BITRATETYPE* pParam = reinterpret_cast<OMX_VIDEO_CONFIG_BITRATETYPE*>(pCompConfig);
      memcpy(pParam, &m_sConfigBitrate, sizeof(m_sConfigBitrate));
      break;
   }
   case OMX_IndexConfigVideoFramerate:
   {
      OMX_CONFIG_FRAMERATETYPE* pParam = reinterpret_cast<OMX_CONFIG_FRAMERATETYPE*>(pCompConfig);
       memcpy(pParam, &m_sConfigFramerate, sizeof(m_sConfigFramerate));
      break;
   }
   case OMX_IndexConfigCommonRotate:
   {
      OMX_CONFIG_ROTATIONTYPE* pParam = reinterpret_cast<OMX_CONFIG_ROTATIONTYPE*>(pCompConfig);
       if(m_eState != OMX_StateLoaded)
       {
         // we only allow this at init time!
         OMX_VENC_MSG_ERROR("Rotation can only be configured in loaded state",0,0,0);
         return OMX_ErrorIncorrectStateOperation;
       }
       memcpy(pParam, &m_sConfigFrameRotation, sizeof(m_sConfigFrameRotation));
       break;
   }
   case QOMX_IndexConfigVideoIntraRefresh:
   {
      OMX_VIDEO_PARAM_INTRAREFRESHTYPE* pParam = reinterpret_cast<OMX_VIDEO_PARAM_INTRAREFRESHTYPE*>(pCompConfig);
       memcpy(pParam, &m_sParamIntraRefresh, sizeof(m_sParamIntraRefresh));
       break;
   }
   case QOMX_IndexConfigVideoIntraperiod:
   {
      QOMX_VIDEO_INTRAPERIODTYPE* pParam = reinterpret_cast<QOMX_VIDEO_INTRAPERIODTYPE*>(pCompConfig);
      memcpy(pParam, &m_sConfigIntraPeriod, sizeof(m_sConfigIntraPeriod));
      break;
   }

   case QOMX_IndexConfigVideoTemporalSpatialTradeOff: 
   {
      QOMX_VIDEO_TEMPORALSPATIALTYPE* pParam = reinterpret_cast<QOMX_VIDEO_TEMPORALSPATIALTYPE*>(pCompConfig);
      memcpy(pParam, &m_sTsTradeoffFactor, sizeof(m_sTsTradeoffFactor));
       break;
   }
   case OMX_IndexConfigVideoIntraVOPRefresh:
   {
      OMX_CONFIG_INTRAREFRESHVOPTYPE* pParam = reinterpret_cast<OMX_CONFIG_INTRAREFRESHVOPTYPE*>(pCompConfig);
      memcpy(pParam, &m_sConfigIntraRefreshVOP, sizeof(m_sConfigIntraRefreshVOP));
      break;
   }
   default:
      OMX_VENC_MSG_ERROR("unsupported index %d", (int) nIndex, 0, 0);
      return OMX_ErrorUnsupportedIndex;
   }
   return OMX_ErrorNone; 
}

OMX_ERRORTYPE Venc::set_config(OMX_IN  OMX_HANDLETYPE hComponent,
                               OMX_IN  OMX_INDEXTYPE nIndex, 
                               OMX_IN  OMX_PTR pCompConfig)
{

   ////////////////////////////////////////////////////////////////
   // Supported Config Index           Type
   // =============================================================
   // OMX_IndexConfigVideoBitrate      OMX_VIDEO_CONFIG_BITRATETYPE
   // OMX_IndexConfigVideoFramerate    OMX_CONFIG_FRAMERATETYPE
   // OMX_IndexConfigCommonRotate      OMX_CONFIG_ROTATIONTYPE     
   // QOMX_IndexConfigVideoIntraRefresh,   OMX_VIDEO_PARAM_INTRAREFRESHTYPE
   // QOMX_IndexConfigVideoIntraperiod     QOMX_VIDEO_INTRAPERIODTYPE
   // OMX_IndexConfigVideoIntraVOPRefresh  OMX_CONFIG_INTRAREFRESHVOPTYPE
   // QOMX_IndexConfigTimestampScale       QOMX_TIMESTAMPSCALETYPE
   //////////////////////////////////////////////////////////////////

   if (pCompConfig == NULL)
   {
      OMX_VENC_MSG_ERROR("param is null", 0, 0, 0);
      return OMX_ErrorBadParameter;
   }
   if (m_eState == OMX_StateInvalid)
   {
      OMX_VENC_MSG_ERROR("can't be in invalid state", 0, 0, 0);
      return OMX_ErrorIncorrectStateOperation;
   }
   // params will be validated prior to venc_init
   switch (nIndex)
   {
   case OMX_IndexConfigVideoBitrate:
   {
      OMX_VIDEO_CONFIG_BITRATETYPE* pParam = reinterpret_cast<OMX_VIDEO_CONFIG_BITRATETYPE*>(pCompConfig);
      // also need to set other params + config
      if (pParam->nPortIndex == (OMX_U32) PORT_INDEX_IN)
      {
      m_sInPortDef.format.video.nBitrate = pParam->nEncodeBitrate;
      }
      else
      {
      m_sOutPortDef.format.video.nBitrate = pParam->nEncodeBitrate;
     }
      OMX_VENC_MSG_HIGH("Setting Bitrate to %d from %d", pParam->nEncodeBitrate, m_sConfigBitrate.nEncodeBitrate, 0);
      memcpy(&m_sConfigBitrate, pParam, sizeof(m_sConfigBitrate));
      m_sParamBitrate.nTargetBitrate = pParam->nEncodeBitrate;
      m_sConfigBitrate.nEncodeBitrate= pParam->nEncodeBitrate;
      if( m_bDeviceStart == OMX_TRUE )
      {
        if(change_quality() != OMX_ErrorNone) 
      {
         OMX_VENC_MSG_ERROR("Failed to set Bitrate=%d", m_sConfigBitrate.nEncodeBitrate, 0, 0);
         return OMX_ErrorUndefined;
      }
      }
      else
      {
         m_bPendingChangeQuality = OMX_TRUE;
      }
      break;
   }
   case OMX_IndexConfigVideoFramerate:
   {
      OMX_CONFIG_FRAMERATETYPE* pParam = reinterpret_cast<OMX_CONFIG_FRAMERATETYPE*>(pCompConfig);
      OMX_VENC_MSG_HIGH("Setting Framerate to %d", pParam->xEncodeFramerate, 0, 0);
      memcpy(&m_sConfigFramerate, pParam, sizeof(m_sConfigFramerate));
      if (pParam->nPortIndex == (OMX_U32) PORT_INDEX_IN)
      {
      m_sInPortDef.format.video.xFramerate = pParam->xEncodeFramerate;
      m_sInPortFormat.xFramerate = pParam->xEncodeFramerate;
      }
      else
      {
      m_sOutPortDef.format.video.xFramerate = pParam->xEncodeFramerate;
      m_sOutPortFormat.xFramerate = pParam->xEncodeFramerate;
      }
      if( m_bDeviceStart == OMX_TRUE )
      {
         if(change_quality() != OMX_ErrorNone)
      {
         OMX_VENC_MSG_ERROR("Failed to set Framerate=%d", m_sConfigFramerate.xEncodeFramerate, 0, 0);
         return OMX_ErrorUndefined;
      }
      }
      else
      {
         m_bPendingChangeQuality = OMX_TRUE;
      }
      break;
   }
   case OMX_IndexConfigCommonRotate:
   {
      OMX_CONFIG_ROTATIONTYPE* pParam = reinterpret_cast<OMX_CONFIG_ROTATIONTYPE*>(pCompConfig);
      memcpy(&m_sConfigFrameRotation, pParam, sizeof(m_sConfigFrameRotation));
      break;
   }
case QOMX_IndexConfigVideoIntraRefresh:
   {
      OMX_VIDEO_PARAM_INTRAREFRESHTYPE* pParam = reinterpret_cast<OMX_VIDEO_PARAM_INTRAREFRESHTYPE*>(pCompConfig);
      memcpy(&m_sParamIntraRefresh, pParam, sizeof(m_sParamIntraRefresh));
	  venc_set_intra_refresh(m_nDLHandle, m_sParamIntraRefresh.nAirMBs);
      break;
   }
   case QOMX_IndexConfigVideoIntraperiod:
   {
      QOMX_VIDEO_INTRAPERIODTYPE* pParam = reinterpret_cast<QOMX_VIDEO_INTRAPERIODTYPE*>(pCompConfig);
      memcpy(&m_sConfigIntraPeriod, pParam, sizeof(m_sConfigIntraPeriod));
      m_sParamMPEG4.nPFrames = m_sConfigIntraPeriod.nPFrames;
      m_sParamH263.nPFrames = m_sConfigIntraPeriod.nPFrames;
      m_sParamAVC.nPFrames = m_sConfigIntraPeriod.nPFrames;

      if(venc_set_intra_period(m_nDLHandle, m_sConfigIntraPeriod.nPFrames+1) != VENC_STATUS_SUCCESS)
      {
         OMX_VENC_MSG_ERROR("Failed to change Intra Period",0,0,0);
      }
      break;
   }
   case QOMX_IndexConfigVideoTemporalSpatialTradeOff:
   {
      QOMX_VIDEO_TEMPORALSPATIALTYPE* pParam = reinterpret_cast<QOMX_VIDEO_TEMPORALSPATIALTYPE*>(pCompConfig);
      if (pParam->nTSFactor > 0 && pParam->nTSFactor <= 100)
      {
         memcpy(&m_sTsTradeoffFactor, pParam, sizeof(m_sTsTradeoffFactor));
         if ((m_sOutPortDef.format.video.eCompressionFormat == OMX_VIDEO_CodingMPEG4) ||
             (m_sOutPortDef.format.video.eCompressionFormat == OMX_VIDEO_CodingH263))
         {
			  quality.max_qp = 8 + (int) roundingup(m_sTsTradeoffFactor.nTSFactor * 0.23);
         }
         else if (m_sOutPortDef.format.video.eCompressionFormat == OMX_VIDEO_CodingAVC)
         {
			  quality.max_qp = 33 + roundingup(m_sTsTradeoffFactor.nTSFactor * 0.18);
         }
		  quality.min_qp = 2; // Hardcode this value to 2 for all codecs.
         if( m_bDeviceStart == OMX_TRUE )
   {
        if(change_quality() != OMX_ErrorNone)
	  {
               OMX_VENC_MSG_ERROR("Failed to set SpatialTemporal TradeOff=(%d,%d)", m_sTsTradeoffFactor.nTSFactor,0,0);
               return OMX_ErrorUndefined;
            }
         }
         else
         {
            m_bPendingChangeQuality = OMX_TRUE;
         }
      }
      else
      {
         OMX_VENC_MSG_ERROR("Incorrect Value sent for SpatialTemporal TradeOff=(%d,%d)", m_sTsTradeoffFactor.nTSFactor,0,0);
         return OMX_ErrorUndefined;
   	  }
      break;
   }
   case OMX_IndexConfigVideoIntraVOPRefresh:
	   {
		   OMX_CONFIG_INTRAREFRESHVOPTYPE* pParam = reinterpret_cast<OMX_CONFIG_INTRAREFRESHVOPTYPE*>(pCompConfig);
		  memcpy(&m_sConfigIntraRefreshVOP, pParam, sizeof(m_sConfigIntraRefreshVOP)); 
		  if(m_sConfigIntraRefreshVOP.nPortIndex != PORT_INDEX_OUT)
		   {
			   OMX_VENC_MSG_ERROR("Request for I frame Command not supported on this port",0,0,0);
			   return OMX_ErrorBadPortIndex ;
		   }
		   else
		   {
			   if(m_sConfigIntraRefreshVOP.IntraRefreshVOP == OMX_TRUE)
			   {
				   
				   if(venc_request_key_frame(m_nDLHandle) != VENC_STATUS_SUCCESS)
				   {
					   OMX_VENC_MSG_ERROR("Failed to set I frame",0,0,0);
				   }
			   }
			   else
			   {
				   OMX_VENC_MSG_ERROR("Request for I frame Command not supported in this state",0,0,0);
			   }
		   }
		   break;
	   }
   default:
      OMX_VENC_MSG_ERROR("unsupported index %d", (int) nIndex, 0, 0);
       return OMX_ErrorUnsupportedIndex;
   }
   return OMX_ErrorNone; 
}

OMX_ERRORTYPE Venc::change_quality()
{
	quality.target_bitrate = m_sConfigBitrate.nEncodeBitrate;
	quality.max_frame_rate = m_sConfigFramerate.xEncodeFramerate >> 16;
	quality.scaling_factor = 0;
#ifndef FEATURE_LINUX_A
	quality.scaling_factor = m_sTimeStampScale.nTimestampScale >> 16;
    if(quality.scaling_factor > 1)
    {
      quality.target_bitrate = quality.target_bitrate/quality.scaling_factor;   
      quality.max_frame_rate = quality.max_frame_rate/quality.scaling_factor;
   }
#endif
	if ((venc_change_quality(m_nDLHandle, &quality)) != VENC_STATUS_SUCCESS)
	{
		OMX_VENC_MSG_ERROR("Failed to change the Quality ", 0,0,0);
		return OMX_ErrorUndefined;
	}
	return OMX_ErrorNone;
}


OMX_ERRORTYPE Venc::get_extension_index(OMX_IN  OMX_HANDLETYPE hComponent,
                                        OMX_IN  OMX_STRING cParameterName,
                                        OMX_OUT OMX_INDEXTYPE* pIndexType)
{

   OMX_VENC_MSG_HIGH("Entered get_extension_index",0,0,0);
   (void) hComponent;
   if(NULL == cParameterName)
   {
	   OMX_VENC_MSG_ERROR("Invalid Component name",0,0,0);
	   return OMX_ErrorBadParameter;
   }
   if(NULL == pIndexType)
   {
	  OMX_VENC_MSG_ERROR("Invalid Indextype",0,0,0);
	  return OMX_ErrorBadParameter;
   }
   if(strcmp(cParameterName, "OMX.QCOM.index.param.syntaxhdr") == 0)
      *pIndexType = (OMX_INDEXTYPE)(QOMX_IndexParamVideoSyntaxHdr);
   else if(strcmp(cParameterName, "OMX.QCOM.index.config.intraperiod") == 0)
      *pIndexType = (OMX_INDEXTYPE)(QOMX_IndexConfigVideoIntraperiod);
   else if(strcmp(cParameterName, "OMX.QCOM.index.config.randomIntrarefresh") == 0)
      *pIndexType = (OMX_INDEXTYPE)(QOMX_IndexConfigVideoIntraRefresh);
   else if(strcmp(cParameterName, "OMX.QCOM.index.config.video.TemporalSpatialTradeOff") == 0)
      *pIndexType = (OMX_INDEXTYPE)(QOMX_IndexConfigVideoTemporalSpatialTradeOff);
   else
   {
      OMX_VENC_MSG_ERROR("Unsupported ParameterName", 0,0,0);
	     return OMX_ErrorBadParameter;
   }
   return OMX_ErrorNone;
}


OMX_ERRORTYPE Venc::get_state(OMX_IN  OMX_HANDLETYPE hComponent,
                              OMX_OUT OMX_STATETYPE* pState)
{
   (void) hComponent;
   if (pState == NULL)
   {
      return OMX_ErrorBadParameter;
   }
   *pState = m_eState;
   return OMX_ErrorNone; 
}

    
OMX_ERRORTYPE Venc::component_tunnel_request(OMX_IN  OMX_HANDLETYPE hComp,
                                             OMX_IN  OMX_U32 nPort,
                                             OMX_IN  OMX_HANDLETYPE hTunneledComp,
                                             OMX_IN  OMX_U32 nTunneledPort,
                                             OMX_INOUT  OMX_TUNNELSETUPTYPE* pTunnelSetup)
{
   (void) hComp;
   (void) nPort;
   (void) hTunneledComp;
   (void) nTunneledPort;
   (void) pTunnelSetup;
   return OMX_ErrorTunnelingUnsupported;
} 

OMX_ERRORTYPE Venc::use_buffer(OMX_IN OMX_HANDLETYPE hComponent,
                               OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
                               OMX_IN OMX_U32 nPortIndex,
                               OMX_IN OMX_PTR pAppPrivate,
                               OMX_IN OMX_U32 nSizeBytes,
                               OMX_IN OMX_U8* pBuffer)
{

   if (ppBufferHdr == NULL || nSizeBytes == 0 || pBuffer == NULL)
   {
      OMX_VENC_MSG_ERROR("bad param 0x%p %ld 0x%p",ppBufferHdr, nSizeBytes, pBuffer);
      return OMX_ErrorBadParameter;
   }

   if(m_eState == OMX_StateInvalid)
   {
      OMX_VENC_MSG_ERROR("UseBuffer - called in Invalid State", 0, 0, 0);
      return OMX_ErrorInvalidState;
   }

   if (nPortIndex == (OMX_U32) PORT_INDEX_IN)
   {
      if(m_in_alloc_cnt >= m_sInPortDef.nBufferCountActual)
      {
          OMX_VENC_MSG_ERROR("Trying to allocate more buffers than actual buffer count", 0, 0, 0);
          return OMX_ErrorInsufficientResources;
      }
      OMX_VENC_MSG_HIGH("client allocated input buffer for component %ld",m_sInPortDef.nBufferSize, 0, 0);
      if(nSizeBytes != m_sInPortDef.nBufferSize)
      {
         OMX_VENC_MSG_ERROR("buffer size does not match our requirements: client ask for(%ld) and we require (%ld)", nSizeBytes, m_sInPortDef.nBufferSize, 0);
         return OMX_ErrorBadParameter;
      }

      /* Allocation of Input Buffer headers done only in AllocateBuffer
         or UseBuffer. Allocation of structures done only once */
      if(0 == m_in_alloc_cnt)
      {
         m_sInBuffHeaders = (OMX_BUFFERHEADERTYPE*)calloc
                            (sizeof(OMX_BUFFERHEADERTYPE), m_sInPortDef.nBufferCountActual);
         if(NULL == m_sInBuffHeaders)
         {
            OMX_VENC_MSG_ERROR("Insufficent memory", 0, 0, 0);
            return OMX_ErrorInsufficientResources;
         }

         m_sPrivateInPortData = (PrivatePortData*)calloc
                                (sizeof(PrivatePortData), m_sInPortDef.nBufferCountActual);

         if(NULL == m_sPrivateInPortData)
         {
            free(m_sInBuffHeaders);
            m_sInBuffHeaders = NULL;
            OMX_VENC_MSG_ERROR("Insufficent memory", 0, 0, 0);
            return OMX_ErrorInsufficientResources;
         }
      }

      OMX_INIT_STRUCT(&m_sInBuffHeaders[m_in_alloc_cnt], OMX_BUFFERHEADERTYPE);
      m_sInBuffHeaders[m_in_alloc_cnt].pBuffer = pBuffer;
      m_sInBuffHeaders[m_in_alloc_cnt].nAllocLen = nSizeBytes;
      m_sInBuffHeaders[m_in_alloc_cnt].pAppPrivate = pAppPrivate;
      m_sInBuffHeaders[m_in_alloc_cnt].nInputPortIndex = (OMX_U32) PORT_INDEX_IN;
      m_sInBuffHeaders[m_in_alloc_cnt].nOutputPortIndex = (OMX_U32) PORT_INDEX_NONE;
      m_sInBuffHeaders[m_in_alloc_cnt].pInputPortPrivate = &m_sPrivateInPortData[m_in_alloc_cnt];
      m_sPrivateInPortData[m_in_alloc_cnt].bComponentAllocated = OMX_FALSE;  
      *ppBufferHdr = &m_sInBuffHeaders[m_in_alloc_cnt];
      m_in_alloc_cnt++;

      if(m_in_alloc_cnt == m_sInPortDef.nBufferCountActual)
      {
         OMX_VENC_MSG_LOW("I/P port populated", 0, 0, 0);
         m_sInPortDef.bPopulated = OMX_TRUE;
	 m_bIsQcomPvt = OMX_TRUE;
         if(BITMASK_PRESENT(m_flags, OMX_COMPONENT_INPUT_ENABLE_PENDING))
         {
            OMX_VENC_MSG_LOW("sending event for I/P port enable complete", 0, 0, 0);
            BITMASK_CLEAR((m_flags), OMX_COMPONENT_INPUT_ENABLE_PENDING);
            m_sCallbacks.EventHandler(m_hSelf,
                                       m_pAppData,
                                       OMX_EventCmdComplete,
                                       (OMX_U32) OMX_CommandPortEnable,
                                       (OMX_U32) PORT_INDEX_IN,
                                       NULL);
         }
      }
   }
   else if (nPortIndex == (OMX_U32) PORT_INDEX_OUT)
   {
      OMX_VENC_MSG_HIGH("client allocated output buffer for component of size %ld",m_sOutPortDef.nBufferSize, 0, 0);
      if(m_out_alloc_cnt >= m_sOutPortDef.nBufferCountActual)
      {
          OMX_VENC_MSG_ERROR("Trying to allocate more buffers than actual buffer count", 0, 0, 0);
          return OMX_ErrorInsufficientResources;
      }
      if (nSizeBytes != m_sOutPortDef.nBufferSize)
      {
           OMX_VENC_MSG_ERROR("buffer size does not match our requirements: client ask for(%ld) and we require (%ld)", nSizeBytes, m_sOutPortDef.nBufferSize, 0);
         return OMX_ErrorBadParameter;
      }

      /* Allocation of Input Buffer headers done only in AllocateBuffer
         or UseBuffer. Allocation of structures done only once */
      if(0 == m_out_alloc_cnt)
      {
         m_sOutBuffHeaders = (OMX_BUFFERHEADERTYPE*)calloc
                             (sizeof(OMX_BUFFERHEADERTYPE), m_sOutPortDef.nBufferCountActual);
         if(NULL == m_sOutBuffHeaders)
         {
            OMX_VENC_MSG_ERROR("Insufficent memory", 0, 0, 0);
            return OMX_ErrorInsufficientResources;
         }

         m_sPrivateOutPortData = (PrivatePortData*)calloc
                                 (sizeof(PrivatePortData), m_sOutPortDef.nBufferCountActual);

         if(NULL == m_sPrivateOutPortData)
         {
            free(m_sOutBuffHeaders);
            m_sOutBuffHeaders = NULL;
            OMX_VENC_MSG_ERROR("Insufficent memory", 0, 0, 0);
            return OMX_ErrorInsufficientResources;
         }
      }
      
      OMX_INIT_STRUCT(&m_sOutBuffHeaders[m_out_alloc_cnt], OMX_BUFFERHEADERTYPE);
      m_sOutBuffHeaders[m_out_alloc_cnt].pBuffer = pBuffer;
      m_sOutBuffHeaders[m_out_alloc_cnt].nAllocLen = nSizeBytes;
      m_sOutBuffHeaders[m_out_alloc_cnt].pAppPrivate = pAppPrivate;
      m_sOutBuffHeaders[m_out_alloc_cnt].nInputPortIndex = (OMX_U32) PORT_INDEX_NONE;
      m_sOutBuffHeaders[m_out_alloc_cnt].nOutputPortIndex = (OMX_U32) PORT_INDEX_OUT;
      m_sOutBuffHeaders[m_out_alloc_cnt].pOutputPortPrivate = &m_sPrivateOutPortData[m_out_alloc_cnt];
      m_sPrivateOutPortData[m_out_alloc_cnt].bComponentAllocated = OMX_FALSE;
      *ppBufferHdr = &m_sOutBuffHeaders[m_out_alloc_cnt];
      m_out_alloc_cnt++;

      if(m_out_alloc_cnt == m_sOutPortDef.nBufferCountActual)
      {
         OMX_VENC_MSG_LOW("O/P port populated", 0, 0, 0);
         m_sOutPortDef.bPopulated = OMX_TRUE;
         if(BITMASK_PRESENT(m_flags, OMX_COMPONENT_OUTPUT_ENABLE_PENDING))
         {
            OMX_VENC_MSG_LOW("sending event complete for O/P port enable", 0, 0, 0);                       
            BITMASK_CLEAR((m_flags), OMX_COMPONENT_OUTPUT_ENABLE_PENDING);
            m_sCallbacks.EventHandler(m_hSelf,
                                       m_pAppData,
                                       OMX_EventCmdComplete,
                                       (OMX_U32) OMX_CommandPortEnable,
                                       (OMX_U32) PORT_INDEX_OUT,
                                       NULL);
         }
      }
   }
   else
   {
      OMX_VENC_MSG_ERROR("invalid port index", 0, 0, 0);
      return OMX_ErrorBadPortIndex;
   }
   //Check if all the buffers have been allocated and the encoder has been initialized
   if(allocate_done() && m_bDeviceInit_done == OMX_TRUE)
   {
      if(BITMASK_PRESENT(m_flags, OMX_COMPONENT_IDLE_PENDING))
      {
         OMX_VENC_MSG_LOW("Sending event complete for state transition from loaded to Idle", 0, 0, 0);
         BITMASK_CLEAR((m_flags), OMX_COMPONENT_IDLE_PENDING);
         m_eState = OMX_StateIdle;
         m_sCallbacks.EventHandler(m_hSelf,
                                    m_pAppData,
                                    OMX_EventCmdComplete,
                                    (OMX_U32) OMX_CommandStateSet,
                                    (OMX_U32) m_eState, NULL);
      }
   }
   return OMX_ErrorNone;
}

OMX_ERRORTYPE Venc::allocate_buffer(OMX_IN OMX_HANDLETYPE hComponent,
                                    OMX_INOUT OMX_BUFFERHEADERTYPE** ppBuffer,
                                    OMX_IN OMX_U32 nPortIndex,
                                    OMX_IN OMX_PTR pAppPrivate,
                                    OMX_IN OMX_U32 nSizeBytes)
{
   if (ppBuffer == NULL)
   {
      return OMX_ErrorBadParameter;
   }

   if(m_eState == OMX_StateInvalid)
   {
      OMX_VENC_MSG_ERROR("AllocateBuffer - called in Invalid State", 0, 0, 0);
      return OMX_ErrorInvalidState;
   }
  
   if (nPortIndex == (OMX_U32) PORT_INDEX_IN)
   {
    OMX_VENC_MSG_HIGH("Attempt to allocate buffer of %ld bytes for INPUT PORT", m_sInPortDef.nBufferSize, 0, 0);  
      if(m_in_alloc_cnt >= m_sInPortDef.nBufferCountActual)
      {
          OMX_VENC_MSG_ERROR("Trying to allocate more buffers than actual buffer count", 0, 0, 0);
          return OMX_ErrorInsufficientResources;
      }
      if(nSizeBytes != m_sInPortDef.nBufferSize)
      {
         OMX_VENC_MSG_ERROR("buffer size does not match our requirements %ld %ld",
           nSizeBytes, m_sInPortDef.nBufferSize, 0);
         return OMX_ErrorBadParameter;
      }
      
      OMX_BUFFERHEADERTYPE* pBuffer = NULL;
      PrivatePortData* pPrivateData = NULL;
      /* Allocation of Input Buffer headers done only in AllocateBuffer
         or UseBuffer. Allocation of structures done only once */
      if(0 == m_in_alloc_cnt)
      {
         m_sInBuffHeaders = (OMX_BUFFERHEADERTYPE*)calloc
                            (sizeof(OMX_BUFFERHEADERTYPE), m_sInPortDef.nBufferCountActual);

         if(NULL == m_sInBuffHeaders)
         {
            OMX_VENC_MSG_ERROR("Insufficent memory", 0, 0, 0);
            return OMX_ErrorInsufficientResources;
         }

         m_sPrivateInPortData = (PrivatePortData*)calloc
                                (sizeof(PrivatePortData), m_sInPortDef.nBufferCountActual);

         if(NULL == m_sPrivateInPortData)
         {
            free(m_sInBuffHeaders);
            m_sInBuffHeaders = NULL;
            OMX_VENC_MSG_ERROR("Insufficent memory", 0, 0, 0);
            return OMX_ErrorInsufficientResources;
         }
      }

      pBuffer = &m_sInBuffHeaders[m_in_alloc_cnt];
      pPrivateData = &m_sPrivateInPortData[m_in_alloc_cnt];

      if (venc_pmem_alloc((uint32) this->m_nDLHandle,
                          nSizeBytes,
                          &pPrivateData->sPmemInfo) != VENC_STATUS_SUCCESS)
      {
         OMX_VENC_MSG_ERROR("failed to allocate buffer", 0, 0, 0);
         return OMX_ErrorInsufficientResources;
      }

      OMX_INIT_STRUCT(pBuffer, OMX_BUFFERHEADERTYPE);
      pBuffer->pBuffer = (OMX_U8*) pPrivateData->sPmemInfo.buff;
      OMX_VENC_MSG_LOW("nPortIndex - %ld, allocated 0x%p", nPortIndex, pBuffer->pBuffer, 0);
      if (pBuffer->pBuffer != NULL)
      {
         pBuffer->nAllocLen = nSizeBytes;
         pBuffer->pAppPrivate = pAppPrivate;
         pBuffer->nInputPortIndex = (OMX_U32) PORT_INDEX_IN;
         pBuffer->nOutputPortIndex = (OMX_U32) PORT_INDEX_NONE;
         pBuffer->pInputPortPrivate = pPrivateData;
         pPrivateData->bComponentAllocated = OMX_TRUE;
         *ppBuffer = pBuffer;
         m_in_alloc_cnt++;
      }
      else
      {
         OMX_VENC_MSG_ERROR("failed to allocate buffer", 0, 0, 0);
         return OMX_ErrorInsufficientResources;
      }
      if(m_in_alloc_cnt == m_sInPortDef.nBufferCountActual)
      {
         OMX_VENC_MSG_LOW("I/P port populated", 0, 0, 0);        
         m_sInPortDef.bPopulated = OMX_TRUE;
         if(BITMASK_PRESENT(m_flags, OMX_COMPONENT_INPUT_ENABLE_PENDING))
         {
            OMX_VENC_MSG_LOW("sending event for I/P port enable complete", 0, 0, 0);
            BITMASK_CLEAR((m_flags), OMX_COMPONENT_INPUT_ENABLE_PENDING);
            m_sCallbacks.EventHandler(m_hSelf,
                                       m_pAppData,
                                       OMX_EventCmdComplete,
                                       (OMX_U32) OMX_CommandPortEnable,
                                       (OMX_U32) PORT_INDEX_IN,
                                       NULL);
         }
      }
   }
   else if (nPortIndex == (OMX_U32) PORT_INDEX_OUT)
   {
      OMX_VENC_MSG_HIGH("Component Allocate O/P PORT buffer of %ld bytes", m_sOutPortDef.nBufferSize, 0, 0);  
      if(m_out_alloc_cnt >= m_sOutPortDef.nBufferCountActual)
      {
          OMX_VENC_MSG_ERROR("Trying to allocate more buffers than actual buffer count", 0, 0, 0);
          return OMX_ErrorInsufficientResources;
      }
      if(nSizeBytes != m_sOutPortDef.nBufferSize)
      {
         OMX_VENC_MSG_ERROR("buffer size does not match our requirements %ld %ld",
           nSizeBytes, m_sOutPortDef.nBufferSize, 0);
         return OMX_ErrorBadParameter;
      }

      OMX_BUFFERHEADERTYPE* pBuffer = NULL;
      PrivatePortData* pPrivateData = NULL;

      /* Allocation of Input Buffer headers done only in AllocateBuffer
                  or UseBuffer. Allocation of structures done only once */
      if(0 == m_out_alloc_cnt)
      {
         m_sOutBuffHeaders = (OMX_BUFFERHEADERTYPE*)calloc
                             (sizeof(OMX_BUFFERHEADERTYPE), m_sOutPortDef.nBufferCountActual);

         if(NULL == m_sOutBuffHeaders)
         {
            OMX_VENC_MSG_ERROR("Insufficent memory", 0, 0, 0);
            return OMX_ErrorInsufficientResources;
         }

         m_sPrivateOutPortData = (PrivatePortData*)calloc
                                 (sizeof(PrivatePortData), m_sOutPortDef.nBufferCountActual);

         if(NULL == m_sPrivateOutPortData)
         {
            free(m_sOutBuffHeaders);
            m_sOutBuffHeaders = NULL;
            OMX_VENC_MSG_ERROR("Insufficent memory", 0, 0, 0);
            return OMX_ErrorInsufficientResources;
         }
      }

      pBuffer = &m_sOutBuffHeaders[m_out_alloc_cnt];
      pPrivateData = &m_sPrivateOutPortData[m_out_alloc_cnt];

      OMX_INIT_STRUCT(pBuffer, OMX_BUFFERHEADERTYPE);
      pBuffer->pBuffer = (OMX_U8*) malloc(nSizeBytes);
      OMX_VENC_MSG_LOW("allocated 0x%p", pBuffer->pBuffer, 0, 0);
      if (pBuffer->pBuffer != NULL)
      {
         pBuffer->nAllocLen = nSizeBytes;
         pBuffer->pAppPrivate = pAppPrivate;
         pBuffer->nInputPortIndex = (OMX_U32) PORT_INDEX_NONE;
         pBuffer->nOutputPortIndex = (OMX_U32) PORT_INDEX_OUT;
         pBuffer->pOutputPortPrivate = pPrivateData;
         pPrivateData->bComponentAllocated = OMX_TRUE;
         *ppBuffer = pBuffer;
         m_out_alloc_cnt++;
      }
      else
      {
         OMX_VENC_MSG_ERROR("failed to allocate buffer", 0, 0, 0);
         return OMX_ErrorInsufficientResources;
      }
      if(m_out_alloc_cnt == m_sOutPortDef.nBufferCountActual)
      {
         OMX_VENC_MSG_LOW("O/P port populated", 0, 0, 0);
         m_sOutPortDef.bPopulated = OMX_TRUE;
         if(BITMASK_PRESENT(m_flags, OMX_COMPONENT_OUTPUT_ENABLE_PENDING))
         {
            OMX_VENC_MSG_LOW("sending event complete for O/P port enable", 0, 0, 0);            
            BITMASK_CLEAR((m_flags), OMX_COMPONENT_OUTPUT_ENABLE_PENDING);
            m_sCallbacks.EventHandler(m_hSelf,
                                       m_pAppData,
                                       OMX_EventCmdComplete,
                                       OMX_CommandPortEnable,
                                       (OMX_U32) PORT_INDEX_OUT,
                                       NULL);
         }
      }
   }
   else
   {
      // we only allocate for the output port
      // use buffer must be called on the input port
      OMX_VENC_MSG_ERROR("unsupported port index for allocation", 0, 0, 0);
      return OMX_ErrorBadPortIndex;
   }
   //Check if all the buffers have been allocated and the encoder has been initialized 
   if(allocate_done()&& m_bDeviceInit_done == OMX_TRUE)
   {
      if(BITMASK_PRESENT(m_flags, OMX_COMPONENT_IDLE_PENDING))
      {
         OMX_VENC_MSG_LOW("Sending event complete for state transition from loaded to Idle", 0, 0, 0);
         BITMASK_CLEAR((m_flags), OMX_COMPONENT_IDLE_PENDING);
         m_eState = OMX_StateIdle;
         m_sCallbacks.EventHandler(m_hSelf,
                                    m_pAppData,
                                    OMX_EventCmdComplete,
                                    (OMX_U32) OMX_CommandStateSet,
                                    (OMX_U32) m_eState, NULL);
      }
   }
   return OMX_ErrorNone; 
}

/**********************************************************************//**
 * @brief release_done
 *
 * checks if all the I/P and O/P buffers have been released
 * 
 *************************************************************************/
OMX_BOOL Venc::release_done(void)
{
   OMX_BOOL bRet = OMX_FALSE;

   if((m_out_alloc_cnt == 0)&& (m_in_alloc_cnt == 0))
   {
      bRet = OMX_TRUE;
   }
   return bRet;
}


OMX_ERRORTYPE Venc::free_buffer(OMX_IN  OMX_HANDLETYPE hComponent,
                                OMX_IN  OMX_U32 nPortIndex,
                                OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
   OMX_ERRORTYPE eRet = OMX_ErrorNone;
   if (pBuffer == NULL)
   {
      OMX_VENC_MSG_ERROR("null param", 0, 0, 0);
      return OMX_ErrorBadParameter;
   }

   if(m_eState == OMX_StateIdle && (BITMASK_PRESENT(m_flags, OMX_COMPONENT_LOADING_PENDING)))
   {
      OMX_VENC_MSG_LOW(" Free buffer while Component in Loading pending ", 0, 0, 0);
   }
   else if((m_sInPortDef.bEnabled == OMX_FALSE && nPortIndex == PORT_INDEX_IN)
           || (m_sOutPortDef.bEnabled == OMX_FALSE && nPortIndex == PORT_INDEX_OUT))
   {
      OMX_VENC_MSG_LOW("Free Buffer while port %d disabled", nPortIndex, 0, 0);
   }
   else if(m_eState == OMX_StateExecuting || m_eState == OMX_StatePause)
   {
      OMX_VENC_MSG_ERROR(" Invalid state to free buffer", 0, 0, 0);
      m_sCallbacks.EventHandler(m_hSelf,
                                 m_pAppData,
                                 OMX_EventError,
                                 (OMX_U32) OMX_ErrorPortUnpopulated,
                                 (OMX_U32) m_eState, 0);
      return eRet;
   }
   else
   {
      OMX_VENC_MSG_ERROR(" Invalid state to free buffer, buffer lost", 0, 0, 0);

      m_sCallbacks.EventHandler(m_hSelf,
                                 m_pAppData,
                                 OMX_EventError,
                                 (OMX_U32) OMX_ErrorPortUnpopulated,
                                 (OMX_U32) m_eState, 0);
   }
   OMX_VENC_MSG_LOW("freeing 0x%p", pBuffer->pBuffer, 0, 0);

   if (nPortIndex == PORT_INDEX_OUT)
   {
      PrivatePortData* pPortData = 
         ((PrivatePortData*) pBuffer->pOutputPortPrivate);
      if(m_out_alloc_cnt == 0)
      {
          OMX_VENC_MSG_ERROR("No more buffers to free on the O/P port", 0, 0, 0);
          return OMX_ErrorUndefined;
      }
      if(pPortData->bComponentAllocated)
      {
         for(int i = 0; i < (int) m_sOutPortDef.nBufferCountActual; i++)
         {
             if((pBuffer->pBuffer == m_sOutBuffHeaders[i].pBuffer))
             {
                OMX_VENC_MSG_HIGH("Component freeing output buffer 0x%p", pBuffer->pBuffer, 0, 0);
				m_sOutPortDef.bPopulated = OMX_FALSE;
                free(m_sOutBuffHeaders[i].pBuffer);
                m_sOutBuffHeaders[i].pBuffer = NULL;
                m_out_alloc_cnt--;
                break;
             }
         }
      }
      else 
      {
         OMX_VENC_MSG_LOW("client allocated buffer,just need to release the header", 0, 0, 0);
         m_out_alloc_cnt--;
         m_sOutPortDef.bPopulated = OMX_FALSE;
      }

      if(0 == m_out_alloc_cnt )
      {
         OMX_VENC_MSG_LOW("freeing the O/P buffer headers", 0, 0, 0);
         if(m_sPrivateOutPortData)
         {
            free(m_sPrivateOutPortData);
            m_sPrivateOutPortData = NULL;
         }
         if(m_sOutBuffHeaders)
         {
            free(m_sOutBuffHeaders);
            m_sOutBuffHeaders = NULL;
         }
      }

      if(BITMASK_PRESENT((m_flags), OMX_COMPONENT_OUTPUT_DISABLE_PENDING) && (m_out_alloc_cnt == 0))
      {
         OMX_VENC_MSG_LOW("sending event for O/P Port disable complete", 0, 0, 0);
         BITMASK_CLEAR((m_flags), OMX_COMPONENT_OUTPUT_DISABLE_PENDING);
         m_sCallbacks.EventHandler(m_hSelf,
                                    m_pAppData,
                                    OMX_EventCmdComplete,
                                    (OMX_U32) OMX_CommandPortDisable,
                                    (OMX_U32) PORT_INDEX_OUT,
                                    NULL);
      }
   }
   else if (nPortIndex == PORT_INDEX_IN)
   {
      PrivatePortData* pPortData = 
         ((PrivatePortData*) pBuffer->pInputPortPrivate);
      if(m_in_alloc_cnt == 0)
      {
         OMX_VENC_MSG_ERROR("No more buffers to free on the I/P port", 0, 0, 0);
         return OMX_ErrorUndefined;
      }

       if(pPortData->bComponentAllocated)
       {
          for(int i = 0; i < (int) m_sInPortDef.nBufferCountActual; i++)
          {
             if((pBuffer->pBuffer == m_sInBuffHeaders[i].pBuffer))
             {
                m_sInPortDef.bPopulated = OMX_FALSE;
		OMX_VENC_MSG_HIGH("Component freeing input buffer 0x%p", pBuffer->pBuffer, 0, 0);                
                m_in_alloc_cnt--;
                if (venc_pmem_free((uint32) this->m_nDLHandle,
                                   &pPortData->sPmemInfo) != VENC_STATUS_SUCCESS)
                {
                   OMX_VENC_MSG_ERROR("failed to free pmem", 0, 0, 0);
                }
                break;
             }
          }
       }
       else 
       {
          OMX_VENC_MSG_LOW("client allocated buffer,just need to release the header", 0, 0, 0);
          m_in_alloc_cnt--;
          m_sInPortDef.bPopulated = OMX_FALSE;
       }

       if(0 == m_in_alloc_cnt )
       {
          OMX_VENC_MSG_LOW("freeing the I/P buffer headers", 0, 0, 0);
          if(m_sPrivateInPortData)
          {
             free(m_sPrivateInPortData);
             m_sPrivateInPortData = NULL;
          }
          if(m_sInBuffHeaders)
          {
             free(m_sInBuffHeaders);
             m_sInBuffHeaders = NULL;
          }
        }

       if(BITMASK_PRESENT((m_flags), OMX_COMPONENT_INPUT_DISABLE_PENDING) && (m_in_alloc_cnt == 0))
       {
          OMX_VENC_MSG_LOW("sending event for I/P port disable complete", 0, 0, 0);
          BITMASK_CLEAR((m_flags), OMX_COMPONENT_INPUT_DISABLE_PENDING);
          m_sCallbacks.EventHandler(m_hSelf,
                                     m_pAppData,
                                     OMX_EventCmdComplete,
                                     (OMX_U32) OMX_CommandPortDisable,
                                     (OMX_U32) PORT_INDEX_IN,
                                      NULL);
       }
    }
    else
    {
       OMX_VENC_MSG_ERROR("Bad port index to free buffer", 0, 0, 0);
       eRet = OMX_ErrorBadParameter;
    }
    if(release_done()&& m_bDeviceExit_done == OMX_TRUE)
    {
       if(m_eState == OMX_StateIdle && BITMASK_PRESENT((m_flags), OMX_COMPONENT_LOADING_PENDING))
       {
          OMX_VENC_MSG_LOW("sending event for state transition from Idle to Loaded", 0, 0, 0);
          BITMASK_CLEAR((m_flags), OMX_COMPONENT_LOADING_PENDING);
          m_eState = OMX_StateLoaded;
          m_bDeviceExit_done = OMX_FALSE;
          m_sCallbacks.EventHandler(m_hSelf,                               
                                     m_pAppData,                            
                                     OMX_EventCmdComplete,                  
                                     OMX_CommandStateSet,                   
                                     m_eState,                                
                                     NULL);                                 
       }
    }
    return eRet;
}

OMX_ERRORTYPE Venc::encode_frame(OMX_BUFFERHEADERTYPE* pBuffer)
{

   OMX_BUFFERHEADERTYPE* pInput;
   OMX_BUFFERHEADERTYPE* pOutput;
   OMX_ERRORTYPE result = OMX_ErrorNone;
#define IS_INPUT(pBuffer) (pBuffer->nInputPortIndex == PORT_INDEX_IN)

   if (pBuffer == NULL)
   {
      OMX_VENC_MSG_ERROR("buffer is null", 0, 0, 0);
      return OMX_ErrorBadParameter;
   }

   /****************************************************
    * FIRST WE NEED AN INPUT BUFFER AND AN OUTPUT BUFFER
    ****************************************************/

   // if this is an input buffer we also need an output buffer
   if (IS_INPUT(pBuffer))
   {
      pInput = pBuffer;
      OMX_VENC_MSG_LOW("pInput->pBuffer:0x%p", pInput->pBuffer, 0, 0);
      OMX_U32 nBuff;
      (void) m_pFreeOutBM->GetNumBuffers(&nBuff);
      if (nBuff > 0)
      {
         OMX_VENC_MSG_LOW("we have an output buffer so lets encode", 0, 0, 0);

         result = m_pFreeOutBM->PopFirstBuffer(&pOutput);
         if (result != OMX_ErrorNone)
         {
            OMX_VENC_MSG_ERROR("failed to pop free output buffer", 0, 0, 0);
            return result;
         }
         if(pInput->hMarkTargetComponent != NULL && pInput->pMarkData != NULL)
         {
            OMX_VENC_MSG_LOW("Marking the O/P Buffer", 0, 0, 0);
            pOutput->hMarkTargetComponent = pInput->hMarkTargetComponent;
            pOutput->pMarkData = pInput->pMarkData;
         }
      }
      else
      {
         OMX_VENC_MSG_LOW("we have no output buffer so lets save input buffer", 0, 0, 0);

         // save input buffer until we get an output buffer
         result = m_pFreeInBM->PushBuffer(pInput);
         if (result != OMX_ErrorNone)
         {
            OMX_VENC_MSG_ERROR("failed to push input buffer", 0, 0, 0);
         }
         return result;
      }
   }
   // if this is an output buffer we also need an input buffer
   else
   {
      pOutput = pBuffer;

      if ((m_bGetSyntaxHdr == OMX_TRUE) && 
          (m_sOutPortFormat.eCompressionFormat == OMX_VIDEO_CodingMPEG4 || 
           m_sOutPortFormat.eCompressionFormat == OMX_VIDEO_CodingAVC))
      {
         int32 nSize;
         m_bGetSyntaxHdr = OMX_FALSE;
         translate_config(&m_sConfigParam);
         if (venc_get_syntax_hdr(m_nDLHandle, 
                                 (uint32*) pOutput->pBuffer, 
                                 pOutput->nAllocLen, 
                                 &nSize,&m_sConfigParam) != VENC_STATUS_SUCCESS)
         {
            OMX_VENC_MSG_ERROR("failed to push input buffer", 0, 0, 0);
            return OMX_ErrorUndefined; 
         }
         else
         {
            pOutput->nFilledLen = (OMX_U32) nSize;
            pOutput->nFlags = OMX_BUFFERFLAG_CODECCONFIG;
	    #ifdef VENC_DUMP_OUTPUT 
	         mem_dump((char*)pOutput->pBuffer,pOutput->nFilledLen,0);
            #endif
	    OMX_VENC_MSG_HIGH("pOutput->pBuffer:0x%p,pOutput->nFilledLen=%d",pOutput->pBuffer,pOutput->nFilledLen, 0);
		    result = m_sCallbacks.FillBufferDone(m_hSelf, m_pAppData, pOutput);
            if (result != OMX_ErrorNone)
            {
               OMX_VENC_MSG_ERROR("failed to invoke FBD", 0, 0, 0);
            }
            return result; 
         }
      }
      else
      {
         OMX_VENC_MSG_LOW("pOutput->pBuffer:0x%p", pOutput->pBuffer, 0, 0);
         OMX_U32 nBuff;
         (void) m_pFreeInBM->GetNumBuffers(&nBuff);
         if (nBuff > 0)
         {
            OMX_VENC_MSG_LOW("we have an input buffer so lets encode", 0, 0, 0);

            result = m_pFreeInBM->PopFirstBuffer(&pInput);
            if (result != OMX_ErrorNone)
            {
               OMX_VENC_MSG_ERROR("failed to pop free input buffer", 0, 0, 0);
               return result;
            }
            if(pInput->hMarkTargetComponent != NULL && pInput->pMarkData != NULL)
            {
               OMX_VENC_MSG_LOW("Marking the O/P Buffer", 0, 0, 0);
               pOutput->hMarkTargetComponent = pInput->hMarkTargetComponent;
               pOutput->pMarkData = pInput->pMarkData;
            }
         }
         else
         {
            OMX_VENC_MSG_LOW("we have no input buffer so lets save output buffer", 0, 0, 0);

            // save output buffer until we get an input buffer
            result = m_pFreeOutBM->PushBuffer(pOutput);
            if (result != OMX_ErrorNone)
            {
               OMX_VENC_MSG_ERROR("failed to push output buffer", 0, 0, 0);
            }
            return result;
         }
      }
   }

   // now we have both an input and an output buffer
   pOutput->nTimeStamp = pInput->nTimeStamp;
#ifndef FEATURE_LINUX_A
   //scale timestamp
   if((m_sTimeStampScale.nTimestampScale >> 16 ) > 1)
   {
      pOutput->nTimeStamp = pInput->nTimeStamp*
		                      (m_sTimeStampScale.nTimestampScale >> 16);
   }
#endif
   pOutput->nFilledLen = 0;
   pOutput->nOffset = 0;

   // Check for EOS.
   // NOTE: even if there is no data to encode, encoder requires
   // an input and output buffer to start EOS processing (per the OMX spec).

   if (pInput->nFlags & OMX_BUFFERFLAG_EOS)
   {
       // This is the end of stream, but this buffer is not empty
      OMX_VENC_MSG_HIGH("got OMX_BUFFERFLAG_EOS flag with nFilledLen=%ld,output_buffer=0x%x",pInput->nFilledLen,(uint32*)pOutput->pBuffer,0);

      pOutput->nFlags |= OMX_BUFFERFLAG_EOS;
   }
   else
   {
      // size can't be zero if eos is set
      if (pInput->nFilledLen == 0)
      {
         OMX_VENC_MSG_ERROR("EOS flag is not set, but size is 0",0,0,0);
         return OMX_ErrorUndefined;
      }
   }
   
   /****************************************************
    * NOW WE CAN ENCODE
    ****************************************************/
   venc_encode_frame_param_type param;
   if (m_bIsQcomPvt)
   {
     /* We dont require 3 level casting in Eclair base line , Need to clean this code */
      OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *pParam = reinterpret_cast<OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *>(pInput->pPlatformPrivate);
      OMX_VENC_MSG_MEDIUM("Inside qcom_ext with luma:(fd:%d ", pParam->pmem_fd,0,0);
      param.luma.buff = (void*) pInput->pBuffer;
      param.luma.handle = (void*) pParam->pmem_fd;
      param.luma.offset = pParam->offset;
      param.luma.size = pInput->nSize;
      param.chroma.handle = (void*) pParam->pmem_fd;
	   param.chroma.buff = pInput->pBuffer + m_sInPortDef.format.video.nFrameWidth * 
                          m_sInPortDef.format.video.nFrameHeight;
      param.chroma.offset = pParam->offset;
      param.chroma.offset += m_sInPortDef.format.video.nFrameWidth * 
         m_sInPortDef.format.video.nFrameHeight;
      param.chroma.size = pInput->nSize;
      #ifdef VENC_DUMP_INPUT
      mem_dump((char*)pInput->pBuffer,m_sInPortDef.format.video.nFrameWidth * m_sInPortDef.format.video.nFrameHeight*1.5,1) ;
      #endif
   }
   else
   {
      PrivatePortData* pPrivateData = (PrivatePortData*) pInput->pInputPortPrivate;

      OMX_VENC_MSG_MEDIUM("luma:(addr:0x%p)",pInput->pBuffer, 0, 0);
      param.luma = pPrivateData->sPmemInfo;
      param.chroma.handle = param.luma.handle ;
      param.chroma.buff = (void*)((uint8*)param.luma.buff + m_sInPortDef.format.video.nFrameWidth * 
                                                             m_sInPortDef.format.video.nFrameHeight);
      param.chroma.offset = param.luma.offset;
      param.chroma.offset += m_sInPortDef.format.video.nFrameWidth * 
         m_sInPortDef.format.video.nFrameHeight;
      #ifdef VENC_DUMP_INPUT
      mem_dump((char*)pInput->pBuffer,m_sInPortDef.format.video.nFrameWidth * m_sInPortDef.format.video.nFrameHeight*1.5,1) ;
      #endif
    }

   param.video_bitstream = (uint32*) pOutput->pBuffer;
   param.video_bitstream_size = pOutput->nAllocLen;
   param.timestamp = (uint64) pInput->nTimeStamp;
#ifndef FEATURE_LINUX_A
   if((m_sTimeStampScale.nTimestampScale >> 16 ) > 1)
   {
     param.timestamp = (uint64) (pInput->nTimeStamp*
		               (m_sTimeStampScale.nTimestampScale >> 16));
   }
#endif
   param.color = VENC_YUV420_PACKED_PLANAR;
   param.offset.x = 0;
   param.offset.y = 0;
   param.input_client_data = (void*) pInput;
   param.output_client_data = (void*) pOutput;

   if (pInput->nFlags & OMX_BUFFERFLAG_EOS)
   {
      param.eos = TRUE;
   }
   else
   {
      param.eos = FALSE;
   }

   // we have eos with zero length
   PrivatePortData* pOutPrivateData = (PrivatePortData*) pOutput->pOutputPortPrivate;
   if (pInput->nFilledLen == 0 && pInput->nFlags & OMX_BUFFERFLAG_EOS)
   {
      // Device layer does not support zero-sized input frames.
      // So device layer is going to encode this frame anyways.
      // When device layer is done, we are going to override the output frame length with zero.
      pOutPrivateData->bZeroLen = OMX_TRUE;
   }
   else
   {
      pOutPrivateData->bZeroLen = OMX_FALSE;
   }

   ++m_nInputDown;
   ++m_nOutputDown;

   venc_status_type status = venc_encode_frame(m_nDLHandle, &param);
   if (status != VENC_STATUS_SUCCESS)
   {
      OMX_VENC_MSG_ERROR("failed to encode", 0, 0, 0);

      --m_nInputDown;
      --m_nOutputDown;

      if (IS_INPUT(pBuffer))
      {
         // we need to put the output buffer back on free list
         m_pFreeOutBM->PushBuffer(pOutput);
      }
      else
      {
         // we need to put the input buffer back on free list
         m_pFreeInBM->PushBuffer(pInput);
      }
      return OMX_ErrorUndefined;
   }
   
   return result; 
}


OMX_ERRORTYPE Venc::empty_this_buffer(OMX_IN  OMX_HANDLETYPE hComponent,
                                      OMX_IN  OMX_BUFFERHEADERTYPE* pInBuffer)
{
   VencMsgQ::MsgIdType msgId;
   VencMsgQ::MsgDataType msgData;
   OMX_VENC_MSG_MEDIUM("emptying buffer...", 0, 0, 0);
   (void) hComponent;

   if ((pInBuffer == NULL) || (pInBuffer->nSize != sizeof(OMX_BUFFERHEADERTYPE)))
   {
      OMX_VENC_MSG_ERROR("buffer is null or buffer size is invalid", 0, 0, 0);
      return OMX_ErrorBadParameter;
   }
   if(pInBuffer->nVersion.nVersion != OMX_SPEC_VERSION)
   {
       OMX_VENC_MSG_ERROR("omx_venc::etb--> OMX Version Invalid", 0, 0, 0);       
       return OMX_ErrorVersionMismatch;
   }
   if (pInBuffer->nInputPortIndex != (OMX_U32)PORT_INDEX_IN)
   {
      OMX_VENC_MSG_ERROR("Bad port index to call empty_this_buffer", 0, 0, 0);
      return OMX_ErrorBadPortIndex;
   }
   if(!m_sInPortDef.bEnabled)
   {
      OMX_VENC_MSG_ERROR("can not call empty_this_buffer while I/P port is disabled", 0, 0, 0);
      return OMX_ErrorIncorrectStateOperation;
   }

   msgId = VencMsgQ::MSG_ID_EMPTY_BUFF;
   msgData.sEncodeBuffer.pBuffer = pInBuffer;
   msgData.sEncodeBuffer.hComponent = hComponent;
   if(m_pMsgQ->PushMsg(msgId, &msgData)!= OMX_ErrorNone)
   {
        OMX_VENC_MSG_LOW("Pushing the I/P buffer to component Queue failed", 0, 0, 0);
        m_sCallbacks.EventHandler(m_hSelf,
                                 m_pAppData,
                                 OMX_EventError,
                                 (OMX_U32) OMX_ErrorUndefined,
                                 (OMX_U32) m_eState, 0);
        return OMX_ErrorUndefined;
   }
   return OMX_ErrorNone;
}
/**********************************************************************//**
 * @brief Process thread message empty_this_buffer_proxy
 * processing of the empty_this_buffer in component context
 *
 *************************************************************************/
void Venc::empty_this_buffer_proxy(OMX_HANDLETYPE hComponent,
                                   OMX_BUFFERHEADERTYPE* pBuffer)
{

   if(pBuffer->hMarkTargetComponent && pBuffer->pMarkData)
   {
      if (pBuffer->hMarkTargetComponent == hComponent)
      {
         OMX_VENC_MSG_LOW("propagate mark and send mark through event call back", 0, 0, 0);
         m_Mark_port = OMX_TRUE;
      }
      else
      {
          OMX_VENC_MSG_LOW("Propagate mark from I/P to O/P buffers ", 0, 0, 0);
          
      }
      //if we have both propagate mark as well as mark command we need to ignore command
      m_Mark_command = OMX_FALSE;
   }
   else if(m_Mark_command)
   {
      OMX_VENC_MSG_LOW("Obtained mark from send_command ", 0, 0, 0);
      pBuffer->hMarkTargetComponent = m_Markdata.hMarkTargetComponent;
      pBuffer->pMarkData = m_Markdata.pMarkData;
      m_Mark_command = OMX_FALSE;
   }

   if (encode_frame(pBuffer) != OMX_ErrorNone)
   {
      OMX_VENC_MSG_ERROR("Empty this buffer failed", 0, 0, 0);
      m_sCallbacks.EventHandler(m_hSelf,
                                m_pAppData,
                                OMX_EventError,
                                (OMX_U32) OMX_ErrorUndefined,
                                (OMX_U32) m_eState, 0);
    }
}
/**********************************************************************//**
 * @brief Process thread message fill_this_buffer_proxy
 * processing of the fill_this_buffer in component context
 *
 *************************************************************************/
void Venc::fill_this_buffer_proxy(OMX_BUFFERHEADERTYPE *pBuffer)
{
    if(encode_frame(pBuffer)!= OMX_ErrorNone)
    {
        OMX_VENC_MSG_ERROR("Fill this buffer failed", 0, 0, 0);
        m_sCallbacks.EventHandler(m_hSelf,
                                 m_pAppData,
                                 OMX_EventError,
                                 (OMX_U32) OMX_ErrorUndefined,
                                 (OMX_U32) m_eState, 0);
    }
}

OMX_ERRORTYPE Venc::fill_this_buffer(OMX_IN  OMX_HANDLETYPE hComponent,
                                     OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{ 
   VencMsgQ::MsgIdType msgId;
   VencMsgQ::MsgDataType msgData;
   OMX_VENC_MSG_MEDIUM("filling buffer...", 0, 0, 0);
   (void) hComponent;
   if ((pBuffer == NULL) || (pBuffer->nSize != sizeof(OMX_BUFFERHEADERTYPE)))
   {
      OMX_VENC_MSG_ERROR("buffer is null or buffer size is invalid", 0, 0, 0);
      return OMX_ErrorBadParameter;
   }

   if(pBuffer->nVersion.nVersion != OMX_SPEC_VERSION)
   {
       OMX_VENC_MSG_ERROR("omx_venc::etb--> OMX Version Invalid", 0, 0, 0);
       return OMX_ErrorVersionMismatch;
   }

   if (pBuffer->nOutputPortIndex != (OMX_U32)PORT_INDEX_OUT)
   {
      OMX_VENC_MSG_ERROR("Bad port index to call fill_this_buffer", 0, 0, 0);
      return OMX_ErrorBadPortIndex;
   }

   if(!m_sOutPortDef.bEnabled)
   {
       OMX_VENC_MSG_ERROR("can not call fill_this_buffer while O/P port is disabled", 0, 0, 0);
       return OMX_ErrorIncorrectStateOperation;
   }
   msgId = VencMsgQ::MSG_ID_FILL_BUFF;
   msgData.sEncodeBuffer.pBuffer = pBuffer;
   if(m_pMsgQ->PushMsg(msgId, &msgData)!= OMX_ErrorNone)
   {
        OMX_VENC_MSG_LOW("Pushing the O/P buffer to component Queue failed", 0, 0, 0);
        m_sCallbacks.EventHandler(m_hSelf,
                                 m_pAppData,
                                 OMX_EventError,
                                 (OMX_U32) OMX_ErrorUndefined,
                                 (OMX_U32) m_eState, 0);
        return OMX_ErrorUndefined;

   }
   return OMX_ErrorNone;
}

OMX_ERRORTYPE Venc::set_callbacks(OMX_IN  OMX_HANDLETYPE hComponent,
                                  OMX_IN  OMX_CALLBACKTYPE* pCallbacks, 
                                  OMX_IN  OMX_PTR pAppData)
{
   if (pCallbacks == NULL || 
       hComponent == NULL ||
       pCallbacks->EmptyBufferDone == NULL ||
       pCallbacks->FillBufferDone == NULL || 
       pCallbacks->EventHandler == NULL)
   {
      return OMX_ErrorBadParameter;
   }

   m_sCallbacks.EventHandler = pCallbacks->EventHandler;
   m_sCallbacks.EmptyBufferDone = pCallbacks->EmptyBufferDone;
   m_sCallbacks.FillBufferDone = pCallbacks->FillBufferDone;
   m_pAppData = pAppData;
   m_hSelf = hComponent;
   
   return OMX_ErrorNone; 
}

OMX_ERRORTYPE Venc::component_deinit(OMX_IN  OMX_HANDLETYPE hComponent)
{
   OMX_ERRORTYPE result = OMX_ErrorNone;
   #ifdef VENC_DUMP_INPUT
   if(m_finput != NULL)
   venc_file_close(m_finput);
   #endif
   #ifdef VENC_DUMP_OUTPUT
   if(m_foutput != NULL)
   venc_file_close(m_foutput);
   #endif
   OMX_VENC_MSG_HIGH("deinitializing component...", 0, 0, 0);

   if (m_bComponentInitialized == OMX_TRUE)
   {
      if (m_eState == OMX_StateLoaded || m_eState == OMX_StateInvalid)
      {
         if (venc_unload(m_nDLHandle) != OMX_ErrorNone)
         {
            OMX_VENC_MSG_ERROR("failed to unload device layer", 0, 0, 0);
            result = OMX_ErrorUndefined;
         }
      }
      else if (m_eState == OMX_StateExecuting)
      {
         // set m_bPanic to OMX_TRUE so that
         // venc_stop will call venc_exit and venc_unload
         // to bring the state to OMX_StateInvalid
         m_bPanic = OMX_TRUE;
         OMX_VENC_MSG_ERROR("wrong state(OMX_StateExecuting)  call venc_stop", 0, 0, 0);
         if (m_bDeviceStart)
         {
            if (venc_stop(m_nDLHandle) != OMX_ErrorNone)
            {
               OMX_VENC_MSG_ERROR("failed to stop device layer", 0, 0, 0);
               result = OMX_ErrorUndefined;
            }
         }
         else if (m_bDeviceInit_done)
         {
            m_bDeviceExit_done = OMX_FALSE;
            if (venc_exit(m_nDLHandle) != OMX_ErrorNone)
            {
               OMX_VENC_MSG_ERROR("failed to stop device layer", 0, 0, 0);
               result = OMX_ErrorUndefined;
            }
         }
      }
      else if (m_eState == OMX_StateIdle)
      {
         // set m_bPanic to OMX_TRUE so that
         // venc_exit will call venc_unload
         // to bring the state to OMX_StateInvalid
         m_bPanic = OMX_TRUE;
         OMX_VENC_MSG_ERROR("wrong state(OMX_StateIdle) call venc_exit", 0, 0, 0);
         if (m_bDeviceInit_done)
         {
            m_bDeviceExit_done  = OMX_FALSE;
            if (venc_exit(m_nDLHandle) != OMX_ErrorNone)
            {
               OMX_VENC_MSG_ERROR("failed to stop device layer", 0, 0, 0);
               result = OMX_ErrorUndefined;
            }
         }
      }
      
      if (result == OMX_ErrorNone)
      {
         (void) venc_signal_wait(m_loadUnloadSignal, 2000);

         if (m_eState != OMX_StateInvalid)
         {
            OMX_VENC_MSG_ERROR("device layer unload failed", 0, 0, 0);
         }
         else
         {
            venc_sys_down(); 
         }
      }

      if (m_pMsgQ->PushMsg(VencMsgQ::MSG_ID_EXIT,
                           NULL) == OMX_ErrorNone)
      {
         if (venc_thread_destroy(m_thread, NULL) != 0)
         {
            OMX_VENC_MSG_ERROR("failed to destroy thread", 0, 0, 0);
            return OMX_ErrorUndefined;
         }
      }
      else
      {
         OMX_VENC_MSG_ERROR("failed to send thread exit msg", 0, 0, 0);
         return OMX_ErrorUndefined;
      } 

      if (m_pMsgQ)
         delete m_pMsgQ;
      if (m_pFreeInBM)
         delete m_pFreeInBM;
      if (m_pFreeOutBM)
         delete m_pFreeOutBM;
      if (m_pComponentName)
         free(m_pComponentName); 
   }

   m_bComponentInitialized = OMX_FALSE;

   OMX_VENC_MSG_HIGH("Encoder exit", 0, 0, 0);

   return result; 
}

OMX_ERRORTYPE Venc::use_EGL_image(OMX_IN OMX_HANDLETYPE hComponent,
                                  OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
                                  OMX_IN OMX_U32 nPortIndex,
                                  OMX_IN OMX_PTR pAppPrivate,
                                  OMX_IN void* eglImage)
{
   return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE Venc::component_role_enum(OMX_IN OMX_HANDLETYPE hComponent,
                                        OMX_OUT OMX_U8 *cRole,
                                        OMX_IN OMX_U32 nIndex)
{

   if (cRole == NULL)
   {
      return OMX_ErrorBadParameter;
   }

  if(!strcmp(m_pComponentName,"OMX.qcom.video.encoder.mpeg4"))
   {
        if((0 == nIndex) && cRole)
        {
         memcpy(cRole, pRoleMPEG4, strlen(pRoleMPEG4) + 1);
            OMX_VENC_MSG_LOW("component_role_enum", 0, 0, 0);
        }
        else
        {
            return OMX_ErrorNoMore; 
        }
    }
   else if(!strcmp(m_pComponentName,"OMX.qcom.video.encoder.h263"))
   {
        if((0 == nIndex) && cRole)
        {
         memcpy(cRole, pRoleH263, strlen(pRoleH263) + 1);
   }
   else
   {
            OMX_VENC_MSG_LOW("No more roles", 0, 0, 0);
      return OMX_ErrorNoMore;
   }
}
   else if(!strcmp(m_pComponentName,"OMX.qcom.video.encoder.avc"))
   {
      if((0 == nIndex) && cRole)
      {
         memcpy(cRole, pRoleAVC, strlen(pRoleAVC) + 1);
      }
      else
      {
         OMX_VENC_MSG_LOW("No more roles", 0, 0, 0);
         return OMX_ErrorNoMore;
      }
   }
   else
   {
        OMX_VENC_MSG_ERROR("Querying Role on Unknown Component", 0, 0, 0);
        return OMX_ErrorInvalidComponentName;
   }
   return OMX_ErrorNone;
}

int Venc::component_thread(void* pClassObj)
{
   OMX_BOOL bRunning = OMX_TRUE;
   Venc* pVenc = reinterpret_cast<Venc*>(pClassObj);

   OMX_VENC_MSG_LOW("component thread has started", 0, 0, 0);


   if (pVenc == NULL)
   {
      OMX_VENC_MSG_ERROR("thread param is null. exiting.", 0, 0, 0);
      return 1;
   }

   while (bRunning)
   {
      VencMsgQ::MsgType msg;
      if (pVenc->m_pMsgQ->PopMsg(&msg) != OMX_ErrorNone)
      {
         OMX_VENC_MSG_ERROR("failed to pop msg", 0, 0, 0);
      }

      switch (msg.id)
      {
      case VencMsgQ::MSG_ID_EXIT:
         OMX_VENC_MSG_LOW("got MSG_ID_EXIT", 0, 0, 0);
         bRunning = OMX_FALSE;
         break;
      case VencMsgQ::MSG_ID_MARK_BUFFER:
         OMX_VENC_MSG_LOW("got MSG_ID_MARK_BUFFER for port %d", (int) msg.data.sMarkBuffer.nPortIndex, 0, 0);
         pVenc->process_mark_buffer(msg.data.sMarkBuffer.nPortIndex,
                                    &msg.data.sMarkBuffer.sMarkData);
         if (venc_semaphore_post(pVenc->m_cmd_sem) != 0)
         {
            OMX_VENC_MSG_ERROR("error posting semaphore", 0, 0, 0);
         }
         break;
      case VencMsgQ::MSG_ID_PORT_ENABLE:
         OMX_VENC_MSG_LOW("got MSG_ID_PORT_ENABLE for port %d", (int) msg.data.nPortIndex, 0, 0);
         pVenc->process_port_enable(msg.data.nPortIndex);
         if (venc_semaphore_post(pVenc->m_cmd_sem) != 0)
         {
            OMX_VENC_MSG_ERROR("error posting semaphore", 0, 0, 0);
         }
         break;
      case VencMsgQ::MSG_ID_PORT_DISABLE:
         OMX_VENC_MSG_LOW("got MSG_ID_PORT_DISABLE for port %d", (int) msg.data.nPortIndex, 0, 0);
         pVenc->process_port_disable(msg.data.nPortIndex);
         if (venc_semaphore_post(pVenc->m_cmd_sem) != 0)
         {
            OMX_VENC_MSG_ERROR("error posting semaphore", 0, 0, 0);
         }
         break;
      case VencMsgQ::MSG_ID_FLUSH:
         OMX_VENC_MSG_LOW("got MSG_ID_FLUSH for port %d", (int) msg.data.nPortIndex, 0, 0);
         pVenc->process_flush(msg.data.nPortIndex);
         if (venc_semaphore_post(pVenc->m_cmd_sem) != 0)
         {
            OMX_VENC_MSG_ERROR("error posting semaphore", 0, 0, 0);
         }
         break;
      case VencMsgQ::MSG_ID_STATE_CHANGE:
         OMX_VENC_MSG_LOW("got MSG_ID_STATE_CHANGE %d", (int) msg.data.eState, 0, 0);
         pVenc->process_state_change(msg.data.eState);
         break;
      case VencMsgQ::MSG_ID_DL_STATUS:
         OMX_VENC_MSG_LOW("got MSG_ID_DL_STATUS", 0, 0, 0);
         pVenc->process_DL_status((OMX_U32) msg.data.sDLStatus.nDLHandle,
                                  (OMX_U32) msg.data.sDLStatus.eCommand,
                                  (OMX_U32) msg.data.sDLStatus.eStatus);
         break;
      case VencMsgQ::MSG_ID_DL_FRAME_DONE:
         OMX_VENC_MSG_LOW("got MSG_ID_DL_FRAME_DONE", 0, 0, 0);
         pVenc->process_DL_frame_done((OMX_U32) msg.data.sDLFrameDone.eStatus,
                                      (void*) &msg.data.sDLFrameDone.sFrameInfo);
         break;
      case VencMsgQ::MSG_ID_YUV_BUFF_DONE:
          OMX_VENC_MSG_LOW("got MSG_ID_YUV_BUFF_DONE", 0, 0, 0);
          pVenc->process_DL_empty_done((void*) &msg.data.sDLYUVDone.sBuffInfo);
          break;
      case VencMsgQ::MSG_ID_EMPTY_BUFF:
          OMX_VENC_MSG_LOW("got MSG_ID_INPUT_BUFF", 0, 0, 0);
          pVenc->empty_this_buffer_proxy((OMX_HANDLETYPE )msg.data.sEncodeBuffer.hComponent,(OMX_BUFFERHEADERTYPE *)msg.data.sEncodeBuffer.pBuffer);
          break;
      case VencMsgQ::MSG_ID_FILL_BUFF:
          OMX_VENC_MSG_LOW("got MSG_ID_FILL_BUFF", 0, 0, 0);
          pVenc->fill_this_buffer_proxy((OMX_BUFFERHEADERTYPE *)msg.data.sEncodeBuffer.pBuffer);
          break;
      default:
         OMX_VENC_MSG_ERROR("invalid msg id %d", (int) msg.id, 0, 0);
         break;
      }
   }

   OMX_VENC_MSG_LOW("component thread is exiting", 0, 0, 0);

   return 0;
}

void Venc::process_state_change(OMX_STATETYPE eState)
{
   if (eState == m_eState)
   {
      OMX_VENC_MSG_ERROR("attempted to change to the same state", 0, 0, 0);
      m_sCallbacks.EventHandler(m_hSelf, 
                                 m_pAppData, 
                                 OMX_EventError, 
                                 (OMX_U32) OMX_ErrorSameState, 
                                 (OMX_U32) 0 , NULL);
      if (venc_semaphore_post(m_cmd_sem) != 0)
      {
         OMX_VENC_MSG_ERROR("error posting semaphore", 0, 0, 0);
      }
      return;
   }

   ///////////////////////////////////////////////////////////////
   // DEVICE LAYER STATE MACHINE
   // component_init:                              venc_load
   // OMX_StateLoaded    --> OMX_StateIdle:        venc_init
   // OMX_StateIdle      --> OMX_StateExecuting:   venc_start
   // OMX_StateExecuting --> OMX_StateIdle:        venc_stop
   // OMX_StateIdle      --> OMX_StateLoaded:      venc_exit
   // component_deinit:                            venc_unload
   ///////////////////////////////////////////////////////////////

   // We will issue OMX_EventCmdComplete when we get device layer
   // status callback for the corresponding state.

   switch (eState)
   {
   case OMX_StateInvalid:
   {
      OMX_VENC_MSG_HIGH("Attempt to go to OMX_StateInvalid", 0, 0, 0);
      if(m_bDeviceStart)
      {
         BITMASK_SET(m_flags, OMX_COMPONENT_INVALID_PENDING);
         if (venc_stop(m_nDLHandle) != VENC_STATUS_SUCCESS)
         {
            OMX_VENC_MSG_ERROR("failed to stop device layer", 0, 0, 0);
         }
      }
      else if (m_bDeviceInit_done)
      {
         m_bDeviceExit_done  = OMX_FALSE;
         BITMASK_SET(m_flags, OMX_COMPONENT_INVALID_PENDING);
         if (venc_exit(m_nDLHandle) != VENC_STATUS_SUCCESS)
         {
            OMX_VENC_MSG_ERROR("failed to exit device layer", 0, 0, 0);
         }
      }
      else
      {
         m_eState = eState;
         m_sCallbacks.EventHandler(m_hSelf, 
                                    m_pAppData,
                                    OMX_EventError, 
                                    (OMX_U32) OMX_ErrorInvalidState,
                                    (OMX_U32) 0 , NULL);
         m_sCallbacks.EventHandler(m_hSelf,
                                    m_pAppData, 
                                    OMX_EventCmdComplete, 
                                    (OMX_U32) OMX_CommandStateSet, 
                                    (OMX_U32) m_eState, NULL);
      }
      break;
   }
   case OMX_StateLoaded:
   {
      OMX_VENC_MSG_HIGH("Attempt to go to OMX_StateLoaded", 0, 0, 0);
      if (m_eState == OMX_StateIdle)
      {
         //if both the ports are disabled we can transit to Loaded state
         if(!m_sInPortDef.bEnabled && !m_sOutPortDef.bEnabled)
         {
            OMX_VENC_MSG_LOW("posting event for state change to Loaded", 0, 0, 0);
            m_eState = eState;
            m_sCallbacks.EventHandler(m_hSelf,
                                       m_pAppData,
                                       OMX_EventCmdComplete,
                                       (OMX_U32) OMX_CommandStateSet,
                                       (OMX_U32) m_eState, NULL);
         }
         else 
         {
            OMX_VENC_MSG_LOW("Setting the loading pending flag", 0, 0, 0);  	
            BITMASK_SET(m_flags, OMX_COMPONENT_LOADING_PENDING);
            m_bDeviceExit_done = OMX_FALSE;
            if (venc_exit(m_nDLHandle) != VENC_STATUS_SUCCESS)
            {
               OMX_VENC_MSG_ERROR("failed to exit device layer", 0, 0, 0);
            }
         }
      }
      else if(m_eState == OMX_StateWaitForResources)
      {
         m_eState = eState;
         m_sCallbacks.EventHandler(m_hSelf,
                                    m_pAppData,
                                    OMX_EventCmdComplete,
                                    (OMX_U32) OMX_CommandStateSet,
                                    (OMX_U32) m_eState, NULL);
      }
      else
      {
         OMX_VENC_MSG_ERROR("invalid state transition to OMX_StateLoaded", 0, 0, 0);
         m_sCallbacks.EventHandler(m_hSelf,
                                    m_pAppData, 
                                    OMX_EventError, 
                                    (OMX_U32) OMX_ErrorIncorrectStateTransition, 
                                    (OMX_U32) 0 , NULL);
      }
      break;
   }
   case OMX_StateIdle:
   {
      OMX_VENC_MSG_HIGH("Attempt to go to OMX_StateIdle", 0, 0, 0);
      if (m_eState == OMX_StateLoaded)
      {
         //if both the ports are disabled we can transit to Idle state
         if(!m_sInPortDef.bEnabled && !m_sOutPortDef.bEnabled)
         {
            OMX_VENC_MSG_LOW("posting event for state change to Idle", 0, 0, 0);
            m_eState = eState;
            m_sCallbacks.EventHandler(m_hSelf,
                                       m_pAppData,
                                       OMX_EventCmdComplete,
                                       (OMX_U32) OMX_CommandStateSet,
                                       (OMX_U32) m_eState, NULL);
            
          }
          else 
          {
             //Configure and init the video encoder, 
             //once done and buffers are allocated we can move to idle state
             BITMASK_SET(m_flags, OMX_COMPONENT_IDLE_PENDING);
             translate_config(&m_sConfigParam);
             m_bDeviceInit_done = OMX_FALSE;
             if (venc_init(m_nDLHandle, &m_sConfigParam, DL_frame_callback, this) != OMX_ErrorNone)
		     {
			    OMX_VENC_MSG_ERROR("venc_init failed during transition from loaded to idle ", 0, 0, 0);
		     }
          }
      }
      else if (m_eState == OMX_StateExecuting || m_eState == OMX_StatePause)
      {
            if(!m_sInPortDef.bEnabled && !m_sOutPortDef.bEnabled)
            {
                m_eState = OMX_StateIdle;                
                m_sCallbacks.EventHandler(m_hSelf,
                                           m_pAppData, 
                                           OMX_EventCmdComplete,
                                           (OMX_U32) OMX_CommandStateSet,
                                           (OMX_U32) m_eState, NULL);
            }
            else
            {
               OMX_VENC_MSG_LOW("Req to Move to Idle: call venc_stop", 0, 0, 0);
               BITMASK_SET(m_flags, OMX_COMPONENT_IDLE_PENDING);
               if (venc_stop(m_nDLHandle) != VENC_STATUS_SUCCESS)
               {
                  OMX_VENC_MSG_ERROR("failed to stop device layer", 0, 0, 0);
               }
            }
      }
      else if(m_eState == OMX_StateWaitForResources)
      {
         m_eState = eState;
         m_sCallbacks.EventHandler(m_hSelf,
                                    m_pAppData,
                                    OMX_EventCmdComplete,
                                    (OMX_U32) OMX_CommandStateSet,
                                    (OMX_U32) m_eState, NULL);
      }
      else
      {
         OMX_VENC_MSG_ERROR("invalid state transition to OMX_StateIdle", 0, 0, 0);
         m_sCallbacks.EventHandler(m_hSelf,
                                    m_pAppData, 
                                    OMX_EventError, 
                                    (OMX_U32) OMX_ErrorIncorrectStateTransition, 
                                    (OMX_U32) 0 , NULL);
      }
      break;
   }
   case OMX_StateExecuting:
	   {
		int ret = 0;
		   OMX_VENC_MSG_HIGH("Attempt to go to OMX_StateExecuting", 0, 0, 0);
		   if (m_eState == OMX_StateIdle)
		   {
			   //if both the ports are disabled we can transit to Executing state
			   if(!m_sInPortDef.bEnabled && !m_sOutPortDef.bEnabled)
			   {
				   m_eState = OMX_StateExecuting;                
				   m_sCallbacks.EventHandler(m_hSelf,
					   m_pAppData, 
					   OMX_EventCmdComplete,
					   (OMX_U32) OMX_CommandStateSet,
					   (OMX_U32) m_eState, NULL);
			   }
			   else if(m_sInPortDef.bEnabled && m_sOutPortDef.bEnabled)
			   {
#ifdef VENC_DUMP_INPUT
				   if(m_finput == NULL)
				   {
				   ret = venc_file_open(&m_finput,"/data/dump_input.yuv", 0);
				   if (ret != 0)
				   {
					   OMX_VENC_MSG_ERROR("failed open /sdcard/input.yuv file", 0, 0, 0);
				   }
				   else
				   {
					   OMX_VENC_MSG_HIGH("file open succesfully", 0, 0, 0);
				   }
				   }
#endif
#ifdef VENC_DUMP_OUTPUT
				   if(m_foutput == NULL)
				   {
				   ret = venc_file_open(&m_foutput,"/data/dump_output.264", 0);
				   if (ret != 0)
				   {
					   OMX_VENC_MSG_ERROR("failed open /data/output.264 file", 0, 0, 0);
				   }else
				   {
					   OMX_VENC_MSG_HIGH("file open succesfully", 0, 0, 0);
				   }
				   }
#endif

				   if (venc_start(m_nDLHandle) != VENC_STATUS_SUCCESS)
				   {
					   OMX_VENC_MSG_ERROR("Failed to start device layer", 0, 0, 0);
					   m_sCallbacks.EventHandler(m_hSelf,
						   m_pAppData, 
						   OMX_EventError, 
						   (OMX_U32) OMX_ErrorUndefined, 
						   (OMX_U32) 0 , NULL);
				   }
			   }
			   else
			   {
				   OMX_VENC_MSG_ERROR("Failed to move OMX_StateExecuting %d,%d", m_sInPortDef.bEnabled, m_sOutPortDef.bEnabled, 0);
			   }
		   }
		   else if(m_eState == OMX_StatePause)
		   {
			   if (venc_start(m_nDLHandle) != VENC_STATUS_SUCCESS)
			   {
				   OMX_VENC_MSG_ERROR("Failed to resume device layer", 0, 0, 0);
				   m_sCallbacks.EventHandler(m_hSelf,
					   m_pAppData, 
					   OMX_EventError, 
					   (OMX_U32) OMX_ErrorUndefined, 
					   (OMX_U32) 0 , NULL);
			   }
		   }
		   else
		   {
			   OMX_VENC_MSG_ERROR("invalid state transition to OMX_StateExecuting", 0, 0, 0);
			   m_sCallbacks.EventHandler(m_hSelf,
				   m_pAppData, 
				   OMX_EventError, 
				   (OMX_U32) OMX_ErrorIncorrectStateTransition, 
				   (OMX_U32) 0 , NULL);
		   }
		   break;
	   }
   case OMX_StatePause:
   {
      OMX_VENC_MSG_HIGH("Attempt to go to OMX_StatePause", 0, 0, 0);
      if(m_eState == OMX_StateExecuting)
      {
          if (venc_pause(m_nDLHandle) != VENC_STATUS_SUCCESS)
          {
            OMX_VENC_MSG_ERROR("Failed to pause device layer", 0, 0, 0);
            m_sCallbacks.EventHandler(m_hSelf,
                                       m_pAppData, 
                                       OMX_EventError, 
                                       (OMX_U32) OMX_ErrorUndefined, 
                                       (OMX_U32) 0 , NULL);
          }
      }
      else if (m_eState == OMX_StateIdle)
      {
         if (venc_start(m_nDLHandle) != VENC_STATUS_SUCCESS)
         {
            OMX_VENC_MSG_ERROR("Failed to resume device layer", 0, 0, 0);
            m_sCallbacks.EventHandler(m_hSelf,
               m_pAppData, 
               OMX_EventError, 
               (OMX_U32) OMX_ErrorUndefined, 
               (OMX_U32) 0 , NULL);
         }
         BITMASK_SET(m_flags, OMX_COMPONENT_PAUSE_PENDING);
      
      }
      else
      {
         OMX_VENC_MSG_LOW("Attempt to go to OMX_StatePause", 0, 0, 0);
         m_sCallbacks.EventHandler(m_hSelf, 
                                    m_pAppData, 
                                    OMX_EventError, 
                                    (OMX_U32) OMX_ErrorIncorrectStateTransition,
                                    (OMX_U32) 0, NULL);
      }
      break;
   }
   case OMX_StateWaitForResources:
   {
      OMX_VENC_MSG_HIGH("Attempt to go to OMX_StateWaitForResources", 0, 0, 0);
      OMX_VENC_MSG_ERROR("Transitioning to OMX_StateWaitForResources", 0, 0, 0);
      if(m_eState == OMX_StateLoaded)
      {
         m_eState = eState;
         m_sCallbacks.EventHandler(m_hSelf, 
                                    m_pAppData, 
                                    OMX_EventCmdComplete,
                                    (OMX_U32) OMX_CommandStateSet,
                                    (OMX_U32) m_eState, NULL);
      }
      else
      {
         OMX_VENC_MSG_ERROR("Incorrect State Transition from %d to Wait For Resources",(int) m_eState, 0, 0);
         m_sCallbacks.EventHandler(m_hSelf,
                                    m_pAppData,
                                    OMX_EventError,
                                    (OMX_U32) OMX_ErrorIncorrectStateTransition,
                                    (OMX_U32) 0, NULL);
      }
      break;
   }
   default:
   {
      OMX_VENC_MSG_ERROR("invalid state %d", (int) eState, 0, 0);
      m_sCallbacks.EventHandler(m_hSelf, 
                                 m_pAppData, 
                                 OMX_EventError, 
                                 (OMX_U32) OMX_ErrorBadParameter,
                                 (OMX_U32) 0 , NULL);
   }
   }
   if (venc_semaphore_post(m_cmd_sem) != 0)
   {
      OMX_VENC_MSG_ERROR("error posting semaphore", 0, 0, 0);
   }
}
void  Venc::translate_config(venc_config_param_type *pConfigParam) 
{
   // update all init time params (run time params updated at run time)
   pConfigParam->skip_syntax_hdr = TRUE;

   if (m_sOutPortDef.format.video.eCompressionFormat == OMX_VIDEO_CodingMPEG4)
   {
      pConfigParam->coding_type = VENC_CODING_MPEG4;

      switch ((OMX_VIDEO_MPEG4PROFILETYPE) m_sParamProfileLevel.eProfile)
      {
         case OMX_VIDEO_MPEG4ProfileSimple:
            pConfigParam->coding_profile_level.mp4_profile_level.mp4_profile = 
               VENC_MPEG4_PROFILE_SIMPLE;
         break;
         case OMX_VIDEO_MPEG4ProfileSimpleScalable:
             pConfigParam->coding_profile_level.mp4_profile_level.mp4_profile = 
               VENC_MPEG4_PROFILE_SIMPLESCALABLE;
         break;
         case OMX_VIDEO_MPEG4ProfileCore:
            pConfigParam->coding_profile_level.mp4_profile_level.mp4_profile = 
               VENC_MPEG4_PROFILE_CORE;
         break;
         case OMX_VIDEO_MPEG4ProfileMain:
            pConfigParam->coding_profile_level.mp4_profile_level.mp4_profile = 
               VENC_MPEG4_PROFILE_MAIN;
         break;
         case OMX_VIDEO_MPEG4ProfileNbit:
            pConfigParam->coding_profile_level.mp4_profile_level.mp4_profile = 
               VENC_MPEG4_PROFILE_NBIT;
         break;
         case OMX_VIDEO_MPEG4ProfileScalableTexture:
            pConfigParam->coding_profile_level.mp4_profile_level.mp4_profile = 
               VENC_MPEG4_PROFILE_SCALABLETEXTURE;
         break;
         case OMX_VIDEO_MPEG4ProfileSimpleFace:
            pConfigParam->coding_profile_level.mp4_profile_level.mp4_profile = 
               VENC_MPEG4_PROFILE_SIMPLEFACE;
         break;
         case OMX_VIDEO_MPEG4ProfileSimpleFBA:
            pConfigParam->coding_profile_level.mp4_profile_level.mp4_profile = 
               VENC_MPEG4_PROFILE_SIMPLEFBA;
         break;
         case  OMX_VIDEO_MPEG4ProfileBasicAnimated:
             pConfigParam->coding_profile_level.mp4_profile_level.mp4_profile = 
                VENC_MPEG4_PROFILE_BASICANIMATED;
         break;
         case OMX_VIDEO_MPEG4ProfileHybrid:
            pConfigParam->coding_profile_level.mp4_profile_level.mp4_profile = 
               VENC_MPEG4_PROFILE_HYBRID;
         break;
         case OMX_VIDEO_MPEG4ProfileAdvancedRealTime:
            pConfigParam->coding_profile_level.mp4_profile_level.mp4_profile = 
               VENC_MPEG4_PROFILE_ADVANCEDREALTIME;
         break;
         case OMX_VIDEO_MPEG4ProfileCoreScalable:
            pConfigParam->coding_profile_level.mp4_profile_level.mp4_profile = 
              VENC_MPEG4_PROFILE_CORESCALABLE;
         break;
         case OMX_VIDEO_MPEG4ProfileAdvancedCoding:
            pConfigParam->coding_profile_level.mp4_profile_level.mp4_profile = 
              VENC_MPEG4_PROFILE_ADVANCEDCODING;
         break;
         case OMX_VIDEO_MPEG4ProfileAdvancedCore:
            pConfigParam->coding_profile_level.mp4_profile_level.mp4_profile = 
               VENC_MPEG4_PROFILE_ADVANCEDCORE;
         break;
         case OMX_VIDEO_MPEG4ProfileAdvancedScalable:
            pConfigParam->coding_profile_level.mp4_profile_level.mp4_profile = 
               VENC_MPEG4_PROFILE_ADVANCEDSCALABLE;
         break;
         default:
            OMX_VENC_MSG_ERROR("invalid mp4 profile %d", (int) m_sParamProfileLevel.eProfile, 0, 0);
         break;
      }

      switch ((OMX_VIDEO_MPEG4LEVELTYPE) m_sParamProfileLevel.eLevel)
      {
         case OMX_VIDEO_MPEG4Level0:
            pConfigParam->coding_profile_level.mp4_profile_level.mp4_level = 
               VENC_MPEG4_LEVEL_0;
         break;
         case OMX_VIDEO_MPEG4Level0b:
            pConfigParam->coding_profile_level.mp4_profile_level.mp4_level = 
               VENC_MPEG4_LEVEL_0B;
         break;
         case OMX_VIDEO_MPEG4Level1:
            pConfigParam->coding_profile_level.mp4_profile_level.mp4_level = 
               VENC_MPEG4_LEVEL_1;
         break;
         case OMX_VIDEO_MPEG4Level2:
            pConfigParam->coding_profile_level.mp4_profile_level.mp4_level = 
               VENC_MPEG4_LEVEL_2;
         break;
         case OMX_VIDEO_MPEG4Level3:
            pConfigParam->coding_profile_level.mp4_profile_level.mp4_level = 
               VENC_MPEG4_LEVEL_3;
         break;
         case OMX_VIDEO_MPEG4Level4:
            pConfigParam->coding_profile_level.mp4_profile_level.mp4_level = 
               VENC_MPEG4_LEVEL_4;
         break;
         case OMX_VIDEO_MPEG4Level4a:
            pConfigParam->coding_profile_level.mp4_profile_level.mp4_level = 
               VENC_MPEG4_LEVEL_4A;
         break;
         case OMX_VIDEO_MPEG4Level5:
            pConfigParam->coding_profile_level.mp4_profile_level.mp4_level = 
               VENC_MPEG4_LEVEL_5;
         default:
            OMX_VENC_MSG_ERROR("invalid mp4 level %ld", m_sParamProfileLevel.eLevel, 0, 0);
         break;
      }
      pConfigParam->intra_period = m_sParamMPEG4.nPFrames + 1;
      pConfigParam->time_resolution = (uint16) m_sParamMPEG4.nTimeIncRes;
      pConfigParam->ac_pred_on = m_sParamMPEG4.bACPred;
   }
   else if (m_sOutPortDef.format.video.eCompressionFormat == OMX_VIDEO_CodingH263)
   {
      pConfigParam->coding_type = VENC_CODING_H263;
      switch ((OMX_VIDEO_H263PROFILETYPE) m_sParamProfileLevel.eProfile)
      {
         case OMX_VIDEO_H263ProfileBaseline:
            pConfigParam->coding_profile_level.h263_profile_level.h263_profile = 
               VENC_H263_PROFILE_BASELINE;
         break;
         case OMX_VIDEO_H263ProfileH320Coding:
            pConfigParam->coding_profile_level.h263_profile_level.h263_profile = 
               VENC_H263_PROFILE_H320_CODING;
         break;
         case OMX_VIDEO_H263ProfileBackwardCompatible:
            pConfigParam->coding_profile_level.h263_profile_level.h263_profile = 
               VENC_H263_PROFILE_BACKWARD_COMPATIBLE;
         break;
         case OMX_VIDEO_H263ProfileISWV2:
            pConfigParam->coding_profile_level.h263_profile_level.h263_profile = 
               VENC_H263_PROFILE_ISWV2;
         break;
         case OMX_VIDEO_H263ProfileISWV3:
            pConfigParam->coding_profile_level.h263_profile_level.h263_profile = 
               VENC_H263_PROFILE_ISWV3;
         break;
         case OMX_VIDEO_H263ProfileHighCompression:
            pConfigParam->coding_profile_level.h263_profile_level.h263_profile = 
               VENC_H263_PROFILE_HIGH_COMPRESSION;
         break;
         case OMX_VIDEO_H263ProfileInternet:
            pConfigParam->coding_profile_level.h263_profile_level.h263_profile = 
               VENC_H263_PROFILE_INTERNET;
         break;
         case OMX_VIDEO_H263ProfileInterlace:
            pConfigParam->coding_profile_level.h263_profile_level.h263_profile = 
               VENC_H263_PROFILE_INTERLACE;
         break;
         case OMX_VIDEO_H263ProfileHighLatency:
            pConfigParam->coding_profile_level.h263_profile_level.h263_profile = 
               VENC_H263_PROFILE_HIGH_LATENCY;
         break;
         default:
            OMX_VENC_MSG_ERROR("invalid h263 profile %d", (int) m_sParamProfileLevel.eProfile, 0, 0);
         break;
      }

      switch ((OMX_VIDEO_H263LEVELTYPE) m_sParamProfileLevel.eLevel)
      {
         case OMX_VIDEO_H263Level10:
            pConfigParam->coding_profile_level.h263_profile_level.h263_level = 
               VENC_H263_LEVEL_10;
         break;
         case OMX_VIDEO_H263Level20:
            pConfigParam->coding_profile_level.h263_profile_level.h263_level = 
               VENC_H263_LEVEL_20;
         break;
         case OMX_VIDEO_H263Level30:
            pConfigParam->coding_profile_level.h263_profile_level.h263_level = 
               VENC_H263_LEVEL_30;
         break;
         case OMX_VIDEO_H263Level40:
            pConfigParam->coding_profile_level.h263_profile_level.h263_level = 
               VENC_H263_LEVEL_40;
         break;
         case OMX_VIDEO_H263Level45:
            pConfigParam->coding_profile_level.h263_profile_level.h263_level = 
              VENC_H263_LEVEL_45;
         break;
         case OMX_VIDEO_H263Level50:
            pConfigParam->coding_profile_level.h263_profile_level.h263_level = 
               VENC_H263_LEVEL_50;
         break;
         case OMX_VIDEO_H263Level60:
            pConfigParam->coding_profile_level.h263_profile_level.h263_level =
               VENC_H263_LEVEL_60;
         break;
         case OMX_VIDEO_H263Level70:
            pConfigParam->coding_profile_level.h263_profile_level.h263_level = 
               VENC_H263_LEVEL_70;
         break;
         default:
            OMX_VENC_MSG_ERROR("invalid h263 level %d", m_sParamProfileLevel.eLevel, 0, 0);
         break;
      }
      pConfigParam->intra_period = m_sParamH263.nPFrames + 1;
      pConfigParam->ac_pred_on = FALSE;
   }
   else
   {
      pConfigParam->coding_type = VENC_CODING_H264;
      switch ((OMX_VIDEO_AVCPROFILETYPE) m_sParamProfileLevel.eProfile)
      {
         case OMX_VIDEO_AVCProfileBaseline:
            pConfigParam->coding_profile_level.h264_profile_level.h264_profile = VENC_H264_PROFILE_BASELINE;
				break;
         default:
            OMX_VENC_MSG_ERROR("invalid h264 profile %d", m_sParamProfileLevel.eProfile, 0, 0);
         break;
      }

      switch ((OMX_VIDEO_AVCLEVELTYPE) m_sParamProfileLevel.eLevel)
      {
         case OMX_VIDEO_AVCLevel1:
            pConfigParam->coding_profile_level.h264_profile_level.h264_level = VENC_H264_LEVEL_1;
				  break;
         case OMX_VIDEO_AVCLevel1b:
            pConfigParam->coding_profile_level.h264_profile_level.h264_level = VENC_H264_LEVEL_1b;
				  break;
         case OMX_VIDEO_AVCLevel11:
            pConfigParam->coding_profile_level.h264_profile_level.h264_level = VENC_H264_LEVEL_11;
				  break;
         case OMX_VIDEO_AVCLevel12:
            pConfigParam->coding_profile_level.h264_profile_level.h264_level = VENC_H264_LEVEL_12;
              break;
         case OMX_VIDEO_AVCLevel13:
            pConfigParam->coding_profile_level.h264_profile_level.h264_level = VENC_H264_LEVEL_13;
              break;
         case OMX_VIDEO_AVCLevel2:
            pConfigParam->coding_profile_level.h264_profile_level.h264_level = VENC_H264_LEVEL_2;
  	      break;
	  case OMX_VIDEO_AVCLevel21:
            pConfigParam->coding_profile_level.h264_profile_level.h264_level = VENC_H264_LEVEL_121;
              break;
         case OMX_VIDEO_AVCLevel22:
            pConfigParam->coding_profile_level.h264_profile_level.h264_level = VENC_H264_LEVEL_122;
               break;
         default:
            OMX_VENC_MSG_ERROR("invalid h264 level %d", m_sParamProfileLevel.eLevel, 0, 0);
         break;
      }

      pConfigParam->intra_period = m_sParamAVC.nPFrames + 1;         
      pConfigParam->time_resolution = (uint16) ((m_sConfigFramerate.xEncodeFramerate >> 16) * 2); // 2 times the frame rate
      pConfigParam->ac_pred_on = FALSE;
   }

   pConfigParam->init_qp_iframe = (uint32) m_sParamQPs.nQpI;
   pConfigParam->init_qp_pframe = (uint32) m_sParamQPs.nQpP;
   pConfigParam->init_qp_bframe = (uint32) m_sParamQPs.nQpB;

   if (m_sParamIntraRefresh.eRefreshMode == OMX_VIDEO_IntraRefreshCyclic)
   {
      pConfigParam->intra_refresh.nMBCount = (uint32) m_sParamIntraRefresh.nCirMBs;
   }
   else
   {
      pConfigParam->intra_refresh.nMBCount = 0;
   }

   pConfigParam->frame_height = m_sOutPortDef.format.video.nFrameHeight;
   pConfigParam->frame_width = m_sOutPortDef.format.video.nFrameWidth;
   pConfigParam->encoder_quality.target_bitrate = m_sConfigBitrate.nEncodeBitrate;
   pConfigParam->encoder_quality.max_frame_rate = m_sConfigFramerate.xEncodeFramerate >> 16;
   pConfigParam->encoder_quality.scaling_factor = 0;
#ifndef FEATURE_LINUX_A
   pConfigParam->encoder_quality.scaling_factor = m_sTimeStampScale.nTimestampScale >> 16;
   if(pConfigParam->encoder_quality.scaling_factor > 1)
   {
      pConfigParam->encoder_quality.target_bitrate = pConfigParam->encoder_quality.target_bitrate/
		                                              pConfigParam->encoder_quality.scaling_factor;   
      pConfigParam->encoder_quality.max_frame_rate = pConfigParam->encoder_quality.max_frame_rate/
		                                              pConfigParam->encoder_quality.scaling_factor;
      pConfigParam->intra_period = pConfigParam->intra_period/
		                           pConfigParam->encoder_quality.scaling_factor;
   }
#endif
   pConfigParam->encoder_quality.min_qp = quality.min_qp;
   pConfigParam->encoder_quality.max_qp = (uint16) quality.max_qp;
   pConfigParam->iframe_vol_inject = FALSE;
   pConfigParam->region_of_interest.roi_array = NULL;
   pConfigParam->region_of_interest.roi_size = 0;

   pConfigParam->error_resilience.rm_type = VENC_RM_NONE;
   pConfigParam->error_resilience.rm_spacing = 0;
   pConfigParam->error_resilience.hec_interval = 0;

   if (m_sOutPortDef.format.video.eCompressionFormat == OMX_VIDEO_CodingMPEG4)
   {

      // mp4 header extension
      if (m_sErrorCorrection.bEnableHEC == OMX_TRUE)
      {
         pConfigParam->error_resilience.hec_interval = 1;
      }

      // mp4 header extension
      if (m_sParamMPEG4.nHeaderExtension > 0)
      {
         pConfigParam->error_resilience.hec_interval = (uint32) m_sParamMPEG4.nHeaderExtension;
      }

      // mp4 resync marker
      if ((m_sErrorCorrection.nResynchMarkerSpacing > 0) && 
		  ( m_sParamBitrate.eControlRate == OMX_Video_ControlRateConstantSkipFrames) )
      {
         pConfigParam->error_resilience.rm_type = VENC_RM_BITS;
         pConfigParam->error_resilience.rm_spacing = (uint32) m_sErrorCorrection.nResynchMarkerSpacing;
      }
   }
   else if (m_sOutPortDef.format.video.eCompressionFormat == OMX_VIDEO_CodingH263)
   {

      // annex k
      if (m_sParamMPEG4.nSliceHeaderSpacing > 0)
      {
         pConfigParam->error_resilience.rm_type = VENC_RM_MB;
         pConfigParam->error_resilience.rm_spacing = (uint32) m_sParamMPEG4.nSliceHeaderSpacing;
      }

      // gob header

      if ((m_sParamH263.nGOBHeaderInterval > 0)&& 
		  (m_sParamBitrate.eControlRate == OMX_Video_ControlRateConstantSkipFrames))
      {
         pConfigParam->error_resilience.rm_type = VENC_RM_GOB;
         pConfigParam->error_resilience.rm_spacing = (uint32) m_sParamH263.nGOBHeaderInterval;
      }

   }
   if (m_sConfigEncoderModeType.nMode != QOMX_VIDEO_EncoderModeMMS)
   {
   switch (m_sParamBitrate.eControlRate)
   {
      case OMX_Video_ControlRateDisable:
         pConfigParam->rc_selection = VENC_RC_NONE;
      break;
      case OMX_Video_ControlRateVariable:
         pConfigParam->rc_selection = VENC_RC_VBR_CFR;
      break;
      case OMX_Video_ControlRateVariableSkipFrames:
         pConfigParam->rc_selection = VENC_RC_VBR_VFR;
      break;
      case OMX_Video_ControlRateConstantSkipFrames:
         pConfigParam->rc_selection = VENC_RC_CBR_VFR;
      break; 
      default:
         OMX_VENC_MSG_ERROR("invalid rate control selection", 0, 0, 0);
         pConfigParam->rc_selection = VENC_RC_NONE;
   }
   }
   else
   {
      pConfigParam->rc_selection = VENC_RC_VBR_VFR_TIGHT;
   }
   switch(m_sConfigFrameRotation.nRotation)
   {
     case 0:
        pConfigParam->rotation = VENC_ROT_0;
        break;
     case 90:
         pConfigParam->rotation = VENC_ROT_90;
         break;
     case 180:
         pConfigParam->rotation = VENC_ROT_180;
         break;
     case 270:
         pConfigParam->rotation = VENC_ROT_270;
         break;
          default:
         pConfigParam->rotation = VENC_ROT_0;   
         OMX_VENC_MSG_ERROR("Rotation is: %ld in %d", pConfigParam->rotation, m_sConfigFrameRotation.nRotation, 0);
              break;         
   }
	if(pConfigParam->rc_selection == VENC_RC_CBR_VFR && (m_sOutPortDef.format.video.eCompressionFormat == OMX_VIDEO_CodingAVC ))
	{
        pConfigParam->init_qp_iframe = 40;
        pConfigParam->init_qp_pframe = 35;
   }
}

void Venc::process_flush(OMX_U32 nPortIndex)
{
   OMX_VENC_MSG_HIGH("flushing...", 0, 0, 0);

   if ((PortIndexType) nPortIndex == PORT_INDEX_BOTH)
   {
      venc_flush(m_nDLHandle);
      m_bFlushingIndex_in = OMX_TRUE;
      m_bFlushingIndex_out = OMX_TRUE;
   }
   else
   {
      OMX_VENC_MSG_ERROR("We only support flushing on input+output ports simultaneously", 0, 0, 0);
      m_sCallbacks.EventHandler(m_hSelf,
                                 m_pAppData,
                                 OMX_EventError,
                                 (OMX_U32) OMX_ErrorBadPortIndex,
                                 (OMX_U32) 0,
                                 NULL);
   }
}

void Venc::process_port_enable(OMX_U32 nPortIndex)
{
   // trying to enable I/P or both the I/P and O/P ports
   if((nPortIndex == (OMX_U32)PORT_INDEX_IN) || (nPortIndex == (OMX_U32)OMX_ALL))
   {
      m_sInPortDef.bEnabled = OMX_TRUE;
      //enable the port if in loded state, ensure we have not recived request for Idle state
      if(OMX_StateLoaded == m_eState && !BITMASK_PRESENT(m_flags,OMX_COMPONENT_IDLE_PENDING))
      {
         m_sCallbacks.EventHandler(m_hSelf,
                                    m_pAppData,
                                    OMX_EventCmdComplete,
                                    (OMX_U32) OMX_CommandPortEnable,
                                    (OMX_U32) PORT_INDEX_IN,
                                    NULL);
      }
      else
      {
         BITMASK_SET(m_flags, OMX_COMPONENT_INPUT_ENABLE_PENDING);
      }
   }
   if((nPortIndex == (OMX_U32)PORT_INDEX_OUT) || (nPortIndex == (OMX_U32)OMX_ALL))
   {
      m_sOutPortDef.bEnabled = OMX_TRUE;
      if(OMX_StateLoaded == m_eState && !BITMASK_PRESENT(m_flags,OMX_COMPONENT_IDLE_PENDING))
      {
         m_sCallbacks.EventHandler(m_hSelf,
                                    m_pAppData,
                                    OMX_EventCmdComplete,
                                    (OMX_U32) OMX_CommandPortEnable,
                                    (OMX_U32) PORT_INDEX_OUT,
                                    NULL);
      }
      //wait until all buffers are allocated
      else
      {
         BITMASK_SET(m_flags, OMX_COMPONENT_OUTPUT_ENABLE_PENDING);
      }
   }
 
   //if both ports are enabled, venc_init and venc_start has to be called 
   if(m_sOutPortDef.bEnabled && m_sInPortDef.bEnabled)
   {
      if(OMX_StateIdle == m_eState || OMX_StateExecuting == m_eState)
      {
          OMX_VENC_MSG_HIGH("Calling VENC_INIT while Enabling the ports", 0, 0, 0);
		  translate_config(&m_sConfigParam);
		  m_bDeviceInit_done = OMX_FALSE;
		  if (venc_init(m_nDLHandle, &m_sConfigParam, DL_frame_callback, this) != OMX_ErrorNone)
		  {
			  OMX_VENC_MSG_ERROR("venc_init failed during transition from loaded to idle ", 0, 0, 0);
		  }
      }
   }
}

void Venc::process_port_disable(OMX_U32 nPortIndex)
{
   // trying to disable I/P or both the I/P and O/P ports
   if((nPortIndex == (OMX_U32)PORT_INDEX_IN) || (nPortIndex == (OMX_U32)OMX_ALL))
   {
      m_sInPortDef.bEnabled = OMX_FALSE;
      //when in loaded state and all buffers released, we are good to disable the port 
      if(OMX_StateLoaded == m_eState)
      {
         m_sCallbacks.EventHandler(m_hSelf,
                                    m_pAppData,
                                    OMX_EventCmdComplete,
                                    (OMX_U32) OMX_CommandPortDisable,
                                    (OMX_U32) PORT_INDEX_IN,
                                    NULL);
      }
      else
      {
         BITMASK_SET(m_flags, OMX_COMPONENT_INPUT_DISABLE_PENDING);
      }
   }
// trying to disable O/P or both the I/P and O/P ports
   if((nPortIndex == (OMX_U32)PORT_INDEX_OUT) || (nPortIndex == (OMX_U32)OMX_ALL))
   {
      m_sOutPortDef.bEnabled = OMX_FALSE;
      //when in loaded state and all buffers released, we are good to disable the port 
      if(OMX_StateLoaded == m_eState)
      {
         m_sCallbacks.EventHandler(m_hSelf,
                                    m_pAppData,
                                    OMX_EventCmdComplete,
                                    (OMX_U32) OMX_CommandPortDisable,
                                    (OMX_U32) PORT_INDEX_OUT,
                                    NULL);
      }
      else
      {
         BITMASK_SET(m_flags, OMX_COMPONENT_OUTPUT_DISABLE_PENDING);
      }
   }

   if(!m_sOutPortDef.bEnabled || !m_sInPortDef.bEnabled)
   {
      // in Idle state we need to ensure venc_exit is called
      if(OMX_StateIdle == m_eState)
      {
         //wait until exit is complete and all I/P buffers are released         
         if (m_bDeviceInit_done)
         {
            m_bDeviceExit_done = OMX_FALSE;
            if (venc_exit(m_nDLHandle) != VENC_STATUS_SUCCESS)
            {
               OMX_VENC_MSG_ERROR("failed to exit device layer", 0, 0, 0);
            }
         } 
         //if exit is complete and all buffers are released, good to disable the port
         else 
         {
             if(BITMASK_PRESENT(m_flags, OMX_COMPONENT_INPUT_DISABLE_PENDING)&& m_in_alloc_cnt == 0)
             {
                 BITMASK_CLEAR(m_flags, OMX_COMPONENT_INPUT_DISABLE_PENDING);
                 m_sCallbacks.EventHandler(m_hSelf,
                                           m_pAppData,
                                           OMX_EventCmdComplete,
                                           (OMX_U32) OMX_CommandPortDisable,
                                           (OMX_U32) PORT_INDEX_IN,
                                           NULL);
             }
             if(BITMASK_PRESENT(m_flags, OMX_COMPONENT_OUTPUT_DISABLE_PENDING)&& m_out_alloc_cnt == 0)
             {
                 BITMASK_CLEAR(m_flags, OMX_COMPONENT_OUTPUT_DISABLE_PENDING);
                 m_sCallbacks.EventHandler(m_hSelf,
                                           m_pAppData,
                                           OMX_EventCmdComplete,
                                           (OMX_U32) OMX_CommandPortDisable,
                                           (OMX_U32) PORT_INDEX_OUT,
                                           NULL);
             }
         }
      }
      //in executing or pause state we need to ensure both stop and exit have been called
      else if(OMX_StatePause == m_eState || OMX_StateExecuting == m_eState)
      {
         if(m_bDeviceStart == OMX_TRUE)
         {
            if (venc_stop(m_nDLHandle) != VENC_STATUS_SUCCESS)
            {
               OMX_VENC_MSG_ERROR("failed to stop device layer", 0, 0, 0);
            }
         }
         else if (m_bDeviceInit_done == OMX_TRUE)
         {
            m_bDeviceExit_done = OMX_FALSE;
            if (venc_exit(m_nDLHandle) != VENC_STATUS_SUCCESS)
            {
               OMX_VENC_MSG_ERROR("failed to exit device layer", 0, 0, 0);
            }
         } 
         //if exit is complete and all buffers released, disable the port
         else if(m_bDeviceExit_done == OMX_TRUE)
         {
            if(BITMASK_PRESENT(m_flags, OMX_COMPONENT_INPUT_DISABLE_PENDING)&& m_in_alloc_cnt == 0)
            {
               BITMASK_CLEAR(m_flags, OMX_COMPONENT_INPUT_DISABLE_PENDING);
               m_sCallbacks.EventHandler(m_hSelf,
                                         m_pAppData,
                                         OMX_EventCmdComplete,
                                         (OMX_U32) OMX_CommandPortDisable,
                                         (OMX_U32) PORT_INDEX_IN,
                                         NULL);
            }
            if(BITMASK_PRESENT(m_flags, OMX_COMPONENT_OUTPUT_DISABLE_PENDING)&& m_out_alloc_cnt == 0)
            {
               BITMASK_CLEAR(m_flags, OMX_COMPONENT_OUTPUT_DISABLE_PENDING);
               m_sCallbacks.EventHandler(m_hSelf,
                                         m_pAppData,
                                         OMX_EventCmdComplete,
                                         (OMX_U32) OMX_CommandPortDisable,
                                         (OMX_U32) PORT_INDEX_OUT,
                                         NULL);
            }
         }
      }
   }
}

void Venc::process_mark_buffer(OMX_U32 nPortIndex,
                               const OMX_MARKTYPE* pMarkData)
{
   if(pMarkData == NULL)
   {
      OMX_VENC_MSG_ERROR("param is null", 0, 0, 0);
      return ;
   }
   OMX_VENC_MSG_HIGH("Process mark buffer command Obtained", 0, 0, 0);
   m_Mark_command = OMX_TRUE;
   m_Markdata.hMarkTargetComponent = pMarkData->hMarkTargetComponent;
   m_Markdata.pMarkData = pMarkData->pMarkData;
}
/**********************************************************************//**
 * @brief allocate_done
 *
 * checks if all the I/P and O/P buffers have been allocated
 * 
 *************************************************************************/
OMX_BOOL Venc::allocate_done(void)
{
   OMX_BOOL ret = OMX_FALSE;
   if((m_sInPortDef.bPopulated == OMX_TRUE) && (m_sOutPortDef.bPopulated == OMX_TRUE))
   {
      ret = OMX_TRUE;
   }
   return ret;
}
/**********************************************************************//**
 * @brief Process thread message MSG_ID_DL_STATUS
 *
 * This method abstracts the header file, params are to be interpreted
 * as follows:
 *
 * @param nDLHandle: Handle to the device layer, as indicated by the 
 *                   status for VENC_CMD_LOAD.
 * @param nCommand: Completed device layer command (venc_command_type)
 * @param nStatus: Status for the completed command (venc_status_type)
 *************************************************************************/
void Venc::process_DL_status(OMX_U32 nDLHandle,
                             OMX_U32 nCommand,
                             OMX_U32 nStatus)
{

   venc_command_type eCommand = static_cast<venc_command_type>(nCommand);
   venc_status_type eStatus = static_cast<venc_status_type>(nStatus);
   
#define GOTO_STATE(eState)                                              \
   {                                                                    \
      m_eState = eState;                                                \
      m_sCallbacks.EventHandler(m_hSelf,                               \
                                 m_pAppData,                            \
                                 OMX_EventCmdComplete,                  \
                                 (OMX_U32) OMX_CommandStateSet,         \
                                 (OMX_U32) eState,                      \
                                 NULL);                                 \
   }

   if (eStatus != VENC_STATUS_SUCCESS)
   {
      OMX_VENC_MSG_ERROR("device layer command failed (%d)", (int) nCommand, 0, 0);

      if (m_sCallbacks.EventHandler != NULL)
      {
         m_sCallbacks.EventHandler(m_hSelf,
                                   m_pAppData,
                                   OMX_EventError,
                                   (OMX_U32) OMX_ErrorUndefined,
                                   (OMX_U32) 0,
                                   NULL);
      }
   }
   else switch (eCommand)
   {

      ///////////////////////////////////////////////////////////////
      // DL Load Status
      ///////////////////////////////////////////////////////////////
      case VENC_CMD_LOAD:
         OMX_VENC_MSG_HIGH("got DL status for VENC_CMD_LOAD", 0, 0, 0);
         // This will unblock the caller of Venc::component_init
         m_nDLHandle = nDLHandle;
         m_eState = OMX_StateLoaded;
         venc_signal_set(m_loadUnloadSignal);
         break;

      ///////////////////////////////////////////////////////////////
      // DL Init Status
      ///////////////////////////////////////////////////////////////
      case VENC_CMD_INIT:
         OMX_VENC_MSG_HIGH("got DL status for VENC_CMD_INIT", 0, 0, 0);
         m_bDeviceInit_done = OMX_TRUE;
         m_bDeviceExit_done = OMX_FALSE;
         if(allocate_done()&& BITMASK_PRESENT(m_flags, OMX_COMPONENT_IDLE_PENDING))
         {
            BITMASK_CLEAR(m_flags, OMX_COMPONENT_IDLE_PENDING);
            OMX_VENC_MSG_HIGH("State Transition from Loaded to Idle", 0, 0, 0);
            GOTO_STATE(OMX_StateIdle);
         }
         else if(OMX_StateIdle == m_eState)
         {
             if (BITMASK_PRESENT(m_flags, OMX_COMPONENT_INPUT_ENABLE_PENDING) && 
                 m_sInPortDef.bPopulated)
             {       
                BITMASK_CLEAR(m_flags, OMX_COMPONENT_INPUT_ENABLE_PENDING);
                m_sCallbacks.EventHandler(m_hSelf,
                                           m_pAppData,
                                           OMX_EventCmdComplete,
                                           (OMX_U32) OMX_CommandPortEnable,
                                           (OMX_U32) PORT_INDEX_IN,
                                           NULL);
             }
             if (BITMASK_PRESENT(m_flags, OMX_COMPONENT_OUTPUT_ENABLE_PENDING)&& 
                 m_sOutPortDef.bPopulated)
             {          
                BITMASK_CLEAR(m_flags, OMX_COMPONENT_OUTPUT_ENABLE_PENDING);
                m_sCallbacks.EventHandler(m_hSelf,
                                           m_pAppData,
                                           OMX_EventCmdComplete,
                                           (OMX_U32) OMX_CommandPortEnable,
                                           (OMX_U32) PORT_INDEX_OUT,
                                           NULL);
             }
         }
         else if (OMX_StateExecuting == m_eState)
         {
            if (venc_start(m_nDLHandle) != VENC_STATUS_SUCCESS)
            {
               OMX_VENC_MSG_ERROR("failed to start device layer", 0, 0, 0);
            }
         }

         break;

      ///////////////////////////////////////////////////////////////
      // DL Start Status
      ///////////////////////////////////////////////////////////////
      case VENC_CMD_START:
         OMX_VENC_MSG_HIGH("got DL status for VENC_CMD_START", 0, 0, 0);
         /* Check whether we have a pending changequality from init state:*/
         if( m_bPendingChangeQuality == OMX_TRUE )
         {
            OMX_VENC_MSG_MEDIUM( "change_quality() call required...",0,0,0);
            change_quality();
            m_bPendingChangeQuality = OMX_FALSE;
         }
         else
         {
            OMX_VENC_MSG_MEDIUM( "NO change_quality() call required... sending.",0,0,0);
         }
         m_bDeviceStart = OMX_TRUE;
         if(BITMASK_PRESENT(m_flags, OMX_COMPONENT_PAUSE_PENDING))
         {
            if (venc_pause(m_nDLHandle) != VENC_STATUS_SUCCESS)
            {
               OMX_VENC_MSG_ERROR("Failed to pause device layer", 0, 0, 0);
               m_sCallbacks.EventHandler(m_hSelf,
                  m_pAppData, 
                  OMX_EventError, 
                  (OMX_U32) OMX_ErrorUndefined, 
                  (OMX_U32) 0 , NULL);
            }
         }
         else if(OMX_StateExecuting == m_eState)
         {
             if (BITMASK_PRESENT(m_flags, OMX_COMPONENT_INPUT_ENABLE_PENDING) && 
                 m_sInPortDef.bPopulated)
             {       
                BITMASK_CLEAR(m_flags, OMX_COMPONENT_INPUT_ENABLE_PENDING);
                m_sCallbacks.EventHandler(m_hSelf,
                                           m_pAppData,
                                           OMX_EventCmdComplete,
                                           (OMX_U32) OMX_CommandPortEnable,
                                           (OMX_U32) PORT_INDEX_IN,
                                           NULL);
             }
             if (BITMASK_PRESENT(m_flags, OMX_COMPONENT_OUTPUT_ENABLE_PENDING)&& 
                 m_sOutPortDef.bPopulated)
             {          
                BITMASK_CLEAR(m_flags, OMX_COMPONENT_OUTPUT_ENABLE_PENDING);
                m_sCallbacks.EventHandler(m_hSelf,
                                           m_pAppData,
                                           OMX_EventCmdComplete,
                                           (OMX_U32) OMX_CommandPortEnable,
                                           (OMX_U32) PORT_INDEX_OUT,
                                           NULL);
             }
         }
         else
         {
            OMX_VENC_MSG_HIGH("State Transition from Idle to Executing", 0, 0, 0);
            GOTO_STATE(OMX_StateExecuting);
            m_bGetSyntaxHdr = OMX_TRUE;
         }
         break;

      ///////////////////////////////////////////////////////////////
      // DL Pause Status
      ///////////////////////////////////////////////////////////////
      case VENC_CMD_PAUSE:
         OMX_VENC_MSG_HIGH("got DL status for VENC_CMD_PAUSE", 0, 0, 0);
         if(BITMASK_PRESENT(m_flags, OMX_COMPONENT_PAUSE_PENDING))
         {
            BITMASK_CLEAR(m_flags, OMX_COMPONENT_PAUSE_PENDING);
         }
         GOTO_STATE(OMX_StatePause);
         break;

      ///////////////////////////////////////////////////////////////
      // DL Stop Status
      ///////////////////////////////////////////////////////////////
      case VENC_CMD_STOP:
      {
         OMX_VENC_MSG_HIGH("got DL status for VENC_CMD_STOP", 0, 0, 0);
         m_bDeviceStart = OMX_FALSE;

         OMX_U32 nInBuffers = 0;
         OMX_U32 nOutBuffers = 0;
         int i = 0;
         OMX_BUFFERHEADERTYPE* pBuffer;

         //release any I/P buffers in the free queue
         (void) m_pFreeInBM->GetNumBuffers(&nInBuffers);     
         OMX_VENC_MSG_HIGH("num input buffers to free %d", nInBuffers, 0, 0);
         for (i = 0; i < (int) nInBuffers; i++)
         {
            OMX_VENC_MSG_HIGH("releasing input buffer after venc_stop", 0, 0, 0);
            if (m_pFreeInBM->PopFirstBuffer(&pBuffer) == OMX_ErrorNone)
            {
               pBuffer->nFilledLen = 0;
               if (m_sCallbacks.EmptyBufferDone(m_hSelf,m_pAppData,pBuffer)
                   != OMX_ErrorNone)
               {
                  OMX_VENC_MSG_ERROR("EBD failed", 0, 0, 0);
               }
            }
            else
            {
               OMX_VENC_MSG_ERROR("failed to pop buffer", 0, 0, 0);
            }
         }

        //release any O/P buffers in the free queue
        (void) m_pFreeOutBM->GetNumBuffers(&nOutBuffers);
        OMX_VENC_MSG_HIGH("num output buffers to free %d", nOutBuffers, 0, 0);
        for (i = 0; i < (int) nOutBuffers; i++)
        {
           OMX_VENC_MSG_HIGH("releasing output buffer while flushing O/P port", 0, 0, 0);
           if (m_pFreeOutBM->PopFirstBuffer(&pBuffer) == OMX_ErrorNone)
           {
               pBuffer->nFilledLen = 0;
               if (m_sCallbacks.FillBufferDone(m_hSelf,m_pAppData,pBuffer)
                   != OMX_ErrorNone)
               {
                   OMX_VENC_MSG_ERROR("FBD failed", 0, 0, 0);
               }
           }
           else
           {
              OMX_VENC_MSG_ERROR("failed to pop buffer", 0, 0, 0);
           }
        }    
      
        if (m_bPanic)
        {
           m_eState = OMX_StateIdle;
           OMX_VENC_MSG_LOW("m_bPanic is true call venc_exit", 0, 0, 0);
           m_bDeviceExit_done = OMX_FALSE;
           if (venc_exit(m_nDLHandle) != VENC_STATUS_SUCCESS)
           {
              OMX_VENC_MSG_ERROR("failed to exit device layer", 0, 0, 0);
           }
        }
        else if( BITMASK_PRESENT(m_flags,OMX_COMPONENT_INVALID_PENDING))
        {
           m_bDeviceExit_done = OMX_FALSE;
           if (venc_exit(m_nDLHandle) != VENC_STATUS_SUCCESS)
           {
              OMX_VENC_MSG_ERROR("failed to exit device layer", 0, 0, 0);
           }
        }
        else if(!m_sOutPortDef.bEnabled || !m_sInPortDef.bEnabled)
        {
           m_bDeviceExit_done = OMX_FALSE;
           if (venc_exit(m_nDLHandle) != VENC_STATUS_SUCCESS)
           {
              OMX_VENC_MSG_ERROR("failed to exit device layer", 0, 0, 0);
           }
        }
        else if(BITMASK_PRESENT(m_flags, OMX_COMPONENT_IDLE_PENDING))
        {
           //check if have any buffers in the device layer
           if(m_nInputDown > 0 || m_nOutputDown > 0)
           {
              OMX_VENC_MSG_ERROR("Device layer still has buffers find the bug (%d, %d)",
                 m_nInputDown, m_nOutputDown, 0);
           }

           OMX_VENC_MSG_HIGH("State Transition done from Executing to Idle", 0, 0, 0);
           GOTO_STATE(OMX_StateIdle);
        }  
        break;
      }

      ///////////////////////////////////////////////////////////////
      // DL Exit Status
      ///////////////////////////////////////////////////////////////
      case VENC_CMD_EXIT:
      {
         OMX_VENC_MSG_LOW("got DL status for VENC_CMD_EXIT", 0, 0, 0);
         m_bDeviceExit_done = OMX_TRUE;
         m_bDeviceInit_done = OMX_FALSE;
         if (m_bPanic)
         {
            m_eState = OMX_StateLoaded;
            OMX_VENC_MSG_HIGH("m_bPanic is true call free up Port In Out", 0, 0, 0);
            free_port_inout();
            OMX_VENC_MSG_HIGH("go to unloaded sate call venc_unload", 0, 0, 0);
            if (venc_unload(m_nDLHandle) != VENC_STATUS_SUCCESS)
            {
               OMX_VENC_MSG_ERROR("failed to unload device layer", 0, 0, 0);
            }
         }
         else if(BITMASK_PRESENT(m_flags,OMX_COMPONENT_INVALID_PENDING))
         {
            BITMASK_CLEAR(m_flags, OMX_COMPONENT_INVALID_PENDING);
            OMX_VENC_MSG_HIGH("request to transit for Invalid state,completed VENC_EXIT", 0, 0, 0);
            m_eState = OMX_StateInvalid;
            m_sCallbacks.EventHandler(m_hSelf, 
                                       m_pAppData,
                                       OMX_EventError, 
                                       (OMX_U32) OMX_ErrorInvalidState,
                                       (OMX_U32) 0 , NULL);
            m_sCallbacks.EventHandler(m_hSelf,
                                       m_pAppData, 
                                       OMX_EventCmdComplete, 
                                       (OMX_U32) OMX_CommandStateSet, 
                                       (OMX_U32) m_eState, NULL);
          }
          else if (!m_sOutPortDef.bEnabled || !m_sInPortDef.bEnabled)
          {
             if(BITMASK_PRESENT(m_flags, OMX_COMPONENT_INPUT_DISABLE_PENDING) && m_in_alloc_cnt ==0)
             {       
                BITMASK_CLEAR(m_flags, OMX_COMPONENT_INPUT_DISABLE_PENDING);
                m_sCallbacks.EventHandler(m_hSelf,
                                           m_pAppData,
                                           OMX_EventCmdComplete,
                                           (OMX_U32) OMX_CommandPortDisable,
                                           (OMX_U32) PORT_INDEX_IN,
                                           NULL);
             }
             if(BITMASK_PRESENT(m_flags, OMX_COMPONENT_OUTPUT_DISABLE_PENDING)&& m_out_alloc_cnt == 0)
             {          
                BITMASK_CLEAR(m_flags, OMX_COMPONENT_OUTPUT_DISABLE_PENDING);
                m_sCallbacks.EventHandler(m_hSelf,
                                           m_pAppData,
                                           OMX_EventCmdComplete,
                                           (OMX_U32) OMX_CommandPortDisable,
                                           (OMX_U32) PORT_INDEX_OUT,
                                           NULL);
             }
          }
          else if(release_done()&& BITMASK_PRESENT(m_flags, OMX_COMPONENT_LOADING_PENDING)) 
          {
             OMX_VENC_MSG_HIGH("Going from Idle to Loaded state", 0, 0, 0);
             BITMASK_CLEAR((m_flags), OMX_COMPONENT_LOADING_PENDING);
             GOTO_STATE(OMX_StateLoaded);         
          }
         }
         break;

      ///////////////////////////////////////////////////////////////
      // DL Unload Status
      ///////////////////////////////////////////////////////////////
      case VENC_CMD_UNLOAD:
         OMX_VENC_MSG_HIGH("got DL status for VENC_CMD_UNLOAD", 0, 0, 0);
         m_eState = OMX_StateInvalid;
         venc_signal_set(m_loadUnloadSignal);
         break;

      ///////////////////////////////////////////////////////////////
      // DL Encode Status
      ///////////////////////////////////////////////////////////////
      case VENC_CMD_ENCODE_FRAME:
         OMX_VENC_MSG_MEDIUM("got DL status for VENC_CMD_ENCODE_FRAME", 0, 0, 0);
         break;

      ///////////////////////////////////////////////////////////////
      // DL Flush Status
      ///////////////////////////////////////////////////////////////
      case VENC_CMD_FLUSH:
      {
         OMX_VENC_MSG_HIGH("got DL status for VENC_CMD_FLUSH", 0, 0, 0);
         // This lock is added to make sure that the EBD's are not delivered in Idle state
         OMX_U32 nInBuffers = 0;
         OMX_U32 nOutBuffers = 0;
         int i;
         OMX_BUFFERHEADERTYPE* pBuffer;

         if (m_bFlushingIndex_in)
         {
            OMX_VENC_MSG_HIGH("flushing free input buffers...", 0, 0, 0);
            (void) m_pFreeInBM->GetNumBuffers(&nInBuffers);     
            for (i = 0; i < (int) nInBuffers; i++)
            {
               if (m_pFreeInBM->PopFirstBuffer(&pBuffer) == OMX_ErrorNone)
               {
                  pBuffer->nFilledLen = 0;
                  if (m_sCallbacks.EmptyBufferDone(m_hSelf,m_pAppData,pBuffer)
                      != OMX_ErrorNone)
                  {
                     OMX_VENC_MSG_ERROR("EBD failed", 0, 0, 0);
                  }
               }
               else
               {
                  OMX_VENC_MSG_ERROR("failed to pop buffer", 0, 0, 0);
               }
            }

            m_bFlushingIndex_in = OMX_FALSE;

            m_sCallbacks.EventHandler(m_hSelf,
                                      m_pAppData,
                                      OMX_EventCmdComplete,
                                      (OMX_U32) OMX_CommandFlush,
                                      (OMX_U32) PORT_INDEX_IN,
                                      NULL);

            if (m_nInputDown > 0)
            {
               // sanity check device layer should not own any buffers at this point
               OMX_VENC_MSG_ERROR("Device layer still has %d input buffers, find the bug", m_nInputDown, 0, 0);
            }
         }

         if (m_bFlushingIndex_out)
         {
            OMX_VENC_MSG_HIGH("flushing free output buffers...", 0, 0, 0);
            (void) m_pFreeOutBM->GetNumBuffers(&nOutBuffers);
            for (i = 0; i < (int) nOutBuffers; i++)
            {
               if (m_pFreeOutBM->PopFirstBuffer(&pBuffer) == OMX_ErrorNone)
               {
                  pBuffer->nFilledLen = 0;
                  if (m_sCallbacks.FillBufferDone(m_hSelf,m_pAppData,pBuffer)
                      != OMX_ErrorNone)
                  {
                     OMX_VENC_MSG_ERROR("FBD failed", 0, 0, 0);
                  }
               }
               else
               {
                  OMX_VENC_MSG_ERROR("failed to pop buffer", 0, 0, 0);
               }
            }      

            m_bFlushingIndex_out = OMX_FALSE;

            m_sCallbacks.EventHandler(m_hSelf,
                                      m_pAppData,
                                      OMX_EventCmdComplete,
                                      (OMX_U32) OMX_CommandFlush,
                                      (OMX_U32) PORT_INDEX_OUT,
                                      NULL);

            if(m_nOutputDown > 0)
            {
               // sanity check device layer should not own any buffers at this point
               OMX_VENC_MSG_ERROR("Device layer still has %d output buffers, find the bug", m_nOutputDown, 0, 0);
            }
         }

         break;
      }
      case VENC_CMD_SET_INTRA_REFRESH:
	    {
         OMX_VENC_MSG_LOW("got DL status for VENC_CMD_SET_INTRA_REFRESH", 0, 0, 0); 
		     break;
	    }
	  case VENC_CMD_REQUEST_KEY_FRAME:
		{
           OMX_VENC_MSG_LOW("got DL status for VENC_CMD_REQUEST_KEY_FRAME", 0, 0, 0);
           break;
		}
      case VENC_CMD_SET_INTRA_PERIOD:
		{
           OMX_VENC_MSG_LOW("got DL status for VENC_CMD_SET_INTRA_PERIOD", 0, 0, 0);
           break;
		}
     case VENC_CMD_CHANGE_QUALITY:
        {
           OMX_VENC_MSG_LOW("got DL status for VENC_CMD_CHANGE_QUALITY", 0, 0, 0);
           break;
        }
      case VENC_CMD_REQUEST_SYNTAX_HEADER:
      case VENC_CMD_REGISTER_CHANNEL_STATS_CB:
         OMX_VENC_MSG_ERROR("we never sent this command %d", (int) eCommand, 0, 0);
         break;
      default:
         OMX_VENC_MSG_ERROR("invalid command status %d", (int) eCommand, 0, 0);
         break;
   }
}

/**********************************************************************//**
 * @brief Process thread message MSG_ID_DL_FRAME_DONE
 *
 * This method abstracts the header file, params are to be interpreted
 * as follows:
 *
 * @param nStatus: Status for the completed command (venc_status_type).
 * @param pInfo: Frame specific information (venc_frame_info_type).
 *************************************************************************/
void Venc::process_DL_frame_done(OMX_U32 nStatus,
                                 const void* pInfo)
{
   venc_status_type eStatus = static_cast<venc_status_type>(nStatus);
   const venc_frame_info_type* pFrameInfo = 
      reinterpret_cast<const venc_frame_info_type*>(pInfo);
   if(pFrameInfo == NULL)
   {
      OMX_VENC_MSG_ERROR("client data is null in status callback", 0, 0, 0);
      return;
   }
   switch (eStatus)
   {
   case VENC_STATUS_SUCCESS:
      OMX_VENC_MSG_MEDIUM("frame was coded successfully with length %d ",pFrameInfo->len, 0, 0);
      #ifdef VENC_DUMP_OUTPUT 
	   mem_dump((char*)pFrameInfo->frame_ptr,pFrameInfo->len,0);
      #endif
      break;
   case VENC_STATUS_NOT_ENCODED:
      OMX_VENC_MSG_MEDIUM("frame was not coded", 0, 0, 0);
      break;
   case VENC_STATUS_FLUSHING:
      OMX_VENC_MSG_MEDIUM("frame was flushed", 0, 0, 0);
      break;
   default:
      OMX_VENC_MSG_ERROR("got failure status for frame encode %d", (int) eStatus, 0, 0);
      break;
   };

   --m_nOutputDown;

   OMX_BUFFERHEADERTYPE* pBuffer = (OMX_BUFFERHEADERTYPE*) pFrameInfo->output_client_data;
   OMX_VENC_MSG_MEDIUM("pBuffer =0x%x,EoS =%d", pBuffer->pBuffer,(pBuffer->nFlags & OMX_BUFFERFLAG_EOS), 0);
   pBuffer->nFilledLen = pFrameInfo->len;

   if(pBuffer->nFilledLen > pBuffer->nAllocLen)
   {
      OMX_VENC_MSG_ERROR("Buffer overrun.....Increase output buffer size filled(%d) vs Allocated (%d)", pBuffer->nFilledLen, pBuffer->nAllocLen, 0);
   }
   if (pFrameInfo->frame_type == 0)
   {
      OMX_VENC_MSG_MEDIUM("ITS I FRAME", 0, 0, 0);
      pBuffer->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;
   }

   PrivatePortData* pOutPrivateData = (PrivatePortData*) pBuffer->pOutputPortPrivate;

   // override the frame size to zero
   if (pOutPrivateData->bZeroLen == OMX_TRUE)
   {
      pBuffer->nFilledLen = 0;
   }

   if (pBuffer->nFilledLen > 0)
   {
      pBuffer->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
   }

   // make a copy of the header to prevent race conditions
   // it is possible for client to modify the buffer header
   // before we process the buffer header after the
   // fill done callback
   OMX_BUFFERHEADERTYPE sBufferCopy = *pBuffer;

   if (m_sCallbacks.FillBufferDone(m_hSelf,m_pAppData,pBuffer) != OMX_ErrorNone)
   {
       OMX_VENC_MSG_ERROR("FBD failed", 0, 0, 0);
   }

   //if the buffer is marked, send the event for the same
   if (m_Mark_port == OMX_TRUE && sBufferCopy.hMarkTargetComponent && sBufferCopy.pMarkData )
   {
       OMX_VENC_MSG_LOW("buffer mark compete,sending the event", 0, 0, 0);
       m_sCallbacks.EventHandler(m_hSelf, 
                                  m_pAppData,
                                  OMX_EventMark, 
                                  (OMX_U32) 0,
                                  (OMX_U32) 0 , sBufferCopy.pMarkData );
       m_Mark_port = OMX_FALSE;
   }

   // Set buffer flag event for EOS
   if (sBufferCopy.nFlags & OMX_BUFFERFLAG_EOS)
   {
      OMX_VENC_MSG_MEDIUM("EOS obtained at the O/P buffer", 0, 0, 0);
      m_sCallbacks.EventHandler(m_hSelf, 
                                m_pAppData,
                                OMX_EventBufferFlag, 
                                (OMX_U32) OMX_BUFFERFLAG_EOS,
                                (OMX_U32) sBufferCopy.nFlags , NULL);
   }
}

/**********************************************************************//**
 * @brief Process thread message MSG_ID_YUV_BUFF_DONE
 *
 * This method abstracts the header file, params are to be interpreted
 * as follows:
 *
 *************************************************************************/

void Venc::process_DL_empty_done(const void* pInfo)
{
   const venc_status_param_type* pBuffInfo = 
        reinterpret_cast<const venc_status_param_type*>(pInfo);
   OMX_BUFFERHEADERTYPE* buffer = (OMX_BUFFERHEADERTYPE*) pBuffInfo->handshake_status.input_client_data;

   --m_nInputDown;

   if (m_sCallbacks.EmptyBufferDone(m_hSelf,m_pAppData,buffer) != OMX_ErrorNone)
   {
       OMX_VENC_MSG_ERROR("FBD failed", 0, 0, 0);
   }
}

/**********************************************************************//**
 * @brief Device layer status callback
 * 
 * Sends MSG_ID_DL_STATUS to the component thread for processing
 *************************************************************************/
void Venc::DL_status_callback(uint32 nDLHandle,
                              venc_event_type eStatusEvent,
                              venc_status_param_type sParam,
                              void* pClientData)
{
   OMX_VENC_MSG_LOW("Got DL status callback", 0, 0, 0);
   Venc* pVenc = reinterpret_cast<Venc*>(pClientData);
   if (pVenc == NULL)
   {
      OMX_VENC_MSG_ERROR("client data is null in status callback", 0, 0, 0);
      return;
   }

   VencMsgQ::MsgIdType msgId;
   VencMsgQ::MsgDataType msgData;

   switch (eStatusEvent)
   {
   case VENC_EVENT_CMD_COMPLETE:
      // device layer command status
      msgId = VencMsgQ::MSG_ID_DL_STATUS;
      OMX_VENC_MSG_MEDIUM("Got DL status cmd complete", 0, 0, 0);
      msgData.sDLStatus.nDLHandle = nDLHandle;
      msgData.sDLStatus.eCommand = sParam.cmd_status.cmd_complete;
      msgData.sDLStatus.eStatus = sParam.cmd_status.cmd_status;           
      pVenc->m_pMsgQ->PushMsg(msgId, &msgData);   
      break;

   case VENC_EVENT_INPUT_HANDSHAKE:
      OMX_VENC_MSG_MEDIUM("got DL status yuv handshake", 0, 0, 0);
      (void) nDLHandle; // we don't need this
      msgId = VencMsgQ::MSG_ID_YUV_BUFF_DONE;
      msgData.sDLYUVDone.sBuffInfo = sParam;
      pVenc->m_pMsgQ->PushMsg(msgId, &msgData);
      break;

   default:
      OMX_VENC_MSG_ERROR("got unexpected status %d", (int) eStatusEvent, 0, 0);
      break;;
   }

}

/**********************************************************************//**
 * @brief Device layer frame callback
 * 
 * Sends MSG_ID_FRAME_DONE to the component thread for processing
 *************************************************************************/
void Venc::DL_frame_callback(uint32 nDLHandle,
                             venc_status_type eStatus,
                             venc_frame_info_type sFrameInfo,
                             void* pClientData)
{
   OMX_VENC_MSG_MEDIUM("Got DL frame callback", 0, 0, 0);
   Venc* pVenc = reinterpret_cast<Venc*>(pClientData);
   if (pVenc == NULL)
   {
      OMX_VENC_MSG_ERROR("client data is null in frame callback", 0, 0, 0);
      return;
   }

   VencMsgQ::MsgIdType msgId = VencMsgQ::MSG_ID_DL_FRAME_DONE;
   VencMsgQ::MsgDataType msgData;
   (void) nDLHandle; // we don't need this
   msgData.sDLFrameDone.eStatus = eStatus;
   msgData.sDLFrameDone.sFrameInfo = sFrameInfo;

   pVenc->m_pMsgQ->PushMsg(msgId, &msgData);
}

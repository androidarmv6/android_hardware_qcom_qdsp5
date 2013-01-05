/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/src/venctest_Encoder.cpp#6 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2010-06-12   as     Added support for Dynamic configuration for IntraRefreshMB
2010-04-02   as     Added support for TimeStamp scale factor
2010-03-22   as     Added approved OMX extension
2009-10-08   as     Corrected level for Mpeg4 Vt pass.
2009-09-28   as     Changed qp range for Mpeg4 and H264.
2009-08-24   bc     On-target integration updates
2009-08-13   bc     Beefed up error handling
2009-08-13   bc     cleanup
2009-08-13   bc     Resolved action items from code reviews 61288 and 61438
2009-08-13   bc     Moved stats thread to device layer
2009-08-13   bc     OMX_Init and OXM_Deinit no longer called per instance (per the IL spec)
2009-07-06   bc     Added error handling for async commands
2009-06-30   bc     Added support for DVS
2009-06-23   bc     Decreased stats thread wait time for better precision
2009-05-06   bc     Initial revision

========================================================================== */

/*========================================================================
  Include Files
 ==========================================================================*/
#include "venctest_Encoder.h"
#include "venctest_Debug.h"
#include "venctest_Signal.h"
#include "venctest_SignalQueue.h"
#include "OMX_Component.h"
#include "OMX_QCOMExtns.h"
#include "string.h" 
#include "stdlib.h"

namespace venctest
{

   static const OMX_U32 PORT_INDEX_IN  = 0;
   static const OMX_U32 PORT_INDEX_OUT = 1;
   struct CmdType
   {
      OMX_EVENTTYPE   eEvent;
      OMX_COMMANDTYPE eCmd;
      OMX_U32         sCmdData;
      OMX_ERRORTYPE   eResult;
   };

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   Encoder::Encoder()
   {
      VENC_TEST_MSG_ERROR("Should not be here this is a private constructor", 0, 0, 0);
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   Encoder::Encoder(EmptyDoneCallbackType pEmptyDoneFn,
                    FillDoneCallbackType pFillDoneFn,
                    OMX_PTR pAppData,
                    OMX_VIDEO_CODINGTYPE eCodec)
      : m_pSignalQueue(new SignalQueue(32, sizeof(CmdType))),
        m_pEventFn(NULL),
        m_pEmptyDoneFn(pEmptyDoneFn),
        m_pFillDoneFn(pFillDoneFn),
        m_pAppData(pAppData),
        m_pInputBuffers(NULL),
        m_pOutputBuffers(NULL),
        m_hEncoder(NULL),
        m_eState(OMX_StateLoaded),
        m_nInputBuffers(0),
        m_nOutputBuffers(0),
        m_nInputBufferSize(0),
        m_nOutputBufferSize(0),
        m_eCodec(eCodec),
        m_nInFrameIn(0),
        m_nOutFrameIn(0),
        m_nInFrameOut(0),
        m_nOutFrameOut(0)
   {
      static OMX_CALLBACKTYPE callbacks = {EventCallback,
                                           EmptyDoneCallback,
                                           FillDoneCallback};

      if (m_pEmptyDoneFn == NULL)
      {
         VENC_TEST_MSG_ERROR("Empty buffer fn is NULL", 0, 0, 0);
      }
      if (m_pFillDoneFn == NULL)
      {
         VENC_TEST_MSG_ERROR("Fill buffer fn is NULL", 0, 0, 0);
      }

      // MPEG4
      if (eCodec == OMX_VIDEO_CodingMPEG4)
      {
         VENC_TEST_MSG_MEDIUM("Getting handle for mpeg4", 0, 0, 0);
         if (OMX_GetHandle(&m_hEncoder,
                           "OMX.qcom.video.encoder.mpeg4",
                           this,
                           &callbacks) != OMX_ErrorNone)
         {
            VENC_TEST_MSG_ERROR("Error getting component handle", 0, 0, 0);
         }
      }
      // H263
      else if (eCodec == OMX_VIDEO_CodingH263)
      {
         VENC_TEST_MSG_MEDIUM("Getting handle for h263", 0, 0, 0);
         if (OMX_GetHandle(&m_hEncoder,
                           "OMX.qcom.video.encoder.h263",
                           this,
                           &callbacks) != OMX_ErrorNone)
         {
            VENC_TEST_MSG_ERROR("Error getting component handle", 0, 0, 0);
         }
      }
      // H264
      else if (eCodec == OMX_VIDEO_CodingAVC)
      {
         VENC_TEST_MSG_MEDIUM("Getting handle for avc", 0, 0, 0);
         if (OMX_GetHandle(&m_hEncoder,
                           "OMX.qcom.video.encoder.avc",
                           this,
                           &callbacks) != OMX_ErrorNone)
         {
            VENC_TEST_MSG_ERROR("Error getting component handle", 0, 0, 0);
         }
      }
      else
      {
         VENC_TEST_MSG_ERROR("Invalid codec selection", 0, 0, 0);
      }
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   Encoder::~Encoder()
   {
      OMX_FreeHandle(m_hEncoder);
      if (m_pSignalQueue)
      {
         delete m_pSignalQueue;
      }
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Encoder::Configure(EncoderConfigType* pConfig)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;

      if (pConfig == NULL)
      {
         result = OMX_ErrorBadParameter;
      }
      else
      {
         m_nInputBuffers = pConfig->nInBufferCount;
         m_nOutputBuffers = pConfig->nOutBufferCount;
      }

      //////////////////////////////////////////
      // codec specific config
      //////////////////////////////////////////
	              
      OMX_VIDEO_PARAM_MPEG4TYPE mp4;
      OMX_VIDEO_PARAM_H263TYPE h263;
      OMX_VIDEO_PARAM_AVCTYPE avc;

      if (result == OMX_ErrorNone)
      {
         if (m_eCodec == OMX_VIDEO_CodingMPEG4)
         {
            mp4.nPortIndex = (OMX_U32) PORT_INDEX_OUT; // output
            result = OMX_GetParameter(m_hEncoder,
                                      OMX_IndexParamVideoMpeg4,
                                      (OMX_PTR) &mp4);

            if (result == OMX_ErrorNone)
            {
               mp4.nTimeIncRes = pConfig->nTimeIncRes;
               mp4.nPFrames = pConfig->nIntraPeriod - 1;

               mp4.eProfile = OMX_VIDEO_MPEG4ProfileSimple;

               if (pConfig->nOutputFrameWidth > 176 && 
                   pConfig->nOutputFrameHeight > 144)
               {
                  mp4.eLevel = OMX_VIDEO_MPEG4Level2;
               }
               else if(((pConfig->nOutputFrameWidth == 176 && 
                        pConfig->nOutputFrameHeight == 144) ||
				        (pConfig->nOutputFrameWidth == 144 && 
					    pConfig->nOutputFrameHeight == 176)) && 
					    (pConfig->eResyncMarkerType == RESYNC_MARKER_BITS))
               {
				   mp4.eLevel = OMX_VIDEO_MPEG4Level0b;
			   }
               else
               {
                  mp4.eLevel = OMX_VIDEO_MPEG4Level0;
               }
               mp4.bACPred = OMX_TRUE;
               mp4.bSVH = pConfig->bEnableShortHeader;
               mp4.nHeaderExtension = pConfig->nHECInterval;

               result = OMX_SetParameter(m_hEncoder,
                                         OMX_IndexParamVideoMpeg4,
                                         (OMX_PTR) &mp4);
            }
         }
         else if (m_eCodec == OMX_VIDEO_CodingH263)
         {
            h263.nPortIndex = (OMX_U32) PORT_INDEX_OUT; // output
            result = OMX_GetParameter(m_hEncoder,
                                      OMX_IndexParamVideoH263,
                                      (OMX_PTR) &h263);

            if (result == OMX_ErrorNone)
            {
               h263.nPFrames = pConfig->nIntraPeriod - 1;
               h263.nBFrames = 0;

               h263.eProfile = OMX_VIDEO_H263ProfileBaseline;

               h263.eLevel = OMX_VIDEO_H263Level10;
	       if(((pConfig->nOutputFrameWidth == 176 && 
                      pConfig->nOutputFrameHeight == 144) ||
		      (pConfig->nOutputFrameWidth == 144 && 
		       pConfig->nOutputFrameHeight == 176)) && 
	               (pConfig->nBitrate >= 128000))
	            {
		       h263.eLevel = OMX_VIDEO_H263Level45;
		     }

               h263.bPLUSPTYPEAllowed = OMX_FALSE;
               h263.nAllowedPictureTypes = 2;
               h263.bForceRoundingTypeToZero = OMX_TRUE;
               h263.nPictureHeaderRepetition = 0;


               if (pConfig->eResyncMarkerType == RESYNC_MARKER_GOB)
               {
                  h263.nGOBHeaderInterval = pConfig->nResyncMarkerSpacing;
               }
               else if (pConfig->eResyncMarkerType == RESYNC_MARKER_MB)
               {
                  OMX_VIDEO_PARAM_MPEG4TYPE mp4;
                  mp4.nPortIndex = (OMX_U32) PORT_INDEX_OUT; // output
                  result = OMX_GetParameter(m_hEncoder,
                                            OMX_IndexParamVideoMpeg4,
                                            (OMX_PTR) &mp4);

                  if (result == OMX_ErrorNone)
                  {
                     // annex k
                     mp4.nSliceHeaderSpacing = pConfig->nResyncMarkerSpacing;

                     result = OMX_SetParameter(m_hEncoder,
                                               OMX_IndexParamVideoMpeg4,
                                               (OMX_PTR) &mp4);
                  }

                  h263.nGOBHeaderInterval = 0;
               }
               else
               {
                  h263.nGOBHeaderInterval = 0;
               }

               result = OMX_SetParameter(m_hEncoder,
                                         OMX_IndexParamVideoH263,
                                         (OMX_PTR) &h263);
            }
         }
         else if (m_eCodec == OMX_VIDEO_CodingAVC)
         {
            avc.nPortIndex = (OMX_U32) PORT_INDEX_OUT; // output
            result = OMX_GetParameter(m_hEncoder,
                                      OMX_IndexParamVideoAvc,
                                      (OMX_PTR) &avc);

            if (result == OMX_ErrorNone)
            {
               avc.nPFrames = pConfig->nIntraPeriod - 1;
               avc.nBFrames = 0;
               avc.eProfile = OMX_VIDEO_AVCProfileBaseline;
               avc.eLevel = OMX_VIDEO_AVCLevel1;
			   if (pConfig->nOutputFrameWidth * pConfig->nOutputFrameHeight / 256 <= 99)
			   {
				   avc.eLevel = OMX_VIDEO_AVCLevel1b;
			   }
               else if (pConfig->nOutputFrameWidth * pConfig->nOutputFrameHeight / 256  <= 396)
               {
                  avc.eLevel = OMX_VIDEO_AVCLevel2; 
               }
               else if (pConfig->nOutputFrameWidth * pConfig->nOutputFrameHeight / 256 <= 792)
               {
                  avc.eLevel = OMX_VIDEO_AVCLevel21;    
               }
               else if (pConfig->nOutputFrameWidth * pConfig->nOutputFrameHeight / 256  <= 1620)
               {
                  avc.eLevel = OMX_VIDEO_AVCLevel22; 
               }

               if (pConfig->eResyncMarkerType == RESYNC_MARKER_MB)
               {
                  avc.nSliceHeaderSpacing = pConfig->nResyncMarkerSpacing;
               }
               else
               {
                  avc.nSliceHeaderSpacing = 0;
               }

               result = OMX_SetParameter(m_hEncoder,
                                         OMX_IndexParamVideoAvc,
                                         (OMX_PTR) &avc);
            }
         }

      }

      //////////////////////////////////////////
      // error resilience
      //////////////////////////////////////////
      if (result == OMX_ErrorNone)
      {
         // see if we already configured error resilience (which is the case for h263)
         if (pConfig->eResyncMarkerType != RESYNC_MARKER_GOB)
         {
            OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE errorCorrection; //OMX_IndexParamVideoErrorCorrection
            errorCorrection.nPortIndex = (OMX_U32) PORT_INDEX_OUT; // output
            result = OMX_GetParameter(m_hEncoder,
                                      OMX_IndexParamVideoErrorCorrection,
                                      (OMX_PTR) &errorCorrection);

            if (result == OMX_ErrorNone)
            {
               if (m_eCodec == OMX_VIDEO_CodingMPEG4)
               {
                  errorCorrection.bEnableHEC = pConfig->nHECInterval == 0 ? OMX_FALSE : OMX_TRUE;
               }
               else
               {
                  errorCorrection.bEnableHEC = OMX_FALSE;
               }

               if (pConfig->eResyncMarkerType == RESYNC_MARKER_BITS)
               {
                  errorCorrection.bEnableResync = OMX_TRUE;
                  errorCorrection.nResynchMarkerSpacing = pConfig->nResyncMarkerSpacing;
               }
               else
               {
                  errorCorrection.bEnableResync = OMX_FALSE;
                  errorCorrection.nResynchMarkerSpacing = 0;
               }

               errorCorrection.bEnableDataPartitioning = OMX_FALSE;    // MP4 Resync

               result = OMX_SetParameter(m_hEncoder,
                                         OMX_IndexParamVideoErrorCorrection,
                                         (OMX_PTR) &errorCorrection);

               result = OMX_SetParameter(m_hEncoder,
                                         OMX_IndexParamVideoErrorCorrection,
                                         (OMX_PTR) &errorCorrection);
            }
         }
      }

      //////////////////////////////////////////
      // qp
      //////////////////////////////////////////
      if (result == OMX_ErrorNone)
      {
         QOMX_VIDEO_TEMPORALSPATIALTYPE tsTradeoffFactor;

         /* Retrive the current information from the IL component */
         result = OMX_GetConfig(m_hEncoder,
                               (OMX_INDEXTYPE) QOMX_IndexConfigVideoTemporalSpatialTradeOff, 
                               (OMX_PTR) &tsTradeoffFactor);
         if (result == OMX_ErrorNone)
         {
            /* Populate structure with the specific information */
            tsTradeoffFactor.nTSFactor = (OMX_U32) pConfig->nMaxQp;
            // Set the index here
            OMX_SetConfig(m_hEncoder,
                         (OMX_INDEXTYPE)QOMX_IndexConfigVideoTemporalSpatialTradeOff, 
                         (OMX_PTR) &tsTradeoffFactor);
         }
      }
#ifndef FEATURE_LINUX_A 
	  //////////////////////////////////////////
      // ScaleFactor
      //////////////////////////////////////////
      if (result == OMX_ErrorNone)
      {
         QOMX_TIMESTAMPSCALETYPE timestampScaleType;

         /* Retrive the current information from the IL component */
         result = OMX_GetParameter(m_hEncoder,
                             (OMX_INDEXTYPE) QOMX_IndexConfigTimestampScale,
                             (OMX_PTR) &timestampScaleType);
         if (result == OMX_ErrorNone)
         {
            /* Populate structure with the specific information */
            /* Populate timestamp scale factor with the specific information */
            timestampScaleType.nTimestampScale = (OMX_U32) pConfig->nScaleFactor <<16;
            // Set the index here
            OMX_SetParameter(m_hEncoder,
                                  (OMX_INDEXTYPE) QOMX_IndexConfigTimestampScale,
                                  (OMX_PTR) &timestampScaleType);
         }
      }
#endif
      //////////////////////////////////////////
      // bitrate
      //////////////////////////////////////////
      if (result == OMX_ErrorNone)
      {
         OMX_VIDEO_PARAM_BITRATETYPE bitrate; // OMX_IndexParamVideoBitrate
         bitrate.nPortIndex = (OMX_U32) PORT_INDEX_OUT; // output
         result = OMX_GetParameter(m_hEncoder,
                                   OMX_IndexParamVideoBitrate,
                                   (OMX_PTR) &bitrate);
         if (result == OMX_ErrorNone)
         {
            bitrate.eControlRate = pConfig->eControlRate;
            bitrate.nTargetBitrate = pConfig->nBitrate;
            result = OMX_SetParameter(m_hEncoder,
                                      OMX_IndexParamVideoBitrate,
                                      (OMX_PTR) &bitrate);
         }
      }
      ///////////////////////////////////////////////////////
      // OMX_IndexParamVideoProfileLevelQuerySupported
      ///////////////////////////////////////////////////////

      if (result == OMX_ErrorNone)
      {
         OMX_VIDEO_PARAM_PROFILELEVELTYPE m_sParamProfileLevel;
         m_sParamProfileLevel.nProfileIndex = 0;
         m_sParamProfileLevel.nPortIndex = (OMX_U32) PORT_INDEX_OUT; // output
		 result = OMX_GetParameter(m_hEncoder,
                                   OMX_IndexParamVideoProfileLevelQuerySupported,
                                   (OMX_PTR) &m_sParamProfileLevel);
         if (result == OMX_ErrorNone)
         {
		    if(m_eCodec ==  OMX_VIDEO_CodingMPEG4)
			{
				m_sParamProfileLevel.eLevel = mp4.eLevel;
				m_sParamProfileLevel.eProfile = mp4.eProfile;
			}
			else if (m_eCodec ==  OMX_VIDEO_CodingH263)
			{
			   m_sParamProfileLevel.eLevel = h263.eLevel;
			   m_sParamProfileLevel.eProfile = h263.eProfile;
			}
			else if (m_eCodec ==  OMX_VIDEO_CodingAVC)
			{
				m_sParamProfileLevel.eLevel = avc.eLevel;
			    m_sParamProfileLevel.eProfile = avc.eProfile;
			}
             result = OMX_SetParameter(m_hEncoder,
                                      OMX_IndexParamVideoProfileLevelCurrent,
                                      (OMX_PTR) &m_sParamProfileLevel);
         }
      }

      //////////////////////////////////////////
      // quantization
      //////////////////////////////////////////
      if (result == OMX_ErrorNone)
      {
         if (pConfig->eControlRate == OMX_Video_ControlRateDisable)
         {
            ///////////////////////////////////////////////////////////
            // QP Config
            ///////////////////////////////////////////////////////////
            OMX_VIDEO_PARAM_QUANTIZATIONTYPE qp; // OMX_IndexParamVideoQuantization
            qp.nPortIndex = (OMX_U32) PORT_INDEX_OUT; // output
            result = OMX_GetParameter(m_hEncoder,
                                      OMX_IndexParamVideoQuantization,
                                      (OMX_PTR) &qp);
            if (result == OMX_ErrorNone)
            {
               if (m_eCodec == OMX_VIDEO_CodingAVC)
               {
                  qp.nQpI = 30;
                  qp.nQpP = 30;
               }
               else
               {
                  qp.nQpI = 10;
                  qp.nQpP = 10;
               }
               result = OMX_SetParameter(m_hEncoder,
                                         OMX_IndexParamVideoQuantization,
                                         (OMX_PTR) &qp);
            }
         }
      }

      //////////////////////////////////////////
      // frame rate
      //////////////////////////////////////////
      if (result == OMX_ErrorNone)
      {
         OMX_CONFIG_FRAMERATETYPE framerate; // OMX_IndexConfigVideoFramerate
         framerate.nPortIndex = (OMX_U32) PORT_INDEX_IN; // input
         result = OMX_GetConfig(m_hEncoder,
                                OMX_IndexConfigVideoFramerate,
                                (OMX_PTR) &framerate);
         if (result == OMX_ErrorNone)
         {
            framerate.xEncodeFramerate = pConfig->nFramerate << 16;
            result = OMX_SetConfig(m_hEncoder,
                                   OMX_IndexConfigVideoFramerate,
                                   (OMX_PTR) &framerate);
         }
      }

      //////////////////////////////////////////
      // rotation
      //////////////////////////////////////////
      if (result == OMX_ErrorNone)
      {
         OMX_CONFIG_ROTATIONTYPE framerotate; // OMX_IndexConfigCommonRotate
         framerotate.nPortIndex = (OMX_U32) PORT_INDEX_OUT; // output
         result = OMX_GetConfig(m_hEncoder,
                                OMX_IndexConfigCommonRotate,
                                (OMX_PTR) &framerotate);
         if (result == OMX_ErrorNone)
         {

            framerotate.nRotation = pConfig->nRotation;

            result = OMX_SetConfig(m_hEncoder,
                                   OMX_IndexConfigCommonRotate,
                                   (OMX_PTR) &framerotate);
         }
      }

      //////////////////////////////////////////
      // intra refresh
      //////////////////////////////////////////
      if (result == OMX_ErrorNone)
      {
         OMX_VIDEO_PARAM_INTRAREFRESHTYPE ir; // OMX_IndexParamVideoIntraRefresh
         ir.nPortIndex = (OMX_U32) PORT_INDEX_OUT; // output
         result = OMX_GetParameter(m_hEncoder,
                                   OMX_IndexParamVideoIntraRefresh,
                                   (OMX_PTR) &ir);
         if (result == OMX_ErrorNone)
         {
            /// @todo need an extension dynamic for intra refresh configuration
            ir.eRefreshMode = pConfig->bEnableIntraRefresh == OMX_TRUE ?
               OMX_VIDEO_IntraRefreshCyclic : OMX_VIDEO_IntraRefreshMax;
            ir.nCirMBs = 5;

            result = OMX_SetParameter(m_hEncoder,
                                      OMX_IndexParamVideoIntraRefresh,
                                      (OMX_PTR) &ir);
         }
      }

      //////////////////////////////////////////
      // input buffer requirements
      //////////////////////////////////////////
      if (result == OMX_ErrorNone)
      {
         OMX_PARAM_PORTDEFINITIONTYPE inPortDef; // OMX_IndexParamPortDefinition
         inPortDef.nPortIndex = (OMX_U32) PORT_INDEX_IN; // input
         result = OMX_GetParameter(m_hEncoder,
                                   OMX_IndexParamPortDefinition,
                                   (OMX_PTR) &inPortDef);
         if (result == OMX_ErrorNone)
         {
            inPortDef.nBufferCountActual = m_nInputBuffers;
            inPortDef.format.video.nFrameWidth = pConfig->nFrameWidth;
            inPortDef.format.video.nFrameHeight = pConfig->nFrameHeight;
            result = OMX_SetParameter(m_hEncoder,
                                      OMX_IndexParamPortDefinition,
                                      (OMX_PTR) &inPortDef);
            result = OMX_GetParameter(m_hEncoder,
                                      OMX_IndexParamPortDefinition,
                                      (OMX_PTR) &inPortDef);

            if (m_nInputBuffers != (OMX_S32) inPortDef.nBufferCountActual)
            {
               VENC_TEST_MSG_ERROR("Buffer reqs dont match...", 0, 0, 0);
            }
         }

         m_nInputBufferSize = (OMX_S32) inPortDef.nBufferSize;
      }

      //////////////////////////////////////////
      // output buffer requirements
      //////////////////////////////////////////
      if (result == OMX_ErrorNone)
      {
         OMX_PARAM_PORTDEFINITIONTYPE outPortDef; // OMX_IndexParamPortDefinition
         outPortDef.nPortIndex = (OMX_U32) PORT_INDEX_OUT; // output
         result = OMX_GetParameter(m_hEncoder,
                                   OMX_IndexParamPortDefinition,
                                   (OMX_PTR) &outPortDef);
         if (result == OMX_ErrorNone)
         {
            outPortDef.nBufferCountActual = m_nOutputBuffers;
            outPortDef.format.video.nFrameWidth = pConfig->nOutputFrameWidth;
            outPortDef.format.video.nFrameHeight = pConfig->nOutputFrameHeight;
            result = OMX_SetParameter(m_hEncoder,
                                      OMX_IndexParamPortDefinition,
                                      (OMX_PTR) &outPortDef);
            result = OMX_GetParameter(m_hEncoder,
                                      OMX_IndexParamPortDefinition,
                                      (OMX_PTR) &outPortDef);

            if (m_nOutputBuffers != (OMX_S32) outPortDef.nBufferCountActual)
            {
               VENC_TEST_MSG_ERROR("Buffer reqs dont match...", 0, 0, 0);
            }
         }

         m_nOutputBufferSize = (OMX_S32) outPortDef.nBufferSize;
      }

      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Encoder::GoToExecutingState()
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;

      if (m_eState == OMX_StateLoaded)
      {
         ///////////////////////////////////////
         // 1. send idle state command
         // 2. allocate input buffers
         // 3. allocate output buffers
         // 4. wait for idle state complete
         // 5. send executing state command and wait for complete
         ///////////////////////////////////////

         m_nInFrameIn = 0;
         m_nOutFrameIn = 0;
         m_nInFrameOut = 0;
         m_nOutFrameOut = 0;

         // send idle state comand
         VENC_TEST_MSG_MEDIUM("going to state OMX_StateIdle...", 0, 0, 0);
         result = SetState(OMX_StateIdle, OMX_FALSE);

         if (result == OMX_ErrorNone)
         {
            result = AllocateBuffers();
         }

         // wait for idle state complete
         if (result == OMX_ErrorNone)
         {
            result = WaitState(OMX_StateIdle);
         }

         // send executing state command and wait for complete
         if (result == OMX_ErrorNone)
         {
            VENC_TEST_MSG_MEDIUM("going to state OMX_StateExecuting...", 0, 0, 0);
            result = SetState(OMX_StateExecuting, OMX_TRUE);
         }

      }
      else
      {
         VENC_TEST_MSG_ERROR("invalid state", 0, 0, 0);
         result = OMX_ErrorIncorrectStateTransition;
      }

      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Encoder::GetOutOfBandSyntaxHeader(OMX_U8* pSyntaxHdr,
                                                   OMX_S32 nSyntaxHdrLen,
                                                   OMX_S32* pSyntaxHdrFilledLen)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;
      QOMX_VIDEO_SYNTAXHDRTYPE syntax;
      QOMX_VIDEO_SYNTAXHDRTYPE* pSyntax;
      syntax.nBytes = 0;
         result = OMX_GetParameter(m_hEncoder,
                                (OMX_INDEXTYPE) QOMX_IndexParamVideoSyntaxHdr,
                                   (OMX_PTR) &syntax);
      if ((result == OMX_ErrorNone) && (syntax.nBytes))
      {
         nSyntaxHdrLen = syntax.nBytes;
	        pSyntax = (QOMX_VIDEO_SYNTAXHDRTYPE*) malloc(sizeof(QOMX_VIDEO_SYNTAXHDRTYPE) + nSyntaxHdrLen - 1);
	        pSyntax->nBytes = syntax.nBytes;
         if (pSyntaxHdr != NULL && nSyntaxHdrLen > 0 && pSyntaxHdrFilledLen != NULL)
         {
             pSyntax->nPortIndex = (OMX_U32) PORT_INDEX_OUT;
         result = OMX_GetParameter(m_hEncoder,
				                                  (OMX_INDEXTYPE) QOMX_IndexParamVideoSyntaxHdr,
				                                  (OMX_PTR) pSyntax);
             if ((result == OMX_ErrorNone) && (syntax.nBytes))
         {
		              *pSyntaxHdrFilledLen = (OMX_S32) pSyntax->nBytes;
		              memcpy(pSyntaxHdr,pSyntax->data, sizeof(char) * pSyntax->nBytes); 
		              delete pSyntax;
		              pSyntax = NULL;
         }
         else
         {
            VENC_TEST_MSG_ERROR("failed to get syntax header", 0, 0, 0);
         }
      }
      else
      {
         VENC_TEST_MSG_ERROR("bad param(s)", 0, 0, 0);
         result = OMX_ErrorBadParameter;
      }
    }
    else
    {
         VENC_TEST_MSG_ERROR("bad param(s)", 0, 0, 0);
         result = OMX_ErrorBadParameter;
     }
      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Encoder::GoToLoadedState()
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;

      ///////////////////////////////////////
      // 1. send idle state command and wait for complete
      // 2. send loaded state command
      // 3. free input buffers
      // 4. free output buffers
      // 5. wait for loaded state complete
      ///////////////////////////////////////

      // send idle state comand and wait for complete
      if (m_eState == OMX_StateExecuting ||
          m_eState == OMX_StatePause)
      {
         VENC_TEST_MSG_MEDIUM("going to state OMX_StateIdle...", 0, 0, 0);
         result = SetState(OMX_StateIdle, OMX_TRUE);
      }


      // send loaded state command
      VENC_TEST_MSG_MEDIUM("going to state OMX_StateLoaded...", 0, 0, 0);
      result = SetState(OMX_StateLoaded, OMX_FALSE);

      if (result == OMX_ErrorNone)
      {
         result = FreeBuffers();

         // wait for loaded state complete
         result = WaitState(OMX_StateLoaded);
      }

      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Encoder::DeliverInput(OMX_BUFFERHEADERTYPE* pBuffer)
   {
      VENC_TEST_MSG_MEDIUM("deliver input frame %d", m_nInFrameIn, 0, 0);
      ++m_nInFrameIn;

      return OMX_EmptyThisBuffer(m_hEncoder, pBuffer);
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Encoder::DeliverOutput(OMX_BUFFERHEADERTYPE* pBuffer)
   {
      VENC_TEST_MSG_MEDIUM("deliver output frame %d", m_nOutFrameIn, 0, 0);
      ++m_nOutFrameIn;
      pBuffer->nFlags = 0;
      return OMX_FillThisBuffer(m_hEncoder, pBuffer);
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_BUFFERHEADERTYPE** Encoder::GetBuffers(OMX_BOOL bIsInput)
   {
      OMX_BUFFERHEADERTYPE** ppBuffers;
      if (m_eState == OMX_StateExecuting)
      {
         ppBuffers = (bIsInput == OMX_TRUE) ? m_pInputBuffers : m_pOutputBuffers;
      }
      else
      {
         ppBuffers = NULL;
      }
      return ppBuffers;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Encoder::RequestIntraVOP()
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;
      OMX_CONFIG_INTRAREFRESHVOPTYPE vop;

      vop.nPortIndex = (OMX_U32) PORT_INDEX_OUT; // output
      result = OMX_GetConfig(m_hEncoder,
                             OMX_IndexConfigVideoIntraVOPRefresh,
                             (OMX_PTR) &vop);

      if (result == OMX_ErrorNone)
      {
         vop.IntraRefreshVOP = OMX_TRUE;
         result = OMX_SetConfig(m_hEncoder,
                                OMX_IndexConfigVideoIntraVOPRefresh,
                                (OMX_PTR) &vop);
      }
      else
      {
         VENC_TEST_MSG_ERROR("failed to get state", 0, 0, 0);
      }
      return result;
   }
/////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Encoder::SetIntraRefresh(OMX_S32 nIntraRefresh)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;
      OMX_VIDEO_PARAM_INTRAREFRESHTYPE intrarefresh;

	  intrarefresh.nPortIndex = (OMX_U32) PORT_INDEX_OUT; // output
      result = OMX_GetConfig(m_hEncoder,
                             (OMX_INDEXTYPE) QOMX_IndexConfigVideoIntraRefresh,
                             (OMX_PTR) &intrarefresh);

      if (result == OMX_ErrorNone)
      {
         intrarefresh.eRefreshMode = (OMX_VIDEO_INTRAREFRESHTYPE) QOMX_VIDEO_IntraRefreshRandom;
		 intrarefresh.nAirMBs = nIntraRefresh;
         result = OMX_SetConfig(m_hEncoder,
                                (OMX_INDEXTYPE) QOMX_IndexConfigVideoIntraRefresh,
                                (OMX_PTR) &intrarefresh);
      }
      else
      {
         VENC_TEST_MSG_ERROR("Failed to get IntraRefresh", 0, 0, 0);
      }
      return result;
   }
   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Encoder::SetIntraPeriod(OMX_S32 nIntraPeriod)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;
      QOMX_VIDEO_INTRAPERIODTYPE intra;

      intra.nPortIndex = (OMX_U32) PORT_INDEX_OUT; // output
      result = OMX_GetConfig(m_hEncoder,
                             (OMX_INDEXTYPE) QOMX_IndexConfigVideoIntraperiod,
                             (OMX_PTR) &intra);

      if (result == OMX_ErrorNone)
      {
         intra.nPFrames = (OMX_U32) nIntraPeriod - 1;
         result = OMX_SetConfig(m_hEncoder,
                                (OMX_INDEXTYPE) QOMX_IndexConfigVideoIntraperiod,
                                (OMX_PTR) &intra);
      }
      else
      {
         VENC_TEST_MSG_ERROR("failed to get state", 0, 0, 0);
      }
      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Encoder::ChangeQuality(OMX_S32 nFramerate,
                                        OMX_S32 nBitrate,
                                        OMX_S32 nMinQp,
                                        OMX_S32 nMaxQp,
			                OMX_S32 nIntraPeriod,
					OMX_S32 nIntraRefreshMBs)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;

      (void) nMinQp;
      (void) nMaxQp;

      //////////////////////////////////////////
      // frame rate
      //////////////////////////////////////////
      if (result == OMX_ErrorNone)
      {
         OMX_CONFIG_FRAMERATETYPE framerate; // OMX_IndexConfigVideoFramerate
         framerate.nPortIndex = (OMX_U32) PORT_INDEX_IN; // input
         result = OMX_GetConfig(m_hEncoder,
                                OMX_IndexConfigVideoFramerate,
                                (OMX_PTR) &framerate);
         if (result == OMX_ErrorNone)
         {
            framerate.xEncodeFramerate = ((OMX_U32) nFramerate) << 16;
            result = OMX_SetConfig(m_hEncoder,
                                   OMX_IndexConfigVideoFramerate,
                                   (OMX_PTR) &framerate);
         }
      }

      //////////////////////////////////////////
      // bitrate
      //////////////////////////////////////////
      if (result == OMX_ErrorNone)
      {
         OMX_VIDEO_CONFIG_BITRATETYPE bitrate; // OMX_IndexConfigVideoFramerate
         bitrate.nPortIndex = (OMX_U32) PORT_INDEX_OUT;
         result = OMX_GetConfig(m_hEncoder,
                                OMX_IndexConfigVideoBitrate,
                                (OMX_PTR) &bitrate);
         if (result == OMX_ErrorNone)
         {
            bitrate.nEncodeBitrate = (OMX_U32) nBitrate;
            result = OMX_SetConfig(m_hEncoder,
                                   OMX_IndexConfigVideoBitrate,
                                   (OMX_PTR) &bitrate);
         }
      }
      //////////////////////////////////////////
      // qp
      //////////////////////////////////////////
	   
      int index;
      result = OMX_GetExtensionIndex(m_hEncoder, "OMX.QCOM.index.config.video.TemporalSpatialTradeOff", (OMX_INDEXTYPE *)(&index));

      if (result == OMX_ErrorNone)
      {
		VENC_TEST_MSG_HIGH("The Index = 0x%x after call to OMX_GetExtensionIndex....", index,0,0);
	  }
	  else
      {
		VENC_TEST_MSG_ERROR("Failed to get index for OMX.QCOM.index.config.video.TemporalSpatialTradeOff",0,0,0);
	  }

      if (result == OMX_ErrorNone)
      {
         QOMX_VIDEO_TEMPORALSPATIALTYPE tsTradeoffFactor;

         /* Retrive the current information from the IL component */
         result = OMX_GetConfig(m_hEncoder,
                               (OMX_INDEXTYPE) index, 
                               (OMX_PTR) &tsTradeoffFactor);
         if (result == OMX_ErrorNone)
         {
            /* Populate structure with the specific information */
            tsTradeoffFactor.nTSFactor = (OMX_U32) nMaxQp;
            // Set the index here
            OMX_SetConfig(m_hEncoder,
                         (OMX_INDEXTYPE) index, 
                         (OMX_PTR) &tsTradeoffFactor);
         }
      }
	  if (result == OMX_ErrorNone && (nIntraRefreshMBs > 0))
	  {
		  OMX_VIDEO_PARAM_INTRAREFRESHTYPE IntraRefresh;

		  IntraRefresh.nPortIndex = (OMX_U32) PORT_INDEX_OUT; // output
		  result = OMX_GetConfig(m_hEncoder,
			  (OMX_INDEXTYPE) QOMX_IndexConfigVideoIntraRefresh,
			  (OMX_PTR) &IntraRefresh);

		  if (result == OMX_ErrorNone)
		  {
			  IntraRefresh.eRefreshMode = (OMX_VIDEO_INTRAREFRESHTYPE) QOMX_VIDEO_IntraRefreshRandom;
			  IntraRefresh.nAirMBs = nIntraRefreshMBs;
			  result = OMX_SetConfig(m_hEncoder,
				  (OMX_INDEXTYPE) QOMX_IndexConfigVideoIntraRefresh,
				  (OMX_PTR) &IntraRefresh);
		  }
		  else
		  {
			  VENC_TEST_MSG_ERROR("Failed to get IntraRefresh", 0, 0, 0);
		  }
	  }
         if (result == OMX_ErrorNone && (nIntraPeriod > 0))
	  {
		  QOMX_VIDEO_INTRAPERIODTYPE Intra;

		  Intra.nPortIndex = (OMX_U32) PORT_INDEX_OUT; // output
		  result = OMX_GetConfig(m_hEncoder,
			  (OMX_INDEXTYPE) QOMX_IndexConfigVideoIntraperiod,
			  (OMX_PTR) &Intra);

		  if (result == OMX_ErrorNone)
		  {
			  Intra.nPFrames = (OMX_U32) nIntraPeriod - 1;
			  result = OMX_SetConfig(m_hEncoder,
				  (OMX_INDEXTYPE) QOMX_IndexConfigVideoIntraperiod,
				  (OMX_PTR) &Intra);
		  }
		  else
		  {
			  VENC_TEST_MSG_ERROR("failed to get state", 0, 0, 0);
		  }
	  }
      if (result != OMX_ErrorNone)
      {
         VENC_TEST_MSG_ERROR("error changing quality", 0, 0, 0);
      }
      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Encoder::AllocateBuffers()
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;

      m_pInputBuffers = new OMX_BUFFERHEADERTYPE*[m_nInputBuffers];
      m_pOutputBuffers = new OMX_BUFFERHEADERTYPE*[m_nOutputBuffers];

      int i;

      // allocate input buffers
      for (i = 0; i < m_nInputBuffers; i++)
      {
         VENC_TEST_MSG_MEDIUM("allocating input buffer %d", i, 0, 0);
         result = OMX_AllocateBuffer(m_hEncoder,
                                     &m_pInputBuffers[i],
                                     PORT_INDEX_IN, // port index
                                     this, // pAppPrivate
                                     m_nInputBufferSize);
         if (result != OMX_ErrorNone)
         {
            VENC_TEST_MSG_ERROR("error allocating input buffer", 0, 0, 0);
            break;
         }
      }

      // allocate output buffers
      for (i = 0; i < m_nOutputBuffers; i++)
      {
         VENC_TEST_MSG_MEDIUM("allocating output buffer %d", i, 0, 0);
         result = OMX_AllocateBuffer(m_hEncoder,
                                     &m_pOutputBuffers[i],
                                     PORT_INDEX_OUT, // port index
                                     this, // pAppPrivate
                                     m_nOutputBufferSize);
         if (result != OMX_ErrorNone)
         {
            VENC_TEST_MSG_ERROR("error allocating output buffer", 0, 0, 0);
            break;
         }
      }
      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Encoder::FreeBuffers()
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;

      int i;

      // free input buffers

      if (m_pInputBuffers)
      {
         for (i = 0; i < m_nInputBuffers; i++)
         {
            VENC_TEST_MSG_MEDIUM("freeing input buffer %d", i, 0, 0);

            result = OMX_FreeBuffer(m_hEncoder,
                                    PORT_INDEX_IN, // port index
                                    m_pInputBuffers[i]);
            if (result != OMX_ErrorNone)
            {
               VENC_TEST_MSG_ERROR("error freeing input buffer", 0, 0, 0);
            }
            if (result != OMX_ErrorNone)
            {
               VENC_TEST_MSG_ERROR("error freeing input pmem", 0, 0, 0);
            }
         }

         delete [] m_pInputBuffers;
         m_pInputBuffers = NULL;
      }

      // free output buffers
      if (m_pOutputBuffers)
      {
         for (i = 0; i < m_nOutputBuffers; i++)
         {
            VENC_TEST_MSG_MEDIUM("freeing output buffer %d", i, 0, 0);
            result = OMX_FreeBuffer(m_hEncoder,
                                    PORT_INDEX_OUT, // port index
                                    m_pOutputBuffers[i]);
            if (result != OMX_ErrorNone)
            {
               VENC_TEST_MSG_ERROR("error freeing output buffer", 0, 0, 0);
            }

            if (result != OMX_ErrorNone)
            {
               VENC_TEST_MSG_ERROR("error freeing output pmem", 0, 0, 0);
            }
         }

         delete [] m_pOutputBuffers;
         m_pOutputBuffers = NULL;
      }

      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Encoder::SetState(OMX_STATETYPE eState,
                                   OMX_BOOL bSynchronous)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;

      result = OMX_SendCommand(m_hEncoder,
                               OMX_CommandStateSet,
                               (OMX_U32) eState,
                               NULL);

      if (result == OMX_ErrorNone)
      {
         if (result == OMX_ErrorNone)
         {
            if (bSynchronous == OMX_TRUE)
            {
               result = WaitState(eState);
               if (result != OMX_ErrorNone)
               {
                  VENC_TEST_MSG_ERROR("failed to wait state", 0, 0, 0);
               }
            }
         }
      }
      else
      {
         VENC_TEST_MSG_ERROR("failed to set state", 0, 0, 0);
      }
      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Encoder::WaitState(OMX_STATETYPE eState)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;
      CmdType cmd;

      (void) m_pSignalQueue->Pop(&cmd, sizeof(cmd), 0); // infinite wait
      result = cmd.eResult;

      if (cmd.eEvent == OMX_EventCmdComplete)
      {
         if (cmd.eCmd == OMX_CommandStateSet)
         {
            if ((OMX_STATETYPE) cmd.sCmdData == eState)
            {
               m_eState = (OMX_STATETYPE) cmd.sCmdData;
            }
            else
            {
               VENC_TEST_MSG_ERROR("wrong state (%d)", (int) cmd.sCmdData, 0, 0);
               result = OMX_ErrorUndefined;
            }
         }
         else
         {
            VENC_TEST_MSG_ERROR("expecting state change", 0, 0, 0);
            result = OMX_ErrorUndefined;
         }
      }
      else
      {
         VENC_TEST_MSG_ERROR("expecting cmd complete", 0, 0, 0);
         result = OMX_ErrorUndefined;
      }

      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Encoder::Flush()
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;
      result = OMX_SendCommand(m_hEncoder,
                               OMX_CommandFlush,
                               OMX_ALL,
                               NULL);
      if (result == OMX_ErrorNone)
      {
         CmdType cmd;
         if (m_pSignalQueue->Pop(&cmd, sizeof(cmd), 0) != OMX_ErrorNone)
         {
            VENC_TEST_MSG_ERROR("error popping signal", 0, 0, 0);
         }
         result = cmd.eResult;
         if (cmd.eEvent != OMX_EventCmdComplete || cmd.eCmd != OMX_CommandFlush)
         {
            VENC_TEST_MSG_ERROR("expecting flush", 0, 0, 0);
            result = OMX_ErrorUndefined;
         }

         if (result != OMX_ErrorNone)
         {
            VENC_TEST_MSG_ERROR("failed to wait for flush", 0, 0, 0);
            result = OMX_ErrorUndefined;
         }
         else
         {
            if (m_pSignalQueue->Pop(&cmd, sizeof(cmd), 0) != OMX_ErrorNone)
            {
               VENC_TEST_MSG_ERROR("error popping signal", 0, 0, 0);
            }
            else
            {
               result = cmd.eResult;
               if (cmd.eEvent != OMX_EventCmdComplete || cmd.eCmd != OMX_CommandFlush)
               {
                  VENC_TEST_MSG_ERROR("expecting flush", 0, 0, 0);
                  result = OMX_ErrorUndefined;
               }
            }

            if (result != OMX_ErrorNone)
            {
               VENC_TEST_MSG_ERROR("failed to wait for flush", 0, 0, 0);
               result = OMX_ErrorUndefined;
            }
         }
      }
      else
      {
         VENC_TEST_MSG_ERROR("failed to set state", 0, 0, 0);
      }
      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Encoder::EventCallback(OMX_IN OMX_HANDLETYPE hComponent,
                                        OMX_IN OMX_PTR pAppData,
                                        OMX_IN OMX_EVENTTYPE eEvent,
                                        OMX_IN OMX_U32 nData1,
                                        OMX_IN OMX_U32 nData2,
                                        OMX_IN OMX_PTR pEventData)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;

      if (eEvent == OMX_EventCmdComplete)
      {
         if ((OMX_COMMANDTYPE) nData1 == OMX_CommandStateSet)
         {
            VENC_TEST_MSG_MEDIUM("Event callback with state change", 0, 0, 0);

            switch ((OMX_STATETYPE) nData2)
            {
            case OMX_StateLoaded:
               VENC_TEST_MSG_HIGH("state status OMX_StateLoaded", 0, 0, 0);
               break;
            case OMX_StateIdle:
               VENC_TEST_MSG_HIGH("state status OMX_StateIdle", 0, 0, 0);
               break;
            case OMX_StateExecuting:
               VENC_TEST_MSG_HIGH("state status OMX_StateExecuting", 0, 0, 0);
               break;
            case OMX_StatePause:
               VENC_TEST_MSG_HIGH("state status OMX_StatePause", 0, 0, 0);
               break;
            case OMX_StateInvalid:
               VENC_TEST_MSG_HIGH("state status OMX_StateInvalid", 0, 0, 0);
               break;
            case OMX_StateWaitForResources:
               VENC_TEST_MSG_HIGH("state status OMX_StateWaitForResources", 0, 0, 0);
               break;
            }

            CmdType cmd;
            cmd.eEvent = OMX_EventCmdComplete;
            cmd.eCmd = OMX_CommandStateSet;
            cmd.sCmdData = nData2;
            cmd.eResult = result;
            result = ((Encoder*) pAppData)->m_pSignalQueue->Push(&cmd, sizeof(cmd));
         }
         else if ((OMX_COMMANDTYPE) nData1 == OMX_CommandFlush)
         {
            VENC_TEST_MSG_MEDIUM("Event callback with flush status", 0, 0, 0);
            VENC_TEST_MSG_HIGH("flush status", 0, 0, 0);

            CmdType cmd;
            cmd.eEvent = OMX_EventCmdComplete;
            cmd.eCmd = OMX_CommandFlush;
            cmd.sCmdData = 0;
            cmd.eResult = result;
            result = ((Encoder*) pAppData)->m_pSignalQueue->Push(&cmd, sizeof(cmd));
         }
         else
         {
            VENC_TEST_MSG_HIGH("error status", 0, 0, 0);
            VENC_TEST_MSG_ERROR("Unimplemented command", 0, 0, 0);
         }
      }
      else if (eEvent == OMX_EventError)
      {
         VENC_TEST_MSG_ERROR("async error", 0, 0, 0);
         CmdType cmd;
         cmd.eEvent = OMX_EventError;
         cmd.eCmd = OMX_CommandMax;
         cmd.sCmdData = 0;
         cmd.eResult = (OMX_ERRORTYPE) nData1;
         result = ((Encoder*) pAppData)->m_pSignalQueue->Push(&cmd, sizeof(cmd));
      }
      else if (eEvent == OMX_EventBufferFlag)
      {
         VENC_TEST_MSG_MEDIUM("got buffer flag event", 0, 0, 0);
      }
      else
      {
         VENC_TEST_MSG_ERROR("Unimplemented event", 0, 0, 0);
      }

      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Encoder::EmptyDoneCallback(OMX_IN OMX_HANDLETYPE hComponent,
                                            OMX_IN OMX_PTR pAppData,
                                            OMX_IN OMX_BUFFERHEADERTYPE* pBuffer)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;
      VENC_TEST_MSG_MEDIUM("releasing input frame %d", ((Encoder*) pAppData)->m_nInFrameOut, 0, 0);
      if (((Encoder*) pAppData)->m_pEmptyDoneFn)
      {
         ((Encoder*) pAppData)->m_pEmptyDoneFn(hComponent,
                                               ((Encoder*) pAppData)->m_pAppData, // forward the client from constructor
                                               pBuffer);
      }
      ++((Encoder*) pAppData)->m_nInFrameOut;
      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Encoder::FillDoneCallback(OMX_IN OMX_HANDLETYPE hComponent,
                                           OMX_IN OMX_PTR pAppData,
                                           OMX_IN OMX_BUFFERHEADERTYPE* pBuffer)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;
      VENC_TEST_MSG_MEDIUM("releasing output frame %d", ((Encoder*) pAppData)->m_nOutFrameOut, 0, 0);

      if (((Encoder*) pAppData)->m_pFillDoneFn)
      {
         ((Encoder*) pAppData)->m_pFillDoneFn(hComponent,
                                              ((Encoder*) pAppData)->m_pAppData, // forward the client from constructor
                                              pBuffer);
      }
      ++((Encoder*) pAppData)->m_nOutFrameOut;
      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   
   OMX_ERRORTYPE Encoder::SetEncoderMode()
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;
      QOMX_VIDEO_PARAM_ENCODERMODETYPE encoderModeType;

      VENC_TEST_MSG_LOW("SetEncoderMode()", 0, 0, 0);
      result = OMX_GetParameter(m_hEncoder,
                             (OMX_INDEXTYPE) QOMX_IndexParamVideoEncoderMode,
                             (OMX_PTR) &encoderModeType);

      VENC_TEST_MSG_LOW("SetEncoderMode() - GetParameter encoderModeType.nMode = 0x%x", encoderModeType.nMode, 0, 0);

      if (result == OMX_ErrorNone && encoderModeType.nMode != QOMX_VIDEO_EncoderModeMMS)
      {
         encoderModeType.nMode = QOMX_VIDEO_EncoderModeMMS;
         result = OMX_SetParameter(m_hEncoder,
                                (OMX_INDEXTYPE) QOMX_IndexParamVideoEncoderMode,
                                (OMX_PTR) &encoderModeType);
         if (result == OMX_ErrorNone)
         {
            VENC_TEST_MSG_LOW("encoderModeType.nMode set to 0x%x", encoderModeType.nMode, 0, 0);
         }
         else
         {
            VENC_TEST_MSG_ERROR("failed to set EncoderModeType", 0, 0, 0);
         }
      }
      else
      {
         VENC_TEST_MSG_ERROR("failed to get EncoderModeType", 0, 0, 0);
      }
      return result;
   }
} // namespace venctest

/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/src/venctest_TestEncode.cpp#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2009-08-13   bc     Fixed bug from uninitialized variable
2009-06-30   bc     Added support for DVS
2009-06-23   bc     More pedantic with flag checking
2009-05-06   bc     Initial revision

========================================================================== */

/*========================================================================
  Include Files
 ==========================================================================*/
#include "venctest_ComDef.h"
#include "venctest_Debug.h"
#include "venctest_TestEncode.h"
#include "venctest_Time.h"
#include "venctest_FileSource.h"
#include "venctest_FileSink.h"
#include "venctest_Encoder.h"

namespace venctest
{

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   TestEncode::TestEncode()
      : ITestCase(),    // invoke the base class constructor
        m_pSource(NULL),
        m_pSink(NULL),
        m_pEncoder(NULL),
        m_nFramesCoded(0),
        m_nBits(0)
   {
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   TestEncode::~TestEncode()
   {
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE TestEncode::ValidateAssumptions(EncoderConfigType* pConfig,
                                                        DynamicConfigType* pDynamicConfig)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;
      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE TestEncode::Execute(EncoderConfigType* pConfig,
                                     DynamicConfigType* pDynamicConfig,
                                     OMX_S32 nTestNum)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;

      OMX_TICKS nStartTime = 0;
      OMX_TICKS nEndTime;
      OMX_TICKS nRunTimeSec;
      OMX_TICKS nRunTimeMillis;

      //==========================================
      // Create and configure the file source (yuv reader)
      if (result == OMX_ErrorNone)
      {
         VENC_TEST_MSG_HIGH("Creating source...", 0, 0, 0);
         m_pSource = new FileSource(m_pSemaphore, pConfig->bProfileMode, m_bIsVTPath);
         result = CheckError(m_pSource->Configure(
            pConfig->nFrames, pConfig->nFramerate, pConfig->nFrameWidth,
            pConfig->nFrameHeight, pConfig->nInBufferCount, SourceDeliveryFn,
            pConfig->cInFileName, pConfig->nDVSXOffset, pConfig->nDVSYOffset,
            pConfig->bProfileMode));
      }

      //==========================================
      // Create and configure the file sink (m4v writer)
      if (result == OMX_ErrorNone)
      {
         VENC_TEST_MSG_HIGH("Creating sink...", 0, 0, 0);
         m_pSink = new FileSink(m_pSemaphore, pConfig->bProfileMode, m_bIsVTPath);
         result = CheckError(m_pSink->Configure(
            pConfig->nFrames, pConfig->cOutFileName, nTestNum, SinkReleaseFn));
      }

      //==========================================
      // Create and configure the encoder
      if (result == OMX_ErrorNone)
      {
         VENC_TEST_MSG_HIGH("Creating encoder...", 0, 0, 0);
         m_pEncoder = new Encoder(EBD,
                                  FBD,
                                  this, // set the test case object as the callback app data
                                  pConfig->eCodec);
         result = CheckError(m_pEncoder->Configure(pConfig));
      }

      //==========================================
      // Go to executing state (also allocate buffers)
      if (result == OMX_ErrorNone)
      {
         VENC_TEST_MSG_HIGH("Go to executing state...", 0, 0, 0);
         result = CheckError(m_pEncoder->GoToExecutingState());
      }

      //==========================================
      // Get the allocated input buffers
      if (result == OMX_ErrorNone)
      {
         OMX_BUFFERHEADERTYPE** ppInputBuffers;
         ppInputBuffers = m_pEncoder->GetBuffers(OMX_TRUE);
         for (int i = 0; i < pConfig->nInBufferCount; i++)
         {
            ppInputBuffers[i]->pAppPrivate = m_pEncoder; // set the encoder as the private app data
            result = CheckError(m_pSource->SetFreeBuffer(ppInputBuffers[i])); // give ownership to source
            if (result != OMX_ErrorNone)
            {
               break;
            }
         }
      }

      //==========================================
      // Get the allocated output buffers
      if (result == OMX_ErrorNone)
      {
         OMX_BUFFERHEADERTYPE** ppOutputBuffers;
         ppOutputBuffers = m_pEncoder->GetBuffers(OMX_FALSE);
         for (int i = 0; i < pConfig->nOutBufferCount; i++)
         {
            ppOutputBuffers[i]->pAppPrivate = m_pEncoder; // set the encoder as the private app data
            result = CheckError(m_pEncoder->DeliverOutput(
               ppOutputBuffers[i])); // give ownership to encoder
            if (result != OMX_ErrorNone)
            {
               break;
            }
         }
      }

      //==========================================
      // Get the sink ready to write m4v output
      if (result == OMX_ErrorNone)
      {
         VENC_TEST_MSG_HIGH("starting the sink thread...", 0, 0, 0);
         nStartTime = Time::GetTimeMicrosec();
         result = CheckError(m_pSink->Start());
      }

      //==========================================
      // Start reading and delivering frames
      if (result == OMX_ErrorNone)
      {
         VENC_TEST_MSG_HIGH("starting the source thread...", 0, 0, 0);
         result = CheckError(m_pSource->Start());
      }

      //==========================================
      // Wait for the source to finish delivering all frames
      if (m_pSource != NULL)
      {
         VENC_TEST_MSG_HIGH("waiting for source to finish...", 0, 0, 0);
         result = CheckError(m_pSource->Finish());
         VENC_TEST_MSG_HIGH("source is finished", 0, 0, 0);
      }

      //==========================================
      // Wait for the sink to finish writing all frames
      if (m_pSink != NULL)
      {
         VENC_TEST_MSG_HIGH("waiting for sink to finish...", 0, 0, 0);
         result = CheckError(m_pSink->Finish());
         VENC_TEST_MSG_HIGH("sink is finished", 0, 0, 0);
      }

      //==========================================
      // Tear down the encoder (also deallocate buffers)
      if (m_pEncoder != NULL)
      {
         VENC_TEST_MSG_HIGH("Go to loaded state...", 0, 0, 0);
         result = CheckError(m_pEncoder->GoToLoadedState());
      }

      //==========================================
      // Compute stats
      nEndTime = Time::GetTimeMicrosec();
      nRunTimeMillis = (nEndTime - nStartTime) / 1000;   // convert to millis
      nRunTimeSec = nRunTimeMillis / 1000;               // convert to seconds

      VENC_TEST_MSG_PROFILE("Time = %d millis, Encoded = %d, Dropped = %d",
         (int) nRunTimeMillis,
         (int) m_nFramesCoded,
         (int) (pConfig->nFrames - m_nFramesCoded));

      if (nRunTimeSec > 0) // ensure no divide by zero
      {
         VENC_TEST_MSG_PROFILE("Bitrate = %d, InputFPS = %d, OutputFPS = %d",
            (int) (m_nBits / nRunTimeSec),
            (int) (pConfig->nFrames / nRunTimeSec),
            (int) (m_nFramesCoded / nRunTimeSec));
      }
      else
      {
         VENC_TEST_MSG_PROFILE("Bitrate = %d, InputFPS = %d, OutputFPS = %d",
            0, 0, 0);
      }

      VENC_TEST_MSG_PROFILE("Avg encode time = %d millis per frame",
         (int) (nRunTimeMillis / pConfig->nFrames), 0, 0);

      // determine the test result
      if (result == OMX_ErrorNone)
      {
         if (pConfig->eControlRate != OMX_Video_ControlRateDisable &&
             pConfig->bProfileMode == OMX_TRUE)
         {
            static const double errorThreshold = .15; // error percentage threshold

            OMX_S32 nBitrateDelta = (OMX_S32) (pConfig->nBitrate - (m_nBits / nRunTimeSec));
            if (nBitrateDelta < 0)
            {
               nBitrateDelta = -nBitrateDelta;
            }
            if ((double) nBitrateDelta > pConfig->nBitrate * errorThreshold)
            {
               VENC_TEST_MSG_ERROR("test failed with bitrate %d. bitrate delta is %d max allowed is approx %d",
                  (int) pConfig->nBitrate,
                  (int) nBitrateDelta,
                  (int) (pConfig->nBitrate * errorThreshold));
               result = CheckError(OMX_ErrorUndefined);
            }

            OMX_S32 nFramerateDelta = (OMX_S32) (pConfig->nFramerate - (pConfig->nFrames / nRunTimeSec));
            if (nFramerateDelta < 0)
            {
               nFramerateDelta = -nFramerateDelta;
            }
            if ((double) nFramerateDelta > pConfig->nFramerate * errorThreshold)
            {
               VENC_TEST_MSG_ERROR("test failed with frame rate %d. frame rate delta is %d max allowed is approx %d",
                  (int) pConfig->nFramerate,
                  (int) nFramerateDelta,
                  (int) ((pConfig->nFrames / nRunTimeSec) * errorThreshold));
               result = CheckError(OMX_ErrorUndefined);
            }
         }
      }

      //==========================================
      // Free our helper classes
      if (m_pSource)
         delete m_pSource;
      if (m_pSink)
         delete m_pSink;
      if (m_pEncoder)
         delete m_pEncoder;

      return result;

   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   void TestEncode::SourceDeliveryFn(OMX_BUFFERHEADERTYPE* pBuffer)
   {
      // Deliver YUV data from source to encoder
      ((Encoder*) pBuffer->pAppPrivate)->DeliverInput(pBuffer);
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   void TestEncode::SinkReleaseFn(OMX_BUFFERHEADERTYPE* pBuffer)
   {
      // Deliver bitstream buffer from sink to encoder
      ((Encoder*) pBuffer->pAppPrivate)->DeliverOutput(pBuffer);
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE TestEncode::EBD(OMX_IN OMX_HANDLETYPE hComponent,
                                        OMX_IN OMX_PTR pAppData,
                                        OMX_IN OMX_BUFFERHEADERTYPE* pBuffer)
   {
      TestEncode* pTester = (TestEncode*) pAppData;

      // Deliver free yuv buffer to source
      return pTester->m_pSource->SetFreeBuffer(pBuffer);
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE TestEncode::FBD(OMX_IN OMX_HANDLETYPE hComponent,
                                        OMX_IN OMX_PTR pAppData,
                                        OMX_IN OMX_BUFFERHEADERTYPE* pBuffer)
   {

      TestEncode* pTester = (TestEncode*) pAppData;

      // get performance data
      if (pBuffer->nFilledLen != 0)
      {
         // if it's only the syntax header don't count it as a frame
         if ((pBuffer->nFlags & OMX_BUFFERFLAG_CODECCONFIG) == 0 &&
             (pBuffer->nFlags & OMX_BUFFERFLAG_ENDOFFRAME))
         {
            ++pTester->m_nFramesCoded;
         }

         // always count the bits regarding whether or not its only syntax header
         pTester->m_nBits = pTester->m_nBits + (OMX_S32) (pBuffer->nFilledLen * 8);
      }

      // Deliver encoded m4v output to sink for file write
      return pTester->m_pSink->Write(pBuffer);
   }

} // namespace venctest

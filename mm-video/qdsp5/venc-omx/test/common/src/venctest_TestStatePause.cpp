/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/src/venctest_TestStatePause.cpp#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2009-08-24   bc     On-target integration updates
2009-05-06   bc     Initial revision

========================================================================== */

/*========================================================================
  Include Files
 ==========================================================================*/
#include "venctest_ComDef.h"
#include "venctest_Debug.h"
#include "venctest_TestStatePause.h"
#include "venctest_Time.h"
#include "venctest_Encoder.h"
#include "venctest_Queue.h"
#include "venctest_Mutex.h"

namespace venctest
{
   static const OMX_S32 PORT_INDEX_IN = 0;
   static const OMX_S32 PORT_INDEX_OUT = 1;

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   TestStatePause::TestStatePause()
      : ITestCase(),    // invoke the base class constructor
        m_pEncoder(NULL),
        m_pMutex(NULL),
        m_pInputQueue(NULL),
        m_pOutputQueue(NULL),
        m_nTimeStamp(0)
   {
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   TestStatePause::~TestStatePause()
   {
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE TestStatePause::ValidateAssumptions(EncoderConfigType* pConfig,
                                                     DynamicConfigType* pDynamicConfig)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;
      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE TestStatePause::Execute(EncoderConfigType* pConfig,
                                         DynamicConfigType* pDynamicConfig,
                                         OMX_S32 nTestNum)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;

      if (result == OMX_ErrorNone)
      {
         //==========================================
         // Create input buffer queue
         VENC_TEST_MSG_HIGH("Creating input buffer queue...", 0, 0, 0);
         m_pInputQueue = new Queue(pConfig->nInBufferCount,
                                   sizeof(OMX_BUFFERHEADERTYPE*));

         //==========================================
         // Create output buffer queue
         VENC_TEST_MSG_HIGH("Creating output buffer queue...", 0, 0, 0);
         m_pOutputQueue = new Queue(pConfig->nOutBufferCount,
                                    sizeof(OMX_BUFFERHEADERTYPE*));
      }

      if (result == OMX_ErrorNone)
      {
         //==========================================
         // Create input buffer queue
         VENC_TEST_MSG_HIGH("Creating input buffer queue...", 0, 0, 0);
         m_pMutex = new Mutex();
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
            result = CheckError(m_pInputQueue->Push(
               &ppInputBuffers[i], sizeof(ppInputBuffers[i]))); // store buffers in queue
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
            result = CheckError(m_pOutputQueue->Push(
               &ppOutputBuffers[i], sizeof(ppOutputBuffers[i]))); // store buffers in queue
            if (result != OMX_ErrorNone)
            {
               break;
            }
         }
      }

      //==========================================
      // Deliver buffers and go to pause state
      static const OMX_S32 nRuns = 4;
      static const OMX_BOOL bPortMap[nRuns][2] =
         { {OMX_FALSE,  OMX_FALSE},    // no input or output buffers
           {OMX_TRUE,   OMX_FALSE},    // input buffers only
           {OMX_FALSE,  OMX_TRUE},     // output buffers only
           {OMX_TRUE,   OMX_TRUE} };   // input and output buffers
      for (OMX_S32 i = 0; i < nRuns; i++)
      {
         VENC_TEST_MSG_HIGH("Deliver buffers %d", (int) i, 0, 0);
         result = CheckError(DeliverBuffers(
            bPortMap[i][0], bPortMap[i][1], pConfig));
         if (result != OMX_ErrorNone)
         {
            VENC_TEST_MSG_ERROR("failed to deliver buffers %d", (int) i, 0, 0);
            break;
         }

         VENC_TEST_MSG_HIGH("Enter pause %d", (int) i, 0, 0);
         result = CheckError(m_pEncoder->SetState(OMX_StatePause, OMX_TRUE));
         if (result != OMX_ErrorNone)
         {
            VENC_TEST_MSG_ERROR("failed enter pause state %d", (int) i, 0, 0);
            break;
         }

         VENC_TEST_MSG_HIGH("Check queues %d: %d %d",
            (int) i, m_pInputQueue->GetSize(), m_pOutputQueue->GetSize());

         if (result != OMX_ErrorNone)
         {
            VENC_TEST_MSG_ERROR("missing some buffers %d", (int) i, 0, 0);
            break;
         }

         VENC_TEST_MSG_HIGH("Enter executing %d", (int) i, 0, 0);
         result = CheckError(m_pEncoder->SetState(OMX_StateExecuting, OMX_TRUE));
         if (result != OMX_ErrorNone)
         {
            VENC_TEST_MSG_ERROR("failed enter executing state %d", (int) i, 0, 0);
            break;
         }
      }

      //==========================================
      // go back to pause state
      if (result == OMX_ErrorNone)
      {
         VENC_TEST_MSG_HIGH("Enter pause", 0, 0, 0);
         result = CheckError(m_pEncoder->SetState(OMX_StatePause, OMX_TRUE));
         if (result != OMX_ErrorNone)
         {
            VENC_TEST_MSG_ERROR("failed enter pause state", 0, 0, 0);
         }
      }

      //==========================================
      // verify we can transition from pause to idle
      if (result == OMX_ErrorNone)
      {
         VENC_TEST_MSG_HIGH("Enter idle", 0, 0, 0);
         result = CheckError(m_pEncoder->SetState(OMX_StateIdle, OMX_TRUE));
         if (result != OMX_ErrorNone)
         {
            VENC_TEST_MSG_ERROR("failed enter idle state", 0, 0, 0);
         }
      }

      //==========================================
      // ensure encoder has returned all buffers before finalizing idle transition
      if (result == OMX_ErrorNone)
      {
         OMX_S32 in = m_pInputQueue->GetSize();
         OMX_S32 out = m_pOutputQueue->GetSize();

         if (in != pConfig->nInBufferCount ||
             out != pConfig->nOutBufferCount)
         {
            VENC_TEST_MSG_ERROR("encoder did not return all buffers %d %d", in, out, 0);
            result = CheckError(OMX_ErrorUndefined);
         }
      }

      //==========================================
      // go back to executing state
      if (result == OMX_ErrorNone)
      {
         VENC_TEST_MSG_HIGH("Enter executing", 0, 0, 0);
         result = CheckError(m_pEncoder->SetState(OMX_StateExecuting, OMX_TRUE));
         if (result != OMX_ErrorNone)
         {
            VENC_TEST_MSG_ERROR("failed enter executing state", 0, 0, 0);
         }
      }

      //==========================================
      // go to pause state
      if (result == OMX_ErrorNone)
      {
         VENC_TEST_MSG_HIGH("Enter pause", 0, 0, 0);
         result = CheckError(m_pEncoder->SetState(OMX_StatePause, OMX_TRUE));
         if (result != OMX_ErrorNone)
         {
            VENC_TEST_MSG_ERROR("failed enter pause state", 0, 0, 0);
         }
      }

      //==========================================
      // verify we can deliver buffers in the pause state
      if (result == OMX_ErrorNone)
      {
         VENC_TEST_MSG_HIGH("Deliver buffers", 0, 0, 0);
         result = CheckError(DeliverBuffers(
            OMX_TRUE, OMX_TRUE, pConfig));
         if (result != OMX_ErrorNone)
         {
            VENC_TEST_MSG_ERROR("failed to deliver buffers in pause state", 0, 0, 0);
         }
      }
      //==========================================
      // go to idle state
      if (m_pEncoder != NULL)
      {
         VENC_TEST_MSG_HIGH("Enter idle", 0, 0, 0);
         result = CheckError(m_pEncoder->SetState(OMX_StateIdle, OMX_TRUE));
         if (result != OMX_ErrorNone)
         {
            VENC_TEST_MSG_ERROR("failed enter idle state", 0, 0, 0);
         }
      }

      //==========================================
      // ensure encoder has returned all buffers before finalizing idle transition
      if (result == OMX_ErrorNone)
      {
         if (m_pInputQueue->GetSize() != pConfig->nInBufferCount ||
             m_pOutputQueue->GetSize() != pConfig->nOutBufferCount)
         {
            VENC_TEST_MSG_ERROR("encoder did not return all buffers", 0, 0, 0);
            result = CheckError(OMX_ErrorUndefined);
         }
      }

      //==========================================
      // go to loaded state
      if (m_pEncoder != NULL)
      {
         VENC_TEST_MSG_HIGH("Enter loaded", 0, 0, 0);
         result = CheckError(m_pEncoder->SetState(OMX_StateLoaded, OMX_FALSE));
         if (result != OMX_ErrorNone)
         {
            VENC_TEST_MSG_ERROR("failed enter loaded state", 0, 0, 0);
         }

         result = CheckError(m_pEncoder->FreeBuffers());
         if (result != OMX_ErrorNone)
         {
            VENC_TEST_MSG_ERROR("failed to free buffers", 0, 0, 0);
         }

         result = CheckError(m_pEncoder->WaitState(OMX_StateLoaded));
         if (result != OMX_ErrorNone)
         {
            VENC_TEST_MSG_ERROR("failed to wait for loaded", 0, 0, 0);
         }
      }

      //==========================================
      // Free our helper classes
      if (m_pEncoder)
         delete m_pEncoder;
      if (m_pInputQueue)
         delete m_pInputQueue;
      if (m_pOutputQueue)
         delete m_pOutputQueue;
      if (m_pMutex)
         delete m_pMutex;

      return result;

   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE TestStatePause::DeliverBuffers(OMX_BOOL bDeliverInput,
                                                OMX_BOOL bDeliverOutput,
                                                EncoderConfigType* pConfig)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;

      // deliver all input buffers
      if (bDeliverInput == OMX_TRUE)
      {
         OMX_BUFFERHEADERTYPE* pBuffer;
         OMX_S32 nSize = m_pInputQueue->GetSize();
         for (OMX_S32 i = 0; i < nSize; i++)
         {
            m_pMutex->Lock();
            result = CheckError(m_pInputQueue->Pop(
               (OMX_PTR) &pBuffer, sizeof(pBuffer)));
            m_pMutex->UnLock();
            if (result != OMX_ErrorNone)
            {
               VENC_TEST_MSG_ERROR("failed to pop input", 0, 0, 0);
               break;
            }

            pBuffer->nTimeStamp = m_nTimeStamp;
            m_nTimeStamp = m_nTimeStamp + 1000000 / pConfig->nFramerate;
            pBuffer->nFilledLen = pConfig->nFrameWidth * pConfig->nFrameHeight * 3 / 2;
            result = CheckError(m_pEncoder->DeliverInput(pBuffer));
            if (result != OMX_ErrorNone)
            {
               VENC_TEST_MSG_ERROR("failed to deliver input", 0, 0, 0);
               break;
            }
         }
      }

      // deliver all output buffers
      if (bDeliverOutput == OMX_TRUE)
      {
         OMX_BUFFERHEADERTYPE* pBuffer;
         OMX_S32 nSize = m_pOutputQueue->GetSize();
         for (OMX_S32 i = 0; i < nSize; i++)
         {
            m_pMutex->Lock();
            result = CheckError(m_pOutputQueue->Pop(
               (OMX_PTR) &pBuffer, sizeof(pBuffer)));
            m_pMutex->UnLock();
            if (result != OMX_ErrorNone)
            {
               VENC_TEST_MSG_ERROR("failed to pop output", 0, 0, 0);
               break;
            }

            result = CheckError(m_pEncoder->DeliverOutput(pBuffer));
            if (result != OMX_ErrorNone)
            {
               VENC_TEST_MSG_ERROR("failed to deliver output", 0, 0, 0);
               break;
            }
         }
      }

      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE TestStatePause::EBD(OMX_IN OMX_HANDLETYPE hComponent,
                                     OMX_IN OMX_PTR pAppData,
                                     OMX_IN OMX_BUFFERHEADERTYPE* pBuffer)
   {
      VENC_TEST_MSG_HIGH("queuing input buffer with size %d", pBuffer->nFilledLen, 0, 0);
      ((TestStatePause*) pAppData)->m_pMutex->Lock();
      OMX_ERRORTYPE result = ((TestStatePause*) pAppData)->m_pInputQueue->Push(&pBuffer, sizeof(pBuffer));
      ((TestStatePause*) pAppData)->m_pMutex->UnLock();
      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE TestStatePause::FBD(OMX_IN OMX_HANDLETYPE hComponent,
                                     OMX_IN OMX_PTR pAppData,
                                     OMX_IN OMX_BUFFERHEADERTYPE* pBuffer)
   {
      VENC_TEST_MSG_HIGH("queuing output buffer with size %d", pBuffer->nFilledLen, 0, 0);
      ((TestStatePause*) pAppData)->m_pMutex->Lock();
      OMX_ERRORTYPE result = ((TestStatePause*) pAppData)->m_pOutputQueue->Push(&pBuffer, sizeof(pBuffer));
      ((TestStatePause*) pAppData)->m_pMutex->UnLock();
      return result;
   }

} // namespace venctest

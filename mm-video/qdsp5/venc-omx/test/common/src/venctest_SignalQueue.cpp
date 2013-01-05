/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/src/venctest_SignalQueue.cpp#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2009-05-06   bc     Initial revision

========================================================================== */

/*========================================================================
  Include Files
 ==========================================================================*/
#include "venctest_Debug.h"
#include "venctest_Mutex.h"
#include "venctest_Queue.h"
#include "venctest_Signal.h"
#include "venctest_SignalQueue.h"

namespace venctest
{

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   SignalQueue::SignalQueue()
   {
      VENC_TEST_MSG_ERROR("default constructor should not be here (private)", 0, 0, 0);
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   SignalQueue::SignalQueue(OMX_S32 nMaxQueueSize,
                            OMX_S32 nMaxDataSize)
      :  m_pSignal(new Signal()),
         m_pMutex(new Mutex()),
         m_pQueue(new Queue(nMaxQueueSize, nMaxDataSize))
   {
      VENC_TEST_MSG_LOW("constructor %d %d", nMaxQueueSize, nMaxDataSize, 0);
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   SignalQueue::~SignalQueue()
   {
      VENC_TEST_MSG_LOW("destructor", 0, 0, 0);
      if (m_pMutex != NULL)
         delete m_pMutex;
      if (m_pSignal != NULL)
         delete m_pSignal;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE SignalQueue::Pop(OMX_PTR pData,
                                  OMX_S32 nDataSize,
                                  OMX_S32 nTimeoutMillis)
   {
      VENC_TEST_MSG_LOW("Pop", 0, 0, 0);
      OMX_ERRORTYPE result = OMX_ErrorNone;

      // wait for signal or for data to come into queue
      while (GetSize() == 0 && result == OMX_ErrorNone)
      {
         result = m_pSignal->Wait(nTimeoutMillis);
      }

      // did we timeout?
      if (result == OMX_ErrorNone)
      {
         // lock mutex
         m_pMutex->Lock();

         result = m_pQueue->Pop(pData, nDataSize);

         // unlock mutex
         m_pMutex->UnLock();
      }
      else if (result != OMX_ErrorTimeout)
      {
         VENC_TEST_MSG_ERROR("Error waiting for signal", 0, 0, 0);
         result = OMX_ErrorUndefined;
      }

      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE SignalQueue::Push(OMX_PTR pData,
                                   OMX_S32 nDataSize)
   {
      VENC_TEST_MSG_LOW("Push", 0, 0, 0);
      OMX_ERRORTYPE result = OMX_ErrorNone;

      // lock mutex
      m_pMutex->Lock();

      result = m_pQueue->Push(pData, nDataSize);

      // unlock mutex
      m_pMutex->UnLock();


      if (result == OMX_ErrorNone)
      {
         m_pSignal->Set();
      }

      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE SignalQueue::Peek(OMX_PTR pData,
                                   OMX_S32 nDataSize)
   {
      VENC_TEST_MSG_LOW("Peek", 0, 0, 0);
      OMX_ERRORTYPE result = OMX_ErrorNone;

      // lock mutex
      m_pMutex->Lock();

      result = m_pQueue->Peek(pData, nDataSize);

      // unlock mutex
      m_pMutex->UnLock();

      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_S32 SignalQueue::GetSize()
   {
      return m_pQueue->GetSize();
   }

} // namespace venctest

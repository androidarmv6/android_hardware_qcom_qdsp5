/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/src/venctest_Queue.cpp#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2009-08-24   bc     On-target integration updates
2009-05-06   bc     Initial revision

========================================================================== */

/*========================================================================
  Include Files
 ==========================================================================*/
#include "venctest_Debug.h"
#include "venctest_Queue.h"
#include "venc_queue.h"

namespace venctest
{

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   Queue::Queue()
   {
      VENC_TEST_MSG_ERROR("default constructor should not be here (private)", 0, 0, 0);
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   Queue::Queue(OMX_S32 nMaxQueueSize,
                OMX_S32 nMaxDataSize)
      :  m_pHandle(NULL)
   {
      VENC_TEST_MSG_LOW("constructor %d %d", nMaxQueueSize, nMaxDataSize, 0);
      if (venc_queue_create((void**) &m_pHandle, (int) nMaxQueueSize, (int) nMaxDataSize) != 0)
      {
         VENC_TEST_MSG_ERROR("failed to create queue", 0, 0, 0);
      }
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   Queue::~Queue()
   {
      VENC_TEST_MSG_LOW("destructor", 0, 0, 0);
      if (venc_queue_destroy((void*) m_pHandle) != 0)
      {
         VENC_TEST_MSG_ERROR("failed to create queue", 0, 0, 0);
      }
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Queue::Pop(OMX_PTR pData,
                            OMX_S32 nDataSize)
   {
      VENC_TEST_MSG_LOW("Pop", 0, 0, 0);

      OMX_ERRORTYPE result = Peek(pData, nDataSize);

      if (result == OMX_ErrorNone)
      {
         if (venc_queue_pop((void*) m_pHandle, pData, (int) nDataSize) != 0)
         {
            VENC_TEST_MSG_ERROR("failed to pop queue", 0, 0, 0);
            result = OMX_ErrorUndefined;
         }
      }

      return result;

   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Queue::Push(OMX_PTR pData,
                             OMX_S32 nDataSize)
   {
      VENC_TEST_MSG_LOW("Push", 0, 0, 0);
      OMX_ERRORTYPE result = OMX_ErrorNone;

      if (venc_queue_push((void*) m_pHandle, (void*) pData, (int) nDataSize) != 0)
      {
         VENC_TEST_MSG_ERROR("failed to push onto queue", 0, 0, 0);
         result = OMX_ErrorUndefined;
      }

      return result;
   }

   OMX_ERRORTYPE Queue::Peek(OMX_PTR pData,
                             OMX_S32 nDataSize)
   {
      VENC_TEST_MSG_LOW("Pop", 0, 0, 0);
      OMX_ERRORTYPE result = OMX_ErrorNone;

      if (venc_queue_peek((void*) m_pHandle, (void*) pData, (int) nDataSize) != 0)
      {
         VENC_TEST_MSG_ERROR("failed to peek into queue", 0, 0, 0);
         result = OMX_ErrorUndefined;
      }

      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_S32 Queue::GetSize()
   {
      return (OMX_S32) venc_queue_size((void*) m_pHandle);
   }

} // namespace venctest

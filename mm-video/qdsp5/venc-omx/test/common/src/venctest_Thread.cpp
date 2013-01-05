/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/src/venctest_Thread.cpp#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2009-08-13   bc     Removed OS dependency from test framework
2009-05-06   bc     Initial revision

========================================================================== */

/*========================================================================
  Include Files
 ==========================================================================*/
#include "venctest_Debug.h"
#include "venctest_Thread.h"
#include "venc_thread.h"

namespace venctest
{

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   Thread::Thread()
      : m_pFn(NULL),
        m_nPriority(0),
        m_pThread(NULL),
        m_pThreadArgs(NULL)
   {
      VENC_TEST_MSG_LOW("constructor", 0, 0, 0);
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   Thread::~Thread()
   {
      VENC_TEST_MSG_LOW("destructor", 0, 0, 0);
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Thread::Start(StartFnType pFn,
                               OMX_PTR pThreadArgs,
                               OMX_S32 nPriority)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;

      VENC_TEST_MSG_LOW("Start", 0, 0, 0);
      m_pThreadArgs = pThreadArgs;
      m_pFn = pFn;
      
      if (venc_thread_create(&m_pThread, ThreadEntry, this, (int) nPriority) != 0)
      {
         VENC_TEST_MSG_ERROR("failed to create thread", 0, 0, 0);
         result = OMX_ErrorUndefined;
      }

      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Thread::Join(OMX_ERRORTYPE* pThreadResult)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;
      int thread_result;

      VENC_TEST_MSG_LOW("Join", 0, 0, 0);
      
      if (venc_thread_destroy(m_pThread, &thread_result) != 0)
      {
         VENC_TEST_MSG_ERROR("failed to destroy thread", 0, 0, 0);
      }

      if (pThreadResult != NULL)
      {
         *pThreadResult = (OMX_ERRORTYPE) thread_result;
      }

      m_pThread = NULL;

      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   int Thread::ThreadEntry(void* pThreadData)
   {
      Thread* pThread = (Thread*) pThreadData;
      VENC_TEST_MSG_LOW("ThreadEntry", 0, 0, 0);
      OMX_ERRORTYPE result = OMX_ErrorNone;

      if (pThread != NULL)
      {
         result = pThread->m_pFn(pThread->m_pThreadArgs);
      }
      else
      {
         VENC_TEST_MSG_ERROR("failed to create thread", 0, 0, 0);
         result = OMX_ErrorUndefined;
      }


      return (int) result;
   }
} // namespace venctest

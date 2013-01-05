/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/src/venctest_ITestCase.cpp#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2010-06-12   as     Added support for serial mode encode.
2009-05-06   bc     Initial revision

========================================================================== */

/*========================================================================
  Include Files
 ==========================================================================*/
#include "venctest_ITestCase.h"
#include "venctest_Thread.h"
#include "venctest_Debug.h"
#include "venctest_Config.h"
#include "venc_semaphore.h"

namespace venctest
{

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   ITestCase::ITestCase()
      : m_pThread(new Thread()),
        m_eTestResult(OMX_ErrorNone),
        m_nTestNum(0),
	m_bIsVTPath(OMX_FALSE) 
   {
      if (venc_semaphore_create(&m_pSemaphore, 0, 1) != 0)
      {
         VENC_TEST_MSG_MEDIUM("error creating semaphore, sync will not work", 0, 0, 0);
      }
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   ITestCase::~ITestCase()
   {
      if (m_pThread)
      {
         delete m_pThread;
      }
      if (m_pSemaphore)
      {
	     (void)venc_semaphore_destroy(m_pSemaphore);   
      }      
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE ITestCase::Start(OMX_STRING pConfigFileName,
                                  OMX_S32 nTestNum)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;

      Config config;
      config.GetEncoderConfig(&m_sConfig);
      config.GetDynamicConfig(&m_sDynamicConfig);

      m_nTestNum = nTestNum;

      result = config.Parse(pConfigFileName, &m_sConfig, &m_sDynamicConfig);
      if (result == OMX_ErrorNone)
      {
         result = ValidateAssumptions(&m_sConfig, &m_sDynamicConfig);

         if (result == OMX_ErrorNone)
         {
     	    if (m_sConfig.eControlRate == OMX_Video_ControlRateConstantSkipFrames || 
		        m_sConfig.eResyncMarkerType != RESYNC_MARKER_NONE) 
	        {
                  m_bIsVTPath = OMX_TRUE;
		}
 
            VENC_TEST_MSG_MEDIUM("Starting test thread...", 0, 0, 0);
            if (m_pThread)
            {
               result = m_pThread->Start(ThreadEntry,    // thread entry
                                         this,           // thread args
                                         0);             // priority
            }
            else
            {
               VENC_TEST_MSG_ERROR("Start test thread failed...", 0, 0, 0);
               result = OMX_ErrorUndefined;
            }
         }
         else
         {
            VENC_TEST_MSG_ERROR("Invalid config. Assumptions not validated", 0, 0, 0);
         }
      }
      else
      {
         VENC_TEST_MSG_ERROR("Error parsing config file", 0, 0, 0);
      }

      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE ITestCase::Finish()
   {
      OMX_ERRORTYPE result = OMX_ErrorUndefined;

      if (m_pThread)
      {
         OMX_ERRORTYPE threadResult;

         VENC_TEST_MSG_MEDIUM("waiting for thread to finish...",0,0,0);

         // wait for thread to exit
         result = m_pThread->Join(&threadResult);

         if (result == OMX_ErrorNone)
         {
            result = threadResult;
         }

         if (threadResult != OMX_ErrorNone)
         {
            VENC_TEST_MSG_ERROR("test case thread execution error", 0, 0, 0);
         }
      }

      // let's not over-ride the original error with a different result
      if (m_eTestResult == OMX_ErrorNone)
      {
         m_eTestResult = result;
      }

      return m_eTestResult;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE ITestCase::CheckError(OMX_ERRORTYPE eTestResult)
   {
      // The first error we encounter (if one actually occurs) is the final test result.
      // Do not over-ride the original error!
      if (m_eTestResult == OMX_ErrorNone)
      {
         m_eTestResult = eTestResult;
      }

      // Simply return the result that was passed in
      return eTestResult;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE ITestCase::ThreadEntry(OMX_PTR pThreadData)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;

      ITestCase* pTest = (ITestCase*) pThreadData;
      result = pTest->Execute(&pTest->m_sConfig, &pTest->m_sDynamicConfig,
         pTest->m_nTestNum);

      if (pTest->m_eTestResult == OMX_ErrorNone)
      {
         result = pTest->m_eTestResult;
      }

      return pTest->m_eTestResult;
   }

} // namespace venctest

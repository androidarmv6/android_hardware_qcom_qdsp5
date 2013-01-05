/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/src/venctest_Mutex.cpp#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2009-08-13   bc     Removed OS dependency from test framework
2009-05-06   bc     Initial revision

========================================================================== */

/*========================================================================
  Include Files
 ==========================================================================*/
#include "venctest_ComDef.h"
#include "venctest_Mutex.h"
#include "venctest_Debug.h"
#include "venc_mutex.h"

namespace venctest
{

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   Mutex::Mutex() :
      m_pMutex(NULL)
   {
      if (venc_mutex_create(&m_pMutex) != 0)
      {
         VENC_TEST_MSG_ERROR("failed to init mutex", 0, 0, 0);
      }
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   Mutex::~Mutex()
   {
      if (venc_mutex_destroy(m_pMutex) != 0)
      {
         VENC_TEST_MSG_ERROR("failed to destroy mutex", 0, 0, 0);
      }
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Mutex::Lock()
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;
      if (venc_mutex_lock(m_pMutex) != 0)
      {
         VENC_TEST_MSG_ERROR("failed to lock mutex", 0, 0, 0);
         result = OMX_ErrorUndefined;
      }
      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Mutex::UnLock()
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;
      if (venc_mutex_unlock(m_pMutex) != 0)
      {
         VENC_TEST_MSG_ERROR("failed to unlock mutex", 0, 0, 0);
         result = OMX_ErrorUndefined;
      }
      return result;
   }
}

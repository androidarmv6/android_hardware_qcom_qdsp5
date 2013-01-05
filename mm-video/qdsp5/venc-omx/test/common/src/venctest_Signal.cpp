/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/src/venctest_Signal.cpp#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2009-08-13   bc     Removed OS dependency from test framework
2009-08-13   bc     Beefed up OS abstraction error handling
2009-05-06   bc     Initial revision

========================================================================== */

/*========================================================================
  Include Files
 ==========================================================================*/
#include "venctest_Signal.h"
#include "venctest_Debug.h"
#include "venc_signal.h"

namespace venctest
{

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   Signal::Signal()
      : m_pSignal(NULL)
   {
      if (venc_signal_create(&m_pSignal) != 0)
      {
         VENC_TEST_MSG_ERROR("error creating signal", 0, 0, 0);
      }
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   Signal::~Signal()
   {
      if (venc_signal_destroy(m_pSignal) != 0)
      {
         VENC_TEST_MSG_ERROR("error destroying signal", 0, 0, 0);
      }
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Signal::Set()
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;
      if (venc_signal_set(m_pSignal) != 0)
      {
         VENC_TEST_MSG_ERROR("error setting signal", 0, 0, 0);
         result = OMX_ErrorUndefined;
      }
      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Signal::Wait(OMX_S32 nTimeoutMillis)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;
      
      int ret = venc_signal_wait(m_pSignal, (int) nTimeoutMillis);

      if (ret == 2)
      {
         result = OMX_ErrorTimeout;
      }
      else if (ret != 0)
      {
         VENC_TEST_MSG_ERROR("error waiting for signal", 0, 0, 0);
         result = OMX_ErrorUndefined;
      }

      return result;
   }

} // namespace venctest

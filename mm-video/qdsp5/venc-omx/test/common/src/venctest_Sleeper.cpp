/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/src/venctest_Sleeper.cpp#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2009-08-13   bc     Removed OS dependency from test framework
2009-05-06   bc     Initial revision

========================================================================== */

/*========================================================================
  Include Files
 ==========================================================================*/
#include "venctest_Sleeper.h"
#include "venctest_Debug.h"
#include "venc_sleep.h"

namespace venctest
{
   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Sleeper::Sleep(OMX_S32 nTimeMillis)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;

      if (venc_sleep((int) nTimeMillis) != 0)
      {
         VENC_TEST_MSG_ERROR("error sleeping", 0, 0, 0);
         result = OMX_ErrorUndefined;
      }

      return result;
   }
} // namespace venctest

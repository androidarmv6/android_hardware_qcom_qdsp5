#ifndef _VENC_TEST_SLEEPER_H
#define _VENC_TEST_SLEEPER_H
/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/inc/venctest_Sleeper.h#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2009-04-17   bc     Initial revision

 ========================================================================== */

/*========================================================================

                     INCLUDE FILES FOR MODULE

==========================================================================*/
#include "OMX_Core.h"

namespace venctest
{

   /**
    * @brief Utility class for sleeping
    */
   class Sleeper
   {
      public:

         /**
          * @brief Sleep for the specified time
          *
          * @param nTimeMillis The time to sleep in milliseconds
          */
         static OMX_ERRORTYPE Sleep(OMX_S32 nTimeMillis);

      private:
         Sleeper() {}
         ~Sleeper() {}
   };
} // namespace venctest

#endif // #ifndef _VENC_TEST_SLEEPER_H

#ifndef _VENC_TEST_TIME_H
#define _VENC_TEST_TIME_H
/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/inc/venctest_Time.h#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2009-07-14   bc     Updated documentation
2009-04-17   bc     Initial revision

 ========================================================================== */

/*========================================================================

                     INCLUDE FILES FOR MODULE

==========================================================================*/
#include "OMX_Core.h"

namespace venctest
{

   /**
    * @brief Utility class for getting system time
    */
   class Time
   {
      public:

         /**
          * @brief Get the timestamp in microseconds
          */
         static OMX_TICKS GetTimeMicrosec();

      private:
         Time() {}
         ~Time() {}
   };
} // namespace venctest

#endif // #ifndef _VENC_TEST_TIME_H

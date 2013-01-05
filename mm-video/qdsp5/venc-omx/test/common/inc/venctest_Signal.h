#ifndef _VENC_TEST_SIGNAL_H
#define _VENC_TEST_SIGNAL_H
/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/inc/venctest_Signal.h#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2009-08-13   bc     Removed OS dependency from test framework
2009-04-17   bc     Initial revision

 ========================================================================== */

/*========================================================================

                     INCLUDE FILES FOR MODULE

==========================================================================*/
#include "OMX_Core.h"

namespace venctest
{

   /**
    * @brief Class for sending signals to threads
    *
    * Signals behave similarly to binary (not counting) semaphores.
    */
   class Signal
   {
      public:

         /**
          * @brief Constructor
          */
         Signal();

         /**
          * @brief Destructor
          */
         ~Signal();

      public:

         /**
          * @brief Set a signal
          */
         OMX_ERRORTYPE Set();

         /**
          * @brief Wait for signal to be set
          *
          * @param nTimeoutMillis Milliseconds before timeout. Specify 0 for infinite.
          */
         OMX_ERRORTYPE Wait(OMX_S32 nTimeoutMillis);

      private:

         void* m_pSignal;
   };
}

#endif // #ifndef _VENC_TEST_SIGNAL_H

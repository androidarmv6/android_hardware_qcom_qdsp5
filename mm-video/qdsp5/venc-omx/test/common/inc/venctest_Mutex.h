#ifndef _VENC_TEST_MUTEX_H
#define _VENC_TEST_MUTEX_H
/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/inc/venctest_Mutex.h#1 $

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
    * @brief Mutex class
    */
   class Mutex
   {
      public:

         /**
          * @brief Constructor
          */
         Mutex();

         /**
          * @brief Destructor
          */
         ~Mutex();

         /**
          * @brief Locks the mutex
          */
         OMX_ERRORTYPE Lock();

         /**
          * @brief Unlocks the mutex
          */
         OMX_ERRORTYPE UnLock();
      private:

         void* m_pMutex;
   };
}

#endif // #ifndef _VENC_TEST_MUTEX_H

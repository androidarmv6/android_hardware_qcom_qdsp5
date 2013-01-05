#ifndef _VENC_TEST_THREAD_H
#define _VENC_TEST_THREAD_H
/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/inc/venctest_Thread.h#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2009-08-13   bc     Removed OS dependency from test framework
2009-04-17   bc     Initial revision

 ========================================================================== */

/*========================================================================

                     INCLUDE FILES FOR MODULE

==========================================================================*/
#include "OMX_Core.h"
#include "venc_thread.h"

namespace venctest
{

   /**
    * @brief Thread class
    */
   class Thread
   {
      public:

         /**
          * @brief Thread start function pointer
          */
         typedef OMX_ERRORTYPE (*StartFnType) (OMX_PTR pThreadData);

      public:

         /**
          * @brief Constructor (NOTE: does not create the thread!)
          */
         Thread();

         /**
          * @brief Destructor (NOTE: does not destroty the thread!)
          */
         ~Thread();

         /**
          * @brief Starts the thread at the specified function
          *
          * @param pFn Pointer to the thread start function
          * @param pThreadArgs Arguments passed in to the thread (null is valid)
          * @param nPriority Priority of the thread, unique to each platform.
          */
         OMX_ERRORTYPE Start(StartFnType pFn,
                             OMX_PTR pThreadArgs,
                             OMX_S32 nPriority);

         /**
          * @brief Joins the thread
          *
          * Function will block until the thread exits.
          *
          * @param pThreadResult If not NULL the threads status will be store here
         */
         OMX_ERRORTYPE Join(OMX_ERRORTYPE* pThreadResult);

      private:
         static int ThreadEntry(void* pThreadData);

      private:
         StartFnType m_pFn;
         OMX_S32 m_nPriority;
         void* m_pThread;
         OMX_PTR m_pThreadArgs;
   };
}

#endif // #ifndef _VENC_TEST_THREAD_H

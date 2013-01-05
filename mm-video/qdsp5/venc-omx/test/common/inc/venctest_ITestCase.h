#ifndef _VENC_TEST_ITEST_CASE_H
#define _VENC_TEST_ITEST_CASE_H
/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/inc/venctest_ITestCase.h#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2010-06-12   as     Added support for serial mode encode.
2009-04-17   bc     Initial revision

 ========================================================================== */

/*========================================================================

                     INCLUDE FILES FOR MODULE

==========================================================================*/
#include "OMX_Core.h"
#include "venctest_ComDef.h"

namespace venctest
{

   class Thread;     // forward declaration

   /**
    * @brief A test case interface
    */
   class ITestCase
   {
      public:

         /**
          * @brief Constructor
          *
          * Must explicitly be called by all derived classes
          */
         ITestCase();

         /**
          * @brief Destructor
          */
         virtual ~ITestCase();

      public:

         /**
          * @brief Start the test asynchronously
          */
         OMX_ERRORTYPE Start(OMX_STRING pConfigFileName,
                             OMX_S32 nTestNum);

         /**
          * @brief Block until the test case is finished
          *
          * @return The final test result
          */
         OMX_ERRORTYPE Finish();

   protected:

         /**
          * @brief Checks for an error and sets the final test result.
          *
          * When the test case exection is complete, the first error encountered
          * will be returned.
          *
          * @returns eTestResult
          */
         OMX_ERRORTYPE CheckError(OMX_ERRORTYPE eTestResult);

      private:

         /**
          * @brief The execution function for the test case
          *
          */
         virtual OMX_ERRORTYPE Execute(EncoderConfigType* pConfig,
                                       DynamicConfigType* pDynamicConfig,
                                       OMX_S32 nTestNum) = 0;

         /**
          * @brief Validates test case specific configuration assumptions.
          *
          * Required to be implemented by each test case. If no assumptions are
          * made, simply return OMX_ErrorNone.
          */
         virtual OMX_ERRORTYPE ValidateAssumptions(EncoderConfigType* pConfig,
                                                   DynamicConfigType* pDynamicConfig) = 0;

         /**
          * @brief Thread entry
          *
          * Invokes the child class's Execute method
          */
         static OMX_ERRORTYPE ThreadEntry(OMX_PTR pThreadData);

      private:
         Thread* m_pThread;
         EncoderConfigType m_sConfig;
         DynamicConfigType m_sDynamicConfig;
         OMX_ERRORTYPE m_eTestResult;
         OMX_S32 m_nTestNum;
      protected: 
         void * m_pSemaphore;
         OMX_BOOL m_bIsVTPath;
   };

} // namespace venctest

#endif // #ifndef  _VENC_TEST_ITEST_CASE_H

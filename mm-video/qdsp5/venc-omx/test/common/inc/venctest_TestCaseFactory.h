#ifndef _VENC_TEST_CASE_FACTORY_H
#define _VENC_TEST_CASE_FACTORY_H
/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/inc/venctest_TestCaseFactory.h#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2009-07-14   bc     Updated documentation
2009-04-17   bc     Initial revision

 ========================================================================== */

/*========================================================================

                     INCLUDE FILES FOR MODULE

==========================================================================*/
#include "OMX_Core.h"
#include "venctest_ComDef.h"

namespace venctest
{

   class ITestCase;  // forward declaration

   /**
    * @brief A test case class factory. All test cases created/destroyed through this class.
    */
   class TestCaseFactory
   {
      public:

         /**
          * @brief Allocates a test case object
          */
         static ITestCase* AllocTest(OMX_STRING pTestName);

         /**
          * @brief Destroys a test case object
          */
         static OMX_ERRORTYPE DestroyTest(ITestCase* pTest);

      private:
         TestCaseFactory(){}
         ~TestCaseFactory(){}
   };

} // namespace venctest

#endif // #ifndef  _VENC_TEST_CASE_FACTORY_H

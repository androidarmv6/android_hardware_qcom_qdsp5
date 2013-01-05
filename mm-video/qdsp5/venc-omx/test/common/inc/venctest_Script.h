#ifndef _VENC_TEST_SCRIPT_H
#define _VENC_TEST_SCRIPT_H
/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/inc/venctest_Script.h#1 $

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
   class File; // forward declaration

   /**
    * @brief Class for parsing test case script file
    */
   class Script
   {
      public:

         /**
          * @brief Constructor
          */
         Script();

         /**
          * @brief Destructor
          */
         ~Script();

      public:

         /**
          * @brief Parses the script file
          *
          * This method can only be called one time per instantiation of this class
          *
          * @param pFileName The name of the script file
          */
         OMX_ERRORTYPE Configure(OMX_STRING pFileName);

         /**
          * @brief Parses the next test from the test script
          *
          * @return OMX_ErrorNoMore if there are no more tests to run
          */
         OMX_ERRORTYPE NextTest(TestDescriptionType* pTestDescription);

      private:
         File* m_pFile;
   };

} // namespace venctest

#endif // #ifndef _VENC_TEST_SCRIPT_H

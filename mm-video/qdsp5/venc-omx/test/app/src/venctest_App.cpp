/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/app/src/venctest_App.cpp#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2009-08-24   bc     windows dependency cleanup
2009-08-13   bc     OMX_Init and OXM_Deinit no longer called per instance (per the IL spec)
2009-05-06   bc     Initial revision

========================================================================== */

/*========================================================================
  Include Files
 ==========================================================================*/
#include <stdlib.h>
#include "venctest_Script.h"
#include "venctest_Debug.h"
#include "venctest_ComDef.h"
#include "venctest_ITestCase.h"
#include "venctest_TestCaseFactory.h"


void RunScript(OMX_STRING pScriptFile)
{
   OMX_S32 nPass = 0;
   OMX_S32 nFail = 0;
   OMX_S32 nTestCase = 0;
   OMX_ERRORTYPE result = OMX_ErrorNone;


   venctest::Script script;
   script.Configure(pScriptFile);

   if (result == OMX_ErrorNone)
   {
      venctest::TestDescriptionType testDescription;
      do
      {
         result = script.NextTest(&testDescription);

         if (result == OMX_ErrorNone)
         {
            for (OMX_S32 i = 0; i < testDescription.nSession; i++)
            {
               venctest::ITestCase* pTest =
                  venctest::TestCaseFactory::AllocTest((OMX_STRING) testDescription.cTestName);

               if (pTest != NULL)
               {
                  VENC_TEST_MSG_HIGH("Running test %d", nPass + nFail, 0, 0);
                  result = pTest->Start(testDescription.cConfigFile, i);

                  if (result == OMX_ErrorNone)
                  {
                     result = pTest->Finish();
                     if (result == OMX_ErrorNone)
                     {
                        ++nPass;
                     }
                     else
                     {
                        VENC_TEST_MSG_ERROR("test %d failed", nPass + nFail, 0, 0);
                        ++nFail;
                     }
                  }
                  else
                  {
                     VENC_TEST_MSG_ERROR("error starting test", 0, 0, 0);
                     ++nFail;
                  }

                  (void) venctest::TestCaseFactory::DestroyTest(pTest);
               }
               else
               {
                  VENC_TEST_MSG_ERROR("unable to alloc test", 0, 0, 0);
               }

            }
         }
         else if (result != OMX_ErrorNoMore)
         {
            VENC_TEST_MSG_ERROR("error parsing script", 0, 0, 0);
         }

      } while (result != OMX_ErrorNoMore);
   }

   VENC_TEST_MSG_HIGH("passed %d out of %d tests", (int) nPass, (int) nPass + nFail, 0);

}

void RunTest(OMX_STRING pTestName,
             OMX_STRING pConfigFile,
             OMX_S32 nSession)
{
   OMX_S32 nPass = 0;
   OMX_S32 nFail = 0;
   OMX_ERRORTYPE result = OMX_ErrorNone;

   for (OMX_S32 i = 0; i < nSession; i++)
   {
      venctest::ITestCase* pTest =
         venctest::TestCaseFactory::AllocTest((OMX_STRING) pTestName);

      if (pTest != NULL)
      {
         VENC_TEST_MSG_HIGH("Running test %d", nPass + nFail, 0, 0);
         result = pTest->Start(pConfigFile, i);

         if (result == OMX_ErrorNone)
         {
            result = pTest->Finish();
            if (result == OMX_ErrorNone)
            {
               ++nPass;
            }
            else
            {
               ++nFail;
            }
         }
         else
         {
            VENC_TEST_MSG_ERROR("error starting test", 0, 0, 0);
         }

         (void) venctest::TestCaseFactory::DestroyTest(pTest);
      }
      else
      {
         VENC_TEST_MSG_ERROR("unable to alloc test", 0, 0, 0);
      }
   }

   VENC_TEST_MSG_HIGH("passed %d out of %d tests", (int) nPass, (int) nPass + nFail, 0);
}

int main (int argc, char *argv[])
{
   OMX_Init();

   if (argc == 2)
   {
      OMX_STRING pScriptFile = (OMX_STRING) argv[1];

      RunScript(pScriptFile);
   }
   else if (argc == 4)
   {
      OMX_STRING pTestName = (OMX_STRING) argv[1];
      OMX_STRING pConfigFile = (OMX_STRING) argv[2];
      OMX_STRING pNumSession = (OMX_STRING) argv[3];
      OMX_S32 nSession;

      nSession = atoi((char*) pNumSession);

      RunTest(pTestName, pConfigFile, nSession);
   }
   else
   {
      VENC_TEST_MSG_ERROR("invalid number of command args %d", argc, 0, 0);
      VENC_TEST_MSG_ERROR("./mm-venc-omx-test ENCODE Config.cfg 1",0,0,0);
   }

   OMX_Deinit();
}

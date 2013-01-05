/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/src/venctest_TestCaseFactory.cpp#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2009-06-30   bc     Removed temporary test case for coupling buffers
2009-05-06   bc     Initial revision

========================================================================== */

/*========================================================================
  Include Files
 ==========================================================================*/
#include "venctest_TestCaseFactory.h"
#include "venctest_ITestCase.h"
#include "venctest_Thread.h"
#include "venctest_Debug.h"
#include "venctest_Parser.h"

// test case object headers
#include "venctest_TestChangeQuality.h"
#include "venctest_TestGetSyntaxHdr.h"
#include "venctest_TestEncode.h"
#include "venctest_TestStateExecutingToIdle.h"
#include "venctest_TestStatePause.h"
#include "venctest_TestFlush.h"

namespace venctest
{

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   ITestCase* TestCaseFactory::AllocTest(OMX_STRING pTestName)
   {
      ITestCase* pTest = NULL;

      if (Parser::StringICompare(pTestName, "CHANGE_QUALITY") == 0)
      {
         pTest = new TestChangeQuality;
      }
      else if (Parser::StringICompare(pTestName, "GET_SYNTAX_HDR") == 0)
      {
         pTest = new TestGetSyntaxHdr;
      }
      else if (Parser::StringICompare(pTestName, "ENCODE") == 0)
      {
         pTest = new TestEncode;
      }
      else if (Parser::StringICompare(pTestName, "STATE_EXECUTING_TO_IDLE") == 0)
      {
         pTest = new TestStateExecutingToIdle;
      }
      else if (Parser::StringICompare(pTestName, "FLUSH") == 0)
      {
         pTest = new TestFlush;
      }
      else if (Parser::StringICompare(pTestName, "STATE_PAUSE") == 0)
      {
         pTest = new TestStatePause;
      }
      else
      {
         VENC_TEST_MSG_ERROR("invalid test name", 0, 0, 0);
      }

      return pTest;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE TestCaseFactory::DestroyTest(ITestCase* pTest)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;

      if (pTest)
      {
         delete pTest;
      }
      else
      {
         VENC_TEST_MSG_ERROR("NULL param", 0, 0, 0);
         result = OMX_ErrorBadParameter;
      }

      return result;
   }

} // namespace venctest

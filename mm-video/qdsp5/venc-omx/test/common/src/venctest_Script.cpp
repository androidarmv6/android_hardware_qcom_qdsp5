/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/src/venctest_Script.cpp#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2009-05-06   bc     Initial revision

========================================================================== */

/*========================================================================
  Include Files
 ==========================================================================*/
#include "venctest_Debug.h"
#include "venctest_Script.h"
#include "venctest_File.h"
#include "venctest_Parser.h"
#include "venctest_ComDef.h"
#include <stdlib.h>

namespace venctest
{

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   Script::Script()
      : m_pFile(new File)
   {
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   Script::~Script()
   {
      if (m_pFile)
      {
         delete m_pFile;
      }
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Script::Configure(OMX_STRING pFileName)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;
      if (pFileName != NULL)
      {
         if (m_pFile)
         {
            result = m_pFile->Open(pFileName, OMX_TRUE);

            if (result != OMX_ErrorNone)
            {
               VENC_TEST_MSG_ERROR("failed to open file", 0, 0, 0);
            }
         }
         else
         {
            result = OMX_ErrorUndefined;
         }
      }
      else
      {
         VENC_TEST_MSG_ERROR("null param", 0, 0, 0);
         result = OMX_ErrorBadParameter;
      }
      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Script::NextTest(TestDescriptionType* pTestDescription)
   {
      static const OMX_S32 maxFieldName = 64;
      static const OMX_S32 maxLineLen = maxFieldName * 2;

      OMX_ERRORTYPE result = OMX_ErrorNone;

      ParserStrVector v;
      OMX_S32 nLine = 0;

      while (result != OMX_ErrorNoMore)
      {
         char pBuf[maxLineLen];
         char* pTrimmed;
         OMX_S32 nChars = Parser::ReadLine(m_pFile, maxLineLen, pBuf);
         v.clear();
         if (nChars > 0)
         {
            (void) Parser::RemoveComments(pBuf);
            pTrimmed = Parser::Trim(pBuf);

            // No empty lines
            if (strlen(pTrimmed) != 0)
            {
               (void) Parser::TokenizeString(&v, pTrimmed, "\t ");
               if (v.size() == 3)
               {
                  break;
               }
               else
               {
                  VENC_TEST_MSG_ERROR("badly formatted script file. %d tokens on line %d", v.size(), nLine, 0);
                  result = OMX_ErrorUndefined;
               }
            }
         }
         else if (nChars < 0)
         {
            result = OMX_ErrorNoMore;
         }
         ++nLine;
      }

      if (result == OMX_ErrorNone)
      {
         memcpy(pTestDescription->cTestName, v[0], strlen(v[0])+1);

         memcpy(pTestDescription->cConfigFile, v[1], strlen(v[1])+1);

         pTestDescription->nSession = (OMX_S32) atoi(v[2]);
      }

      return result;
   }

} // namespace venctest

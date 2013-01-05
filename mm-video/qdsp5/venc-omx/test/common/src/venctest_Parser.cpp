/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/src/venctest_Parser.cpp#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2009-06-23   bc     Added support for spaces within quotes. Quoted text is treated as a single token.
                    Primarily for allowing m4v file to be written to WM dir "\Storage Card"
2009-05-06   bc     Initial revision

========================================================================== */

/*========================================================================
  Include Files
 ==========================================================================*/
#include "venctest_ComDef.h"
#include "venctest_Parser.h"
#include "venctest_File.h"
#include "venctest_Debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <cctype>
#include <string.h>

namespace venctest
{

   ParserStrVector::ParserStrVector()
      : count(0)
   {
      clear();
   }

   ParserStrVector::~ParserStrVector()
   {
      clear();
   }

   OMX_S32 ParserStrVector::size() { return count; }

   void ParserStrVector::clear()
   {
      count = 0;
   }

   OMX_STRING ParserStrVector::operator[] (OMX_S32 i)
   {
      if (i < count)
      {
         return pStr[i];
      }
      else
      {
         return (OMX_STRING) NULL;
      }
   }

   void ParserStrVector::push_back(OMX_STRING s)
   {
      if (count < ParserMaxStrings)
      {
         pStr[count++] = s;
      }
   }


   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_S32 Parser::ReadLine(File* pFile,
                            OMX_S32 nMaxRead,
                            OMX_STRING pBuf)
   {
      OMX_S32 nTotal = 0;   // total number of chars read
      OMX_S32 nRead = 0;    // fread return value
      OMX_S32 numChars = 0;   // number of chars read excluding \r\n
      OMX_S32 keepGoing = 1;

      pFile->Read((OMX_U8*) &pBuf[nTotal], 1, &nRead);
      while (keepGoing &&
             nTotal < nMaxRead &&
             nRead > 0)
      {
         switch (pBuf[nTotal])
         {
            case '\n':
               keepGoing = 0;
            case '\r':
               pBuf[nTotal] = (char) 0;
               break;
            default:
               numChars++;
         }

         nTotal++;
         if (keepGoing)
         {
            pFile->Read((OMX_U8*) &pBuf[nTotal], 1, &nRead);
         }
      }

      // terminate string
      pBuf[numChars] = 0;

      // we have reached the end of the file
      // and nothing was read
      if (nRead <= 0 && nTotal == 0)
      {
         return -1;
      }

      // several cases:
      // 1. end of line with chars read (return n)
      // 2. end of line with no chars read (return 0)
      // 3. end of file with chars read (return n)
      //    next time function is called -1 will be returned
      return numChars;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Parser::StringToLower(OMX_STRING pStr,
                                       OMX_STRING pLowerStr)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;
      OMX_S32 i, l;
      if (!pStr)
      {
         return OMX_ErrorBadParameter;
      }

      l = (OMX_S32) strlen(pStr);

      for (i = 0; i < l; i++)
      {
         pLowerStr[i] = (char) tolower(pStr[i]);
      }
      pLowerStr[i] = (char) 0;

      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_S32 Parser::StringICompare(OMX_STRING pStr1,
                                  OMX_STRING pStr2)
   {
      OMX_S32 result = 0;
      OMX_S32 strl1,strl2;

      strl1 = (OMX_S32) strlen(pStr1);
      strl2 = (OMX_S32) strlen(pStr2);

      if (strl1 != strl2)
      {
         result = 1; // not equal if lenghts are different
      }
      else if (strl1 == 0 && strl2 == 0)
      {
         result = 0; // equal if empty string
      }
      else
      {

         OMX_STRING pLower1 = (OMX_STRING) malloc(strl1+1);
         OMX_STRING pLower2 = (OMX_STRING) malloc(strl2+1);

         memcpy(pLower1, pStr1, strl1+1);
         memcpy(pLower2, pStr2, strl2+1);

         StringToLower(pStr1, pLower1);
         StringToLower(pStr2, pLower2);

         result = (OMX_S32) strcmp(pLower1, pLower2);

         free(pLower1);
         free(pLower2);
      }
      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Parser::RemoveComments(OMX_STRING pStr)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;
      OMX_STRING p;
      if (!pStr)
      {
         return OMX_ErrorBadParameter;
      }
      if ((p = StrChrs(pStr, "#")) != NULL)
      {
         *p = 0;
      }
      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_S32 Parser::TokenizeString(ParserStrVector* pTokens,
                                  OMX_STRING pStr,
                                  OMX_STRING pDelims)
   {
      char* pOut;
      char* pIn = pStr;

      if (!pTokens || !(pIn = Trim(pIn)) || !pDelims)
      {
         return 0;
      }

      if (*pIn)
      {
         pTokens->push_back(pIn);
         while (*pIn && (pOut = StrChrs(pIn, pDelims)))
         {
            if (*pIn == '"' ||
                *pIn == '\'')
            {
               // ignore delimeters in quotes
               char* pQuote = pIn;
               int len = (int) strlen(pIn);
               for (int i = 0; i < len; i++)
               {
                  ++pIn;
                  if (*pQuote == *pIn)
                  {
                     *pIn = (char) 0;
                     ++pIn;
                     break;
                  }
               }
            }
            else
            {
               // advance past the last delimeter
               for (int i = 0; i < (int) strlen(pDelims); i++)
               {
                  if (*pOut && *pOut == pDelims[i])
                  {
                     *pOut = 0;
                     pOut++;
                     i = -1;
                  }
               }

               if (strlen(pOut))
               {
                  if (*pOut == '\'' || *pOut == '"')
                  {
                     // remove the quotation mark
                     pTokens->push_back(pOut+1);
                  }
                  else
                  {
                     pTokens->push_back(pOut);
                  }
               }
               pIn = pOut;
            }
         }

         return (OMX_S32) pTokens->size();
      }
      return 0;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_STRING Parser::Trim(OMX_STRING pStr)
   {
      OMX_STRING pEnd;
      OMX_STRING pStart;
      OMX_STRING p;

      if (!pStr)
      {
         return 0;
      }

      pStart = pStr;
      pEnd = pStr + strlen(pStr) - 1;

      // remove leading whitespace
      while (*pStart && (*pStart == ' ' || *pStart == '\t'))
      {
         pStart++;
      }

      // remove trailing whitespace
      p = pEnd;
      while ((*p) && (*p == ' ' || *p == '\t'))
      {
         if (p == pStart)
         {
            break;
         }
         *p = 0;
         p--;
      }

      return pStart;
   }

   /////////////////////////////////////////////////////////////////////////////
   //////////////////////////////////////////////////////////////////////////////////////////////////////////   //////////////////////////////////////////////////
   OMX_STRING Parser::StrChrs(OMX_STRING pStr,
                              OMX_STRING pDelims)
   {
      OMX_S32 nDelim = (OMX_S32) strlen(pDelims);
      OMX_S32 len = (OMX_S32) strlen(pStr);
      OMX_STRING ret = 0;
      OMX_S32 i = 0, j = 0;

      if (len == 0 || nDelim == 0)
         return ret;

      for (i = 0; i < len; i++)
      {
         for (j = 0; j < nDelim; j++)
         {
            if (pDelims[j] == pStr[i])
            {
               ret = &pStr[i];
               return ret;
            }
         }
      }

      return ret;
   }

   OMX_ERRORTYPE Parser::AppendTestNum(OMX_STRING pFileName,
                                       OMX_S32 nTestNum)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;


      if (pFileName != NULL)
      {
         static const OMX_S32 nMaxExtnLen = 64;

         OMX_S32 nMaxFileNameLen = VENC_TEST_MAX_STRING;
         OMX_STRING pInsert = StrChrs(pFileName, (OMX_STRING) ".");

         // ensure the extension is less than 64 chars
         if (pInsert != NULL && strlen(pInsert) >= nMaxExtnLen - 1)
         {
            // if the file extension is too big then just ignore it 
            // and advance to the end of the string
            pInsert = NULL;
         }

         if (pInsert != NULL)
         {
            char pExtnCopy[nMaxExtnLen];

            // make a copy of the file extension
            memcpy(pExtnCopy, (char*) pInsert, strlen((char*) pInsert) + 1);

            nMaxFileNameLen -= ((OMX_S32) pInsert) - ((OMX_S32) pFileName);

            // append the number and the file extension
            sprintf((char* ) pInsert, "_%05d%s", 
               (int) nTestNum, (char*) pExtnCopy);
         }
         else
         {
            // we have no file extension so just append the number
            pInsert = pFileName + strlen((char*) pFileName);

            nMaxFileNameLen -= ((OMX_S32) pInsert) - ((OMX_S32) pFileName);

            sprintf((char* ) pInsert, "_%05d", (int) nTestNum);
         }
      }
      else
      {
         result = OMX_ErrorBadParameter;
      }

      return result;
   }

} ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// namespace venctest

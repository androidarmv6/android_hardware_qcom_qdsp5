/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/src/venctest_File.cpp#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2009-08-13   bc     Removed OS dependency from test framework
2009-05-06   bc     Initial revision

========================================================================== */

/*========================================================================
  Include Files
 ==========================================================================*/
#include "venctest_ComDef.h"
#include "venctest_Debug.h"
#include "venctest_File.h"
#include "venc_file.h"

namespace venctest
{
   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   File::File()
      : m_pFile(NULL),
        m_bReadOnly(OMX_TRUE)
   {
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   File::~File()
   {
      if (m_pFile)
         venc_file_close(m_pFile);
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE File::Open(OMX_STRING pFileName,
                            OMX_BOOL bReadOnly)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;
      if (pFileName != NULL)
      {
         if (m_pFile == NULL)
         {
            int ret;
            m_bReadOnly = bReadOnly;
            if (bReadOnly == OMX_TRUE)
            {
               ret = venc_file_open(&m_pFile, (char*) pFileName, 1);
            }
            else
            {
               ret = venc_file_open(&m_pFile, (char*) pFileName, 0);
            }

            if (ret != 0)
            {
               VENC_TEST_MSG_ERROR("Unable to open file", 0, 0, 0);
               result = OMX_ErrorUndefined;
            }
         }
         else
         {
            VENC_TEST_MSG_ERROR("File is already open", 0, 0, 0);
            result = OMX_ErrorUndefined;
         }
      }
      else
      {
         VENC_TEST_MSG_ERROR("Null param", 0, 0, 0);
         result = OMX_ErrorBadParameter;
      }
      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE File::Read(OMX_U8* pBuffer,
                            OMX_S32 nBytes,
                            OMX_S32* pBytesRead)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;
      if (m_bReadOnly == OMX_TRUE)
      {
         if (pBuffer != NULL)
         {
            if (nBytes > 0)
            {
               if (pBytesRead != NULL)
               {
                  *pBytesRead = (OMX_S32) venc_file_read(m_pFile, (void*) pBuffer, (int) nBytes);
               }
               else
               {
                  VENC_TEST_MSG_ERROR("Null param", 0, 0, 0);
                  result = OMX_ErrorBadParameter;
               }
            }
            else
            {
               VENC_TEST_MSG_ERROR("Bytes must be > 0", 0, 0, 0);
               result = OMX_ErrorBadParameter;
            }
         }
         else
         {
            VENC_TEST_MSG_ERROR("Null param", 0, 0, 0);
            result = OMX_ErrorBadParameter;
         }
      }
      else
      {
         VENC_TEST_MSG_ERROR("File is open for writing", 0, 0, 0);
         result = OMX_ErrorUndefined;
      }
      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE File::Write(OMX_U8* pBuffer,
                             OMX_S32 nBytes,
                             OMX_S32* pBytesWritten)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;
      if (m_bReadOnly == OMX_FALSE)
      {
         if (pBuffer != NULL)
         {
            if (nBytes > 0)
            {
               if (pBytesWritten != NULL)
               {
                  *pBytesWritten = venc_file_write(m_pFile, (void*) pBuffer, (int) nBytes);
               }
               else
               {
                  VENC_TEST_MSG_ERROR("Null param", 0, 0, 0);
                  result = OMX_ErrorBadParameter;
               }
            }
            else
            {
               VENC_TEST_MSG_ERROR("Bytes must be > 0", 0, 0, 0);
               result = OMX_ErrorBadParameter;
            }
         }
         else
         {
            VENC_TEST_MSG_ERROR("Null param", 0, 0, 0);
            result = OMX_ErrorBadParameter;
         }
      }
      else
      {
         VENC_TEST_MSG_ERROR("File is open for reading", 0, 0, 0);
         result = OMX_ErrorUndefined;
      }
      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE File::SeekStart(OMX_S32 nBytes)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;
      if (nBytes >= 0)
      {
         if (venc_file_seek_start(m_pFile, (int) nBytes) != 0)
         {
            VENC_TEST_MSG_ERROR("failed to seek", 0, 0, 0);
            result = OMX_ErrorUndefined;
         }
      }
      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE File::Close()
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;
      if (m_pFile != NULL)
      {
         venc_file_close(m_pFile);
         m_pFile = NULL;
      }
      else
      {
         VENC_TEST_MSG_ERROR("File was already closed", 0, 0, 0);
         result = OMX_ErrorUndefined;
      }
      return result;
   }
} // namespace venctest

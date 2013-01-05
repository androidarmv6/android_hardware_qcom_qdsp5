/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/src/venctest_FileSink.cpp#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2010-06-12   as     Added support for serial mode encode.
2009-08-24   bc     On-target integration updates
2009-06-23   bc     Added support for zero-frames
2009-05-06   bc     Initial revision

========================================================================== */

/*========================================================================
  Include Files
 ==========================================================================*/
#include "venctest_ComDef.h"
#include "venctest_Debug.h"
#include "venctest_Thread.h"
#include "venctest_SignalQueue.h"
#include "venctest_File.h"
#include "venctest_Time.h"
#include "venctest_FileSink.h"
#include "venctest_Parser.h"
#include "venc_semaphore.h"

namespace venctest
{
   static const OMX_S32 MAX_BUFFER_ASSUME = 16;

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   FileSink::FileSink(void* aSemaphore, OMX_BOOL bIsProfileMode, OMX_BOOL bIsVTPath)
      : m_nFrames(0),
        m_pFile(),
        m_pBufferQueue(new SignalQueue(MAX_BUFFER_ASSUME, sizeof(OMX_BUFFERHEADERTYPE*))),
        m_pThread(new Thread()),
        m_bStarted(OMX_FALSE),
        m_pFrameReleaseFn(NULL),
        m_pSemaphore(aSemaphore),
        m_bIsProfileMode(bIsProfileMode),
        m_bIsVTPath(bIsVTPath)
   {
      VENC_TEST_MSG_LOW("created sink", 0, 0, 0);
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   FileSink::~FileSink()
   {
      if (m_pFile != NULL)
      {
         (void) m_pFile->Close();
         delete m_pFile;
      }
      if (m_pBufferQueue != NULL)
      {
         delete m_pBufferQueue;
      }
      if (m_pThread != NULL)
      {
         delete m_pThread;
      }
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE FileSink::Configure(OMX_S32 nFrames,
                                     OMX_STRING pFileName,
                                     OMX_S32 nTestNum,
                                     FrameReleaseFnType pFrameReleaseFn)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;
      if (nFrames >= 0 &&
          pFrameReleaseFn != NULL)
      {
         m_nFrames = nFrames;
         m_pFrameReleaseFn = pFrameReleaseFn;

         if (pFileName != NULL &&
             Parser::StringICompare("", pFileName) != 0)
         {
            (void) Parser::AppendTestNum(pFileName, nTestNum);

            m_pFile = new File();
            if (m_pFile != NULL)
            {
               VENC_TEST_MSG_MEDIUM("Opening output file...", 0, 0, 0);
               result = m_pFile->Open(pFileName, OMX_FALSE);
               if (result != OMX_ErrorNone)
               {
                  VENC_TEST_MSG_ERROR("Failed to open file", 0, 0, 0);
               }
            }
            else
            {
               VENC_TEST_MSG_ERROR("Failed to allocate file", 0, 0, 0);
               result = OMX_ErrorInsufficientResources;
            }
         }
         else
         {
            VENC_TEST_MSG_MEDIUM("No output file", 0, 0, 0);
         }
      }
      else
      {
         VENC_TEST_MSG_ERROR("Bad param(s)", 0, 0, 0);
         result = OMX_ErrorBadParameter;
      }
      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE FileSink::Write(OMX_BUFFERHEADERTYPE* pBuffer)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;

      if (pBuffer != NULL && pBuffer->pBuffer != NULL)
      {
         result = m_pBufferQueue->Push(&pBuffer,
                                       sizeof(OMX_BUFFERHEADERTYPE**));
         if (result != OMX_ErrorNone)
         {
            VENC_TEST_MSG_ERROR("failed to push buffer",0,0,0);
         }
      }
      else
      {
         VENC_TEST_MSG_ERROR("bad params",0,0,0);
         result = OMX_ErrorBadParameter;
      }
      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE FileSink::Start()
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;

      // make sure we've been configured
      if (m_nFrames >= 0)
      {
         VENC_TEST_MSG_MEDIUM("starting thread...",0,0,0);
         result = m_pThread->Start(SinkThreadEntry,  // thread fn
                                   this,             // thread data
                                   0);               // thread priority
         if (result == OMX_ErrorNone)
         {
            m_bStarted = OMX_TRUE;
         }
         else
         {
            VENC_TEST_MSG_ERROR("failed to start thread",0,0,0);
         }
      }
      else
      {
         VENC_TEST_MSG_ERROR("source has not been configured",0,0,0);
         result = OMX_ErrorUndefined;
      }
      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE FileSink::Finish()
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;

      if (m_bStarted == OMX_TRUE)
      {
         if (m_pThread != NULL)
         {
            OMX_ERRORTYPE threadResult;

            VENC_TEST_MSG_MEDIUM("waiting for thread to finish...",0,0,0);

            // wait for thread to exit
            result = m_pThread->Join(&threadResult);

            if (result == OMX_ErrorNone)
            {
               result = threadResult;
            }

            if (threadResult != OMX_ErrorNone)
            {
               VENC_TEST_MSG_ERROR("sink thread execution error", 0, 0, 0);
            }
         }

         m_bStarted = OMX_FALSE;
      }
      else
      {
         VENC_TEST_MSG_ERROR("already stopped",0,0,0);
         result = OMX_ErrorIncorrectStateTransition;
      }

      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE  FileSink::SinkThreadEntry(OMX_PTR pThreadData)
   {
      OMX_ERRORTYPE result = OMX_ErrorBadParameter;
      if (pThreadData)
      {
         result = ((FileSink*) pThreadData)->SinkThread();
      }
      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE FileSink::SinkThread()
   {
      OMX_BOOL bKeepGoing = OMX_TRUE;
      OMX_ERRORTYPE result = OMX_ErrorNone;

      VENC_TEST_MSG_MEDIUM("thread has started",0,0,0);

      for (OMX_S32 i = 0; i < m_nFrames && bKeepGoing == OMX_TRUE; i++)
      {

         OMX_BUFFERHEADERTYPE* pBuffer = NULL;

         result = m_pBufferQueue->Pop(&pBuffer,
                                      sizeof(pBuffer),
                                      0); // wait forever

         if (result == OMX_ErrorNone)
         {
            if (pBuffer != NULL)
            {
               OMX_S32 nBytes;

               if (m_pFile != NULL)
               {
                  if (pBuffer->nFilledLen > 0)
                  {
                     VENC_TEST_MSG_MEDIUM("writing frame %d with %d bytes...", i, (int) pBuffer->nFilledLen, 0);
                     result = m_pFile->Write(pBuffer->pBuffer,
                                             pBuffer->nFilledLen,
                                             &nBytes);

                     if (result != OMX_ErrorNone)
                     {
                        VENC_TEST_MSG_ERROR("error writing to file...",0,0,0);
                     }
                     else if (nBytes != pBuffer->nFilledLen)
                     {
                        VENC_TEST_MSG_ERROR("mismatched number of bytes in file write", 0, 0, 0);
                        result = OMX_ErrorUndefined;
                     }
                  }
                  else
                  {
                     VENC_TEST_MSG_HIGH("skipping frame %d...",i,0,0);
                  }

               }
               else
               {
                  VENC_TEST_MSG_MEDIUM("received frame %d...",i,0,0);
               }

               if (pBuffer->nFlags & OMX_BUFFERFLAG_CODECCONFIG)
               {
                  // this is just the syntax header, not a frame so increase loop count
                  VENC_TEST_MSG_HIGH("got codecconfig",i,0,0);
                  ++m_nFrames;
               }
               else
               {  
                  if (!m_bIsProfileMode)
                  {
                        if (m_pSemaphore)
                        {                   
                           if (venc_semaphore_post(m_pSemaphore) != 0)
                           {
                              VENC_TEST_MSG_ERROR("sem_t: error posting semaphore", 0, 0, 0);
                           }
                           else
                           {
                              VENC_TEST_MSG_LOW("sem_t: post() successful... buffer =%d",i,0,0);
                           }
                        }	                  
                  }
               }

               if (pBuffer->nFlags & OMX_BUFFERFLAG_EOS)
               {
                  // this is the last frame. note that we may get fewer frames
                  // than expected if RC is enabled with frame skips
                  VENC_TEST_MSG_HIGH("got eos",i,0,0);
                  bKeepGoing = OMX_FALSE;
               }

               m_pFrameReleaseFn(pBuffer);
            }
            else
            {
               VENC_TEST_MSG_ERROR("Buffer is null",0,0,0);
            }
         }
         else
         {
            VENC_TEST_MSG_ERROR("Error getting buffer",0,0,0);
            bKeepGoing = OMX_FALSE;
         }
      }
      VENC_TEST_MSG_HIGH("Sink thread is exiting...",0,0,0);
      return result;
   }

} // namespace venctest

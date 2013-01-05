/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/src/venctest_FileSource.cpp#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2010-06-12   as     Added support for serial mode encode.
2009-08-24   bc     On-target integration updates
2009-06-30   bc     Added support for DVS
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
#include "venctest_Sleeper.h"
#include "venctest_File.h"
#include "venctest_Time.h"
#include "venctest_FileSource.h"
#include "venc_semaphore.h"

namespace venctest
{
   static const OMX_S32 MAX_BUFFER_ASSUME = 16;

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   FileSource::FileSource(void* aSemaphore, OMX_BOOL bIsProfileMode, OMX_BOOL bIsVTPath)
      : m_nFrames(0),
        m_nFramesRegistered(0),
        m_nFramerate(0),
        m_nFrameWidth(0),
        m_nFrameHeight(0),
        m_nBuffers(0),
        m_nDVSXOffset(0),
        m_nDVSYOffset(0),
        m_pFile(NULL),
        m_pBufferQueue(new SignalQueue(MAX_BUFFER_ASSUME, sizeof(OMX_BUFFERHEADERTYPE*))),
        m_pThread(new Thread()),
        m_bStarted(OMX_FALSE),
        m_pFrameDeliverFn(NULL), 
        m_pSemaphore(aSemaphore),
        m_bIsProfileMode(bIsProfileMode), 
        m_bIsVTPath(bIsVTPath)
   {
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   FileSource::~FileSource()
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
   OMX_ERRORTYPE FileSource::Configure(OMX_S32 nFrames,
                                       OMX_S32 nFramerate,
                                       OMX_S32 nFrameWidth,
                                       OMX_S32 nFrameHeight,
                                       OMX_S32 nBuffers,
                                       FrameDeliveryFnType pFrameDeliverFn,
                                       OMX_STRING pFileName,
                                       OMX_S32 nDVSXOffset,
                                       OMX_S32 nDVSYOffset,
                                       OMX_BOOL bProfileMode)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;

      if (nFrames >= 0 &&
          nFramerate > 0 &&
          nFrameWidth > 0 &&
          nFrameHeight > 0 &&
          nDVSXOffset >= 0 &&
          nDVSYOffset >= 0 &&
          nBuffers > 0 &&
          nBuffers <= MAX_BUFFER_ASSUME &&
          pFrameDeliverFn != NULL)
      {
         m_nFrames = nFrames;
         m_nFramerate = nFramerate;
         m_nFrameWidth = nFrameWidth;
         m_nFrameHeight = nFrameHeight;
         m_nBuffers = nBuffers;
         m_nDVSXOffset = nDVSXOffset;
         m_nDVSYOffset = nDVSYOffset;
         m_pFrameDeliverFn = pFrameDeliverFn;
         m_bIsProfileMode = bProfileMode;

         if (pFileName != NULL)
         {
            m_pFile = new File();
            if (m_pFile != NULL)
            {
               result = m_pFile->Open(pFileName, OMX_TRUE);
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
   OMX_ERRORTYPE FileSource::ChangeFrameRate(OMX_S32 nFramerate)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;
      if (nFramerate > 0)
      {
         m_nFramerate = nFramerate;
      }
      else
      {
         VENC_TEST_MSG_ERROR("bad frame rate", 0, 0, 0);
         result = OMX_ErrorBadParameter;
      }
      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE FileSource::Start()
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;

      // make sure we've been configured
      if (m_nFrames >= 0 &&
          m_nFramerate >= 0 &&
          m_nBuffers > 0)
      {
         if (m_nFramesRegistered == m_nBuffers)
         {

            VENC_TEST_MSG_MEDIUM("starting thread...",0,0,0);

            result = m_pThread->Start(SourceThreadEntry,  // thread fn
                                      this,               // thread data
                                      0);                 // thread priority

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
            VENC_TEST_MSG_ERROR("need to register all buffers with the source",0,0,0);
            result = OMX_ErrorUndefined;
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
   OMX_ERRORTYPE FileSource::Finish()
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
               VENC_TEST_MSG_ERROR("source thread execution error", 0, 0, 0);
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
   OMX_ERRORTYPE FileSource::SetFreeBuffer(OMX_BUFFERHEADERTYPE* pBuffer)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;

      if (pBuffer != NULL && pBuffer->pBuffer != NULL)
      {
         // if we have not started then the client is registering buffers
         if (m_bStarted == OMX_FALSE)
         {
            // need to fill the buffer with YUV data upon registration
            if (m_nFramesRegistered < m_nBuffers &&
                m_bIsProfileMode == OMX_TRUE)
            {

               VENC_TEST_MSG_MEDIUM("register buffer",0,0,0);

               if (m_pFile)
               {
                  OMX_S32 nFrameBytes = m_nFrameWidth * m_nFrameHeight * 3 / 2;
                  OMX_S32 nBytesRead;
                  result = m_pFile->Read(pBuffer->pBuffer,
                                         nFrameBytes,
                                         &nBytesRead);
                  if (result != OMX_ErrorNone ||
                      nBytesRead != nFrameBytes)
                  {
                     VENC_TEST_MSG_HIGH("yuv file is too small. seeking to start.",
                           0, 0, 0);
                     result = m_pFile->SeekStart(0);
                     result = m_pFile->Read(pBuffer->pBuffer,
                                            nFrameBytes,
                                            &nBytesRead);
                  }
               }
            }

            ++m_nFramesRegistered;
         }
         result = m_pBufferQueue->Push(&pBuffer,
                                       sizeof(OMX_BUFFERHEADERTYPE**));
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
   OMX_ERRORTYPE FileSource::SourceThreadEntry(OMX_PTR pThreadData)
   {
      OMX_ERRORTYPE result = OMX_ErrorBadParameter;
      if (pThreadData)
      {
         result = ((FileSource*) pThreadData)->SourceThread();
      }
      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE FileSource::SourceThread()
   {
      OMX_BOOL bKeepGoing = OMX_TRUE;
      OMX_ERRORTYPE result = OMX_ErrorNone;
      OMX_TICKS nTimeStamp = 0;

      VENC_TEST_MSG_MEDIUM("thread has started",0,0,0);

      for (OMX_S32 i = 0; i < m_nFrames && bKeepGoing == OMX_TRUE; i++)
      {

         OMX_BUFFERHEADERTYPE* pBuffer = NULL;
         const OMX_S32 nFrameBytes = m_nFrameWidth * m_nFrameHeight * 3 / 2;

         // Since frame rate can change at any time, let's make sure that we use
         // the same frame rate for the duration of this loop iteration
         OMX_S32 nFramerate = m_nFramerate;

         // If in live mode we deliver frames in a real-time fashion
         if (m_bIsProfileMode == OMX_TRUE)
         {
            Sleeper::Sleep(1000 / nFramerate);

            if (m_pBufferQueue->GetSize() <= 0)
            {
               VENC_TEST_MSG_MEDIUM("No buffers. Block until buffer available...", 0, 0, 0);
            }

            VENC_TEST_MSG_MEDIUM("Wait for free buffer...",0,0,0);

            result = m_pBufferQueue->Pop(&pBuffer,
                                         sizeof(pBuffer),
                                         0); // wait forever
         }

         // if not in live mode, we deliver frames as they become available
         else
         {
            result = m_pBufferQueue->Pop(&pBuffer,
                                         sizeof(pBuffer),
                                         0); // wait forever
            if (m_pFile != NULL)
            {
                  OMX_S32 nBytesRead;
                  result = m_pFile->Read(pBuffer->pBuffer,
                                         nFrameBytes,
                                         &nBytesRead);
                  if (result != OMX_ErrorNone ||
                      nBytesRead != nFrameBytes)
                  {
                     VENC_TEST_MSG_HIGH("yuv file is too small. seeking to start.",
                           0, 0, 0);
                     result = m_pFile->SeekStart(0);
                     result = m_pFile->Read(pBuffer->pBuffer,
                                            nFrameBytes,
                                            &nBytesRead);
                  }
            }
         }

         if (result == OMX_ErrorNone)
         {
            if (pBuffer != NULL)
            {
               VENC_TEST_MSG_MEDIUM("delivering frame %d...",i,0,0);

	      if (m_bIsProfileMode == OMX_TRUE)
               {
                  nTimeStamp = (OMX_TICKS) Time::GetTimeMicrosec();
               }
               else
               {
                  nTimeStamp = nTimeStamp + (OMX_TICKS) (1000000 / nFramerate);
               }

               pBuffer->nFilledLen = nFrameBytes;
               pBuffer->nTimeStamp = nTimeStamp;

               // set the EOS flag if this is the last frame
               pBuffer->nFlags = 0;
               if (i == m_nFrames - 1)
               {
                  pBuffer->nFlags = OMX_BUFFERFLAG_EOS;
               }

               pBuffer->nOffset = ((m_nFrameWidth * m_nDVSYOffset) + m_nDVSXOffset) * 3 / 2;
               m_pFrameDeliverFn(pBuffer);
               if(!m_bIsProfileMode)
                 { 
                  if (m_bIsVTPath) 
                  {
                     if (m_pSemaphore)
                     {
                        if (venc_semaphore_wait(m_pSemaphore, 0) != 0)
                        {
                           VENC_TEST_MSG_ERROR("sem_t: Source; error waiting for vt semaphore", 0, 0, 0);
                        }
                        else
                        {
                           VENC_TEST_MSG_LOW("Sem_t: Source; VT sem_wait ... buffer =%d",i+1,0,0);
                        }
                     }
                     else
                     {
                        VENC_TEST_MSG_ERROR("sem_t: Source; semaphore is NULL", 0, 0, 0);
                     }
                  }
                  else
                  {
                     // Take semaphore for camcoder path after first frame
                     if ( i > 0 ) 
                     {
                        if (m_pSemaphore)
                        {
                           if (venc_semaphore_wait(m_pSemaphore, 0) != 0)
                           {
                              VENC_TEST_MSG_ERROR("sem_t: Source; error waiting for cam semaphore", 0, 0, 0);
                           }
                           else
                           {
                              VENC_TEST_MSG_LOW("Sem_t: Source; Cam sem_wait ... buffer =%d",i+1,0,0);
                           }
                        } // end of if (m_pSemaphore)                  
                      } // end of if (i>0)
                  } // end of if(m_bIsVTPath)
                 } // end of if(m_bIsProfileMode)
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
      VENC_TEST_MSG_HIGH("Source thread is exiting...",0,0,0);
      return result;
   }
} // namespace venctest

#ifndef _VENC_TEST_FILE_SOURCE_H
#define _VENC_TEST_FILE_SOURCE_H
/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/inc/venctest_FileSource.h#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2010-06-12   as     Added support for serial mode encode.
2009-06-30   bc     Added support for DVS
2009-04-17   bc     Initial revision

 ========================================================================== */

/*========================================================================

                     INCLUDE FILES FOR MODULE

==========================================================================*/
#include "OMX_Core.h"

namespace venctest
{
   class File;          // forward declaration
   class SignalQueue;   // forward declaration
   class Thread;        // forward declaration

   /**
    * @brief Delivers YUV data in two different modes.
    *
    * In live mode, buffers are pre-populated with YUV data at the time
    * of configuration. The source will loop through and deliver the
    * pre-populated buffers throughout the life of the session. Frames will be
    * delivered at the configured frame rate. This mode is useful for gathering
    * performance statistics as no file reads are performed at run-time.
    *
    * In  non-live mode, buffers are populated with YUV data at run time.
    * Buffers are delivered downstream as they become available. Timestamps
    * are based on the configured frame rate, not on the system clock.
    *
    */
   class FileSource
   {
      public:

         /**
          * @brief Frame callback for YUV buffer deliver
          */
         typedef void (*FrameDeliveryFnType) (OMX_BUFFERHEADERTYPE* pBuffer);

      public:

         /**
          * @brief Constructor
          */
         FileSource(void* aSemaphore = NULL, OMX_BOOL bIsProfileMode = OMX_TRUE, OMX_BOOL bIsVTPath = OMX_FALSE);				 

         /**
          * @brief Destructor
          */
         ~FileSource();

      public:

         /**
          * @brief Configures the source
          *
          * @param nFrames The number of frames to to play.
          * @param nFramerate The frame rate to emulate.
          * @param nFrameWidth The frame width.
          * @param nFrameHeight The frame height.
          * @param nBuffers The number of buffers allocated for the session.
          * @param pFrameDeliverFn Frame delivery callback.
          * @param pFileName Name of the file to read from
          * @param nDVSXOffset Smaller frame pixel offset in the x direction
          * @param nDVSYOffset Smaller frame pixel offset in the y direction
          * @param bEnableLiveMode If true will encode in real time.
          */
         OMX_ERRORTYPE Configure(OMX_S32 nFrames,
                                 OMX_S32 nFramerate,
                                 OMX_S32 nFrameWidth,
                                 OMX_S32 nFrameHeight,
                                 OMX_S32 nBuffers,
                                 FrameDeliveryFnType pFrameDeliverFn,
                                 OMX_STRING pFileName,
                                 OMX_S32 nDVSXOffset,
                                 OMX_S32 nDVSYOffset,
                                 OMX_BOOL bProfileMode);

         /**
          * @brief Changes the frame rate
          *
          * The frame rate will take effect immediately.
          *
          * @param nFramerate The new frame rate.
          */
         OMX_ERRORTYPE ChangeFrameRate(OMX_S32 nFramerate);

         /**
          * @brief Starts the delivery of buffers.
          *
          * All buffers should be registered through the SetFreeBuffer function
          * prior to calling this function.
          *
          * Source will continue to deliver buffers at the specified
          * rate until the specified number of frames have been delivered.
          *
          * Note that Source will not deliver buffers unless it has ownership
          * of atleast 1 buffer. If Source does not have ownership of buffer when
          * timer expires, it will block until a buffer is given to the component.
          * Free buffers should be given to the Source through SetFreeBuffer
          * function.
          */
         OMX_ERRORTYPE Start();

         /**
          * @brief Wait for the thread to finish.
          *
          * Function will block until the Source has delivered all frames.
          */
         OMX_ERRORTYPE Finish();

         /**
          * @brief Gives ownership of the buffer to the source.
          *
          * All bufffers must be registered with the Source prior
          * to invoking Start. This will give initial ownership to
          * the source.
          *
          * After Start is called, buffers must be given ownership
          * when yuv buffers are free.
          *
          * @param pBuffer Pointer to the buffer
          */
         OMX_ERRORTYPE SetFreeBuffer(OMX_BUFFERHEADERTYPE* pBuffer);

      private:
         static OMX_ERRORTYPE SourceThreadEntry(OMX_PTR pThreadData);
         OMX_ERRORTYPE SourceThread();

      private:
         OMX_S32 m_nFrames;
         OMX_S32 m_nFramesRegistered;
         OMX_S32 m_nFramerate;
         OMX_S32 m_nFrameWidth;
         OMX_S32 m_nFrameHeight;
         OMX_S32 m_nBuffers;
         OMX_S32 m_nDVSXOffset;
         OMX_S32 m_nDVSYOffset;
         File* m_pFile;
         SignalQueue* m_pBufferQueue;
         Thread* m_pThread;
         OMX_BOOL m_bStarted;
         FrameDeliveryFnType m_pFrameDeliverFn;
         void * m_pSemaphore; 
         OMX_BOOL m_bIsProfileMode;
         OMX_BOOL m_bIsVTPath;
   };

} // namespace venctest

#endif // #ifndef _VENC_TEST_FILE_SOURCE_H

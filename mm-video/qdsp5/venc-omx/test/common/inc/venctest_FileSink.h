#ifndef _VENC_TEST_FILE_SINK_H
#define _VENC_TEST_FILE_SINK_H
/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/inc/venctest_FileSink.h#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2010-06-12   as     Added support for serial mode encode.
2009-04-17   bc     Initial revision

 ========================================================================== */

/*========================================================================

                     INCLUDE FILES FOR MODULE

==========================================================================*/
#include "OMX_Core.h"

namespace venctest
{

   class SignalQueue;   // forward declaration

   /**
    * @brief Writes encoded bitstream to file.
    *
    * Frames are written to file asynchronously as they are delivered.
    */
   class FileSink
   {
      public:

         /**
          * @brief Frame callback for bitstream buffer release
          */
         typedef void (*FrameReleaseFnType) (OMX_BUFFERHEADERTYPE* pBuffer);

      public:

         /**
          * @brief Constructor
          */
         FileSink(void*  aSemaphore = NULL, OMX_BOOL bIsProfileMode = OMX_TRUE, OMX_BOOL bIsVTPath = OMX_FALSE);

         /**
          * @brief Destructor
          */
         ~FileSink();

      public:

         /**
          * @brief Configure the file writer
          *
          * @param nFrames The number of frames to write. Thread exits when nFrames are written.
          * @param pFileName The name of the output file. Specify NULL for no file I/O.
          * @param nTestNum The test number
          * @param pFrameReleaseFn The callback to invoke when after frames are written
          */
         OMX_ERRORTYPE Configure(OMX_S32 nFrames,
                                 OMX_STRING pFileName,
                                 OMX_S32 nTestNum,
                                 FrameReleaseFnType pFrameReleaseFn);

         /**
          * @brief Asynchronously write an encoded frame to file.
          *
          * @param pBuffer The frame to write.
          */
         OMX_ERRORTYPE Write(OMX_BUFFERHEADERTYPE* pBuffer);

         /**
          * @brief Starts the writer thread.
          *
          */
         OMX_ERRORTYPE Start();

         /**
          * @brief Wait for writer thread to finish.
          *
          * Function will block until the Sink has received and written all frames.
          */
         OMX_ERRORTYPE Finish();

      private:
         static OMX_ERRORTYPE SinkThreadEntry(OMX_PTR pThreadData);
         OMX_ERRORTYPE SinkThread();

      private:
         OMX_S32 m_nFrames;
         File* m_pFile;
         SignalQueue* m_pBufferQueue;
         Thread* m_pThread;
         OMX_BOOL m_bStarted;
         FrameReleaseFnType m_pFrameReleaseFn;
         void * m_pSemaphore;
         OMX_BOOL m_bIsProfileMode;
         OMX_BOOL m_bIsVTPath;
   };

} // namespace venctest

#endif // #ifndef _VENC_TEST_FILE_SINK_H

#ifndef _VENC_TEST_ENCODER_H
#define _VENC_TEST_ENCODER_H
/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/inc/venctest_Encoder.h#3 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2010-06-12   as     Added support for Dynamic configuration for IntraRefreshMB
2010-03-24   as     Added OMX extension for encoder mode.
2009-08-13   bc     Moved stats thread to device layer
2009-07-14   bc     Updated documentation
2009-07-09   bc     Added error handling for async commands
2009-04-17   bc     Initial revision

 ========================================================================== */

/*========================================================================

                     INCLUDE FILES FOR MODULE

==========================================================================*/
#include "OMX_Core.h"
#include "venctest_ComDef.h"

namespace venctest
{

   class Signal;        // forward declaration
   class SignalQueue;   // forward declaration

   /**
    * @brief Class that wraps/simplifies OMX IL encoder component interactions.
    */
   class Encoder
   {
      public:

         /**
          * @brief Event cb type. Refer to OMX IL spec for param details.
          */
         typedef OMX_ERRORTYPE (*EventCallbackType)(
                                OMX_IN OMX_HANDLETYPE hComponent,
                                OMX_IN OMX_PTR pAppData,
                                OMX_IN OMX_EVENTTYPE eEvent,
                                OMX_IN OMX_U32 nData1,
                                OMX_IN OMX_U32 nData2,
                                OMX_IN OMX_PTR pEventData);

         /**
          * @brief Empty buffer done cb type. Refer to OMX IL spec for param details.
          */
         typedef OMX_ERRORTYPE (*EmptyDoneCallbackType)(
                                OMX_IN OMX_HANDLETYPE hComponent,
                                OMX_IN OMX_PTR pAppData,
                                OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);

         /**
          * @brief Fill buffer done cb type. Refer to OMX IL spec for param details.
          */
         typedef OMX_ERRORTYPE (*FillDoneCallbackType)(
                                OMX_OUT OMX_HANDLETYPE hComponent,
                                OMX_OUT OMX_PTR pAppData,
                                OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer);
      public:

         /**
          * @brief Constructor
          *
          * @param pEmptyDoneFn Empty buffer done callback. Refer to OMX IL spec
          * @param pFillDoneFn Fill buffer done callback. Refer to OMX IL spec
          * @param pAppData Client data passed to buffer and event callbacks
          * @param eCodec The codec
          */
         Encoder(EmptyDoneCallbackType pEmptyDoneFn,
                 FillDoneCallbackType pFillDoneFn,
                 OMX_PTR pAppData,
                 OMX_VIDEO_CODINGTYPE eCodec);

         /**
          * @brief Destructor
          */
         ~Encoder();

      private:

         /**
          * @brief Private default constructor. Use public constructor.
          */
         Encoder();


      public:

         ///////////////////////////////////////////////////////////
         ///////////////////////////////////////////////////////////
         // Init time methods
         ///////////////////////////////////////////////////////////
         ///////////////////////////////////////////////////////////

         /**
          * @brief Configure the encoder
          *
          * @param pConfig The encoder configuration
          */
         OMX_ERRORTYPE Configure(EncoderConfigType* pConfig);

         /**
          * @brief Synchronously transitions the encoder to OMX_StateExecuting.
          *
          * Only valid in OMX_StateLoaded;
          */
         OMX_ERRORTYPE GoToExecutingState();

         /**
          * @brief Get the out of band syntax header
          *
          * Only valid in OMX_StateLoaded;
          */
         OMX_ERRORTYPE GetOutOfBandSyntaxHeader(OMX_U8* pSyntaxHdr,
                                                OMX_S32 nSyntaxHdrLen,
                                                OMX_S32* pSyntaxHdrFilledLen);

         ///////////////////////////////////////////////////////////
         ///////////////////////////////////////////////////////////
         // Run time methods
         ///////////////////////////////////////////////////////////
         ///////////////////////////////////////////////////////////

         /**
          * @brief Synchronously transitions the encoder to OMX_StateLoaded.
          *
          * Only valid in OMX_StateExecuting;
          */
         OMX_ERRORTYPE GoToLoadedState();

         /**
          * @brief Deliver an input (yuv) buffer to the encoder.
          *
          * @param pBuffer The populated input buffer.
          */
         OMX_ERRORTYPE DeliverInput(OMX_BUFFERHEADERTYPE* pBuffer);

         /**
          * @brief Deliver an output (bitstream) buffer to the encoder.
          *
          * @param pBuffer The un-populated output buffer.
          */
         OMX_ERRORTYPE DeliverOutput(OMX_BUFFERHEADERTYPE* pBuffer);

         /**
          * @brief Get array of input or output buffer pointers header.
          *
          * Only valid in the executing state after all buffers have been allocated.
          *
          * @param bIsInput Set to OMX_TRUE for input OMX_FALSE for output.
          * @return NULL upon failure, array of buffer header pointers otherwise.
          */
         OMX_BUFFERHEADERTYPE** GetBuffers(OMX_BOOL bIsInput);

         /**
          * @brief Request for an iframe to be generated.
          */
         OMX_ERRORTYPE RequestIntraVOP();

         /**
          * @brief Set the intra period. It is valid to change this configuration at run-time
          *
          * @param nIntraPeriod The iframe interval in units of frames
          */
         OMX_ERRORTYPE SetIntraPeriod(OMX_S32 nIntraPeriod);
         /**
          * @brief Set the intra Refresh MB's. It is valid to change this configuration at run-time
          *
          * @param nIntraMBs The intra  MB's in a p Frame
          */
         OMX_ERRORTYPE SetIntraRefresh(OMX_S32 nIntraMBs);
         /**
          * @brief Change the encoding quality
          *
          * @param nFramerate The updated frame rate
          * @param nBitrate The updated bitrate
          * @param nMinQp The updated min qp
          * @param nMaxQp The updated max qp
          */
         OMX_ERRORTYPE ChangeQuality(OMX_S32 nFramerate,
                                     OMX_S32 nBitrate,
                                     OMX_S32 nMinQp,
                                     OMX_S32 nMaxQp,
				     OMX_S32 nIntraPeriod,
				     OMX_S32 nIntraRefreshMBs);

         /**
          * @brief Set the encoder state
          *
          * This method can be asynchronous or synchronous. If asynchonous,
          * WaitState can be called to wait for the corresponding state
          * transition to complete.
          *
          * @param eState The state to enter
          * @param bSynchronous If OMX_TRUE, synchronously wait for the state transition to complete
          */
         OMX_ERRORTYPE SetState(OMX_STATETYPE eState,
                                OMX_BOOL bSynchronous);

         /**
          * @brief Wait for the corresponding state transition to complete
          *
          * @param eState The state to wait for
          */
         OMX_ERRORTYPE WaitState(OMX_STATETYPE eState);

         /**
          * @brief Allocate all input and output buffers
          */
         OMX_ERRORTYPE AllocateBuffers();

         /**
          * @brief Free all input and output buffers
          */
         OMX_ERRORTYPE FreeBuffers();

         /**
          * @brief Flush the encoder
          */
         OMX_ERRORTYPE Flush();

	 /**
          * @brief Set the EncoderMode
          */
         OMX_ERRORTYPE SetEncoderMode(); 
         
      private:

         static OMX_ERRORTYPE EventCallback(OMX_IN OMX_HANDLETYPE hComponent,
                                            OMX_IN OMX_PTR pAppData,
                                            OMX_IN OMX_EVENTTYPE eEvent,
                                            OMX_IN OMX_U32 nData1,
                                            OMX_IN OMX_U32 nData2,
                                            OMX_IN OMX_PTR pEventData);

         static OMX_ERRORTYPE EmptyDoneCallback(OMX_IN OMX_HANDLETYPE hComponent,
                                                OMX_IN OMX_PTR pAppData,
                                                OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);

         static OMX_ERRORTYPE FillDoneCallback(OMX_IN OMX_HANDLETYPE hComponent,
                                               OMX_IN OMX_PTR pAppData,
                                               OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);

      private:
         SignalQueue* m_pSignalQueue;
         EncoderConfigType m_sConfig;
         EventCallbackType m_pEventFn;
         EmptyDoneCallbackType m_pEmptyDoneFn;
         FillDoneCallbackType m_pFillDoneFn;
         OMX_PTR m_pAppData;
         OMX_BUFFERHEADERTYPE** m_pInputBuffers;
         OMX_BUFFERHEADERTYPE** m_pOutputBuffers;
         OMX_HANDLETYPE m_hEncoder;
         OMX_STATETYPE m_eState;
         OMX_S32 m_nInputBuffers;
         OMX_S32 m_nOutputBuffers;
         OMX_S32 m_nInputBufferSize;
         OMX_S32 m_nOutputBufferSize;
         OMX_VIDEO_CODINGTYPE m_eCodec;
         OMX_S32 m_nInFrameIn;
         OMX_S32 m_nOutFrameIn;
         OMX_S32 m_nInFrameOut;
         OMX_S32 m_nOutFrameOut;


   };

} // namespace venctest

#endif // #ifndef  _VENC_TEST_ENCODER_H

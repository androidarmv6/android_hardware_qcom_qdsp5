#ifndef _VENC_TEST_CHANGE_QUALITY_H
#define _VENC_TEST_CHANGE_QUALITY_H
/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/inc/venctest_TestChangeQuality.h#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2009-04-17   bc     Initial revision

 ========================================================================== */

/*========================================================================

                     INCLUDE FILES FOR MODULE

==========================================================================*/
#include "OMX_Core.h"
#include "venctest_ITestCase.h"

namespace venctest
{
   class FileSource;       // forward declaration
   class FileSink;         // forward declaration
   class Encoder;          // forward declaration
   class Mutex;            // forward declaration

   /**
    * @brief Test case for dynamic quality configuration
    */
   class TestChangeQuality : public ITestCase
   {
      public:

         /**
          * @brief Constructor
          */
         TestChangeQuality() ;

         /**
          * @brief Destructor
          */
         virtual ~TestChangeQuality();

      private:

         virtual OMX_ERRORTYPE Execute(EncoderConfigType* pConfig,
                                       DynamicConfigType* pDynamicConfig,
                                       OMX_S32 nTestNum);

         virtual OMX_ERRORTYPE ValidateAssumptions(EncoderConfigType* pConfig,
                                                   DynamicConfigType* pDynamicConfig);

         static void SourceDeliveryFn(OMX_BUFFERHEADERTYPE* pBuffer);

         static void SinkReleaseFn(OMX_BUFFERHEADERTYPE* pBuffer);

         static OMX_ERRORTYPE EBD(OMX_IN OMX_HANDLETYPE hComponent,
                                  OMX_IN OMX_PTR pAppData,
                                  OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);

         static OMX_ERRORTYPE FBD(OMX_IN OMX_HANDLETYPE hComponent,
                                  OMX_IN OMX_PTR pAppData,
                                  OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);

      private:
         FileSource* m_pSource;
         FileSink* m_pSink;
         Encoder* m_pEncoder;
         Mutex* m_pMutex;
         OMX_S32 m_nFramesCoded;
         OMX_S32 m_nFramesDelivered;
         OMX_S64 m_nBits;
   };

} // namespace venctest

#endif // #ifndef _VENC_TEST_CHANGE_QUALITY_H

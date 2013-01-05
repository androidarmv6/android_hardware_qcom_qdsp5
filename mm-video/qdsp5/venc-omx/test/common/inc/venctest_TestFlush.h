#ifndef _VENC_TEST_FLUSH_H
#define _VENC_TEST_FLUSH_H
/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/inc/venctest_TestFlush.h#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2009-07-14   bc     Updated documentation
2009-04-17   bc     Initial revision

 ========================================================================== */

/*========================================================================

                     INCLUDE FILES FOR MODULE

==========================================================================*/
#include "OMX_Core.h"
#include "venctest_ITestCase.h"
#include "venctest_ComDef.h"

namespace venctest
{
   class Encoder;          // forward declaration
   class Queue;            // forward declaration
   class File;             // forward declaration

   /**
    * @brief Test case that for flushing. A stress test.
    */
   class TestFlush : public ITestCase
   {
      public:

         /**
          * @brief Constructor
          */
         TestFlush() ;

         /**
          * @brief Destructor
          */
         virtual ~TestFlush();

      private:

         virtual OMX_ERRORTYPE Execute(EncoderConfigType* pConfig,
                                       DynamicConfigType* pDynamicConfig,
                                       OMX_S32 nTestNum);

         virtual OMX_ERRORTYPE ValidateAssumptions(EncoderConfigType* pConfig,
                                                   DynamicConfigType* pDynamicConfig);

         OMX_ERRORTYPE DeliverBuffers(OMX_BOOL bDeliverInput,
                                      OMX_BOOL bDeliverOutput,
                                      EncoderConfigType* pConfig);

         OMX_ERRORTYPE CheckBufferQueues(OMX_S32 nInputBuffers,
                                         OMX_S32 nOutputBuffers);

         static OMX_ERRORTYPE EBD(OMX_IN OMX_HANDLETYPE hComponent,
                                  OMX_IN OMX_PTR pAppData,
                                  OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);

         static OMX_ERRORTYPE FBD(OMX_IN OMX_HANDLETYPE hComponent,
                                  OMX_IN OMX_PTR pAppData,
                                  OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);

      private:
         Encoder* m_pEncoder;
         Queue* m_pInputQueue;
         Queue* m_pOutputQueue;
         File* m_pSource;
         File* m_pSink;
         OMX_TICKS m_nTimeStamp;
   };

} // namespace venctest

#endif // #ifndef _VENC_TEST_FLUSH_H

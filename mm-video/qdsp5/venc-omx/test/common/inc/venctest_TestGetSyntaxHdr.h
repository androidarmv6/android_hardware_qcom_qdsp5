#ifndef _VENC_TEST_GET_SYNTAX_HDR
#define _VENC_TEST_GET_SYNTAX_HDR
/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/inc/venctest_TestGetSyntaxHdr.h#1 $

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

namespace venctest
{
   class Encoder;          // forward declaration
   class SignalQueue;      // forward declaration

   /**
    * @brief Test case for in-band and out-of-band syntax header.
    */
   class TestGetSyntaxHdr : public ITestCase
   {
      public:

         /**
          * @brief Constructor
          */
         TestGetSyntaxHdr() ;

         /**
          * @brief Destructor
          */
         virtual ~TestGetSyntaxHdr();

      private:

         virtual OMX_ERRORTYPE Execute(EncoderConfigType* pConfig,
                                       DynamicConfigType* pDynamicConfig,
                                       OMX_S32 nTestNum);

         virtual OMX_ERRORTYPE ValidateAssumptions(EncoderConfigType* pConfig,
                                                   DynamicConfigType* pDynamicConfig);

         OMX_ERRORTYPE GetInBandSyntaxHeader(OMX_BUFFERHEADERTYPE* pBuffer);

         static OMX_ERRORTYPE EBD(OMX_IN OMX_HANDLETYPE hComponent,
                                  OMX_IN OMX_PTR pAppData,
                                  OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);

         static OMX_ERRORTYPE FBD(OMX_IN OMX_HANDLETYPE hComponent,
                                  OMX_IN OMX_PTR pAppData,
                                  OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);

      private:
         Encoder* m_pEncoder;
         SignalQueue* m_pSignalQueue;
   };

} // namespace venctest

#endif // #ifndef _VENC_TEST_GET_SYNTAX_HDR

#ifndef _VENC_TEST_CONFIG_H
#define _VENC_TEST_CONFIG_H
/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/inc/venctest_Config.h#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2009-04-17   bc     Initial revision

 ========================================================================== */

/*========================================================================

                     INCLUDE FILES FOR MODULE

==========================================================================*/
#include "OMX_Core.h"
#include "venctest_ComDef.h"


namespace venctest
{
   class File; // forward declaration

   /**
    * @brief Class for configuring video encoder and video encoder test cases
    */
   class Config
   {
      public:

         /**
          * @brief Constructor
          */
         Config();

         /**
          * @brief Destructor
          */
         ~Config();

         /**
          * @brief Parses the config file obtaining the configuration
          *
          * @param pFileName The name of the config file
          * @param pEncoderConfig The encoder config.
          * @param pDynamicConfig The dynamic encoder config. Optional (NULL is valid).
          */
         OMX_ERRORTYPE Parse(OMX_STRING pFileName,
                             EncoderConfigType* pEncoderConfig,
                             DynamicConfigType* pDynamicConfig);

         /**
          * @brief Gets the current or default encoder configuration.
          *
          * @param pEncoderConfig The configuration
          */
         OMX_ERRORTYPE GetEncoderConfig(EncoderConfigType* pEncoderConfig);

         /**
          * @brief Gets the current or default dynamic encoder configuration.
          *
          * @param pDynamicConfig The configuration
          */
         OMX_ERRORTYPE GetDynamicConfig(DynamicConfigType* pDynamicConfig);

      private:
         EncoderConfigType m_sEncoderConfig;
         DynamicConfigType m_sDynamicConfig;
   };
} // namespace venctest

#endif // #ifndef _VENC_TEST_CONFIG_H

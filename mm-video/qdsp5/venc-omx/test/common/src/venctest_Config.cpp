/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/src/venctest_Config.cpp#2 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2010-06-12   as     Added support for Dynamic configuration for IntraRefreshMB
2010-04-02   as     Added support for TimeStamp scale factor
2009-06-30   bc     Added support for DVS
2009-05-06   bc     Initial revision

========================================================================== */

/*========================================================================
  Include Files
 ==========================================================================*/
#include "venctest_ComDef.h"
#include "venctest_Debug.h"
#include "venctest_Config.h"
#include "venctest_Parser.h"
#include "venctest_File.h"
#include <stdlib.h>

namespace venctest
{
   struct ConfigEnum
   {
      const OMX_STRING pEnumName;
      OMX_S32 eEnumVal;
   };

   // Codecs
   static ConfigEnum g_pCodecEnums[] =
   {
      {"MP4",  (int) OMX_VIDEO_CodingMPEG4},
      {"H263", (int) OMX_VIDEO_CodingH263},
      {"H264", (int) OMX_VIDEO_CodingAVC},
      {0, 0}
   };

   // Rate control flavors
   static ConfigEnum g_pVencRCEnums[] =
   {
      {"RC_OFF",     (int) OMX_Video_ControlRateDisable},
      {"RC_VBR_VFR", (int) OMX_Video_ControlRateVariableSkipFrames},       // Camcorder
      {"RC_VBR_CFR", (int) OMX_Video_ControlRateVariable},                 // Camcorder
      {"RC_CBR_VFR", (int) OMX_Video_ControlRateConstantSkipFrames},       // QVP
      {0, 0}
   };

   // Resync marker types
   static ConfigEnum m_pResyncMarkerType[] =
   {
      {"NONE",        (int) RESYNC_MARKER_NONE},
      {"BITS",        (int) RESYNC_MARKER_BITS},
      {"MB",          (int) RESYNC_MARKER_MB},
      {"GOB",         (int) RESYNC_MARKER_GOB},
      {0, 0}
   };


   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_S32 ParseEnum(ConfigEnum* pConfigEnum,
                     OMX_STRING pEnumName)
   {
      OMX_S32 idx = 0;
      while (pConfigEnum[idx].pEnumName)
      {
         if (Parser::StringICompare(pEnumName,
                                    pConfigEnum[idx].pEnumName) == 0)
         {
            return pConfigEnum[idx].eEnumVal;
         }
         idx++;
      }
      VENC_TEST_MSG_ERROR("error could not find enum", 0,0,0);
      return -1;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   Config::Config()
   {
      static const OMX_STRING pInFileName = "";

      // set some default values
      memset(&m_sEncoderConfig, 0, sizeof(m_sEncoderConfig));
      m_sEncoderConfig.eCodec = OMX_VIDEO_CodingMPEG4;
      m_sEncoderConfig.eControlRate = OMX_Video_ControlRateDisable;
      m_sEncoderConfig.nFrameWidth = 640;
      m_sEncoderConfig.nFrameHeight = 480;
      m_sEncoderConfig.nOutputFrameWidth = m_sEncoderConfig.nFrameWidth;
      m_sEncoderConfig.nOutputFrameHeight = m_sEncoderConfig.nFrameHeight;
      m_sEncoderConfig.nDVSXOffset = 0;
      m_sEncoderConfig.nDVSYOffset = 0;
      m_sEncoderConfig.nBitrate = 768000;
      m_sEncoderConfig.nFramerate = 30;
      m_sEncoderConfig.nTimeIncRes = 30;
      m_sEncoderConfig.nRotation = 0;
      m_sEncoderConfig.nInBufferCount = 3;
      m_sEncoderConfig.nOutBufferCount = 3;
      m_sEncoderConfig.nHECInterval = 0;
      m_sEncoderConfig.nResyncMarkerSpacing = 0;
      m_sEncoderConfig.eResyncMarkerType = RESYNC_MARKER_NONE;
      m_sEncoderConfig.bEnableIntraRefresh = OMX_FALSE;
      m_sEncoderConfig.nFrames = 30;
      m_sEncoderConfig.bEnableShortHeader = OMX_FALSE;
      m_sEncoderConfig.nIntraPeriod = m_sEncoderConfig.nFramerate * 2;
      m_sEncoderConfig.nMinQp = 2;
      m_sEncoderConfig.nMaxQp = 31;
      m_sEncoderConfig.bProfileMode = OMX_FALSE;

      // default dynamic config
      m_sDynamicConfig.nIFrameRequestPeriod = 0;
      m_sDynamicConfig.nUpdatedBitrate = m_sEncoderConfig.nBitrate;
      m_sDynamicConfig.nUpdatedFramerate = m_sEncoderConfig.nFramerate;
      m_sDynamicConfig.nUpdatedMinQp = m_sEncoderConfig.nMinQp;
      m_sDynamicConfig.nUpdatedMaxQp = m_sEncoderConfig.nMaxQp;
      m_sDynamicConfig.nUpdatedFrames = m_sEncoderConfig.nFrames;
      m_sDynamicConfig.nUpdatedIntraRefreshMBs = 0;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   Config::~Config()
   {
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Config::Parse(OMX_STRING pFileName,
                               EncoderConfigType* pEncoderConfig,
                               DynamicConfigType* pDynamicConfig)
   {
      static const OMX_S32 maxLineLen = 256;

      OMX_ERRORTYPE result = OMX_ErrorNone;


      OMX_S32 nLine = 0;

      ParserStrVector v;
      char pBuf[maxLineLen];
      char* pTrimmed;

      File f;
      result = f.Open(pFileName,
                      OMX_TRUE);

      if (result != OMX_ErrorNone)
      {
         VENC_TEST_MSG_ERROR("error opening file",0,0,0);
         return OMX_ErrorBadParameter;
      }

      while (Parser::ReadLine(&f, maxLineLen, pBuf) != -1)
      {
         OMX_S32 nTok = 0;
         (void) Parser::RemoveComments(pBuf);
         pTrimmed = Parser::Trim(pBuf);
         if (strlen(pTrimmed) != 0)
            nTok = Parser::TokenizeString(&v, pTrimmed, "\t =");

         VENC_TEST_MSG_LOW("ntok = %d", nTok,0,0);
         switch (v.size())
         {
         case 0: // blank line
            break;
         case 1: // use default value
            break;
         case 2: // user has a preferred config

            ///////////////////////////////////////////
            ///////////////////////////////////////////
            // Encoder config
            ///////////////////////////////////////////
            ///////////////////////////////////////////
            if (Parser::StringICompare("FrameWidth", v[0]) == 0)
            {
               m_sEncoderConfig.nFrameWidth = (OMX_S32) atoi(v[1]);
               VENC_TEST_MSG_LOW("FrameWidth = %d", m_sEncoderConfig.nFrameWidth,0,0);
            }
            else if (Parser::StringICompare("FrameHeight", v[0]) == 0)
            {
               m_sEncoderConfig.nFrameHeight = (OMX_S32) atoi(v[1]);
               VENC_TEST_MSG_LOW("FrameHeight = %d", m_sEncoderConfig.nFrameHeight,0,0);
            }
            else if (Parser::StringICompare("OutputFrameWidth", v[0]) == 0)
            {
               m_sEncoderConfig.nOutputFrameWidth = (OMX_S32) atoi(v[1]);
               VENC_TEST_MSG_LOW("OutputFrameWidth = %d", m_sEncoderConfig.nOutputFrameWidth,0,0);
            }
            else if (Parser::StringICompare("OutputFrameHeight", v[0]) == 0)
            {
               m_sEncoderConfig.nOutputFrameHeight = (OMX_S32) atoi(v[1]);
               VENC_TEST_MSG_LOW("OutputFrameHeight = %d", m_sEncoderConfig.nOutputFrameHeight,0,0);
            }
            else if (Parser::StringICompare("DVSXOffset", v[0]) == 0)
            {
               m_sEncoderConfig.nDVSXOffset = (OMX_S32) atoi(v[1]);
               VENC_TEST_MSG_LOW("DVSXOffset = %d", m_sEncoderConfig.nDVSXOffset,0,0);
            }
            else if (Parser::StringICompare("DVSYOffset", v[0]) == 0)
            {
               m_sEncoderConfig.nDVSYOffset = (OMX_S32) atoi(v[1]);
               VENC_TEST_MSG_LOW("DVSYOffset = %d", m_sEncoderConfig.nDVSYOffset,0,0);
            }
            else if (Parser::StringICompare("Rotation", v[0]) == 0)
            {
               m_sEncoderConfig.nRotation = (OMX_S32) atoi(v[1]);
               VENC_TEST_MSG_LOW("Rotation = %d", m_sEncoderConfig.nRotation,0,0);
            }
            else if (Parser::StringICompare("FPS", v[0]) == 0)
            {
               m_sEncoderConfig.nFramerate = (OMX_S32) atoi(v[1]);
               VENC_TEST_MSG_LOW("FPS = %d", m_sEncoderConfig.nFramerate,0,0);
            }
            else if (Parser::StringICompare("Bitrate", v[0]) == 0)
            {
               m_sEncoderConfig.nBitrate = (OMX_S32) atoi(v[1]);
               VENC_TEST_MSG_LOW("Bitrate = %d", m_sEncoderConfig.nBitrate,0,0);
            }
            else if (Parser::StringICompare("RC", v[0]) == 0)
            {
               m_sEncoderConfig.eControlRate =
                  (OMX_VIDEO_CONTROLRATETYPE) ParseEnum(g_pVencRCEnums, v[1]);
               VENC_TEST_MSG_LOW("RC = %d", m_sEncoderConfig.eControlRate,0,0);
            }
            else if (Parser::StringICompare("Codec", v[0]) == 0)
            {
               m_sEncoderConfig.eCodec = (OMX_VIDEO_CODINGTYPE) ParseEnum(g_pCodecEnums, v[1]);
               VENC_TEST_MSG_LOW("Codec = %d", m_sEncoderConfig.eCodec,0,0);
            }
            else if (Parser::StringICompare("InBufferCount", v[0]) == 0)
            {
               m_sEncoderConfig.nInBufferCount = (OMX_S32) atoi(v[1]);
               VENC_TEST_MSG_LOW("InBufferCount = %d", m_sEncoderConfig.nInBufferCount,0,0);
            }
            else if (Parser::StringICompare("OutBufferCount", v[0]) == 0)
            {
               m_sEncoderConfig.nOutBufferCount = (OMX_S32) atoi(v[1]);
               VENC_TEST_MSG_LOW("OutBufferCount = %d", m_sEncoderConfig.nOutBufferCount,0,0);
            }
            else if (Parser::StringICompare("HECInterval", v[0]) == 0)
            {
               m_sEncoderConfig.nHECInterval = (OMX_S32) atoi(v[1]);
               VENC_TEST_MSG_LOW("HECInterval = %d\n", (int) m_sEncoderConfig.nHECInterval,0,0);
            }
            else if (Parser::StringICompare("ResyncMarkerSpacing", v[0]) == 0)
            {
               m_sEncoderConfig.nResyncMarkerSpacing = (OMX_S32) atoi(v[1]);
               VENC_TEST_MSG_LOW("ResyncMarkerSpacing = %d\n", (int) m_sEncoderConfig.nResyncMarkerSpacing, 0, 0);
            }
            else if (Parser::StringICompare("ResyncMarkerType", v[0]) == 0)
            {
               m_sEncoderConfig.eResyncMarkerType = (ResyncMarkerType) ParseEnum(m_pResyncMarkerType, v[1]);
               VENC_TEST_MSG_LOW("ResyncMarkerType = %d\n", (int) m_sEncoderConfig.eResyncMarkerType, 0, 0);
            }
            else if (Parser::StringICompare("EnableIntraRefresh", v[0]) == 0)
            {
               m_sEncoderConfig.bEnableIntraRefresh = (OMX_BOOL) atoi(v[1]) == 1 ? OMX_TRUE : OMX_FALSE;
               VENC_TEST_MSG_LOW("EnableIntraRefresh = %d\n", (int) m_sEncoderConfig.bEnableIntraRefresh, 0, 0);
            }
            else if (Parser::StringICompare("TimeIncRes", v[0]) == 0)
            {
               m_sEncoderConfig.nTimeIncRes = (OMX_S32) atoi(v[1]);
               VENC_TEST_MSG_LOW("TimeIncRes = %d\n", (int) m_sEncoderConfig.nTimeIncRes, 0, 0);
            }
            else if (Parser::StringICompare("EnableShortHeader", v[0]) == 0)
            {
               m_sEncoderConfig.bEnableShortHeader = (OMX_BOOL) atoi(v[1]);
               VENC_TEST_MSG_LOW("EnableShortHeader = %d\n", (int) m_sEncoderConfig.bEnableShortHeader, 0, 0);
            }
            else if (Parser::StringICompare("IntraPeriod", v[0]) == 0)
            {
               m_sEncoderConfig.nIntraPeriod = (OMX_S32) atoi(v[1]);
               VENC_TEST_MSG_LOW("IntraPeriod = %d\n", (int) m_sEncoderConfig.nIntraPeriod, 0, 0);
            }

            else if (Parser::StringICompare("InFile", v[0]) == 0)
            {
               memcpy(m_sEncoderConfig.cInFileName, v[1], (strlen(v[1])+1));
               VENC_TEST_MSG_LOW("InFile", 0,0,0);
            }
            else if (Parser::StringICompare("OutFile", v[0]) == 0)
            {
               memcpy(m_sEncoderConfig.cOutFileName, v[1], (strlen(v[1])+1));
               VENC_TEST_MSG_LOW("OutFile", 0,0,0);
            }
            else if (Parser::StringICompare("NumFrames", v[0]) == 0)
            {
               m_sEncoderConfig.nFrames = (OMX_S32) atoi(v[1]);
               VENC_TEST_MSG_LOW("NumFrames = %d", m_sEncoderConfig.nFrames,0,0);
            }
            else if (Parser::StringICompare("MinQp", v[0]) == 0)
            {
               m_sEncoderConfig.nMinQp = (OMX_S32) atoi(v[1]);
               VENC_TEST_MSG_LOW("MinQp = %d", m_sEncoderConfig.nMinQp,0,0);
            }
            else if (Parser::StringICompare("MaxQp", v[0]) == 0)
            {
               m_sEncoderConfig.nMaxQp = (OMX_S32) atoi(v[1]);
               VENC_TEST_MSG_LOW("MaxQp = %d", m_sEncoderConfig.nMaxQp,0,0);
            }
	    else if (Parser::StringICompare("ScaleFactor", v[0]) == 0)
            {
               m_sEncoderConfig.nScaleFactor = (OMX_S32) atoi(v[1]);
               VENC_TEST_MSG_LOW("ScaleFactor = %d", m_sEncoderConfig.nScaleFactor,0,0);
            }

            ///////////////////////////////////////////
            ///////////////////////////////////////////
            // Dynamic config
            ///////////////////////////////////////////
            ///////////////////////////////////////////
            else if (Parser::StringICompare("IFrameRequestPeriod", v[0]) == 0)
            {
               m_sDynamicConfig.nIFrameRequestPeriod = (OMX_S32) atoi(v[1]);
               VENC_TEST_MSG_LOW("IFrameRequestPeriod = %d\n", (int) m_sDynamicConfig.nIFrameRequestPeriod, 0, 0);
            }
            else if (Parser::StringICompare("UpdatedBitrate", v[0]) == 0)
            {
               m_sDynamicConfig.nUpdatedBitrate = (OMX_S32) atoi(v[1]);
               VENC_TEST_MSG_LOW("UpdatedBitrate = %d\n", (int) m_sDynamicConfig.nUpdatedBitrate, 0, 0);
            }
            else if (Parser::StringICompare("UpdatedFPS", v[0]) == 0)
            {
               m_sDynamicConfig.nUpdatedFramerate = (OMX_S32) atoi(v[1]);
               VENC_TEST_MSG_LOW("UpdatedFPS = %d\n", (int) m_sDynamicConfig.nUpdatedFramerate, 0, 0);
            }
            else if (Parser::StringICompare("UpdatedMinQp", v[0]) == 0)
            {
               m_sDynamicConfig.nUpdatedMinQp = (OMX_S32) atoi(v[1]);
               VENC_TEST_MSG_LOW("UpdatedMinQp = %d\n", (int) m_sDynamicConfig.nUpdatedMinQp, 0, 0);
            }
            else if (Parser::StringICompare("UpdatedMaxQp", v[0]) == 0)
            {
               m_sDynamicConfig.nUpdatedMaxQp = (OMX_S32) atoi(v[1]);
               VENC_TEST_MSG_LOW("UpdatedMaxQp = %d\n", (int) m_sDynamicConfig.nUpdatedMaxQp, 0, 0);
            }
            else if (Parser::StringICompare("UpdatedNumFrames", v[0]) == 0)
            {
               m_sDynamicConfig.nUpdatedFrames = (OMX_S32) atoi(v[1]);
               VENC_TEST_MSG_LOW("UpdatedNumFrames = %d\n", (int) m_sDynamicConfig.nUpdatedFrames, 0, 0);
            }
            else if (Parser::StringICompare("UpdatedIntraPeriod", v[0]) == 0)
            {
               m_sDynamicConfig.nUpdatedIntraPeriod = (OMX_S32) atoi(v[1]);
               VENC_TEST_MSG_LOW("UpdatedIntraPeriod = %d\n", (int) m_sDynamicConfig.nUpdatedIntraPeriod, 0, 0);
            }
	    else if (Parser::StringICompare("UpdatedIntraRefreshMBs", v[0]) == 0)
            {
               m_sDynamicConfig.nUpdatedIntraRefreshMBs = (OMX_S32) atoi(v[1]);
               VENC_TEST_MSG_LOW("UpdatedIntraPeriod = %d\n", (int) m_sDynamicConfig.nUpdatedIntraRefreshMBs, 0, 0);
            }
            else if (Parser::StringICompare("ProfileMode", v[0]) == 0)
            {
               m_sEncoderConfig.bProfileMode = (OMX_BOOL) atoi(v[1]);
               VENC_TEST_MSG_LOW("ProfileMode = %d\n", (int) m_sEncoderConfig.bProfileMode, 0, 0);
            }
            else
            {
               VENC_TEST_MSG_ERROR("invalid config file key line %d", nLine, 0, 0);
            }
            break;
         default:
            VENC_TEST_MSG_ERROR("error with config parsing line %d", nLine, 0, 0);
            break;
         }
         v.clear();

         ++nLine;
      }

      (void) f.Close();

      memcpy(pEncoderConfig, &m_sEncoderConfig, sizeof(m_sEncoderConfig));

      if (pDynamicConfig != NULL) // optional param
      {
         memcpy(pDynamicConfig, &m_sDynamicConfig, sizeof(m_sDynamicConfig));
      }
      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Config::GetEncoderConfig(EncoderConfigType* pEncoderConfig)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;
      if (pEncoderConfig != NULL)
      {
         memcpy(pEncoderConfig, &m_sEncoderConfig, sizeof(m_sEncoderConfig));
      }
      else
      {
         result = OMX_ErrorBadParameter;
      }
      return result;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   OMX_ERRORTYPE Config::GetDynamicConfig(DynamicConfigType* pDynamicConfig)
   {
      OMX_ERRORTYPE result = OMX_ErrorNone;
      if (pDynamicConfig != NULL)
      {
         memcpy(pDynamicConfig, &m_sDynamicConfig, sizeof(m_sDynamicConfig));
      }
      else
      {
         result = OMX_ErrorBadParameter;
      }
      return result;
   }

} // namespace venctest

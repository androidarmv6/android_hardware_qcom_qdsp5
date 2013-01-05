#ifndef _VENC_TEST_COMDEF_H
#define _VENC_TEST_COMDEF_H
/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/test/common/inc/venctest_ComDef.h#2 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2010-06-12   as     Added support for Dynamic configuration for IntraRefreshMB
2010-04-02   as     Added support for TimeStamp scale factor
2009-06-30   bc     Added support for DVS
2009-04-17   bc     Initial revision

 ========================================================================== */

/*========================================================================

                     INCLUDE FILES FOR MODULE

==========================================================================*/
#include "OMX_Core.h"
#include "OMX_Video.h"

namespace venctest
{

   /**
    * @brief Maximum length for a file name
    */
   static const OMX_S32 VENC_TEST_MAX_STRING = 128;

   /**
    * @brief Resync marker types
    */
   enum ResyncMarkerType
   {
      RESYNC_MARKER_NONE,     ///< No resync marker
      RESYNC_MARKER_BITS,     ///< Resync marker for MPEG4, H.264, and H.263 annex K
      RESYNC_MARKER_MB,       ///< MB resync marker for MPEG4, H.264, and H.263 annex K
      RESYNC_MARKER_GOB       ///< GOB resync marker for H.263
   };

   /**
    * @brief Encoder configuration
    */
   struct EncoderConfigType
   {
      ////////////////////////////////////////
      //======== Common static config
      OMX_VIDEO_CODINGTYPE eCodec;                ///< Config File Key: Codec
      OMX_VIDEO_CONTROLRATETYPE eControlRate;     ///< Config File Key: RC
      OMX_S32 nResyncMarkerSpacing;               ///< Config File Key: ResyncMarkerSpacing
      ResyncMarkerType eResyncMarkerType;         ///< Config File Key: ResyncMarkerType
      OMX_BOOL bEnableIntraRefresh;               ///< Config File Key: EnableIntraRefresh
      OMX_S32 nFrameWidth;                        ///< Config File Key: FrameWidth
      OMX_S32 nFrameHeight;                       ///< Config File Key: FrameHeight
      OMX_S32 nOutputFrameWidth;                  ///< Config File Key: OutputFrameWidth
      OMX_S32 nOutputFrameHeight;                 ///< Config File Key: OutputFrameHeight
      OMX_S32 nDVSXOffset;                        ///< Config File Key: DVSXOffset
      OMX_S32 nDVSYOffset;                        ///< Config File Key: DVSYOffset
      OMX_S32 nBitrate;                           ///< Config File Key: Bitrate
      OMX_S32 nFramerate;                         ///< Config File Key: FPS
      OMX_S32 nRotation;                          ///< Config File Key: Rotation
      OMX_S32 nInBufferCount;                     ///< Config File Key: InBufferCount
      OMX_S32 nOutBufferCount;                    ///< Config File Key: OutBufferCount
      char cInFileName[VENC_TEST_MAX_STRING];     ///< Config File Key: InFile
      char cOutFileName[VENC_TEST_MAX_STRING];    ///< Config File Key: OutFile
      OMX_S32 nFrames;                            ///< Config File Key: NumFrames
      OMX_S32 nIntraPeriod;                       ///< Config File Key: IntraPeriod
      OMX_S32 nMinQp;                             ///< Config File Key: MinQp
      OMX_S32 nMaxQp;                             ///< Config File Key: MaxQp
      OMX_BOOL bProfileMode;                      ///< Config File Key: ProfileMode
      OMX_S32 nScaleFactor;                       ///< Config File Key: ScaleFactor for timestamp.

      ////////////////////////////////////////
      //======== Mpeg4 static config
      OMX_S32 nHECInterval;                       ///< Config File Key: HECInterval
      OMX_S32 nTimeIncRes;                        ///< Config File Key: TimeIncRes
      OMX_BOOL bEnableShortHeader;                ///< Config File Key: EnableShortHeader

      ////////////////////////////////////////
      //======== H.263 static config

   };

   /**
    * @brief Dynamic configuration
    */
   struct DynamicConfigType
   {
      OMX_S32 nIFrameRequestPeriod;               ///< Config File Key: IFrameRequestPeriod
      OMX_S32 nUpdatedFramerate;                  ///< Config File Key: UpdatedFPS
      OMX_S32 nUpdatedBitrate;                    ///< Config File Key: UpdatedBitrate
      OMX_S32 nUpdatedMinQp;                      ///< Config File Key: MinQp
      OMX_S32 nUpdatedMaxQp;                      ///< Config File Key: MaxQp
      OMX_S32 nUpdatedFrames;                     ///< Config File Key: UpdatedNumFrames
      OMX_S32 nUpdatedIntraPeriod;                ///< Config File Key: UpdatedIntraPeriod
      OMX_S32 nUpdatedIntraRefreshMBs;            ///< Config File Key: IntraRefreshMBs
   };

   /**
    * @brief Test description structure
    */
   struct TestDescriptionType
   {
      char cTestName[VENC_TEST_MAX_STRING];     ///< The name of the test
      char cConfigFile[VENC_TEST_MAX_STRING];   ///< Encoder config file name
      OMX_S32 nSession;                         ///< Number of sessions to run
   };
}

#endif // #ifndef _VENC_TEST_COMDEF_H

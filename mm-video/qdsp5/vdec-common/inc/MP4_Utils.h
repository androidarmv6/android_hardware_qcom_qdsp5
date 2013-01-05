#ifndef MP4_UTILS_H
#define MP4_UTILS_H
/*=============================================================================
      MP4_UTILS.h

DESCRIPTION
  This file declares an MP4_Utils helper class to help parse the
  VOL (Video Object Layer) header and the short header in a raw bitstream.

  There is a dependency on MPEG4 decoder.

  Historocally, this was part of MP4_TL.h, and the only way to parse headers
  was to have an instance of MP4_TL. However, the future direction for MP4_TL
  is that (a) it's constructor be private, (b) instances are to be created by
  a static member factory, and (c) this factory can only be called at
  vdec_create time.

  This poses a problem when we wish to parse VOL (or other) headers before
  calling vdec_create.

  The initial solution was to permit MP4_TL to be contructed, and have its
  iDecodeVOLHeader method called without having its Initialize method called,
  but this required hacking its destructor to not ASSERT when called in the
  NOTINIT state. This goes against the desire to restrict how and when an
  MP4_TL object is created. Thus, we factor the code out, in the hopes
  that the future MP4_TL implementation will use it.

LIMITATIONS:

ABSTRACT:
   MPEG 4 video decoding and encoding functionality is accomplished by
   several modules including but not limited to the: media player
   application, MPEG 4 engine, MPEG 4 video codec, & MPEG 4 audio codec
   as outlined below:

EXTERNALIZED FUNCTIONS
  List functions and a brief description that are exported from this file

INITIALIZATION AND SEQUENCING REQUIREMENTS
  is only needed if the order of operations is important.

Copyright(c)  2008  QUALCOMM, Incorporated. All Rights Reserved.
Qualcomm Proprietary and Confidential

============================================================================*/
/* =======================================================================
                             Edit History

$Header: 
$DateTime: 
$Change: 

========================================================================== */

/* ==========================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */

#include "qtv_msg.h"
#include "OMX_Types.h"
#include "OMX_Core.h"

/* <EJECT> */
/* ==========================================================================

                DEFINITIONS AND DECLARATIONS FOR MODULE

This section contains definitions for constants, macros, types, variables
and other items needed by this module.

========================================================================== */


/*---------------------------------------------------------------------------
** MPEG 4 Command Queue, Statistics Queue, Free Queues and Available Packets
**--------------------------------------------------------------------------- */

/* Structure used to manage input data.
*/

#define MP4ERROR_SUCCESS     0


#define SIMPLE_PROFILE_LEVEL0				0x08
#define SIMPLE_PROFILE_LEVEL1				0x01
#define SIMPLE_PROFILE_LEVEL2				0x02
#define SIMPLE_PROFILE_LEVEL3				0x03
#define SIMPLE_PROFILE_LEVEL4A            		0x04
#define SIMPLE_PROFILE_LEVEL5            		0x05
#define SIMPLE_PROFILE_LEVEL6            		0x06
#define SIMPLE_PROFILE_LEVEL0B                          0x09

#define SIMPLE_SCALABLE_PROFILE_LEVEL0                  0x10
#define SIMPLE_SCALABLE_PROFILE_LEVEL1                  0x11
#define SIMPLE_SCALABLE_PROFILE_LEVEL2                  0x12

#define ADVANCED_SIMPLE_PROFILE_LEVEL0                 0xF0
#define ADVANCED_SIMPLE_PROFILE_LEVEL1                 0xF1
#define ADVANCED_SIMPLE_PROFILE_LEVEL2                 0xF2
#define ADVANCED_SIMPLE_PROFILE_LEVEL3                 0xF3
#define ADVANCED_SIMPLE_PROFILE_LEVEL4                 0xF4
#define ADVANCED_SIMPLE_PROFILE_LEVEL5                 0xF5

#define VISUAL_OBJECT_SEQUENCE_START_CODE   0x000001B0
#define VISUAL_OBJECT_SEQUENCE_END_CODE 	0x000001B1
#define VIDEO_OBJECT_LAYER_START_CODE_MASK  0xFFFFFFF0 
#define VIDEO_OBJECT_LAYER_START_CODE       0x00000120
#define SHORT_HEADER_MASK                   0xFFFFFC00
#define SHORT_HEADER_START_CODE             0x00008000
#define MPEG4_SHAPE_RECTANGULAR	            0x00
#define EXTENDED_PAR                        0xF 
#define SHORT_VIDEO_START_MARKER			0x20
#define MP4_INVALID_VOL_PARAM   (0x0001)  // unsupported VOL parameter
#define MP4ERROR_UNSUPPORTED_UFEP                   -1068
#define MP4ERROR_UNSUPPORTED_SOURCE_FORMAT          -1069
#define MASK(x) (0xFFFFFFFF >> (32 - (x)))

#define VOP_START_CODE                      0x000001B6
#define VOL_START_CODE                      0x000001B0
#define VISUAL_OBJECT_TYPE_VIDEO_ID         0x1
#define VISUAL_OBJECT_START_CODE            0x000001B5
#define VIDEO_OBJECT_START_CODE_MASK        0xFFFFFFE0
#define VIDEO_OBJECT_START_CODE             0x00000100
// Supported object types (table 6-10 ISO/IEC 14496-2)
#define RESERVED_OBJECT_TYPE                0x0
#define SIMPLE_OBJECT_TYPE                  0x1
#define SIMPLE_SCALABLE_OBJECT_TYPE         0x2
#define CORE_OBJECT_TYPE                    0x3
#define MAIN_OBJECT_TYPE                    0x4
#define N_BIT_OBJECT_TYPE                   0x5
#define BASIC_ANIMATED_2D_TEXTURE           0x6
#define ANIMATED_2D_MESH                    0x7
#define ADVANCED_SIMPLE                     0x11

#define MP4_VGA_WIDTH             640
#define MP4_VGA_HEIGHT            480
#define MP4_WVGA_WIDTH            800
#define MP4_WVGA_HEIGHT           480

#ifdef FEATURE_QTV_WVGA_ENABLE
  #define MP4_MAX_DECODE_WIDTH    MP4_WVGA_WIDTH
  #define MP4_MAX_DECODE_HEIGHT   MP4_WVGA_HEIGHT
#else
  #define MP4_MAX_DECODE_WIDTH    MP4_VGA_WIDTH
  #define MP4_MAX_DECODE_HEIGHT   MP4_VGA_HEIGHT
#endif /*FEATURE_QTV_WVGA_ENABLE*/


typedef struct 
{
   unsigned char *data;
   unsigned long int numBytes;
}mp4StreamType;


class MP4_Utils
{
private:
  struct posInfoType
  {
    unsigned char*  bytePtr;
    unsigned char   bitPos;
  };

  posInfoType m_posInfo;
  unsigned char *m_dataBeginPtr;

  unsigned short m_SrcWidth,m_SrcHeight;        // Dimensions of the source clip

  bool validate_profile_and_level(unsigned long int profile_and_level_indication, unsigned long int video_object_type_indication);

public:

  unsigned short SrcWidth(void) const { return m_SrcWidth; }
  unsigned short SrcHeight(void) const { return m_SrcHeight; }

  /* <EJECT> */
/*===========================================================================

FUNCTION:
  MP4_Utils constructor

DESCRIPTION:
  Constructs an instance of the MP4 Parser.
  
RETURN VALUE:
  None.
===========================================================================*/
MP4_Utils(signed short *pError);

/* <EJECT> */
/*===========================================================================

FUNCTION:
  MP4_Utils destructor

DESCRIPTION:
  Destructs an instance of the MP4_Utils.
  
RETURN VALUE:
  None.
===========================================================================*/
~MP4_Utils();

/*
============================================================================
*/
OMX_ERRORTYPE populateHeightNWidthFromShortHeader( mp4StreamType *psBits);

/* <EJECT> */
/*===========================================================================

FUNCTION:
  iDecodeVOLHeader

DESCRIPTION:
  This function decodes the VOL (Visual Object Layer) header 
  (ISO/IEC 14496-2:1999/Amd.1:2000(E), section 6.3.3)

INPUT/OUTPUT PARAMETERS:
  psBits - pointer to input stream of bits
  psVOL  - pointer to structure containing VOL information required 
           by the decoder

RETURN VALUE:
  Error code

SIDE EFFECTS:
  None.

===========================================================================*/
OMX_ERRORTYPE populateHeightNWidthFromVolHeader(mp4StreamType *psBits);


/* <EJECT> */
/*===========================================================================
FUNCTION:
  read_bit_field

DESCRIPTION:
  This helper function reads a field of given size (in bits) out of a raw
  bitstream. 

INPUT/OUTPUT PARAMETERS:
  posPtr:   Pointer to posInfo structure, containing current stream position
            information

  size:     Size (in bits) of the field to be read; assumed size <= 32
  
  NOTE: The bitPos is the next available bit position in the byte pointed to
        by the bytePtr. The bit with the least significant position in the byte
        is considered bit number 0.

RETURN VALUE:
  Value of the bit field required (stored in a 32-bit value, right adjusted).

SIDE EFFECTS:
  None.
---------------------------------------------------------------------------*/
static unsigned long int read_bit_field(posInfoType* posPtr, unsigned long int size);

/* ======================================================================
FUNCTION
  MP4_Utils::validateMetaData

DESCRIPTION
  Validate the metadata (VOL or short header), get the width and height
  return true if level and profile are supported

PARAMETERS
  IN OMX_U8 *encodedBytes
      the starting point of metadata
 
  IN OMX_U32 totalBytes
      the number of bytes of the metadata
 
  OUT unsigned &height
      the height of the clip

  OUT unsigned &width
      the width of the clip

RETURN VALUE
  true if level and profile are supported
  false otherwise
========================================================================== */
OMX_ERRORTYPE validateMetaData(OMX_U8 *encodedBytes,
                      OMX_U32 totalBytes,
                      unsigned &height,
                      unsigned &width,
                      unsigned &cropx,
                      unsigned &cropy,
                      unsigned &cropdx, 
                      unsigned &cropdy);

};
#endif /*  MP4_UTILS_H */


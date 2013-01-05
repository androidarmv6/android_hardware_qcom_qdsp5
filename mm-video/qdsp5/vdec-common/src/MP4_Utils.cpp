#include "MP4_Utils.h"
# include <stdio.h>

//#define MASK(x) (0xFFFFFFFF >> (32 - (x)))
//typedef  signed short       signed short;


/* -----------------------------------------------------------------------
** Forward Declarations
** ----------------------------------------------------------------------- */

/* =======================================================================
**                            Function Definitions
** ======================================================================= */

/*<EJECT>*/ 
/*===========================================================================
FUNCTION:
  MP4_Utils constructor

DESCRIPTION:
  Constructs an instance of the Mpeg4 Utilitys.
  
RETURN VALUE:
  None.
===========================================================================*/
MP4_Utils::MP4_Utils(signed short* pError)
{
  m_SrcWidth = 0;
  m_SrcHeight = 0;
  *pError = MP4ERROR_SUCCESS;
}

/* <EJECT> */ 
/*===========================================================================

FUNCTION:
  MP4_Utils destructor

DESCRIPTION:
  Destructs an instance of the Mpeg4 Utilities.
  
RETURN VALUE:
  None.
===========================================================================*/
MP4_Utils::~MP4_Utils()
{
}

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
unsigned long int MP4_Utils::read_bit_field
(
  posInfoType* posPtr,
  unsigned long int       size
)
{
  unsigned char*  bits    = &posPtr->bytePtr[0];
  unsigned long int  bitBuf  = (bits[0]<<24) | (bits[1]<<16) | (bits[2]<<8) | bits[3];

  unsigned long int value = (bitBuf >> (32 - posPtr->bitPos - size)) & MASK(size);

  /* Update the offset in preparation for next field    */
  posPtr->bitPos += size;

  while ( posPtr->bitPos >= 8 )
  {
    posPtr->bitPos -= 8;
    posPtr->bytePtr++;
  }
    return value;

}

/* <EJECT> */ 
/*===========================================================================
FUNCTION:
  find_code

DESCRIPTION:
  This helper function searches a bitstream for a specific 4 byte code. 

INPUT/OUTPUT PARAMETERS:
  bytePtr:          pointer to starting location in the bitstream
  size:             size (in bytes) of the bitstream
  codeMask:         mask for the code we are looking for
  referenceCode:    code we are looking for

RETURN VALUE:
  Pointer to a valid location if the code is found; 0 otherwise.

SIDE EFFECTS:
  None.
---------------------------------------------------------------------------*/
static unsigned char* find_code
(
   unsigned char*    bytePtr,
   unsigned long int    size,
   unsigned long int    codeMask,
   unsigned long int    referenceCode
)
{
  unsigned long int code = 0xFFFFFFFF;
  for (unsigned long int i = 0; i < size; i++)
  {
    code <<= 8;
    code |= *bytePtr++;
        
    if ( (code & codeMask) == referenceCode )
    {
      return bytePtr;
    }
  }

  //printf("Unable to find code");

  return NULL;
}

/*
=============================================================================
FUNCTION:
  populateHeightNWidthFromShortHeader

DESCRIPTION:
  This function 
  - parses the short header and populates frame height and width 
    into MP4_Utils.
  - check the profile and level. 
    It will return MP4ERROR_SUCCESS if profile and level are supported
    return error otherwise 

INPUT/OUTPUT PARAMETERS:
  psBits - pointer to input stream of bits

RETURN VALUE:
  MP4ERROR_SUCCESS if profile and level are supported
  error otherwise 

SIDE EFFECTS:
  None.

=============================================================================
*/
OMX_ERRORTYPE MP4_Utils::populateHeightNWidthFromShortHeader
(
  mp4StreamType *psBits
)
{
  bool extended_ptype = false;
  bool opptype_present = false;
  bool fCustomSourceFormat = false; 
  unsigned long int marker_bit;
  unsigned long int source_format;
  m_posInfo.bitPos  = 0;
  m_posInfo.bytePtr = psBits->data;
  m_dataBeginPtr = psBits->data;
	//22 -> short_video_start_marker
	if(SHORT_VIDEO_START_MARKER != read_bit_field (&m_posInfo, 22)) 
		return OMX_ErrorStreamCorrupt;
	//8 -> temporal_reference
	//1 -> marker bit
	//1 -> split_screen_indicator
	//1 -> document_camera_indicator
	//1 -> full_picture_freeze_release
	read_bit_field (&m_posInfo, 13);
    source_format = read_bit_field (&m_posInfo, 3);
	switch (source_format)
  {
     case 1:
      // sub-QCIF 
      m_SrcWidth = 128;
      m_SrcHeight = 96;
      break;

  case 2:
      // QCIF 
      m_SrcWidth = 176;
      m_SrcHeight = 144;
      break;

  case 3:
      // CIF 
      m_SrcWidth = 352;
      m_SrcHeight = 288;
      break;

  case 4:
      // 4CIF 
      m_SrcWidth = 704;
      m_SrcHeight = 576;
      break;

  case 5:
      // 16CIF 
      m_SrcWidth = 1408;
      m_SrcHeight = 1152;
      break;
 
  case 7:
        extended_ptype = true;
        break;

  default:
      return OMX_ErrorStreamCorrupt;
  }

  if (extended_ptype) 
  {
    /* Plus PTYPE (PLUSPTYPE) 
    ** This codeword of 12 or 30 bits is comprised of up to three subfields: 
    ** UFEP, OPPTYPE, and MPPTYPE.  OPPTYPE is present only if UFEP has a 
    ** particular value. 
    */

    /* Update Full Extended PTYPE (UFEP) */
    unsigned long int ufep = read_bit_field (&m_posInfo, 3);
    switch (ufep) 
    {
      case 0:
      /* Only MMPTYPE fields are included in current picture header, the
      ** optional part of PLUSPTYPE (OPPTYPE) is not present
      */
          opptype_present = false;
          break;

      case 1:
      /* all extended PTYPE fields (OPPTYPE and MPPTYPE) are included in 
      ** current picture header 
      */  
          opptype_present = true;
          break;

      default:
          return OMX_ErrorUnsupportedSetting;
    }
 
    if (opptype_present) 
    {
      /* The Optional Part of PLUSPTYPE (OPPTYPE) (18 bits) */
      /* source_format*/
      source_format = read_bit_field (&m_posInfo, 3);     
      switch (source_format) 
      {
        case 1:
          /* sub-QCIF */
          m_SrcWidth = 128;
          m_SrcHeight = 96;
          break;
      
        case 2:
                /* QCIF */
          m_SrcWidth = 176;
          m_SrcHeight = 144;
          break;
      
        case 3:
                /* CIF */
          m_SrcWidth = 352;
          m_SrcHeight = 288;
          break;
      
        case 4:
                /* 4CIF */
          m_SrcWidth = 704;
          m_SrcHeight = 576;
          break;
      
        case 5:
                /* 16CIF */
          m_SrcWidth = 1408;
          m_SrcHeight = 1152;
          break;
      
        case 6:
          /* custom source format */
          fCustomSourceFormat = true;
          break;
      
        default:
          return OMX_ErrorUnsupportedSetting;
      }

      /* Custom PCF */
      read_bit_field (&m_posInfo, 1);

      /* Continue parsing to determine whether H.263 Profile 1,2, or 3 is present.
      ** Only Baseline profile P0 is supported
      ** Baseline profile doesn't have any ANNEX supported.
      ** This information is used initialize the DSP. First parse past the 
      ** unsupported optional custom PCF and Annexes D, E, and F. 
      */
      unsigned long int PCF_Annex_D_E_F = read_bit_field (&m_posInfo, 3);
      if ( PCF_Annex_D_E_F != 0 ) return OMX_ErrorUnsupportedSetting;

      /* Parse past bit for Annex I, J, K, N, R, S, T */
      unsigned long int PCF_Annex_I_J_K_N_R_S_T = read_bit_field (&m_posInfo, 7);
      if ( PCF_Annex_I_J_K_N_R_S_T != 0 ) return OMX_ErrorUnsupportedSetting;

      /* Parse past one marker bit, and three reserved bits */
      read_bit_field (&m_posInfo, 4);

      /* Parse past the 9-bit MPPTYPE */
      read_bit_field (&m_posInfo, 9);

      /* Read CPM bit */
      unsigned long int continuous_presence_multipoint = read_bit_field (&m_posInfo, 1);
      if (fCustomSourceFormat)
      {
        if ( continuous_presence_multipoint )
        {
          /* PSBI always follows immediately after CPM if CPM = "1", so parse 
          ** past the PSBI.
          */
          read_bit_field (&m_posInfo, 2);
        }
        /* Extract the width and height from the Custom Picture Format (CPFMT) */
        unsigned long int pixel_aspect_ration_code = read_bit_field (&m_posInfo, 4);
        if ( pixel_aspect_ration_code == 0 ) return OMX_ErrorStreamCorrupt;

        unsigned long int picture_width_indication = read_bit_field (&m_posInfo, 9);
        m_SrcWidth = ((picture_width_indication & 0x1FF) + 1) << 2;

        marker_bit = read_bit_field (&m_posInfo, 1);
        if ( marker_bit == 0 ) return OMX_ErrorStreamCorrupt;

        unsigned long int picture_height_indication = read_bit_field (&m_posInfo, 9);
        m_SrcHeight = (picture_height_indication & 0x1FF) << 2;
        QTV_MSG_PRIO1(QTVDIAG_GENERAL, QTVDIAG_PRIO_FATAL,"m_SrcHeight =  %d\n",m_SrcHeight);
        QTV_MSG_PRIO1(QTVDIAG_GENERAL, QTVDIAG_PRIO_FATAL,"m_SrcWidth =  %d\n",m_SrcWidth);
      }
    }
    else
    {
      /* UFEP must be "001" for INTRA picture types 
       * This indicates MPPTYPE
       */
      return OMX_ErrorStreamCorrupt;
    }
  }
  else
  {
    /* It's not an extended PTYPE */

    /* Coding type*/
    read_bit_field (&m_posInfo, 1);

    /* Continue parsing to detmine whether H.263 Profile 1, 2, or 3 is present.
       We only support Baseline Profile (Profile 0)
    */
    unsigned long int PCF_Annex_D_E_F_G = read_bit_field (&m_posInfo, 4);
    if ( PCF_Annex_D_E_F_G != 0 ) return OMX_ErrorUnsupportedSetting;
  }

  if (m_SrcWidth * m_SrcHeight > MP4_MAX_DECODE_WIDTH * MP4_MAX_DECODE_HEIGHT)
  {
    /* Frame dimesions greater than maximum size supported */
    QTV_MSG_PRIO2(QTVDIAG_GENERAL, QTVDIAG_PRIO_ERROR,
                  "Frame Dimensions not supported %d %d",m_SrcWidth,m_SrcHeight );
    return OMX_ErrorUnsupportedSetting;
  }
  return OMX_ErrorNone;
}
/* <EJECT> */ 
/*===========================================================================

FUNCTION:
  populateHeightNWidthFromVolHeader

DESCRIPTION:
  This function parses the VOL header and populates frame height and width 
  into MP4_Utils.

INPUT/OUTPUT PARAMETERS:
  psBits - pointer to input stream of bits

RETURN VALUE:
  MP4ERROR_SUCCESS if profile and level are supported
  error otherwise 

SIDE EFFECTS:
  None.

===========================================================================*/
 
OMX_ERRORTYPE MP4_Utils::populateHeightNWidthFromVolHeader
(
  mp4StreamType *psBits
)
{
  unsigned long int profile_and_level_indication = 0;
  unsigned long int video_object_type_indication = 0;

  m_posInfo.bitPos = 0;
  m_posInfo.bytePtr = psBits->data;
  m_dataBeginPtr = psBits->data;
  m_posInfo.bytePtr = find_code(m_posInfo.bytePtr,
                                psBits->numBytes,
                                MASK(32),
                                VISUAL_OBJECT_SEQUENCE_START_CODE);
  if ( m_posInfo.bytePtr == NULL )    /* Stream not starting with VISUAL_OBJECT_SEQUENCE_START_CODE */
  {
    QTV_MSG(QTVDIAG_VIDEO_TASK,
            "Video bit stream is not starting with VISUAL_OBJECT_SEQUENCE_START_CODE");
       /* re-initialize   */
    m_posInfo.bitPos  = 0;
    m_posInfo.bytePtr = psBits->data;

    m_posInfo.bytePtr = find_code(m_posInfo.bytePtr,
                                  psBits->numBytes,
                                  SHORT_HEADER_MASK,
                                  SHORT_HEADER_START_CODE);
    if( m_posInfo.bytePtr )
    {
      return populateHeightNWidthFromShortHeader(psBits);
    }
    else
    {
      m_posInfo.bitPos = 0;
      m_posInfo.bytePtr = psBits->data;
    }
  }
  else
  {
    profile_and_level_indication = read_bit_field (&m_posInfo, 8);
	QTV_MSG_PRIO1(QTVDIAG_GENERAL, QTVDIAG_PRIO_FATAL,"profile_and_level_indication =  %d\n",profile_and_level_indication);
  }

  /* parsing Visual Object(VO) header*/
   m_posInfo.bytePtr = find_code(m_posInfo.bytePtr,
                             psBits->numBytes,
                             MASK(32),
                             VISUAL_OBJECT_START_CODE);
   if(m_posInfo.bytePtr == NULL)
   {
      m_posInfo.bitPos = 0;
      m_posInfo.bytePtr = psBits->data;
   }
   else
   {
      unsigned long int is_visual_object_identifier = read_bit_field (&m_posInfo, 1);
      if ( is_visual_object_identifier )
      {
         /* visual_object_verid*/
         read_bit_field (&m_posInfo, 4);
         /* visual_object_priority*/
         read_bit_field (&m_posInfo, 3);
      }

      /* visual_object_type*/
      unsigned long int visual_object_type = read_bit_field (&m_posInfo, 4);
      if ( visual_object_type != VISUAL_OBJECT_TYPE_VIDEO_ID )
      {
         QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_ERROR,
            "visual_object_type can only be VISUAL_OBJECT_TYPE_VIDEO_ID");
	return OMX_ErrorStreamCorrupt;
      }
      /* skipping video_signal_type params*/


      /*parsing Video Object header*/
      m_posInfo.bytePtr = find_code(m_posInfo.bytePtr,
                                    psBits->numBytes,
                                    VIDEO_OBJECT_START_CODE_MASK,
                                    VIDEO_OBJECT_START_CODE);
      if ( m_posInfo.bytePtr == NULL )
      {
         QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_FATAL,
            "Unable to find VIDEO_OBJECT_START_CODE");
	return OMX_ErrorStreamCorrupt;
      }
   }

  /* video_object_layer_start_code*/
  m_posInfo.bitPos = 0;
  m_posInfo.bytePtr = find_code(m_posInfo.bytePtr, 
                              psBits->numBytes, 
                              VIDEO_OBJECT_LAYER_START_CODE_MASK,
                              VIDEO_OBJECT_LAYER_START_CODE);
  
  if ( m_posInfo.bytePtr == NULL )
  {
    m_posInfo.bitPos = 0;
    m_posInfo.bytePtr = psBits->data;
    m_posInfo.bytePtr = find_code(m_posInfo.bytePtr,
                                  psBits->numBytes, 
                                  SHORT_HEADER_MASK,
                                  SHORT_HEADER_START_CODE);
    if( m_posInfo.bytePtr )
    {
      return populateHeightNWidthFromShortHeader(psBits);
    }
    else
    {
      m_posInfo.bitPos = 0;
      m_posInfo.bytePtr = psBits->data;
      return OMX_ErrorStreamCorrupt;
    }
  }
  // 1 -> random accessible VOL
  read_bit_field (&m_posInfo, 1);

  // 8 -> video_object_type indication
  video_object_type_indication = read_bit_field (&m_posInfo, 8);
  QTV_MSG_PRIO1(QTVDIAG_GENERAL, QTVDIAG_PRIO_FATAL,"video_object_type_indication =  %d\n",video_object_type_indication);
  // 1 -> is_object_layer_identifier
  if(read_bit_field (&m_posInfo, 1))
  {
   // 4 -> video_object_layer_verid
     // 3 -> video_object_layer_priority
   read_bit_field(&m_posInfo, 7);
  }
  // 4 -> aspect_ratio_info
  if(EXTENDED_PAR == read_bit_field(&m_posInfo, 4))
  {
   //8 -> par_width
   //8 -> par_height
     read_bit_field(&m_posInfo, 16);
  }
  //1 -> vol_control_parameters
  if(read_bit_field (&m_posInfo, 1))
  {
   //2 -> chroma_format
   //1 -> low_delay
     read_bit_field (&m_posInfo, 3); 
   //1-> vbv_parameters
   if(read_bit_field (&m_posInfo, 1))
   {
     //15 -> first_half_bit_rate
     //1 -> marker_bit
     //15 -> latter_half_bit_rate
     //1 -> marker_bit
     //15 -> first_half_vbv_buffer_size
     //1 -> marker_bit
     //3 -> latter_half_vbv_buffer_size
     //11 -> first_half_vbv_occupancy
     //1 -> marker_bit
     //15 -> latter_half_vbv_occupancy
     //1 -> marker_bit
     read_bit_field (&m_posInfo, 79);
   }
  }
  if(MPEG4_SHAPE_RECTANGULAR != (unsigned char)read_bit_field (&m_posInfo, 2))
  {
     //printf("returning NON_RECTANGULAR_SHAPE \n");
     return OMX_ErrorStreamCorrupt;
  }
  //1 -> marker bit
  read_bit_field (&m_posInfo, 1);
  //16 -> vop_time_increment_resolution
  unsigned short time_increment_res = (unsigned short)read_bit_field (&m_posInfo, 16);
  int i,j;
  int nBitsTime;
  // claculating VOP resolution
  i = time_increment_res-1;
  j = 0;
  while (i)
  {
    j++;
    i>>=1;
  }
  if (j)
    nBitsTime = j;
  else
    nBitsTime = 1; 
  
  //1 -> marker_bit
  read_bit_field (&m_posInfo, 1);
  //1 -> fixed_vop_rate
  if(read_bit_field (&m_posInfo, 1))
  {
    //nBitsTime -> fixed_vop_increment 
  read_bit_field (&m_posInfo,nBitsTime);
  }
  if(1 != read_bit_field (&m_posInfo, 1)) return OMX_ErrorStreamCorrupt;
  m_SrcWidth = read_bit_field (&m_posInfo, 13);
  if(1 != read_bit_field (&m_posInfo, 1)) return OMX_ErrorStreamCorrupt;
  m_SrcHeight = read_bit_field (&m_posInfo, 13);
  
  // not doing the remaining parsing
  if (!validate_profile_and_level(profile_and_level_indication,video_object_type_indication))
    return OMX_ErrorUnsupportedSetting;
  return OMX_ErrorNone;
}

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
OMX_ERRORTYPE MP4_Utils::validateMetaData(OMX_U8 *encodedBytes,
                                 OMX_U32 totalBytes,
                                 unsigned &height,
                                 unsigned &width,
                                 unsigned &cropx,
                                  unsigned &cropy,
                                  unsigned &cropdx, 
                                  unsigned &cropdy)
{
  OMX_ERRORTYPE result = OMX_ErrorNone;
  mp4StreamType dataStream;
  dataStream.data = (unsigned char*)encodedBytes;
  dataStream.numBytes = totalBytes;
  result = populateHeightNWidthFromVolHeader(&dataStream);
  cropx = cropy = 0;
  height = cropdy = m_SrcHeight;
  width = cropdx = m_SrcWidth;
  return result;
}

/*===========================================================================
FUNCTION:
  validate_profile_and_level

DESCRIPTION:
  This function validate the profile and level that is supported.

INPUT/OUTPUT PARAMETERS:
  unsigned long int profile_and_level_indication

RETURN VALUE:
  false it it's not supported
  true otherwise

SIDE EFFECTS:
  None.
===========================================================================*/
bool MP4_Utils::validate_profile_and_level(unsigned long int profile_and_level_indication, unsigned long int video_object_type_indication )
{
  DEBUG_PRINT("MP4 profile and level %d\n", profile_and_level_indication);
  if (   (profile_and_level_indication != SIMPLE_PROFILE_LEVEL0) &&
         (profile_and_level_indication != SIMPLE_PROFILE_LEVEL1) &&
         (profile_and_level_indication != SIMPLE_PROFILE_LEVEL2) &&
         (profile_and_level_indication != SIMPLE_PROFILE_LEVEL3) &&
	 (profile_and_level_indication != SIMPLE_PROFILE_LEVEL4A) &&
	 (profile_and_level_indication != SIMPLE_PROFILE_LEVEL5) &&
	 (profile_and_level_indication != SIMPLE_PROFILE_LEVEL6) &&
	 (profile_and_level_indication != SIMPLE_PROFILE_LEVEL0B) &&
         (profile_and_level_indication != SIMPLE_SCALABLE_PROFILE_LEVEL0) &&
         (profile_and_level_indication != SIMPLE_SCALABLE_PROFILE_LEVEL1) &&
         (profile_and_level_indication != SIMPLE_SCALABLE_PROFILE_LEVEL2) &&
	 (profile_and_level_indication != RESERVED_OBJECT_TYPE) )
  {
    DEBUG_PRINT_ERROR("Caution: INVALID_PROFILE_AND_LEVEL ");      
    return false;
  }
  DEBUG_PRINT("MP4 Video Object Type %d\n", video_object_type_indication);
  if ( (video_object_type_indication != SIMPLE_OBJECT_TYPE) &&
       (video_object_type_indication != SIMPLE_SCALABLE_OBJECT_TYPE) &&
       (video_object_type_indication != ADVANCED_SIMPLE) &&
       (video_object_type_indication != RESERVED_OBJECT_TYPE) &&
       (video_object_type_indication != MAIN_OBJECT_TYPE))
  {
    DEBUG_PRINT_ERROR("Caution: INVALID_VIDEO_OBJECT_TYPE ");
    return false;
  }

  return true;
}

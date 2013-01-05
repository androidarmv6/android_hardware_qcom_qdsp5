#ifndef VDECODER_TYPES_H
#define VDECODER_TYPES_H
/* =======================================================================
                               vdecoder_types.h
DESCRIPTION
  This file defines all the types used by the QTV video decoder API.

Copyright (c) 2007 by QUALCOMM, Incorporated.  All Rights Reserved. 
QUALCOMM Proprietary and Confidential.
========================================================================== */

/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/VdecCommon/main/latest/src/vdecoder_types.h#5 $
$DateTime: 2009/03/03 14:14:44 $
$Change: 853763 $

========================================================================== */

/* =======================================================================
**               Includes and Public Data Declarations
** ======================================================================= */

/* ==========================================================================

                        DATA DECLARATIONS

========================================================================== */
/* -----------------------------------------------------------------------
** Constant / Define Declarations
** ----------------------------------------------------------------------- */
/* The number of characters in fourcc codes.
*/
#define VDEC_FOURCC_CHARS 4

/* Maximum supported decoder height and width.
*/
#define VDEC_MAX_DECODE_WIDTH  320
#define VDEC_MAX_DECODE_HEIGHT 240

/* The number of characters required to store a fourcc code as a string.
*/
#define VDEC_FOURCC_STRLEN ( VDEC_FOURCC_CHARS + 1 )

#ifdef FEATURE_MP4_TEMPORAL_SCALABILITY
  #define VDEC_MAX_LAYERS 4
#else
  #define VDEC_MAX_LAYERS 1
#endif

//Max number of layers the player handles.
//I think this is actually used as max media layers,
//max video layers, and max audio layers
#ifdef FEATURE_QTV_OSCAR_DECODER
  /*-------------------------------------------------------------------------
    OSCAR has 2 layers of scalability, and defining
    FEATURE_MP4_TEMPORAL_SCALABILITY actually causes problems, so we manually
    override the layer count here.
  -------------------------------------------------------------------------*/
  #define COMMON_MAX_LAYERS 2
#elif defined(FEATURE_MP4_TEMPORAL_SCALABILITY)
# define COMMON_MAX_LAYERS 4
#else
# define COMMON_MAX_LAYERS 1
#endif /* FEATURE_MP4_TEMPORAL_SCALABILITY */

#define H264_NAL_LENGTH_SIZE 4

/* -----------------------------------------------------------------------
** Type Declarations
** ----------------------------------------------------------------------- */

/* Simple types.
*/
typedef unsigned char VDEC_BYTE;

/* Generic void type used for decoder-stream identifiers.  Stream IDs
** are returned from decoder-creation functions and expected as input
** to all other API functions.
*/
typedef void* VDEC_STREAM_ID;

/* Error code used to return information about failures to the
** outside world.
*/

typedef unsigned long int VDEC_ERROR;

#define  VDEC_ERR_EVERYTHING_FINE             0    /* no problemo                          */
#define  VDEC_ERR_OPERATION_UNSUPPORTED       1    /* we can't do that at this time        */
#define  VDEC_ERR_NULL_STREAM_ID              2    /* a bogus stream ID pointer            */
#define  VDEC_ERR_NULL_POINTER                3    /* one of the data pointers is null     */
#define  VDEC_ERR_UNKNOWN_FTYP                4    /* dunno what to do with this ftyp      */
#define  VDEC_ERR_OUT_OF_MEMORY               5    /* an attempt to allocate memory failed */
#define  VDEC_ERR_INIT_TIMEOUT                6    /* timed out waiting for DSP init       */
#define  VDEC_ERR_MYSTERY_ERROR               7    /* what's broke?  Dunno!                */
#define  VDEC_ERR_LAYER_OUT_OF_BOUNDS         8    /* we were given a bogus layer number   */
#define  VDEC_ERR_NO_INPUT_AVAILABLE          9    /* trying to decode, but no input?      */
#define  VDEC_ERR_BOGUS_PARAMETER            10    /* application fed us something silly   */
#define  VDEC_ERR_BAD_BLOBTYPE               11    /* BLOB doesn't contain what we want    */
#define  VDEC_ERR_CORRUPT_BLOB               12    /* BLOB doesn't make sense              */
#define  VDEC_ERR_WRONG_DECODER_TYPE         13    /* wrong decoder type for attempted op  */
#define  VDEC_ERR_OUT_OF_BUFFERS             14    /* No frame buffers to satisfy request  */
#define  VDEC_ERR_RESOURCE_UNAVAILABLE       15    /* resource unavailable for the decoder */
#define  VDEC_ERR_INVALID_PARAMETER          16    /* parameter is not supported           */
#define  VDEC_ERR_INVALID_INPUT              17    /* invalid data fed to the decoder      */
#define  VDEC_ERR_UNSUPPORTED_DIMENSIONS     18    /* unsupported dimensions               */

#define  VDEC_ERR_DSP_ERR_BASE            10000    /* errors above this are from mp4codec.
                                                      Subtract VDEC_ERR_DSP_ERR_BASE to get
                                                      the error code from mp4codec.        */


/* Status code passed to the frame-ready callback to indicate what caused
** the callback.  Right now, the following things can trigger that frame callback:
** - a successful decode
** - an internal decoding error
** - a flushing of the frame-buffer queue.
*/
typedef unsigned long int VDEC_CB_STATUS;

#define  VDEC_STATUS_FLUSH             -1       /* no data in frame */

#define  VDEC_STATUS_GOOD_FRAME         0       /* everything OK */
#define  VDEC_STATUS_DECODE_ERROR       1       /* no data in frame */
#define  VDEC_STATUS_FATAL_ERROR        2       /* fatal error, restart decoder */
#define  VDEC_STATUS_VOP_NOT_CODED      3       /* Healthy decode, non-coded VOP (so no frame data)
                                                   THIS ONE IS TEMPORARY! */

  /* These are additional return codes specific to vdec
  */
#ifdef FEATURE_FRE
#define  VDEC_STATUS_FRE_FRAME          5 
#endif

#define  VDEC_STATUS_SUSPEND_DONE       6        /* suspend done */
#define  VDEC_STATUS_EOS                8        /* EOS acked */


/* Video frame dimensions are tossed around in these structures
*/
typedef struct
{
  unsigned short width;
  unsigned short height;
}
VDEC_DIMENSIONS;

/* Color format supported by the decoder.*/
typedef unsigned long int VDEC_FOURCC;

//VDEC_FOURCC can take any of the following values:-
#define VDEC_FOURCC_I420         0x30323449  /* YCrCb 4:2:0 planar format, Y followed by Cb and Cr planes */
#define VDEC_FOURCC_NV21         0x3231564E  /* YCrCb 4:2:0 planar format, interleaved Cb/Cr with Cb in the MSB */
#define VDEC_FOURCC_BI_RGB       0x00000000  /* Uncompressed RGB format */


/* The color format and pixel depth together define the format
** of a frame.
*/
typedef struct
{
  VDEC_FOURCC   fourcc;   /* The color format */
  VDEC_BYTE     bbp;      /* The pixel color depth, in bits per pixel */
}
VDEC_FRAME_FORMAT;


/* Indicates stream events. In the event of a stream status change, the 
** corresponding frame is the first frame to take on the new stream 
** properties.
*/ 
typedef unsigned long int VDEC_STREAM_STATUS;

#define   VDEC_STREAM_UNCHANGED  0    /* no change in stream status */
#define   VDEC_STREAM_SWITCHED   1    /* stream has switched */


/* VDEC_POST_FILTER_FRAME_INFO
** 
**  Structure containing post-filter frame information. 
**
**  unsigned short  lumaHistogram[256]
**    The luma histogram for the frame, distributed over 256 bins. The 
**    total number of samples equals 32 multiplied by the number of 
**    macroblocks in the frame.
*/
typedef struct
{
unsigned short  lumaHistogram[256];
}
VDEC_POST_FILTER_FRAME_INFO;

/* VDEC_POST_FILTER_MB_INFO
** 
**  Structure containing post-filter MB information. Details on how to
**  interpret this interface is specified in Annex A: Post Filter
**  Interface Specification.
*/
typedef struct
{
  unsigned short  quant;
  VDEC_BYTE       mbType; 
  signed short    mvx[4];
  signed short    mvy[4];
  VDEC_BYTE       blockTypeConfig;
  VDEC_BYTE       codedBlockConfig;
  VDEC_BYTE       subBlockConfig;
}
VDEC_POST_FILTER_MB_INFO;


typedef struct
{
  unsigned long                 numConcealedMbs;
  unsigned long                 numIntraMbs; 
  unsigned long                 timeIncrement; 
  unsigned long long                        derivedTimestamp;
  VDEC_STREAM_STATUS            streamStatus;
  VDEC_POST_FILTER_FRAME_INFO * pPostFilterFrameInfo;
  VDEC_POST_FILTER_MB_INFO *    pPostFilterMbInfo; 
}
VDEC_EXT_FRAME;

/* Frame types */
typedef unsigned long int VDEC_FRAMETYPE;

#define  VDEC_FRAMETYPE_UNKNOWN       0
#define  VDEC_FRAMETYPE_I             1
#define  VDEC_FRAMETYPE_P             2
#define  VDEC_FRAMETYPE_B             3 
#define  VDEC_FRAMETYPE_INTERPOLATED  4 


/* Structure used to manage frame buffers in the decoder.
**
** Or, at least, this is what we'd like it to look like someday.
*/
typedef struct
{
  VDEC_STREAM_ID    stream;   /* who owns this frame?                       */
  VDEC_BYTE *       pBuf;      /* where is it?                               */

  VDEC_DIMENSIONS   dimensions;  /* the source dimensions of the decoded
                                    frame                                     */
  unsigned short    stride;     /* This is the number of pixels between the
                                   first pixel of two sequential lines. It 
                                   may be greater than the width if frame
                                   includes padding                          */
  VDEC_FRAME_FORMAT format;    /* how's it formatted?                        */
  unsigned long long     timestamp; /* timestamp for this frame, in ms since
                                  beginning of clip - set by vdec_decode
                                  when it returns the frame to the app       */
  unsigned short    layer;     /* layer ID from which this frame comes       */

  VDEC_FRAMETYPE    frameType;  /* the frame type                            */
  VDEC_EXT_FRAME    extFrame;    /* extended frame data                      */

}
VDEC_FRAME;

/* Structure used to manage input data.
*/
typedef struct
{
  /* Number of layers in use.
  */
  int     layers;

  /* The bitstream buffers.  If more than one layer, [0] is always the base.
  */
  unsigned char  *buffer     [ VDEC_MAX_LAYERS ];

  /* IN/OUT param:
  ** Presentation timestamps for data in each layer.
  */
  unsigned long long  timestamp  [ VDEC_MAX_LAYERS ];

  /* buffer_size must be filled by the application with the number of
  ** bytes available in each buffer.
  **
  ** buffer_size is altered by the decoder, to indicate how many
  ** unprocessed bytes remain in each buffer.
  */
  unsigned long int  buffer_size[ VDEC_MAX_LAYERS ];

  /* IN/OUT param:
  ** IN: current position for the decoder to read.
  ** OUT: position of unread data remaining in each buffer.
  */
  unsigned long int  buffer_pos[ VDEC_MAX_LAYERS ];

  /* OUT:
  **
  ** The frameEventPending flag is set by the decoder to indicate which layers
  ** were consumed past an end-of-frame: in other words, to indicate
  ** from which layers we can expect a decode-done event soon.
  **
  ** The application should not write into this array, it is strictly
  ** for decoder output.
  */
  unsigned char bFrameEventPending[ VDEC_MAX_LAYERS ];

  /* userData is provided as a convenience for applications that wish
  ** to associate additional information with input buffers.  It is entirely
  ** unused by VDEC.
  */
  void* userData[ VDEC_MAX_LAYERS ];

  /* end of stream flag. Decoder sets this flag to indicate end of stream to DSP*/
  unsigned char eOSIndicator[ VDEC_MAX_LAYERS ];
}
VDEC_INPUT_BUFFER;

typedef struct
{
    char *base;            /* virtual base addr of buffer */
    unsigned long int fd;             /*pmem file descriptor*/
}
VDEC_SLICE_BUFFER_INFO;

typedef void * (*VDEC_MALLOC_FN)( const VDEC_STREAM_ID   stream, 
                                  void *         pCbData,
                                  unsigned long int         uSize );

typedef void     (*VDEC_FREE_FN)( const VDEC_STREAM_ID   stream,
                                  void *         pCbData, 
                                  void *         ptr );


/* Callback function used to return input buffers to the application.
**
** Parameters:
**
**   stream:  the stream ID of the decoder returning the buffer.
**   pInput:  the input buffer, which the decoder has either finished
**            processing, or which is being flushed (and is therefore
**            unread or only partially read).  How can we tell which it is?
**            check the read pointer in the input buffer.  Is it at the end?
**   pCbData: the client data the application passed in when it called
**            vdec_queue.
*/
typedef void (*VDEC_QUEUE_CB_FN)( const VDEC_STREAM_ID      stream,
                                  VDEC_INPUT_BUFFER * const pInput,
                                  void * const              pCbData );

/* Callback function used to return frame buffers to the application.
**
** Parameters:
**
**   stream:  the stream ID of the decoder returning the buffer.
**   status:          the reason for the callback. The pFrame parameter 
                      should only be used if the status indicates a  
                      successful decode.
**   nBytesConsumed:  the number of bytes consumed from the queued
**                    bitstream to produce the frame.
**   pQueueCbData:    the client data the application passed in when it 
**                    provided the frame buffer in the first place.
*/

typedef void (*VDEC_EVENT_CB_FN)( const VDEC_STREAM_ID  stream,
                                  VDEC_CB_STATUS        status,
                                  VDEC_FRAME * const   pFrame,
                                  unsigned long int                nBytesConsumed,
                                  void * const          pEventCbData );


/* The VDEC_AUDIO_CONFIG is used to provide audio information for combined
** audio/video DSP initialization.
*/
typedef unsigned long VDEC_CONCURRENCY_CONFIG;

#define  VDEC_CONCURRENT_NONE                        0
#define  VDEC_CONCURRENT_AUDIO_EVRC                  1
#define  VDEC_CONCURRENT_AUDIO_QCELP                 2
#define  VDEC_CONCURRENT_AUDIO_MIDI                  3
#define  VDEC_CONCURRENT_AUDIO_AMR                   4 
#define  VDEC_CONCURRENT_AUDIO_AAC                   5  
#define  VDEC_CONCURRENT_AUDIO_AAC_PLUS              6
#define  VDEC_CONCURRENT_AUDIO_MP3                   7
#define  VDEC_CONCURRENT_AUDIO_G723                  8
#define  VDEC_CONCURRENT_AUDIO_CONC                  9
#define  VDEC_CONCURRENT_AUDIO_REAL                 10 
#define  VDEC_CONCURRENT_AUDIO_AMR_WB               11
#define  VDEC_CONCURRENT_AUDIO_AMR_WB_PLUS          12
#define  VDEC_CONCURRENT_AUDIO_DUBBING              13
#define  VDEC_CONCURRENT_VIDEO_MPEG4_ENCODER        14 
#define  VDEC_CONCURRENT_VIDEO_H264_ENCODER         15
//audio override formats
#define  VDEC_CONCURRENT_AUD_OVER_ENC_QCELP13K      16 
#define  VDEC_CONCURRENT_AUD_OVER_ENC_AMR           17
#define  VDEC_CONCURRENT_AUD_OVER_ENC_EVRC          18

/* Enumeration of all the supported VDEC parameters.
*/
typedef unsigned long int VDEC_PARAMETER_ID;

#define  VDEC_PARM_DIMENSIONS                      0
#define  VDEC_PARM_START_CODE_DETECTION            1
#define  VDEC_PARM_TIME_INCREMENT_RESOLUTION       2 
#define  VDEC_PARM_SIZE_OF_NAL_LENGTH_FIELD        3 
#define  VDEC_PARM_DEBLOCKER_ENABLE                4
#define  VDEC_PARM_FRE_ENABLE                      5
#define  VDEC_PARM_ENABLE_XSCALING                 6
#define  VDEC_PARM_LCDB_STRENGTH                   7
#define  VDEC_PARM_EXTERNAL_CLOCK_ENABLE		   8
/* Each VDEC parameter is tied to a corresponding VDEC parameter stucture,
** below.
*/
typedef struct
{
  /* sizeOfNalLengthField: the size of the NAL length field */
  unsigned long int sizeOfNalLengthField;
}
VDEC_SIZE_OF_NAL_LENGTH_FIELD;

typedef struct
{
  /* timeIncrementResolution: the time increment resolution. */
  unsigned long int timeIncrementResolution;
}
VDEC_TIME_INCREMENT_RESOLUTION;

typedef struct
{
  /* bDeblockerEnable: TRUE enables post-loop deblocking, FALSE
  **                   disables it.
  */
  unsigned char bDeblockerEnable;
}
VDEC_DEBLOCKER_ENABLE;


typedef struct
{
  /* bStartCodeDetectionEnabled: TRUE means detection is enabled, FALSE
  **                             means it is disabled. 
*/
  unsigned char bStartCodeDetectionEnable;
}
VDEC_START_CODE_DETECTION;

typedef struct
{
  /* bFreEnable: TRUE enables FRE, FALSE disables it. */
  unsigned char bFreEnable;
}
VDEC_FRE_ENABLE;

typedef struct
{
  /* bExternalClock: TRUE enables FRE, FALSE disables it. */
  unsigned char bExternalClock;
}
VDEC_EXTERNAL_CLOCK_ENABLE;

typedef struct
{
  /*0 – indicates use pretuned default values(provided by the first word following this variable).
    1 – user configures the LCDB setting with the below 7 parms*/
  unsigned char bLCDBThreshold;

  /* LCDB strength settings */
  unsigned short LCDB_INTRA_MB_QP_THD0; 
  unsigned short LCDB_INTER_MB_QP_THD_BND0;
  unsigned short LCDB_INTER_MB_QP_THD_INSIDE0;
  unsigned short LCDB_INTER_MB_QP_THD_BND4;
  unsigned short LCDB_INTER_MB_QP_THD_INSIDE4;
  unsigned short LCDB_NUM_NC_BLKS_THD;
  unsigned short LCDB_QP_JUMP_NOTCODED_BLKS;
}
VDEC_LCDB_STRENGTH;

typedef struct
{
  /* bScalingEnable: TRUE enables xscaling, FALSE
  **                   disables it.
*/
  unsigned char bScalingEnable;
}
VDEC_XSCALING_ENABLE;

/* Structure containing all the parameter info required for the
** vdec_get_parameter() and vdec_set_parameter() APIs.
*/
typedef union
{
  VDEC_DIMENSIONS                 dimensions;        
  VDEC_START_CODE_DETECTION       startCodeDetection;
  VDEC_TIME_INCREMENT_RESOLUTION  timeIncrementResolution;
  VDEC_SIZE_OF_NAL_LENGTH_FIELD   sizeOfNalLengthField;
  VDEC_DEBLOCKER_ENABLE           deblockerEnable;
  VDEC_FRE_ENABLE                 freEnable;
  VDEC_LCDB_STRENGTH              lcdbStrength;
  VDEC_XSCALING_ENABLE            xScalingEnable;
  VDEC_EXTERNAL_CLOCK_ENABLE	  externalClockEnable;
}
VDEC_PARAMETER_DATA;

/* -----------------------------------------------------------------------
** Global Constant Data Declarations
** ----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
** Global Data Declarations
** ----------------------------------------------------------------------- */

/* =======================================================================
**                          Macro Definitions
** ======================================================================= */

/* =======================================================================
MACRO MYOBJ

ARGS
  xx_obj - this is the xx argument

DESCRIPTION:
  Complete description of what this macro does
========================================================================== */

/* =======================================================================
**                        Function Declarations
** ======================================================================= */

/* ======================================================================
FUNCTION
  SAMPLE_FUNC

DESCRIPTION
  Thorough, meaningful description of what this function does.

DEPENDENCIES
  List any dependencies for this function, global variables, state,
  resource availability, etc.

RETURN VALUE
  Enumerate possible return values

SIDE EFFECTS
  Detail any side effects.

========================================================================== */

#endif /* VDECODER_TYPES_H */

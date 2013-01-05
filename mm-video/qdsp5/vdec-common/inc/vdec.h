/*======================================================================== 
     
                         Video Decoder interface


   */ /** @file vdec.h 
      This file defines the Video decoder interface.

   Copyright (c) 2008 by QUALCOMM, Incorporated.  All Rights Reserved. 
   QUALCOMM Proprietary and Confidential.
*//*====================================================================== */ 
     
/*======================================================================== 
                               Edit History 
     
  $Header: //source/qcom/qct/multimedia2/Video/Dec/VdecCore/main/latest/src/vdec.h#8 $
  $DateTime: 2008/10/10 15:47:44 $
  $Change: 760730 $
     
  when       who     what, where, why 
  --------   ---     ------------------------------------------------------- 
  07/21/08   gra     Initial version of vdec.h 
     
=============================================================================*/

#ifndef _SIMPLE_VDEC_H_
#define _SIMPLE_VDEC_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The following are fourcc hex representations of the video
 * codecs currently supported by the vdec core
 */
#define H264_FOURCC  0x61766331
#define MPEG4_FOURCC 0x646D3476

/**
 * The following frame flags are used to provide additional
 * input/output frame properties
 */
#define FRAME_FLAG_EOS          0x00000001
#define FRAME_FLAG_FLUSHED      0x00000002
#define FRAME_FLAG_FATAL_ERROR  0x00000004

#define VDEC_NFRAMES 8
#define MAX_OUT_BUFFERS 18
#define VDEC_INPUT_SIZE (128 * 1024 * 2)


/** 
 *  A structure which identifies the decoder. 
 *  The stucture is returned when the decoder
 *  is opened and used in subsequent calls to
 *  identify the decoder.
 */
struct VDecoder;

typedef enum Vdec_ReturnType
{
  VDEC_SUCCESS        = 0, /* no error */          
  VDEC_EFAILED        = 1, /* general failure */    
  VDEC_EOUTOFBUFFERS   = 2  /* buffer not available for decode */
}
Vdec_ReturnType;

/**
 * This lists the possible status codes of the output frame.
 */
typedef enum Vdec_StatusType
{
   VDEC_FRAME_DECODE_SUCCESS,
   VDEC_FRAME_DECODE_ERROR,
   VDEC_FATAL_ERROR,
   VDEC_FLUSH_DONE,
   VDEC_END_OF_STREAM,
} Vdec_StatusType;

/** 
 * This lists all the possible encoded frame types 
 */
typedef enum Vdec_PictureType
{
   VDEC_PICTURE_TYPE_I,
   VDEC_PICTURE_TYPE_P,
   VDEC_PICTURE_TYPE_B,
   VDEC_PICTURE_TYPE_BI,
   VDEC_PICTURE_TYPE_SKIP,
   VDEC_PICTURE_TYPE_UNKNOWN
} Vdec_PictureType;

/** 
 * This lists all the possible frame types 
 */
typedef enum Vdec_PictureFormat
{
   VDEC_PROGRESSIVE_FRAME,
   VDEC_INTERLACED_FRAME,
   VDEC_INTERLACED_FIELD
} Vdec_PictureFormat;

/**
 * This lists all the possible picture resolutions 
 */
typedef enum Vdec_PictureRes
{
   VDEC_PICTURE_RES_1x1,
   VDEC_PICTURE_RES_2x1,
   VDEC_PICTURE_RES_1x2,
   VDEC_PICTURE_RES_2x2
} Vdec_PictureRes;


/**
 * This defines the structure of the pan scan parameters used in 
 * Advanced Profile Frame display 
 */ 
#define MAX_VC1_PAN_SCAN_WINDOWS 4
typedef struct Vdec_VC1PanScanRegions
{
   signed long int numWindows;
   signed long int winWidth    [MAX_VC1_PAN_SCAN_WINDOWS];
   signed long int winHeight   [MAX_VC1_PAN_SCAN_WINDOWS];
   signed long int winHorOffset[MAX_VC1_PAN_SCAN_WINDOWS];
   signed long int winVerOffset[MAX_VC1_PAN_SCAN_WINDOWS];
} Vdec_VC1PanScanRegions;

/**
 * This structure specifies the detials of the
 * output frame.
 *
 * status - Status code of the frame
 * userData1 - User data, used internally by the decoder
 * userData2 - unsigned long int representation of the associated input cookie
 * nDecPicWidth - Width of the decoded picture
 * nDecPicHeigh - Height of the decoded picture
 * nDispPicWidth - Width of the picture to display
 * nDispPicHeight - Height of the picture to display
 * ePicType[2] - Coding type of the decoded frame (both fields)
 * ePicFormat - Picture coding format
 * nVC1RangeY - Scale factor for Luma Range Mapping
 * nVC1RangeUV - Scale factor for Chroma Range Mapping
 * ePicResolution - Scaling factor for frame wrt full resolution frame
 * nRepeatProgFrames - Number of times frame may be repeated by display
 * bRepeatFirstField - 1st field can be repeated after 2nd in a pair
 * bTopFieldFirst - Field closer to top to be displayed before bottom
 * bFrameInterpFlag - Data not suitable for inter-frame interpolation
 * panScan - Pan scan window regions
 * reserved1 - reserved field, should be set to 0
 * reserved2 - reserved field, should be set to 0
 */
#define MAX_FIELDS 2
typedef struct Vdec_FrameDetailsType
{
   Vdec_StatusType        status;
   unsigned long int                 userData1;
   unsigned long int                 userData2;
   unsigned long int                 nDecPicWidth;
   unsigned long int                 nDecPicHeight;
   unsigned long int                 nDispPicWidth;
   unsigned long int                 nDispPicHeight;
   Vdec_PictureType       ePicType[MAX_FIELDS];
   Vdec_PictureFormat     ePicFormat;
   unsigned long int                 nVC1RangeY;
   unsigned long int                 nVC1RangeUV;
   Vdec_PictureRes        ePicResolution;
   unsigned long int                 nRepeatProgFrames;
   unsigned long int                 bRepeatFirstField;
   unsigned long int                 bTopFieldFirst;
   unsigned long int                 bFrameInterpFlag;
   Vdec_VC1PanScanRegions panScan;
   unsigned long int                 nPercentConcealedMacroblocks;
} Vdec_FrameDetailsType;

/**
 * This structure specifies the buffer requirements of the
 * decoder.
 */
typedef struct Vdec_BufferRequirements
{
   unsigned long int bufferSize;     /* Buffer size, in bytes */
   unsigned long int numBuffers;     /* Number of buffers */
} Vdec_BufferRequirements;

/**
 * This structure specifies information about the input and output allocated
 * buffers.
 */
typedef struct Vdec_BufferInfo
{
   unsigned char  *base;       /* Starting virtual address */
   unsigned long int bufferSize; /* Buffer size */
   unsigned long int numBuffers; /* Total number of buffers */
} Vdec_BufferInfo;

/**
 * This structure specifies a decoder (input or output) frame.
 */
typedef struct vdec_frame 
{
   unsigned char         *base;         /* virtual base addr of buffer */
   unsigned              phys;         /* physical base addr of buffer */
   signed long long      timestamp;    /* frame timestamp */
   unsigned int          flags;        /* holds frame flags properties */
   Vdec_FrameDetailsType frameDetails; /* Details on decoded frame */
   int pmem_id;                        /* fd */
   unsigned pmem_offset;               /* offset from start of pmem region */
} vdec_frame;

/**
 * This structure defines an input video frame sent to decoder
 */
typedef struct video_input_frame_info 
{
   void            *data;        /* Pointer to the input data */
   unsigned int     len;         /* Input frame length in bytess */
   signed long long timestamp;   /* Timestamp,forwarded from i/p to o/p */
   unsigned int     flags;       /* Flag to indicate EOS */   
   unsigned long int           userData;     /* User data associated with the frame */
   signed long int            avSyncState; /* AV sync state */
} video_input_frame_info;

/**
 * This structure is a decoder context that should be filled up by the client
 * before calling vdec_open
 */
typedef struct vdec_context 
{   
   /* The following fields are to be filled up by the client if it 
    * has the corresponding information.
    */
   unsigned long int  width;                /* Width of the image */
   unsigned long int  height;               /* Height of the image */
   unsigned long int  fourcc;               /* 4cc for type of decoder */
   unsigned char    *sequenceHeader;       /* parameter set */
   unsigned long int  sequenceHeaderLen;    /* length of parameter set*/
   unsigned char bRenderDecodeOrder;   /* Render decode order*/
   unsigned int size_of_nal_length_field;

   /////////////////////////////////To be removed in future.
   char  kind[128];  // new code to pass the role from omx vdec. 
                     // This has to be replaced with fourcc in future
   void (*process_message)(struct vdec_context *ctxt, unsigned char id);
   void *extra;
   //////////////////////////////////
   /* The following fields are to be filled up by the vdec (decoder) 
    * if it has the corresponding information.
    */

   /* Input Output Buffer requirements
    * This might be filled in by vdec if the vdec decides on the
    * buffer requirements.
    */
   Vdec_BufferRequirements inputReq;
   Vdec_BufferRequirements outputReq;

   /* Input/Ouput Buffers
    * These hold the location and size of the input and output
    * buffers. This might be filled up by the vdec after a call
    * to vdec_commit_memory.
    */
   Vdec_BufferInfo inputBuffer;
   Vdec_BufferInfo outputBuffer;

   /* 
    * Callback function that will be called when output frame is complete 
    * call vdec_release_frame() when you are done with it
    */
   void (*frame_done)(struct vdec_context *ctxt, 
                      struct vdec_frame *frame);

   /**
    * Callback function that will be called when input buffer is no longer in 
	* use
    */
   void (*buffer_done)(struct vdec_context *ctxt, void *cookie);

   int adsp_fd;
} vdec_context;

/**
  * This method is to be used to open the decoder.
  *
  *  @param[inout] ctxt
  *     Video decoder context. The client of the decoder should
  *     fill in the appropriate fields in the structure.After
  *     the return from the call, the decoder will fill up
  *     certain fields in the structure.
  * 
  *  @return
  *     Video decoder strucure. This value has to be passed as
  *     the first argument in further calls. A NULL return value
  *     indicates that the decoder could not be opened.
  */
struct VDecoder *vdec_open(struct vdec_context * ctxt);

/**
  * This method is to be used to allocate the memory needed by decoder.
  * This method needs to be called before decoding can begin. Also,
  * vdec_open should have been called before this.
  *
  * Prerequisite: vdec_open should have been called.
  *
  * @param[in] dec
  *     Pointer to the Video decoder.
  * 
  * @return 
  *  
  * 0 - success. Memory is allocated successfully.
  * 
  * -1 - Failure. Memory allocation failed.
  *
  */
Vdec_ReturnType vdec_commit_memory(struct VDecoder *dec);

/**
  * This method is to be used to close a video decoder.
  *
  * Prerequisite: vdec_open should have been called.
  * 
  *  @param[in] dec
  *     Pointer to the Video decoder.
  */
Vdec_ReturnType vdec_close(struct VDecoder *dec);

/**
  * This method is to be used to post an input buffer.
  *
  * Prerequisite: vdec_open should have been called.
  *
  *  @param[in] dec
  *     Pointer to the Video decoder.
  *
  *  @param[in] frame
  *     The input frame to be decoded. The frame should point to an area in
  *     shared memory for WinMo solution.
  *
  *  @param[in] cookie
  *     A cookie for the input frame. The cookie is used to identify the 
  *     frame when the  buffer_done callback function is called.The cookie 
  *     is unused by the decoder otherwise.
  *
  *  @return
  *     0 if the input buffer is posted successfully.
  *     -1 if the input buffer could not be sent to the decoder.
  */
Vdec_ReturnType vdec_post_input_buffer(struct VDecoder *dec, 
                                       struct video_input_frame_info *frame,
                                       void *cookie);

/**
  * This method is used to release an output frame back to the decoder.
  *
  * Prerequisite: vdec_open should have been called.
  *
  *  @param[in] dec
  *     Pointer to the Video decoder.
  *
  *  @param[in] frame
  *     The released output frame.
  */
Vdec_ReturnType vdec_release_frame(struct VDecoder *dec, 
                                   struct vdec_frame *frame);

/**
  * This method is used to flush the frames in the decoder.
  *
  * Prerequisite: vdec_open should have been called.
  *
  *  @param[in] dec
  *     Pointer to the Video decoder.
  *
  *  @param[in] nFlushedFrames
  *     The number of flushed frames. A value of -1 indicates that n
  *     information is available on number of flushed frames.
  */
Vdec_ReturnType vdec_flush(struct VDecoder* dec, int *nFlushedFrames);


#ifdef __cplusplus
}
#endif

#endif

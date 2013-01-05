#ifndef VENC_DEV_API_H
#define VENC_DEV_API_H
/*===========================================================================

V E N C _ D E V _ A P I . H

DESCRIPTION

This header file defines the interface to the Video Encoder Device Layer.

REFERENCES


Copyright (c) 2007 by Qualcomm, Incorporated. All Rights Reserved. 
Qualcomm Proprietary and Confidential.
===========================================================================*/

/*===========================================================================

EDIT HISTORY FOR FILE

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/device/inc/venc_dev_api.h#4 $


when        who     what, where, why
(ISO Date)
--------    ---     ----------------------------------------------------------
2010/03/24  as      Added scaling factor to support HFR
2009/09/24  as      Added Level 2 for H264 codec.
2009/09/24  as      Added new parameter in syntax header
2009/08/24  bc      H264 level updates
2009/08/07  bc      Initial revision
===========================================================================*/

/*===========================================================================

INCLUDE FILES FOR MODULE

===========================================================================*/
#ifdef __cplusplus
extern "C"
{
#endif

#include "venc_def.h"

/*===========================================================================

DATA DECLARATIONS

===========================================================================*/

// Handle assigned when VENC_CMD_LOAD fails
#define VENC_INVAL_HANDLE        0xffffffff

/*--------------------------------------------------------------------------
TYPEDEF VENC_STATUS_TYPE

The VENC_STATUS_TYPE enumeration defines the standard VENC Device layer
status.

VENC Device returns status in one of the four ways:

1. All VENC Device API functions return a status synchrnously.
2. Status reporting in response to command completion through status_cb.
3. Status reporting in response to an unsolicited error through status_cb.
4. Status reporting regarding to a frame through frame_cb.

VENC_STATUS_SUCCESS
   In the context of synchronous return, this means the request has been
   accepted.
   In the context of frame_cb, this means the frame was encoded with
   no errors detected.
   In the context of event_cb for command, this means the command was
   proceeded with no errors detected.
VENC_STATUS_FAILURE
   TBD
VENC_STATUS_INVALID_STATE
   This error status is returned synchrnously when a command is issued in
   incorrect state.
VENC_STATUS_INSUFFICIENT_RESOURCES
   This error status indicates an irrecoverable error due to insufficient
   resources.
VENC_STATUS_RESOUCES_LOST
   This error status indicates an irrecoverable error due to resoures lost
   or possibly preemption.
VENC_STATUS_NOT_ENCODED
   This error indicates a frame is not encoded.
VENC_STATUS_FLUSHING
   Synchronous status indicates invalid command while flushing.
   Async status indicates buffer was released while flushing and that buffer 
   should not be processed.
--------------------------------------------------------------------------*/
typedef enum venc_status_type
{
   VENC_STATUS_SUCCESS = 0,
   VENC_STATUS_FAILURE,
   VENC_STATUS_INVALID_STATE,
   VENC_STATUS_INSUFFICIENT_RESOURCES,
   VENC_STATUS_RESOUCES_LOST,
   VENC_STATUS_NOT_ENCODED,
   VENC_STATUS_FLUSHING
} venc_status_type;

/*--------------------------------------------------------------------------
TYPEDEF VENC_CODING_TYPE

This enumeration defines all supported coding types.
--------------------------------------------------------------------------*/
typedef enum venc_coding_type
{
   VENC_CODING_MPEG4,
   VENC_CODING_H263,
   VENC_CODING_H264
} venc_coding_type;
/*--------------------------------------------------------------------------
TYPEDEF VENC_MPEG4_PROFILE_TYPE

This enumeration defines all supported/unsupported MPEG4 profile types.
--------------------------------------------------------------------------*/
typedef enum venc_mpeg4_profile_type
{
   VENC_MPEG4_PROFILE_SIMPLE,             ///< Simple Profile, Levels 1-3
   VENC_MPEG4_PROFILE_SIMPLESCALABLE,     ///< Unsupported
   VENC_MPEG4_PROFILE_CORE,               ///< Unsupported
   VENC_MPEG4_PROFILE_MAIN,               ///< Unsupported
   VENC_MPEG4_PROFILE_NBIT,               ///< Unsupported
   VENC_MPEG4_PROFILE_SCALABLETEXTURE,    ///< Unsupported
   VENC_MPEG4_PROFILE_SIMPLEFACE,         ///< Unsupported
   VENC_MPEG4_PROFILE_SIMPLEFBA,          ///< Unsupported
   VENC_MPEG4_PROFILE_BASICANIMATED,      ///< Unsupported
   VENC_MPEG4_PROFILE_HYBRID,             ///< Unsupported
   VENC_MPEG4_PROFILE_ADVANCEDREALTIME,   ///< Unsupported
   VENC_MPEG4_PROFILE_CORESCALABLE,       ///< Unsupported
   VENC_MPEG4_PROFILE_ADVANCEDCODING,     ///< Unsupported
   VENC_MPEG4_PROFILE_ADVANCEDCORE,       ///< Unsupported
   VENC_MPEG4_PROFILE_ADVANCEDSCALABLE    ///< Unsupported
} venc_mpeg4_profile_type;

/*--------------------------------------------------------------------------
TYPEDEF VENC_MPEG4_LEVEL_TYPE

This enumeration defines all supported MPEG4 levels.
--------------------------------------------------------------------------*/
typedef enum venc_mpeg4_level_type
{
   VENC_MPEG4_LEVEL_0,
   VENC_MPEG4_LEVEL_0B,
   VENC_MPEG4_LEVEL_1,
   VENC_MPEG4_LEVEL_2,
   VENC_MPEG4_LEVEL_3,
   VENC_MPEG4_LEVEL_4,
   VENC_MPEG4_LEVEL_4A,
   VENC_MPEG4_LEVEL_5
} venc_mpeg4_level_type;

/*--------------------------------------------------------------------------
TYPEDEF VENC_MPEG4_PARAM_TYPE

This structure defines a container for MPEG4-specific parameters.
--------------------------------------------------------------------------*/
typedef struct venc_mp4_profile_level_type
{
   venc_mpeg4_profile_type mp4_profile;
   venc_mpeg4_level_type   mp4_level;
} venc_mp4_profile_level_type;

/*--------------------------------------------------------------------------
TYPEDEF VENC_H263_PROFILE_TYPE

This enumeration defines all supported/unsupported H263 profiles.
--------------------------------------------------------------------------*/
typedef enum venc_h263_profile_type
{
   VENC_H263_PROFILE_BASELINE,
   VENC_H263_PROFILE_H320_CODING,
   VENC_H263_PROFILE_BACKWARD_COMPATIBLE,
   VENC_H263_PROFILE_ISWV2,               ///< includes annex j, k, and t
   VENC_H263_PROFILE_ISWV3,
   VENC_H263_PROFILE_HIGH_COMPRESSION,
   VENC_H263_PROFILE_INTERNET,
   VENC_H263_PROFILE_INTERLACE,
   VENC_H263_PROFILE_HIGH_LATENCY
} venc_h263_profile_type;

/*--------------------------------------------------------------------------
TYPEDEF VENC_H263_LEVEL_TYPE

This enumeration defines all supported/unsupported H263 levels.
--------------------------------------------------------------------------*/
typedef enum venc_h263_level_type
{
   VENC_H263_LEVEL_10,
   VENC_H263_LEVEL_20,
   VENC_H263_LEVEL_30,  ///< Unsupported
   VENC_H263_LEVEL_40,  ///< Unsupported
   VENC_H263_LEVEL_45,  ///< Unsupported
   VENC_H263_LEVEL_50,  ///< Unsupported
   VENC_H263_LEVEL_60,  ///< Unsupported
   VENC_H263_LEVEL_70   ///< Unsupported
} venc_h263_level_type;

/*--------------------------------------------------------------------------
TYPEDEF VENC_H263_PROFILE_LEVEL_TYPE

This structure defines a container type for all supported H263 specific
parameters.
--------------------------------------------------------------------------*/
typedef struct venc_h263_profile_level_type
{
   venc_h263_profile_type  h263_profile;
   venc_h263_level_type    h263_level;
} venc_h263_profile_level_type;

/*--------------------------------------------------------------------------
TYPEDEF VENC_H264_PROFILE_TYPE

This enumeration defines all supported/unsupported H264 profiles.
--------------------------------------------------------------------------*/
typedef enum venc_h264_profile_type
{
   VENC_H264_PROFILE_BASELINE,
   VENC_H264_PROFILE_MAIN,
   VENC_H264_PROFILE_EXTENDED,
   VENC_H264_PROFILE_HIGH
} venc_h264_profile_type;

/*--------------------------------------------------------------------------
TYPEDEF VENC_H264_LEVEL_TYPE

This enumeration defines all supported/unsupported H264 levels.
--------------------------------------------------------------------------*/
typedef enum venc_h264_level_type
{
   VENC_H264_LEVEL_1,
   VENC_H264_LEVEL_1b,
   VENC_H264_LEVEL_11,
   VENC_H264_LEVEL_12,
   VENC_H264_LEVEL_13,
   VENC_H264_LEVEL_2,
   VENC_H264_LEVEL_121,
   VENC_H264_LEVEL_122
} venc_h264_level_type;

/*--------------------------------------------------------------------------
TYPEDEF VENC_H264_PROFILE_LEVEL_TYPE

This structure defines a container type for all supported H264 specific
parameters.
--------------------------------------------------------------------------*/
typedef struct venc_h264_profile_level_type
{
   venc_h264_profile_type  h264_profile;
   venc_h264_level_type    h264_level;
} venc_h264_profile_level_type;

/*--------------------------------------------------------------------------
TYPEDEF VENC_CODING_PROFILE_LEVEL_TYPE

This union defines parameters spcific to a coding type.
--------------------------------------------------------------------------*/
typedef union venc_coding_profile_level_type
{
   venc_mp4_profile_level_type  mp4_profile_level;
   venc_h263_profile_level_type h263_profile_level;
   venc_h264_profile_level_type h264_profile_level;
} venc_coding_profile_level_type;

/*--------------------------------------------------------------------------
TYPEDEF VENC_ROTATION_TYPE

Types of rotations supported by VENC.
--------------------------------------------------------------------------*/
typedef enum venc_rotation_type
{
   VENC_ROT_0,
   VENC_ROT_90,
   VENC_ROT_180,
   VENC_ROT_270
} venc_rotation_type;

/*--------------------------------------------------------------------------
TYPEDEF VENC_QUALITY_TYPE

This structure defines the encoder quality type.

In VENC Device layer, encoder quality is represented a 4-tuple defined
as below.

target_bitrate
   Target bitrate per second for the encoded video.
max_frame_rate
   Target frame rate per second for the encoded video.
min_qp
   Min quantization parameter value. Valid range is [1,31] for MP4.
max_qp
   Max quantization parameter value. Valid range is [1,31] for MP4.
--------------------------------------------------------------------------*/
typedef struct venc_quality_type
{
   int32    target_bitrate;
   int32    max_frame_rate;
   uint16   min_qp;
   uint16   max_qp;
   double   scaling_factor;
} venc_quality_type;

/*--------------------------------------------------------------------------
TYPEDEF VENC_REGION_OF_INT_TYPE

This structure defines the data for client to specify a region of
interest.
--------------------------------------------------------------------------*/
typedef struct venc_region_of_int_type
{
   uint32* roi_array;
   uint32  roi_size;
} venc_region_of_int_type;

/*--------------------------------------------------------------------------
TYPEDEF VENC_INTRA_REFRESH_TYPE

This struct defines the number of I - MB's in P frame
--------------------------------------------------------------------------*/

typedef struct venc_intra_refresh_type
{
   uint32 nMBCount;
} venc_intra_refresh_type;

/*--------------------------------------------------------------------------
TYPEDEF VENC_RESYNC_MARKER_TYPE

This enum defines the type of resync marker to be selected (if any)
--------------------------------------------------------------------------*/
typedef enum venc_resync_marker_type
{
   VENC_RM_NONE,
   VENC_RM_BITS,
   VENC_RM_MB,
   VENC_RM_GOB
} venc_resync_marker_type;

/*--------------------------------------------------------------------------
TYPEDEF VENC_ERROR_RESILIENCE_TYPE
This struct defines error resilience scheme for the encoder
--------------------------------------------------------------------------*/

typedef struct venc_error_resilience_type
{
   venc_resync_marker_type       rm_type;               ///< resync marker configuration
   uint32                        rm_spacing;            ///< if rm, num bits/mbs/gobs between markers
   uint32                        hec_interval;          ///< 0 => disable
                                                        ///< non zero => units of slices or resync markers between header extension
} venc_error_resilience_type;

/*--------------------------------------------------------------------------
TYPEDEF VENC_RC_TYPE

This enumeration defines all supported rate control algorithms.
--------------------------------------------------------------------------*/
typedef enum venc_rc_type
{
   VENC_RC_NONE,      ///< no rate control
   VENC_RC_CBR_VFR,   ///< constant bit rate, variable frame rate
   VENC_RC_VBR_CFR,   ///< variable bit rate, constant frame rate
   VENC_RC_VBR_VFR,   ///< variable bit rate, variable frame rate
   VENC_RC_VBR_VFR_TIGHT    ///< constant bit rate, constant frame rate
} venc_rc_type;

/*--------------------------------------------------------------------------
TYPEDEF VENC_CONFIG_PARAM_TYPE

This structure defines all configurable parameters exposed by VENC
Device Layer API. 

venc_set_param() can be called after venc_load() but before venc_init().
In other words, parameters can only be accessed when VENC Device is in a
loaded but otherwise idle state. If VENC Device is already running,
client must bring the running VENC back to loaded/idle state by calling
venc_stop() and venc_exit() in order to change parameters.

Certain "parameters" can be configured on the run. These configurations
are exposed through seperate API functions.
--------------------------------------------------------------------------*/
typedef struct
{
   /// Type of coding to perform
   venc_coding_type                coding_type;

   /// profile and level configurations
   venc_coding_profile_level_type  coding_profile_level;

   /// input frame width
   uint32                          frame_width;

   /// input frame height
   uint32                          frame_height;

   /// camera rotation
   venc_rotation_type              rotation;

   /// bitrate framerate min/max QPs
   venc_quality_type               encoder_quality;

   /// intra period selection
   uint32                          intra_period;

   /// MP4 time increment resolution
   uint16                          time_resolution;

   /// enable ac prediction flag
   boolean                         ac_pred_on;

   /// syntax header injection flag for key frames
   boolean                         iframe_vol_inject;

   /// intra refresh type
   venc_intra_refresh_type         intra_refresh;

   /// list of macroblocks to prioritize/optimize
   venc_region_of_int_type         region_of_interest;

   /// error resilienc scheme
   venc_error_resilience_type      error_resilience;

   /// rate control flavors
   venc_rc_type                    rc_selection;

   /// initial qp for iframe
   uint32                          init_qp_iframe;

   /// initial qp for pframe
   uint32                          init_qp_pframe;

   /// initial qp for bframe
   uint32                          init_qp_bframe;
   
   /// if true, no syntax header will be attached to the first frame
   boolean                         skip_syntax_hdr;

} venc_config_param_type;


/*--------------------------------------------------------------------------
TYPEDEF VENC_DVS_OFFSET_TYPE

This structure defines the data type to represent DVS offset.
--------------------------------------------------------------------------*/
typedef struct
{
   uint32 x;
   uint32 y;
} venc_frm_offset_type;

/*--------------------------------------------------------------------------
TYPEDEF VENC_COMMAND_TYPE

This enumeration specifies what command a command-based event_cb is for.
--------------------------------------------------------------------------*/
typedef enum
{
   /// Indicates load command has been processed
   VENC_CMD_LOAD = 0,

   /// Indicates init command has been processed
   VENC_CMD_INIT,

   /// Indicates start command has been processed
   VENC_CMD_START,

   /// Indicates stop command has been processed
   VENC_CMD_STOP,

   /// Indicates pause command has been processed
   VENC_CMD_PAUSE,

   /// Ignored
   VENC_CMD_ENCODE_FRAME,

   /// Indicates exit command has been processed
   VENC_CMD_EXIT,

   /// Indicates intra period command has been processed
   VENC_CMD_SET_INTRA_PERIOD,

   /// Indicates syntax header request has been processed
   VENC_CMD_REQUEST_SYNTAX_HEADER,

   /// Indicates intra refresh command has been processed
   VENC_CMD_SET_INTRA_REFRESH,

   /// Indicates flush command has been processed
   VENC_CMD_FLUSH,

   /// Indicates channel stats registration command has been processed
   VENC_CMD_REGISTER_CHANNEL_STATS_CB,

   /// Indicates unload command has been processed
   VENC_CMD_UNLOAD,

   /// Indicates change quality command has been processed
   VENC_CMD_CHANGE_QUALITY,

   /// Indicates n-frame depth command has been processed
   VENC_CMD_SET_N_FRAME_DEPTH,

   /// Indicates region of interest command has been processed
   VENC_CMD_SET_REGION_OF_INTEREST,

   /// Indicates key-frame request has been processed
   VENC_CMD_REQUEST_KEY_FRAME,
   
   /// Any commands above this value are considered private
   VENC_CMD_PRIVATE
} venc_command_type;

/*--------------------------------------------------------------------------
TYPEDEF VENC_EVENT_TYPE

This enumeration defines the range of possible event types a status
callback communicates.

VENC_EVENT_CMD_COMPLETE
   When a command processing is completed, status callback will report
   the command processing result with this event type.
VENC_EVENT_ASYNC
   This event type corresponds to any unsolicited status callbacks.
VENC_EVENT_INPUT_HANDSHAKE
   This event type indicates to client that the input frame (corresponding
   to the timestamp) can be released since VENC Device Layer has done
   DSP encoding.
--------------------------------------------------------------------------*/
typedef enum venc_event_type
{
   VENC_EVENT_CMD_COMPLETE,
   VENC_EVENT_ASYNC,
   VENC_EVENT_INPUT_HANDSHAKE,
} venc_event_type;

typedef struct venc_cmd_status_type
{
   venc_command_type     cmd_complete;
   venc_status_type      cmd_status;
} venc_cmd_status_type;

typedef struct venc_async_status_type
{
   venc_status_type      async_status;
} venc_async_status_type;

typedef struct venc_handshake_status_type
{
   venc_status_type      handshake_status;
   void*                 input_client_data;
   boolean               eos;
   uint64                timestamp;
} venc_handshake_status_type;

/*--------------------------------------------------------------------------
TYPEDEF VENC_STATUS_PARAM_TYPE

Used to report event-specific status parameters in the status callback. Note
that two status parameters are included with each callback. Refer to 
documentation for venc_status_cb_ptr_type for each parameter's intended
interpretation. 

cmd_status
   Used in conjunction with VENC_EVENT_CMD_COMPLETE
async_status
   Used in conjunction with VENC_EVENT_ASYNC
handshake_status
   Used in conjunction with VENC_EVENT_INPUT_HANDSHAKE
--------------------------------------------------------------------------*/
typedef union
{
   venc_cmd_status_type          cmd_status;
   venc_async_status_type        async_status;
   venc_handshake_status_type    handshake_status;
} venc_status_param_type;

/*--------------------------------------------------------------------------
TYPEDEF VENC_STATUS_CB_PTR_TYPE

This typedef specifies the signature of VENC Device Layer API status
callback function type. This callback must be registered at the time
of venc_init()

handle
   The handle the CB is intended for.
status_event
   The event the callback function is in response to.
   Note that the status callback is never used to report
   VENC_CMD_ENCODE_FRAME.
   Encoding status is always reported through the frame
   callback.
event_desc1
   If status_event is for VENC_EVENT_CMD_COMPLETE, this
   parameter shall specify a value of venc_command_type.

   If status_event is for VENC_EVENT_ASYNC or VENC_EVENT_INPUT_HANDSHAKE
   this parameter shall specify a value of venc_status_type
event_desc2
   If status_event is for VENC_EVENT_CMD_COMPLETE, this
   parameter shall specify a value of venc_status_type
   to inform client of command processing status for the
   cmd indicated in event_desc1.

   If status_event is for VENC_EVENT_ASYNC, this parameter
   is NOT used.

   The only EXCEPTION is if the EVENT_ASYNC in question is
   VENC_EVENT_INPUT_HANDSHAKE, in which case this parameter
   specifies the timestamp of the input frame Device Layer is
   releasing.
client_data
   Client specific data.
--------------------------------------------------------------------------*/
typedef void  (*venc_status_cb_ptr_type)
(
   uint32                  handle,
   venc_event_type         status_event,
   venc_status_param_type  param,
   void                    *client_data
);
/*--------------------------------------------------------------------------
TYPEDEF VENC_COLOR_FORMAT

This definition specifies the color formats for the input frames.
--------------------------------------------------------------------------*/
typedef enum venc_color_format_type
{
   VENC_YUV420_PLANAR,
   VENC_YUV420_PACKED_PLANAR
} venc_color_format_type;

/*--------------------------------------------------------------------------
TYPEDEF VENC_FRAME_INFO_TYPE

This structure defines the data type of the information associated with
an encoded frame.
--------------------------------------------------------------------------*/
typedef struct
{
   uint32   *frame_ptr;     ///< Pointer to coded frame data
   uint32   len;            ///< Length of coded frame in bytes
   int8     frame_type;     ///< 0=iframe, 1=pframe
   uint64   timestamp;      ///< Sampling time of video frame (microseconds)
   uint16   header_size;    ///< Packet Head Size in bits
   byte     header_buf[32]; ///< Packet Header Buffer
   boolean  last_seg;       ///< Indicates if this is last segment of a frame
   void*    output_client_data;
   boolean  eos;
} venc_frame_info_type;

/*--------------------------------------------------------------------------
TYPEDEF VENC_FRAME_INFO_TYPE

This structure defines the data type of the information associated with
an encoded frame.
--------------------------------------------------------------------------*/
typedef struct 
{
    void* handle;       // platform specific handle (typecast as necessary)
    uint32 offset;      // offset to start of buffer (e.g. if allocated from a pool)
    uint32 size;        // size of the buffer
    void* buff;         // the virtual address of the buffer
} venc_pmem_info;


/*--------------------------------------------------------------------------
TYPEDEF VENC_FRAME_CB_PTR_TYPE

This typedef specifies the data callback function signature client must
implement and register with VENC Device API. The callback must be
registered with VENC Device layer at the time of venc_start().

The frame callback is used to report the coding status. If the status
does not indicate VENC_STATUS_SUCCESS all venc_frame_info_type fields
except 'handle' and 'timestamp'.
--------------------------------------------------------------------------*/
typedef void (*venc_frame_cb_ptr_type)
(
   uint32               handle,
   venc_status_type     status,
   venc_frame_info_type frame_info,
   void                 *client_data
);

/*--------------------------------------------------------------------------
TYPEDEF VENC_CHANNEL_STATES_TYPE

This structure defines the data type for channel statistics VENC will
poll (through a registered callback of venc_channel_stats_cb_ptr_type).
--------------------------------------------------------------------------*/
typedef struct venc_channel_stats_type
{
   uint32 buffer_occupancy;
   uint32 channel_error;
} venc_channel_stats_type;

/*--------------------------------------------------------------------------
TYPEDEF VENC_CHANNEL_STATS_CB_PTR_TYPE

This typedef specifies the callback function signature client may wish
to implement and register with VENC Device API. If registered, VENC will
invoke the registered callback on a per-frame basis and makes real-time
encoding adjustments and decisions based on the channel statistics.
--------------------------------------------------------------------------*/
typedef void (*venc_channel_stats_cb_ptr_type)
(
   uint32                     handle,
   venc_channel_stats_type    *channel_stats,
   void                       *client_data
);

/*--------------------------------------------------------------------------
TYPEDEF VENC_ENCODE_FRAME_PARAM_TYPE

This structure specifies the information client needs to provide with
each input frame to be encoded.
--------------------------------------------------------------------------*/
typedef struct venc_encode_frame_param_type
{
   venc_pmem_info          luma;
   venc_pmem_info          chroma;
   uint32                  *video_bitstream;     // ptr to the output bitstream
   int32                   video_bitstream_size; // size in bytes
   uint64                  timestamp;
   venc_color_format_type  color;
   venc_frm_offset_type    offset;
   void*                   input_client_data;
   void*                   output_client_data;
   boolean                 eos;
} venc_encode_frame_param_type;

/*===========================================================================

API FUNCTIONS DEFINITION

===========================================================================*/


/*===========================================================================
FUNCTION ()

DESCRIPTION

DEPENDENCIES

PARAMETERS

RETURN VALUE

IMPLEMENTATION NOTES

===========================================================================*/
void venc_sys_up
(
   void* env_data
);

/*===========================================================================
FUNCTION ()

DESCRIPTION

DEPENDENCIES

PARAMETERS

RETURN VALUE

IMPLEMENTATION NOTES

===========================================================================*/
void venc_sys_down
(
   void
);


/*===========================================================================
FUNCTION VENC_LOAD()

DESCRIPTION
This is the entry point to VENC Device layer. A client must invoke
this function to obtain a valid handle prior to any other VENC-related
operations. Client will identify itself with the obtained handle for
all subsequent VENC interactions.

DEPENDENCIES
None.

PARAMETERS
(r/w)  pointer to an uint32 to store the handle assignment.

RETURN VALUE
VENC_STATUS_SUCCESS - a valid handle will be assigned
VENC_STATUS_FAILURE - cannot allocate valid handle

IMPLEMENTATION NOTES

===========================================================================*/
venc_status_type venc_load
(
   venc_status_cb_ptr_type    status_cb,
   void                       *status_client_data
);
/*===========================================================================
FUNCTION VENC_UNLOAD()

DESCRIPTION
Client reliquishes handle when it is done with VEND Device layer.

DEPENDENCIES
This function should only be called when client has clened up VENC Device.

PARAMETERS
(r) handle to be reliquished.

RETURN VALUE
VENC_STATUS_SUCCESS - Client has successfully unloaded the device layer.
VENC_STATUS_FAILURE - Device layer cannot be unloaded.

IMPLEMENTATION NOTES

===========================================================================*/
venc_status_type venc_unload
(
   uint32 handle
);

/*===========================================================================
FUNCTION VENC_INIT()

DESCRIPTION

DEPENDENCIES

PARAMETERS

RETURN VALUE

IMPLEMENTATION NOTES

===========================================================================*/
venc_status_type venc_init
(
   uint32                  handle,
   venc_config_param_type  *param,
   venc_frame_cb_ptr_type  frame_cb,
   void                    *frame_client_data
);

/*===========================================================================

FUNCTION VENC_START()

DESCRIPTION

DEPENDENCIES

PARAMETERS

RETURN VALUE

IMPLEMENTATION NOTES

===========================================================================*/
venc_status_type venc_start
(
   uint32                        handle
);

/*===========================================================================
FUNCTION VENC_STOP()

DESCRIPTION

DEPENDENCIES

PARAMETERS

RETURN VALUE

IMPLEMENTATION NOTES
Once device layer has been commanded to stop, the stop status will not
be issued until device layer has coded all frames in its queue.

===========================================================================*/
venc_status_type venc_stop
(
   uint32 handle
);

/*===========================================================================
FUNCTION VENC_PAUSE()

DESCRIPTION

DEPENDENCIES

PARAMETERS

RETURN VALUE

IMPLEMENTATION NOTES

===========================================================================*/
venc_status_type venc_pause
(
   uint32 handle
);

/*===========================================================================
FUNCTION VENC_EXIT()

DESCRIPTION

DEPENDENCIES

PARAMETERS

RETURN VALUE

IMPLEMENTATION NOTES

===========================================================================*/
venc_status_type venc_exit
(
   uint32 handle
);

/*===========================================================================
FUNCTION VENC_REQUEST_KEY_FRAME()

DESCRIPTION

DEPENDENCIES

PARAMETERS

RETURN VALUE

IMPLEMENTATION NOTES

===========================================================================*/
venc_status_type venc_request_key_frame
(
   uint32 handle
);


/*===========================================================================
FUNCTION VENC_CHANGE_QUALITY()

DESCRIPTION

DEPENDENCIES

PARAMETERS

RETURN VALUE

IMPLEMENTATION NOTES

===========================================================================*/
venc_status_type venc_change_quality
(
   uint32            handle,
   venc_quality_type *quality
);

/*===========================================================================
FUNCTION VENC_ENCODE_FRAME()

DESCRIPTION

DEPENDENCIES

PARAMETERS

RETURN VALUE

IMPLEMENTATION NOTES

===========================================================================*/
venc_status_type venc_encode_frame
(
   uint32                        handle,
   venc_encode_frame_param_type  *param
);

/*===========================================================================
FUNCTION VENC_SET_INTRA_PERIOD()

DESCRIPTION

DEPENDENCIES

PARAMETERS

RETURN VALUE

IMPLEMENTATION NOTES

===========================================================================*/
venc_status_type venc_set_intra_period
(
   uint32   handle,
   uint16   intra_period
);

/*===========================================================================
FUNCTION VENC_SET_NFRAME_DEPTH()

DESCRIPTION

DEPENDENCIES

PARAMETERS

RETURN VALUE

IMPLEMENTATION NOTES

===========================================================================*/
venc_status_type venc_set_nframe_depth
(
   uint32 handle,
   uint32 n_frame_depth
);

/*===========================================================================
FUNCTION VENC_SET_REGION_OF_INTEREST()

DESCRIPTION

DEPENDENCIES

PARAMETERS

RETURN VALUE

IMPLEMENTATION NOTES

===========================================================================*/
venc_status_type venc_set_region_of_interest
(
   uint32 handle,
   venc_region_of_int_type *roi
);

/*===========================================================================
FUNCTION VENC_SET_INTRA_REFRESH()

DESCRIPTION

DEPENDENCIES

PARAMETERS

RETURN VALUE

IMPLEMENTATION NOTES

===========================================================================*/
venc_status_type venc_set_intra_refresh
(
   uint32 handle,
   uint32 num_mbs
);

/*===========================================================================
FUNCTION VENC_SET_ERROR_RESILIENCE()

DESCRIPTION

DEPENDENCIES

PARAMETERS

RETURN VALUE

IMPLEMENTATION NOTES

===========================================================================*/
venc_status_type venc_set_error_resilience
(
   uint32 handle,
   venc_error_resilience_type error_resilience
);

/*===========================================================================
FUNCTION VENC_GET_SYNTAX_HDR()

DESCRIPTION
Synchronously returns syntax header in the supplied buffer.

DEPENDENCIES

PARAMETERS
handle - Handle to device layer instance
byte_buffer - buffer for syntax header storage
bytebuffer_size - size of buffer in bytes
header_size - number of bytes stored in buffer (output param)

RETURN VALUE
VENC_STATUS_SUCCESS if successful
VENC_STATUS_FAILURE if buffer is too small

IMPLEMENTATION NOTES

===========================================================================*/
venc_status_type venc_get_syntax_hdr
(
   uint32   handle,
   uint32   *byte_buffer,
   int32    bytebuffer_size,
   int32                   *header_size,
   venc_config_param_type  *param
);

/*===========================================================================
FUNCTION VENC_REQUEST_SYNTAX_HDR()

DESCRIPTION

DEPENDENCIES

PARAMETERS

RETURN VALUE

IMPLEMENTATION NOTES

===========================================================================*/
venc_status_type venc_request_syntax_hdr
(
   uint32   handle
);

/*===========================================================================
FUNCTION VENC_FLUSH()

DESCRIPTION

DEPENDENCIES

PARAMETERS
@param handle The device layer handle

RETURN VALUE

IMPLEMENTATION NOTES

===========================================================================*/
venc_status_type venc_flush
(
   uint32 handle
);


/*===========================================================================
FUNCTION VENC_REGISTER_CHANNEL_STAT_CB()

DESCRIPTION

DEPENDENCIES

PARAMETERS

RETURN VALUE

IMPLEMENTATION NOTES

===========================================================================*/
venc_status_type venc_register_channel_stats_cb
(
   uint32                           handle,
   venc_channel_stats_cb_ptr_type   stats_cb,
   void                             *client_data
);

/*===========================================================================
FUNCTION VENC_PMEM_ALLOC()

DESCRIPTION

DEPENDENCIES

PARAMETERS

RETURN VALUE

IMPLEMENTATION NOTES

===========================================================================*/
venc_status_type venc_pmem_alloc
(
   uint32                           handle,
   uint32                           size,
   venc_pmem_info                   *pmem
);

/*===========================================================================
FUNCTION VENC_PMEM_FREE()

DESCRIPTION

DEPENDENCIES

PARAMETERS

RETURN VALUE

IMPLEMENTATION NOTES

===========================================================================*/
venc_status_type venc_pmem_free
(
   uint32                           handle,
   venc_pmem_info                   *pmem
);

#ifdef __cplusplus
}                               /* closing brace for extern "C" */
#endif

#endif                          /* VENCDRV_H */


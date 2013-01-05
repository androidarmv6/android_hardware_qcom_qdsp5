#ifndef VDL_TYPES_I_H
#define VDL_TYPES_I_H

/* =======================================================================
                               VDL_Types_i.h
DESCRIPTION This file is included internally within Driver layer
  
Copyright (c) 2007 by QUALCOMM, Incorporated.  All Rights Reserved. 
QUALCOMM Proprietary and Confidential.
========================================================================== */

/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/QDSP5VdecDriver/main/latest/src/VDL_Types_i.h#3 $
$DateTime: 2008/11/24 16:46:18 $
$Change: 791733 $

========================================================================== */

/*==========================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */
#include "VDL_Types.h"
extern "C"
{
	#include "adsprtossvc.h"
}

#define VDL_SQCIF_WIDTH           128
#define VDL_SQCIF_HEIGHT           96
#define VDL_QCIF_WIDTH            176
#define VDL_QCIF_HEIGHT           144
#define VDL_CIF_WIDTH             352
#define VDL_CIF_HEIGHT            288
#define VDL_ONEEIGHTH_VGA_WIDTH   240
#define VDL_ONEEIGHTH_VGA_HEIGHT  160
#define VDL_ONEQUARTER_VGA_WIDTH  320
#define VDL_ONEQUARTER_VGA_HEIGHT 240
#define VDL_ONEHALF_VGA_WIDTH     480
#define VDL_ONEHALF_VGA_HEIGHT    368
#define VDL_VGA_WIDTH             640
#define VDL_VGA_HEIGHT            480
#define VDL_WVGA_WIDTH            800 
#define VDL_WVGA_HEIGHT           480
#define VDL_240_WIDTH             240
#define VDL_192_HEIGHT            192

typedef enum
{
  IDLE = 0,
  READY,
  ERROR
}VDL_state_type;        /* state of the VDL instance should be one of these */

typedef enum
{
  VDL_SUCCESS          = 0, /* cmd sucessful */    
  VDL_FAILURE,   
  VDL_FAILURE_FATAL,
  VDL_INVALID_STATE,
  VDL_INVALID_PARM             
} VDL_Return_Code_Type;                 /* return codes used while processing commands */

typedef enum
{
  HIGH = 0,
  MEDIUM,
  NONE
}VDL_priority_type;    /* used to maintain priority of the commmand to be processed, is also used by
                         scheduler to maintain instance priorities */

typedef enum
{
  ARM_MESSAGE_TYPE = 0,
  DSP_MESSAGE_TYPE,
  NO_MSG_TYPE
}VDL_msg_type;                       /* message types are ARM and DSP*/

typedef struct
{
  adsp_rtos_module_type module;
   unsigned char ack;
} VDL_QDSP_Download_Done_Info_Type;

typedef struct
{
  unsigned short   FatalErrorReg;   /* QDSP error register value */
} VDL_QDSP_Fatal_Error_Info_Type;

/*
** Define the decode ready information structure
**/
typedef struct
{
  unsigned short   SubframeQueueDepth; /* Holds queue depth on QDSP */
} VDL_QDSP_Decode_Ready_Info_Type;

/*
** Define the subframe done information structure
**/
typedef struct
{
  unsigned short   PacketSeqNum;       /* Holds subframe packet sequence num */
  unsigned short   SubframeQueueDepth; /* Holds queue depth on QDSP */
} VDL_QDSP_Subframe_Done_Info_Type;

typedef struct
{
  unsigned short FrameAck;
} VDL_QDSP_Frame_Done_Info_Type;

typedef struct
{
  unsigned short DisplayWorthy;
} VDL_QDSP_PP_Frame_Done_Info_Type;

typedef enum
{
  VDL_QDSP_MOD_READY       = 0, 
  VDL_QDSP_MOD_DISABLE,
  VDL_QDSP_INIT_STATUS,             
  VDL_QDSP_ACTIVE_ACK,  
  VDL_QDSP_DECODE_READY,
  VDL_QDSP_VIDEO_PKT_REQUEST,
  VDL_QDSP_SUB_FRAME_DONE,  
  VDL_QDSP_PP_ENABLE_CMD_DONE,
  VDL_QDSP_FRAME_DECODE_DONE,
  VDL_QDSP_PP_FRAME_DONE,
  VDL_QDSP_VIDEO_FATAL_ERROR,    
  VDL_QDSP_RESET_DONE,                     
  VDL_QDSP_WATCHDOG_TIME_OUT,
  VDL_QDSP_MAX_SERVICE_TYPES
} VDL_QDSP_Message_Type;

typedef struct
{
  VDL_Return_Code_Type   ReturnCode;   /* return status code */
  VDL_QDSP_Message_Type      MsgCode;
  union
  {
    VDL_QDSP_Download_Done_Info_Type DownloadDone;
    VDL_QDSP_Fatal_Error_Info_Type   FatalError;
    VDL_QDSP_Subframe_Done_Info_Type SubFrameDone;
    VDL_QDSP_Decode_Ready_Info_Type  DecodeReady;
    VDL_QDSP_Frame_Done_Info_Type    FrameDone;
    VDL_QDSP_PP_Frame_Done_Info_Type PP_FrameDone;
  } Cmd;
} VDL_Rx_msg_type;                     

#define VDL_QDSP_NUM_FRAME_DONE_REGS 8

typedef enum
  {
    VDL_CREATE = 0,
    VDL_DOWNLOAD,
    VDL_SEND_SLICE,
    VDL_FLUSH,
    VDL_SUSPEND,
    VDL_RESUME,
    VDL_DESTROY,
    VDL_FRAME_CB,
    VDL_FRAME_NOT_CODED_CB,
    VDL_SEND_PP_CMD
  }VDL_Tx_msg;              /* message codes for messages from ARM */

typedef struct
{
  VDL_Tx_msg  msg;  
} VDL_Tx_msg_type;           /* ARM message type */


#endif


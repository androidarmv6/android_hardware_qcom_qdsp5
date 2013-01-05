#ifndef VDL_RTOS_TYPES_I_H
#define VDL_RTOS_TYPES_I_H

/* =======================================================================
                               VDL_RTOS_Types_i.h
DESCRIPTION
   This file has the data structures that will be used in the message or 
   event callbacks from DSP

Copyright (c) 2007 by QUALCOMM, Incorporated.  All Rights Reserved. 
QUALCOMM Proprietary and Confidential.
========================================================================== */

/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/QDSP5VdecDriver/main/latest/src/VDL_RTOS_Types_i.h#2 $
$DateTime: 2008/11/24 16:46:18 $
$Change: 791733 $

========================================================================== */

/*==========================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */
#include "VDL_Types.h"
#include "VDL_Types_i.h"

#define VDL_QDSP_SUBFRAME_INFO(first,last)  (((last)<<1) | (first))

/*==========================================================================

                        DATA DECLARATIONS

========================================================================== */
#ifndef FEATURE_VDL_TEST_PC
#include "pmem.h"
#endif

#define MAX_SUBFRAME_QUEUE_DEPTH 5            


typedef enum
{
  VDL_QDSP_RTOS_STATE_ERROR   = -1,
  VDL_QDSP_RTOS_STATE_SLEEP   = 0,
  VDL_QDSP_RTOS_STATE_ACTIVE  = 1,
  VDL_QDSP_RTOS_STATE_MAX     = 0x7FFF /* Force size to be 16-bits */
#ifndef T_WINNT
} __attribute((packed)) VDL_QDSP_RTOS_State_Type;
#else
} VDL_QDSP_RTOS_State_Type;
#endif //T_WINNT

typedef struct
{
  VDL_QDSP_RTOS_State_Type  State;
  unsigned short Reserved;
} VDL_QDSP_RTOS_State_Msg_Type;


typedef struct
{
  unsigned short  SeqNum;
  unsigned short  StreamId;
} VDL_QDSP_RTOS_Subframe_Done_Msg_Type;

typedef struct
{
  unsigned short  SeqNum;
  unsigned short  StreamId;
} VDL_QDSP_RTOS_PP_Enable_Done_Msg_Type;

typedef struct
{
  unsigned short  SeqNum;
  unsigned short  StreamId;
} VDL_QDSP_RTOS_Frame_Done_Msg_Type;

typedef struct
{
  unsigned short  SeqNum;
  unsigned short  StreamId;
  unsigned short  DisplayWorthy;
} VDL_QDSP_RTOS_PP_Frame_Done_Msg_Type;

typedef enum
{
  VDL_QDSP_ERR_NONE              = 0, /* no error */
  VDL_QDSP_ERR_INVALID_CMD       = 1, /* wrong control command */
  VDL_QDSP_ERR_INVALID_PACKET    = 2, /* wrong packet command */
  /* Reserved 3-4 */
  VDL_QDSP_ERR_MESSAGE_LOST      = 5, 
  /* Reserved 6-15 */
  VDL_QDSP_ERR_FRAME_HDR_ID      = 16, /* bad frame packet  ID for frame header packet  */
  VDL_QDSP_ERR_FRAME_HDR_MARKER  = 17, /* no marker for frame header packet  */
  VDL_QDSP_ERR_MB_ID             = 18, /* bad MB packet ID for MB packet */
  VDL_QDSP_ERR_MB_MARKER         = 19, /* no marker for MB packet   */
  VDL_QDSP_ERR_MAX               = 0x7FFF
#ifndef T_WINNT
} __attribute__((packed)) VDL_QDSP_RTOS_Error_Type;
#else
} VDL_QDSP_RTOS_Error_Type;
#endif //T_WINNT

typedef struct
{
  VDL_QDSP_RTOS_Error_Type  Error;
  unsigned short StreamId;
} VDL_QDSP_RTOS_Fatal_Error_Msg_Type;

typedef struct
{
   VDL_Video_Type   VideoFormat;
   VDL_Concurrency_Type ConcurrencyFormat;
   adsp_rtos_module_type   QDSPModule;
} VDL_RTOS_QDSP_Module_Table_Type;     

#endif


#ifndef VDL_RTOS_DSP_VLD_H
#define VDL_RTOS_DSP_VLD_H

/* =======================================================================
                               VDL_RTOS.h
DESCRIPTION
   This file has the data structures and function prototypes for 
    the RTOS implementation of VDL

Copyright (c) 2007 by QUALCOMM, Incorporated.  All Rights Reserved. 
QUALCOMM Proprietary and Confidential.
 
========================================================================== */

/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/QDSP5VdecDriver/main/latest/src/VDL_RTOS.h#9 $
$DateTime: 2009/03/20 16:05:25 $
$Change: 868607 $

========================================================================== */

/*==========================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */
#ifdef T_WINNT
    #define ETIMEDOUT (110)
#else
    #include <errno.h>
#endif

#include "VDL_RTOS_Types_i.h"
#include "vdecoder_types.h"
#include "qdsp_vdec.h"

//Low power defines
#ifdef FEATURE_VDL_TEST_PC 
/* until CSIM bug is fixed we will pass one entire frame 
   in subframe packet. For PC based FrmWrk Only*/
#define VDL_RTOS_LP_PKT_BUFFER_SIZE      250000
#else
#define VDL_RTOS_LP_PKT_BUFFER_SIZE      1
#endif

#define VDL_RTOS_LP_PKT_BUFFER_ID        0xFF
#define VDL_RTOS_LP_SUBFRAME_QUEUE_DEPTH 6
#define MAX_SUBFRAME_QUEUE_DEPTH 6

#define VDL_RTOS_LP_STATS_BUF_ADDR 0xc020482c

#define VDL_QDSP_SUBFRAME_INFO(first,last)  (((last)<<1) | (first))

/*==========================================================================

                        DATA DECLARATIONS

========================================================================== */

/*
** Define supported QDSP interface types.
*/
typedef enum
{
  VDL_DSP_INTERFACE_DEFAULT = 0,
  VDL_DSP_INTERFACE_RTOS_QTV_LP,
  VDL_DSP_INTERFACE_MAX          /* Max interface types */
} VDL_DSPInterfaceType;

typedef struct
{
  unsigned short QDSPVideoPktRequestEvent;
  unsigned short MaxQDSPSubframesQueueDepth;  /*Max subframes queue depth at the DSP */
  unsigned long int UsedSubframeQueueDepth;
  unsigned short subframePacketSeqNum; /* Used to save the sf seq number from the previous read */    
  unsigned short prevSFDoneSeqNum;
  VDL_DSPInterfaceType  DSPInterface;
} VDL_RTOS_QDSP_InfoType;

extern VDL_RTOS_QDSP_InfoType         VDL_QDSP_Info;


typedef struct
{
  void *pStatsData;                 /* pointer to VDEC frame data for the frame */
  VDL_Decode_Return_Type ReturnCode; /* return code on the status of the decoded frame */
  unsigned char bIsFlushed;           /* flag indicating whether the frame decoded must be flushed or not */
  unsigned char isFlushMarker;            /* flush marker to mark the end stats buffer until which the 
                                       flush operation needs to be performed */

} VDL_Decode_Stats_Type;

typedef struct
{
  q_link_type Link;           /* link to the stats queue */
  VDL_Decode_Stats_Type DecodeStats;  /* decode stats */
} VDL_Decode_Stats_Packet_Type;

typedef struct
{
  unsigned long int frameCount;                 /* current frame count */      
  VDL_Decoder_Cb_Type DecoderCB;      /* decode done call back function */
  void  *DecoderUserData;            /* user data for DecodeDoneCB */
  unsigned long int Width,Height;               /* Source Dimensions */
  unsigned long int DecWidth,DecHeight;         /* Rounded dimensions for decoding */          
} VDL_Codec_Info_Type;

typedef struct
{       
  signed long long dsp_start_time_ms;
  signed long long dsp_end_time_ms;

  signed long long dsp_idle_start_time_ms;
  signed long long dsp_idle_end_time_ms;

  signed long long dsp_cumulative_dec_time_ms;
  signed long long dsp_cumulative_idle_time_ms;
  signed long long dsp_average_dec_time_ms;
  signed long long dsp_average_idle_time_ms;
  signed long long dsp_peak_dec_time_ms;
  signed long long dsp_peak_idle_time_ms;
  signed long long numIdleOccurrences;

  signed long long numFramesSentToDSP;
  signed long long numFramesRxdFromDSP;

  signed long long numSubFramesSentToDSP;
  signed long long numSubFramesDoneFromDSP;

} VDL_Statistics_Type;


class VDL_RTOS
{

public:

  VDL_RTOS(VDL_ERROR * const pErr_out);
  virtual ~VDL_RTOS();
  void SetState(VDL_state_type state);

  virtual VDL_ERROR VDL_Process_Send_Video_Slice(void);
  //virtual void VDL_init_stats_pkt_buffer(void);
  virtual void VDL_Process_QDSP_Sub_Frame_Done(void);
  virtual void CallDecoderCBOnNonCodedVOPs(void);
  virtual void CallDecoderCBOnNonCodedVOPsForVLDDSP(void);
  virtual void VDL_Process_QDSP_Frame_Decode_Done(void);
  virtual void VDL_Process_QDSP_Fatal_Error(void);
  virtual VDL_Return_Code_Type VDL_QDSP_Send_Video_Subframe_Pkt(VDL_Slice_Pkt_Type * pData);
  virtual void VDL_Process_Send_PP_cmd(void);
  virtual VDL_Slice_Pkt_Type *VDL_Get_Slice_Buffer();
  VDL_ERROR VDL_Initialize_Structures(VDL_Interface_Type iface);
  void* m_pMod;

  VDL_QDSP_Video_Subframe_Packet_Cmd_Type VideoSubframePacketCmd;

  VDL_ERROR VDL_Queue_Stats_Buffer(void *pDecodeStats,
                                   VDL_Decode_Return_Type retCode);

  VDL_ERROR VDL_Free_Slice_Buffer( VDL_Slice_Pkt_Type *pSliceBuf );
  VDL_ERROR VDL_Queue_Slice_Buffer(VDL_Slice_Pkt_Type *pSliceBuf);
  VDL_ERROR VDL_Queue_Fake_EOS_Slice();

  void VDL_init_slice_free_q(VDEC_SLICE_BUFFER_INFO* input,
                             unsigned short num_buffers);
  VDL_Decode_Stats_Packet_Type* VDL_Get_Stats_Buffer(void);

  VDL_ERROR VDL_Initialize_CodecInfo(
                                           VDL_Decoder_Cb_Type  DecoderCB,
                                           void                 *ddUserData,
                                           VDL_Video_Type VideoFormat,
                                           VDL_Concurrency_Type ConcurrencyFormat,
                                           unsigned int width, unsigned int height
                                           );

  VDL_ERROR VDL_Flush();
  VDL_ERROR VDL_Resume(VDL_Concurrency_Type ConcurrencyFormat);
  VDL_ERROR VDL_Terminate();
  VDL_ERROR VDL_Suspend();
  unsigned char VDL_IsSliceBufAvailableForDecode();
  unsigned char VDL_Set_QDSP_Module(adsp_rtos_module_type module);

  VDL_state_type m_state;   /* state of the VDL instance */ 
protected: 
  void VDL_Flush_All_Slices(void);
  void VDL_Flush_All_Stats(void); 
  void VDL_Flush_Slice_Queues(void);
  int VDL_Flush_Stats_Queue(void);
  void VDL_Free_Stats_Buffer(VDL_Decode_Stats_Packet_Type *pCurrentDecodeStats );
  void VDL_Delete_Stats_Buffers( q_type * const q );
  void VDL_Delete_Slice_Buffers( q_type * const q );

  unsigned short num_stats_buffers;    /* to hold number of stats buffers */
  unsigned short num_slice_buffers;    /* to hold number of slice buffers */

  VDL_Codec_Info_Type VDL_Codec_Info;
  unsigned int streamID; /* stream ID ranging from 0 to 4 for multiple instance support
                            0 is used for single instance */

  q_type    VDL_stats_q;                          /* stats queue */
  q_type    VDL_stats_free_q;                     /* Free stats queue */
  q_type    VDL_slice_q;                          /* Slice queue */
  q_type    VDL_slice_free_q;                     /* Free slice queue */
  q_type    VDL_slice_dsp_q;                      /* slice wait queue */
  q_type    VDL_slice_sent_q;                     /* slice queue to resend slices incase of redownload
                                                     to support suspend-resume kindof functionality..
                                                     Should not be used if the aDSP is using one combo
                                                     image..*/
  int m_pending_frame_dones;          /* Used during flush operation, this corresposponds to the number of frame 
                                     ** dones pending from the DSP..*/
  pthread_mutex_t m_pending_frame_done_lock; /* Used during flush operation */
  pthread_cond_t m_all_frame_done_cond;

  /*This variable is used to store the subframe count of the messages in
    the DSP queue */
  unsigned int VDL_Cur_QDSP_Sub_Frame_Queue_Depth;

  /* number of statistics packet buffer*/
  unsigned long int VDL_DSP_VLD_Slice_Buffer_Size;

  VDL_Statistics_Type VDL_Statistics;
  unsigned short BufferQueueRunningCount;
  FILE *pSliceFile;
  adsp_rtos_module_type qdsp_module;

  VDL_Video_Type VideoFormatType;

  pthread_mutex_t lockSendSlice;
  pthread_mutex_t lockFlushProcessing;
  pthread_mutex_t lockEOSProcessing;

};
#endif

#ifndef VDL_H
#define VDL_H

/*=======================================================================
                               VDL.h
DESCRIPTION
  VDL base class definition  

Copyright (c) 2007 by QUALCOMM, Incorporated.  All Rights Reserved. 
QUALCOMM Proprietary and Confidential.
========================================================================== */

/*=======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/QDSP5VdecDriver/main/latest/src/VDL.h#5 $
$DateTime: 2009/03/06 16:57:31 $
$Change: 857226 $

========================================================================== */

/*==========================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */
#include "VDL_Types.h"
#include "VDL_Types_i.h"
#include "queue.h"
#include "qtv_msg.h"
#include "qtvsystem.h"
#include "vdecoder_types.h"
#include <pthread.h>
#include <errno.h>

/* maximum instances supported by VDL class is 
   currently limited to 50 */
#define VDL_MAX_INSTANCES 50
#define VDL_INVALID_INSTANCE 0xFF

#define VLD_ONEQUARTER_VGA_WIDTH  320
#define VLD_ONEQUARTER_VGA_HEIGHT 240
#define VLD_ONEQUARTER_VGA_RESOLUTION   (VLD_ONEQUARTER_VGA_WIDTH * VLD_ONEQUARTER_VGA_HEIGHT)


#define VLD_ONEHALF_VGA_WIDTH  480
#define VLD_ONEHALF_VGA_HEIGHT 368
#define VLD_ONEHALF_VGA_RESOLUTION   (VLD_ONEHALF_VGA_WIDTH * VLD_ONEHALF_VGA_HEIGHT)

#define VLD_VGA_WIDTH  640
#define VLD_VGA_HEIGHT 480
#define VLD_VGA_RESOLUTION   (VLD_VGA_WIDTH * VLD_VGA_HEIGHT)

#define VLD_WVGA_WIDTH  800
#define VLD_WVGA_HEIGHT 480
#define VLD_WVGA_RESOLUTION   (VLD_WVGA_WIDTH * VLD_WVGA_HEIGHT)

class VDL_RTOS;

class VDL
{

public:

  VDL(VDL_ERROR * const pErr_out);
  virtual ~VDL();

  VDL_ERROR VDL_Initialize(
                                VDL_Decoder_Cb_Type   DecoderCB,      /* decode done call back function */
                                void                 *ddUserData,    /* user data for DecoderCB */
                                VDL_Video_Type VideoFormat,           /* video format */
                                VDL_Concurrency_Type ConcurrencyFormat,  /* audio/concurrency format */
				int adsp_fd
                                );      
  VDL_ERROR VDL_Flush();
  VDL_ERROR VDL_Resume(VDL_Concurrency_Type ConcurrencyFormat);
  VDL_ERROR VDL_Terminate();
  VDL_ERROR VDL_Suspend();

  VDL_ERROR VDL_Configure_HW(unsigned int width, unsigned int height,VDL_Profile_Type profile,VDL_Video_Type VideoFormat,
	                         unsigned char **FrameBuffAddr, 
                             unsigned int num_frame_buffers,
                             void *frameHeader,
                             VDEC_SLICE_BUFFER_INFO* input,
                             unsigned short num_slice_buffers);

  VDL_Slice_Pkt_Type *VDL_Get_Slice_Buffer();
  VDL_ERROR VDL_Queue_Slice_Buffer(VDL_Slice_Pkt_Type *pSliceBuf);
  VDL_ERROR VDL_Free_Slice_Buffer(VDL_Slice_Pkt_Type *pSliceBuf);   

  //PP Pkts are required only for FRE
  VDL_PP_cmd_Info_Type *VDL_Get_PP_Pkt_Buffer();
  VDL_ERROR VDL_Queue_PP_Pkt_Buffer(VDL_PP_cmd_Info_Type *p_PPCmdBuf);
  VDL_ERROR VDL_Free_PP_Pkt_Buffer(VDL_PP_cmd_Info_Type *p_PPCmdBuf);   

  VDL_ERROR VDL_Queue_Stats_Buffer(void *pDecodeStats,VDL_Decode_Return_Type retCode); 

  VDL_Interface_Type VDL_Get_Interface_Type(unsigned long int Height,
						 unsigned long int Width,
						 VDL_Video_Type VideoFormat, 
						 VDL_Profile_Type profile
                         );

  unsigned char VDL_IsSliceBufAvailableForDecode();
  VDL_RTOS *VDLInterfaceInstance;
  
  pthread_mutex_t m_sleep_ack_lock; /* Used during SLEEP operation */
  pthread_cond_t m_sleep_ack_cond;
  unsigned char m_sleepAcked;

private:

  void* m_pMod;

  typedef struct {
   VDL_Decoder_Cb_Type   DecoderCB;
   void                 *ddUserData;    /* user data for DecoderCB */
   VDL_Video_Type        VideoFormat;           /* video format */
   VDL_Concurrency_Type  ConcurrencyFormat;
   int 			 adsp_fd;
  }VDL_init_config_data_type;
  VDL_init_config_data_type VDL_init_config_data;
  int axi_fd;

};

#endif


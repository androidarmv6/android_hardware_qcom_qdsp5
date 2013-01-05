/* =======================================================================
                               VDL.cpp
DESCRIPTION
   This file has the implementation for VDL

Copyright (c) 2007 by QUALCOMM, Incorporated.  All Rights Reserved. 
QUALCOMM Proprietary and Confidential.

========================================================================== */

/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/QDSP5VdecDriver/main/latest/src/vdl.cpp#18 $
$DateTime: 2009/04/08 14:21:16 $
$Change: 882913 $

========================================================================== */

/*==========================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "VDL.h"
#include "VDL_RTOS.h"
#include "VDL_Types_i.h"
#include "mp4buf.h"
#include "adsprtossvc.h"

#ifndef T_WINNT
#ifdef _ANDROID_
#include "cutils/log.h"
#endif //_ANDROID_
#include <sys/time.h>
#include <unistd.h>
#endif //T_WINNT


extern "C" {
#include "qutility.h"
}

//FIXME: These values are auto-generated; so we shouldnot
//use it like this
#define QDSP_mpuVDecCmdQueue              4
#define AXI_FREQ_VGA_OR_BELOW             61440
#define AXI_FREQ_WVGA                     122880

#include "assert.h"

/*********************************************************
This global variable is accessed in VDL_QDSP_RTOS_Event_Cb
to check for pthread locks and broadcast conditions.It is
set in VDL_Configure_HW and stores the pointer to current 
VDL class. 
**********************************************************/
static VDL* VDL_cur_instance = NULL;
static VDL_Interface_Type VDL_Interface_wmv[VDL_PROFILE_MAX] = {
  VDL_INTERFACE_RTOS,
	VDL_INTERFACE_ARM,
	VDL_INTERFACE_ARM,
};

static VDL_Interface_Type VDL_Interface[VDL_VIDEO_MAX] = {
    VDL_INTERFACE_NONE,
    VDL_INTERFACE_RTOS,
	VDL_INTERFACE_RTOS,
	VDL_INTERFACE_RTOS,
	VDL_INTERFACE_RTOS,
	VDL_INTERFACE_NONE,
	VDL_INTERFACE_RTOS,
	VDL_INTERFACE_RTOS
};   

/*
typedef struct
{
  unsigned short Cmd;
  unsigned short Reserved;
} VdecDriverActiveCmdType;
*/
/*===========================================================================*/

/*
 * Video Driver callback method for messages from QDSP Services
 *
 * @param[in] msg_id       one of the QDSP_VIDEOTASK msg ids
 * @param[in] msg_buf      associated data with the callback
 * @param[in] msg_len      length of the msg buffer
 */

static void VDL_QDSP_RTOS_Msg_Cb( uint16_t msgid,
								  void *data, 
								  uint32_t length, 
								  void *cookie
                                );


/*
 * Video Driver callback method for events from QDSP Services
 *
 * @param[in] pEvent       event type
 */
static void VDL_QDSP_RTOS_Event_Cb( adsp_rtos_event_data_type *pEvent);

/***************************************************************
Function: getTimeSpec

Description: This function sets the timesp (tsParam) for the 
		     requested time in ms.

INPUT      : 
   tsParam : pointer to timespec structure.
   ms      : time to wait for in ms.

OUTPUT     : void
             time would be populate din timespec pointer passed
			 as parameter.
****************************************************************/
void getTimeSpec(struct timespec *tsParam,int ms){
	if(!tsParam) return;     
	struct timespec   *ts = tsParam;
	struct timeval    tv;
	long long int time_ns = ms * 1000000;
	gettimeofday(&tv, 0);
	/* Convert from timeval to timespec */
	ts->tv_sec  = tv.tv_sec;
	ts->tv_nsec = tv.tv_usec * 1000;
	/*Wait no longer than 200msec*/
	if((ts->tv_nsec + time_ns) < 1000000000)
	{
		ts->tv_nsec += (time_ns);
	}
	else
	{
		//increment the sec
		ts->tv_sec  += 1;
		//incr the nsec accordingly
		ts->tv_nsec = (ts->tv_nsec + time_ns - 1000000000);
	}
}


/*===========================================================================

FUNCTION:
  VDL constructor

DESCRIPTION:
  The VDL constructor

DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:  
  VDL_ERROR
  
RETURN VALUE:
  None

SIDE EFFECTS:
  None

===========================================================================*/
VDL::VDL
(
VDL_ERROR * const pErr_out
)
{ 
  *pErr_out = VDL_ERR_NONE;
   VDLInterfaceInstance = NULL;
   VDL_init_config_data.DecoderCB = NULL;
   m_sleepAcked = 0;
   axi_fd = 0;

   int result1 = pthread_cond_init(&m_sleep_ack_cond, 0);
   int result2 = pthread_mutex_init(&m_sleep_ack_lock, 0);

   if(result1 || result2) 
   {
     QTV_MSG_PRIO1(QTVDIAG_GENERAL, QTVDIAG_PRIO_MED, 
                  "VDL err=%d mutex_init()..!!",result1);
     *pErr_out = VDL_ERR_MYSTERY;
   }
  
   QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_MED, "VDL instance created!");

}


/*===========================================================================

FUNCTION:
  VDL_Initialize

DESCRIPTION:
  This function initializes the decoder

DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  decoder callback function
  user data for decoder callback
  encoded video format
  audio / concurrency format
  
  
RETURN VALUE:
  VDL_ERROR

SIDE EFFECTS:
  None

===========================================================================*/
VDL_ERROR VDL::VDL_Initialize(
                               VDL_Decoder_Cb_Type   DecoderCB,        /* decoder call back function */
                               void                 *ddUserData,      /* user data for DecoderCB */    
                               VDL_Video_Type VideoFormat,             /* video format */
                               VDL_Concurrency_Type ConcurrencyFormat,  /* audio/concurrency format */
			       int adsp_fd
                               )
{
  VDL_ERROR retVal = VDL_ERR_NONE; 
  VDL_init_config_data.ddUserData  =  ddUserData;
  VDL_init_config_data.VideoFormat =  VideoFormat;
  VDL_init_config_data.DecoderCB   =  DecoderCB;
  VDL_init_config_data.ConcurrencyFormat = ConcurrencyFormat;
  VDL_init_config_data.adsp_fd = adsp_fd;

  return retVal;

}
/*===========================================================================
FUNCTION:
  VDL_Queue_Stats_Buffer

DESCRIPTION:
  puts a stats buffer to the stats Queue. 

DEPENDENCIES
  None.

INPUT/OUTPUT PARAMETERS:
  stats buffer pointer,
  return code of VDL_Decode_Return_Type
  
RETURN VALUE
  error code provided by VDL_ERROR

SIDE EFFECTS:
  None

============================================================================*/
VDL_ERROR VDL::VDL_Queue_Stats_Buffer(void *pDecodeStats,VDL_Decode_Return_Type retCode)
{

  QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_MED, "VDL_Queue_Stats_Buffer()");
  VDL_ERROR retVal = VDL_ERR_NONE; 

  if(VDLInterfaceInstance)
  {
	retVal = VDLInterfaceInstance->VDL_Queue_Stats_Buffer(pDecodeStats,retCode);
  }
  else if(retCode == VDL_END_OF_SEQUENCE)
  {
        QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_HIGH, "EOS received before sneding any valid frame to decoder");
        VDL_init_config_data.DecoderCB(retCode,NULL,VDL_init_config_data.ddUserData);
  }
  return retVal;
}

/*===========================================================================

FUNCTION:
  VDL_Get_Interface_Type

DESCRIPTION:
  This function return the appopriate ARM-DSP Interface type so that upper layers can pack
  data accordingly.
  
DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
 VDL Instance pointer
 Height
 Width
 Video codec type
 profile of the video codec
  
RETURN VALUE:
  error code provided by VDL_ERROR

SIDE EFFECTS:
  None

===========================================================================*/

VDL_Interface_Type VDL:: VDL_Get_Interface_Type(unsigned long int Height,
						 unsigned long int Width,
						 VDL_Video_Type VideoFormat, 
						 VDL_Profile_Type profile
                         )
{
  VDL_Interface_Type Interface;

  (void)Height;
  (void)Width;

  if (VideoFormat == VDL_VIDEO_WMV)
  {
    Interface = VDL_Interface_wmv[profile];

  }
  else
  {
	Interface = VDL_Interface[VideoFormat];
  }

  if(VideoFormat == VDL_VIDEO_H264 && 
    (VDL_init_config_data.ConcurrencyFormat == VDL_AUDIO_AAC  
     || VDL_init_config_data.ConcurrencyFormat == VDL_AUDIO_NONE ))
  {
#ifdef FEATURE_QTV_WVGA_ENABLE
    if(Width*Height <= VLD_WVGA_RESOLUTION)
#else
    if(Width*Height <= VLD_ONEHALF_VGA_RESOLUTION)
#endif
    {
#ifdef T_WINNT
      Interface = VDL_INTERFACE_RTOS;
#else
      Interface = VDL_INTERFACE_RTOS_VLD_DSP;
#endif
      QTV_MSG_PRIO2(QTVDIAG_GENERAL, QTVDIAG_PRIO_HIGH, "H264 VLD_DSP Interface! VideoFormat = %d,AudioFormat = %d",VideoFormat,VDL_init_config_data.ConcurrencyFormat);
    }
    else
    {
      Interface = VDL_INTERFACE_RTOS;
      QTV_MSG_PRIO2(QTVDIAG_GENERAL, QTVDIAG_PRIO_HIGH, "H264 VLD_ARM Interface! VideoFormat = %d,AudioFormat = %d",VideoFormat,VDL_init_config_data.ConcurrencyFormat);
    }
  }
#ifdef FEATURE_MPEG4_VLD_DSP
  else if ((VideoFormat == VDL_VIDEO_MPEG4) || (VideoFormat == VDL_VIDEO_H263))
  {
#ifdef T_WINNT 
      Interface = VDL_INTERFACE_RTOS;
#else
      Interface = VDL_INTERFACE_RTOS_VLD_DSP;
#endif
      QTV_MSG_PRIO2(QTVDIAG_GENERAL, QTVDIAG_PRIO_HIGH, "Mpeg4 VLD_DSP Interface! VideoFormat = %d,AudioFormat = %d",VideoFormat,VDL_init_config_data.ConcurrencyFormat);
  }
#endif
  
  return Interface;
}

/*===========================================================================

FUNCTION:
  VDL_Configure_HW

DESCRIPTION:
  Downloads the DSP image
  
DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  width and height of the frame
  
RETURN VALUE:
  error code provided by VDL_ERROR

SIDE EFFECTS:
  None

===========================================================================*/
VDL_ERROR VDL::VDL_Configure_HW(unsigned int width, unsigned int height, VDL_Profile_Type profile,VDL_Video_Type VideoFormat,
								unsigned char **FrameBuffAddr, 
                                unsigned int num_frame_buffers,
                                void *frameHeader,
                                VDEC_SLICE_BUFFER_INFO* input, 
                                unsigned short num_slice_buffers)
{
  adsp_rtos_module_type qdsp_module = QDSP_MODULE_VIDEOTASK;
  //*Interface = VDL_ARM_INTERFACE;
  VDL_Interface_Type Interface;
  VDL_ERROR retVal = VDL_ERR_NONE;
  unsigned long clk_rate = 98000000;
  int axi_freq = AXI_FREQ_VGA_OR_BELOW;
  int adsp_fd = VDL_init_config_data.adsp_fd;

  if((width*height) <= (MP4_VGA_WIDTH * MP4_VGA_HEIGHT)/2)
  {
    clk_rate = 61440000;
  }

  if((width*height) >= VLD_WVGA_RESOLUTION )
  {
    axi_freq = AXI_FREQ_WVGA;
  }
  axi_fd = open("/dev/system_bus_freq", O_RDWR);
  if (axi_fd < 0) 
  {
    QTV_MSG_PRIO1(QTVDIAG_GENERAL, QTVDIAG_PRIO_ERROR, "ERROR - VDL_Configure_HW: cannot open axi_freq %d\n",
                  axi_fd);
  }
  else
  {
    QTV_MSG_PRIO1(QTVDIAG_GENERAL, QTVDIAG_PRIO_HIGH, 
                  "VDL_Configure_HW: Request axi freq %d\n",
                  axi_freq);
    if (write(axi_fd, &axi_freq, sizeof(axi_freq)) < 0)
    {
      QTV_MSG_PRIO1(QTVDIAG_GENERAL, QTVDIAG_PRIO_ERROR, 
                    "ERROR - VDL_Configure_HW: Request axi freq %d failed\n",
                    axi_freq);
    }
  }

  //video format might have changed now(ex. MP4/WMV)
  VDL_init_config_data.VideoFormat = VideoFormat;

  Interface = VDL_Get_Interface_Type(height,width,VDL_init_config_data.VideoFormat,profile);
  VDL_cur_instance = this;
  switch (Interface)
  {
  case VDL_INTERFACE_RTOS:
  case VDL_INTERFACE_RTOS_VLD_DSP:
      VDLInterfaceInstance = (VDL_RTOS *)QTV_New_Args(VDL_RTOS, (&retVal) );
      break;

  default:
      QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_ERROR, "VDL_Configure_HW: Interface Not supported");
      return VDL_ERR_MYSTERY;
  }

  if(retVal != VDL_ERR_NONE)
  {
    QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_ERROR, "VDL_Configure_HW: Init Failure when remapping");
    return retVal;
  }

#ifdef FEATURE_TURBO_MODULE
  /*
   * Turbo module should be enabled for all resolutions above VGA. Since VGA
   * has 1200 macroblocks so any resolution having more than 1200 macroblocks
   * should have turbo mode enabled.
   */
  if (((width*height >> 8) > 1200) && (VideoFormat == VDL_VIDEO_H264))
  {
    QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_HIGH, "VDL_Configure_HW: Enable QDSP_MODULE_VIDEO_AAC_VOC_TURBO");
    qdsp_module = QDSP_MODULE_VIDEO_AAC_VOC_TURBO;
    adsp_rtos_register_client( QDSP_MODULE_VIDEO_AAC_VOC_TURBO, VDL_QDSP_RTOS_Msg_Cb,VDL_QDSP_RTOS_Event_Cb, (void*)this, -1);
    adsp_rtos_enable(QDSP_MODULE_VIDEO_AAC_VOC_TURBO);
    usleep(20*1000);
  }
  else
#endif
  {
    qdsp_module = QDSP_MODULE_VIDEOTASK;
  }
  VDLInterfaceInstance->VDL_Set_QDSP_Module(qdsp_module);

  adsp_rtos_register_client( QDSP_MODULE_VIDEOTASK, VDL_QDSP_RTOS_Msg_Cb,VDL_QDSP_RTOS_Event_Cb, (void*)this, adsp_fd );
  adsp_rtos_enable(QDSP_MODULE_VIDEOTASK);
  usleep(20*1000);
  adsp_rtos_register_pmem(QDSP_MODULE_VIDEOTASK,input[0].fd, input[0].base);
 
  adsp_rtos_set_clkrate(QDSP_MODULE_VIDEOTASK,clk_rate);
  //clk_regime_sec_enable(CLKRGM_VDC_CLK);

  retVal = VDLInterfaceInstance->VDL_Initialize_Structures(Interface);

  /* initialize codec specific information */
  retVal= VDLInterfaceInstance->VDL_Initialize_CodecInfo( VDL_init_config_data.DecoderCB,
                                                          VDL_init_config_data.ddUserData,                     
                                                          VDL_init_config_data.VideoFormat,           
                                                          VDL_init_config_data.ConcurrencyFormat,
                                                          width, height
                                                          );

  VDLInterfaceInstance->VDL_init_slice_free_q(input,num_slice_buffers);

//FIXME: Update the frameHeader with FrameBuffAddr in the VDL client itself..

//DSP is already downloaded.. No need to call adsp_register_app
//  if( VDL_QDSP_Info.DSPInterface == VDL_DSP_INTERFACE_RTOS_QTV_LP )
//  {
//
//      adsp_rtos_register_app( QDSP_MODULE_VIDEOTASK,
//                              VDL_QDSP_RTOS_Msg_Cb,
//                              VDL_QDSP_RTOS_Event_Cb );
//
//      adsp_rtos_register_app( QDSP_MODULE_VDEC_LP_MODE,
//                              VDL_QDSP_RTOS_Msg_Cb,
//                              VDL_QDSP_RTOS_Event_Cb );
//  }

//No need to enable the clocks. Already done by the lower
//  driver underneath

  //Now enable the QDSP modules. Enable LP_MODE here. VIDEO_TASK will get 
  //enabled in the event callback.

  /*Now send the Active cmd*/
  QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH , "Sending ACTIVE Cmd");
  VdecDriverActiveCmdType ActiveCmd;
  ActiveCmd.Cmd = QDSP_VIDEOTASK_VDEC_ACTIVE_CMD;
  ActiveCmd.Reserved = 0xBAD;

  if(VDL_ERR_NONE==retVal)
  {
     adsp_rtos_send_command_16( QDSP_MODULE_VIDEOTASK,
                                QDSP_mpuVDecCmdQueue,
                                (unsigned short*)&ActiveCmd,
                               (unsigned long int)(sizeof(ActiveCmd)/ sizeof(unsigned short))
                               );
                               
     QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH , "Sent ACTIVE Cmd..!");
  }
  else
  {
     retVal = VDL_ERR_MYSTERY;
  }
  return retVal;
}

/*===========================================================================

FUNCTION VDL_Get_Slice_Buffer

DESCRIPTION
  This function returns a slice buffer 

DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE
  Pointer to slice buffer

SIDE EFFECTS
  None.

===========================================================================*/
VDL_Slice_Pkt_Type * VDL::VDL_Get_Slice_Buffer(void)
{
  QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_MED, "VDL::VDL_Get_Slice_Buffer()");
  VDL_Slice_Pkt_Type* retVal = NULL; 

  if(VDLInterfaceInstance)
  {
	retVal = VDLInterfaceInstance->VDL_Get_Slice_Buffer(); 
  }

  return retVal;
}
/*===========================================================================

FUNCTION VDL_Free_Slice_Buffer

DESCRIPTION
  This function returns a slice buffer to the free queue for reuse.

DEPENDENCIES
  None.

INPUT/OUTPUT PARAMETERS:
  slice buffer pointer

RETURN VALUE
  error code provided by VDL_ERROR

SIDE EFFECTS
  None.

===========================================================================*/
VDL_ERROR VDL::VDL_Free_Slice_Buffer( VDL_Slice_Pkt_Type *pSliceBuf )
{
  QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_MED, "VDL_Free_Slice_Buffer()");
  VDL_ERROR retVal = VDL_ERR_NONE; 

  if(VDLInterfaceInstance)
  {
	retVal = VDLInterfaceInstance->VDL_Free_Slice_Buffer(pSliceBuf); 
  }

  return retVal;
}


/*===========================================================================

FUNCTION VDL_Queue_Slice_Buffer

DESCRIPTION
  This function queues a slice buffer into the slice queue

DEPENDENCIES
  None.

INPUT/OUTPUT PARAMETERS:
  pointer to slice buffer

RETURN VALUE:
  error code provided by VDL_ERROR

SIDE EFFECTS:
  None.

===========================================================================*/
VDL_ERROR VDL::VDL_Queue_Slice_Buffer(VDL_Slice_Pkt_Type *pSliceBuf)
{
  QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_MED, "VDL::VDL_Queue_Slice_Buffer()");
  VDL_ERROR retVal = VDL_ERR_NONE; 

  if(VDLInterfaceInstance)
  {
    retVal = VDLInterfaceInstance->VDL_Queue_Slice_Buffer(pSliceBuf); 
  }

  return retVal;
}

/*===========================================================================

FUNCTION VDL_Get_PP_Pkt_Buffer

DESCRIPTION
  This function returns a PP cmd buffer 

DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE
  Pointer to pp command info buffer

SIDE EFFECTS
  None.

===========================================================================*/
VDL_PP_cmd_Info_Type * VDL::VDL_Get_PP_Pkt_Buffer(void)
{
  return NULL;
}
/*===========================================================================

FUNCTION VDL_Free_PP_Pkt_Buffer

DESCRIPTION
  This function returns a pp command buffer to the free queue for reuse.

DEPENDENCIES
  None.

INPUT/OUTPUT PARAMETERS:
  slice buffer pointer

RETURN VALUE
  error code provided by VDL_ERROR

SIDE EFFECTS
  None.

===========================================================================*/
VDL_ERROR VDL::VDL_Free_PP_Pkt_Buffer( VDL_PP_cmd_Info_Type *p_PPCmdBuf )
{
  return VDL_ERR_MYSTERY;
}


/*===========================================================================

FUNCTION VDL_Queue_PP_Pkt_Buffer

DESCRIPTION
  This function queues a pp command buffer into the slice queue

DEPENDENCIES
  None.

INPUT/OUTPUT PARAMETERS:
  pointer to slice buffer

RETURN VALUE:
  error code provided by VDL_ERROR

SIDE EFFECTS:
  None.

===========================================================================*/
VDL_ERROR VDL::VDL_Queue_PP_Pkt_Buffer(VDL_PP_cmd_Info_Type *p_PPCmdBuf)
{
  return VDL_ERR_MYSTERY;
}

/*===========================================================================

FUNCTION:
  VDL_Suspend

DESCRIPTION:
  This function queues the suspend command in the VDL command queue
  
DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  None
  
RETURN VALUE:
  error code provided by VDL_ERROR

SIDE EFFECTS:
  None

===========================================================================*/
VDL_ERROR VDL::VDL_Suspend(void)
{
  QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_MED, "VDL::VDL_Suspend()");
  VDL_ERROR retVal = VDL_ERR_NONE; 

  if(VDLInterfaceInstance)
  {
	retVal = VDLInterfaceInstance->VDL_Suspend(); 
  }

  /*Send the Sleep Cmd*/
  QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH , "Sending SLEEP Cmd");
  VdecDriverSleepCmdType SleepCmd;
  SleepCmd.Cmd = (unsigned short)QDSP_VIDEOTASK_VDEC_SLEEP_CMD;
  SleepCmd.Reserved = 0xBAD;

  if( VDL_ERR_NONE==retVal )
  {
    adsp_rtos_send_command_16( QDSP_MODULE_VIDEOTASK,
                               QDSP_mpuVDecCmdQueue,
                               (unsigned short*)&SleepCmd,
                               (unsigned long int)(sizeof(SleepCmd)/ sizeof(unsigned short))
                             );
    QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH , "Sent SLEEP Cmd..!");
	/*Wait for Sleep Ack on a timeout*/
    /* Wait on the condition variable until signalled by the aDSP */
    struct timespec   ts;
    pthread_mutex_lock(&m_sleep_ack_lock);
    while (!m_sleepAcked) {
      getTimeSpec(&ts,200);
      int result = pthread_cond_timedwait(&m_sleep_ack_cond, 
                                      &m_sleep_ack_lock,
                                      &ts);

      if(result == ETIMEDOUT)
      {
        QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_ERROR, 
                     "Waiting on all SLEEP ACK timedout..!");
        break;
      }
    }
    pthread_mutex_unlock(&m_sleep_ack_lock);
    /*Resetting m_sleepAcked even if we timedout on aDSP*/
    m_sleepAcked = 0;
  }
  else
  {
    retVal = VDL_ERR_MYSTERY;
  }
  return retVal;
}
/*===========================================================================

FUNCTION:
  VDL_Resume

DESCRIPTION:
  To resume decode after a suspend
  
DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  None
  
RETURN VALUE:
  error code provided by VDL_ERROR

SIDE EFFECTS:
  None

===========================================================================*/
VDL_ERROR VDL::VDL_Resume(VDL_Concurrency_Type ConcurrencyFormat)
{
  QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_MED, "VDL::VDL_Resume()");
  VDL_ERROR retVal = VDL_ERR_NONE; 

  if(VDLInterfaceInstance)
  {
	retVal = VDLInterfaceInstance->VDL_Resume(ConcurrencyFormat); 
  }

  return retVal;
}
/*===========================================================================
FUNCTION:
  VDL_Flush

DESCRIPTION:
  This function adds a slice buffer with a flush marker into the slice queue 
  and a stats buffer into the stats queue with the flush marker. The flush
  command is also queued in the command queue of the VDL instance.
  
DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
 None

RETURN VALUE:
 error code provided by VDL_ERROR

SIDE EFFECTS:
  None

===========================================================================*/
VDL_ERROR VDL::VDL_Flush(void)
{
  QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_MED, "VDL::VDL_Flush()");
  VDL_ERROR retVal = VDL_ERR_NONE; 

  if(VDLInterfaceInstance)
  {
	retVal = VDLInterfaceInstance->VDL_Flush();
  }
  else
  {
    QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_MED, "No VDL interface to flush\n");
  }

  return retVal;
}
/*===========================================================================

FUNCTION:
  VDL_Terminate

DESCRIPTION:
  This routine does the clean up operations on the VDL instance
  
DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  None
  
RETURN VALUE:
  error codes provided by VDL_ERROR

SIDE EFFECTS:
  None

===========================================================================*/
VDL_ERROR VDL::VDL_Terminate(void)
{
  QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_MED, "VDL::VDL_Terminate()");
  VDL_ERROR retVal = VDL_ERR_NONE; 
  int axi_freq = 0;

  if(VDLInterfaceInstance)
  {
	retVal = VDLInterfaceInstance->VDL_Terminate(); 
  }
  else
  {
    QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_MED, "Error: VDL_Terminate called with NULL Instance");         
  }

  // Remove AXI requirement that we set at VDL_Configure_HW
  if (axi_fd > 0) 
  {
    if (write(axi_fd, &axi_freq, sizeof(axi_freq)) < 0)
    {
      QTV_MSG_PRIO1(QTVDIAG_GENERAL, QTVDIAG_PRIO_ERROR,
                    "ERROR - Failed to remove AXI requirement that's been set previously\n",
                    axi_freq);
    }
    close(axi_fd);
    axi_fd = 0;
  }



  return retVal;
}

/*===========================================================================

FUNCTION:
  VDL destructor

DESCRIPTION:
  VDL destructor
  
DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  None
  
RETURN VALUE:
  None

SIDE EFFECTS:
  None

===========================================================================*/
VDL::~VDL()
{
  QTV_Delete(VDLInterfaceInstance);
  VDLInterfaceInstance = NULL;
  (void) pthread_cond_destroy(&m_sleep_ack_cond);
  (void) pthread_mutex_destroy(&m_sleep_ack_lock);

  QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_MED, "VDL destructor");         
}

unsigned char VDL::VDL_IsSliceBufAvailableForDecode()
{
  if (VDLInterfaceInstance)
  {
    return VDLInterfaceInstance->VDL_IsSliceBufAvailableForDecode();
  }
  else
  {
    return 0;
  }
}

/*===========================================================================

FUNCTION:
  VDL_QDSP_RTOS_Msg_Cb

DESCRIPTION:
  This function determines the type of interrupt from the QDSP and it will
  report appropriate action to the ARM to process the request.

DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  message ID, message buffer, message length

RETURN VALUE:
  None.

SIDE EFFECTS:
  None.
===========================================================================*/
void VDL_QDSP_RTOS_Msg_Cb( uint16_t msgid,
                           void *data, uint32_t length, void *cookie)
{
  VDL *VDL_instance = (VDL*)cookie; 
  unsigned int index;

  QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED, "******VDL_QDSP_RTOS_Msg_Cb\n\n");      

  VDL_QDSP_RTOS_State_Msg_Type  *pStateMsg;
  
  qdsp_videotask_vdecmpurlist_msg_type  video_id =
  (qdsp_videotask_vdecmpurlist_msg_type) msgid;    

  switch ( video_id )
  {
    case QDSP_VIDEOTASK_VDEC_STATE_MSG_ID:
      {
        pStateMsg = (VDL_QDSP_RTOS_State_Msg_Type*)data;
       // ASSERT( (msg_len * sizeof(unsigned short)) == sizeof(VDL_QDSP_RTOS_State_Msg_Type) );
        
        switch ( pStateMsg->State )
        {
          case VDL_QDSP_RTOS_STATE_ACTIVE:
            QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF,QTVDIAG_PRIO_HIGH, "***ACTIVE ACK\n\n");
            VDL_instance->VDLInterfaceInstance->SetState(READY);
            VDL_instance->VDLInterfaceInstance->VDL_Process_Send_Video_Slice();
            break;

          case VDL_QDSP_RTOS_STATE_SLEEP:
            QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH, "RESET DONE");                 

            /*Signal that we got back the SLEEP ACK*/
            pthread_mutex_lock(&VDL_instance->m_sleep_ack_lock);
            if(!VDL_instance->m_sleepAcked)
            { 
              VDL_instance->m_sleepAcked = 1;
              pthread_cond_broadcast(&VDL_instance->m_sleep_ack_cond);
            }
            pthread_mutex_unlock(&VDL_instance->m_sleep_ack_lock);

            break;

          case VDL_QDSP_RTOS_STATE_ERROR:
            VDL_instance->VDLInterfaceInstance->SetState((VDL_state_type)ERROR);
            break;

          default:
            QTV_MSG_PRIO1( QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_ERROR, "Unknown state %d", pStateMsg->State );
            break;
        }
      }
      break;


    case QDSP_VIDEOTASK_VDEC_SUBF_DONE_MSG_ID:
      {   
        VDL_QDSP_RTOS_Subframe_Done_Msg_Type *pSubfDoneMsg =
        (VDL_QDSP_RTOS_Subframe_Done_Msg_Type*) data;

        QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH, "****SUBFRAME DONE!\n\n");
        VDL_instance->VDLInterfaceInstance->VDL_Process_QDSP_Sub_Frame_Done();
      }
      break;

    case QDSP_VIDEOTASK_VDEC_FATAL_ERR_MSG_ID:
      {
        VDL_QDSP_RTOS_Fatal_Error_Msg_Type *pErrorMsg =
        (VDL_QDSP_RTOS_Fatal_Error_Msg_Type*) data;

        QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_FATAL, "FATAL ERROR!");
        //ASSERT( (msg_len * sizeof(unsigned short)) == sizeof( VDL_QDSP_RTOS_Fatal_Error_Msg_Type ) );
        QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_FATAL, "Error msg: %d", pErrorMsg->Error);
        VDL_instance->VDLInterfaceInstance->SetState((VDL_state_type)ERROR);
        VDL_instance->VDLInterfaceInstance->VDL_Process_QDSP_Fatal_Error();
      }
      break;

    case QDSP_VIDEOTASK_VDEC_FRAME_DONE_MSG_ID:
      {
        VDL_QDSP_RTOS_Frame_Done_Msg_Type *pFrameDoneMsg =
        (VDL_QDSP_RTOS_Frame_Done_Msg_Type*) data;

        QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH, "FRAME DONE!");
        VDL_instance->VDLInterfaceInstance->VDL_Process_QDSP_Frame_Decode_Done();
      }
      break;  

    default:
      QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_ERROR, 
                    "Unsupported VDL_QDSP_rtos_msg_cb(%d)!", video_id );
      break;
  }
}

/*===========================================================================

FUNCTION:
  VDL_QDSP_RTOS_EVENT_CB

DESCRIPTION:
  This function determines the type of interrupt from the QDSP and it will
  report appropriate action to the ARM to process the request.

DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  event pointer

RETURN VALUE:
  None.

SIDE EFFECTS:
  None.
===========================================================================*/
void VDL_QDSP_RTOS_Event_Cb(adsp_rtos_event_data_type *pEvent)
{
  VDL *VDL_instance;
  VDL_Rx_msg_type VDL_QDSP_Message_Info;
  unsigned int index;

  VDL_instance = VDL_cur_instance;  

  QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED, "VDL_QDSP_rtos_event_cb");

  if ( pEvent == NULL )
  {
    QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_ERROR, "NULL event pointer");
    return;
  }

  switch ( pEvent->status )
  {   
    case ADSP_RTOS_MOD_READY:
      QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH, "MOD READY!");         
      break;

    case ADSP_RTOS_MOD_DISABLE:
      QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH, "MOD DISABLE!");         
      //FIXME: Need to figure out how to handle this
      break;

    case ADSP_RTOS_CMD_SUCCESS:
      QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH, "ADSP_RTOS_CMD_SUCCESS!");         
    break;

    default:
      QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_ERROR, "Illegal QDSP mod/img event status %d", pEvent->status);
      break;
  }

  return;
}


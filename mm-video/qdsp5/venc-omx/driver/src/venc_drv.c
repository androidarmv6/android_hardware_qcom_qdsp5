/*===========================================================================

                      V E N C _ D R V . C   

DESCRIPTION

  This file implements the device driver for OpenMM Encoder, DVS

REFERENCES


Copyright (c) 2004 - 2007 by QUALCOMM, Incorporated.  All Rights Reserved.

===========================================================================*/

/*===========================================================================

                      EDIT HISTORY FOR FILE

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/driver/src/venc_drv.c#7 $

   
when       who     what, where, why
--------   ---     ----------------------------------------------------------
06/12/10   as      Added feature define flag for vdc 120mhz.
01/26/10   as      Added new API for cache invalidate
10/05/09   as      Support for HFR
10/3/09    as      Support for Vt pass for H264
09/10/09   as      Fixed offset bug and removed sleep.
08/10/09   as      AXI polling, Turbo mode support
07/13/09   as      Encoder to use base address for reigtering YUV
06/18/09   as      Defination of new invalidate API for cache
03/27/09   as      Adding TE detection as feature for Google donut & HWTE_ENABLE
03/27/09   as      Auto detection API for TE
02/26/09   as      Using pmem instaed of pmem_adsp 
01/22/09   as      To compile with adsp services 
 01/21/09  as      Debug messages for andriod & adsp changes wrt to 2.6.27
12/29/08   as      Support added to merge virtual address passing 
                   sync with andriod release 
07/09/08   bc      Changesd QVP debug messages to VENC debug messages. 
                   Also fixed pmem memory leak.
06/13/08   bc      Added support for LE, featurized android with FEATURE_LINUX_A,
                   removed venc_pmem.h and venc_pmem.c
06/12/18   bc      Replaced CVencDriver on android.
11/20/07   bc      Aligned to CVencDriver.h
11/20/07   bc      Updated to be a csim only file
09/18/07   bc      Integrated with device layer
6/5/07     xz      Initial Revision

===========================================================================*/


/*===========================================================================

                      INCLUDE FILES FOR MODULE

===========================================================================*/

#include "adsprtossvc.h"
#include <stdio.h>
#include <unistd.h>
#include "venc_drv.h"
#include "venc_debug.h"
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/android_pmem.h>
#include <fcntl.h>


#ifdef _ANDROID_LOG_
#define LOG_NDEBUG 0
#define LOG_TAG "VENC_DRV"
#endif
#define VENC_DRV_MAX_NUM_INST 3

// assume a max of 20 buffers
#define MAX_YUV_BUFFERS (20)
int FDS[MAX_YUV_BUFFERS];

// changes for AXI, VDC, TURBO mode.
#define AXI_FREQ_HVGA_OR_BELOW 122880
#define AXI_FREQ_VGA_OR_WVGA   200000
#define VGA_MB_NUMBER          1200
#define WVGA_MB_NUMBER         1500
static int axi_fd = -1 ;
volatile boolean bIsTurboModeSet = FALSE ;
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
typedef struct //__attribute__ ((__packed__))
{
   uint16  command;
   uint16  vencStandard        :8;
   uint16  h263AnnexISpt       :1;
   uint16  h263AnnexJSpt       :1;
   uint16  h263AnnexTSpt       :1;
   uint16  rcFlag              :2;
   uint16  TeVersion           :1;
   uint16  hfr                 :1;
   uint16  reserved1           :1;
   uint16  application         :1;
   uint16  oneMVFlag           :1;
   uint16  acdcPredEnable      :2;
   uint16  roundingBitCtrl     :1;
   uint16  rotationFlag        :2;
   uint16  reserved3           :1;
   uint16  maxMVx              :2;
   uint16  maxMVy              :2;
   uint16  Reserved2           :4;
   uint16  fourMVThreshold;
   uint16  ise_mv_cost_fac;
   uint16  encFrameHeightInMB  :8;
   uint16  encFrameWidthInMB   :8;
   uint16  vencDMPartition;
} qdsp_venc_config_cmd_type;
      
typedef struct //__attribute__ ((__packed__))
{
   uint16  command;
   uint16  qp           :8;
   uint16  frameID      :7;
   uint16  frameType    :1;
   uint16  frameRhoBudgetHighWord;
   uint16  frameRhoBudgetLowWord;
   uint16  vencInputYAddr_MSW;   
   uint16  vencInputYAddr_LSW;   
   uint16  vencInputCbCrAddr_MSW;   
   uint16  vencInputCbCrAddr_LSW;   
   uint16  refVopBufPtr_High;   
   uint16  refVopBufPtr_Low;   
   uint16  encodedPktBufPtr_High;
   uint16  encodedPktBufPtr_Low;
   uint16  encodedPktBufSize_High;
   uint16  encodedPktBufSize_Low;
   uint16  ufReconVopBufPtr_High;
   uint16  ufReconVopBufPtr_Low;
   uint16  ftReconVopBufPtr_High;
   uint16  ftReconVopBufPtr_Low;
} qdsp_venc_frame_start_cmd_type;
   
typedef struct //__attribute__ ((__packed__))
{
   uint16  command;
   uint16  max_frame_qp_up_delta     :8;
   uint16  max_frame_qp_down_delta   :8;
   uint16  min_frame_qp              :8;
   uint16  max_frame_qp              :8;
} qdsp_venc_rc_cfg_cmd_type;

typedef struct //__attribute__ ((__packed__))
{
   uint16  command; 
   uint16  intra_refresh_num;
   uint16  intra_refresh_index[15];
} qdsp_venc_intra_refresh_cmd_type;

typedef struct //__attribute__ ((__packed__))
{
   uint16  command; 
   uint16  payload;
} qdsp_venc_clock_cmd_type;

typedef struct //__attribute__ ((__packed__))
{
   uint16  command; 
} qdsp_venc_idle_cmd_type;

typedef struct //__attribute__ ((__packed__))
{
   uint16  command; 
} qdsp_venc_active_cmd_type;


typedef enum
{ 
  VENCI_DRV_STATE_CLOSED,
  VENCI_DRV_STATE_OPENED,
  VENCI_DRV_STATE_SUSPENDED  
}venci_drv_state_type;

typedef struct 
{
  pmem_info ref_frame_buf1;
  pmem_info ref_frame_buf2;
  pmem_info enc_frame_buf1;
  pmem_info enc_frame_buf2;
} venci_drv_buffers_type;

typedef struct 
{
  uint8 inUse;
  qdsp_venc_config_cmd_type venc_config_cmd;
  venci_drv_state_type venc_drv_state; 
  venci_drv_cb_func_ptr_type venc_drv_cb_func_ptr;                  
  venci_drv_buffers_type venc_drv_buffers;
} venci_drv_object_type;

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

static volatile boolean venc_drv_mod_ready = FALSE;
static volatile boolean venc_active_cmd_done_recved = FALSE;
static volatile boolean venc_idle_cmd_done_recved = FALSE;

static qdsp_venc_config_cmd_type venc_config_cmd;
static qdsp_venc_frame_start_cmd_type  venc_frame_start_cmd;
static qdsp_venc_active_cmd_type venc_active_cmd;
static qdsp_venc_idle_cmd_type venc_idle_cmd;
static qdsp_venc_rc_cfg_cmd_type venc_rc_cfg_cmd;
static qdsp_venc_intra_refresh_cmd_type venc_intra_refresh_cmd;
static qdsp_venc_clock_cmd_type venc_clock_cmd;

static venci_drv_state_type venc_drv_state;
static venci_drv_object_type venc_drv_objects[VENC_DRV_MAX_NUM_INST];
static uint8 num_active_venc_instances = 0;

void venc_drv_rtos_msg_cb( uint16 msg_id, void* msg_buf, uint32 msg_len );
void venc_drv_rtos_event_cb( adsp_rtos_event_data_type *pEvent);

/*===========================================================================
FUNCTION ()

DESCRIPTION

DEPENDENCIES

   Buffers in the memory need to be physical memory. 

PARAMETERS

RETURN VALUE

IMPLEMENTATION NOTES

===========================================================================*/
void qdsp_videoenctask_mpuvenccmdqueue_command_drv(adsp_rtos_module_type module,
                                             qdsp_videoenctask_mpuvenccmdqueue_cmd_type cmd,
                                              uint16_t* cmd_buf,uint32_t cmd_size) 
{
  
    cmd_buf[0] = (uint16_t) cmd;
    VENC_MSG_LOW("qdsp_videoenctask_mpuvenccmdqueue_command_drv",0,0,0);
    qdsp_rtos_send_command_16 ( module,
                              QDSP_mpuVEncCmdQueue,
                              cmd_buf,
                              cmd_size );
}

int GetHwID()
{
  FILE *fp;
  int raw_version;
  int hw_revision = 0 ;
  
  if((fp=fopen("/sys/devices/system/soc/soc0/raw_version","r")) == NULL)
  {
    VENC_MSG_ERROR("Cannot open file.\n", 0, 0, 0);
  }

  fscanf(fp, "%d %d", &raw_version, 1); /* read from file */
  VENC_MSG_HIGH("raw_version %d",raw_version, 0, 0);
  hw_revision = ((raw_version >> 3 ) & 0x1); //HW TE = 0 FW = 1  MSB of the nibble
  fclose( fp );
  return hw_revision;

}

/*===========================================================================
  FUNCTION () Configured_HW

  DESCRIPTION :- this will use to configured AXI, VDC,aDSP.

  DEPENDENCIES

  PARAMETERS

  RETURN VALUE

  IMPLEMENTATION NOTES

  ===========================================================================*/
void Configured_HW(venci_drv_config_cmd_type* cfg)
{
 
   unsigned long vdc_clk = 96; //96UL in mhz ;
   unsigned long adsp_clk = 245; //default adsp value@245mhz
    int axi_freq = AXI_FREQ_VGA_OR_WVGA;
	int FrameWidthInMB = cfg->encFrameWidthInMB ;
	int FrameHeightInMB= cfg->encFrameHeightInMB;
    
    #ifdef FEATURE_VDC120MHZ
	if(((FrameWidthInMB*FrameHeightInMB) >= WVGA_MB_NUMBER) && 
		(cfg->fps >= 30 )&& ((cfg->vencStandard == 0) ||(cfg->vencStandard == 1))) //only for Mpeg4/h263 WVGA@30fps
	{
    vdc_clk =  120 ;
	}
    #endif
    
	if(((FrameWidthInMB*FrameHeightInMB) <= VGA_MB_NUMBER/2) && (cfg->fps < 30 ))
	{
		axi_freq = AXI_FREQ_HVGA_OR_BELOW;//for HVGA and small resolution.
	}
	axi_fd = open("/dev/system_bus_freq",O_RDWR);
	if (axi_fd < 0)
	{
		VENC_MSG_ERROR("cannot open axi_freq %d",axi_fd,0,0);
	}
	else
	{
	    VENC_MSG_HIGH("Request axi freq %d",axi_freq,0,0);
		if (write(axi_fd,&axi_freq,sizeof(axi_freq)) < 0)
		{
	    	VENC_MSG_ERROR("Request axi freq %d failed %d",axi_freq,0,0);
		}
	}
   
	if((FrameWidthInMB*FrameHeightInMB) >= VGA_MB_NUMBER )
	{
     #ifdef FEATURE_TURBO_MODULE
	 bIsTurboModeSet = TRUE ;
     VENC_MSG_HIGH("Enable turbo mode",0,0,0);
     adsp_rtos_register_app( QDSP_MODULE_VIDEO_AAC_VOC_TURBO,  venc_drv_rtos_msg_cb,
                              venc_drv_rtos_event_cb );
     adsp_rtos_enable(QDSP_MODULE_VIDEO_AAC_VOC_TURBO);
     adsp_clk = 320;     
     usleep(20*1000);
     #endif
	}

      VENC_MSG_HIGH("registering app with adsp svc...",0,0,0);
      adsp_rtos_register_app( QDSP_MODULE_VIDEOENCTASK,
                              venc_drv_rtos_msg_cb,
                              venc_drv_rtos_event_cb );
  
	  adsp_rtos_register_pmem(QDSP_MODULE_VIDEOENCTASK,cfg->ref_frame_buf1.handle,cfg->ref_frame_buf1.addr);
      VENC_MSG_HIGH("ref_frame_buf1.addr -- 0x%08x - Fd %d",cfg->ref_frame_buf1.addr,cfg->ref_frame_buf1.handle,0);

	  adsp_rtos_register_pmem(QDSP_MODULE_VIDEOENCTASK,cfg->ref_frame_buf2.handle,cfg->ref_frame_buf2.addr);
      VENC_MSG_HIGH("ref_frame_buf2.addr -- 0x%08x - Fd %d",cfg->ref_frame_buf2.addr,cfg->ref_frame_buf2.handle,0);

	  adsp_rtos_register_pmem(QDSP_MODULE_VIDEOENCTASK,cfg->enc_frame_buf1.handle,cfg->enc_frame_buf1.addr);
      VENC_MSG_HIGH("enc_frame_buf1.addr -- 0x%08x - Fd %d",cfg->enc_frame_buf1.addr,cfg->enc_frame_buf1.handle,0);

	  adsp_rtos_register_pmem(QDSP_MODULE_VIDEOENCTASK,cfg->enc_frame_buf2.handle,cfg->enc_frame_buf2.addr);
      VENC_MSG_HIGH("enc_frame_buf2.addr -- 0x%08x - Fd %d",cfg->enc_frame_buf2.addr,cfg->enc_frame_buf2.handle,0);
	  
     
      VENC_MSG_HIGH("enabling videoenc task...",0,0,0);
      adsp_rtos_enable(QDSP_MODULE_VIDEOENCTASK);  
      
  	  VENC_MSG_HIGH("vdc_clk = %ld mhz,adsp_clk = %ld mhz",vdc_clk,adsp_clk,0);	  
      adsp_rtos_set_clkrate(QDSP_MODULE_VIDEOENCTASK,vdc_clk*1000000);
	
#ifdef FEATURE_VDC120MHZ
    if(((FrameWidthInMB*FrameHeightInMB) >= WVGA_MB_NUMBER) && 
		(cfg->fps >= 30 )&& ((cfg->vencStandard == 0)||(cfg->vencStandard == 1))) //only for Mpeg4 WVGA@30fps
	{
     /*  Info on VDC Clock interface:
     * clk_regime_msm_get_clk_freq_khz(XXX) returns in KHz
     * valueToSendDSP = (vdc_clk/adsp_clk) in Q8
     * valueToSendDSP = ((vdc_clk*256)/adsp_clk)
     */
     venc_clock_cmd.payload = (uint16)((vdc_clk*256)/adsp_clk);
	 VENC_MSG_HIGH("venc_clock_cmd.payload = %d",venc_clock_cmd.payload,0,0);
      //Issue Video Encoder venc clock Command
		qdsp_videoenctask_mpuvenccmdqueue_command_drv (
			QDSP_MODULE_VIDEOENCTASK,
			QDSP_VIDEOENCTASK_VENC_VENC_CLOCK_CMD, 
			(uint16*)(&venc_clock_cmd),
			sizeof(qdsp_venc_clock_cmd_type) / 2);
     }
#endif
}

/*===========================================================================
  FUNCTION () Reset_HW

  DESCRIPTION :- this will use to reset AXI, VDC,aDSP.

  DEPENDENCIES

  PARAMETERS

  RETURN VALUE

  IMPLEMENTATION NOTES

  ===========================================================================*/
void Reset_HW()
{
	 int axi_freq = 0;
     adsp_rtos_set_clkrate(QDSP_MODULE_VIDEOENCTASK,61440000); //reduce VDC clock, be on safer side.
	 adsp_rtos_disable(QDSP_MODULE_VIDEOENCTASK );
     VENC_MSG_HIGH("adsp_rtos_disable",0,0,0);
	 if(bIsTurboModeSet)
	 {    
      #ifdef FEATURE_TURBO_MODULE
	  VENC_MSG_HIGH("shutting down turbo mode",0,0,0);
	  bIsTurboModeSet = FALSE ;
      adsp_rtos_disable(QDSP_MODULE_VIDEO_AAC_VOC_TURBO);
      #endif
     }

	 if (axi_fd > 0)
	 {
	   if (write(axi_fd, &axi_freq, sizeof(axi_freq)) < 0)
		{
		  VENC_MSG_ERROR("Failed to remove AXI requirement that's been set previously %d",axi_freq,0,0);
		}
		close(axi_fd);
	 	axi_fd = -1;
	 }
}

uint32 venc_drv_open(uint32* handle)
{
   uint32 ret = 0;
   int i;

   *handle = 0xffff;
   for (i = 0; i < VENC_DRV_MAX_NUM_INST ; i++) 
   {
      if (venc_drv_objects[i].inUse == 0)
      {
         *handle = (uint32) i;
         venc_drv_objects[i].inUse = 1;
         break;
      }
   }

   if (*handle == 0xffff) 
   {
      // To be Done
      // release the lock
      VENC_MSG_ERROR("VENC_DRV_START: FAILURE: handle == -1", 0, 0, 0);
      ret = -1;
   }

   return ret;
}

/*===========================================================================
  FUNCTION ()

  DESCRIPTION

  DEPENDENCIES

  Buffers in the memory need to be physical memory. 

  PARAMETERS

  RETURN VALUE

  IMPLEMENTATION NOTES

  ===========================================================================*/
uint32 venc_drv_start (uint32 handle, 
                       venci_drv_cb_func_ptr_type cbk_ptr,                      
                       venci_drv_config_cmd_type*  cfg)
{
  int i, cmd_counter = 0;
  qdsp_venc_config_cmd_type *venc_config_cmd_ptr;
  uint32 hw_revision =  0;
   #ifdef GET_HWID 
   hw_revision =   GetHwID();  
   #endif

   if((hw_revision == 1) || (hw_revision == 0))
   {
    VENC_MSG_HIGH("TE version = %d",hw_revision, 0, 0);
   }
   else
   {
    VENC_MSG_ERROR("TE version not known %d, set to HW TE",hw_revision, 0, 0);
    hw_revision = 0;
    }
   for(i =0 ; i < MAX_YUV_BUFFERS ; i++)   
   {
       FDS[i] = 0;   
   }
  if (cbk_ptr == NULL)
  {
      VENC_MSG_ERROR("VENC_DRV_START: FAILURE: cbk_ptr NULL", 0, 0, 0);
    return -5;
  }

  switch (venc_drv_state) 
  {
    case VENCI_DRV_STATE_CLOSED:
          venc_drv_mod_ready = FALSE;
          Configured_HW(cfg) ;
             // wait for the venc_drv_rtos_event_cb 
         if (venc_drv_mod_ready == FALSE) 
         {
         while(cmd_counter < 100)
          {    
            VENC_MSG_MEDIUM("still waiting on mod ready",0,0,0);
            usleep(1000 * 10);
            cmd_counter ++;
            if (venc_drv_mod_ready) 
            {
					VENC_MSG_HIGH("mod is ready in %d ms",cmd_counter,0,0);
               break;
            }
         }
      if (venc_drv_mod_ready == FALSE) 
        {
         VENC_MSG_ERROR("VENC_DRV_START: FAILURE: venc_drv_mode_ready FALSE",0,0,0); 
         return -2;
        }
      }
		VENC_MSG_HIGH("got mod ready in %d ms", cmd_counter, 0, 0);

      // save the cfg send the configuration to DSP
      venc_config_cmd_ptr = &venc_drv_objects[handle].venc_config_cmd;  
 
      venc_config_cmd_ptr->rcFlag = cfg->rcFlag;    // 0 for MB RC; 1 for frame RC; 2 for 2 stage RC
  
      venc_config_cmd_ptr->h263AnnexISpt = cfg->h263AnnexISpt;  
      venc_config_cmd_ptr->h263AnnexJSpt = cfg->h263AnnexJSpt;  
      venc_config_cmd_ptr->h263AnnexTSpt = cfg->h263AnnexTSpt;

      venc_config_cmd_ptr->vencStandard =  cfg->vencStandard;

      venc_config_cmd_ptr->maxMVy = cfg->maxMVy;
      venc_config_cmd_ptr->maxMVx = cfg->maxMVx;
      venc_config_cmd_ptr->rotationFlag = cfg->rotationFlag ;
      venc_config_cmd_ptr->roundingBitCtrl = cfg->roundingBitCtrl;
      venc_config_cmd_ptr->acdcPredEnable = cfg->acdcPredEnable;
      venc_config_cmd_ptr->oneMVFlag = cfg-> oneMVFlag ;

      // if DSP need to do partial run length coding
		if (cfg->partialRunLengthFlag == 1 || (cfg->vencStandard == 2))
      {
        venc_config_cmd_ptr->application = 0;   // DSP use QCamcorder path
			VENC_MSG_HIGH("camcorder pass for DSP",0,0,0);
      }
      else
      {
        venc_config_cmd_ptr->application = 1;   // DSP use QVP path
			VENC_MSG_HIGH("QVP pass for DSP",0,0,0);
      }

      venc_config_cmd_ptr->fourMVThreshold = cfg->fourMVThreshold;
      venc_config_cmd_ptr->ise_mv_cost_fac = cfg->ise_mv_cost_fac;
if(cfg->rotationFlag == 0 || cfg->rotationFlag == 2)
{
      venc_config_cmd_ptr->encFrameWidthInMB = cfg->encFrameWidthInMB;
      venc_config_cmd_ptr->encFrameHeightInMB = cfg->encFrameHeightInMB;
    
}
else  // if rotated
{
                VENC_MSG_HIGH("Rotate width & height",0,0,0);
                venc_config_cmd_ptr->encFrameWidthInMB = cfg->encFrameHeightInMB ;
		venc_config_cmd_ptr->encFrameHeightInMB = cfg->encFrameWidthInMB;
}
      venc_config_cmd_ptr->vencDMPartition = 0;
		if(cfg->fps > 30)
		{
			venc_config_cmd_ptr->hfr = 1;  // this is for higher frame rate 120 fps @qvga   Enable             
		}
		else
		{
			venc_config_cmd_ptr->hfr = 0;  // this is for higher frame rate 120 fps @qvga 
		}
		venc_config_cmd_ptr->TeVersion = hw_revision;
		venc_config_cmd_ptr->reserved1 = 0;
   
		VENC_MSG_HIGH("TE VERSION=%d,HighFrameRate=%d,send VENC_CONFIG ",hw_revision,venc_config_cmd_ptr->hfr,0);   
    
      //Issue Video Encoder venc Configuration Command
      qdsp_videoenctask_mpuvenccmdqueue_command_drv (
        QDSP_MODULE_VIDEOENCTASK,
        QDSP_VIDEOENCTASK_VENC_CONFIG_CMD, 
        (uint16*)(venc_config_cmd_ptr),
        sizeof(qdsp_venc_config_cmd_type) / 2);

      venc_drv_objects[handle].venc_drv_buffers.enc_frame_buf1 = cfg->enc_frame_buf1;   
      venc_drv_objects[handle].venc_drv_buffers.enc_frame_buf2 = cfg->enc_frame_buf2;
      venc_drv_objects[handle].venc_drv_buffers.ref_frame_buf1 = cfg->ref_frame_buf1;
      venc_drv_objects[handle].venc_drv_buffers.ref_frame_buf2 = cfg->ref_frame_buf2;

      venc_drv_objects[handle].venc_drv_cb_func_ptr = cbk_ptr;

      venc_active_cmd_done_recved = FALSE;
      // sends the active CMD
      VENC_MSG_HIGH("send VENC_ACTIVE", 0, 0, 0);  
      qdsp_videoenctask_mpuvenccmdqueue_command_drv(
        QDSP_MODULE_VIDEOENCTASK,
        QDSP_VIDEOENCTASK_VENC_ACTIVE_CMD,
        (uint16*)(&venc_active_cmd),
        sizeof(qdsp_venc_active_cmd_type) / 2);

      // Wait for 5 ms
      usleep(1000 * 5);

      cmd_counter = 0;

   //    venc_active_cmd_done_recved = TRUE;
      // the venc_active_cmd_done_recved should be set in venc_rtos_msg_cb
      if (venc_active_cmd_done_recved == FALSE)
      {
         VENC_MSG_HIGH("active cmd response not received, continue waiting...", 0, 0, 0);
        while(cmd_counter < 3)
        {    
            VENC_MSG_MEDIUM("still waiting on active cmd response", 0, 0, 0);
            usleep(1000 * 50);
          cmd_counter ++;
          if (venc_active_cmd_done_recved) 
          {
               VENC_MSG_HIGH("finally got active cmd response", 0, 0, 0);
            break;
          }
        }
        if (venc_active_cmd_done_recved == FALSE)
        {
         VENC_MSG_ERROR("VENC_DRV_START: FAILURE:venc_active_cmd_done_recved FALSE", 0, 0, 0); 
         return -3;   
        }
      }
      VENC_MSG_HIGH("got active command response", 0, 0, 0);
      venc_drv_state = VENCI_DRV_STATE_OPENED;
      num_active_venc_instances++;
      break;

    case VENCI_DRV_STATE_SUSPENDED:
      VENC_MSG_ERROR("VENC_DRV_START: FAILURE: in state VENC_DRV_STATE_SUSPENDED!", 0, 0, 0);
      return -4;
    default:
      break;  
  }
  return 0;
}
/*===========================================================================
FUNCTION ()

DESCRIPTION
   

DEPENDENCIES

   Buffers in the memory need to be physical memory. 

PARAMETERS

RETURN VALUE

IMPLEMENTATION NOTES

===========================================================================*/
uint32 venc_drv_stop (uint32 handle)
{

  uint8 cmd_counter = 0;
  uint32 ret_val = 0;
    VENC_MSG_MEDIUM("Handle :- %d,venc_drv_objects[handle].inUse = %d ,venc_drv_state = %d",
                     handle,venc_drv_objects[handle].inUse,venc_drv_state);
  // To be Done
  // acquire the lock
  if ((num_active_venc_instances == 0) || 
      (handle >= VENC_DRV_MAX_NUM_INST) ||  
      (venc_drv_objects[handle].inUse == 0) ||
      (venc_drv_state == VENCI_DRV_STATE_CLOSED))
  {
      VENC_MSG_ERROR("Rcvd venc_drv_stop cmd for invalid instance or at wrong state", 0, 0, 0);   
      return -1;
  }

  num_active_venc_instances --;

  if (num_active_venc_instances == 0) 
  {

    venc_drv_state = VENCI_DRV_STATE_CLOSED;
    //Issue Video Encoder IDLE Command
    venc_idle_cmd_done_recved = FALSE;
   VENC_MSG_HIGH("Send VENC_IDLE_CMD", 0, 0, 0);
    qdsp_videoenctask_mpuvenccmdqueue_command_drv(
      QDSP_MODULE_VIDEOENCTASK,
      QDSP_VIDEOENCTASK_VENC_IDLE_CMD,
      (uint16*)(&venc_idle_cmd),
      sizeof(qdsp_venc_idle_cmd_type)/2);

      usleep(1000 * 5);
    // the venc_idle_cmd_done_recved should be set in venc_rtos_msg_cb
    if (venc_idle_cmd_done_recved == FALSE)
    {
      while(cmd_counter < 3)
      {    
            usleep(1000 * 50);
        cmd_counter ++;
        if (venc_idle_cmd_done_recved) 
        {
          break;
        }
      }
      if (venc_idle_cmd_done_recved == FALSE)
      {
        ret_val = -1;   
      }
    }

    // disable the VIDEOENCTASK
      VENC_MSG_HIGH("venc_idle_cmd_done_recved %d cmd_counter %d", venc_idle_cmd_done_recved, cmd_counter, 0);
      Reset_HW();
  }

  return ret_val;
}


uint32 venc_drv_close (uint32 handle)
{
  venc_drv_objects[handle].inUse = 0;
  return 0;
}
/*===========================================================================
FUNCTION ()

DESCRIPTION
   

DEPENDENCIES

   Buffers in the memory need to be physical memory. 

PARAMETERS

RETURN VALUE

IMPLEMENTATION NOTES

===========================================================================*/
uint32 venc_drv_write (uint32 handle,
                      venci_drv_encode_cmd_type* encode)
{
   int i = 0;  
   static uint16 frameId = 1;
   int iIsFDSet = 0;
   uint16* yFrame = (uint16*) (encode->luma.addr);
   uint16* uvFrame = (uint16*) ( encode->chroma.addr);
    venci_drv_buffers_type *venc_drv_buf_ptr = &(venc_drv_objects[handle].venc_drv_buffers);  
   VENC_MSG_MEDIUM("Getting FD %d",encode->luma.handle,0,0);
   for(i =0 ; i < MAX_YUV_BUFFERS ; i++) 
   {
     if(FDS[i] == (int) encode->luma.handle)
     {
        VENC_MSG_MEDIUM("Already registered FD %d,Luma 0x%x,chroma:-0x%x",encode->luma.handle,yFrame,uvFrame);
        iIsFDSet = 1;
        break;
     }
     else if (FDS[i] == 0)
     {
       break;
     }
   }

   if(0 == iIsFDSet)
   {
      FDS[i]= (int) encode->luma.handle;
		VENC_MSG_HIGH("Registering  fd =%d luma.addr = 0x%x,chroma.addr =0x%X ", encode->luma.handle,encode->luma.addr, encode->chroma.addr)  ;
     adsp_rtos_register_pmem(QDSP_MODULE_VIDEOENCTASK,encode->luma.handle,(uint8*)encode->luma.addr- encode->luma.offset); 
     VENC_MSG_HIGH("base address:-0x%x",(uint8*)encode->luma.addr- encode->luma.offset,0,0);
   }
 

  if ((num_active_venc_instances == 0) || 
      (handle > VENC_DRV_MAX_NUM_INST) ||  
      (venc_drv_objects[handle].inUse == 0) ||
      (venc_drv_state != VENCI_DRV_STATE_OPENED))
  {
      VENC_MSG_ERROR("Rcvd venc_drv_write cmd for invalid instance",0,0,0);   
    return -1;
  }

  // maximum 16 instances can be supported
  venc_frame_start_cmd.frameID = frameId + (handle << 3);  
  venc_frame_start_cmd.qp = encode->QP;
  venc_frame_start_cmd.frameType = encode->frameType;

  venc_frame_start_cmd.frameRhoBudgetHighWord = 
    (uint16) ((encode->frame_rho_budget & 0xFFFF0000) >> 16);
  venc_frame_start_cmd.frameRhoBudgetLowWord = 
    (uint16) (encode->frame_rho_budget & 0x0000FFFF);

  venc_frame_start_cmd.vencInputYAddr_MSW = 
    (uint16) ((((uint32)yFrame) & 0xFFFF0000) >> 16);
  venc_frame_start_cmd.vencInputYAddr_LSW = 
    (uint16) (((uint32)yFrame) & 0x0000FFFF);
  venc_frame_start_cmd.vencInputCbCrAddr_MSW =
    (uint16) ((((uint32)uvFrame) & 0xFFFF0000) >> 16);
  venc_frame_start_cmd.vencInputCbCrAddr_LSW =  
    (uint16) (((uint32)uvFrame) & 0x0000FFFF);   

  venc_frame_start_cmd.encodedPktBufSize_High = (uint16) (encode->encodedPktBufSize >> 16);
  venc_frame_start_cmd.encodedPktBufSize_Low = (uint16) (encode->encodedPktBufSize & 0x0000FFFF); 
  if (frameId == 1)
  { 
	//Reference Frame Buffer
    venc_frame_start_cmd.refVopBufPtr_High = 
      (uint16)((uint32)(venc_drv_buf_ptr->ref_frame_buf1.addr) >> 16);

    venc_frame_start_cmd.refVopBufPtr_Low  = 
      (uint16)((uint32)(venc_drv_buf_ptr->ref_frame_buf1.addr) & 0x0000FFFF);

    // Enocded frame pointer
    venc_frame_start_cmd.encodedPktBufPtr_High = 
      (uint16)((uint32)(venc_drv_buf_ptr->enc_frame_buf1.addr) >> 16);
    venc_frame_start_cmd.encodedPktBufPtr_Low = 
      (uint16)((uint32)(venc_drv_buf_ptr->enc_frame_buf1.addr) & 0x0000FFFF);

    //Reconstruction Frame Buffer
    venc_frame_start_cmd.ufReconVopBufPtr_High = 
    venc_frame_start_cmd.ftReconVopBufPtr_High = 
      (uint16)((uint32)(venc_drv_buf_ptr->ref_frame_buf2.addr) >> 16);

    venc_frame_start_cmd.ufReconVopBufPtr_Low = 
    venc_frame_start_cmd.ftReconVopBufPtr_Low = 
      (uint16)((uint32)(venc_drv_buf_ptr->ref_frame_buf2.addr) & 0x0000FFFF);
    frameId = 2;
  }
  else
  {
    	  //Reference Frame Buffer
    venc_frame_start_cmd.refVopBufPtr_High = 
      (uint16)((uint32)(venc_drv_buf_ptr->ref_frame_buf2.addr) >> 16);
    venc_frame_start_cmd.refVopBufPtr_Low  = 
      (uint16)((uint32)(venc_drv_buf_ptr->ref_frame_buf2.addr) & 0x0000FFFF);

    // Enocded frame pointer
    venc_frame_start_cmd.encodedPktBufPtr_High = 
      (uint16)((uint32)(venc_drv_buf_ptr->enc_frame_buf2.addr) >> 16);

    venc_frame_start_cmd.encodedPktBufPtr_Low = 
      (uint16)((uint32)(venc_drv_buf_ptr->enc_frame_buf2.addr) & 0x0000FFFF);

    //Reconstruction Frame Buffer
    venc_frame_start_cmd.ufReconVopBufPtr_High = 
    venc_frame_start_cmd.ftReconVopBufPtr_High = 
      (uint16)((uint32)(venc_drv_buf_ptr->ref_frame_buf1.addr) >> 16);

    venc_frame_start_cmd.ufReconVopBufPtr_Low = 
    venc_frame_start_cmd.ftReconVopBufPtr_Low = 
      (uint16)((uint32)(venc_drv_buf_ptr->ref_frame_buf1.addr) & 0x0000FFFF);
    frameId = 1;
  }

  if (num_active_venc_instances > 1) 
  {
    VENC_MSG_MEDIUM("num_active_venc_instances %d ",num_active_venc_instances, 0, 0); 
    //Issue Video Encoder venc Configuration Command
    qdsp_videoenctask_mpuvenccmdqueue_command_drv (
      QDSP_MODULE_VIDEOENCTASK,
      QDSP_VIDEOENCTASK_VENC_CONFIG_CMD, 
      (uint16*)(&venc_drv_objects[handle].venc_config_cmd),
      sizeof(qdsp_venc_config_cmd_type) / 2);
  }
    VENC_MSG_MEDIUM("send frame start command", 0, 0, 0); 
  qdsp_videoenctask_mpuvenccmdqueue_command_drv (
    QDSP_MODULE_VIDEOENCTASK,
    QDSP_VIDEOENCTASK_VENC_FRAME_START_CMD,
    (uint16*)&venc_frame_start_cmd, 
    sizeof(qdsp_venc_frame_start_cmd_type)/2);

   VENC_MSG_MEDIUM("Kick QDSP Video Encoder Backend", 0, 0, 0);   

  return 0;
}
/*===========================================================================
FUNCTION ()

DESCRIPTION
   
DEPENDENCIES

   Buffers in the memory need to be physical memory. 

PARAMETERS

RETURN VALUE

IMPLEMENTATION NOTES

===========================================================================*/
uint32 venc_drv_ioctl (uint32 handle,
                       venci_drv_ioctl_cmd_type* ioctl_cmd)
{
  uint16 i;
  uint16 ir_count=0;
  qdsp_venc_config_cmd_type *venc_config_cmd_ptr;
  uint16 cmd_type = ioctl_cmd->ioctl_command_type;
  venci_drv_config_cmd_type* cfg;
  venci_drv_intra_refresh_cmd_type *ir_cmd_ptr;
  venci_drv_rc_cfg_cmd_type *rc_cfg_ptr;

  if ((num_active_venc_instances == 0) || 
      (handle > VENC_DRV_MAX_NUM_INST) ||  
      (venc_drv_objects[handle].inUse == 0) ||
      (venc_drv_state != VENCI_DRV_STATE_OPENED))
  {
      VENC_MSG_ERROR("Rcvd venc_drv_ioctl cmd for invalid instance", 0, 0, 0);   
    return -1;
  }
  switch (cmd_type)
  {

    case VENCI_DRV_IOCTL_INTRA_REFRESH_CMD:
    {
      ir_cmd_ptr = &(ioctl_cmd->venc_drv_intra_refresh_cmd);
      for (i = 0; i <ir_cmd_ptr->num_intra_mb; i++)
      {         
        venc_intra_refresh_cmd.intra_refresh_index[ir_count++]= ir_cmd_ptr->mb_index_array[i];
         
        if (ir_count == 15) //command can only contain 15 intra-refreshed MBs
        {
          //send one intra-refresh command
          venc_intra_refresh_cmd.intra_refresh_num = ir_count;
          VENC_MSG_MEDIUM("send venc_intra_refresh_cmd command %d ",ir_count,0,0);
          qdsp_videoenctask_mpuvenccmdqueue_command_drv (
                  QDSP_MODULE_VIDEOENCTASK,
                  QDSP_VIDEOENCTASK_VENC_INTRA_REFRESH_CMD,
                  (uint16*)&venc_intra_refresh_cmd, 
                  sizeof(qdsp_venc_intra_refresh_cmd_type)/2);

          ir_count = 0;
        }
      }

      //intra-refresh
      if (ir_count > 0)
      {
        venc_intra_refresh_cmd.intra_refresh_num = ir_count;
         VENC_MSG_MEDIUM("send venc_intra_refresh_cmd command %d ",ir_count,0,0);
        qdsp_videoenctask_mpuvenccmdqueue_command_drv (
                QDSP_MODULE_VIDEOENCTASK,
                QDSP_VIDEOENCTASK_VENC_INTRA_REFRESH_CMD,
                (uint16*)&venc_intra_refresh_cmd, 
                sizeof(qdsp_venc_intra_refresh_cmd_type)/2);
      }
    }
    break;

    case VENCI_DRV_IOCTL_CONFIG_CMD:
    {
      cfg = &(ioctl_cmd->venc_drv_config_cmd);
      venc_config_cmd_ptr = &venc_drv_objects[handle].venc_config_cmd;  

      venc_config_cmd_ptr->rcFlag = cfg->rcFlag;    // 0 for MB RC; 1 for frame RC; 2 for 2 stage RC


      venc_config_cmd_ptr->h263AnnexISpt = cfg->h263AnnexISpt;  
      venc_config_cmd_ptr->h263AnnexJSpt = cfg->h263AnnexJSpt;  
      venc_config_cmd_ptr->h263AnnexTSpt = cfg->h263AnnexTSpt;

      venc_config_cmd_ptr->vencStandard =  cfg->vencStandard;
      venc_config_cmd_ptr->maxMVy = cfg->maxMVy;
      venc_config_cmd_ptr->maxMVx = cfg->maxMVx;
      venc_config_cmd_ptr->rotationFlag = cfg->rotationFlag ;
      venc_config_cmd_ptr->roundingBitCtrl = cfg->roundingBitCtrl;
      venc_config_cmd_ptr->acdcPredEnable = cfg->acdcPredEnable;
      venc_config_cmd_ptr->oneMVFlag = cfg-> oneMVFlag ;

      // if DSP need to do partial run length coding
      if (cfg->partialRunLengthFlag == 1)
      {
        venc_config_cmd_ptr->application = 0;   // DSP use QCamcorder path
      }
      else
      {
        venc_config_cmd_ptr->application = 1;   // DSP use QVP path
      }

      venc_config_cmd_ptr->fourMVThreshold = cfg->fourMVThreshold;
      venc_config_cmd_ptr->ise_mv_cost_fac = cfg->ise_mv_cost_fac;

      venc_config_cmd_ptr->encFrameWidthInMB = cfg->encFrameWidthInMB;
      venc_config_cmd_ptr->encFrameHeightInMB = cfg->encFrameHeightInMB;

      venc_config_cmd_ptr->vencDMPartition = 0;

      venc_drv_objects[handle].venc_drv_buffers.enc_frame_buf1 = cfg->enc_frame_buf1;   
      venc_drv_objects[handle].venc_drv_buffers.enc_frame_buf2 = cfg->enc_frame_buf2;
      venc_drv_objects[handle].venc_drv_buffers.ref_frame_buf1 = cfg->ref_frame_buf1;
      venc_drv_objects[handle].venc_drv_buffers.ref_frame_buf1 = cfg->ref_frame_buf2;

      // for mulitple instance case, the VENC_CONFIG_CMD will be sent only before FRAME_START
      if (num_active_venc_instances == 1) {
        //Issue Video Encoder venc Configuration Command
        qdsp_videoenctask_mpuvenccmdqueue_command_drv (
          QDSP_MODULE_VIDEOENCTASK,
          QDSP_VIDEOENCTASK_VENC_CONFIG_CMD, 
          (uint16*)(venc_config_cmd_ptr),
          sizeof(qdsp_venc_config_cmd_type) / 2);
      }      
    }
    break;

    case VENCI_DRV_IOCTL_RC_CFG_CMD:
    {
      rc_cfg_ptr = &(ioctl_cmd->venc_drv_rc_cfg_cmd);  

      venc_rc_cfg_cmd.max_frame_qp_down_delta = rc_cfg_ptr->max_frame_qp_down_delta;
      venc_rc_cfg_cmd.max_frame_qp_up_delta = rc_cfg_ptr->max_frame_qp_up_delta;
  
      venc_rc_cfg_cmd.max_frame_qp = rc_cfg_ptr->max_frame_qp;
      venc_rc_cfg_cmd.min_frame_qp = rc_cfg_ptr->min_frame_qp;
      
      qdsp_videoenctask_mpuvenccmdqueue_command_drv (
        QDSP_MODULE_VIDEOENCTASK,
        QDSP_VIDEOENCTASK_VENC_RC_CFG_CMD,
        (uint16*)&venc_rc_cfg_cmd, 
        sizeof(venc_rc_cfg_cmd)/2);

    }
    break;

    default:

      VENC_MSG_ERROR("Rcvd invalid venc_drv_ioctl cmd %d", cmd_type, 0, 0);   

    break;
  }
  return 0;
}

void venc_drv_rtos_event_cb
(
   adsp_rtos_event_data_type *pEvent
)
{

  if(pEvent == NULL)
  {
      VENC_MSG_ERROR ("venc_drv_rtos_event_cb: call back with NULL event pointer", 0, 0, 0);
    return;
  }
   VENC_MSG_HIGH("venc_drv_rtos_event_cb(status: %d,module: %d,image: %d)",
                     pEvent->status, pEvent->module, pEvent->image );

  switch ( pEvent->status )
  {
    case ADSP_RTOS_MOD_READY:
      switch (venc_drv_state)
      { 
        case VENCI_DRV_STATE_CLOSED:
        {  
          venc_drv_mod_ready = TRUE;
          break;
        }
        default:
         VENC_MSG_ERROR("ADSP_RTOS_MOD_READY : QDSP callback in unexpected state!", 0, 0, 0);
      } /* switch */
      break;

    case ADSP_RTOS_MOD_DISABLE:
      switch (venc_drv_state)
      {
        case VENCI_DRV_STATE_OPENED : 
          venc_drv_mod_ready = FALSE;
          venc_drv_state = VENCI_DRV_STATE_SUSPENDED;
          break;
        default:
         VENC_MSG_ERROR("ADSP_RTOS_MOD_DISABLE  : QDSP callback in unexpected state!", 0, 0, 0);
      } /* switch */
      adsp_rtos_disable_event_rsp( QDSP_MODULE_VIDEOENCTASK );

      break;

    case ADSP_RTOS_CMD_FAIL:
      // TBD, resent the command after some time the block new command
      VENC_MSG_ERROR ("ADSP_RTOS_CMD_FAIL: Command Failed", 0, 0, 0);
      break;

    case ADSP_RTOS_CMD_SUCCESS:
      break;

    default:
      VENC_MSG_ERROR("Illegal module status!", 0, 0, 0);
      break;
  } /* switch */
  
} /* venc_drv_rtos_event_cb */
         


void venc_drv_rtos_msg_cb( uint16 msg_id, void* msg_buf, uint32 msg_len )
{
  uint16 venc_cmd_id; 
  uint8 buf_num, handle;
  uint32* buf_ptr;
  venci_cb_stru_type cb_stru;
  venci_drv_buffers_type *venc_drv_buf_ptr; 
  qdsp_videoenctask_vencmpurlist_msg_type  video_id =
    (qdsp_videoenctask_vencmpurlist_msg_type) msg_id;

  (void) msg_len; // warning removal
  switch( video_id )
  {
    case QDSP_VIDEOENCTASK_VENC_FRAME_DONE_MSG_ID:

      VENC_MSG_MEDIUM("received QDSP_VIDEOENCTASK_VENC_FRAME_DONE_MSG", 0, 0, 0);
      cb_stru.venc_data_cb.cb_type = 0;

      buf_num = (uint8) ((*((uint16*)msg_buf)) & 0x007FU);
      handle = buf_num >> 3;

      venc_drv_buf_ptr = &(venc_drv_objects[handle].venc_drv_buffers); 

      if ((buf_num & 0x3)==1) 
      {        
        buf_ptr = venc_drv_buf_ptr->enc_frame_buf1.addr;
      }
      else
      {
        buf_ptr = venc_drv_buf_ptr->enc_frame_buf2.addr;
      } 

      cb_stru.venc_data_cb.frame_type = (uint8) (((*((uint16*)msg_buf)) & 0x0080U) >> 7); // 0 for I frame, 1 for P frame 
      cb_stru.venc_data_cb.frame_len = (uint32) (*((uint16*)msg_buf+1)) + ((*((uint16*)msg_buf+2)) << 16);
      cb_stru.venc_data_cb.frame_addr = buf_ptr;
      venc_drv_objects[handle].venc_drv_cb_func_ptr ( 0,   // handle id
                                                      cb_stru);
      break;
    case QDSP_VIDEOENCTASK_VENC_CMD_DONE_MSG_ID:
      venc_cmd_id = *((uint16*)msg_buf);
      switch (venc_cmd_id)
      {
        case QDSP_VIDEOENCTASK_VENC_ACTIVE_CMD:
         VENC_MSG_HIGH ("received QDSP_VIDEOENCTASK_VENC_CMD_DONE_MSG for VENC_ACTIVE_CMD", 0, 0, 0);
          venc_active_cmd_done_recved = TRUE;
          break;
        case QDSP_VIDEOENCTASK_VENC_IDLE_CMD:
         VENC_MSG_HIGH ("received QDSP_VIDEOENCTASK_VENC_CMD_DONE_MSG for VENC_IDLE_CMD", 0, 0, 0);
          venc_idle_cmd_done_recved = TRUE;
          break;
        default:                
         VENC_MSG_HIGH ("received QDSP_VIDEOENCTASK_VENC_CMD_DONE_MSG for cmd %d", venc_cmd_id, 0, 0);
          break;
      }        
      break;
    case QDSP_VIDEOENCTASK_VENC_STATUS_MSG_ID:
      VENC_MSG_HIGH ("received QDSP_VIDEOENCTASK_VENC_STATUS_MSG", 0, 0, 0);
        /*TBD */
      break;
    case QDSP_VIDEOENCTASK_VENC_WRONG_CMD_MSG_ID:
      VENC_MSG_ERROR ("received QDSP_VIDEOENCTASK_VENC_WRONG_CMD_MSG", 0, 0, 0);
      break;
    default:

      VENC_MSG_ERROR ("received invalid message ID from VIDEOENC task, %d", video_id, 0, 0);
      break;
  }

}

/*===========================================================================
FUNCTION ()venc_drv_malloc

DESCRIPTION


===========================================================================*/
uint32 venc_drv_malloc (uint32 handle, 
                        pmem_info* pmem)
{
	uint32 i = 0; 
	uint32 ret = 0;
	struct pmem_region region;
	pmem->size = (pmem->size + 4095) & (~4095);         
	if (pmem->cached == TRUE)
	{
		pmem->handle = open("/dev/pmem", O_RDWR);
		VENC_MSG_HIGH("opening /dev/pmem(cache) files...size :- %d",pmem->size,0,0);
		pmem->cached = TRUE ;
	}
	else
	{
		pmem->handle = open("/dev/pmem_adsp", O_RDWR);
		VENC_MSG_HIGH("opening /dev/pmem_adsp files...size :- %d",pmem->size,0,0);
		pmem->cached = FALSE ;
	}

	if (pmem->handle < 0)
	{
		VENC_MSG_ERROR("error could not open pmem device: %s", strerror(errno),0,0);
		perror("cannot open pmem device");
		return -1;
	}

	pmem->addr = mmap(NULL,pmem->size, PROT_READ | PROT_WRITE,MAP_SHARED, pmem->handle, 0);
	if (pmem->addr == MAP_FAILED) 
	{
		VENC_MSG_ERROR("error mmap failed: %s", strerror(errno),0,0);
		close(pmem->handle);
		pmem->handle = -1;
		return -1;
	}

	VENC_MSG_HIGH("phys lookup success virt=0x%x -- FD=%d -- size - %d",
		pmem->addr,pmem->handle, pmem->size);

	pmem->offset = 0;
	return ret;

}

uint32 venc_drv_free (uint32 handle, pmem_info* pmem)
{
   uint32 i = 0; 
   uint32 ret = 0;
	if (pmem->handle > 0)
    {
	       VENC_MSG_HIGH("free up memory Addr =0x%x -- FD=%d -- size - %d",
                       pmem->addr,pmem->handle, pmem->size);  
          munmap(pmem->addr,pmem->size);
          close(pmem->handle);
          pmem->addr = NULL;
          pmem->handle = -1;
		  return ret ;
     
    }
    else
	{
		VENC_MSG_ERROR("cant free, null pointer==0x%x -- FD=%d -- size - %d",
                       pmem->addr,pmem->handle, pmem->size);
        ret = -1;
	    return ret;
   }
}
uint32 venc_drv_cache_op(uint32 handle,pmem_info* pmem,uint32 size)
{
   if (pmem->cached == TRUE)
   {
	   struct pmem_addr pmem_addr;	
	   pmem_addr.vaddr = (unsigned long)(pmem->addr) ;
	   pmem_addr.offset = 0 ; 
	   pmem_addr.length = (size+ 4095) & (~4095);   
	   VENC_MSG_MEDIUM("Invalidating & clean: Fd = %d with address 0x%x, length = %d",
		   pmem->handle,pmem_addr.vaddr,pmem_addr.length);

	   if (ioctl(pmem->handle,PMEM_CLEAN_INV_CACHES,&pmem_addr)) 
	   {
		   VENC_MSG_ERROR("pmem cache failed",0,0,0);
		   return -1;
	   }
	   return 0;
   }
     return 0;
}

uint32 venc_drv_cache_inv(uint32 handle,pmem_info* pmem,uint32 size)
{
	if (pmem->cached == TRUE)
	{
		struct pmem_addr pmem_addr;	
		pmem_addr.vaddr = (unsigned long)(pmem->addr) ;
		pmem_addr.offset = 0 ; 
		pmem_addr.length = (size+ 4095) & (~4095);   
	VENC_MSG_MEDIUM("Invalidating cache: Fd = %d with address 0x%x, length = %d",
                     pmem->handle,pmem_addr.vaddr,pmem_addr.length);

		if (ioctl(pmem->handle,PMEM_INV_CACHES,&pmem_addr)) 
       {
		VENC_MSG_ERROR("pmem cache failed",0,0,0);
			return -1;
	}
     return 0;
    }
     return 0;
}

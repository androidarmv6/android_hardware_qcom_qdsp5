#ifndef VENC_DRV_H
#define VENC_DRV_H

#ifdef __cplusplus
extern "C" {
#endif

/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                           V E N C     D R V 
                           I N T E R F A C E


   Because VENC Device Layer is strctured as a platform-independent 
   modular entity, this interface, which is internal to device layer, 
   abstracts the platform-specific driver layer. 


Copyright (c) 2007 by Qualcomm, Incorporated. All Rights Reserved. 
Qualcomm Proprietary and Confidential.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/


/*===========================================================================

                            EDIT HISTORY FOR FILE
$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/driver/inc/venc_drv.h#3 $

when          who    what, where, why
----------    ---    ----------------------------------------------------------
01/26/10      as     Added new API for cache invalidate
10/05/09      as     Support for HFR
08/13/09      bc     cleaned up driver abstraction interface
12/29/08      as      Support added to merge virtual address passing 
                      sync with andriod release 
02/28/2008    bc     Added handshake to protect dsp pkt buffers for parallel
                     processing design
10/10/2007    js     Renamed all data types with prefix venci_, as going forward,
                     this iface will be internal only to Device Layer and platform
                     independent. 
09/19/2007    js     Cleaned up Device Layer side interface. Formatted 
                     for code review and archiving. 
===========================================================================*/


/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/
#include "venc_def.h"

/*===========================================================================

                         EXTERNAL DATA DECLARATIONS

===========================================================================*/


typedef struct 
{
    void* handle;
    uint32 offset;
    uint32 size;
    void* addr;
    boolean cached;
}pmem_info;

typedef struct
{
   pmem_info luma;
   pmem_info chroma;
   int16 xOffset;
   int16 yOffset;
   uint32 frame_rho_budget;
   uint32 encodedPktBufSize;
   uint8 frameType;
   uint16 QP;
} venci_drv_encode_cmd_type;

typedef struct 
{ 
  uint16  ioctl_command_type;
  uint16  max_frame_qp_up_delta;
  uint16  max_frame_qp_down_delta;
  uint16  min_frame_qp;
  uint16  max_frame_qp;
} venci_drv_rc_cfg_cmd_type;

typedef struct 
{
  uint16  ioctl_command_type;
  uint16  num_intra_mb;
  uint16* mb_index_array;
} venci_drv_intra_refresh_cmd_type;

typedef struct 
{
  uint16 ioctrl_command_type;
  uint8  vencStandard;
  uint8  partialRunLengthFlag;       // should be 1 if no error resident support
  uint8  h263AnnexISpt;
  uint8  h263AnnexJSpt;
  uint8  h263AnnexTSpt;
  uint8  rcFlag;
  uint8  oneMVFlag;
  uint8  acdcPredEnable;
  uint8  roundingBitCtrl;
  uint8  rotationFlag;
  uint8  maxMVx;
  uint8  maxMVy;
  uint16 fps ;
  uint16 fourMVThreshold;
  uint16 ise_mv_cost_fac;
  uint16 encFrameHeightInMB;
  uint16 encFrameWidthInMB;
  pmem_info ref_frame_buf1;
  pmem_info ref_frame_buf2;
  pmem_info enc_frame_buf1;
  pmem_info enc_frame_buf2;
} venci_drv_config_cmd_type;

typedef union
{
  uint16 ioctl_command_type;
  venci_drv_intra_refresh_cmd_type venc_drv_intra_refresh_cmd;
  venci_drv_config_cmd_type venc_drv_config_cmd; 
  venci_drv_rc_cfg_cmd_type venc_drv_rc_cfg_cmd;
} venci_drv_ioctl_cmd_type;

#define VENCI_DRV_IOCTL_INTRA_REFRESH_CMD 1
#define VENCI_DRV_IOCTL_CONFIG_CMD 2
#define VENCI_DRV_IOCTL_RC_CFG_CMD 3

typedef struct 
{
  uint8 cb_type;            // type = 1 for control cmd
  uint8 ctrl_cmd;           // the int8 value will be:
                           // 1: resource temporarily unavailable
                           // 2: resource available
} venci_ctrl_cmd_cb_type;

typedef struct
{
  uint8 cb_type;               // type = 0
  uint8 frame_type;            // I frame or P frame
  uint32 frame_len;            // frame length 
  uint32* frame_addr;          // addess of encoded frame
} venci_data_cb_type;  

typedef union
{
  uint8 cb_type;
  venci_ctrl_cmd_cb_type venc_ctrl_cmd_cb;
  venci_data_cb_type venc_data_cb;  
} venci_cb_stru_type;

typedef int32 (* venci_drv_cb_func_ptr_type) 
(
  uint32 handle,
  venci_cb_stru_type cb_stru             
);

/*===========================================================================
FUNCTION VENC_DRV_OPEN()

DESCRIPTION
   VENC_DRV_OPEN() SYNCHRONOUSLY opens the platform-specific video encoder
   driver device. 

DEPENDENCIES
   None

PARAMETERS
   handle (out) driver device's client handle

RETURN VALUE
    0 success
   -1 failure

IMPLEMENTATION NOTES
    Currently assumes synchronous return. 

===========================================================================*/
uint32 venc_drv_open(uint32* handle);

/*===========================================================================
FUNCTION VENC_DRV_START()

DESCRIPTION
   VENC_DRV_START() SYNCHRONOUSLY opens the platform-specific video encoder
   driver device with the provided configurations. 

   If function returned successfully, the video encoder driver device is
   ready to accept incoming data through Write(). 

DEPENDENCIES
   Buffers in the memory need to be physical memory. 

PARAMETERS
   handle (in) handle of an opened driver. 
   cbk_ptr (in) pointer to driver device callback function. Data/status are
                communicated to client via this callback. 
   cfg (in) Driver configurations. 

RETURN VALUE
    0 success
   -1 failure

IMPLEMENTATION NOTES
    Currently assumes synchronous return. 

===========================================================================*/
uint32 venc_drv_start(uint32 handle, 
                      venci_drv_cb_func_ptr_type cbk_ptr,     // Client cb fn
                      venci_drv_config_cmd_type*  cfg);       // Encoder config 

/*===========================================================================
FUNCTION VENC_DRV_STOP()

DESCRIPTION
   Synchrnously stops a previously started device driver. 

DEPENDENCIES
   None. 

PARAMETERS
   handle (in) handle of an opened driver to be stopped. 

RETURN VALUE
   0 success

IMPLEMENTATION NOTES
    Currently assumes synchronous return. 
===========================================================================*/
uint32 venc_drv_stop (uint32 handle);

/*===========================================================================
FUNCTION VENC_DRV_CLOSE()

DESCRIPTION
   Synchrnously closes a previously opened device driver. 

DEPENDENCIES
   None. 

PARAMETERS
   handle (in) handle of an opened driver to be closed. 

RETURN VALUE
   0 success

IMPLEMENTATION NOTES
    Currently assumes synchronous return. 
===========================================================================*/
uint32 venc_drv_close (uint32 handle);

/*===========================================================================
FUNCTION VENC_DRV_WRITE()

DESCRIPTION
   Writing an input frame into DSP for lossy encoding. Encoded frame, or
   encoding status, will be asynchronously returned through the registered
   callback function ptr. 

DEPENDENCIES
   All the memories need to be physical memory. 

PARAMETERS
   handle (in) valid client handle of device driver
   yFrame/uvFrame (in) pointer to the input frame to be encoded. 
                       Must be physical addresses. 
   xOffset/yOffset (in) DVS offsets
   frame_rho_budget (in)
   encodedPktBufSize (in) Size of the output buffer for the requested frame. 
   frameType (in) I or P
   QP (in)

RETURN VALUE
   0 success

   encoded frame/frame encoding status - returned asynchrnously through
                                         registered callback. 

IMPLEMENTATION NOTES
===========================================================================*/
uint32 venc_drv_write (uint32 handle,
                      venci_drv_encode_cmd_type* encode);

/*===========================================================================
FUNCTION VENC_DRV_IOCTL()

DESCRIPTION
   This function allows various controls and configurations of an opened 
   device driver. 

DEPENDENCIES
   handle must be a valid handle to an already opened driver. 

PARAMETERS
   handle (in)
   ioctl_cmd (in) - Specifies the type of the command and the payload
                    associated with the command. 

RETURN VALUE
   0 success. 

IMPLEMENTATION NOTES
   Unless explicity stated otherwise, all IOCTL are assumed to be synchronous. 

===========================================================================*/                       
uint32 venc_drv_ioctl (uint32 handle,
                       venci_drv_ioctl_cmd_type* ioctl_cmd); 

/*===========================================================================
FUNCTION VENC_DRV_MALLOC()

DESCRIPTION
   Wrapper function which abstracts the platform-dependent memory allocation.

DEPENDENCIES
   None. 

PARAMETERS
   size (in) size in number of bytes of the memory to be allocated
   vir_addr/phy_addr (out) pointer of pointers to allocated memory (virtual 
                           and physical)

RETURN VALUE
   -1 failure
   number of bytes allocated otherwise. 

IMPLEMENTATION NOTES
   This API may be remoted or local, depending on the platform specific 
   memory implementation. 

===========================================================================*/                       
uint32 venc_drv_malloc (uint32 handle,
                        pmem_info* pmem);

/*===========================================================================
FUNCTION VENC_DRV_FREE()

DESCRIPTION
   Abstraction of platform-specific memory deallocation. 

DEPENDENCIES
   None. 

PARAMETERS
   addr (in) memory to be deallocated. 

RETURN VALUE
   0 success. 

IMPLEMENTATION NOTES
   This API may be remoted or local, depending on the platform specific 
   memory implementation. 

===========================================================================*/                       
uint32 venc_drv_free (uint32 handle, pmem_info* pmem);

/*===========================================================================
FUNCTION VENC_DRV_CACHE_OP()

DESCRIPTION
   Abstraction of platform-specific cache invalidation. 

DEPENDENCIES
   None. 

PARAMETERS
   number of  size to be invalidated & clean
   addr (in) cached memory . 

RETURN VALUE
   0 success. 

IMPLEMENTATION NOTES
   This API may be remoted or local, depending on the platform specific 
   memory implementation. 

===========================================================================*/ 
uint32 venc_drv_cache_op(uint32 handle,pmem_info* pmem,uint32 size) ;

/*===========================================================================
FUNCTION VENC_DRV_CACHE_OP()

DESCRIPTION
   Abstraction of platform-specific cache invalidation. 

DEPENDENCIES
   None. 

PARAMETERS
   number of  size to be invalidated
   addr (in) cached memory . 

RETURN VALUE
   0 success. 

IMPLEMENTATION NOTES
   This API may be remoted or local, depending on the platform specific 
   memory implementation. 

===========================================================================*/ 
uint32 venc_drv_cache_inv(uint32 handle,pmem_info* pmem,uint32 size) ;

#ifdef __cplusplus
}
#endif

#endif // VENC_DRV_H

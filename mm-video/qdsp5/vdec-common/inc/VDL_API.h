#ifndef VDL_API_H
#define VDL_API_H

/* =======================================================================
                               VDL-API.h
DESCRIPTION
  API provided by the Driver Layer

Copyright (c) 2007 by QUALCOMM, Incorporated.  All Rights Reserved. 
QUALCOMM Proprietary and Confidential.
========================================================================== */

/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/QDSP5VdecDriver/main/latest/src/VDL_API.h#1 $
$DateTime: 2008/08/05 12:05:56 $
$Change: 717059 $

========================================================================== */

/*==========================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */

#include "VDL_Types.h"
#include "vdecoder_types.h"
#ifdef  __cplusplus
extern "C" {
#endif


/*==========================================================================

                        DATA DECLARATIONS

========================================================================== */
/*===========================================================================*/
/* FUNCTION DEFINITIONS                                                      */
/*===========================================================================*/
/*===========================================================================

FUNCTION:
  VDL_Create

DESCRIPTION:
  This function creates an instance for the VDL layer by calling the VDL 
  constructor

DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  stream ID,
  VDL_ERROR
  
RETURN VALUE:
  VDL Instance

SIDE EFFECTS:
  None

===========================================================================*/
void* VDL_Create(VDL_ERROR *const pErr_out);

/*===========================================================================

FUNCTION:
  VDL_Initialize

DESCRIPTION:
  This function calls the Initialize function using the appropriate VDL instance
  
DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  VDL instance pointer (void *)
  decoder callback function
  user data for decoder callback
  encoded video format, concurrency format
  overriding DSP image for transcoding / audio dubbing
  
RETURN VALUE:
  error code provided by VDL_ERROR

SIDE EFFECTS:
  None

===========================================================================*/
VDL_ERROR VDL_Initialize(
						void *stream,    
						VDL_Decoder_Cb_Type   DecoderCB,  /* decode done call back function */
						void                 *ddUserData,    /* user data for DecoderCB */
						VDL_Video_Type VideoFormat,           /* video format */
						VDL_Concurrency_Type ConcurrencyFormat,  /* audio/concurrency format */ 
						int adsp_fd
                      );

/*===========================================================================

FUNCTION:
  VDL_Terminate

DESCRIPTION:
  This function calls the Terminate function using the appropriate VDL instance
  
DEPENDENCIES
  VDL_TERMINATE MANDATES THAT VDL_SUSPEND BE COMPLETED BEFORE

INPUT/OUTPUT PARAMETERS:
  VDL instance pointer (void *)
  
RETURN VALUE:
  error code provided by VDL_ERROR

SIDE EFFECTS:
  None

===========================================================================*/
VDL_ERROR VDL_Terminate(void *stream);

/*===========================================================================

FUNCTION:
  VDL_Destroy

DESCRIPTION:
  This function calls the Destroy function using the appropriate VDL instance
  
DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  VDL instance pointer (void *)
  
RETURN VALUE:
  error code provided by VDL_ERROR

SIDE EFFECTS:
  None

===========================================================================*/
VDL_ERROR VDL_Destroy(void *stream);

/*===========================================================================

FUNCTION:
  VDL_Flush

DESCRIPTION:
  This function calls the Flush function using the appropriate VDL instance
  
DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  VDL instance pointer (void *)
  
RETURN VALUE:
  error code provided by VDL_ERROR

SIDE EFFECTS:
  None

===========================================================================*/
VDL_ERROR VDL_Flush(void *stream);

/*===========================================================================

FUNCTION:
  VDL_Suspend

DESCRIPTION:
  This function calls the Suspend function using the appropriate VDL instance
  
DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  VDL instance pointer (void *)
  
RETURN VALUE:
  error code provided by VDL_ERROR

SIDE EFFECTS:
  None

===========================================================================*/
VDL_ERROR VDL_Suspend(void *stream);

/*===========================================================================

FUNCTION:
  VDL_Resume

DESCRIPTION:
  This function calls the Resume function using the appropriate VDL instance
  
DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  VDL instance pointer (void *)
  
RETURN VALUE:
  error code provided by VDL_ERROR

SIDE EFFECTS:
  None

===========================================================================*/
VDL_ERROR VDL_Resume(void *stream,
				   VDL_Concurrency_Type ConcurrencyFormat  /* audio/concurrency format */  
                  );

/*===========================================================================

FUNCTION:
  VDL_V2P

DESCRIPTION:
  This function converts a virtal pmem address to a physical address
  
DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  stream - VDL instance pointer (void *)
  vAddr  - input virtual address
  pAddr  - output physical address
  
RETURN VALUE:
  error code provided by VDL_ERROR

SIDE EFFECTS:
  None

===========================================================================*/
VDL_ERROR VDL_V2P(void *stream, unsigned int vAddr, unsigned int *pAddr);

/*===========================================================================

FUNCTION:
  VDL_Configure_HW

DESCRIPTION:
  This function calls the VDL_Configure_HW function using the appropriate VDL instance
  
DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  VDL instance pointer (void *)
  height, width
  
RETURN VALUE:
  error code provided by VDL_ERROR

SIDE EFFECTS:
  None

===========================================================================*/
VDL_ERROR VDL_Configure_HW(void *stream,
			   unsigned long int Height,
			   unsigned long int Width,
			   VDL_Profile_Type profile,
                           VDL_Video_Type VideoFormat,
                           unsigned char **frame_buffers,
                           unsigned int num_frame_buffers,
                           void *frameHeader,
                           VDEC_SLICE_BUFFER_INFO* input,
                           unsigned short num_slice_buffers);

/*===========================================================================

FUNCTION:
  VDL_Get_Slice_Buffer

DESCRIPTION:
  This function calls the VDL_Get_Slice_Buffer function using the appropriate VDL instance
  
DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  VDL instance pointer (void *)
  
RETURN VALUE:
  Pointer to slice buffer of VDL_Slice_Pkt_Type

SIDE EFFECTS:
  None

===========================================================================*/
VDL_Slice_Pkt_Type * VDL_Get_Slice_Buffer(void *stream);

/*===========================================================================

FUNCTION:
  VDL_Free_Slice_Buffer

DESCRIPTION:
  This function calls the VDL_Free_Slice_Buffer function using the appropriate VDL instance
  
DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  VDL instance pointer (void *)
  Pointer to slice buffer of VDL_Slice_Pkt_Type
  
RETURN VALUE:
  error code provided by VDL_ERROR

SIDE EFFECTS:
  None

===========================================================================*/
VDL_ERROR VDL_Free_Slice_Buffer(void *stream,
							  VDL_Slice_Pkt_Type *pSliceBuf 
							 );

/*===========================================================================

FUNCTION:
  VDL_Queue_Slice_Buffer

DESCRIPTION:
  This function calls the VDL_Queue_Slice_Buffer function using the appropriate VDL instance
  
DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  VDL instance pointer (void *)
  Pointer to slice buffer of VDL_Slice_Pkt_Type
  
RETURN VALUE:
  error code provided by VDL_ERROR

SIDE EFFECTS:
  None

===========================================================================*/
VDL_ERROR VDL_Queue_Slice_Buffer(void *stream,
							   VDL_Slice_Pkt_Type *pSliceBuf  
							  );

/*===========================================================================

FUNCTION:
  VDL_Queue_Stats_Buffer

DESCRIPTION:
  This function calls the VDL_Queue_Stats_Buffer function using the appropriate VDL instance
  
DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  Pointer to VDEC FRAME data for the frame 
  
RETURN VALUE:
  error code provided by VDL_ERROR

SIDE EFFECTS:
  None

===========================================================================*/
VDL_ERROR VDL_Queue_Stats_Buffer(void *stream,
							   void *pDecodeStats,
							   VDL_Decode_Return_Type retCode
							  );

VDL_ERROR VDL_IsSliceBufAvailableForDecode(void *stream, 
                                           unsigned char* pBufAvailable);

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

VDL_Interface_Type VDL_Get_Interface_Type(void *stream,
					  unsigned long int Height,
					  unsigned long int Width,
					  VDL_Video_Type VideoFormat, 
					  VDL_Profile_Type profile
                                         );
                                          

/*===========================================================================

FUNCTION:
  VDL_Get_PP_Pkt_Buffer

DESCRIPTION:
  This function calls the VDL_Get_PP_Pkt_Buffer function using the appropriate VDL instance
  
DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  VDL instance pointer (void *)
  
RETURN VALUE:
  Pointer to slice buffer of VDL_PP_cmd_Info_Type

SIDE EFFECTS:
  None

===========================================================================*/
VDL_PP_cmd_Info_Type * VDL_Get_PP_Pkt_Buffer(void *stream);

/*===========================================================================

FUNCTION:
  VDL_Free_PP_Pkt_Buffer

DESCRIPTION:
  This function calls the VDL_Free_PP_Cmd_Info_Buffer function using the appropriate VDL instance
  
DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  VDL instance pointer (void *)
  Pointer to slice buffer of VDL_PP_cmd_Info_Type
  
RETURN VALUE:
  error code provided by VDL_ERROR

SIDE EFFECTS:
  None

===========================================================================*/
VDL_ERROR VDL_Free_PP_Pkt_Buffer(void *stream,
                              VDL_PP_cmd_Info_Type *p_PPCmdBuf 
                             );

/*===========================================================================

FUNCTION:
  VDL_Queue_PP_Pkt_Buffer

DESCRIPTION:
  This function calls the VDL_Queue_PP_Pkt_Buffer function using the appropriate VDL instance
  
DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  VDL instance pointer (void *)
  Pointer to slice buffer of VDL_PP_cmd_Info_Type
  
RETURN VALUE:
  error code provided by VDL_ERROR

SIDE EFFECTS:
  None

===========================================================================*/
VDL_ERROR VDL_Queue_PP_Pkt_Buffer(void *stream,
                               VDL_PP_cmd_Info_Type *p_PPCmdBuf  
                              );
#ifdef __cplusplus
};
#endif

#endif


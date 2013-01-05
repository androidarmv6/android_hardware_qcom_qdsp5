/* =======================================================================
                               VDL-API.cpp
DESCRIPTION
  Implementation of the Driver Layer API

Copyright (c) 2007 by QUALCOMM, Incorporated.  All Rights Reserved. 
QUALCOMM Proprietary and Confidential.
========================================================================== */

/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/QDSP5VdecDriver/main/latest/src/VDL_API.cpp#1 $
$DateTime: 2008/08/05 12:05:56 $
$Change: 717059 $

========================================================================== */

/*==========================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */
#include "VDL_API.h"
#include "VDL.h"
#include "vdecoder_types.h"
/*==========================================================================

                DEFINITIONS AND DECLARATIONS FOR MODULE

This section contains definitions for constants, macros, types, variables
and other items needed by this module.

=========================================================================*/

/*===========================================================================

FUNCTION:
  VDL_Create

DESCRIPTION:
  This function creates an instance for the Driver layer by calling the VDL 
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
void* VDL_Create(VDL_ERROR * const pErr_out)
{   
  
  VDL *VDL_instance;

  #ifdef FEATURE_VDL_TEST_PC
  VDL_instance = new VDL(pErr_out);
  #else  
  VDL_instance = (VDL *)QTV_New_Args(VDL, (pErr_out) );  
  #endif

  if (VDL_instance==NULL)
    *pErr_out = VDL_ERR_NULL_STREAM_ID;

  return(void *)VDL_instance;
}


/*===========================================================================

FUNCTION:
  VDL_Initialize

DESCRIPTION:
  This function calls the Initialize function using the appropriate VDL instance
  
DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  VDL instance pointer (void *)
  decoder calback function
  user data for decoder callback
  encoded video format, audio/concurrency format  
  
RETURN VALUE:
  error code provided by VDL_ERROR

SIDE EFFECTS:
  None

===========================================================================*/
VDL_ERROR VDL_Initialize(
                      void *stream,    
                      VDL_Decoder_Cb_Type   DecoderCB,        /* decode done call back function */
                      void                 *ddUserData,      /* user data for DecoderCB */
                      VDL_Video_Type VideoFormat,             /* Encoded video format */
                      VDL_Concurrency_Type ConcurrencyFormat,  /* Encoded audio/concurrency format */ 
		      int adsp_fd

                      )
{
  VDL *VDL_instance = ( VDL *)stream;

  if ( VDL_instance == NULL )
  {
    return VDL_ERR_NULL_STREAM_ID;
  }

  return VDL_instance->VDL_Initialize(DecoderCB, ddUserData, VideoFormat, ConcurrencyFormat, adsp_fd);  

}


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
VDL_ERROR VDL_Terminate(void *stream)
{
  VDL *VDL_instance = ( VDL *)stream;

  if ( VDL_instance == NULL )
  {
    return VDL_ERR_NULL_STREAM_ID;
  }

  return VDL_instance->VDL_Terminate( );  

}

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
VDL_ERROR VDL_Destroy(void *stream)
{
  VDL *VDL_instance = ( VDL *)stream;

  if ( VDL_instance == NULL )
  {
    return VDL_ERR_NULL_STREAM_ID;
  }

  #ifdef FEATURE_VDL_TEST_PC
  delete VDL_instance;
  #else
  QTV_Delete(VDL_instance);
  #endif

  return VDL_ERR_NONE;
}


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
VDL_ERROR VDL_Flush(void *stream)
{
  VDL *VDL_instance = ( VDL *)stream;

  if ( VDL_instance == NULL )
  {
    return VDL_ERR_NULL_STREAM_ID;
  }

  return VDL_instance->VDL_Flush();

}


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
VDL_ERROR VDL_Suspend(void *stream)
{
  VDL *VDL_instance = ( VDL *)stream;

  if ( VDL_instance == NULL )
  {
    return VDL_ERR_NULL_STREAM_ID;
  }

  return VDL_instance->VDL_Suspend( );

}


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
                  )
{
  VDL *VDL_instance = ( VDL *)stream;

  if ( VDL_instance == NULL )
  {
    return VDL_ERR_NULL_STREAM_ID;
  }

  return VDL_instance->VDL_Resume(ConcurrencyFormat);
}

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
VDL_ERROR VDL_V2P(void *stream, unsigned int vAddr, unsigned int *pAddr)
{
  VDL *VDL_instance = ( VDL *)stream;

  if ( VDL_instance == NULL )
  {
    return VDL_ERR_NULL_STREAM_ID;
  }

  return VDL_ERR_MYSTERY;
}

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
                           unsigned short num_slice_buffers)
{
  VDL *VDL_instance = ( VDL *)stream;

  if ( VDL_instance == NULL )
  {
    return VDL_ERR_NULL_STREAM_ID;
  }

  return VDL_instance->VDL_Configure_HW(Width,Height, profile,VideoFormat,
                                        frame_buffers,num_frame_buffers,frameHeader,
                                        input, num_slice_buffers);
}

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
VDL_Slice_Pkt_Type * VDL_Get_Slice_Buffer(void *stream)
{
  VDL *VDL_instance = ( VDL *)stream;

  if ( VDL_instance == NULL )
  {
    return NULL;
  }

  return VDL_instance->VDL_Get_Slice_Buffer();

}


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
							 )
{

  VDL *VDL_instance = ( VDL *)stream;

  if ( VDL_instance == NULL )
  {
    return VDL_ERR_NULL_STREAM_ID;
  }

  return VDL_instance->VDL_Free_Slice_Buffer(pSliceBuf);  

}

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
							  )
{
  VDL *VDL_instance = ( VDL *)stream;

  if ( VDL_instance == NULL )
  {
    return VDL_ERR_NULL_STREAM_ID;
  }

  return VDL_instance->VDL_Queue_Slice_Buffer(pSliceBuf); 

}


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
VDL_PP_cmd_Info_Type * VDL_Get_PP_Pkt_Buffer(void *stream)
{
  VDL *VDL_instance = ( VDL *)stream;

  if ( VDL_instance == NULL )
  {
    return NULL;
  }

  return VDL_instance->VDL_Get_PP_Pkt_Buffer();

}

/*===========================================================================

FUNCTION:
  VDL_Free_PP_Pkt_Buffer

DESCRIPTION:
  This function calls the VDL_Free_PP_Pkt_Buffer function using the appropriate VDL instance
  
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
                             )
{
  VDL *VDL_instance = ( VDL *)stream;

  if ( VDL_instance == NULL )
  {
    return VDL_ERR_NULL_STREAM_ID;
  }

  return VDL_instance->VDL_Free_PP_Pkt_Buffer(p_PPCmdBuf);  

}

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
                              )
{
  VDL *VDL_instance = ( VDL *)stream;

  if ( VDL_instance == NULL )
  {
    return VDL_ERR_NULL_STREAM_ID;
  }

  return VDL_instance->VDL_Queue_PP_Pkt_Buffer(p_PPCmdBuf); 
}

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

							  )
{
  VDL *VDL_instance = ( VDL *)stream;

  if ( VDL_instance == NULL )
  {
    return VDL_ERR_NULL_STREAM_ID;
  }

  return VDL_instance->VDL_Queue_Stats_Buffer(pDecodeStats, retCode);

}

VDL_ERROR VDL_IsSliceBufAvailableForDecode(void *stream, 
                                           unsigned char* pBufAvailable)
{
  VDL *VDL_instance = ( VDL *)stream;

  if ( VDL_instance == NULL )
  {
    return VDL_ERR_NULL_STREAM_ID;
  }

  *pBufAvailable = VDL_instance->VDL_IsSliceBufAvailableForDecode();
  return VDL_ERR_NONE;
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
  Pointer to VDEC FRAME data for the frame 
  
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
                                         )
{
  VDL *VDL_instance = ( VDL *)stream;

  if ( VDL_instance == NULL )
  {
    return VDL_INTERFACE_NONE;
  }

  return VDL_instance->VDL_Get_Interface_Type(Height,Width,VideoFormat, profile);

}


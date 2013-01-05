/*=============================================================================
        h264_Pal.cpp
DESCRIPTION:
   This file provides necessary interface for the MPEG 4 QDSP.

  ABSTRACT:

Copyright (c) 2007 by QUALCOMM, Incorporated.  All Rights Reserved. 
QUALCOMM Proprietary and Confidential.

============================================================================*/
/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/H264Vdec/main/latest/src/PL/h264_pal.cpp#3 $
$DateTime: 2008/08/07 11:20:21 $
$Change: 718711 $

========================================================================== */

/* =======================================================================
**               Includes and Public Data Declarations
** ======================================================================= */
#include "h264_pal.h"
#include "qtv_msg.h"
//#include "VDL_RTOS.h"

#ifndef FEATURE_VDL_TEST_PC
#include "pmem.h"
#include "pmem_ids.h"
#endif

#ifndef FEATURE_H264_CONSOLE_APP
  #ifndef _CFP
extern "C" {
//#include "rex.h"
}
  #endif
#endif

extern "C" {
#include "VDL_API.h"
#include "assert.h"
}

#ifndef T_MSM7500
  #define H264_FRUC_MB_HEADER_PACKET_TYPE	(0x8203)
#else
  #define H264_FRUC_MB_HEADER_PACKET_TYPE	(0xA202)
  #define H264_FRUC_MAX_MBS_IN_FRUC_SUB_FRAME     300
  #define H264_FRUC_FRAME_HEADER_PACKET_TYPE (0xA201)
#endif

#define H264_MB_HEADER_PACKET_TYPE    (0x8202)
#define H264_FRAME_HEADER_PACKET_TYPE (0x8201)
#define H264_MB_END_MARKER            (0x7FFF)

#ifndef T_MSM7500
  #define MAX_H264_GUARD_SIZE    1800 // Based on Max 854 words
// Spec dated Oct 25, 2005
#else
  #define MAX_H264_GUARD_SIZE    1000 // ANR-TBD Recompute 
#endif

// IPCM helper members
#define H264_IPCM_NUM_LUMA   128
#define H264_IPCM_NUM_CHROMA  32


/*===========================================================================
FUNCTION:
  H264 PAL constructor

DESCRIPTION:
  Stores teh DL instance pointer passed in.
  

INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE:
  None

SIDE EFFECTS: 
==============================================================================*/
H264_PAL::H264_PAL(void *pDL)
{
  //Store the DL Pointer
  pDLPtr = pDL;
  pCurrentSliceBuf = NULL;

  //Set Default to VLD in ARM
  VLDInterfaceType = VDL_INTERFACE_ARM;
  if (pDLPtr == NULL)
  {
    QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_FATAL,
                 "DL Instance pointer NULL ");
  }
  else
  {
    //Initialize Blk Height and Width table according to MSM spec
    blk_ht_wd[0] = 0;
    blk_ht_wd[1] = 0;
    blk_ht_wd[2] = 1;
    blk_ht_wd[3] = 0;
    blk_ht_wd[4] = 2;

    QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_HIGH,"H264 PL Created Successfully");
  }
}

/*===========================================================================
FUNCTION:
  Destructor

DESCRIPTION:
  Destroys PAL instance.
  

INPUT/OUTPUT PARAMETERS:
 None

RETURN VALUE:
  PL_ERR_NONE when succesfull

SIDE EFFECTS: 
==============================================================================*/
H264_PAL::~H264_PAL()
{
  pDLPtr = NULL;
  QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_HIGH,"H264 PL Destroyed Successfully");
}

/*===========================================================================
FUNCTION:
  Initialize

DESCRIPTION:
  Initializes the DL.
  

INPUT/OUTPUT PARAMETERS:
  DecoderCB - shim layer callback to be called when frames are done
  ddUserData - user data passed in by shimlayer
  AudioFormat - Audio format

RETURN VALUE:
  PL_ERR_NONE when succesfull

SIDE EFFECTS: 
==============================================================================*/
PL_ERROR H264_PAL::Initialize(VDL_Decoder_Cb_Type   DecoderCB,        /* decode done call back function */
                              void *ddUserData,      /* user data for DecoderCB */
                              VDL_Concurrency_Type concurrencyConfig,            /* Encoded audio format */
			      int adsp_fd
                             )
{
  // Initialize DL layer
  VDL_ERROR DL_ErrorCode;
  PL_ERROR PL_ErrorCode = PL_ERR_NONE;

  //Initialize the DL layer for this instance's use
  if (pDLPtr == NULL)
  {
    QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_FATAL,
                 "DL Instance not created");
    PL_ErrorCode = PL_ERR_VDL_UNINTIALIZED;
  }
  else
  {
    //Initialize DL
    DL_ErrorCode = VDL_Initialize(pDLPtr,
                                  DecoderCB,
                                  ddUserData,
                                  VDL_VIDEO_H264,
                                  concurrencyConfig,
				  adsp_fd
                                  );
    if (DL_ErrorCode != VDL_ERR_NONE)
    {
      QTV_MSG_PRIO1(QTVDIAG_VIDEO_TASK,QTVDIAG_PRIO_FATAL,
                    "Driver Layer Initialization failed with error code %d",
                    DL_ErrorCode);
      PL_ErrorCode = PL_ERR_VDL_LAYER;
    }
    else
    {
      QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_HIGH,"H264 PL Initialized Successfully");
    }
  }
  return PL_ErrorCode;
}

/*===========================================================================
FUNCTION:
  ConfigureHW

DESCRIPTION:
  Configures the Driver layer HW
  

INPUT/OUTPUT PARAMETERS:
  height - Frame height
  width - Frame width

RETURN VALUE:
  PL_ERR_NONE when succesfull

SIDE EFFECTS: 
==============================================================================*/
PL_ERROR H264_PAL::ConfigureHW(unsigned short height,
                               unsigned short width,
                               unsigned char **frame_buffers,
                               unsigned int num_frame_buffers,
                               VDEC_SLICE_BUFFER_INFO * pSliceBufferInfo )
{
  VDL_ERROR DL_ErrorCode;
  PL_ERROR PL_ErrorCode = PL_ERR_NONE;

  //Initialize the DL layer for this instance's use
  if (pDLPtr == NULL)
  {
    QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_FATAL,
                 "DL Instance not created");
    PL_ErrorCode = PL_ERR_VDL_UNINTIALIZED;
  }
  else
  {

     switch ( num_frame_buffers )
     {    
	   case 18:
	   case 17:
	   case 16:
	   {
	     h264_frame_header.FrameBuf15Low  = (unsigned short) (((unsigned long int)frame_buffers[15]) & 0xFFFF);
		 h264_frame_header.FrameBuf15High = (unsigned short) (((unsigned long int)frame_buffers[15]) >> 16);
       }  
	   /*lint -fallthrough*/
	   case 15:
	   {
         h264_frame_header.FrameBuf14Low  = (unsigned short) (((unsigned long int)frame_buffers[14]) & 0xFFFF);
         h264_frame_header.FrameBuf14High = (unsigned short) (((unsigned long int)frame_buffers[14]) >> 16);
       }
	   /*lint -fallthrough*/
	   case 14:
	   {
	     h264_frame_header.FrameBuf13Low  = (unsigned short) (((unsigned long int)frame_buffers[13]) & 0xFFFF);
	     h264_frame_header.FrameBuf13High = (unsigned short) (((unsigned long int)frame_buffers[13]) >> 16);
	   }
	   /*lint -fallthrough*/
	   case 13:
	   {
	     h264_frame_header.FrameBuf12Low  = (unsigned short) (((unsigned long int)frame_buffers[12]) & 0xFFFF);
	     h264_frame_header.FrameBuf12High = (unsigned short) (((unsigned long int)frame_buffers[12]) >> 16);
	   }
	   /*lint -fallthrough*/
	   case 12:
	   {
		 h264_frame_header.FrameBuf11Low  = (unsigned short) (((unsigned long int)frame_buffers[11]) & 0xFFFF);
		 h264_frame_header.FrameBuf11High = (unsigned short) (((unsigned long int)frame_buffers[11]) >> 16);
	   }
	   /*lint -fallthrough*/
	   case 11:
	   {
		 h264_frame_header.FrameBuf10Low  = (unsigned short) (((unsigned long int)frame_buffers[10]) & 0xFFFF);
		 h264_frame_header.FrameBuf10High = (unsigned short) (((unsigned long int)frame_buffers[10]) >> 16);
	   }
	   /*lint -fallthrough*/
	   case 10:
	   {
		 h264_frame_header.FrameBuf9Low  = (unsigned short) (((unsigned long int)frame_buffers[9]) & 0xFFFF);
		 h264_frame_header.FrameBuf9High = (unsigned short) (((unsigned long int)frame_buffers[9]) >> 16);
	   }
	   /*lint -fallthrough*/
	   case 9:
	   {
		 h264_frame_header.FrameBuf8Low  = (unsigned short) (((unsigned long int)frame_buffers[8]) & 0xFFFF);
		 h264_frame_header.FrameBuf8High = (unsigned short) (((unsigned long int)frame_buffers[8]) >> 16);
	   }
	   /*lint -fallthrough*/
	   case 8:
	   {
		 h264_frame_header.FrameBuf7Low  = (unsigned short) (((unsigned long int)frame_buffers[7]) & 0xFFFF);
		 h264_frame_header.FrameBuf7High = (unsigned short) (((unsigned long int)frame_buffers[7]) >> 16);
	   }
	   /*lint -fallthrough*/
	   case 7:
	   {
		 h264_frame_header.FrameBuf6Low  = (unsigned short) (((unsigned long int)frame_buffers[6]) & 0xFFFF);
		 h264_frame_header.FrameBuf6High = (unsigned short) (((unsigned long int)frame_buffers[6]) >> 16);
	   }
	   /*lint -fallthrough*/
	   case 6:
	   {
		 h264_frame_header.FrameBuf5Low  = (unsigned short) (((unsigned long int)frame_buffers[5]) & 0xFFFF);
		 h264_frame_header.FrameBuf5High = (unsigned short) (((unsigned long int)frame_buffers[5]) >> 16);
	   }
	   /*lint -fallthrough*/
	   case 5:
	   {
         h264_frame_header.FrameBuf4Low  = (unsigned short) (((unsigned long int)frame_buffers[4]) & 0xFFFF);
         h264_frame_header.FrameBuf4High = (unsigned short) (((unsigned long int)frame_buffers[4]) >> 16);
	   }
	   /*lint -fallthrough*/
	   case 4:
	   {
		 h264_frame_header.FrameBuf3Low  = (unsigned short) (((unsigned long int)frame_buffers[3]) & 0xFFFF);
		 h264_frame_header.FrameBuf3High = (unsigned short) (((unsigned long int)frame_buffers[3]) >> 16);
	   }
	   /*lint -fallthrough*/
	   case 3:
	   {
		 h264_frame_header.FrameBuf2Low  = (unsigned short) (((unsigned long int)frame_buffers[2]) & 0xFFFF);
		 h264_frame_header.FrameBuf2High = (unsigned short) (((unsigned long int)frame_buffers[2]) >> 16);
	   }
	   /*lint -fallthrough*/
	   case 2:
	   {
		 h264_frame_header.FrameBuf1Low  = (unsigned short) (((unsigned long int)frame_buffers[1]) & 0xFFFF);
		 h264_frame_header.FrameBuf1High = (unsigned short) (((unsigned long int)frame_buffers[1]) >> 16);
	   }
	   /*lint -fallthrough*/
	   case 1:
	   {
		 h264_frame_header.FrameBuf0Low  = (unsigned short) (((unsigned long int)frame_buffers[0]) & 0xFFFF);
		 h264_frame_header.FrameBuf0High = (unsigned short) (((unsigned long int)frame_buffers[0]) >> 16);
		 break;
	   }
	   /*lint -fallthrough*/
	   default:
	   {
	     /* Although this code mallocs a buffer if we dont provide one, we will never
		  * use it that way so if someone does, assert 
	      */
	     ASSERT(0);
	     break;
	   }
	}

	DL_ErrorCode = VDL_Configure_HW(pDLPtr,
                                    height,
                                    width,
                                    VDL_PROFILE_SIMPLE, 
                                    VDL_VIDEO_H264, 
                                    frame_buffers, 
                                    num_frame_buffers, 
                                    (void*)&h264_frame_header,
                                    pSliceBufferInfo,
                                    8);

    if (DL_ErrorCode != VDL_ERR_NONE)
    {
      QTV_MSG_PRIO1(QTVDIAG_VIDEO_TASK,QTVDIAG_PRIO_FATAL,
                    "Driver Layer hardware config failed with error code %d",
                    DL_ErrorCode);
      PL_ErrorCode = PL_ERR_VDL_LAYER;
    }
    else
    {
      QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_HIGH,"H264 HardwareConfig Done Successfully");
    }
  }
  //Configuration successfull. Check the DSP interface type.
  VLDInterfaceType = VDL_Get_Interface_Type(pDLPtr,height,width,VDL_VIDEO_H264,VDL_PROFILE_SIMPLE);
  QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_HIGH,"H264 HardwareConfig VLD in DSP Interface");

  return PL_ErrorCode;
}

PL_ERROR H264_PAL::EOS(void* outputAddr)
{
  return PL_ERR_NONE;
}
/*===========================================================================
FUNCTION:
  Terminate

DESCRIPTION:
  Terminates the DL
  

INPUT/OUTPUT PARAMETERS:
 None

RETURN VALUE:
  PL_ERR_NONE when succesfull

SIDE EFFECTS: 
==============================================================================*/
PL_ERROR H264_PAL::Terminate()
{
  VDL_ERROR DL_ErrorCode;
  PL_ERROR PL_ErrorCode = PL_ERR_NONE;

  //Initialize the DL layer for this instance's use
  if (pDLPtr == NULL)
  {
    QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_FATAL,
                 "DL Instance not created");
    PL_ErrorCode = PL_ERR_VDL_UNINTIALIZED;
  }
  else
  {
    DL_ErrorCode = VDL_Terminate(pDLPtr);
    if (DL_ErrorCode != VDL_ERR_NONE)
    {
      QTV_MSG_PRIO1(QTVDIAG_VIDEO_TASK,QTVDIAG_PRIO_FATAL,
                    "Driver Layer Termination failed with error code %d",
                    DL_ErrorCode);
      PL_ErrorCode = PL_ERR_VDL_LAYER;
    }
    else
    {
      QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_HIGH,"H264 PL Terminated Successfully");
    }
  }
  return PL_ErrorCode;  
}

/*===========================================================================
FUNCTION:
  Flush 

DESCRIPTION:
  Flushes any frame frame obtained..
  free's the current slice buffer.
  

INPUT/OUTPUT PARAMETERS:
 None

RETURN VALUE:
  PL_ERR_NONE when succesfull

SIDE EFFECTS: 
==============================================================================*/
PL_ERROR H264_PAL::Flush()
{
  VDL_ERROR DL_ErrorCode;
  PL_ERROR PL_ErrorCode = PL_ERR_NONE;

  //Initialize the DL layer for this instance's use
  if (pDLPtr == NULL)
  {
    QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_FATAL,
                 "DL Instance not created");
    PL_ErrorCode = PL_ERR_VDL_UNINTIALIZED;
  }
  else
  {
    //Free the current slice buffer
    FreeCurrentSliceBuffer();

    DL_ErrorCode = VDL_Flush(pDLPtr);
    if (DL_ErrorCode != VDL_ERR_NONE)
    {
      QTV_MSG_PRIO1(QTVDIAG_VIDEO_TASK,QTVDIAG_PRIO_FATAL,
                    "Driver Layer Flush failed with error code %d",
                    DL_ErrorCode);
      PL_ErrorCode = PL_ERR_VDL_LAYER;
    }
    else
    {
      QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_HIGH,"H264 PL Flush Successful");
    }
  }
  return PL_ErrorCode;  
}

/*===========================================================================
FUNCTION:
  Suspend

DESCRIPTION:
  Suspends DL
  

INPUT/OUTPUT PARAMETERS:
 None

RETURN VALUE:
  PL_ERR_NONE when succesfull

SIDE EFFECTS: 
==============================================================================*/
PL_ERROR H264_PAL::Suspend()
{
  VDL_ERROR DLErrorCode;
  PL_ERROR PLErrorCode = PL_ERR_NONE;
  // Suspend decoding, terminate and disable the DSP.
  if (pDLPtr == NULL)
  {
    QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_FATAL,
                 "DL Instance not created");
    PLErrorCode = PL_ERR_VDL_UNINTIALIZED;
  }
  else
  {
    DLErrorCode = VDL_Suspend(pDLPtr);
    if (DLErrorCode != VDL_ERR_NONE)
    {
      QTV_MSG_PRIO1(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_FATAL,
                    "DL_Suspend errored with return code = %d",DLErrorCode);
      PLErrorCode = PL_ERR_VDL_LAYER;
    }
    else
    {
      QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_HIGH,"H264 PL Suspend Successful");
    }
  }
  return PLErrorCode; 
}

/*===========================================================================
FUNCTION:
  Resume

DESCRIPTION:
  Resume's operation of  DL for current instance
  

INPUT/OUTPUT PARAMETERS:
 None

RETURN VALUE:
  PL_ERR_NONE when succesfull

SIDE EFFECTS: 
==============================================================================*/
PL_ERROR H264_PAL::Resume(VDL_Concurrency_Type concurrencyConfig)
{
  VDL_ERROR DLErrorCode;
  PL_ERROR PLErrorCode = PL_ERR_NONE;
  // The DSP is about to be re-initialized, ensure that the
  // initialization-done event is clear
  if (pDLPtr == NULL)
  {
    QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_FATAL,
                 "DL Instance not created");
    PLErrorCode = PL_ERR_VDL_UNINTIALIZED;
  }
  else
  {
    DLErrorCode = VDL_Resume(pDLPtr,concurrencyConfig);
    if (DLErrorCode != VDL_ERR_NONE)
    {
      QTV_MSG_PRIO1(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_FATAL,
                    "DL_resume failed with error code = %d",DLErrorCode);
      PLErrorCode = PL_ERR_VDL_LAYER;
    }
    else
    {
      QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_HIGH,"H264 PL Resume Successful");
    }
  }
  return PLErrorCode;
}

/*===========================================================================
FUNCTION:
  FreeCurrentSliceBuffer

DESCRIPTION:
  Frees the current slice buffer in use
  

INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE:
  None

SIDE EFFECTS: 
==============================================================================*/
void H264_PAL::FreeCurrentSliceBuffer(void)
{
  if (pCurrentSliceBuf != NULL)
  {
    VDL_Free_Slice_Buffer(pDLPtr,pCurrentSliceBuf);
    pCurrentSliceBuf = NULL;
  }
  return;
}

/*===========================================================================
FUNCTION:
  QueueStatsBuffer

DESCRIPTION:
  Queue the stats buffer to DL
  

INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE:
  None

SIDE EFFECTS: 
==============================================================================*/
void H264_PAL::QueueStatsBuffer(void *pDecodeStats,unsigned char isFruc)
{
  if (isFruc)
  {
    (void)VDL_Queue_Stats_Buffer( pDLPtr,pDecodeStats,VDL_DECODE_FRAME_FRE );
  }
  else
  {
    (void)VDL_Queue_Stats_Buffer( pDLPtr,pDecodeStats,VDL_DECODE_NO_ERROR );
  }
}

/*===========================================================================
FUNCTION:
  QueueEOSStatsBuffer

DESCRIPTION:
  Queue the EOS stats buffer to DL
  

INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE:
  None

SIDE EFFECTS: 
==============================================================================*/
void H264_PAL::QueueEOSStatsBuffer(void *pDecodeStats)
{
  (void)VDL_Queue_Stats_Buffer( pDLPtr,pDecodeStats, VDL_END_OF_SEQUENCE );
}

/*===========================================================================
FUNCTION:
  IsSliceBufAvailableForDecode

DESCRIPTION:
  Query the DL layer to see if a slice buffer is available in the free slice
  buffer pool.  

INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE:
  None

SIDE EFFECTS: 
==============================================================================*/
unsigned char H264_PAL::IsSliceBufAvailableForDecode()
{
  unsigned char sliceBufAvailable = 0;
  VDL_IsSliceBufAvailableForDecode( pDLPtr, &sliceBufAvailable );
  return sliceBufAvailable;
}

/*===========================================================================
FUNCTION:
  FillMBHeader

DESCRIPTION:
  Write MB header to the DSP buffer.               

INPUT/OUTPUT PARAMETERS:
  num_mb_pkt_words : number of 16-bit words in the entire MB packet.
  isFrucMb - specifies whether this is a FRUC frame
RETURN VALUE:
  None

SIDE EFFECTS:
  MB header is written to the DSP packet buffer for the current MB.

==============================================================================*/
void H264_PAL::FillMBHeader(unsigned short num_mb_pkt_words,unsigned char isFrucMb)
{
  *slice_write_ptr ++ = num_mb_pkt_words;

  // Fill up the MB header here ...
  if (isFrucMb) *slice_write_ptr ++ = H264_FRUC_MB_HEADER_PACKET_TYPE;
  else *slice_write_ptr ++ = H264_MB_HEADER_PACKET_TYPE;

  *slice_write_ptr ++ = mb_hdr_spkt.MB_Info;
  *slice_write_ptr ++ = mb_hdr_spkt.QP_Values;
  *slice_write_ptr ++ = mb_hdr_spkt.Filter_Offsets;
}


/*===========================================================================
FUNCTION:
  FillIntraModes

DESCRIPTION:
  Writes it to the intra modes sub packet of the DSP for the current macroblock
  Block order:
     0   1   4   5             
     2   3   6   7     
     8   9  12   13               
     10  11  14   15               

INPUT/OUTPUT PARAMETERS:
  pkt : buffer that has 16 intra_4x4 modes, with boundary conditions

RETURN VALUE:
  None

SIDE EFFECTS: 
  sub packet intra modes is written for the current MB.

==============================================================================*/
void H264_PAL::FillIntraModes (unsigned short*  pkt)
{
#ifndef T_MSM7500
  // fill up the intra modes in the packet buffer (SKPT_INTRAMODES)
  *slice_write_ptr ++ = pkt[0]+(pkt[1]<<5)+(pkt[4]<<10);
  *slice_write_ptr ++ = pkt[5]+(pkt[2]<<5)+(pkt[3]<<10);
  *slice_write_ptr ++ = pkt[6]+(pkt[7]<<5)+(pkt[8]<<10);
  *slice_write_ptr ++ = pkt[9]+(pkt[12]<<5)+(pkt[13]<<10);
  *slice_write_ptr ++ = pkt[10]+(pkt[11]<<5)+(pkt[14]<<10);
  *slice_write_ptr ++ = pkt[15]; 
#else
  *slice_write_ptr ++ = pkt[5] << 12
                        | (pkt[4] & 0x0F) << 8
                        | (pkt[1] & 0x0F) << 4
                        | (pkt[0] & 0x0F);

  *slice_write_ptr ++ = pkt[7] << 12
                        | (pkt[6] & 0x0F) << 8
                        | (pkt[3] & 0x0F) << 4
                        | (pkt[2] & 0x0F);

  *slice_write_ptr ++ = pkt[13] << 12
                        | (pkt[12] & 0x0F) << 8
                        | (pkt[9] & 0x0F) << 4
                        | (pkt[8] & 0x0F);

  *slice_write_ptr ++ = pkt[15] << 12
                        | (pkt[14] & 0x0F) << 8
                        | (pkt[11] & 0x0F) << 4
                        | (pkt[10] & 0x0F);

  *slice_write_ptr ++ = ((pkt[7] >> 4) << 14)
                        | ((pkt[6] >> 4) << 12)
                        | ((pkt[3] >> 4) << 10)
                        | ((pkt[2] >> 4) << 8)
                        | ((pkt[5] >> 4) << 6)
                        | ((pkt[4] >> 4) << 4)
                        | ((pkt[1] >> 4) << 2)
                        | (pkt[0] >> 4) ;

  *slice_write_ptr ++ = ((pkt[15] >> 4) << 14)
                        | ((pkt[14] >> 4) << 12)
                        | ((pkt[11] >> 4) << 10)
                        | ((pkt[10] >> 4) << 8)
                        | ((pkt[13] >> 4) << 6)
                        | ((pkt[12] >> 4) << 4)
                        | ((pkt[9] >> 4) << 2)
                        | (pkt[8] >> 4) ;
#endif
}


/*===========================================================================
FUNCTION:
  FillMotionVectors

DESCRIPTION:
  Fills the reconstructed motion vector for each block into the Motion Vector
  sub packet and also puts in additional parameters as described below.
          ** Blk_config bit 15:14 = 0 (reserved)
          ** Blk_config bit    13 = B-VOP indicator
          ** Blk_config bit    12 = Luma indicator
          ** Blk_config bit  11:8 = Ref_idx
          ** Blk_config bit   7:6 = Vertical_pos
          ** Blk_config bit   5:4 = Horizontal_pos
          ** Blk_config bit   3:2 = Block_height
          ** Blk_config bit   1:0 = Block_width
          **
INPUT/OUTPUT PARAMETERS:
  mv      : pointer to MV_X and MV_Y for the given block (reconstructed)
  ref_idx : reference frame index for the MV
  x_pos   : Horizontal postion of the block
  y_pos   : Vertical postion of the block
  width   : width in pixels of MB
  height  : height in pixels of MB

RETURN VALUE:
  None

SIDE EFFECTS:
  sub packet intra modes is written for the current MB.

==============================================================================*/
void H264_PAL::FillMotionVectors (signed short    mv_x, 
                                  signed short mv_y,
                                  unsigned short   ref_idx,
                                  unsigned short   x_pos, 
                                  unsigned short   y_pos,
                                  unsigned short   width, 
                                  unsigned short   height,
                                  unsigned char   *h264_map_ref_tab_L0
#ifdef FEATURE_H264_B_FRAMES
                                  ,unsigned char   *h264_map_ref_tab_L1
                                  ,unsigned char    b_vop_indicator
                                  ,unsigned char    listidx
#endif
                                 )
{
#ifndef T_MSM7500
  /*write the motion vectors */
  *slice_write_ptr ++ = mv_x;
  *slice_write_ptr ++ = mv_y;

#ifdef FEATURE_H264_B_FRAMES
  if (listidx == 0)
#endif
  {
    /* Construct the block cfg word below. */
    *slice_write_ptr ++ = h264_map_ref_tab_L0[ref_idx] << 8 
                          | y_pos << 6 
                          | x_pos << 4 
                          | blk_ht_wd[height] << 2 
                          | blk_ht_wd[width] 
                          | 1 << 12;
  }
#ifdef FEATURE_H264_B_FRAMES
  else
  {
    /* Construct the block cfg word below. */
    *slice_write_ptr ++ = h264_map_ref_tab_L1[ref_idx] << 8 
                          | y_pos << 6 
                          | x_pos << 4 
                          | blk_ht_wd[height] << 2 
                          | blk_ht_wd[width] 
                          | 1 << 12
                          | (b_vop_indicator ? (1 << 13) : 0);
  }
#endif

#else
  /* 7500 needes motion vectors in Q3 format */

  *slice_write_ptr++ = mv_x << 1;
  *slice_write_ptr++ = mv_y << 1;

#ifdef FEATURE_H264_B_FRAMES
  if (listidx == 0)
#endif   
  {
    /* Construct the block cfg word below. */
    *slice_write_ptr++ = h264_map_ref_tab_L0[ref_idx] << 8 
                         | x_pos << 6 
                         | blk_ht_wd[width]<< 4 
                         | y_pos << 2 
                         | blk_ht_wd[height] 
                         | 1 << 12;
  }
#ifdef FEATURE_H264_B_FRAMES
  else
  {
    /* Construct the block cfg word below. */
    *slice_write_ptr++ = h264_map_ref_tab_L1[ref_idx] << 8 
                         | x_pos << 6 
                         | blk_ht_wd[width]<< 4 
                         | y_pos << 2 
                         | blk_ht_wd[height] 
                         | 1 << 12
                         | (b_vop_indicator ? (1 << 13) : 0);
  }
#endif /* FEATURE_H264_B_FRAMES */
#endif /* T_MSM7500 */

}

/*===========================================================================
FUNCTION:
  FillSubPacketsToDSPSliceBuffer

DESCRIPTION:
  This function is called from the H.264 library decode MB function and is 
  passed a H264QDSP_MBPacketType which contains decoded sub packets. This 
  function is reponsible for filling a slice buffer if it already exists or
  getting a new slice buffer if it does not or if the packet does not fit 
  in the existing slice buffer.

INPUT/OUTPUT PARAMETERS:
  *pkt    : Source of data (ARM VLC decoded) to be used to pack the slice buffer

RETURN VALUE:
  None

SIDE EFFECTS: 
  Packet ready for DSP consumption is generated and is ready to be queued

==============================================================================*/
PL_ERROR H264_PAL::FillSubPacketsToDSPSliceBuffer( unsigned short mb_cnt,
                                                   unsigned short num_mbs,
                                                   unsigned char isFruc,
                                                   unsigned short frameSizeInMbs)
{
  unsigned short numBytesWritten;
  unsigned short mb_pkt_words;

  mb_pkt_words = slice_write_ptr - slice_cur_mb_start;
  numBytesWritten = mb_pkt_words << 1;


  if (mb_cnt == 0) // New frame use a new slice buffer
  {
    pCurrentSliceBuf->fFirstMB = 1;
    pCurrentSliceBuf->fAllocateYUVBuf = 1;    
    pCurrentSliceBuf->SliceDataSize = sizeof( h264_frame_header );

    /* Also, according to the new interface, we don't count the frame    */
    /* header as a MacroBlock (in pSliceBuf->NumMacroBlocks) unlike the  */     
    /* old interface                                                     */
  }
  else if ( mb_cnt >= num_mbs )
  {
    /* Basically indicates that we got more bits in the rbsp than is required
    ** to decode a frame ... for example in case of QVGA we already got 300 mb 
    ** worth of data and then some (which might be because of possible error 
    ** conditon) so ignore these extra bits
    */
    QTV_MSG_PRIO1(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_HIGH,"Past MB boundary MB count %d", mb_cnt);
    return PL_ERR_NONE;
  }

  // YB should allow packet size larger than 64K
  pCurrentSliceBuf->SliceDataSize += numBytesWritten;
  pCurrentSliceBuf->NumMacroBlocks++;


  // Set the cur MB ptr for the next MB
  NoteCurrentMBStart( slice_cur_mb_start + mb_pkt_words );

  if (mb_cnt == (num_mbs -1))
  {
    pCurrentSliceBuf->fLastMB = 1;

    //Assign seq num for the slice buf being queued
    //Seq Num can range from 0-4095, since the seq num field in subframe packet is 12 bits
#ifndef T_MSM7500
    //pCurrentSliceBuf->SubframePktSeqNum = (H264SubframepktSeqnum++ % 4096); 
#else
    //pCurrentSliceBuf->SubframePktSeqNum = h264_cur_frame_num << 12 | (H264SubframepktSeqnum++ % 4096);

    if (!isFruc)
    {
      pCurrentSliceBuf->CodecType =  VDL_CODEC_H264; 
    }
    else
    {
      pCurrentSliceBuf->CodecType = VDL_CODEC_FRUC; 
      pCurrentSliceBuf->NumMacroBlocks = frameSizeInMbs;
    }

#endif


    /* We are about to queue the dequeued slice buffer onto the mp4_slice_q.
    ** Set pDequeuedSlice to NULL to indicate that there is no dequeued slice
    ** buffer to free.
    */
    /* Queue the slice to the graph task and dont signal it to be picked up
    ** yet
    */
    (void)VDL_Queue_Slice_Buffer(pDLPtr,pCurrentSliceBuf);

    pCurrentSliceBuf = NULL;
  }
  else if ((pCurrentSliceBuf->SliceBufferSize - (pCurrentSliceBuf->SliceDataSize)) < MAX_H264_GUARD_SIZE)
  {
    pCurrentSliceBuf->fLastMB = 0;

    //Fill the seq num for the slice buf being queued
    //Seq Num can range from 0-4095, since the seq num field in subframe packet is 12 bits
#ifndef T_MSM7500
    //pCurrentSliceBuf->SubframePktSeqNum = (H264SubframepktSeqnum++ % 4096); 
#else
    //pCurrentSliceBuf->SubframePktSeqNum = h264_cur_frame_num << 12 | (H264SubframepktSeqnum++ % 4096);
    if (!isFruc)
    {
      pCurrentSliceBuf->CodecType = VDL_CODEC_H264;
    }
    else
    {
      pCurrentSliceBuf->CodecType = VDL_CODEC_FRUC;
      pCurrentSliceBuf->NumMacroBlocks = frameSizeInMbs;
    }
#endif


    /* We are about to queue the dequeued slice buffer onto the mp4_slice_q.
    ** Set pDequeuedSlice to NULL to indicate that there is no dequeued slice
    ** buffer to free.
    */
    /* Queue the slice to the graph task and dont signal it to be picked up
    ** yet
    */
    (void)VDL_Queue_Slice_Buffer(pDLPtr,pCurrentSliceBuf);
    pCurrentSliceBuf = NULL;

    //and get another slice buffer and point to slice data 
    slice_cur_mb_start = GetNewSliceBufferDataPtr();
    if (slice_cur_mb_start == NULL) return PL_ERR_OUT_OF_MEMORY;

    pCurrentSliceBuf->fFirstMB = 0;
    pCurrentSliceBuf->fAllocateYUVBuf = 0;
  }
  //else wait for next MB to pack into the slice buffer and keep packing 
  //until slice buffer is full before sending it over
  return PL_ERR_NONE;
}


/*===========================================================================
FUNCTION:
  GetNewSliceBufferDataPtr

DESCRIPTION:
  This function is called to get the pointer to a new slice data buffer, so that
  the decoded MB information can be written to it. 

INPUT/OUTPUT PARAMETERS:
  Ptr to Slice Data

RETURN VALUE:
  None

SIDE EFFECTS: 
  Note: Also sets the pCurrentSliceBuf which is of static scope to this file
==============================================================================*/
unsigned short* H264_PAL::GetNewSliceBufferDataPtr(void)
{
  pCurrentSliceBuf = VDL_Get_Slice_Buffer(pDLPtr);

  if (pCurrentSliceBuf == NULL)
  {
    QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_FATAL, "Out of memory");
    return NULL;
  }
  return(unsigned short*) pCurrentSliceBuf->pSliceData;
}


/*===========================================================================
FUNCTION:     AddFrameHeader

DESCRIPTION:  
  This fn MUST be called up every frame before adding MBs. 
  It does the following to setup for MB packing into slicebuf
    1. Get a new slice buffer from the MP4 layer
    2. Fills up the frame header
    3. Increments the slice buffer pointer just beyond the frame header, 
       ready for filling the MBs

RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
PL_ERROR H264_PAL::AddFrameHeader()
{
  unsigned short numBytes;

  /*-------------------------------------------------------------------------
    Get a new slice buffer pointer
  -------------------------------------------------------------------------*/
  slice_write_ptr = GetNewSliceBufferDataPtr();
  if (slice_write_ptr == NULL) return PL_ERR_OUT_OF_MEMORY;

  /*-------------------------------------------------------------------------
    Copy the entire frame header into the slice buf
  -------------------------------------------------------------------------*/
  numBytes = sizeof( H264PL_FrameHeaderPacketType );
  memcpy (slice_write_ptr, &h264_frame_header, numBytes);
  slice_write_ptr += numBytes >> 1;

  //Note the pointer to the start of cur MB in the packet buffer
  NoteCurrentMBStart(slice_write_ptr);
  return PL_ERR_NONE;
}

/*===========================================================================
FUNCTION:     
  InitIPCMMb

DESCRIPTION:  
  This function initializes the IPCM Luma, Cb and Cr pointers, so that the 
  IPCM data can be filled
  Note: This function MUST be called before calling the FillIPCMPkt fn

INPUT/OUTPUT PARAMETERS:
  None
  
RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
void H264_PAL::InitIPCMMb(void)
{
  pH264IPCMLuma = slice_write_ptr;
  pH264IPCMCr   = slice_write_ptr + H264_IPCM_NUM_LUMA;
  pH264IPCMCb   = pH264IPCMCr + H264_IPCM_NUM_CHROMA;

  slice_write_ptr += (H264_IPCM_NUM_LUMA + 2*H264_IPCM_NUM_CHROMA);
}

/*===========================================================================
FUNCTION:     
  FillIPCMPkt

DESCRIPTION:  
  Given an IPCM Word pointer, this fn packs the hi and lo pixels. Once done, 
  it increments the IPCM ptr to point to the next position. 
  Note: This function is an internal function to be used by the 
        H264_IPCM_FILL_* macros only. Callers should use these macros to 
        fill the IPCM data

INPUT/OUTPUT PARAMETERS:
  ppIPCMWord: Ptr to slice buffer location where IPCM data is to be filled
  loPix     : IPCM Lo data to be filled in slice buffer
  hiPix     : IPCM Hi data to be filled in slice buffer

RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
void H264_PAL::FillIPCMPkt(H264PL_COLORINFO Ptr2assign, 
                           unsigned short index, 
                           unsigned short value)
{
  switch (Ptr2assign)
  {
  case LUMA:
    pH264IPCMLuma[index] = value;
    break;
  case CHROMA_CB:
    pH264IPCMCb[index] = value;
    break;
  case CHROMA_CR:
    pH264IPCMCr[index] = value;
    break;
  default:
    QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK, QTVDIAG_PRIO_HIGH,"Not supported");
  }
}

/*---------------------------------------------------------------------------
  Old style 7500 helpers
---------------------------------------------------------------------------*/
#ifdef T_MSM7500
  #ifdef REVERSED_ORDER
    #define RUN_LVL_STEP  2
  #else
    #define RUN_LVL_STEP -2
  #endif

signed short h264_run_lvl_temp_array[16];
short H264RunLevels[32], *pRunlevels;
signed short coeffs[16];

/*===========================================================================
FUNCTION:     
  InitACResid

DESCRIPTION:  
  This function initializes the Num AC residuals inthe slice buffer. It also
  sets the slice buffer and runlevels pointers based to the num  coeffs, so 
  the packing can be done accordingly later, using FillACResid and 
  completed through DoneACResid. This function MUST be called before
  invoking th eother two functiones mentioned.
  
INPUT/OUTPUT PARAMETERS:
  numCoeff: Number of coefficients for the current MB. This should be set 
            to a nonzero value if there are any ac residuals

RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
void H264_PAL::InitACResid(unsigned short numCoeff)
{
  if (numCoeff > 8)
  {
    pRunlevels = &H264RunLevels[0];
    pH264NumCoeff = slice_write_ptr ++;
    *pH264NumCoeff = (unsigned short) 0xFFFF;

#ifdef REVERSED_ORDER
    pH264Coeff = pRunlevels;
    pRunlevels += 2 * numCoeff;
#else
    pRunlevels += 2 * numCoeff;
    pH264Coeff = (unsigned short*)(pRunlevels - 2);
#endif
  }
  else
  {
    pH264NumCoeff = slice_write_ptr ++;
    *pH264NumCoeff = (unsigned short) numCoeff;

#ifdef REVERSED_ORDER
    pH264Coeff = slice_write_ptr;
    slice_write_ptr += 2 * numCoeff;
#else
    slice_write_ptr += 2 * numCoeff;
    pH264Coeff = (unsigned short*)(slice_write_ptr - 2);
#endif
  }
}

/*===========================================================================
FUNCTION:     
  FillACResid

DESCRIPTION:  
  This function packs the next set of runs and levels in the slice buffer
  
INPUT/OUTPUT PARAMETERS:
  run: The next run value to be packed in slice buffer
  lvl: The next level value to be packed in slice buffer
  
RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
void H264_PAL::FillACResid(unsigned short run, unsigned short lvl)
{
  pH264Coeff[0] = run;
  pH264Coeff[1] = lvl;
  pH264Coeff += RUN_LVL_STEP;
}

/*===========================================================================
FUNCTION:     
  DoneACResid

DESCRIPTION:  
  This function packs the run lvl data into the slice buffer in case where 
  the data was filled in a temporary buffer (Currently this is done when 
  there are more than 8 residuals)
  
INPUT/OUTPUT PARAMETERS:
  numCoeff: The number of coefficients to be packed in the slice buffer
  
RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
void H264_PAL::DoneACResid(unsigned short numCoeff)
{
  if (numCoeff > 8)
  {
    signed short j,k;
    pRunlevels = (short*)slice_write_ptr;

    for (k=0;k<16;k++)
      *slice_write_ptr ++ = 0;

    /* do Run Level expansion */
    for ( j=0; j<2*numCoeff; j+=2)
    {
      //RUN
      for (k=0;k<H264RunLevels[j];k++)
        *pRunlevels++ = 0;
      //LEVEL
      *pRunlevels++ = H264RunLevels[j+1];
    }
  }
}

/*===========================================================================
FUNCTION:     
  InitDCResid

DESCRIPTION:  
  This function initializes the slice buffer for DC residuals. It guarentees 
  a minimum packing of 4 words for dc residuals ven if there are no residuals
  
INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
void H264_PAL::InitDCResid(void)
{
  // For DC, write 4 zero coefficients at first. This may be over written if 
  // needed
  slice_write_ptr[0] = 0;
  slice_write_ptr[1] = 0;
  slice_write_ptr[2] = 0;
  slice_write_ptr[3] = 0;
  slice_write_ptr += 4;
}

/*===========================================================================
FUNCTION:     
  InitDCCoeff

DESCRIPTION:  
  This function preps the slice buffer ptr to handle nonzero DC residuals. 
  It rewinds the slice buffer pointer (moved forward through InitDCResid)
  such that the non-zero residuals can be packed.
  
INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE: 
  numCoeff: Number of coefficients for the current MB. This should be set 
            to a nonzero value if there are any ac residuals
  zerosLeft: Number of zeros left

SIDE EFFECTS: 
  None. 
===========================================================================*/
void H264_PAL::InitDCCoeff(unsigned short numCoeff, unsigned short zerosLeft)
{
  pH264Coeff = (unsigned short*)(slice_write_ptr - 4 + zerosLeft + numCoeff - 1);
}

/*===========================================================================
FUNCTION:     
  FillDCResid

DESCRIPTION:  
  This function packs the next dc coeff in the slice buffer. It also moves
  the slice packing pointer accordingly to pack the next residual
  
INPUT/OUTPUT PARAMETERS:
  coeff: The next coefficient value to be packed in slice buffer
  numzeros: The number of zeros remaining
  
RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
void H264_PAL::FillDCResid(unsigned short coeff, unsigned short numzeros)
{
  *pH264Coeff = coeff;
  pH264Coeff -= numzeros + 1;
}
#endif // T_MSM7500

/*===========================================================================
FUNCTION:     
  FillFrameHeaderConstants

DESCRIPTION:  
  Fills the default frame header details
  
INPUT/OUTPUT PARAMETERS:
  FrameBuffAddr - buffer buffer array address
  num_frame_buffers - number of frame buffers
  
RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
void H264_PAL::FillFrameHeaderConstants(unsigned char** FrameBuffAddr,unsigned int num_frame_buffers)
{
  h264_frame_header.PacketId  = H264_FRAME_HEADER_PACKET_TYPE;
  if ( FrameBuffAddr )
  {
    memset(&(h264_frame_header.FrameBuf0High), 64, 0);
    switch ( num_frame_buffers )
    {
    case 18:
    case 17:
    case 16:
      {
        h264_frame_header.FrameBuf15Low  = (unsigned short) ((unsigned long int)FrameBuffAddr[15] & 0xFFFF);
        h264_frame_header.FrameBuf15High = (unsigned short) ((unsigned long int)FrameBuffAddr[15] >> 16);
      }
      /*lint -fallthrough*/
    case 15:
      {
        h264_frame_header.FrameBuf14Low  = (unsigned short) ((unsigned long int)FrameBuffAddr[14] & 0xFFFF);
        h264_frame_header.FrameBuf14High = (unsigned short) ((unsigned long int)FrameBuffAddr[14] >> 16);
      }
      /*lint -fallthrough*/
    case 14:
      {
        h264_frame_header.FrameBuf13Low  = (unsigned short) ((unsigned long int)FrameBuffAddr[13] & 0xFFFF);
        h264_frame_header.FrameBuf13High = (unsigned short) ((unsigned long int)FrameBuffAddr[13] >> 16);
      }
      /*lint -fallthrough*/
    case 13:
      {
        h264_frame_header.FrameBuf12Low  = (unsigned short) ((unsigned long int)FrameBuffAddr[12] & 0xFFFF);
        h264_frame_header.FrameBuf12High = (unsigned short) ((unsigned long int)FrameBuffAddr[12] >> 16);
      }
      /*lint -fallthrough*/
    case 12:
      {
        h264_frame_header.FrameBuf11Low  = (unsigned short) ((unsigned long int)FrameBuffAddr[11] & 0xFFFF);
        h264_frame_header.FrameBuf11High = (unsigned short) ((unsigned long int)FrameBuffAddr[11] >> 16);
      }
      /*lint -fallthrough*/
    case 11:
      {
        h264_frame_header.FrameBuf10Low  = (unsigned short) ((unsigned long int)FrameBuffAddr[10] & 0xFFFF);
        h264_frame_header.FrameBuf10High = (unsigned short) ((unsigned long int)FrameBuffAddr[10] >> 16);
      }
      /*lint -fallthrough*/
    case 10:
      {
        h264_frame_header.FrameBuf9Low  = (unsigned short) ((unsigned long int)FrameBuffAddr[9] & 0xFFFF);
        h264_frame_header.FrameBuf9High = (unsigned short) ((unsigned long int)FrameBuffAddr[9] >> 16);
      }
      /*lint -fallthrough*/
    case 9:
      {
        h264_frame_header.FrameBuf8Low  = (unsigned short) ((unsigned long int)FrameBuffAddr[8] & 0xFFFF);
        h264_frame_header.FrameBuf8High = (unsigned short) ((unsigned long int)FrameBuffAddr[8] >> 16);
      }
      /*lint -fallthrough*/
    case 8:
      {
        h264_frame_header.FrameBuf7Low  = (unsigned short) ((unsigned long int)FrameBuffAddr[7] & 0xFFFF);
        h264_frame_header.FrameBuf7High = (unsigned short) ((unsigned long int)FrameBuffAddr[7] >> 16);
      }
      /*lint -fallthrough*/
    case 7:
      {
        h264_frame_header.FrameBuf6Low  = (unsigned short) ((unsigned long int)FrameBuffAddr[6] & 0xFFFF);
        h264_frame_header.FrameBuf6High = (unsigned short) ((unsigned long int)FrameBuffAddr[6] >> 16);
      }
      /*lint -fallthrough*/
    case 6:
      {
        h264_frame_header.FrameBuf5Low  = (unsigned short) ((unsigned long int)FrameBuffAddr[5] & 0xFFFF);
        h264_frame_header.FrameBuf5High = (unsigned short) ((unsigned long int)FrameBuffAddr[5] >> 16);
      }
      /*lint -fallthrough*/
    case 5:
      {
        h264_frame_header.FrameBuf4Low  = (unsigned short) ((unsigned long int)FrameBuffAddr[4] & 0xFFFF);
        h264_frame_header.FrameBuf4High = (unsigned short) ((unsigned long int)FrameBuffAddr[4] >> 16);
      }
      /*lint -fallthrough*/
    case 4:
      {
        h264_frame_header.FrameBuf3Low  = (unsigned short) ((unsigned long int)FrameBuffAddr[3] & 0xFFFF);
        h264_frame_header.FrameBuf3High = (unsigned short) ((unsigned long int)FrameBuffAddr[3] >> 16);
      }
      /*lint -fallthrough*/
    case 3:
      {
        h264_frame_header.FrameBuf2Low  = (unsigned short) ((unsigned long int)FrameBuffAddr[2] & 0xFFFF);
        h264_frame_header.FrameBuf2High = (unsigned short) ((unsigned long int)FrameBuffAddr[2] >> 16);
      }
      /*lint -fallthrough*/
    case 2:
      {
        h264_frame_header.FrameBuf1Low  = (unsigned short) ((unsigned long int)FrameBuffAddr[1] & 0xFFFF);
        h264_frame_header.FrameBuf1High = (unsigned short) ((unsigned long int)FrameBuffAddr[1] >> 16);
      }
      /*lint -fallthrough*/
    case 1:
      {
        h264_frame_header.FrameBuf0Low  = (unsigned short) ((unsigned long int)FrameBuffAddr[0] & 0xFFFF);
        h264_frame_header.FrameBuf0High = (unsigned short) ((unsigned long int)FrameBuffAddr[0] >> 16);
        break;
      }
      /*lint -fallthrough*/
    default:
      {
        /* Although this code mallocs a buffer if we dont provide one, we will never
        ** use it that way so if someone does, assert 
        */
        ASSERT(0);
      }
    }

  }

  h264_frame_header.EndMarker = H264_MB_END_MARKER;
}

/*===========================================================================
FUNCTION:     
  UpdateFrameHeader

DESCRIPTION:  
  updates the frame header
  
INPUT/OUTPUT PARAMETERS:
  X_Dimension - x dimesion of frame
  Y_Dimension - y dimesion of frame
  FrameInfo_0 
  FrameInfo_1
  Output_frame_buffer - output frame buffer address
  iSFrucFrame - specifies whether this is fruc frame

RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
void H264_PAL::UpdateFrameHeader(unsigned short X_Dimension,unsigned short Y_Dimension,
                                 unsigned short FrameInfo_0,unsigned short FrameInfo_1,
                                 unsigned char* Output_frame_buffer,unsigned char iSFrucFrame,
				 unsigned char** ref_frames_arr)
{
  h264_frame_header.X_Dimension = X_Dimension;
  h264_frame_header.Y_Dimension = Y_Dimension;
  h264_frame_header.FrameInfo_0  = FrameInfo_0;
  h264_frame_header.FrameInfo_1  = FrameInfo_1;
  h264_frame_header.PacketId = H264_FRAME_HEADER_PACKET_TYPE;
#ifdef T_MSM7500
  if (iSFrucFrame)
    h264_frame_header.PacketId     = H264_FRUC_FRAME_HEADER_PACKET_TYPE;
  /* Line width for 7500  should be multiple of 32  of X_Dimension 
     in case video hardware cache is turned on otherwise it is just X_Dimension.
  */
  h264_frame_header.LineWidth     = h264_frame_header.X_Dimension;
#endif
  //Fill the output frame buffer address information
  #ifdef FEATURE_VDL_TEST_PC
  h264_frame_header.OutputFrameBufHigh = (unsigned long int)Output_frame_buffer >> 16;
  h264_frame_header.OutputFrameBufLow  = (unsigned long int)Output_frame_buffer & 0xFFFF;
  #else
  VDL_ERROR err;
  unsigned int pAddr = (unsigned long int)Output_frame_buffer;
  //unsigned int vAddr = (unsigned long int)Output_frame_buffer;

  //VDL_V2P(pDLPtr, vAddr, &pAddr);

  h264_frame_header.OutputFrameBufHigh = (unsigned long int)pAddr >> 16;
  h264_frame_header.OutputFrameBufLow  = (unsigned long int)pAddr & 0xFFFF; 
  #endif
}

/*===========================================================================
FUNCTION:     
  UpdateMBQP_Values

DESCRIPTION:  
  updates QP values for the MB
  
INPUT/OUTPUT PARAMETERS:
  Value - QP value

RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
void H264_PAL::UpdateMBQP_Values(unsigned short Value)
{
  mb_hdr_spkt.QP_Values = Value;
}

/*===========================================================================
FUNCTION:     
  UpdateMBFilter_Offsets

DESCRIPTION:  
  updates Filter_Offsets values for the MB
  
INPUT/OUTPUT PARAMETERS:
  Value - Filter_Offsets

RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
void H264_PAL::UpdateMBFilter_Offsets(unsigned short Value)
{
  mb_hdr_spkt.Filter_Offsets = Value;
}

/*===========================================================================
FUNCTION:
  UpdateMbInfo

DESCRIPTION:
  updates mb_info values for the MB
  
INPUT/OUTPUT PARAMETERS:
  Value - mb_info

RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
void H264_PAL::UpdateMbInfo(unsigned short Value)
{
  mb_hdr_spkt.MB_Info = Value;
}

/*===========================================================================
FUNCTION:     
  FillEndMarker
  
DESCRIPTION:  
  Fills the end marker of the frame

INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE:
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
void H264_PAL::FillEndMarker()
{
  *slice_write_ptr ++ = H264_MB_END_MARKER;
}

#ifdef FEATURE_ARM_ONLY
/*===========================================================================
FUNCTION:     
  FillRunLevelPair

DESCRIPTION:  
  Fills run/level pairs
  
INPUT/OUTPUT PARAMETERS:
  total_coeff - total coefficients
  runlevels - run level array

RETURN VALUE: 
  None

SIDE EFFECTS: 
  None. 
===========================================================================*/
void H264_PAL::FillRunLevelPair(unsigned int total_coeff,short *runlevels)
{

  //Incremented during intialization...so decrement it..
  slice_write_ptr -=4;
  unsigned short *ptr;
  ptr = slice_write_ptr;
  int i,j;
  for ( j=0; j<2*total_coeff; j+=2)
  {
    //RUN
    for (k=0;k<runlevels[j];k++)
      *ptr++ = 0;

    //LEVEL
    *ptr++ = runlevels[j+1];
  }
}
#endif

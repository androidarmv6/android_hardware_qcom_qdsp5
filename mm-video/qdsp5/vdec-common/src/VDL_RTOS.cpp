/* =======================================================================
                               VDL_RTOS.cpp
DESCRIPTION
   This file has the RTOS implementation for VDL

========================================================================== */

/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/QDSP5VdecDriver/main/latest/src/VDL_RTOS.cpp#18 $
$DateTime: 2009/03/20 16:05:25 $
$Change: 868607 $

========================================================================== */

/*==========================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */
//#include "h264_TL.h"
#include "qtv_msg.h"
#include "qtvsystem.h"
#include "VDL_RTOS.h"
#include "adsprtossvc.h"
#include "TL_types.h"
extern "C"
{
#include "qutility.h"
}
#ifndef T_WINNT
    #include "assert.h"    // for ASSERT
    #ifdef _ANDROID_
      #include "cutils/log.h"
    #endif
    #include <sys/time.h>
#else
    #include <assert.h>
#endif //T_WINNT

#include <errno.h>
#include <pthread.h>

/* Local defintion for boolean variables */
#ifdef TRUE
    #undef TRUE
#endif
#ifdef FALSE
    #undef FALSE
#endif
#define TRUE   1   /* Boolean true value. */
#define FALSE  0   /* Boolean false value. */

#define VDL_QDSP_SUBFRAME_INFO0(first)  (first)
#define VDL_QDSP_SUBFRAME_INFO1(Stats_DM_Exchange, Pkt_DM_exchange)  ((Stats_DM_Exchange<<1) | (Pkt_DM_exchange))

#define QDSP_mpuVDecPktQueue    5
#define VDEC_INPUT_SIZE (128 * 1024 * 2)
VDL_RTOS_QDSP_InfoType         VDL_QDSP_Info;

#ifdef PROFILE_DECODER
extern unsigned int qperf_total_frame_cnt; 
#endif
QPERF_INIT(dsp_decode);

//extern unsigned int VDL_RTOS::VDL_Cur_QDSP_Sub_Frame_Queue_Depth=0;
/*===========================================================================

FUNCTION:
  VDL VDL_RTOS constructor

DESCRIPTION:
  This function initializes the necessary variables and calls the function to
  save the stream ID to instance mapping table

DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  Pointer to VDL_ERROR type
  
RETURN VALUE:
  None

SIDE EFFECTS:
  None

===========================================================================*/
VDL_RTOS::VDL_RTOS
(
  VDL_ERROR * const pErr_out
)
{
  m_state = IDLE;
  BufferQueueRunningCount = 0;
  streamID = 0;
  QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_MED, "VDL RTOS_DSP_VLD instance created!");
  VDL_DSP_VLD_Slice_Buffer_Size = VDEC_INPUT_SIZE;
  VDL_Cur_QDSP_Sub_Frame_Queue_Depth = 0;
  qdsp_module = QDSP_MODULE_VIDEOTASK;
#ifdef LOG_YUV_SLICES
  struct timeval time_open={0};
  int hash_number = 0;
  char yuv_filename[50];
  if(!gettimeofday(&time_open,NULL))
  {
    hash_number = time_open.tv_usec % 10000;
  }
  sprintf(yuv_filename, "/sdcard/slice_buffer%d.bin", hash_number);
#ifdef T_WINNT
  pSliceFile = fopen("../../debug/slice_buffer.bin", "wb");
#elif _ANDROID_
  pSliceFile = fopen(yuv_filename, "wb");
#else
  pSliceFile = fopen("slice_buffer.bin", "wb");
#endif
  if(!pSliceFile)
  {
    QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_ERROR, 
                  "Cannot start slice_buffer logging %s - fopen failed!\n", yuv_filename);
  }
  else
  {
    QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_ERROR, 
                  "Creating slice buffer file %s\n", yuv_filename);
  }
#endif
   //QPERF_RESET(dsp_decode);

  QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED, "[vdl] init mutexes\n");
  int result1 = pthread_mutex_init(&lockSendSlice,NULL);

  /* Required to handle concurrency during flush operation */
  int result2 = pthread_mutex_init(&lockFlushProcessing,NULL);
  int result3 = pthread_cond_init(&m_all_frame_done_cond, 0);
  int result4 = pthread_mutex_init(&m_pending_frame_done_lock, 0);
  int result5 = pthread_mutex_init(&lockEOSProcessing,NULL);


  if(result1 || 
     result2 || result3 || result4) 
  {
    QTV_MSG_PRIO1(QTVDIAG_GENERAL, QTVDIAG_PRIO_MED, 
                 "VDL RTOS_DSP_VLD err=%d mutex_init()..!!",result1);
    *pErr_out = VDL_ERR_MYSTERY;
  }

  /*Initialize VDL Statistics*/
  memset(&VDL_Statistics, 0, sizeof(VDL_Statistics_Type));

  m_pending_frame_dones = 0;
  VideoFormatType = VDL_VIDEO_NONE;

  QPERF_RESET(dsp_decode);
}

void VDL_RTOS::SetState(VDL_state_type state)
{
  m_state = state;
#ifdef LOG_YUV_SLICES
  if(pSliceFile && (m_state == ERROR))
  {
    fclose(pSliceFile);
  }
#endif
}

/*===========================================================================

FUNCTION:
  VDL VDL_RTOS destructor

DESCRIPTION:
  This function initializes the necessary variables and calls the function to
  save the stream ID to instance mapping table

DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  Pointer to VDL_ERROR type
  
RETURN VALUE:
  None

SIDE EFFECTS:
  None

===========================================================================*/
VDL_RTOS::~VDL_RTOS
(

)
{
#ifdef PROFILE_DECODER
  usecs_t time_taken_by_dsp = QPERF_TERMINATE(dsp_decode);
  float avDspTime = (float)time_taken_by_dsp/(qperf_total_frame_cnt*1000);
  QTV_PERF_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_FATAL,"===========================================================\n");
  QTV_PERF_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_FATAL,"                      Dsp Statistics                       \n");
  QTV_PERF_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_FATAL,"===========================================================\n");
  QTV_PERF_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_FATAL,"Total number of frames decoded = %ld\n",qperf_total_frame_cnt);
  QTV_PERF_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_FATAL,"Average Dsp time/frame(ms)     = %f\n",avDspTime);
  QTV_PERF_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_FATAL,"Frames Dsp Decoded/sec)        = %f\n",1000/avDspTime);
  QTV_PERF_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_FATAL,"===========================================================\n");
#endif 
  QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_MED, "VDL RTOS_DSP_VLD instance destructed!");

  (void) pthread_mutex_destroy(&lockSendSlice);
  (void) pthread_mutex_destroy(&lockFlushProcessing);
  (void) pthread_cond_destroy(&m_all_frame_done_cond);
  (void) pthread_mutex_destroy(&m_pending_frame_done_lock);
  (void) pthread_mutex_destroy(&lockEOSProcessing);
}

/*===========================================================================

FUNCTION:
  VDL_Process_Send_Video_Slice

DESCRIPTION:
  Send slices upto one frame to the DSP

DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE:
  None.

SIDE EFFECTS:
  None.
===========================================================================*/
VDL_ERROR VDL_RTOS::VDL_Process_Send_Video_Slice(void)
{
  VDL_Slice_Pkt_Type   *pSliceBuf = NULL;
  unsigned char sliceFromSentQ= FALSE;
  VDL_FrameStats_Pkt_Type *pStatsPtr = NULL;
  VDL_ERROR retVal = VDL_ERR_NONE;
  pthread_mutex_lock(&lockSendSlice);
  QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH, "VDL_RTOS::VDL_Send_Video_Slice()");

  switch (m_state)
  {
    case READY:       
    do
    {
      VDL_Return_Code_Type result;

      /* This is the only place where we update subframe queue depth*/
      VDL_Cur_QDSP_Sub_Frame_Queue_Depth = VDL_Statistics.numSubFramesSentToDSP - 
                                           VDL_Statistics.numSubFramesDoneFromDSP;

      QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF,QTVDIAG_PRIO_HIGH,"send_slice, start: VDL_Cur_QDSP_Sub_Frame_Queue_Depth : %d", 
                    VDL_Cur_QDSP_Sub_Frame_Queue_Depth);

      if (VDL_Cur_QDSP_Sub_Frame_Queue_Depth >=  VDL_QDSP_Info.MaxQDSPSubframesQueueDepth)
      {
          QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF,QTVDIAG_PRIO_HIGH,
                       "No space on dsp queue...waiting for q availability");
	  retVal = VDL_ERR_MYSTERY;
          break;
      }

        /*  if the queue count of VDL_slice_sent_q is > 0 , then  there are slices that need to 
          be sent from VDL_slice_sent_q */

        if (q_cnt(&VDL_slice_sent_q) > 0)
        {
          QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF,QTVDIAG_PRIO_HIGH,"Sending slices from slice sent q");
          pSliceBuf = (VDL_Slice_Pkt_Type *)q_get( &VDL_slice_sent_q );                       
          sliceFromSentQ = TRUE;
        }
        else
        {
          QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF,QTVDIAG_PRIO_HIGH,"Sending slices from slice q");
          pSliceBuf = (VDL_Slice_Pkt_Type *)q_get( &VDL_slice_q);             
          sliceFromSentQ = FALSE;
        }

        /* If there's no queued video slice to send, then this is an error */
        if (pSliceBuf == NULL)
        {
          QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH, "*****no slice to send\n");
          //m_state = ERROR;
          break;
        }

        /* if slices are currently being sent from VDL_slice_sent_q, then
           get the first slice from VDL_slice_sent_q, else get the first slice
           from VDL_slice_q */
        q_put( &VDL_slice_dsp_q, &pSliceBuf->link);

        /* send the video subframe packet command */
        result = VDL_QDSP_Send_Video_Subframe_Pkt (pSliceBuf);                       

        if (result != VDL_SUCCESS)
        {
          SetState((VDL_state_type)ERROR);
	  retVal = VDL_ERR_MYSTERY;
          break;
        }

        QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF,QTVDIAG_PRIO_HIGH,"sent slice!!!!");

        if (pSliceBuf->fFirstMB && !sliceFromSentQ)
        {
          VDL_Statistics.numFramesSentToDSP++;

          QTV_MSG_PRIO1( QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED,
                         "Frames sent to DSP %lld",  VDL_Statistics.numFramesSentToDSP);                          
        }
      } while (pSliceBuf);              

      break;

    default:
      QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED, "Cannot send slices in %d state", m_state);
      break;
  }

  pthread_mutex_unlock(&lockSendSlice);

  return retVal;
}

/*===========================================================================
FUNCTION:
  VDL_Process_QDSP_Sub_Frame_Done

DESCRIPTION:
  This function processes a VDL_QDSP_SUB_FRAME_DONE message from
  the DSP.

DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE:
  None.

SIDE EFFECTS:
  None.

============================================================================*/
void VDL_RTOS::VDL_Process_QDSP_Sub_Frame_Done(void)
{

  VDL_Slice_Pkt_Type   *pBuf = NULL;

#ifndef T_WINNT
  pthread_mutex_lock(&lockSendSlice); 
#endif
  VDL_Statistics.numSubFramesDoneFromDSP++;
#ifndef T_WINNT
  pthread_mutex_unlock(&lockSendSlice);
#endif
  QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH, "VDL_Process_QDSP_Sub_Frame_Done()");
  switch (m_state)
  {
    /*
     assuming we got a mod disable and we went to IDLE state. 
     Then, the higher layers need not know about the unavailability 
     of the dsp image and should still receive any pending frame dones
     from the DSP
    */
    case IDLE:
    case READY:            

      /* search through the slices in VDL_slice_dsp_q and find the first slice for which
      we did not receive a subframe done*/
      pBuf = (VDL_Slice_Pkt_Type*) q_check( &VDL_slice_dsp_q );
      while (pBuf != NULL)
      {
        if (!pBuf->isSubFrameDone)
          break;
        pBuf = (VDL_Slice_Pkt_Type*) q_next( &VDL_slice_dsp_q, &( pBuf->link ));
      }

      if (pBuf==NULL)
      {
        SetState((VDL_state_type)ERROR);
        QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH, 
                     "VDL_Process_QDSP_Sub_Frame_Done() m_state = ERROR");
        break;
      }
      else
      {
        /* now pBuf has the slice for which we received the subframe done, 
        so mark subframe done as received for the slice */
        pBuf->isSubFrameDone = TRUE;
      }

      if(q_check( &VDL_slice_q ) || q_check(&VDL_slice_sent_q))
      {
        /*Send one more slice to the aDSP*/
        QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH, "Send another subframe in ADSP THREAD CONTEXT\n");
        VDL_Process_Send_Video_Slice();

      }
      break;

    default:
      QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED, "Cannot process sub frame done!!Invalid state: %d", 
                    (VDL_state_type)m_state);
      break;

  }

  return;

} /* VDL_Process_QDSP_Sub_Frame_Done */

/*===========================================================================
FUNCTION:
 CallDecoderCBOnNonCodedVOPsForVLDDSP
 
DESCRIPTION:
  This Function Will clear up the stats queue of any
  Not coded VOPS that we did not sent to DSP. This
  function should be called whenever we receive
  a message call back from DSP.
  CallDecoderCBOnNonCodedVOPsForVLDDSP is different
  from CallDecoderCBOnNonCodedVOPsFor in respect that
  it does not clear up the stats queue off EOS since
  in case of VLD in DSP we send EOS to DSP. Decdoder
  callbacks will be called from every Not coded Vop
  in stats queue. These callbacks are suppressed
  at vdec layer presently and not propagated
  upwards.

DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE:
  None.

============================================================================*/
void VDL_RTOS::CallDecoderCBOnNonCodedVOPsForVLDDSP(void)
{
   VDL_Decode_Return_Type type = VDL_DECODE_FRAME_ERROR;
   VDL_Decode_Stats_Packet_Type* pDecodeStats = (VDL_Decode_Stats_Packet_Type *) q_check(&VDL_stats_q);
   for (pDecodeStats; 
       ( (pDecodeStats !=NULL) && (pDecodeStats->DecodeStats.ReturnCode != VDL_DECODE_NO_ERROR)
        && (pDecodeStats->DecodeStats.ReturnCode != VDL_DECODE_FRAME_FRE) &&
	(pDecodeStats->DecodeStats.ReturnCode != VDL_END_OF_SEQUENCE) &&
	(pDecodeStats->DecodeStats.ReturnCode != VDL_END_OF_SEQUENCE_FOR_FLUSH));
        pDecodeStats = (VDL_Decode_Stats_Packet_Type *) q_check(&VDL_stats_q))
   {
      pDecodeStats = (VDL_Decode_Stats_Packet_Type *) q_get(&VDL_stats_q);
      QTV_MSG_PRIO2(QTVDIAG_GENERAL, QTVDIAG_PRIO_MED, "Decremented VDL_stats_q to: %d, retCode: %d", q_cnt(&VDL_stats_q), pDecodeStats->DecodeStats.ReturnCode);
      if (pDecodeStats->DecodeStats.bIsFlushed == FALSE)
	  {
	    type = pDecodeStats->DecodeStats.ReturnCode;
	  }
	  else
      {
        type = VDL_FRAME_FLUSHED;          
      } 
	  QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_ERROR,"***** Processing second CB: %d",type );
      VDL_Codec_Info.DecoderCB( type,pDecodeStats->DecodeStats.pStatsData,
                                VDL_Codec_Info.DecoderUserData );
      VDL_Free_Stats_Buffer( pDecodeStats );
   }
}

void VDL_RTOS::CallDecoderCBOnNonCodedVOPs(void)
{
   VDL_Decode_Return_Type type = VDL_DECODE_FRAME_ERROR;
   VDL_Decode_Stats_Packet_Type* pDecodeStats = (VDL_Decode_Stats_Packet_Type *) q_check(&VDL_stats_q);
   for (pDecodeStats; 
       ( (pDecodeStats !=NULL) && (pDecodeStats->DecodeStats.ReturnCode != VDL_DECODE_NO_ERROR)
        && (pDecodeStats->DecodeStats.ReturnCode != VDL_DECODE_FRAME_FRE) );
        pDecodeStats = (VDL_Decode_Stats_Packet_Type *) q_check(&VDL_stats_q))
   {
      pDecodeStats = (VDL_Decode_Stats_Packet_Type *) q_get(&VDL_stats_q);
      QTV_MSG_PRIO2(QTVDIAG_GENERAL, QTVDIAG_PRIO_MED, "Decremented VDL_stats_q to: %d, retCode: %d", q_cnt(&VDL_stats_q), pDecodeStats->DecodeStats.ReturnCode);
      if (pDecodeStats->DecodeStats.bIsFlushed == FALSE)
	  {
	    type = pDecodeStats->DecodeStats.ReturnCode;
	  }
	  else
      {
        type = VDL_FRAME_FLUSHED;          
      } 
	  QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_ERROR,"***** Processing second CB: %d",type );
      VDL_Codec_Info.DecoderCB( type,pDecodeStats->DecodeStats.pStatsData,
                                VDL_Codec_Info.DecoderUserData );
      VDL_Free_Stats_Buffer( pDecodeStats );
   }
}
/*===========================================================================
FUNCTION:
  VDL_Process_QDSP_Frame_Decode_Done

DESCRIPTION:
  This function processes a VDL_QDSP_FRAME_DECODE_DONE message from
  the DSP.

DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  pMessageInfo

RETURN VALUE:
  None.

SIDE EFFECTS:
  None.

============================================================================*/
void VDL_RTOS::VDL_Process_QDSP_Frame_Decode_Done(void)
{
  VDL_Decode_Return_Type type = VDL_DECODE_FRAME_ERROR;
  VDL_Decode_Stats_Packet_Type* pDecodeStats;   
  void *FrameStatsInPtr,*FrameStatsOutPtr ;
  VDL_Slice_Pkt_Type   *pBuf;


  QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH, "VDL_Process_QDSP_Frame_Decode_Done()");   
  QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH, "VDL_Stats_q cnt: %d", q_cnt(&VDL_stats_q));   


  /*We either allow flush OR process frame dones: both of which access the slice/stats queues */
  pthread_mutex_lock(&lockFlushProcessing);
  if(VDL_QDSP_Info.DSPInterface != VDL_DSP_INTERFACE_RTOS_QTV_LP)
  {
    pthread_mutex_lock(&lockEOSProcessing);
  }
  switch (m_state)
  {
    /*
    assuming we got a mod disable and we went to IDLE state. 
    Then, the higher layers need not know about the unavailability 
    of the dsp image and should still receive any pending frame dones
    from the DSP
   */
    case IDLE:
    case READY:        

      /* Get the associated decode stats from the VDL_stats_q */
      if(VDL_QDSP_Info.DSPInterface == VDL_DSP_INTERFACE_RTOS_QTV_LP) {
	      if(VideoFormatType != VDL_VIDEO_H264)
        	CallDecoderCBOnNonCodedVOPsForVLDDSP();
      } else {
        	CallDecoderCBOnNonCodedVOPs();
      }

      pDecodeStats = (VDL_Decode_Stats_Packet_Type *) q_get(&VDL_stats_q);
      if( pDecodeStats )
      {
        if (pDecodeStats->DecodeStats.ReturnCode != VDL_DECODE_FRAME_FRE)
        {
          pBuf = (VDL_Slice_Pkt_Type*) q_get( &VDL_slice_dsp_q );

#if 0
          if(pBuf->isStatsFromL2 && pBuf->fFirstMB)
          {
            /*FIXME:Copy the statistics to upper layer*/
            //FrameStatsOutPtr = (void*)pBuf->pFrameStatsOutPtr->pStats_pkt;
            //FrameStatsInPtr = (void*)pBuf->pFrameStatsInPtr;
          }
#endif

          VDL_Free_Slice_Buffer( pBuf );
          pBuf = (VDL_Slice_Pkt_Type*) q_check( &VDL_slice_dsp_q );
          while ((pBuf != NULL) && (!pBuf->fFirstMB))
          {
            pBuf = (VDL_Slice_Pkt_Type*) q_get( &VDL_slice_dsp_q );
            VDL_Free_Slice_Buffer( pBuf );       
            pBuf = (VDL_Slice_Pkt_Type*) q_check( &VDL_slice_dsp_q );
          }
          QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH, "Deleted slices from slice dsp q:");
        }
        else
        {
          //ALL FRE frames should go to PP FRAME Done processing.check out
          QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_ERROR,"FRE FRAME received but not as PP Frame Done");
          ASSERT(0);
        }
      }
 
      if (pDecodeStats)
      {
	      if (pDecodeStats->DecodeStats.bIsFlushed == FALSE)
	      {
		      type = pDecodeStats->DecodeStats.ReturnCode;
	      }
	      else
	      {
		      type = VDL_FRAME_FLUSHED;        
	      }
	      VDL_Statistics.numFramesRxdFromDSP++;
	      QPERF_END(dsp_decode);
	      if(VDL_Statistics.numFramesSentToDSP != VDL_Statistics.numFramesRxdFromDSP)
	      {
		      QPERF_START(dsp_decode);
	      }
#ifdef FEATURE_FRE
	      if (pDecodeStats->DecodeStats.ReturnCode != VDL_DECODE_FRAME_FRE)
#endif
	      {
		      if (pDecodeStats->DecodeStats.ReturnCode != VDL_END_OF_SEQUENCE_FOR_FLUSH) {
		      VDL_Codec_Info.DecoderCB( type,
				      pDecodeStats->DecodeStats.pStatsData,
				      VDL_Codec_Info.DecoderUserData );
		      }
		      memset( pDecodeStats, 0, sizeof( VDL_Decode_Stats_Packet_Type ) );
		      q_put( &VDL_stats_free_q, &pDecodeStats->Link );                
	      }
#ifdef FEATURE_FRE
	      else
	      {
		      //ALL FRE frames should go to PP FRAME Done processing.check out
		      QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_ERROR,"FRE FRAME received but not as PP Frame Done");
		      ASSERT(0);
	      }
#endif
	      if(VDL_QDSP_Info.DSPInterface == VDL_DSP_INTERFACE_RTOS_QTV_LP) {
		      if(VideoFormatType != VDL_VIDEO_H264)
			      CallDecoderCBOnNonCodedVOPsForVLDDSP();
	      } else {
		      CallDecoderCBOnNonCodedVOPs();
	      }
      }
      else
      { /* pDecodeStats == NULL */
        QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_ERROR,"pDecodeStats NULL" );
        VDL_Statistics.numFramesRxdFromDSP++;  
      }         

      /*Signal if we in the flushing operation*/
      pthread_mutex_lock(&m_pending_frame_done_lock);
      if(m_pending_frame_dones)
      { 
        if(!(--m_pending_frame_dones))
        {
          pthread_cond_broadcast(&m_all_frame_done_cond);
        }
      }
      pthread_mutex_unlock(&m_pending_frame_done_lock);

      QTV_MSG_PRIO1( QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED,
                     "Frames received from DSP %lld",  VDL_Statistics.numFramesRxdFromDSP); 

      break;

    default:
      QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED, "Cannot process frame \
                    decode done!!Invalid state: %d", (VDL_state_type)m_state);
      break;
  }
  if(VDL_QDSP_Info.DSPInterface != VDL_DSP_INTERFACE_RTOS_QTV_LP)
  { 	  
    pthread_mutex_unlock(&lockEOSProcessing);
  }
  pthread_mutex_unlock(&lockFlushProcessing);

} /* VDL_Process_QDSP_Frame_Decode_Done */

/*===========================================================================
FUNCTION:
  VDL_Process_QDSP_Fatal_Error

DESCRIPTION:
  This function processes a FATAL ERROR message from
  the DSP.

DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  pMessageInfo

RETURN VALUE:
  None.

SIDE EFFECTS:
  None.

============================================================================*/
void VDL_RTOS::VDL_Process_QDSP_Fatal_Error(void)
{ 
  VDL_Decode_Stats_Packet_Type* pDecodeStats = NULL;
  VDL_Slice_Pkt_Type   *pBuf;

  pthread_mutex_lock(&lockFlushProcessing);
  if(VDL_QDSP_Info.DSPInterface != VDL_DSP_INTERFACE_RTOS_QTV_LP)
  {
    pthread_mutex_lock(&lockEOSProcessing);
  }
  pDecodeStats = (VDL_Decode_Stats_Packet_Type *) q_get(&VDL_stats_q);
  QTV_MSG_PRIO1( QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED,
                 "VDL_Process_QDSP_Fatal_Error pDecodeStats %p",  pDecodeStats);
  if( pDecodeStats )
  {
    VDL_Statistics.numFramesRxdFromDSP++;

    VDL_Codec_Info.DecoderCB( VDL_FATAL_ERROR,
                              pDecodeStats->DecodeStats.pStatsData,
                              VDL_Codec_Info.DecoderUserData );
    memset( pDecodeStats, 0, sizeof( VDL_Decode_Stats_Packet_Type ) );
    q_put( &VDL_stats_free_q, &pDecodeStats->Link );                
  }

  while ( (pBuf = (VDL_Slice_Pkt_Type*) q_get( &VDL_slice_q )) != NULL)
  {
    VDL_Free_Slice_Buffer( pBuf );
  }

  while ( (pBuf = (VDL_Slice_Pkt_Type*) q_get( &VDL_slice_dsp_q )) != NULL)
  {
    VDL_Free_Slice_Buffer( pBuf );
  }

  while ( (pBuf = (VDL_Slice_Pkt_Type*) q_get( &VDL_slice_sent_q )) != NULL)
  {
    VDL_Free_Slice_Buffer( pBuf );
  }

  if(VDL_QDSP_Info.DSPInterface != VDL_DSP_INTERFACE_RTOS_QTV_LP)
  { 	  
    pthread_mutex_unlock(&lockEOSProcessing);
  }
  pthread_mutex_unlock(&lockFlushProcessing);
}

/*===========================================================================

FUNCTION:
  VDL_QDSP_Send_Video_Subframe_Pkt

DESCRIPTION:
  Send the subframe packet command to the DSP 

DEPENDENCIES
  None
  
INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE:
  VDL_Return_Code_Type

SIDE EFFECTS:
  None.
===========================================================================*/
VDL_Return_Code_Type VDL_RTOS::VDL_QDSP_Send_Video_Subframe_Pkt(VDL_Slice_Pkt_Type * pData)
{
  QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED, ">>>>> VDL_QDSP_Send_Video_Subframe_Pkt seqNum: %d", VDL_QDSP_Info.subframePacketSeqNum);
  ASSERT(pData);
  ASSERT(pData->pNextMacroBlock);
  ASSERT(pData->NumMacroBlocks > 0);
  //ASSERT(pData->pFrameStatsInPtr);

  /* Return if input slice pointer is NULL */
  if (pData == NULL)
  {
    QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_ERROR, "Null slice pointer");
    return VDL_FAILURE_FATAL;
  }

  /* Return if the macro block pointer is NULL */
  if (pData->pNextMacroBlock == NULL)
  {
    QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_ERROR,"Null macro block pointer");
    return VDL_FAILURE_FATAL;
  }

  /* Return immediately if there no macro blocks to send */
  if (pData->NumMacroBlocks == 0)
  {
    QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_ERROR, "Rx'd slice buffer with 0 MB");
    return VDL_FAILURE_FATAL;
  }

  memset(&VideoSubframePacketCmd, 0, sizeof(VideoSubframePacketCmd));
  VideoSubframePacketCmd.cmd = 0; 
  /* Fill the Subframe packet structure */
  VideoSubframePacketCmd.PacketSeqNum = (VDL_QDSP_Info.subframePacketSeqNum++ % 4096);  
  VideoSubframePacketCmd.StreamId  = streamID; 
  /*Frame Buffer related info*/
  VideoSubframePacketCmd.PktSizeHi    = 
  ((pData->SliceDataSize >> 1) >> 16) & 0xFFFF;
  VideoSubframePacketCmd.PktSizeLo    = 
  (pData->SliceDataSize >> 1) & 0xFFFF;
  /*
#ifndef FEATURE_VDL_TEST_PC
  VideoSubframePacketCmd.PktAddrHi    = 
  (((unsigned long int)pData->pSliceDataPhysical) >> 16) & 0xFFFF;
  VideoSubframePacketCmd.PktAddrLo    =
  (((unsigned long int)pData->pSliceDataPhysical)) & 0xFFFF;
#else
*/
  VideoSubframePacketCmd.PktAddrHi    = 
  (((unsigned long int)pData->pSliceData) >> 16) & 0xFFFF;
  VideoSubframePacketCmd.PktAddrLo    =
  (((unsigned long int)pData->pSliceData)) & 0xFFFF;
//#endif
#if 0  //jlk - Enable this stuff when DSP stats bufs are working
  if(VDL_DSP_INTERFACE_RTOS_QTV_LP == VDL_QDSP_Info.DSPInterface)
  {
    if(!pData->isStatsFromL2)
    {
      /*no l2 stats available..Use the one provided by PL*/
      pStatsPtr2DSP = pData->pFrameStatsInPtr;
    }
    else
    {
      //ASSERT(pData->pFrameStatsOutPtr);
      //pStatsPtr2DSP = pData->pFrameStatsOutPtr->pStats_pkt;
    }
  }
#endif

  if(VDL_DSP_INTERFACE_RTOS_QTV_LP == VDL_QDSP_Info.DSPInterface)
  {
    /*Checkout whether it should be in bytes or words*/
    VideoSubframePacketCmd.Stats_PktSizeHi    = ((sizeof(VDL_Frame_Stats_Pkt_Type) >> 1) >> 16) & 0xFFFF;
    VideoSubframePacketCmd.Stats_PktSizeLo    = ((sizeof(VDL_Frame_Stats_Pkt_Type) >> 1)) & 0xFFFF;
    
    VideoSubframePacketCmd.Stats_PktAddrHi    = (((unsigned long int)VDL_RTOS_LP_STATS_BUF_ADDR) >> 16) & 0xFFFF;
    VideoSubframePacketCmd.Stats_PktAddrLo    = (((unsigned long int)VDL_RTOS_LP_STATS_BUF_ADDR)) & 0xFFFF;
    VideoSubframePacketCmd.Stats_Partition    = 0x0000;
  
    /* DM/DMA packet exchange mode..for regular RTOS it is always DM */
    VideoSubframePacketCmd.SubframeInfo_1 = VDL_QDSP_SUBFRAME_INFO1(pData->isStatsFromL2,pData->SliceDataSize <= VDL_RTOS_LP_PKT_BUFFER_SIZE /* Pkt DM_exchange */);
  
    /*These fileds can probably be filled by PL*/
    VideoSubframePacketCmd.SubframeInfo_0 = VDL_QDSP_SUBFRAME_INFO0(pData->fFirstMB);
  }
  else
  {
    //We are anyways memset-ing VideoSubframePacketCmd to 0
    //VideoSubframePacketCmd.Stats_PktSizeHi    = 0x0000;
    //VideoSubframePacketCmd.Stats_PktSizeLo    = 0x0000;
    //VideoSubframePacketCmd.Stats_PktAddrHi    = 0x0000;
    //VideoSubframePacketCmd.Stats_PktAddrLo    = 0x0000;
    //VideoSubframePacketCmd.Stats_Partition    = 0x0000;  
    /* DM/DMA packet exchange mode..for regular RTOS it is always DM */
    //VideoSubframePacketCmd.SubframeInfo_1 = 0x0000;
  
    /*These fileds can probably be filled by PL*/
    VideoSubframePacketCmd.SubframeInfo_0 = VDL_QDSP_SUBFRAME_INFO(pData->fFirstMB, pData->fLastMB);
  }

  VideoSubframePacketCmd.CodecType = pData->CodecType;
  VideoSubframePacketCmd.NumMBs       = pData->NumMacroBlocks;

  #ifdef FEATURE_VDL_LOG_SLICE_BUF
  int wroteBytes = m_psliceBufferLog2->Write( (char*)pData->pSliceData, 
                                                             pData->SliceDataSize );
  
  FARF(ERROR,(&F,"Send subframe to aDSP-> Wrote %d out of %d bytes", wroteBytes, 
                  pData->SliceDataSize));
  #endif //FEATURE_VDL_LOG_SLICE_BUF

  //VDL_Watchdog_Set(VDL_WATCHDOG_TIMEOUT);

    QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_LOW, "Sending slice  to adsp rtos (queuing to adsp cmd q)\n");
    adsp_rtos_send_command_16(QDSP_MODULE_VIDEOTASK,   
								(unsigned long int)QDSP_mpuVDecPktQueue,
                      			(unsigned short*) &VideoSubframePacketCmd,
								(unsigned long int) (sizeof(VideoSubframePacketCmd) / sizeof(unsigned short))
							  );
    if( pData->fFirstMB ){
      if(VDL_Statistics.numFramesSentToDSP == VDL_Statistics.numFramesRxdFromDSP)
      {
        QPERF_START(dsp_decode);
      }
    }
	//TBD: Implement watchdog timer
    VDL_Statistics.numSubFramesSentToDSP++;
	QTV_MSG_PRIO1(  QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH , 
					"Sent VideoSubframePacketCmd.., SFQDepth=%u !",
                    (unsigned) (VDL_Statistics.numSubFramesSentToDSP - VDL_Statistics.numSubFramesDoneFromDSP)
				  );
    return VDL_SUCCESS;
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
VDL_Slice_Pkt_Type * VDL_RTOS::VDL_Get_Slice_Buffer(void)
{
  VDL_Slice_Pkt_Type *pSliceBuf;

  QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED, "VDL_Get_Slice_Buffer()");

  /* Get a slice buffer.  If one's not available on the free queue,
  ** return NULL.
  */
  pSliceBuf = (VDL_Slice_Pkt_Type *) q_get( &VDL_slice_free_q );
  /* If we failed to obtain a slice buffer, we have no choice but
  ** to return NULL
  */
  if ( !pSliceBuf )
  {
    QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_FATAL, "pSliceBuf is NULL");
    return NULL;
  }

  pSliceBuf->pNextMacroBlock = (unsigned short *) pSliceBuf->pSliceData;
  pSliceBuf->NumMacroBlocks = 0;
  pSliceBuf->SliceDataSize      = 0;
  pSliceBuf->pFrameStatsInPtr = NULL;
  pSliceBuf->pFrameStatsOutPtr = NULL;
  pSliceBuf->isStatsFromL2 = TRUE;
  pSliceBuf->SliceBufferSize = VDL_DSP_VLD_Slice_Buffer_Size;

  pSliceBuf->isSubFrameDone = 0;
  pSliceBuf->isFlushMarker = 0;
  pSliceBuf->SliceSeqNum = 0;
  return(pSliceBuf);

}

void VDL_RTOS::VDL_init_slice_free_q(VDEC_SLICE_BUFFER_INFO* input, 
                                             unsigned short num_buffers)
{
  VDL_Slice_Pkt_Type *pSliceBuf;

  for(int index=0; index < num_buffers; index++)
  {
    pSliceBuf = (VDL_Slice_Pkt_Type *)QTV_Malloc( sizeof( VDL_Slice_Pkt_Type ));

    if ( pSliceBuf )
    {
      pSliceBuf->pSliceData = reinterpret_cast <void*>(input[index].base);
      //pSliceBuf->pSliceDataPhysical = reinterpret_cast <void*>(input[index].phys);

      //printx("VDL_init_slice_free_q virt:%p phys:%p\n", 
      //        pSliceBuf->pSliceData, pSliceBuf->pSliceDataPhysical);

      /* prepare the new buffer to go into a queue */
      q_link( pSliceBuf, &( pSliceBuf->link ) );
      q_put( &VDL_slice_free_q, &( pSliceBuf->link ) );
      num_slice_buffers++;
    }
    else
    {
      QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED, 
                   "VDL_init_slice_free_q(): Out of Memory..!");
      break;
    }
  }
}

VDL_ERROR VDL_RTOS::VDL_Free_Slice_Buffer( VDL_Slice_Pkt_Type *pSliceBuf )
{
  QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED, "VDL_Free_Slice_Buffer()");
  if(pSliceBuf==NULL)
  {
     QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_ERROR, "pSliceBuf is NULL");
     return VDL_ERR_MYSTERY;
  }
  q_put( &VDL_slice_free_q, &pSliceBuf->link );
  return VDL_ERR_NONE;
}

/*===========================================================================

FUNCTION:
  VDL_Initialize_LP_Structures

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
VDL_ERROR VDL_RTOS::VDL_Initialize_Structures(VDL_Interface_Type iface)
{
  /* Specify the interface parameters */
  if (VDL_INTERFACE_RTOS == iface)
  {
    VDL_QDSP_Info.DSPInterface = VDL_DSP_INTERFACE_DEFAULT;
    VDL_QDSP_Info.MaxQDSPSubframesQueueDepth = MAX_SUBFRAME_QUEUE_DEPTH;
  }
  else
  {
    VDL_QDSP_Info.DSPInterface = VDL_DSP_INTERFACE_RTOS_QTV_LP;
    VDL_QDSP_Info.MaxQDSPSubframesQueueDepth = VDL_RTOS_LP_SUBFRAME_QUEUE_DEPTH;
  }

  num_slice_buffers = 0; 
  VDL_QDSP_Info.subframePacketSeqNum = 0;

  VDL_QDSP_Info.prevSFDoneSeqNum     = 0xBAD;  
  VDL_QDSP_Info.UsedSubframeQueueDepth = 0;

  //Stats-q init
  num_stats_buffers = 0;
  (void)q_init(&VDL_stats_q);
  (void)q_init(&VDL_stats_free_q);
  QTV_MSG_PRIO1(QTVDIAG_GENERAL, QTVDIAG_PRIO_MED, "Init'd VDL_stats_q, cnt: %d", q_cnt(&VDL_stats_q));

  //Slice-q's init
  (void) q_init (&VDL_slice_q);
  (void) q_init (&VDL_slice_free_q);
  (void) q_init (&VDL_slice_dsp_q);
  (void) q_init (&VDL_slice_sent_q);

  return VDL_ERR_NONE;
}


/*===========================================================================

FUNCTION:
  VDL_Process_Send_PP_cmd

DESCRIPTION:
  Send pp cmd to the DSP

DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE:
  None.

SIDE EFFECTS:
  None.
===========================================================================*/
void VDL_RTOS::VDL_Process_Send_PP_cmd(void)
{
  //Reqd only for FRE
  return; 
}

VDL_ERROR VDL_RTOS::VDL_Queue_Stats_Buffer(void *pDecodeStats,
                                                   VDL_Decode_Return_Type retCode)
{
  QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_MED, "VDL_RTOS::VDL_Queue_Stats_Buffer()");

  if (retCode == VDL_END_OF_SEQUENCE)
  {
    QTV_MSG_PRIO1(QTVDIAG_GENERAL, QTVDIAG_PRIO_LOW, "VDL EOS received, stats_q cnt: %d", q_cnt(&VDL_stats_q));
  }
  /* get a stats buffer packet */
  VDL_Decode_Stats_Packet_Type *pCurrentDecodeStats = VDL_Get_Stats_Buffer();

  if (pCurrentDecodeStats == NULL)
  {
    QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_FATAL, "pCurrentDecodeStats is NULL");  
    return VDL_ERR_OUT_OF_MEMORY;
  }

  /* update the stats buffer data in the packet based on what was passed */
  pCurrentDecodeStats->DecodeStats.pStatsData = pDecodeStats;
  pCurrentDecodeStats->DecodeStats.ReturnCode = retCode;
  q_put(&VDL_stats_q, &pCurrentDecodeStats->Link);
  if ((VDL_QDSP_Info.DSPInterface != VDL_DSP_INTERFACE_RTOS_QTV_LP) && (retCode == VDL_END_OF_SEQUENCE))
  {
    pthread_mutex_lock(&lockEOSProcessing);
    VDL_Decode_Stats_Packet_Type* pStats = (VDL_Decode_Stats_Packet_Type *) q_check(&VDL_stats_q);
	for (pStats; ( (pStats !=NULL) && (pStats->DecodeStats.ReturnCode != VDL_DECODE_NO_ERROR)
         && (pStats->DecodeStats.ReturnCode != VDL_DECODE_FRAME_FRE) );
         pStats = (VDL_Decode_Stats_Packet_Type *) q_check(&VDL_stats_q))
	{
	     pStats = (VDL_Decode_Stats_Packet_Type *) q_get(&VDL_stats_q);
         QTV_MSG_PRIO2(QTVDIAG_GENERAL, QTVDIAG_PRIO_MED, "Decremented VDL_stats_q to: %d, retCode: %d", q_cnt(&VDL_stats_q), pStats->DecodeStats.ReturnCode);
         VDL_Codec_Info.DecoderCB( pStats->DecodeStats.ReturnCode,
                                  pStats->DecodeStats.pStatsData,
                                  VDL_Codec_Info.DecoderUserData );
                 VDL_Free_Stats_Buffer(pStats);
	}
	pthread_mutex_unlock(&lockEOSProcessing);
  }
  return VDL_ERR_NONE;
}

VDL_Decode_Stats_Packet_Type* VDL_RTOS::VDL_Get_Stats_Buffer(void)
{
  QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_MED, "VDL_Get_Stats_Buffer()");
  VDL_Decode_Stats_Packet_Type* pBuf = (VDL_Decode_Stats_Packet_Type *) q_get(&VDL_stats_free_q);

  if (pBuf == NULL) 
  {
    /* Get a new decode statistics buffer */
    pBuf = (VDL_Decode_Stats_Packet_Type*) QTV_Malloc(sizeof(VDL_Decode_Stats_Packet_Type));
    num_stats_buffers++;
  }

  if ( pBuf != NULL )
  {
    memset( pBuf, 0, sizeof( VDL_Decode_Stats_Packet_Type ) );
  }

  return pBuf;
}

VDL_ERROR VDL_RTOS::VDL_Queue_Slice_Buffer(VDL_Slice_Pkt_Type *pSliceBuf)
{
  VDL_ERROR retVal = VDL_ERR_NONE;
  QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED, "VDL_Queue_Slice_Buffer()");

  ASSERT (pSliceBuf);

  if ((pSliceBuf != NULL) && (m_state != ERROR))
  {
    ASSERT(pSliceBuf->pSliceData);
    pSliceBuf->pNextMacroBlock = (unsigned short *)pSliceBuf->pSliceData;
    pSliceBuf->NumMacroBlocksToSend = pSliceBuf->NumMacroBlocks;
    pSliceBuf->SliceSeqNum = BufferQueueRunningCount++ ;
    #ifdef FEATURE_VDL_LOG_SLICE_BUF
    //VDL_Log_Slice_Buffer(pSliceBuf, VDL_QDSP_Image_Info.VideoFormat);
    int wroteBytes = m_psliceBufferLog1->Write( (char*)pSliceBuf->pSliceData,
                                                  pSliceBuf->SliceDataSize );

    FARF(ERROR,(&F,"Queue subframe to VDL-> Wrote %d out of %d bytes", wroteBytes, 
                      pSliceBuf->SliceDataSize));

    #endif

#ifdef LOG_YUV_SLICES
    if (pSliceFile)
    {
      int byteswritten = fwrite(pSliceBuf->pSliceData,sizeof(unsigned short),pSliceBuf->SliceDataSize/sizeof(unsigned short),pSliceFile);		
      fflush(pSliceFile);
      QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_LOW, "bytes written: %d\n", byteswritten);
    }
#endif

    /* Queue video slice packet */
    q_put (&VDL_slice_q, &pSliceBuf->link);

    QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH, "Q video slice %d",
                  pSliceBuf->NumMacroBlocks);

    /* if one frame worth of slices have been queued in the slice queue,
       then queue a command in the medium priority ARM command queue, which in turn
       updates the state of the instance and signals the VDL Agent task */
    if (pSliceBuf->fLastMB)
    {
      //Send the slice to the DSP directly
      retVal = VDL_Process_Send_Video_Slice();
    }
  }

  return retVal;
}

/* ======================================================================
FUNCTION
 VDL_Initialize_CodecInfo

DESCRIPTION
 Initialize codec specific information

DEPENDENCIES
  None.

INPUT/OUTPUT PARAMETERS:
  Decoder callback function
  user data pointer
  encoded video format
  audio/concurrency format


RETURN VALUE
  error code provided by VDL_ERROR

SIDE EFFECTS
  None.
========================================================================== */
VDL_ERROR VDL_RTOS::VDL_Initialize_CodecInfo(
                                         VDL_Decoder_Cb_Type  DecoderCB,  
                                         void                 *ddUserData,                       
                                         VDL_Video_Type VideoFormat,         
                                         VDL_Concurrency_Type ConcurrencyFormat,
                                         unsigned int width, unsigned int height
                                         )
{
  
  QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED, "VDL_Initialize_Codec_Info()");        
  ASSERT (DecoderCB);    

  if (DecoderCB == NULL )
  {
    QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_FATAL," NULL decoder callback! Returning Failure");
    return VDL_ERR_INIT_FAILURE;
  }

  VDL_Codec_Info.DecoderCB    = DecoderCB;
  VDL_Codec_Info.DecoderUserData = ddUserData; 
  VDL_Codec_Info.frameCount = 0;   
  VDL_Codec_Info.Width  = width;
  VDL_Codec_Info.Height = height;
  VideoFormatType = VideoFormat;

  if (( height & 15) != 0)
  {
    VDL_Codec_Info.DecHeight = (( height >> 4) + 1) << 4;
  }
  if (( width & 15) != 0)
  {
    VDL_Codec_Info.DecWidth = (( width >> 4) + 1) << 4;
  }

  //VDL_QDSP_Image_Info.ConcurrencyFormat = (VDL_Concurrency_Type)ConcurrencyFormat;
  //VDL_QDSP_Image_Info.VideoFormat = (VDL_Video_Type)VideoFormat;

  QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED,"Video format: %d", (VDL_Video_Type)VideoFormat);
  QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED,"Concurrency format: %d", (VDL_Concurrency_Type)ConcurrencyFormat);

  return VDL_ERR_NONE;

}

/*===========================================================================

FUNCTION:
  VDL_Flush_All_Slices

DESCRIPTION:
  This function flushes all the slices in slice queue. This dunction is 
  solely created for the purpose to flush the slice q completely without 
  any restrictions. This function should be called very carefully, mainly 
  when the dsp goes in an irrecoverable state. If the dsp hangs/crashes, 
  it does not give us a frame done callback so we have to clear the 
  slice q in that case. 
   
DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE:
  None

SIDE EFFECTS:
  None

===========================================================================*/
void VDL_RTOS::VDL_Flush_All_Slices(void)
{
  VDL_Slice_Pkt_Type*          slice_ptr;
  VDL_Slice_Pkt_Type*          next_slice_ptr;
  slice_ptr = ( VDL_Slice_Pkt_Type* )( q_check( &VDL_slice_q ) );
  while ( slice_ptr )
  {
    next_slice_ptr = ( VDL_Slice_Pkt_Type* )
                     ( q_next( &VDL_slice_q, &( slice_ptr->link ) ) );
    q_delete( &VDL_slice_q, &( slice_ptr->link ) );
    VDL_Free_Slice_Buffer( slice_ptr );
    slice_ptr = next_slice_ptr;
  }

  if (m_state == ERROR) 
  {
    QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH, "VDL_Flush_All_Slices");
    /* remove all queued slice buffers in the wait Q */
    while ((slice_ptr = (VDL_Slice_Pkt_Type *) (q_get (&VDL_slice_dsp_q))) != NULL)
    {
      QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH, "Free DSP Slice: %x",slice_ptr);
      VDL_Free_Slice_Buffer( slice_ptr );
    }
  }
}
/*===========================================================================

FUNCTION:
  VDL_Flush_All_Stats

DESCRIPTION:
  This function flushes all the stats buffers in stats queue. This function is 
  solely created for the purpose to flush the stats q completely without 
  any restrictions. This function should be called very carefully, mainly 
  when the dsp goes in an irrecoverable state. If the dsp hangs/crashes, 
  it does not give us a frame done callback so we need to generate the 
  frame done call backs for the frames which are with dsp and clear the stats q. 
  
DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  None
  
RETURN VALUE:
  None

SIDE EFFECTS:
  None

===========================================================================*/
void VDL_RTOS::VDL_Flush_All_Stats(void)
{
  VDL_Decode_Stats_Packet_Type* stats_ptr;
  VDL_Decode_Stats_Packet_Type* next_stats_ptr;
  unsigned int              stats_flushed = 0;
  unsigned int              stats_count = 0;
  unsigned int              pending_stats_count = 0;
  stats_ptr = ( VDL_Decode_Stats_Packet_Type* )( q_check( &VDL_stats_q ) );
  while(stats_ptr){
      stats_ptr->DecodeStats.bIsFlushed = TRUE;
      next_stats_ptr = ( VDL_Decode_Stats_Packet_Type* )
                       ( q_next( &VDL_stats_q, &( stats_ptr->Link ) ) );

      VDL_Codec_Info.DecoderCB( VDL_FRAME_FLUSHED,
                               stats_ptr->DecodeStats.pStatsData,
                               VDL_Codec_Info.DecoderUserData );
      VDL_Free_Stats_Buffer( stats_ptr );
      stats_ptr = next_stats_ptr;
  }                
}
/*===========================================================================

FUNCTION:
  VDL_Flush_Slice_Queues

DESCRIPTION:
  This function flushes the slice queues
  
DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  None
  
RETURN VALUE:
  None

SIDE EFFECTS:
  None

===========================================================================*/
void VDL_RTOS::VDL_Flush_Slice_Queues(void)
{
  VDL_Slice_Pkt_Type*          slice_ptr;
  VDL_Slice_Pkt_Type*          next_slice_ptr;
  unsigned int                 slices_flushed = 0;

  slice_ptr = ( VDL_Slice_Pkt_Type* )( q_check( &VDL_slice_q ) );

  /* go through the slice queue and search for the slices where the firstMB is not 
   set. From that point onwards, the rest of the slice queue can be deleted */
  /* no slices implies no stats buffers */

  /* Find the first slice which is marked as beginning-of-frame.
     Dont flush this but cleanup the remaining.*/
  while ( slice_ptr && !slice_ptr->fFirstMB )
  {
    slice_ptr = 
    ( VDL_Slice_Pkt_Type* )( q_next( &VDL_slice_q, &( slice_ptr->link ) ) );
  }

  while ( slice_ptr )
  {
    next_slice_ptr = ( VDL_Slice_Pkt_Type* )
                     ( q_next( &VDL_slice_q, &( slice_ptr->link ) ) );
    q_delete( &VDL_slice_q, &( slice_ptr->link ) );
    VDL_Free_Slice_Buffer( slice_ptr );
    slice_ptr = next_slice_ptr;
    ++slices_flushed;
  }

  /* go through the slice sent queue and search for the slices where the firstMB is not 
    set. From that point onwards, the rest of the slice sent queue can be deleted */
   /* no slices implies no stats buffers */

   /* Find the first slice which is marked as beginning-of-frame.
   */
/*  
  unsigned int cntFramesFlushedSliceSentQ=0;
  VDL_Slice_Pkt_Type*          slice_sent_ptr;
  slice_sent_ptr = ( VDL_Slice_Pkt_Type* )( q_check( &VDL_slice_sent_q ) );
  VDL_Slice_Pkt_Type*          next_slice_sent_ptr;    

  while ( slice_sent_ptr && !slice_sent_ptr->fFirstMB )
  {
    slice_sent_ptr = 
    ( VDL_Slice_Pkt_Type* )( q_next( &VDL_slice_sent_q, &( slice_sent_ptr->link ) ) );
  }


  while ( slice_sent_ptr )
  {
    if (slice_sent_ptr->fLastMB)
      cntFramesFlushedSliceSentQ++;

    next_slice_sent_ptr = ( VDL_Slice_Pkt_Type* )
                          ( q_next( &VDL_slice_sent_q, &( slice_sent_ptr->link ) ) );
    q_delete( &VDL_slice_sent_q, &( slice_sent_ptr->link ) );
    VDL_Free_Slice_Buffer( slice_sent_ptr );
    slice_sent_ptr = next_slice_sent_ptr;
    ++slices_flushed;
  } 
*/  
  /* If the frames sent from the slice sent q have been flushed, then account for those flushed 
     frames in VDL_Statistics.numFramesSentToDSP */
/*  VDL_Statistics.numFramesSentToDSP = VDL_Statistics.numFramesSentToDSP - cntFramesFlushedSliceSentQ;

  QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF,QTVDIAG_PRIO_HIGH,"Number of frames sent to DSP: %d", 
                VDL_Statistics.numFramesSentToDSP);
  QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF,QTVDIAG_PRIO_HIGH,"Number of frames received from DSP: %d", 
                VDL_Statistics.numFramesRxdFromDSP);        
*/

}

/*===========================================================================

FUNCTION:
  VDL_Flush_Stats_Queue

DESCRIPTION:
  This function flushes the stats queue
  
DEPENDENCIES
  None

INPUT/OUTPUT PARAMETERS:
  None
  
RETURN VALUE:
  None

SIDE EFFECTS:
  None

===========================================================================*/
int VDL_RTOS::VDL_Flush_Stats_Queue(void)
{
  /*Need to access lock this func along with frame done from aDSP..*/
  VDL_Decode_Stats_Packet_Type* stats_ptr;
  VDL_Decode_Stats_Packet_Type* next_stats_ptr;
  unsigned int              stats_flushed = 0;
  unsigned int              stats_count = 0;
  unsigned int              pending_stats_count = 0;

  stats_ptr = ( VDL_Decode_Stats_Packet_Type* )( q_check( &VDL_stats_q ) );

  QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_FATAL, "Flush VDL_stats_q: stats_ptr %p",stats_ptr);  
  /* numFramesRxdFromDSP is implicitly protected since both Flush_Stats_Queue and Frame_Done are
  ** mutually locked..
  ** numFramesSentToDSP is implicitly protected since both Flush_Stats_Queue and Process_Send_Video_Slice 
  ** are mutually locked */
  pending_stats_count = VDL_Statistics.numFramesSentToDSP - VDL_Statistics.numFramesRxdFromDSP;
  stats_count = pending_stats_count;

  while (stats_count)
  {
    if (!stats_ptr)
    {
      QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_FATAL, "Stats ptr is NULL");  
      ASSERT(0);
    }

    stats_ptr->DecodeStats.bIsFlushed = TRUE;

    if ((VDL_QDSP_Info.DSPInterface == VDL_DSP_INTERFACE_RTOS_QTV_LP) && 
		    ((stats_ptr->DecodeStats.ReturnCode == VDL_END_OF_SEQUENCE) || stats_ptr->DecodeStats.ReturnCode == VDL_END_OF_SEQUENCE_FOR_FLUSH ))
    {
      QTV_MSG_PRIO( QTVDIAG_DEC_DSP_IF,
                    QTVDIAG_PRIO_HIGH,
                    " VDL_Process_Flush: retained a EOS Frame/stats buffer");
      stats_count--;
    }
    if ((stats_ptr->DecodeStats.ReturnCode == VDL_DECODE_NO_ERROR) ||
        (stats_ptr->DecodeStats.ReturnCode == VDL_DECODE_FRAME_FRE))

    {
      QTV_MSG_PRIO( QTVDIAG_DEC_DSP_IF,
                    QTVDIAG_PRIO_MED,
                    " VDL_Process_Flush: retained a good Frame/stats buffer");
      stats_count--;
    }
    else
    {
      QTV_MSG_PRIO1( QTVDIAG_DEC_DSP_IF,
                     QTVDIAG_PRIO_MED,
                     " VDL_Process_Flush: retained a not coded/bad Frame/Stats Buffer and type is %d", 
                     stats_ptr->DecodeStats.ReturnCode);
    }

    stats_ptr = ( VDL_Decode_Stats_Packet_Type* )
                ( q_next( &VDL_stats_q, &( stats_ptr->Link ) ) );                
  }

  /* The rest of the stats buffers in the stats queue can be deleted */
  if ( stats_ptr )
  {
    do
    {
      next_stats_ptr = ( VDL_Decode_Stats_Packet_Type* )
                       ( q_next( &VDL_stats_q, &( stats_ptr->Link ) ) );

      VDL_Codec_Info.DecoderCB( VDL_FRAME_FLUSHED,
                               stats_ptr->DecodeStats.pStatsData,
                               VDL_Codec_Info.DecoderUserData );
      VDL_Free_Stats_Buffer( stats_ptr );
      stats_ptr = next_stats_ptr;
      ++stats_flushed;

      QTV_MSG_PRIO1( QTVDIAG_DEC_DSP_IF,
                     QTVDIAG_PRIO_MED,
                     " VDL_Flush_Stats_Queue: Stats buffers flushed: %d", 
                     stats_flushed);
    
    }while (stats_ptr);
  }
  return pending_stats_count;
}

typedef struct
{
   unsigned short PacketId;            /* Should be MP4_FRAME_HEADER_PACKET_TYPE */
   unsigned short EndMarker;           /* Should be MP4_MB_END_MARKER            */
} VDL_Fake_Eos_Header_Type;

VDL_ERROR VDL_RTOS::VDL_Queue_Fake_EOS_Slice() {
	if (m_state != ERROR)
	{
		VDL_Slice_Pkt_Type *fake_eos_slice = VDL_Get_Slice_Buffer();
		VDL_Fake_Eos_Header_Type fake_eos_header;
		DecodeStatsType *pBuf = NULL;
		VDL_ERROR retVal = VDL_ERR_NONE;
		if(fake_eos_slice) {
			fake_eos_slice->pNextMacroBlock = (unsigned short *) fake_eos_slice->pSliceData;
			fake_eos_slice->NumMacroBlocks = 1;
			fake_eos_slice->pFrameStatsInPtr = NULL;
			fake_eos_slice->pFrameStatsOutPtr = NULL;
			fake_eos_slice->isStatsFromL2 = TRUE;
			fake_eos_slice->SliceBufferSize = VDL_DSP_VLD_Slice_Buffer_Size;

			fake_eos_slice->isSubFrameDone = 0;
			fake_eos_slice->isFlushMarker = 0;
			fake_eos_slice->SliceSeqNum = 0;

			if( VideoFormatType == VDL_VIDEO_H264) {
	                      fake_eos_header.PacketId = 0xB203;
                        } else {
                                fake_eos_header.PacketId = 0xBD03;
                        }

			fake_eos_header.EndMarker = 0x7FFF;

			fake_eos_slice->fAllocateYUVBuf = TRUE;
			fake_eos_slice->fFirstMB = TRUE;

			fake_eos_slice->fLastMB = TRUE;
			//fake_eos_slice->CodecType = VDL_CODEC_H264_VLD;
			memcpy (fake_eos_slice->pSliceData, &fake_eos_header, sizeof( VDL_Fake_Eos_Header_Type ));
			fake_eos_slice->SliceDataSize = sizeof( VDL_Fake_Eos_Header_Type );
			QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK,QTVDIAG_PRIO_FATAL,"Queueing EOS For Flush slice buffer\n");


			pBuf =  (DecodeStatsType*) QTV_Malloc(sizeof(DecodeStatsType));
			if(pBuf) {
			  VDL_Queue_Stats_Buffer(pBuf, VDL_END_OF_SEQUENCE_FOR_FLUSH);
			  retVal = VDL_Queue_Slice_Buffer(fake_eos_slice);
			} else {
			  QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK,QTVDIAG_PRIO_FATAL,"No Memory\n");
			  retVal = VDL_ERR_OUT_OF_MEMORY;
			}
		} else {
			QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK,QTVDIAG_PRIO_FATAL,"Couldnot get slice\n");
			retVal = VDL_ERR_MYSTERY;
		}
		return VDL_ERR_NONE;
	}

	QTV_MSG_PRIO(QTVDIAG_VIDEO_TASK,QTVDIAG_PRIO_FATAL,"In ERROR state, Should not queue fake EOS slice to q5");
	return VDL_ERR_INVALID_STATE;
}


VDL_ERROR VDL_RTOS::VDL_Flush()
{
  //QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF,QTVDIAG_PRIO_HIGH,"flush: VDL_Cur_QDSP_Sub_Frame_Queue_Depth : %d", VDL_Cur_QDSP_Sub_Frame_Queue_Depth);
  /**********************************************************************
  0. Synchronous implentation of Flush
  1. Flush all the slices from the slice_q
  2. Flush all the stats buffers from the stats_q
  3. If there is a pending slice (slice_dsp_q) awating frame done then wait 
     until signalled via msg_cb..
  4. If no signal comes, it means something has gone wrong with dsp, so 
     we need to clear the slice q and call callbacks for the stats q.  
  ***********************************************************************/
 
  QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED, "VDL_Flush()");
  int              pending_stats_count = 0;
  int result = 0;
  struct timespec   ts;
  struct timeval    tv;
  VDL_ERROR retVal = VDL_ERR_NONE;

  /* This condition is no longer required. We should be able to flush in 
   * idle state. Reason being, say if the dsp is in some bad state because 
   * of which we did not get active ack so we are not able to send slices 
   * to dsp. The decoder will hang saying out of buffers. Now, if the user
   * tries to kill the app flush will be called in idle state and we need 
   * to return the buffers to upper layer.
  
  if (m_state == IDLE )
  {
    QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED, "Cannot flush in IDLE state");
    return VDL_ERR_NONE;
  }
  */
  /*To keep it simple during flush operation we protect against sending any new slices or
  ** processing frame_decode_dones */
  QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED, "lock flush processing\n");;
  pthread_mutex_lock(&lockFlushProcessing);
  /*Block sending slices while we do this cleanup*/
  QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED, "lock Send Slice\n");;
  pthread_mutex_lock(&lockSendSlice);

  QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED, "Flush all Slice queues");
  VDL_Flush_Slice_Queues();
  QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED, "Flush all Stats queue");
  m_pending_frame_dones = VDL_Flush_Stats_Queue();             

  pthread_mutex_unlock(&lockSendSlice);
  pthread_mutex_unlock(&lockFlushProcessing);

  /* Wait on the condition variable until signalled by the aDSP */
  pthread_mutex_lock(&m_pending_frame_done_lock);
  while (m_pending_frame_dones) {
    gettimeofday(&tv, 0);
    /* Convert from timeval to timespec */
    ts.tv_sec  = tv.tv_sec;
    ts.tv_nsec = tv.tv_usec * 1000;
    /*Wait no longer than 200msec*/
    //ts.tv_nsec += (200000000);
    if((ts.tv_nsec + 200000000) < 1000000000)
    {
      ts.tv_nsec += (200000000);
    }
    else
    {
      //increment the sec
      ts.tv_sec  += 1;
      //incr the nsec accordingly
      ts.tv_nsec = (ts.tv_nsec + 200000000 - 1000000000);
    }

    QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH, "Waiting on all FrameDone event, \
                                                         m_pending_frame_dones=%d",m_pending_frame_dones);
    result = pthread_cond_timedwait(&m_all_frame_done_cond, 
                                    &m_pending_frame_done_lock,
                                    &ts);

    if(result == ETIMEDOUT)
    {
      QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_ERROR, "Waiting on all FrameDone timedout..!");
      break;
    }
  }
  pthread_mutex_unlock(&m_pending_frame_done_lock);

  if(result == ETIMEDOUT) {
	  pthread_mutex_lock(&m_pending_frame_done_lock);
	  QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_ERROR, "Queue Fake EOS\n");
	  retVal = VDL_Queue_Fake_EOS_Slice();
	  if(retVal == VDL_ERR_NONE) {
		  //QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_ERROR, "m_pending_frame_dones1  = %d\n", m_pending_frame_dones);
		  //printf("m_pending_frame_dones1  = %d\n", m_pending_frame_dones);
		  ++m_pending_frame_dones;
		  while (m_pending_frame_dones) {
			  gettimeofday(&tv, 0);
			  // Convert from timeval to timespec 
			  ts.tv_sec  = tv.tv_sec;
			  ts.tv_nsec = tv.tv_usec * 1000;
			  //ts.tv_nsec += (200000000);
			  if((ts.tv_nsec + 200000000) < 1000000000)
			  {
				  ts.tv_nsec += (200000000);
			  }
			  else
			  {
				  //increment the sec
				  ts.tv_sec  += 1;
				  //incr the nsec accordingly
				  ts.tv_nsec = (ts.tv_nsec + 200000000 - 1000000000);
			  }

			  QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH, "Waiting on all FrameDone event, \
					  m_pending_frame_dones=%d",m_pending_frame_dones);
			  result = pthread_cond_timedwait(&m_all_frame_done_cond, 
					  &m_pending_frame_done_lock,
					  &ts);

			  if(result == ETIMEDOUT)
			  {
				  QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_ERROR, "Waiting on all FrameDone timedout..2!");
				  //printf("Waiting on all FrameDone timedout..2!");
				  break;
			  }
		  }
		  QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_ERROR, "m_pending_frame_dones2  = %d\n", m_pending_frame_dones);
	  }
	  pthread_mutex_unlock(&m_pending_frame_done_lock);
  }
  /* Since DSP went into an irrecoverable state so we need to 
   * clear the Slice q and stats q.
   */                       	
  if(result == ETIMEDOUT) {
	  pthread_mutex_lock(&lockSendSlice);
	  pthread_mutex_lock(&lockFlushProcessing);
	  VDL_Flush_All_Slices();
	  VDL_Flush_All_Stats();
	  VDL_Statistics.numFramesRxdFromDSP = VDL_Statistics.numFramesSentToDSP;
	  pthread_mutex_unlock(&lockFlushProcessing);
	  pthread_mutex_unlock(&lockSendSlice);
  }
  QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF,QTVDIAG_PRIO_LOW,"After flush: q_cnt(slice q): %d",q_cnt(&VDL_slice_q));
  QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF,QTVDIAG_PRIO_LOW,"After flush: q_cnt(slice dsp q): %d",q_cnt(&VDL_slice_dsp_q));
  QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF,QTVDIAG_PRIO_LOW,"After flush: q_cnt(slice sent q): %d",q_cnt(&VDL_slice_sent_q));
  QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF,QTVDIAG_PRIO_LOW,"After flush: q_cnt(stats q):%d",q_cnt(&VDL_stats_q));
  return VDL_ERR_NONE;
}

VDL_ERROR VDL_RTOS::VDL_Suspend()
{
  return VDL_ERR_NONE;
}

VDL_ERROR VDL_RTOS::VDL_Resume(VDL_Concurrency_Type ConcurrencyFormat)
{
  return VDL_ERR_NONE;
}

VDL_ERROR VDL_RTOS::VDL_Terminate()
{
  QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED,"VDL Terminate ");
  //Disable the VIDEO_Task
  if((m_state == READY) || (m_state == ERROR))
  {
    adsp_rtos_disable(QDSP_MODULE_VIDEOTASK);
#ifdef FEATURE_TURBO_MODULE
    if ((qdsp_module == QDSP_MODULE_VIDEO_AAC_VOC_TURBO) || (qdsp_module == QDSP_MODULE_VIDEO_AMR_TURBO))
    {
      adsp_rtos_disable(qdsp_module);
    }
#endif
  }

#if 1 // DEBUG Prints crashing 
    QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH, 
               "***VDL_Statistics.numSubFramesSentToDSP = %lld",VDL_Statistics.numSubFramesSentToDSP);
    QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH, 
               "***VDL_Statistics.numSubFramesDoneFromDSP = %lld",VDL_Statistics.numSubFramesDoneFromDSP);
    QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH, 
               "***VDL_Statistics.numFramesSentToDSP = %lld",VDL_Statistics.numFramesSentToDSP);
    QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_HIGH, 
               "***VDL_Statistics.numFramesRxdFromDSP = %lld",VDL_Statistics.numFramesRxdFromDSP);
#endif

  //QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF,QTVDIAG_PRIO_HIGH,"terminate: VDL_Cur_QDSP_Sub_Frame_Queue_Depth : %d", VDL_Cur_QDSP_Sub_Frame_Queue_Depth);
  VDL_Decode_Stats_Packet_Type *pStatsPkt = NULL;
  VDL_Slice_Pkt_Type          *pSlicePkt = NULL;
#if 1

  /* delete all the slice buffers from all the slice queues */
  while ((pSlicePkt = (VDL_Slice_Pkt_Type *) (q_get (&VDL_slice_q))) != NULL)
  {
    QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED, "Free Slice: %x",pSlicePkt);
    VDL_Free_Slice_Buffer( pSlicePkt );
  }
  
  /* remove all queued slice buffers in the wait Q */
  while ((pSlicePkt = (VDL_Slice_Pkt_Type *) (q_get (&VDL_slice_dsp_q))) != NULL)
  {
    QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED, "Free DSP Slice: %x",pSlicePkt);
    VDL_Free_Slice_Buffer( pSlicePkt );
  }
  q_destroy(&VDL_slice_dsp_q);
  QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED, "Free Slice complete");

  QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED, "Deleting stats buffers");
  /* free the stats buffers from the stats queue if any */
  while ((pStatsPkt = (VDL_Decode_Stats_Packet_Type *) (q_check (&VDL_stats_q))) != NULL)
  {
    //    QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED,"Flushed CB");
    //    VDL_Codec_Info.DecoderCB( VDL_FRAME_FLUSHED,
    //                             pStatsPkt->DecodeStats.pStatsData,
    //                             VDL_Codec_Info.DecoderUserData );
    QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED,"Free Stats buffer: %x",pStatsPkt);
    VDL_Free_Stats_Buffer(pStatsPkt);
  }       
  q_destroy(&VDL_stats_q);

  VDL_Delete_Stats_Buffers( &VDL_stats_free_q );
  q_destroy(&VDL_stats_free_q);

  QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED,"Delete Slice Buffers");
  VDL_Delete_Slice_Buffers( &VDL_slice_free_q );
  q_destroy(&VDL_slice_free_q);
  q_destroy(&VDL_slice_q);

  q_destroy(&VDL_slice_sent_q);
#endif
  return VDL_ERR_NONE;
}

void VDL_RTOS::VDL_Free_Stats_Buffer(VDL_Decode_Stats_Packet_Type *pCurrentDecodeStats )
{
  QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_MED, "VDL_Free_Stats_Buffer() - Moving stats buffer from stats_q into stats_free_q");
  if (NULL == pCurrentDecodeStats) 
  {
     QTV_MSG_PRIO(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_ERROR, "VDL_RTOS_VLD_DSP::VDL_Free_Stats_Buffer - pCurrentDecodeStats is NULL");
     return;
  }
  q_delete( &VDL_stats_q, &pCurrentDecodeStats->Link ); 
  memset( pCurrentDecodeStats, 0, sizeof( VDL_Decode_Stats_Packet_Type ) );
  q_put( &VDL_stats_free_q, &pCurrentDecodeStats->Link );
}

/*===========================================================================
Function: VDL_Delete_Slice_Buffers

Description: Release all the memory held by slice buffers in the given queue.

DEPENDENCIES
  None.

INPUT/OUTPUT PARAMETERS:
  slice buffer queue

RETURN VALUE
  None.

SIDE EFFECTS
  None.

============================================================================*/
void VDL_RTOS::VDL_Delete_Slice_Buffers( q_type * const q )
{
  VDL_Slice_Pkt_Type * pPkt;

  QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED, "VDL_Delete_Slice_Buffers() slice_q:%x", q);

  ASSERT( q );

  while ( ( pPkt = ( VDL_Slice_Pkt_Type * )q_get( q ) ) != NULL )
  {
    ASSERT( pPkt->pSliceData ); 

    //Memory would be freed by the client
    //VDL_Deallocate_Slice_Buffer(pPkt->pSliceData);

    QTV_Free( pPkt );    
    --num_slice_buffers;
  }
  if(num_slice_buffers != 0) {
    QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_ERROR, "Leaked :%d bytes\n", num_slice_buffers * sizeof(VDL_Slice_Pkt_Type));
  }
}

/*===========================================================================
Function: VDL_Delete_Stats_Buffers

Description: Release all the memory held by stats buffers in the given queue.

DEPENDENCIES
  None.

INPUT/OUTPUT PARAMETERS:
  stats buffer queue

RETURN VALUE
  None.

SIDE EFFECTS
  None.

============================================================================*/
void VDL_RTOS::VDL_Delete_Stats_Buffers( q_type * const q )
{
  VDL_Decode_Stats_Packet_Type * pPkt;

  QTV_MSG_PRIO1(QTVDIAG_DEC_DSP_IF, QTVDIAG_PRIO_MED, "VDL_Delete_Stats_Buffers() stats_q:%x", q);

  ASSERT( q );

  while ( ( pPkt = ( VDL_Decode_Stats_Packet_Type * )q_get( q ) ) != NULL )
  {
    QTV_Free( pPkt );    
    --num_stats_buffers;
  }
  ASSERT(num_stats_buffers == 0);
}

unsigned char VDL_RTOS::VDL_IsSliceBufAvailableForDecode()
{
  if(q_cnt(&VDL_slice_free_q))
  {
    return TRUE;
  }
  return FALSE;
}

unsigned char VDL_RTOS::VDL_Set_QDSP_Module(adsp_rtos_module_type module)
{
  if (module >= QDSP_MODULE_MAX) 
  {
    return FALSE;
  }
  qdsp_module = module;
  return TRUE;
}


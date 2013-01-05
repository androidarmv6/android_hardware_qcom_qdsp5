/* Copyright (c) 2007 by QUALCOMM, Incorporated.  All Rights Reserved. 
 * QUALCOMM Proprietary and Confidential.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

extern "C"
{
   #include "pmem.h"
}
#include "qtv_msg.h"
#include "vdecoder_types.h"
#include "vdecoder_i.h"
#include "qutility.h"
#include "qtvsystem.h"
#include "vdec.h"

#define NEED_VDEC_LP 0

#ifndef T_WINNT
#ifdef _ANDROID_
	extern "C" {                   
		#include <utils/Log.h>
	}
#endif //_ANDROID_
#endif

#ifndef T_WINNT
    #include <sys/ioctl.h>
    #include <unistd.h>
    #ifndef QLE_BUILD
        #include <inttypes.h>
        #include <linux/msm_adsp.h>
    #endif //QLE_BUILD
#endif //T_WINNT

#ifdef PROFILE_DECODER
unsigned int qperf_total_frame_cnt = 0;
#endif

#ifdef T_WINNT
#define LOG_YUV_FRAMES
#endif


struct VDecoder
{
    unsigned arena_size;
    struct pmem arena;
    
    int dead;

    // shared with core 
    unsigned width;
    unsigned height;

    struct vdec_frame input[VDEC_NFRAMES];  // VDEC_INPUT_SIZE
    struct vdec_frame output[MAX_OUT_BUFFERS]; // w * h * 1.5
    void *core;
    void *adsp_module;

    void *fake;

    struct vdec_context *ctxt;
};



VDecoder* vdec_decoder_info;
unsigned int vdec_output_frame_index;
VDEC_SLICE_BUFFER_INFO slice_buffer_info[VDEC_NFRAMES];

QPERF_INIT(arm_decode);
QPERF_INIT(frame_data);

#ifdef LOG_YUV_FRAMES
FILE * pYUVFile;
#endif
#ifdef LOG_INPUT_BUFFERS
FILE * pInputFile;
static int counter = 0;
#endif

extern VideoDecoder::VDEC_CREATION_FUNC      pCreateFnMpeg4;
extern VideoDecoder::VDEC_CREATION_FUNC      pCreateFnH264;
extern VideoDecoder::VDEC_CREATION_FUNC      pCreateFnWmv;
int timestamp = 0;
static unsigned int nFrameDoneCnt = 0;
static unsigned int nGoodFrameCnt = 0;


void core_msg_func(void *cookie, unsigned msgid,
                   void *data, unsigned length)
{
	QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"vdec: core_msg_func()\n");
}
static void vdec_input_buffer_release_cb_handler(const VDEC_STREAM_ID,
                                                 VDEC_INPUT_BUFFER * const pInput,
                                                 void * const cookie )
{
  QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"[vdec_core] vdec_frame_buffer_release_cb_handler()\n");
  vdec_decoder_info->ctxt->buffer_done(vdec_decoder_info->ctxt, cookie);
  return;
}
static void* vdec_frame_buffer_malloc_cb_handler (const VDEC_STREAM_ID,
                                                  void *pContext,
                                                  unsigned long int size)
{
	QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_LOW,"vdec: vdec_frame_buffer_malloc_cb_handler()\n");
  if(vdec_decoder_info->ctxt->outputBuffer.numBuffers <= vdec_output_frame_index)
  {
	  QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,"vdec: frame buffer malloc failed, index: %d\n", vdec_output_frame_index);
    return NULL;
  }
  else
  {
    //return (void*)vdec_decoder_info->output[vdec_output_frame_index++].phys;
    return (void*)vdec_decoder_info->output[vdec_output_frame_index++].base;
  }
}

static void vdec_frame_buffer_free_cb_handler (const VDEC_STREAM_ID stream,
                                               void *pContext,
                                               void* ptr)
{
	QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"vdec: vdec_frame_buffer_free_cb_handler()\n");

  // Callbacks from vdec to free the frame buffer are  not handled at this level.
  // Simply return.
  return;
}

void vdec_frame_cb_handler ( const VDEC_STREAM_ID /*stream*/,
                             VDEC_CB_STATUS       status,
                             VDEC_FRAME * const   pFrame,
                             unsigned long int               /*nBytesConsumed*/,
                             void * const         pCbData )
{
  int index;
 
  ++nFrameDoneCnt;
  QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"vdec: frame done cb cnt: %d", nFrameDoneCnt); 
  switch(status)
  {
    case VDEC_STATUS_GOOD_FRAME:
    case VDEC_STATUS_FLUSH:
    {
      if (NULL == pFrame)
      {
		  QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_HIGH,"vdec: pFrame parameter is NULL, dropping frame\n");
        break;
      }

      //Iterate through the output frame array to locate corresponding index
      for (index=0; index<vdec_decoder_info->ctxt->outputBuffer.numBuffers; index++)
      {
        //if (pFrame->pBuf == (VDEC_BYTE*)vdec_decoder_info->output[index].phys)
        if (pFrame->pBuf == (VDEC_BYTE*)vdec_decoder_info->output[index].base)
        {
           break;
        }
      }
      if (vdec_decoder_info->ctxt->outputBuffer.numBuffers == index)
      {
		  QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_HIGH,"vdec: error: unable to map address %p, dropping frame\n", pFrame->pBuf);
      }
      else
      {
        if(VDEC_STATUS_FLUSH == status)
        {
          vdec_decoder_info->output[index].flags |= FRAME_FLAG_FLUSHED;
        }
        else
        {
          ++nGoodFrameCnt;
#ifdef PROFILE_DECODER
          ++qperf_total_frame_cnt;
#endif
          vdec_decoder_info->output[index].flags = 0;
          QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_LOW,"vdec: callback status good frame, cnt: %d\n", nGoodFrameCnt);
        
#ifdef LOG_YUV_FRAMES
          if (pYUVFile)
          {
            int size = vdec_decoder_info->width*vdec_decoder_info->height*1.5;
        fwritex(vdec_decoder_info->output[index].base, size, pYUVFile);
          }
#endif
        }
        if(nGoodFrameCnt >1) 
        {
          QPERF_END(frame_data);
        }
	QPERF_START(frame_data);
        vdec_decoder_info->output[index].timestamp = pFrame->timestamp;
	if(pFrame->frameType == VDEC_FRAMETYPE_I)
        {
            vdec_decoder_info->output[index].frameDetails.ePicType[0] = VDEC_PICTURE_TYPE_I;
        }
        if(pFrame->frameType == VDEC_FRAMETYPE_B)
        {
            vdec_decoder_info->output[index].frameDetails.ePicType[0] = VDEC_PICTURE_TYPE_B;
        }
        if(pFrame->frameType == VDEC_FRAMETYPE_P)
        {
            vdec_decoder_info->output[index].frameDetails.ePicType[0] = VDEC_PICTURE_TYPE_P;
        }
        vdec_decoder_info->ctxt->frame_done(vdec_decoder_info->ctxt, &vdec_decoder_info->output[index]);
      }
      break;
    }

    case VDEC_STATUS_DECODE_ERROR:
    {
		QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,"vdec: callback status decode error\n");
      break;
    }

    case VDEC_STATUS_FATAL_ERROR:
    {
      QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_FATAL,"vdec: callback status error fatal\n");
      //Iterate through the output frame array to locate corresponding index
      for (index=0; index<vdec_decoder_info->ctxt->outputBuffer.numBuffers; index++)
      {
        if (pFrame->pBuf == (VDEC_BYTE*)vdec_decoder_info->output[index].base)
        {
           break;
        }
      }
      if (vdec_decoder_info->ctxt->outputBuffer.numBuffers == index)
      {
        QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_FATAL,
                      "vdec: error: unable to map address %p for VDEC_STATUS_FATAL_ERROR, dropping frame\n", pFrame->pBuf);
      }
      else
      {
        QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_FATAL,"vdec: frame done index = %d\n",index);
        vdec_decoder_info->output[index].flags = FRAME_FLAG_FATAL_ERROR;
        vdec_decoder_info->ctxt->frame_done(vdec_decoder_info->ctxt, &vdec_decoder_info->output[index]);
      }
      break;
    }

    case VDEC_STATUS_VOP_NOT_CODED:
    {
		QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_HIGH,"vdec: callback status not coded\n");
      break;
    }

    case VDEC_STATUS_SUSPEND_DONE:
    {
		QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"vdec: callback status suspend done\n");
      break;
    }

    case VDEC_STATUS_EOS:
    {
      static struct vdec_frame frame;
      memset(&frame, 0, sizeof(frame));
      frame.flags |= FRAME_FLAG_EOS;
      QPERF_END(frame_data);
      vdec_decoder_info->ctxt->frame_done(vdec_decoder_info->ctxt, &frame);
      
	  QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_LOW,"vdec: callback status EOS\n");
      break;
    }

    default:
		QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_HIGH,"vdec: callback status unknown status\n");
      break;
  }

  return;
}

Vdec_ReturnType vdec_close(struct VDecoder *dec)
{
  VDEC_ERROR err;
  VideoDecoder* pDec = (VideoDecoder*)(dec->core);
  QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_LOW,"vdec: vdec_close()\n");
#ifdef PROFILE_DECODER
  usecs_t time_taken_by_arm = QPERF_TERMINATE(arm_decode);
  float avArmTime = (float)time_taken_by_arm/(qperf_total_frame_cnt*1000);
  usecs_t frame_data_time = QPERF_TERMINATE(frame_data);
  QTV_PERF_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_FATAL,"===========================================================\n");
  QTV_PERF_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_FATAL,"                     Arm Statistics                        \n");
  QTV_PERF_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_FATAL,"===========================================================\n");
  QTV_PERF_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_FATAL,"Total number of frames decoded = %ld\n",qperf_total_frame_cnt);
  QTV_PERF_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_FATAL,"Average Arm time/frame(ms)     = %f\n",avArmTime);
  QTV_PERF_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_FATAL,"Frames Arm Decoded/sec         = %f\n",1000/avArmTime);
  QTV_PERF_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_FATAL,"===========================================================\n");
  QTV_PERF_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_FATAL,"                    Frame Done Statistics                  \n");
  QTV_PERF_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_FATAL,"===========================================================\n");
  QTV_PERF_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_FATAL,"Frame done cumulative time     = %lld\n",frame_data_time);
  QTV_PERF_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_FATAL,"Frames Done per second         = %f\n",(float)(qperf_total_frame_cnt-1)*1000000/frame_data_time);
  QTV_PERF_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_FATAL,"===========================================================\n");
#endif
#ifdef LOG_YUV_FRAMES
  if (pYUVFile)
  {
    fclose (pYUVFile);
	pYUVFile = NULL;
  }
#endif
#ifdef LOG_INPUT_BUFFERS
  if (pInputFile)
  {
    fclose (pInputFile);
  }
#endif
  vdec_output_frame_index = 0;

#if NEED_VDEC_LP
  if (vdec->fake)
  {
    //jlk - adsp_close() calls adsp_disable right now.  Calling adsp_disable() twice causes problems
    //Renable this line when we fix the kernel driver
    //adsp_disable((adsp_module*)vdec->fake);
    adsp_close((adsp_module*)vdec->fake);
  }
  else
  {
	  QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_HIGH,"vdec: adsp modules is NULL\n");
  }
#endif
  nFrameDoneCnt = 0;
  nGoodFrameCnt = 0;

  if (dec->core)
  {
	QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_LOW,"vdec: calling Suspend");
    err = pDec->Suspend( );
    if (VDEC_ERR_EVERYTHING_FINE != err)
    {
		QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,"vdec: Suspend returned error: %d\n", (int)err);
    }
	QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_LOW,"vdec: calling vdec_destroy"); 
	QTV_Delete( (VideoDecoder*)(dec->core) );
  }
  else
  {
	  QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_HIGH,"vdec: core is NULL");
  }
    pmem_free(&dec->arena);
	free(dec);
	QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"vdec: closed\n");
	return VDEC_SUCCESS;
}
Vdec_ReturnType vdec_commit_memory(struct VDecoder* dec)
{
  unsigned frame_size = 0;
  unsigned output_sz = 0;
  unsigned input_sz = VDEC_INPUT_SIZE;
  unsigned off = 0;
  int n;
  int r;
  
  frame_size = dec->width * dec->height;
  output_sz = frame_size * 3/2;
  if (dec->ctxt->inputReq.bufferSize > VDEC_INPUT_SIZE)
  {
    input_sz = dec->ctxt->inputReq.bufferSize;
  }

  dec->arena_size = VDEC_NFRAMES * VDEC_INPUT_SIZE + dec->ctxt->outputBuffer.numBuffers * output_sz;
  if (pmem_alloc(&dec->arena, dec->arena_size)) {
    QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,"vdec: failed to allocate pmem arena (%d bytes)\n",
                  dec->arena_size);
    return VDEC_EFAILED;
  }
	QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"vdec: allocated %d bytes of pmem\n", dec->arena_size);
  off = 0;
  for (n = 0; n < VDEC_NFRAMES; n++) {
    struct vdec_frame *fr = dec->input + n;
    fr->pmem_id = dec->arena.fd;
    fr->pmem_offset = off;
    fr->phys = dec->arena.phys + off;
    fr->base = ((unsigned char*) dec->arena.data) + off;
    off += input_sz;
    QTV_MSG_PRIO5(QTVDIAG_GENERAL,QTVDIAG_PRIO_LOW,"      input[%d]  base=%p phys=0x%08x off=0x%08x id=%d\n",
                  n, fr->base, fr->phys, fr->pmem_offset, fr->pmem_id);
  }
  for (n = 0; n < dec->ctxt->outputBuffer.numBuffers; n++) {
    struct vdec_frame *fr = dec->output + n;
    fr->pmem_id = dec->arena.fd;
    fr->pmem_offset = off;
    fr->phys = dec->arena.phys + off;
    fr->base = ((unsigned char*) dec->arena.data) + off;
    off += output_sz;
    memset(fr->base,0,frame_size);
    memset(&(((unsigned char*)fr->base)[frame_size]),128,frame_size>>1);
    QTV_MSG_PRIO5(QTVDIAG_GENERAL,QTVDIAG_PRIO_LOW,"      output[%d] base=%p phys=0x%08x off=0x%08x id=%d\n",
                  n, fr->base, fr->phys, fr->pmem_offset, fr->pmem_id);
  }

  dec->ctxt->inputBuffer.base        =(unsigned char*) (dec->input[0].base);
  dec->ctxt->inputBuffer.bufferSize  = input_sz;
  dec->ctxt->inputBuffer.numBuffers  = VDEC_NFRAMES;
  dec->ctxt->outputBuffer.base       = (unsigned char*) &(dec->output[0]); 
  dec->ctxt->outputBuffer.bufferSize = output_sz;
  return VDEC_SUCCESS;
}
struct VDecoder *vdec_open(struct vdec_context *ctxt)
{
  struct VDecoder *dec = NULL;
  VDEC_ERROR err = 0;
  const VDEC_CONCURRENCY_CONFIG concurrencyConfig = VDEC_CONCURRENT_NONE;
  VideoDecoder* pDec = NULL;
  dec = (VDecoder*)calloc(1, sizeof(struct VDecoder));
  if (!dec) { 
    return 0;
  }
  dec->ctxt = ctxt;
  dec->width = ctxt->width;
  dec->height = ctxt->height;
  dec->ctxt->outputBuffer.numBuffers = ctxt->outputBuffer.numBuffers;
  if(VDEC_SUCCESS != vdec_commit_memory(dec)) {	
    return 0;
  }

  QPERF_RESET(arm_decode);
  QPERF_RESET(frame_data);
  nFrameDoneCnt = 0;
  nGoodFrameCnt = 0;
#ifdef PROFILE_DECODER
  qperf_total_frame_cnt = 0;
#endif

  vdec_output_frame_index = 0;
  timestamp = 0;
  int i;
  VDEC_PARAMETER_DATA codeDetectionEnable;
  codeDetectionEnable.startCodeDetection.bStartCodeDetectionEnable = false; // by default set to false; MPEG4 doesnt require it

  QTV_MSG_PRIO2(QTVDIAG_GENERAL,QTVDIAG_PRIO_LOW,"vdec: vdec_open(). width: %d, height: %d\n", dec->width, dec->height);

  vdec_decoder_info = dec;
  QTV_MSG_PRIO3(QTVDIAG_GENERAL,QTVDIAG_PRIO_LOW,"vdec: vdec_open(). width: %d, height: %d kind[%s]\n", 
                                                   vdec_decoder_info->ctxt->width, vdec_decoder_info->ctxt->height, 
                                                   vdec_decoder_info->ctxt->kind);

  if(!strcmp(vdec_decoder_info->ctxt->kind,"OMX.qcom.video.decoder.avc"))
  {
	dec->core = reinterpret_cast<VDEC_STREAM_ID>(pCreateFnH264(&err));
	QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_LOW,"vdec: Creating H264 Decoder [%p]\n",dec->core);
	VDEC_PARAMETER_DATA sizeOfNalLengthField;
        sizeOfNalLengthField.sizeOfNalLengthField.sizeOfNalLengthField = dec->ctxt->size_of_nal_length_field;
		QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_LOW,"vdec: NAL lenght [%d]\n",dec->ctxt->size_of_nal_length_field);
        pDec = (VideoDecoder*)(dec->core);
        if (0 == dec->ctxt->size_of_nal_length_field) 
        {
		   QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_LOW,"vdec: START CODE....\n");
	       codeDetectionEnable.startCodeDetection.bStartCodeDetectionEnable = true;
		   if(!pDec) 
	          err = VDEC_ERR_NULL_STREAM_ID;
	       else
		      err = pDec->SetParameter(VDEC_PARM_START_CODE_DETECTION,&codeDetectionEnable);
           if (VDEC_ERR_EVERYTHING_FINE != err)
           {
		      // TBD- printx("[vdec_core] set start code detection parameter failed: %d", (int)err);
		      QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,"[vdec_core] set start code detection parameter failed: %d", (int)err);
              goto fail_initialize;
           }
        }
        else if(dec->ctxt->size_of_nal_length_field > 0 && dec->ctxt->size_of_nal_length_field <= 4)
        {
			QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,"vdec: NALU LENGTH[%d]\n",dec->ctxt->size_of_nal_length_field);
           // test size of NAL length field decoder support 
	       if(!pDec) 
	          err = VDEC_ERR_NULL_STREAM_ID;
	       else
             err = pDec->SetParameter( VDEC_PARM_SIZE_OF_NAL_LENGTH_FIELD, &sizeOfNalLengthField );
	       if (VDEC_ERR_EVERYTHING_FINE != err)
           {
             // TBD- printx("[vdec_core] set start code detection parameter failed: %d", (int)err);
             goto fail_initialize;
           }
	    }
	    else
        {
			QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,"vdec: Invalid size of nal length field: %d\n", dec->ctxt->size_of_nal_length_field);
            goto fail_core;
        } 
  }
  else if ((!strcmp(vdec_decoder_info->ctxt->kind,"OMX.qcom.video.decoder.mpeg4")) 
	       || (!strcmp(vdec_decoder_info->ctxt->kind,"OMX.qcom.video.decoder.h263")))
  {
	  dec->core = reinterpret_cast<VDEC_STREAM_ID>(pCreateFnMpeg4(&err));
	  pDec = (VideoDecoder*)(dec->core);
	  QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_LOW,"vdec: Creating MP4 Decoder [%p]\n",dec->core);
  }
  else if (!strcmp(vdec_decoder_info->ctxt->kind,"OMX.qcom.video.decoder.vc1"))
  {
	  dec->core = reinterpret_cast<VDEC_STREAM_ID>(pCreateFnWmv(&err));
	  pDec = (VideoDecoder*)(dec->core);
	  QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_LOW,"vdec: Creating WMV Decoder [%p]\n",dec->core);
  }
  else
  { 
	QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,"Incorrect codec kind\n");
    goto fail_core;
  }

  if (VDEC_ERR_EVERYTHING_FINE != err || NULL == dec->core)
  {
	  QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,"vdec: create failed\n");
      goto fail_core;
  }

  for(i=0; i<VDEC_NFRAMES; i++)
  {
    slice_buffer_info[i].base = (char*)dec->input[i].base;
    //slice_buffer_info[i].phys = dec->input[i].phys;
    slice_buffer_info[i].fd = dec->input[i].pmem_id;
  }
  
  VDEC_DIMENSIONS frameSize;
  frameSize.height = dec->height;
  frameSize.width = dec->width;
  QTV_MSG_PRIO2(QTVDIAG_GENERAL,QTVDIAG_PRIO_HIGH,"vdec: Init width: %d height %d\n", frameSize.width, frameSize.height);

/*  
  VDEC_PARAMETER_DATA deblockerInfo;
  deblockerInfo.deblockerEnable.bDeblockerEnable = true;

  QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_HIGH,"[vdec_core] SetParameter\n"); 
  err = vdec_set_parameter(dec->core, VDEC_PARM_DEBLOCKER_ENABLE, &deblockerInfo); 
  QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_HIGH,"[vdec_core] after SetParameter\n",err); 
  if (VDEC_ERR_EVERYTHING_FINE != err)
  {
    //TBD - printx("[vdec_core] set deblocker enable parameter failed: %d", (int)err);
    goto fail_initialize;
  }
*/

 
  QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_HIGH,"vdec: vdec_initialize\n"); 
  if(!pDec) 
	  err = VDEC_ERR_NULL_STREAM_ID;
  else
  {
      err = pDec->Initialize(1,
			                 vdec_frame_buffer_malloc_cb_handler,
				             vdec_frame_buffer_free_cb_handler,
                             vdec_frame_cb_handler,
				             NULL,
				             frameSize,
				             VDEC_CONCURRENT_NONE,
				             slice_buffer_info,
					     dec->ctxt->outputBuffer.numBuffers,
					     ctxt->adsp_fd);
  }

  QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_HIGH,"vdec: vdec_initialize\n",err); 
  if (err != VDEC_ERR_EVERYTHING_FINE)
  {
	  QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,"vdec: initialization failed: %d\n", (int)err);
      goto fail_initialize;
  }

#if NEED_VDEC_LP
#ifdef _ANDROID_
  dec->fake = adsp_open("/dev/adsp/VDEC_LP_MODE", dec->core, core_msg_func);
#else
  dec->fake = adsp_open("/dev/VDEC_LP_MODE", dec->core, core_msg_func);
#endif //_ANDROID_
  if (!dec->fake)
  {
	  QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,"vdec: adsp_open() failed error: %s\n", strerror(errno));
    goto fail_initialize;
  }
  if (adsp_enable((adsp_module*)dec->fake)) goto fail_enable;
#endif

#ifdef LOG_YUV_FRAMES
#ifdef T_WINNT
  pYUVFile = fopen ( "../debug/yuvframes.yuv" , "wb" );
#elif _ANDROID_
  pYUVFile = fopen ( "/data/yuvframes.yuv" , "wb" );
#else
  pYUVFile = fopen ( "yuvframes.yuv" , "wb" );
#endif
#endif

#ifdef LOG_INPUT_BUFFERS
#ifdef T_WINNT
  pInputFile = fopen ( "../debug/inputbuffers.bin" , "wb" );
#elif _ANDROID_
  pInputFile = fopen ( "/data/inputbuffers.bin" , "wb" );
#else
  pInputFile = fopen ( "inputbuffers.bin" , "wb" );
#endif
#endif

  return dec;
fail_enable:
#if NEED_VDEC_LP
  adsp_close((adsp_module*)dec->fake);
#endif
fail_initialize:
  if(pDec) {
	  pDec->Suspend( );
	  QTV_Delete( pDec );
  }
fail_core:
  if (dec) {
    dec->dead = 1;
    free(dec);
  }
  return NULL;

}

Vdec_ReturnType vdec_post_input_buffer(struct VDecoder *dec, video_input_frame_info *frame, void *cookie)
{
  QTV_MSG_PRIO3(QTVDIAG_GENERAL,QTVDIAG_PRIO_LOW,"vdec: post_input data=%p len=%d cookie=%p\n", frame->data, frame->len, cookie);
#ifdef LOG_INPUT_BUFFERS
  static int take_input = 1;
#endif
  int fatal_err = 0;

  /*checkBufAvail flag is needed since we do not need to checkout
   * YUV/Slice buffer incase the NAL corresponds to same frame.
   * This is required for multiple NALs in one input buffer
   */
  bool checkBufAvail = true;
  VDEC_INPUT_BUFFER input;
  VideoDecoder *pDec = (VideoDecoder*)(dec->core);
  VDEC_ERROR err  = VDEC_ERR_EVERYTHING_FINE;
  if (NULL == dec || NULL == frame || NULL == frame->data )
  {
	  QTV_MSG_PRIO3(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,"vdec: error: encountered NULL parameter dec: 0x%x frame: 0x%x data: 0x%x\n", 
		   (unsigned int)dec, 
		   (unsigned int)frame,
		   (unsigned int)frame->data);
    return VDEC_EFAILED;
  }

  input.buffer[0]      = (unsigned char*)frame->data;
  input.timestamp[0]   = (long long)frame->timestamp;
  input.buffer_size[0] = (unsigned long int)frame->len;
  input.buffer_pos[0]  = 0;
  input.layers         = 1;
  input.eOSIndicator[0]= false;

  QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"vdec: received ts: %lld", frame->timestamp);
  if (frame->timestamp < timestamp )
  {
	  QTV_MSG_PRIO2(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"vdec: error: out of order stamp! %d < %d\n",
                  (int)(frame->timestamp&0xFFFFFFFF), timestamp);
  }
  timestamp = (int)frame->timestamp;

  QTV_MSG_PRIO2(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"vdec: vdec_core_post_input. buffer_size[0]: %ld frame->flags: 0x%x\n",
                input.buffer_size[0], frame->flags);

  if (input.buffer_size[0] == 0 && frame->flags & FRAME_FLAG_EOS)
  {
	  QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_LOW,"vdec: Zero-length buffer with EOS bit set\n");
          input.eOSIndicator[0] = true;

    if(pDec)
		err =  pDec->EOS( );
	else
		err = VDEC_ERR_NULL_STREAM_ID;

    if(VDEC_ERR_OUT_OF_BUFFERS == err) return VDEC_EOUTOFBUFFERS; 
    vdec_decoder_info->ctxt->buffer_done(vdec_decoder_info->ctxt, cookie);
    if (VDEC_ERR_EVERYTHING_FINE == err) return VDEC_SUCCESS;
    return VDEC_EFAILED;
  }

  QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_LOW,"vdec: vdec_core_post_input\n");

#ifdef LOG_INPUT_BUFFERS
  if (take_input)
  {
    fwritex((unsigned char*)frame->data, frame->len, pInputFile);
	QTV_MSG_PRIO2(QTVDIAG_GENERAL,QTVDIAG_PRIO_HIGH,"vdec: frame %d frame->len %d\n", counter++, frame->len);
  }
#endif

  do {
    QPERF_TIME(arm_decode, err = pDec->Decode( &input, checkBufAvail ));
    if (VDEC_ERR_EVERYTHING_FINE != err)
    {
		QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_HIGH,"vdec: vdec_decoder error: %d\n", (int)err);
		if(VDEC_ERR_UNSUPPORTED_DIMENSIONS == err) {
			fatal_err = 1;
			break;
		}
    }
    checkBufAvail = false;
  } while( ( VDEC_ERR_EVERYTHING_FINE == err ) && ( 0 != input.buffer_size[0] ) );

#ifdef LOG_INPUT_BUFFERS
  take_input = (err==14?0:1);
#endif
  if(VDEC_ERR_OUT_OF_BUFFERS == err) return VDEC_EOUTOFBUFFERS;
  vdec_input_buffer_release_cb_handler(pDec,&input,cookie);
  if(VDEC_ERR_EVERYTHING_FINE == err) return VDEC_SUCCESS;
  if(fatal_err) {
      static struct vdec_frame frame;
      memset(&frame, 0, sizeof(frame));
      frame.flags |= FRAME_FLAG_FATAL_ERROR;
      QPERF_END(frame_data);
      vdec_decoder_info->ctxt->frame_done(vdec_decoder_info->ctxt, &frame);
  }
  return VDEC_EFAILED;
}

Vdec_ReturnType vdec_release_frame(struct VDecoder *dec, struct vdec_frame *frame)
{
  VDEC_FRAME vdecFrame;
  VideoDecoder *pDec = (VideoDecoder*)(dec->core);
  QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_LOW,"vdec: release_frame %p\n", frame);  
  if (NULL == dec || NULL == frame)
  {
	  QTV_MSG_PRIO2(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,"vdec: error: encountered NULL parameter vdec: 0x%x frame: 0x%x", (unsigned int)dec, (unsigned int)frame);
      return VDEC_EFAILED;
  } 
  //vdecFrame.pBuf = (VDEC_BYTE*)frame->phys;
  vdecFrame.pBuf = (VDEC_BYTE*)frame->base;
  vdecFrame.timestamp = (unsigned long long)frame->timestamp;
  pDec->ReuseFrameBuffer(&vdecFrame);

  QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_LOW,"vdec: released_frame with ptr: 0x%x", (unsigned int)vdecFrame.pBuf);
  return VDEC_SUCCESS;
}

Vdec_ReturnType vdec_flush(struct VDecoder* dec, int *nFlushedFrames)
{
  QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_LOW,"vdec: flush \n");
  VideoDecoder* pDec = (VideoDecoder*)(dec->core);
  if(!pDec) return VDEC_EFAILED;
  pDec->Flush( nFlushedFrames );
  return VDEC_SUCCESS;
}

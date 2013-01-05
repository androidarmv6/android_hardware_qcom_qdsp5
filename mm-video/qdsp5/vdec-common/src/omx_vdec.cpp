/*============================================================================
                            O p e n M A X   w r a p p e r s
                             O p e n  M A X   C o r e

*//** @file omx_vdec.c
  This module contains the implementation of the OpenMAX core & component.

Copyright (c) 2006-2007 QUALCOMM Incorporated.
All Rights Reserved. Qualcomm Proprietary and Confidential.
*//*========================================================================*/

/*============================================================================
                              Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/OmxVdec/rel/1.2/omx_vdec.cpp#30 $
when       who     what, where, why
--------   ---     -------------------------------------------------------
05/06/08   ---     Created file.

============================================================================*/

//////////////////////////////////////////////////////////////////////////////
//                             Include Files
//////////////////////////////////////////////////////////////////////////////

#include<string.h>
#include<pthread.h>
#include<stdlib.h>
#ifndef PC_DEBUG
    #include<unistd.h>
#endif
#include<errno.h>
#include "omx_vdec.h"
#include <fcntl.h>
#include "MP4_Utils.h"
#include "qutility.h"
#include "qtv_msg.h"

#define H264_SUPPORTED_WIDTH (480)
#define H264_SUPPORTED_HEIGHT (368)

#define MPEG4_SUPPORTED_WIDTH (480)
#define MPEG4_SUPPORTED_HEIGHT (368)

#define VC1_SP_MP_START_CODE        0xC5000000
#define VC1_SP_MP_START_CODE_MASK   0xFF000000
#define VC1_AP_START_CODE           0x00000100
#define VC1_AP_START_CODE_MASK      0xFFFFFF00
#define VC1_STRUCT_C_PROFILE_MASK   0xF0
#define VC1_STRUCT_B_LEVEL_MASK     0xE0000000       
#define VC1_SIMPLE_PROFILE          0
#define VC1_MAIN_PROFILE            1
#define VC1_ADVANCE_PROFILE         3
#define VC1_SIMPLE_PROFILE_LOW_LEVEL  0
#define VC1_SIMPLE_PROFILE_MED_LEVEL  2
#define VC1_STRUCT_C_LEN            4
#define VC1_STRUCT_C_POS            8
#define VC1_STRUCT_A_POS            12
#define VC1_STRUCT_B_POS            24
#define VC1_SEQ_LAYER_SIZE          36

QPERF_INIT(empty_time);
#ifdef PROFILE_DECODER
static int empty_cnt = 0;
#endif

#ifdef _ANDROID_
    extern "C"{
        #include<utils/Log.h>
    }
#endif//_ANDROID_


genericQueue::genericQueue()
{
	head = NULL;
	tail = NULL;
	numElements = 0;
}

int genericQueue::Enqueue(void* data)
{
	if(NULL == data) return 1;
	node* new_node = new node;
	new_node->data = data;
	new_node->next = NULL;
	if(0 == numElements)
	{
		head = new_node;
	}
	else{
		tail->next = new_node;
	}
	
	tail = new_node;
	++numElements;
	return 0;
}

void* genericQueue::Dequeue()
{
        if(!head) return NULL;
	void* retVal = head->data;
	node* head_next = head->next;
	delete head;
	head = head_next;
	--numElements;
	if(0 == numElements)
	{
          DEBUG_PRINT("FA: Setting Tail to NULL\n");
	  tail = NULL;
	}
	return retVal;
}

int genericQueue::GetSize()
{
	int ret = numElements;
	return ret;
}

void* genericQueue::checkHead()
{
	void* ret = NULL;
	DEBUG_PRINT("FA: check Head\n");
	DEBUG_PRINT("FA: check Head: after mutex\n");
	if(head)
		ret = head->data;
	else
	{
		DEBUG_PRINT("Queue Head is NULL\n");
		ret = NULL;
	}
	return ret;
}
void* genericQueue::checkTail()
{
	void* ret = NULL;
	if(tail)
		ret = tail->data;
	else
	{
		DEBUG_PRINT("Tail is NULL\n");
		ret = NULL;
	}
	return ret;
}

genericQueue::~genericQueue()
{
	node* tmp = head;
	node* tmp_next;
	while(tmp)
	{
		tmp_next = tmp->next;
		delete tmp;
		tmp = tmp_next;
	}
	head = NULL;
	tail = NULL;
}


#if 0
OMX_U8 off_buf[128*1024];
#endif // might be required if decoder wants to process SPS+PPS inband

void* message_thread(void* input)
{
    unsigned char id;
    int n;
    omx_vdec* omx = reinterpret_cast<omx_vdec*>(input);
    DEBUG_PRINT("omx_vdec: message thread start\n");
    while (1){
#ifndef PC_DEBUG
        n = read(omx->m_pipe_in, &id, 1);
#endif // PC_DEBUG
        if(0 == n) break;
        if (1 == n) {
		if(id == TERMINATE_MESSAGE_THREAD){
			DEBUG_PRINT("Got Terminate Message Thread Command\n");
			break;
		}
            omx->process_event_cb(&(omx->m_vdec_cfg), id);
        }
        #ifdef QLE_BUILD
            if (n < 0) break;
        #else
        if ((n < 0) && (errno != EINTR)) break;
        #endif
    }
    DEBUG_PRINT("omx_vdec: message thread stop\n");
    return 0;
}
void post_message(omx_vdec *omx, unsigned char id)
{
      DEBUG_PRINT("omx_vdec: post_message %d\n", id);
#ifndef PC_DEBUG
      write(omx->m_pipe_out, &id, 1);
#else
     omx->process_event_cb(&(omx->m_vdec_cfg),id);
#endif // PC_DEBUG
}

omx_vdec* omx_vdec::g_pVdecInstance = NULL;
pthread_mutex_t omx_vdec::instance_lock = PTHREAD_MUTEX_INITIALIZER;

// omx_cmd_queue destructor
omx_vdec::omx_cmd_queue::~omx_cmd_queue()
{
  // Nothing to do
}

// omx cmd queue constructor
omx_vdec::omx_cmd_queue::omx_cmd_queue(): m_read(0),m_write(0),m_size(0)
{
    memset(m_q,0,sizeof(omx_event)*OMX_CORE_CONTROL_CMDQ_SIZE); 
}

// omx cmd queue insert
bool omx_vdec::omx_cmd_queue::insert_entry(unsigned p1, unsigned p2, unsigned id)
{
  bool ret = true; 
  if(m_size < OMX_CORE_CONTROL_CMDQ_SIZE)
  {
    m_q[m_write].id       = id;
    m_q[m_write].param1   = p1;
    m_q[m_write].param2   = p2;
    m_write++;
    m_size ++; 
    if(m_write >= OMX_CORE_CONTROL_CMDQ_SIZE)
    {
      m_write = 0; 
    }
  }
  else
  {
    ret = false; 
    DEBUG_PRINT_ERROR("ERROR!!! Command Queue Full\n"); 
  }
  return ret; 
}

// omx cmd queue pop
bool omx_vdec::omx_cmd_queue::pop_entry(unsigned *p1, unsigned *p2, unsigned *id)
{
  bool ret = true; 
  if (m_size > 0)
  {
    *id = m_q[m_read].id; 
    *p1 = m_q[m_read].param1; 
    *p2 = m_q[m_read].param2; 
    // Move the read pointer ahead
    ++m_read;
    --m_size;
    if(m_read >= OMX_CORE_CONTROL_CMDQ_SIZE)
    {
      m_read = 0; 
    }
  }
  else
  {
    ret = false; 
  }
  return ret; 
}

// Retrieve the first mesg type in the queue
unsigned omx_vdec::omx_cmd_queue::get_q_msg_type()
{
    return m_q[m_read].id;
}

// factory function executed by the core to create instances
void *get_omx_component_factory_fn(void)
{
  return (void*)omx_vdec::get_instance();
}

#ifdef _ANDROID_
VideoHeap::VideoHeap(int fd, size_t size, void* base)
{
    // dup file descriptor, map once, use pmem
    init(dup(fd), base, size, 0 , "/dev/pmem_adsp");
}
#endif // _ANDROID_

omx_vdec* omx_vdec::get_instance()
{
  omx_vdec* retVal = NULL;
  pthread_mutex_lock(&instance_lock);
  if(g_pVdecInstance) {
	DEBUG_PRINT_ERROR("Video decoder instance already exists.\n");
  } else {
	  g_pVdecInstance = new omx_vdec;
	  retVal = g_pVdecInstance;
  }
  pthread_mutex_unlock(&instance_lock);
  return retVal;
}
/* ======================================================================
FUNCTION
  omx_vdec::omx_vdec

DESCRIPTION
  Constructor

PARAMETERS
  None

RETURN VALUE
  None. 
========================================================================== */
omx_vdec::omx_vdec(): m_state(OMX_StateInvalid),
                      m_app_data(NULL),
                      m_color_format(OMX_COLOR_FormatYUV420Planar),
                      m_loc_use_buf_hdr(NULL),
                      m_vdec(NULL),
                      input(NULL),
                      m_inp_mem_ptr(NULL),
                      m_inp_buf_mem_ptr(NULL),
                      m_out_mem_ptr(NULL),
                      m_first_pending_buf_idx(-1),
                      m_outstanding_frames(0),
                      m_pending_flush(0),
                      m_eos_timestamp(0),
                      m_out_buf_count(OMX_VIDEO_DEC_NUM_OUTPUT_BUFFERS),
                      m_out_bm_count(0),
                      m_inp_buf_count(OMX_VIDEO_DEC_NUM_INPUT_BUFFERS),
                      m_inp_buf_size(OMX_VIDEO_DEC_INPUT_BUFFER_SIZE),
                      m_inp_bm_count(0),
                      m_inp_bPopulated(OMX_FALSE),
                      m_out_bPopulated(OMX_FALSE),
                      m_height(0),
                      m_width(0),
                      m_port_height(0),
                      m_port_width(0),
                      m_crop_x(0),
                      m_crop_y(0),
                      m_crop_dx(0),
                      m_crop_dy(0),
                      m_flags(0),
                      m_nalu_bytes(0),
                      m_msg_cnt(0),
                      m_cmd_cnt(0),
                      m_etb_cnt(0),
                      m_ebd_cnt(0),
                      m_nal_bd_cnt(0),
                      m_ftb_cnt(0),
                      m_fbd_cnt(0),
                      m_inp_bEnabled(OMX_TRUE),
                      m_out_bEnabled(OMX_TRUE),
                      m_event_port_settings_sent(false),
                      m_is_use_buffer(false),
                      m_bEoSNotifyPending(false),
                      m_platform_list(NULL),
                      m_platform_entry(NULL),
                      m_pmem_info(NULL),
                      m_omx_utils(0),
                      m_multiple_nals_per_buffer(false),
                      m_ftb_rxd_cnt(0),
                      m_ftb_rxd_flg(false),
		      m_out_flags(0),
#ifdef _ANDROID_
		      m_heap_ptr(NULL),
#endif
                      flush_before_vdec_op_q(NULL)
{
  /* Assumption is that , to begin with , we have all the frames with decoder */
    memset(&m_cmp,       0,     sizeof(m_cmp)); 
    memset(&m_cb,        0,      sizeof(m_cb)); 
    memset(&m_vdec_cfg,  0,sizeof(m_vdec_cfg)); 
    memset(&m_frame_info, 0,sizeof(m_frame_info)); 
    m_vendor_config.pData = NULL;
    m_vdec_cfg.adsp_fd = -1;

    return; 
}


/* ======================================================================
FUNCTION
  omx_vdec::~omx_vdec

DESCRIPTION
  Destructor

PARAMETERS
  None

RETURN VALUE
  None. 
========================================================================== */
omx_vdec::~omx_vdec()
{
    m_pmem_info = NULL;
    m_nalu_bytes = 0;
    m_ftb_rxd_cnt = 0;
    m_ftb_rxd_flg = false;
    m_port_width = m_port_height = 0;
    pthread_mutex_lock(&instance_lock);
    g_pVdecInstance = NULL;
    pthread_mutex_unlock(&instance_lock);
    
    if(flush_before_vdec_op_q)
    {
	    delete flush_before_vdec_op_q;
	    flush_before_vdec_op_q = NULL;
    }

    return; 
}

/* ======================================================================
FUNCTION
  omx_vdec::OMXCntrlFrameDoneCbStub

DESCRIPTION
  Frame done callback from the video decoder 

PARAMETERS
  1. ctxt(I)  -- Context information to the self.
  2. frame(I) -- video frame decoded
  

RETURN VALUE
  None. 

========================================================================== */
void omx_vdec::frame_done_cb_stub(struct vdec_context *ctxt, struct vdec_frame *frame)
{
  omx_vdec *pThis = (omx_vdec *) ctxt->extra; 

  pThis->post_event((unsigned)ctxt,(unsigned)frame,OMX_COMPONENT_GENERATE_FRAME_DONE);

  return; 
}
/* ======================================================================
FUNCTION
  omx_vdec::OMXCntrlFrameDoneCb

DESCRIPTION
  Frame done callback from the video decoder 

PARAMETERS
  1. ctxt(I)  -- Context information to the self.
  2. frame(I) -- video frame decoded
  

RETURN VALUE
  None. 

========================================================================== */
void omx_vdec::frame_done_cb(struct vdec_context *ctxt, struct vdec_frame *frame)
{
  omx_vdec *pThis = (omx_vdec *) ctxt->extra; 
  OMX_BUFFERHEADERTYPE *pBufHdr;

  pBufHdr = (OMX_BUFFERHEADERTYPE *)pThis->m_out_mem_ptr; 

  DEBUG_PRINT("Frame Done CB cnt[%d] timestamp[%lld] frame->flags %d pThis->m_out_mem_ptr %p\n",\
                            (pThis->m_fbd_cnt+1),pBufHdr->nTimeStamp, frame->flags, pThis->m_out_mem_ptr);

  if(pThis->m_out_mem_ptr)
  {
    unsigned int i;
    for(i=0; i< pThis->m_out_buf_count; i++,pBufHdr++)
    {
      if(pBufHdr->pOutputPortPrivate == frame)
      {
        if(BITMASK_ABSENT(&(pThis->m_out_flags),i))
        {
          DEBUG_PRINT("\n Warning: Double framedone - Frame is still with IL client \n");
          return;
        }
        break; 
      }
    }
    // Copy from PMEM area to user defined area 
    if(pThis->omx_vdec_get_use_buf_flg())
    {
        pThis->omx_vdec_cpy_user_buf(pBufHdr);
    }

    DEBUG_PRINT("Frame Done CB i %d, pThis->m_out_buf_count %d\n", i, pThis->m_out_buf_count);

    if(i<pThis->m_out_buf_count)
    {
      
        BITMASK_CLEAR(&(pThis->m_out_flags),i);
        pThis->m_fbd_cnt++;
        DEBUG_PRINT("FBD: Count %d %x %lld\n", pThis->m_fbd_cnt, pThis->m_out_flags,
                                           frame->timestamp); 

      if(pThis->m_cb.FillBufferDone)
      {
        OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *pPMEMInfo = NULL; 
       
        if(pBufHdr->pPlatformPrivate) 
        {
           pPMEMInfo  = (OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *)
                ((OMX_QCOM_PLATFORM_PRIVATE_LIST *)
                    pBufHdr->pPlatformPrivate)->entryList->entry;
           DEBUG_PRINT("\n PMEM fd %u, Offset %x \n", (unsigned)pPMEMInfo->pmem_fd,
                           (unsigned)pPMEMInfo->offset);
        }
        
        if(frame->flags & FRAME_FLAG_FLUSHED)
        {
          pBufHdr->nFilledLen = 0;
          DEBUG_PRINT("\n **** Flushed Frame-%d **** \n",i);
        }
        else
        {
          pBufHdr->nFilledLen = pThis->get_output_buffer_size(); 
          pBufHdr->nOffset   += pThis->get_output_buffer_offset();
        }
        // If the decoder provides frame done for last frame then set the eos flag. 
        if((frame->flags        & FRAME_FLAG_EOS) || 
          ((frame->timestamp > 0) && (frame->timestamp == pThis->m_eos_timestamp)))
        {
          DEBUG_PRINT("\n **** Frame-%d EOS  with last timestamp **** \n",i);
          pBufHdr->nFlags |= OMX_BUFFERFLAG_EOS; 
        }
        else if (frame->flags &  FRAME_FLAG_FATAL_ERROR) 
        {
          DEBUG_PRINT_ERROR("\n **** Frame Fatal Error **** \n");
          pThis->post_event(OMX_EventError, OMX_ErrorInvalidState, OMX_COMPONENT_GENERATE_EVENT);
          pBufHdr->nFlags = OMX_BUFFERFLAG_DECODEONLY;
        }
		if(VDEC_PICTURE_TYPE_I == frame->frameDetails.ePicType[0])
        {
            pBufHdr->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;
        }
 
        pBufHdr->nTimeStamp = frame->timestamp;
      
        PrintFrameHdr(pBufHdr); 
        DEBUG_PRINT("FBD %d with TS %d \n", pThis->m_fbd_cnt,
                         (unsigned)pBufHdr->nTimeStamp); 
      	if(pThis->m_outstanding_frames < 0)
      	{
          ++pThis->m_outstanding_frames;
          DEBUG_PRINT("FBD Outstanding frame cnt %d\n", pThis->m_outstanding_frames);
#ifdef  OMX_VDEC_NONUI_VERSION
          if((pThis->m_fbd_cnt < 1) || (frame->flags & FRAME_FLAG_FLUSHED))
          {
            pThis->m_cb.FillBufferDone(&pThis->m_cmp, pThis->m_app_data, pBufHdr);
          }
          else
          {
            pThis->post_event((unsigned)&pThis->m_cmp, (unsigned)pBufHdr, OMX_COMPONENT_GENERATE_FTB);
          }
#else
          pThis->m_cb.FillBufferDone(&pThis->m_cmp, pThis->m_app_data, pBufHdr);
#endif
        }
      }
      else
      {
        DEBUG_PRINT_ERROR("Error: FrameDoneCb Ignored due to NULL callbacks \n"); 
      }
    }
    else
    {
      // fake frame provided by the decoder to indicate end of stream 
      if(frame->flags & FRAME_FLAG_EOS)
      {
        OMX_BUFFERHEADERTYPE *pBufHdr = (OMX_BUFFERHEADERTYPE *)pThis->m_out_mem_ptr; 
        DEBUG_PRINT("\n **** Fake Frame EOS **** \n");
        for(i=0; i< pThis->m_out_buf_count; i++,pBufHdr++)
        {
          if(BITMASK_PRESENT(&(pThis->m_out_flags),i))
          {
            BITMASK_CLEAR(&(pThis->m_out_flags),i);
            break; 
          }
        }
        if(i<pThis->m_out_buf_count)
        {
          DEBUG_PRINT("EOS Indication used buffer numbered %d\n", i);
          pBufHdr->nFlags |= OMX_BUFFERFLAG_EOS; 
          pBufHdr->nFilledLen = 0;
          pBufHdr->nTimeStamp = frame->timestamp;
          if(pBufHdr->nTimeStamp == 0)
          {
            DEBUG_PRINT("eos timestamp used is %lld\n",pThis->m_eos_timestamp); 
            pBufHdr->nTimeStamp = pThis->m_eos_timestamp; 
          }
          pThis->m_cb.FillBufferDone(&pThis->m_cmp, pThis->m_app_data, pBufHdr);
          pThis->m_bEoSNotifyPending = false;
          ++pThis->m_outstanding_frames;
          DEBUG_PRINT("FBD Outstanding frame cnt %d\n", pThis->m_outstanding_frames);
        }
        else
        {
          DEBUG_PRINT("Failed to send EOS to the IL Client\n");
          pThis->m_bEoSNotifyPending = true;
        }
      } else if (frame->flags &  FRAME_FLAG_FATAL_ERROR) {
	      DEBUG_PRINT_ERROR("\n **** Frame Fatal Error **** \n");
	      pThis->m_state = OMX_StateInvalid;
	      pThis->post_event(OMX_EventError, OMX_ErrorInvalidState, OMX_COMPONENT_GENERATE_EVENT);
      }
    }
  }
  else
  {
    DEBUG_PRINT_ERROR("Error: InvalidCb Ignored due to NULL Out storage \n"); 
  }
  return; 
}

/* ======================================================================
FUNCTION
  omx_vdec::OMXCntrlBufferDoneCbStub

DESCRIPTION
  Buffer done callback from the decoder. This stub posts the command to the 
  decoder pipe which will be executed by the decoder later to the client. 

PARAMETERS
  1. ctxt(I)   -- Context information to the self.
  2. cookie(I) -- Context information related to the specific input buffer 

RETURN VALUE
  true/false

========================================================================== */
void omx_vdec::buffer_done_cb_stub(struct vdec_context *ctxt, void *cookie)
{
  omx_vdec  *pThis              = (omx_vdec *) ctxt->extra; 

  pThis->post_event((unsigned)ctxt,(unsigned)cookie,OMX_COMPONENT_GENERATE_BUFFER_DONE);
  return;
}


/* ======================================================================
FUNCTION
  omx_vdec::OMXCntrlBufferDoneCb

DESCRIPTION
  Buffer done callback from the decoder.

PARAMETERS
  1. ctxt(I)   -- Context information to the self.
  2. cookie(I) -- Context information related to the specific input buffer 

RETURN VALUE
  true/false

========================================================================== */
void omx_vdec::buffer_done_cb(struct vdec_context *ctxt, void *cookie)
{
  omx_vdec  *pThis              = (omx_vdec *) ctxt->extra; 

    if(pThis->m_multiple_nals_per_buffer)
    {
        pthread_mutex_lock(&pThis->m_nal_bd_lock);
        --pThis->m_nal_bd_cnt;

        DEBUG_PRINT("Rxed nal_bd cnt[%u] BufHdr[%p]\n",\
                                           pThis->m_nal_bd_cnt,
                                           (OMX_BUFFERHEADERTYPE*)cookie);
        pthread_mutex_unlock(&pThis->m_nal_bd_lock);
    }
    else
    {
        if(pThis->m_cb.EmptyBufferDone)
        {
            OMX_BUFFERHEADERTYPE *bufHdr = (OMX_BUFFERHEADERTYPE *)cookie; 
            PrintFrameHdr(bufHdr); 
            unsigned int nPortIndex =  
                    bufHdr - (OMX_BUFFERHEADERTYPE *)pThis->m_inp_mem_ptr; 
            if(nPortIndex < pThis->m_inp_buf_count)
            {
                if(BITMASK_ABSENT(&pThis->m_flags,
                         (OMX_COMPONENT_FIRST_BUFFER_PENDING+nPortIndex)))
                {
                    pThis->m_cb.EmptyBufferDone(&pThis->m_cmp , 
                                                 pThis->m_app_data, bufHdr);
                }
                else
                {
                    DEBUG_PRINT("buffer Done callback received for pending buffer; Ignoring!!\n"); 
                }
            }
            else
            {
                DEBUG_PRINT_ERROR("ERROR!! Buffer Done Callback Came with Incorrect buffer\n"); 
		pThis->m_cb.EmptyBufferDone(&pThis->m_cmp ,
				pThis->m_app_data, bufHdr);
            }
        }
        else
        {
            DEBUG_PRINT("BufferDoeCb Ignored due to NULL callbacks \n"); 
        }
    }
    return;
}

/* ======================================================================
FUNCTION
  omx_vdec::OMXCntrlProcessMsgCb

DESCRIPTION
  IL Client callbacks are generated through this routine. The decoder 
  provides the thread context for this routine. 

PARAMETERS
  ctxt -- Context information related to the self.
  id   -- Event identifier. This could be any of the following: 
          1. Command completion event
          2. Buffer done callback event 
          3. Frame done callback event

RETURN VALUE
  None.  

========================================================================== */
void omx_vdec::process_event_cb(struct vdec_context *ctxt, unsigned char id)
{
  unsigned      p1; // Parameter - 1
  unsigned      p2; // Parameter - 2
  unsigned      ident; 
  unsigned qsize=0; // qsize
  omx_vdec  *pThis              = (omx_vdec *) ctxt->extra; 

  if(!pThis)
  {
    DEBUG_PRINT_ERROR("ERROR : ProcessMsgCb: Context is incorrect; bailing out\n"); 
    return; 
  }
  
  // Protect the shared queue data structure
  do 
  {
#ifndef PC_DEBUG
    pthread_mutex_lock(&pThis->m_lock); 
#endif // PC_DEBUG
    qsize = pThis->m_ftb_q.m_size; 
    if(qsize)
    {
      pThis->m_ftb_q.pop_entry(&p1,&p2,&ident); 
      DEBUG_PRINT("FTBQ[%d] CMDQ[%d] ETBQ[%d] PendList=%d\n",\
                                     pThis->m_ftb_q.m_size,
                                     pThis->m_cmd_q.m_size,
                                     pThis->m_etb_q.m_size,
                                     pThis->m_in_pend_nals.size());
    }
    else
    {
      qsize = pThis->m_cmd_q.m_size; 
      if(qsize)
      {
          DEBUG_PRINT("CMDQ[%d] FTBQ[%d] ETBQ[%d] PendList=%d\n",\
                                     pThis->m_cmd_q.m_size,
                                     pThis->m_ftb_q.m_size,
                                     pThis->m_etb_q.m_size,
                                     pThis->m_in_pend_nals.size());
          pThis->m_cmd_q.pop_entry(&p1,&p2,&ident); 
      }
      else
      {
          pthread_mutex_lock(&pThis->m_nal_bd_lock);
          // process ETB and EBD only if no pending nals and also
          // all bds have been recieved from decoder
          qsize = pThis->m_etb_q.m_size;
          //if((!pThis->m_in_pend_nals.size()) && !pThis->m_nal_bd_cnt)
          if(qsize && (!pThis->m_in_pend_nals.size()))
          {
              DEBUG_PRINT("ETBQ[%d] FTBQ[%d] CMDQ[%d] PendList=%d nal_bd=%u\n",\
                                     pThis->m_etb_q.m_size,
                                     pThis->m_ftb_q.m_size,
                                     pThis->m_cmd_q.m_size,
                                     pThis->m_in_pend_nals.size(),
                                     pThis->m_nal_bd_cnt);
              pThis->m_etb_q.pop_entry(&p1,&p2,&ident); 
          }
          else if(qsize)
          {
              DEBUG_PRINT("<==>FTBQ[%d] CMDQ[%d] ETBQ[%d] PendList=%d nal_bd[%u]\n",\
                                     pThis->m_ftb_q.m_size,
                                     pThis->m_cmd_q.m_size,
                                     pThis->m_etb_q.m_size,
                                     pThis->m_in_pend_nals.size(),
                                     pThis->m_nal_bd_cnt);
              qsize = 0;
          }
          else{}
          pthread_mutex_unlock(&pThis->m_nal_bd_lock);
      }
    }
    if(qsize)
    {
      pThis->m_msg_cnt ++;
    }
#ifndef PC_DEBUG
    pthread_mutex_unlock(&pThis->m_lock); 
#endif // PC_DEBUG

    if(qsize > 0)
    {
      id = ident; 
      DEBUG_PRINT("Process ->%d[%d]ebd %d fbd %d oc %d %x,%x\n",pThis->m_state,ident, pThis->m_etb_cnt,
            pThis->m_fbd_cnt,pThis->m_outstanding_frames,pThis->m_flags >> 3,
            pThis->m_out_flags);
      if(id == OMX_COMPONENT_GENERATE_EVENT)
      {
        if (pThis->m_cb.EventHandler)
        {
          if (p1 == OMX_CommandStateSet)
          {
             pThis->m_state = (OMX_STATETYPE) p2;
             DEBUG_PRINT("Process -> state set to %d \n", pThis->m_state);
             pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                   OMX_EventCmdComplete, p1, p2, NULL );
          }
          /* posting error events for illegal state transition */
          else if (p1 == OMX_EventError)
          {
             if(p2 == OMX_ErrorInvalidState)
             {
                pThis->m_state = OMX_StateInvalid;
                pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                 OMX_EventError, OMX_ErrorInvalidState, 0, NULL );
                pThis->execute_omx_flush(OMX_ALL);
             }
             else
             {
                 pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                   OMX_EventError, p2, 0, NULL );
            
             }
          }
          else if (p1 == OMX_CommandPortDisable)
          {
             DEBUG_PRINT("Process -> Port %d set to PORT_STATE_DISABLED state \n", p2);
             pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                   OMX_EventCmdComplete, p1, p2, NULL );
          }
          else if (p1 == OMX_CommandPortEnable)
          {
             DEBUG_PRINT("Process -> Port %d set to PORT_STATE_ENABLED state \n", p2);
             pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                   OMX_EventCmdComplete, p1, p2, NULL );
          }
          else
          {
		  pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
				  OMX_EventCmdComplete, p1, p2, NULL );
	  }
        }
        else
        {
          DEBUG_PRINT_ERROR("Error: ProcessMsgCb ignored due to NULL callbacks\n"); 
        }
      }
      else if (id == OMX_COMPONENT_GENERATE_EVENT_FLUSH)
      {
             pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                   OMX_EventCmdComplete, p1, p2, NULL );
      }
      else if(id == OMX_COMPONENT_GENERATE_BUFFER_DONE)
      {
        buffer_done_cb((struct vdec_context *)p1,(void *)p2); 
      }
      else if(id == OMX_COMPONENT_GENERATE_FRAME_DONE)
      {
        frame_done_cb((struct vdec_context *)p1,(struct vdec_frame *)p2); 
        /* Delay Event Command Complete for State Set command for IDLE in case of Pending buffers done callbacks from DSP */
	if(pThis->m_pending_flush && !pThis->m_outstanding_frames)
        {
            pThis->m_pending_flush = 0;
            pThis->m_state = OMX_StateIdle;
            pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                     OMX_EventCmdComplete, OMX_CommandStateSet,OMX_StateIdle, NULL );
        }
      }
      else if(id == OMX_COMPONENT_GENERATE_ETB)
      {
        pThis->empty_this_buffer_proxy((OMX_HANDLETYPE)p1,(OMX_BUFFERHEADERTYPE *)p2); 
      }
      else if(id == OMX_COMPONENT_GENERATE_FTB)
      {
        pThis->fill_this_buffer_proxy((OMX_HANDLETYPE)p1,(OMX_BUFFERHEADERTYPE *)p2); 
      }
      else if(id == OMX_COMPONENT_GENERATE_COMMAND)
      {
        pThis->send_command_proxy(&pThis->m_cmp,(OMX_COMMANDTYPE)p1,(OMX_U32)p2,(OMX_PTR)NULL); 
      }
      else if(id == OMX_COMPONENT_PUSH_PENDING_BUFS)
      {
        pThis->push_pending_buffers_proxy(); 
      }
      else
      {
        DEBUG_PRINT_ERROR("Error: ProcessMsgCb Ignored due to Invalid Identifier\n"); 
      }
    }
    else
    {
    }
    pthread_mutex_lock(&pThis->m_lock); 
    qsize = pThis->m_cmd_q.m_size + pThis->m_ftb_q.m_size; 
    pthread_mutex_unlock(&pThis->m_lock); 
  } while(qsize>0);
  return; 
}



/* ======================================================================
FUNCTION
  omx_vdec::ComponentInit

DESCRIPTION
  Initialize the component.

PARAMETERS
  ctxt -- Context information related to the self.
  id   -- Event identifier. This could be any of the following: 
          1. Command completion event
          2. Buffer done callback event 
          3. Frame done callback event

RETURN VALUE
  None.  

========================================================================== */
OMX_ERRORTYPE omx_vdec::component_init(OMX_STRING role)
{

  OMX_ERRORTYPE eRet = OMX_ErrorNone; 
  int fds[2];
  int r;

  // Check if multiple instances of the decoder is running
  int mod_fd;
#ifdef _ANDROID_
  mod_fd = open("/dev/adsp/VIDEOTASK", O_RDWR); 
#elif !PC_DEBUG
  mod_fd = open("/dev/VIDEOTASK", O_RDWR); 
#endif
#ifndef PC_DEBUG
  if(mod_fd < 0)
  {   
      DEBUG_PRINT("Omx_vdec::Comp Init Returning failure\n");
      // Another decoder instance is running. return from here.
      return OMX_ErrorInsufficientResources; 
  }
  m_vdec_cfg.adsp_fd = mod_fd;
#endif \\PC_DEBUG

    pthread_mutex_init(&m_lock, NULL);
    pthread_mutex_init(&m_ftb_lock, NULL);
    pthread_mutex_init(&m_nal_bd_lock, NULL);
#ifndef PC_DEBUG
    sem_init(&m_cmd_lock,0,0);
#endif

  m_vdec_cfg.buffer_done     = buffer_done_cb_stub; 
  m_vdec_cfg.frame_done      = frame_done_cb_stub; 
  m_vdec_cfg.process_message = process_event_cb; 
  m_vdec_cfg.height          = OMX_CORE_QCIF_HEIGHT; 
  m_vdec_cfg.width           = OMX_CORE_QCIF_WIDTH; 
  m_vdec_cfg.extra           = this; 
  // Copy the role information which provides the decoder kind
  strncpy(m_vdec_cfg.kind,role,128);

    if(!strncmp(m_vdec_cfg.kind,"OMX.qcom.video.decoder.mpeg4",OMX_MAX_STRINGNAME_SIZE))
    {
         strncpy((char *)m_cRole, "video_decoder.mpeg4",OMX_MAX_STRINGNAME_SIZE);
	 m_out_buf_count = OMX_VIDEO_DEC_NUM_OUTPUT_BUFFERS + 2;
    }
    else if(!strncmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.h263",OMX_MAX_STRINGNAME_SIZE))
    {
         strncpy((char *)m_cRole, "video_decoder.h263",OMX_MAX_STRINGNAME_SIZE);
	 m_out_buf_count = OMX_VIDEO_DEC_NUM_OUTPUT_BUFFERS + 2;
    }
    else if(!strncmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.avc",OMX_MAX_STRINGNAME_SIZE))
    {
          strncpy((char *)m_cRole, "video_decoder.avc",OMX_MAX_STRINGNAME_SIZE);
	  m_out_buf_count = OMX_VIDEO_DEC_NUM_OUTPUT_BUFFERS + 2;
    }
    else if(!strncmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.vc1",OMX_MAX_STRINGNAME_SIZE))
    {
          strncpy((char *)m_cRole, "video_decoder.vc1",OMX_MAX_STRINGNAME_SIZE);
    }
    else
    {
        DEBUG_PRINT_ERROR("\n Unknown Component\n");
        eRet = OMX_ErrorInvalidComponentName;
    }

  m_crop_dy = m_height      = m_vdec_cfg.height; 
  m_crop_dx = m_width       = m_vdec_cfg.width; 
  m_port_height             = m_height;
  m_port_width              = m_width;
  m_state                   = OMX_StateLoaded;
  m_first_pending_buf_idx   = -1;
  m_outstanding_frames      = 0;
  m_out_buf_num_port_reconfig = m_out_buf_count;
  #ifndef PC_DEBUG
  if(pipe(fds)){
    //DEBUG_PRINT_ERROR("pipe creation failed\n");
    eRet = OMX_ErrorInsufficientResources; 
  }
  else{
        m_pipe_in = fds[0];
        m_pipe_out = fds[1];
        r = pthread_create(&msg_thread_id,0,message_thread,this);
        if(r < 0) eRet = OMX_ErrorInsufficientResources;
  }
  #endif
  if(!strcmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.avc"))
  {
      H264Utils *h264Utils = new H264Utils(this);
      m_omx_utils = dynamic_cast<OmxUtils*>(h264Utils);
  }

  flush_before_vdec_op_q = new genericQueue();
  if(flush_before_vdec_op_q == NULL)
  {
    DEBUG_PRINT_ERROR("flush_before_vdec_op_q creation failed\n");
    eRet = OMX_ErrorInsufficientResources; 
  }

  QPERF_RESET(empty_time);
#ifdef PROFILE_DECODER
  empty_cnt = 0;
#endif
  return eRet;
}

/* ======================================================================
FUNCTION
  omx_vdec::GetComponentVersion

DESCRIPTION
  Returns the component version. 

PARAMETERS
  TBD.

RETURN VALUE
  OMX_ErrorNone. 

========================================================================== */
OMX_ERRORTYPE  omx_vdec::get_component_version(OMX_IN OMX_HANDLETYPE               hComp, 
                                                  OMX_OUT OMX_STRING          componentName, 
                                                  OMX_OUT OMX_VERSIONTYPE* componentVersion, 
                                                  OMX_OUT OMX_VERSIONTYPE*      specVersion, 
                                                  OMX_OUT OMX_UUIDTYPE*       componentUUID)
{
    if(m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Get Comp Version in Invalid State\n"); 
        return OMX_ErrorInvalidState;
    }
  /* TBD -- Return the proper version */
  return OMX_ErrorNone;
}
/* ======================================================================
FUNCTION
  omx_vdec::SendCommand

DESCRIPTION
  Returns zero if all the buffers released..

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_vdec::send_command(OMX_IN OMX_HANDLETYPE hComp, 
                                          OMX_IN OMX_COMMANDTYPE  cmd, 
                                          OMX_IN OMX_U32       param1, 
                                          OMX_IN OMX_PTR      cmdData)
{
    if(m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Send Command in Invalid State\n"); 
        return OMX_ErrorInvalidState;
    }
    post_event((unsigned)cmd,(unsigned)param1,OMX_COMPONENT_GENERATE_COMMAND); 

#ifndef PC_DEBUG
    sem_wait(&m_cmd_lock);
#endif

    return OMX_ErrorNone; 
}

/* ======================================================================
FUNCTION
  omx_vdec::SendCommand

DESCRIPTION
  Returns zero if all the buffers released..

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_vdec::send_command_proxy(OMX_IN OMX_HANDLETYPE hComp, 
                                          OMX_IN OMX_COMMANDTYPE  cmd, 
                                          OMX_IN OMX_U32       param1, 
                                          OMX_IN OMX_PTR      cmdData)
{
  OMX_ERRORTYPE eRet = OMX_ErrorNone; 
  //   Handle only IDLE and executing
  OMX_STATETYPE eState = (OMX_STATETYPE) param1; 
  int bFlag = 1; 
  int sem_posted = 0;
  if(cmd == OMX_CommandStateSet)
  {
    /***************************/
    /* Current State is Loaded */
    /***************************/
    if(m_state == OMX_StateLoaded)
    {
      if(eState == OMX_StateIdle)
      {
        //if all buffers are allocated or all ports disabled
        if(allocate_done() || 
          (m_inp_bEnabled == OMX_FALSE && m_out_bEnabled == OMX_FALSE))
        {
          DEBUG_PRINT("OMXCORE-SM: Loaded-->Idle\n");
        }
        else
        {
          DEBUG_PRINT("OMXCORE-SM: Loaded-->Idle-Pending\n");
          BITMASK_SET(&m_flags, OMX_COMPONENT_IDLE_PENDING); 
          // Skip the event notification 
          bFlag = 0;
        }
      }
      /* Requesting transition from Loaded to Loaded */
      else if(eState == OMX_StateLoaded)
      {
        DEBUG_PRINT("OMXCORE-SM: Loaded-->Loaded\n");
        post_event(OMX_EventError,OMX_ErrorSameState,OMX_COMPONENT_GENERATE_EVENT);
        eRet = OMX_ErrorSameState;
      }
      /* Requesting transition from Loaded to WaitForResources */
      else if(eState == OMX_StateWaitForResources)
      {
        /* Since error is None , we will post an event at the end of this function definition */
        DEBUG_PRINT("OMXCORE-SM: Loaded-->WaitForResources\n");
      }
      /* Requesting transition from Loaded to Executing */ 
      else if(eState == OMX_StateExecuting)
      {
        DEBUG_PRINT("OMXCORE-SM: Loaded-->Executing\n");
        post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,OMX_COMPONENT_GENERATE_EVENT);
        eRet = OMX_ErrorIncorrectStateTransition;
      }
      /* Requesting transition from Loaded to Pause */ 
      else if(eState == OMX_StatePause)
      {
        DEBUG_PRINT("OMXCORE-SM: Loaded-->Pause\n");
        post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,OMX_COMPONENT_GENERATE_EVENT);
        eRet = OMX_ErrorIncorrectStateTransition;
      }
      /* Requesting transition from Loaded to Invalid */ 
      else if(eState == OMX_StateInvalid)
      {
        DEBUG_PRINT("OMXCORE-SM: Loaded-->Invalid\n");
        post_event(OMX_EventError,OMX_ErrorInvalidState, OMX_COMPONENT_GENERATE_EVENT);
        eRet = OMX_ErrorInvalidState;
      }
      else
      {
        DEBUG_PRINT_ERROR("OMXCORE-SM: Loaded-->Invalid(%d Not Handled)\n",eState);
        eRet = OMX_ErrorBadParameter; 
      }
    }

    /***************************/
    /* Current State is IDLE */
    /***************************/
    else if(m_state == OMX_StateIdle)
    {
      if(eState == OMX_StateLoaded)
      {
        if(release_done())
        {
          /* Since error is None , we will post an event at the end of this function definition */
          DEBUG_PRINT("OMXCORE-SM: Idle-->Loaded\n");
        }
        else
        {
          DEBUG_PRINT("OMXCORE-SM: Idle-->Loaded-Pending\n");
          BITMASK_SET(&m_flags, OMX_COMPONENT_LOADING_PENDING); 
          // Skip the event notification 
          bFlag = 0; 
        }
      }
      /* Requesting transition from Idle to Executing */ 
      else if(eState == OMX_StateExecuting)
      {
        /* Since error is None , we will post an event at the end of this function definition */
        DEBUG_PRINT("OMXCORE-SM: Idle-->Executing\n");
        
      }
      /* Requesting transition from Idle to Idle */
      else if(eState == OMX_StateIdle)
      {
        DEBUG_PRINT("OMXCORE-SM: Idle-->Idle\n");
        post_event(OMX_EventError,OMX_ErrorSameState,OMX_COMPONENT_GENERATE_EVENT);
        eRet = OMX_ErrorSameState;
      }
      /* Requesting transition from Idle to WaitForResources */
      else if(eState == OMX_StateWaitForResources)
      {
        DEBUG_PRINT("OMXCORE-SM: Idle-->WaitForResources\n");
        post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,OMX_COMPONENT_GENERATE_EVENT);
        eRet = OMX_ErrorIncorrectStateTransition;
      }
       /* Requesting transition from Idle to Pause */
       else if(eState == OMX_StatePause)
      {
        /* Since error is None , we will post an event at the end of this function definition */
        DEBUG_PRINT("OMXCORE-SM: Idle-->Pause\n");
      }
      /* Requesting transition from Idle to Invalid */
       else if(eState == OMX_StateInvalid)
      {
        DEBUG_PRINT("OMXCORE-SM: Idle-->Invalid\n");
        post_event(OMX_EventError,OMX_ErrorInvalidState,OMX_COMPONENT_GENERATE_EVENT);
        eRet = OMX_ErrorInvalidState;
      }
      else
      {
        DEBUG_PRINT_ERROR("OMXCORE-SM: Idle --> %d Not Handled\n",eState);
        eRet = OMX_ErrorBadParameter; 
      }
    }

    /******************************/
    /* Current State is Executing */
    /******************************/
    else if(m_state == OMX_StateExecuting)
    {
       /* Requesting transition from Executing to Idle */
       if(eState == OMX_StateIdle)
       {
         /* Since error is None , we will post an event at the end of this function definition */
         DEBUG_PRINT("\n OMXCORE-SM: Executing --> Idle \n"); 
	 if(!sem_posted) {
		 sem_post(&m_cmd_lock);
		 sem_posted = 1;	 
	 }
         execute_omx_flush(OMX_ALL);
         if(!m_vdec)
         {
             DEBUG_PRINT("Rxed Flush Before Dec is opened\n");
         }
         else
         {
             /* Delay Event Cmd Complete if there are any Outstanding Frames pending from DSP */
             if(0 != m_outstanding_frames)
             {
                m_pending_flush = 1;
                bFlag = 0; 
             }
          }
       }
       /* Requesting transition from Executing to Paused */
       else if(eState == OMX_StatePause)
       {
         /* Since error is None , we will post an event at the end of this function definition */
         DEBUG_PRINT("\n OMXCORE-SM: Executing --> Paused \n");
       }
       /* Requesting transition from Executing to Loaded */
       else if(eState == OMX_StateLoaded)
       {
         DEBUG_PRINT("\n OMXCORE-SM: Executing --> Loaded \n");
         post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,OMX_COMPONENT_GENERATE_EVENT);
         eRet = OMX_ErrorIncorrectStateTransition;
       }
       /* Requesting transition from Executing to WaitForResources */
       else if(eState == OMX_StateWaitForResources)
       {
         DEBUG_PRINT("\n OMXCORE-SM: Executing --> WaitForResources \n");
         post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,OMX_COMPONENT_GENERATE_EVENT);
         eRet = OMX_ErrorIncorrectStateTransition;
       }
       /* Requesting transition from Executing to Executing */
       else if(eState == OMX_StateExecuting)
       {
         DEBUG_PRINT("\n OMXCORE-SM: Executing --> Executing \n");
         post_event(OMX_EventError,OMX_ErrorSameState,OMX_COMPONENT_GENERATE_EVENT);
         eRet = OMX_ErrorSameState;
       }
       /* Requesting transition from Executing to Invalid */
       else if(eState == OMX_StateInvalid)
       {
         DEBUG_PRINT("\n OMXCORE-SM: Executing --> Invalid \n");
         post_event(OMX_EventError,OMX_ErrorInvalidState,OMX_COMPONENT_GENERATE_EVENT);
         eRet = OMX_ErrorInvalidState;
       }
       else
       {
         DEBUG_PRINT_ERROR("OMXCORE-SM: Executing --> %d Not Handled\n",eState);
         eRet = OMX_ErrorBadParameter; 
       }
    }
    /***************************/
    /* Current State is Pause  */
    /***************************/
    else if(m_state == OMX_StatePause)
    {
      /* Requesting transition from Pause to Executing */
      if(eState == OMX_StateExecuting)
      {
        /* Since error is None , we will post an event at the end of this function definition */
        DEBUG_PRINT("\n Pause --> Executing \n");
      }
      /* Requesting transition from Pause to Idle */
      else if(eState == OMX_StateIdle)
      {
        /* Since error is None , we will post an event at the end of this function definition */
        DEBUG_PRINT("\n Pause --> Idle \n");
	 if(!sem_posted) {
		 sem_post(&m_cmd_lock);
		 sem_posted = 1;	 
	 }
        execute_omx_flush(OMX_ALL); 
      }
      /* Requesting transition from Pause to loaded */
      else if(eState == OMX_StateLoaded)
      {
        DEBUG_PRINT("\n Pause --> loaded \n");
        post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,OMX_COMPONENT_GENERATE_EVENT);
        eRet = OMX_ErrorIncorrectStateTransition;
      }
      /* Requesting transition from Pause to WaitForResources */
      else if(eState == OMX_StateWaitForResources)
      {
        DEBUG_PRINT("\n Pause --> WaitForResources \n");
        post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,OMX_COMPONENT_GENERATE_EVENT);
        eRet = OMX_ErrorIncorrectStateTransition;
      }
      /* Requesting transition from Pause to Pause */
      else if(eState == OMX_StatePause)
      {
        DEBUG_PRINT("\n Pause --> Pause \n");
        post_event(OMX_EventError,OMX_ErrorSameState,OMX_COMPONENT_GENERATE_EVENT);
        eRet = OMX_ErrorSameState;
      }
       /* Requesting transition from Pause to Invalid */
      else if(eState == OMX_StateInvalid)
      {
        DEBUG_PRINT("\n Pause --> Invalid \n");
        post_event(OMX_EventError,OMX_ErrorInvalidState,OMX_COMPONENT_GENERATE_EVENT);
        eRet = OMX_ErrorInvalidState;
      }
      else
      {
        DEBUG_PRINT("OMXCORE-SM: Paused --> %d Not Handled\n",eState);
        eRet = OMX_ErrorBadParameter; 
      }
    }
     /***************************/
    /* Current State is WaitForResources  */
    /***************************/
    else if(m_state == OMX_StateWaitForResources)
    {
      /* Requesting transition from WaitForResources to Loaded */
      if(eState == OMX_StateLoaded)
      {
        /* Since error is None , we will post an event at the end of this function definition */
        DEBUG_PRINT("OMXCORE-SM: WaitForResources-->Loaded\n");
      }
      /* Requesting transition from WaitForResources to WaitForResources */
      else if (eState == OMX_StateWaitForResources)
      {
        DEBUG_PRINT("OMXCORE-SM: WaitForResources-->WaitForResources\n");
        post_event(OMX_EventError,OMX_ErrorSameState,OMX_COMPONENT_GENERATE_EVENT);
        eRet = OMX_ErrorSameState;
      }
      /* Requesting transition from WaitForResources to Executing */
      else if(eState == OMX_StateExecuting)
      {
        DEBUG_PRINT("OMXCORE-SM: WaitForResources-->Executing\n");
        post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,OMX_COMPONENT_GENERATE_EVENT);
        eRet = OMX_ErrorIncorrectStateTransition;
      }
      /* Requesting transition from WaitForResources to Pause */
      else if(eState == OMX_StatePause)
      {
        DEBUG_PRINT("OMXCORE-SM: WaitForResources-->Pause\n");
        post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,OMX_COMPONENT_GENERATE_EVENT);
        eRet = OMX_ErrorIncorrectStateTransition;
      }
      /* Requesting transition from WaitForResources to Invalid */
      else if(eState == OMX_StateInvalid)
      {
        DEBUG_PRINT("OMXCORE-SM: WaitForResources-->Invalid\n");
        post_event(OMX_EventError,OMX_ErrorInvalidState,OMX_COMPONENT_GENERATE_EVENT);
        eRet = OMX_ErrorInvalidState;
      }
      /* Requesting transition from WaitForResources to Loaded - is NOT tested by Khronos TS */
  
    }
    else
    {
      DEBUG_PRINT_ERROR("OMXCORE-SM: %d --> %d(Not Handled)\n",m_state,eState);
      eRet = OMX_ErrorBadParameter; 
    }
  }
    /********************************/
    /* Current State is Invalid */
    /*******************************/
    else if(m_state == OMX_StateInvalid)
    {
      /* State Transition from Inavlid to any state */
      if(eState == (OMX_StateLoaded || OMX_StateWaitForResources || OMX_StateIdle || OMX_StateExecuting || OMX_StatePause || OMX_StateInvalid))
      {
        DEBUG_PRINT("OMXCORE-SM: Invalid -->Loaded\n");
        post_event(OMX_EventError,OMX_ErrorInvalidState,OMX_COMPONENT_GENERATE_EVENT);
        eRet = OMX_ErrorInvalidState;
      }
    }
  else if (cmd == OMX_CommandFlush)
  {
	 if(!sem_posted) {
		 sem_post(&m_cmd_lock);
		 sem_posted = 1;	 
	 }
    execute_omx_flush(param1);
    if(0 == param1 || OMX_ALL == param1)
    {
	    //generate input flush event only.
	    post_event(OMX_CommandFlush,OMX_CORE_INPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_EVENT_FLUSH);
    }
    if(1 == param1 || OMX_ALL == param1)
    {
	    //generate output flush event only.
	    post_event(OMX_CommandFlush,OMX_CORE_OUTPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_EVENT_FLUSH);
    }

    bFlag = 0; 
    pthread_mutex_lock(&m_ftb_lock);
    m_ftb_rxd_flg = false;
    m_ftb_rxd_cnt = 0;
    pthread_mutex_unlock(&m_ftb_lock);
  }
  else if ( cmd == OMX_CommandPortEnable)
  {
      if(param1 != OMX_CORE_OUTPUT_PORT_INDEX)
      {
          DEBUG_PRINT_ERROR("OMXCORE-SM:Enable should be on Ouput Port\n");
      }
      DEBUG_PRINT_ERROR("OMXCORE-SM:Recieved command ENABLE (%d)\n",cmd);
      pthread_mutex_lock(&m_ftb_lock);
      m_ftb_rxd_flg = false;
      m_ftb_rxd_cnt = 0;
      pthread_mutex_unlock(&m_ftb_lock);

      // call vdec_open 
      if(!m_vdec)
      { 
          m_vdec_cfg.height          = m_height; 
          m_vdec_cfg.width           = m_width; 
          m_vdec_cfg.size_of_nal_length_field = m_nalu_bytes;
          m_vdec_cfg.inputReq.bufferSize      = m_inp_buf_size;
	  m_vdec_cfg.outputBuffer.numBuffers  = m_out_buf_count; 
          m_vdec = vdec_open(&m_vdec_cfg);
          if(!m_vdec)
          {
              m_cb.EventHandler(&m_cmp, m_app_data, OMX_EventError, 
                                              OMX_CommandStateSet, 0, NULL );
              DEBUG_PRINT_ERROR("ERROR!!! vdec_open failed\n");
#ifndef PC_DEBUG
              sem_post(&m_cmd_lock);
#endif
              return OMX_ErrorHardware;
          }
          // Populate the Buffer Headers
          omx_vdec_get_out_buf_hdrs();           
          // Populate Use Buffer Headers
          if(omx_vdec_get_use_buf_flg()){
              omx_vdec_dup_use_buf_hdrs();
              omx_vdec_get_out_use_buf_hdrs();
              omx_vdec_add_entries();
              omx_vdec_display_out_buf_hdrs();
          }

      }
      if(param1 == OMX_CORE_INPUT_PORT_INDEX || param1 == OMX_ALL)
      {
	      m_inp_bEnabled = OMX_TRUE;

	      if((m_state == OMX_StateLoaded && !BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING))
			      || allocate_input_done())
	      {
		      post_event(OMX_CommandPortEnable,OMX_CORE_INPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_EVENT);
	      }
	      else
	      {
		      DEBUG_PRINT("OMXCORE-SM: Disabled-->Enabled Pending\n");
		      BITMASK_SET(&m_flags, OMX_COMPONENT_INPUT_ENABLE_PENDING);
		      // Skip the event notification
		      bFlag = 0;
	      }
      }
      if(param1 == OMX_CORE_OUTPUT_PORT_INDEX || param1 == OMX_ALL)
      {
          m_out_bEnabled = OMX_TRUE;

          if((m_state == OMX_StateLoaded && !BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING))
              || (allocate_output_done()))
          {
             post_event(OMX_CommandPortEnable,OMX_CORE_OUTPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_EVENT);

          }
          else
          {
              DEBUG_PRINT("OMXCORE-SM: Disabled-->Enabled Pending\n");
              BITMASK_SET(&m_flags, OMX_COMPONENT_OUTPUT_ENABLE_PENDING);
              // Skip the event notification
              bFlag = 0;
          }
      }
  } 
  else if (cmd == OMX_CommandPortDisable)
  {
      if(param1 != OMX_CORE_OUTPUT_PORT_INDEX)
      {
          DEBUG_PRINT_ERROR("OMXCORE-SM:Disable should be on Ouput Port\n");
      }
      DEBUG_PRINT_ERROR("OMXCORE-SM:Recieved command DISABLE (%d)\n",cmd);

      if(param1 == OMX_CORE_INPUT_PORT_INDEX || param1 == OMX_ALL)
      {
          m_inp_bEnabled = OMX_FALSE;
          if((m_state == OMX_StateLoaded || m_state == OMX_StateIdle)
              && release_input_done())
          {
             post_event(OMX_CommandPortDisable,OMX_CORE_INPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_EVENT);
          }
          else 
          {
             BITMASK_SET(&m_flags, OMX_COMPONENT_INPUT_DISABLE_PENDING);
             if(m_state == OMX_StatePause ||m_state == OMX_StateExecuting)
             {
	 if(!sem_posted) {
		 sem_post(&m_cmd_lock);
		 sem_posted = 1;	 
	 }
                execute_omx_flush(OMX_CORE_INPUT_PORT_INDEX);
             }

             // Skip the event notification
             bFlag = 0;
          }
      }
      if(param1 == OMX_CORE_OUTPUT_PORT_INDEX || param1 == OMX_ALL)
      {
           m_out_bEnabled = OMX_FALSE;
          
          if((m_state == OMX_StateLoaded || m_state == OMX_StateIdle)
              && release_output_done())
          {
             post_event(OMX_CommandPortDisable,OMX_CORE_OUTPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_EVENT);
          }
          else 
         {
             BITMASK_SET(&m_flags, OMX_COMPONENT_OUTPUT_DISABLE_PENDING);
             if(m_state == OMX_StatePause ||m_state == OMX_StateExecuting)
      {
	 if(!sem_posted) {
		 sem_post(&m_cmd_lock);
		 sem_posted = 1;	 
	 }
			execute_omx_flush(OMX_CORE_OUTPUT_PORT_INDEX);
      }
             
      // Skip the event notification
      bFlag = 0;

          }
      }
  }
  else 
  {
    DEBUG_PRINT_ERROR("Error: Invalid Command received other than StateSet (%d)\n",cmd);
    eRet = OMX_ErrorNotImplemented;
  }
  if(eRet == OMX_ErrorNone && bFlag)
  {
    post_event(cmd,eState,OMX_COMPONENT_GENERATE_EVENT); 
  }
#ifndef PC_DEBUG
	 if(!sem_posted) {
		 sem_post(&m_cmd_lock);
		 sem_posted = 1;	 
	 }
#endif
  return eRet;
}

/* ======================================================================
FUNCTION
  omx_vdec::ExecuteOmxFlush

DESCRIPTION
  Executes the OMX flush.

PARAMETERS
  flushtype - input flush(1)/output flush(0)/ both.

RETURN VALUE
  true/false

========================================================================== */
bool omx_vdec::execute_omx_flush(OMX_U32 flushType)
{
	bool bRet = false;
	if(flushType == 0 || flushType == OMX_ALL)
	{
		//flush input only
		execute_input_flush();
	}
	if(flushType == 1 || flushType == OMX_ALL)
	{
		//flush output only 
		execute_output_flush();
	}
	return bRet; 
}
/*=========================================================================
FUNCTION : execute_output_flush

DESCRIPTION
  Executes the OMX flush at OUTPUT PORT.

PARAMETERS
  None.

RETURN VALUE
  true/false
==========================================================================*/
bool omx_vdec::execute_output_flush(void)
{
	bool bRet = false; 
	OMX_BUFFERHEADERTYPE *pOutBufHdr = (OMX_BUFFERHEADERTYPE *)m_out_mem_ptr;
	unsigned int p1 = 0;
	unsigned int p2 = 0;
	unsigned id = 0;
	OMX_BUFFERHEADERTYPE *ftb_buf_hdr = NULL;

	if(!pOutBufHdr || !m_vdec)
	{
		DEBUG_PRINT_ERROR("Omx Flush issued when vdec is not initialized yet.\n"); 
		if (pOutBufHdr)
		{
			unsigned nPortIndex;
			//We will ignore this Queue once m_vdec is created. This will cause no harm 
			//now since when output buffers are created in m_vdec, m_vdec assumes that
			//all buffers are with itself.
			OMX_BUFFERHEADERTYPE* tmp_buf_hdr = (OMX_BUFFERHEADERTYPE*)flush_before_vdec_op_q->Dequeue();
	 		while(tmp_buf_hdr)
			{
				nPortIndex = tmp_buf_hdr - ((OMX_BUFFERHEADERTYPE *) m_out_mem_ptr);
				BITMASK_CLEAR(&m_out_flags,nPortIndex);
				++m_outstanding_frames;
				m_cb.FillBufferDone(&m_cmp, m_app_data, tmp_buf_hdr);
				DEBUG_PRINT("Flushing output buffer = %d",m_out_buf_count);
				tmp_buf_hdr = (OMX_BUFFERHEADERTYPE*)flush_before_vdec_op_q->Dequeue();
			}		
		}
	}
	else
	{
		int nFlushFrameCnt = 0;
		unsigned i; 
		/* . Execute the decoder flush */
		DEBUG_PRINT("\n *** Calling vdec_flush ***  \n");
		if (VDEC_SUCCESS == vdec_flush(m_vdec, &nFlushFrameCnt))
		{
			DEBUG_PRINT("\n *** Flush Succeeded : Flush Frame Cnt %d *** \n", nFlushFrameCnt);
		}
		else 
		{
			DEBUG_PRINT("\n *** Flush Failed *** \n");
		}
		while(m_ftb_q.m_size > 0)
		{
			DEBUG_PRINT("Flushing FTB Q\n");
			m_ftb_q.pop_entry(&p1,&p2,&id); 
			if(OMX_COMPONENT_GENERATE_FRAME_DONE == id) {
				frame_done_cb((struct vdec_context *)&m_vdec_cfg,(struct vdec_frame *)p2);
			} else if (OMX_COMPONENT_GENERATE_FTB == id && p2) {
				ftb_buf_hdr = (OMX_BUFFERHEADERTYPE *)p2;
				frame_done_cb((struct vdec_context *)&m_vdec_cfg,(struct vdec_frame *)(ftb_buf_hdr->pOutputPortPrivate));
			}
		}

	}



	return bRet; 
}
/*=========================================================================
FUNCTION : execute_input_flush

DESCRIPTION
  Executes the OMX flush at INPUT PORT.

PARAMETERS
  None.

RETURN VALUE
  true/false
==========================================================================*/
bool omx_vdec::execute_input_flush(void)
{
	bool bRet = false; 
	OMX_BUFFERHEADERTYPE *pInpBufHdr = (OMX_BUFFERHEADERTYPE *)m_inp_mem_ptr;
	unsigned int p1 = 0;
	unsigned int p2 = 0;
	unsigned id = 0;
	if(!pInpBufHdr)
	{
		DEBUG_PRINT_ERROR("Error - Omx Input Flush issued when input buffer is not initialized yet.\n"); 
	}
	else
	{
		int nFlushFrameCnt = 0;
		unsigned i; 
		/*  Take care of the pending buffers in the input side */
		/* During flush clear the pending buffer index. Otherwise FTB followed by flush 
		 ** could push duplicate frames to the decoder 
		 */
		m_first_pending_buf_idx   = -1;
		for(i=0;i<m_inp_buf_count;i++,pInpBufHdr++)
		{
			if(BITMASK_PRESENT((&m_flags), (OMX_COMPONENT_FIRST_BUFFER_PENDING+i)))
			{
				BITMASK_CLEAR((&m_flags),(OMX_COMPONENT_FIRST_BUFFER_PENDING+i));
				post_event((unsigned)&m_vdec_cfg,(unsigned)pInpBufHdr,OMX_COMPONENT_GENERATE_BUFFER_DONE);
			}
		}
		DEBUG_PRINT("m_etb_q.m_size = %d\n",m_etb_q.m_size);
		while(m_etb_q.m_size > 0)
		{
			DEBUG_PRINT("Flushing ETB Q\n");
			m_etb_q.pop_entry(&p1,&p2,&id); 
			buffer_done_cb((struct vdec_context *)&m_vdec_cfg,(void *)p2); 
		}

	}

	return bRet; 
}


/* ======================================================================
FUNCTION
  omx_vdec::SendCommandEvent

DESCRIPTION
  Send the event to decoder pipe.  This is needed to generate the callbacks 
  in decoder thread context. 

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
bool omx_vdec::post_event(unsigned int p1, 
                          unsigned int p2, 
                          unsigned int id)
{
  bool bRet      =                      false; 

#ifndef PC_DEBUG
  pthread_mutex_lock(&m_lock); 
#endif // PC_DEBUG
  m_cmd_cnt ++; 
  if(id == OMX_COMPONENT_GENERATE_FTB || (id == OMX_COMPONENT_GENERATE_FRAME_DONE))
  {
    m_ftb_q.insert_entry(p1,p2,id); 
  }
  else if((id == OMX_COMPONENT_GENERATE_ETB)  || (id == OMX_COMPONENT_GENERATE_EBD) || (id == OMX_COMPONENT_GENERATE_BUFFER_DONE))
  //else if((id == OMX_COMPONENT_GENERATE_ETB)  )
  {
    m_etb_q.insert_entry(p1,p2,id); 
  }
  else
  {
    m_cmd_q.insert_entry(p1,p2,id); 
  }

  bRet = true; 
  post_message(this, id); 
#ifndef PC_DEBUG
  pthread_mutex_unlock(&m_lock); 
#endif // PC_DEBUG
  // for all messages that needs a callback before
  // vdec is actually opened, skips the proxy
  DEBUG_PRINT("Post -->%d[%x,%d]ebd %d fbd %d oc %d %x,%x \n",m_state,
          (unsigned)m_vdec,id, m_etb_cnt,
          m_fbd_cnt,m_outstanding_frames,m_flags >> 3,
          m_out_flags); 
  return bRet; 
}

/* ======================================================================
FUNCTION
  omx_vdec::GetParameter

DESCRIPTION
  OMX Get Parameter method implementation 

PARAMETERS
  <TBD>.

RETURN VALUE
  Error None if successful. 

========================================================================== */
OMX_ERRORTYPE  omx_vdec::get_parameter(OMX_IN OMX_HANDLETYPE     hComp, 
                                           OMX_IN OMX_INDEXTYPE paramIndex,  
                                           OMX_INOUT OMX_PTR     paramData)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone; 

    DEBUG_PRINT("get_parameter: \n");
    if(m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Get Param in Invalid State\n"); 
        return OMX_ErrorInvalidState;
    }
    if(paramData == NULL)
    {
        DEBUG_PRINT("Get Param in Invalid paramData \n");
        return OMX_ErrorBadParameter;    
    }
  switch(paramIndex)
  {
    case OMX_IndexParamPortDefinition:
    {
      OMX_PARAM_PORTDEFINITIONTYPE *portDefn;
      portDefn = (OMX_PARAM_PORTDEFINITIONTYPE *) paramData;

      DEBUG_PRINT("get_parameter: OMX_IndexParamPortDefinition\n");

        portDefn->nVersion.nVersion = OMX_SPEC_VERSION;
        portDefn->nSize = sizeof(portDefn); 
        portDefn->eDomain    = OMX_PortDomainVideo;
        portDefn->format.video.nFrameHeight =  m_crop_dy;
        portDefn->format.video.nFrameWidth  =  m_crop_dx;
        portDefn->format.video.nStride = m_width;
        portDefn->format.video.nSliceHeight = m_height;
        portDefn->format.video.xFramerate= 25;

      if (0 == portDefn->nPortIndex)
      {
        portDefn->eDir =  OMX_DirInput;
        /*Actual count is based on input buffer count*/
        portDefn->nBufferCountActual = m_inp_buf_count;
        /*Set the Min count*/
        portDefn->nBufferCountMin    = OMX_VIDEO_DEC_NUM_INPUT_BUFFERS;
        portDefn->nBufferSize        = m_inp_buf_size;
        portDefn->format.video.eColorFormat       = OMX_COLOR_FormatUnused;

        if(!strncmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.avc",OMX_MAX_STRINGNAME_SIZE))
        {
            portDefn->format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
        }
        else if(!strncmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.h263",OMX_MAX_STRINGNAME_SIZE))
        {
            portDefn->format.video.eCompressionFormat = OMX_VIDEO_CodingH263;
        }
        else if(!strncmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.mpeg4",OMX_MAX_STRINGNAME_SIZE))
        {
            portDefn->format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG4;
        }
        else if(!strncmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.vc1",OMX_MAX_STRINGNAME_SIZE))
        {
            portDefn->format.video.eCompressionFormat = OMX_VIDEO_CodingWMV;
        }
        else
        {
            DEBUG_PRINT("No more Compression formats supported \n");
            eRet = OMX_ErrorNoMore;
        }

        portDefn->bEnabled   = m_inp_bEnabled;
        portDefn->bPopulated = m_inp_bPopulated;
      }
      else if (1 == portDefn->nPortIndex)
      {
        portDefn->eDir =  OMX_DirOutput;
        portDefn->bEnabled   = m_out_bEnabled;
        portDefn->bPopulated = m_out_bPopulated;
        if((m_height != 0x0) && (m_width != 0x0))
        {
            portDefn->nBufferSize = m_height * m_width * 1.5;
        }
        else
        {
            portDefn->format.video.nFrameHeight =  OMX_CORE_QCIF_HEIGHT;
            portDefn->format.video.nFrameWidth  =  OMX_CORE_QCIF_WIDTH;
            portDefn->format.video.nStride      = OMX_CORE_QCIF_WIDTH;
            portDefn->format.video.nSliceHeight     = OMX_CORE_QCIF_HEIGHT;
            // if set param is init with HxW as 0, then fake the HxW to the user to be QCIF 
            portDefn->nBufferSize = (OMX_CORE_QCIF_HEIGHT * OMX_CORE_QCIF_WIDTH * 1.5);
        } 
	portDefn->nBufferCountActual = m_out_buf_count;
	portDefn->nBufferCountMin    = m_out_buf_count;
        portDefn->format.video.eColorFormat = m_color_format; 
        portDefn->format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
      }
      else
      {
        portDefn->eDir =  OMX_DirMax;
        DEBUG_PRINT(" get_parameter: Bad Port idx %d", 
                 (int)portDefn->nPortIndex);
        eRet = OMX_ErrorBadPortIndex;
      }

      break;
    }
    case OMX_IndexParamVideoInit:
    {
      OMX_PORT_PARAM_TYPE *portParamType = 
                              (OMX_PORT_PARAM_TYPE *) paramData; 
      DEBUG_PRINT("get_parameter: OMX_IndexParamVideoInit\n");

      portParamType->nVersion.nVersion = OMX_SPEC_VERSION;
      portParamType->nSize = sizeof(portParamType); 
      portParamType->nPorts           = 2;
      portParamType->nStartPortNumber = 0;
      break;
    }
    case OMX_IndexParamVideoPortFormat:
    {
      OMX_VIDEO_PARAM_PORTFORMATTYPE *portFmt = 
                     (OMX_VIDEO_PARAM_PORTFORMATTYPE *)paramData;
      DEBUG_PRINT("get_parameter: OMX_IndexParamVideoPortFormat\n");

      portFmt->nVersion.nVersion = OMX_SPEC_VERSION;
      portFmt->nSize             = sizeof(portFmt); 

      if (0 == portFmt->nPortIndex)
      {
	      if (0 == portFmt->nIndex) 
	      {
		      if (!strncmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.avc",OMX_MAX_STRINGNAME_SIZE)) 
		      {
			      portFmt->eColorFormat =  OMX_COLOR_FormatUnused;
			      portFmt->eCompressionFormat = OMX_VIDEO_CodingAVC;
		      }
		      else if (!strncmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.h263",OMX_MAX_STRINGNAME_SIZE)) 
		      {
			      portFmt->eColorFormat =  OMX_COLOR_FormatUnused;
			      portFmt->eCompressionFormat = OMX_VIDEO_CodingH263; 
		      }
		      else if (!strncmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.mpeg4",OMX_MAX_STRINGNAME_SIZE)) 
		      {
			      portFmt->eColorFormat =  OMX_COLOR_FormatUnused;
			      portFmt->eCompressionFormat = OMX_VIDEO_CodingMPEG4;
		      }
          else if (!strncmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.vc1",OMX_MAX_STRINGNAME_SIZE)) 
		      {
			      portFmt->eColorFormat =  OMX_COLOR_FormatUnused;
			      portFmt->eCompressionFormat = OMX_VIDEO_CodingWMV;
		      }
	      }
	      else 
	      {
		      DEBUG_PRINT_ERROR("get_parameter: OMX_IndexParamVideoPortFormat:"\
				      " NoMore compression formats\n");
		      eRet =  OMX_ErrorNoMore; 
	      }
      }
      else if (1 == portFmt->nPortIndex)
      {
        if (0 == portFmt->nIndex)
        {
           portFmt->eColorFormat = 
            (OMX_COLOR_FORMATTYPE)OMX_QCOM_COLOR_FormatYVU420SemiPlanar; 
           portFmt->eCompressionFormat =  OMX_VIDEO_CodingUnused;
        }
        else if (1 == portFmt->nIndex)
        {
           portFmt->eColorFormat = OMX_COLOR_FormatYUV420Planar;
           portFmt->eCompressionFormat = OMX_VIDEO_CodingUnused;  
        }
        else
        {
           DEBUG_PRINT("get_parameter: OMX_IndexParamVideoPortFormat:"\
                  " NoMore Color formats\n");
           eRet =  OMX_ErrorNoMore;
        }
      }
      else
      {
        DEBUG_PRINT_ERROR("get_parameter: Bad port index %d\n", (int)portFmt->nPortIndex);
        eRet = OMX_ErrorBadPortIndex;
      }
      break;
    }
    /*Component should support this port definition*/
    case OMX_IndexParamAudioInit:
    {
        OMX_PORT_PARAM_TYPE *audioPortParamType = (OMX_PORT_PARAM_TYPE *) paramData; 
        DEBUG_PRINT("get_parameter: OMX_IndexParamAudioInit\n");
        audioPortParamType->nVersion.nVersion = OMX_SPEC_VERSION;
        audioPortParamType->nSize = sizeof(audioPortParamType); 
        audioPortParamType->nPorts           = 0;
        audioPortParamType->nStartPortNumber = 0;
        break;
    }
    /*Component should support this port definition*/
    case OMX_IndexParamImageInit:
    {
        OMX_PORT_PARAM_TYPE *imagePortParamType = (OMX_PORT_PARAM_TYPE *) paramData; 
        DEBUG_PRINT("get_parameter: OMX_IndexParamImageInit\n");
        imagePortParamType->nVersion.nVersion = OMX_SPEC_VERSION;
        imagePortParamType->nSize = sizeof(imagePortParamType); 
        imagePortParamType->nPorts           = 0;
        imagePortParamType->nStartPortNumber = 0;
        break;

    }
    /*Component should support this port definition*/
    case OMX_IndexParamOtherInit:
    {
        DEBUG_PRINT_ERROR("get_parameter: OMX_IndexParamOtherInit %08x\n", paramIndex);
        eRet =OMX_ErrorUnsupportedIndex;
        break;
    }
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_PARAM_COMPONENTROLETYPE *comp_role;
        comp_role = (OMX_PARAM_COMPONENTROLETYPE *) paramData;
        comp_role->nVersion.nVersion = OMX_SPEC_VERSION;
        comp_role->nSize = sizeof(*comp_role); 

        DEBUG_PRINT("Getparameter: OMX_IndexParamStandardComponentRole %d\n",paramIndex);
        if(NULL != comp_role->cRole)
        {
            strncpy((char*)comp_role->cRole,(const char*)m_cRole,OMX_MAX_STRINGNAME_SIZE);
        }
        else
        {
            DEBUG_PRINT_ERROR("Getparameter: OMX_IndexParamStandardComponentRole %d is passed with NULL parameter for role\n",paramIndex);
            eRet =OMX_ErrorBadParameter;
        }
        break;
    }
    /* Added for parameter test */
    case OMX_IndexParamPriorityMgmt:
        {

            OMX_PRIORITYMGMTTYPE *priorityMgmType = (OMX_PRIORITYMGMTTYPE *) paramData;
            DEBUG_PRINT("get_parameter: OMX_IndexParamPriorityMgmt\n");
            priorityMgmType->nVersion.nVersion = OMX_SPEC_VERSION;
            priorityMgmType->nSize = sizeof(priorityMgmType);
            
            break;
        }
    /* Added for parameter test */
    case OMX_IndexParamCompBufferSupplier:
        {
            OMX_PARAM_BUFFERSUPPLIERTYPE *bufferSupplierType = (OMX_PARAM_BUFFERSUPPLIERTYPE*) paramData;
            DEBUG_PRINT("get_parameter: OMX_IndexParamCompBufferSupplier\n");
            
            bufferSupplierType->nSize = sizeof(bufferSupplierType);
            bufferSupplierType->nVersion.nVersion = OMX_SPEC_VERSION;
            if(0 == bufferSupplierType->nPortIndex)
                bufferSupplierType->nPortIndex = OMX_BufferSupplyUnspecified;
            else if (1 == bufferSupplierType->nPortIndex)
                bufferSupplierType->nPortIndex = OMX_BufferSupplyUnspecified;
            else
                eRet = OMX_ErrorBadPortIndex;


            break;
        }
    case OMX_IndexParamVideoAvc:
        {
            DEBUG_PRINT("get_parameter: OMX_IndexParamVideoAvc %08x\n", paramIndex);
            break;
        }
    case OMX_IndexParamVideoMpeg4:
        {
            DEBUG_PRINT("get_parameter: OMX_IndexParamVideoMpeg4 %08x\n", paramIndex);
            break;
        }
    case OMX_IndexParamVideoH263:
        {
            DEBUG_PRINT("get_parameter: OMX_IndexParamVideoH263 %08x\n", paramIndex);
            break;
        }
    case OMX_IndexParamVideoWmv:
        {
            DEBUG_PRINT("get_parameter: OMX_IndexParamVideoWMV %08x\n", paramIndex);
            break;
        }
    default:
    {
      DEBUG_PRINT_ERROR("get_parameter: unknown param %08x\n", paramIndex);
      eRet =OMX_ErrorUnsupportedIndex;
    }

  }

  DEBUG_PRINT("\n get_parameter returning Height %d , Width %d \n", m_height, m_width);
  return eRet;

}

/* ======================================================================
FUNCTION
  omx_vdec::Setparameter

DESCRIPTION
  OMX Set Parameter method implementation.

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if successful. 

========================================================================== */
OMX_ERRORTYPE  omx_vdec::set_parameter(OMX_IN OMX_HANDLETYPE     hComp, 
                                           OMX_IN OMX_INDEXTYPE paramIndex, 
                                           OMX_IN OMX_PTR        paramData)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone; 
    int           i;

    if(m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Set Param in Invalid State\n"); 
        return OMX_ErrorInvalidState;
    }
    if(paramData == NULL)
    {
         DEBUG_PRINT("Get Param in Invalid paramData \n");
         return OMX_ErrorBadParameter;    
    }
  switch(paramIndex)
  {
    case OMX_IndexParamPortDefinition:
    {
      OMX_PARAM_PORTDEFINITIONTYPE *portDefn;
      portDefn = (OMX_PARAM_PORTDEFINITIONTYPE *) paramData;

      /*set_parameter can be called in loaded state 
      or disabled port */

      /* When the component is in Loaded state and IDLE Pending*/
      if(((m_state == OMX_StateLoaded)&& 
          !BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING))
         /* Or while the I/P or the O/P port or disabled */
         ||((OMX_DirInput == portDefn->eDir && m_inp_bEnabled == 0)||
         (OMX_DirOutput == portDefn->eDir && m_out_bEnabled == 0)))
      {
       DEBUG_PRINT("Set Parameter called in valid state"); 
      }
      else
      {
         DEBUG_PRINT_ERROR("Set Parameter called in Invalid State\n"); 
         return OMX_ErrorIncorrectStateOperation;
      }
      DEBUG_PRINT("set_parameter: OMX_IndexParamPortDefinition H= %d, W = %d\n",
             (int)portDefn->format.video.nFrameHeight,
             (int)portDefn->format.video.nFrameWidth);

      eRet = omx_vdec_validate_port_param(portDefn->format.video.nFrameHeight,
                                          portDefn->format.video.nFrameWidth,
					  portDefn->nBufferCountActual);
      if(eRet != OMX_ErrorNone)
      {
         return OMX_ErrorUnsupportedSetting;
      }
      if(OMX_DirOutput == portDefn->eDir)
      {
        DEBUG_PRINT("set_parameter: OMX_IndexParamPortDefinition on output port\n");
        DEBUG_PRINT("set_parameter op port: stride %d\n", (int)portDefn->format.video.nStride);
      }
      else if(OMX_DirInput == portDefn->eDir)
      {
         if(m_height != portDefn->format.video.nFrameHeight ||
            m_width  != portDefn->format.video.nFrameWidth)
         {
           DEBUG_PRINT("set_parameter ip port: stride %d\n", (int)portDefn->format.video.nStride);
           // Re-start case since image dimensions have changed
           DEBUG_PRINT("\nSetParameter: Dimension mismatch.  Old H: %d New H: %d Old W: %d New W: %d\n", m_height, (int)portDefn->format.video.nFrameHeight, m_width, (int)portDefn->format.video.nFrameWidth);
           // set the HxW only if non-zero
           if((portDefn->format.video.nFrameHeight != 0x0) && (portDefn->format.video.nFrameWidth != 0x0))
           {
               m_crop_x = m_crop_y = 0;
               m_crop_dy = m_port_height = m_vdec_cfg.height = m_height = portDefn->format.video.nFrameHeight;
               m_crop_dx = m_port_width = m_vdec_cfg.width  = m_width  = portDefn->format.video.nFrameWidth;
               DEBUG_PRINT("\n SetParam with new H %d and W %d\n", m_height, m_width );

               if ((m_height % 16) != 0)
               {
                   DEBUG_PRINT("\n Height %d is not a multiple of 16", m_height);
                   m_vdec_cfg.height = (m_height / 16 + 1) *  16;
                   m_height = m_vdec_cfg.height;
                   DEBUG_PRINT("\n Height %d adjusted to %d \n", m_height, m_vdec_cfg.height);
               }
               if (m_width % 16 != 0)
               {
                   DEBUG_PRINT("\n Width %d is not a multiple of 16", m_width);
                   m_vdec_cfg.width = (m_width / 16 + 1) *  16;
                   m_width = m_vdec_cfg.width;
                   DEBUG_PRINT("\n Width %d adjusted to %d \n", m_width, m_vdec_cfg.width);
               }
           }
        }
        else
        {
          DEBUG_PRINT("\n set_parameter: Image Dimensions same  \n");
        }
        /*
          If actual buffer count is greater than the Min buffer
          count,change the actual count.
          m_inp_buf_count is initialized to OMX_CORE_NUM_INPUT_BUFFERS
          in the constructor
        */
        if ( portDefn->nBufferCountActual > OMX_VIDEO_DEC_NUM_INPUT_BUFFERS )
        {
            m_inp_buf_count = portDefn->nBufferCountActual;
        }
        else if( portDefn->nBufferCountActual < OMX_VIDEO_DEC_NUM_INPUT_BUFFERS ) 
        {
            DEBUG_PRINT(" Set_parameter: Actual buffer count = %d less than Min Buff count", 
                      portDefn->nBufferCountActual);
            eRet = OMX_ErrorBadParameter;
        }

        if ( portDefn->nBufferSize < OMX_VIDEO_DEC_INPUT_BUFFER_SIZE)
        {
            DEBUG_PRINT_ERROR("Set_parameter: Set input buffer size %d less than 256KB", 
                              portDefn->nBufferSize);
            eRet = OMX_ErrorBadParameter;
        }
        else
        {
            m_inp_buf_size = portDefn->nBufferSize;
        }

      }
      else if (portDefn->eDir ==  OMX_DirMax)
      {
          DEBUG_PRINT(" Set_parameter: Bad Port idx %d", 
                      (int)portDefn->nPortIndex);
          eRet = OMX_ErrorBadPortIndex;
      }          
    }
    break;


    case OMX_IndexParamVideoPortFormat:
    {
      OMX_VIDEO_PARAM_PORTFORMATTYPE *portFmt = 
                     (OMX_VIDEO_PARAM_PORTFORMATTYPE *)paramData;
      DEBUG_PRINT("set_parameter: OMX_IndexParamVideoPortFormat %d\n", 
              portFmt->eColorFormat);        

      DEBUG_PRINT("set_parameter: OMX_IndexParamVideoPortFormat %d\n", 
             portFmt->eColorFormat);        
      if(1 == portFmt->nPortIndex)
      {

         m_color_format = portFmt->eColorFormat;
      }
    }
    break;

     case OMX_IndexParamStandardComponentRole:
     {
          OMX_PARAM_COMPONENTROLETYPE *comp_role;
          comp_role = (OMX_PARAM_COMPONENTROLETYPE *) paramData;
          DEBUG_PRINT("set_parameter: OMX_IndexParamStandardComponentRole %s\n", 
                       comp_role->cRole);  
          if(!strncmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.avc",OMX_MAX_STRINGNAME_SIZE))
          {
              if(!strncmp((char*)comp_role->cRole,"video_decoder.avc",OMX_MAX_STRINGNAME_SIZE))
              {
                  strncpy((char*)m_cRole,"video_decoder.avc",OMX_MAX_STRINGNAME_SIZE);
              }
              else
              {
                  DEBUG_PRINT_ERROR("Setparameter: unknown Index %s\n", comp_role->cRole);
                  eRet =OMX_ErrorUnsupportedSetting;
              }
          }
          else if(!strncmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.mpeg4",OMX_MAX_STRINGNAME_SIZE))
          {
              if(!strncmp((const char*)comp_role->cRole,"video_decoder.mpeg4",OMX_MAX_STRINGNAME_SIZE))
              {
                  strncpy((char*)m_cRole,"video_decoder.mpeg4",OMX_MAX_STRINGNAME_SIZE);
              }
              else
              {
                  DEBUG_PRINT_ERROR("Setparameter: unknown Index %s\n", comp_role->cRole);
                  eRet = OMX_ErrorUnsupportedSetting;
              }
          }
          else if(!strncmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.h263",OMX_MAX_STRINGNAME_SIZE))
          {
              if(!strncmp((const char*)comp_role->cRole,"video_decoder.h263",OMX_MAX_STRINGNAME_SIZE))
              {
                  strncpy((char*)m_cRole,"video_decoder.h263",OMX_MAX_STRINGNAME_SIZE);
              }
              else
              {
                  DEBUG_PRINT_ERROR("Setparameter: unknown Index %s\n", comp_role->cRole);
                  eRet =OMX_ErrorUnsupportedSetting;
              }
          }
          else if(!strncmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.vc1",OMX_MAX_STRINGNAME_SIZE))
          {
              if(!strncmp((const char*)comp_role->cRole,"video_decoder.vc1",OMX_MAX_STRINGNAME_SIZE))
              {
                  strncpy((char*)m_cRole,"video_decoder.vc1",OMX_MAX_STRINGNAME_SIZE);
              }
              else
              {
                  DEBUG_PRINT_ERROR("Setparameter: unknown Index %s\n", comp_role->cRole);
                  eRet =OMX_ErrorUnsupportedSetting;
              }
          }
          else
          {
               DEBUG_PRINT_ERROR("Setparameter: unknown param %s\n", m_vdec_cfg.kind);
               eRet = OMX_ErrorInvalidComponentName;
          }
          break;
     }

    case OMX_IndexParamPriorityMgmt:
        {
            if(m_state != OMX_StateLoaded)
            {
               DEBUG_PRINT_ERROR("Set Parameter called in Invalid State\n"); 
               return OMX_ErrorIncorrectStateOperation;
            }
            OMX_PRIORITYMGMTTYPE *priorityMgmtype = (OMX_PRIORITYMGMTTYPE*) paramData;
            DEBUG_PRINT("set_parameter: OMX_IndexParamPriorityMgmt %d\n", 
              priorityMgmtype->nGroupID);        

            DEBUG_PRINT("set_parameter: priorityMgmtype %d\n", 
             priorityMgmtype->nGroupPriority);  

            m_priority_mgm.nGroupID = priorityMgmtype->nGroupID;
            m_priority_mgm.nGroupPriority = priorityMgmtype->nGroupPriority;

            break;
        }

      case OMX_IndexParamCompBufferSupplier:
      {
          OMX_PARAM_BUFFERSUPPLIERTYPE *bufferSupplierType = (OMX_PARAM_BUFFERSUPPLIERTYPE*) paramData;
            DEBUG_PRINT("set_parameter: OMX_IndexParamCompBufferSupplier %d\n", 
                bufferSupplierType->eBufferSupplier);   
             if(bufferSupplierType->nPortIndex == 0 || bufferSupplierType->nPortIndex ==1)
                m_buffer_supplier.eBufferSupplier = bufferSupplierType->eBufferSupplier;
             
             else
         
             eRet = OMX_ErrorBadPortIndex;
 
          break;
        
      }
      case OMX_IndexParamVideoAvc:
          {
              DEBUG_PRINT("set_parameter: OMX_IndexParamVideoAvc %d\n", 
                    paramIndex);        
              break;
          }
      case OMX_IndexParamVideoMpeg4:
          {
              DEBUG_PRINT("set_parameter: OMX_IndexParamVideoMpeg4 %d\n", 
                    paramIndex);        
              break;
          }
      case OMX_IndexParamVideoH263:
          {
              DEBUG_PRINT("set_parameter: OMX_IndexParamVideoH263 %d\n", 
                    paramIndex);        
              break;
          }
      case OMX_IndexParamVideoWmv:
          {
              DEBUG_PRINT("set_parameter: OMX_IndexParamVideoWMV %d\n", 
                    paramIndex);        
              break;
          }
    default:
    {
      DEBUG_PRINT_ERROR("Setparameter: unknown param %d\n", paramIndex);
      eRet = OMX_ErrorUnsupportedIndex;
    }
  }

  return eRet;
}

/* ======================================================================
FUNCTION
  omx_vdec::GetConfig

DESCRIPTION
  OMX Get Config Method implementation. 

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if successful. 

========================================================================== */
OMX_ERRORTYPE  omx_vdec::get_config(OMX_IN OMX_HANDLETYPE      hComp, 
                                        OMX_IN OMX_INDEXTYPE configIndex, 
                                        OMX_INOUT OMX_PTR     configData)
{
    if(m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Get Config in Invalid State\n"); 
        return OMX_ErrorInvalidState;
    }
  DEBUG_PRINT_ERROR("Error: get_config Not Implemented\n"); 
  return OMX_ErrorNotImplemented;
}

/* ======================================================================
FUNCTION
  omx_vdec::SetConfig

DESCRIPTION
  OMX Set Config method implementation 

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if successful. 
========================================================================== */
OMX_ERRORTYPE  omx_vdec::set_config(OMX_IN OMX_HANDLETYPE      hComp, 
                                        OMX_IN OMX_INDEXTYPE configIndex, 
                                        OMX_IN OMX_PTR        configData)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_VIDEO_CONFIG_NALSIZE *pNal;
    
    DEBUG_PRINT("set_config, store the NAL length size\n"); 
    if(m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Set Config in Invalid State\n"); 
        return OMX_ErrorInvalidState;
    }
    if( m_state == OMX_StateExecuting)
    {
        DEBUG_PRINT_ERROR("set_config:Ignore in Exe state\n");
        return ret;
    }
    if(configIndex == OMX_IndexVendorVideoExtraData)
    {
        OMX_VENDOR_EXTRADATATYPE *config = (OMX_VENDOR_EXTRADATATYPE*)configData;
        unsigned height, width, cropx, cropy,cropdx,cropdy;
	unsigned numOutFrames = m_out_buf_count;

        if(!strcmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.avc"))
        {
          OMX_U32 extra_size;
          // Parsing done here for the AVC atom is definitely not generic
          // Currently this piece of code is working, but certainly 
          // not tested with all .mp4 files.
          // Incase of failure, we might need to revisit this
          // for a generic piece of code.
          
          // Retrieve size of NAL length field
          // byte #4 contains the size of NAL lenght field
          m_nalu_bytes = (config->pData[4] & 0x03) + 1;

          extra_size = 0;
          if(m_nalu_bytes > 2)
          {
            /* Presently we assume that only one SPS and one PPS in AvC1 Atom */
            extra_size = (m_nalu_bytes - 2)*2;
          }

          // SPS starts from byte #6
          OMX_U8 *pSrcBuf = (OMX_U8*)(&config->pData[6]);
          OMX_U8 *pDestBuf;
          m_vendor_config.nPortIndex = config->nPortIndex;
          
          // minus 6 --> SPS starts from byte #6
          // minus 1 --> picture param set byte to be ignored from avcatom
          m_vendor_config.nDataSize  = config->nDataSize -6 - 1 + extra_size;
          m_vendor_config.pData = (OMX_U8*)malloc((config->nDataSize -6 -1));
          OMX_U32 len;
          OMX_U8  index=0;
          // case where SPS+PPS is sent as part of set_config
          pDestBuf = m_vendor_config.pData;
          
          DEBUG_PRINT("Rxd SPS+PPS nPortIndex[%d] len[%d] data[0x%x]\n",\
                       m_vendor_config.nPortIndex,
                       m_vendor_config.nDataSize,
                       m_vendor_config.pData);
          while(index<2)
          {
            OMX_U8* psize;
            len = *pSrcBuf;
            len = len << 8;
            len |= *(pSrcBuf+1); 
            psize = (OMX_U8*)&len;
            memcpy(pDestBuf+m_nalu_bytes,pSrcBuf+2,len);
            for( int i = 0; i < m_nalu_bytes; i++ )
            {
              pDestBuf[i] = psize[ m_nalu_bytes - 1 - i ];
            }
            //memcpy(pDestBuf,pSrcBuf,(len+2));
            pDestBuf += len+m_nalu_bytes;
            pSrcBuf += len+2;
            index ++;
            pSrcBuf++;// skip picture param set
            len=0;
          }
        }
        else if((!strcmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.mpeg4"))
            || (!strcmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.h263"))) 
        {
            m_vendor_config.nPortIndex = config->nPortIndex;
            m_vendor_config.nDataSize  = config->nDataSize ;
            m_vendor_config.pData = (OMX_U8*)malloc((config->nDataSize));
            memcpy(m_vendor_config.pData,config->pData,config->nDataSize);
        }
        else if(!strcmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.vc1"))
        {
          if (((*((OMX_U32 *)config->pData)) & VC1_SP_MP_START_CODE_MASK) == VC1_SP_MP_START_CODE) 
          {
            QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_LOW,
                         "set_config - VC1 simple/main profile\n");
            m_vendor_config.nPortIndex = config->nPortIndex;
            m_vendor_config.nDataSize  = config->nDataSize;
            m_vendor_config.pData = (OMX_U8*)malloc(config->nDataSize);
            memcpy(m_vendor_config.pData,config->pData,config->nDataSize);
          }
          else if (*((OMX_U32 *)config->pData) == 0x0F010000)
          {
            QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,
                          "set_config - Error: VC1 Advance profile unsupported\n");
            ret = OMX_ErrorUnsupportedSetting;
          }
          else if ((config->nDataSize == VC1_STRUCT_C_LEN) && 
                   (((*((OMX_U32 *)config->pData) & VC1_STRUCT_C_PROFILE_MASK ) >> 30) == VC1_SIMPLE_PROFILE))
          {
            QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_LOW,
                         "set_config - VC1 Simple profile\n");
            m_vendor_config.nPortIndex = config->nPortIndex;
            m_vendor_config.nDataSize  = config->nDataSize;
            m_vendor_config.pData = (OMX_U8*)malloc(config->nDataSize);
            memcpy(m_vendor_config.pData,config->pData,config->nDataSize);
          }
          else
          {
            QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,
                          "set_config - Error: Unknown VC1 profile\n");
            ret = OMX_ErrorUnsupportedSetting;
          }  
        }
        if ((ret == OMX_ErrorNone) && 
            !omx_vdec_check_port_settings(config->pData, config->nDataSize, height, width,
                                                        cropx, cropy,cropdx,cropdy,numOutFrames)) 
        {
          ret = OMX_ErrorUnsupportedSetting;
        }
    }
    else if(configIndex == OMX_IndexConfigVideoNalSize)
    {
        pNal = reinterpret_cast<OMX_VIDEO_CONFIG_NALSIZE*>(configData);
        m_nalu_bytes = pNal->nNaluBytes; 
        if(m_nalu_bytes < 1 || m_nalu_bytes >4)
        {
            DEBUG_PRINT_ERROR("set_config, invalid NAL length size [%d]\n",m_nalu_bytes);
            m_nalu_bytes = 4; 
            ret = OMX_ErrorBadParameter;
        }
    }
    return ret;
}

/* ======================================================================
FUNCTION
  omx_vdec::GetExtensionIndex

DESCRIPTION
  OMX GetExtensionIndex method implementaion.  <TBD>

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if everything successful. 

========================================================================== */
OMX_ERRORTYPE  omx_vdec::get_extension_index(OMX_IN OMX_HANDLETYPE      hComp, 
                                                OMX_IN OMX_STRING      paramName, 
                                                OMX_OUT OMX_INDEXTYPE* indexType)
{
    DEBUG_PRINT_ERROR("get_extension_index: Error, Not implemented\n");
    if(m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Get Extension Index in Invalid State\n"); 
        return OMX_ErrorInvalidState;
    }
    return OMX_ErrorNotImplemented;
}

/* ======================================================================
FUNCTION
  omx_vdec::GetState

DESCRIPTION
  Returns the state information back to the caller.<TBD>

PARAMETERS
  <TBD>.

RETURN VALUE
  Error None if everything is successful. 
========================================================================== */
OMX_ERRORTYPE  omx_vdec::get_state(OMX_IN OMX_HANDLETYPE  hComp, 
                                       OMX_OUT OMX_STATETYPE* state)
{
  *state = m_state; 
  DEBUG_PRINT("get_state: Returning the state %d\n",*state); 
  return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
  omx_vdec::ComponentTunnelRequest

DESCRIPTION
  OMX Component Tunnel Request method implementation. <TBD>

PARAMETERS
  None.

RETURN VALUE
  OMX Error None if everything successful. 

========================================================================== */
OMX_ERRORTYPE  omx_vdec::component_tunnel_request(OMX_IN OMX_HANDLETYPE                hComp, 
                                                     OMX_IN OMX_U32                        port, 
                                                     OMX_IN OMX_HANDLETYPE        peerComponent, 
                                                     OMX_IN OMX_U32                    peerPort, 
                                                     OMX_INOUT OMX_TUNNELSETUPTYPE* tunnelSetup)
{
  DEBUG_PRINT_ERROR("Error: component_tunnel_request Not Implemented\n");
  return OMX_ErrorNotImplemented;
}

/* ======================================================================
FUNCTION
  omx_vdec::UseInputBuffer

DESCRIPTION
  Helper function for Use buffer in the input pin

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_vdec::use_input_buffer(
                         OMX_IN OMX_HANDLETYPE            hComp,
                         OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                         OMX_IN OMX_U32                   port,
                         OMX_IN OMX_PTR                   appData,
                         OMX_IN OMX_U32                   bytes, 
                         OMX_IN OMX_U8*                   buffer)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE       *bufHdr; // buffer header
    unsigned                         i; // Temporary counter
    if(bytes <= m_inp_buf_size)
    {
        if(!m_inp_mem_ptr)
        {
            int nBufHdrSize   = m_inp_buf_count * sizeof(OMX_BUFFERHEADERTYPE);
            m_inp_bm_count    = BITMASK_SIZE(m_inp_buf_count);
            m_inp_mem_ptr     = (char *)calloc( (nBufHdrSize + m_inp_bm_count ), 1);

            if(m_inp_mem_ptr)
            { 
                /*First time input should be null*/
                if (input)
                {
                    free (m_inp_mem_ptr);
                    m_inp_mem_ptr = NULL;
                    return OMX_ErrorUndefined;
                }
                /* Allocate array of pointers based on actual buffer count */ 
                input = (OMX_BUFFERHEADERTYPE **) 
                malloc(sizeof(OMX_BUFFERHEADERTYPE*) * m_inp_buf_count);

                /*If allocation fails return error*/
                if (input == NULL)
                {
                    free (m_inp_mem_ptr);
                    m_inp_mem_ptr = NULL;
                    return OMX_ErrorInsufficientResources;
                 }

                // We have valid Input memory block here
                DEBUG_PRINT("Ist User Input Buffer(%d,%d,%d)\n", \
                                                 m_inp_buf_count,
                                                 nBufHdrSize,
                                                 m_inp_bm_count);
                *bufferHdr    = (OMX_BUFFERHEADERTYPE *) m_inp_mem_ptr;
                bufHdr        = (OMX_BUFFERHEADERTYPE *) m_inp_mem_ptr;
                input[0]=*bufferHdr;
                BITMASK_SET(&m_inp_bm_count,0);
                // Settting the entire storage nicely
                for(i=0; i < m_inp_buf_count ; i++, bufHdr++)
                {
                    memset(bufHdr,0,sizeof(OMX_BUFFERHEADERTYPE));
                    bufHdr->nSize             = sizeof(OMX_BUFFERHEADERTYPE);
                    bufHdr->nVersion.nVersion = OMX_SPEC_VERSION;
                    bufHdr->nAllocLen         = m_inp_buf_size;
                    bufHdr->pAppPrivate       = appData;
                    bufHdr->nInputPortIndex   = OMX_CORE_INPUT_PORT_INDEX;
                }
            }
            else
            {
                DEBUG_PRINT("Input buffer memory allocation failed\n");
                eRet =  OMX_ErrorInsufficientResources;
            }
        }
        else
        {

            for(i=0; i< m_inp_buf_count; i++)
            {
                if(BITMASK_ABSENT(&m_inp_bm_count,i))
                {
                    // bit space available
                    break;
                }
            }
            if(i < m_inp_buf_count)
            {
                *bufferHdr = ((OMX_BUFFERHEADERTYPE *)m_inp_mem_ptr) + i ;
                (*bufferHdr)->pAppPrivate = appData;
                BITMASK_SET(&m_inp_bm_count,i);
                input[i] = *bufferHdr;
            }
            else
            { 
                eRet = OMX_ErrorInsufficientResources;
            }
        }
        (*bufferHdr)->pBuffer = (OMX_U8 *)buffer; 
        DEBUG_PRINT("BUffer Header[%p] buffer=%p\n",\
                                         *bufferHdr,(*bufferHdr)->pBuffer);
    }
    else
    { 
        eRet = OMX_ErrorInsufficientResources;
    }
    return eRet;
}


/* ======================================================================
FUNCTION
  omx_vdec::UseOutputBuffer

DESCRIPTION
  Helper function for Use buffer in the input pin

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_vdec::use_output_buffer(
                         OMX_IN OMX_HANDLETYPE            hComp,
                         OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                         OMX_IN OMX_U32                   port,
                         OMX_IN OMX_PTR                   appData,
                         OMX_IN OMX_U32                   bytes, 
                         OMX_IN OMX_U8*                   buffer)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone; 
    OMX_BUFFERHEADERTYPE       *bufHdr; // buffer header
    unsigned                         i; // Temporary counter
    if(!m_out_mem_ptr)
    {
        int nBufHdrSize        = 0; 
        int nPlatformEntrySize = 0; 
        int nPlatformListSize  = 0; 
        int nPMEMInfoSize = 0; 
        OMX_QCOM_PLATFORM_PRIVATE_LIST      *pPlatformList; 
        OMX_QCOM_PLATFORM_PRIVATE_ENTRY     *pPlatformEntry; 
        OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *pPMEMInfo; 

        DEBUG_PRINT("Ist Use Output Buffer(%d)\n",\
                                            m_out_buf_count); 
        nBufHdrSize        = m_out_buf_count*sizeof(OMX_BUFFERHEADERTYPE); 
        nPMEMInfoSize      = m_out_buf_count * 
                              sizeof(OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO); 
        nPlatformListSize  = m_out_buf_count *
                                   sizeof(OMX_QCOM_PLATFORM_PRIVATE_LIST); 
        nPlatformEntrySize = m_out_buf_count *
                                  sizeof(OMX_QCOM_PLATFORM_PRIVATE_ENTRY); 
        DEBUG_PRINT("UOB::TotalBufHdr %d BufHdrSize %d PMEM %d PL %d\n",\
                         nBufHdrSize,
                         sizeof(OMX_BUFFERHEADERTYPE),
                         nPMEMInfoSize,
                         nPlatformListSize); 
        DEBUG_PRINT("UOB::PE %d bmSize %d \n",nPlatformEntrySize,
                         m_out_bm_count); 

        /* 
         * Memory for output side involves the following: 
         * 1. Array of Buffer Headers 
         * 2. Platform specific information List 
         * 3. Platform specific Entry List 
         * 4. PMem Information entries 
         * 5. Bitmask array to hold the buffer allocation details
         * In order to minimize the memory management entire allocation 
         * is done in one step. 
         */
        // Alloc mem for out buffer headers
        m_out_mem_ptr = (char *)calloc(nBufHdrSize , 1); 
        // Alloc mem for platform specific info
        char *pPtr=NULL;
        pPtr = (char*) calloc(nPlatformListSize + nPlatformEntrySize +
                              nPMEMInfoSize,1);
        // Alloc mem for maintaining a copy of use buf headers
        char *omxHdr = (char*)calloc(nBufHdrSize,1);
        m_loc_use_buf_hdr = (OMX_BUFFERHEADERTYPE*)omxHdr;

        if(m_out_mem_ptr && m_loc_use_buf_hdr && pPtr )
        {
            bufHdr          = (OMX_BUFFERHEADERTYPE *) m_out_mem_ptr; 
            m_platform_list = (OMX_QCOM_PLATFORM_PRIVATE_LIST *) pPtr ;
            m_platform_entry= (OMX_QCOM_PLATFORM_PRIVATE_ENTRY *)
                              (((char*) m_platform_list) + nPlatformListSize);
            m_pmem_info     = (OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *)
                              (((char*)m_platform_entry) + nPlatformEntrySize);
            pPlatformList   = m_platform_list;
            pPlatformEntry  = m_platform_entry;
            pPMEMInfo       = m_pmem_info;
            DEBUG_PRINT("UOB::Memory Allocation Succeeded for OUT port%p\n",\
                                                          m_out_mem_ptr);

            // Settting the entire storage nicely
            DEBUG_PRINT("UOB::bHdr %p OutMem %p PE %p pmem[%p]\n",bufHdr, 
                                                    m_out_mem_ptr,
                                                    pPlatformEntry,
                                                    pPMEMInfo); 
            for(i=0; i < m_out_buf_count ; i++)
            {
                memset(bufHdr,0,sizeof(OMX_BUFFERHEADERTYPE)); 
                bufHdr->nSize              = sizeof(OMX_BUFFERHEADERTYPE); 
                bufHdr->nVersion.nVersion  = OMX_SPEC_VERSION; 
                // Set the values when we determine the right HxW param
                bufHdr->nAllocLen          = get_output_buffer_size();
                bufHdr->nFilledLen         = 0;
                bufHdr->pAppPrivate        = appData; 
                bufHdr->nOutputPortIndex   = OMX_CORE_OUTPUT_PORT_INDEX; 
                // Platform specific PMEM Information 
                // Initialize the Platform Entry 
                pPlatformEntry->type       = OMX_QCOM_PLATFORM_PRIVATE_PMEM; 
                pPlatformEntry->entry      = pPMEMInfo;
                // Initialize the Platform List 
                pPlatformList->nEntries    = 1; 
                pPlatformList->entryList   = pPlatformEntry;

                // Assign the buffer space to the bufHdr
                bufHdr->pBuffer            = buffer;
                // Keep this NULL till vdec_open is done
                bufHdr->pOutputPortPrivate = NULL; 
                pPMEMInfo->offset          =  0; 
                pPMEMInfo->pmem_fd = 0; 
                bufHdr->pPlatformPrivate = pPlatformList; 
                // Move the buffer and buffer header pointers 
                bufHdr++; 
                pPMEMInfo++; 
                pPlatformEntry++;
                pPlatformList++;
            }
            *bufferHdr = (OMX_BUFFERHEADERTYPE *)m_out_mem_ptr;
            BITMASK_SET(&m_out_bm_count,0x0); 
        }
        else
        {
            DEBUG_PRINT_ERROR("Output buf mem alloc failed[0x%x][0x%x][0x%x]\n",\
                               m_out_mem_ptr, m_loc_use_buf_hdr, pPtr); 
            eRet =  OMX_ErrorInsufficientResources; 
            return eRet;
        }
    }
    else
    {
        for(i=0; i< m_out_buf_count; i++)
        {
            if(BITMASK_ABSENT(&m_out_bm_count,i))
            {
                break; 
            }
        }
        if(i < m_out_buf_count)
        {
            // found an empty buffer at i
            *bufferHdr = ((OMX_BUFFERHEADERTYPE *)m_out_mem_ptr) + i ;
            (*bufferHdr)->pAppPrivate = appData; 
            (*bufferHdr)->pBuffer     = buffer;
            BITMASK_SET(&m_out_bm_count,i);
        }
        else
        {
            DEBUG_PRINT("All Output Buf Allocated:\n"); 
            eRet = OMX_ErrorInsufficientResources; 
            return eRet;
        }
    }
    if(allocate_done())
    {
        omx_vdec_display_in_buf_hdrs();
        omx_vdec_display_out_buf_hdrs();
        omx_vdec_set_use_buf_flg();
    }
    return eRet; 
}

/* ======================================================================
FUNCTION
  omx_vdec::UseBuffer

DESCRIPTION
  OMX Use Buffer method implementation. 

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None , if everything successful. 

========================================================================== */
OMX_ERRORTYPE  omx_vdec::use_buffer(
                         OMX_IN OMX_HANDLETYPE            hComp, 
                         OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr, 
                         OMX_IN OMX_U32                   port, 
                         OMX_IN OMX_PTR                   appData, 
                         OMX_IN OMX_U32                   bytes, 
                         OMX_IN OMX_U8*                   buffer)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    if(m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Use Buffer in Invalid State\n"); 
        return OMX_ErrorInvalidState;
    }
    if(port == OMX_CORE_INPUT_PORT_INDEX)
    {
      eRet = use_input_buffer(hComp,bufferHdr,port,appData,bytes,buffer);
    }
    else if(port == OMX_CORE_OUTPUT_PORT_INDEX)
    {
      eRet = use_output_buffer(hComp,bufferHdr,port,appData,bytes,buffer);
    }
    else
    {
      DEBUG_PRINT_ERROR("Error: Invalid Port Index received %d\n",(int)port);
      eRet = OMX_ErrorBadPortIndex;
    }

    if(eRet == OMX_ErrorNone)
    {
      if(allocate_done()){
        if(BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING))
        {
          // Send the callback now
          BITMASK_CLEAR((&m_flags),OMX_COMPONENT_IDLE_PENDING); 
          post_event(OMX_CommandStateSet,OMX_StateIdle,
                             OMX_COMPONENT_GENERATE_EVENT); 
        }
      }
      if(port == OMX_CORE_INPUT_PORT_INDEX && m_inp_bPopulated)
      {
	      if(BITMASK_PRESENT(&m_flags,OMX_COMPONENT_INPUT_ENABLE_PENDING))
	      {
		      BITMASK_CLEAR((&m_flags),OMX_COMPONENT_INPUT_ENABLE_PENDING);
		      post_event(OMX_CommandPortEnable,
				      OMX_CORE_INPUT_PORT_INDEX,
				      OMX_COMPONENT_GENERATE_EVENT); 
	      }

      }
      else if(port == OMX_CORE_OUTPUT_PORT_INDEX && m_out_bPopulated)
      {
          if(BITMASK_PRESENT(&m_flags,OMX_COMPONENT_OUTPUT_ENABLE_PENDING))

        {
          // Populate the Buffer Headers
          omx_vdec_get_out_buf_hdrs();           
          // Populate Use Buffer Headers
          if(omx_vdec_get_use_buf_flg())
          {
            omx_vdec_dup_use_buf_hdrs();
            omx_vdec_get_out_use_buf_hdrs();
            omx_vdec_add_entries();
            omx_vdec_display_out_buf_hdrs();
          }
             BITMASK_CLEAR((&m_flags),OMX_COMPONENT_OUTPUT_ENABLE_PENDING);
          post_event(OMX_CommandPortEnable,
                     OMX_CORE_OUTPUT_PORT_INDEX,
                     OMX_COMPONENT_GENERATE_EVENT); 
          m_event_port_settings_sent = false;
        }
      }
    }
    return eRet;
}


/* ======================================================================
FUNCTION
  omx_vdec::AllocateInputBuffer

DESCRIPTION
  Helper function for allocate buffer in the input pin

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_vdec::allocate_input_buffer(
                         OMX_IN OMX_HANDLETYPE            hComp, 
                         OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr, 
                         OMX_IN OMX_U32                   port, 
                         OMX_IN OMX_PTR                   appData, 
                         OMX_IN OMX_U32                   bytes)
{
  OMX_ERRORTYPE eRet = OMX_ErrorNone; 
  OMX_BUFFERHEADERTYPE       *bufHdr; // buffer header
  char*                          buf; // buffer pointer 
  unsigned                         i; // Temporary counter

  if(bytes <= m_inp_buf_size)
  {
    if(!m_inp_mem_ptr)
    {
      int nBufHdrSize   = m_inp_buf_count * sizeof(OMX_BUFFERHEADERTYPE); 
      int nBufSize      = m_inp_buf_count * m_inp_buf_size;
      //m_inp_bm_count    = BITMASK_SIZE(m_inp_buf_count); 
      //m_inp_mem_ptr     = (char *)calloc( (nBufSize + nBufHdrSize ), 1); 
      m_inp_mem_ptr     = (char *)calloc( (nBufHdrSize ), 1); 
      m_inp_buf_mem_ptr     = (char *)calloc( (nBufSize ), 1); 

      if(m_inp_mem_ptr)
      {
         /*First time input should be null*/
         if (input)
         {
            free (m_inp_mem_ptr);
            m_inp_mem_ptr = NULL;
            free (m_inp_buf_mem_ptr);
            m_inp_buf_mem_ptr = NULL;
            return OMX_ErrorUndefined;
         }
        /* Allocate array of pointers based on actual buffer count */ 
        input = (OMX_BUFFERHEADERTYPE **) 
        malloc(sizeof(OMX_BUFFERHEADERTYPE*) * m_inp_buf_count);

        /*If allocation fails return error*/
        if (input == NULL)
        {
            free (m_inp_mem_ptr);
            m_inp_mem_ptr = NULL;
            free (m_inp_buf_mem_ptr);
            m_inp_buf_mem_ptr = NULL;
            return OMX_ErrorInsufficientResources;
        }
        // We have valid Input memory block here
        DEBUG_PRINT("Allocating First Input Buffer(%d,%d,%d)\n",\
                                                          m_inp_buf_count,
                                                          nBufHdrSize,
                                                          m_inp_bm_count); 
        *bufferHdr    =
        bufHdr        = (OMX_BUFFERHEADERTYPE *) m_inp_mem_ptr; 
        //buf           = ((char *)bufHdr) + nBufHdrSize ; 
        buf           = m_inp_buf_mem_ptr; 
        input[0]=*bufferHdr; 
        BITMASK_SET(&m_inp_bm_count,0);

        // Settting the entire storage nicely
        for(i=0; i < m_inp_buf_count ; i++, bufHdr++,
                     buf+=m_inp_buf_size)
        {
          memset(bufHdr,0,sizeof(OMX_BUFFERHEADERTYPE)); 
          bufHdr->pBuffer           = (OMX_U8 *)buf; 
          bufHdr->nSize             = sizeof(OMX_BUFFERHEADERTYPE); 
          bufHdr->nVersion.nVersion = OMX_SPEC_VERSION; 
          bufHdr->nAllocLen         = m_inp_buf_size; 
          bufHdr->pAppPrivate       = appData; 
          bufHdr->nInputPortIndex   = OMX_CORE_INPUT_PORT_INDEX; 
          DEBUG_PRINT("Input Buffer %p bufHdr[%p]\n",bufHdr->pBuffer,bufHdr); 
          //buf += OMX_CORE_INPUT_BUFFER_SIZE;
        }
      }
      else
      {
        DEBUG_PRINT("Input buffer memory allocation failed\n"); 
        eRet =  OMX_ErrorInsufficientResources; 
      }
    }
    else
    {
      for(i=0; i< m_inp_buf_count; i++)
      {
        if(BITMASK_ABSENT(&m_inp_bm_count,i))
        {
          // bit space available
          break; 
        }
      }
      if(i < m_inp_buf_count)
      {
        // found an empty buffer at i
        *bufferHdr = ((OMX_BUFFERHEADERTYPE *)m_inp_mem_ptr) + i ;
        (*bufferHdr)->pAppPrivate = appData; 
        BITMASK_SET(&m_inp_bm_count,i);
        input[i] = *bufferHdr;
      }
      else
      {
        eRet = OMX_ErrorInsufficientResources; 
      }

    }
  }
  else
  {
    eRet = OMX_ErrorInsufficientResources; 
  }
  return eRet; 
}


/* ======================================================================
FUNCTION
  omx_vdec::AllocateOutputBuffer

DESCRIPTION
  Helper fn for AllocateBuffer in the output pin

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if everything went well.

========================================================================== */
OMX_ERRORTYPE  omx_vdec::allocate_output_buffer(
                         OMX_IN OMX_HANDLETYPE            hComp, 
                         OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr, 
                         OMX_IN OMX_U32                   port, 
                         OMX_IN OMX_PTR                   appData, 
                         OMX_IN OMX_U32                   bytes)
{
  OMX_ERRORTYPE eRet = OMX_ErrorNone; 
  OMX_BUFFERHEADERTYPE       *bufHdr; // buffer header
  unsigned                         i; // Temporary counter
  if(!m_out_mem_ptr)
  {
    int nBufHdrSize        = 0; 
    int nPlatformEntrySize = 0; 
    int nPlatformListSize  = 0; 
    int nPMEMInfoSize = 0; 
    OMX_QCOM_PLATFORM_PRIVATE_LIST      *pPlatformList;
    OMX_QCOM_PLATFORM_PRIVATE_ENTRY     *pPlatformEntry;
    OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *pPMEMInfo;

    DEBUG_PRINT("Allocating First Output Buffer(%d)\n",m_out_buf_count); 
    nBufHdrSize        = m_out_buf_count * sizeof(OMX_BUFFERHEADERTYPE); 

    nPMEMInfoSize      = m_out_buf_count * 
                         sizeof(OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO); 
    nPlatformListSize  = m_out_buf_count *
                         sizeof(OMX_QCOM_PLATFORM_PRIVATE_LIST); 
    nPlatformEntrySize = m_out_buf_count *
                         sizeof(OMX_QCOM_PLATFORM_PRIVATE_ENTRY);
 
    DEBUG_PRINT("TotalBufHdr %d BufHdrSize %d PMEM %d PL %d\n",nBufHdrSize,
                         sizeof(OMX_BUFFERHEADERTYPE),
                         nPMEMInfoSize,
                         nPlatformListSize); 
    DEBUG_PRINT("PE %d bmSize %d \n",nPlatformEntrySize,
                         m_out_bm_count); 

    /* 
     * Memory for output side involves the following: 
     * 1. Array of Buffer Headers 
     * 2. Platform specific information List 
     * 3. Platform specific Entry List 
     * 4. PMem Information entries 
     * 5. Bitmask array to hold the buffer allocation details
     * In order to minimize the memory management entire allocation 
     * is done in one step. 
     */
    m_out_mem_ptr = (char *)calloc(nBufHdrSize,1); 
    // Alloc mem for platform specific info 
    char *pPtr=NULL;
    pPtr = (char*) calloc(nPlatformListSize + nPlatformEntrySize +
                                     nPMEMInfoSize,1);
    if(m_out_mem_ptr && pPtr)
    {
      bufHdr          = (OMX_BUFFERHEADERTYPE *) m_out_mem_ptr; 
      m_platform_list = (OMX_QCOM_PLATFORM_PRIVATE_LIST *)(pPtr);
      m_platform_entry= (OMX_QCOM_PLATFORM_PRIVATE_ENTRY *)
                        (((char *) m_platform_list)  + nPlatformListSize);
      m_pmem_info     = (OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *)
                        (((char *) m_platform_entry) + nPlatformEntrySize);
      pPlatformList   = m_platform_list;
      pPlatformEntry  = m_platform_entry;
      pPMEMInfo       = m_pmem_info;

      DEBUG_PRINT("Memory Allocation Succeeded for OUT port%p\n",m_out_mem_ptr);

      // Settting the entire storage nicely
      DEBUG_PRINT("bHdr %p OutMem %p PE %p\n",bufHdr, m_out_mem_ptr,pPlatformEntry); 
      DEBUG_PRINT(" Pmem Info = %p \n",pPMEMInfo); 
      for(i=0; i < m_out_buf_count ; i++)
      {
        memset(bufHdr,0,sizeof(OMX_BUFFERHEADERTYPE)); 
        bufHdr->nSize              = sizeof(OMX_BUFFERHEADERTYPE); 
        bufHdr->nVersion.nVersion  = OMX_SPEC_VERSION; 
        // Set the values when we determine the right HxW param
        bufHdr->nAllocLen          = bytes; 
        bufHdr->nFilledLen         = 0; 
        bufHdr->pAppPrivate        = appData; 
        bufHdr->nOutputPortIndex   = OMX_CORE_OUTPUT_PORT_INDEX; 
        // Platform specific PMEM Information 
        // Initialize the Platform Entry 
        //DEBUG_PRINT("Initializing the Platform Entry for %d\n",i); 
        pPlatformEntry->type       = OMX_QCOM_PLATFORM_PRIVATE_PMEM; 
        pPlatformEntry->entry      = pPMEMInfo;
        // Initialize the Platform List 
        pPlatformList->nEntries    = 1; 
        pPlatformList->entryList   = pPlatformEntry;
        // Keep pBuffer NULL till vdec is opened
        bufHdr->pBuffer            = (OMX_U8 *)0xDEADBEEF;
        bufHdr->pOutputPortPrivate = NULL; 
        pPMEMInfo->offset          =  0; 
          pPMEMInfo->pmem_fd = 0; 
        bufHdr->pPlatformPrivate = pPlatformList; 
        // Move the buffer and buffer header pointers 
        bufHdr++; 
        pPMEMInfo++; 
        pPlatformEntry++;
        pPlatformList++;
      }
      *bufferHdr = (OMX_BUFFERHEADERTYPE *)m_out_mem_ptr;
      //m_out_bm_ptr[0]=0x1; 
      BITMASK_SET(&m_out_bm_count,0);
    }
    else
    {
      DEBUG_PRINT_ERROR("Output buf mem alloc failed[0x%x][0x%x]\n",\
                                        m_out_mem_ptr, pPtr); 
      eRet =  OMX_ErrorInsufficientResources; 
    }
  }
  else
  {
    for(i=0; i< m_out_buf_count; i++)
    {
      if(BITMASK_ABSENT(&m_out_bm_count,i))
      {
        break; 
      }
    }
    if(i < m_out_buf_count)
    {
      // found an empty buffer at i
      *bufferHdr = ((OMX_BUFFERHEADERTYPE *)m_out_mem_ptr) + i ;
      (*bufferHdr)->pAppPrivate = appData; 
      BITMASK_SET(&m_out_bm_count,i);
    }
    else
    {
      DEBUG_PRINT("All the Output Buffers have been Allocated ; Returning Insufficient \n"); 
      eRet = OMX_ErrorInsufficientResources; 
    }

  }
  return eRet; 
}


// AllocateBuffer  -- API Call
/* ======================================================================
FUNCTION
  omx_vdec::AllocateBuffer

DESCRIPTION
  Returns zero if all the buffers released..

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_vdec::allocate_buffer(OMX_IN OMX_HANDLETYPE                hComp, 
                                     OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr, 
                                     OMX_IN OMX_U32                        port, 
                                     OMX_IN OMX_PTR                     appData, 
                                     OMX_IN OMX_U32                       bytes)
{

    OMX_ERRORTYPE eRet = OMX_ErrorNone; // OMX return type 
                 
    DEBUG_PRINT("\n Allocate buffer on port %d \n", (int)port);
    if(m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Allocate Buf in Invalid State\n"); 
        return OMX_ErrorInvalidState;
    }

    // What if the client calls again. 
    if(port == OMX_CORE_INPUT_PORT_INDEX)
    {
      eRet = allocate_input_buffer(hComp,bufferHdr,port,appData,bytes);
    }
    else if(port == OMX_CORE_OUTPUT_PORT_INDEX)
    {
      eRet = allocate_output_buffer(hComp,bufferHdr,port,appData,bytes);
    }
    else
    {
      DEBUG_PRINT_ERROR("Error: Invalid Port Index received %d\n",(int)port); 
      eRet = OMX_ErrorBadPortIndex; 
    }
    DEBUG_PRINT("Checking for Output Allocate buffer Done"); 
    if(eRet == OMX_ErrorNone)
    {
        if(allocate_done()){
            if(BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING))
            {
                // Send the callback now
                BITMASK_CLEAR((&m_flags),OMX_COMPONENT_IDLE_PENDING); 
                post_event(OMX_CommandStateSet,OMX_StateIdle,
                                   OMX_COMPONENT_GENERATE_EVENT); 
            }
        }
        if(port == OMX_CORE_INPUT_PORT_INDEX && m_inp_bPopulated)
        {
          if(BITMASK_PRESENT(&m_flags,OMX_COMPONENT_INPUT_ENABLE_PENDING))
          {
             BITMASK_CLEAR((&m_flags),OMX_COMPONENT_INPUT_ENABLE_PENDING); 
             post_event(OMX_CommandPortEnable,
                        OMX_CORE_INPUT_PORT_INDEX,
                        OMX_COMPONENT_GENERATE_EVENT); 
          }
        }
        if(port == OMX_CORE_OUTPUT_PORT_INDEX && m_out_bPopulated)
            {
          if(BITMASK_PRESENT(&m_flags,OMX_COMPONENT_OUTPUT_ENABLE_PENDING))
          {
		  omx_vdec_get_out_buf_hdrs();
#ifdef _ANDROID_
		  DEBUG_PRINT("Here1\n");
		  OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *pPMEMInfo = m_pmem_info;
		  // create IMemoryHeap object
		  //unsigned arena_sz = VDEC_NFRAMES * VDEC_INPUT_SIZE + VDEC_NFRAMES * m_vdec_cfg.outputBuffer.bufferSize;
		  unsigned arena_sz =  VDEC_NFRAMES * VDEC_INPUT_SIZE + m_out_buf_count * m_vdec_cfg.outputBuffer.bufferSize;
		  m_heap_ptr =
			  new VideoHeap(((vdec_frame*)(m_vdec_cfg.outputBuffer.base))->pmem_id,arena_sz , ((vdec_frame*)(m_vdec_cfg.outputBuffer.base))->base);
		  for(unsigned i=0; i < m_out_buf_count ; i++)
		  {
			  DEBUG_PRINT("here pPMEMInfo = %p\n",pPMEMInfo);
			  pPMEMInfo->pmem_fd =  (OMX_U32) m_heap_ptr.get();
			  pPMEMInfo++;
		  }
#endif//_ANDROID_

             BITMASK_CLEAR((&m_flags),OMX_COMPONENT_OUTPUT_ENABLE_PENDING); 
                post_event(OMX_CommandPortEnable,
                           OMX_CORE_OUTPUT_PORT_INDEX,
                           OMX_COMPONENT_GENERATE_EVENT); 
                m_event_port_settings_sent = false;
            }
        }
    }
    DEBUG_PRINT("Allocate Buffer exit with ret Code %d\n",eRet); 
    return eRet;
}

// Free Buffer - API call
/* ======================================================================
FUNCTION
  omx_vdec::FreeBuffer

DESCRIPTION

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_vdec::free_buffer(OMX_IN OMX_HANDLETYPE         hComp, 
                                      OMX_IN OMX_U32                 port, 
                                      OMX_IN OMX_BUFFERHEADERTYPE* buffer)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone; 
    unsigned int nPortIndex; 

    DEBUG_PRINT("In for decoder free_buffer \n");

    if(m_state == OMX_StateIdle && (BITMASK_PRESENT(&m_flags ,OMX_COMPONENT_LOADING_PENDING)))
    {
        DEBUG_PRINT(" free buffer while Component in Loading pending\n");
    }
    else if((m_inp_bEnabled == OMX_FALSE && port == OMX_CORE_INPUT_PORT_INDEX)||
            (m_out_bEnabled == OMX_FALSE && port == OMX_CORE_OUTPUT_PORT_INDEX))
    {
        DEBUG_PRINT("Free Buffer while port %d disabled\n", port); 
    }
    else if(m_state == OMX_StateExecuting || m_state == OMX_StatePause)
    {
        DEBUG_PRINT("Invalid state to free buffer,ports need to be disabled\n");
        post_event(OMX_EventError, 
                   OMX_ErrorPortUnpopulated, 
                   OMX_COMPONENT_GENERATE_EVENT);

        return eRet;
    }
    else if(m_state != OMX_StateInvalid)
    {
        DEBUG_PRINT("Invalid state to free buffer,port lost Buffers\n");
        post_event(OMX_EventError, 
                   OMX_ErrorPortUnpopulated, 
                   OMX_COMPONENT_GENERATE_EVENT);
    }
 
    if(port == OMX_CORE_INPUT_PORT_INDEX)
    {
        // check if the buffer is valid 
        nPortIndex = buffer - (OMX_BUFFERHEADERTYPE*) m_inp_mem_ptr; 

        DEBUG_PRINT("free_buffer on i/p port - Port idx %d \n", nPortIndex); 
        if(nPortIndex < m_inp_buf_count)
        {
          // Clear the bit associated with it. 
          BITMASK_CLEAR(&m_inp_bm_count,nPortIndex); 
          m_inp_bPopulated = OMX_FALSE;
        }
        else
        {
            DEBUG_PRINT_ERROR("Error: free_buffer , \
                               Port Index calculation came out Invalid\n"); 
            eRet = OMX_ErrorBadPortIndex; 
        }
        
        if(BITMASK_PRESENT((&m_flags),OMX_COMPONENT_INPUT_DISABLE_PENDING) && release_input_done())
        {
            DEBUG_PRINT("MOVING TO DISABLED STATE \n");
            BITMASK_CLEAR((&m_flags),OMX_COMPONENT_INPUT_DISABLE_PENDING); 
            post_event(OMX_CommandPortDisable, 
                       OMX_CORE_INPUT_PORT_INDEX, 
                       OMX_COMPONENT_GENERATE_EVENT);
        }
    }
    else if(port == OMX_CORE_OUTPUT_PORT_INDEX)
    {
        // check if the buffer is valid 
        nPortIndex = buffer - (OMX_BUFFERHEADERTYPE*)m_out_mem_ptr; 
        if(nPortIndex < m_out_buf_count)
        {
            DEBUG_PRINT("free_buffer on o/p port - Port idx %d \n", nPortIndex); 
            // Clear the bit associated with it. 
            BITMASK_CLEAR(&m_out_bm_count,nPortIndex); 
            m_out_bPopulated = OMX_FALSE;
            if(omx_vdec_get_use_buf_flg() )
            {
                OMX_BUFFERHEADERTYPE * temp;
                // Remove both the mappings.
                temp = m_use_buf_hdrs.find(buffer);
                if(buffer && temp)
                {
                    m_use_buf_hdrs.erase(buffer);
                    m_use_buf_hdrs.erase(temp);
                }
            }
        }
        else
        {
            DEBUG_PRINT_ERROR("Error: free_buffer , \
                              Port Index calculation came out Invalid\n"); 
            eRet = OMX_ErrorBadPortIndex; 
        }
        if(BITMASK_PRESENT((&m_flags),OMX_COMPONENT_OUTPUT_DISABLE_PENDING) && release_output_done() )
        {
            DEBUG_PRINT("FreeBuffer : If any Disable event pending,post it\n"); 
            
                DEBUG_PRINT("MOVING TO DISABLED STATE \n");
                BITMASK_CLEAR((&m_flags),OMX_COMPONENT_OUTPUT_DISABLE_PENDING);
		if(true == m_event_port_settings_sent) {
			m_out_buf_count = m_out_buf_num_port_reconfig;
		}
		if(m_out_mem_ptr) {
			free(m_out_mem_ptr);
			m_out_mem_ptr = NULL;
		}
		if(m_platform_list) {
			free(m_platform_list);
			m_platform_list = NULL;
		}
		m_platform_entry = NULL;
		m_pmem_info = NULL;
                post_event(OMX_CommandPortDisable, 
                           OMX_CORE_OUTPUT_PORT_INDEX, 
                           OMX_COMPONENT_GENERATE_EVENT);

        }
        if(release_done() && omx_vdec_get_use_buf_flg() )
        {
                DEBUG_PRINT("Resetting use_buf flag\n");
                omx_vdec_reset_use_buf_flg();
        }
    }
    else
    {
        eRet = OMX_ErrorBadPortIndex; 
    }
    if((eRet == OMX_ErrorNone) && 
       (BITMASK_PRESENT(&m_flags ,OMX_COMPONENT_LOADING_PENDING)))
    {
        if(release_done())
        {
            // Send the callback now
            BITMASK_CLEAR((&m_flags),OMX_COMPONENT_LOADING_PENDING); 
            post_event(OMX_CommandStateSet, OMX_StateLoaded,
                                      OMX_COMPONENT_GENERATE_EVENT); 
        }
    }
    return eRet;
}

/* ======================================================================
FUNCTION
  omx_vdec::EmptyThisBuffer

DESCRIPTION
  This routine is used to push the encoded video frames to 
  the video decoder. 

PARAMETERS
  None.

RETURN VALUE
  OMX Error None if everything went successful. 

========================================================================== */
OMX_ERRORTYPE  omx_vdec::empty_this_buffer(OMX_IN OMX_HANDLETYPE         hComp, 
                                              OMX_IN OMX_BUFFERHEADERTYPE* buffer)
{
#ifdef PROFILE_DECODER
  ++empty_cnt;
  if(empty_cnt > 1) 
  {
    QPERF_END(empty_time);
  }
  QPERF_START(empty_time);
#endif
  unsigned height=0; 
  unsigned width=0;
  unsigned cropx = 0, cropy = 0, cropdx = 0, cropdy = 0;
  unsigned numOutFrames=m_out_buf_count; 
  OMX_ERRORTYPE ret = OMX_ErrorNone; 
  OMX_ERRORTYPE ret1 = OMX_ErrorNone;

  OMX_U8 *bufferCon = buffer->pBuffer;
  OMX_U32 bufferLen = buffer->nFilledLen;
  unsigned nBufferIndex = buffer-((OMX_BUFFERHEADERTYPE *)m_inp_mem_ptr);

  if(m_state == OMX_StateInvalid)
  {
      DEBUG_PRINT_ERROR("Empty this buffer in Invalid State\n");
      post_event((unsigned)&m_vdec_cfg, (unsigned)buffer,OMX_COMPONENT_GENERATE_BUFFER_DONE);
      post_event(OMX_EventError, OMX_ErrorInvalidState, OMX_COMPONENT_GENERATE_EVENT);
      return OMX_ErrorNone;
   }

  if(!m_vdec )
  {
      if(nBufferIndex > m_inp_buf_count)
      {
          DEBUG_PRINT_ERROR("omx_vdec::etb--> Buffer Index Invalid\n");
          post_event((unsigned)&m_vdec_cfg, (unsigned)buffer,OMX_COMPONENT_GENERATE_BUFFER_DONE);
          post_event(OMX_EventError, OMX_ErrorBadPortIndex, OMX_COMPONENT_GENERATE_EVENT);
          return OMX_ErrorNone;
      }
      if(m_event_port_settings_sent == true){
          BITMASK_SET((&m_flags), OMX_COMPONENT_FIRST_BUFFER_PENDING+nBufferIndex);
          return OMX_ErrorNone;
      }

      // scenario where SPS and PPS are sent as apart of set_config
      if(m_vendor_config.pData)
      {
          DEBUG_PRINT("Sending SPS+PPS as part of vdec_open\n");
          bufferCon = m_vendor_config.pData;
          bufferLen = m_vendor_config.nDataSize;
          m_vdec_cfg.sequenceHeader    = bufferCon;
          m_vdec_cfg.sequenceHeaderLen = bufferLen;
#if 0
printf("copying to offline bufs\n");
memcpy(off_buf,bufferCon,bufferLen);
memcpy(&off_buf[bufferLen],buffer->pBuffer,buffer->nFilledLen);
memcpy(buffer->pBuffer,off_buf,(bufferLen+buffer->nFilledLen));
      buffer->nFilledLen += bufferLen;
#endif // if required to pass INBAND
      }

      ret = omx_vdec_check_port_settings(bufferCon,
                                         bufferLen,
                                         height,width, cropx,cropy,cropdx,cropdy,numOutFrames);
      if ((strncmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.vc1", 26) == 0) &&
          (((*((OMX_U32 *)buffer->pBuffer)) & VC1_SP_MP_START_CODE_MASK) == VC1_SP_MP_START_CODE))
      {
        DEBUG_PRINT("VC1 Sequence Header\n");
        if (buffer->nFilledLen > VC1_SEQ_LAYER_SIZE) 
        {
          /* OMX client push sequence header and data. Need to push STRUCT_C and data*/
          memcpy(&buffer->pBuffer[buffer->nOffset+VC1_STRUCT_C_POS+VC1_STRUCT_C_LEN], 
                 &buffer->pBuffer[buffer->nOffset+VC1_SEQ_LAYER_SIZE],
                 buffer->nFilledLen - VC1_SEQ_LAYER_SIZE);

          buffer->nOffset += VC1_STRUCT_C_POS;
          buffer->nFilledLen = buffer->nFilledLen - VC1_SEQ_LAYER_SIZE + VC1_STRUCT_C_LEN;
        }
        else
        {
          buffer->nOffset += VC1_STRUCT_C_POS;
          buffer->nFilledLen = VC1_STRUCT_C_LEN;
        }
      }

      DEBUG_PRINT("ETB::after parsing-->Ht[%d] Wd[%d] m_ht[%d] m_wdth[%d]\n",\
                  height,width, m_height,m_width);
      DEBUG_PRINT("ETB::after parsing-->cropdy[%d] Cropdx[%d] \n",\
                  cropdy,cropdx);
       m_crop_x = cropx;
       m_crop_y = cropy;
       m_crop_dx = cropdx;
       m_crop_dy = cropdy;

      if(ret == OMX_ErrorNone && (ret1 = omx_vdec_validate_port_param(height,width,numOutFrames)) == OMX_ErrorNone)
      {
          // second if added to take care of a case where HxW is set to 0.
          // This will avoid Port Settings change proc
          if((((( height == m_height) && (width == m_width)) && (( height == cropdy) && (width == cropdx)) )  || 
             (( height == cropdy == OMX_CORE_QCIF_HEIGHT) && (width == cropdx == OMX_CORE_QCIF_WIDTH))) && numOutFrames == m_out_buf_count)
          {
              m_height = m_port_height = height;
              m_width  = m_port_width  = width;
              // if NALU is zero assume START CODE
              m_vdec_cfg.height          = m_port_height; 
              m_vdec_cfg.width           = m_port_width; 
              m_vdec_cfg.size_of_nal_length_field = m_nalu_bytes; 
              m_vdec_cfg.inputReq.bufferSize      = m_inp_buf_size;
              m_vdec_cfg.outputBuffer.numBuffers  = m_out_buf_count; 
              m_vdec = vdec_open(&m_vdec_cfg);
              DEBUG_PRINT("vdec_open[%p]\n",m_vdec);
              if(!m_vdec)
              {                
                    BITMASK_SET((&m_flags), OMX_COMPONENT_FIRST_BUFFER_PENDING+nBufferIndex);
                    post_event(OMX_EventError, OMX_ErrorInsufficientResources, OMX_COMPONENT_GENERATE_EVENT);
                    DEBUG_PRINT_ERROR("ERROR!!! vdec_open failed\n");
                    return OMX_ErrorNone;
              }
              m_vdec_cfg.sequenceHeader    = bufferCon;
              m_vdec_cfg.sequenceHeaderLen = bufferLen;
              // Populate Buffer Headers
              omx_vdec_get_out_buf_hdrs();
              // Populate Use Buffer Headers
              if(omx_vdec_get_use_buf_flg()){
                  omx_vdec_dup_use_buf_hdrs();
                  omx_vdec_get_out_use_buf_hdrs();
                  omx_vdec_display_out_buf_hdrs();
                  omx_vdec_add_entries();
                  omx_vdec_display_out_use_buf_hdrs();
              }
	      OMX_BUFFERHEADERTYPE* tmp_buf_hdr = (OMX_BUFFERHEADERTYPE*)flush_before_vdec_op_q->Dequeue();
	      while(tmp_buf_hdr) {
		      vdec_release_frame(m_vdec,
				      (vdec_frame *)tmp_buf_hdr->pOutputPortPrivate);
		      tmp_buf_hdr = (OMX_BUFFERHEADERTYPE*)flush_before_vdec_op_q->Dequeue();
	      }
#ifdef _ANDROID_
              OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *pPMEMInfo = m_pmem_info;
              // create IMemoryHeap object
              unsigned arena_sz = VDEC_NFRAMES * VDEC_INPUT_SIZE + m_out_buf_count * m_vdec_cfg.outputBuffer.bufferSize;
              m_heap_ptr =
              new VideoHeap(((vdec_frame*)(m_vdec_cfg.outputBuffer.base))->pmem_id,arena_sz , ((vdec_frame*)(m_vdec_cfg.outputBuffer.base))->base);
              for(unsigned i=0; i < m_out_buf_count ; i++)
              {
                pPMEMInfo->pmem_fd =  (OMX_U32) m_heap_ptr.get();
                pPMEMInfo++;
              }
              /*DEBUG_PRINT("VideoHeap : fd %d data %d size %d offset %d\n",\
                          ((vdec_frame*)(m_vdec_cfg.outputBuffer.base))->pmem_id,
                          ((vdec_frame*)(m_vdec_cfg.outputBuffer.base))->base,
                          arena_sz,
                          ((vdec_frame*)(m_vdec_cfg.outputBuffer.base))->pmem_offset);
              DEBUG_PRINT("m_heap_ptr =%d", (unsigned) m_heap_ptr.get());*/
#endif//_ANDROID_
          }
          else
          {
              // Store the Ht and Width param so that the client can do a GetParam
              m_event_port_settings_sent = true;
              m_height = m_port_height = height;
              m_width  = m_port_width  = width;
	      m_out_buf_num_port_reconfig = numOutFrames;
              // Notify Apps about the Event [ PortSettingsChangedEvent ]
              if(m_cb.EventHandler)
              {
                  m_cb.EventHandler(&m_cmp, m_app_data, 
                                      OMX_EventPortSettingsChanged,
                                      OMX_CORE_OUTPUT_PORT_INDEX, 0, NULL );
                  DEBUG_PRINT("Sending OMX EVENT PORT_SETTINGS_CHANGED EVENT \n");
              }
              if(!m_multiple_nals_per_buffer)
              {
                  if(m_first_pending_buf_idx <0 )
                  {
                      m_first_pending_buf_idx = nBufferIndex;
                  }
                  BITMASK_SET((&m_flags),\
                           OMX_COMPONENT_FIRST_BUFFER_PENDING+nBufferIndex);
                  return OMX_ErrorNone;
              }
          }
      } else if (ret == OMX_ErrorStreamCorrupt) {
	      DEBUG_PRINT_ERROR("OMX_ErrorStreamCorrupt\n");
	      post_event((unsigned)&m_vdec_cfg, (unsigned)buffer,OMX_COMPONENT_GENERATE_BUFFER_DONE);
	      post_event(OMX_EventError, OMX_ErrorInvalidState, OMX_COMPONENT_GENERATE_EVENT);
	      return OMX_ErrorNone;
      }
      else
      {
	  DEBUG_PRINT_ERROR("Unsupported clip\n");
	  post_event((unsigned)&m_vdec_cfg, (unsigned)buffer,OMX_COMPONENT_GENERATE_BUFFER_DONE);
          post_event(OMX_EventError, OMX_ErrorInvalidState, OMX_COMPONENT_GENERATE_EVENT);	
          return OMX_ErrorNone;
      }
  } 
  post_event((unsigned)hComp, (unsigned)buffer,OMX_COMPONENT_GENERATE_ETB); 
  return OMX_ErrorNone; 
}

/* ======================================================================
FUNCTION
  omx_vdec::EmptyThisBuffer

DESCRIPTION
  This routine is used to push the encoded video frames to 
  the video decoder. 

PARAMETERS
  None.

RETURN VALUE
  OMX Error None if everything went successful. 

========================================================================== */
OMX_ERRORTYPE  omx_vdec::empty_this_buffer_proxy(OMX_IN OMX_HANDLETYPE         hComp, 
                                                 OMX_IN OMX_BUFFERHEADERTYPE* buffer)
{
    int push_cnt = 0; 
    unsigned nPortIndex = buffer-((OMX_BUFFERHEADERTYPE *)m_inp_mem_ptr); 
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    ++m_etb_cnt;
    DEBUG_PRINT("ETB: Count %u TS %lld au_pending_cnt=%d\n", \
                      m_etb_cnt, buffer->nTimeStamp,m_in_pend_nals.size());

    if(nPortIndex < m_inp_buf_count)
    {
        if(m_first_pending_buf_idx  >= 0  && ((m_flags >> 3) & 0x3) && 
          (m_first_pending_buf_idx != (int)nPortIndex) && !m_multiple_nals_per_buffer)
        {
            // Buffer-2 will hold the existing buffer in hand
            // We are trying to append the data to buffer2
            OMX_BUFFERHEADERTYPE *buffer2 = input[m_first_pending_buf_idx];
            signed long long T1 = buffer->nTimeStamp; 
            signed long long T2 = buffer2->nTimeStamp;
            {
            BITMASK_SET((&m_flags),\
                         OMX_COMPONENT_FIRST_BUFFER_PENDING+nPortIndex);
            DEBUG_PRINT("Setting the pending flag for buffer-%d (%x) \n",\
                                             nPortIndex+1, (m_flags >> 3)); 
            push_cnt = push_pending_buffers_proxy(); 
            }
        }
        else if(!m_multiple_nals_per_buffer)
        {
            push_cnt += push_one_input_buffer(buffer); 
        }
        else
        {
            DEBUG_PRINT("To parse [%d] elements\n",buffer->nFilledLen);
            // parse and keep the AU ready to be sent
            ret = m_omx_utils->parseInputBitStream(buffer);
        }
    }
    else
    {
        DEBUG_PRINT_ERROR("FATAL ERROR: Why client is pushing the invalid buffer\n"); 
    }
    DEBUG_PRINT("empty_this_buffer_proxy pushed %d frames to the decoder\n",push_cnt); 
  return ret;
}


/* ======================================================================
FUNCTION
  omx_vdec::PushPendingBuffers

DESCRIPTION
  Internal method used to push the pending buffers. 

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
unsigned  omx_vdec::push_pending_buffers(void)
{
  post_event(0, 0,OMX_COMPONENT_PUSH_PENDING_BUFS);
  return 0; 
}
/* ======================================================================
FUNCTION
  omx_vdec::PushPendingBufers

DESCRIPTION
  This routine is used to push the  pending buffers if decoder 
  has space. 

PARAMETERS
  None.

RETURN VALUE
  Returns the push count. either 0, 1 or 2. 

========================================================================== */
unsigned  omx_vdec::push_pending_buffers_proxy(void)
{
  unsigned push_cnt = 0; 
  if( (m_first_pending_buf_idx >= 0) && (m_first_pending_buf_idx < (int)m_inp_buf_count) &&
      (((m_flags >> 3) & 0x3))
      )
  {
    // If both buffers are pending try to push the first one
    push_cnt  += push_one_input_buffer(input[m_first_pending_buf_idx]);
    if(push_cnt == 0)
    {
      // If we are not able to push then skip the next step 
      return push_cnt; 
    }
  }
  if( (m_first_pending_buf_idx >= 0) && (m_first_pending_buf_idx < (int)m_inp_buf_count) &&
      (input[m_first_pending_buf_idx]->nFlags & OMX_BUFFERFLAG_EOS) && ((m_flags >> 3) & 0x3))
  {
    // Either previous push success or only one pending buffer
    push_cnt  += push_one_input_buffer(input[m_first_pending_buf_idx]);
  }
  if(m_first_pending_buf_idx >=  (int)m_inp_buf_count)
  {
    DEBUG_PRINT_ERROR("FATAL Error: pending index invalid\n"); 
  }
  DEBUG_PRINT("push_pending_buffers pushed %d frames to the decoder\n",push_cnt); 
  return push_cnt;
}
/* ======================================================================
FUNCTION
  omx_vdec::PushOneInputBuffer

DESCRIPTION
  This routine is used to push the encoded video frames to 
  the video decoder. 

PARAMETERS
  None.

RETURN VALUE
  True if it is able to the push the buffer to the decoders. 

========================================================================== */
unsigned  omx_vdec::push_one_input_buffer(OMX_IN OMX_BUFFERHEADERTYPE* buffer)
{
  unsigned push_cnt = 0; 
  DEBUG_PRINT("push_one_input_buffer \n"); 
    
  unsigned nPortIndex = buffer-((OMX_BUFFERHEADERTYPE *)m_inp_mem_ptr); 
  if(nPortIndex < m_inp_buf_count)
  {
    memset(&m_frame_info,0,sizeof(m_frame_info)); 
    if(buffer->nFlags & OMX_BUFFERFLAG_EOS)
    {
      DEBUG_PRINT("empty_this_buffer: EOS received with TS %d\n",(int)buffer->nTimeStamp); 
      m_eos_timestamp = buffer->nTimeStamp; 
      m_frame_info.flags = FRAME_FLAG_EOS; 
    }
    PrintFrameHdr(buffer); 
    m_frame_info.data      = buffer->pBuffer + buffer->nOffset;
    m_frame_info.len       = buffer->nFilledLen;
    m_frame_info.timestamp = buffer->nTimeStamp;
    BITMASK_CLEAR((&m_flags),OMX_COMPONENT_FIRST_BUFFER_PENDING + nPortIndex);
    int nRet = vdec_post_input_buffer(m_vdec, &(m_frame_info), buffer); 
    DEBUG_PRINT("vdec_post_input_buffer returned %d\n",nRet); 
    if(VDEC_EOUTOFBUFFERS == nRet)
    {
      BITMASK_SET((&m_flags), OMX_COMPONENT_FIRST_BUFFER_PENDING+nPortIndex);
      DEBUG_PRINT("Setting the pending flag for buffer-%d (%x)\n",nPortIndex+1,
                                                                (m_flags >> 3)); 
      if(m_first_pending_buf_idx < 0)
      {
        m_first_pending_buf_idx = nPortIndex;
        DEBUG_PRINT("\n First pending buffer index is set to %d (%x)\n",
                                m_first_pending_buf_idx,(m_flags >> 3));
      }
    }
    else
    {
      ++m_ebd_cnt; 
      DEBUG_PRINT("\n ETB Count %u \n",m_ebd_cnt); 
      m_first_pending_buf_idx = -1; 
      // For the time being assume we have two buffers only. 
      for (unsigned i=0;i<m_inp_buf_count;i++)
      {
        if(BITMASK_PRESENT((&m_flags), (OMX_COMPONENT_FIRST_BUFFER_PENDING+i)))
        {
          m_first_pending_buf_idx = i;
          break; 
        }
      }
      DEBUG_PRINT("\n First pending buffer index is set to %d (%x)\n",
                              m_first_pending_buf_idx,(m_flags >> 3));
      push_cnt++; 

    }
  }
  return push_cnt;
}
/* ======================================================================
FUNCTION
  omx_vdec::FillThisBuffer

DESCRIPTION
  IL client uses this method to release the frame buffer 
  after displaying them. 

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_vdec::fill_this_buffer(OMX_IN OMX_HANDLETYPE         hComp, 
                                                  OMX_IN OMX_BUFFERHEADERTYPE* buffer)
{
  if(m_state == OMX_StateInvalid)
  {
      DEBUG_PRINT_ERROR("FTB in Invalid State\n");  
      return OMX_ErrorInvalidState;
  }

  if(OMX_FALSE == m_out_bEnabled) {
	  QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_ERROR, "FTB when port disabled\n");
	  return OMX_ErrorIncorrectStateOperation;
  }
  post_event((unsigned) hComp, (unsigned)buffer,OMX_COMPONENT_GENERATE_FTB);
  return OMX_ErrorNone; 
}
/* ======================================================================
FUNCTION
  omx_vdec::FillThisBuffer

DESCRIPTION
  IL client uses this method to release the frame buffer 
  after displaying them. 

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_vdec::fill_this_buffer_proxy(
                         OMX_IN OMX_HANDLETYPE        hComp, 
                         OMX_IN OMX_BUFFERHEADERTYPE* bufferAdd)
{
	OMX_ERRORTYPE nRet = OMX_ErrorNone;
	OMX_BUFFERHEADERTYPE *buffer = bufferAdd;
	pthread_mutex_lock(&m_ftb_lock);
	if(m_ftb_rxd_cnt < m_out_buf_count)
	{
		m_ftb_rxd_cnt++;
		if(m_ftb_rxd_cnt == m_out_buf_count)
			m_ftb_rxd_flg = true;
	}
	pthread_mutex_unlock(&m_ftb_lock);
	PrintFrameHdr(buffer); 
	unsigned push_cnt = 0;
	// pOutMem points to the start of the array 
	unsigned nPortIndex = buffer - ((OMX_BUFFERHEADERTYPE *) m_out_mem_ptr);

	if(BITMASK_PRESENT(&(m_out_flags),nPortIndex))
	{
		DEBUG_PRINT("FTB[%d] Ignored \n",nPortIndex);
		return OMX_ErrorNone;
	}

	if( (m_event_port_settings_sent == true) || !m_vdec || (m_out_bEnabled != OMX_TRUE) )
	{
		DEBUG_PRINT("OMX_VDEC::FTB --> Decoder Not initialised\n");
		// do we need to have !m_vdec check here??
		flush_before_vdec_op_q->Enqueue((void*)buffer);
		BITMASK_SET(&(m_out_flags),nPortIndex);
		--m_outstanding_frames;
		return OMX_ErrorNone;
	}  

	m_ftb_cnt++;

	if(omx_vdec_get_use_buf_flg() )
	{
		// Get the PMEM buf
		OMX_BUFFERHEADERTYPE* tempHdr;
		tempHdr = m_use_buf_hdrs.find(buffer);
		if(tempHdr)
		{
			DEBUG_PRINT("FTB::Found bufHdr[0x%x]0x%x-->0x%x\n",\
					buffer,buffer->pBuffer,
					tempHdr,tempHdr->pBuffer);
			// Map the pBuf add to pMEM address.
			buffer= tempHdr;
		}
		else{
			DEBUG_PRINT("FTB::No match found  bufHdr[0x%x] \n", buffer);
		}
	}
	if((nPortIndex < m_out_buf_count) )
	{
		if ( true == m_bEoSNotifyPending)
		{
			unsigned int i = 0;
			OMX_BUFFERHEADERTYPE *pBufHdr = 
				&(((OMX_BUFFERHEADERTYPE *)m_out_mem_ptr)[nPortIndex]); 
			DEBUG_PRINT("FTB: EOS notify pending - Generate EOS buffer[%d]\n",\
					nPortIndex);
			pBufHdr->nFlags     = OMX_BUFFERFLAG_EOS; 
			pBufHdr->nFilledLen = 0;
			pBufHdr->nTimeStamp = m_eos_timestamp; 
			DEBUG_PRINT("FBD Outstanding frame cnt %d\n", \
					m_outstanding_frames);
			m_bEoSNotifyPending = false;
			m_cb.FillBufferDone(&m_cmp, m_app_data, pBufHdr);
		}
		else
		{
			vdec_frame *frame = (vdec_frame *)buffer->pOutputPortPrivate;

			if(frame && frame->flags)
			{
				DEBUG_PRINT("FTB Reset frame flags\n");
				frame->flags = 0;
			}
			if(buffer && buffer->nFlags)
			{
				DEBUG_PRINT("FTB Reset buffer hdr flags\n");
				buffer->nFlags = 0;
			}
			if(frame && m_vdec)
			{
				BITMASK_SET(&(m_out_flags),nPortIndex);
				// Release frame should be called only while executing
				// We stash the h64 frame inside the OutputPortPrivate field
				vdec_release_frame(m_vdec,
						(vdec_frame *)buffer->pOutputPortPrivate); 

				--m_outstanding_frames;

				if(m_multiple_nals_per_buffer )
				{
					Vdec_ReturnType vdec_ret;
					video_input_frame_info *frame_info = NULL; 
					// use FTB to push any pending AU
					while(m_in_pend_nals.size()  && m_ftb_rxd_flg)
					{
						//frame_info = m_in_pend_nals.get_front_key();
						// push AU first from this list;
						frame_info = m_in_pend_nals.get_front_key();
						OMX_BUFFERHEADERTYPE* hdr = m_in_pend_nals.find(frame_info);
						vdec_ret = vdec_post_input_buffer(m_vdec,
								m_in_pend_nals.get_front_key(),
								m_in_pend_nals.find(frame_info));
						if(VDEC_EOUTOFBUFFERS == vdec_ret)
						{
							DEBUG_PRINT("Still no buffers available at VDEC\n");
							// Insert the current frame in the pending list
							break;
						}
						else
						{
							pthread_mutex_lock(&m_nal_bd_lock);
							++m_nal_bd_cnt;
							DEBUG_PRINT("\n NAL Sent nal_bd :Count %u \n",\
									m_nal_bd_cnt);
							pthread_mutex_unlock(&m_nal_bd_lock);
							m_in_pend_nals.erase(frame_info);
							// delete the frame info
							free(frame_info);
						}
					}
					if(!m_ftb_rxd_flg)
					{
						DEBUG_PRINT("FTBProxy::All FTB's from OMX IL still not rxed\n");
					}
				} 
			}
		}
		if (m_state == OMX_StateExecuting)
		{
			if(!m_multiple_nals_per_buffer)
			{
				push_cnt = push_pending_buffers_proxy(); 
			}
		}
		DEBUG_PRINT("FTB Pushed %d input frames at the same time\n",\
				push_cnt);

	}
	else
	{
		DEBUG_PRINT_ERROR("FATAL ERROR:Invalid Port Index[%d]\n",nPortIndex); 
	}

	return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
  omx_vdec::SetCallbacks

DESCRIPTION
  Set the callbacks. 

PARAMETERS
  None.

RETURN VALUE
  OMX Error None if everything successful. 

========================================================================== */
OMX_ERRORTYPE  omx_vdec::set_callbacks(OMX_IN OMX_HANDLETYPE        hComp, 
                                           OMX_IN OMX_CALLBACKTYPE* callbacks, 
                                           OMX_IN OMX_PTR             appData)
{
  m_cb       = *callbacks; 
  m_app_data =    appData;
  return OMX_ErrorNotImplemented;
}

/* ======================================================================
FUNCTION
  omx_vdec::ComponentDeInit

DESCRIPTION
  Destroys the component and release memory allocated to the heap. 

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if everything successful. 

========================================================================== */
OMX_ERRORTYPE  omx_vdec::component_deinit(OMX_IN OMX_HANDLETYPE hComp)
{
#ifdef PROFILE_DECODER
    usecs_t total_time_empty = QPERF_TERMINATE(empty_time);
    QTV_PERF_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_FATAL,"======================================================================\n");
    QTV_PERF_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_FATAL,"                   Open Max Statistics                                \n");
    QTV_PERF_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_FATAL,"======================================================================\n");
    QTV_PERF_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_FATAL,"empty this buffer rate = %f\n",(float)empty_cnt*1000000/total_time_empty);
    QTV_PERF_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_FATAL,"empty this buffer total time = %lld\n",total_time_empty);
    QTV_PERF_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_FATAL,"empty this buffer count = %d\n",empty_cnt);
    QTV_PERF_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_FATAL,"======================================================================\n");
#endif
    if ((OMX_StateLoaded != m_state) && (OMX_StateInvalid != m_state))
    {
        DEBUG_PRINT_ERROR("WARNING:Rxd DeInit,OMX not in LOADED/INVALID state %d\n",\
                          m_state);    
    }

    if(m_vdec)
    {
        vdec_close(m_vdec);
        m_vdec = NULL; 
    }
    if(m_vdec_cfg.adsp_fd >= 0) {
	close(m_vdec_cfg.adsp_fd);
    }
    if(m_out_mem_ptr)
    {
        DEBUG_PRINT("Freeing the Output Memory\n"); 
        free(m_out_mem_ptr); 
        m_out_mem_ptr = NULL; 
    }
    if(m_inp_mem_ptr)
    {
        DEBUG_PRINT("Freeing the Input Memory\n"); 
        free(m_inp_mem_ptr); 
        m_inp_mem_ptr = NULL; 
    }
    if (input)
    {
        DEBUG_PRINT("Freeing the Input array for buffers\n");
        free (input);
        input = NULL;
    }
    if(m_inp_buf_mem_ptr)
    {
        DEBUG_PRINT("Freeing the Input Memory\n"); 
        free(m_inp_buf_mem_ptr); 
        m_inp_buf_mem_ptr = NULL; 
    }
    if(m_omx_utils)
    {
        delete m_omx_utils;
        m_omx_utils = NULL;
    }
    if(m_platform_list)
    {
        free(m_platform_list);
        m_platform_list = NULL;
    }
    if(m_use_buf_hdrs.size())
    {
        DEBUG_PRINT_ERROR("WARNING::Num of ele still in the container=%d\n",\
                          m_use_buf_hdrs.size());
        m_use_buf_hdrs.show();
        m_use_buf_hdrs.eraseall();
      
    }
    if(m_loc_use_buf_hdr)
    {
        DEBUG_PRINT("Freeing the UseBuffer Header Memory\n"); 
        free(m_loc_use_buf_hdr);
        m_loc_use_buf_hdr = NULL;
    }
    m_ftb_rxd_cnt = 0;
    m_ftb_rxd_flg = false;
    if(m_vendor_config.pData)
    {
        free(m_vendor_config.pData);
        m_vendor_config.pData = NULL;
    }

#ifndef PC_DEBUG
    post_message(this, TERMINATE_MESSAGE_THREAD);
    pthread_join(msg_thread_id,NULL);
    if(m_pipe_in) close(m_pipe_in);
    if(m_pipe_out) close(m_pipe_out);
#endif

#ifndef PC_DEBUG
    sem_destroy(&m_cmd_lock);
#endif // PC_DEBUG
    pthread_mutex_destroy(&m_lock);
    pthread_mutex_destroy(&m_ftb_lock);
    pthread_mutex_destroy(&m_nal_bd_lock);

    DEBUG_PRINT("Unread mesg FTB-Q[%d] CMD-Q[%d] ETB-Q[%d] nal_bd[%d]\n",\
                              m_ftb_q.m_size,m_cmd_q.m_size,m_etb_q.m_size,
                              m_nal_bd_cnt);
    // Reset counters in mesg queues
    m_ftb_q.m_size=0;
    m_cmd_q.m_size=0;
    m_etb_q.m_size=0;
    m_ftb_q.m_read = m_ftb_q.m_write =0;
    m_cmd_q.m_read = m_cmd_q.m_write =0;
    m_etb_q.m_read = m_etb_q.m_write =0;

    DEBUG_PRINT("Pending NALs[%d]\n",m_in_pend_nals.size());
    struct video_input_frame_info *frame_info ;
    while(m_in_pend_nals.size())
    {
        frame_info = m_in_pend_nals.get_front_key();
        m_in_pend_nals.erase(frame_info);
        free(frame_info);
    }
#ifdef _ANDROID_
  // Clear the strong reference
    if(m_heap_ptr != NULL) {
	    m_heap_ptr.clear();
	    m_heap_ptr = NULL;
    }
#endif // _ANDROID_
  return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
  omx_vdec::UseEGLImage

DESCRIPTION
  OMX Use EGL Image method implementation <TBD>.

PARAMETERS
  <TBD>.

RETURN VALUE
  Not Implemented error. 

========================================================================== */
OMX_ERRORTYPE  omx_vdec::use_EGL_image(OMX_IN OMX_HANDLETYPE                hComp, 
                                          OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr, 
                                          OMX_IN OMX_U32                        port, 
                                          OMX_IN OMX_PTR                     appData, 
                                          OMX_IN void*                      eglImage)
{
    DEBUG_PRINT_ERROR("Error : use_EGL_image:  Not Implemented \n"); 
    return OMX_ErrorNotImplemented;
}

/* ======================================================================
FUNCTION
  omx_vdec::ComponentRoleEnum

DESCRIPTION
  OMX Component Role Enum method implementation. 

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if everything is successful. 
========================================================================== */
OMX_ERRORTYPE  omx_vdec::component_role_enum(OMX_IN OMX_HANDLETYPE hComp, 
                                                OMX_OUT OMX_U8*        role, 
                                                OMX_IN OMX_U32        index)
{
	OMX_ERRORTYPE eRet = OMX_ErrorNone;

	if(!strncmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.mpeg4",OMX_MAX_STRINGNAME_SIZE))
	{
		if((0 == index) && role)
		{
			strncpy((char *)role, "video_decoder.mpeg4",OMX_MAX_STRINGNAME_SIZE);
			DEBUG_PRINT("component_role_enum: role %s\n",role);
		}
		else
		{
			eRet = OMX_ErrorNoMore; 
		}
	}
	else if(!strncmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.h263",OMX_MAX_STRINGNAME_SIZE))
	{
		if((0 == index) && role)
		{
			strncpy((char *)role, "video_decoder.h263",OMX_MAX_STRINGNAME_SIZE);
			DEBUG_PRINT("component_role_enum: role %s\n",role);
		}
		else
		{
			DEBUG_PRINT("\n No more roles \n");
			eRet = OMX_ErrorNoMore; 
		}
	}
	else if(!strncmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.avc",OMX_MAX_STRINGNAME_SIZE))
	{
		if((0 == index) && role)
		{
			strncpy((char *)role, "video_decoder.avc",OMX_MAX_STRINGNAME_SIZE);
			DEBUG_PRINT("component_role_enum: role %s\n",role);
		}
		else
		{
			DEBUG_PRINT("\n No more roles \n");
			eRet = OMX_ErrorNoMore; 
		}
	}
	else if(!strncmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.vc1",OMX_MAX_STRINGNAME_SIZE))
	{
		if((0 == index) && role)
		{
			strncpy((char *)role, "video_decoder.vc1",OMX_MAX_STRINGNAME_SIZE);
			DEBUG_PRINT("component_role_enum: role %s\n",role);
		}
		else
		{
			DEBUG_PRINT("\n No more roles \n");
			eRet = OMX_ErrorNoMore; 
		}
	}
	else
	{
		DEBUG_PRINT_ERROR("\n Querying Role on Unknown Component\n");
		eRet = OMX_ErrorInvalidComponentName;
	}
	return eRet;
}




/* ======================================================================
FUNCTION
  omx_vdec::AllocateDone

DESCRIPTION
  Checks if entire buffer pool is allocated by IL Client or not. 
  Need this to move to IDLE state. 

PARAMETERS
  None.

RETURN VALUE
  true/false. 

========================================================================== */
bool omx_vdec::allocate_done(void)
{
  bool bRet = false;
  bool bRet_In = false;
  bool bRet_Out = false;
  
  bRet_In = allocate_input_done();
  bRet_Out = allocate_output_done();
 
  if(bRet_In && bRet_Out)
  {
      bRet = true;
  }
 
  return bRet; 
}
/* ======================================================================
FUNCTION
  omx_vdec::AllocateInputDone

DESCRIPTION
  Checks if I/P buffer pool is allocated by IL Client or not. 

PARAMETERS
  None.

RETURN VALUE
  true/false. 

========================================================================== */
bool omx_vdec::allocate_input_done(void)
{
  bool bRet = false; 
  unsigned i=0; 

  if (m_inp_mem_ptr == NULL)
  {
      return bRet; 
  }
  if(m_inp_mem_ptr )
  {
    for(;i<m_inp_buf_count;i++)
    {
      if(BITMASK_ABSENT(&m_inp_bm_count,i))
      {
        break; 
      }
    }
  }
  if(i==m_inp_buf_count)
  {
    bRet = true; 
  }
  if(i==m_inp_buf_count && m_inp_bEnabled)
  {
     m_inp_bPopulated = OMX_TRUE;
  }
  return bRet; 
}
/* ======================================================================
FUNCTION
  omx_vdec::AllocateOutputDone

DESCRIPTION
  Checks if entire O/P buffer pool is allocated by IL Client or not. 

PARAMETERS
  None.

RETURN VALUE
  true/false. 

========================================================================== */
bool omx_vdec::allocate_output_done(void)
{
  bool bRet = false; 
  unsigned j=0; 

  if (m_out_mem_ptr == NULL)
  {
      return bRet; 
  }

  if(m_out_mem_ptr )
  {
    for(;j<m_out_buf_count;j++)
    {
      if(BITMASK_ABSENT(&m_out_bm_count,j))
      {
        break; 
      }
    }
  }

  if(j==m_out_buf_count)
  {
    bRet = true; 
  }

  if(j==m_out_buf_count && m_out_bEnabled)
  {
     m_out_bPopulated = OMX_TRUE;
  }
  return bRet; 
}

/* ======================================================================
FUNCTION
  omx_vdec::ReleaseDone

DESCRIPTION
  Checks if IL client has released all the buffers. 

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
bool omx_vdec::release_done(void)
{
  bool bRet = false; 

  if(release_input_done())
    {
      if(release_output_done())
      {
          bRet = true;
      }
      }
  return bRet; 
}


/* ======================================================================
FUNCTION
  omx_vdec::ReleaseOutputDone

DESCRIPTION
  Checks if IL client has released all the buffers. 

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
bool omx_vdec::release_output_done(void)
{
  bool bRet = false; 
  unsigned i=0,j=0; 

  if(m_out_mem_ptr)
    {
      for(;j<m_out_buf_count;j++)
      {
        if(BITMASK_PRESENT(&m_out_bm_count,j))
        {
          break; 
        }
      }
    if(j==m_out_buf_count)
    {
      bRet = true; 
    }
  }
  else
  {
    DEBUG_PRINT_ERROR("Error: Invalid Inp/OutMem pointers \n"); 
      bRet = true;
  }
  return bRet; 
}
/* ======================================================================
FUNCTION
  omx_vdec::ReleaseInputDone

DESCRIPTION
  Checks if IL client has released all the buffers. 

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
bool omx_vdec::release_input_done(void)
{
  bool bRet = false; 
  unsigned i=0,j=0; 

  if(m_inp_mem_ptr)
  {
      for(;j<m_inp_buf_count;j++)
      {
        if( BITMASK_PRESENT(&m_inp_bm_count,j))
        {
          break; 
        }
      }
    if(j==m_inp_buf_count)
    {
      bRet = true; 
    }
  }
  else
  {
    DEBUG_PRINT_ERROR("Error: Invalid Inp/OutMem pointers \n"); 
    bRet = true;
  }
  return bRet; 
}
/* ======================================================================
FUNCTION
  omx_vdec::omx_vdec_get_out_buf_hdrs

DESCRIPTION
  Get the PMEM area from video decoder

PARAMETERS
  None.

RETURN VALUE
  None 
========================================================================== */
void omx_vdec::omx_vdec_get_out_buf_hdrs()
{
    OMX_BUFFERHEADERTYPE                *bufHdr;
    OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *pPMEMInfo = m_pmem_info;

    if(!m_out_mem_ptr)
	    return;
    bufHdr = (OMX_BUFFERHEADERTYPE *) m_out_mem_ptr;

    vdec_frame *base_frame  = (vdec_frame*)m_vdec_cfg.outputBuffer.base;
    if(base_frame){
        for(unsigned int i=0; i < m_out_buf_count ; i++)
        {
            bufHdr->nAllocLen = get_output_buffer_size();
            //bufHdr->nFilledLen= get_output_buffer_size();
            bufHdr->nFilledLen= 0;
            bufHdr->pBuffer = (OMX_U8*)(base_frame[i].base);
            bufHdr->pOutputPortPrivate =(void*) &base_frame[i];

            pPMEMInfo->offset = base_frame[i].pmem_offset;
#ifdef CBSP20
            /* Get the pmem_adsp file descriptor stored in the Decoder structure pVdec */
            pPMEMInfo->pmem_fd = base_frame[i].pmem_id;
            DEBUG_PRINT("pmem_adsp fd = %d\n", pPMEMInfo->pmem_fd); 
#endif 
            DEBUG_PRINT("Output Buffer param: Index [%d]: \
                    fd 0x%x output 0x%x base 0x%x off 0x%x\n",i,
                    (unsigned)pPMEMInfo->pmem_fd,
                    &base_frame[i],
                    (unsigned)base_frame[i].base,
                    (unsigned)pPMEMInfo->offset);
            DEBUG_PRINT("Output [%d]: buf %x \n",\
                    i,(unsigned)bufHdr->pBuffer);
            bufHdr++;
            pPMEMInfo++;
        }
    }
    return;
}


/* ======================================================================
FUNCTION
  omx_vdec::omx_vdec_get_out_use_buf_hdrsfer

DESCRIPTION
  Maintain a local copy of the output use buffers

PARAMETERS
  None.

RETURN VALUE
  None 
========================================================================== */
void omx_vdec::omx_vdec_get_out_use_buf_hdrs()
{
    OMX_BUFFERHEADERTYPE                *bufHdr;
    int nBufHdrSize        = 0;
    DEBUG_PRINT("GET OUTPUT USE BUF\n");

    bufHdr = (OMX_BUFFERHEADERTYPE *) m_loc_use_buf_hdr;

    vdec_frame *base_frame  = (vdec_frame*)m_vdec_cfg.outputBuffer.base;
    nBufHdrSize = m_out_buf_count*sizeof(OMX_BUFFERHEADERTYPE);
    if(base_frame){
        for(unsigned int i=0; i < m_out_buf_count ; i++)
        {
            bufHdr->nAllocLen = get_output_buffer_size();
            //bufHdr->nFilledLen= get_output_buffer_size(); 
            bufHdr->nFilledLen= 0; 

            bufHdr->pBuffer = (OMX_U8 *)(base_frame[i].base);
            bufHdr->pOutputPortPrivate =(void*) &base_frame[i];
            // just the offset instead of physical address
            DEBUG_PRINT("OutputBuffer[%d]: buf[0x%x]: pmem[0x%x] \n",\
                    i,(unsigned)bufHdr->pBuffer,
                    (OMX_U8*)(base_frame[i].base));
            bufHdr++;
        }
    }
}

/* ======================================================================
FUNCTION
  omx_vdec::omx_vdec_check_port_settings

DESCRIPTION
  Parse meta data to check the height and width param
  Check the level and profile

PARAMETERS
  None.

RETURN VALUE
  OMX_ErrorNone, if profile and level are supported
  OMX_ErrorUnsupportedSetting, if profile and level are not supported
========================================================================== */
OMX_ERRORTYPE omx_vdec::omx_vdec_check_port_settings(OMX_U8* buffer,
                        OMX_U32 filledLen,
                        unsigned &height, 
                        unsigned &width,
                        unsigned &cropx,
                        unsigned &cropy,
                        unsigned &cropdx,
                        unsigned &cropdy,
			unsigned &numOutFrames)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 nal_size = 0;
    if(!strcmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.avc"))
    {
        
        OMX_U8 *encodedBytes =  buffer;
        if(m_vendor_config.pData)
        {
            // if SPS+PPS recieved out of band[ as OMX_SetConfig]
            // hard code nal length for detecting HxW to 2 always.
            // irrespective of AVCC atom 'lenghtSizeMinusOne'
            // Also,dont append SPS+PPS with the first ETB contents
            // just send SPS+PPS to H264 parser            
            nal_size= 2;
        }
        else
        {
            nal_size = m_nalu_bytes;
        } 
        DEBUG_PRINT("Before Parsing height[%d] width[%d] NALLen=%d\n",\
                                           height,width,m_nalu_bytes);
	if (false == m_omx_utils->validateMetaData(encodedBytes,filledLen,nal_size,
                                                   height, width, cropx, cropy, cropdx, cropdy, numOutFrames))
        {
          DEBUG_PRINT_ERROR("Unsupported profile, level, or widht, height\n");
          ret = OMX_ErrorUnsupportedSetting;
        }
        DEBUG_PRINT("Parsing Done height[%d] width[%d] NALLen=%d\n",\
                                           height,width,nal_size);
    }
    else if((!strcmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.mpeg4"))
            || (!strcmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.h263"))) 
    {
      signed short pError;
      MP4_Utils mp4_parser(&pError);
      ret = mp4_parser.validateMetaData(buffer, filledLen, height, width,
                                               cropx, cropy, cropdx, cropdy);
      if((height % 16) != 0)
      {
          DEBUG_PRINT("\n Height %d is not a multiple of 16", height);
          height = (height / 16 + 1) *  16;
          DEBUG_PRINT("\n Height adjusted to %d \n", height);
      }
      if ((width % 16) != 0)
      {
          DEBUG_PRINT("\n Width %d is not a multiple of 16", width);
          width = (width / 16 + 1) *  16;
          DEBUG_PRINT("\n Width adjusted to %d \n", width);
      }
      DEBUG_PRINT("MPEG4/H263 ht[%d] wdth[%d]\n", height, width);
    }
    else if(!strcmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.vc1"))
    {
      if (((*((OMX_U32 *)buffer)) & VC1_SP_MP_START_CODE_MASK) == VC1_SP_MP_START_CODE)
      {
        OMX_U32 level = 0xF;
        OMX_U32 profile = 0xF;
        OMX_U8 *p_struct_c = NULL;
        OMX_U8 *p_struct_b = NULL;
        QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_LOW, "Entire VC1 SP/MP Sequence Layer data\n");

        /* STRUCT_C will be followed by STRUCT_A (8 bytes), 0x0000000C, STRUCT_B (12 bytes)
         * Level is part of STRUCT_B
         */
        p_struct_c = &buffer[VC1_STRUCT_C_POS];
        p_struct_b = &buffer[VC1_STRUCT_B_POS]; 

        if (p_struct_c) 
        {
          profile = (*((OMX_U32 *)p_struct_c) & VC1_STRUCT_C_PROFILE_MASK) >> 4;
        }
        
        if (p_struct_b) 
        {
          level = (*((OMX_U32 *)p_struct_b) & VC1_STRUCT_B_LEVEL_MASK) >> 29;
        }

        // height and width is part of STRUCT_A
        height = *((OMX_U32 *)&(buffer[VC1_STRUCT_A_POS]));
        width  = *((OMX_U32 *)&(buffer[VC1_STRUCT_A_POS+4]));
        DEBUG_PRINT("omx_vdec_check_port_settings - VC1 Simple/Main profile, %d x %d %x x %x, profile %d, level %d\n",
                      width, height, width, height, profile, level);
        if ((profile != VC1_SIMPLE_PROFILE) ||
            ((level != VC1_SIMPLE_PROFILE_LOW_LEVEL) && (level != VC1_SIMPLE_PROFILE_MED_LEVEL)))
        {
          DEBUG_PRINT_ERROR("Error - Unsupported VC1 profile %d, level %d\n", profile, level);
          ret = OMX_ErrorUnsupportedSetting;
        }
      }
      else if (*((OMX_U32 *)buffer) == 0x0F010000)
      {
        OMX_U16 temp_dimension = ((OMX_U16)(buffer[6]) << 4) | ((OMX_U16)(buffer[7]) >> 4);
        width = 2*(temp_dimension+1);

        temp_dimension = ((OMX_U16)(buffer[7] & 0x0F) << 8) | (OMX_U16)(buffer[8]);
        height = 2*(temp_dimension+1);

        DEBUG_PRINT_ERROR("omx_vdec_check_port_settings - Error: VC1 Advance profile unssuported, %d x %d\n",
                          width, height);
        ret = OMX_ErrorUnsupportedSetting;
      }
      else if (((*((OMX_U32 *)buffer) & VC1_STRUCT_C_PROFILE_MASK ) >> 30) == VC1_SIMPLE_PROFILE) 
      {
        DEBUG_PRINT("omx_vdec_check_port_settings - STRUCT_C VC1 Simple profile doesn't have widht and height info\n");
        DEBUG_PRINT("Couldn't validate profile and level");
        height = m_height;
        width = m_width;
      }
      else
      {
        DEBUG_PRINT_ERROR("omx_vdec_check_port_settings - ERROR: Unknown VC1 profile. Couldn't find height and width\n");
        ret = OMX_ErrorUnsupportedSetting;
      }
      cropdy = height;
      cropdx = width;
      cropx = cropy = 0;

      if((height % 16) != 0)
      {
          DEBUG_PRINT("\n Height %d is not a multiple of 16", height);
          height = (height / 16 + 1) *  16;
          DEBUG_PRINT("\n Height adjusted to %d \n", height);
      }
      if ((width % 16) != 0)
      {
          DEBUG_PRINT("\n Width %d is not a multiple of 16", width);
          width = (width / 16 + 1) *  16;
          DEBUG_PRINT("\n Width adjusted to %d \n", width);
      }
    }

    return ret;
}


/* ======================================================================
FUNCTION
  omx_vdec::omx_vdec_validate_port_param

DESCRIPTION
  Get the PMEM area from video decoder

PARAMETERS
  None.

RETURN VALUE
  None 
========================================================================== */
OMX_ERRORTYPE omx_vdec::omx_vdec_validate_port_param(int height, int width,unsigned numOutFrames)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    long hxw = height*width;
    long lmt_hxw = 0;

    /* There is a limitataion at VCodec2 where 32 is the smallest frame size 
     * H264, H263, and MP4 use VCodec2. Only VC1 doesn't have this limitation.
     */
    int min_width = 32;
    int min_height = 32;
    int notSupported = 0;
#if defined(_ANDROID_) && !defined(FEATURE_QTV_WVGA_ENABLE)
    if(!strcmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.avc"))
    {
       lmt_hxw =  H264_SUPPORTED_WIDTH * H264_SUPPORTED_HEIGHT;
       if((numOutFrames > OMX_VIDEO_DEC_NUM_OUTPUT_BUFFERS + 2) && (hxw >= OMX_CORE_VGA_HEIGHT * OMX_CORE_VGA_WIDTH)) {
	       notSupported = 1;
       }
    }
    else if((!strcmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.mpeg4"))
            || (!strcmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.h263")))    

    {
       lmt_hxw =  MPEG4_SUPPORTED_WIDTH * MPEG4_SUPPORTED_HEIGHT;
    }
    else if (!strcmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.vc1")) 
    {
      lmt_hxw =  OMX_CORE_CIF_HEIGHT*OMX_CORE_CIF_WIDTH;
      min_width = min_height = 0;
    }
#else
    if ((!strcmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.avc")) ||
        (!strcmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.mpeg4")) ||
        (!strcmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.h263")))
    {
      lmt_hxw =  OMX_CORE_WVGA_HEIGHT*OMX_CORE_WVGA_WIDTH;
    }
    else if (!strcmp(m_vdec_cfg.kind, "OMX.qcom.video.decoder.vc1")) 
    {
      lmt_hxw =  OMX_CORE_CIF_HEIGHT*OMX_CORE_CIF_WIDTH;
      min_width = min_height = 0;
    }
    if(height > OMX_CORE_WVGA_WIDTH || width > OMX_CORE_WVGA_WIDTH) {
	    notSupported = 1;
    }
#endif

    if ((hxw > lmt_hxw) || (width < min_width) || (height < min_height) || notSupported)
    {
      ret = OMX_ErrorNotImplemented;
      DEBUG_PRINT("Invalid Ht[%d] wdth[%d]\n",height,width);
    }
    return ret;
}


/* ======================================================================
FUNCTION
  omx_vdec::omx_vdec_add_entries

DESCRIPTION
  Add the buf header entries to the container

PARAMETERS
  None.

RETURN VALUE
  None 
========================================================================== */
void omx_vdec::omx_vdec_add_entries()
{
    OMX_BUFFERHEADERTYPE* pOmxHdr = 
                                  (OMX_BUFFERHEADERTYPE*)m_out_mem_ptr;
    OMX_BUFFERHEADERTYPE* pOmxOutHdr = m_loc_use_buf_hdr;
    for(unsigned int i=0;i<8;i++,pOmxHdr++,pOmxOutHdr++)
    {
        m_use_buf_hdrs.insert(pOmxOutHdr, pOmxHdr);
        m_use_buf_hdrs.insert(pOmxHdr, pOmxOutHdr);
    } 

}

/* ======================================================================
FUNCTION
  omx_vdec::omx_vdec_dup_use_buf_hdrs

DESCRIPTION
  Populate the copy buffer [ use buffer ]

PARAMETERS
  None.

RETURN VALUE
  None 
========================================================================== */
OMX_ERRORTYPE omx_vdec::omx_vdec_dup_use_buf_hdrs( )
{
    OMX_ERRORTYPE        eRet = OMX_ErrorNone; 
    OMX_BUFFERHEADERTYPE *bufHdr; // buffer header
    OMX_BUFFERHEADERTYPE *pHdr = (OMX_BUFFERHEADERTYPE*)m_out_mem_ptr;
    int                  nBufHdrSize            = 0;

    nBufHdrSize        = m_out_buf_count*sizeof(OMX_BUFFERHEADERTYPE);
    memcpy(m_loc_use_buf_hdr,pHdr,nBufHdrSize);
    omx_vdec_display_out_use_buf_hdrs();
    return eRet;
}

void omx_vdec::omx_vdec_cpy_user_buf(OMX_BUFFERHEADERTYPE* pBufHdr)
{
    OMX_BUFFERHEADERTYPE *bufHdr;
    bufHdr = m_use_buf_hdrs.find(pBufHdr);
    if(bufHdr)
    {
        DEBUG_PRINT("CPY::Found bufHdr[0x%x]0x%x-->[0x%x]0x%x\n",\
                     pBufHdr->pBuffer,pBufHdr, bufHdr,bufHdr->pBuffer);
        // Map the local buff address to the user space address.
        // Basically pMEM -->pBuf translation

        // DO A MEMCPY from pMEM are to the user defined add
        DEBUG_PRINT("CPY::[bufferHdr]pBuffer maps[0x%x]0x%x-->[0x%x]0x%x\n",\
                     pBufHdr->pBuffer,pBufHdr, bufHdr,bufHdr->pBuffer);
       // first buffer points to user defined add, sec one to PMEM area
       memcpy(pBufHdr->pBuffer,bufHdr->pBuffer,get_output_buffer_size());
    }
    else{
        DEBUG_PRINT("CPY::No match found  bufHdr[0x%x] \n", pBufHdr);
        omx_vdec_display_out_use_buf_hdrs();
    }
}

void omx_vdec::send_nal(OMX_BUFFERHEADERTYPE* bufHdr,
                       OMX_U8* pBuffer,unsigned int dataLen)
{
    struct video_input_frame_info *frame_info ;
    int nRet;

    DEBUG_PRINT("Send AU To Dec,pending AU[%d]\n",m_in_pend_nals.size());
    DEBUG_PRINT("bufHdr[0x%x] buffer[0x%x] len[%d]\n",\
                                                bufHdr,pBuffer,dataLen);
    unsigned nPortIndex = bufHdr-((OMX_BUFFERHEADERTYPE *)m_inp_mem_ptr); 

    if(!m_vdec)
    {
        //store the parsed info
        frame_info = (struct video_input_frame_info *)
                             malloc(sizeof(struct video_input_frame_info));
        frame_info->data      = pBuffer;
        frame_info->len       = dataLen;
        frame_info->timestamp = bufHdr->nTimeStamp;
        m_in_pend_nals.insert(frame_info,bufHdr);
        DEBUG_PRINT("VDEC not yet Init, store the parsed info cnt[%d]\n",\
                                                     m_in_pend_nals.size());
        return;
    }
    if(nPortIndex < m_inp_buf_count)
    {
        if(m_in_pend_nals.size() && m_ftb_rxd_flg)
        {
            DEBUG_PRINT("send_nal::current pending list=%d\n",\
                                                    m_in_pend_nals.size());
            // insert the current frame at the end of the list
            frame_info = (struct video_input_frame_info *)
                     malloc(sizeof(struct video_input_frame_info));
            frame_info->data      = pBuffer;
            frame_info->len       = dataLen;
            frame_info->timestamp = bufHdr->nTimeStamp;
            if(bufHdr->nFlags & OMX_BUFFERFLAG_EOS)
            {
                DEBUG_PRINT("send_nal: EOS received with TS %d\n",\
                                                 (int)bufHdr->nTimeStamp); 
                m_eos_timestamp = bufHdr->nTimeStamp; 
                frame_info->flags = FRAME_FLAG_EOS; 
            }
            m_in_pend_nals.insert(frame_info, bufHdr);

            frame_info = m_in_pend_nals.get_front_key();

            // push NALS in the rxed order;
            nRet = vdec_post_input_buffer(m_vdec, 
                                       frame_info,
                                       m_in_pend_nals.find(frame_info));
            if(VDEC_EOUTOFBUFFERS == nRet)
            {
                DEBUG_PRINT("Still no buffers available at VDEC\n");
            }
            else
            {
                pthread_mutex_lock(&m_nal_bd_lock);
                ++m_nal_bd_cnt;
                DEBUG_PRINT("\nSent nal_bd :Count %u \n",m_nal_bd_cnt); 
                pthread_mutex_unlock(&m_nal_bd_lock);
                m_in_pend_nals.erase(frame_info);
                // delete the frame info 
                free(frame_info);
            }
            return;
        }
        frame_info = (struct video_input_frame_info *)
                     malloc(sizeof(struct video_input_frame_info));
        
        if(bufHdr->nFlags & OMX_BUFFERFLAG_EOS)
        {
            DEBUG_PRINT("SEND AU: EOS received with TS %d\n",\
                                                 (int)bufHdr->nTimeStamp); 
            m_eos_timestamp = bufHdr->nTimeStamp; 
            frame_info->flags = FRAME_FLAG_EOS; 
        }
        //PrintFrameHdr(bufHdr); 
        frame_info->data      = pBuffer;
        frame_info->len       = dataLen;
        frame_info->timestamp = bufHdr->nTimeStamp;
        if(m_ftb_rxd_flg)
        { 
            nRet = vdec_post_input_buffer(m_vdec, frame_info, bufHdr); 
            DEBUG_PRINT("vdec_post_input_buffer returned %d\n",nRet); 
            if(VDEC_EOUTOFBUFFERS == nRet)
            {
                // Push the AU to pending 
                m_in_pend_nals.insert(frame_info, bufHdr);
                m_in_pend_nals.show();
                DEBUG_PRINT("\nPending AU index is set to %d \n",
                                m_in_pend_nals.size());
            }
            else
            {
                pthread_mutex_lock(&m_nal_bd_lock);
                ++m_nal_bd_cnt;
                DEBUG_PRINT("\n Sent nal_bd :Count %u \n",m_nal_bd_cnt); 
                pthread_mutex_unlock(&m_nal_bd_lock);
                // delete the frame info 
                free(frame_info);
            }
        }
        else
        {
            DEBUG_PRINT("Still no FTB from Client\n");
            // Push the AU to pending
            m_in_pend_nals.insert(frame_info, bufHdr);
            m_in_pend_nals.show();
        }
    }
    return ;
}


void omx_vdec::omx_vdec_inform_ebd(OMX_BUFFERHEADERTYPE* pBufHdr)
{
    DEBUG_PRINT("EBD callback cnt[%d]\n",m_ebd_cnt+1);
    post_event((unsigned)&m_vdec_cfg, (unsigned)pBufHdr,OMX_COMPONENT_GENERATE_BUFFER_DONE);
    return;
}
/*
OMX_ERRORTYPE omx_vdec::empty_buffer_done(OMX_HANDLETYPE         hComp,
                                 OMX_BUFFERHEADERTYPE* buffer)
{
    m_ebd_cnt++;
    DEBUG_PRINT("Informing Client abt EBD cnt[%d]\n",m_ebd_cnt);
    unsigned int nPortIndex = 
             buffer-(OMX_BUFFERHEADERTYPE *)m_inp_mem_ptr;
    if(m_cb.EmptyBufferDone && (nPortIndex < OMX_VIDEO_DEC_NUM_INPUT_BUFFERS))
    {
        m_cb.EmptyBufferDone(hComp ,m_app_data, buffer);
    }
    return OMX_ErrorNone;
}
*/
void omx_vdec::omx_vdec_display_in_buf_hdrs()
{
    OMX_BUFFERHEADERTYPE* omxhdr = ((OMX_BUFFERHEADERTYPE *)m_inp_mem_ptr) ;
    DEBUG_PRINT("^^^^^INPUT BUF HDRS^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
    for(unsigned int i=0;i<2;i++)
    {
        DEBUG_PRINT("hdr[0x%x] buffer[0x%x]\n",omxhdr+i,(omxhdr+i)->pBuffer);
    }
    DEBUG_PRINT("^^^^^^^INPUT BUF HDRS^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
}

void omx_vdec::omx_vdec_display_out_buf_hdrs()
{
    OMX_BUFFERHEADERTYPE* omxhdr = ((OMX_BUFFERHEADERTYPE *)m_out_mem_ptr) ;
    DEBUG_PRINT("^^^^^^^^OUTPUT BUF HDRS^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
    for(unsigned int i=0;i<8;i++)
    {
        DEBUG_PRINT("hdr[0x%x] buffer[0x%x]\n",omxhdr+i,(omxhdr+i)->pBuffer);
        DEBUG_PRINT("timestamp[0x%x] nFilledLen[0x%x]\n",(omxhdr+i)->nTimeStamp,(omxhdr+i)->nFilledLen);
    }
    DEBUG_PRINT("^^^^^^^^^OUTPUT BUF HDRS^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");

}

void omx_vdec::omx_vdec_display_out_use_buf_hdrs()
{
    OMX_BUFFERHEADERTYPE* omxhdr = m_loc_use_buf_hdr;
    DEBUG_PRINT("^^^^^^^^^USE OUTPUT BUF HDRS^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
    for(unsigned int i=0;i<8;i++)
    {
        DEBUG_PRINT("hdr[0x%x] buffer[0x%x]\n",omxhdr+i,(omxhdr+i)->pBuffer);
        DEBUG_PRINT("timestamp[0x%x] nFilledLen[0x%x]\n",(omxhdr+i)->nTimeStamp,(omxhdr+i)->nFilledLen);
    }
    DEBUG_PRINT("^^^^^^^^^^^USE OUTPUT BUF HDRS^^^^^^^^^^^^^^^^^^^^^^^^^\n");
    DEBUG_PRINT("^^^^^^^^^^USE BUF HDRS MAPPING^^^^^^^^^^^^^^^^^^^^^^^^^\n");
    m_use_buf_hdrs.show();
    DEBUG_PRINT("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
}

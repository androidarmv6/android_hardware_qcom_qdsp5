
#ifndef __OMX_VDEC_H__
#define __OMX_VDEC_H__
/*============================================================================
                            O p e n M A X   Component
                                Video Decoder 

*//** @file comx_vdec.h
  This module contains the class definition for openMAX decoder component.

Copyright (c) 2006-2008 QUALCOMM Incorporated.
All Rights Reserved. Qualcomm Proprietary and Confidential.
*//*========================================================================*/

/*============================================================================
                              Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/OmxVdec/rel/1.2/omx_vdec.h#6 $
when       who     what, where, why
--------   ---     -------------------------------------------------------
05/20/08   ssk     Created file.

============================================================================*/

//////////////////////////////////////////////////////////////////////////////
//                             Include Files
//////////////////////////////////////////////////////////////////////////////

#include<stdlib.h>

#include <stdio.h>
#ifdef _ANDROID_
    #include <binder/MemoryHeapBase.h> 
#endif // _ANDROID_
#include <pthread.h>
#ifndef PC_DEBUG
#include <semaphore.h>
#endif
#include "OMX_Core.h"
#include "OMX_QCOMExtns.h"
#include "vdec.h"
#include "qc_omx_component.h"
#include "Map.h"
#include "OmxUtils.h"
#include "qtv_msg.h"

extern "C" {
  OMX_API void * get_omx_component_factory_fn(void);
}


#ifdef _ANDROID_
    using namespace android;
    // local pmem heap object
    class VideoHeap : public MemoryHeapBase
    {
    public:
        VideoHeap(int fd, size_t size, void* base);
        virtual ~VideoHeap() {}
    };
#endif // _ANDROID_
//////////////////////////////////////////////////////////////////////////////
//                       Module specific globals
//////////////////////////////////////////////////////////////////////////////
#define OMX_SPEC_VERSION  0x00000101


//////////////////////////////////////////////////////////////////////////////
//               Macros 
//////////////////////////////////////////////////////////////////////////////
#define PrintFrameHdr(bufHdr) DEBUG_PRINT("bufHdr %x buf %x size %d TS %d\n",\
                       (unsigned) bufHdr,\
                       (unsigned)((OMX_BUFFERHEADERTYPE *)bufHdr)->pBuffer,\
                       (unsigned)((OMX_BUFFERHEADERTYPE *)bufHdr)->nFilledLen,\
                       (unsigned)((OMX_BUFFERHEADERTYPE *)bufHdr)->nTimeStamp)

// BitMask Management logic
#define BITS_PER_BYTE        0x20
#define BITMASK_SIZE(mIndex) (((mIndex) + BITS_PER_BYTE - 1)/BITS_PER_BYTE)
#define BITMASK_OFFSET(mIndex) ((mIndex)/BITS_PER_BYTE)
#define BITMASK_FLAG(mIndex) (1 << ((mIndex) % BITS_PER_BYTE))
#define BITMASK_CLEAR(mArray,mIndex) (mArray)[BITMASK_OFFSET(mIndex)] &=  ~(BITMASK_FLAG(mIndex))  
#define BITMASK_SET(mArray,mIndex)  (mArray)[BITMASK_OFFSET(mIndex)] |=  BITMASK_FLAG(mIndex)  
#define BITMASK_PRESENT(mArray,mIndex) ((mArray)[BITMASK_OFFSET(mIndex)] & BITMASK_FLAG(mIndex))
#define BITMASK_ABSENT(mArray,mIndex) (((mArray)[BITMASK_OFFSET(mIndex)] & BITMASK_FLAG(mIndex)) == 0x0)
#define BITMASK_PRESENT(mArray,mIndex) ((mArray)[BITMASK_OFFSET(mIndex)] & BITMASK_FLAG(mIndex))
#define BITMASK_ABSENT(mArray,mIndex) (((mArray)[BITMASK_OFFSET(mIndex)] & BITMASK_FLAG(mIndex)) == 0x0)

#define OMX_VIDEO_DEC_NUM_INPUT_BUFFERS   2
#define OMX_VIDEO_DEC_NUM_OUTPUT_BUFFERS  VDEC_NFRAMES
#ifdef FEATURE_QTV_WVGA_ENABLE
#define OMX_VIDEO_DEC_INPUT_BUFFER_SIZE   (256*1024)
#else
#define OMX_VIDEO_DEC_INPUT_BUFFER_SIZE   (128*1024)
#endif
#define OMX_CORE_CONTROL_CMDQ_SIZE   100
#define OMX_CORE_QCIF_HEIGHT         144
#define OMX_CORE_QCIF_WIDTH          176
#define OMX_CORE_CIF_HEIGHT          288
#define OMX_CORE_CIF_WIDTH           352
#define OMX_CORE_VGA_HEIGHT          480
#define OMX_CORE_VGA_WIDTH           640
#define OMX_CORE_WVGA_HEIGHT         480
#define OMX_CORE_WVGA_WIDTH          800

#define TERMINATE_MESSAGE_THREAD 200

class genericQueue
{
	private:
		struct node
		{	
			void* data;
			node* next;
		};
		node* head;
		node* tail;
		int numElements;
		//pthread_mutex_t queue_protector;
	public:

		genericQueue();
		
		int Enqueue(void* data);
		void* Dequeue();
		int GetSize();
		void* checkHead();
		void* checkTail();
		
		~genericQueue();		
};


class OmxUtils;

// OMX video decoder class 
class omx_vdec: public qc_omx_component
{
public: 
  int 			     m_pipe_in; // for communicating with the message thread    
  int 			     m_pipe_out; // for communicating with the message thread
  pthread_t                  msg_thread_id;
  // video decoder input structure
  struct vdec_context       m_vdec_cfg;   // video decoder input structure
  
  omx_vdec();  // constructor
  virtual ~omx_vdec();  // destructor
  static omx_vdec* get_instance();
  
    OMX_ERRORTYPE allocate_buffer(
                     OMX_HANDLETYPE       hComp,
                     OMX_BUFFERHEADERTYPE **bufferHdr,
                     OMX_U32              port,
                     OMX_PTR              appData,
                     OMX_U32              bytes);
                 
  
    OMX_ERRORTYPE component_deinit(OMX_HANDLETYPE hComp);
  
    OMX_ERRORTYPE component_init(OMX_STRING role);
  
    OMX_ERRORTYPE component_role_enum(OMX_HANDLETYPE hComp,
                                      OMX_U8         *role,
                                      OMX_U32        index);
                                  
    OMX_ERRORTYPE component_tunnel_request(OMX_HANDLETYPE      hComp,
                                           OMX_U32             port,
                                           OMX_HANDLETYPE      peerComponent,
                                           OMX_U32             peerPort,
                                           OMX_TUNNELSETUPTYPE *tunnelSetup);
                                       
    OMX_ERRORTYPE empty_this_buffer(OMX_HANDLETYPE       hComp,
                                    OMX_BUFFERHEADERTYPE *buffer);

                                
  
    OMX_ERRORTYPE fill_this_buffer(OMX_HANDLETYPE       hComp,
                                   OMX_BUFFERHEADERTYPE *buffer);
                               

    OMX_ERRORTYPE free_buffer(OMX_HANDLETYPE         hComp,
                              OMX_U32                 port,
                              OMX_BUFFERHEADERTYPE *buffer);
                           
    OMX_ERRORTYPE get_component_version(OMX_HANDLETYPE hComp,
                                        OMX_STRING       componentName,
                                        OMX_VERSIONTYPE  *componentVersion,
                                        OMX_VERSIONTYPE  *specVersion,
                                        OMX_UUIDTYPE     *componentUUID);
                                    
    OMX_ERRORTYPE get_config(OMX_HANDLETYPE hComp,
                             OMX_INDEXTYPE  configIndex,
                             OMX_PTR        configData);
                          
    OMX_ERRORTYPE get_extension_index(OMX_HANDLETYPE hComp,
                                      OMX_STRING     paramName,
                                      OMX_INDEXTYPE  *indexType);
                                  
    OMX_ERRORTYPE get_parameter(OMX_HANDLETYPE hComp,
                                OMX_INDEXTYPE  paramIndex,
                                OMX_PTR        paramData);
                             
    OMX_ERRORTYPE get_state(OMX_HANDLETYPE hComp,
                            OMX_STATETYPE *state);
                           
                                    

    OMX_ERRORTYPE send_command(OMX_HANDLETYPE  hComp,
                               OMX_COMMANDTYPE cmd,
                               OMX_U32         param1,
                               OMX_PTR         cmdData);
                   
                         
    OMX_ERRORTYPE set_callbacks(OMX_HANDLETYPE   hComp,
                                OMX_CALLBACKTYPE *callbacks,
                                OMX_PTR          appData);
                             
    OMX_ERRORTYPE set_config(OMX_HANDLETYPE hComp,
                             OMX_INDEXTYPE  configIndex,
                             OMX_PTR        configData);
                          
    OMX_ERRORTYPE set_parameter(OMX_HANDLETYPE hComp,
                                OMX_INDEXTYPE  paramIndex,
                                OMX_PTR        paramData);
                             
    OMX_ERRORTYPE use_buffer(OMX_HANDLETYPE      hComp,
                             OMX_BUFFERHEADERTYPE **bufferHdr,
                             OMX_U32              port,
                             OMX_PTR              appData,
                             OMX_U32              bytes,
                             OMX_U8               *buffer);
                          

    OMX_ERRORTYPE use_EGL_image(OMX_HANDLETYPE     hComp,
                                OMX_BUFFERHEADERTYPE **bufferHdr,
                                OMX_U32              port,
                                OMX_PTR              appData,
                                void *               eglImage);
  
    static void   buffer_done_cb(struct vdec_context *ctxt,
                                 void *cookie);

    static void   buffer_done_cb_stub(struct vdec_context *ctxt,
                                      void *cookie);
                                   
    static void   frame_done_cb_stub(struct vdec_context *ctxt,
                                     struct vdec_frame *frame);

    static void   frame_done_cb(struct vdec_context *ctxt,
                                struct vdec_frame *frame);
                                  
    static void   process_event_cb(struct vdec_context *ctxt,
                                   unsigned char id);

    // Send one parsed NAL to the decoder
    void   send_nal(OMX_BUFFERHEADERTYPE* bufHdr,
                  OMX_U8* pBuffer,
                  unsigned int dataLen);

    // Inform EBD to OMX IL Client
    void  omx_vdec_inform_ebd(OMX_BUFFERHEADERTYPE* pBufHdr);
    int areNumOutBuffersOk(unsigned numOutFrames);

private:
    static omx_vdec* g_pVdecInstance;
    static pthread_mutex_t instance_lock;
    // Bit Positions 
    enum flags_bit_positions
    {
        // Defer transition to IDLE
        OMX_COMPONENT_IDLE_PENDING            =0x1,  
        // Defer transition to LOADING
        OMX_COMPONENT_LOADING_PENDING         =0x2,
        // First  Buffer Pending  
        OMX_COMPONENT_FIRST_BUFFER_PENDING    =0x3,
        // Second Buffer Pending  
        OMX_COMPONENT_SECOND_BUFFER_PENDING   =0x4,  
        // Defer transition to Enable
        OMX_COMPONENT_INPUT_ENABLE_PENDING    =0x5,
        // Defer transition to Enable
        OMX_COMPONENT_OUTPUT_ENABLE_PENDING   =0x6,
        // Defer transition to Disable
        OMX_COMPONENT_INPUT_DISABLE_PENDING   =0x7,
        // Defer transition to Disable
        OMX_COMPONENT_OUTPUT_DISABLE_PENDING  =0x8,
    };

    // Deferred callback identifiers
    enum 
    {
        //Event Callbacks from the vdec component thread context
        OMX_COMPONENT_GENERATE_EVENT       = 0x1,
        //Buffer Done callbacks from the vdec component thread context
        OMX_COMPONENT_GENERATE_BUFFER_DONE = 0x2,
        //Frame Done callbacks from the vdec component thread context
        OMX_COMPONENT_GENERATE_FRAME_DONE  = 0x3,
        //Buffer Done callbacks from the vdec component thread context
        OMX_COMPONENT_GENERATE_FTB         = 0x4,
        //Frame Done callbacks from the vdec component thread context
        OMX_COMPONENT_GENERATE_ETB         = 0x5,
        //Command
        OMX_COMPONENT_GENERATE_COMMAND     = 0x6,
        //Push-Pending Buffers
        OMX_COMPONENT_PUSH_PENDING_BUFS    = 0x7,
        // Empty Buffer Done callbacks
        OMX_COMPONENT_GENERATE_EBD         = 0x8,
        //Flush Event Callbacks from the vdec component thread context
        OMX_COMPONENT_GENERATE_EVENT_FLUSH       = 0x9,
    };

    enum port_indexes 
    {
        OMX_CORE_INPUT_PORT_INDEX        =0,
        OMX_CORE_OUTPUT_PORT_INDEX       =1
    };

    struct omx_event
    {
        unsigned param1; 
        unsigned param2;
        unsigned id; 
    };

    struct omx_cmd_queue
    {
        omx_event m_q[OMX_CORE_CONTROL_CMDQ_SIZE]; 
        unsigned m_read; 
        unsigned m_write; 
        unsigned m_size; 

        omx_cmd_queue(); 
        ~omx_cmd_queue(); 
        bool insert_entry(unsigned p1, unsigned p2, unsigned id); 
        bool pop_entry(unsigned *p1,unsigned *p2, unsigned *id); 
        // get msgtype of the first ele from the queue
        unsigned get_q_msg_type(); 

    }; 
    // Store buf Header mapping between OMX client and 
    // PMEM allocated.
    typedef Map<OMX_BUFFERHEADERTYPE*, OMX_BUFFERHEADERTYPE*> use_buffer_map;
    // Maintain a list of NALs to be sent to the decoder in the 
    // order parsed
    typedef Map<video_input_frame_info*, OMX_BUFFERHEADERTYPE*> pending_nals;

    // Get the pMem area from Video decoder
    void          omx_vdec_get_out_buf_hdrs();
    // Get the pMem area from video decoder and copy to local use buf hdrs
    void          omx_vdec_get_out_use_buf_hdrs();
    // Copy the decoded frame to the user defined buffer area
    void          omx_vdec_cpy_user_buf(OMX_BUFFERHEADERTYPE* pBufHdr);

    void          omx_vdec_add_entries(); 
 
    // Make a copy of the buf headers --> use buf only 
    OMX_ERRORTYPE omx_vdec_dup_use_buf_hdrs();

    OMX_ERRORTYPE omx_vdec_check_port_settings(
                                    OMX_U8* buffer,
                                    OMX_U32 nFilledLen,
                                    unsigned  &height, unsigned &width,
                                    unsigned &cropl,unsigned &cropr,
                                    unsigned &cropt, unsigned &cropb,
				    unsigned &numOutFrames);
    OMX_ERRORTYPE omx_vdec_validate_port_param(int height, int width, unsigned numOutFrames);

    void          omx_vdec_display_in_buf_hdrs();
    void          omx_vdec_display_out_buf_hdrs();
    void          omx_vdec_display_out_use_buf_hdrs();

    bool allocate_done(void);
    bool allocate_input_done(void);
    bool allocate_output_done(void);

    OMX_ERRORTYPE allocate_input_buffer(OMX_HANDLETYPE       hComp,
                                        OMX_BUFFERHEADERTYPE **bufferHdr,
                                        OMX_U32              port,
                                        OMX_PTR              appData,
                                        OMX_U32              bytes);
                                    
    OMX_ERRORTYPE allocate_output_buffer(OMX_HANDLETYPE       hComp,
                                         OMX_BUFFERHEADERTYPE **bufferHdr,
                                         OMX_U32 port,OMX_PTR appData,
                                         OMX_U32              bytes);
                                     
    OMX_ERRORTYPE use_input_buffer(OMX_HANDLETYPE hComp,
                                   OMX_BUFFERHEADERTYPE  **bufferHdr,
                                   OMX_U32               port,
                                   OMX_PTR               appData,
                                   OMX_U32               bytes,
                                   OMX_U8                *buffer);

    OMX_ERRORTYPE use_output_buffer(OMX_HANDLETYPE hComp,
                                   OMX_BUFFERHEADERTYPE   **bufferHdr,
                                   OMX_U32                port,
                                   OMX_PTR                appData,
                                   OMX_U32                bytes,
                                   OMX_U8                 *buffer);

    bool execute_omx_flush(OMX_U32);
    bool execute_output_flush(void);
    bool execute_input_flush(void);
    unsigned push_one_input_buffer(OMX_BUFFERHEADERTYPE *buffer);
    unsigned push_pending_buffers(void);
    unsigned push_pending_buffers_proxy(void);
/*    OMX_ERRORTYPE empty_buffer_done(OMX_HANDLETYPE hComp,
                                    OMX_BUFFERHEADERTYPE * buffer);
*/
    OMX_ERRORTYPE empty_this_buffer_proxy(OMX_HANDLETYPE       hComp,
                                        OMX_BUFFERHEADERTYPE *buffer);

    OMX_ERRORTYPE fill_this_buffer_proxy(OMX_HANDLETYPE       hComp,
                                       OMX_BUFFERHEADERTYPE *buffer);
    bool release_done();

    bool release_output_done();
    bool release_input_done();
  
    OMX_ERRORTYPE send_command_proxy(OMX_HANDLETYPE  hComp,
                                     OMX_COMMANDTYPE cmd,
                                     OMX_U32         param1,
                                     OMX_PTR         cmdData);

    inline unsigned get_output_buffer_size()
    {
        return (((m_port_height * m_port_width)/2) * 3); 
    }

    inline unsigned get_output_buffer_offset()
    {
        unsigned offset = 0;
        if(m_crop_x || m_crop_y)
        {
            offset = m_crop_y * m_port_width + m_crop_x;
        }
        return offset;
    }

    inline void omx_vdec_set_use_buf_flg()
    {
        m_is_use_buffer = true;
    }
    inline void omx_vdec_reset_use_buf_flg()
    {
        m_is_use_buffer = false;
    }
    inline bool omx_vdec_get_use_buf_flg()
    {
        return m_is_use_buffer; 
    }
    bool post_event(unsigned int p1,
                  unsigned int p2,
                  unsigned int id);

    //*************************************************************
    //*******************MEMBER VARIABLES *************************
    //*************************************************************
    // OMX State
    OMX_STATETYPE         m_state;   
    // Application data
    OMX_PTR               m_app_data;  
    // Application callbacks
    OMX_CALLBACKTYPE      m_cb;   
    OMX_COLOR_FORMATTYPE  m_color_format;                          
    OMX_PRIORITYMGMTTYPE m_priority_mgm ;
    OMX_PARAM_BUFFERSUPPLIERTYPE m_buffer_supplier;
    OMX_BUFFERHEADERTYPE  **input; 
    OMX_BUFFERHEADERTYPE* m_loc_use_buf_hdr;

    // video decoder context
    struct VDecoder       *m_vdec;   
    // fill this buffer queue
    omx_cmd_queue         m_ftb_q;
    // Command Q for rest of the events
    omx_cmd_queue         m_cmd_q;
    omx_cmd_queue         m_etb_q;
    // Input memory pointer
    char                  *m_inp_mem_ptr; 
    char                  *m_inp_buf_mem_ptr; 
    // Output memory pointer
    char                  *m_out_mem_ptr; 

    int                   m_first_pending_buf_idx;
    int                   m_outstanding_frames;
    /* Pending Flush Check */
    int                   m_pending_flush;      
    // EOS Timestamp
    signed long long      m_eos_timestamp;
    // bitmask array size for output side
    unsigned int          m_out_bm_count;
    // Number of Output Buffers
    unsigned char         m_out_buf_count; 
    // Number of Input Buffers
    unsigned int	  m_out_buf_num_port_reconfig;
    unsigned int          m_inp_buf_count;
    // Size of Input Buffers
    unsigned int          m_inp_buf_size;
    // bitmask array size for input side
    unsigned int          m_inp_bm_count;
    //Input port Populated
    OMX_BOOL              m_inp_bPopulated;
    //Output port Populated
    OMX_BOOL              m_out_bPopulated;
    //Height
    unsigned int          m_height;   
    // Width
    unsigned int          m_width;   
    // Storage of HxW during dynamic port reconfig
    unsigned int          m_port_height;   
    unsigned int          m_port_width;   

    unsigned int m_crop_x;
    unsigned int m_crop_y;
    unsigned int m_crop_dx;
    unsigned int m_crop_dy;
    // encapsulate the waiting states.
    unsigned int          m_flags;  
    // size of NAL length
    OMX_U32               m_nalu_bytes;
    genericQueue* flush_before_vdec_op_q;

#ifdef _ANDROID_
    // Heap pointer to frame buffers
    sp<MemoryHeapBase>    m_heap_ptr;
#endif //_ANDROID_
    unsigned int         m_out_flags; 
    // message count
    unsigned              m_msg_cnt;
    // command count
    unsigned              m_cmd_cnt;
    // Empty This Buffer count
    unsigned              m_etb_cnt;
    // Empty Buffer Done Count
    unsigned              m_ebd_cnt;
    // NAL buffer done cnt
    unsigned              m_nal_bd_cnt;
    // Fill This Buffer count
    unsigned              m_ftb_cnt;
    // Fill Buffer done count
    unsigned              m_fbd_cnt;
    // store I/P PORT state
    OMX_BOOL              m_inp_bEnabled;
    // store O/P PORT state
    OMX_BOOL              m_out_bEnabled;
    // to know whether Event Port Settings change has been triggered or not.
    bool                  m_event_port_settings_sent;
    // is USE Buffer in use
    bool                  m_is_use_buffer;
    //bool                  m_is_use_pmem_buffer;
    // EOS notify pending to the IL client
    bool                  m_bEoSNotifyPending; 

    // check if it is multiple NALs per buffer
    bool                  m_multiple_nals_per_buffer;

    pthread_mutex_t       m_lock; 

    // wait till the first 8 FTB's are recieved
    pthread_mutex_t       m_ftb_lock;
    OMX_U8                m_ftb_rxd_cnt;
    OMX_U8                m_cRole[OMX_MAX_STRINGNAME_SIZE];
    bool                  m_ftb_rxd_flg;

    // mutex for handling nal bd count
    pthread_mutex_t       m_nal_bd_lock;
#ifndef PC_DEBUG
    //sem to handle the minimum procesing of commands
    sem_t                 m_cmd_lock;
#endif

    use_buffer_map        m_use_buf_hdrs;
    pending_nals          m_in_pend_nals;

    // Input frame details
    struct video_input_frame_info       m_frame_info;
    // Platform specific details
    OMX_QCOM_PLATFORM_PRIVATE_LIST      *m_platform_list;
    OMX_QCOM_PLATFORM_PRIVATE_ENTRY     *m_platform_entry;
    OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *m_pmem_info;
    OmxUtils                            *m_omx_utils;

    // SPS+PPS sent as part of set_config
    OMX_VENDOR_EXTRADATATYPE            m_vendor_config;    

};

#endif // __OMX_VDEC_H__





#include "vdecoder_i.h"
#include <stdio.h>

 //Constructor
VideoDecoder::VideoDecoder( void )
	 :m_bDecoderInitialized(false),
	 fnPtrInitialize(NULL),
	 fnPtrReuseFrameBuffer(NULL),
     fnPtrFlush(NULL),
	 fnPtrEOS(NULL),
	 fnPtrSuspend(NULL),
	 fnPtrResume(NULL),
	 fnPtrDecode(NULL),
	 fnPtrSetParameter(NULL),
	 fnPtrGetParameter(NULL),
	 fnPtrDestructor(NULL)
 {};

 //Destructor
VideoDecoder::~VideoDecoder( void )
 {
	 if(fnPtrDestructor) (this->*fnPtrDestructor)();
 };

 // Subclasses are required to implement Following Functions;
 // currently there is no common behavior for the base class
VDEC_ERROR VideoDecoder::Initialize
 (
    const unsigned short            nLayers,
    VDEC_MALLOC_FN                  pfnFrameBufferMalloc,
    VDEC_FREE_FN                    pfnFrameBufferFree,
    VDEC_EVENT_CB_FN                pEventCb,
    void * const                    pEventCbData,
    VDEC_DIMENSIONS                &frameSize,
    VDEC_CONCURRENCY_CONFIG         concurrencyConfig,
    VDEC_SLICE_BUFFER_INFO *        slice_buffer_info,
    unsigned			    numOutBuffers,
    int				    adsp_fd
	)
 {  
	if(!fnPtrInitialize) return VDEC_ERR_NULL_POINTER;
	return (this->*(fnPtrInitialize))(
					nLayers,
					pfnFrameBufferMalloc,
					pfnFrameBufferFree,
					pEventCb,
					pEventCbData,
					frameSize,
					concurrencyConfig,
					slice_buffer_info,
					numOutBuffers,
					adsp_fd
		);
 };

 VDEC_ERROR VideoDecoder::ReuseFrameBuffer( VDEC_FRAME * const pFrame )
 {
	 if(!fnPtrReuseFrameBuffer) return VDEC_ERR_NULL_POINTER;
	 return (this->*fnPtrReuseFrameBuffer)(pFrame); 
 };
 VDEC_ERROR VideoDecoder::Flush( int *nFlushedFrames )
 {
	 if(!fnPtrFlush) return VDEC_ERR_NULL_POINTER;
	 return (this->*fnPtrFlush)(nFlushedFrames);
 };
 VDEC_ERROR VideoDecoder::EOS( void )
 {
	 if(!fnPtrEOS) return VDEC_ERR_NULL_POINTER;
	 return (this->*fnPtrEOS)();
 };
 VDEC_ERROR VideoDecoder::Suspend( void )
 {
	 if(!fnPtrSuspend) return VDEC_ERR_NULL_POINTER;
	 return (this->*fnPtrSuspend)();
 };
 VDEC_ERROR VideoDecoder::Resume( VDEC_CONCURRENCY_CONFIG concurrencyConfig)
 {
	 if(!fnPtrResume) return VDEC_ERR_NULL_POINTER;
	 return (this->*fnPtrResume)(concurrencyConfig);
 };
 VDEC_ERROR VideoDecoder::Decode( VDEC_INPUT_BUFFER * input , bool checkBufAvail)
 {
	 if(!fnPtrDecode) return VDEC_ERR_NULL_POINTER;
	 return (this->*fnPtrDecode)(input, checkBufAvail);
 };
  VDEC_ERROR VideoDecoder::SetParameter( VDEC_PARAMETER_ID parameter,
                                         VDEC_PARAMETER_DATA * const info )
  {
	  if(!fnPtrSetParameter) return VDEC_ERR_NULL_POINTER;
	  return (this->*fnPtrSetParameter)(parameter,info);
  };
  VDEC_ERROR VideoDecoder::GetParameter( VDEC_PARAMETER_ID parameter,
	                                     VDEC_PARAMETER_DATA *info )
  {
	  if(!fnPtrGetParameter) return VDEC_ERR_NULL_POINTER;
	  return (this->*fnPtrGetParameter)(parameter,info);
  };

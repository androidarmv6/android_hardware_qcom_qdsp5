#ifndef OMX_VENC_H
#define OMX_VENC_H

/*============================================================================
@file OMX_Venc.h

Definition for the omx mpeg4 video encoder component. This class provides
a 1 to 1 mapping of functions defined in the OMX_COMPONENTTYPE structure. 
Refer to OMX_Component.h for the description
of these functions.

                     Copyright (c) 2008 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
============================================================================*/

/*----------------------------------------------------------------------------
 *
 * $Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/omx/inc/OMX_Venc.h#5 $ 
 *
 * when         who     what, where, why
 * ----------   ---     ----------------------------------------------------
 * 2010-04-02   as      Added support for TimeStamp scale factor.
 * 2010-03-22   as      Added approved OMX extension 
 * 2009-10-02   as      Added support for change quality, out of band vol.
 * 2009-08-24   bc      On-target integration updates
 * 2009-08-13   bc      OMX component failure case bug fixes 
 * 2009-04-05   as      sync with andriod release 
 * 2008-12-15   bc      Updates for single thread design
 * 2008-11-20   bc      Warning removal
 * 2008-08-14   bc      Fixed pmem buffer deallocation crash
 * 2008-07-15   bc      Buffer management bug fix
 * 2008-07-15   bc      Added support for h263
 * 2008-06-12   bc      Temporary debugging update
 * 2008-06-11   bc      Bug fixes for initial target integration.
 * 2008-06-03   bc      Created file.
 *
 * -------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * Include Files
 * -------------------------------------------------------------------------*/
#include "venc_dev_api.h"
#include "qc_omx_component.h"
#include "OMX_Types.h"
#include "OMX_Core.h"
#include "OMX_QCOMExtns.h"

/*----------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * -------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
 * Type Declarations
 * -------------------------------------------------------------------------*/
class VencBufferManager; // forward declaration
class VencMsgQ; // forward declaration

/*---------------------------------------------------------------------------
 * Class Definitions
 ---------------------------------------------------------------------------*/

/**
 * The omx mpeg4 video encoder class.
 */
class Venc : public qc_omx_component
{
public:

  /**********************************************************************//**
   * @brief Class constructor
   *************************************************************************/
   static Venc* get_instance(); 

  /**********************************************************************//**
   * @brief Class destructor
   *************************************************************************/
   virtual ~Venc();

  /**********************************************************************//**
   * @brief Initializes the component
   *
   * @return error if unsuccessful.
   *************************************************************************/
   virtual OMX_ERRORTYPE component_init(OMX_IN OMX_STRING pComponentName);

   //////////////////////////////////////////////////////////////////////////
   /// For the following methods refer to corresponding function descriptions
   /// in the OMX_COMPONENTTYPE structure in OMX_Componenent.h
   //////////////////////////////////////////////////////////////////////////

   virtual OMX_ERRORTYPE get_component_version(
      OMX_IN  OMX_HANDLETYPE hComponent,
      OMX_OUT OMX_STRING pComponentName,
      OMX_OUT OMX_VERSIONTYPE* pComponentVersion,
      OMX_OUT OMX_VERSIONTYPE* pSpecVersion,
      OMX_OUT OMX_UUIDTYPE* pComponentUUID);

   virtual OMX_ERRORTYPE send_command(
      OMX_IN  OMX_HANDLETYPE hComponent,
      OMX_IN  OMX_COMMANDTYPE Cmd,
      OMX_IN  OMX_U32 nParam1,
      OMX_IN  OMX_PTR pCmdData);

   virtual OMX_ERRORTYPE get_parameter(
      OMX_IN  OMX_HANDLETYPE hComponent, 
      OMX_IN  OMX_INDEXTYPE nParamIndex,  
      OMX_INOUT OMX_PTR pComponentParameterStructure);

   virtual OMX_ERRORTYPE set_parameter(
      OMX_IN  OMX_HANDLETYPE hComponent, 
      OMX_IN  OMX_INDEXTYPE nIndex,
      OMX_IN  OMX_PTR pComponentParameterStructure);


   virtual OMX_ERRORTYPE get_config(
      OMX_IN  OMX_HANDLETYPE hComponent,
      OMX_IN  OMX_INDEXTYPE nIndex, 
      OMX_INOUT OMX_PTR pComponentConfigStructure);


   virtual OMX_ERRORTYPE set_config(
      OMX_IN  OMX_HANDLETYPE hComponent,
      OMX_IN  OMX_INDEXTYPE nIndex, 
      OMX_IN  OMX_PTR pComponentConfigStructure);


   virtual OMX_ERRORTYPE get_extension_index(
      OMX_IN  OMX_HANDLETYPE hComponent,
      OMX_IN  OMX_STRING cParameterName,
      OMX_OUT OMX_INDEXTYPE* pIndexType);


   virtual OMX_ERRORTYPE get_state(
      OMX_IN  OMX_HANDLETYPE hComponent,
      OMX_OUT OMX_STATETYPE* pState);

    
   virtual OMX_ERRORTYPE component_tunnel_request(
      OMX_IN  OMX_HANDLETYPE hComp,
      OMX_IN  OMX_U32 nPort,
      OMX_IN  OMX_HANDLETYPE hTunneledComp,
      OMX_IN  OMX_U32 nTunneledPort,
      OMX_INOUT  OMX_TUNNELSETUPTYPE* pTunnelSetup); 

   virtual OMX_ERRORTYPE use_buffer(
      OMX_IN OMX_HANDLETYPE hComponent,
      OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
      OMX_IN OMX_U32 nPortIndex,
      OMX_IN OMX_PTR pAppPrivate,
      OMX_IN OMX_U32 nSizeBytes,
      OMX_IN OMX_U8* pBuffer);

   virtual OMX_ERRORTYPE allocate_buffer(
      OMX_IN OMX_HANDLETYPE hComponent,
      OMX_INOUT OMX_BUFFERHEADERTYPE** ppBuffer,
      OMX_IN OMX_U32 nPortIndex,
      OMX_IN OMX_PTR pAppPrivate,
      OMX_IN OMX_U32 nSizeBytes);

   virtual OMX_ERRORTYPE free_buffer(
      OMX_IN  OMX_HANDLETYPE hComponent,
      OMX_IN  OMX_U32 nPortIndex,
      OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

   virtual OMX_ERRORTYPE empty_this_buffer(
      OMX_IN  OMX_HANDLETYPE hComponent,
      OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

   virtual OMX_ERRORTYPE fill_this_buffer(
      OMX_IN  OMX_HANDLETYPE hComponent,
      OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

   virtual OMX_ERRORTYPE set_callbacks(
      OMX_IN  OMX_HANDLETYPE hComponent,
      OMX_IN  OMX_CALLBACKTYPE* pCallbacks, 
      OMX_IN  OMX_PTR pAppData);

   virtual OMX_ERRORTYPE component_deinit(
      OMX_IN  OMX_HANDLETYPE hComponent);

   virtual OMX_ERRORTYPE use_EGL_image(
      OMX_IN OMX_HANDLETYPE hComponent,
      OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
      OMX_IN OMX_U32 nPortIndex,
      OMX_IN OMX_PTR pAppPrivate,
      OMX_IN void* eglImage);

   virtual OMX_ERRORTYPE component_role_enum(
      OMX_IN OMX_HANDLETYPE hComponent,
      OMX_OUT OMX_U8 *cRole,
      OMX_IN OMX_U32 nIndex);

private:
   static Venc* g_pVencInstance;

   /// Number of input frames required
   static const OMX_S32 MIN_IN_BUFFERS = 4;

   /// Number of output frames required
   static const OMX_S32 MIN_OUT_BUFFERS = 6;

   /// Port indexes according to the OMX IL spec
   enum PortIndexType
   {
      PORT_INDEX_IN = 0,
      PORT_INDEX_OUT = 1,
      PORT_INDEX_BOTH = -1,
      PORT_INDEX_NONE = -2
   };

   // Bit Positions
   enum flags_bit_positions
   {
      // Defer transition to IDLE
      OMX_COMPONENT_IDLE_PENDING = 0x1,
      // Defer transition to LOADING
      OMX_COMPONENT_LOADING_PENDING = 0x2,
      // Defer transition to Enable
      OMX_COMPONENT_INPUT_ENABLE_PENDING = 0x3,
      // Defer transition to Enable
      OMX_COMPONENT_OUTPUT_ENABLE_PENDING = 0x4,
      // Defer transition to Disable
      OMX_COMPONENT_INPUT_DISABLE_PENDING = 0x5,
      // Defer transition to Disable
      OMX_COMPONENT_OUTPUT_DISABLE_PENDING = 0x6,
      //Defer going to the Invalid state
      OMX_COMPONENT_INVALID_PENDING = 0x9,
      //Defer going to Pause state
      OMX_COMPONENT_PAUSE_PENDING = 0xA
   };

   /// Private data for buffer header field pInputPortPrivate
   struct PrivatePortData
   {
      OMX_BOOL bComponentAllocated; ///< Did we allocate this buffer?
      venc_pmem_info sPmemInfo;     ///< pmem info

      OMX_BOOL bZeroLen;            ///< Override the frame length to zero for EOS
   };

private:

  /**********************************************************************//**
   * @brief Class constructor
   *************************************************************************/
   Venc();
   
   /**********************************************************************//**
   * @brief encode frame function.
   *
   * Method will check to see if there is an available input
   * and output buffer. If so, then it will encode, otherwise it will 
   * queue the specified buffer until it receives another buffer.
   *
   * @param pBuffer: pointer to the input or output buffer.
   * @return NULL (ignore)
   *************************************************************************/
   OMX_ERRORTYPE encode_frame(OMX_BUFFERHEADERTYPE* pBuffer);

  /**********************************************************************//**
   * @brief Component thread main function.
   *
   * @param pClassObj: Pointer to the Venc instance.
   * @return NULL (ignore)
   *************************************************************************/
   static int component_thread(void* pClassObj);

  /**********************************************************************//**
   * @brief Process thread message MSG_ID_STATE_CHANGE
   *
   * @param nPortIndex: The port index
   *************************************************************************/
   void process_state_change(OMX_STATETYPE eState);

  /**********************************************************************//**
   * @brief Process thread message MSG_ID_FLUSH
   *
   * @param nPortIndex: The port index
   *************************************************************************/
   void process_flush(OMX_U32 nPortIndex);

  /**********************************************************************//**
   * @brief Process thread message MSG_ID_STATE_CHANGE
   *
   * @param nPortIndex: The port index
   *************************************************************************/
   void process_port_enable(OMX_U32 nPortIndex);

  /**********************************************************************//**
   * @brief Process thread message MSG_ID_STATE_CHANGE
   *
   * @param nPortIndex: The port index
   *************************************************************************/
   void process_port_disable(OMX_U32 nPortIndex);

  /**********************************************************************//**
   * @brief Process thread message MSG_ID_STATE_CHANGE
   *
   * @param nPortIndex: The port index
   * @param pMarkData: The mark buffer data
   *************************************************************************/
   void process_mark_buffer(OMX_U32 nPortIndex,
                            const OMX_MARKTYPE* pMarkData);

  /**********************************************************************//**
   * @brief Process thread message MSG_ID_DL_STATUS
   *
   * Refer to source file for detailed description of params.
   *************************************************************************/
   void process_DL_status(OMX_U32 nDLHandle,
                          OMX_U32 nCommand,
                          OMX_U32 nStatus);

  /**********************************************************************//**
   * @brief Process thread message MSG_ID_DL_FRAME_DONE
   *
   * Refer to source file for detailed description of params.
   *************************************************************************/
   void process_DL_frame_done(OMX_U32 nStatus,
                              const void* pInfo);
  /**********************************************************************//**
   * @brief Process thread message process_DL_empty_done
   *
   * Refer to source file for detailed description of params.
   *************************************************************************/

   void process_DL_empty_done(const void* pInfo);

   static void DL_status_callback(uint32 nDLHandle,
                                  venc_event_type eStatusEvent,
                                  venc_status_param_type sParam,
                                  void* pClientData);
   static void DL_frame_callback(uint32 nDLHandle,
                                 venc_status_type eStatus,
                                 venc_frame_info_type sFrameInfo,
                                 void* pClientData);
  /**********************************************************************//**
   * @brief Process thread message empty_this_buffer_proxy
   *
   * Refer to source file for detailed description of params.
   *************************************************************************/
   void empty_this_buffer_proxy(OMX_HANDLETYPE hComponent,OMX_BUFFERHEADERTYPE *buffer);
  /**********************************************************************//**
   * @brief Process thread message fill_this_buffer_proxy
   *
   * Refer to source file for detailed description of params.
   *************************************************************************/

   void fill_this_buffer_proxy(OMX_BUFFERHEADERTYPE *buffer);

   /**********************************************************************//**
   * cleanup PORT_INDEX_OUT & PORT_INDEX_IN as error recovey mechanism
   *
   *************************************************************************/
   void free_port_inout(void);

   /**********************************************************************//**
   * Dump input yuv(input = 1) & output bitstream(input =0) 
   *
   *************************************************************************/

   void mem_dump(char* data, int n_bytes,int input) ;

   /**********************************************************************//**
   * Check for the actual buffer allocate/deallocate complete
   *   
   *************************************************************************/
   OMX_BOOL allocate_done(void);
   OMX_BOOL release_done();
   /**********************************************************************//**
   * Configure video encoder before intialization
   *   
   *************************************************************************/
   void translate_config(venc_config_param_type* pConfigParam);
   /*************************************************************************
    * change QP, FrameRate, Bitrate during the session
    **************************************************************************/
   OMX_ERRORTYPE change_quality();
private:

   /// thread object
   void* m_thread;

   /// semaphore object
   void* m_cmd_sem;

   /// semaphore object
   void* m_loadUnloadSignal;

   /// Specifies the port currently flushing
   OMX_BOOL m_bFlushingIndex_in;
   OMX_BOOL m_bFlushingIndex_out;

   /// Free input buffer manager
   VencBufferManager* m_pFreeInBM;

   /// Free output buffer manager
   VencBufferManager* m_pFreeOutBM;

   /// Message Q
   VencMsgQ* m_pMsgQ;

   /// Device layer config
   venc_config_param_type m_sDLConfig;

   /// Device layer handle
   OMX_U32 m_nDLHandle;

   /// Private input buffer data
   PrivatePortData *m_sPrivateInPortData;

   /// Private output buffer data
   PrivatePortData *m_sPrivateOutPortData;

   /// Device Layer Configuration parameter
   venc_config_param_type m_sConfigParam;

   // Dynamic Configuration params
   venc_quality_type quality;

private:

   OMX_STATETYPE m_eState;
   OMX_STATETYPE m_eTargetState;
   OMX_CALLBACKTYPE m_sCallbacks;
   OMX_PTR m_pAppData;
   OMX_HANDLETYPE m_hSelf;
   OMX_PORT_PARAM_TYPE m_sPortParam;
   OMX_PORT_PARAM_TYPE m_sPortParam_img;
   OMX_PORT_PARAM_TYPE m_sPortParam_audio;
   OMX_PARAM_PORTDEFINITIONTYPE m_sInPortDef;
   OMX_PARAM_PORTDEFINITIONTYPE m_sOutPortDef;
   OMX_MARKTYPE m_Markdata;
   OMX_BOOL m_bPanic;
   OMX_BOOL m_bDeviceInit_done;
   OMX_BOOL m_bDeviceStart;
   OMX_BOOL m_bDeviceExit_done;
   OMX_U32 m_flags;
   OMX_U32 m_in_alloc_cnt;
   OMX_U32 m_out_alloc_cnt;
   OMX_U32 m_nInputDown;         // number of input buffers pushed down to device layer
   OMX_U32 m_nOutputDown;        // number of output buffers pushed down to device layer
   OMX_BOOL m_Mark_command;
   OMX_BOOL m_Mark_port;
   OMX_BOOL m_bIsQcomPvt;
   OMX_PTR m_finput;
   OMX_PTR m_foutput;
   OMX_QCOM_PARAM_PORTDEFINITIONTYPE m_sInQcomPortDef;
   OMX_QCOM_PLATFORMPRIVATE_EXTN m_sQcomPlatformPvt;
   OMX_VIDEO_PARAM_PORTFORMATTYPE m_sInPortFormat;
   OMX_VIDEO_PARAM_PORTFORMATTYPE m_sOutPortFormat;
   OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE m_sErrorCorrection;
   OMX_PRIORITYMGMTTYPE m_sPriorityMgmt;
   OMX_PARAM_BUFFERSUPPLIERTYPE m_sInBufSupplier;
   OMX_PARAM_BUFFERSUPPLIERTYPE m_sOutBufSupplier;
   OMX_VIDEO_PARAM_MPEG4TYPE m_sParamMPEG4;
   OMX_VIDEO_PARAM_AVCTYPE m_sParamAVC;
   OMX_VIDEO_PARAM_BITRATETYPE m_sParamBitrate;
   OMX_VIDEO_PARAM_PROFILELEVELTYPE m_sParamProfileLevel;
   OMX_VIDEO_PARAM_INTRAREFRESHTYPE m_sParamIntraRefresh;
   OMX_VIDEO_CONFIG_BITRATETYPE m_sConfigBitrate;
   OMX_CONFIG_FRAMERATETYPE m_sConfigFramerate;
   OMX_CONFIG_ROTATIONTYPE m_sConfigFrameRotation;
   OMX_VIDEO_PARAM_H263TYPE m_sParamH263;
   OMX_VIDEO_PARAM_QUANTIZATIONTYPE m_sParamQPs;
   OMX_CONFIG_INTRAREFRESHVOPTYPE m_sConfigIntraRefreshVOP;
   QOMX_VIDEO_TEMPORALSPATIALTYPE m_sTsTradeoffFactor;
   QOMX_VIDEO_SYNTAXHDRTYPE m_sSyntaxhdr;
   QOMX_VIDEO_INTRAPERIODTYPE m_sConfigIntraPeriod;
   QOMX_VIDEO_PARAM_ENCODERMODETYPE m_sConfigEncoderModeType;
   #ifndef FEATURE_LINUX_A
   QOMX_TIMESTAMPSCALETYPE m_sTimeStampScale;
   #endif
   OMX_VIDEO_CONFIG_NALSIZE m_sConfigNAL;
   OMX_BUFFERHEADERTYPE *m_sInBuffHeaders;
   OMX_BUFFERHEADERTYPE *m_sOutBuffHeaders;
   OMX_STRING m_pComponentName;
   OMX_U8 m_cRole[OMX_MAX_STRINGNAME_SIZE];
   OMX_BOOL m_bGetSyntaxHdr;
   OMX_BOOL m_bComponentInitialized;
   OMX_BOOL m_bPendingChangeQuality;
};

#endif

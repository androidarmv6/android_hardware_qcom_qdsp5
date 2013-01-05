#ifndef OMX_VENC_MSG_Q_H
#define OMX_VENC_MSG_Q_H
/*============================================================================
@file OMX_VencMsgQ.h

Message queue class for video encoder openMax component..

                     Copyright (c) 2008 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
============================================================================*/

/*----------------------------------------------------------------------------
 *
 * $Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/omx/src/OMX_VencMsgQ.h#1 $ 
 *
 * when         who     what, where, why
 * ----------   ---     ----------------------------------------------------
 * 2009-07-17   bc      Introduced OMX compliance
 * 2009-01-21   as      Andriod changes merge
 * 2007-10-10   bc      Created file.
 *
 * -------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * Include Files
 * -------------------------------------------------------------------------*/

#include "OMX_Core.h"
#include "venc_dev_api.h"
#include "venc_mutex.h"
#include "venc_signal.h"

/*----------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * -------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * Type Declarations
 * -------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
 * Class Definitions
 ---------------------------------------------------------------------------*/

///@todo document
class VencMsgQ
{
public:
   /// max size for component thread message queue
   static const int MAX_MSG_QUEUE_SIZE = 32;

   /// Ids for thread messages
   enum MsgIdType
   {
      MSG_ID_EXIT,         ///< Thread exit command
      MSG_ID_STATE_CHANGE, ///< State change command
      MSG_ID_FLUSH,        ///< Flush command
      MSG_ID_PORT_DISABLE, ///< Port disable command
      MSG_ID_PORT_ENABLE,  ///< Port enable command
      MSG_ID_MARK_BUFFER,  ///< Mark buffer command
      MSG_ID_DL_STATUS,    ///< Device layer status callback
      MSG_ID_DL_FRAME_DONE,   ///< Device layer frame done callback
      MSG_ID_YUV_BUFF_DONE, ///< Device layer EBD call back
      MSG_ID_EMPTY_BUFF,      ///<ETB processing in compoent thread
      MSG_ID_FILL_BUFF        ///<FTB processing in component thread
   };

   /// Data for thread messages
   union MsgDataType
   {
      OMX_STATETYPE eState; ///< State associated with MSG_ID_STATE_CHANGE
      OMX_U32 nPortIndex;   ///< Port index for MSG_ID_FLUSH, 
                            ///<                MSG_ID_PORT_ENABLE
                            ///<                MSG_ID_PORT_DISABLE

      struct
      {
          OMX_BUFFERHEADERTYPE *pBuffer; ///<Input or Output buffer
          OMX_HANDLETYPE hComponent; ///<Input hComponent
      }sEncodeBuffer;
      /// anonymous structure for MSG_ID_MARK_BUFFER
      struct
      {
         OMX_U32 nPortIndex;      ///< Corresponding port for command
         OMX_MARKTYPE sMarkData;  ///< Mark data structure
      } sMarkBuffer;

      /// anonymous structure for MSG_ID_DL_STATUS
      struct
      {
         uint32 nDLHandle;
         venc_command_type eCommand;
         venc_status_type eStatus;
      } sDLStatus;

      /// anonymous structure for MSG_ID_FRAME_DONE
      struct
      {
         venc_status_type eStatus;
         venc_frame_info_type sFrameInfo;
      } sDLFrameDone;
      ///anonymous structure for MSG_ID_EMPTY_BUFF_DONE
      struct
      {
          venc_status_param_type sBuffInfo;
      }sDLYUVDone;
   };

   /// Msgs for component thread
   struct MsgType
   {
      MsgIdType id;     ///< message id
      MsgDataType data; ///< message data
   };

public:

   VencMsgQ() :
      m_nHead(0),
      m_nSize(0)
   {
      memset(m_aMsgQ, 0, sizeof(MsgType) * MAX_MSG_QUEUE_SIZE);
      if (venc_mutex_create(&m_mutex) != 0)
      {
         OMX_VENC_MSG_ERROR("failed to create mutex", 0, 0, 0);
      }
      if (venc_signal_create(&m_signal) != 0)
      {
         OMX_VENC_MSG_ERROR("failed to create signal", 0, 0, 0);
      }
   }

   ~VencMsgQ() 
   {
      if (venc_mutex_destroy(m_mutex) != 0)
      {
         OMX_VENC_MSG_ERROR("failed to destroy mutex", 0, 0, 0);
      }
      if (venc_signal_destroy(m_signal) != 0)
      {
         OMX_VENC_MSG_ERROR("failed to destroy signal", 0, 0, 0);
      }
   }

   OMX_ERRORTYPE PushMsg(MsgIdType eMsgId,
                         const MsgDataType* pMsgData)
   {

      venc_mutex_lock(m_mutex);

      // find the tail of the queue
      int idx = (m_nHead + m_nSize) % MAX_MSG_QUEUE_SIZE;

      if (m_nSize >= MAX_MSG_QUEUE_SIZE)
      {       
         venc_mutex_unlock(m_mutex);
         return OMX_ErrorInsufficientResources;
      }

      // put data at tail of queue
      if (pMsgData != NULL)
      {
         memcpy(&m_aMsgQ[idx].data, pMsgData, sizeof(MsgDataType));
      }
      m_aMsgQ[idx].id = eMsgId;

      // update queue size
      ++m_nSize;

      // unlock queue
      venc_mutex_unlock(m_mutex);

      // signal the component thread
      venc_signal_set(m_signal);

   
      return OMX_ErrorNone;
   }

   OMX_ERRORTYPE PopMsg(MsgType* pMsg)
   {

      // listen for a message...   
      while (m_nSize == 0)
      {
         venc_signal_wait(m_signal, 0);
      }     

      venc_mutex_lock(m_mutex);

      // get msg at head of queue
      memcpy(pMsg,
              &m_aMsgQ[m_nHead],
              sizeof(MsgType));

      // pop message off the queue
      m_nHead = (m_nHead + 1) % MAX_MSG_QUEUE_SIZE;
      --m_nSize;
      venc_mutex_unlock(m_mutex);

      return OMX_ErrorNone;
   }

private:
   
   MsgType m_aMsgQ[MAX_MSG_QUEUE_SIZE];  ///< queue data
   int m_nHead;   ///< head of the queue
   int m_nSize;   ///< size of the queue
   
   void* m_mutex;  ///< message q lock
   void* m_signal;  ///< message q data available signal
};
#endif // #ifndef OMX_VENC_MSG_Q_H

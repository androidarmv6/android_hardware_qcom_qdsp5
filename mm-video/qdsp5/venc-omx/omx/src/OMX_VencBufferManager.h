#ifndef OMX_VENC_BUFFER_MANAGER_H
#define OMX_VENC_BUFFER_MANAGER_H
/*============================================================================
@file OMX_VencBufferManager.h

Buffer manager class for video encoder openMax component.

                     Copyright (c) 2008 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
============================================================================*/

/*----------------------------------------------------------------------------
 *
 * $Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/omx/src/OMX_VencBufferManager.h#1 $ 
 *
 * when         who     what, where, why
 * ----------   ---     ----------------------------------------------------
 * 2008-07-15   bc      Buffer management bug fix
 * 2007-10-10   bc      Created file.
 *
 * -------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * Include Files
 * -------------------------------------------------------------------------*/

#include "OMX_Core.h"

/*----------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * -------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * Type Declarations
 * -------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
 * Class Definitions
 ---------------------------------------------------------------------------*/

/// Buffer manager class
class VencBufferManager
{
public:
   /// Constructor
   VencBufferManager(OMX_ERRORTYPE* pResult);
   /// Destructor
   ~VencBufferManager();
   /// Pops a buffer with the specified timestamp
   OMX_ERRORTYPE PopBuffer(OMX_BUFFERHEADERTYPE** ppBuffer, 
                           OMX_TICKS nTimeStamp);
   /// Pops first buffer in list
   OMX_ERRORTYPE PopFirstBuffer(OMX_BUFFERHEADERTYPE** ppBuffer);
   /// Pushes a buffer in the list
   OMX_ERRORTYPE PushBuffer(OMX_BUFFERHEADERTYPE* pBuffer);
   /// Get the number of buffers in the list
   OMX_ERRORTYPE GetNumBuffers(OMX_U32* pnBuffers);
   
private:
   /// List node
   struct Node
   {
      OMX_BUFFERHEADERTYPE* pBuffer;
      Node* pNext;
   };

private:
   /// Default constructor unallowed
   VencBufferManager() {}

   /// Allocate node from free node pool
   Node* AllocNode();
   
   /// Put node back in the free node pool
   void FreeNode(Node* pNode);

private:

   /// head of list
   Node * m_pHead;

   /// number of buffers in list
   int m_nBuffers;

   // max buffer number assumption
   static const int MAX_FREE_BUFFERS = 10;

   /// free node pool
   Node m_sFreeNodePool[MAX_FREE_BUFFERS];
};

#endif // #ifndef OMX_VENC_DEVICE_CALLBACK_H

/*============================================================================
  FILE: OMX_VencBufferManager.cpp
  
  OVERVIEW: Buffer manager for video encoder omx component.
  
                       Copyright (c) 2008 QUALCOMM Incorporated.
                                All Rights Reserved.
                          Qualcomm Confidential and Proprietary
============================================================================*/

/*============================================================================
  EDIT HISTORY FOR MODULE
  
  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order. Please
  use ISO format for dates.
  
  $Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/omx/src/OMX_VencBufferManager.cpp#2 $$DateTime: 2010/03/22 15:41:26 $$Author: c_ashish $
  
  when        who  what, where, why
  ----------  ---  -----------------------------------------------------------
  2009-03-19  as   Android debug messages
  2009-01-21  as   Android changes merge
  2008-07-15  bc   Buffer management bug fix
  2008-06-10  bc   Created file.
  
============================================================================*/

/*----------------------------------------------------------------------------
 * Include Files
 * -------------------------------------------------------------------------*/
#include "OMX_VencBufferManager.h"
#include "OMX_VencDebug.h"
#include <stdio.h>
#include <string.h>

/*----------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * -------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * Type Declarations
 * -------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * Global Data Definitions
 * -------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * Static Variable Definitions
 * -------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * Static Function Declarations and Definitions
 * -------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * Externalized Function Definitions
 * -------------------------------------------------------------------------*/

VencBufferManager::VencBufferManager(OMX_ERRORTYPE* pResult)
   : m_pHead(NULL),
     m_nBuffers(0)
{
   if (pResult == NULL)
   {
      OMX_VENC_MSG_ERROR("result is null", 0, 0, 0);
      return;
   }
   else
   {
      *pResult = OMX_ErrorNone;
   }

   memset(&m_sFreeNodePool, 0, sizeof(m_sFreeNodePool));
}

VencBufferManager::~VencBufferManager()
{
}

OMX_ERRORTYPE
VencBufferManager::PopBuffer(OMX_BUFFERHEADERTYPE** ppBuffer,
                             OMX_TICKS nTimeStamp)
{
   Node *pSave = NULL;
   Node *pCurr;
   pCurr = m_pHead;

   if (!ppBuffer)
   {
      return OMX_ErrorBadParameter;
   }
   else if (m_pHead == NULL)
   {
      OMX_VENC_MSG_ERROR("list is empty", 0, 0, 0);
      return OMX_ErrorUndefined;
   }
   else
   {
      while (pCurr != NULL && 
             pCurr->pBuffer->nTimeStamp != nTimeStamp)
      {
         pSave = pCurr;
         pCurr = pCurr->pNext;
      }

      if (pCurr && pCurr->pBuffer->nTimeStamp == nTimeStamp)
      {
         if (pCurr == m_pHead)
         {
            *ppBuffer = pCurr->pBuffer;
            pCurr = pCurr->pNext;
            FreeNode(m_pHead);
            m_pHead = pCurr;
         }
         else
         {
            *ppBuffer = pCurr->pBuffer;
            pSave->pNext = pCurr->pNext;
            FreeNode(pCurr);
         }
         --m_nBuffers;

         return OMX_ErrorNone;
      }
      else
      {
         OMX_VENC_MSG_ERROR("buffer with specified time does not exist", 0, 0, 0);
         return OMX_ErrorUndefined;
      }
   }
}

OMX_ERRORTYPE
VencBufferManager::PopFirstBuffer(OMX_BUFFERHEADERTYPE** ppBuffer)
{
   if (!ppBuffer)
   {
      return OMX_ErrorBadParameter;
   }
   else
   {
      if (m_pHead)
      {
         OMX_ERRORTYPE result;
         result = PopBuffer(ppBuffer, m_pHead->pBuffer->nTimeStamp);
         return result;
      }
      OMX_VENC_MSG_ERROR("list is empty", 0, 0, 0);
      return OMX_ErrorUndefined;
   }
}

OMX_ERRORTYPE
VencBufferManager::PushBuffer(OMX_BUFFERHEADERTYPE* pBuffer)
{
   Node *pSave;
   Node *pCurr;
   Node *pTemp;

   pTemp = AllocNode();
   if (!pTemp)
   {
      OMX_VENC_MSG_ERROR("no more buffers to allocate", 0, 0, 0);
      return OMX_ErrorInsufficientResources;
   }
   
   pTemp->pBuffer = pBuffer;
   pTemp->pNext = NULL;

   if (m_pHead == NULL)
   {
      m_pHead = pTemp;
   }
   else
   {
      pCurr = m_pHead;
      while (pCurr != NULL)
      {
         pSave = pCurr;
         pCurr = pCurr->pNext;
      }
      pSave->pNext = pTemp;
   }
   m_nBuffers++;

   return OMX_ErrorNone;
}

OMX_ERRORTYPE
VencBufferManager::GetNumBuffers(OMX_U32* pnBuffers)
{
   if (!pnBuffers)
      return OMX_ErrorBadParameter;

   *pnBuffers = m_nBuffers;
   return OMX_ErrorNone;
}

VencBufferManager::Node* 
VencBufferManager::AllocNode()
{
   for (int i = 0; i < MAX_FREE_BUFFERS; i++)
   {
      if (m_sFreeNodePool[i].pBuffer == NULL)
      {
         return &m_sFreeNodePool[i];
      }
   }
   return NULL;
}

void
VencBufferManager:: FreeNode(Node* pNode)
{
   for (int i = 0; i < MAX_FREE_BUFFERS; i++)
   {
      if (pNode == &m_sFreeNodePool[i])
      {
         pNode->pBuffer = NULL;
         pNode->pNext = NULL;
         return;
      }
   }

   OMX_VENC_MSG_ERROR("invalid buffer", 0, 0, 0);  // will never happen by nature of calling function
}

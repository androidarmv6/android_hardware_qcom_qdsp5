/* =======================================================================
                               qtvsystem.c
DESCRIPTION
   This file provides definitions for the QTV Tasks.

EXTERNALIZED FUNCTIONS
  List functions and a brief description that are exported from this file

INITIALIZATION AND SEQUENCING REQUIREMENTS
  Detail how to initialize and use this service.  The sequencing aspect
  is only needed if the order of operations is important.

Copyright (c) 2007 by QUALCOMM, Incorporated.  All Rights Reserved. 
QUALCOMM Proprietary and Confidential.
========================================================================== */

/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/VdecCommon/main/latest/src/qtvsystem.cpp#1 $
$DateTime: 2008/08/05 12:05:56 $
$Change: 717059 $

========================================================================== */

/* ==========================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */
#include "qtvsystem.h"
#include <stdio.h>

#ifndef PLATFORM_CSIM 
//  #include "OEMFeatures.h"
#endif

#if( defined(FEATURE_QTV_MEMORY_LEAK_DEBUG) || defined (FEATURE_QTV_MEMORY_DEBUG) )
# include "qtv_msg.h"
#endif /* FEATURE_QTV_MEMORY_LEAK_DEBUG */

#ifndef PLATFORM_CSIM
  #ifdef FEATURE_BREW_3_0
  # include "AEE_OEMHeap.h"
  # include "OEMHeap.h"
  #else
//  # include "AEE_OEM.h"
  #endif /* FEATURE_BREW_3_0 */
#endif

/* ==========================================================================

                DEFINITIONS AND DECLARATIONS FOR MODULE

This section contains definitions for constants, macros, types, variables
and other items needed by this module.

========================================================================== */
/* -----------------------------------------------------------------------
** Constant / Define Declarations
** ----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
** Type Declarations
** ----------------------------------------------------------------------- */
#ifdef FEATURE_QTV_MEMORY_LEAK_DEBUG
typedef struct {
  const void *pointer;
  unsigned long int      bytes;
  const char *file;
  unsigned long int      line;
} QtvMallocType;
#endif /* FEATURE_QTV_MEMORY_LEAK_DEBUG */

/* -----------------------------------------------------------------------
** Local Object Definitions
** ----------------------------------------------------------------------- */
#ifdef FEATURE_QTV_MEMORY_LEAK_DEBUG
#define SIZE_OF_MEM_DEBUG_ARRAY 1000
static QtvMallocType qtv_malloc_table[SIZE_OF_MEM_DEBUG_ARRAY] = {0};
/* Current and maximum byte counts can only be kept accurately if free can
 * determine how much memory has been free'd.
 */
static unsigned long int        qtv_malloc_cur_bytes   = 0; 
static unsigned long int        qtv_malloc_max_bytes   = 0;
#endif /* FEATURE_QTV_MEMORY_LEAK_DEBUG */

/* Keep track of the total bytes allocated by Qtv.  This does not take into
 * account qtv_realloc calls unless FEATURE_QTV_MEMORY_LEAK_DEBUG is defined.
 */
static unsigned long int        qtv_malloc_total_bytes = 0; 

/* -----------------------------------------------------------------------
** Forward Declarations
** ----------------------------------------------------------------------- */

/* =======================================================================
**                            Function Definitions
** ======================================================================= */
#ifdef FEATURE_QTV_MEMORY_LEAK_DEBUG
/* ======================================================================
FUNCTION
  QTV_DumpAllocated

DESCRIPTION
   Walk through the array of currently valid allocations.  Report via diag
   a summary of the items, and information about each one.

DEPENDENCIES
  This function depends on the feature FEATURE_QTV_MEMORY_LEAK_DEBUG
  being defined throughout this file.  In particular, the memory tracking
  variables manipulated by alloc and free are traced.

RETURN VALUE
  None

SIDE EFFECTS
  Outputs one QXDM message for the number of allocated items and the total
  allocated size.  Then outputs N indented messages for each still-allocated
  item.
========================================================================== */
void QTV_DumpAllocated(void)
{
  unsigned long int index;

  QTV_MSG_PRIO(QTVDIAG_DEBUG,QTVDIAG_PRIO_DEBUG, "*****************************************" );
  QTV_MSG_PRIO2(QTVDIAG_DEBUG,QTVDIAG_PRIO_DEBUG, "QTV_Dump total=%d, max=%d",
           qtv_malloc_total_bytes, qtv_malloc_max_bytes);

  for( index = 0; index < ARRAY_SIZE(qtv_malloc_table); index++ )
  {
    if(qtv_malloc_table[index].pointer != NULL)
    {
      QTV_MSG_PRIO4(QTVDIAG_DEBUG, QTVDIAG_PRIO_DEBUG,"  [%s:%d] ptr=%x size=%d",
               qtv_malloc_table[index].file, qtv_malloc_table[index].line,
               qtv_malloc_table[index].pointer, qtv_malloc_table[index].bytes);
    }
  }
  QTV_MSG_PRIO(QTVDIAG_DEBUG, QTVDIAG_PRIO_DEBUG, "*****************************************" );
}

/* ======================================================================
FUNCTION 
  qtv_malloc_add_entry

DESCRIPTION
  Helper function that adds a mem alloc entry to the qtv_malloc_table.

DEPENDENCIES
  none

RETURN VALUE
  none

SIDE EFFECTS
  none
  
========================================================================== */
static void qtv_malloc_add_entry(
  const void *pMemory,
  unsigned long int numBytes,
  const char *file,
  unsigned long int line
  )
{
  unsigned long int i;
  /* If memory was allocated, store the information into the mem_pointer_array
   * table.
   */
  if( numBytes != 0 && pMemory != NULL )
  {
    for( i = 0; i < ARRAY_SIZE(qtv_malloc_table); i++ )
    {
      if( qtv_malloc_table[i].pointer == NULL )
      {
        /* Found an empty spot, so store the data */
        qtv_malloc_table[i].pointer = pMemory;
        qtv_malloc_table[i].bytes   = numBytes;
        qtv_malloc_table[i].file    = file;
        qtv_malloc_table[i].line    = line;
        break;
      }
    }
    /* Error checking */
    if( i >= ARRAY_SIZE(qtv_malloc_table) )
    {
      QTV_MSG_PRIO3(QTVDIAG_DEBUG, QTVDIAG_PRIO_DEBUG, "Unable to store malloc info: [%s:%d] %d bytes",
                         file, line, numBytes );
    }

    /* Increment the number of bytes currently allocated */
    qtv_malloc_cur_bytes   += numBytes;
    /* Keep track of the watermark of bytes allocated */
    qtv_malloc_max_bytes    = MAX(qtv_malloc_max_bytes, qtv_malloc_cur_bytes);
  }
}

/* ======================================================================
FUNCTION 
  qtv_malloc_find_entry

DESCRIPTION
  Helper function that finds a mem alloc entry in the qtv_malloc_table given a
  previously added/allocated memory pointer.

DEPENDENCIES
  none

RETURN VALUE
  Pointer to the QtvMallocType structure if element was found.  Returns NULL
  otherwise.
  
SIDE EFFECTS
  none
  
========================================================================== */
static QtvMallocType* qtv_malloc_find_entry(const void *pMemory)
{
  unsigned long int i;

  if( pMemory != NULL )
  {
    /* Search through the table to find the entry corresponding to the pointer */
    for( i = 0; i < ARRAY_SIZE(qtv_malloc_table); i++ )
    {
      if( qtv_malloc_table[i].pointer == pMemory )
      {
        /* Found the entry.  Return it. */
        return &qtv_malloc_table[i];
      }
    }
  }
  
  /* Error reporting */
  return NULL;
}

/* ======================================================================
FUNCTION 
  qtv_malloc_change_entry

DESCRIPTION
  Modifies the entry while maintaining accurate allcoated byte counts.

DEPENDENCIES
  none

RETURN VALUE
  none

SIDE EFFECTS
  none
  
========================================================================== */
static void qtv_malloc_change_entry(
  QtvMallocType *pEntry,
  const void    *pMemory,
  unsigned long int numBytes,
  const char    *file,
  unsigned long int line
  )
{
  /* Decrement the number of bytes previously allocated by Qtv */
  qtv_malloc_cur_bytes -= pEntry->bytes;
    
  /* Change the information at this location */
  pEntry->pointer = pMemory;
  pEntry->bytes   = numBytes;
  pEntry->file    = file;
  pEntry->line    = line;

  /* Increment the number of bytes allocated by Qtv */
  qtv_malloc_total_bytes += numBytes;
  qtv_malloc_cur_bytes   += numBytes;
  /* Keep track of the watermark of bytes allocated */
  qtv_malloc_max_bytes = MAX(qtv_malloc_max_bytes, qtv_malloc_cur_bytes);
}

/* ======================================================================
FUNCTION 
  qtv_malloc_delete_entry

DESCRIPTION
  Helper function that removes a mem alloc entry to the qtv_malloc_table given a
  previously added/allocated memory pointer.  In addition keeps track of total
  allocated byte counts.

DEPENDENCIES
  none

RETURN VALUE
  none

SIDE EFFECTS
  none
  
========================================================================== */
static void qtv_malloc_delete_entry(const void *pMemory)
{
  QtvMallocType *pEntry = qtv_malloc_find_entry(pMemory);

  if( pEntry != NULL )
  {
    /* Decrement the number of bytes allocated by Qtv */
    qtv_malloc_cur_bytes -= pEntry->bytes;

    /* Clear out the old contents.  Must set 'pointer' field to NULL to let it
     * be reused. */
    memset( pEntry, 0, sizeof(*pEntry) );
  }
  else
  {
    QTV_MSG_PRIO1( QTVDIAG_DEBUG, QTVDIAG_PRIO_DEBUG, "Unable to find malloc info on free: ptr=%d", pMemory );
  }
}
#endif /* FEATURE_QTV_MEMORY_LEAK_DEBUG */

/* ======================================================================
FUNCTION 
  Get_Qtv_Env

DESCRIPTION
  This function will be used internally by QTV_* macros to fetch the IEnv* of 
  the Vld thread.
  All the memory allocation/deallocation should happen in the context of this Env.

RETURN VALUE
  IEnv*

========================================================================== */
// IEnv* Get_Qtv_Env()
// {
//   //fetch pEnv with tls_Get
//    int nErr;
//    LSEntry *plse = NULL;
//    ComponentEnv* pCmpEnv = NULL;

//    nErr = tls_Get((unsigned long int)QTV_NEW_KEY, &plse);

//    if (0 == nErr) {
//       pCmpEnv = (ComponentEnv*)plse;
//    }
//    else
//    {
//      ASSERT(0);
//    }
//    return pCmpEnv->pEnv;
// }

/* ======================================================================
FUNCTION 
  qtv_malloc

DESCRIPTION
  Allocate memory from the heap.

  This function should not be called directly from Qtv code.  Instead, call
  QTV_Malloc() To be independent of FEATURE_QTV_MEMORY_DEBUG.

DEPENDENCIES
  none

RETURN VALUE
  Returns a pointer to numBytes of memory or NULL if unable to allocate memory.

SIDE EFFECTS
  modifies tables to track memory leaks
  
========================================================================== */
void* qtv_malloc(unsigned long int numBytes, const char *file, unsigned short line)
{
  void *pMemory;
#if( defined(FEATURE_QTV_MEMORY_DEBUG) || defined(FEATURE_QTV_MEMORY_LEAK_DEBUG) )
  const char *pFile = strrchr( file, '\\' );
  if(pFile == NULL)
  {
    pFile = strrchr(file, '/'); 
  } /* in case of linux, use forward slash */
  if(pFile == NULL) 
  { // if still null, print error msg, page fault about to ensue
    pFile = file; 
  }
  else
  {
    pFile++; 
  } /* Just past the last '\' */
#endif /* FEATURE_QTV_MEMORY_DEBUG || FEATURE_QTV_MEMORY_LEAK_DEBUG */
  
#ifdef T_WINNT
  //INTLOCK();
#endif

  /* Allocate the memory from the heap */
  pMemory = (void*) new unsigned char[numBytes]; //malloc(numBytes);
//   (void) Get_Qtv_Env()->ErrMalloc(numBytes,&pMemory);

#ifdef T_WINNT
  //INTFREE();
#endif

  if( pMemory != NULL )
  {
#ifdef FEATURE_QTV_MEMORY_LEAK_DEBUG
    /* Leak tracking */
    qtv_malloc_add_entry( pMemory, numBytes, pFile, line );
#endif /* FEATURE_QTV_MEMORY_LEAK_DEBUG */

#ifdef FEATURE_QTV_MEMORY_DEBUG
    QTV_MSG_PRIO4( QTVDIAG_DEBUG,QTVDIAG_PRIO_DEBUG, "[%s:%d] qtv_malloc(%d), %x",
                       pFile, line, numBytes, pMemory );
#endif /* FEATURE_QTV_MEMORY_DEBUG */
    
    /* Increment the number of bytes allocated by Qtv */
    qtv_malloc_total_bytes += numBytes;
  }
  
  return(pMemory);
}


/* ======================================================================
FUNCTION 
  qtv_free

DESCRIPTION
  Free memory back to the heap to be reused later.

  This function should not be called directly from Qtv code.  Instead, call
  QTV_Free() To be independent of FEATURE_QTV_MEMORY_DEBUG.

DEPENDENCIES
  This memory must have been allocated by a call to QTV_Malloc.

RETURN VALUE
  none

SIDE EFFECTS
  modifies tables to track memory leaks
  
========================================================================== */
void qtv_free(void* pMemory, const char* file, unsigned short line)
{
#ifdef FEATURE_QTV_MEMORY_DEBUG
  const char *pFile = strrchr( file, '\\' );
  if(pFile == NULL) 
  {
    pFile = strrchr(file, '/'); 
  } /* in case of linux, use forward slash */
  if(pFile == NULL) 
  { // if still null, print error msg, page fault about to ensue
    pFile = file;
  }
  else 
  {
    pFile++; 
  } /* Just past the last '\' */
#endif /* FEATURE_QTV_MEMORY_DEBUG */

#ifdef FEATURE_QTV_MEMORY_LEAK_DEBUG
  /* Leak tracking */
  qtv_malloc_delete_entry( pMemory );
#endif /* FEATURE_QTV_MEMORY_LEAK_DEBUG */

#ifdef T_WINNT
 //INTLOCK();
#endif

  /* Free the memory */

  delete ((int*)pMemory);
  //  (void) Get_Qtv_Env()->Free(pMemory);

#ifdef T_WINNT
 //INTFREE();
#endif

#ifdef FEATURE_QTV_MEMORY_DEBUG
  QTV_MSG_PRIO3( QTVDIAG_DEBUG,QTVDIAG_PRIO_DEBUG, "[%s:%d] qtv_free(%x)", pFile, line, pMemory );/*lint !e449 */
#endif /* FEATURE_QTV_MEMORY_DEBUG */
}

/* ======================================================================
FUNCTION 
  qtv_new

DESCRIPTION
  Allocate memory from the heap.  This function replaces the standard C++ 'new'
  operator in order to track memory usage by Qtv.

  This function should not be called directly from Qtv code.  Instead, call
  QTV_New() To be independent of FEATURE_QTV_MEMORY_DEBUG.

DEPENDENCIES
  This memory must have been allocated by a call to QTV_Malloc.

RETURN VALUE
  none

SIDE EFFECTS
  modifies tables to track memory leaks
  
========================================================================== */
void *qtv_new(void* pMemory, unsigned long int numBytes, const char *file, unsigned short line)
{
#if( defined(FEATURE_QTV_MEMORY_DEBUG) || defined(FEATURE_QTV_MEMORY_LEAK_DEBUG) )
  const char *pFile = strrchr( file, '\\' );
  if(pFile == NULL) 
  {
    pFile = strrchr(file, '/'); 
  } /* in case of linux, use forward slash */
  if(pFile == NULL)
  { // if still null, print error msg, page fault about to ensue
    pFile = file;
  }
  else 
  {
    pFile++; 
  } /* Just past the last '\' */
#endif /* FEATURE_QTV_MEMORY_DEBUG || FEATURE_QTV_MEMORY_LEAK_DEBUG */

  if( pMemory != NULL )
  {
#ifdef FEATURE_QTV_MEMORY_LEAK_DEBUG
    /* Leak tracking */
    qtv_malloc_add_entry( pMemory, numBytes, pFile, line );
#endif /* FEATURE_QTV_MEMORY_LEAK_DEBUG */

#ifdef FEATURE_QTV_MEMORY_DEBUG
    QTV_MSG_PRIO4( QTVDIAG_DEBUG,QTVDIAG_PRIO_DEBUG, "[%s:%d] qtv_new(%d), %x",
                       pFile, line, numBytes, pMemory );
#endif /* FEATURE_QTV_MEMORY_DEBUG */

    /* Increment the number of bytes allocated by Qtv */
    qtv_malloc_total_bytes += numBytes;
  }
    
  return(pMemory);
}

/* ======================================================================
FUNCTION 
  qtv_delete

DESCRIPTION
  Free memory back to the heap to be reused later.  This function assumes that
  the memory has already been deleted.  This function just keeps track of
  information for debugging memory errors.

  This function should not be called directly from Qtv code.  Instead, call
  QTV_Delete() to be independent of FEATURE_QTV_MEMORY_DEBUG.

DEPENDENCIES
  This memory must have been allocated by a call to QTV_Malloc.

RETURN VALUE
  none

SIDE EFFECTS
  modifies tables to track memory leaks
  
========================================================================== */
void qtv_delete(void* pMemory, const char *file, unsigned short line)
{
#ifdef FEATURE_QTV_MEMORY_DEBUG
  const char *pFile = strrchr( file, '\\' );
  if(pFile == NULL)
  {
    pFile = strrchr(file, '/');
  } /* in case of linux, use forward slash */
  if(pFile == NULL)
  { // if still null, print error msg, page fault about to ensue
    pFile = file;
  }
  else
  {
    pFile++;
  } /* Just past the last '\' */
#endif /* FEATURE_QTV_MEMORY_DEBUG */

#ifdef FEATURE_QTV_MEMORY_LEAK_DEBUG
  /* Leak tracking */
  qtv_malloc_delete_entry( pMemory );
#endif /* FEATURE_QTV_MEMORY_LEAK_DEBUG */

#ifdef FEATURE_QTV_MEMORY_DEBUG
  QTV_MSG_PRIO3( QTVDIAG_DEBUG, QTVDIAG_PRIO_DEBUG, "[%s:%d] qtv_delete(%x)", pFile, line, pMemory );
#endif /* FEATURE_QTV_MEMORY_DEBUG */
}




#ifndef QTVSYSTEM_H
#define QTVSYSTEM_H
/* =======================================================================
                               qtvsystem.h
DESCRIPTION
   This file provides definitions for the QTV Tasks.

Copyright (c) 2007 by QUALCOMM, Incorporated.  All Rights Reserved. 
QUALCOMM Proprietary and Confidential.
========================================================================== */

/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/VdecCommon/main/latest/src/qtvsystem.h#2 $
$DateTime: 2008/10/30 11:58:22 $
$Change: 773671 $


========================================================================== */

/* =======================================================================
**               Includes and Public Data Declarations
** ======================================================================= */

#define QTV_NEW_KEY  ( 0x0104289f )
//used for thread local storage of the components env ptr.
/* typedef struct { */
/*   LSEntry    lse; */
/*   IEnv*   pEnv; */
/* }ComponentEnv; */

/* ==========================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */
#ifdef __cplusplus
extern "C"
{
#endif

/* ==========================================================================

                        DATA DECLARATIONS

========================================================================== */
/* -----------------------------------------------------------------------
** Constant / Define Declarations
** ----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
** Type Declarations
** ----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
** Global Constant Data Declarations
** ----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
** Global Data Declarations
** ----------------------------------------------------------------------- */

/* =======================================================================
**                          Macro Definitions
** ======================================================================= */

/*************************************************************************
 * The following macro definitions are the only dynamic memory allocation
 * routines that are to be used in Qtv.
 ************************************************************************* */
  
/* =======================================================================
MACRO QTV_Malloc

ARGS 
  n - the number of bytes to allocate

DESCRIPTION:
  This macro calls qtv_malloc in order to allocate memory from the heap and
  track memory allocations for debugging purposes.  A pointer to the allocated
  memory is returned.
========================================================================== */
#define QTV_Malloc(n)       qtv_malloc((n), __FILE__, __LINE__)

/* =======================================================================
MACRO QTV_Free

ARGS 
  p - pointer to memory which will be free'd

DESCRIPTION:
  This macro calls qtv_free in order to free memory from the heap and
  track memory allocations for debugging purposes.
========================================================================== */
#define QTV_Free(p)         qtv_free((p), __FILE__, __LINE__)

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
void* qtv_malloc(unsigned long int numBytes, const char *file, unsigned short line);

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
void  qtv_free(void* pMemory, const char *file, unsigned short line);

#ifdef __cplusplus
}
#endif /* __cplusplus */

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
/* IEnv* Get_Qtv_Env(); */

/* =======================================================================
MACRO QTV_New

ARGS 
  t - type of object to allocate and construct

DESCRIPTION:
  This macro calls the C++ 'new' operator in order to allocate and construct an
  object of the requested type.  Then qtv_new is called in order to track memory
  allocations for debugging purposes.  A pointer to the newly constructed object
  is returned.
========================================================================== */
#define QTV_New(t)   ((t*)qtv_new(new  t, sizeof(t), __FILE__, __LINE__))  
  
/* =======================================================================
MACRO QTV_New_Args

ARGS 
  t - type of object to allocate and construct
  a - list of arguments to pass to constructor.  MUST BE SURROUNDED BY
      PARENTHESES.

DESCRIPTION:
  This macro calls the C++ 'new' operator in order to allocate and construct an
  object of the requested type passing the arguments specified.  Then qtv_new is
  called in order to track memory allocations for debugging purposes.  A pointer
  to the newly constructed object is returned.
========================================================================== */
#define QTV_New_Args(t,a)   ((t*)qtv_new(new t a, sizeof(t), __FILE__, __LINE__))

/* =======================================================================
MACRO QTV_Delete

ARGS 
  p - pointer to object which will be destructed and free'd

DESCRIPTION:
  This macro calls the C++ 'delete' operator in order to deconstruct and free
  the specified object.  Then qtv_delete is called in order to track memory
  allocations for debugging purposes.
========================================================================== */
#define QTV_Delete(p)       delete(p); qtv_delete((p), __FILE__, __LINE__)

/* =======================================================================
MACRO MYOBJ

ARGS
  xx_obj - this is the xx argument

DESCRIPTION:
  Complete description of what this macro does
========================================================================== */

/* =======================================================================
**                        Function Declarations
** ======================================================================= */


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
void *qtv_new(void* pMemory, unsigned long int numBytes, const char *file, unsigned short line);

/* ======================================================================
FUNCTION 
  qtv_delete

DESCRIPTION
  Free memory back to the heap to be reused later.  This function replaces the
  standard C++ 'delete' operator in order to track memory usage by Qtv.

  This function should not be called directly from Qtv code.  Instead, call
  QTV_Delete() to be independent of FEATURE_QTV_MEMORY_DEBUG.

DEPENDENCIES
  This memory must have been allocated by a call to QTV_Malloc.

RETURN VALUE
  none

SIDE EFFECTS
  modifies tables to track memory leaks
  
========================================================================== */
void qtv_delete(void* pMemory, const char *file, unsigned short line);
  

#endif /* QTVSYSTEM_H */

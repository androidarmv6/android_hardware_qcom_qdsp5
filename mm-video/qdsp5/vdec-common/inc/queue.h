#ifndef QUEUE_H
#define QUEUE_H

/*===========================================================================

            Q U E U E    S E R V I C E S    H E A D E R    F I L E

DESCRIPTION
 This file contains types and declarations associated with the Queue
 Services.

Copyright (c) 2007 by QUALCOMM, Incorporated.  All Rights Reserved. 
QUALCOMM Proprietary and Confidential.
===========================================================================*/


/*===========================================================================

                      EDIT HISTORY FOR FILE

This section contains comments describing changes made to this file.
Notice that changes are listed in reverse chronological order.

$Header: //source/qcom/qct/multimedia2/Video/Dec/VdecCommon/main/latest/src/queue.h#4 $
   
when       who     what, where, why
--------   ---     ----------------------------------------------------------
01/30/03   ifk     Removed FEATUREization to allow for uniform binary 
                   generation, removed q_delete().  Features
                   FEATURE_Q_NO_SELF_QPTR and FEATURE_Q_SINGLE_LINK are
                   defined here until the switchover to the new queue is
                   complete at which point these should be removed.
05/16/02    gr     Added macro Q_ALREADY_QUEUED.
09/28/01    gr     Merged in changes from the MSP archive. Includes
                   queue-specific mutual exclusion for Win32 targets and the
                   new function q_linear_delete. Removed support for UTF_ASP.
01/25/01   day     Merged from MSM5105_COMMON.00.00.05.
                   Added support for MSM5000_IRAM_FWD.
                   Added support for UTF_ASP.
01/13/00   jct     Add q_linear_search
12/10/99   gr      Merged from the MSM3000 Rel 4.0c.
                   Re-arranged data in link structures to fix bugs that were
                   occurring when FEATURE_Q_SINGLE_LINK was defined but
                   FEATURE_Q_NO_SELF_QPTR was not.
                   Introduced q_last_get().
                   Added extern "C" to allow this file to export the queue
                   routines to a C++ program.  Changed use of word to int to 
                   count the number of elements in a queue.  Upgraded comments
                   to include function headers.  Removed the inclusion of
                   comdef.h since it is no longer needed.
                   Added macros to support renaming of queue functions and
                   to intercept calls to queue functions. 
04/09/99    sk     Comments changed.
04/06/99    sk     Introduced FEATURE_Q_NO_SELF_QPTR and FEATURE_Q_SINGLE_LINK
12/16/98   jct     Removed 80186 support
09/24/98   jct     Updated copyright
12/20/96   rdh     Changed to Pascal call convention.
02/08/96   jca     Added missing #include.
04/22/92   ip      Initial porting of file from Brassboard to DMSS.
02/20/92   arh     Added new function q_check.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/

#include <pthread.h>
#include "qtv_msg.h"

/*===========================================================================

                        DATA DECLARATIONS

===========================================================================*/
/*---------------------------------------------------------------------------
  Queue Link Structure

  The following link structure is really the heart of the Queue Services.
  It is used as a link field with variables which allows them to be moved
  on and off queues. It is also used in the definition of queues themselves.

  Do NOT directly access the fields of a link! Only the Queue Services
  should do this.
---------------------------------------------------------------------------*/
typedef struct q_link_struct
{
  struct q_link_struct  *next_link_ptr;       /* Ptr to next link in list. */
} q_link_type;

/*---------------------------------------------------------------------------
  Queue Structure

  The following structure is used by the Queue Services to represent a
  queue.
 
  Do NOT access the fields of a queue directly. Only the Queue Services
  should do this.

  The head_link member should be the first item in the structure.
---------------------------------------------------------------------------*/
typedef struct q_struct
{
  q_link_type  head_link;              /* Head link, points to first item  */
  q_link_type *tail_link_ptr;          /* Pointer to last item             */
  int          link_cnt;               /* Number of items enqueued         */
/*   ICritSect*   pCS;                    /\* To protect the queue *\/ */
  pthread_mutex_t qLock;
} q_type;


/*===========================================================================

                             Macro Definitions

===========================================================================*/
/*===========================================================================

MACRO Q_ALREADY_QUEUED

DESCRIPTION
   Evaluates to true if the item passed in is already in a queue and to
   false otherwise.

===========================================================================*/
#define Q_ALREADY_QUEUED( q_link_ptr )                                      \
                                      ((q_link_ptr)->next_link_ptr != NULL)


#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

#define q_init          vdec_q_init
#define q_destroy       vdec_q_destroy
#define q_link          vdec_q_link
#define q_put           vdec_q_put
#define q_get           vdec_q_get
#define q_last_get      vdec_q_last_get
#define q_next          vdec_q_next
#define q_insert        vdec_q_insert
#define q_delete        vdec_q_delete   
#define q_check         vdec_q_check
#define q_last_check    vdec_q_last_check
#define q_cnt           vdec_q_cnt
#define q_linear_search vdec_q_linear_search
/*===========================================================================

                       Function Pointer Definitions

===========================================================================*/
/*===========================================================================

FUNCTION TYPEDEF Q_COMPARE_FUNC_TYPE

DESCRIPTION
  Used by the searching functions to determine if an item is in the queue.

DEPENDENCIES
  None

RETURN VALUE
  Returns non zero if the element should be operated upon, 0 otherwise

SIDE EFFECTS
  None

===========================================================================*/
typedef int (*q_compare_func_type)
(
  void *item_ptr,
  void *compare_val
);


/*===========================================================================

                            Function Declarations

===========================================================================*/
/*===========================================================================

FUNCTION Q_INIT

DESCRIPTION
  This function initializes a specified queue. It should be called for each
  queue prior to using it.

DEPENDENCIES
  None.

RETURN VALUE
  A pointer to the initialized queue.

SIDE EFFECTS
  The specified queue is initialized for use with Queue Services.

===========================================================================*/
extern q_type *q_init
(
 q_type *q_ptr//,                  /* Queue to be initialized.              */
  //  IEnv* pEnv
);

/*===========================================================================

FUNCTION Q_DESTROY

DESCRIPTION
  This function destroys/releases a specified queue's internal data structures. 
  It should be called for each queue after using it.

DEPENDENCIES
  None.

RETURN VALUE
  A pointer to the initialized queue.

SIDE EFFECTS
  The specified queue is initialized for use with Queue Services.

===========================================================================*/
extern void *q_destroy
(
  q_type *q_ptr                   /* Queue to be destroy.              */
);

/*===========================================================================

FUNCTION Q_LINK

DESCRIPTION
  This function initializes a specified link. It should be called for each
  link prior to using it.

DEPENDENCIES
  None.

RETURN VALUE
  A pointer to the initialized link.

SIDE EFFECTS
  The specified link is initialized for use with the Queue Services.

===========================================================================*/
extern q_link_type *q_link
(
  void        *item_ptr,          /* Pointer to item containing link       */
  q_link_type *link_ptr           /* Pointer to link field wihtin the item */
);


/*===========================================================================

FUNCTION Q_PUT

DESCRIPTION
  This function enqueues an item onto a specified queue using a specified
  link.  The item is enqueued at the end of the queue.

DEPENDENCIES
  The specified queue should have been previously initialized via a call
  to q_init. The specified link field of the item should have been prev-
  iously initialized via a call to q_init_link.

RETURN VALUE
  None.

SIDE EFFECTS
  The specified item is placed at the tail of the specified queue.

===========================================================================*/
extern void q_put
(
  q_type      *q_ptr,             /* Queue on which item is to be queued.  */
  q_link_type *link_ptr           /* Link to use for queueing item.        */
);


/*===========================================================================

FUNCTION Q_GET

DESCRIPTION
  This function removes an item from the head of a specified queue and
  returns it's link.

DEPENDENCIES
  The specified queue should have been initialized previously via a call
  to q_init.

RETURN VALUE
  A pointer to the first item's link. If the specified queue is empty, then
  NULL is returned.

SIDE EFFECTS
  The head item, if any, is removed from the specified queue.

===========================================================================*/
extern void *q_get
(
  q_type *q_ptr                   /* Queue from which the item is returned */
);


/*===========================================================================

FUNCTION Q_LAST_GET

DESCRIPTION
  This function removes an item from the tail of a specified queue and
  returns it's link.

DEPENDENCIES
  The specified queue should have been initialized previously via a call
  to q_init.

RETURN VALUE
  The link of the last item in the queue.  A NULL is returned if there is no
  item on the queue.

SIDE EFFECTS
  The tail item is removed from the specified queue.

===========================================================================*/
extern void *q_last_get
(
  q_type *q_ptr                   /* Queue from which the item is returned */
);


/*===========================================================================

FUNCTION Q_NEXT

DESCRIPTION
  This function returns a pointer to the item link which comes after the
  specified item on the specified queue.

DEPENDENCIES
  The specified queue should have been initialized previously via a call to
  q_init.  The specified link should have been acquired from a previous call
  to q_check/q_next.

RETURN VALUE
  A pointer to the next item on the queue. If the end of the queue is reached
  then NULL is returned.

SIDE EFFECTS
  Returns a pointer to an item in the queue without de-queuing it.

===========================================================================*/
extern void *q_next
(
  q_type      *q_ptr,             /* Queue from which item is returned     */
  q_link_type *link_ptr           /* Link whose next link is required      */
);


/*===========================================================================

FUNCTION Q_INSERT

DESCRIPTION
  This function inserts a specified item insert_link_ptr) before another
  specified item (next_link_ptr) on a specified queue (q_ptr).

DEPENDENCIES
  The specified queue should have been initialized previously via a call
  to q_init; insert_link_ptr and next_link_ptr should have been initialized
  via calls to q_link.

RETURN VALUE
  None.

SIDE EFFECTS
  insert_link_ptr's associated item is inserted before next_link_ptr's item.

===========================================================================*/
extern void q_insert
(
  q_type      *q_ptr,
  q_link_type *insert_link_ptr,
  q_link_type *next_link_ptr
);


/*===========================================================================

FUNCTION Q_DELETE

DESCRIPTION
  This function removes a specified item from a specified queue.

DEPENDENCIES
  The specified queue should have been initialized previously via a call
  to q_init. The specified link should have been initialized via call to
  q_link.

RETURN VALUE
  None.

SIDE EFFECTS
  Input item is deleted from the queue.

===========================================================================*/
extern void q_delete
(
  q_type       *q_ptr,            /* Pointer to the queue                  */
  q_link_type  *delete_link_ptr   /* Pointer to link to be removed from q  */
);


/*===========================================================================

FUNCTION Q_CHECK

DESCRIPTION
  This function returns a pointer to the link of the item at the head of the
  queue.  The item is not removed from the queue.

DEPENDENCIES
  The specified queue should have been initialized previously via a call
  to q_init.

RETURN VALUE
  A pointer to the first queued item's link. If the specified queue is
  empty, then NULL is returned.

SIDE EFFECTS
  Pointer to link of an item in the queue is returned without de-queuing
  the item.

===========================================================================*/
extern void *q_check
(
  q_type *q_ptr                   /* Pointer to a queue                    */
);


/*===========================================================================

FUNCTION Q_LAST_CHECK

DESCRIPTION
  This function returns a pointer to the link of the item at the tail of the
  specified queue.  The item is not removed from the queue.

DEPENDENCIES
  The specified queue should have been initialized previously via a call
  to q_init.

RETURN VALUE
  The link of the last item in the queue, or NULL if there are no items on
  the queue.

SIDE EFFECTS
  Pointer to link of an item on the queue is returned without de-queuing
  the item.

===========================================================================*/
extern void *q_last_check
(
  q_type *q_ptr                   /* Queue from which the item is returned */
);


/*===========================================================================

FUNCTION Q_CNT

DESCRIPTION
  This function returns the number of items currently queued on a specified
  queue.

DEPENDENCIES
  The specified queue should have been initialized previously via a call
  to q_init.

RETURN VALUE
  The number of items currently queued on the specified queue.

SIDE EFFECTS
  None.

===========================================================================*/
extern int q_cnt
(
  q_type  *q_ptr                  /* Queue whose item count is required    */
);


/*===========================================================================

FUNCTION Q_LINEAR_SEARCH

DESCRIPTION
  Given a comparison function, this function traverses the elements in a
  queue, calls the compare function on each item, and returns a pointer to
  the current element's link if the user passed compare function returns
  non-zero on that element.

  The user compare function should return 0 if the current element is not
  the element in which the compare function is interested.

  The user compare function SHOULD NOT dequeue a node.

DEPENDENCIES
  The specified queue should have been initialized previously via a call to
  q_init.

RETURN VALUE
  A pointer to the found element's queue link.

SIDE EFFECTS
  A pointer to a linked element's link is returned without de-queuing the
  item.

===========================================================================*/
extern void *q_linear_search
(
  q_type              *q_ptr,          /* Queue to be traversed            */
  q_compare_func_type  compare_func,   /* Compare function to be called    */
  void                *compare_val     /* Cookie to pass to compare_func   */
);

#ifdef __cplusplus
   }
#endif /* __cplusplus */

#endif /* QUEUE_H */

/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        Q U E U E    S E R V I C E S

GENERAL DESCRIPTION

  A queue is a simple data structure used for logically storing and re-
  trieving data blocks, in a first in - first out (FIFO) order without
  physically copying them. Software tasks and interrupt handlers can use
  queues for cleanly transferring data blocks to one another.

                +-------+     +-------+     +-------+
                | DATA  |     | DATA  |     | DATA  |
                | BLOCK |     | BLOCK |     | BLOCK |
  +-------+     +-------+     +-------+     +-------+
  | QUEUE |---->| LINK  |---->| LINK  |---->| LINK  |---->NULL
  +-------+     +-------+     +-------+     +-------+
  |       |     |       |     |       |     |       |
  +-------+     |       |     |       |     |       |
                +-------+     +-------+     +-------+


  The Queue Services provide a small set of declarations and functions for
  defining and initializing a queue, defining and initializing links with-
  in a data block, placing a data block at the tail of a queue, removing a
  data block from the head of a queue, and removing a data block from any
  position in a queue.

  Aside from requiring each data block to contain a link, the Queue Services
  impose no restrictions on the size of structure of data blocks used with
  with queues. This allows software to pass virtually any type of data on
  queues. Notice that a data block may contain multiple links allowing it to
  be placed simultaneously on multiple queues.


EXTERNALIZED FUNCTIONS

  q_init
    This function initializes a queue. It should be called on behalf of a
    queue prior to using the queue.

  q_link
    This function initializes a link field. It should be called on behalf
    of a link field prior to using the link filed with the other Queue
    Services.

  q_get
    This function removes the data block at head of a queue and returns a
    pointer to the data block's link field. If the queue is empty, then a
    NULL pointer is returned.

  q_check
    This function returns a pointer to the data block's link field at the
    head of a queue.  The data block is not removed from the queue. If the
    queue is empty, then a NULL pointer is returned.

  q_last_get
    This function removes the data block at the tail of a queue and returns a
    pointer to the data block's link field. If the queue is empty, then a
    NULL pointer is returned.

  q_put
    This function places a data block at the tail of a queue.

  q_cnt
    This function returns the number of items currently on a queue.

  q_linear_search
    Performs a linear search of the queue calling a user callback on each
    nodal visit to determine whether or not the search should be terminated.
    The user callback SHOULD NOT dequeue a node.

INITIALIZATION AND SEQUENCING REQUIREMENTS

  Prior to use, a queue must be initialized by calling q_init. Similarly,
  a link must be initialized prior to use by calling q_link.

Copyright (c) 2007 by QUALCOMM, Incorporated.  All Rights Reserved. 
QUALCOMM Proprietary and Confidential.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================
                              Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/VdecCommon/main/latest/src/queue.c#1 $
   
when       who     what, where, why
--------   ---     ----------------------------------------------------------
01/30/03   ifk     Removed FEATURIZATION to allow for uniform binary
                   generation.  Removed q_linear_delete(), added q_prev().
                   Added ASSERTs to check arguments and fixed some bugs.
                   The extra ASSERTs can be turned off by defining
                   FEATURE_QUEUE_NO_STRICT_CHECK.
09/28/01    gr     Merged in changes from the MSP archive. Includes clean-up
                   of source code and comments, queue-specific mutual
                   exclusion for Win32 targets and the new function
                   q_linear_delete. Removed support for UTF_ASP.
01/25/01   day     Merged from MSM5105_COMMON.00.00.05.
                   Added support for MSM5000_IRAM_FWD.
                   Added support for UTF_ASP.
01/13/00   jct     Add q_linear_search
12/10/99   gr      Merged from MSM3000 Rel 4.0c.
                   Fixes to q_insert, q_delete.
                   Introduced q_last_get().
                   Changed type of queue count from word to int.
                   Added calls to Q_XCEPT macros in all queue functions.
04/09/99    sk     Comments changed.
04/06/99    sk     Introduced FEATURE_Q_NO_SELF_QPTR and
                   FEATURE_Q_SINGLE_LINK
12/16/98   jct     Removed 80186 support
10/08/98   jct     Moved INTLOCK in q_put to beginning of function, added
                   setting link pointer to NULL in q_delete
09/24/98   jct     Update copyright
12/20/96   rdh     Removed from Boot Block, changed to Pascal call convention.
04/08/96   dna     Added comment to each func. saying it is in the Boot Block
02/28/96   dna     Prepended bb_ to all function names and put module in the
                   Boot Block.
04/23/92   ip      Initial porting of file from Brassboard to DMSS.
02/20/92   arh     Added q_check routine

===========================================================================*/
/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "queue.h"
#include "assert.h"

/*===========================================================================

                DEFINITIONS AND DECLARATIONS FOR MODULE

===========================================================================*/
#define Q_LOCK( q ) (void) pthread_mutex_lock(&q->qLock); //ICritSect_Enter(q->pCS);
#define Q_FREE( q ) (void) pthread_mutex_unlock(&q->qLock); // ICritSect_Leave(q->pCS);

#define QUEUE_CHECK_INIT( q_ptr )                                           \
  QUEUE_ASSERT( NULL != (q_ptr)->head_link.next_link_ptr );                 \
  QUEUE_ASSERT( NULL != (q_ptr)->tail_link_ptr );                           \
  QUEUE_ASSERT( NULL != (q_ptr)->tail_link_ptr->next_link_ptr );

#define QUEUE_ASSERT(cond)

/*===========================================================================

                            FORWARD DECLARATIONS

===========================================================================*/
static q_link_type *q_prev
(
  q_type      *q_ptr,             /* The queue on which link_ptr is queued */
  q_link_type *link_ptr           /* Item whose predecessor is required    */
);


/*===========================================================================

                           EXTERNALIZED FUNTIONS

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
q_type *q_init
(
 q_type *q_ptr//,                   /* Queue to be initialized.              */
  //  IEnv* pEnv
)
{
  int result = 0;
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  /*-------------------------------------------------------------------------
    Verify parameters
  -------------------------------------------------------------------------*/
  ASSERT( NULL != q_ptr );

  /*-------------------------------------------------------------------------
    Set the queue's head_link and tail_link_ptr to point to &head_link.
    Set the count to zero.
  -------------------------------------------------------------------------*/
  q_ptr->head_link.next_link_ptr = q_ptr->tail_link_ptr = &q_ptr->head_link;
  q_ptr->link_cnt = 0;

  //  q_ptr->pCS = NULL;
  //Call the equivalent c mapping functions
/*   result = IEnv_CreateInstance( pEnv,  */
/*                                 AEECLSID_CritSect,  */
/*                                 (void**)&q_ptr->pCS); */
  result = pthread_mutex_init(&q_ptr->qLock,NULL);
  ASSERT (0 == result);
  //  ASSERT (NULL != q_ptr->pCS);

  return q_ptr;
} /* q_init() */

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
void *q_destroy
(
  q_type *q_ptr                   /* Queue to be destroy.              */
)
{
/*   ICritSect_Release(q_ptr->pCS); */
//  q_ptr->pCS = NULL;
   (void) pthread_mutex_destroy(&q_ptr->qLock);
   return q_ptr;
} /* q_destroy*/

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
q_link_type* q_link
(

  void        *item_ptr,          /* Pointer to item containing link       */
  q_link_type *link_ptr           /* Pointer to link field within the item */
)
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  /*-------------------------------------------------------------------------
    Verify parameters
  -------------------------------------------------------------------------*/
  ASSERT( NULL != link_ptr );

  /*-------------------------------------------------------------------------
    Set the link's next_link_ptr to NULL
  -------------------------------------------------------------------------*/
  link_ptr->next_link_ptr = NULL;

  return link_ptr;
} /* q_link() */


/*===========================================================================

FUNCTION Q_PUT

DESCRIPTION
  This function enqueues an item onto a specified queue using a specified
  link.  The item is enqueued at the end of the queue.

DEPENDENCIES
  The specified queue should have been previously initialized via a call
  to q_init. The specified link field of the item should have been prev-
  iously initialized via a call to q_link.

RETURN VALUE
  None.

SIDE EFFECTS
  The specified item is placed at the tail of the specified queue.

===========================================================================*/
void q_put
(
  q_type      *q_ptr,             /* Queue on which item is to be queued.  */
  q_link_type *link_ptr           /* Link to use for queueing item.        */
)
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  /*-------------------------------------------------------------------------
    Verify parameters
  -------------------------------------------------------------------------*/
  ASSERT( NULL != q_ptr );
  ASSERT( NULL != link_ptr );

  /*-------------------------------------------------------------------------
    Check that the queue is initialized.  Works if
    FEATURE_QUEUE_NO_STRICT_CHECK is turned off.
  -------------------------------------------------------------------------*/
  QUEUE_CHECK_INIT( q_ptr );

  Q_LOCK( q_ptr );

  /*-------------------------------------------------------------------------
    link_ptr is made to point to the queue's head_link, the queue's
    tail_link_ptr and the last link are made to point to link_ptr which then
    becomes the last link on the queue.  Increment count of items on queue.
  -------------------------------------------------------------------------*/
  link_ptr->next_link_ptr = &q_ptr->head_link;
  q_ptr->tail_link_ptr    = q_ptr->tail_link_ptr->next_link_ptr = link_ptr;
  q_ptr->link_cnt++;

  Q_FREE( q_ptr );

  return;
} /* q_put() */

/*lint -e636
 * ptr to strong type 'q_link_type' versus another type
 * Lint is not smart enough to think that struct q_link_struct == q_link_type
 */
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
void *q_get
(
  q_type *q_ptr                   /* Queue from which the item is returned */
)
{
  q_link_type  *link_ptr = NULL;  /* The link to be returned               */
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  /*-------------------------------------------------------------------------
    Verify parameters
  -------------------------------------------------------------------------*/
  ASSERT( NULL != q_ptr );

  /*-------------------------------------------------------------------------
    Check that the queue is initialized.  Works if
    FEATURE_QUEUE_NO_STRICT_CHECK is turned off.
  -------------------------------------------------------------------------*/
  QUEUE_CHECK_INIT( q_ptr );

  Q_LOCK( q_ptr );

  /*-------------------------------------------------------------------------
    Can only get an item if the queue is non empty
  -------------------------------------------------------------------------*/
  if( q_ptr->head_link.next_link_ptr != &q_ptr->head_link )
  {
    /*-----------------------------------------------------------------------
      Get the first queued item.  Make queue's head_link point to the link
      this item is pointing to.
    -----------------------------------------------------------------------*/
    link_ptr                       = q_ptr->head_link.next_link_ptr;
    q_ptr->head_link.next_link_ptr = link_ptr->next_link_ptr;

    /*-----------------------------------------------------------------------
      If this is the only item then adjust queue's tail_link_ptr.
    -----------------------------------------------------------------------*/
    if( link_ptr == q_ptr->tail_link_ptr )
    {
      q_ptr->tail_link_ptr = link_ptr->next_link_ptr;
    }

    /*-----------------------------------------------------------------------
      Decrement queue count.  Mark item as no longer in the queue.
    -----------------------------------------------------------------------*/
    q_ptr->link_cnt--; 
    link_ptr->next_link_ptr = NULL;  
  }

  Q_FREE( q_ptr );

  return (void *)link_ptr;
} /* q_get() */


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
void *q_last_get
(
  q_type *q_ptr                   /* Queue from which the item is returned */
)
{
  q_link_type  *prev_link_ptr = NULL;  /* Predecessor to the last link     */
  q_link_type  *ret_link_ptr  = NULL;  /* Link to be returned              */
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  /*-------------------------------------------------------------------------
    Verify parameters
  -------------------------------------------------------------------------*/
  ASSERT( NULL != q_ptr );
 
  /*-------------------------------------------------------------------------
    Check that the queue is initialized.  Works if
    FEATURE_QUEUE_NO_STRICT_CHECK is turned off.
  -------------------------------------------------------------------------*/
  QUEUE_CHECK_INIT( q_ptr );

  Q_LOCK( q_ptr );

  /*-------------------------------------------------------------------------
    Check if there are any items on the queue
  -------------------------------------------------------------------------*/
  if( &q_ptr->head_link != q_ptr->tail_link_ptr )
  {
    /*-----------------------------------------------------------------------
      Get the last link and it's predecessor on the queue.  Remove the last
      item from the queue.  The ASSERT fires if the link is not on the queue
      and QUEUE_STRICT_CHECK is turned on.
    -----------------------------------------------------------------------*/
    ret_link_ptr                = q_ptr->tail_link_ptr;
    prev_link_ptr               = q_prev( q_ptr, ret_link_ptr );
    QUEUE_ASSERT( NULL != ret_link_ptr );
    ret_link_ptr->next_link_ptr = NULL;

    /*-----------------------------------------------------------------------
      If this was the only element then set prev_link_ptr to the head_link
      of the queue.
    -----------------------------------------------------------------------*/
    if( NULL == prev_link_ptr )
    {
      prev_link_ptr = &q_ptr->head_link;
    }

    /*-----------------------------------------------------------------------
      Make the last link's predecessor the last link in the queue.  Decrement
      item count.
    -----------------------------------------------------------------------*/
    prev_link_ptr->next_link_ptr = &q_ptr->head_link;
    q_ptr->tail_link_ptr         = prev_link_ptr;
    q_ptr->link_cnt--;
  }

  Q_FREE( q_ptr );
 
  return (void *)ret_link_ptr;
}  /* q_last_get() */


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
void *q_next
(
  q_type      *q_ptr,             /* Queue from which item is returned     */
  q_link_type *link_ptr           /* Link whose next link is required      */
)
{
  q_link_type *ret_link_ptr = NULL;    /* Link to be returned              */
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  /*-------------------------------------------------------------------------
    Verify parameters
  -------------------------------------------------------------------------*/
  ASSERT( NULL != q_ptr );
  ASSERT( NULL != link_ptr );

  /*-------------------------------------------------------------------------
    Check that the queue is initialized.  Works if
    FEATURE_QUEUE_NO_STRICT_CHECK is turned off.
  -------------------------------------------------------------------------*/
  QUEUE_CHECK_INIT( q_ptr );

  Q_LOCK( q_ptr );

  /*-------------------------------------------------------------------------
    If the passed link is the last link on the queue return NULL, otherwise
    return the link after the passed link.
  -------------------------------------------------------------------------*/
  if( link_ptr != q_ptr->tail_link_ptr )
  {
    ret_link_ptr = link_ptr->next_link_ptr;
  }

  Q_FREE( q_ptr );

  return (void *)ret_link_ptr;
} /* q_next() */


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
void q_insert
(
  q_type      *q_ptr,                  /* Pointer to the queue             */
  q_link_type *insert_link_ptr,        /* Pointer to link to be inserted   */
  q_link_type *next_link_ptr           /* This comes after insert_link_ptr */
)
{
  q_link_type  *prev_link_ptr = NULL;  /* Link before next_link_ptr        */
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  /*-------------------------------------------------------------------------
    Verify parameters
  -------------------------------------------------------------------------*/
  ASSERT( NULL != q_ptr );
  ASSERT( NULL != insert_link_ptr );
  ASSERT( NULL != next_link_ptr );

  /*-------------------------------------------------------------------------
    Check that the queue is initialized.  Works if
    FEATURE_QUEUE_NO_STRICT_CHECK is turned off.
  -------------------------------------------------------------------------*/
  QUEUE_CHECK_INIT( q_ptr );

  /*-------------------------------------------------------------------------
    If the next_link_ptr is the same as the head_link ASSERT.
  -------------------------------------------------------------------------*/
  QUEUE_ASSERT( next_link_ptr != &q_ptr->head_link );

  Q_LOCK( q_ptr );

  /*-------------------------------------------------------------------------
    Find the predecessor of next_link_ptr.  If next_link_ptr is the first
    link on the queue, set the prev_link_ptr to the head_link.
  -------------------------------------------------------------------------*/
  prev_link_ptr = q_prev( q_ptr, next_link_ptr );

  if( NULL == prev_link_ptr &&
      q_ptr->head_link.next_link_ptr == next_link_ptr
    )
  {
    prev_link_ptr = &q_ptr->head_link;
  }

  /*-------------------------------------------------------------------------
    ASSERT if next_link_ptr was not found on the queue.  Else insert
    insert_link_ptr before next_link_ptr.
  -------------------------------------------------------------------------*/
  QUEUE_ASSERT( NULL != prev_link_ptr );
  insert_link_ptr->next_link_ptr = next_link_ptr;
  prev_link_ptr->next_link_ptr   = insert_link_ptr;
  q_ptr->link_cnt++;  

  Q_FREE( q_ptr );

  return;
} /* q_insert() */


/*===========================================================================
 
FUNCTION Q_DELETE

DESCRIPTION
  This function removes a specified item from a specified queue.

DEPENDENCIES
  The specified queue should have been initialized previously via a call
  to q_init.  The specified link should have been initialized via call to
  q_link.

RETURN VALUE
  None.

SIDE EFFECTS
  Item is deleted from the queue.

===========================================================================*/
void q_delete
(
  q_type       *q_ptr,            /* Pointer to the queue                  */
  q_link_type  *delete_link_ptr   /* Pointer to link to be removed from q  */
)
{
  q_link_type *prev_link_ptr = NULL;   /* Predecessor of delete_link_ptr   */
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  /*-------------------------------------------------------------------------
    Verify parameters
  -------------------------------------------------------------------------*/
  ASSERT( NULL != q_ptr );
  ASSERT( NULL != delete_link_ptr );

  /*-------------------------------------------------------------------------
    Check that the queue is initialized.  Works if
    FEATURE_QUEUE_NO_STRICT_CHECK is turned off.
  -------------------------------------------------------------------------*/
  QUEUE_CHECK_INIT( q_ptr );

  /*-------------------------------------------------------------------------
    Check if the item to be deleted is head_link, if not then delete the
    item else assert.
  -------------------------------------------------------------------------*/
  QUEUE_ASSERT( delete_link_ptr != &q_ptr->head_link )

  Q_LOCK( q_ptr );

  /*-------------------------------------------------------------------------
    Find the predecessor of the item to be deleted.
  -------------------------------------------------------------------------*/
  prev_link_ptr = q_prev( q_ptr, delete_link_ptr );

  /*-------------------------------------------------------------------------
    If this is the first item set prev_link_ptr to head_link.
  -------------------------------------------------------------------------*/
  if( NULL == prev_link_ptr &&
      q_ptr->head_link.next_link_ptr == delete_link_ptr
    )
  {
    prev_link_ptr = &q_ptr->head_link;
  }

  /*-------------------------------------------------------------------------
    If we found the link on the queue, remove the item.
  -------------------------------------------------------------------------*/
  if( NULL != prev_link_ptr )
  {
    prev_link_ptr->next_link_ptr = delete_link_ptr->next_link_ptr;

    /*-----------------------------------------------------------------------
      If this is the last item, fix the queue's tail_link_ptr.
    -----------------------------------------------------------------------*/
    if( delete_link_ptr == q_ptr->tail_link_ptr )
    {
      q_ptr->tail_link_ptr = prev_link_ptr;
    }

    q_ptr->link_cnt--;
    delete_link_ptr->next_link_ptr = NULL;
  }

  Q_FREE( q_ptr );

  return;
} /* q_delete() */


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
void *q_check
(
  q_type *q_ptr                   /* Pointer to a queue                    */
)
{
  q_link_type  *link_ptr = NULL;       /* link pointer to be returned      */
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  /*-------------------------------------------------------------------------
    Verify parameter
  -------------------------------------------------------------------------*/
  ASSERT( NULL != q_ptr );

  /*-------------------------------------------------------------------------
    Check that the queue is initialized.  Works if
    FEATURE_QUEUE_NO_STRICT_CHECK is turned off.
  -------------------------------------------------------------------------*/
  QUEUE_CHECK_INIT( q_ptr );

  /*-------------------------------------------------------------------------
    Return the first link if there is an item on the queue.
  -------------------------------------------------------------------------*/
  Q_LOCK( q_ptr );

  if( 0 < q_ptr->link_cnt )
  {
    link_ptr = q_ptr->head_link.next_link_ptr;
  }

  Q_FREE( q_ptr );

  return  (void *)link_ptr;
} /* q_check() */


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
void *q_last_check
(
  q_type *q_ptr                   /* Queue from which the item is returned */
)
{
  q_link_type  *link_ptr = NULL;       /* Link to be returned              */
  /*- - - - - - - - - - - -  - - - - - - - - - - - - - - - - - - - - - - - */
  /*-------------------------------------------------------------------------
    Verify parameters
  -------------------------------------------------------------------------*/
  ASSERT( NULL != q_ptr );

  /*-------------------------------------------------------------------------
    Check that the queue is initialized.  Works if
    FEATURE_QUEUE_NO_STRICT_CHECK is turned off.
  -------------------------------------------------------------------------*/
  QUEUE_CHECK_INIT( q_ptr );

  /*-------------------------------------------------------------------------
    If there are items on the queue, return the last item
  -------------------------------------------------------------------------*/
  Q_LOCK( q_ptr );

  if( 0 < q_ptr->link_cnt )
  {
    link_ptr = q_ptr->tail_link_ptr;
  }

  Q_FREE( q_ptr );

  return  (void *)link_ptr;
}  /* q_last_check */


/*===========================================================================
 
FUNCTION Q_CNT

DESCRIPTION
  This function returns the number of items currently queued on a specified
  queue.

DEPENDENCIES
  The specified queue should have been initialized previously via a call to
  q_init.

RETURN VALUE
  The number of items currently queued on the specified queue.

SIDE EFFECTS
  None.

===========================================================================*/
int q_cnt
(
  q_type  *q_ptr                  /* Queue whose item count is required    */
)
{
  /*- - - - - - - - - - - -  - - - - - - - - - - - - - - - - - - - - - - - */
  /*-------------------------------------------------------------------------
    Verify parameter
  -------------------------------------------------------------------------*/
  ASSERT( NULL != q_ptr );

  /*-------------------------------------------------------------------------
    Check that the queue is initialized.  Works if
    FEATURE_QUEUE_NO_STRICT_CHECK is turned off.
  -------------------------------------------------------------------------*/
  QUEUE_CHECK_INIT( q_ptr );

  /*-------------------------------------------------------------------------
    Return link_cnt field.
  -------------------------------------------------------------------------*/
  return q_ptr->link_cnt;
} /* q_cnt() */


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
void *q_linear_search
(
  q_type              *q_ptr,          /* Queue to be traversed            */
  q_compare_func_type  compare_func,   /* Compare function to be called    */
  void                *compare_val     /* Cookie to pass to compare_func   */
)
{
  q_link_type *link_ptr = NULL;        /* Link to be returned              */
  /*- - - - - - - - - - - -  - - - - - - - - - - - - - - - - - - - - - - - */
  /*-------------------------------------------------------------------------
    Verify parameters.
  -------------------------------------------------------------------------*/
  ASSERT( NULL != q_ptr );
  ASSERT( NULL != compare_func );

  /*-------------------------------------------------------------------------
    Check that the queue is initialized.  Works if
    FEATURE_QUEUE_NO_STRICT_CHECK is turned off.
  -------------------------------------------------------------------------*/
  QUEUE_CHECK_INIT( q_ptr );

  link_ptr = q_check( q_ptr );

  while( NULL != link_ptr )
  {
    if( 0 != compare_func( link_ptr, compare_val ) )
    {
      break;
    }
    link_ptr = q_next( q_ptr, link_ptr );
  } /* while */

  return link_ptr;
} /* q_linear_search() */


/*===========================================================================

                            INTERNAL FUNTIONS

===========================================================================*/
/*===========================================================================
 
FUNCTION Q_PREV

DESCRIPTION
  This function returns a pointer to the link of the predecessor on the
  specified queue of the element passed in as a parameter.

DEPENDENCIES
  The specified queue should have been initialized previously via a call
  to q_init.  The specified link should have been initialized via call to
  q_link.

RETURN VALUE
  The predecessor of the specified element or NULL if the specified element
  is not found on the queue or is the first item.

SIDE EFFECTS
  A pointer to a queued element's link is returned without de-queueing the
  item.

===========================================================================*/
static q_link_type *q_prev
(
  q_type      *q_ptr,             /* The queue on which link_ptr is queued */
  q_link_type *link_ptr           /* Item whose predecessor is required    */
)
{
  q_link_type *prev_link_ptr = NULL;   /* The predecessor of the item      */
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  /*-------------------------------------------------------------------------
    Verify parameters
  -------------------------------------------------------------------------*/
  ASSERT( NULL != q_ptr );
  ASSERT( NULL != link_ptr );

  /*-------------------------------------------------------------------------
    Check that the queue is initialized.  Works if 
    FEATURE_QUEUE_NO_STRICT_CHECK is not turned on.
  -------------------------------------------------------------------------*/
  QUEUE_CHECK_INIT( q_ptr );

  /*-------------------------------------------------------------------------
    If the link passed in is the head_link ASSERT.  Else find the predecessor
    of the passed item on the queue.  If this is the first item return NULL.
  -------------------------------------------------------------------------*/
  QUEUE_ASSERT( link_ptr != &q_ptr->head_link )

  prev_link_ptr = (link_ptr == q_ptr->head_link.next_link_ptr) ?
                    NULL                                       :
                    q_ptr->head_link.next_link_ptr;

  while( NULL != prev_link_ptr )
  {
    if( prev_link_ptr->next_link_ptr == link_ptr )
    {
      break;
    }
    prev_link_ptr = ( prev_link_ptr->next_link_ptr == &q_ptr->head_link ) ?
                      NULL                                                :
                      prev_link_ptr->next_link_ptr;
  } /* while */

  return prev_link_ptr;
} /* q_prev() */
/*lint +e636 */

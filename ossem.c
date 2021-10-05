/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                              OS KERNEL                                    */
/*                                                                           */
/*                  COPYRIGHT (c) 1994 by JOHN C. OVERTON                    */
/*              Advanced Communication Development Tools, Inc                */
/*                                                                           */
/*                                                                           */
/*            Module:  OSSEM.C                                               */
/*                                                                           */
/*             Title:  Create, delete and manage semaphores.                 */
/*                                                                           */
/*       Description:  This module contains:                                 */
/*                                                                           */
/*                     OsSemCreate() - Creates a semaphore.                  */
/*                     OsSemDelete() - Deletes a semaphore.                  */
/*                     OsSemWait()   - Waits for a semaphore to be posted.   */
/*                     OsSemPost()   - Posts a semaphore.                    */
/*                                                                           */
/*                     These semaphores are counting semaphores. They are    */
/*                     referenced by an interger handle. When a semaphore    */
/*                     is created, it can be preset to any number. When a    */
/*                     process waits on a semaphore, it is decremented, and  */
/*                     if the count goes negative, the process will wait.    */
/*                     When a process posts a semaphore, the count is        */
/*                     incremented, and if the count goes positive, then     */
/*                     a waiting process will be allowed to run again.       */
/*                                                                           */
/*            Author:  John C. Overton                                       */
/*                                                                           */
/*              Date:  05/09/94                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#include "oskernel.h"



/*---------------------------------------------------------------------------*/
/* Static local routines in this module...                                   */
/*---------------------------------------------------------------------------*/

static int AllocSem( void );           /* Allocate next semaphore id.        */



/*---------------------------------------------------------------------------*/
/* OsSemCreate() -- Create a new semaphore...                                */
/*---------------------------------------------------------------------------*/

HANDLE   OsSemCreate(int Count)
{
   HANDLE     SID;                     /* New semaphore id number.           */
   SEMAPHORE *Sem;                     /* Pointer to new semaphore tab entry.*/


   OsDisable();                        /* Disable interrupts.                */


   /*------------------------------------------------------------------------*/
   /* Allocate a semaphore structure...                                      */
   /*------------------------------------------------------------------------*/
   if ((Sem = (SEMAPHORE *) OsAlloc(sizeof(SEMAPHORE))) == NULL) {
      OsEnable();                      /* Enable interrupts.                 */
      return (SYSERR);                 /* Can not allocate any more.         */
   }

   /*------------------------------------------------------------------------*/
   /* Allocate a handle for new semaphore...                                 */
   /*------------------------------------------------------------------------*/
   if ((SID = OsHandCreate(&SemaphoreAnchor, (void *) Sem)) == SYSERR) {
      OsFree(Sem);                     /* Can not use semaphore struct.      */
      OsEnable();                      /* Enable interrupts.                 */
      return (SYSERR);                 /* Can not allocate any more.         */
   }

   Sem->Count = Count;                 /* Set count.                         */

   OsHandUnprotect(SemaphoreAnchor, SID); /* Unprotect resource.             */

   OsEnable();                         /* Enable interrupts.                 */
   return SID;                         /* Return with new Semaphore handle.  */
}



/*---------------------------------------------------------------------------*/
/* OsSemDelete() -- Destroy a semaphore, free up table entry, ready procs... */
/*---------------------------------------------------------------------------*/

int   OsSemDelete(HANDLE Sem)
{
   SEMAPHORE *S;                       /* Pointer to semaphore structure.    */
   PROCESS   *P;                       /* Pointer to process structure.      */

   OsDisable();                        /* Disable interrupts.                */

   if ((S = (SEMAPHORE *) OsHandDestroy(SemaphoreAnchor, Sem)) == NULL) {
      OsEnable();                      /* Enable interrupts.                 */
      return SYSERR;                   /* Return with error.                 */
   }


   P = ChainFirst( &S->WaitList );     /* Get first process waiting.         */
   while (P) {                         /* While there are processes waiting, */
      OsReady(P->Pid);                 /* Ready process.                     */
      P = ChainNext( &P->Link );       /* Next process in chain.             */
   }

   OsFree(S);                          /* Free semaphore structure.          */

   OsEnable();                         /* Enable interrupts.                 */
   return SYSOK;                       /* Return with no errors.             */
}



/*---------------------------------------------------------------------------*/
/* OsWait() -- Decrement semaphore count and make process wait if count      */
/*             is below 0...                                                 */
/*---------------------------------------------------------------------------*/


int   OsWait(HANDLE Sem)
{
   SEMAPHORE *S;                       /* Pointer to semaphore structure.    */
   PROCESS   *P;                       /* Pointer to process structure.      */

   OsDisable();                        /* Disable interrupts.                */

   if ((S = (SEMAPHORE *) OsHandFind(SemaphoreAnchor, Sem)) == NULL) {
      OsEnable();                      /* Enable interrupts.                 */
      return SYSERR;                   /* Return with error.                 */
   }

   /*------------------------------------------------------------------------*/
   /* If count is negative, then make current process wait...                */
   /*------------------------------------------------------------------------*/
   if ( --S->Count < 0 ) {                 /* Decrement count.               */
      P = OsHandFind(ProcessAnchor, CurrPid);  /* Get current proc's struct. */
      Unchain( &ReadyAnchor, &P->Link);    /* Remove process from ready chain*/
      P->State = PRWAIT;                   /* State is now "waiting".        */
      ChainQueue( &S->WaitList, &P->Link); /* Queue onto semaphore.          */
      OsSched();                           /* Now, let others run.           */
   }

   OsEnable();                         /* Enable interrupts.                 */
   return SYSOK;                       /* Return with no errors.             */
}



/*---------------------------------------------------------------------------*/
/* OsPost() -- Increment semaphore count and ready process if count goes     */
/*             above 0...                                                    */
/*---------------------------------------------------------------------------*/

int   OsPost(HANDLE Sem)
{
   SEMAPHORE *S;                       /* Pointer to semaphore structure.    */
   PROCESS   *P;                       /* Pointer to process structure.      */

   OsDisable();                        /* Disable interrupts.                */

   if ((S = (SEMAPHORE *) OsHandFind(SemaphoreAnchor, Sem)) == NULL) {
      OsEnable();                      /* Enable interrupts.                 */
      return SYSERR;                   /* Return with error.                 */
   }

   if ( S->Count++ < 0 )               /* If semaphore count is still neg.   */
                                       /* Ready top waiting process...       */
      OsReady( ((PROCESS *) ChainPop( &S->WaitList ))->Pid );

   OsEnable();                         /* Enable interrupts.                 */
   return SYSOK;                       /* Return with no errors.             */
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                              OS KERNEL                                    */
/*                                                                           */
/*                  COPYRIGHT (c) 1994 by JOHN C. OVERTON                    */
/*              Advanced Communication Development Tools, Inc                */
/*                                                                           */
/*                                                                           */
/*            Module:  OSPROC.C                                              */
/*                                                                           */
/*             Title:  Create, kill, manage, and schedule processes.         */
/*                                                                           */
/*       Description:  This module contains the following:                   */
/*                                                                           */
/*                     OsCreate()  - Create a process that ready to run.     */
/*                     OsSched()   - Schedule process with highest priority. */
/*                     OsKill()    - Kill a process.                         */
/*                     OsReturn()  - Kills currently running process.        */
/*                     OsReady()   - Make a process ready to run.            */
/*                     OsSuspend() - Suspend a ready process.                */
/*                     OsResume()  - Resume a suspended process.             */
/*                     OsGetPid()  - Get process id of currently running     */
/*                                   process.                                */
/*                                                                           */
/*            Author:  John C. Overton                                       */
/*                                                                           */
/*              Date:  04/23/94                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#include "oskernel.h"


static int  NewPid(        void );


/*---------------------------------------------------------------------------*/
/* OsCreate  --  Create a process to start running a procedure               */
/*---------------------------------------------------------------------------*/

HANDLE  OsCreate(
   void     *procaddr,                 /* Procedure address.                 */
   int       ssize,                    /* Stack size in words.               */
   int       priority,                 /* Process priority >= 0.             */
   char     *name,                     /* Name ( for debugging ).            */
   char     *data )                    /* Argument passed to new process.    */

{
   HANDLE   Pid;                       /* Stores new process id.             */
   PROCESS *pptr;                      /* Pointer to process table entry.    */
   int      i;
   USHORT  *stk;                       /* Stack address.                     */


   ssize = ((ssize + 3) & (~3));       /* Round size to long word boundary.  */

   if ( ssize < MIN_STACK_SIZE )       /* Make sure at least minimum size.   */
      ssize = MIN_STACK_SIZE;

   if ( priority < 1 ) {               /* Check a few parms.           */
   	OsEnable();
   	return(SYSERR);
   }


   /*------------------------------------------------------------------------*/
   /* Create process structure...                                            */
   /*------------------------------------------------------------------------*/

   pptr  = (PROCESS *) OsAlloc(sizeof(PROCESS));

   /*------------------------------------------------------------------------*/
   /* Create handle for process (same as process id)...                      */
   /*------------------------------------------------------------------------*/

   if ((Pid = OsHandCreate(&ProcessAnchor, (void *) pptr)) == SYSERR)  {
   	OsFree(pptr);                    /* Free process structure, can't use. */
   	return(SYSERR);                  /* Can't create process due to handle.*/
   }

   /*------------------------------------------------------------------------*/
   /* Allocate a stack for process...                                        */
   /*------------------------------------------------------------------------*/

   if ( (stk = (USHORT *) OsAlloc(ssize)) == NULL) {   /* Allocate stack.    */
   	OsFree(pptr);                    /* Free process structure, can't use. */
   	OsHandUnprotect(ProcessAnchor, Pid);
   	OsHandDestroy(  ProcessAnchor, Pid);
   	return(SYSERR);                  /* Can't create process, no stack.    */
   }


   /*------------------------------------------------------------------------*/
   /* Put all the information into the process structure...                  */
   /*------------------------------------------------------------------------*/

   ChainInit(&pptr->Link, pptr);       /* Initialize Link fields.            */
   pptr->Pid    = Pid;                 /* Process id of this process.        */
   strncpy(pptr->Name, name, PNMLEN);  /* Process' name.                     */
   pptr->Name[PNMLEN - 1] = '\0';      /* Assure null termination.           */
   pptr->Prio   = priority;            /* Process priority.                  */
   pptr->Base   = (BYTE *) stk;        /* Base (bottom) of stack.            */
   pptr->StkLen = ssize;               /* Size of stack.                     */
   pptr->State  = PRSUSP;              /* Make it look suspended for OsReady.*/

   ((BYTE *) stk) += ssize;            /* Position stack pointer.            */


   /*------------------------------------------------------------------------*/
   /*************** BEGINNING OF IMPLEMENATION SPECIFIC CODE *****************/
   /*------------------------------------------------------------------------*/

   *--stk    = (USHORT) FP_SEG(data);
   *--stk    = (USHORT) FP_OFF(data);

   *--stk    = (USHORT) FP_SEG(OsReturn);
   *--stk    = (USHORT) FP_OFF(OsReturn);

   *--stk    = (USHORT) FP_SEG(procaddr);
   *--stk    = (USHORT) FP_OFF(procaddr);

   *--stk    = (USHORT) 0x0000;        /* BP = 0 */

   *--stk    = (USHORT) 0x0200;        /* FLAGS = Enable interrupts. */

   *--stk    = (USHORT) 0x0000;        /* AX = 0 */
   *--stk    = (USHORT) 0x0000;        /* CX = 0 */
   *--stk    = (USHORT) 0x0000;        /* DX = 0 */
   *--stk    = (USHORT) 0x0000;        /* BX = 0 */
   *--stk    = (USHORT) 0x0000;        /* SP = 0 */
   *--stk    = (USHORT) 0x0000;        /* BP = 0 */
   *--stk    = (USHORT) 0x0000;        /* SI = 0 */
   *--stk    = (USHORT) 0x0000;        /* DI = 0 */

   *--stk    = (USHORT) 0x0000;        /* ES = 0 */

   pptr->Stack = (BYTE *) stk;

   /*------------------------------------------------------------------------*/
   /**************** END OF IMPLEMENATION SPECIFIC CODE **********************/
   /*------------------------------------------------------------------------*/

   OsDisable();
   NumProc++;                          /* Count of created processes.        */
   OsEnable();
   OsHandUnprotect( ProcessAnchor, Pid );   /* Unprotect handle.             */

   OsReady(Pid);                       /* Make task ready to run.            */

   return(Pid);
}




/*---------------------------------------------------------------------------*/
/* OsSched() -- Reschedule processing...                                     */
/*---------------------------------------------------------------------------*/

int  OsSched( void )

{
   register PROCESS *cptr;             /* Currently running process.         */
   register PROCESS *tptr;             /* Top process in ready queue.        */
   register PROCESS *nptr;             /* Next process in ready queue.       */


   OsDisable();                        /* Disable interrupts.                */

   /*------------------------------------------------------------------------*/
   /* First, go do sleeper check to see if any sleepers have expired...      */
   /*------------------------------------------------------------------------*/

   OsSleepCheck();                     /* Check for any expired sleepers.    */

   /*------------------------------------------------------------------------*/
   /* Get first process in ready chain. If there are none (normaly the low-  */
   /* est prior task is always runnable) then loop until there is a process. */
   /*------------------------------------------------------------------------*/

   while ((tptr = ChainFirst(&ReadyAnchor)) == NULL) {
      enable();                        /* Open a window for interrupts.      */
      disable();                       /* Maybe an isr will ready a task.    */
   }

   nptr = ChainNext( &tptr->Link );    /* Get second process in ready queue. */


   /*------------------------------------------------------------------------*/
   /* No context switch is needed if highest priority process is the         */
   /* currently running process, and there are no others with same priority. */
   /*------------------------------------------------------------------------*/

   if (tptr->Pid == CurrPid) {         /* If current one is top of queue...  */

      if (nptr != NULL) {              /* Are there more in the ready queue? */
         if (tptr->Prio == nptr->Prio) { /* And, does it have same priority? */

            ChainPop( &ReadyAnchor );  /* Pop off currently running process. */
            OsReady( tptr->Pid );      /* Reschedule current one for later.  */
            tptr = nptr;               /* Now make next one the top of queue.*/
         }
      }
   }

   if (tptr->Pid == CurrPid) {         /* If current one is top of queue...  */
      OsEnable();                      /* Enable interrupts.                 */
      return (SYSOK);                  /* Return.                            */
   }


   /*------------------------------------------------------------------------*/
   /* Now, if current process is still eligable, make it ready...            */
   /*------------------------------------------------------------------------*/

   cptr = (PROCESS *) OsHandFind(ProcessAnchor, CurrPid);  /* Get curr proc. */

   cptr->Disable = DisableCount;       /* Save disable count.                */

   if (cptr->State == PRCURR)          /* Make it ready for next time.       */
      cptr->State = PRREADY;


   /*------------------------------------------------------------------------*/
   /* Now, switch context to new process...                                  */
   /*------------------------------------------------------------------------*/

   CurrPid = tptr->Pid;                /* Save current process id.           */
   tptr->State = PRCURR;               /* Make top on current one.           */

   DisableCount = tptr->Disable;       /* New disable count.                 */

   OsSwitch(&cptr->Stack, &tptr->Stack);                 /* Switch context.  */


   /*------------------------------------------------------------------------*/
   /* The resumed process picks up here. (If it was a new task, then         */
   /* OsSwitch() does not return, but instead goes directly to new task.)... */
   /* If there were any killed processes, then free them now...              */
   /*------------------------------------------------------------------------*/

   while ((cptr = ChainPop( &KilledAnchor )) != NULL) {
      OsHandDestroy(ProcessAnchor, cptr->Pid); /* Destroy handle (Pid).      */
      OsFree( cptr->Base );            /* Free killed proc's stack.          */
      OsFree( cptr );                  /* Free killed proc's structure.      */
   }

   OsEnable();                         /* Enable interrupts.                 */

   return (SYSOK);                     /* Return to new process.             */
}



/*---------------------------------------------------------------------------*/
/* OsReturn() -- Kills process when it returns...                            */
/*---------------------------------------------------------------------------*/

int  OsReturn( void )
{
   return ( OsKill( CurrPid ));
}




/*---------------------------------------------------------------------------*/
/* OsKill() -- Kill process, given process id...                             */
/*---------------------------------------------------------------------------*/

int  OsKill( HANDLE Pid )

{
   PROCESS *pptr;                      /* Points to process entry for pid.   */
   MESSAGE *Msg;                       /* Messages that need to be freed.    */
   int      State;                     /* State of process on entry.         */

   OsDisable();

   if ((pptr = (PROCESS *) OsHandFind(ProcessAnchor, Pid)) == NULL)  {
      OsEnable();
      return(SYSERR);
   }

   State = pptr->State;                /* Save current state of process.     */

   if(pptr->Flags & PROCESS_CANT_KILL) /* Can we kill this process?          */
      if (Pid != CurrPid)              /* Only if it's the current process.  */
         return SYSERR;                /* Otherwise, it's an error.          */


   switch (State)  {                   /* Depending on current state...      */

      case PRSLEEP:                    /* Process is sleeping on event.      */
         OsAwake(Pid);                 /* First wake process up.             */

      case PRCURR:                     /* This is the currently running proc.*/
      case PRREADY:                    /* Process is ready to run.           */
         Unchain(&ReadyAnchor, &pptr->Link);   /* Remove from ready queue.   */
         break;

      case PRWAIT:                     /* Process waiting on semaphore.      */
         /* SemTab[pptr->Sem].SemCnt++; */

      default:
         break;
   }

   NumProc--;                          /* Count of created processes.        */
   pptr->State = PRKILL;               /* Put process in killed state.       */
   ChainPush( &KilledAnchor, &pptr->Link); /* Free stack & proc later.     */

   while ((Msg = ChainPop(&pptr->Msgs)) != NULL) {
      if (Msg->Data != NULL)           /* Does data need to be free'd?       */
         OsFree(Msg->Data);
      if (Msg->Pid > 0)                /* Is there a waiter waiting for msg? */
         OsReady(Msg->Pid);
      OsFree(Msg);                     /* Free message structure.            */
   }

   OsEnable();                         /* Enable interrupts again.           */

   if (State == PRCURR)                /* This was current process...        */
      OsSched();                       /* Schedule and run next process.     */

   return(SYSOK);                      /* Process has been killed.           */
}



/*---------------------------------------------------------------------------*/
/* OsGetPid() -- Return current process id (handle)...                       */
/*---------------------------------------------------------------------------*/

HANDLE  OsGetPid( void )

{
   return CurrPid;

}




/*---------------------------------------------------------------------------*/
/* OsReady() -- Make process ready to run...                                 */
/*---------------------------------------------------------------------------*/

int  OsReady( HANDLE Pid )

{
   PROCESS *pptr;
   PROCESS *p, *q;

   OsDisable();                        /* Disable interupts while chaining.  */

   if ((pptr = (PROCESS *) OsHandFind(ProcessAnchor, Pid)) == NULL)  {
      OsEnable();                      /* Enable interrupts now.             */
      return SYSERR;                   /* No, error.                         */
   }

   if (pptr->State == PRREADY ||       /* If already in ready chain, or      */
       pptr->State == PRCURR )  {      /* If currently running...            */
      OsEnable();                      /* Enable interrupts now.             */
      return SYSOK;                    /*   then don't add it again.         */
   }

   if (pptr->State == PRWAKING) {      /* If process is waking up...         */

   }

   if (pptr->State == PRSLEEP) {       /* If process is sleeping...          */
      pptr->State = PRWAKING;          /* Say that it is waking up.          */
      OsAwake(Pid);                    /* Wake up process which calls us back*/
      OsEnable();
      return SYSOK;
   }

   pptr->State = PRREADY;              /* Make process state ready state.    */


   /*------------------------------------------------------------------------*/
   /* Add process to ready queue in priority order...                        */
   /*------------------------------------------------------------------------*/

   p = ChainFirst( &ReadyAnchor );     /* Get first in chain.                */
   q = NULL;                           /* Reset previous pointer.            */
   while (p) {
      if ( pptr->Prio > p->Prio )      /* Insert before p process?           */
         break;                        /* Yes...                             */
      q = p;                           /* No. Save current as previous.      */
      p = ChainNext( &p->Link );       /* Point to next on chain.            */
   }

   if (q)                              /* Are we following another one?      */
      Chain( &ReadyAnchor, &q->Link, &pptr->Link);
   else                                /* Insert at top of chain.            */
      Chain( &ReadyAnchor, NULL,     &pptr->Link);

   OsEnable();                         /* Enable interrupts now.             */

   return SYSOK;                       /* Return with good return code.      */
}



/*---------------------------------------------------------------------------*/
/* OsResume() -- Make a process ready to run...                              */
/*---------------------------------------------------------------------------*/

int  OsResume( HANDLE Pid )

{
   PROCESS *pptr;

   OsDisable();                        /* Disable when messing with Process. */

   if ((pptr = (PROCESS *) OsHandFind(ProcessAnchor, Pid)) == NULL)  {
      OsEnable();                      /* Enable interrupts now.             */
      return SYSERR;                   /* No, error.                         */
   }

   if (pptr->State != PRSUSP) {        /* Was process suspended?             */
      OsEnable();                      /* Enable interrupts now.             */
      return SYSERR;                   /* No, error.                         */
   }

   OsReady(Pid);                       /* Make process ready to run.         */

   OsEnable();                         /* Enable interrupts now.             */

   return SYSOK;                       /* Return with good return code.      */
}




/*---------------------------------------------------------------------------*/
/* OsSuspend() -- Suspend a process...                                       */
/*---------------------------------------------------------------------------*/

int  OsSuspend( HANDLE Pid )

{
   PROCESS *pptr, *p, *q;
   int      State;

   OsDisable();                        /* Disable interrupts.                */

   if ((pptr = (PROCESS *) OsHandFind(ProcessAnchor, Pid)) == NULL)  {
      OsEnable();
      return SYSERR;                   /* No, error.                         */
   }

   State = pptr->State;                /* Save process state.                */

   if (State != PRREADY &&             /* Is process not in ready state or   */
       State != PRCURR  ) {            /* not the currently running process? */
      OsEnable();
      return SYSERR;                   /* Then error.                        */
   }

   Unchain(&ReadyAnchor, &pptr->Link); /* Remove from ready queue.           */

   pptr->State = PRSUSP;               /* Mark process as suspended.         */

   OsEnable();                         /* Enable interrupts.                 */

   if (State == PRCURR)                /* If this one is currently running...*/
      OsSched();                       /* Reschedule processes.              */

   return SYSOK;                       /* Return with good return code.      */
}


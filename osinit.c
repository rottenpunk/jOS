/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                              OS KERNEL                                    */
/*                                                                           */
/*                  COPYRIGHT (c) 1994 by JOHN C. OVERTON                    */
/*              Advanced Communication Development Tools, Inc                */
/*                                                                           */
/*                                                                           */
/*            Module:  OSINIT.C                                              */
/*                                                                           */
/*             Title:  Initialize the Kernel.                                */
/*                                                                           */
/*       Description:  OsInit() will make first process running.             */
/*                                                                           */
/*            Author:  John C. Overton                                       */
/*                                                                           */
/*              Date:  04/23/94                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#include "oskernel.h"



/*---------------------------------------------------------------------------*/
/* OsInit() -- Initialize the OS KERNEL...                                   */
/*---------------------------------------------------------------------------*/


int  OsInit( void )

{
   HANDLE   Pid;                       /* Stores new process id.             */
   PROCESS *pptr;                      /* Pointer to process table entry.    */


   OsDisable();                        /* Disable interrupts.                */


   /*------------------------------------------------------------------------*/
   /* Initialize fields...                                                   */
   /*------------------------------------------------------------------------*/



   /*------------------------------------------------------------------------*/
   /* Set up first process...                                                */
   /*------------------------------------------------------------------------*/

   NumProc++;                          /* First procedure.                   */
   pptr  =  (PROCESS *) OsAlloc(sizeof(PROCESS));

   /*------------------------------------------------------------------------*/
   /* Create handle for first process (handle same as process id)...         */
   /*------------------------------------------------------------------------*/
   if ((Pid = OsHandCreate(&ProcessAnchor, (void *) pptr)) == SYSERR)  {
   	OsEnable();
   	return(SYSERR);                  /* Can't create process due to handle.*/
   }

   /*------------------------------------------------------------------------*/
   /* Fill in process structure...                                           */
   /*------------------------------------------------------------------------*/
   pptr->Flags |= PROCESS_CANT_KILL;   /* Make sure it can't be killed.      */
   ChainInit(&pptr->Link, pptr);       /* Initialize Link fields.            */
   pptr->Pid    = Pid;                 /* Process id of this process.        */
   pptr->State  = PRREADY;             /* Start it in ready state.           */
   strncpy(pptr->Name, "INIT", PNMLEN);      /* Process' name.               */
   pptr->Prio   = -32767;              /* Process priority. Lowest possible. */
   pptr->Base   = NULL;                /* Base (bottom) of stack.            */
   pptr->StkLen = 0;                   /* Size of stack.                     */
   Chain( &ReadyAnchor, NULL, &pptr->Link);   /* Put on ready queue.         */
   CurrPid = Pid;                      /* Set current process id number.     */

   OsHandUnprotect( ProcessAnchor, Pid);  /* Unprotect ?                     */

   OsSleepInit();                      /* Initialize sleep functions.        */
   OsDevInit();                        /* Initialize device functions.       */

   OsEnable();
   return(SYSOK);
}




/*---------------------------------------------------------------------------*/
/* OsTerm() -- Terminate the OS KERNEL...                                    */
/*---------------------------------------------------------------------------*/


int  OsTerm( void )

{
   OsSleepTerm();                      /* Terminate sleep functions.         */
   OsDevTerm();                        /* Terminate device functions.        */

   return(SYSOK);
}

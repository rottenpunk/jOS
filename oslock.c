/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                              OS KERNEL                                    */
/*                                                                           */
/*                  COPYRIGHT (c) 1994 by JOHN C. OVERTON                    */
/*              Advanced Communication Development Tools, Inc                */
/*                                                                           */
/*                                                                           */
/*            Module:  OSLOCK.C                                              */
/*                                                                           */
/*             Title:  Lock and unlock a resource and serialize access.      */
/*                                                                           */
/*       Description:  This module contains:                                 */
/*                                                                           */
/*                     OsLock()    - Lock a resource.                        */
/*                     OsUnlock()  - Unlock a resource.                      */
/*                                                                           */
/*                                                                           */
/*            Author:  John C. Overton                                       */
/*                                                                           */
/*              Date:  11/17/94                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#include "oskernel.h"





/*---------------------------------------------------------------------------*/
/* OsLock() -- Lock a resource...                                            */
/*---------------------------------------------------------------------------*/

int   OsLock(HANDLE *Lock)
{
   PROCESS   *Process;
   PROCESS   *Process2;

   OsDisable();                        /* Disable interrupts.                */

   Process = (PROCESS *) OsHandProtect(ProcessAnchor, CurrPid);

   if (*Lock == 0) {
      *Lock = Process->Pid;
      OsHandUnprotect(ProcessAnchor, CurrPid);
      OsEnable();
      return (SYSOK);
   }

   if ((Process2 = (PROCESS *) OsHandProtect(ProcessAnchor, *Lock)) == NULL)  {
      OsHandUnprotect(ProcessAnchor, CurrPid);
      OsEnable();
      return (SYSERR);
   }

   Process->Lock  = 0;                 /* Just to make sure.                 */
   Process2->Lock = CurrPid;           /* Chain.                             */

   OsHandUnprotect(ProcessAnchor, *Lock);     /* Unlock processes.           */
   OsHandUnprotect(ProcessAnchor, CurrPid);

   *Lock = CurrPid;                    /* We are last process to wait.       */

   OsSuspend(CurrPid);                 /* Make us wait until our turn.       */

   OsEnable();                         /* Enable interrupts.                 */
   return SYSOK;                       /* Return                             */
}



/*---------------------------------------------------------------------------*/
/* OsUnlock() -- Unlock a resource...                                        */
/*---------------------------------------------------------------------------*/

int   OsUnlock(HANDLE *Lock)
{
   PROCESS   *Process;

   OsDisable();                        /* Disable interrupts.                */

   Process = (PROCESS *) OsHandProtect(ProcessAnchor, CurrPid);

   if (Process->Lock == 0) {           /* If no waiters on lock...           */

      if (*Lock != Process->Pid) {     /* Are we the one that locked it?     */
         OsHandUnprotect(ProcessAnchor, CurrPid);
         OsEnable();
         return SYSERR;                /* No, error then!                    */
      }

      *Lock = 0;                       /* Clear, no one was waiting.         */

   } else {

      OsResume(Process->Lock);         /* Resume next waiter.                */
      Process->Lock = 0;               /* Clear chain pointer.               */
   }

   OsEnable();                         /* Enable interrupts.                 */
   return SYSOK;                       /* Return, no errors.                 */
}

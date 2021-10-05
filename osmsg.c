/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                              OS KERNEL                                    */
/*                                                                           */
/*                  COPYRIGHT (c) 1994 by JOHN C. OVERTON                    */
/*              Advanced Communication Development Tools, Inc                */
/*                                                                           */
/*                                                                           */
/*            Module:  OSMSG.C                                               */
/*                                                                           */
/*             Title:  Send and Receive messages.                            */
/*                                                                           */
/*       Description:  This module contains:                                 */
/*                                                                           */
/*                     OsMsgSend()    - Send a message to a process.         */
/*                     OsMsgRecv()    - Receive a message.                   */
/*                                                                           */
/*                                                                           */
/*            Author:  John C. Overton                                       */
/*                                                                           */
/*              Date:  11/04/94                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#include "oskernel.h"





/*---------------------------------------------------------------------------*/
/* OsMsgSend() -- Send a message to a process...                                */
/*---------------------------------------------------------------------------*/

int   OsMsgSend(HANDLE Pid, void *Data, int Length, int Wait)
{
   MESSAGE   *Msg;
   PROCESS   *Process;

   OsDisable();                        /* Disable interrupts.                */

   if ((Process = (PROCESS *) OsHandFind(ProcessAnchor, Pid)) == NULL)  {
      OsEnable();
      return(SYSERR);
   }

   Msg = OsAlloc( sizeof(MESSAGE));    /* Get another message structure.     */
   Msg->Data = OsAlloc(Length);        /* Get memory for message.            */
   Msg->Length = Length;               /* Save length of message.            */
   memcpy(Msg->Data, Data, Length);    /* Copy data to safe place.           */
   ChainInit(&Msg->Link, Msg);         /* Initialize link fields.            */
   ChainQueue(&Process->Msgs, &Msg->Link);  /* Queue up message.             */
   Process->MsgCount++;

   if (Process->State == PRRECV)       /* Is process waiting for a message?  */
      OsReady(Pid);                    /* Yes, so make it ready.             */

   if (Process->MsgCount > NMSG ||     /* Can we queue more messages?        */
       Wait == True  ) {               /*   or are we to wait anyhow?        */
      Msg->Pid = OsGetPid();           /* Say that we are suspended.         */
      Unchain( &ReadyAnchor, &(Process->Link)); /* Remove from ready chain.  */
      Process->State = PRSEND;         /* Say process is waiting to send.    */
      OsSched();                       /* Let someone else run.              */
   }

   OsEnable();                         /* Enable interrupts.                 */
   return SYSOK;                       /* Return                             */
}



/*---------------------------------------------------------------------------*/
/* OsMsgRecv() -- Receive a message. Wait if no messages available...           */
/*---------------------------------------------------------------------------*/

int   OsMsgRecv(void **Data, int *Length, int Wait)
{
   MESSAGE   *Msg;
   PROCESS   *Process;

   OsDisable();                        /* Disable interrupts.                */

   Process = (PROCESS *) OsHandFind(ProcessAnchor, CurrPid);

   if (Process->MsgCount == 0 && Wait) {    /* Need to wait for message?     */
      Unchain( &ReadyAnchor, &(Process->Link)); /* Remove from ready chain.  */
      Process->State = PRRECV;         /* Say process is waiting for message.*/
      OsSched();                       /* Let some else run.                 */
   }

   if (Process->MsgCount > 0 && Wait) {
      Process->MsgCount--;             /* One less message.                  */
      Msg = ChainPop( &Process->Msgs); /* Pop off a message.                 */
      *Data = Msg->Data;               /* Pass data to caller.               */
      *Length = Msg->Length;           /* Pass data lenbgth to caller.       */
      if (Msg->Pid)                    /* Is there a waiting process?        */
         OsResume(Msg->Pid);           /* Then resume it.                    */
      OsFree(Msg);                     /* Free message structure.            */
      OsEnable();                      /* Enable interrupts.                 */
      return(SYSOK);                   /* Return to caller.                  */
   }

   OsEnable();                         /* Enable interrupts.                 */
   return SYSNOMSG;                    /* Return indicating no messages.     */
}




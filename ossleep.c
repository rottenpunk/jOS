/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                              OS KERNEL                                    */
/*                                                                           */
/*                  COPYRIGHT (c) 1994 by JOHN C. OVERTON                    */
/*              Advanced Communication Development Tools, Inc                */
/*                                                                           */
/*                                                                           */
/*            Module:  OSSLEEP.C                                             */
/*                                                                           */
/*             Title:  Various sleeping related functions.                   */
/*                                                                           */
/*       Description:  This module contains:                                 */
/*                                                                           */
/*                     OsSleepInit()  - Initialize Sleep routines.           */
/*                     OsSleepTerm()  - Terminate Sleep routines.            */
/*                     OsSleepCheck() - Check for Sleep expirations.         */
/*                     OsSleepReady() - Unsleep a sleeper.                   */
/*                     OsSleep()      - Suspend a process for period of time.*/
/*                                                                           */
/*                                                                           */
/*            Author:  John C. Overton                                       */
/*                                                                           */
/*              Date:  10/27/94                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#include "oskernel.h"


static void interrupt (*Old_Timer_Vector)(void);
static void interrupt TimerTick(void);




/*---------------------------------------------------------------------------*/
/* OsSleepInit() -- Initialize the sleep functions...                        */
/*---------------------------------------------------------------------------*/

int   OsSleepInit(void)
{

   OsDisable();                        /* Disable interrupts.                */

   Old_Timer_Vector = getvect(0x08);   /* Get old timer tick vector address. */
   setvect(0x08, TimerTick);           /* Set our routine in its place.      */

   Seconds = time(NULL);               /* Get current time value.            */

   OsEnable();                         /* Enable interrupts.                 */
   return SYSOK;                       /* Return with no errors.             */
}



/*---------------------------------------------------------------------------*/
/* OsSleepTerm() -- Terminate the sleep functions...                         */
/*---------------------------------------------------------------------------*/

int   OsSleepTerm(void)
{

   OsDisable();                        /* Disable interrupts.                */

   setvect(0x08, Old_Timer_Vector);    /* Restore DOS' timer tick vector.    */

   OsEnable();                         /* Enable interrupts.                 */
   return SYSOK;                       /* Return with no errors.             */
}



/*---------------------------------------------------------------------------*/
/* OsSleepCheck() -- Called from OsSched() to check for expirations...       */
/*---------------------------------------------------------------------------*/

int   OsSleepCheck( void )
{
   EVENT  *Event;

   OsDisable();                        /* Disable interrupts.                */

   /*------------------------------------------------------------------------*/
   /* Check for expired events, look at top event, if expired, then ready it.*/
   /*------------------------------------------------------------------------*/

   while ((Event = ChainFirst(&EventAnchor)) != NULL) {

      if ( (Event->Seconds <  Seconds ) ||
           (Event->Seconds == Seconds && Event->Millisecs <= Millisecs) ) {

         OsAwake( Event->Pid );        /* Put task in ready chain.           */

      } else {
         break;
      }
   }

   OsEnable();                         /* Enable interrupts.                 */
   return SYSOK;                       /* Return with no errors.             */
}



/*---------------------------------------------------------------------------*/
/* OsSleep() -- Make process sleep for a period of time...                   */
/*---------------------------------------------------------------------------*/

int   OsSleep(long Hundreds)
{
   EVENT   *Event, *p, *q;
   PROCESS *Process;


   OsDisable();                        /* Disable interrupts.                */

   Event = OsAlloc( sizeof(EVENT));    /* Allocate an event block.           */
   ChainInit( &Event->Link, Event );   /* Initialize link fields.            */
   Event->Seconds   = Seconds   + (Hundreds / 100);
   Event->Millisecs = Millisecs + ((Hundreds % 100) * 10);
   Event->Pid       = CurrPid;         /* Save our pid.                      */

   Process = OsHandProtect(ProcessAnchor, OsGetPid());

   p = ChainFirst( &EventAnchor );     /* Get first event in chain.          */
   q = NULL;
   while ( p ) {

      if ((Event->Seconds <  p->Seconds) ||
          (Event->Seconds == p->Seconds && Event->Millisecs <= p->Millisecs)) {
         break;
      }
      q = p;                           /* Make current previous event.       */
      p = ChainNext( &p->Link );       /* Run chain to next event.           */
   }

   if (q)
      Chain( &EventAnchor, &q->Link, &Event->Link );
   else
      Chain( &EventAnchor, NULL,     &Event->Link );

   Unchain( &ReadyAnchor, &(Process->Link)); /* Remove us from ready chain.  */
   Process->State = PRSLEEP;           /* Say process is sleeping.           */

   OsHandUnprotect(ProcessAnchor, Process->Pid);

   OsEnable();                         /* Enable interrupts.                 */
   OsSched();

   return SYSOK;                       /* Return with no errors.             */
}



/*---------------------------------------------------------------------------*/
/* OsAwake() -- Wakeup a sleeper...                                          */
/*---------------------------------------------------------------------------*/

int   OsAwake(HANDLE Pid )
{
   EVENT    *Event;
   PROCESS  *Process;

   OsDisable();                        /* Disable interrupts.                */
   Event = ChainFirst( &EventAnchor ); /* Get first sleeper.                 */

   while (Event) {
      if (Event->Pid == Pid) {
         Unchain( &EventAnchor, &Event->Link );
         OsFree( Event );
         Process = OsHandProtect(ProcessAnchor, Pid);
         Process->State = PRSUSP;      /* In-between state.                  */
         OsHandUnprotect(ProcessAnchor, Pid);
         OsReady( Pid );               /* Put task in ready chain.           */
         OsEnable();                   /* Enable interrupts.                 */
         return SYSOK;
      }

      Event = ChainNext( &Event->Link );
   }

   OsEnable();                         /* Enable interrupts.                 */
   return SYSERR;                      /* Return, pid not found.             */
}



/*---------------------------------------------------------------------------*/
/* TimerTick() -- increment time interval counters...                        */
/*---------------------------------------------------------------------------*/

static void interrupt TimerTick(void)
{
   Old_Timer_Vector();

   Millisecs += 55;
   if (Millisecs >= 990) {
      Seconds++;
      Millisecs = 0;
   }
}

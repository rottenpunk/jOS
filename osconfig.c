/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                              OS KERNEL                                    */
/*                                                                           */
/*                  COPYRIGHT (c) 1994 by JOHN C. OVERTON                    */
/*              Advanced Communication Development Tools, Inc                */
/*                                                                           */
/*                                                                           */
/*            Module:  CONFIG.C                                              */
/*                                                                           */
/*             Title:  Configuration file.                                   */
/*                                                                           */
/*       Description:  Contains global configuration variables.              */
/*                                                                           */
/*            Author:  John C. Overton                                       */
/*                                                                           */
/*              Date:  04/23/94                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#include "oskernel.h"


/*---------------------------------------------------------------------------*/
/* Device tables...                                                          */
/*---------------------------------------------------------------------------*/

struct DeviceType DeviceTypeTable[] = {

   {"PORT1",  0x3f8,  0,  4,  0,  0},
   {"PORT2",  0x2f8,  0,  3,  0,  0},
   {"PORT3",  0x3e8,  0,  4,  0,  0},
   {"PORT4",  0x2e8,  0,  3,  0,  0},
   {NULL,         0,  0,  0,  0,  0}
};

extern CommOpen(    DEVICE *Device, int options);
extern CommClose(   DEVICE *Device);
extern CommRecv(    DEVICE *Device, char *Buffer, int Length);
extern CommSend(    DEVICE *Device, char *Buffer, int Length);
extern CommControl( DEVICE *Device, int Function, long Value);


struct DeviceDriver DeviceDriverTable[] = {

   {NULL, NULL, CommOpen, CommClose, CommRecv, CommSend, CommControl, NULL},
   {-1,   -1,   NULL,     NULL,      NULL,     NULL,     NULL,        NULL}
};


void        *DeviceAnchor = NULL;      /* Device handle manager anchor.      */


/*---------------------------------------------------------------------------*/
/* Process related variables...                                              */
/*---------------------------------------------------------------------------*/

ANCHOR       ReadyAnchor;              /* Chain of ready processes.          */
ANCHOR       KilledAnchor;             /* Chain of killed processes.         */
void        *ProcessAnchor = NULL;     /* Process handle manager anchor.     */
int          NumProc;                  /* Handle to currently active process.*/
HANDLE       CurrPid;                  /* Handle to currently executing proc.*/


/*---------------------------------------------------------------------------*/
/* Interrupt enable/disable variables...                                     */
/*---------------------------------------------------------------------------*/

int          DisableCount;             /* Nested level of interrupts diabled.*/


/*---------------------------------------------------------------------------*/
/* Semaphore related variables...                                            */
/*---------------------------------------------------------------------------*/

void        *SemaphoreAnchor = NULL;   /* Semaphore handle manager anchor.   */


/*---------------------------------------------------------------------------*/
/* Sleep (event) related variables...                                        */
/*---------------------------------------------------------------------------*/

ANCHOR       EventAnchor;              /* Chain of events for sleeping pids. */
ULONG        Seconds;                  /* Time of day in seconds.            */
USHORT       Millisecs;                /* Fraction of a second.              */

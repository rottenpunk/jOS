/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                              OS KERNEL                                    */
/*                                                                           */
/*                  COPYRIGHT (c) 1994 by JOHN C. OVERTON                    */
/*              Advanced Communication Development Tools, Inc                */
/*                                                                           */
/*                                                                           */
/*            Module:  KERNEL.H                                              */
/*                                                                           */
/*             Title:  Os Kernel include file.                               */
/*                                                                           */
/*       Description:  This file includes all definitions for the OS KERNEL. */
/*                                                                           */
/*            Author:  John C. Overton                                       */
/*                                                                           */
/*              Date:  04/23/94                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Common system includes...                                                 */
/*---------------------------------------------------------------------------*/

#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "oscommon.h"
#include "oschain.h"
#include "os.h"



/*---------------------------------------------------------------------------*/
/* Configuration options...                                                  */
/*---------------------------------------------------------------------------*/

#ifndef  NMSG
#define  NMSG         12               /* Set maximum messages queable.      */
#endif



/*---------------------------------------------------------------------------*/
/* Process state constants...                                                */
/*---------------------------------------------------------------------------*/

#define  PRCURR         1              /* Process is currently RUNNING.      */
#define  PRREADY        2              /* Process is on READY queue.         */
#define  PRSUSP         3              /* Process is SUSPENDED.              */
#define  PRWAIT         4              /* Process is on semaphore QUEUE.     */
#define  PRKILL         5              /* Process has been KILLed.           */
#define  PRRECV         6              /* Process waiting to RECEIVE msg.    */
#define  PRSEND         7              /* Process waiting to SEND msg.       */
#define  PRLOCK         8              /* Process is waiting for LOCK.       */
#define  PRSLEEP        9              /* Process is SLEEPING.               */
#define  PRWAKING      10              /* Process is waking up.              */



/*---------------------------------------------------------------------------*/
/* Miscellaneous process definitions...                                      */
/*---------------------------------------------------------------------------*/

#define  PNMLEN      9                 /* Length of process "name".          */
#define  NULLPROC    0                 /* ID of the null process.            */


/*---------------------------------------------------------------------------*/
/* Process table entry...                                                    */
/*---------------------------------------------------------------------------*/

struct Process {
   LINK            Link;               /* Queue of processes.                */
   HANDLE          Pid;                /* Process ID of this process.        */
   char            State;              /* Process state: PRCURR, etc.        */
   short           Prio;               /* Process priority.                  */
   BYTE           *Base;               /* Lower base of run time stack.      */
   BYTE           *Stack;              /* Saved stack pointer.               */
   USHORT          StkLen;             /* Stack length.                      */
   char            Name[PNMLEN];       /* Process name.                      */
   short           Flags;              /* Process flags.                     */
   int             Disable;            /* Disable nest count.                */
   HANDLE          Sem;                /* Semaphore if process waiting.      */
   ANCHOR          Msgs;               /* Messages semt to process.          */
   BYTE            MsgCount;           /* Messages presently queued.         */
   HANDLE          Lock;               /* Wait chain for lock.               */
};


typedef struct Process PROCESS;        /* Alternate for process struct.      */


/*---------------------------------------------------------------------------*/
/* Definitions for Flags above...                                            */
/*---------------------------------------------------------------------------*/

#define PROCESS_CANT_KILL      0x80    /* Can not kill this process.         */




/*---------------------------------------------------------------------------*/
/* Semaphore table entry...                                                  */
/*---------------------------------------------------------------------------*/

struct Semaphore {
   int            Count;               /* Semaphore count.                   */
   ANCHOR         WaitList;            /* Chain of waiting processes.        */
};

typedef struct Semaphore SEMAPHORE;    /* Alternate for semaphore struct.    */



/*---------------------------------------------------------------------------*/
/* Event structure...                                                        */
/*---------------------------------------------------------------------------*/

struct Event {
   LINK           Link;                /* Queue of events.                   */
   ULONG          Seconds;             /* Expire when seconds.               */
   USHORT         Millisecs;           /* Expire when millisecs.             */
   HANDLE         Pid;                 /* Process thats sleeping.            */
};

typedef struct Event EVENT;            /* Alternate for event structure.     */



/*---------------------------------------------------------------------------*/
/* Message structure...                                                      */
/*---------------------------------------------------------------------------*/

struct Message {
   LINK           Link;                /* Link of messages.                  */
   BYTE          *Data;                /* Pointer to message.                */
   USHORT         Length;              /* Length of message.                 */
   HANDLE         Pid;                 /* Pid if process is waiting.         */
};

typedef struct Message MESSAGE;        /* Alternate for message struct.      */



/*---------------------------------------------------------------------------*/
/* Device structure for opened device instance...                            */
/*---------------------------------------------------------------------------*/

struct Device {
   HANDLE   Handle;                    /* Device instance handle number.     */
   int      Type;                      /* Device type number.                */
   int      Driver;                    /* Device driver number.              */
   void    *Misc;                      /* Miscellanious data (or pointer to).*/
};

typedef struct Device DEVICE;



/*---------------------------------------------------------------------------*/
/* Device type table. Driver is offset into device driver table...           */
/*---------------------------------------------------------------------------*/

struct DeviceType {
   char    *Name;                      /* Device name.                       */
   int      Port1;                     /* I/O Port #1.                       */
   int      Port2;                     /* I/O Port #1.                       */
   int      Int;                       /* Interrupt number.                  */
   int      DMA;                       /* DMA number to use.                 */
   int      Driver;                    /* Driver number into driver table.   */
};

typedef struct DeviceType DEVICETYPE;

extern struct DeviceType DeviceTypeTable[];



/*---------------------------------------------------------------------------*/
/* Device Driver table. Defines device drivers...                            */
/*---------------------------------------------------------------------------*/

struct DeviceDriver {
   int    (*Init)(    int i);
   int    (*Term)(    int i);
   int    (*Open)(    DEVICE *Device, int Options);
   int    (*Close)(   DEVICE *Device );
   int    (*Read)(    DEVICE *Device, char *Buffer, int Length);
   int    (*Write)(   DEVICE *Device, char *Buffer, int Length);
   int    (*Control)( DEVICE *Device, int Function, long Value);
   int    (*Seek)(    DEVICE *Device, long Position);
};

typedef struct DeviceDriver DEVICEDRIVER;

extern struct DeviceDriver DeviceDriverTable[];



/*---------------------------------------------------------------------------*/
/* external definitions in CONFIG.C...                                       */
/*---------------------------------------------------------------------------*/

extern ANCHOR     ReadyAnchor;         /* Anchor of ready processes.         */
extern ANCHOR     KilledAnchor;        /* Anchor of killed processes.        */
extern void      *ProcessAnchor;       /* Handle anchor for process handles. */
extern int        NumProc;             /* Currently active processes.        */
extern HANDLE     CurrPid;             /* Currently executing process.       */

extern int        DisableCount;        /* Count of OsDisable nestings.       */

extern void      *SemaphoreAnchor;     /* Handle anchor for sem handles.     */

extern ANCHOR     EventAnchor;         /* Anchor of sleep events.            */
extern ULONG      Seconds;             /* Date in seconds.                   */
extern USHORT     Millisecs;           /* Fraction of current second.        */

extern void      *DeviceAnchor;        /* Anchor for Device instance handles.*/


/*---------------------------------------------------------------------------*/
/* Routines internal to the Kernel...                                        */
/*---------------------------------------------------------------------------*/

void      OsSwitch(     BYTE **Stack1, BYTE **Stack2); /* Switch context.    */

int       OsSleepInit(  void );        /* Initialize Sleep functions.        */
int       OsSleepTerm(  void );        /* Terminate Sleep functions.         */
int       OsSleepCheck( void );        /* Check for expired sleepers.        */
int       OsReady(      HANDLE  Pid);  /* Make process ready to run.         */
int       OsDevInit(    void );        /* Initialize device functions.       */
int       OsDevTerm(    void );        /* Terminate device functions.        */
void     *OsHandFind(   void *A, HANDLE  Nbr);    /* Find handle, rtn resrce.*/


/*---------------------------------------------------------------------------*/
/* Definitions for interrupt service routines...                             */
/*---------------------------------------------------------------------------*/

#define   OsIntEnter()     (DisableCount++)
#define   OsIntExit()      (DisableCount--)




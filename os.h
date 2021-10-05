/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                              OS KERNEL                                    */
/*                                                                           */
/*                  COPYRIGHT (c) 1994 by JOHN C. OVERTON                    */
/*              Advanced Communication Development Tools, Inc                */
/*                                                                           */
/*                                                                           */
/*            Module:  OS.H                                                  */
/*                                                                           */
/*             Title:  OS KERNEL Function prototypes.                        */
/*                                                                           */
/*       Description:  Define all user available functions.                  */
/*                                                                           */
/*            Author:  John C. Overton                                       */
/*                                                                           */
/*              Date:  04/26/94                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Important definitions...                                                  */
/*---------------------------------------------------------------------------*/

#define SYSERR      -1                      /* Return code = error.          */
#define SYSOK        0                      /* Return code = good.           */

#define SYSNOMSG     1                      /* No messages to receive.       */

typedef unsigned long  HANDLE;              /* Universal OS handle.          */

/*---------------------------------------------------------------------------*/
/* Available functions...                                                    */
/*---------------------------------------------------------------------------*/

void     *OsAlloc(      int     Length);    /* Allocate a block of memory.   */

int       OsAwake(      HANDLE  Pid );      /* Wakeup a specific sleeper.    */

int       OsClose(      HANDLE  FileNbr );  /* Close connection to device.   */

int       OsControl(    HANDLE  FileNbr,    /* Control device.               */
                        int     Function,
                        long    Value  );

HANDLE    OsCreate(                         /* Create Process.               */
                        void     *ProcAddr, /* Procedure address.            */
                        int       SSize,    /* Stack size in words.          */
                        int       Priority, /* Process priority >= 0.        */
                        char     *Name,     /* Name ( for debugging ).       */
                        char     *Data );   /* parameter passed to proc.     */

void      OsDisable(    void );             /* Disable interrupts.           */

void      OsEnable(     void );             /* Enable interrupts.            */

int       OsFree(       void *);            /* Free a block of memory.       */

HANDLE    OsGetPid(     void );             /* Get current process id.       */

HANDLE    OsHandCreate(void **A, void *Resource); /* Create a new handle.    */

void     *OsHandDestroy(void *A, HANDLE  Nbr);    /* Destroy a handle.       */

void     *OsHandProtect(void *A, HANDLE  Nbr);    /* Protect handle rtn res. */

int       OsHandUnprotect( void *A, HANDLE  Nbr); /* Unprotect handle.       */

int       OsInit(       void );             /* Initialize OS KERNEL.         */

int       OsKill(       HANDLE  Pid);       /* Kill a process.               */

int       OsLock(       HANDLE *Lock);      /* Lock a resource.              */

int       OsMsgRecv(    void  **Data,       /* Receive a message.            */
                        int    *Length,
                        int     Wait);

int       OsMsgSend(    HANDLE  Pid,        /* Send a message to a process.  */
                        void   *Data,
                        int     Length,
                        int     Wait);

HANDLE    OsOpen(       char   *Name,       /* Open connection to device.    */
                        int     Options );

int       OsPost(       HANDLE  Sem);       /* Post a semaphore.             */

int       OsRead(       HANDLE  FileNbr,    /* Read to device.               */
                        char   *Buffer,
                        int     Length );

int       OsResume(     HANDLE  Pid);       /* Unsuspend process. Make ready.*/

int       OsReturn(     void );             /* Kills currently running proc. */

int       OsSched(      void );             /* Reshedule running process.    */

int       OsSeek(       HANDLE  FileNbr,    /* Seek on device.               */
                        long    Position );

int       OsSleep(      long    Hundreds);  /* Wait for a period of time.    */

int       OsSuspend(    HANDLE  Pid);       /* Suspend process.              */

HANDLE    OsSemCreate(  int     Count);     /* Create a semaphore, set count.*/

int       OsSemDelete(  HANDLE  Sem);       /* Delete a semaphore.           */

int       OsTerm(       void );             /* Terminate OS KERNEL.          */

int       OsWait(       HANDLE  Sem);       /* Wait on a semaphore.          */

int       OsWrite(      HANDLE  FileNbr,    /* Write to device.              */
                        char   *Buffer,
                        int     Length  );

int       OsUnlock(     HANDLE *Lock);      /* Unlock a resource.            */


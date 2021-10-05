
# jOS -- A small operating system for an embedded system (c) 1994-2021, John Overton              

  
This is an earlier embedded operating system that I originally wrote for a project on an 80186 processor with
plans to use it on a future 80486 project.  The original project was a communications device that translated 
IBM mainframe 3270 datastreams into various ANSI/ASCII terminals. (Yea, I know, right?)

jOS supports: 

- processes (threads) 
- counting semaphores 
- locks 
- messages 
- device interface (similar to Unix)
- a "handle" resource manager 

The device interface simply puts a usable interface to device managers, providing an 
open/close/read/write/control/seek frontend to drivers.  The "handle" resoruce manager
can associate a resource, such as a process to a handle and its purpose is to 
prevent resources from being destroyed if they are being used by other processes or interrupt 
routines.  

Include os.h in modules that require interacting with jOS and you have access to these routines:


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



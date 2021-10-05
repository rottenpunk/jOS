//----------------------------------------------------------------------------
//
//                              OS KERNEL
//
//                  COPYRIGHT (c) 1994 by JOHN C. OVERTON
//              Advanced Communication Development Tools, Inc
//
//
//            Module:  OSHANDLE.C
//
//             Title:  Manage resources by handle number.
//
//       Description:  This module contains:
//
//                     OsBuffAlloc   - Allocate a buffer.
//                     OsBuffFree    - Free a buffer.
//                     OsBuff        -
//                     OsBuff        -
//
//            Author:  John C. Overton
//
//              Date:  10/01/95
//
//----------------------------------------------------------------------------

#include "oskernel.h"


#define  OS_BUFFER_BAD       1
#define  OS_BUFFER_TOO_BIG   2
#define  OS_BUFFER_ID        "BUFR"   // ID at start of buffer header.


//----------------------------------------------------------------------------
// Buffer structure...
//----------------------------------------------------------------------------

typedef struct {
   BYTE     Id[4];                     // Buffer identifier.
   LINK     Link;                      // Link of allocated or free buffers.
   LINK     Queue;                     // Queue of buffers.
   HANDLE   Pid;                       // Owner of this buffer.
   BYTE     AnchorIndex;               // Index into Buffer anchor table.
   BYTE     Reserved;                  // Reserverd for future expansion.
   USHORT   Size;                      // Total size of buffer.
   USHORT   Head;                      // Index to start of data.
   USHORT   Tail;                      // Index to end of data.
   BYTE     Buffer[1];                 // Actual buffer area.
} BUFFER;

typedef struct {
   USHORT   Index;                     // Index into anchor table.
   USHORT   Size;                      // Size of these buffers.
   USHORT   MaxAllow;                  // Maximum allowable allocatable bufs.
   USHORT   AllocCount;                // Nbr of buffers currently allocated.
   USHORT   FreeCount;                 // Nbr of buffers that are free.
   USHORT   WaitCount;                 // Count of tasks currently waiting.
   HANDLE   Event;                     // Incase some one has to wait for one
   ANCHOR   Free;                      // Chain of free buffers.
   ANCHOR   Alloc;                     // Chain of allocated buffers.

} BUFFER_ANCHOR;

BUFFER_ANCHOR BufferAnchor[] = {

   // Index  Size Allow Alloc Free Waiting Event   Free Chain   Alloc Chain
   // -----  ---- ----- ----- ---- ------- -----   ----------   -----------

   {     1,   256,  200,    0,   0,      0,    0,      {NULL},       {NULL} },
   {     2,   512,  100,    0,   0,      0,    0,      {NULL},       {NULL} },
   {     3,  1600,   50,    0,   0,      0,    0,      {NULL},       {NULL} },
   {     4,  4096,    0,    0,   0,      0,    0,      {NULL},       {NULL} },
   {     5,  8192,    5,    0,   0,      0,    0,      {NULL},       {NULL} },
   {     6, 16384,    5,    0,   0,      0,    0,      {NULL},       {NULL} },
   {     7, 32768,    5,    0,   0,      0,    0,      {NULL},       {NULL} },
   {     0,     0,    0,    0,   0,      0,    0,      {NULL},       {NULL} }
};





//----------------------------------------------------------------------------
// OsBuffAlloc() -- Allocate a new buffer...
//----------------------------------------------------------------------------

int  OsBuffAlloc(void **RetBuffer, USHORT Size)
{
   BUFFER        *Buffer;              // New buffer.
   BUFFER_ANCHOR *Anchor;              // Pointer into buffer anchor table.


   Anchor = BufferAnchor;              // Use both pointer and index for speed.

   //--------------------------------------------------------------------------
   // Find anchor table entry that matches size...
   //--------------------------------------------------------------------------
   while (Anchor->Size != 0) {         // Check entries until end of table.
      if (Size <= Anchor->Size) {      // Does req size match this one?
         break;                        // Yes, then we can continue.
      }
      Anchor++;                        // To next anchor table entry.
   }

   //--------------------------------------------------------------------------
   // Check to see if we are allowed to allocate buffers this size...
   //--------------------------------------------------------------------------
   while (Anchor->MaxAllow == 0 && Anchor->Size != 0)
      Anchor++;


   if (Anchor->Size == 0)              // Was request too big?
      return OS_BUFFER_TOO_BIG;        // Yes, tell user.

   OsDisable();                        // Disable interrupts.

   while (1) {

      //----------------------------------------------------------------------
      // See if there is a free buffer to give to user...
      //----------------------------------------------------------------------
      if (Anchor->FreeCount) {               // If there are some available,
         Anchor->FreeCount--;                // Keep track of free buffers.
         Buffer = ChainPop( &Anchor->Free ); // Pop one off free stack.
         Buffer->Head   = 0;                 // Clear head index.
         Buffer->Tail   = 0;                 // Clear tail index.
         break;
      }

      //----------------------------------------------------------------------
      // Try to allocate memory for a new buffer, and give to user...
      //----------------------------------------------------------------------
      if (Anchor->AllocCount < Anchor->MaxAllow) {  // If we're allowed more...
         Buffer = OsAlloc( Anchor->Size +           // Allocate memory for buf.
                           sizeof(BUFFER) - 1 );
         Buffer->Size = Anchor->Size;            // Set buffer size in buf.
         Buffer->AnchorIndex = Anchor->Index;    // Save index into anchor tab.
         memcpy(Buffer->Id, OS_BUFFER_ID, sizeof(Buffer->Id));
         break;
      }

      //----------------------------------------------------------------------
      // Too many buffers have been allocated for this size, make user wait
      // until one becomes available...
      //----------------------------------------------------------------------

      Anchor->WaitCount++;             // Indicate a task is waiting.
      OsEventWait(Anchor->Event);      // Wait until a buffer is free.
      Anchor->WaitCount--;             // Indicate a task is not waiting.
   }

   Anchor->AllocCount++;                       // Keep count of allocated
   ChainPush(&Anchor->Alloc, &Buffer->Link);   // Put bfr on alloc chain.
   OsEnable();                                 // Re-enable interrupts.
   Buffer->Pid = OsGetPid();                   // Save Owner's Pid.
   *RetBuffer = (void **) &(Buffer->Buffer);   // Return buff ptr to caller.

   return SYSOK;
}


//----------------------------------------------------------------------------
// OsBuffFree() -- Free a buffer...
//----------------------------------------------------------------------------

int  OsBuffFree(void *PassBuffer)
{
   BUFFER*        Buffer;
   BUFFER_ANCHOR* Anchor;


   //--------------------------------------------------------------------------
   // Back up to start of buffer header...
   //--------------------------------------------------------------------------
   Buffer = (BUFFER *) ((char*)PassBuffer -
                        (char*) (((BUFFER*)(0))->Buffer));

   Anchor = &BufferAnchor[Buffer->AnchorIndex];  // Get buff anchor table entry.

   if ( memcmp(Buffer->Id, OS_BUFFER_ID, sizeof(Buffer->Id)) != 0)
      return OS_BUFFER_BAD;            // Something wrong with buffer.

   OsDisable();                        // Disable interrupts.

   Unchain(&Anchor->Alloc,  &Buffer->Link);   // Unchain from allocate chain.
   ChainPush(&Anchor->Free, &Buffer->Link);   // Chain into free chain.

   OsEnable();                         // Re-enable interrupts.

   return SYSOK;
}

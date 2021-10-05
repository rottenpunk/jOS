/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                              OS KERNEL                                    */
/*                                                                           */
/*                  COPYRIGHT (c) 1994 by JOHN C. OVERTON                    */
/*              Advanced Communication Development Tools, Inc                */
/*                                                                           */
/*                                                                           */
/*            Module:  OSHANDLE.C                                            */
/*                                                                           */
/*             Title:  Manage resources by handle number.                    */
/*                                                                           */
/*       Description:  This module contains:                                 */
/*                                                                           */
/*                     OsHandCreate  - Allocate a new handle.                */
/*                     OsHandDestroy - Destroy handle.                       */
/*                     OsHandEnqueue - Enqueue a resource given handle.      */
/*                     OsHandDequeue - Dequeue a resource given handle.      */
/*                                                                           */
/*            Author:  John C. Overton                                       */
/*                                                                           */
/*              Date:  11/04/94                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#include "oskernel.h"



/*---------------------------------------------------------------------------*/
/* Handle structures...                                                      */
/*---------------------------------------------------------------------------*/

struct HandleElement {
   USHORT   Reference;                 /* Handle create reference number.    */
   USHORT   Number;                    /* Handle number.                     */
   void    *Resource;                  /* Pointer to resource or free chain. */
   HANDLE   Pid;                       /* Pid of destroyer, if waiting.      */
   USHORT   Use;                       /* Count of users of handle.          */
};


struct HandleSegment {
   struct HandleSegment *Next;         /* Next segment in chain.             */
   struct HandleElement  Handles[256]; /* Handles in segment.                */
};


struct HandleAnchor {
   USHORT HanCount;                    /* Count of allocated handles.        */
   USHORT SegCount;                    /* Count of allocated segments.       */
   struct HandleElement *Free;         /* Chain of free handles.             */
   struct HandleSegment *Segments[256];/* Pointers to segments.              */
};



/*---------------------------------------------------------------------------*/
/* Local routines...                                                         */
/*---------------------------------------------------------------------------*/




/*---------------------------------------------------------------------------*/
/* OsHandCreate() -- Allocate a new handle...                                */
/*---------------------------------------------------------------------------*/

HANDLE OsHandCreate(void **A, void *Resource)
{
   struct HandleAnchor  *Anchor;
   struct HandleSegment *Segment;
   struct HandleElement *Handle;
   int                   i;

   OsDisable();                        /* Disable interrupts.                */

   /*------------------------------------------------------------------------*/
   /* Check to see if Anchor has been allocated yet...                       */
   /*------------------------------------------------------------------------*/
   if ((Anchor = (struct HandleAnchor *) *A) == NULL)
      Anchor = (struct HandleAnchor *)*A = OsAlloc(sizeof(struct HandleAnchor));

   /*------------------------------------------------------------------------*/
   /* See if a handle can be allocated off of free chain. If not, then need  */
   /* to allocate another segment full of handles, preformat them, then      */
   /* allocate one of the new handles...                                     */
   /*------------------------------------------------------------------------*/
   if ( (Handle = Anchor->Free) == NULL) {

      if (Anchor->SegCount < 256) {
         Segment = OsAlloc(sizeof(struct HandleSegment));
         Anchor->Segments[Anchor->SegCount++] = Segment;
         for (i = 0; i < 256; i++ ) {
            Segment->Handles[i].Number    = Anchor->HanCount++;
            Segment->Handles[i].Reference = 1;
            (struct HandleElement *)(Segment->Handles[i].Resource) =
                                                                Anchor->Free;
            Anchor->Free = &(Segment->Handles[i]);
         }
         Handle = Anchor->Free;

      } else
         return SYSERR;                /* Can not allocate another segment.  */
   }

   /*------------------------------------------------------------------------*/
   /* Now, we have a handle. So set it up and pass it back...                */
   /*------------------------------------------------------------------------*/
   Anchor->Free = (struct HandleElement *) Handle->Resource;
   Handle->Resource = Resource;
   Handle->Use = 2;                    /* Indicate in use and protected.     */

   OsEnable();
   return ((ULONG)Handle->Number | ((ULONG)Handle->Reference << 16));
}



/*---------------------------------------------------------------------------*/
/* OsHandDestroy() -- Destroy a handle...                                    */
/*---------------------------------------------------------------------------*/

void *OsHandDestroy(void *A, HANDLE  Nbr)
{
   struct HandleAnchor  *Anchor;
   struct HandleSegment *Segment;
   struct HandleElement *Handle;
   void                 *Resource;


   if ((Anchor = (struct HandleAnchor *) A) == NULL)
      return NULL;

   OsDisable();                        /* Disable interrupts.                */

   if ((Segment = Anchor->Segments[(Nbr >> 8) & 0xff]) != NULL) {
      Handle = &(Segment->Handles[Nbr & 0xff]);
      if (Handle->Reference == Nbr >> 16 &&      /* Reference match?         */
          Handle->Use       >  0) {              /* Not being free'd?        */

         if (Handle->Use-- > 0) {      /* Increment use count.               */
            Handle->Pid = CurrPid;     /* Get our Pid.
            OsSuspend(CurrPid);        /* Wait until all are finished.       */
         } else {
            Handle->Reference++;       /* Bump up reference count.           */
         }
         Resource = Handle->Resource;  /* Save resource.                     */
         (struct HandleElement *) Handle->Resource = Anchor->Free;
         Anchor->Free = Handle;        /* Chain on free chain.               */
         OsEnable();                   /* Enable interrupts.                 */
         return Resource;              /* Return destroyed ok.               */

      }
   }

   OsEnable();                         /* Enable interrupts.                 */
   return NULL;                        /* Return, handle not found.          */
}



/*---------------------------------------------------------------------------*/
/* OsHandProtect() -- Protect a handle from being destroyed...               */
/*---------------------------------------------------------------------------*/

void  *OsHandProtect(void *A, HANDLE  Nbr)
{
   struct HandleElement *Handle;
   struct HandleSegment *Segment;
   struct HandleAnchor  *Anchor;


   if ((Anchor = (struct HandleAnchor *) A) == NULL)
      return NULL;

   OsDisable();                        /* Disable interrupts.                */

   if ((Segment = Anchor->Segments[(Nbr >> 8) & 0xff]) != NULL) {
      Handle = &(Segment->Handles[Nbr & 0xff]);
      if (Handle->Reference == Nbr >> 16 &&   /* Reference match?         */
          Handle->Use       != 0xffff    &&   /* Not max use count?       */
          Handle->Use       >  0) {           /* Not being free'd?        */
         Handle->Use++;             /* Increment use count.               */
         OsEnable();                /* Enable interrupts.                 */
         return Handle->Resource;   /* Return with resource.              */
      }
   }
   OsEnable();                         /* Enable interrupts.                 */
   return NULL;                        /* Return, handle not valid.          */
}



/*---------------------------------------------------------------------------*/
/* OsHandUnprotect() -- Unprotect a handle from being destroyed...           */
/*---------------------------------------------------------------------------*/

int  OsHandUnprotect(void *A, HANDLE  Nbr)
{
   struct HandleElement *Handle;
   struct HandleSegment *Segment;
   struct HandleAnchor  *Anchor;


   if ((Anchor = (struct HandleAnchor *) A) == NULL)
      return NULL;

   OsDisable();                        /* Disable interrupts.                */

   if ((Segment = Anchor->Segments[(Nbr >> 8) & 0xff]) != NULL) {
      Handle = &(Segment->Handles[Nbr & 0xff]);
      if (Handle->Reference == Nbr >> 16 &&   /* Reference match?         */
          Handle->Use       >  0) {           /* Not being free'd?        */
         if (Handle->Use-- == 0) {  /* Increment use count.               */
            OsResume(Handle->Pid);  /* If zero, then unlock destroyer.    */
            Handle->Reference++;    /* Bump up reference count.           */
         }
         OsEnable();                /* Enable interrupts.                 */
         return SYSOK;              /* Return, handle unprotected.        */
      }
   }

   OsEnable();                         /* Enable interrupts.                 */
   return SYSERR;                      /* Return, handle not found.          */
}



/*---------------------------------------------------------------------------*/
/* OsHandFind() -- Locate a handle, return resource...                       */
/*---------------------------------------------------------------------------*/

void  *OsHandFind(void *A, HANDLE  Nbr)
{
   struct HandleElement *Handle;
   struct HandleSegment *Segment;
   struct HandleAnchor  *Anchor;


   if ((Anchor = (struct HandleAnchor *) A) == NULL)
      return NULL;

   OsDisable();                        /* Disable interrupts.                */

   if ((Segment = Anchor->Segments[(Nbr >> 8) & 0xff]) != NULL) {
      Handle = &(Segment->Handles[Nbr & 0xff]);
      if (Handle->Reference == Nbr >> 16 &&   /* Reference match?         */
          Handle->Use       >  0) {           /* Not being free'd?        */
         OsEnable();                /* Enable interrupts.                 */
         return Handle->Resource;   /* Return with resource.              */
      }
   }

   OsEnable();                         /* Enable interrupts.                 */
   return NULL;                        /* Return, handle not found.          */
}

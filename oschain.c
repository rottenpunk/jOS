/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                              OS KERNEL                                    */
/*                                                                           */
/*                  COPYRIGHT (c) 1994 by JOHN C. OVERTON                    */
/*              Advanced Communication Development Tools, Inc                */
/*                                                                           */
/*                                                                           */
/*            Module:  OSCHAIN.C                                             */
/*                                                                           */
/*             Title:  Chain functions.                                      */
/*                                                                           */
/*       Description:  OsChain() and OsUnchain() will manage a doubly linked */
/*                     list of chain elements.                               */
/*                                                                           */
/*            Author:  John C. Overton                                       */
/*                                                                           */
/*              Date:  04/25/94                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#include "oskernel.h"


/*---------------------------------------------------------------------------*/
/* Chain() -- Link in a chain LINK...                                        */
/*---------------------------------------------------------------------------*/

void Chain( ANCHOR  *Anchor,
            LINK    *Old,
            LINK    *New)
{
   New->Next = NULL;
   New->Prev = Old;

   if (Old) {
      New->Next = Old->Next;
      Old->Next = New;
   } else {
      New->Next     = Anchor->First;
      Anchor->First = New;
   }

   if (New->Next)
      New->Next->Prev = New;
   else
      Anchor->Last = New;
}



/*---------------------------------------------------------------------------*/
/* Unchain() -- Unlink a chain LINK...                                       */
/*---------------------------------------------------------------------------*/

void *Unchain( ANCHOR  *Anchor,
               LINK    *Link )
{
   LINK *p, *q;

   p = Link->Prev;
   q = Link->Next;

   if (p)
      p->Next = q;
   else
      Anchor->First = q;

   if (q)
      q->Prev = p;
   else
      Anchor->Last = p;

   Link->Next = Link->Prev = NULL;

   return Link->Element;
}



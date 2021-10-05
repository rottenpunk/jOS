/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                              OS KERNEL                                    */
/*                                                                           */
/*                  COPYRIGHT (c) 1994 by JOHN C. OVERTON                    */
/*              Advanced Communication Development Tools, Inc                */
/*                                                                           */
/*                                                                           */
/*            Module:  OSMEM.C                                               */
/*                                                                           */
/*             Title:  Memory Manager.                                       */
/*                                                                           */
/*       Description:  This module contains:                                 */
/*                                                                           */
/*                     OsAlloc()   - Allocate a block of memory.             */
/*                     OsFree()    - Free a block of memory.                 */
/*                                                                           */
/*                                                                           */
/*            Author:  John C. Overton                                       */
/*                                                                           */
/*              Date:  01/02/95                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#include "oskernel.h"



/*---------------------------------------------------------------------------*/
/* OsAlloc() -- Allocate a block of storage...                               */
/*---------------------------------------------------------------------------*/

void *OsAlloc(int Length)
{
   return calloc(Length, 1);           /* Return                             */
}



/*---------------------------------------------------------------------------*/
/* OsFree() -- Free a block of storage...                                    */
/*---------------------------------------------------------------------------*/

int   OsFree(void *p)
{

   free(p);                            /* Free memory block.                 */
   return SYSOK;                       /* Return, no errors.                 */
}

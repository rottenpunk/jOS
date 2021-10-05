/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                              OS KERNEL                                    */
/*                                                                           */
/*                  COPYRIGHT (c) 1994 by JOHN C. OVERTON                    */
/*              Advanced Communication Development Tools, Inc                */
/*                                                                           */
/*                                                                           */
/*            Module:  OSENABLE.C                                            */
/*                                                                           */
/*             Title:  Enable/Disable interrupts.                            */
/*                                                                           */
/*       Description:  OsEnable() will enable interrupts if no other         */
/*                     callers have it disabled.                             */
/*                                                                           */
/*                     OsDisable() will disable interrupts and keep count    */
/*                     disablers.                                            */
/*                                                                           */
/*            Author:  John C. Overton                                       */
/*                                                                           */
/*              Date:  04/23/94                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#include "oskernel.h"



/*---------------------------------------------------------------------------*/
/* OsEnable  --  Enable interrupts...                                        */
/*---------------------------------------------------------------------------*/


void OsEnable( void )

{
   if (!--DisableCount) {              /* If count is down to zero...        */
      enable();                        /* .. enable interrupts.              */
   }
}



/*---------------------------------------------------------------------------*/
/* OsDisable --  Disable interrupts...                                       */
/*---------------------------------------------------------------------------*/


void OsDisable( void )

{
   disable();                          /* Disable interrupts.                */
   DisableCount++;                     /* Keep count of callers.             */
}


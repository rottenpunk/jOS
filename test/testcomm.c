/*---------------------------------------------------------------------------*/
/*                                                                           */
/*               *******************************************                 */
/*               *                                         *                 */
/*               *              OS KERNEL                  *                 */
/*               *                                         *                 */
/*               *  COPYRIGHT (c) 1994 by JOHN C. OVERTON  *                 */
/*               *                                         *                 */
/*               *******************************************                 */
/*                                                                           */
/*            Module:  TESTCOMM.C                                            */
/*                                                                           */
/*             Title:  Test OSCOMM                                           */
/*                                                                           */
/*       Description:  Test the OS KERNEL communications routines.           */
/*                                                                           */
/*            Author:  John C. Overton                                       */
/*                                                                           */
/*              Date:  11/21/94                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "os.h"

static void Task1( char far *Data );

HANDLE   fd;



static void Task1( char far *Data )
{
   int      i;
   char     Buffer[80];


   printf("Here in %s\n", Data);

   OsWrite(fd, "It this thing working?\n", 23);
   i = OsRead(fd, Buffer, sizeof(Buffer));

   printf("Length = %d, %s\n", i, Buffer);

   return 0;
}




void main ()
{

   OsInit();                           /* Initialize kernel.                 */

   if ( OsCreate( Task1, 512, 10, "Task1", "Task1") == -1) {
      fprintf(stderr, "OsCreate() 1 error\n");
      exit(1);
   }

   fd = OsOpen("PORT1", 0);

   while (1) {

      OsSched();

      if (kbhit())
         if (toupper(getch()) == 'Y')
            break;

   }

   OsClose(fd);                        /* Close port.                        */

   OsTerm();                           /* Terminate OS KERNEL.               */
}

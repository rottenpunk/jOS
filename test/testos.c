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
/*            Module:  TESTOS.C                                              */
/*                                                                           */
/*             Title:  Test OS KERNEL                                        */
/*                                                                           */
/*       Description:  Test the OS KERNEL routines.                          */
/*                                                                           */
/*            Author:  John C. Overton                                       */
/*                                                                           */
/*              Date:  04/26/94                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "os.h"

static void Task1( char far *Data );
static void Task2( char far *Data );
static void Task3( char far *Data );
static void Task4( char far *Data );

static HANDLE Sem1;
static HANDLE Sem2;
static HANDLE Sem3;


void main ()
{
   HANDLE  Pid;

   OsInit();                           /* Initialize kernel.                 */


   if ( (Sem1 = OsSemCreate(0)) == -1) {
      fprintf(stderr, "OsSemCreate() 1 error\n");
      exit(1);
   }

   if ( (Sem2 = OsSemCreate(0)) == -1) {
      fprintf(stderr, "OsSemCreate() 2 error\n");
      exit(1);
   }

   if ( (Sem3 = OsSemCreate(0)) == -1) {
      fprintf(stderr, "OsSemCreate() 3 error\n");
      exit(1);
   }

   if ( (Pid = OsCreate( Task1, 512, 10, "Task1", "Task1")) == -1) {
      fprintf(stderr, "OsCreate() 1 error\n");
      exit(1);
   }

   fprintf(stderr, "Created Task %08lX\n", Pid);

   if ( (Pid = OsCreate( Task2, 512, 10, "Task2", "Task2")) == -1) {
      fprintf(stderr, "OsCreate() 2 error\n");
      exit(1);
   }

   fprintf(stderr, "Created Task %08lX\n", Pid);

   if ( (Pid = OsCreate( Task3, 512, 10, "Task3", "Task3")) == -1) {
      fprintf(stderr, "OsCreate() 3 error\n");
      exit(1);
   }

   fprintf(stderr, "Created Task %08lX\n", Pid);

   if ( (Pid = OsCreate( Task4, 512, 10, "Task4", "Task4")) == -1) {
      fprintf(stderr, "OsCreate() 4 error\n");
      exit(1);
   }

   fprintf(stderr, "Created Task %08lX\n", Pid);

   while (1) {

      OsSched();

      if (kbhit())
         if (toupper(getch()) == 'Y')
            break;

   }

   OsTerm();                           /* Terminate OS KERNEL.               */
}



static void Task1( char far *Data )
{
   int i;

   printf("Here in %s\n", Data);

   printf("Before 100 wait\n");
   OsSleep(100);
   printf("After 100 wait\n");

   for (i = 0; i < 1000; i++) {
      OsSleep(100);
      printf("wake up After 100 wait\n");
   }

   OsSuspend( OsGetPid() );
}


static void Task2( char far *Data )
{
   printf("Here in %s\n", Data);
   printf("Before 200 wait\n");
   OsSleep(200);
   printf("After 200 wait\n");
   printf("Task %s waiting on Sem %d\n", Data, Sem1 );
   OsWait(Sem1);
   printf("Task %s return from waiting on Sem %d\n", Data, Sem1 );
   OsSuspend( OsGetPid() );
}


static void Task3( char far *Data )
{
   printf("Here in %s\n", Data);
   printf("Before 300 wait\n");
   OsSleep(300);
   printf("After 300 wait\n");
   OsPost(Sem1);
   OsSuspend( OsGetPid() );
}


static void Task4( char far *Data )
{
   int i;

   printf("Here in %s\n", Data);
   printf("Before 400 wait\n");
   OsSleep(400);
   printf("After 400 wait\n");

   for (i = 0; i < 1000; i++) {
      OsSleep(10);
      printf("wake up After 10 wait\n");
   }

   OsSuspend( OsGetPid() );
}

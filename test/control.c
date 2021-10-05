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
#include <ctype.h>
#include <dos.h>
#include <conio.h>

#include "boolean.h"

#include "os.h"


#define  BIOS_DATA_PAGE   0x0040
#define  LPT1             0x0008
#define  LPT2             0x000A


#define  DATA_PORT(Port)    (Port)
#define  STATUS_PORT(Port)  (Port + 1)
#define  CONTROL_PORT(Port) (Port + 2)




/*---------------------------------------------------------------------------*/
/* Global data...                                                            */
/*---------------------------------------------------------------------------*/

unsigned short far *PortPtr;
unsigned short Port;
int   Time = 1;
static char  CurrentByte[8] = {0,0,0,0,0,0,0,0};


#define MATCH(a, b) (strcmp(a, b) == 0)


/*---------------------------------------------------------------------------*/
/* Local internal functions...                                               */
/*---------------------------------------------------------------------------*/

void  SendByte(char Address, char Data);
char  ReadByte(char Address);
Bool  TestSwitch(int Nbr);
void  SetSwitch(int Nbr);
void  ResetSwitch(int Nbr);

static void Task1( char far *Data );
static void Task2( char far *Data );

static int Sem1;
static int Sem2;
static int Sem3;


void main ()
{

   PortPtr = MK_FP(BIOS_DATA_PAGE, LPT1);
   Port = *PortPtr;

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

   if ( OsCreate( Task1, 512, 10, "Task1", "Task1") == -1) {
      fprintf(stderr, "OsCreate() 1 error\n");
      exit(1);
   }

   if ( OsCreate( Task2, 512, 10, "Task2", "Task2") == -1) {
      fprintf(stderr, "OsCreate() 2 error\n");
      exit(1);
   }


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


   printf("Printer Port = %02X\n", Port);

	SetSwitch(1);

   while(1) {

      for (i=0; i < 50; i++ ) {
      	SetSwitch(0);
         ResetSwitch(0);
      }
      OsSleep(6000);

      // SendByte(0, TestData1++);
      // SendByte(1, TestData2--);

      // TestData3 = ReadByte(4);
      // TestData4 = ReadByte(5);

      //printf("Port 4 = %02X, Port 5 = %02X\n", TestData3, TestData4);

      OsSleep(6000);

   }

}


static void Task2( char far *Data )
{
   printf("Here in %s\n", Data);

   while (1) {

   	if (TestSwitch(0)) {

         printf("Switch 0 is now on\n");
   		SetSwitch(2);
         OsSleep(2000);
   		ResetSwitch(2);

      }
      OsSleep(100);
   }
}




/*---------------------------------------------------------------------------*/
/* SendByte() - Process a command...                                         */
/*---------------------------------------------------------------------------*/

void  SendByte(char Address, char Data)
{

   outportb(DATA_PORT(Port), Address); /* Set device address.                */
   outportb(CONTROL_PORT(Port), 0x01); /* Latch address into LS137.          */
   outportb(CONTROL_PORT(Port), 0x00); /* Latch address into LS137.          */
   outportb(DATA_PORT(Port), Data);    /* Put data onto our bus.             */
   outportb(CONTROL_PORT(Port), 0x02); /* Enable output of LS137.            */
   outportb(CONTROL_PORT(Port), 0x00); /* Latch output of data into a LS573. */

}


/*---------------------------------------------------------------------------*/
/* ReadByte() - Process a command...                                         */
/*---------------------------------------------------------------------------*/

char  ReadByte(char Address)
{
   char  Data;

   outportb(DATA_PORT(Port), Address); /* Set device address.                */
   outportb(CONTROL_PORT(Port), 0x01); /* Latch address into LS137.          */
   outportb(CONTROL_PORT(Port), 0x02); /* Enable output of LS137.            */
   Data = inportb(STATUS_PORT(Port)) >> 4;    /* Read low nibble of data.    */
   outportb(CONTROL_PORT(Port), 0x0a);   /* set to get other nibble of data. */
   Data |= inportb(STATUS_PORT(Port)) & 0xf0; /* Get high nibble of data.    */
   outportb(CONTROL_PORT(Port), 0x00); /* Reset control port.                */

   return Data;
}


Bool TestSwitch(int Nbr)
{
	char   Addr;

	Addr = (Nbr / 8) + 4;

	return ( (ReadByte(Addr) & (1 << (Nbr % 8) )) != 1 );
}


void  SetSwitch(int Nbr)
{
	char  	Addr;

	Addr = (Nbr / 8);
   CurrentByte[Addr] |= ( 1 << (Nbr % 8));
   SendByte(Addr, CurrentByte[Addr] );

}

void  ResetSwitch(int Nbr)
{
	char  	Addr;

	Addr = (Nbr / 8);
   CurrentByte[Addr] &= ~( 1 << (Nbr % 8));
   SendByte(Addr, CurrentByte[Addr] );

}


/*---------------------------------------------------------------------------*/
/* Hex() - Convert a string from hexadecimal to integer...                   */
/*---------------------------------------------------------------------------*/

Bool Hex(char *Str, unsigned int   *Result)
{
   char c;

   *Result = 0;
   while (1) {
      if (*Str == '\0')
         break;
      if (!isxdigit(*Str))
         return False;

      *Result *= 16;
      c = toupper(*(Str++));
      if (c >= 'A' && c <= 'F')
         *Result += c - 'A' + 10;
      else
         *Result += c - '0';
   }
   return True;
}

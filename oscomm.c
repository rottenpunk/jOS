/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                              OS KERNEL                                    */
/*                                                                           */
/*                  COPYRIGHT (c) 1994 by JOHN C. OVERTON                    */
/*              Advanced Communication Development Tools, Inc                */
/*                                                                           */
/*                                                                           */
/*           Program:  COMM.C                                                */
/*                                                                           */
/*             Title:  COM port driver.                                      */
/*                                                                           */
/*       Description:  Hardware driver for the 8250 type COM ports on PC     */
/*                     screen.                                               */
/*                                                                           */
/*            Author:  John C. Overton                                       */
/*                                                                           */
/*              Date:  03/19/94                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

//#include <dos.h>
//#include <bios.h>
//#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "oscomm.h"
#include "oskernel.h"

#define OUTP(p, d) outportb(p, d)
#define INP(p)     inportb(p)


/*---------------------------------------------------------------------------*/
/* COM port structure created for each open com port...                      */
/*---------------------------------------------------------------------------*/

struct Port {
   struct Port  *Next;                 /* Next Port using same INT.          */
   USHORT        Addr;                 /* 8250 Base I/O port address.        */
   USHORT        Int;                  /* Interrupt number.                  */

   USHORT        Flags;                /* Various flags, see below.          */
   BYTE          Options;              /* Various options, see below.        */

   BYTE          Error;                /* Any errors received.               */
   BYTE          Status;               /* Current Modem lines.               */

   int           XOffPt;               /* Amount of buffer that forces XOFF. */
   int           XOnPt;                /* Amount of buffer that unXOFFs.     */
   char          XOnChar;              /* XON character.                     */
   char          XOffChar;             /* XOFF character.                    */
   char          TermChar;             /* Line terminating character.        */

   int           RecvCnt;              /* Count of chars received in RecvBuf.*/
   int           RecvLen;              /* Size of recieve buffer.            */
   int           RecvIn;               /* Input index into receive buffer.   */
   int           RecvOut;              /* Output index into receive buffer.  */
   char         *RecvBuf;              /* Receive buffer.                    */
   HANDLE        RecvLock;             /* Receive waiters lock.              */
   HANDLE        RecvPid;              /* Process currently receiving.       */

   int           SendCnt;              /* Count of chars send from SendBuf.  */
   int           SendLen;              /* Size of send buffer.               */
   char         *SendBuf;              /* Send buffer.                       */
   HANDLE        SendLock;             /* Send waiters lock.                 */
   HANDLE        SendPid;              /* Process currently sending.         */
};

typedef struct Port PORT;



/*---------------------------------------------------------------------------*/
/* Definitions for Flags above...                                            */
/*---------------------------------------------------------------------------*/

#define FLAG_XOFF_RECEIVED  0x8000     /* Inbound XOFF received.             */
#define FLAG_XOFF_SENT      0x4000     /* Outbound XOFF sent.                */
#define FLAG_XOFF_PENDING   0x2000     /* Outbound XOFF sent.                */
#define FLAG_XON_PENDING    0x1000     /* Outbound XOFF sent.                */

#define FLAG_RECV_BUF_FULL  0x0800     /* Recv interrupt but buffer was full.*/
#define FLAG_RTS_OFF        0x0400     /* RTS was set off 'cause buf was ful.*/
#define FLAG_SENDER_WAITING 0x0200     /* Sender is waiting.                 */
#define FLAG_RECVER_WAITING 0x0100     /* Receiver is waiting.               */

#define FLAG_SEND_DISABLE   0x0080     /* Disable sending data (XOFF recved).*/



/*---------------------------------------------------------------------------*/
/* Definitions for Options above...                                          */
/*---------------------------------------------------------------------------*/

#define OPTION_SOFT_FLOW    0x80       /* Software XON/XOFF supported.       */
#define OPTION_HARD_FLOW    0x40       /* Hardware RTS off when buf gets full*/
#define OPTION_TERM_CHAR    0x20       /* Receive up to terminate character. */


/*---------------------------------------------------------------------------*/
/* Useful definitions...                                                     */
/*---------------------------------------------------------------------------*/

#define XON                 0x11       /* XON character.                     */
#define XOFF                0x13       /* XOFF character.                    */
#define CR                  '\n'       /* Carraige return character.         */


/*---------------------------------------------------------------------------*/
/* Various definitions...                                                    */
/*---------------------------------------------------------------------------*/

#define COMM_BUFFER_LENGTH  128        /* Default receive buffer.            */

#define INTCONT1_OCW1       0x21       /* 8259 interrupt controller OCW1.    */
#define INTCONT1_OCW2       0x20       /* 8259 interrupt controller OCW2.    */

#define INTCONT2_OCW1       0xa1       /* 8259 interrupt controller OCW1.    */
#define INTCONT2_OCW2       0xa0       /* 8259 interrupt controller OCW2.    */

#define CHAIN_IRQBIT        0x04       /* Chained Int controller IRQ bit.    */



/*---------------------------------------------------------------------------*/
/* 8250 register and bit setting definitions...                              */
/*---------------------------------------------------------------------------*/

#define DATA_PORT(Port)     (Port->Addr)

/*---------------------------------------------------------------------------*/
/* Interrupt enable register...  (Base + 1)                                  */
/*---------------------------------------------------------------------------*/

#define IER_PORT(port)      (Port->Addr + 1)

#define RECEIVED_DATA_AVAILABLE    0x01
#define TRANSMIT_HOLD_EMPTY        0x02
#define RECEIVER_LINE_STATUS       0x04
#define MODEM_STATUS               0x08

/*---------------------------------------------------------------------------*/
/* Interrupt Identify register (read only)...   (Base + 2)                   */
/*---------------------------------------------------------------------------*/

#define IIR_PORT(Port)      (Port->Addr + 2)

#define INTERRUPT_PENDING_0        0x01
#define INTERRUPT_ID_BIT_1         0x02
#define INTERRUPT_ID_BIT_2         0x04

#define INTERRUPT_MODEM            0x00
#define INTERRUPT_TRANSMITTER      INTERRUPT_ID_BIT_1
#define INTERRUPT_RECEIVER         INTERRUPT_ID_BIT_2
#define INTERRUPT_LINE_STATUS      (INTERRUPT_ID_BIT_1 | INTERRUPT_ID_BIT_2)

/*---------------------------------------------------------------------------*/
/* Line control register...    (Base + 3)                                    */
/*---------------------------------------------------------------------------*/

#define LCR_PORT(Port)  (Port->Addr + 3)

#define WORD_LENGTH_SELECT_1       0x01   /* 00 = 5 bits, 01 = 6 bits.       */
#define WORD_LENGTH_SELECT_2       0x02   /* 10 = 7 bits, 11 = 8 bits.       */
#define NUMBER_STOP_BITS           0x04   /*  0 = 1 stop,  1 = 2 stop.       */
#define PARITY_ENABLE              0x08
#define EVEN_PARITY_SELECT         0x10
#define STICK_PARITY               0x20
#define SET_BREAK                  0x40
#define DIVISOR_LATCH_BIT          0x80

/*---------------------------------------------------------------------------*/
/* Modem control register...   (Base + 4)                                    */
/*---------------------------------------------------------------------------*/

#define MCR_PORT(Port)   (Port->Addr + 4)

#define DTR                        0x01
#define RTS                        0x02
#define OUT_1                      0x04
#define OUT_2                      0x08
#define LOOP                       0x10

/*---------------------------------------------------------------------------*/
/* Line status register...     (Base + 5)                                    */
/*---------------------------------------------------------------------------*/

#define LSR_PORT(Port)   (Port->Addr + 5)

#define DATA_READY                 0x01
#define OVERRUN_ERROR              0x02
#define PARITY_ERROR               0x04
#define FRAMING_ERROR              0x08
#define BREAK_INTERRUPT            0x10
#define TRANS_HOLDING_REGISTER     0x20
#define TRANS_SHIFT_REGISTER       0x40

/*---------------------------------------------------------------------------*/
/* Modem status register...    (Base + 6)                                    */
/*---------------------------------------------------------------------------*/

#define MSR_PORT(Port)   (Port->Addr + 6)

#define DCTS                       0x01
#define DDSR                       0x02
#define TERI                       0x04
#define DDCD                       0x08
#define CTS                        0x10
#define DSR                        0x20
#define RI                         0x40
#define DCD                        0x80



/*---------------------------------------------------------------------------*/
/* Static subroutines local to this module...                                */
/*---------------------------------------------------------------------------*/

static int  CommCheckErrors(PORT *Port);
static int  CommModemStatus(PORT *Port);

static void ProcessInt(int Int);       /* Second level interrupt routine.    */

//void interrupt  SerInt3(  void );      /* Handle Interrupts for INT 3.       */
//void interrupt  SerInt4(  void );      /* Handle Interrupts for INT 4.       */
//void interrupt  SerInt5(  void );      /* Handle Interrupts for INT 5.       */
//void interrupt  SerInt7(  void );      /* Handle Interrupts for INT 7.       */
//void interrupt  SerInt10( void );      /* Handle Interrupts for INT 10.      */
//void interrupt  SerInt11( void );      /* Handle Interrupts for INT 11.      */
//void interrupt  SerInt12( void );      /* Handle Interrupts for INT 12.      */
//void interrupt  SerInt15( void );      /* Handle Interrupts for INT 15.      */

void   SerInt3(  void );      /* Handle Interrupts for INT 3.       */
void   SerInt4(  void );      /* Handle Interrupts for INT 4.       */
void   SerInt5(  void );      /* Handle Interrupts for INT 5.       */
void   SerInt7(  void );      /* Handle Interrupts for INT 7.       */
void   SerInt10( void );      /* Handle Interrupts for INT 10.      */
void   SerInt11( void );      /* Handle Interrupts for INT 11.      */
void   SerInt12( void );      /* Handle Interrupts for INT 12.      */
void   SerInt15( void );      /* Handle Interrupts for INT 15.      */



/*---------------------------------------------------------------------------*/
/* Static local data...                                                      */
/*---------------------------------------------------------------------------*/

PORT *IntTable[16];                    /* Chain of Ports for each interrupt. */

//typedef void far interrupt (*INTFUNC)( void );
typedef void (*INTFUNC)( void );

INTFUNC  OldVec[16];                   /* Place to save old int routines.    */

INTFUNC  NewVec[16] = {                /* Our int routines...                */

   NULL,                               /* Interrupt routine for INT 0.       */
   NULL,                               /* Interrupt routine for INT 1.       */
   NULL,                               /* Interrupt routine for INT 2.       */
   SerInt3,                            /* Interrupt routine for INT 3.       */
   SerInt4,                            /* Interrupt routine for INT 4.       */
   SerInt5,                            /* Interrupt routine for INT 5.       */
   NULL,                               /* Interrupt routine for INT 6.       */
   SerInt7,                            /* Interrupt routine for INT 7.       */
   NULL,                               /* Interrupt routine for INT 8.       */
   NULL,                               /* Interrupt routine for INT 9.       */
   SerInt10,                           /* Interrupt routine for INT 10.      */
   SerInt11,                           /* Interrupt routine for INT 11.      */
   SerInt12,                           /* Interrupt routine for INT 12.      */
   NULL,                               /* Interrupt routine for INT 13.      */
   NULL,                               /* Interrupt routine for INT 14.      */
   SerInt15                            /* Interrupt routine for INT 15.      */
};




/*---------------------------------------------------------------------------*/
/* First Level Interrupt routines...                                         */
/*---------------------------------------------------------------------------*/

//void interrupt SerInt3()               /* Handle Interrupts for INT 3.       */
void SerInt3()               /* Handle Interrupts for INT 3.       */
{
   ProcessInt(3);                      /* Do processing for this int.        */
}

//void interrupt SerInt4()               /* Handle Interrupts for INT 4.       */
void SerInt4()               /* Handle Interrupts for INT 4.       */
{
   ProcessInt(4);                      /* Do processing for this int.        */
}

//void interrupt SerInt5()               /* Handle Interrupts for INT 5.       */
void  SerInt5()               /* Handle Interrupts for INT 5.       */
{
   ProcessInt(5);                      /* Do processing for this int.        */
}

//void interrupt SerInt7()               /* Handle Interrupts for INT 7.       */
void SerInt7()               /* Handle Interrupts for INT 7.       */
{
   ProcessInt(7);                      /* Do processing for this int.        */
}

//void interrupt SerInt10()              /* Handle Interrupts for INT 10.      */
void  SerInt10()              /* Handle Interrupts for INT 10.      */
{
   ProcessInt(10);                     /* Do processing for this int.        */
}

//void interrupt SerInt11()              /* Handle Interrupts for INT 11.      */
void  SerInt11()              /* Handle Interrupts for INT 11.      */
{
   ProcessInt(11);                     /* Do processing for this int.        */
}

//void interrupt SerInt12()              /* Handle Interrupts for INT 12.      */
void SerInt12()              /* Handle Interrupts for INT 12.      */
{
   ProcessInt(12);                     /* Do processing for this int.        */
}

//void interrupt SerInt15()              /* Handle Interrupts for INT 15.      */
void  SerInt15()              /* Handle Interrupts for INT 15.      */
{
   ProcessInt(15);                     /* Do processing for this int.        */
}



/*---------------------------------------------------------------------------*/
/* Second Level Interrupt routine will handle for Port...                    */
/*---------------------------------------------------------------------------*/

static void ProcessInt(int Int)
{
   PORT  *Port;
   unsigned char  IntIdent;
   unsigned char  c;


   OsIntEnter();                       /* Tell OSKERNEL we're in an isr.     */
   Port = IntTable[Int];               /* Get first Port for this interrupt. */
   while (Port) {                      /* While there are ports to check.    */

      IntIdent = INP(IIR_PORT(Port));

      switch (IntIdent) {

         case INTERRUPT_TRANSMITTER:   /* Transmitter ready to send...       */

            if (INP(LSR_PORT(Port)) & TRANS_HOLDING_REGISTER) {

               /*------------------------------------------------------------*/
               /* First, check for pending XON/XOFF characters to send...    */
               /*------------------------------------------------------------*/
               if ((Port->Flags & FLAG_XOFF_PENDING) ||
                   (Port->Flags & FLAG_XON_PENDING)) {

                  if (Port->Flags & FLAG_XOFF_PENDING) {
                     if (Port->Flags & FLAG_XON_PENDING) {
                        Port->Flags &= ~(FLAG_XOFF_PENDING | FLAG_XON_PENDING);
                     } else {
                        OUTP(DATA_PORT(Port), Port->XOffChar);
                        Port->Flags |= FLAG_XOFF_SENT;
                        Port->Flags &= ~FLAG_XOFF_PENDING;
                     }
                  } else {
                     OUTP(DATA_PORT(Port), Port->XOnChar);
                     Port->Flags &= ~FLAG_XON_PENDING;
                  }

               /*------------------------------------------------------------*/
               /* Otherwise, check for data character to output...           */
               /*------------------------------------------------------------*/
               } else {
                  if (Port->SendLen && Port->SendLen > Port->SendCnt) {
                     OUTP(DATA_PORT(Port), Port->SendBuf[Port->SendCnt++]);
                  }

                  /*---------------------------------------------------------*/
                  /* Need to wake up a waiter?                               */
                  /*---------------------------------------------------------*/
                  if ((Port->Flags & FLAG_SENDER_WAITING) &&
                      (Port->SendLen == Port->SendCnt) ) {
                     OsResume(Port->SendPid);
                     Port->Flags &= ~FLAG_SENDER_WAITING;
                  }
               }
            }

            break;

         case INTERRUPT_RECEIVER:      /* Received a byte...                 */

            if (INP(LSR_PORT(Port)) & DATA_READY) {

               c = INP(DATA_PORT(Port));   /* Get data from UART.        */

               /*------------------------------------------------------------*/
               /* Input a data byte into the buffer, if it will fit...       */
               /*------------------------------------------------------------*/
               if (Port->RecvCnt < Port->RecvLen) {  /* If will fit in buffer*/
                  Port->RecvBuf[Port->RecvIn++] = c; /* Put data into buffer.*/
                  if (Port->RecvIn == Port->RecvLen) /* Wrap buffer indexes? */
                  	Port->RecvIn = 0;               /* Reset index then.    */
                  Port->RecvCnt++;
               } else {
                  Port->Flags |= FLAG_RECV_BUF_FULL;
               }

               /*------------------------------------------------------------*/
               /* Need to wake up a waiter?                                  */
               /*------------------------------------------------------------*/
               if (Port->Flags & FLAG_RECVER_WAITING) {
                  OsResume(Port->RecvPid);
                  Port->Flags &= ~FLAG_RECVER_WAITING;
               }

               /*------------------------------------------------------------*/
               /* Handle XON/XOFF and RTS flow control...                    */
               /*------------------------------------------------------------*/
               if (Port->RecvCnt > Port->XOffPt) {

                  if ((Port->Options & OPTION_HARD_FLOW) &&
                                             !(Port->Flags & FLAG_RTS_OFF)) {
                     OUTP(MCR_PORT(Port), (INP(MCR_PORT(Port)) & ~RTS));
                     Port->Flags |= FLAG_RTS_OFF;
                  }

                  if ( (Port->Options & OPTION_SOFT_FLOW) &&
                                           !(Port->Flags & FLAG_XOFF_SENT))  {
                     if (INP(LSR_PORT(Port)) & TRANS_HOLDING_REGISTER)
                        OUTP(DATA_PORT(Port), Port->XOffChar);
                     else
                        Port->Flags |= FLAG_XOFF_PENDING;
                  }
               }

            }
            break;

         case INTERRUPT_LINE_STATUS:
            CommCheckErrors(Port);           /* Check for errors.         */
            break;

         case INTERRUPT_MODEM:
            CommModemStatus(Port);     /* Check current modem status...      */
            break;
      }

      if (Port->Int < 7)                /* Reset interrupt controller.    */
         OUTP(INTCONT1_OCW2, 0x20); /* Generic EOI to 8259.           */
      else
         OUTP(INTCONT2_OCW2, 0x20); /* Generic EOI to 8259.           */

      Port = Port->Next;               /* Next port for this interrupt.      */
   }

   (*OldVec[Int])();                   /* Call prior interrupt handler.      */
   OsIntExit();                        /* Tell OSKERNEL we're out of isr.    */
}



/*---------------------------------------------------------------------------*/
/* Opens port by setting up port and installing interrupt, if needed...      */
/*---------------------------------------------------------------------------*/

int   CommOpen(DEVICE *Device, int Options)
{
   PORT         *Port;                 /* New Port structure for port.       */
   unsigned int  IntMask;              /* A place to create interrupt mask.  */


   Port = (PORT *) OsAlloc(sizeof(PORT)); /* Get new port structure.         */

   ((PORT *) Device->Misc) = Port;     /* Save connection to Port thru Dev.  */

   Port->Addr  = DeviceTypeTable[Device->Type].Port1;
   Port->Int   = DeviceTypeTable[Device->Type].Int;

   Port->RecvCnt = 0;                  /* Nothing received yet.              */

   Port->RecvLen = COMM_BUFFER_LENGTH; /* Default buffer size.               */

   Port->RecvBuf = (char *) OsAlloc(Port->RecvLen);

   Port->RecvIn = Port->RecvOut = 0;   /* Set circular buffer values.        */
   Port->XOffPt = Port->RecvCnt / 50 * 49;     /* Chars in buff to send XOFF.*/
   Port->XOnPt  = Port->RecvCnt - Port->XOffPt;/* Chars in buff to send XON. */

   Port->XOffChar = XOFF;              /* Default XOFF character.            */
   Port->XOnChar  = XON;               /* Default XON character.             */
   Port->TermChar = CR;                /* Default line terminating character.*/

   Port->Options  = OPTION_SOFT_FLOW | /* Set default line protocol opts.    */
                    OPTION_HARD_FLOW |
                    OPTION_TERM_CHAR;

   OUTP(LCR_PORT(Port), 0x03);     /* 8 bits no parity, 1 stop.          */
   OUTP(MCR_PORT(Port), DTR | RTS | OUT_2);       /* Set on modem lines. */
   OUTP(IER_PORT(Port), 0 );                      /* Reset  int enables. */

   OsDisable();

   Port->Next          = IntTable[Port->Int];
   IntTable[Port->Int] = Port;

   if (Port->Next == NULL) {           /* Is this first one?                 */

      OldVec[Port->Int] = getvect(Port->Int+8); /* Save old int vector.      */
      setvect(Port->Int+8, NewVec[Port->Int]);  /* Set up int vector to ISR. */

      IntMask = 1 << (Port->Int & 0x07);

      if (Port->Int < 8) {             /* Enable 8259 interrupts.            */
         OUTP(INTCONT1_OCW1, ~IntMask      & INP(INTCONT1_OCW1));
      } else {
         OUTP(INTCONT2_OCW1, ~IntMask      & INP(INTCONT2_OCW1));
         OUTP(INTCONT1_OCW1, ~CHAIN_IRQBIT & INP(INTCONT1_OCW1));
      }
   }

   INP( DATA_PORT(Port));                         /* Clear data register.*/

   OUTP(IER_PORT(Port), (RECEIVED_DATA_AVAILABLE |  /* Enable interrupts.*/
                             RECEIVER_LINE_STATUS    |
                             TRANSMIT_HOLD_EMPTY     |
                             MODEM_STATUS)      );

   OsEnable();

   return(SYSOK);                      /* Return with Port structure.        */
}



/*---------------------------------------------------------------------------*/
/* Close COM port. If no more ports for same int, reset 8259 and vector...   */
/*---------------------------------------------------------------------------*/

int  CommClose(DEVICE *Device)
{
   PORT  *Port;
   PORT  *PrevPort = NULL;
   PORT  *CurPort;
   unsigned int  IntMask;


   Port = (PORT *) Device->Misc;       /* Get port definition.               */

   /*------------------------------------------------------------------------*/
   /* Remove from interrupt table...                                         */
   /*------------------------------------------------------------------------*/

   OsDisable();

   CurPort = IntTable[Port->Int];
   while (CurPort) {
      if (CurPort == Port) {
         if (PrevPort) {
            PrevPort->Next = Port->Next;
         } else {
            IntTable[Port->Int] = Port->Next;
         }
         break;
      }
      PrevPort = CurPort;
      CurPort  = CurPort->Next;
   }

   OsEnable();

   /*------------------------------------------------------------------------*/
   /* If no more ports, disable interrupt on 8259 and reset vector...        */
   /*------------------------------------------------------------------------*/

   if (IntTable[Port->Int] == NULL) {

      IntMask = 1 << (Port->Int & 0x07);

      if (Port->Int < 8) {             /* Enable 8259 interrupts.            */
         OUTP(INTCONT1_OCW1, IntMask      | INP(INTCONT1_OCW1));
      } else {
         OUTP(INTCONT2_OCW1, IntMask      | INP(INTCONT2_OCW1));
/*       OUTP(INTCONT1_OCW1, ~CHAIN_IRQBIT & INP(INTCONT1_OCW1)); */
      }

      setvect(Port->Int+8, OldVec[Port->Int]);     /* Reset orig int vector. */
   }

   OsFree(Port->RecvBuf);              /* Free receive buffer.               */
   OsFree(Port);                       /* Free Port Structure.               */
   return SYSOK;
}



/*---------------------------------------------------------------------------*/
/* Send a block of data...                                                   */
/*---------------------------------------------------------------------------*/

int  CommSend(DEVICE *Device, char *Buffer, int Length)
{
   PORT   *Port;


   Port = (PORT *) Device->Misc;       /* Get Port structure.                */

   OsLock(&Port->SendLock);            /* Lock Port for senders.             */
   OsDisable();

   Port->SendBuf = Buffer;             /* Store buffer pointer.              */
   Port->SendLen = Length;             /* Length to send.                    */
   Port->SendCnt = 0;                  /* Reset send count.                  */
   Port->SendPid = OsGetPid();         /* We're the one that will wait.      */

   /*------------------------------------------------------------------------*/
   /* Can we send the first byte?                                            */
   /*------------------------------------------------------------------------*/

   if ( !(Port->Flags & (FLAG_XON_PENDING | FLAG_XOFF_PENDING)) ) {
      if (INP(LSR_PORT(Port)) & TRANS_HOLDING_REGISTER) {
         OUTP(DATA_PORT(Port), Port->SendBuf[Port->SendCnt++]);
      }
   }

   /*------------------------------------------------------------------------*/
   /* If more to send, then we must suspend until all bytes are outputted... */
   /*------------------------------------------------------------------------*/
   if (Port->SendCnt < Port->SendLen) {   /* Is there more to wait on?       */
      Port->Flags |= FLAG_SENDER_WAITING; /* Say someone is waiting.         */
      OsSuspend(Port->SendPid);           /* Make us wait 'til isr sends all.*/
   }

   Length -= Port->SendCnt;            /* Calculate actual number sent.      */

   OsEnable();
   OsUnlock(&Port->SendLock);          /* Unlock port for senders.           */

   return Length;
}


/*---------------------------------------------------------------------------*/
/* Receive a block of data...                                                */
/*---------------------------------------------------------------------------*/

int  CommRecv(DEVICE *Device, char *Buffer, int Length)
{
   PORT   *Port;
   char    c;
   int     ActualLength = 0;


   Port = (PORT *) Device->Misc;       /* Get Port structure.                */

   OsLock(&Port->RecvLock);            /* Lock Port for receivers.           */
   OsDisable();                        /* Turn off interrupts.               */


   /*------------------------------------------------------------------------*/
   /* Fill up receive buffer. If there are no more bytes available, then     */
   /* suspend us until there are more bytes to get...                        */
   /*------------------------------------------------------------------------*/

   while (Length > 0) {

      /*---------------------------------------------------------------------*/
      /* Fill up receiver's buffer until full, or Port buffer empty, or      */
      /* line termination character is recieved (if option is on)...         */
      /*---------------------------------------------------------------------*/
      while (Length > 0 && Port->RecvCnt > 0) {
         *Buffer++ = c = Port->RecvBuf[Port->RecvOut++];
         Length--;
         Port->RecvCnt--;
         ActualLength++;
         if (Port->RecvOut == Port->RecvLen)
            Port->RecvOut = 0;
         if ((Port->Options & OPTION_TERM_CHAR) && c == Port->TermChar)
            break;
      }

      /*---------------------------------------------------------------------*/
      /* Does caller need to wait until more bytes have been received?       */
      /*---------------------------------------------------------------------*/
      if (Length > 0) {
         Port->Flags |= FLAG_RECVER_WAITING;
         Port->SendPid = OsGetPid();
         OsSuspend( Port->SendPid );   /* Wait until more comes in.          */
      }

      if ((Port->Options & OPTION_TERM_CHAR) && c == Port->TermChar)
         break;
   }

   /*------------------------------------------------------------------------*/
   /* Handle XON/XOFF and RTS flow control...                                */
   /*------------------------------------------------------------------------*/
   if (Port->RecvCnt < Port->XOnPt) {
      Port->Flags &= ~FLAG_XOFF_PENDING;   /* Reset incase it was on.        */
      if (Port->Flags & FLAG_RTS_OFF) {
         OUTP(MCR_PORT(Port), (INP(MCR_PORT(Port)) | RTS));
      }
      if (Port->Flags & FLAG_XOFF_SENT) {
         if (INP(LSR_PORT(Port)) & TRANS_HOLDING_REGISTER)
            OUTP(DATA_PORT(Port), Port->XOnChar);
         else
            Port->Flags |= FLAG_XON_PENDING;
         Port->Flags &= ~FLAG_XOFF_SENT;
      }
   }


   OsEnable();                         /* Enable interrupts again.           */
   OsUnlock(&Port->RecvLock);          /* Unlock receive for others to use.  */

   return ActualLength;
}



/*---------------------------------------------------------------------------*/
/* Device control...                                                         */
/*---------------------------------------------------------------------------*/

int  CommControl(DEVICE *Device, int Function, long Value)
{
   PORT   *Port;
   BYTE    portval;
   BYTE    blo;
   BYTE    bhi;
   BYTE    LcrReg;
   int     StatReg;


   Port = (PORT *) Device->Misc;       /* Get port definition.               */

   switch (Function) {

      case COMM_SET_BAUDRATE:
         switch (Value) {              /* Baud rate LSB's and MSB's */
            case 50:     bhi = 0x9;  blo = 0x00;  break;
            case 75:     bhi = 0x6;  blo = 0x00;  break;
            case 110:    bhi = 0x4;  blo = 0x17;  break;
            case 150:    bhi = 0x3;  blo = 0x00;  break;
            case 300:    bhi = 0x1;  blo = 0x80;  break;
            case 600:    bhi = 0x0;  blo = 0xC0;  break;
            case 1200:   bhi = 0x0;  blo = 0x60;  break;
            case 1800:   bhi = 0x0;  blo = 0x40;  break;
            case 2000:   bhi = 0x0;  blo = 0x3A;  break;
            case 2400:   bhi = 0x0;  blo = 0x30;  break;
            case 4800:   bhi = 0x0;  blo = 0x18;  break;
            case 9600:   bhi = 0x0;  blo = 0x0C;  break;
            case 19200:  bhi = 0x0;  blo = 0x06;  break;
            case 38400:  bhi = 0x0;  blo = 0x03;  break;
            case 56000:  bhi = 0x0;  blo = 0x02;  break;
            case 112000: bhi = 0x0;  blo = 0x01;  break;
            default:
         	   return SYSERR;
         }
                                             /* Say we're loading baud rate. */
         OUTP(LCR_PORT(Port),    0x80 | INP(LCR_PORT(Port)));
         OUTP(DATA_PORT(Port),   blo);   /* Send LSB for baud rate.      */
         OUTP(DATA_PORT(Port)+1, bhi);   /* Send MSB for baud rate.      */
         break;


      case COMM_SET_DATABITS:
         LcrReg = INP(LCR_PORT(Port));
         LcrReg &= ~(WORD_LENGTH_SELECT_1 | WORD_LENGTH_SELECT_2);
         switch (Value) {
            case 5:
               break;
            case 6:
               LcrReg |= WORD_LENGTH_SELECT_1;
               break;
            case 7:
               LcrReg |= WORD_LENGTH_SELECT_2;
               break;
            case 8:
               LcrReg |= WORD_LENGTH_SELECT_1 | WORD_LENGTH_SELECT_2;
               break;
         }
         OUTP(LCR_PORT(Port), LcrReg);   /* Re-write LCR register.       */
         break;

      case COMM_SET_STOPBITS:
         LcrReg = INP(LCR_PORT(Port));
         LcrReg &= ~NUMBER_STOP_BITS;
         switch (Value) {
            case 1:
               break;
            case 2:
               LcrReg = NUMBER_STOP_BITS;
               break;
         }
         OUTP(LCR_PORT(Port), LcrReg);   /* Re-write LCR register.       */
         break;

      case COMM_SET_PARITY:
         LcrReg = INP(LCR_PORT(Port));
         LcrReg &= ~(PARITY_ENABLE | EVEN_PARITY_SELECT);
         switch (Value) {
            case 'N':
            case 'n':
               break;
            case 'E':
            case 'e':
               LcrReg |= PARITY_ENABLE | EVEN_PARITY_SELECT;
               break;
            case 'O':
            case 'o':
               LcrReg |= PARITY_ENABLE;
               break;
         }
         OUTP(LCR_PORT(Port), LcrReg);   /* Re-write LCR register.       */
         break;

      case COMM_SET_DTR:
         if (Value == 0)                     /* Set DTR off?                 */
            StatReg = INP(MCR_PORT(Port)) & ~DTR;
         else                                /* Else set DTR on...           */
            StatReg = INP(MCR_PORT(Port)) | DTR;
         OUTP(MCR_PORT(Port), StatReg);
         break;

      case COMM_SET_RTS:
         if (Value == 0)                     /* Set RTS off?                 */
            StatReg = INP(MCR_PORT(Port)) & ~RTS;
         else                                /* Else set RTS on...           */
            StatReg = INP(MCR_PORT(Port)) | RTS;
         OUTP(MCR_PORT(Port), StatReg);
         break;

      case COMM_SET_OUT1:
         if (Value == 0)                     /* Set OUT1 off?                */
            StatReg = INP(MCR_PORT(Port)) & ~OUT_1;
         else                                /* Else set OUT1 on...          */
            StatReg = INP(MCR_PORT(Port)) | OUT_1;
         OUTP(MCR_PORT(Port), StatReg);
         break;

      case COMM_SET_OUT2:
         if (Value == 0)                     /* Set OUT2 off?                */
            StatReg = INP(MCR_PORT(Port)) & ~OUT_2;
         else                                /* Else set OUT2 on...          */
            StatReg = INP(MCR_PORT(Port)) | OUT_2;
         OUTP(MCR_PORT(Port), StatReg);
         break;

      case COMM_SET_TERMCHAR:
         Port->TermChar = (char) Value;      /* Set line terminating char.   */
         Port->Options |= OPTION_TERM_CHAR;  /* And turn on option.          */
         break;

      case COMM_RESET_TERMCHAR:
         Port->Options &= ~OPTION_TERM_CHAR; /* Turn off option.             */
         break;

      case COMM_RESET:
         break;

      case COMM_RETURN_ERRORS:
         return (Port->Error);

      case COMM_RETURN_STATUS:
         return (Port->Status);

      case COMM_SEND_BREAK:
         break;

      default:
         return SYSERR;
   }

   return SYSOK;
}



/*---------------------------------------------------------------------------*/
/* Check current Modem status...                                             */
/*---------------------------------------------------------------------------*/

static int  CommModemStatus(PORT *Port)
{
   unsigned char StatReg;

   StatReg = INP(MSR_PORT(Port));

   Port->Status &= ~(MODEM_CTS | MODEM_DSR | MODEM_RI | MODEM_DCD);

   if (StatReg & CTS)
      Port->Status |= MODEM_CTS;

   if (StatReg & DSR)
      Port->Status |= MODEM_DSR;

   if (StatReg & RI)
      Port->Status |= MODEM_RI;

   if (StatReg & DCD)
      Port->Status |= MODEM_DCD;

   return (Port->Status);
}


/*---------------------------------------------------------------------------*/
/* Check for any line errors...                                              */
/*---------------------------------------------------------------------------*/

static int  CommCheckErrors(PORT *Port)
{
   int   StatReg;

   StatReg = INP(LSR_PORT(Port));

   if (StatReg & OVERRUN_ERROR)
      Port->Error |= ERROR_OVERRUN;

   if (StatReg & PARITY_ERROR)
      Port->Error |= ERROR_PARITY;

   if (StatReg & FRAMING_ERROR)
      Port->Error |= ERROR_FRAMING;

   if (StatReg & BREAK_INTERRUPT)
      Port->Error |= ERROR_BREAK;

   return (Port->Error);
}



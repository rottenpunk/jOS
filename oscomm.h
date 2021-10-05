/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                              OS KERNEL                                    */
/*                                                                           */
/*                  COPYRIGHT (c) 1994 by JOHN C. OVERTON                    */
/*              Advanced Communication Development Tools, Inc                */
/*                                                                           */
/*                                                                           */
/*            Module:  COMM.H                                                */
/*                                                                           */
/*             Title:  Com port and 8250 serial driver.                      */
/*                                                                           */
/*       Description:  Serial driver for OS KERNEL.                          */
/*                                                                           */
/*            Author:  John C. Overton                                       */
/*                                                                           */
/*              Date:  11/17/94                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/



/*---------------------------------------------------------------------------*/
/* Definitions for OsControl() value...                                      */
/*---------------------------------------------------------------------------*/

#define COMM_SET_BAUDRATE      1
#define COMM_SET_DATABITS      2
#define COMM_SET_STOPBITS      3
#define COMM_SET_PARITY        4
#define COMM_SET_DTR           5
#define COMM_SET_RTS           6
#define COMM_SET_OUT1          7
#define COMM_SET_OUT2          8
#define COMM_SET_TERMCHAR      9
#define COMM_RESET_TERMCHAR   10
#define COMM_RESET            11
#define COMM_RETURN_ERRORS    12
#define COMM_RETURN_STATUS    13
#define COMM_SEND_BREAK       14


/*---------------------------------------------------------------------------*/
/* Definitions returned by COMM_RETURN_ERRORS...                             */
/*---------------------------------------------------------------------------*/

#define ERROR_OVERRUN       0x08
#define ERROR_PARITY        0x04
#define ERROR_FRAMING       0x02
#define ERROR_BREAK         0x01


/*---------------------------------------------------------------------------*/
/* Definitions returned by COMM_RETURN_STATUS...                             */
/*---------------------------------------------------------------------------*/

#define MODEM_CTS           0x08
#define MODEM_DSR           0x04
#define MODEM_RI            0x02
#define MODEM_DCD           0x01


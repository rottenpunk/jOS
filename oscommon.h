/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                              OS KERNEL                                    */
/*                                                                           */
/*                  COPYRIGHT (c) 1994 by JOHN C. OVERTON                    */
/*              Advanced Communication Development Tools, Inc                */
/*                                                                           */
/*                                                                           */
/*            Module:  GLOBALS.H                                             */
/*                                                                           */
/*             Title:  Os Kernel general definitions, equates, and includes. */
/*                                                                           */
/*       Description:  This is a general heading file with various def-      */
/*                     initions in it for the OS KERNAL.                     */
/*                                                                           */
/*            Author:  John C. Overton                                       */
/*                                                                           */
/*              Date:  04/23/94                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/




/*---------------------------------------------------------------------------*/
/* Boolean definition...                                                     */
/*---------------------------------------------------------------------------*/

#if      !defined(__BOOLEAN_H)
#define  __BOOLEAN_H

typedef enum {False = 0, True} Bool;        /* Define Bool boolean types.    */
typedef enum {FALSE = 0, TRUE} BOOL;        /* Define Bool boolean types.    */

#endif /* __BOOLEAN_H */


/*---------------------------------------------------------------------------*/
/* Common typedefs...                                                        */
/*---------------------------------------------------------------------------*/

typedef unsigned long   ULONG;
typedef          long    LONG;
typedef unsigned short  USHORT;
typedef          short   SHORT;
typedef unsigned char    BYTE;
typedef void        (*ROUTINE)( void );


/*---------------------------------------------------------------------------*/
/* Important definitions...                                                  */
/*---------------------------------------------------------------------------*/

#define MIN_STACK_SIZE  256




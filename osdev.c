/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                              OS KERNEL                                    */
/*                                                                           */
/*                  COPYRIGHT (c) 1994 by JOHN C. OVERTON                    */
/*              Advanced Communication Development Tools, Inc                */
/*                                                                           */
/*                                                                           */
/*            Module:  OSDEV.C                                               */
/*                                                                           */
/*             Title:  I/O Devices interfaces.                               */
/*                                                                           */
/*       Description:  This module contains:                                 */
/*                                                                           */
/*                     OsDevInit() - Initialize all devices needing initing. */
/*                     OsDevTerm() - terminate all devices needing terming.  */
/*                     OsOpen()    - Open a device.                          */
/*                     OsClose()   - Close a device.                         */
/*                     OsRead()    - Read from a device.                     */
/*                     OsWrite()   - Write to a device.                      */
/*                     OsControl() - Control a device.                       */
/*                     OsSeek()    - Seek on a device.                       */
/*                                                                           */
/*                                                                           */
/*            Author:  John C. Overton                                       */
/*                                                                           */
/*              Date:  11/04/94                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#include "oskernel.h"




/*---------------------------------------------------------------------------*/
/* OsOpen() -- Open a device instance...                                     */
/*---------------------------------------------------------------------------*/

HANDLE  OsOpen(char *Name, int  Options)
{
   DEVICE       *Device;
   DEVICETYPE   *DeviceType;
   DEVICEDRIVER *DeviceDriver;
   int           DeviceTypeNbr = 0;


   /*------------------------------------------------------------------------*/
   /* Look up device name in device table...                                 */
   /*------------------------------------------------------------------------*/
   DeviceType = DeviceTypeTable;       /* Start of device type table.        */

   while (DeviceType->Name != NULL) {

      if (strcmp(DeviceType->Name, Name) == 0)   /* Is this the device?      */
         break;

      DeviceTypeNbr++;                 /* Keep track of offset into table.   */
   }

   /*------------------------------------------------------------------------*/
   /* If not found, then error...                                            */
   /*------------------------------------------------------------------------*/
   if (DeviceType->Name == NULL) {     /* Later, we'll default to file sys.  */
      return SYSERR;                   /* For now, reject open.              */
   }

   /*------------------------------------------------------------------------*/
   /* Find device type table...                                              */
   /*------------------------------------------------------------------------*/
   DeviceDriver = &DeviceDriverTable[DeviceType->Driver];

   /*------------------------------------------------------------------------*/
   /* Allocate a Device structure and associate device type and driver...    */
   /*------------------------------------------------------------------------*/
   Device = OsAlloc(sizeof(DEVICE));   /* Get a device structure.            */
   Device->Type   = DeviceTypeNbr;     /* Index into device type table.      */
   Device->Driver = DeviceType->Driver;/* Index into device driver table.    */

   /*------------------------------------------------------------------------*/
   /* Get device instance number (handle) by registering with                */
   /* handle services...                                                     */
   /*------------------------------------------------------------------------*/
   if ((Device->Handle = OsHandCreate(&DeviceAnchor, (void *) Device)) == 0) {
      return SYSERR;
   }

   /*------------------------------------------------------------------------*/
   /* Now, call open service for this device type...                         */
   /*------------------------------------------------------------------------*/
   if (DeviceDriver->Open != NULL) {
      if ( (*DeviceDriver->Open)(Device, Options) > 0) {
         OsHandUnprotect(DeviceAnchor, Device->Handle);
         OsHandDestroy(DeviceAnchor, Device->Handle);
         return SYSERR;
      }
   }

   OsHandUnprotect(DeviceAnchor, Device->Handle);

   return Device->Handle;              /* Return witn new handle.            */

}



/*---------------------------------------------------------------------------*/
/* OsClose() -- Close device...                                              */
/*---------------------------------------------------------------------------*/

int   OsClose(HANDLE Handle)
{
   DEVICEDRIVER *DeviceDriver;
   DEVICE       *Device;
   int           rc;


   if ((Device = (DEVICE *) OsHandDestroy(DeviceAnchor, Handle)) == NULL)
      return SYSERR;                   /* File number not found.             */

   DeviceDriver = &DeviceDriverTable[Device->Driver];

   if (DeviceDriver->Close != NULL)    /* Is there a close routine?          */
      rc = (*DeviceDriver->Close)(Device);

   OsFree(Device);                     /* Device structure.                  */

   return rc;                          /* Return with close return code.     */
}



/*---------------------------------------------------------------------------*/
/* OsRead() -- Read from device. Return actual nbr of bytes read...          */
/*---------------------------------------------------------------------------*/

int   OsRead(HANDLE Handle, char *Buffer, int Length)
{
   DEVICEDRIVER *DeviceDriver;
   DEVICE       *Device;
   int           rc;

   if ((Device = (DEVICE *) OsHandProtect(DeviceAnchor, Handle)) == NULL)
      return SYSERR;                   /* Handle number not found.           */

   DeviceDriver = &DeviceDriverTable[Device->Driver];

   if (DeviceDriver->Read != NULL)
      rc = (*DeviceDriver->Read)(Device, Buffer, Length);

   OsHandUnprotect(DeviceAnchor, Device->Handle);

   return rc;
}



/*---------------------------------------------------------------------------*/
/* OsWrite() -- Write to device...                                           */
/*---------------------------------------------------------------------------*/

int   OsWrite(HANDLE Handle, char *Buffer, int Length)
{
   DEVICEDRIVER *DeviceDriver;
   DEVICE       *Device;
   int           rc;

   if ((Device = (DEVICE *) OsHandProtect(DeviceAnchor, Handle)) == NULL)
      return SYSERR;                   /* Handle number not found.           */

   DeviceDriver = &DeviceDriverTable[Device->Driver];

   if (DeviceDriver->Write != NULL)
      rc = (*DeviceDriver->Write)(Device, Buffer, Length);

   OsHandUnprotect(DeviceAnchor, Device->Handle);

   return rc;
}



/*---------------------------------------------------------------------------*/
/* OsSeek() -- Position file to specific offset...                           */
/*---------------------------------------------------------------------------*/

int   OsSeek(HANDLE Handle, long Position)
{
   DEVICEDRIVER *DeviceDriver;
   DEVICE       *Device;
   int           rc;

   if ((Device = (DEVICE *) OsHandProtect(DeviceAnchor, Handle)) == NULL)
      return SYSERR;                   /* File number not found.             */

   DeviceDriver = &DeviceDriverTable[Device->Driver];

   if (DeviceDriver->Seek != NULL)
      rc = (*DeviceDriver->Seek)(Device, Position);

   OsHandUnprotect(DeviceAnchor, Device->Handle);

   return rc;
}



/*---------------------------------------------------------------------------*/
/* OsControl() -- Control device...                                          */
/*---------------------------------------------------------------------------*/

int   OsControl(HANDLE Handle, int Function, long Value)
{
   DEVICEDRIVER *DeviceDriver;
   DEVICE       *Device;
   int           rc;

   if ((Device = (DEVICE *) OsHandProtect(DeviceAnchor, Handle)) == NULL)
      return SYSERR;                   /* File number not found.             */

   DeviceDriver = &DeviceDriverTable[Device->Driver];

   if (DeviceDriver->Control != NULL)
      rc = (*DeviceDriver->Control)(Device, Function, Value);

   OsHandUnprotect(DeviceAnchor, Device->Handle);

   return rc;
}



/*---------------------------------------------------------------------------*/
/* OsDevInit() -- Initialize all initializable device drivers...             */
/*---------------------------------------------------------------------------*/

int   OsDevInit(void)
{
   DEVICEDRIVER *DeviceDriver;
   int           i = 0;

   DeviceDriver = DeviceDriverTable;    /* Start of device driver table.     */

   while (DeviceDriver->Init != (void *) -1) { /* End of table yet?          */

      if ( DeviceDriver->Init != NULL )/* If there's an init routine, then...*/
         (*DeviceDriver->Init)(i);     /* ...Do it.                          */

      i++;
      DeviceDriver++;                  /* Next table entry.                  */
   }

   return SYSOK;                       /* Return ok.                         */
}



/*---------------------------------------------------------------------------*/
/* OsDevTerm() -- Terminate all terminatable device drivers...               */
/*---------------------------------------------------------------------------*/

int   OsDevTerm(void)
{
   DEVICEDRIVER *DeviceDriver;
   int           i = 0;

   DeviceDriver = DeviceDriverTable;   /* Start of device driver table.      */

   while (DeviceDriver->Term != (void *) -1) {

      if ( DeviceDriver->Term != NULL )/* If there's an init routine, then...*/
         (*DeviceDriver->Term)(i);     /* ...Do it.                          */

      i++;
      DeviceDriver++;                  /* Next table entry.                  */
   }

   return SYSOK;                       /* Return ok.                         */
}

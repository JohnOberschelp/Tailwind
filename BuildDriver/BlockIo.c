//
//  Tailwind: A File System Driver
//  Copyright (C) 2024 John Oberschelp
//
//  This program is free software: you can redistribute it and/or modify it under the
//  terms of the GNU General Public License as published by the Free Software
//  Foundation, either version 3 of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License along with this
//  program. If not, see https://www.gnu.org/licenses/.
//

#include "Common.h"

//////////////////////////////////////////////////////////////////////

NTSTATUS ReadBlockDevice( PDEVICE_OBJECT DeviceObject, U8 Offset, U4 Length, V_ Buffer, VERIFY Verify )

{
    KEVENT          Event;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER   OffsetAsLargeInteger;

ASSERT( ALIGNED_256( Offset ) );
ASSERT( ALIGNED_256( Length ) );
ASSERT( Length );

LogFormatted( "(%p,%X,%X,%p)\n", ( V_ ) DeviceObject, ( U4 ) Offset, Length, Buffer );

    if ( Length == 0 ) return 0;

    OffsetAsLargeInteger.QuadPart = Offset;

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    PIRP Irp = IoBuildSynchronousFsdRequest( IRP_MJ_READ, DeviceObject, Buffer, Length, &OffsetAsLargeInteger, &Event, &IoStatusBlock );
    if ( ! Irp ) return STATUS_INSUFFICIENT_RESOURCES;


//SetFlag( Irp->Flags, IRP_NOCACHE );
//KeDelayExecutionThread?


    //  Override verification?
    if ( Verify == NO_VERIFY )
    {
        SetBit( IoGetNextIrpStackLocation( Irp )->Flags, SL_OVERRIDE_VERIFY_VOLUME );
    }


    NTSTATUS Status = IoCallDriver( DeviceObject, Irp );
    if ( Status == STATUS_PENDING )
    {
LogString( "Waiting.\n" );
        KeWaitForSingleObject( &Event, Executive, KernelMode, FALSE, 0 );
        Status = IoStatusBlock.Status;
    }


//  NTSTATUS Status = IoCallDriver( DeviceObject, Irp );
//  LARGE_INTEGER NegativeOne;
//  NegativeOne.QuadPart = -1000000LL;
//  if ( Status == STATUS_PENDING )
//  {
//      NTSTATUS Status2;
//      do
//      {
//          Status2 = KeWaitForSingleObject( &Event, Executive, KernelMode, FALSE, &NegativeOne );
//AlwaysLogString( "try\n" );
//
//      }  while ( Status2 == STATUS_TIMEOUT );
//      Status = IoStatusBlock.Status;
//  }
//


if ( Status )
{
AlwaysLogFormatted( "(%p,%X,%X,%p) %X\n", ( V_ ) DeviceObject, ( U4 ) Offset, Length, Buffer, Status );
}

    return Status;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS WriteBlockDevice( PDEVICE_OBJECT DeviceObject, U8 Offset, U4 Length, V_ Buffer, VERIFY Verify )

{
    KEVENT          Event;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER   OffsetAsLargeInteger;

ASSERT( ALIGNED_256( Offset ) );
ASSERT( ALIGNED_256( Length ) );
ASSERT( Length );

    if ( Length == 0 ) return 0;

    OffsetAsLargeInteger.QuadPart = Offset;

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    PIRP Irp = IoBuildSynchronousFsdRequest( IRP_MJ_WRITE, DeviceObject, Buffer, Length, &OffsetAsLargeInteger, &Event, &IoStatusBlock );
    if ( ! Irp )
    {
AlwaysBreakToDebugger();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //  Override verification?
    if ( Verify == NO_VERIFY )
    {
        SetBit( IoGetNextIrpStackLocation( Irp )->Flags, SL_OVERRIDE_VERIFY_VOLUME );
    }


    NTSTATUS Status = IoCallDriver( DeviceObject, Irp );
    if ( Status == STATUS_PENDING )
    {
        KeWaitForSingleObject( &Event, Executive, KernelMode, FALSE, 0 );
        Status = IoStatusBlock.Status;
    }

if ( Status )
{
AlwaysLogFormatted( "(%p,%X,%X,%p) %X\n", ( V_ ) DeviceObject, ( U4 ) Offset, Length, Buffer, Status );
}

    return Status;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS DeviceIoControlRequest( ULONG IoControlCode,  PDEVICE_OBJECT DeviceObject, V_ InputBuffer, ULONG InputBufferLength, V_ OutputBuffer, ULONG OutputBufferLength )

{
    KEVENT          Event;
    IO_STATUS_BLOCK IoStatusBlock;

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    PIRP Irp = IoBuildDeviceIoControlRequest( IoControlCode, DeviceObject, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, FALSE, &Event, &IoStatusBlock );
    if ( ! Irp )
    {
AlwaysBreakToDebugger();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS Status = IoCallDriver( DeviceObject, Irp );
    if ( Status == STATUS_PENDING )
    {
        KeWaitForSingleObject( &Event, Executive, KernelMode, FALSE, 0 );
        Status = IoStatusBlock.Status;
    }


if ( Status )
{
AlwaysLogFormatted( "(%p,%X) %X\n", ( V_ ) DeviceObject, ( U4 ) IoControlCode, Status );
}

    return Status;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


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
//
//NTSTATUS CompletionRoutine( PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context )
//
//{
//    UNREFERENCED_PARAMETER( DeviceObject );
//    UNREFERENCED_PARAMETER( Context );
//
//    //  From CDFS source "Add the hack-o-ramma to fix formats." TODO iduknow!
//    if ( Irp->PendingReturned ) IoMarkIrpPending( Irp );
//
//    return STATUS_SUCCESS;
//}
//
//////////////////////////////////////////////////////////////////////

NTSTATUS IrpMjDeviceControl( ICB_ Icb )

{
//static int tempx = 0;
//AlwaysLogFormatted( "*************************************** IrpMjDeviceControl call %d\n", tempx++ );

    PDEVICE_OBJECT DeviceObject = Icb->DeviceObject;
    if ( DeviceObject == Volume_TailwindDeviceObject ) return STATUS_INVALID_DEVICE_REQUEST;

    PIRP    Irp    = Icb->Irp;
    IRPSP_  IrpSp  = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    FCB_ Fcb = FileObject->FsContext;

    if ( ! Fcb->IsAVolume ) return STATUS_INVALID_PARAMETER;

    IoCopyCurrentIrpStackLocationToNext( Irp );

////IoSetCompletionRoutine( Irp, CompletionRoutine, 0, TRUE, TRUE, TRUE );

    VCB_ Vcb = DeviceObject->DeviceExtension;

    PDEVICE_OBJECT PhysicalDeviceObject = Vcb->PhysicalDeviceObject;

    NTSTATUS Status = IoCallDriver( PhysicalDeviceObject, Irp );

    Icb->DoCompleteRequest = FALSE;

    return Status;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


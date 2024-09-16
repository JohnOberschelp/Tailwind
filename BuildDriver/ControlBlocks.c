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

ICB_  MakeIcb( PDEVICE_OBJECT DeviceObject, PIRP Irp )

{
    ICB_ Icb = AllocateAndZeroMemory( sizeof( ICB ) );
    if ( ! Icb )  return 0;

    Icb->Irp               = Irp;
    Icb->DeviceObject      = DeviceObject;
    Icb->DoCompleteRequest = TRUE;  //  Default to the normal case.

    return Icb;
}
//////////////////////////////////////////////////////////////////////

void UnmakeIcb( ICB_ Icb )

{
    FreeMemory( Icb );
}

//////////////////////////////////////////////////////////////////////

FCB_ MakeFcb( VCB_ Vcb, ID Id )

{
    FCB_ Fcb = AllocateAndZeroMemory( sizeof( FCB ) );
    if ( ! Fcb ) return 0;

    Fcb->Vcb = Vcb;

    Fcb->Id = Id;


AttachLinkLast( &Vcb->OpenFcbsChain, &Fcb->OpenFcbsLink );

    return Fcb;
}

//////////////////////////////////////////////////////////////////////

void UnmakeFcb( FCB_ Fcb )

{
    DetachLink( &( Fcb->Vcb )->OpenFcbsChain, &Fcb->OpenFcbsLink );

    FreeMemory( Fcb );
}

//////////////////////////////////////////////////////////////////////

VCB_ MakeVcb(  ICB_ Icb, PDEVICE_OBJECT VolumeDeviceObject, PDEVICE_OBJECT PhysicalDeviceObject )

{
    VCB_ Vcb = VolumeDeviceObject->DeviceExtension;

    Zero( Vcb, sizeof( VCB ) );

    Vcb->IsAVolume = TRUE;

    PIRP     Irp    = Icb->Irp;
    IRPSP_   IrpSp  = IoGetCurrentIrpStackLocation( Irp );
    PVPB     Vpb    = IrpSp->Parameters.MountVolume.Vpb;

    Vcb->Vpb = Vpb;
    Vpb->DeviceObject = VolumeDeviceObject;

    Vcb->VolumeDeviceObject   = VolumeDeviceObject;
    Vcb->PhysicalDeviceObject = PhysicalDeviceObject;

    return Vcb;
}

//////////////////////////////////////////////////////////////////////

void  UnmakeVcb( VCB_ Vcb )

{
    ClearVpbBit( Vcb->Vpb, VPB_MOUNTED );

    FreeMemory( Vcb->FirstBlock );

    IoDeleteDevice( Vcb->VolumeDeviceObject );
}

//////////////////////////////////////////////////////////////////////

CCB_ MakeCcb()

{
    CCB_ Ccb = AllocateAndZeroMemory( sizeof( CCB ) );
    if ( ! Ccb ) return 0;

    Ccb->FirstQuery = TRUE;

    return Ccb;
}

//////////////////////////////////////////////////////////////////////

void UnmakeCcb( CCB_ Ccb )

{
    FreeMemory( Ccb );
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


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

NTSTATUS IrpMjCleanup( ICB_ Icb )  //  All user's handles are closed.

{
    PDEVICE_OBJECT DeviceObject = Icb->DeviceObject;

    if ( DeviceObject == Volume_TailwindDeviceObject )
    {
AlwaysLogString( "just returning cuz TailwindDeviceObject\n" );
        return STATUS_SUCCESS;
    }


    VCB_ Vcb = DeviceObject->DeviceExtension;

    PIRP    Irp    = Icb->Irp;
    IRPSP_  IrpSp  = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT FileObject = IrpSp->FileObject;


    FCB_ Fcb = FileObject->FsContext;

    //  TODO The line in cdfs cleanup.c # 120 talks about no work for an unopened file,
    //  so lets try ignoring it?
    if ( ! Fcb )  //  I think this happened when I tried to shut down snick instead of .reboot
    {
BreakToDebugger();  //  eb eip 0x90
AlwaysLogString( "IrpMjCleanup called with a zero Fcb\n" );
        return STATUS_SUCCESS;
    }


    if ( Fcb->IsAVolume )
    {
AlwaysLogString( "IrpMjCleanup called on volume!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" );
        if ( Vcb->VolumeLocked )
        {
            Vcb->VolumeLocked = FALSE;
            ClearVpbBit( Vcb->Vpb, VPB_LOCKED );
        }
    }
    else
    {

        if ( IdIsAFile( Fcb->Id ) )
        {

BreakToDebugger();

            PEPROCESS Process = IoGetRequestorProcess( Irp );
            UnlockAllForProcess( Process, &Fcb->LockRanges );

        }

        Vcb->VcbNumOpenHandles--;

        SetBit( FileObject->Flags, FO_CLEANUP_COMPLETE );
    }

    return STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS IrpMjClose( ICB_ Icb )  //  All references are gone.

{
    PDEVICE_OBJECT DeviceObject = Icb->DeviceObject;
    if ( DeviceObject == Volume_TailwindDeviceObject ) return STATUS_SUCCESS;

    PIRP    Irp    = Icb->Irp;
    IRPSP_  IrpSp  = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    FCB_ Fcb = FileObject->FsContext;

    if ( ! Fcb )
//  I think this happened when I tried to shut down snick instead of .reboot
//  I think it also happened when I jiggled the thumb drive to get it to connect.
    {
AlwaysLogString( "Called with a zero Fcb!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" );
AlwaysLogString( "Just going to pretend all is ok!!!!!!!!!!!!!!!!!!!!!!!\n" );
BreakToDebugger();  //  eb eip 0x90
        return STATUS_SUCCESS;  //  TODO
    }

    if ( Fcb->IsAVolume )
    {
AlwaysLogString( "IrpMjClose called on volume!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" );
        VCB_ Vcb = DeviceObject->DeviceExtension;
        if ( ! Vcb->Dismounted )
        {
AlwaysLogString( "Note: ! Vcb->Dismounted\n" );
           return STATUS_ACCESS_DENIED;//  TODO ?? ------<-----------------------
        }

        Vcb->VcbNumReferences--;
AlwaysLogFormatted( "Vcb->VcbNumReferences now %d\n", Vcb->VcbNumReferences );

        if ( Vcb->VcbNumReferences == 0 )
        {
AlwaysLogString( "Unmaking Vcb\n" );
            UnmakeVcb( Vcb );
        }
    }
    else
    {
        CCB_ Ccb = FileObject->FsContext2;
        UnmakeCcb( Ccb );

        Fcb->NumReferences--;
LogFormatted( "Decremented FcbReferences to %d\n", Fcb->NumReferences );
        if ( Fcb->NumReferences == 0 )
        {

            if ( Fcb->DeleteIsPending )
            {
LogString( "Reference count is 0 and FCB_DELETE_PENDING.\n" );
                if ( IdIsADirectory( Fcb->Id ) && ChildrenTree( Entries[Fcb->Id] ) )
                {
LogString( "DELETE - It is a dir with children; we are supposed to silently ignore the unmake.\n" );
                }
                else
                {
LogString( "DELETE - unmake entry.\n" );
                    UnmakeEntry( Fcb->Id );
                }
            }

            UnmakeFcb( Fcb );
        }
    }

    return STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS IrpMjFlushBuffers( ICB_ Icb )

{
    UNREFERENCED_PARAMETER( Icb );

//  TODO need to do this. Derrrr.
AlwaysLogString( "IrpMjFlushBuffers - hmmmm\n" );

    return 0;

#if 0

////    UNREFERENCED_PARAMETER( Icb );
    PIRP         Irp        = Icb->Irp;
    IRPSP_       IrpSp      = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    FCB_         Fcb        = FileObject->FsContext;

    if ( ! Fcb )
//  I think this happened when I tried to shut down snick instead of .reboot
//  I think it also happened when I jiggled the thumb drive to get it to connect.
    {
AlwaysLogString( "Called with a zero Fcb!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" );
AlwaysLogString( "Just going to pretend all is ok!!!!!!!!!!!!!!!!!!!!!!!\n" );
AlwaysBreakToDebugger();  //  eb eip 0x90
return STATUS_SUCCESS;  //  TODO
    }

//    ID Id = Fcb->Id;
//    B1 IsAVolume    = Fcb->IsAVolume || IdIsAFile( Fcb->Id ) || )
    B1 IsAFile      = ! Fcb->IsAVolume && IdIsAFile( Fcb->Id );
    B1 IsADirectory = ! Fcb->IsAVolume && IdIsADirectory( Fcb->Id );


    //  For any file, do its flush.
    if ( IsAFile )
    {
AlwaysLogString( "Need a flush for this file here.\n" );
        return 0;
    }

    //  For any directory other than the root, just say done.
    if ( IsADirectory && Entries[ Fcb->Id ]->ParentId != 0 )
    {
        //  "File System Internals" says just say done.
AlwaysLogString( "\"File System Internals\" says just say done.\n" );
        return 0;
    }


    //  For a volume or the root directory, we need to flush all.
    //  "File System Internals" says do this synchronously.
    //  Wait for a signal from background task.
AlwaysLogString( "IRP_MJ_FLUSH_BUFFERS for the volume\n" );
AlwaysLogString( "IRP_MJ_FLUSH_BUFFERS for the volume\n" );
AlwaysLogString( "IRP_MJ_FLUSH_BUFFERS for the volume\n" );
AlwaysLogString( "Need a ligit flush of the volume here.\n" );
    for (;;)
    {
        if ( ! BackgroundThread_Busy ) break;
AlwaysLogString( "Sleeping because BackgroundThread_Busy.\n" );
        SleepForMilliseconds( 100 );  //  TODO
    }

    //  "File System Internals" says pass this irp down, and ignore the result.

/*	

//  IoBuildAsynchronousFsdRequest ( IRP_MJ_FLUSH_BUFFERS
PIRP zzz = IoBuildAsynchronousFsdRequest(
IRP_MJ_FLUSH_BUFFERS,
  DeviceObject,  //  next-lower driver's device object PDEVICE_OBJECT
  0,  //  Buffer,
  0,  //  Length,
  0,  //  StartingOffset,
  [in, optional] PIO_STATUS_BLOCK IoStatusBlock
);
ASSERT( zzz );

IoMakeAssociatedIrp

IoBuildSynchronousFsdRequest

*/

    return 0;



#endif

 }

//////////////////////////////////////////////////////////////////////

NTSTATUS IrpMjShutdown( ICB_ Icb )

{
    UNREFERENCED_PARAMETER( Icb );
AlwaysLogString( "IRP_MJ_SHUTDOWN\n" );
AlwaysLogString( "IRP_MJ_SHUTDOWN\n" );
AlwaysLogString( "IRP_MJ_SHUTDOWN\n" );


//  TODO Should I pass down the IRP_MJ_SHUTDOWN like I am passing down IRP_MJ_FLUSH_BUFFERS?

    return STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


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

//////CDFS "This routine searches for all the volumes mounted on the same real device
//////      of the current DASD handle, and marks them all bad.  The only operation
//////      that can be done on such handles is cleanup and close."

NTSTATUS FsctlInvalidateVolumes( ICB_ Icb )

{
    UNREFERENCED_PARAMETER( Icb );

AlwaysLogString( "Supposed to search for the mounted volumes and mark them cleanup and close only. \n" );
    return STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS FsctlIsPathnameValid( ICB_ Icb )

{
    UNREFERENCED_PARAMETER( Icb );

AlwaysLogString( "FsctlIsPathnameValid always returning STATUS_SUCCESS just like fastfat !!!!!!!!!!!!!!!!!! \n" );

    return STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS TailwindChatSet( S1_ InputBuffer, S1_ OutputBuffer, U4 OutputBufferLength )

{
    UNREFERENCED_PARAMETER( OutputBufferLength );

    ChatVariable = 0;

    for ( int i = 4;; i++ )
    {
        int digit = InputBuffer[i];
        if ( ! digit ) break;
        ChatVariable = ChatVariable * 10 + digit - '0';
    }

    sprintf( OutputBuffer, "ChatVariable set to %d\n", ChatVariable );

    return 0;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS TailwindChatTest( S1_ InputBuffer, S1_ OutputBuffer, U4 OutputBufferLength )

{
    UNREFERENCED_PARAMETER( InputBuffer );

    S1_ message = "Made it to TailwindChatTest.\n";
    if ( OutputBufferLength < strlen( message ) + 1 ) return STATUS_BUFFER_TOO_SMALL;
    strcpy( OutputBuffer, message );


    //LONG Locked;
    //LONG NumSpinning;
    //sprintf( OutputBuffer, "ChatVariable set to %d\n", Volume_MetadataLock );


    return 0;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS TailwindChat( S1_ InputBuffer, S1_ OutputBuffer, U4 OutputBufferLength )

{

    size_t InputBufferLength = strlen( InputBuffer );
    NTSTATUS Status = 0;


    if ( memcmp( InputBuffer, "echo ", 5 ) == 0 )
    {
        if ( OutputBufferLength < InputBufferLength ) return STATUS_BUFFER_TOO_SMALL;
        strcpy( OutputBuffer, InputBuffer + 5 );
    }


    else
    if ( memcmp( InputBuffer, "set ", 4 ) == 0 )
    {
        return TailwindChatSet( InputBuffer, OutputBuffer, OutputBufferLength );
    }


    else
    if ( ( strcmp( InputBuffer, "test" ) == 0 ) || ( memcmp( InputBuffer, "test ", 5 ) == 0 ) )
    {
        return TailwindChatTest( InputBuffer, OutputBuffer, OutputBufferLength );
    }


    else
    if ( strcmp( InputBuffer, "break" ) == 0 )
    {
        if ( OutputBufferLength < 20 ) return STATUS_BUFFER_TOO_SMALL;
        strcpy( OutputBuffer, "(About to break.)" );
DbgBreakPoint();
    }


    else
    if ( strcmp( InputBuffer, "report" ) == 0 )
    {

        S1 report[1024];
        Status = CacheReport( report, 1024 );
        ULONG reportLength = ( ULONG ) strlen( report );
        if ( Status ) return Status;
        if ( OutputBufferLength < reportLength ) return STATUS_BUFFER_TOO_SMALL;
        strcpy( OutputBuffer, report );
    }


    else
    {
        if ( OutputBufferLength < 6 ) return STATUS_BUFFER_TOO_SMALL;
        strcpy( OutputBuffer, "What?" );
    }


    return 0;
}

//////////////////////////////////////////////////////////////////////

//  Buffer Descriptions for I/O Control Codes
//  https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/buffer-descriptions-for-i-o-control-codes
//  "The size of the space that the system allocates for the single input/output buffer
//   is the larger of the two length values."

NTSTATUS FsctlTailwindChat( ICB_ Icb )

{
    PIRP   Irp                = Icb->Irp;
    IRPSP_ IrpSp              = IoGetCurrentIrpStackLocation( Irp );
    ULONG  InputBufferLength  = IrpSp->Parameters.FileSystemControl.InputBufferLength;
    ULONG  OutputBufferLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;
    S1_    Buffer             = Irp->AssociatedIrp.SystemBuffer;

    //ULONG MaxBufferLength    = max( InputBufferLength, OutputBufferLength );

    //  Make a copy of the buffer before we write back onto it.
    S1_ InputBuffer = AllocateMemory( InputBufferLength );
    if ( ! InputBuffer ) return STATUS_INSUFFICIENT_RESOURCES;

    //memcpy( InputBuffer, Buffer, InputBufferLength );
    //InputBuffer[ InputBufferLength      -     1 ] = 0;
    for ( ULONG i = 0;; i++ )
    {
        if ( i == InputBufferLength ) return STATUS_INVALID_PARAMETER;  //  InputBuffer wasn't a zero-terminated string.
        InputBuffer[i] = Buffer[i];
        if ( InputBuffer[i] == 0 ) break;
    }




    //  Now, we can use the buffer for output.
    S1_ OutputBuffer = Buffer;
    OutputBuffer[0] = 0;
    OutputBuffer[OutputBufferLength   -    1 ] = 0;



    NTSTATUS Status = TailwindChat( InputBuffer, OutputBuffer, OutputBufferLength );
    if ( ! Status )
        Irp->IoStatus.Information = strlen( OutputBuffer ) + 1;


    FreeMemory( InputBuffer );
    return Status;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS FsctlUserFsRequest( ICB_ Icb )

{
    PIRP     Irp           = Icb->Irp;
    IRPSP_   IrpSp         = IoGetCurrentIrpStackLocation( Irp );
    ULONG    FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;


//    PDEVICE_OBJECT DeviceObject = Icb->DeviceObject;
//    if ( DeviceObject != Volume_TailwindDeviceObject )
//{
//AlwaysLogFormatted( "WRONG DEVICE returning STATUS_INVALID_DEVICE_REQUEST on $%X\n", FsControlCode );
//return STATUS_INVALID_DEVICE_REQUEST;  //  5480 calls for a play surf  ( roughly eq to  5399 calls below )
//}

//  http://www.osronline.com/section.cfm%5Eid=27.htm
//  https://community.osr.com/t/finding-start-ioctl-value-from-ctl-code/52393
//  https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/defining-i-o-control-codes?redirectedfrom=MSDN
//  http://www.osronline.com/article.cfm%5earticle=229.htm


S1 AsText[200];
GetFsControlCodeAsText( FsControlCode, AsText );
AlwaysLogFormatted( "FsControlCode is $%X %s\n", FsControlCode, AsText );


    switch ( FsControlCode )
    {
      case FSCTL_INVALIDATE_VOLUMES:  return FsctlInvalidateVolumes( Icb );
      case FSCTL_LOCK_VOLUME:         return FsctlLockVolume(        Icb );
      case FSCTL_UNLOCK_VOLUME:       return FsctlUnlockVolume(      Icb );
      case FSCTL_DISMOUNT_VOLUME:     return FsctlDismountVolume(    Icb );
      case FSCTL_MARK_VOLUME_DIRTY:   return FsctlMarkVolumeDirty(   Icb );
      case FSCTL_IS_VOLUME_DIRTY:     return FsctlIsVolumeDirty(     Icb );//??value order
      case FSCTL_IS_VOLUME_MOUNTED:   return FsctlVerifyVolume(      Icb );
      case FSCTL_IS_PATHNAME_VALID:   return FsctlIsPathnameValid(   Icb );
      case FSCTL_TAILWIND_CHAT:       return FsctlTailwindChat(      Icb );

      case FSCTL_QUERY_RETRIEVAL_POINTERS:  //  drop through  reactos fastfat_new fsctl.c returns this, not STATUS_INVALID_PARAMETER;
      case FSCTL_READ_FILE_USN_DATA:        //  drop through

      default:

if ( FsControlCode == 0x2220A6 )
AlwaysLogString( "  ( I noticed I get this when I press the refresh icon in folders. ) \n" );

        return STATUS_INVALID_DEVICE_REQUEST;
    }
}
//  $90054 =   FILE_SYSTEM( $9 )   Function $15   Access FILE_ANY_ACCESS   Method METHOD_BUFFERED

//////////////////////////////////////////////////////////////////////

//  https://community.osr.com/discussion/14623/purge-volume
//  FSCTL_DISMOUNT_VOLUME will both flush and purge(discard) the file system
//  cache for a volume, AND it will work even if you don't already have a lock
//  on the volume. All you need is a valid handle to the volume.

//  https://community.osr.com/discussion/257502/how-to-purge-all-the-cache-of-files

void PurgeVolumesFiles ( VCB_ Vcb, B1 FlushBeforePurge )

{
    UNREFERENCED_PARAMETER( FlushBeforePurge );

    for ( LINK_ L = Vcb->OpenFcbsChain.First; L; L = L->Next )
    {
        FCB_ Fcb = OWNER( FCB, OpenFcbsLink, L );

        ENTRY_ Entry = Entries[Fcb->Id];

AlwaysLogFormatted( "Should be purging %s here.   %d\n", Entry->Name, FlushBeforePurge );

    }

}

//////////////////////////////////////////////////////////////////////

NTSTATUS IrpMjFileSystemControl( ICB_ Icb )

{
    PIRP     Irp   = Icb->Irp;
    IRPSP_   IrpSp = IoGetCurrentIrpStackLocation( Irp );

//S1 AsTextMn[200];
//GetFsControlCodeMinorAsText( IrpSp->MinorFunction, AsTextMn );
//LogFormatted( "minor $%X %s\n", IrpSp->MinorFunction, AsTextMn );

    switch ( IrpSp->MinorFunction )
    {
      case IRP_MN_USER_FS_REQUEST:  return FsctlUserFsRequest( Icb );
      case IRP_MN_MOUNT_VOLUME:     return FsctlMountVolume(   Icb );
      case IRP_MN_VERIFY_VOLUME:    return FsctlVerifyVolume(  Icb );
      default:                      return STATUS_INVALID_DEVICE_REQUEST;
    }

}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


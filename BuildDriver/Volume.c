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

/*

Created a workable volume by:
1 ) Formatted a ( blue 32gb ) USB drive as ExFAT ( not quick == zeros unused bytes )
2 ) Ran HxD ( 2.5.0.0 ) as admin
3 ) Tools > Open Disk > uncheck "Open as Readonly"
4 ) > Physical Disk > Removable disk 1 > OK
5 ) edit      10: t a i l w i n d   i m a g e 00 00              <- tailwind
    edit     1fe: 55 bb ( from 55 aa )
    edit 10'01fe: 55 bb ( from 55 aa )
6 ) Save, exit, unmount, and reboot.
Note: after the format, $200 thru $F'FFFF were 00, $10'0000 was eb 76 90 EXFAT

Also did:
    new!      3: T A I L W I N D
    edit    400: t a i l w i n d 00           <- tailwind
    edit    410: 1 1 1 1
    edit    420: T a i l w i n d 0 00         <- Tailwind0
    edit    430: 2 2 2 2

    also    440: 00 00 00 00 00 00 00 00   = volume has nothing on it
    also    440: ( any other 8 bytes )     = address of the Overview
*/

//////////////////////////////////////////////////////////////////////

//  https://github.com/dosfstools/dosfstools/blob/master/src/mkfs.fat.c

//////////////////////////////////////////////////////////////////////

void SetVpbBit( PVPB Vpb, USHORT Flag )

{
    KIRQL SavedIrql;
    IoAcquireVpbSpinLock( &SavedIrql );
    SetBit( Vpb->Flags, Flag );
    IoReleaseVpbSpinLock( SavedIrql );
}

//////////////////////////////////////////////////////////////////////

void ClearVpbBit( PVPB Vpb, USHORT Flag )

{
    KIRQL SavedIrql;
    IoAcquireVpbSpinLock( &SavedIrql );
    ClearBit( Vpb->Flags, Flag );
    IoReleaseVpbSpinLock( SavedIrql );
}

//////////////////////////////////////////////////////////////////////

NTSTATUS volumeStartup(  )

{
    Volume_OverviewStart  =               2 * 1024 * 1024;  //    $20'0000
    Volume_EntriesStart   =              80 * 1024 * 1024;  //   $500'0000
    Volume_DataStart      =             280 * 1024 * 1024;  //  $1180'0000

////Volume_TotalNumBytes           = 1LL * 32 * 1024 * 1024 * 1024;  //  32GB TODO temp as variable for testing
    Volume_TotalNumBytes           = 1LL *  2 * 1024 * 1024 * 1024;  //   2GB TODO temp as variable for testing

    Volume_OverviewNumBytes = Volume_EntriesStart  - Volume_OverviewStart;
    Volume_EntriesNumBytes  = Volume_DataStart     - Volume_EntriesStart;
    Volume_DataNumBytes     = Volume_TotalNumBytes - Volume_DataStart;

    return 0;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS verifyItIsSameDevice( PDEVICE_OBJECT DeviceObject, VCB_ Vcb )

{
    U1_ Buffer = AllocateMemory( Volume_BlockSize );  //  TODO
    if ( ! Buffer ) return STATUS_INSUFFICIENT_RESOURCES;

    NTSTATUS Status = ReadBlockDevice( DeviceObject, 0, Volume_BlockSize, Buffer, NO_VERIFY );
    if ( NT_SUCCESS( Status ) )
    {
        B1 VolumeHeaderSame = ! memcmp( Vcb->FirstBlock + 0x400, Buffer+0x400, 0x40 );
        Status = VolumeHeaderSame ? STATUS_SUCCESS : STATUS_WRONG_VOLUME;
    }

    ExFreePool( Buffer );

    return Status;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS verifyItIsOurTypeDevice( PDEVICE_OBJECT DeviceObject )

{
    U1_ Buffer = AllocateMemory( Volume_BlockSize );
    if ( ! Buffer ) return STATUS_INSUFFICIENT_RESOURCES;

    NTSTATUS Status = ReadBlockDevice( DeviceObject, 0, Volume_BlockSize, Buffer, NO_VERIFY );
    if ( NT_SUCCESS( Status ) )
    {
        B1 TypeIsTheSame = ! memcmp( Buffer + 0x400, OurFileSystemType, sizeof( OurFileSystemType ) );
        Status = TypeIsTheSame ? STATUS_SUCCESS : STATUS_WRONG_VOLUME;
    }

    ExFreePool( Buffer );

    return Status;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS PrepareTheFileSystem( VCB_ Vcb )

{
    NTSTATUS Status;

UINT64 msFm, msTo;
msFm = CurrentMillisecond();


    //  Set Volume_OverviewStart,Volume_EntriesStart,Volume_DataStart,Volume_EntriesNumBytes, etc.
    Status = volumeStartup();
    if ( Status ) return Status;


    //  Create the initial big chunks.
    Status = SpaceStartup();
    if ( Status ) return Status;


    //  Initialize the Cache to zeroes.
    Status = CacheStartup();
    if ( Status ) return Status;


msTo = CurrentMillisecond();
AlwaysLogFormatted( "%d ms.\n", ( int ) ( msTo - msFm ) );
msFm = CurrentMillisecond();


    U8 OverviewAddress = * ( U8_ ) ( Vcb->FirstBlock + 0x440 );
    if ( OverviewAddress )
    {
AlwaysLogFormatted( "There is an overview address of %X\n", ( U4 ) OverviewAddress );


msTo = CurrentMillisecond();
AlwaysLogFormatted( "%d ms.\n", ( int ) ( msTo - msFm ) );
msFm = CurrentMillisecond();


        //  Recover Volume_TotalNumberOfEntries, Volume_WhereTableMaxId, Volume_OverviewStart, etc.
	    Status = OverviewThaw();
ASSERT( ! Status );


msTo = CurrentMillisecond();
AlwaysLogFormatted( "%d ms.\n", ( int ) ( msTo - msFm ) );
msFm = CurrentMillisecond();


	    Status = EntriesThaw();
ASSERT( ! Status );


msTo = CurrentMillisecond();
AlwaysLogFormatted( "%d ms.\n", ( int ) ( msTo - msFm ) );
msFm = CurrentMillisecond();


        Status = SpaceBuildFromEntries();
ASSERT( ! Status );


msTo = CurrentMillisecond();
AlwaysLogFormatted( "%d ms.\n", ( int ) ( msTo - msFm ) );
msFm = CurrentMillisecond();



Vcb->RootId = 1;




    }
    else
    {
AlwaysLogString( "There is   NO   overview address\n" );


        //  Allocate EntriesBytes and set EntriesFirstFreeByte = 0, etc.
        int NumBytesToAllocateToTheMetadataStore = ROUND_UP( 1'000'000, 4096 );
        Status = EntriesStartupEmpty( NumBytesToAllocateToTheMetadataStore );  //  TODO
        if ( Status ) return Status;


        //  Allocate Entries, and set Volume_WhereTableLastRecycledID = 0, etc.
        Status = WhereTableStartupEmpty( NumBytesToAllocateToTheMetadataStore / 4 );  //  TODO
        if ( Status ) return Status;


        //  Create the root directory.
        ID Id = MakeEntry( 0, A_DIRECTORY, "" );

        if ( ! Id )
        {
            //TODO                delete mem alloc here  a la AirfsDelete( Airfs );
            return STATUS_INSUFFICIENT_RESOURCES;//TODO
        }
        Vcb->RootId = Id;


    }

//Volume_LastMetadataOkCount = Volume_ConservativeMetadataUpdateCount;

#if BACKGROUND_THREAD == YES
    Status = BackgroundThreadStartup( Vcb );
    if ( Status ) return Status;
#else
    AlwaysLogString( "Background thread OFF!\n" );
#endif

    return 0;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS FsctlMountVolume( ICB_ Icb )

{
AlwaysLogString( "top\n" );

    PDEVICE_OBJECT DeviceObject = Icb->DeviceObject;
    if ( DeviceObject != Volume_TailwindDeviceObject ) return STATUS_INVALID_DEVICE_REQUEST;

////VCB_ Vcb = DeviceObject->DeviceExtension;

    PIRP            Irp                  = Icb->Irp;
    IRPSP_          IrpSp                = IoGetCurrentIrpStackLocation( Irp );
    PDEVICE_OBJECT  PhysicalDeviceObject = IrpSp->Parameters.MountVolume.DeviceObject;



Volume_PhysicalDeviceObject = PhysicalDeviceObject;

AlwaysLogString( "is it our device type?\n" );


    NTSTATUS Status = verifyItIsOurTypeDevice( PhysicalDeviceObject );
    if ( ! NT_SUCCESS( Status ) ) return Status;

AlwaysLogString( "yes\n" );

    PDEVICE_OBJECT  VolumeDeviceObject;
    Status = IoCreateDevice( DeviceObject->DriverObject, sizeof( VCB ), 0,
             FILE_DEVICE_DISK_FILE_SYSTEM, 0, FALSE, &VolumeDeviceObject );
// a la fastfat neither FILE_DEVICE_FILE_SYSTEM nor FILE_DEVICE_DISK
    if ( ! NT_SUCCESS( Status ) ) return Status;

    VolumeDeviceObject->StackSize = PhysicalDeviceObject->StackSize + 1;

    VCB_ Vcb = MakeVcb( Icb,  VolumeDeviceObject, PhysicalDeviceObject );


    Status = STATUS_UNSUCCESSFUL;

    do
    {

        Vcb->FirstBlock = AllocateMemory( Volume_BlockSize );
        if ( ! Vcb->FirstBlock ) { Status = STATUS_INSUFFICIENT_RESOURCES; break; }

        Status = ReadBlockDevice( Vcb->PhysicalDeviceObject, 0, Volume_BlockSize, Vcb->FirstBlock, MAY_VERIFY );
        if ( ! NT_SUCCESS( Status ) ) break;

        WCHAR WideLabel[MAXIMUM_VOLUME_LABEL_LENGTH];//MAX_PATH];//TODO MAXIMUM_VOLUME_LABEL_LENGTH
        int NumWideChars = Utf8StringToWideString( ( char* )Vcb->FirstBlock + 0x420, WideLabel );
        Vcb->Vpb->VolumeLabelLength = ( USHORT )( NumWideChars * 2 );
        memcpy( Vcb->Vpb->VolumeLabel, WideLabel, NumWideChars * 2LL );  //  TODO

        Vcb->Vpb->SerialNumber = *( ULONG* )( Vcb->FirstBlock + 0x430 );

        Status = DeviceIoControlRequest( IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, PhysicalDeviceObject, 0, 0, &Vcb->DiskGeometryEx, sizeof( DISK_GEOMETRY_EX ) );
        if ( ! NT_SUCCESS( Status ) ) break;

        Status = DeviceIoControlRequest( IOCTL_DISK_GET_PARTITION_INFO_EX, PhysicalDeviceObject, 0, 0, &Vcb->PartitionInformationEx, sizeof( PARTITION_INFORMATION_EX ) );

    } while ( 0 );

    if ( ! NT_SUCCESS( Status ) )
    {

BreakToDebugger();  //  TODO Got an access voilation on the line below once. 0xC0000005
        FreeMemory( Vcb->FirstBlock );

        if ( VolumeDeviceObject ) IoDeleteDevice( VolumeDeviceObject );
    }

    if ( NT_SUCCESS( Status ) )
    {
        PrepareTheFileSystem( Vcb );

        ClearBit( VolumeDeviceObject->Flags, DO_DEVICE_INITIALIZING );
    }

    return Status;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS FsctlVerifyVolume( ICB_ Icb )

{
    PDEVICE_OBJECT DeviceObject = Icb->DeviceObject;
    if ( DeviceObject == Volume_TailwindDeviceObject ) return STATUS_INVALID_DEVICE_REQUEST;

    VCB_ Vcb = DeviceObject->DeviceExtension;
    PDEVICE_OBJECT PhysicalDeviceObject = Vcb->PhysicalDeviceObject;
    if ( BitIsClear( PhysicalDeviceObject->Flags, DO_VERIFY_VOLUME ) ) return STATUS_SUCCESS;

//  TODO  IoGetDeviceToVerify   IoSetDeviceToVerify	


BreakToDebugger();

    NTSTATUS Status = verifyItIsSameDevice( PhysicalDeviceObject, Vcb );

    if ( ! NT_SUCCESS( Status ) ) return STATUS_WRONG_VOLUME;

    ClearBit( PhysicalDeviceObject->Flags, DO_VERIFY_VOLUME );

    return Status;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS FsctlLockVolume( ICB_ Icb )

{
    PDEVICE_OBJECT DeviceObject = Icb->DeviceObject;
    if ( DeviceObject == Volume_TailwindDeviceObject ) return STATUS_INVALID_DEVICE_REQUEST;

    VCB_ Vcb = DeviceObject->DeviceExtension;
    if ( Vcb->VolumeLocked         ) return STATUS_ACCESS_DENIED;
    if ( Vcb->VcbNumOpenHandles    ) return STATUS_ACCESS_DENIED;
    if ( Vcb->VcbNumReferences > 1 ) return STATUS_ACCESS_DENIED;

    PurgeVolumesFiles( Vcb, TRUE );  //  TODO flush?

    Vcb->VolumeLocked = TRUE;
    SetVpbBit( Vcb->Vpb, VPB_LOCKED );

    return STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS FsctlUnlockVolume( ICB_ Icb )

{
    PDEVICE_OBJECT DeviceObject = Icb->DeviceObject;
    if ( DeviceObject == Volume_TailwindDeviceObject ) return STATUS_INVALID_DEVICE_REQUEST;

    VCB_ Vcb = DeviceObject->DeviceExtension;
    if ( ! Vcb->VolumeLocked ) return STATUS_ACCESS_DENIED;

    Vcb->VolumeLocked = FALSE;
    ClearVpbBit( Vcb->Vpb, VPB_LOCKED );

    return STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS FsctlDismountVolume( ICB_ Icb )

{
    PDEVICE_OBJECT DeviceObject = Icb->DeviceObject;
AlwaysLogFormatted( "FsctlDismountVolume only OK if DeviceObject == Volume_TailwindDeviceObject; = %d\n", DeviceObject == Volume_TailwindDeviceObject );
    if ( DeviceObject == Volume_TailwindDeviceObject ) return STATUS_INVALID_DEVICE_REQUEST;

    VCB_ Vcb = DeviceObject->DeviceExtension;
AlwaysLogFormatted( "FsctlDismountVolume only OK if VolumeLocked; = %d\n", Vcb->VolumeLocked );
    if ( ! Vcb->VolumeLocked ) return STATUS_ACCESS_DENIED;


AlwaysLogString( "Have some writes to do still?\n" );
//AlwaysBreakToDebugger();

    Vcb->Dismounted = TRUE;
    NTSTATUS Status = 0;
//
//    NTSTATUS Status = VolumeRootShutdown( Vcb );
//
//    if ( NT_SUCCESS( Status ) ) Vcb->Dismounted = TRUE;
//



#if BACKGROUND_THREAD == YES
    Status = BackgroundThreadShutdown();
    if ( Status ) AlwaysLogFormatted( "BackgroundThreadShutdown reported a status of $%X\n", Status );
#endif


    Status = EntriesShutdown();
if ( Status ) AlwaysLogFormatted( "EntriesShutdown reported a status of $%X\n", Status );


    Status = WhereTableShutdown();
if ( Status ) AlwaysLogFormatted( "WhereTableShutdown reported a status of $%X\n", Status );


    Status = CacheShutdown();
    if ( Status ) AlwaysLogFormatted( "CacheShutdown reported a status of $%X\n", Status );


    Status = SpaceShutdown();
    if ( Status ) AlwaysLogFormatted( "SpaceShutdown reported a status of $%X\n", Status );


    return Status;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS FsctlMarkVolumeDirty( ICB_ Icb )

{
//    PIRP     Irp           = Icb->Irp;
//    IRPSP_   IrpSp         = IoGetCurrentIrpStackLocation( Irp );
AlwaysLogString( "FsctlMarkVolumeDirty!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n" );

//    if ( Irp->AssociatedIrp.SystemBuffer == 0 )
//        return STATUS_INVALID_USER_BUFFER;
//
//    if ( IrpSp->Parameters.FileSystemControl.OutputBufferLength < sizeof( ULONG ) )
//        return STATUS_INVALID_PARAMETER;

    PDEVICE_OBJECT DeviceObject = Icb->DeviceObject;
    VCB_ Vcb = DeviceObject->DeviceExtension;
    if ( ! Vcb->IsAVolume ) return STATUS_INVALID_PARAMETER;

//TODO    if ( is mounted ) return STATUS_VOLUME_DISMOUNTED;

//    PULONG VolumeState = Irp->AssociatedIrp.SystemBuffer;
//    *VolumeState = Vcb->VolumeDirty;
    Vcb->VolumeDirty = TRUE;
	
//    Irp->IoStatus.Information = sizeof( ULONG );

AlwaysLogString( "FsctlMarkVolumeDirty returning success!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n" );

    return STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS FsctlIsVolumeDirty( ICB_ Icb )

{
AlwaysLogString( "FsctlIsVolumeDirty!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n" );

    PIRP Irp = Icb->Irp;
    if ( Irp->AssociatedIrp.SystemBuffer == 0 ) return STATUS_INVALID_USER_BUFFER;

    IRPSP_  IrpSp = IoGetCurrentIrpStackLocation( Irp );
    ULONG OutputBufferLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;
    if ( OutputBufferLength < sizeof( ULONG ) ) return STATUS_INVALID_PARAMETER;

    PDEVICE_OBJECT DeviceObject = Icb->DeviceObject;
    VCB_ Vcb = DeviceObject->DeviceExtension;
    if ( ! Vcb->IsAVolume ) return STATUS_INVALID_PARAMETER;

//TODO    if ( is mounted ) return STATUS_VOLUME_DISMOUNTED;

    PULONG VolumeState = Irp->AssociatedIrp.SystemBuffer;
    *VolumeState = Vcb->VolumeDirty;
	
    Irp->IoStatus.Information = sizeof( ULONG );

AlwaysLogFormatted( "FsctlIsVolumeDirty returning %d !!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n", ( int ) *VolumeState );

    return STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS IrpMjQueryVolumeInformation( ICB_ Icb )

{
    PIRP                 Irp               = Icb->Irp;
    IRPSP_               IrpSp             = IoGetCurrentIrpStackLocation( Irp );
    V_                   Buffer            = Irp->AssociatedIrp.SystemBuffer;
    ULONG                BufferLength      = IrpSp->Parameters.QueryVolume.Length;
    FS_INFORMATION_CLASS Class             = IrpSp->Parameters.QueryVolume.FsInformationClass;
    int                  NumBytesAvailable = ( int ) BufferLength;
    int                  NumBytesReturning = 0;
    int                  NumBytesMinimum;
    int                  NumBytesIdeal;
    int                  NumBytesRequired;

    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    do
    {

        PDEVICE_OBJECT DeviceObject = Icb->DeviceObject;
        if ( DeviceObject == Volume_TailwindDeviceObject )
        {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
        VCB_ Vcb = DeviceObject->DeviceExtension;


        Zero( Buffer, BufferLength );


        switch ( Class )
        {
	
//--------------------------------------------------------------------

        case FileFsVolumeInformation:
        {
LogString( "Query FileFsVolumeInformation\n" );
            PFILE_FS_VOLUME_INFORMATION Info = Buffer;

            NumBytesMinimum = offsetof( FILE_FS_VOLUME_INFORMATION, VolumeLabel );
            if ( NumBytesAvailable < NumBytesMinimum ) { Status = STATUS_INFO_LENGTH_MISMATCH; break; }

            WCHAR WideVolumeLabel[MAX_PATH];//TODO
            int NumWideChars = Utf8StringToWideString( ( char* )Vcb->FirstBlock+0x420, WideVolumeLabel );  //  TODO

            NumBytesIdeal = NumBytesMinimum + NumWideChars * 2;
            NumBytesReturning = min( NumBytesIdeal, NumBytesAvailable );

            Info->VolumeCreationTime = DriverEntryTime;
            Info->VolumeSerialNumber = *( ULONG* )( Vcb->FirstBlock+0x430 );  //  TODO
            Info->SupportsObjects    = FALSE;
            Info->VolumeLabelLength  = NumWideChars * 2;
            memcpy( Info->VolumeLabel, WideVolumeLabel, ( size_t ) NumBytesReturning - NumBytesMinimum );

            Status = ( NumBytesReturning == NumBytesIdeal ) ? STATUS_SUCCESS : STATUS_BUFFER_OVERFLOW;
        }
        break;

//--------------------------------------------------------------------

        case FileFsSizeInformation:
        {
LogString( "Query FileFsSizeInformation\n" );
            PFILE_FS_SIZE_INFORMATION Info = Buffer;

            NumBytesRequired = sizeof( *Info );
            if ( NumBytesAvailable < NumBytesRequired ) { Status = STATUS_INFO_LENGTH_MISMATCH; break; }

            ULONG BytesPerSector = Vcb->DiskGeometryEx.Geometry.BytesPerSector;

            Info->TotalAllocationUnits.QuadPart     = Vcb->PartitionInformationEx.PartitionLength.QuadPart / Volume_BlockSize;
            Info->AvailableAllocationUnits.QuadPart = ( Vcb->PartitionInformationEx.PartitionLength.QuadPart - FAKE_USED_NUM_BYTES ) / Volume_BlockSize;
            Info->SectorsPerAllocationUnit          = Volume_BlockSize / BytesPerSector;
            Info->BytesPerSector                    = BytesPerSector;

            NumBytesReturning = NumBytesRequired;
            Status = STATUS_SUCCESS;
        }
        break;

//--------------------------------------------------------------------

        case FileFsDeviceInformation:
        {
LogString( "Query FileFsDeviceInformation\n" );
AlwaysBreakToDebugger();  //  TODO untested code
            PFILE_FS_DEVICE_INFORMATION Info = Buffer;

            NumBytesRequired = sizeof( *Info );
            if ( NumBytesAvailable < NumBytesRequired ) { Status = STATUS_INFO_LENGTH_MISMATCH; break; }

            Info->DeviceType      = Vcb->PhysicalDeviceObject->DeviceType;

            ////->DeviceType = FILE_DEVICE_DISK;  //  fastfat
            //// device->DeviceType = dcb->DeviceType;
            ////->DeviceType = Vcb->TargetDeviceObject->DeviceType;
            ////->DeviceType = FILE_DEVICE_CD_ROM;
            //AlwaysLogFormatted( "Query FileFsDeviceInformation device %d or %d ???????\n", Vcb->PhysicalDeviceObject->DeviceType, FILE_DEVICE_DISK );
            //            //fo->DeviceType      = Vcb->PhysicalDeviceObject->DeviceType;
            //            Info->DeviceType      = FILE_DEVICE_DISK;

            Info->Characteristics = Vcb->PhysicalDeviceObject->Characteristics;

            NumBytesReturning = NumBytesRequired;
            Status = STATUS_SUCCESS;
        }
        break;

//--------------------------------------------------------------------

        case FileFsAttributeInformation:
        {
LogString( "Query FileFsAttributeInformation\n" );
            PFILE_FS_ATTRIBUTE_INFORMATION Info = Buffer;

            NumBytesMinimum = offsetof( FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName );
            if ( NumBytesAvailable < NumBytesMinimum ) { Status = STATUS_INFO_LENGTH_MISMATCH; break; }
	
            WCHAR WideFileSystemName[MAX_PATH];//TODO
			int NumWideChars = Utf8StringToWideString( DRIVER_NAME, WideFileSystemName );
	
            NumBytesIdeal = NumBytesMinimum + NumWideChars * 2;
            NumBytesReturning = min( NumBytesIdeal, NumBytesAvailable );


////////////Info->FileSystemAttributes       = FILE_CASE_SENSITIVE_SEARCH | FILE_CASE_PRESERVED_NAMES;
            //  See https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-fscc/ebc7e6e5-4650-4e54-b17c-cf60f6fbeeaa
            Info->FileSystemAttributes = 0
                //  | FILE_SUPPORTS_OPEN_BY_FILE_ID  //  TODO support
                //  | FILE_READ_ONLY_VOLUME
                //  | FILE_CASE_PRESERVED_NAMES
                //  | FILE_CASE_SENSITIVE_SEARCH
// | FILE_CASE_PRESERVED_NAMES
// | FILE_UNICODE_ON_DISK
                ;


            Info->MaximumComponentNameLength = 255;
            Info->FileSystemNameLength       = NumWideChars * 2;
            memcpy( Info->FileSystemName, WideFileSystemName, ( size_t )NumBytesReturning - NumBytesMinimum );

            Status = ( NumBytesReturning == NumBytesIdeal ) ? STATUS_SUCCESS : STATUS_BUFFER_OVERFLOW;
        }
        break;

//--------------------------------------------------------------------

        case FileFsFullSizeInformation:
        {
LogString( "Query FileFsFullSizeInformation\n" );
            PFILE_FS_FULL_SIZE_INFORMATION Info = Buffer;

            NumBytesRequired = sizeof( *Info );
            if ( NumBytesAvailable < NumBytesRequired ) { Status = STATUS_INFO_LENGTH_MISMATCH; break; }

            ULONG BytesPerSector = Vcb->DiskGeometryEx.Geometry.BytesPerSector;

            Info->TotalAllocationUnits.QuadPart           = Vcb->PartitionInformationEx.PartitionLength.QuadPart / Volume_BlockSize;
            Info->CallerAvailableAllocationUnits.QuadPart =
            Info->ActualAvailableAllocationUnits.QuadPart = ( Vcb->PartitionInformationEx.PartitionLength.QuadPart - FAKE_USED_NUM_BYTES ) / Volume_BlockSize;
            Info->SectorsPerAllocationUnit                = Volume_BlockSize / BytesPerSector;
            Info->BytesPerSector                          = BytesPerSector;

            NumBytesReturning = NumBytesRequired;
            Status = STATUS_SUCCESS;
        }
        break;

//--------------------------------------------------------------------

        default:
        {
BreakToDebugger();  //  untested
LogFormatted( "Returning STATUS_INVALID_PARAMETER for Query info class of %d\n", Class );
            Status = STATUS_INVALID_PARAMETER;  //  TODO looks like fastfat returns this
        }
        break;
        }

//--------------------------------------------------------------------

    } while ( 0 );

    Irp->IoStatus.Information = NumBytesReturning;

    if ( Status == STATUS_INFO_LENGTH_MISMATCH || Status == STATUS__PRIVATE__NEVER_SET )
    {
BreakToDebugger();  //  TODO Not sure I should be getting here or that this is the correct status.
    }

    return Status;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


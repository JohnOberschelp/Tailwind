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

void DriverUnload( PDRIVER_OBJECT DriverObject )

{
    UNREFERENCED_PARAMETER( DriverObject );

AlwaysLogString( "Unloading driver.\n" );

    IoDeleteSymbolicLink( &SymbolicName );

    IoUnregisterFileSystem( Volume_TailwindDeviceObject );
    IoDeleteDevice(         Volume_TailwindDeviceObject );

    UninitializeSpinlock( &Volume_MetadataLock );
}

//////////////////////////////////////////////////////////////////////

NTSTATUS IrpMj( ICB_ Icb )

{
    PIRP   Irp   = Icb->Irp;
    IRPSP_ IrpSp = IoGetCurrentIrpStackLocation( Irp );
    NTSTATUS Status;

    switch ( IrpSp->MajorFunction )
    {
      case IRP_MJ_CREATE:                   Status = IrpMjCreate                 ( Icb ); break;
      case IRP_MJ_CLEANUP:                  Status = IrpMjCleanup                ( Icb ); break;
      case IRP_MJ_CLOSE:                    Status = IrpMjClose                  ( Icb ); break;
      case IRP_MJ_READ:                     Status = IrpMjRead                   ( Icb ); break;
      case IRP_MJ_WRITE:                    Status = IrpMjWrite                  ( Icb ); break;
      case IRP_MJ_FLUSH_BUFFERS:            Status = IrpMjFlushBuffers           ( Icb ); break;
      case IRP_MJ_QUERY_INFORMATION:        Status = IrpMjQueryInformation       ( Icb ); break;
      case IRP_MJ_SET_INFORMATION:          Status = IrpMjSetInformation         ( Icb ); break;
      case IRP_MJ_QUERY_VOLUME_INFORMATION: Status = IrpMjQueryVolumeInformation ( Icb ); break;
      case IRP_MJ_DIRECTORY_CONTROL:        Status = IrpMjDirectoryControl       ( Icb ); break;
      case IRP_MJ_FILE_SYSTEM_CONTROL:      Status = IrpMjFileSystemControl      ( Icb ); break;
      case IRP_MJ_DEVICE_CONTROL:           Status = IrpMjDeviceControl          ( Icb ); break;
      case IRP_MJ_LOCK_CONTROL:             Status = IrpMjLockControl            ( Icb ); break;
      case IRP_MJ_SHUTDOWN:                 Status = IrpMjShutdown               ( Icb ); break;

      default:

AlwaysLogFormatted( " Unhandled IRP of ------------------------------- $%X ----- \n", IrpSp->MajorFunction );
AlwaysBreakToDebugger();

        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }


    //  Temporary very crude way to initiate writing metadata.
    if ( NT_SUCCESS( Status ) )
    {
        //switch ( IrpSp->MajorFunction )
        //{
        //  case IRP_MJ_CREATE:            //  maybe
        //  case IRP_MJ_WRITE:             //  maybe
        //  case IRP_MJ_SET_INFORMATION:   //  maybe
            Volume_ConservativeMetadataUpdateCount++;  //  Crude note that we may need to update some metadata.
        //    break;
        //}
    }


    return Status;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS Dispatch( PDEVICE_OBJECT DeviceObject, PIRP Irp )

{
    ICB_ Icb = 0;

    IRPSP_ IrpSp = IoGetCurrentIrpStackLocation( Irp );
    UCHAR MajorFunction = IrpSp->MajorFunction;
//  UCHAR MinorFunction = IrpSp->MinorFunction;

if ( MajorFunction == IRP_MJ_CREATE )
PrintRaw( "\n" );
PrintRaw( "~%04d + %s\n", THETHREAD, GetMajorFunctionName( MajorFunction ) );


    AcquireSpinlock( &Volume_MetadataLock );


    U8 ThisIrpMicrosecondStart = CurrentMicrosecond();
    if ( ! LastIrpMicrosecondStart ) LastIrpMicrosecondStart = ThisIrpMicrosecondStart;

U4 delta = ( U4 ) ( ThisIrpMicrosecondStart - LastIrpMicrosecondStart );
if ( delta > 100 )
{
//PrintRaw( "~%04d + %s ( %d microseconds later )\n", THETHREAD, GetMajorFunctionName( MajorFunction ), delta );
PrintRaw( "~%04d      ( %d microseconds later )\n", THETHREAD, delta );
}
else
{
//PrintRaw( "~%04d + %s\n", THETHREAD, GetMajorFunctionName( MajorFunction ) );
}

    LastIrpMicrosecondStart = ThisIrpMicrosecondStart;


//    B1 AtIrqlPassiveLevel = ( KeGetCurrentIrql() == PASSIVE_LEVEL );
//    if ( AtIrqlPassiveLevel )	
    FsRtlEnterFileSystem();


    NTSTATUS Status;
    Icb = MakeIcb( DeviceObject, Irp );
    if ( ! Icb )
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        Status = IrpMj( Icb );
    }


    if ( Status == STATUS__PRIVATE__NEVER_SET )
    {
AlwaysBreakToDebugger();
AlwaysLogString( "******** SHOULDNT STATUS BE SET? ********\n" );
        Status = STATUS_UNSUCCESSFUL;
    }

    //if ( ! Status ) PrintRaw( "-\n" );
    //else            PrintRaw( "- %s\n", GetStatusAsText( Status ) );

    //  "Never call IoCompleteRequest while holding a spin lock.
    //  Attempting to complete an IRP while holding a spin lock can cause deadlocks."
    // https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-iocompleterequest
    ReleaseSpinlock( &Volume_MetadataLock );


    if ( Icb && Icb->DoCompleteRequest )
    {

        if ( Status == STATUS_PENDING )
        {

ASSERT ( MajorFunction == IRP_MJ_READ );

            if ( ! Irp->MdlAddress )
            {
                PMDL Mdl = IoAllocateMdl( Irp->UserBuffer, IrpSp->Parameters.Read.Length, 0, 0, Irp );
ASSERT ( Mdl );
                try
                {
                    MmProbeAndLockPages( Mdl, Irp->RequestorMode, IoWriteAccess );
                }
                except( EXCEPTION_EXECUTE_HANDLER )
                {
ASSERT( 0 );
                }
            }

            IoMarkIrpPending( Icb->Irp );
            Icb->Irp->IoStatus.Information = 0;  //  TODO ??
            AttachLinkLast( &Volume_PendingReadsChain, &Icb->PendingReadsLink );

        }
        else
        {
            Irp->IoStatus.Status = Status;
            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            UnmakeIcb( Icb );
        }

    }
    else
    {
AlwaysLogString( " NOT completing request.\n" );
    }

//        if ( AtIrqlPassiveLevel )
    FsRtlExitFileSystem();



    //if ( ! Status ) PrintRaw( "-\n" );
    //else            PrintRaw( "- %s = $%X\n", GetStatusAsText( Status ), Status );
    if ( ! Status ) PrintRaw( "~%04d -\n", THETHREAD );
    else            PrintRaw( "~%04d - %s = $%X\n", THETHREAD, GetStatusAsText( Status ), Status );


    return Status;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS DriverEntry( PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath )

{
    UNREFERENCED_PARAMETER( RegistryPath );

    KeQueryPerformanceCounter( &PerformanceCounterFrequencyInTicksPerSecond );
    //  in ticks per second  https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-kequeryperformancecounter


#if DBG
LogString( "Debug version built " __DATE__ " " __TIME__ "\n" );
#else
LogString( "Release version built " __DATE__ " " __TIME__ "\n" );
#endif


    InitializeSpinlock( &Volume_MetadataLock );  //  TODO per volume

    ZeroVolumeGlobals();  //  TODO per volume

    KeQuerySystemTime( &DriverEntryTime );
    RtlInitUnicodeString( &DeviceName   , DEVICE_NAME   );
    RtlInitUnicodeString( &SymbolicName , SYMBOLIC_NAME );
    Volume_DriverObject    = DriverObject;

    DriverObject->DriverUnload = DriverUnload;

    DriverObject->MajorFunction[ IRP_MJ_CREATE                   ] =
    DriverObject->MajorFunction[ IRP_MJ_CLOSE                    ] =
    DriverObject->MajorFunction[ IRP_MJ_READ                     ] =
    DriverObject->MajorFunction[ IRP_MJ_WRITE                    ] =
    DriverObject->MajorFunction[ IRP_MJ_QUERY_INFORMATION        ] =
    DriverObject->MajorFunction[ IRP_MJ_SET_INFORMATION          ] =
    DriverObject->MajorFunction[ IRP_MJ_FLUSH_BUFFERS            ] =
    DriverObject->MajorFunction[ IRP_MJ_QUERY_VOLUME_INFORMATION ] =
    DriverObject->MajorFunction[ IRP_MJ_SET_VOLUME_INFORMATION   ] =
    DriverObject->MajorFunction[ IRP_MJ_DIRECTORY_CONTROL        ] =
    DriverObject->MajorFunction[ IRP_MJ_FILE_SYSTEM_CONTROL      ] =
    DriverObject->MajorFunction[ IRP_MJ_DEVICE_CONTROL           ] =
    DriverObject->MajorFunction[ IRP_MJ_SHUTDOWN                 ] =
    DriverObject->MajorFunction[ IRP_MJ_LOCK_CONTROL             ] =
    DriverObject->MajorFunction[ IRP_MJ_CLEANUP                  ] = Dispatch;

    DriverObject->FastIoDispatch = 0;


    //  Setting the device extension size for the driver to sizeof Volume_
    //  This is accessed by DEVICE_OBJECT.DeviceExtension
    //  see https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ns-wdm-_device_object
    //  Note and TODO I am not using this space yet!!!!!!!!!!!!!!!
    NTSTATUS Status = IoCreateDevice(
            DriverObject,

////////////sizeof( GLOBALS ),
////////////sizeof( VCB ),
            0,

            &DeviceName,
            FILE_DEVICE_DISK_FILE_SYSTEM,
            0, FALSE, &Volume_TailwindDeviceObject );
            // a la fastfat neither FILE_DEVICE_FILE_SYSTEM nor FILE_DEVICE_DISK

    if ( ! NT_SUCCESS( Status ) ) return Status;

    //  TODO Put GLOBALS in DriverObject.DeviceExtension here!???!!!!!!!!!!!

    IoCreateSymbolicLink( &SymbolicName, &DeviceName );

    IoRegisterFileSystem( Volume_TailwindDeviceObject );

////ObReferenceObject( Volume_TailwindDeviceObject );  //  TODO ???


    //ULONG NumActiveProcessors = KeQueryActiveProcessorCount( 0 );

    //SYSTEM_INFO SystemInfo;
    //GetSystemInfo( &SystemInfo );

    return 0;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


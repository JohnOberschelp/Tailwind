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

2022-05-24 on tailwind 2.0.3.5, there were only 2 kinds of read:
times                      after pvlc    after pff install   after pff surf
                           ----------    -----------------    --------------
Mustsync PagingIo Nocache    2877            10                1035
Mustsync NormalIo Maycache    514          9984                8940

*/

//////////////////////////////////////////////////////////////////////

NTSTATUS IrpMjRead_File( ICB_ Icb )

{
    PIRP         Irp            = Icb->Irp;
    IRPSP_       IrpSp          = IoGetCurrentIrpStackLocation( Irp );
    U8           FileOffset     = IrpSp->Parameters.Read.ByteOffset.QuadPart;
    U4           FileNumBytes   = IrpSp->Parameters.Read.Length;
    PFILE_OBJECT FileObject     = IrpSp->FileObject;
    FCB_         Fcb            = FileObject->FsContext;
    CCB_         Ccb            = FileObject->FsContext2;
    ID           Id             = Fcb->Id;

//  https://community.osr.com/discussion/67486/irp-nocache-vs-irp-paging-io
    B1           PagingIo       = BitIsSet( Irp->Flags, IRP_PAGING_IO );
//  B1           Nocache        = BitIsSet( Irp->Flags, IRP_NOCACHE );  //  ex open with FILE_NO_INTERMEDIATE_BUFFERING

if ( PagingIo )  //  https://community.osr.com/t/irp-flag-nocache/1804
LogString( "PagingIo \"call from the Memory Manager (satisfaction of a page fault)\"?\n" );

    B1           SynchronousIo  = BitIsSet( FileObject->Flags, FO_SYNCHRONOUS_IO );

//LogFormatted( "%s %s %s\n",  SynchronousIo?"MustSync":"MaySync",   PagingIo?"PagingIo":"NormalIo",   Nocache?"NoCache":"MayCache" );

//  https://community.osr.com/discussion/256100/what-does-irp-paging-io-means

ASSERT( IrpSp->MinorFunction == IRP_MN_NORMAL );

    Irp->IoStatus.Information = 0;

    if ( IdIsADirectory( Id ) ) return STATUS_INVALID_PARAMETER;

    U1_ Buffer = IrpBuffer( Irp );
    if ( ! Buffer ) return STATUS_INVALID_USER_BUFFER;

    U8 FileSize = DataGetFileNumBytes( Entries[Id] );

LogFormatted( "%s  FileOffset %d  FileNumBytes %d ( FileSize %d )\n",
Entries[Id]->Name, ( int ) FileOffset, ( int ) FileNumBytes, ( int ) FileSize );

    if ( FileNumBytes == 0 && FileOffset <= FileSize ) return STATUS_SUCCESS;

    if ( FileOffset >= FileSize ) return STATUS_END_OF_FILE;

    if ( FileOffset + FileNumBytes > FileSize ) FileNumBytes = ( U4 )( FileSize - FileOffset );  //  TODO


    ENTRY_ Entry = Entries[Id];
    NTSTATUS Status = DataReadFromFile( Entry, FileOffset, FileNumBytes, Buffer );


    if ( Status == STATUS_PENDING )
    {

    }
    else
    {
        if ( SynchronousIo && ! PagingIo && NT_SUCCESS( Status ) )
        {
LogString( " alter CurrentByteOffset? YES \n" );
            IrpSp->FileObject->CurrentByteOffset.QuadPart = FileOffset + FileNumBytes;
            //Too many CurrentByteOffsets? Ccb and FileObject and DeviceObject
            Ccb->CurrentByteOffset = ( int )( FileOffset + FileNumBytes );
        }
        else
        {
LogString( " alter CurrentByteOffset? NO \n" );
        }

        if ( NT_SUCCESS( Status ) ) Irp->IoStatus.Information = FileNumBytes;

    }

//  TODO should we be using fcb Filesize instead of Entry filesize?

//AlwaysLogFormatted( "READING: returning %d bytes   file offset now %d \n",
//( int ) Irp->IoStatus.Information, ( int ) Offset );

LogFormatted( "IoStatus.Information %d   FileObject->CurrentByteOffset %d   Ccb->CurrentByteOffset %d \n",
( int ) Irp->IoStatus.Information, ( int ) IrpSp->FileObject->CurrentByteOffset.QuadPart, ( int ) Ccb->CurrentByteOffset );


    return Status;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS IrpMjWrite_File( ICB_ Icb )

{
    PIRP         Irp           = Icb->Irp;
    IRPSP_       IrpSp         = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT FileObject    = IrpSp->FileObject;
    FCB_         Fcb           = FileObject->FsContext;
    CCB_         Ccb           = FileObject->FsContext2;
    ID           Id            = Fcb->Id;
    B1           PagingIo      = BitIsSet( Irp->Flags, IRP_PAGING_IO );
//  B1           Nocache       = BitIsSet( Irp->Flags, IRP_NOCACHE );
    B1           SynchronousIo = BitIsSet( FileObject->Flags, FO_SYNCHRONOUS_IO );
//int NumBytesWritten = 0;

ASSERT( ! PagingIo );  //  "call from the Memory Manager (satisfaction of a page fault)  https://community.osr.com/t/irp-flag-nocache/1804

//LogFormatted( "%s %s %s\n",  SynchronousIo?"MustSync":"MaySync",   PagingIo?"PagingIo":"NormalIo",   Nocache?"NoCache":"MayCache" );


//  https://github.com/microsoft/Windows-driver-samples/blob/master/filesys/cdfs/write.c
//  https://docs.microsoft.com/en-us/windows-hardware/drivers/ifs/irp-mj-write

//S1_ IntermediateBuffer = Irp->AssociatedIrp.SystemBuffer;
//B1 DoBufferedIo = BitIsSet( DeviceObject->Flags, DO_BUFFERED_IO );


//LogFormatted( "Minor function code is $%X\n", Icb->MinorFunction );
//    if ( BitIsSet( Icb->MinorFunction, IRP_MN_COMPLETE ) )
//    {
//BreakBecauseOfAProblem();
//    }
ASSERT( IrpSp->MinorFunction == IRP_MN_NORMAL );


ASSERT( BitIsClear( Irp->Flags, IRP_PAGING_IO ) );
//ASSERT( BitIsClear( Irp->Flags, IRP_NOCACHE ) );

    Irp->IoStatus.Information = 0;

//B1 Paging = BitIsSet( Irp->Flags, IRP_PAGING_IO );
//B1 Synchronous = BitIsSet( IrpSp->FileObject->Flags, FO_SYNCHRONOUS_IO );
//LogFormatted( "Synchronous = %d\n", ( int ) Synchronous );

//B1 ForceWrite = BitIsSet( IrpSp->Flags, SL_FORCE_DIRECT_WRITE );
//LogFormatted( "ForceWrite = %d\n", ( int ) ForceWrite );

UCHAR MinorFunction = IrpSp->MinorFunction;
if ( MinorFunction != 0 )
{
BreakToDebugger();
}

//U4 Key = IrpSp->Parameters.Write.Key;
//LogFormatted( "Key = %d\n", ( int ) Key );

//U1_ SystemSuppliedBuffer = Irp->AssociatedIrp.SystemBuffer;
//LogFormatted( "SystemSuppliedBuffer = %p\n", SystemSuppliedBuffer );

    U8 FileOffset   = IrpSp->Parameters.Write.ByteOffset.QuadPart;
    U4 FileNumBytes = IrpSp->Parameters.Write.Length;

    if ( IdIsADirectory( Id ) ) return STATUS_INVALID_PARAMETER;

    U1_ Buffer = IrpBuffer( Irp );
    if ( ! Buffer ) return STATUS_INVALID_USER_BUFFER;  //  TODO or STATUS_INVALID_PARAMETER

    //  Special offset means write to end of file.
    U8 FileSizeBeforeWrite = DataGetFileNumBytes( Entries[Id] );
    if ( FileOffset == 0xFFFFFFFFFFFFFFFFLL ) FileOffset = FileSizeBeforeWrite;

    if ( FileNumBytes == 0 && FileOffset <= FileSizeBeforeWrite ) return STATUS_SUCCESS;

    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    do
    {

        //  Reallocate the file, if necessary.
        U8 OffsetAfterWrite = FileOffset + FileNumBytes;
        if ( OffsetAfterWrite > DataGetAllocationNumBytes( Entries[Id] ) )
        {
            U8 NewMinimumAllocationSize = OffsetAfterWrite;
            //  Hack to not do resize as often, at the expense of over-allocating.  TODO
            if ( FileSizeBeforeWrite > 1024 * 1024 )
                {
                U8 BiggerBy25Percent = DataGetAllocationNumBytes( Entries[Id] ) * 5 / 4;
                if ( BiggerBy25Percent > NewMinimumAllocationSize )
                {
LogFormatted( "Deciding to overallocate to %d instead of %d\n", ( int ) BiggerBy25Percent, ( int ) NewMinimumAllocationSize );
                    NewMinimumAllocationSize = BiggerBy25Percent;
                }
            }

            Status = DataReallocateFile( Id, NewMinimumAllocationSize );
            if ( Status ) break;
        }


        //  Fill to offset with zeros, if necessary.
        if ( FileOffset > FileSizeBeforeWrite )
        {
            Status = DataWriteToFile( Entries[Id], FileSizeBeforeWrite, ( int )( FileOffset-FileSizeBeforeWrite ), 0 );
            //  TODO what about status here?
        }

        Status = DataWriteToFile( Entries[Id], FileOffset, FileNumBytes, Buffer );
        if ( Status ) break;

        if ( OffsetAfterWrite > DataGetFileNumBytes( Entries[Id] ) )
        {
            NTSTATUS sss = DataResizeFile( Id, OffsetAfterWrite, DONT_FILL );
ASSERT( ! sss );
        }

    } while ( 0 );

    if ( Status == STATUS_PENDING )
    {

    }
    else
    {
        if ( ! PagingIo )
        {
            SetBit( FileObject->Flags, FO_FILE_MODIFIED );
        }


        if ( SynchronousIo && ! PagingIo && NT_SUCCESS( Status ) )
        {
LogString( " alter CurrentByteOffset? YES \n" );
ASSERT( IrpSp->FileObject == FileObject );  //  TODO
            FileObject->CurrentByteOffset.QuadPart = FileOffset + FileNumBytes;
            Ccb->CurrentByteOffset = ( int )( FileOffset + FileNumBytes );
        }
        else
        {
LogString( " alter CurrentByteOffset? NO \n" );
        }

        SetBit( FileObject->Flags, FO_FILE_MODIFIED );  //  TODO do this? Do this here?

        if ( NT_SUCCESS( Status ) )
            Irp->IoStatus.Information = FileNumBytes;  //  Number of bytes written.

    }

    return Status;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS IrpMjRead_Volume( ICB_ Icb )

{
    UNREFERENCED_PARAMETER( Icb );


    PIRP            Irp          = Icb->Irp;
    IRPSP_          IrpSp        = IoGetCurrentIrpStackLocation( Irp );
    U8              Offset       = IrpSp->Parameters.Read.ByteOffset.QuadPart;
    U4              Length       = IrpSp->Parameters.Read.Length;
    PDEVICE_OBJECT  DeviceObject = Icb->DeviceObject;
    VCB_            Vcb          = DeviceObject->DeviceExtension;

    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    Irp->IoStatus.Information = 0;

    do
    {

        if ( Length == 0 )
        {
            Status = STATUS_SUCCESS;
            break;
        }

//  https://community.osr.com/discussion/67486/irp-nocache-vs-irp-paging-io

        //B1 PagingIo      = BitIsSet( Irp->Flags, IRP_PAGING_IO );
        B1 Nocache       = BitIsSet( Irp->Flags, IRP_NOCACHE );
        //B1 SynchronousIo = BitIsSet( FileObject->Flags, FO_SYNCHRONOUS_IO );
        if ( ! Nocache || Offset & ( SECTOR_SIZE - 1 ) || Length & ( SECTOR_SIZE - 1 ) )
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        U1_ Buffer = IrpBuffer( Irp );
        if ( ! Buffer )
        {
            Status = STATUS_INVALID_USER_BUFFER;
            break;
        }

        Status = ReadBlockDevice( Vcb->PhysicalDeviceObject, Offset, Length, Buffer, MAY_VERIFY );

        if ( Status == STATUS_VERIFY_REQUIRED )
        {
BreakToDebugger();
            Status = IoVerifyVolume( Vcb->PhysicalDeviceObject, FALSE );

            if ( NT_SUCCESS( Status ) )
            {
                Status = ReadBlockDevice( Vcb->PhysicalDeviceObject, Offset, Length, Buffer, MAY_VERIFY );
            }
        }

    } while ( 0 );


    Irp->IoStatus.Information = Length;

    return Status;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS IrpMjWrite_Volume( ICB_ Icb )

{
    UNREFERENCED_PARAMETER( Icb );
/*
    PIRP          Irp         = Icb->Irp;
    IRPSP_        IrpSp       = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT  FileObject  = IrpSp->FileObject;
    FCB_          Fcb         = FileObject->FsContext;
*/

AlwaysBreakToDebugger();

    return 0;  //  TODO
}

//////////////////////////////////////////////////////////////////////

NTSTATUS IrpMjRead( ICB_ Icb )

{
    PIRP          Irp        = Icb->Irp;
    IRPSP_        IrpSp      = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT  FileObject = IrpSp->FileObject;
    FCB_          Fcb        = FileObject->FsContext;

ASSERT( IrpSp->MinorFunction == IRP_MN_NORMAL );

    if ( Fcb->IsAVolume ) return IrpMjRead_Volume( Icb );
    else                  return IrpMjRead_File(   Icb );
}

//////////////////////////////////////////////////////////////////////

NTSTATUS IrpMjWrite( ICB_ Icb )

{
    PIRP          Irp        = Icb->Irp;
    IRPSP_        IrpSp      = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT  FileObject = IrpSp->FileObject;
    FCB_          Fcb        = FileObject->FsContext;

ASSERT( IrpSp->MinorFunction == IRP_MN_NORMAL );

    if ( Fcb->IsAVolume ) return IrpMjWrite_Volume( Icb );
    else                  return IrpMjWrite_File(   Icb );
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


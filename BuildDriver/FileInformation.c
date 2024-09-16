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

#pragma warning( disable:4701 )  //  potentially uninitialized local variable
#pragma warning( disable:4703 )  //  potentially uninitialized local pointer variable

//////////////////////////////////////////////////////////////////////

NTSTATUS IrpMjQueryInformation( ICB_ Icb )

{

    PIRP                   Irp            = Icb->Irp;
    IRPSP_                 IrpSp          = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT           FileObject     = IrpSp->FileObject;
    FCB_                   Fcb            = FileObject->FsContext;
    V_                     Buffer         = Irp->AssociatedIrp.SystemBuffer;
    ULONG                  BufferLength   = IrpSp->Parameters.QueryFile.Length;
    int                    BufferNumBytes = ( int ) BufferLength;
    FILE_INFORMATION_CLASS Class          = IrpSp->Parameters.QueryFile.FileInformationClass;
    ID                     Id             = Fcb->Id;
    NTSTATUS               Status         = STATUS__PRIVATE__NEVER_SET;

    ULONG_PTR* NumBytesReturning_;
    NumBytesReturning_ = &Irp->IoStatus.Information;

    ENTRY_ Entry = Entries[Id];

    switch ( Class )
    {

//--------------------------------------------------------------------

      case FileBasicInformation:
        {
LogString( "Query FileBasicInformation\n" );
            PFILE_BASIC_INFORMATION Info = Buffer;

            if ( BufferNumBytes < sizeof( *Info ) ) return STATUS_INFO_LENGTH_MISMATCH;

            Info->ChangeTime     = GET_ChangeTime;
            Info->CreationTime   = GET_CreationTime;
            Info->LastAccessTime = GET_LastAccessTime;
            Info->LastWriteTime  = GET_LastWriteTime;
            Info->FileAttributes = OrNormal( Entry->FileAttributes );

            *NumBytesReturning_ = sizeof( *Info );

LogFormatted( "Get FileBasicInformation FileAttributes returning $%X\n", Info->FileAttributes );

            return STATUS_SUCCESS;
        }

//--------------------------------------------------------------------

      case FileStandardInformation:
        {
LogString( "Query FileStandardInformation\n" );
            PFILE_STANDARD_INFORMATION Info = Buffer;

            if ( BufferNumBytes < sizeof( *Info ) ) return STATUS_INFO_LENGTH_MISMATCH;

            Info->AllocationSize.QuadPart = DataGetAllocationNumBytes( Entry );
            Info->EndOfFile.QuadPart      = DataGetFileNumBytes( Entry );
            Info->NumberOfLinks           = 1;
            Info->DeletePending           = Fcb->DeleteIsPending;
LogFormatted( "Returning Info->DeletePending of %d\n", Info->DeletePending );
            Info->Directory               = EntryIsADirectory( Entry );

            *NumBytesReturning_ = sizeof( *Info );
            return STATUS_SUCCESS;
        }

//--------------------------------------------------------------------

      case FileInternalInformation:
        {
LogString( " We do not implement Query FileInternalInformation.\n" );
            return STATUS_INVALID_PARAMETER;//STATUS_NOT_IMPLEMENTED;
/*
LogString( "FileInternalInformation\n" );
            PFILE_INTERNAL_INFORMATION Info = Buffer;

            if ( BufferNumBytes < sizeof( *Info ) ) return STATUS_INFO_LENGTH_MISMATCH;

            //  https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/ns-ntifs-_file_internal_information
            Info->IndexNumber.QuadPart = 0;  //  TODO set this to our internal id?
//  "The IndexNumber member ... is the same as the FileId member ..."
//  see https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/ns-ntifs-_file_internal_information

            NumBytesReturning = sizeof( *Info );
            Status = STATUS_SUCCESS;
*/
        }

//--------------------------------------------------------------------

      case FileEaInformation:
        {
LogString( " We do not implement Query FileEaInformation.\n" );
            return STATUS_INVALID_PARAMETER;//STATUS_NOT_IMPLEMENTED;
        }

//--------------------------------------------------------------------

      case FileAccessInformation:
        {
LogString( " We do not implement FileAccessInformation.\n" );
            return STATUS_INVALID_PARAMETER;//STATUS_NOT_IMPLEMENTED;
        }

//--------------------------------------------------------------------

      case FilePositionInformation:
        {
LogString( "Query FilePositionInformation\n" );
            PFILE_POSITION_INFORMATION Info = Buffer;

            if ( BufferNumBytes < sizeof( *Info ) ) return STATUS_INFO_LENGTH_MISMATCH;

            Info->CurrentByteOffset = FileObject->CurrentByteOffset;

            *NumBytesReturning_ = sizeof( *Info );
            return STATUS_SUCCESS;
        }

//--------------------------------------------------------------------

      case FileModeInformation:
        {
LogString( " We do not implement Query FileModeInformation.\n" );
            return STATUS_INVALID_PARAMETER;//STATUS_NOT_IMPLEMENTED;
        }

//--------------------------------------------------------------------

      case FileAlignmentInformation:
        {
LogString( " We do not implement Query FileAlignmentInformation.\n" );
            return STATUS_INVALID_PARAMETER;//STATUS_NOT_IMPLEMENTED;
        }

//--------------------------------------------------------------------

//  The following is an example of a normalized file name for a local file:	
//  \Device\HarddiskVolume1\Documents and Settings\MyUser\My Documents\Test Results.txt
//  https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/fltkernel/ns-fltkernel-_flt_file_name_information
//  Prior to Windows 8, Filter Manager obtained the normalized name for a file or directory
//  by collecting the name information for each component of the file path.
//  https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/fltkernel/nf-fltkernel-fltgetfilenameinformation#see-also
//  "A normalized name is an absolute pathname where each short name component has been replaced with the corresponding long name component, and each name component uses the exact letter casing stored on disk."
//  https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-fscc/20bcadba-808c-4880-b757-4af93e41edf6
//  see FatQueryNameInfo( Icb, Fcb, Ccb, TRUE, Buffer, &Length );

//  This information class returns a FILE_NAME_INFORMATION data element containing an absolute pathname ( section 2.1.5 ).
//  https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-fscc/cb30e415-54c5-4483-a346-822ea90e1e89


      case FileNormalizedNameInformation:  //  48 == $30
      case FileNameInformation:  //  9 == $9
        {

if ( Class == FileNameInformation )
LogString( "Query FileNameInformation\n" );
else
LogString( "Query FileNormalizedNameInformation\n" );

            PFILE_NAME_INFORMATION Info = Buffer;

            int StructNumBytes = offsetof( FILE_NAME_INFORMATION, FileName );
            if ( BufferNumBytes < StructNumBytes ) return STATUS_INFO_LENGTH_MISMATCH;
            int NumWideCharsThatWillFit = ( BufferNumBytes - StructNumBytes ) / 2;


            //  Create a wide version of the path and name; must begin with a backslash.
            //  TODO speedup
//BreakToDebugger();
            WCHAR NewWayWidePathAndName[1024];
            int NumWideChars = WideFullPathAndName( Entry, NewWayWidePathAndName );
            NewWayWidePathAndName[NumWideChars] = 0;  //  TODO temp for printing purposes?
//LogFormatted( "NewWayWidePathAndName = \"%S\"  NumWideChars = %d\n", NewWayWidePathAndName, NumWideChars );
            //  ...the string will begin with a single backslash, regardless of its location.
            //  Thus the file C:\dir1\dir2\filename.ext will appear as \dir1\dir2\filename.ext, ...
ASSERT( NewWayWidePathAndName[0] == L'\\' );
ASSERT( NewWayWidePathAndName[NumWideChars] == 0 );

            //  Report the actual path and name length.
            Info->FileNameLength = NumWideChars*2;

            //  Make adjustments based on whether the whole name will fit in the buffer provided.
            if ( NumWideChars > NumWideCharsThatWillFit )
            {
                //or
                ////  https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-fscc/95f3056a-ebc1-4f5d-b938-3f68a44677a6
                // NO!!! looks like it was hurting pff AND pvlc!!! Status = STATUS_INFO_LENGTH_MISMATCH;  //  0xC0000004
                NumWideChars = NumWideCharsThatWillFit;
                Status = STATUS_BUFFER_OVERFLOW;  //  Looks like fastfat returns this.
            }
            else
            {
                Status = STATUS_SUCCESS;
            }


            //  Copy the correct number of bytes.
            memcpy( Info->FileName, NewWayWidePathAndName, NumWideChars*2LL );


            *NumBytesReturning_ = StructNumBytes + ( U8 ) NumWideChars*2;
            return Status;
        }

//--------------------------------------------------------------------

      case FileAllInformation:
        {
LogString( "Query FileAllInformation\n" );
//BreakToDebugger();
            PFILE_ALL_INFORMATION Info = Buffer;

            int StructNumBytes = offsetof( FILE_ALL_INFORMATION, NameInformation.FileName );
            if ( BufferNumBytes < StructNumBytes ) return STATUS_INFO_LENGTH_MISMATCH;
            int NumWideCharsThatWillFit = ( BufferNumBytes - StructNumBytes ) / 2;

/*
typedef struct _FILE_ALL_INFORMATION {
FILE_BASIC_INFORMATION BasicInformation;
FILE_STANDARD_INFORMATION StandardInformation;
FILE_INTERNAL_INFORMATION InternalInformation;
FILE_EA_INFORMATION EaInformation;
FILE_ACCESS_INFORMATION AccessInformation;
FILE_POSITION_INFORMATION PositionInformation;
FILE_MODE_INFORMATION ModeInformation;
FILE_ALIGNMENT_INFORMATION AlignmentInformation;
FILE_NAME_INFORMATION NameInformation;
} FILE_ALL_INFORMATION, *PFILE_ALL_INFORMATION;
*/

            Info->BasicInformation.CreationTime   = GET_CreationTime;
            Info->BasicInformation.LastAccessTime = GET_LastAccessTime;
            Info->BasicInformation.LastWriteTime  = GET_LastWriteTime;
            Info->BasicInformation.ChangeTime     = GET_ChangeTime;
            Info->BasicInformation.FileAttributes = OrNormal( Entry->FileAttributes );
LogMarker();
            Info->StandardInformation.AllocationSize.QuadPart = DataGetAllocationNumBytes( Entry );
            Info->StandardInformation.EndOfFile.QuadPart      = DataGetFileNumBytes( Entry );
            Info->StandardInformation.NumberOfLinks           = 1;
            Info->StandardInformation.DeletePending           = Fcb->DeleteIsPending;
LogFormatted( "Returning Info->StandardInformation.DeletePending of %d\n", Info->StandardInformation.DeletePending );
            Info->StandardInformation.Directory               = EntryIsADirectory( Entry );

            //  https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/ns-ntifs-_file_internal_information
            Info->InternalInformation.IndexNumber.QuadPart = 0;  //  TODO set this to our internal id?
//  "The IndexNumber member ... is the same as the FileId member ..."
//  see https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/ns-ntifs-_file_internal_information

            Info->EaInformation.EaSize = 0;

LogString( "BOGUS Info->AccessInformation.AccessFlags = 0;\n" );
            Info->AccessInformation.AccessFlags = 0;  //  ACCESS_MASK AccessFlags; ??????????????

LogFormatted( "DUNNO Info->PositionInformation.CurrentByteOffset = FileObject->CurrentByteOffset; ( %d )\n", ( int ) FileObject->CurrentByteOffset.QuadPart );
            Info->PositionInformation.CurrentByteOffset = FileObject->CurrentByteOffset;

LogString( "BOGUS Info->ModeInformation.Mode = 0;\n" );
            Info->ModeInformation.Mode = 0;  //  Mode; ??????????????

LogString( "BOGUS Info->AlignmentInformation.AlignmentRequirement = 0;\n" );
            Info->AlignmentInformation.AlignmentRequirement = 0;  //  ULONG AlignmentRequirement; ?????????

LogMarker();

            //  Create a wide version of the path and name; must begin with a backslash.
            //  TODO speedup
//BreakToDebugger();
            WCHAR NewWayWidePathAndName[1024];
            int NumWideChars = WideFullPathAndName( Entry, NewWayWidePathAndName );
            NewWayWidePathAndName[NumWideChars] = 0;  //  TODO temp for printing purposes?
LogFormatted( "NewWayWidePathAndName = \"%S\"  NumWideChars = %d\n", NewWayWidePathAndName, NumWideChars );
            //  ...the string will begin with a single backslash, regardless of its location.
            //  Thus the file C:\dir1\dir2\filename.ext will appear as \dir1\dir2\filename.ext, ...
ASSERT( NewWayWidePathAndName[0] == L'\\' );
ASSERT( NewWayWidePathAndName[NumWideChars] == 0 );

            //  Report the actual path and name length.
            Info->NameInformation.FileNameLength = NumWideChars*2;

            //  Make adjustments based on whether the whole name will fit in the buffer provided.
            if ( NumWideChars > NumWideCharsThatWillFit )
            {
                Status = STATUS_BUFFER_OVERFLOW;  //  Looks like fastfat returns this.
                //or
                ////  https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-fscc/95f3056a-ebc1-4f5d-b938-3f68a44677a6
                // NO! ! ! lookes like it was hurts pff AND pvlc ! ! ! Status = STATUS_INFO_LENGTH_MISMATCH;  //  0xC0000004
                NumWideChars = NumWideCharsThatWillFit;
            }
            else
            {
                Status = STATUS_SUCCESS;
            }
LogMarker();

            //  Copy the correct number of bytes.
            memcpy( Info->NameInformation.FileName, NewWayWidePathAndName, NumWideChars*2LL );

LogMarker();
            *NumBytesReturning_ = ( U8 ) StructNumBytes + NumWideChars*2LL;
            return Status;
        }

//--------------------------------------------------------------------

      case FileNetworkOpenInformation:
        {
LogString( "Query FileNetworkOpenInformation\n" );
            PFILE_NETWORK_OPEN_INFORMATION Info = Buffer;

            if ( BufferNumBytes < sizeof( *Info ) ) return STATUS_INFO_LENGTH_MISMATCH;

            Info->CreationTime            = GET_CreationTime;
            Info->LastAccessTime          = GET_LastAccessTime;
            Info->LastWriteTime           = GET_LastWriteTime;
            Info->ChangeTime              = GET_ChangeTime;
            Info->AllocationSize.QuadPart = DataGetAllocationNumBytes( Entry );
            Info->EndOfFile.QuadPart      = DataGetFileNumBytes( Entry );
            Info->FileAttributes          = OrNormal( Entry->FileAttributes );
//LogFormatted( " FileNetworkOpenInformation returning times      of $%X\n", ( int ) Info->CreationTime.QuadPart );
//LogFormatted( " FileNetworkOpenInformation returning alloc size of $%X\n", ( int ) Info->AllocationSize.QuadPart );
//LogFormatted( " FileNetworkOpenInformation returning file  size of $%X\n", ( int ) Info->EndOfFile.QuadPart );
//LogFormatted( " FileNetworkOpenInformation returning attributes of $%X\n", ( int ) Info->FileAttributes );
            *NumBytesReturning_ = sizeof( *Info );
            return STATUS_SUCCESS;
        }

//--------------------------------------------------------------------

      case FileAttributeTagInformation:
        {
LogString( "Query FileAttributeTagInformation\n" );
            PFILE_ATTRIBUTE_TAG_INFORMATION Info = Buffer;

            if ( BufferNumBytes < sizeof( *Info ) ) return STATUS_INFO_LENGTH_MISMATCH;

            Info->FileAttributes = OrNormal( Entry->FileAttributes );
            Info->ReparseTag     = 0;

            *NumBytesReturning_ = sizeof( *Info );
            return STATUS_SUCCESS;
        }

//--------------------------------------------------------------------

      case FileRemoteProtocolInformation:  //  55
        {
LogString( " We do not implement Query FileRemoteProtocolInformation.\n" );
////////////return STATUS_INVALID_PARAMETER;//STATUS_NOT_IMPLEMENTED;
            return STATUS_NOT_IMPLEMENTED;
//  btrfs returns STATUS_INVALID_PARAMETER;
//  dokan returns STATUS_NOT_IMPLEMENTED
        }

//--------------------------------------------------------------------

      case FileIdInformation:  //  59
        {
LogString( " We do not implement Query FileIdInformation.\n" );
            return STATUS_INVALID_PARAMETER;//STATUS_NOT_IMPLEMENTED;
        }

//--------------------------------------------------------------------

      default:
        {
//LogFormatted( " Invalid Query info class of %d\n", Class );
//                Status = STATUS_INVALID_INFO_CLASS;
LogFormatted( "Returning STATUS_INVALID_PARAMETER for Query info class of %d\n", Class );
            return STATUS_INVALID_PARAMETER;  //  TODO looks like fastfat returns this
        }

//--------------------------------------------------------------------

    }


}

//////////////////////////////////////////////////////////////////////

NTSTATUS IrpMjSetInformation ( ICB_ Icb )

{

    NTSTATUS      Status       = STATUS__PRIVATE__NEVER_SET;
    PIRP          Irp          = Icb->Irp;
    IRPSP_        IrpSp        = IoGetCurrentIrpStackLocation( Irp );
    V_            SystemBuffer = Irp->AssociatedIrp.SystemBuffer;
    PFILE_OBJECT  FileObject   = IrpSp->FileObject;
    FCB_          Fcb          = FileObject->FsContext;
    CCB_          Ccb          = FileObject->FsContext2;
    ULONG         BufferLength = IrpSp->Parameters.SetFile.Length;
    ID            Id           = Fcb->Id;
    ENTRY_        Entry        = Entries[Id];

    PDEVICE_OBJECT DeviceObject = Icb->DeviceObject;
    if ( DeviceObject == Volume_TailwindDeviceObject ) return STATUS_INVALID_DEVICE_REQUEST;

    VCB_ Vcb = DeviceObject->DeviceExtension;

LogFormatted( "IrpMjSetInformation on %s\n", Entry->Name );


    FILE_INFORMATION_CLASS Class = IrpSp->Parameters.SetFile.FileInformationClass;
    switch ( Class )
    {

//--------------------------------------------------------------------

    //  See "What's in a Name? - Cracking Rename Operations"
    //  for some of the complexities.
    //  https://www.osronline.com/custom.cfm%5Ename=articlePrint.cfm&id=85.htm
      case FileRenameInformation:  //  10
        {
LogString( "Set FileRenameInformation\n" );
            PFILE_RENAME_INFORMATION Info = SystemBuffer;
            if ( BufferLength < sizeof( *Info ) ) return STATUS_INFO_LENGTH_MISMATCH;

            ////  TODO do

            //#if ( _WIN32_WINNT >= _WIN32_WINNT_WIN10_RS1 )
            //union {
            //    B1 ReplaceIfExists;  // FileRenameInformation
            //    ULONG Flags;              // FileRenameInformationEx
            //} DUMMYUNIONNAME;
            //#else
            //B1 ReplaceIfExists;
            //#endif
            //HANDLE RootDirectory;
            //ULONG FileNameLength;
            //WCHAR FileName[1];
            ////  https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/ns-ntifs-_file_rename_information


PFILE_OBJECT AnotherFileObject = IrpSp->Parameters.SetFile.FileObject;
if ( FileObject != AnotherFileObject )
{
//BreakToDebugger();
LogString( "Don't even know what I am looking at with FileObject != AnotherFileObject.\n" );
}

ASSERT( ! Info->RootDirectory );

            if ( ! Entry->ParentId ) return STATUS_INVALID_PARAMETER;///////Cant rename the root.


            //  Convert the ( path and ) name to utf8.
            S1 FullDestinationPathAndName[2048];  //  TODO
            WidePathAndNameCharactersToUtf8String( Info->FileNameLength, Info->FileName, FullDestinationPathAndName );
            S1_ d = FullDestinationPathAndName;
            //  Launching PortableFF returned "\??\E:\FirefoxPortable\Data\profile\prefs.js".
            if ( d[0] == '\\' && d[1] == '?' && d[2] == '?' && d[3] == '\\' && d[5] == ':' && d[6] == '\\' ) d += 6;
            S1_ DestinationPathAndName = d;
LogFormatted( "DestinationPathAndName is \"%s\"\n", DestinationPathAndName );


            //  Locate the destination path and name entries.
            ID  DestinationParentId;
            ID  DestinationId;
            S1_ DestinationName = ( S1_ )-1;//TODO -1 is temp test
            Status = FindByPathAndName( Vcb->RootId, DestinationPathAndName, &DestinationName, &DestinationParentId, &DestinationId );
            if ( ! DestinationParentId )
            {///////  Don't have a home! Are we supposed to create the destination dir( s )?
BreakToDebugger();
                return STATUS_INVALID_PARAMETER;  //  TODO need real value here===================================
            }
            ENTRY_ DestinationParentEntry = Entries[DestinationParentId];
            ENTRY_ DestinationEntry       = Entries[DestinationId];


            //  Handle the case where the destination already exists.
            if ( DestinationEntry )
            {
                if ( ! Info->ReplaceIfExists ) return STATUS_OBJECT_NAME_COLLISION;


                //  fastfat says "We cannot overwrite a directory."
                if ( EntryIsADirectory( DestinationEntry ) ) return STATUS_OBJECT_NAME_COLLISION;


////////////////VCB_ Vcb = DeviceObject->DeviceExtension;  //  TODO ??
                FCB_ DestinationFcb = LookupFcbById( Vcb, DestinationEntry->Id );
                if ( DestinationFcb )// && Fcb->ReferenceCount > 0 )
                {
                    return STATUS_ACCESS_DENIED;
                }


LogString( "DELETE - unmake entry.\n" );
                UnmakeEntry( DestinationId );
//BreakToDebugger();
//LogString( "What about fcb and ccb?\n" );

            }


            DetachEntry( Entry );

            ID MyId = Entry->Id;
            Status = ResizeEntry( MyId, DestinationName, 0 );
            if( Status ) return Status;  //  and reattach?  TODO
            Entry = Entries[MyId];

            AttachEntry( DestinationParentEntry, Entry );


            return Status;
        }


//--------------------------------------------------------------------

      case FileBasicInformation:  //  4
        {
LogFormatted( "Set FileBasicInformation                        BufferLength is %d\n", ( int ) BufferLength );
            PFILE_BASIC_INFORMATION Info = SystemBuffer;
            if ( BufferLength < sizeof( *Info ) ) return STATUS_INFO_LENGTH_MISMATCH;

            //Entry->CreationTime        = Info->CreationTime.QuadPart;
            //Entry->LastAccessTime      = Info->LastAccessTime.QuadPart;
            //Entry->LastWriteTime       = Info->LastWriteTime.QuadPart;
            //Entry->ChangeTime          = Info->ChangeTime.QuadPart;


            ULONG FileAttributes = Info->FileAttributes;


            //  A la fastfat, "silently drop" attributes we don't understand.
            //////////FileAttributes &= FILE_ATTRIBUTE_READONLY  | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM
            //////////                | FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_ARCHIVE;
            FileAttributes &= FILE_ATTRIBUTE_DIRECTORY;  //  TODO what about | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY ?


            //  If the FileAttributes field is zero it means the attributes are not being
            //  set. If any of the time fields are -1 it means that time field is not being
            //  set.
            //  https://community.osr.com/discussion/56142/file-basic-information
            if ( Info->FileAttributes )
            {
                if ( EntryIsAFile( Entry ) )
                {
                    if ( BitIsSet( FileAttributes, FILE_ATTRIBUTE_DIRECTORY ) ) return STATUS_INVALID_PARAMETER;
                }
                else
                {
                    FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
                }

LogFormatted( "Set FileBasicInformation FileAttributes from $%X to $%X\n", Entry->FileAttributes, FileAttributes );

                Entry->FileAttributes = FileAttributes;
            }

            return STATUS_SUCCESS;
        }

//--------------------------------------------------------------------

      case FilePositionInformation:  //  14
        {
LogString( "Set FilePositionInformation\n" );
            PFILE_POSITION_INFORMATION Info = SystemBuffer;
            if ( BufferLength < sizeof( *Info ) ) return STATUS_INFO_LENGTH_MISMATCH;

            Info->CurrentByteOffset.QuadPart = Ccb->CurrentByteOffset;
LogFormatted( "reporting CurrentByteOffset of %d\n", ( int ) Ccb->CurrentByteOffset );
            return STATUS_SUCCESS;
        }

//--------------------------------------------------------------------
/*
        case FileAllocationInformation:  //  19
        {
BreakToDebugger();  //  eb eip 0x90
LogString( "Set FileAllocationInformation\n" );

            PFILE_ALLOCATION_INFORMATION Info = SystemBuffer;
            if ( BufferLength < sizeof( *Info ) ) return STATUS_INFO_LENGTH_MISMATCH;
            if ( EntryIsADirectory( Entry ) ) { Status = STATUS_INVALID_DEVICE_REQUEST; break; }

            U8 RequestedAllocationSize = Info->AllocationSize.QuadPart;
            Status = DataReallllllocateFile( Entry, RequestedAllocationSize );

            //Status = STATUS_SUCCESS;

        }
        break;
*/
//--------------------------------------------------------------------

      case FileEndOfFileInformation:  //  20
        {
LogString( "Set FileEndOfFileInformation\n" );

            PFILE_END_OF_FILE_INFORMATION Info = SystemBuffer;
            if ( BufferLength < sizeof( *Info ) ) return STATUS_INFO_LENGTH_MISMATCH;

            if ( EntryIsADirectory( Entry ) ) return STATUS_INVALID_DEVICE_REQUEST;  //  see fastfat fileinfo.c#4423
            /* if arent allowed, Status = STATUS_ACCESS_DENIED; break; } TODO */


            U8 RequestedEndOfFile = Info->EndOfFile.QuadPart;
            BOOLEAN AdvanceOnly = IrpSp->Parameters.SetFile.AdvanceOnly;

LogFormatted( "Requesting a file size of %d. ( AdvanceOnly=%d. )\n", ( int ) RequestedEndOfFile, AdvanceOnly );
//BreakToDebugger();  //  reallocate the file here  //  eb eip 0x90

            if ( RequestedEndOfFile == DataGetFileNumBytes( Entry ) ) return STATUS_SUCCESS;

            if ( AdvanceOnly && ( RequestedEndOfFile <= DataGetFileNumBytes( Entry ) ) )
            {
LogString( "AdvanceOnly and we aren't advancing so we ignore.\n" );
//BreakToDebugger();
                return STATUS_SUCCESS;
            }


            if ( RequestedEndOfFile > DataGetFileNumBytes( Entry ) )
            {
                Status = DataResizeFile( Id, RequestedEndOfFile, ZERO_FILL );
                if ( Status == STATUS_INSUFFICIENT_RESOURCES ) Status = STATUS_DISK_FULL;
            }
            else
            {
                Status = STATUS_SUCCESS;
            }


            //if ( ! Status )
            //{
            //    //  see fastfat fileinfo.c 4645; not that header also has	
            //    //    LARGE_INTEGER AllocationSize;
            //    //    LARGE_INTEGER FileSize;
            //    //    LARGE_INTEGER ValidDataLength;
            //    if ( Fcb->CommonFCBHeader.ValidDataLength.LowPart > RequestedEndOfFile )
            //    {
            //        Fcb->CommonFCBHeader.ValidDataLength.LowPart = ( int ) RequestedEndOfFile;
            //    }
            //}

            if ( Status == STATUS_SUCCESS )
            {
                NTSTATUS sss = DataResizeFile( Id, RequestedEndOfFile, DONT_FILL );
ASSERT( ! sss );
            }
            return Status;
        }

//--------------------------------------------------------------------

//  2 ways to delete https://stackoverflow.com/questions/50870373/what-is-the-irp-message-generated-on-file-delete-in-a-filter-driver

      case FileDispositionInformation:  //  13
        {
LogString( "Set FileDispositionInformation\n" );
            PFILE_DISPOSITION_INFORMATION Info = SystemBuffer;
            if ( BufferLength < sizeof( *Info ) ) return STATUS_INFO_LENGTH_MISMATCH;

            if ( ! Info->DeleteFile )
            {
LogString( "Clearing DeleteIsPending and FileObject->DeletePending.\n" );
                Fcb->DeleteIsPending = FALSE;  //DELETE_ON_CLOSE
                FileObject->DeletePending = FALSE;  //  TODO OK? One msdn page said read-only.
                //Status = STATUS_SUCCESS;
                //break;
            }
            else
            {  //  TODO
//LogString( "Can we set DeleteIsPending?  Maybe a silent ignore is required here too?\n" );
LogString( "Can we set DeleteIsPending?\n" );
//BreakToDebugger();
                if (
                    //  read only ||  TODO                         //  Can't delete read-only.
                    ( ! Entry->ParentId ) //                         //  Can't delete root.
                )
                {
                    return STATUS_CANNOT_DELETE;
                }


                //  Can't delete non-empty dirs.
                if ( EntryIsADirectory( Entry ) && ChildrenTree( Entry ) )
                {
                    return STATUS_DIRECTORY_NOT_EMPTY;  //  see fastfat fileinfo.c #2512
                }


LogString( "Setting DeleteIsPending and FileObject->DeletePending.\n" );
                Fcb->DeleteIsPending = TRUE;  //DELETE_ON_CLOSE
                FileObject->DeletePending = TRUE;  //  TODO OK? One msdn page said read-only.
            }

            return STATUS_SUCCESS;
        }

//--------------------------------------------------------------------

//  2 ways to delete https://stackoverflow.com/questions/50870373/what-is-the-irp-message-generated-on-file-delete-in-a-filter-driver

      case FileDispositionInformationEx:
        {

LogString( "Set FileDispositionInformationEx\n" );
LogString( "PRETEND WE CANT TILL WE TEST THIS CODE\n" );
if ( TRUE ) {	
//Status = STATUS_INVALID_DEVICE_REQUEST;
return STATUS_INVALID_PARAMETER;
//Status = STATUS_INVALID_INFO_CLASS;
//Status = STATUS_NOT_IMPLEMENTED;
//Status = STATUS_NOT_SUPPORTED;
}
LogString( "Lets not do it for now as it may be a problem.\n" );
            Status = STATUS_INVALID_PARAMETER;

            PFILE_DISPOSITION_INFORMATION_EX Info = SystemBuffer;

            if ( BufferLength < sizeof( *Info ) ) return STATUS_INFO_LENGTH_MISMATCH;

//  https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/ns-ntddk-_file_disposition_information_ex
//  https://social.msdn.microsoft.com/Forums/en-US/420bd0bd-3448-4393-b8d9-7947d5286901/filedispositioninformationex-and-filedispositiononclose?forum=wdk
            //  If we are asked to set or clear delete-on-close...
            if ( BitIsSet( Info->Flags, FILE_DISPOSITION_ON_CLOSE ) )
            {

                if ( BitIsClear( Info->Flags, FILE_DISPOSITION_DELETE ) )
                {
LogString( "Clearing DeleteIsPending.\n" );
                    Fcb->DeleteIsPending = FALSE;  //DELETE_ON_CLOSE
                    FileObject->DeletePending = FALSE;  //  Fastfat does this near fileinfo.c #2664
                }
                else
                {

//LogString( "Can we set DeleteIsPending?  Maybe a silent ignore is required here too?\n" );
LogString( "Can we set DeleteIsPending?\n" );
//BreakToDebugger();
                    if (
                        //  read only ||  TODO                         //  Can't delete read-only.
                        ( ! Entry->ParentId ) //                         //  Can't delete root.
                    )
                    {
                        return STATUS_CANNOT_DELETE;
                    }


                    //  Can't delete non-empty dirs.
                    if ( EntryIsADirectory( Entry ) && ChildrenTree( Entry ) )
                    {
                        return STATUS_DIRECTORY_NOT_EMPTY;  //  see fastfat fileinfo.c #2512
                    }


LogString( "Setting DeleteIsPending.\n" );
                    Fcb->DeleteIsPending = TRUE;  //DELETE_ON_CLOSE
                    FileObject->DeletePending = TRUE;  //  Fastfat does this near fileinfo.c #2634
                }
            }


            if (
                ( BitIsSet( Info->Flags, FILE_DISPOSITION_POSIX_SEMANTICS           ) ) ||
                ( BitIsSet( Info->Flags, FILE_DISPOSITION_FORCE_IMAGE_SECTION_CHECK ) ) ||
// now handled /////( BitIsSet( Info->Flags, FILE_DISPOSITION_ON_CLOSE                  ) ) ||
                ( BitIsSet( Info->Flags, FILE_DISPOSITION_IGNORE_READONLY_ATTRIBUTE ) )
            )
            {
LogFormatted( "TODO what to do with these FILE_DISPOSITIONs? $%X\n", ( int ) Info->Flags );
//  https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/ns-ntddk-_file_disposition_information_ex
//BreakToDebugger();  //  eb eip 0x90
            }
	
            return STATUS_SUCCESS;
        }

//--------------------------------------------------------------------

        default:
        {
LogFormatted( "Set STATUS_INVALID_PARAMETER returned for %d\n", Class );
            return STATUS_INVALID_PARAMETER;
        }

//--------------------------------------------------------------------

    }

}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


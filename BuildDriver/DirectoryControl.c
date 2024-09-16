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

inline B1 DoesStringContainWildcards( S1_ S )

{
    while ( *S ) { if ( *S == '*' || *S == '?' ) return TRUE; S++; }
    return FALSE;
}

//////////////////////////////////////////////////////////////////////

int fillOneQueryDirectoryInfo( FILE_INFORMATION_CLASS FileInformationClass, U1_ InfoAddress, int InfoMaxLength, ENTRY_ Entry )

{
    S1_ Utf8Name = Entry->Name;

    WCHAR WideName[512];  //  TODO

    int NumWideChars = Utf8StringToWideString( Utf8Name, WideName );
    int WideNameNumBytes = NumWideChars * 2;

//WideName[NumWideChars] = 0;  //  Need to terminate for debug output only.
//LogFormatted( " Utf8:\"%s\"  Wide:\"%S\"  wide num bytes is %d\n", Utf8Name, WideName, WideNameNumBytes );

    int InfoNumBytes = 0;

    switch ( FileInformationClass )
    {
      case FileDirectoryInformation:
        {
            PFILE_DIRECTORY_INFORMATION Info = ( PFILE_DIRECTORY_INFORMATION ) InfoAddress;
            int StructNumBytes =         offsetof( FILE_DIRECTORY_INFORMATION, FileName );
            InfoNumBytes = StructNumBytes + WideNameNumBytes;
            if ( InfoNumBytes > InfoMaxLength ) return 0;

            Info->NextEntryOffset = ROUND_UP( InfoNumBytes, 8 );
            Info->FileIndex = 0;
            Info->CreationTime   = GET_CreationTime;
            Info->LastAccessTime = GET_LastAccessTime;
            Info->LastWriteTime  = GET_LastWriteTime;
            Info->ChangeTime     = GET_ChangeTime;
            Info->EndOfFile.QuadPart      = DataGetFileNumBytes( Entry );
            Info->AllocationSize.QuadPart = DataGetAllocationNumBytes( Entry );
            Info->FileAttributes          = OrNormal( Entry->FileAttributes );
            Info->FileNameLength          = WideNameNumBytes;
            memcpy( Info->FileName, WideName, WideNameNumBytes );

/*
LogFormatted( " FileIndex %d \n", ( int )Info->FileIndex );
LogFormatted( " CreationTime   $%p \n", Info->CreationTime.QuadPart );
LogFormatted( " LastAccessTime $%p \n", Info->LastAccessTime.QuadPart );
LogFormatted( " LastWriteTime  $%p \n", Info->LastWriteTime.QuadPart );
LogFormatted( " ChangeTime     $%p \n", Info->ChangeTime.QuadPart );
LogFormatted( " EndOfFile      $%p \n", Info->EndOfFile.QuadPart );
LogFormatted( " AllocationSize $%p \n", Info->AllocationSize.QuadPart );
LogFormatted( "FileName <%.*S> \n", ( int )WideNameNumBytes/2, &Info->FileName );
LARGE_INTEGER CurrentTime; CurrentTime.QuadPart = CurrentMillisecond();
LogFormatted( "( Current Time )  $%p \n", CurrentTime.QuadPart );
*/

        }
        break;

      case FileFullDirectoryInformation:
        {
            PFILE_FULL_DIR_INFORMATION Info = ( PFILE_FULL_DIR_INFORMATION ) InfoAddress;
            int StructNumBytes =        offsetof( FILE_FULL_DIR_INFORMATION, FileName );
            InfoNumBytes = StructNumBytes + WideNameNumBytes;
            if ( InfoNumBytes > InfoMaxLength ) return 0;

            Info->NextEntryOffset = ROUND_UP( InfoNumBytes, 8 );
            Info->FileIndex = 0;
            Info->CreationTime   = GET_CreationTime;
            Info->LastAccessTime = GET_LastAccessTime;
            Info->LastWriteTime  = GET_LastWriteTime;
            Info->ChangeTime     = GET_ChangeTime;
            Info->EndOfFile.QuadPart      = DataGetFileNumBytes( Entry );
            Info->AllocationSize.QuadPart = DataGetAllocationNumBytes( Entry );
            Info->FileAttributes          = OrNormal( Entry->FileAttributes );
            Info->FileNameLength          = WideNameNumBytes;
            Info->EaSize                  = 0;
            memcpy( Info->FileName, WideName, WideNameNumBytes );

/*
LogFormatted( " FileIndex %d \n", ( int )Info->FileIndex );
LogFormatted( " CreationTime   $%p \n", Info->CreationTime.QuadPart );
LogFormatted( " LastAccessTime $%p \n", Info->LastAccessTime.QuadPart );
LogFormatted( " LastWriteTime  $%p \n", Info->LastWriteTime.QuadPart );
LogFormatted( " ChangeTime     $%p \n", Info->ChangeTime.QuadPart );
LogFormatted( " EndOfFile      $%p \n", Info->EndOfFile.QuadPart );
LogFormatted( " AllocationSize $%p \n", Info->AllocationSize.QuadPart );
LogFormatted( " EaSize %d \n", Info->EaSize );
LogFormatted( "FileName <%.*S> \n", ( int )WideNameNumBytes/2, &Info->FileName );
LARGE_INTEGER CurrentTime; CurrentTime.QuadPart = CurrentMillisecond();
LogFormatted( "( Current Time )  $%p \n", CurrentTime.QuadPart );
*/

        }
        break;

      case FileIdFullDirectoryInformation:
        {
            PFILE_ID_FULL_DIR_INFORMATION Info = ( PFILE_ID_FULL_DIR_INFORMATION ) InfoAddress;
            int StructNumBytes =           offsetof( FILE_ID_FULL_DIR_INFORMATION, FileName );
            InfoNumBytes = StructNumBytes + WideNameNumBytes;
            if ( InfoNumBytes > InfoMaxLength ) return 0;

            Info->NextEntryOffset = ROUND_UP( InfoNumBytes, 8 );
            Info->FileIndex = 0;
            Info->CreationTime   = GET_CreationTime;
            Info->LastAccessTime = GET_LastAccessTime;
            Info->LastWriteTime  = GET_LastWriteTime;
            Info->ChangeTime     = GET_ChangeTime;
            Info->EndOfFile.QuadPart      = DataGetFileNumBytes( Entry );
            Info->AllocationSize.QuadPart = DataGetAllocationNumBytes( Entry );
            Info->FileAttributes          = OrNormal( Entry->FileAttributes );
            Info->FileNameLength          = WideNameNumBytes;
            Info->EaSize                  = 0;
            Info->FileId.QuadPart         = 0;  //  TODO set this to our internal id?
            memcpy( Info->FileName, WideName, WideNameNumBytes );
        }
        break;

      case FileBothDirectoryInformation:
        {
            PFILE_BOTH_DIR_INFORMATION Info = ( PFILE_BOTH_DIR_INFORMATION ) InfoAddress;
            int StructNumBytes =        offsetof( FILE_BOTH_DIR_INFORMATION, FileName );
            InfoNumBytes = StructNumBytes + WideNameNumBytes;
            if ( InfoNumBytes > InfoMaxLength ) return 0;

            Info->NextEntryOffset = ROUND_UP( InfoNumBytes, 8 );
            Info->FileIndex = 0;
            Info->CreationTime   = GET_CreationTime;
            Info->LastAccessTime = GET_LastAccessTime;
            Info->LastWriteTime  = GET_LastWriteTime;
            Info->ChangeTime     = GET_ChangeTime;
            Info->EndOfFile.QuadPart      = DataGetFileNumBytes( Entry );
            Info->AllocationSize.QuadPart = DataGetAllocationNumBytes( Entry );
            Info->FileAttributes          = OrNormal( Entry->FileAttributes );
            Info->FileNameLength          = WideNameNumBytes;
            Info->EaSize                  = 0;
            Info->ShortNameLength         = 0;
            Zero( Info->ShortName, sizeof( Info->ShortName ) );
            memcpy( Info->FileName, WideName, WideNameNumBytes );
        }
        break;

      case FileIdBothDirectoryInformation:
        {
            PFILE_ID_BOTH_DIR_INFORMATION Info = ( PFILE_ID_BOTH_DIR_INFORMATION ) InfoAddress;
            int StructNumBytes =           offsetof( FILE_ID_BOTH_DIR_INFORMATION, FileName );
            InfoNumBytes = StructNumBytes + WideNameNumBytes;
            if ( InfoNumBytes > InfoMaxLength ) return 0;

            Info->NextEntryOffset = ROUND_UP( InfoNumBytes, 8 );
            Info->FileIndex = 0;
            Info->CreationTime   = GET_CreationTime;
            Info->LastAccessTime = GET_LastAccessTime;
            Info->LastWriteTime  = GET_LastWriteTime;
            Info->ChangeTime     = GET_ChangeTime;
            Info->EndOfFile.QuadPart      = DataGetFileNumBytes( Entry );
            Info->AllocationSize.QuadPart = DataGetAllocationNumBytes( Entry );
            Info->FileAttributes          = OrNormal( Entry->FileAttributes );
            Info->FileNameLength          = WideNameNumBytes;
            Info->EaSize                  = 0;
            Info->ShortNameLength         = 0;
            Zero( Info->ShortName, sizeof( Info->ShortName ) );
            Info->FileId.QuadPart         = 0;  //  TODO set this to our internal id?
            memcpy( Info->FileName, WideName, WideNameNumBytes );
        }
        break;

      case FileNamesInformation:
        {
            PFILE_NAMES_INFORMATION Info = ( PFILE_NAMES_INFORMATION ) InfoAddress;
            int StructNumBytes =     offsetof( FILE_NAMES_INFORMATION, FileName );
            InfoNumBytes = StructNumBytes + WideNameNumBytes;
            if ( InfoNumBytes > InfoMaxLength ) return 0;

            Info->NextEntryOffset = ROUND_UP( InfoNumBytes, 8 );
            Info->FileIndex = 0;
            Info->FileNameLength = WideNameNumBytes;
            memcpy( Info->FileName, WideName, WideNameNumBytes );
        }
        break;

      default:
        {
LogMarker();
AlwaysBreakToDebugger();
        }
    }

    return ROUND_UP( InfoNumBytes, 8 );
}

//////////////////////////////////////////////////////////////////////

NTSTATUS QueryDirectory( ICB_ Icb )

{
    PIRP                   Irp                     = Icb->Irp;
    IRPSP_                 IrpSp                   = IoGetCurrentIrpStackLocation( Irp );
    FILE_INFORMATION_CLASS FileInformationClass    = IrpSp->Parameters.QueryDirectory.FileInformationClass;
    int                    UserBufferLength        = IrpSp->Parameters.QueryDirectory.Length;
    PUNICODE_STRING        WideNameOrPatternPassed = IrpSp->Parameters.QueryDirectory.FileName;
    ULONG_PTR*             UsedLength_             = &Irp->IoStatus.Information;
    PFILE_OBJECT           FileObject              = IrpSp->FileObject;
    FCB_                   Fcb                     = FileObject->FsContext;
    CCB_                   Ccb                     = FileObject->FsContext2;
    ID                     DirectoryId             = Fcb->Id;

    B1 RestartScan       = BitIsSet( IrpSp->Flags, SL_RESTART_SCAN );
    B1 ReturnSingleEntry = BitIsSet( IrpSp->Flags, SL_RETURN_SINGLE_ENTRY );

    U1_ UserBuffer = IrpBuffer( Irp );
    if ( ! UserBuffer ) return STATUS_INVALID_USER_BUFFER;

    if ( IdIsAFile( DirectoryId ) ) return STATUS_INVALID_PARAMETER;

    ENTRY_ DirectoryEntry = Entries[DirectoryId];

    //  If this is our first time...
    if ( Ccb->FirstQuery )
    {
        //  Keep the name or pattern passed, if any.
        if ( WideNameOrPatternPassed )
        {
            WideCharactersToUtf8String( WideNameOrPatternPassed->Length, WideNameOrPatternPassed->Buffer, Ccb->NameOrPattern );
            //  https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-_fsrtl_advanced_fcb_header-fsrtldoesnamecontainwildcards
            if ( Ccb->NameOrPattern[0] == '*' && Ccb->NameOrPattern[1] == 0 )
            {
                Ccb->NameOrPattern[0] = 0;  //  asterisk same as no pattern.
            }
            else
            {
                Ccb->IsAPattern = DoesStringContainWildcards( Ccb->NameOrPattern );
            }
        }
    }


//LogFormatted( "Ccb->LastProcessedName  is \"%s\"\n", Ccb->LastProcessedName );
//LogFormatted( "Ccb->NameOrPattern      is \"%s\"\n", Ccb->NameOrPattern );
//LogFormatted( "Ccb->IsAPattern = %d\n", ( int ) Ccb->IsAPattern );
//LogFormatted( "RestartScan = %d\n", ( int ) RestartScan );


    //  Get the child to start with.
    ID ChildNode;
    if ( RestartScan )
    {
        ChildNode = EntryFirst( ChildrenTree( DirectoryEntry ) );
    }
    else
    {
        if ( Ccb->FirstQuery )
        {
            if ( Ccb->IsAPattern )
            {
                ChildNode = EntryFirst( ChildrenTree( DirectoryEntry ) );
            }
            else
            {
                if ( Ccb->NameOrPattern[0] ) ChildNode = EntryFind(  ChildrenTree_( DirectoryEntry ), Ccb->NameOrPattern );
                else                         ChildNode = EntryFirst( ChildrenTree( DirectoryEntry ) );
            }
        }
        else
        {
            ChildNode = EntryNear( ChildrenTree_( DirectoryEntry ), Ccb->LastProcessedName, GT );
        }
    }


    //  Add entries while all is well.
    int InfoMaxLength   = UserBufferLength;
    U1_ InfoAddress     = UserBuffer;
    U1_ LastInfoAddress = InfoAddress;
    ENTRY_ LastProcessedChild = 0;
    *UsedLength_ = 0;
    while ( ChildNode )
    {
        ENTRY_ ChildEntry = Entries[ ChildNode ];

        //  Should this entry be included?
        B1 Match;
        if ( Ccb->NameOrPattern[0] == 0 )  //  We are not using a name or pattern; we should always match.
        {
            Match = TRUE;
LogFormatted( "Match = %d for \"%s\" because Ccb->NameOrPattern[0] == 0\n", ( int )Match, ChildEntry->Name );
        }
        else
        {
            S1_ Name = ChildEntry->Name;
            if ( Ccb->IsAPattern )
            {
                Match = WildcardCompare( Ccb->NameOrPattern, ( int ) strlen( Ccb->NameOrPattern ), Name, ( int )strlen( Name ) );
LogFormatted( "Match = %d for \"%s\" because Ccb->IsAPattern of \"%s\"\n", ( int )Match, ChildEntry->Name, Ccb->NameOrPattern );
            }
            else
            {
                Match = ( _stricmp( Name, Ccb->NameOrPattern ) == 0 );
LogFormatted( "Match = %d for \"%s\" because NOT Ccb->IsAPattern of \"%s\"\n", ( int )Match, ChildEntry->Name, Ccb->NameOrPattern );
            }
        }

//LogFormatted( "Match = %d for \"%s\"\n", ( int )Match, ChildEntry->Name );

        if ( Match )
        {
            int InfoNumBytes = fillOneQueryDirectoryInfo( FileInformationClass, InfoAddress, InfoMaxLength, ChildEntry );

            if ( ! InfoNumBytes )  //  If it didn't fit...
            {
                break;
            }

            strcpy( Ccb->LastProcessedName, ChildEntry->Name );
            LastInfoAddress = InfoAddress;
            InfoAddress    += InfoNumBytes;
            *UsedLength_   += InfoNumBytes;
            InfoMaxLength  -= InfoNumBytes;

            LastProcessedChild = ChildEntry;
            if ( ReturnSingleEntry ) break;
        }
        else
        {
            LastProcessedChild = ChildEntry;
        }

        ChildNode = EntryNext( ChildNode );

    }

    if ( LastProcessedChild )
    {
        strcpy( Ccb->LastProcessedName, LastProcessedChild->Name );
    }


    *( ULONG* )LastInfoAddress = 0;  //  Set struct's NextEntryOffset to zero.

    Ccb->FirstQuery = FALSE;



    //////if ( *UsedLength_ ) return STATUS_SUCCESS;
    //////if ( Ccb->FirstQuery ) return STATUS_NO_SUCH_FILE;
    //////return STATUS_NO_MORE_FILES;


//  from  NTAPI Undocumented Functions
//  http://undocumented.ntinternals.net/index.html?page=UserMode%2FUndocumented%20Functions%2FNT%20Objects%2FFile%2FNtQueryDirectoryFile.html
//  STATUS_SUCCESS - Enumeration has results in FileInformation buffer.
//  STATUS_NO_MORE_FILES - FileInformation buffer is empty, and next call isn't needed.
//  STATUS_NO_SUCH_FILE - Returned when FileMask parameter specify exactly one file (don't contains '*' or '?' characters), and queried directory don't contains that file.

    if ( *UsedLength_ ) return STATUS_SUCCESS;

    B1 SpecificFile = Ccb->NameOrPattern[0] && ! Ccb->IsAPattern;
    return SpecificFile ? STATUS_NO_SUCH_FILE : STATUS_NO_MORE_FILES;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS NotifyChangeDirectory ( ICB_ Icb )

{
    PIRP         Irp        = Icb->Irp;
    IRPSP_       IrpSp      = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    FCB_         Fcb        = FileObject->FsContext;

    switch ( IrpSp->MinorFunction )
    {
      case IRP_MN_NOTIFY_CHANGE_DIRECTORY    :
        AlwaysLogString( "IRP_MN_NOTIFY_CHANGE_DIRECTORY !\n" );
        break;

      case IRP_MN_NOTIFY_CHANGE_DIRECTORY_EX :
        AlwaysLogString( "IRP_MN_NOTIFY_CHANGE_DIRECTORY_EX !\n" );
        break;

      default:
        AlwaysLogFormatted( "Oops! NotifyChangeDirectory called with bad minor\n" );
        break;
    }

    if ( IdIsAFile( Fcb->Id ) ) return STATUS_INVALID_PARAMETER;

    PDEVICE_OBJECT DeviceObject = Icb->DeviceObject;
    if ( DeviceObject == Volume_TailwindDeviceObject ) return STATUS_INVALID_DEVICE_REQUEST;

    return STATUS_NOT_IMPLEMENTED;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS IrpMjDirectoryControl( ICB_ Icb )

{
    PIRP     Irp   = Icb->Irp;
    IRPSP_   IrpSp = IoGetCurrentIrpStackLocation( Irp );

    switch ( IrpSp->MinorFunction )
    {
      case IRP_MN_QUERY_DIRECTORY            : return QueryDirectory(         Icb );
      case IRP_MN_NOTIFY_CHANGE_DIRECTORY    : return NotifyChangeDirectory(  Icb );
      case IRP_MN_NOTIFY_CHANGE_DIRECTORY_EX : return NotifyChangeDirectory(  Icb );
    }

    return STATUS_INVALID_DEVICE_REQUEST;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


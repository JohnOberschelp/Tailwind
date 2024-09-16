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

NTSTATUS IrpMjCreate_Entry ( ICB_ Icb )

{
    PIRP           Irp          = Icb->Irp;
    IRPSP_         IrpSp        = IoGetCurrentIrpStackLocation( Irp );
    PDEVICE_OBJECT DeviceObject = Icb->DeviceObject;
    VCB_           Vcb          = DeviceObject->DeviceExtension;
    ULONG          Options      = IrpSp->Parameters.Create.Options;
    U1             Disposition  = Options >> 24;
    ULONG          CreateHint   = Options & 0xFFFFFF;


LogFormatted( "CreateHint is %#010X   Disposition is %s\n", CreateHint, GetDispositionAsText( Disposition ) );
/*
#define FILE_DIRECTORY_FILE                     0x0000'0001
#define FILE_WRITE_THROUGH                      0x0000'0002
#define FILE_SEQUENTIAL_ONLY                    0x0000'0004
#define FILE_NO_INTERMEDIATE_BUFFERING          0x0000'0008
#define FILE_SYNCHRONOUS_IO_ALERT               0x0000'0010
#define FILE_SYNCHRONOUS_IO_NONALERT            0x0000'0020
#define FILE_NON_DIRECTORY_FILE                 0x0000'0040
#define FILE_DISALLOW_EXCLUSIVE                 0x0002'0000
#define FILE_OPEN_REPARSE_POINT                 0x0020'0000
#define FILE_OPEN_FOR_FREE_SPACE_QUERY          0x0080'0000
*/

    //  https://docs.microsoft.com/en-us/windows-hardware/drivers/ifs/irp-mj-create
    UCHAR Flags = IrpSp->Flags;

if ( BitIsSet( Flags, SL_FORCE_ACCESS_CHECK ) )  //  0x01
{
LogString( "TODO Should be doing access checks as if call was from user mode. Security?\n" );
}

    if ( BitIsSet( Flags, SL_OPEN_PAGING_FILE          ) ) AlwaysBreakToDebugger();  //  0x02    //  eb eip 0x90
    if ( BitIsSet( Flags, SL_STOP_ON_SYMLINK           ) ) AlwaysBreakToDebugger();  //  0x08    //  eb eip 0x90
    if ( BitIsSet( Flags, SL_IGNORE_READONLY_ATTRIBUTE ) ) AlwaysBreakToDebugger();  //  0x40    //  eb eip 0x90
    if ( BitIsSet( Flags, SL_CASE_SENSITIVE            ) ) AlwaysBreakToDebugger();  //  0x80    //  eb eip 0x90

    B1 OpenTargetDirectory = BitIsSet( Flags, SL_OPEN_TARGET_DIRECTORY );  //  0x04


    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    do
    {
        PFILE_OBJECT FileObject = IrpSp->FileObject;

        CCB_ Ccb = MakeCcb();
        if ( ! Ccb )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }


        //  Start from an ancestor or root.
        ID AncestorId = Vcb->RootId;
        if ( FileObject->RelatedFileObject )
        {
            FCB_ AncestorFcb = FileObject->RelatedFileObject->FsContext;
            AncestorId = AncestorFcb->Id;
        }


        //  Convert the ( path and? ) name to utf8.
        S1 PathAndName[2048];  //  TODO
        WidePathAndNameCharactersToUtf8String( FileObject->FileName.Length, FileObject->FileName.Buffer, PathAndName );
LogFormatted( "PathAndName is \"%s\"\n", PathAndName );


        //  For funny little OpenTargetDirectory,	
        if ( OpenTargetDirectory )
        {
LogString( "funny little OpenTargetDirectory\n" );
            B1 OK = RemoveTrailingNameFromPath( PathAndName );///////////////s1_
            if ( ! OK )	
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
        }


        //  Find or make an Entry.


        ID ParentId;
        ID EntryId;
        char* Name;
        Status = FindByPathAndName( AncestorId, PathAndName, &Name, &ParentId, &EntryId );
        if ( Status && Status != STATUS_OBJECT_NAME_NOT_FOUND ) break;
ASSERT( ParentId );
        ENTRY_ Entry       = Entries[EntryId];
ASSERT( ( ! Entry ) == ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) );


        //CreateDisposition
        //-----------------
        //Specifies what to do, depending on whether the file already exists, as one of the following values.
        //    FILE_SUPERSEDE      If the file already exists, replace it with the given file. If it does not, create the given file.
        //    FILE_CREATE         If the file already exists, fail the request. If it does not, create the given file.
        //    FILE_OPEN           If the file already exists, open it. If it does not, fail.
        //    FILE_OPEN_IF        If the file already exists, open it. If it does not, create the given file.
        //    FILE_OVERWRITE      If the file already exists, open it and overwrite it. If it does not, fail the request.
        //    FILE_OVERWRITE_IF   If the file already exists, open it and overwrite it. If it does not, create the given file.
        U1 it = Disposition;
        if ( it == FILE_CREATE    &&   Entry ) { Status = STATUS_OBJECT_NAME_COLLISION; break; }
        if ( it == FILE_OPEN      && ! Entry ) { Status = STATUS_OBJECT_NAME_NOT_FOUND; break; } //  TODO correct status?
        if ( it == FILE_OVERWRITE && ! Entry ) { Status = STATUS_OBJECT_NAME_NOT_FOUND; break; } //  TODO correct status?

        if ( ! Entry )
        {


//LogString( "There is no Entry; should we make a file or a directory?\n" );
            B1 MakingANewDirectory = BitIsSet( Options, FILE_DIRECTORY_FILE     );
            B1 MakingANewFile      = BitIsSet( Options, FILE_NON_DIRECTORY_FILE );
            if ( ! MakingANewDirectory && ! MakingANewFile )
            {
                //  If there is a colon, maybe handle specially?!  TODO
                //  I am getting this when I drag FirefoxPortable_91.0.1.English.paf.exe onto e:
                if ( strchr( PathAndName, ':' ) )
                {
LogString( "Returning STATUS_EAS_NOT_SUPPORTED.\n" );
//BreakToDebugger();  //  eb eip 0x90
                Status = STATUS_EAS_NOT_SUPPORTED;  //  TODO is this the right or better result and time and place?
                break;
                }
LogString( "Not Entry and not MakingANewDirectory and not MakingANewFile! \n" );
AlwaysBreakToDebugger();  //  eb eip 0x90
                Status = STATUS_INVALID_PARAMETER;  //  One or the other, right?
                break;
            }


//LogString( "Creating the Entry.\n" );
            U4 DirectoryOrNumRanges = MakingANewDirectory ? A_DIRECTORY : 0;
            EntryId = MakeEntry( ParentId, DirectoryOrNumRanges, Name );

            if ( ! EntryId )
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            Entry = Entries[EntryId];

LogMarker();

/*
            if ( MakingANewFile && it == FILE_OVERWRITE_IF )
            {
                //  Wonder if we ever get here with an AllocationSize.
//LogFormatted( "FILE_OVERWRITE_IF requesting %d bytes.\n", ( int ) Irp->Overlay.AllocationSize.QuadPart );  //  TODO
                if ( Irp->Overlay.AllocationSize.QuadPart != 0 )
                {
LogFormatted( "FILE_OVERWRITE_IF with given AllocationSize of %d. Believe I should NOT ignore it. \n", ( int )Irp->Overlay.AllocationSize.QuadPart );
                }
            }
*/

            //  Irp->Overlay.AllocationSize: Initial allocation size, in bytes, for the file.
            //  A nonzero value has no effect unless the file is being created, overwritten, or superseded.
////////////if ( MakingANewFile && ( it == FILE_CREATE || it == FILE_OVERWRITE || it == FILE_SUPERSEDE ) )  //  TODO what about FILE_OVERWRITE_IF ?
////////////if ( MakingANewFile && ( it == FILE_SUPERSEDE || it == FILE_CREATE || it == FILE_OVERWRITE ) )  //  TODO what about FILE_OVERWRITE_IF ?
////////////if ( MakingANewFile && ( it == FILE_SUPERSEDE || it == FILE_CREATE || it == FILE_OVERWRITE || it == FILE_OVERWRITE_IF ) )
            if ( MakingANewFile )
            {
                U8 InitialAllocationSize = Irp->Overlay.AllocationSize.QuadPart;
                if ( InitialAllocationSize )
                {
LogFormatted( "Requesting an initial allocation of %d bytes.\n", ( int ) InitialAllocationSize );  //  TODO

                    //  Don't set Entry->FileSize here, right??? TODO
                    ID Id = Entry->Id;
                    Status = DataReallocateFile( Id, InitialAllocationSize );
                    Entry = Entries[Id];
                    if ( Status ) break;

                }
            }
        }


        //  We have an Entry.


LogFormatted( "We have an Entry.  Entry->FileAttributes = $%X.\n", Entry->FileAttributes );


        B1 DeleteOnClose = BitIsSet( Options, FILE_DELETE_ON_CLOSE );  //  TODO use FILE_DELETE_ON_CLOSE
        if ( ! Entry->ParentId && DeleteOnClose )
        {
            Status = STATUS_CANNOT_DELETE;  //  Can't delete the root.
            break;
        }


        //  Find or make an Fcb.
//  this line is scaring me because I am not sure we are not recovering a used Id.  TODO TODO TODOTODO
        FCB_ Fcb = LookupFcbById( Vcb, Entry->Id );  //  TODO speedup Don't need to do this search is we had to create the entry.
        if ( ! Fcb )
        {
            Fcb = MakeFcb( Vcb, Entry->Id );  //  TODO when freed AND taken off list if can't make Ccb below?
            if ( ! Fcb )
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
        }


        //  We have an Fcb.
        //  And we are going to succeed.


        if ( it == FILE_SUPERSEDE || it == FILE_OVERWRITE || it == FILE_OVERWRITE_IF )
        {
            ID Id = Entry->Id;
            NTSTATUS xxxxxx = DataResizeFile( Id, 0, ZERO_FILL );  //  TODO difference between SUPERSEDE & OVERWRITE??
ASSERT( ! xxxxxx );
            Entry = Entries[Id];
        }

        if ( DeleteOnClose )
        {
LogString( "Setting DeleteIsPending and FileObject->DeletePending.\n" );  //  TODO
            Fcb->DeleteIsPending = TRUE;
            FileObject->DeletePending = TRUE;  //  TODO OK? One msdn page said read-only.
        }

        Vcb->VcbNumOpenHandles++;
        Vcb->VcbNumReferences++;

        Fcb->NumReferences++;

        FileObject->FsContext       = Fcb;
        FileObject->FsContext2      = Ccb;
        FileObject->PrivateCacheMap = 0;


//---------
        //FileObject->SectionObjectPointer = 0;//&( Fcb->SectionObjectPointers );
        FileObject->SectionObjectPointer = &( Fcb->SectionObjectPointers );

/* Testing : When i run the fsbench mmap test, at CreateFileMappingW() :
If I build this set to 0, I get the confusing GetLastError of
$c1 == 193 == ERROR_BAD_EXE_FORMAT because I actually probably got the Status of
$C000'0020 == STATUS_INVALID_FILE_FOR_SECTION which means
"the file specified does not support sections" per the MSDN ZwCreateSection docs.
If I build it set to &( Fcb->SectionObjectPointers ) , I currently hang.
*/
//---------


        FileObject->Vpb = Vcb->Vpb;

        Status = STATUS_SUCCESS;

LogFormatted( "FcbReferences: %d  NumReferences: %d  NumOpenHandles: %d\n",
                    Fcb->NumReferences, Vcb->VcbNumReferences, Vcb->VcbNumOpenHandles );	

    } while(0);


//  TODO free Ccb Fcb Entry if any failure.xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

    ULONG_PTR Info;

    if ( NT_SUCCESS( Status ) )
    {
        switch( Disposition )
        {
            case FILE_SUPERSEDE:    Info = FILE_SUPERSEDED;  break;
            case FILE_OPEN:         Info = FILE_OPENED;      break;
            case FILE_CREATE:       Info = FILE_CREATED;     break;
            case FILE_OPEN_IF:      Info = FILE_OPENED;      break;
            case FILE_OVERWRITE:    Info = FILE_OVERWRITTEN; break;
            case FILE_OVERWRITE_IF: Info = FILE_OVERWRITTEN; break;
            default:	
BreakToDebugger();
                                    Info = FILE_OPENED;      break;  //  TODO should not happen.
        }
        Irp->IoStatus.Information = Info;
    }

    return Status;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS IrpMjCreate_Volume( ICB_ Icb )

{
    PDEVICE_OBJECT DeviceObject = Icb->DeviceObject;
    VCB_           Vcb          = DeviceObject->DeviceExtension;

    PIRP          Irp        = Icb->Irp;
    IRPSP_        IrpSp      = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT  FileObject = IrpSp->FileObject;
    FileObject->FsContext = Vcb;

    Vcb->VcbNumReferences++;

    Irp->IoStatus.Information = FILE_OPENED;

LogString( "Always succeeds.\n" );

    return STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////

FCB_ LookupFcbById( VCB_ Vcb, ID Id )

{
    for ( LINK_ L = Vcb->OpenFcbsChain.First; L; L = L->Next )
    {
        FCB_ Fcb = OWNER( FCB, OpenFcbsLink, L );

        if ( Fcb->Id == Id ) return Fcb;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS IrpMjCreate ( ICB_ Icb )

{
    PDEVICE_OBJECT DeviceObject = Icb->DeviceObject;
    if ( DeviceObject == Volume_TailwindDeviceObject )
    {
BreakToDebugger();

        Icb->Irp->IoStatus.Information = FILE_OPENED;
        return STATUS_SUCCESS;
    }

    PIRP Irp = Icb->Irp;
    IRPSP_ IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    if ( FileObject->FileName.Length == 0 && FileObject->RelatedFileObject == 0 )
        return IrpMjCreate_Volume( Icb );
    else
        return IrpMjCreate_Entry( Icb );
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


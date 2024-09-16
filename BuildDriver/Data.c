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

Based on "EXTRA/Info/Analysis of the periodic update write policy...", we
should maybe have code that does a writethrough if the drive is not busy,
instead of waiting for a lull with the background thread.

*/
//////////////////////////////////////////////////////////////////////

    ////////////////////////    FEDCBA9876543210
    //....NextProxyAddress = ( U8 ) 0x8000000000001000LL;  //  Starting at high bit on plus 4096.

//////////////////////////////////////////////////////////////////////

NTSTATUS addADataRange( ID Id, U4 NumBytesRequested, U4_ NumBytesResult )

{
    ENTRY_     OldEntry     = Entries[Id];
    FILE_DATA_ OldFileData  = Data( OldEntry );
    U4         OldNumRanges = OldFileData->NumRanges;

    NTSTATUS Status = ResizeEntry( Id, 0, +1 );
    if( Status ) return Status;

    ENTRY_     NewEntry     = Entries[Id];
    FILE_DATA_ NewFileData  = Data( NewEntry );


    U4 NumBytesRoundedUp = ROUND_UP( NumBytesRequested, Volume_BlockSize );
    U4 NumBytesConstrained = min( NumBytesRoundedUp, Volume_SpaceNodeMaxNumBytes );

    NewFileData->AllocationNumBytes += NumBytesConstrained;


    U8 VolumeAddress;
    U4 NumBytesGot;
    Status = SpaceRequestNumBytes( NumBytesConstrained, &VolumeAddress, &NumBytesGot );
ASSERT( ! Status );
    NewFileData->DataRange[OldNumRanges].VolumeAddress = VolumeAddress;


    NewFileData->DataRange[OldNumRanges].NumBytes = NumBytesConstrained;

    *NumBytesResult = NumBytesConstrained;

    return 0;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS removeADataRange( ID Id )

{
    ENTRY_     OldEntry     = Entries[Id];
    FILE_DATA_ OldFileData  = Data( OldEntry );
    U4         OldNumRanges = OldFileData->NumRanges;
ASSERT( OldNumRanges );
    U4         NewNumRanges = OldNumRanges - 1;


    DATA_RANGE RangeToRemove = OldFileData->DataRange[NewNumRanges];


    NTSTATUS Status = ResizeEntry( Id, 0, -1 );
    if( Status ) return Status;


    ENTRY_     NewEntry = Entries[Id];
    FILE_DATA_ NewFileData = Data( NewEntry );
    NewFileData->AllocationNumBytes -= RangeToRemove.NumBytes;


    return 0;
}

//////////////////////////////////////////////////////////////////////

U8 DataGetFileNumBytes( ENTRY_ Entry )

{
    if ( EntryIsADirectory( Entry ) ) return 0;
    FILE_DATA_ FileData = Data( Entry );
    return FileData->FileNumBytes;
}

//////////////////////////////////////////////////////////////////////

U8 DataGetAllocationNumBytes( ENTRY_ Entry )

{
    if ( EntryIsADirectory( Entry ) ) return 0;
    FILE_DATA_ FileData = Data( Entry );
    return FileData->AllocationNumBytes;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS DataReadFromFile( ENTRY_ Entry, U8 Offset, U4 Length, U1_ BufferOut )

{
    if ( Length == 0 ) return 0;

    NTSTATUS Status = AccessCacheForFile( Entry, OUT_OF_CACHE, BufferOut, Offset, Length );
ASSERT( Status == 0 || Status == STATUS_PENDING );

    return Status;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS DataWriteToFile( ENTRY_ Entry, U8 Offset, U4 Length, U1_ BufferIn )

{
    if ( Length == 0 ) return 0;

    NTSTATUS Status = AccessCacheForFile( Entry, INTO_CACHE, BufferIn, Offset, Length );
ASSERT( Status == 0 );

    return Status;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS DataReallocateFile( ID Id, U8 RequestedMinimumAllocationSize )
{
    ENTRY_ Entry = Entries[Id];
    U8 FileNumBytes = DataGetFileNumBytes( Entry );
ASSERT( FileNumBytes <= RequestedMinimumAllocationSize );

    RequestedMinimumAllocationSize = ROUND_UP( RequestedMinimumAllocationSize, Volume_BlockSize );

    U8 OldAllocationSize = DataGetAllocationNumBytes( Entry );

    S8 Delta = RequestedMinimumAllocationSize - OldAllocationSize;

    //  Handle same.
    if ( ! Delta ) return 0;

    //  Handle larger.
    if ( Delta > 0 )
    {
        //  Add as many data ranges as we need to.
        U4 NumBytesWant = ( U4 ) Delta;
        U4 NumBytesGot;
        for (;;)
        {
            NTSTATUS Status = addADataRange( Id, NumBytesWant, &NumBytesGot );
            if ( Status ) return Status;
            if ( NumBytesGot >= NumBytesWant ) break;
            NumBytesWant -= NumBytesGot;
        }
        return 0;
    }

    //  Handle smaller.

    //  While we can remove ranges, do it.
    for ( ;; )
    {
        if ( Data( Entry )->NumRanges == 0 ) break;
        DATA_RANGE LastRange = Data( Entry )->DataRange[Data( Entry )->NumRanges - 1];
        U8 AllocationSizeBefore = Data( Entry )->AllocationNumBytes;
        U8 AllocationSizeAfter  = AllocationSizeBefore - LastRange.NumBytes;
        if ( AllocationSizeAfter < RequestedMinimumAllocationSize ) break;

        NTSTATUS Status = removeADataRange( Id );
        Entry = Entries[Id];
        if ( Status ) return Status;
    }

    //  TODO note: the last ram range may be larger than needed.

    return 0;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS DataResizeFile( ID Id, U8 NewFileSize, FILL_TYPE FillType )

{
    NTSTATUS Status = STATUS__PRIVATE__NEVER_SET;


    //  Reallocate the file, if necessary.
    if ( NewFileSize > DataGetAllocationNumBytes( Entries[Id] ) )
    {
        Status = DataReallocateFile( Id, NewFileSize );  //  the minimum.
        if ( Status ) return Status;
    }


    if ( FillType == ZERO_FILL )
    {
        S8 Expansion = NewFileSize - DataGetFileNumBytes( Entries[Id] );
        if ( Expansion > 0 )
        {
            //  Fill to the new offset with zeroes.
            Status = DataWriteToFile( Entries[Id], DataGetFileNumBytes( Entries[Id] ), ( int ) Expansion, 0 );
            //  TODO what about status here?	
        }
    }

    Data( Entries[Id] )->FileNumBytes = NewFileSize;

    return 0;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


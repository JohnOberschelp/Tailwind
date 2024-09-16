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

NTSTATUS EntriesStartupEmpty( U4 RequestedNumBytes )

{
LogString( "Entering EntriesStartupEmpty.\n" );

    EntriesTotalAllocation = ROUND_UP( RequestedNumBytes, 4096 );  //  TODO

    if ( EntriesBytes ) return STATUS_INVALID_PARAMETER;  //  TODO need real value
    V_ RawBytes = AllocateMemory( EntriesTotalAllocation );
    if ( ! RawBytes ) return STATUS_INSUFFICIENT_RESOURCES;

    memset( RawBytes, -1, EntriesTotalAllocation );  //  TODO temp test

    EntriesBytes = RawBytes;
    EntriesFirstFreeByte     = 0;
    EntriesNumBytesAvailable = EntriesTotalAllocation;

LogString( "Leaving  EntriesStartupEmpty.\n" );

    return 0;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS WhereTableStartupEmpty( U4 RequestedNumBytes )

{
LogString( "Entering WhereTableStartupEmpty.\n" );

ASSERT( ! Entries );
    Volume_WhereTableTotalAllocation = ROUND_UP( RequestedNumBytes, 4096 );  //  TODO
    Entries = AllocateMemory( Volume_WhereTableTotalAllocation );  //  Volume_WhereTableTotalAllocation );
    if ( ! Entries )
    {
        FreeMemory( EntriesBytes );
        EntriesBytes = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    Entries[0] = 0;  //  TODO Index of zero reserved for errors like not found.
    Volume_WhereTableMaxId = Volume_WhereTableTotalAllocation / sizeof( ENTRY_ );
    Volume_WhereTableLastRecycledID = 0;  //  i.e. No recycled IDs.
    Volume_WhereTableFirstUnusedBottomID = 1;  //  Index of zero reserved for errors like not found.

    Volume_TotalNumberOfEntries = 0;

LogString( "Leaving  WhereTableStartupEmpty.\n" );

    return 0;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS EntriesShutdown()

{
    if ( ! EntriesBytes ) return STATUS_INVALID_PARAMETER;  //  TODO need real value

    FreeMemory( EntriesBytes );
    EntriesBytes = 0;

    return 0;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS WhereTableShutdown()

{
    FreeMemory( Entries );
    Entries = 0;

    return 0;
}

//////////////////////////////////////////////////////////////////////

ENTRY_ EntriesAllocate( U4 NumBytesToAllocate )

{
    if ( NumBytesToAllocate > EntriesNumBytesAvailable ) return 0;

    ENTRY_ Entry = ( ENTRY_ ) &EntriesBytes[EntriesFirstFreeByte];
    Zero( Entry, NumBytesToAllocate );

    EntriesFirstFreeByte     += NumBytesToAllocate;
    EntriesNumBytesAvailable -= NumBytesToAllocate;

    return Entry;
}

//////////////////////////////////////////////////////////////////////

void EntriesFree( ENTRY_ Entry )

{
    U4 Size = EntrySize( Entry );

    //  Special case the last entry.
    U1_ u1 = ( U1_ ) Entry;
    if ( u1 + Size ==  &EntriesBytes[ EntriesFirstFreeByte ] )
    {
        EntriesFirstFreeByte     -= Size;
        EntriesNumBytesAvailable += Size;
        return;
    }

    //  Stomp on the ID and save the size.
    U4_ u4 = ( U4_ ) u1;
    u4[0] = 0;
    u4[1] = Size;
}

//////////////////////////////////////////////////////////////////////

//  May goober up any ENTRY_	

void EntriesCompact()

{
    U1_ Fm = EntriesBytes;
    U1_ To = Fm;
    U4 doubleCheckTheNumberOfEntries = 0;
    for (;;)
    {

if ( Fm == EntriesBytes + EntriesFirstFreeByte ) break;

        if ( ( ( U4_ ) Fm ) [0] )
        {
            doubleCheckTheNumberOfEntries++;
            //  An existing Entry.
            ENTRY_ Entry = ( ENTRY_ ) Fm;
            ID Id = Entry->Id;

            U4 Size = EntrySize( Entry );

ASSERT( ( U1_ ) Entries[Id] == Fm );
            if ( Fm != To ) memmove( To, Fm, Size );
            Entries[Id] = ( ENTRY_ ) To;
            Fm += Size;
            To += Size;
        }
        else
        {
            //  A freed Entry.
            U4 Size = ( ( U4_ ) Fm ) [1];
            Fm += Size;
        }
    }

    EntriesFirstFreeByte     = ( U4 ) ( To - &EntriesBytes[0] );
    EntriesNumBytesAvailable = EntriesTotalAllocation - EntriesFirstFreeByte;

AlwaysLogFormatted( "%d doubleCheckTheNumberOfEntries   %d Volume_TotalNumberOfEntries\n", doubleCheckTheNumberOfEntries, Volume_TotalNumberOfEntries );
ASSERT( doubleCheckTheNumberOfEntries == Volume_TotalNumberOfEntries );
}

//////////////////////////////////////////////////////////////////////

NTSTATUS EntriesFreeze( V_ Buffer, U4 NumBytes )

{
    PDEVICE_OBJECT DeviceObject = Volume_PhysicalDeviceObject;
    U8 Offset;
    NTSTATUS Status;

    Offset = Volume_EntriesStart;

AlwaysLogFormatted( "Writing EntriesBytes. %d bytes.\n", NumBytes );
UINT64 msFm = CurrentMillisecond();

    Status = WriteBlockDevice( DeviceObject, Offset, NumBytes, Buffer, MAY_VERIFY );
    if ( Status ) return Status;

UINT64 msTo = CurrentMillisecond();
AlwaysLogFormatted( "Wrote   EntriesBytes. %d bytes in %d ms.\n", NumBytes, ( int ) ( msTo - msFm ) );

    return 0;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS EntriesThaw()

{
//  TODO great time to remove entries from the Volume_WhereTableLastRecycledID chain?
//  need to preserve supporting variables.

    PDEVICE_OBJECT DeviceObject = Volume_PhysicalDeviceObject;
    U8 Offset;
    U4 Length;
    V_ Buffer;
    NTSTATUS Status;

ASSERT( ! Entries );
ASSERT( ! EntriesBytes );

AlwaysLogFormatted( "Reading EntriesBytes. %d bytes.\n", EntriesTotalAllocation );
UINT64 msFm = CurrentMillisecond();

    //  Allocate the EntriesBytes.
    if ( EntriesBytes ) return STATUS_INVALID_PARAMETER;  //  TODO need real value

AlwaysLogString( "allocating\n" );

    V_ RawBytes = AllocateMemory( EntriesTotalAllocation );
    if ( ! RawBytes ) return STATUS_INSUFFICIENT_RESOURCES;

AlwaysLogString( "-1\n" );

    memset( RawBytes, -1, EntriesTotalAllocation );  //  TODO temp test
    EntriesBytes = RawBytes;

AlwaysLogString( "reading\n" );

    //  Read the EntriesBytes.
    Offset = Volume_EntriesStart;
    Length = ROUND_UP( EntriesFirstFreeByte, 4096 );//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    Buffer = EntriesBytes;

AlwaysLogFormatted( "reading %d bytes\n", Length );

    Status = ReadBlockDevice( DeviceObject, Offset, Length, Buffer, MAY_VERIFY );
    if ( Status ) return Status;


UINT64 msTo = CurrentMillisecond();
AlwaysLogFormatted( "Read EntriesBytes. %d bytes in %d ms.\n", Length, ( int ) ( msTo - msFm ) );


    //  Rebuild the Entries array.

AlwaysLogFormatted( "Rebuilding the Entries array. Allocating %d bytes.\n", Volume_WhereTableTotalAllocation );

ASSERT( ! Entries );
    Entries = AllocateMemory( Volume_WhereTableTotalAllocation );
    if ( ! Entries )
    {
        FreeMemory( EntriesBytes );
        EntriesBytes = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    Entries[0] = 0;  //  TODO Index of zero reserved for errors like not found.


AlwaysLogString( "Allocated\n" );

AlwaysLogString( "TEMPORARY set all memory addresses to zero.\n" );

    U1_ Fm = EntriesBytes;
    for (;;)
    {
        if ( Fm == EntriesBytes + EntriesFirstFreeByte ) break;

        if ( ( ( U4_ ) Fm ) [0] )
        {
            //  An existing Entry.
            ENTRY_ Entry = ( ENTRY_ ) Fm;

            ID Id = Entry->Id;
            Entries[Id] = Entry;

            U4 Size = EntrySize( Entry );
            Fm += Size;
        }
        else
        {
            //  A freed Entry.
            U4 Size = ( ( U4_ ) Fm ) [1];
            Fm += Size;
        }
    }
    //  Rebuild a recycled ids list. Any ones that are zero are unused.
    Volume_WhereTableLastRecycledID = 0;
    for ( ID Id = 1; Id < ( int ) Volume_TotalNumberOfEntries; Id++ )
    {
        if ( Entries[Id] ) continue;
        Entries[Id] = ( ENTRY_ ) ( U8 ) Volume_WhereTableLastRecycledID;
        Volume_WhereTableLastRecycledID = Id;
    }

AlwaysLogString( "b\n" );

    return 0;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS OverviewFreeze1( U1_ Buffer )

{
    memset( Buffer, -1, 4096 );  //  TODO temp
    U1_ b = Buffer;

    PUT8( b, 0 )                         //  Will be number of bytes.
    PUT4( b, Volume_TotalNumberOfEntries )
    PUT4( b, Volume_WhereTableMaxId )
    PUT4( b, Volume_WhereTableTotalAllocation )
    PUT4( b, Volume_WhereTableLastRecycledID )
    PUT4( b, Volume_WhereTableFirstUnusedBottomID )


    PUT4( b, Volume_SpaceNodeMaxNumBytes )
    PUT4( b, Volume_BlockSize )
    PUT8( b, Volume_OverviewStart )
    PUT8( b, Volume_OverviewNumBytes )
    PUT8( b, Volume_EntriesStart )
    PUT8( b, Volume_EntriesNumBytes )
    PUT8( b, Volume_DataStart )
    PUT8( b, Volume_DataNumBytes )
    PUT8( b, Volume_TotalNumBytes )


    PUT4( b, EntriesTotalAllocation )
    PUT4( b, EntriesNumBytesAvailable )
    PUT4( b, EntriesFirstFreeByte )


    ( ( U8_ ) Buffer )[0] = b - Buffer;  //  Number of bytes.

AlwaysLogString( "OverviewFreeze1 done.\n" );

    return 0;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS OverviewFreeze2( U1_ Buffer )

{
    PDEVICE_OBJECT DeviceObject = Volume_PhysicalDeviceObject;
    U8 Offset = Volume_OverviewStart;
    U4 Length = 4096;

AlwaysLogFormatted( "Writing.\n" );
UINT64 msFm = CurrentMillisecond();

    NTSTATUS Status = WriteBlockDevice( DeviceObject, Offset, Length, Buffer, MAY_VERIFY );
    if ( Status ) return Status;

UINT64 msTo = CurrentMillisecond();
AlwaysLogFormatted( "Wrote %d bytes in %d ms.\n", Length, ( int ) ( msTo - msFm ) );

    return 0;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS OverviewThaw()

{
    U1 Buffer[4096];

    PDEVICE_OBJECT DeviceObject = Volume_PhysicalDeviceObject;
    U8 Offset = Volume_OverviewStart;
    U4 Length = 4096;//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    NTSTATUS Status = ReadBlockDevice( DeviceObject, Offset, Length, Buffer, MAY_VERIFY );
    if ( Status ) return Status;

    U1_ b = Buffer;

    U8 Length8;

    GET8( b, Length8 )
    GET4( b, Volume_TotalNumberOfEntries )
    GET4( b, Volume_WhereTableMaxId )
    GET4( b, Volume_WhereTableTotalAllocation )
    GET4( b, Volume_WhereTableLastRecycledID )
    GET4( b, Volume_WhereTableFirstUnusedBottomID )


    GET4( b, Volume_SpaceNodeMaxNumBytes )
    GET4( b, Volume_BlockSize )
    GET8( b, Volume_OverviewStart )
    GET8( b, Volume_OverviewNumBytes )
    GET8( b, Volume_EntriesStart )
    GET8( b, Volume_EntriesNumBytes )
    GET8( b, Volume_DataStart )
    GET8( b, Volume_DataNumBytes )
    GET8( b, Volume_TotalNumBytes )


    GET4( b, EntriesTotalAllocation )
    GET4( b, EntriesNumBytesAvailable )
    GET4( b, EntriesFirstFreeByte )

AlwaysLogString( "OverviewThaw\n" );

    return 0;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


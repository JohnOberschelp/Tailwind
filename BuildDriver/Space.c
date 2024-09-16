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

#pragma warning( disable : 4706 )  //  assignment within conditional ( ex ) if ( a = b )

//////////////////////////////////////////////////////////////////////
//
//  Spaces By Address Set

//--------------------------------------------------------------------

inline void byAddressRotateL( SET_NODE_ *rootHandle, SET_NODE_ x )

{
    SET_NODE_ y = x->R, p = x->P;
    if ( x->R = y->L ) y->L->P = x;
    if ( ! ( y->P = p ) )
        *rootHandle = y;
    else
    {
        if ( x == p->L ) p->L = y;
        else             p->R = y;
    }
    ( y->L = x )->P = y;
}

//--------------------------------------------------------------------

inline void byAddressRotateR( SET_NODE_ *rootHandle, SET_NODE_ y )

{
    SET_NODE_ x = y->L, p = y->P;
    if ( y->L = x->R ) x->R->P = y;
    if ( ! ( x->P = p ) )
        *rootHandle = x;
    else
    {
        if ( y == p->L ) p->L = x;
        else             p->R = x;
    }
    ( x->R = y )->P = x;
}

//--------------------------------------------------------------------

static void byAddressSplay( SET_NODE_ *rootHandle, SET_NODE_ x )

{
    SET_NODE_ p;
    while ( p = x->P )
    {
        if ( ! p->P )
        {
            if ( p->L == x ) byAddressRotateR( rootHandle, p );
            else             byAddressRotateL( rootHandle, p );
        }
        else
        {
            if ( p == p->P->R )
            {
                if ( p->R == x ) byAddressRotateL( rootHandle, p->P );
                else             byAddressRotateR( rootHandle, p );
                byAddressRotateL( rootHandle, x->P );
            }
            else
            {
                if ( p->L == x ) byAddressRotateR( rootHandle, p->P );
                else             byAddressRotateL( rootHandle, p );
                byAddressRotateR( rootHandle, x->P );
            }
        }
    }
}

//--------------------------------------------------------------------

inline int byAddressSeek( SET_NODE_ from, SET_NODE_ *resultHandle, U8 address )

{
    SET_NODE_ x = from;
    for ( ;; )
    {

        SPACE_RANGE_ Range = OWNER( SPACE_RANGE, ByVolumeAddress, x );
        U8 address2 = Range->VolumeAddress;

        int diff = ( address == address2 ) ? 0 : ( address < address2 ? -1 : 1 );  //  TODO speedup


        if ( diff < 0 )
        {
            if ( ! x->L ) { *resultHandle = x; return -1; }
            x = x->L;
        }
        else
        if ( diff > 0 )
        {
            if ( ! x->R ) { *resultHandle = x; return  1; }
            x = x->R;
        }
        else              { *resultHandle = x; return  0; }
    }
}

//--------------------------------------------------------------------

SET_NODE_ ByAddressNext( SET_NODE_ x )

{
    if ( x->R ) { x = x->R; while ( x->L ) x = x->L; return x; }
    SET_NODE_ p = x->P;
    while ( p && x == p->R ) { x = p; p = p->P; }
    return p;
}

//--------------------------------------------------------------------

SET_NODE_ ByAddressPrev( SET_NODE_ x )

{
    if ( x->L ) { x = x->L; while ( x->R ) x = x->R; return x; }
    SET_NODE_ p = x->P;
    while ( p && x == p->L ) { x = p; p = p->P; }
    return p;
}

//--------------------------------------------------------------------

SET_NODE_ ByAddressFirst( SET_NODE_ x )  //  NOTE does not bother to go up P to P!!

{
    if ( x ) while ( x->L ) x = x->L;
    return x;
}

//--------------------------------------------------------------------

SET_NODE_ ByAddressLast( SET_NODE_ x )  //  NOTE does not bother to go up P to P!!

{
    if ( x ) while ( x->R ) x = x->R;
    return x;
}

//--------------------------------------------------------------------

SET_NODE_ ByAddressFind( SET_NODE_ *rootHandle, U8 address )

{
    if ( ! *rootHandle ) return 0;
    SET_NODE_ x;
    int direction = byAddressSeek( *rootHandle, &x, address );
    byAddressSplay( rootHandle, x );
    return direction?0:x;
}

//--------------------------------------------------------------------

SET_NODE_ ByAddressNear( SET_NODE_ *rootHandle, U8 address, NEIGHBOR want )

{
    //  Return a node relative to ( <, <=, ==, >=, or > ) an address.
    if ( ! *rootHandle ) return 0;
    SET_NODE_ x;
    int dir = byAddressSeek( *rootHandle, &x, address );
    if ( ( dir == 0 && want == GT ) || ( dir > 0 && want >= GE ) ) x = ByAddressNext( x );
    else
    if ( ( dir == 0 && want == LT ) || ( dir < 0 && want <= LE ) ) x = ByAddressPrev( x );
    else
    if ( dir != 0 && want == EQ ) x = 0;
    if ( x ) byAddressSplay( rootHandle, x );
    return x;
}

//--------------------------------------------------------------------

int ByAddressAttach( SET_NODE_ *rootHandle, SET_NODE_ x, U8 address )

{
    if ( ! *rootHandle )
    {
        *rootHandle = x;
        x->P = x->L = x->R = 0;
    }
    else
    {
        SET_NODE_ f;
        int diff = byAddressSeek( *rootHandle, &f, address );
        if ( ! diff ) return 0;
        if ( diff < 0 ) f->L = x; else f->R = x;
        x->P = f;
        x->L = x->R = 0;
        byAddressSplay( rootHandle, x );
    }
    return 1;
}

//--------------------------------------------------------------------

void ByAddressDetach( SET_NODE_ *rootHandle, SET_NODE_ x )

{
    SET_NODE_ p = x->P;
    if ( ! x->L )
    {
        if ( p ) { if ( p->L == x ) p->L = x->R; else p->R = x->R; }
        else *rootHandle = x->R;
        if ( x->R ) x->R->P = p;
    }
    else if ( ! x->R )
    {
        if ( p ) { if ( p->L == x ) p->L = x->L; else p->R = x->L; }
        else *rootHandle = x->L;
        if ( x->L ) x->L->P = p;
    }
    else
    {
        SET_NODE_ e = x->L;
        if ( e->R )
        {
            do { e = e->R; } while ( e->R );
            if ( e->P->R = e->L ) e->L->P = e->P;
            ( e->L = x->L )->P = e;
        }
        ( e->R = x->R )->P = e;
        if ( e->P = x->P ) { if ( e->P->L == x ) e->P->L = e; else e->P->R = e; }
        else *rootHandle = e;
    }
}

//////////////////////////////////////////////////////////////////////
//
//  Spaces By Size Multiset

//--------------------------------------------------------------------

inline void bySizeRotateL( MULTISET_NODE_ *rootHandle, MULTISET_NODE_ x )

{
    MULTISET_NODE_ y = x->R, p = x->P;
    if ( x->R = y->L ) y->L->P = x;
    if ( ! ( y->P = p ) )
        *rootHandle = y;
    else
    {
        if ( x == p->L ) p->L = y;
        else             p->R = y;
    }
    ( y->L = x )->P = y;
}

//--------------------------------------------------------------------

inline void bySizeRotateR( MULTISET_NODE_ *rootHandle, MULTISET_NODE_ y )

{
    MULTISET_NODE_ x = y->L, p = y->P;
    if ( y->L = x->R ) x->R->P = y;
    if ( ! ( x->P = p ) )
        *rootHandle = x;
    else
    {
        if ( y == p->L ) p->L = x;
        else             p->R = x;
    }
    ( x->R = y )->P = x;
}

//--------------------------------------------------------------------

static void bySizeSplay( MULTISET_NODE_ *rootHandle, MULTISET_NODE_ x )

{
    MULTISET_NODE_ p;
    while ( p = x->P )
    {
        if ( ! p->P )
        {
            if ( p->L == x ) bySizeRotateR( rootHandle, p );
            else             bySizeRotateL( rootHandle, p );
        }
        else
        {
            if ( p == p->P->R )
            {
                if ( p->R == x ) bySizeRotateL( rootHandle, p->P );
                else             bySizeRotateR( rootHandle, p );
                bySizeRotateL( rootHandle, x->P );
            }
            else
            {
                if ( p->L == x ) bySizeRotateR( rootHandle, p->P );
                else             bySizeRotateL( rootHandle, p );
                bySizeRotateR( rootHandle, x->P );
            }
        }
    }
}

//--------------------------------------------------------------------

inline int bySizeSeek( MULTISET_NODE_ from, MULTISET_NODE_ *resultHandle, U8 size )

{
    MULTISET_NODE_ x = from;
    for ( ;; )
    {
        //U8 size1 = ( U8 ) key;
        SPACE_RANGE_ Range = OWNER( SPACE_RANGE, ByNumBytes, x );
        U8 size2 = Range->NumBytes;

        int diff = ( size == size2 ) ? 0 : ( size < size2 ? -1 : 1 );  //  TODO speedup

        if ( diff < 0 )
        {
            if ( ! x->L ) { *resultHandle = x; return -1; }
            x = x->L;
        }
        else
        if ( diff > 0 )
        {
            if ( ! x->R ) { *resultHandle = x; return  1; }
            x = x->R;
        }
        else              { *resultHandle = x; return  0; }
    }
}

//--------------------------------------------------------------------

MULTISET_NODE_ BySizeNext( MULTISET_NODE_ x )

{
    if ( x->R ) { x = x->R; while ( x->L ) x = x->L; return x; }
    MULTISET_NODE_ p = x->P;
    while ( p && x == p->R ) { x = p; p = p->P; }
    return p;
}

//--------------------------------------------------------------------

MULTISET_NODE_ BySizePrev( MULTISET_NODE_ x )

{
    if ( x->L ) { x = x->L; while ( x->R ) x = x->R; return x; }
    MULTISET_NODE_ p = x->P;
    while ( p && x == p->L ) { x = p; p = p->P; }
    return p;
}

//--------------------------------------------------------------------

MULTISET_NODE_ BySizeFirst( MULTISET_NODE_ x )  //  NOTE does not bother to go up P to P!!

{
    if ( x ) while ( x->L ) x = x->L;
    return x;
}

//--------------------------------------------------------------------

MULTISET_NODE_ BySizeLast( MULTISET_NODE_ x )  //  NOTE does not bother to go up P to P!!

{
    if ( x ) while ( x->R ) x = x->R;
    return x;
}

//--------------------------------------------------------------------

MULTISET_NODE_ BySizeFind( MULTISET_NODE_ *rootHandle, U8 size )

{
    if ( ! *rootHandle ) return 0;
    MULTISET_NODE_ x;
    int direction = bySizeSeek( *rootHandle, &x, size );
    bySizeSplay( rootHandle, x );
    return direction?0:x;
}

//--------------------------------------------------------------------

MULTISET_NODE_ BySizeNear( MULTISET_NODE_ *rootHandle, U8 size, NEIGHBOR want )

{
    //  Return a node relative to ( <, <=, ==, >=, or > ) a size.
    if ( ! *rootHandle ) return 0;
    MULTISET_NODE_ x;
    int dir = bySizeSeek( *rootHandle, &x, size );
    if ( ( dir == 0 && want == GT ) || ( dir > 0 && want >= GE ) ) x = BySizeNext( x );
    else
    if ( ( dir == 0 && want == LT ) || ( dir < 0 && want <= LE ) ) x = BySizePrev( x );
    else
    if ( dir != 0 && want == EQ ) x = 0;
    if ( x ) bySizeSplay( rootHandle, x );
    return x;
}

//--------------------------------------------------------------------

void BySizeAttach( MULTISET_NODE_ *rootHandle, MULTISET_NODE_ x, U8 size )

{
    if ( ! *rootHandle )
    {
        *rootHandle = x;
        x->P = x->L = x->R = x->E = 0;
    }
    else
    {
        MULTISET_NODE_ f;
        int diff = bySizeSeek( *rootHandle, &f, size );
        if ( ! diff )
        {
            if ( x->L = f->L ) x->L->P = x;
            if ( x->R = f->R ) x->R->P = x;
            MULTISET_NODE_ p = f->P;
            if ( x->P = p ) { if ( p->L == f ) p->L = x; else p->R = x; }
            else *rootHandle = x;
            ( x->E = f )->P = x;
            f->L = f->R = 0;
        }
        else
        {
            if ( diff < 0 ) f->L = x; else f->R = x;
            x->P = f;
            x->L = x->R = x->E = 0;
        }
        bySizeSplay( rootHandle, x );
    }
}

//--------------------------------------------------------------------

void BySizeDetach( MULTISET_NODE_ *rootHandle, MULTISET_NODE_ x )

{
    MULTISET_NODE_ e = x->E, p = x->P;
    if ( p && p->E == x ) { if ( p->E = e ) e->P = p; }
    else if ( e )
    {
        if ( e->L = x->L ) e->L->P = e;
        if ( e->R = x->R ) e->R->P = e;
        if ( e->P = p ) { if ( p->L == x ) p->L = e; else p->R = e; }
        else *rootHandle = e;
    }
    else if ( ! x->L )
    {
        if ( p ) { if ( p->L == x ) p->L = x->R; else p->R = x->R; }
        else *rootHandle = x->R;
        if ( x->R ) x->R->P = p;
    }
    else if ( ! x->R )
    {
        if ( p ) { if ( p->L == x ) p->L = x->L; else p->R = x->L; }
        else *rootHandle = x->L;
        if ( x->L ) x->L->P = p;
    }
    else
    {
        e = x->L;
        if ( e->R )
        {
            do { e = e->R; } while ( e->R );
            if ( e->P->R = e->L ) e->L->P = e->P;
            ( e->L = x->L )->P = e;
        }
        ( e->R = x->R )->P = e;
        if ( e->P = x->P ) { if ( e->P->L == x ) e->P->L = e; else e->P->R = e; }
        else *rootHandle = e;
    }
}

//////////////////////////////////////////////////////////////////////

#pragma warning(default : 4706)  //  (ex) if (a = b)

//////////////////////////////////////////////////////////////////////

NTSTATUS spaceAllocateAndAttach( U8 AllocateAddress, U4 AllocateNumBytes )

{
    SPACE_RANGE_ Range = AllocateMemory( sizeof( SPACE_RANGE ) );
    if ( ! Range ) return STATUS_INSUFFICIENT_RESOURCES;
    Range->VolumeAddress = AllocateAddress;
    Range->NumBytes = AllocateNumBytes;

    Volume_SpaceNumNodes++;
    Volume_SpaceNumBytes += AllocateNumBytes;

    BySizeAttach(    &Volume_SpaceByNumBytes , &Range->ByNumBytes      , AllocateNumBytes );
    ByAddressAttach( &Volume_SpaceByAddress  , &Range->ByVolumeAddress , AllocateAddress  );

    return 0;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS spaceDetachAndFree( SPACE_RANGE_ Range )

{
    Volume_SpaceNumNodes--;
    Volume_SpaceNumBytes -= Range->NumBytes;

    BySizeDetach(    &Volume_SpaceByNumBytes , &Range->ByNumBytes );
    ByAddressDetach( &Volume_SpaceByAddress  , &Range->ByVolumeAddress );

    FreeMemory( Range );

    return 0;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS SpaceStartup()

{
ASSERT( ! Volume_SpaceByNumBytes );
ASSERT( ! Volume_SpaceByAddress  );
ASSERT( ! Volume_SpaceNumNodes   );

    Volume_SpaceNumBytes = 0;

    U8 VolumeAddress = Volume_DataStart;  //  Skip the volume header.
    while ( VolumeAddress < Volume_TotalNumBytes )
    {
        U8 VolumeNumBytes = min( Volume_SpaceNodeMaxNumBytes, Volume_TotalNumBytes - VolumeAddress );

        NTSTATUS Status = spaceAllocateAndAttach( VolumeAddress, ( U4 ) VolumeNumBytes );
ASSERT( ! Status );

        VolumeAddress += VolumeNumBytes;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS SpaceRequestNumBytes( U4 NumBytesRequested, U8 * VolumeAddressResult, U4 * VolumeNumBytesResult )

{
    NTSTATUS Status;

    NumBytesRequested = ROUND_UP( NumBytesRequested, Volume_BlockSize );

    //  Can we find an available range as big or bigger than we want?
    MULTISET_NODE_ m = BySizeNear( &Volume_SpaceByNumBytes, NumBytesRequested, GE );

    //  If not, we should return as big a range as we can.
    if ( ! m )
    {
        m = BySizeNear( &Volume_SpaceByNumBytes, NumBytesRequested, LT );
        if ( ! m )
        {
            return STATUS_INSUFFICIENT_RESOURCES;  //  We have nothing available.
        }
    }

    Volume_SpaceNumDifferencesFromVolume++;

    //  If the range is not big enough to split off the excess, return the whole thing.
    SPACE_RANGE_ Range = OWNER( SPACE_RANGE, ByNumBytes, m );
    S4 Leftover = Range->NumBytes - NumBytesRequested;
    if ( Leftover < 0 || ( U4 ) Leftover < Volume_BlockSize )
    {
        *VolumeAddressResult  = Range->VolumeAddress;
        *VolumeNumBytesResult = Range->NumBytes;
        spaceDetachAndFree( Range );
        return 0;
    }


    //  Return the front of this range.
    *VolumeAddressResult  = Range->VolumeAddress;
    *VolumeNumBytesResult = NumBytesRequested;


    //  Split off the back of this range.
    U8 A = Range->VolumeAddress  + NumBytesRequested;
    U4 N = Range->NumBytes - NumBytesRequested;
    Status = spaceDetachAndFree( Range );
    Status = spaceAllocateAndAttach( A, N );
	
    return 0;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS SpaceReturnAddressRange( U8 ReturnAddress, U4 ReturnNumBytes )

{
    NTSTATUS Status;
    SET_NODE_ n;
    SPACE_RANGE_ Range;

    //  Can we combine this address range with the previous address range?
    n = ByAddressNear( &Volume_SpaceByAddress, ReturnAddress, LT );  //  Is there a previous?
    if ( n )
    {
        Range = OWNER( SPACE_RANGE, ByVolumeAddress, n );
ASSERT( Range->VolumeAddress + Range->NumBytes <= ReturnAddress );
        if ( Range->VolumeAddress + Range->NumBytes == ReturnAddress )  //  Is it contiguous?
        {
            if ( Range->NumBytes + ReturnNumBytes < Volume_SpaceNodeMaxNumBytes )  //  Would it not be too big?
            {
                //  Account for it and get rid of it.
                ReturnAddress   = Range->VolumeAddress;
                ReturnNumBytes += Range->NumBytes;
                Status = spaceDetachAndFree( Range );
ASSERT( ! Status );
            }
        }
    }

    Volume_SpaceNumDifferencesFromVolume++;

    //  Can we combine this address range with the next address range?
    n = ByAddressNear( &Volume_SpaceByAddress, ReturnAddress, GT );  //  Is there a next?
    if ( n )
    {
        Range = OWNER( SPACE_RANGE, ByVolumeAddress, n );
        if ( ReturnAddress + ReturnNumBytes == Range->VolumeAddress )  //  Is it contiguous?
        {
            if ( ReturnNumBytes + Range->NumBytes < Volume_SpaceNodeMaxNumBytes )  //  Would it not be too big?
            {
                ReturnNumBytes += Range->NumBytes;
                Status = spaceDetachAndFree( Range );
ASSERT( ! Status );
            }
        }
    }

    //  Attach this address range.
    Status = spaceAllocateAndAttach( ReturnAddress, ReturnNumBytes );
ASSERT( ! Status );
	
    return 0;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS SpaceShutdown()

{
    NTSTATUS Status;

    SPACE_RANGE_ Range;
    for (;;)
    {
        MULTISET_NODE_ Node = BySizeFirst( Volume_SpaceByNumBytes );
        if ( ! Node ) break;

        while ( Node->E )
        {
            MULTISET_NODE_ EqualNode = Node->E;
            Range = OWNER( SPACE_RANGE, ByNumBytes, EqualNode );

            Status = spaceDetachAndFree( Range );
ASSERT( ! Status );

        }

        Range = OWNER( SPACE_RANGE, ByNumBytes, Node );
        Status = spaceDetachAndFree( Range );
ASSERT( ! Status );

    }



ASSERT( Volume_SpaceByNumBytes == 0 );  //  TODO temp
ASSERT( Volume_SpaceByAddress == 0 );  //  TODO temp
ASSERT( Volume_SpaceNumNodes == 0 );  //  TODO temp

    Volume_SpaceByNumBytes = 0;
    Volume_SpaceByAddress  = 0;

    return 0;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS spaceRemoveAddressRange( U8 RemoveAddress, U4 RemoveNumBytes )

{
    NTSTATUS Status;
    SET_NODE_ n;
    SPACE_RANGE_ Range;


    //  Find the applicable existing range.
    n = ByAddressNear( &Volume_SpaceByAddress, RemoveAddress, LE );
ASSERT( n );
    Range = OWNER( SPACE_RANGE, ByVolumeAddress, n );
    U8 A = Range->VolumeAddress;
    U4 N = Range->NumBytes;


    //  Since it will change, we know we will be taking it out.
    Status = spaceDetachAndFree( Range );
ASSERT( ! Status );


    //  If we share a beginning with this range...
    if ( A == RemoveAddress )
    {
        //  If we are exactly this range, we are done.
        if ( N == RemoveNumBytes )
        {
            return Status;
        }
ASSERT( N > RemoveNumBytes );
        Status = spaceAllocateAndAttach( A + RemoveNumBytes, N - RemoveNumBytes );
ASSERT( ! Status );
        return Status;
    }


    //  If we share an end with this range...
    if ( A + N == RemoveAddress + RemoveNumBytes )
    {
ASSERT( A < RemoveAddress );
        Status = spaceAllocateAndAttach( A, N - RemoveNumBytes );
ASSERT( ! Status );
        return Status;
    }


    //  We must be in the interior; split this range around us.
    Status = spaceAllocateAndAttach( A, ( U4 ) ( RemoveAddress - A ) );
ASSERT( ! Status );
    Status = spaceAllocateAndAttach( RemoveAddress + RemoveNumBytes, ( U4 ) ( A + N - RemoveAddress - RemoveNumBytes ) );
ASSERT( ! Status );

    return Status;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS SpaceBuildFromEntries()  //  Build the Space sets based on the existing file entries.

{
    NTSTATUS Status;

AlwaysLogString( "Building.\n" );
UINT64 msFm = CurrentMillisecond();

    //  For every Entry...
    U1_ Fm = EntriesBytes;
    for (;;)
    {

        if ( Fm == EntriesBytes + EntriesFirstFreeByte ) break;

        if ( ( ( U4_ ) Fm ) [0] )
        {
            //  An existing Entry.
            ENTRY_ Entry = ( ENTRY_ ) Fm;
            U4 Size = EntrySize( Entry );
            if ( EntryIsAFile( Entry ) )
            {
                FILE_DATA_ FileData = Data( Entry );

                //  For each range...
                for ( U4 i = 0; i < FileData->NumRanges; i++ )
                {
                    DATA_RANGE_ DataRange = &FileData->DataRange[i];
                    U8 V = DataRange->VolumeAddress;
                    U4 N = DataRange->NumBytes;
                    if ( V )
                    {
LogFormatted( " Removing %p %X from %s\n", ( V_ ) V, N, Entry->Name );
                        Status = spaceRemoveAddressRange( V, N );
                        if ( Status ) return Status;
                    }
                }
            }
            Fm += Size;
        }
        else
        {
            //  A freed Entry.
            U4 Size = ( ( U4_ ) Fm ) [1];
            Fm += Size;
        }
    }

UINT64 msTo = CurrentMillisecond();
AlwaysLogFormatted( "Built in %d ms.\n", ( int ) ( msTo - msFm ) );

    return 0;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


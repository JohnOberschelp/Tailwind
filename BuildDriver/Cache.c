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

struct _CACHE_RANGE
{
    CACHE_RANGE_ P, L, R;   //  Parent, Left, Right
    LINK        Link;
    U8   VolumeAddress;
    U1_  MemoryAddress;
    U4   NumBytes;
    B1   IsDirty;
};

//--------------------------------------------------------------------

struct _CACHE
{
    CHAIN        Age;
    CACHE_RANGE_ SetRoot;
    U8           TotalNumDirtyBytes;
    U4           TotalNumDirtyRanges;
    U8           TotalNumCleanBytes;
    U4           TotalNumCleanRanges;
};

//////////////////////////////////////////////////////////////////////

#pragma warning( disable : 4706 )  //  assignment within conditional ( ex ) if ( a = b )


CACHE    Cache;

SPINLOCK CacheLock;

//////////////////////////////////////////////////////////////////////
//
//  CacheRangesSet

//--------------------------------------------------------------------

inline void cacheRangesSetRotateL( CACHE_RANGE_ *rootHandle, CACHE_RANGE_ x )

{
    CACHE_RANGE_ y = x->R, p = x->P;
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

inline void cacheRangesSetRotateR( CACHE_RANGE_ *rootHandle, CACHE_RANGE_ y )

{
    CACHE_RANGE_ x = y->L, p = y->P;
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

static void cacheRangesSetSplay( CACHE_RANGE_ *rootHandle, CACHE_RANGE_ x )

{
    CACHE_RANGE_ p;
    while ( p = x->P )
    {
        if ( ! p->P )
        {
            if ( p->L == x ) cacheRangesSetRotateR( rootHandle, p );
            else             cacheRangesSetRotateL( rootHandle, p );
        }
        else
        {
            if ( p == p->P->R )
            {
                if ( p->R == x ) cacheRangesSetRotateL( rootHandle, p->P );
                else             cacheRangesSetRotateR( rootHandle, p );
                cacheRangesSetRotateL( rootHandle, x->P );
            }
            else
            {
                if ( p->L == x ) cacheRangesSetRotateR( rootHandle, p->P );
                else             cacheRangesSetRotateL( rootHandle, p );
                cacheRangesSetRotateR( rootHandle, x->P );
            }
        }
    }
}

//--------------------------------------------------------------------

inline int cacheRangesSetSeek( CACHE_RANGE_ from, CACHE_RANGE_ *resultHandle, U8 VolumeAddress )

{
    CACHE_RANGE_ x = from;
    for ( ;; )
    {
        S8 diff = VolumeAddress - x->VolumeAddress;
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

CACHE_RANGE_ cacheRangesSetNext( CACHE_RANGE_ x )

{
    if ( x->R ) { x = x->R; while ( x->L ) x = x->L; return x; }
    CACHE_RANGE_ p = x->P;
    while ( p && x == p->R ) { x = p; p = p->P; }
    return p;
}

//--------------------------------------------------------------------

CACHE_RANGE_ cacheRangesSetPrev( CACHE_RANGE_ x )

{
    if ( x->L ) { x = x->L; while ( x->R ) x = x->R; return x; }
    CACHE_RANGE_ p = x->P;
    while ( p && x == p->L ) { x = p; p = p->P; }
    return p;
}

//--------------------------------------------------------------------

CACHE_RANGE_ cacheRangesSetFirst( CACHE_RANGE_ x )  //  NOTE does not bother to go up P to P!!

{
    if ( x ) while ( x->L ) x = x->L;
    return x;
}

//--------------------------------------------------------------------

CACHE_RANGE_ cacheRangesSetLast( CACHE_RANGE_ x )  //  NOTE does not bother to go up P to P!!

{
    if ( x ) while ( x->R ) x = x->R;
    return x;
}

//--------------------------------------------------------------------

CACHE_RANGE_ cacheRangesFind( CACHE_RANGE_ *rootHandle, U8 VolumeAddress )

{
    if ( ! *rootHandle ) return 0;
    CACHE_RANGE_ x;
    int direction = cacheRangesSetSeek( *rootHandle, &x, VolumeAddress );
    cacheRangesSetSplay( rootHandle, x );
    return direction?0:x;
}

//--------------------------------------------------------------------

CACHE_RANGE_ cacheRangesNear( CACHE_RANGE_ *rootHandle, U8 VolumeAddress, NEIGHBOR want )

{
    //  Return a node relative to ( <, <=, ==, >=, or > ) a key.
    if ( ! *rootHandle ) return 0;
    CACHE_RANGE_ x;
    int dir = cacheRangesSetSeek( *rootHandle, &x, VolumeAddress );
    if ( ( dir == 0 && want == GT ) || ( dir > 0 && want >= GE ) ) x = cacheRangesSetNext( x );
    else
    if ( ( dir == 0 && want == LT ) || ( dir < 0 && want <= LE ) ) x = cacheRangesSetPrev( x );
    else
    if ( dir != 0 && want == EQ ) x = 0;
    if ( x ) cacheRangesSetSplay( rootHandle, x );
    return x;
}

//--------------------------------------------------------------------

int cacheRangesAttach( CACHE_RANGE_ *rootHandle, CACHE_RANGE_ x )

{
    if ( ! *rootHandle )
    {
        *rootHandle = x;
        x->P = x->L = x->R = 0;
    }
    else
    {
        CACHE_RANGE_ f;
        int diff = cacheRangesSetSeek( *rootHandle, &f, x->VolumeAddress );
        if ( ! diff ) return 0;
        if ( diff < 0 ) f->L = x; else f->R = x;
        x->P = f;
        x->L = x->R = 0;
        cacheRangesSetSplay( rootHandle, x );
    }
    return 1;
}

//--------------------------------------------------------------------

void cacheRangesDetach( CACHE_RANGE_ *rootHandle, CACHE_RANGE_ x )

{
    CACHE_RANGE_ p = x->P;
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
        CACHE_RANGE_ e = x->L;
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

NTSTATUS CacheStartup()

{

ASSERT( ! Cache.Age.First );
ASSERT( ! Cache.Age.Last  );
ASSERT( ! Cache.SetRoot   );
ASSERT( ! Cache.TotalNumDirtyRanges );
ASSERT( ! Cache.TotalNumDirtyBytes  );

    Zero( &Cache, sizeof( CACHE ) );

    InitializeSpinlock( &CacheLock );

    return 0;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS CacheShutdown()

{
    if ( Cache.TotalNumDirtyRanges )
    {
        for ( int i = 0; i < 5; i++ ) AlwaysLogString( "Need to have no dirty file here!!!\n" );
ASSERT( 0 );
    }

    UninitializeSpinlock( &CacheLock );

    return 0;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS cacheRangeMake( U8 VolumeAddress, U1_ B, U4 NumBytes, CLEAN_OR_DIRTY CleanOrDirty, U4 WithinRangeOffset, U4 WithinRangeNumBytes )

{


//  TODO need to deal with this range does not fit with existing ranges, or overlaps or anything. clean vs dirty


//  For now, just cry out if not simple.
CACHE_RANGE_ lt = cacheRangesNear( &Cache.SetRoot, VolumeAddress, LT );
CACHE_RANGE_ eq = cacheRangesFind( &Cache.SetRoot, VolumeAddress     );
CACHE_RANGE_ gt = cacheRangesNear( &Cache.SetRoot, VolumeAddress, GT );
ASSERT( ! lt || lt->VolumeAddress + lt->NumBytes <= VolumeAddress );  //  No overlap with previous.
ASSERT( ! eq );                                                       //  Doesn't already exist.
ASSERT( ! gt || VolumeAddress + NumBytes <= gt->VolumeAddress );      //  No overlap with next.


ASSERT( VolumeAddress );

    CACHE_RANGE_ CacheRange = AllocateAndZeroMemory( sizeof( CACHE_RANGE ) );
ASSERT( CacheRange );
    if ( ! CacheRange ) return STATUS_INSUFFICIENT_RESOURCES;

    U1_ MemoryAddress = AllocateAndZeroMemory( NumBytes );
ASSERT( MemoryAddress );
    if ( ! MemoryAddress )
    {
        FreeMemory( CacheRange );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    CacheRange->VolumeAddress = VolumeAddress;
    CacheRange->MemoryAddress = MemoryAddress;
    CacheRange->NumBytes      = NumBytes;

    int OK = cacheRangesAttach( &Cache.SetRoot, CacheRange );
ASSERT( OK );
    AttachLinkLast( &Cache.Age, &CacheRange->Link );


    if ( B ) memcpy( MemoryAddress + WithinRangeOffset, B, WithinRangeNumBytes );
    else     Zero(   MemoryAddress + WithinRangeOffset,    WithinRangeNumBytes );


    if ( CleanOrDirty == CLEAN )
    {
        CacheRange->IsDirty = FALSE;
        Cache.TotalNumCleanBytes  += NumBytes;
        Cache.TotalNumCleanRanges += 1;
    }
    else
    {
        CacheRange->IsDirty = TRUE;
        Cache.TotalNumDirtyBytes  += NumBytes;
        Cache.TotalNumDirtyRanges += 1;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////

//  TODO should we be looking for and removing cache when we remove a file range?

void cacheRangeUnmake( U8 VolumeAddress )

{
    CACHE_RANGE_ CacheRange = cacheRangesFind( &Cache.SetRoot, VolumeAddress );
ASSERT( CacheRange );


    if ( CacheRange->IsDirty )
    {
        Cache.TotalNumDirtyBytes  -= CacheRange->NumBytes;
        Cache.TotalNumDirtyRanges -= 1;
    }
    else
    {
        Cache.TotalNumCleanBytes  -= CacheRange->NumBytes;
        Cache.TotalNumCleanRanges -= 1;
    }


    DetachLink(  &Cache.Age, &CacheRange->Link );
    cacheRangesDetach( &Cache.SetRoot, CacheRange );
    FreeMemory( CacheRange->MemoryAddress );
    FreeMemory( CacheRange );
}

//////////////////////////////////////////////////////////////////////

NTSTATUS CacheRangeMakeWithLock( U8 VolumeAddress, U1_ B, U4 NumBytes, CLEAN_OR_DIRTY CleanOrDirty, U4 WithinRangeOffset, U4 WithinRangeNumBytes )

{
    AcquireSpinlock( &CacheLock );

    NTSTATUS Status = cacheRangeMake( VolumeAddress, B, NumBytes, CleanOrDirty, WithinRangeOffset, WithinRangeNumBytes );

    ReleaseSpinlock( &CacheLock );

    return Status;

}
//////////////////////////////////////////////////////////////////////

NTSTATUS CacheBlindlyThrowAwayAll()

{
    AcquireSpinlock( &CacheLock );

    while ( Cache.Age.First )
    {
        LINK_ Link = Cache.Age.First;
        CACHE_RANGE_ CacheRange = OWNER( CACHE_RANGE, Link, Link );

        cacheRangeUnmake( CacheRange->VolumeAddress );
    }

    ReleaseSpinlock( &CacheLock );

    return 0;
}

//////////////////////////////////////////////////////////////////////

void FindFirstUncachedRange( ENTRY_ Entry, U8_ A_, U4_ N_ )

{
    AcquireSpinlock( &CacheLock );

    FILE_DATA_ FileData  = Data( Entry );
ASSERT( FileData );

    for ( U4 i = 0; i < FileData->NumRanges; i++ )
    {
        DATA_RANGE_ DataRange = &FileData->DataRange[i];

        CACHE_RANGE_ CacheRange = cacheRangesFind( &Cache.SetRoot, DataRange->VolumeAddress );
        if ( ! CacheRange )
        {
            *A_ = DataRange->VolumeAddress;
            *N_ = DataRange->NumBytes;
            ReleaseSpinlock( &CacheLock );
            return;
        }
    }

    *A_ = 0;
    ReleaseSpinlock( &CacheLock );
    return;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS AccessCacheForFile( ENTRY_ Entry, DIRECTION Direction, U1_ CallerBuffer, U8 CallerFileOffset, U4 CallerNumBytes )

{
    NTSTATUS Status;

    AcquireSpinlock( &CacheLock );

    FILE_DATA_ FileData = Data( Entry );

    if ( CallerFileOffset + CallerNumBytes > FileData->AllocationNumBytes )
    {
        ReleaseSpinlock( &CacheLock );
        return STATUS_INVALID_USER_BUFFER;  //  TODO status or buffer overflow warning
    }
    U8 CurrentFileOffsetAt = CallerFileOffset;
    U4 CurrentNumBytesLeft = CallerNumBytes;
    U1_ B = CallerBuffer;


    //  Walk the ranges.
    U8 DataRangeFileOffset = 0;

    for ( U4 i = 0; i < FileData->NumRanges; i++ )  //  TODO Speedup? Maybe don't have to go over all ranges?
    {
        if ( CurrentNumBytesLeft == 0 ) break;

        DATA_RANGE_ DataRange = &FileData->DataRange[i];
        U8 DataRangeFileOffsetTo = DataRangeFileOffset + DataRange->NumBytes;
        if ( DataRangeFileOffsetTo > CurrentFileOffsetAt )
        {
            if ( DataRangeFileOffset > CurrentFileOffsetAt + CurrentNumBytesLeft )
            {
AlwaysBreakToDebugger();  //  We won't get here right!?
                break;
            }

            U8 WithinRangeOffset   = max( DataRangeFileOffset   , CurrentFileOffsetAt                       ) - DataRangeFileOffset;
            U8 WithinRangeOffsetTo = min( DataRangeFileOffsetTo , CurrentFileOffsetAt + CurrentNumBytesLeft ) - DataRangeFileOffset;
            U4 WithinRangeNumBytes = ( U4 ) ( WithinRangeOffsetTo - WithinRangeOffset );


            if ( WithinRangeOffset < WithinRangeOffsetTo )
            {

                //  Find the corresponding cache, if any, and deal with it..
                U8 VolumeAddress = DataRange->VolumeAddress;

                CACHE_RANGE_ CacheRange = cacheRangesFind( &Cache.SetRoot, VolumeAddress );
                if ( CacheRange )
                {
ASSERT( CacheRange->NumBytes == DataRange->NumBytes );
                    switch( Direction )
                    {

                      case OUT_OF_CACHE:
                        if ( B ) memcpy( B, CacheRange->MemoryAddress + WithinRangeOffset, WithinRangeNumBytes );
                        break;

                      case INTO_CACHE:
                        if ( B ) memcpy( CacheRange->MemoryAddress + WithinRangeOffset, B, WithinRangeNumBytes );
                        else     Zero(   CacheRange->MemoryAddress + WithinRangeOffset,    WithinRangeNumBytes );
                        if ( ! CacheRange->IsDirty )
                        {
                            CacheRange->IsDirty = TRUE;
                            Cache.TotalNumCleanBytes  -= CacheRange->NumBytes;
                            Cache.TotalNumCleanRanges -= 1;
                            Cache.TotalNumDirtyBytes  += CacheRange->NumBytes;
                            Cache.TotalNumDirtyRanges += 1;
                        }
                        break;
                    }
                }
                else
                {
                    //  The cache range does not exist.

                    switch( Direction )
                    {

                      case OUT_OF_CACHE:
AlwaysLogFormatted( "!!!!!!!!!!!!!!STATUS_PENDING; NEED CACHE FILL on %p %8X\n", ( V_ ) VolumeAddress, DataRange->NumBytes );
                        ReleaseSpinlock( &CacheLock );

                        return STATUS_PENDING;

                      case INTO_CACHE:

                        Status = cacheRangeMake( VolumeAddress, B, DataRange->NumBytes, DIRTY, ( U4 ) WithinRangeOffset, WithinRangeNumBytes );
ASSERT( ! Status );
                        break;

                    }
                }



                CurrentFileOffsetAt += WithinRangeNumBytes;
                CurrentNumBytesLeft -= WithinRangeNumBytes;
                if ( B ) B += WithinRangeNumBytes;

            }
            else
            {
AlwaysBreakToDebugger();  //  We won't get here right!?
            }

        }
        DataRangeFileOffset = DataRangeFileOffsetTo;
    }

    ReleaseSpinlock( &CacheLock );

    return 0;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS CacheReport( S1_ Buffer, int MaxNumBytes )

{

    AcquireSpinlock( &CacheLock );

    S1_ P = Buffer;
    P[0] = 0;
    S1 scratch[256] = {0};  //  TODO should not be necessary to init


    NTSTATUS Status = RtlStringCchPrintfA( scratch, MaxNumBytes,
            "CacheReport %d dirty ranges for %d dirty bytes   %d clean ranges for %d clean bytes",
                    Cache.TotalNumDirtyRanges,
            ( int ) Cache.TotalNumDirtyBytes,
                    Cache.TotalNumCleanRanges,
            ( int ) Cache.TotalNumCleanBytes );

    if ( ! Status ) strcat( P, scratch );
    else            strcat( P, "(oops)" );

    ReleaseSpinlock( &CacheLock );

    return 0;
}

//////////////////////////////////////////////////////////////////////

U4 CacheBackgroundWriteDirtiestToVolume()

{

//AlwaysLogString( "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ CacheBackgroundWriteDirtiestToVolume got in.\n" );

    NTSTATUS Status;

    if ( ! Cache.TotalNumDirtyRanges ) return 0;

    AcquireSpinlock( &CacheLock );

    U4 NumBytesWritten = 0;

    //  Find the first dirty range.
    LINK_ Link = Cache.Age.First;
    while ( Link )
    {

        CACHE_RANGE_ CacheRange = OWNER( CACHE_RANGE, Link, Link );
        if ( CacheRange->IsDirty )
        {

            //  If the range has not yet been assigned a volume address, get one now.
            //  TODO what if the room isnt avail? or not contiguous?
            //  TODO could be a place to combine ranges that are not assigned volume addresses yet. beware of changes to numdirty and numranges
            if ( ! CacheRange->VolumeAddress )
            {
AlwaysBreakToDebugger();

                U8 VolumeAddress;
                U4 NumBytesGot;
                U4 NumBytesRequested = CacheRange->NumBytes;
                Status = SpaceRequestNumBytes( NumBytesRequested, &VolumeAddress, &NumBytesGot );
ASSERT( ! Status );
ASSERT( NumBytesRequested == NumBytesGot );
                if ( Status )
                {
                    ReleaseSpinlock( &CacheLock );
                    return 0;
                }
                CacheRange->VolumeAddress = VolumeAddress;
            }


            //  TODO TODO TODOTODO
            //  Say that we have already written the range by marking it as not dirty, even though, at this point, we are
            //  going to copy the buffer, and RELEASE the meta spinlock while we wait for the volume write.
            //  Obviously (though extremely unlikely), the write could fail and this needs to be delt with.


            U8  Offset = CacheRange->VolumeAddress;
            U4  Length = CacheRange->NumBytes;
            U1_ Buffer = CacheRange->MemoryAddress;


            CacheRange->IsDirty = FALSE;
            Cache.TotalNumCleanBytes  += Length;
            Cache.TotalNumCleanRanges += 1;
            Cache.TotalNumDirtyBytes  -= Length;
            Cache.TotalNumDirtyRanges -= 1;


            PDEVICE_OBJECT DeviceObject = Volume_PhysicalDeviceObject;

            U1_ MetadatalessBuffer = AllocateMemory( Length );
ASSERT( MetadatalessBuffer );

            memcpy( MetadatalessBuffer, Buffer, Length );

            ReleaseSpinlock( &CacheLock );

            Status = WriteBlockDevice( DeviceObject, Offset, Length, MetadatalessBuffer, NO_VERIFY );
ASSERT( ! Status ); //got $8000'0016 STATUS_VERIFY_REQUIRED when no verify override
LogFormatted( "V o l u m e W r i t i n g   %p for %X \n", ( V_ ) Offset, Length );

            FreeMemory( MetadatalessBuffer );

            NumBytesWritten += CacheRange->NumBytes;

            return NumBytesWritten;  //  Just do 1 for now?
        }

        Link = Link->Next;

    }

    ReleaseSpinlock( &CacheLock );

//LogFormatted( "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ CacheBackgroundWriteDirtiestToVolume wrote %d bytes.\n", NumBytesWritten );

    return 0;//NumBytesWritten;
}

//////////////////////////////////////////////////////////////////////

U4 CacheFreeSomeCache()

{

AlwaysLogString( "//////////////////////////////////////// CacheFreeSomeCache got in.\n" );

    AcquireSpinlock( &CacheLock );

    U4 NumBytesUncached = 0;

    //  Free the first clean range.
    LINK_ Link = Cache.Age.First;
    while ( Link )
    {
        CACHE_RANGE_      CacheRange = OWNER( CACHE_RANGE, Link, Link );

        if ( ( ! CacheRange->IsDirty ) && CacheRange->MemoryAddress )
        {
ASSERT ( CacheRange->VolumeAddress );
ASSERT ( CacheRange->MemoryAddress );
            NumBytesUncached = CacheRange->NumBytes;

AlwaysLogFormatted( "//////////////////////////////////////// CacheFreeSomeCache freed                    %d bytes.\n",
CacheRange->NumBytes );

            cacheRangeUnmake( CacheRange->VolumeAddress );

            ReleaseSpinlock( &CacheLock );

            return NumBytesUncached;
        }

        Link = Link->Next;
    }

    ReleaseSpinlock( &CacheLock );

    return 0;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


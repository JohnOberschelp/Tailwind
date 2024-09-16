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

volatile B1 BackgroundThread_STOP    = FALSE;  //  TODO need one thread per volume
volatile B1 BackgroundThread_STOPPED = FALSE;  //  TODO need one thread per volume
volatile B1 BackgroundThread_Busy    = TRUE;   //  TODO need one thread per volume

//////////////////////////////////////////////////////////////////////

B1 RedispatchACompletablePendingReadIrp()

{
    LINK_ Link = Volume_PendingReadsChain.First;
    while ( Link )
    {
        ICB_         Icb        = OWNER( ICB, PendingReadsLink, Link );
        PIRP         Irp        = Icb->Irp;
        IRPSP_       IrpSp      = IoGetCurrentIrpStackLocation( Irp );
        PFILE_OBJECT FileObject = IrpSp->FileObject;
        FCB_         Fcb        = FileObject->FsContext;
        ID           Id         = Fcb->Id;
        ENTRY_       Entry      = Entries[ Id ];
        U8           A          = 0;
        U4           N          = 0;

        FindFirstUncachedRange( Entry, &A, &N );  //  TODO don't need to cache the whole file.
        if ( ! A )
        {
            //  We can complete it; it is a read with all data in cache.
AlwaysLogString( "!!!!!!!!!!!!!!WE CAN COMPLETE A PENDING IRP_MJ_READ\n" );

            DetachLink( &Volume_PendingReadsChain, Link );

            B1 Acquired = AcquireSpinlockOrFail( &Volume_MetadataLock );
            if ( ! Acquired ) return FALSE;

AlwaysLogString( "+( redispatch )\n" );

            FsRtlEnterFileSystem();
            //IoSetTopLevelIrp( Icb->Irp );
            //IoSetTopLevelIrp( ( PIRP ) FSRTL_FSP_TOP_LEVEL_IRP );

AlwaysLogString( "1( redispatch )\n" );

            NTSTATUS Status = IrpMj( Icb );
ASSERT( Status != STATUS_PENDING );
            Irp->IoStatus.Status = Status;
            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            UnmakeIcb( Icb );

AlwaysLogString( "2( redispatch )\n" );

            ReleaseSpinlock( &Volume_MetadataLock );

            //IoSetTopLevelIrp( 0 );  //  TODO need? nBecause recursive?
            FsRtlExitFileSystem();

AlwaysLogString( "- ( redispatch )\n" );

            return TRUE;
        }
        Link = Link->Next;
    }

    ReleaseSpinlock( &Volume_MetadataLock );

    return FALSE;
}

//////////////////////////////////////////////////////////////////////

B1 MakeSomeNeededCacheForAPendingReadIrp()

{
    LINK_ Link = Volume_PendingReadsChain.First;
    while ( Link )
    {
        ICB_         Icb        = OWNER( ICB, PendingReadsLink, Link );
        PIRP         Irp        = Icb->Irp;
        IRPSP_       IrpSp      = IoGetCurrentIrpStackLocation( Irp );
        PFILE_OBJECT FileObject = IrpSp->FileObject;
        FCB_         Fcb        = FileObject->FsContext;
        ID           Id         = Fcb->Id;
        ENTRY_       Entry      = Entries[ Id ];
        U8           A          = 0;
        U4           N          = 0;

        FindFirstUncachedRange( Entry, &A, &N );  //  TODO don't need to cache the whole file.
        if ( A )
        {

AlwaysLogFormatted( "!!!!!!!!!!!!!!BACKGROUND READ INTO CACHE INITIATED %p %X\n", ( V_ ) A, N );

            //  Allocate a buffer.
            U1_ B = AllocateMemory( N );  //  TODO speedup
            if ( ! B ) return FALSE;

            //  Read in the buffer from the volume.
            NTSTATUS Status2 = ReadBlockDevice( Volume_PhysicalDeviceObject, A, N, B, NO_VERIFY );

AlwaysLogFormatted( "!!!!!!!!!!!!!!BACKGROUND READ INTO CACHE got status %X\n", Status2 );
ASSERT( Status2 == 0 );

            //  Create the cache range.
            NTSTATUS Status = CacheRangeMakeWithLock( A, B, N, CLEAN, 0, N );
ASSERT( ! Status );

            FreeMemory( B );
            return TRUE;
        }
        Link = Link->Next;
    }
    return FALSE;
}

//////////////////////////////////////////////////////////////////////

void BackgroundThread( PVOID context )

{
    VCB_     Vcb = context;
    B1       Acquired;
    B1       Did;
    NTSTATUS Status;

    BackgroundThread_Busy = TRUE;

    U8 LastMetaWriteMillisecond = CurrentMillisecond();

    for ( int BackgroundPass = 1 ; ; BackgroundPass++ )
    {
        if ( BackgroundThread_STOP ) break;
        U8 CurrentMicrosecond2 = CurrentMicrosecond();



        //  Let debuggers know we are alive.
        if ( BackgroundPass <= 5 )
        {
            AlwaysLogFormatted( "Print %d of 5 : Hi World.\n", BackgroundPass );
        }



        //
        //  Priority 0: re-dispatch a pending read that has all data in cache.
        //
        Did = RedispatchACompletablePendingReadIrp();
        if ( Did )
        {
            BackgroundThread_Busy = TRUE;
            continue;
        }



        //
        //  Priority 1: Cache some of the volume towards completing a read irp.
        //
        Did = MakeSomeNeededCacheForAPendingReadIrp();
        if ( Did )
        {
            BackgroundThread_Busy = TRUE;
            continue;
        }



        //
        //  Priority ?: help a pending lock.
        //
ASSERT( Volume_PendingLocksChain.First == 0 );



        //
        //  Priority ?: if "needed", remove some clean cache.
        //
        U8 NumCleanBytes = 100'000'000;  //  TODO compute
        if ( NumCleanBytes > 200'000'000 )
        {
            Acquired = AcquireSpinlockOrFail( &Volume_MetadataLock );
            if ( Acquired )
            {
AlwaysBreakToDebugger();
AlwaysLogString( "Free some cache?\n" );
                U4 NumBytesFreed = CacheFreeSomeCache();
                ReleaseSpinlock( &Volume_MetadataLock );
                if ( NumBytesFreed )
                {
                    BackgroundThread_Busy = TRUE;
                    continue;
                }
            }
        }



        //
        //  Priority 2: write some dirty bytes to the volume.
        //
        static U8 MicrosecondOfLastDirtyWrite = 0;
        if ( CurrentMicrosecond2 - MicrosecondOfLastDirtyWrite > 1'000 )
        {
            MicrosecondOfLastDirtyWrite = CurrentMicrosecond2;
            U4 NumBytesWritten = CacheBackgroundWriteDirtiestToVolume();
            if ( NumBytesWritten )
            {
                BackgroundThread_Busy = TRUE;
                continue;
            }
        }



        //
        //  Or possibly write all the metadata?
        //
        U8 Now = CurrentMillisecond();
        if ( ! Volume_LastMetadataOkCount ) Volume_LastMetadataOkCount = Volume_ConservativeMetadataUpdateCount;
        if (    ( Volume_ConservativeMetadataUpdateCount > Volume_LastMetadataOkCount )
             && ( Now - LastMetaWriteMillisecond > 5'000 ) )  //  TODO
        {
            Acquired = AcquireSpinlockOrFail( &Volume_MetadataLock );
            if ( Acquired )
            {
AlwaysLogFormatted( "W R I T I N G   A L L   M E T A D A T A   after irp %d\n", ( int ) Volume_ConservativeMetadataUpdateCount );

                LastMetaWriteMillisecond = Now;

                //  Copy the current Overview.
                U1_ OverviewBuffer = AllocateMemory( 4096 );
ASSERT( OverviewBuffer );
                Status = OverviewFreeze1( OverviewBuffer );
ASSERT( ! Status );

                //  Copy the entries.
                U4 EntriesNumBytes = ROUND_UP( EntriesFirstFreeByte, 4096 );
                U1_ EntriesBuffer = AllocateMemory( EntriesNumBytes );
ASSERT( EntriesBuffer );
                memcpy( EntriesBuffer, EntriesBytes, EntriesNumBytes );

                //  We can free up the metadata now.
                ReleaseSpinlock( &Volume_MetadataLock );

                //  Write the copied Overview, then free the copy.
                Status = OverviewFreeze2( OverviewBuffer );
ASSERT( ! Status );
                FreeMemory( OverviewBuffer );

                //  Write the copied entries, the free the copy.
                Status = EntriesFreeze( EntriesBytes, EntriesNumBytes );
ASSERT( ! Status );
                FreeMemory( EntriesBuffer );

                //  Mark the location so we notice this info.
                *( ( U8_ ) ( Vcb->FirstBlock + 0x440 ) ) = 2 * 1024 * 1024;  //  $20'0000 Volume_OverviewStart TODO
                Status = WriteBlockDevice( Vcb->PhysicalDeviceObject, 0, Volume_BlockSize, Vcb->FirstBlock, MAY_VERIFY );
ASSERT ( ! Status );


U8 Finished = CurrentMillisecond();
AlwaysLogFormatted( "W R I T I N G   A L L   M E T A D A T A   TOOK %d MILLISECONDS \n",
( int ) ( Finished - Now ) );

                Volume_LastMetadataOkCount = Volume_ConservativeMetadataUpdateCount;
////////////////ReleaseSpinlock( &Volume_MetadataLock );
                BackgroundThread_Busy = TRUE;
                continue;
            }

        }



        //  We did not have anything to do.
        BackgroundThread_Busy = FALSE;
        SleepForMilliseconds( 1 );  //  TODO Sleep?


    }

    BackgroundThread_STOPPED = TRUE;

    //PsTerminateSystemThread( STATUS_SUCCESS );  //  Terminate ourself.

    ////  Only get here on failure.
    AlwaysBreakToDebugger();

}

//////////////////////////////////////////////////////////////////////

NTSTATUS BackgroundThreadStartup( VCB_ Vcb )

{
LogString( "Entering BackgroundThreadStartup.\n" );
    NTSTATUS Status;

    HANDLE ThreadHandle = 0;  //  A "kernel handle".

    Status = PsCreateSystemThread( &ThreadHandle, 0L,                0, 0, 0, BackgroundThread, Vcb );
////Status = PsCreateSystemThread( &ThreadHandle, THREAD_ALL_ACCESS, 0, 0, 0, BackgroundThread, Vcb );

    //ZwClose( thread );  //  Will no longer have a handle to the thread.
LogString( "Leaving  BackgroundThreadStartup.\n" );

    return Status;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS BackgroundThreadShutdown()

{
LogString( "Entering BackgroundThreadShutdown\n" );

    BackgroundThread_STOP = TRUE;
    for ( int x = 0; x < 100; x++ )
    {
        if ( BackgroundThread_STOPPED )
        {
LogString( "Leaving  BackgroundThreadShutdown\n" );
            return 0;
        }

        SleepForMilliseconds( 100/2 );  //  TODO
    }
ASSERT( 0 );
    return -1;  //  TODO real value
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


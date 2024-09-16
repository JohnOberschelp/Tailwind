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

//  Locking and Unlocking Byte Ranges in Files
//  https://docs.microsoft.com/en-us/windows/win32/fileio/locking-and-unlocking-byte-ranges-in-files
//  Exclusive Lock - denies all other processes both read and write access to the specified byte range of a file.
//  Shared Lock - denies all processes write access to the specified byte range of a file,
//      including the process that first locks the byte range.
//      This can be used to create a read-only byte range in a file.

/*
0x0000012A  STATUS_FILE_LOCKED_WITH_ONLY_READERS    The file was locked and all users of the file can only read.
0x0000012B  STATUS_FILE_LOCKED_WITH_WRITERS         The file was locked and at least one user of the file can write.
0xC000002A  STATUS_NOT_LOCKED                       An attempt was made to unlock a page of memory that was not locked.
0xC0000054  STATUS_FILE_LOCK_CONFLICT               A requested read/write cannot be granted due to a conflicting file lock.
0xC0000055  STATUS_LOCK_NOT_GRANTED                 A requested file lock cannot be granted due to other existing locks.
0xC000007E  STATUS_RANGE_NOT_LOCKED                 The range specified in NtUnlockFile was not locked.
0xC00001A1  STATUS_INVALID_LOCK_RANGE               A requested file lock operation cannot be processed due to an invalid byte range.
*/
/*
NtUnlockFile
https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntunlockfile
The NtUnlockFile routine takes a range of bytes as specified by the ByteOffset and Length arguments.	
This range must be identical to a range of bytes in the file that was previously locked with a single call to the NtUnlockFile routine.	
It is not possible to unlock two previously locked adjacent ranges with a single call to NtUnlockFile.	
It is also not possible to unlock part of a range that was previously locked with a single call to the NtUnlockFile routine.
*/


//  https://docs.microsoft.com/en-us/windows/win32/debug/system-error-codes--0-499-
// ERROR_NOT_LOCKED 158 ( 0x9E ) The segment is already unlocked.
// ERROR_LOCK_VIOLATION 33 ( 0x21 ) The process cannot access the file because another process has locked a portion of the file.


//  TODO what about defining both LOCK_RANGE_ and LOCK_ or both LOCK_RANGE_ and LOCK_RANGE_SET_ ?


/*


In this situation...

  6002      0:       2 Fm      6 To      4 Len   Write     0000000000 The operation completed successfully.
  6002      1:       3 Fm      6 To      3 Len   Exclusive 0000000000 The operation completed successfully.
  6002      2:       3 Fm      4 To      1 Len   Write     0000000000 The operation completed successfully.
  6002      3:       1 Fm      3 To      2 Len   Unlock    0X0000009E The segment is already unlocked.
  6002      4:       3 Fm      6 To      3 Len   Shared    0000000000 The operation completed successfully.
  6002      5:       2 Fm      6 To      4 Len   Unlock    0X0000009E The segment is already unlocked.
  6002      6:       0 Fm      3 To      3 Len   Write     0000000000 The operation completed successfully.

          1111111111
01234567890123456789
...LLLl..................   shared
...LLLl..................   000001D3A17B4C60

  6002      7:       3 Fm      6 To      3 Len   Unlock    0000000000 The operation completed successfully.

          1111111111
01234567890123456789
...LLLl..................   000001D3A17B4C60

  6002      8:       2 Fm      5 To      3 Len   Write     0X00000021 The process cannot access the file because ano

... it looks like we should unlock the exclusive before the shared?



*/


/*
Status = MayI( READ/WRITE, fm, to, process, thread );  //  File System Internals #562
STATUS_FILE_LOCK_CONFLICT STATUS_LOCK_NOT_GRANTED
ReadLock - No write/modify allowed; may overlap
WriteLock - exclusive - no read or write - may not overlap
*/

//////////////////////////////////////////////////////////////////////

NTSTATUS IrpMjLockControl( ICB_ Icb )

{
    NTSTATUS  Status = STATUS__PRIVATE__NEVER_SET;
    PIRP      Irp    = Icb->Irp;
    IRPSP_    IrpSp  = IoGetCurrentIrpStackLocation( Irp );
    UCHAR     m      = IrpSp->MinorFunction;

//BreakToDebugger();

    if ( Icb->DeviceObject == Volume_TailwindDeviceObject ) return STATUS_INVALID_DEVICE_REQUEST;


    FCB_ Fcb = IrpSp->FileObject->FsContext;
    if ( Fcb->IsAVolume || IdIsADirectory( Fcb->Id ) ) return STATUS_INVALID_PARAMETER;

    ID Id  = Fcb->Id;

//  http://fsfilters.blogspot.com/2011/11/byte-range-locks-and-irpmjcleanup.html
//  http://www.osronline.com/article.cfm%5earticle=103.htm
//  https://docs.microsoft.com/en-us/windows/win32/fileio/locking-and-unlocking-byte-ranges-in-files?redirectedfrom=MSDN
    ULONG  Key        = IrpSp->Parameters.LockControl.Key;
    S8     ByteOffset = IrpSp->Parameters.LockControl.ByteOffset.QuadPart;
    S8     Length     = IrpSp->Parameters.LockControl.Length->QuadPart;
    B1     Exclusive  = BitIsSet( IrpSp->Flags, SL_EXCLUSIVE_LOCK );
    B1     Immediate  = BitIsSet( IrpSp->Flags, SL_FAIL_IMMEDIATELY );


S1_ M
= m == IRP_MN_LOCK              ? "IRP_MN_LOCK"
: m == IRP_MN_UNLOCK_ALL        ? "IRP_MN_UNLOCK_ALL"
: m == IRP_MN_UNLOCK_ALL_BY_KEY ? "IRP_MN_UNLOCK_ALL_BY_KEY"
: m == IRP_MN_UNLOCK_SINGLE     ? "IRP_MN_UNLOCK_SINGLE"
: "IRP_MN_?";
LogFormatted( "Key %d  exclusive?%d  immediate?%d  from $%X  for $%X  on %s  do %s\n",
Key, Exclusive, Immediate, ( int )ByteOffset, ( int )Length, Entries[Id]->Name, M );


//  https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-_fsrtl_advanced_fcb_header-fsrtlprocessfilelock


    switch ( m )
    {
      case IRP_MN_LOCK:
        {
BreakToDebugger();
            PEPROCESS Process = IoGetRequestorProcess( Irp );
            if ( Exclusive ) Status = LockRangeExclusive ( Process, &Fcb->LockRanges, ByteOffset, ByteOffset+Length, Key );
            else             Status = LockRangeShared    ( Process, &Fcb->LockRanges, ByteOffset, ByteOffset+Length, Key );

            if ( Status == STATUS_FILE_LOCK_CONFLICT && ! Immediate )
            {
AlwaysLogString( "PENDING IRP_MN_LOCK\n" );

AlwaysBreakToDebugger();
ASSERT( ( ! Icb->PendingLocksLink.Next ) && ( ! Icb->PendingLocksLink.Prev ) );
                AttachLinkLast( &Volume_PendingLocksChain, &Icb->PendingLocksLink );
                Status = STATUS_PENDING;
            }
            break;
        }

        case IRP_MN_UNLOCK_SINGLE:
        {
BreakToDebugger();
            PEPROCESS Process = IoGetRequestorProcess( Irp );
            Status = UnlockRange( Process, &Fcb->LockRanges, ByteOffset, ByteOffset+Length, Key );

            if ( Status == STATUS_LOCK_NOT_GRANTED && ! Immediate )
            {
AlwaysLogString( "PENDING IRP_MN_UNLOCK_SINGLE\n" );

AlwaysBreakToDebugger();
ASSERT( ( ! Icb->PendingLocksLink.Next ) && ( ! Icb->PendingLocksLink.Prev ) );
                AttachLinkLast( &Volume_PendingLocksChain, &Icb->PendingLocksLink );
                Status = STATUS_PENDING;
            }
            break;
        }

        case IRP_MN_UNLOCK_ALL:
AlwaysLogString( "PENDING IRP_MN_UNLOCK_ALL\n" );
        case IRP_MN_UNLOCK_ALL_BY_KEY:
AlwaysLogString( "PENDING IRP_MN_UNLOCK_ALL_BY_KEY\n" );
        default:
        {
AlwaysLogString( "PENDING default\n" );
AlwaysLogString( "PENDING ( one of the three )\n" );
AlwaysLogString( "PENDING ( one of the three )\n" );
AlwaysLogString( "PENDING ( one of the three )\n" );
BreakToDebugger();
BreakToDebugger();
BreakToDebugger();
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        }
    }


LogStatus();

    return Status;
}

//////////////////////////////////////////////////////////////////////

void LockListAttach ( LOCK_ *FirstLockHandle, LOCK_ LockToAttach )

{
    LockToAttach->Next = *FirstLockHandle;
    *FirstLockHandle = LockToAttach;
}

//////////////////////////////////////////////////////////////////////

void LockListDetach ( LOCK_ *FirstLockHandle, LOCK_ LockToDetach )

{
    if ( *FirstLockHandle == LockToDetach ) { *FirstLockHandle = LockToDetach->Next; return; }

    for ( LOCK_ L = *FirstLockHandle ;; L = L->Next )
    {
        if ( L->Next == LockToDetach ) { L->Next = LockToDetach->Next; return; }
    }
}

//////////////////////////////////////////////////////////////////////

static LOCK_ makeLock( ULONG Key, U8 Fm, U8 To, PEPROCESS Owner, LOCK_TYPE Type )

{
    LOCK_ Lock = AllocateMemory( sizeof( LOCK ) );
    if ( ! Lock ) return 0;

    Lock->Next = 0;
    Lock->Fm = Fm;
    Lock->To = To;
    Lock->Key = Key;
    Lock->Owner = Owner;
    Lock->Type = Type;

//Lock->TimesPendingLock =
//Lock->TimesPendingUnlock = 0;

    return Lock;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS splitLock( LOCK_ OriginalLock, U8 At, LOCK_ *LeftLockOut, LOCK_ *RightLockOut )

{
ASSERT( At > OriginalLock->Fm );
ASSERT( At < OriginalLock->To );

    LOCK_ LeftLock = AllocateMemory( sizeof( LOCK ) );
    if ( ! LeftLock ) return STATUS_INSUFFICIENT_RESOURCES;
    LOCK_ RightLock = OriginalLock;

    *LeftLock = *OriginalLock;
    LeftLock->To  =
    RightLock->Fm = At;

    if ( LeftLockOut  ) *LeftLockOut  = LeftLock;
    if ( RightLockOut ) *RightLockOut = RightLock;

    return 0;
}

//////////////////////////////////////////////////////////////////////
/*
LOCK_ LockListFindFirstExactPreferExact( LOCK_ *FirstLockHandle, U8 Fm, U8 To )
{
    LOCK_ Lock;
    for ( Lock = *FirstLockHandle; Lock; Lock = Lock->Next )
    {
        if ( Lock->Fm == Fm && Lock->To == To ) break;
    }
    return Lock;
}
*/
//////////////////////////////////////////////////////////////////////

LOCK_ LockListFindFirstExactPreferExact( LOCK_ *FirstLockHandle, U8 Fm, U8 To, PEPROCESS Process )

{
    LOCK_ Lock, Lock2;
    for ( Lock = *FirstLockHandle; Lock; Lock = Lock->Next )
    {
        if ( Lock->Fm == Fm && Lock->To == To ) break;
    }

    //  From parallel testing against Windows, preferentially
    //  unlock any exclusive over any shared! ?
    if ( Lock && Lock->Type == SHARED )
    {
        for ( Lock2 = Lock->Next; Lock2; Lock2 = Lock2->Next )
        {
            if ( Lock2->Fm == Fm && Lock2->To == To )
            {
                if ( Lock2->Owner == Process ) { Lock = Lock2; break; }
            }
        }
    }

    return Lock;
}

//////////////////////////////////////////////////////////////////////

LOCK_ LockListFindFirstTouching( LOCK_ *FirstLockHandle, U8 Fm, U8 To )

{
    LOCK_ Lock;
    for ( Lock = *FirstLockHandle; Lock; Lock = Lock->Next )
    {
        //  This lock's range overlaps ( touches ) our range.
        if ( Fm < Lock->To && To > Lock->Fm ) break;
    }

    return Lock;
}

//////////////////////////////////////////////////////////////////////

//  "Locks are released before the CloseHandle function is finished processing."
//  https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-unlockfileex

void UnlockAllForProcess( PEPROCESS Process, LOCK_ *FirstLockHandle )

{
BreakToDebugger();//untested code.
    LOCK_ Lock = *FirstLockHandle;
    for ( ;; )
    {
        if ( ! Lock ) break;
        LOCK_ NextLock = Lock->Next;
        if ( Lock->Owner == Process )           ////////////     IoGetRequestorProcess( Irp )...
        {
            LockListDetach( FirstLockHandle, Lock );
            FreeMemory( Lock );
        }
        Lock = NextLock;
    }
}

//////////////////////////////////////////////////////////////////////

NTSTATUS UnlockRange( PEPROCESS Process, LOCK_ *FirstLockHandle, U8 Fm, U8 To, ULONG Key )

{
if ( Key )
{
BreakToDebugger();
}
    if ( To <= Fm ) return STATUS_INVALID_LOCK_RANGE;//STATUS_INVALID_PARAMETER;

    LOCK_ Lock = LockListFindFirstExactPreferExact( FirstLockHandle, Fm, To, Process );
    if ( ! Lock ) return STATUS_RANGE_NOT_LOCKED;

    if ( Key && Lock->Key != Key ) return STATUS_FILE_LOCK_CONFLICT;

    if ( Lock->Owner && Lock->Owner != Process ) return STATUS_LOCK_NOT_GRANTED;

    LockListDetach( FirstLockHandle, Lock );
    FreeMemory( Lock );

    return 0;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS LockRangeShared( PEPROCESS Process, LOCK_ *FirstLockHandle, U8 Fm, U8 To, ULONG Key )

{
if ( Key )
{
BreakToDebugger();
}
    if ( To <= Fm ) return STATUS_INVALID_LOCK_RANGE;

    LOCK_ Lock = LockListFindFirstExactPreferExact( FirstLockHandle, Fm, To, Process );
    if ( Lock )
    {
        if ( Key && Lock->Key != Key ) return STATUS_FILE_LOCK_CONFLICT;

        if ( Lock->Owner && Lock->Owner != Process ) return STATUS_FILE_LOCK_CONFLICT;
    }

    LOCK_ NewLock = makeLock( Key, Fm, To, Process, SHARED );
    if ( ! NewLock ) return STATUS_INSUFFICIENT_RESOURCES;
    LockListAttach( FirstLockHandle, NewLock );

    return 0;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS LockRangeExclusive( PEPROCESS Process, LOCK_ *FirstLockHandle, U8 Fm, U8 To, ULONG Key )

{
if ( Key )
{
BreakToDebugger();
}
    if ( To <= Fm ) return STATUS_INVALID_LOCK_RANGE;

    LOCK_ Lock = LockListFindFirstTouching( FirstLockHandle, Fm, To );
    if ( Lock ) return STATUS_FILE_LOCK_CONFLICT;

    LOCK_ NewLock = makeLock( Key, Fm, To, Process, EXCLUSIVE );
    if ( ! NewLock ) return STATUS_INSUFFICIENT_RESOURCES;
    LockListAttach( FirstLockHandle, NewLock );

    return 0;
}

//////////////////////////////////////////////////////////////////////
//  "
//  Exclusive Lock - Denies ALL OTHER processes both read and write access to the specified byte range of a file.
//  Shared Lock - Denies ALL processes write access to the specified byte range of a file,
//      including the process that first locks the byte range.
//      This can be used to create a read-only byte range in a file. "

NTSTATUS Readable( PEPROCESS Process, LOCK_ *FirstLockHandle, U8 Fm, U8 To )

{
    LOCK_ Lock;
    for ( Lock = *FirstLockHandle; Lock; Lock = Lock->Next )
    {
        //  If this lock's range overlaps ( touches ) our range...
        if ( Fm < Lock->To && To > Lock->Fm )
        {
//            if ( Lock->Owner )
            if ( Lock->Type == EXCLUSIVE )
            {
                 if ( Lock->Owner != Process ) return STATUS_FILE_LOCK_CONFLICT;
            }
        }
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////
//  "
//  Exclusive Lock - Denies ALL OTHER processes both read and write access to the specified byte range of a file.
//  Shared Lock - Denies ALL processes write access to the specified byte range of a file,
//      including the process that first locks the byte range.
//      This can be used to create a read-only byte range in a file. "

NTSTATUS Writable( PEPROCESS Process, LOCK_ *FirstLockHandle, U8 Fm, U8 To )

{
    UNREFERENCED_PARAMETER( Process );
//BreakToDebugger();

    LOCK_ Lock;
    for ( Lock = *FirstLockHandle; Lock; Lock = Lock->Next )
    {
        //  If this lock's range overlaps ( touches ) our range...
        if ( Fm < Lock->To && To > Lock->Fm )
        {
//            if ( Lock->Owner )
            if ( Lock->Type == EXCLUSIVE )
            {
                if ( Lock->Owner != Process )
                {
                    return STATUS_FILE_LOCK_CONFLICT;
                }
            }
            else
            {
                return STATUS_FILE_LOCK_CONFLICT;
            }
        }
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


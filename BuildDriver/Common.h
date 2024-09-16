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

#pragma once

#include <ntifs.h>
#include <ntdddisk.h>
#include <ntstrsafe.h>
#include <stddef.h>

//////////////////////////////////////////////////////////////////////
//
//  Configuration
//

#define YES 1
#define NO  0

#define DEBUG_PRINTING_ON  NO   //  YES or NO; Turn on or off all regular (not ALWAYS) logging.
#define BREAKPOINTS_ON     NO   //  YES or NO; Stop at breakpoints.
#define BACKGROUND_THREAD  YES  //  YES or NO; Optionally turn off the background thread for testing.

//////////////////////////////////////////////////////////////////////
//
//  Pragmas
//

#pragma warning( disable : 4189 )  //  local variable is initialized but not referenced
#pragma warning( disable : 4200 )  //  zero sized array
#pragma warning( disable : 4201 )  //  nameless struct/union
#pragma warning( disable : 4996 )  //  using something deprecated
#pragma warning( disable : 6011 )  //  dereferencing a null pointer
#pragma warning( disable : 6054 )  //  string might not be zero terminated

//////////////////////////////////////////////////////////////////////
//
//  Defines
//

#define AlwaysBreakToDebugger DbgBreakPoint

#if BREAKPOINTS_ON == YES
    #define BreakToDebugger AlwaysBreakToDebugger
#else
    #define BreakToDebugger(...)
#endif

#define OurFileSystemType               "tailwind"
#define DRIVER_NAME                     "tailwind"  //  TODO
#define DEVICE_NAME                  L"\\tailwind"  //  TODO
#define SYMBOLIC_NAME    L"\\DosDevices\\tailwind"  //  TODO

#define TAILWIND_BASE 0x800
#define FSCTL_TAILWIND_CHAT  CTL_CODE( FILE_DEVICE_FILE_SYSTEM, TAILWIND_BASE+1, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define SECTOR_SIZE    512  //  TODO temp const should this be Volume_BlockSize

#define FAKE_USED_NUM_BYTES ( 5*1024*1024*1024LL )  // 5GB TODO temp const

#define SetBit(     Flags , Bit )    ( ( Flags ) |=  ( Bit ) )
#define ClearBit(   Flags , Bit )    ( ( Flags ) &= ~( Bit ) )
#define BitIsSet(   Flags , Bit )  ( ( ( Flags ) &   ( Bit ) ) != 0 )
#define BitIsClear( Flags , Bit )  ( ( ( Flags ) &   ( Bit ) ) == 0 )

#define ROUND_UP(   Bytes, Units )  ( ( ( Bytes ) + ( Units ) - 1 ) / ( Units ) * ( Units ) )
#define ROUND_DOWN( Bytes, Units )  ( ( ( Bytes )                 ) / ( Units ) * ( Units ) )

#define ALIGNED_256( X ) ( ( ( X ) & ( 256 - 1 ) ) == 0 )

#define IRPSP_ PIO_STACK_LOCATION

#define OWNER( Type, Field, Address ) ( ( Type * )( ( U1_ )( Address ) - ( U8 )( &( ( Type * ) 0 )->Field ) ) )

#define GET_CreationTime     DriverEntryTime
#define GET_LastAccessTime   DriverEntryTime
#define GET_LastWriteTime    DriverEntryTime
#define GET_ChangeTime       DriverEntryTime

#define MAX_PATH 260

#define CUSTOMER_DEFINED_STATUS 0x20000000L
#define STATUS__PRIVATE__NEVER_SET ( CUSTOMER_DEFINED_STATUS | 1 )

#define VOLUME_BLOCK_ALIGNED( x ) (    (   x & ( Volume_BlockSize - 1 )   )    == 0 )

#define GET1( b, v )   v = * ( U1_ ) b; b += 1;
#define GET2( b, v )   v = * ( U2_ ) b; b += 2;
#define GET4( b, v )   v = * ( U4_ ) b; b += 4;
#define GET8( b, v )   v = * ( U8_ ) b; b += 8;

#define PUT1( b, v )   * ( U1_ ) b = ( U1 ) v; b += 1;
#define PUT2( b, v )   * ( U2_ ) b = ( U2 ) v; b += 2;
#define PUT4( b, v )   * ( U4_ ) b = ( U4 ) v; b += 4;
#define PUT8( b, v )   * ( U8_ ) b = ( U8 ) v; b += 8;

//--------------------------------------------------------------------
//
//  Debug Printing
//

//  https://gcc.gnu.org/onlinedocs/cpp/Variadic-Macros.html
//#define __FILENAME__ __FILE__
//#define __FILENAME__ ( strrchr( __FILE__, '\\' ) ? strrchr( __FILE__, '\\' ) + 1 : __FILE__ )
//#define __FILENAME__ strrchr( __FILE__, '\\' ) ? strrchr( __FILE__, '\\' ) + 1 : __FILE__
#define   __FILENAME__ strrchr( __FILE__, '\\' ) + 1

#define THETHREAD ( HandleToUlong( PsGetCurrentThreadId() ) )

#if DEBUG_PRINTING_ON == YES

#define PrintRaw DbgPrint

#define LogMarker    AlwaysLogMarker
#define LogStatus    AlwaysLogStatus
#define LogString    AlwaysLogString
#define LogFormatted AlwaysLogFormatted

#else

#define PrintRaw( ... )     ( void )0
#define LogMarker()         ( void )0
#define LogStatus()         ( void )0
#define LogString( ... )    ( void )0
#define LogFormatted( ... ) ( void )0

#endif

#define AlwaysLogMarker(                ) DbgPrint( "~%04d %24s #%04d %20s \n"                     , THETHREAD, __FILENAME__ , __LINE__ , __func__ )
#define AlwaysLogStatus(                ) DbgPrint( "~%04d %24s #%04d %20s Status = %s = %#010X\n" , THETHREAD, __FILENAME__ , __LINE__ , __func__ , GetStatusAsText( Status ), Status )
#define AlwaysLogString(    string      ) DbgPrint( "~%04d %24s #%04d %20s " string                , THETHREAD, __FILENAME__ , __LINE__ , __func__ )
#define AlwaysLogFormatted( format, ... ) DbgPrint( "~%04d %24s #%04d %20s " format                , THETHREAD, __FILENAME__ , __LINE__ , __func__ , __VA_ARGS__ )

//////////////////////////////////////////////////////////////////////
//
//  Enumerations
//

typedef enum { INTO_CACHE, OUT_OF_CACHE }       DIRECTION;
typedef enum { LT=-2, LE=-1, EQ=0, GE=1, GT=2 } NEIGHBOR;  //  <  <=  =  >=  >
typedef enum { SHARED, EXCLUSIVE }              LOCK_TYPE;
typedef enum { DONT_FILL, ZERO_FILL }           FILL_TYPE;
typedef enum { CLEAN, DIRTY }                   CLEAN_OR_DIRTY;
typedef enum { NO_VERIFY=0, MAY_VERIFY }        VERIFY;

//////////////////////////////////////////////////////////////////////
//
//  Typedefs
//

typedef            void      *V_;   //  Void
typedef unsigned __int8  B1, *B1_;  //  One-byte boolean
typedef          __int8  S1, *S1_;  //  One-byte signed integer
typedef          __int16 S2, *S2_;  //  Two-byte signed integer
typedef          __int32 S4, *S4_;  //  Four-byte signed integer
typedef          __int64 S8, *S8_;  //  Eight-byte signed integer
typedef unsigned __int8  U1, *U1_;  //  One-byte unsigned integer
typedef unsigned __int16 U2, *U2_;  //  Two-byte unsigned integer
typedef unsigned __int32 U4, *U4_;  //  Four-byte unsigned integer
typedef unsigned __int64 U8, *U8_;  //  Eight-byte unsigned integer

typedef S4  ID;
typedef S4_ ID_;

typedef struct _FCB            FCB           , *FCB_           ;  //  File Control Block
typedef struct _ICB            ICB           , *ICB_           ;  //  Irp Control Block
typedef struct _CCB            CCB           , *CCB_           ;  //  Context Control Block
typedef struct _VCB            VCB           , *VCB_           ;  //  Volume Control Block
typedef struct _LOCK           LOCK          , *LOCK_          ;  //  File Lock Range
typedef struct _GLOBALS        GLOBALS       , *GLOBALS_       ;
typedef struct  _ENTRY         ENTRY         , *ENTRY_         ;  //  Either a File or a Directory
typedef struct _SET_NODE       SET_NODE      , *SET_NODE_      ;
typedef struct _MULTISET_NODE  MULTISET_NODE , *MULTISET_NODE_ ;
typedef struct _SPINLOCK       SPINLOCK      , *SPINLOCK_      ;
typedef struct _DATA_RANGE     DATA_RANGE    , *DATA_RANGE_    ;
typedef struct _FILE_DATA      FILE_DATA     , *FILE_DATA_     ;
typedef struct _SPACE_RANGE    SPACE_RANGE   , *SPACE_RANGE_   ;
typedef struct _CACHE          CACHE         , *CACHE_         ;
typedef struct _CACHE_RANGE    CACHE_RANGE   , *CACHE_RANGE_   ;

typedef struct _CHAIN { struct _LINK *First ; struct _LINK *Last; } CHAIN , *CHAIN_ ;
typedef struct _LINK  { struct _LINK *Next  ; struct _LINK *Prev; } LINK  , *LINK_  ;

//////////////////////////////////////////////////////////////////////
//
//  Constants
//

static const U4 A_DIRECTORY = ( U4 ) -1;  //  Indicates NOT a file num ranges, but instead a directory.

//////////////////////////////////////////////////////////////////////
//
//  Structs
//

//--------------------------------------------------------------------

struct _SET_NODE { SET_NODE_ P, L, R; };  //  Parent, Left, Right

//--------------------------------------------------------------------

struct _MULTISET_NODE { MULTISET_NODE_ P, L, R, E; };  //  Parent, Left, Right, Equal

//--------------------------------------------------------------------

struct _DATA_RANGE
{
    U8   VolumeAddress;
    U4   NumBytes;
};

//--------------------------------------------------------------------

struct _FILE_DATA
{
    U8         FileNumBytes;
    U8         AllocationNumBytes;
    U4         NumRanges;
    DATA_RANGE DataRange[100];  //  6   Better for testing than 0.
};

//--------------------------------------------------------------------

struct _ENTRY
{
    ID   Id;

    ID   SiblingTreeParentId;
    ID   SiblingTreeLeftId;
    ID   SiblingTreeRightId;

    ID   ParentId;
    U4   FileAttributes;

    //U8 CreationTime;
    //U8 LastAccessTime;
    //U8 LastWriteTime;
    //U8 ChangeTime;
    //U8 IndexNumber;

    U1   NameNumBytes;
    S1   Name[256];
};
//  In memory, the actual Name num bytes is not 256, but NameNumBytes + 1 for trailing zero.
//  This is always followed by either...
//      For a directory : an ID for the root node of its children
//      For a file      : FILE_DATA

//--------------------------------------------------------------------

struct _SPACE_RANGE
{
    MULTISET_NODE  ByNumBytes;
    SET_NODE       ByVolumeAddress;
    U8             VolumeAddress;
    U4             NumBytes;
};

//--------------------------------------------------------------------

struct _SPINLOCK
{
    LONG Locked;
    LONG NumSpinning;
};

//--------------------------------------------------------------------

struct _LOCK
{
    LOCK_     Next;
    U8        Fm;
    U8        To;
    ULONG     Key;
////FCB_    Owner;
    PEPROCESS Owner;
LOCK_TYPE Type;
//int TimesPendingLock;
//int TimesPendingUnlock;

};

//--------------------------------------------------------------------
//
//  Volume Control Block
//

struct _VCB
{  //  TODO AdvancedFcbHeader or SectionObjectPointers needed for volumes? yes if IsAVolume relied on.
    FSRTL_ADVANCED_FCB_HEADER AdvancedFcbHeader;  //  Need because it indicates if fast I/O is possible?
    SECTION_OBJECT_POINTERS   SectionObjectPointers;  //  CDFS "special pointers used by MM and Cache to manipluate section objects."
    B1                        IsAVolume;  //  Always TRUE.?! TODO

    B1                        Dismounted;
    B1                        VolumeLocked;
    B1                        VolumeDirty;
    CHAIN                     OpenFcbsChain;
    ULONG                     VcbNumReferences;   //  ++ on IRP_MJ_CREATE; -- on IRP_MJ_CLOSE.
    ULONG                     VcbNumOpenHandles;  //  ++ on IRP_MJ_CREATE; -- on IRP_MJ_CLEANUP.
    PVPB                      Vpb;
    PDEVICE_OBJECT            VolumeDeviceObject;
    PDEVICE_OBJECT            PhysicalDeviceObject;
    DISK_GEOMETRY_EX          DiskGeometryEx;
    PARTITION_INFORMATION_EX  PartitionInformationEx;
    U1_                       FirstBlock;
    ID                        RootId;
};

//--------------------------------------------------------------------
//
//  File Control Block
//

struct _FCB
{
    FSRTL_ADVANCED_FCB_HEADER AdvancedFcbHeader;  //  Need because it indicates if fast I/O is possible?
    SECTION_OBJECT_POINTERS   SectionObjectPointers;  //  CDFS "special pointers used by MM and Cache to manipluate section objects."
    B1                        IsAVolume;  //  Always FALSE.

    B1                        DeleteIsPending;
    LINK                      OpenFcbsLink;
    ULONG                     NumReferences;   //  ++ on IRP_MJ_CREATE; -- on IRP_MJ_CLOSE.
    LOCK_                     LockRanges;
    VCB_                      Vcb;
    ID                        Id;
};

//--------------------------------------------------------------------
//
//  Context Control Block
//

struct _CCB
{
    ULONG  CurrentByteOffset;  //  TODO used?
    B1     FirstQuery;
    B1     IsAPattern;
    char   NameOrPattern[512];  //  TODO
    char   LastProcessedName[512];  //  TODO
};

//--------------------------------------------------------------------
//
//  Irp Control Block
//

struct _ICB
{
    PIRP            Irp;
    PDEVICE_OBJECT  DeviceObject;

    LINK            PendingReadsLink;
    LINK            PendingLocksLink;

    B1              DoCompleteRequest;
};

//////////////////////////////////////////////////////////////////////
//
//  Globals
//

extern ENTRY_* Entries;  //  Index to convert an Id to an Entry_
extern unsigned int PrivateSeed;
extern LARGE_INTEGER PerformanceCounterFrequencyInTicksPerSecond;

extern U4             Volume_SpaceNodeMaxNumBytes;
extern U4             Volume_BlockSize;
extern U8             Volume_OverviewStart;
extern U8             Volume_OverviewNumBytes;
extern U8             Volume_EntriesStart;
extern U8             Volume_EntriesNumBytes;
extern U8             Volume_DataStart;
extern U8             Volume_DataNumBytes;
extern U8             Volume_TotalNumBytes;
extern U8             Volume_ConservativeMetadataUpdateCount;
extern U8             Volume_LastMetadataOkCount;
extern SPINLOCK       Volume_MetadataLock;
extern U4             Volume_TotalNumberOfEntries;
extern ID             Volume_WhereTableMaxId;
extern U4             Volume_WhereTableTotalAllocation;
extern ID             Volume_WhereTableLastRecycledID;       //  i.e. No recycled IDs.
extern ID             Volume_WhereTableFirstUnusedBottomID;  //  Index of zero reserved for errors like not found.
extern PDRIVER_OBJECT Volume_DriverObject;
extern PDEVICE_OBJECT Volume_TailwindDeviceObject;
extern PDEVICE_OBJECT Volume_PhysicalDeviceObject;
extern U4             Volume_NumSpaceBytesOnVolume;
extern U4             Volume_NumSpaceRangesOnVolume;
extern MULTISET_NODE_ Volume_SpaceByNumBytes;
extern SET_NODE_      Volume_SpaceByAddress;
extern U4             Volume_SpaceNumNodes;
extern U8             Volume_SpaceNumDifferencesFromVolume;
extern U8             Volume_SpaceNumBytes;
extern CHAIN          Volume_PendingReadsChain;
extern CHAIN          Volume_PendingLocksChain;

extern LARGE_INTEGER  DriverEntryTime;
extern UNICODE_STRING DeviceName;
extern UNICODE_STRING SymbolicName;

extern CACHE Cache;

extern U1_ EntriesBytes;
extern U4  EntriesTotalAllocation;
extern U4  EntriesNumBytesAvailable;
extern U4  EntriesFirstFreeByte;

extern int ChatVariable;

extern volatile B1  BackgroundThread_Busy;
extern U8 LastIrpMicrosecondStart;


//////////////////////////////////////////////////////////////////////
//
//  Inlines
//
//--------------------------------------------------------------------

inline B1 EntryIsADirectory ( ENTRY_ Entry ) { return Entry && BitIsSet(   Entry->FileAttributes, FILE_ATTRIBUTE_DIRECTORY ); }
inline B1 EntryIsAFile      ( ENTRY_ Entry ) { return Entry && BitIsClear( Entry->FileAttributes, FILE_ATTRIBUTE_DIRECTORY ); }
inline B1 IdIsADirectory    ( ID Id        ) { return Id    && BitIsSet(   Entries[Id]->FileAttributes, FILE_ATTRIBUTE_DIRECTORY ); }
inline B1 IdIsAFile         ( ID Id        ) { return Id    && BitIsClear( Entries[Id]->FileAttributes, FILE_ATTRIBUTE_DIRECTORY ); }
inline V_ Zero( V_ Address, size_t NumBytes ) { return memset( Address, 0, NumBytes ); }

//--------------------------------------------------------------------

inline ID_ ChildrenTree_( ENTRY_ Entry )

{
ASSERT( EntryIsADirectory( Entry ) );

    return ( ID_ ) ( ( U1_ ) Entry + offsetof( ENTRY, Name ) + Entry->NameNumBytes + 1 );
}

//--------------------------------------------------------------------

inline ID ChildrenTree( ENTRY_ Entry )

{
ASSERT( EntryIsADirectory( Entry ) );

    return * ( ID_ ) ( ( U1_ ) Entry + offsetof( ENTRY, Name ) + Entry->NameNumBytes + 1 );
}

//--------------------------------------------------------------------

inline FILE_DATA_  Data( ENTRY_ Entry )

{
ASSERT( EntryIsAFile( Entry ) );

    return ( FILE_DATA_ ) ( ( U1_ ) Entry + offsetof( ENTRY, Name ) + Entry->NameNumBytes + 1 );
}

//--------------------------------------------------------------------

inline B1 HasChildren( ID Id )

{
    if ( IdIsAFile( Id ) ) return FALSE;
    if ( ChildrenTree( Entries[Id] ) ) return TRUE;
    return FALSE;
}


//--------------------------------------------------------------------

inline void InitializeSpinlock( SPINLOCK_ Lock )

{
    Lock->Locked      = 0;  //  Initialize the lock to "not locked".
    Lock->NumSpinning = 0;  //  Initialize the number of waiting threads to none.
}

//--------------------------------------------------------------------

inline B1 AcquireSpinlockOrFail( SPINLOCK_ Lock )

{
    //                                                  destination      exchange     Comperand
    LONG PreviouslyLocked = InterlockedCompareExchange( &Lock->Locked,    1,           0 );
    B1 Acquired = ! PreviouslyLocked;
    return Acquired;
}

//--------------------------------------------------------------------

    void SleepForMilliseconds( int Milliseconds );

inline void AcquireSpinlock( SPINLOCK_ Lock )

{
    InterlockedIncrement( &Lock->NumSpinning );
    ////Lock->NumSpinning++;

    U8 NumTimesHadToWait = 0;
    for ( ;; )
    {
        //                                                  destination      exchange     Comperand
        LONG PreviouslyLocked = InterlockedCompareExchange( &Lock->Locked,    1,           0 );
        if ( ! PreviouslyLocked ) break;
        NumTimesHadToWait++;
        //  TODO Do some small useful thing here maybe.
        if ( ( NumTimesHadToWait & 0xFFFFF ) == 0 )  //  TODO constant
        {
LogFormatted( "****!!!! Waited %lld times; sleeping 1 ms here  (%d spinning) \n", NumTimesHadToWait, Lock->NumSpinning );  //  TODO
            SleepForMilliseconds( 1 );
        }
    }

if ( NumTimesHadToWait )
LogFormatted( "Had to wait %lld times.\n", NumTimesHadToWait );

    InterlockedDecrement( &Lock->NumSpinning );
    ////Lock->NumSpinning--;
}

//--------------------------------------------------------------------

inline void ReleaseSpinlock( SPINLOCK_ Lock )

{
    InterlockedExchange( &Lock->Locked, 0 );  //  Switch the lock to "not locked".
}

//--------------------------------------------------------------------

inline void UninitializeSpinlock( SPINLOCK_ Lock )

{
    UNREFERENCED_PARAMETER( Lock );
}

//--------------------------------------------------------------------

inline UINT32 OrNormal( UINT32 FileAttributes )

{
    //  from FAT?
    //const UINT32 NotNormal = FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_ARCHIVE;
    //if ( ( FileAttributes & NotNormal ) == 0 )

    if ( FileAttributes == 0 )
    {
        FileAttributes = FILE_ATTRIBUTE_NORMAL;  //  "Synthesize" FILE_ATTRIBUTE_NORMAL.
    }

    return FileAttributes;
}

//--------------------------------------------------------------------

inline void AttachLinkFirst( CHAIN_ Chain, LINK_ Link )

{
    Link->Prev = 0;
    Link->Next = Chain->First;
    if ( Link->Next )
    {
ASSERT( Link->Next->Prev == 0 );
        Link->Next->Prev = Link;
    }
    Chain->First = Link;
    if ( ! Chain->Last ) Chain->Last = Link;
}

//--------------------------------------------------------------------

inline void AttachLinkLast( CHAIN_ Chain, LINK_ Link )

{
    LINK_ OldLast = Chain->Last;
    if ( ! OldLast )
    {
        Chain->First = Chain->Last = Link;
        Link->Next = Link->Prev = 0;
        return;
    }
ASSERT( OldLast->Next == 0 );
    OldLast->Next = Link;
    Link->Next = 0;
    Link->Prev = OldLast;
    Chain->Last = Link;
}

//--------------------------------------------------------------------

inline void DetachLink( CHAIN_ Chain, LINK_ Link )

{
ASSERT( Chain->First == Link || Link->Next || Link->Prev );

    if ( Link->Prev ) Link->Prev->Next = Link->Next;
    else              Chain->First     = Link->Next;

    if ( Link->Next ) Link->Next->Prev = Link->Prev;
    else              Chain->Last      = Link->Prev;

ASSERT( Link->Next != Link );
ASSERT( Link->Prev != Link );

    Link->Next = Link->Prev = 0;
}

//--------------------------------------------------------------------

inline B1 IsLinked( CHAIN_ Chain, LINK_ Link )

{
    B1 IsIt = Chain->First == Link || Link->Next || Link->Prev;
    return IsIt;
}

//--------------------------------------------------------------------

inline U8 CurrentMillisecond()

{
    LARGE_INTEGER Counter;

    Counter = KeQueryPerformanceCounter( 0 );  //  TODO slow?

    U8 millisecond = Counter.QuadPart * 1'000 / PerformanceCounterFrequencyInTicksPerSecond.QuadPart;
    return millisecond;
}

//--------------------------------------------------------------------

inline U8 CurrentMicrosecond()

{
    LARGE_INTEGER Counter = KeQueryPerformanceCounter( 0 );  //  TODO slow?

    U8 microsecond = Counter.QuadPart * 1'000'000 / PerformanceCounterFrequencyInTicksPerSecond.QuadPart;

    return microsecond;
}

//--------------------------------------------------------------------

inline void CopyString( char * dest, size_t destsz, const char * src )

{
    errno_t Error = strcpy_s( dest, destsz, src );

ASSERT( ! Error );
}

//--------------------------------------------------------------------

inline void* AllocateMemory( U8 NumberOfBytes )

{
    return ExAllocatePool2( POOL_FLAG_UNINITIALIZED | POOL_FLAG_NON_PAGED, NumberOfBytes, 'liaT' );
}

//--------------------------------------------------------------------

inline void* AllocateAndZeroMemory( U8 NumberOfBytes )

{
    void* m = AllocateMemory( NumberOfBytes );
    if ( m ) Zero( m, NumberOfBytes );
    return m;
}

//--------------------------------------------------------------------

inline void FreeMemory( void* Allocation )

{
    if ( Allocation ) ExFreePool( Allocation );
}

//--------------------------------------------------------------------

inline V_ IrpBuffer( PIRP Irp )

{
    if ( Irp->MdlAddress )
    {
LogString( "!!!!! Calling MmGetSystemAddressForMdlSafe !!!!!\n" );
        return MmGetSystemAddressForMdlSafe( Irp->MdlAddress, NormalPagePriority );
    }
    else
    {
        return Irp->UserBuffer;
    }
}

//////////////////////////////////////////////////////////////////////
//
//  Function Prototypes
//

NTSTATUS DataReallocateFile ( ID, U8 MinimumNewAllocationSize );
NTSTATUS DataResizeFile     ( ID, U8 NewFileSize, FILL_TYPE );

//--------------------------------------------------------------------

ID   MakeEntry   ( ID ParentId, U4 DirectoryOrNumRanges, char* Name );
void UnmakeEntry ( ID );
ID   EntryNext   ( ID );
ID   EntryPrev   ( ID );
ID   EntryFirst  ( ID );
ID   EntryLast   ( ID );
ID   EntryFind   ( ID *rootHandle, S1_ Name );
ID   EntryNear   ( ID *rootHandle, S1_ Name, NEIGHBOR want );
void AttachEntry ( ENTRY_ Parent, ENTRY_ Entry );
void DetachEntry ( ENTRY_ Entry );

//--------------------------------------------------------------------

FCB_ LookupFcbById ( VCB_, ID );

NTSTATUS FindByPathAndName ( ID StartId, S1_ PathAndName, S1_ *NameOut, ID *ParentIdOut, ID *EntryIdOut );

//--------------------------------------------------------------------

int Utf8StringToWideString                ( S1_ Utf8String, WCHAR * WideString );
int WideStringToUtf8String                ( WCHAR * WideString, S1_ Utf8String );
int WideCharactersToUtf8String            ( int NumBytesIn, WCHAR* WideCharacters, S1_ Utf8String );
int WidePathAndNameCharactersToUtf8String ( int NumBytesIn, WCHAR* WideCharacters, S1_ Utf8String );
B1  WildcardCompare                       ( char* pattern, int patternLen, char* string, int stringLen );
int WideFullPathAndName                   ( ENTRY_ , WCHAR *WidePathAndNameOut );
B1  RemoveTrailingNameFromPath            ( S1_ PathAndName );

//--------------------------------------------------------------------

U8 DataGetFileNumBytes       ( ENTRY_ );
U8 DataGetAllocationNumBytes ( ENTRY_ );

//--------------------------------------------------------------------

NTSTATUS CacheStartup                         ();
NTSTATUS CacheShutdown                        ();
U4       CacheFreeSomeCache                   ();
NTSTATUS CacheReport                          ( S1_ Buffer, int MaxNumBytes );
U4       CacheBackgroundWriteDirtiestToVolume ();

NTSTATUS BackgroundThreadStartup  ( VCB_ );
NTSTATUS BackgroundThreadShutdown ();

NTSTATUS SpaceStartup            ();
NTSTATUS SpaceShutdown           ();
NTSTATUS SpaceRequestNumBytes    ( U4 NumBytesRequested, U8 * VolumeAddressResult, U4 * VolumeNumBytesResult );
NTSTATUS SpaceReturnAddressRange ( U8 ReturnAddress, U4 ReturnNumBytes );

void SleepForMilliseconds ( int NumMilliseconds );

void     DumpRam ( const void* AddressAsVoid, int NumBytes );

NTSTATUS WriteBlockDevice ( PDEVICE_OBJECT, U8 Offset, U4 Length, V_ Buffer, VERIFY );
NTSTATUS ReadBlockDevice  ( PDEVICE_OBJECT, U8 Offset, U4 Length, V_ Buffer, VERIFY );

void FindFirstUncachedRange ( ENTRY_, U8_ A_, U4_ N_ );

//  TODO Bad?
NTSTATUS CacheRangeMakeWithLock ( U8 VolumeAddress, U1_ B, U4 NumBytes, CLEAN_OR_DIRTY, U4 WithinRangeOffset, U4 WithinRangeNumBytes );

NTSTATUS CacheBlindlyThrowAwayAll ();
NTSTATUS AccessCacheForFile ( ENTRY_, DIRECTION, U1_ CallerBuffer, U8 CallerFileOffset, U4 CallerNumBytes );

NTSTATUS DataReadFromFile ( ENTRY_, U8 Offset, U4 Length, U1_ BufferOut );
NTSTATUS DataWriteToFile  ( ENTRY_, U8 Offset, U4 Length, U1_ BufferIn  );

NTSTATUS ResizeEntry ( ID Id, S1_ NewNameOrZero, S4 DeltaToNumRanges );

NTSTATUS EntriesStartupEmpty ( U4 RequestedNumBytes );
NTSTATUS EntriesShutdown     ();
NTSTATUS EntriesFreeze       ( V_ Buffer, U4 NumBytes );
NTSTATUS EntriesThaw         ();
ENTRY_   EntriesAllocate     ( U4 NumBytesToAllocate );
void     EntriesFree         ( ENTRY_ );
void     EntriesCompact      ();

NTSTATUS WhereTableStartupEmpty ( U4 RequestedNumBytes );
NTSTATUS WhereTableShutdown     ();

NTSTATUS OverviewFreeze1 ();
NTSTATUS OverviewFreeze2 ();
NTSTATUS OverviewThaw    ();

U4 EntrySize ( ENTRY_ );

NTSTATUS SpaceBuildFromEntries ();

void ZeroVolumeGlobals ();

NTSTATUS Dispatch ( PDEVICE_OBJECT , PIRP );
NTSTATUS IrpMj                       ( ICB_ );
NTSTATUS IrpMjLockControl            ( ICB_ );
NTSTATUS IrpMjRead                   ( ICB_ );
NTSTATUS IrpMjRead_File              ( ICB_ );
NTSTATUS IrpMjRead_Volume            ( ICB_ );
NTSTATUS IrpMjWrite                  ( ICB_ );
NTSTATUS IrpMjQueryVolumeInformation ( ICB_ );
NTSTATUS IrpMjCreate                 ( ICB_ );
NTSTATUS IrpMjCleanup                ( ICB_ );
NTSTATUS IrpMjClose                  ( ICB_ );
NTSTATUS IrpMjDeviceControl          ( ICB_ );
NTSTATUS IrpMjDirectoryControl       ( ICB_ );
NTSTATUS IrpMjQueryInformation       ( ICB_ );
NTSTATUS IrpMjFlushBuffers           ( ICB_ );
NTSTATUS IrpMjSetInformation         ( ICB_ );
NTSTATUS IrpMjShutdown               ( ICB_ );
NTSTATUS IrpMjFileSystemControl      ( ICB_ );

ICB_ MakeIcb ( PDEVICE_OBJECT , PIRP );
FCB_ MakeFcb ( VCB_ , ID );
CCB_ MakeCcb ();
VCB_ MakeVcb ( ICB_, PDEVICE_OBJECT VolumeDeviceObject, PDEVICE_OBJECT PhysicalDeviceObject );

void UnmakeIcb ( ICB_ );
void UnmakeFcb ( FCB_ );
void UnmakeCcb ( CCB_ );
void UnmakeVcb ( VCB_ );

NTSTATUS DeviceIoControlRequest ( ULONG IoControlCode, PDEVICE_OBJECT DeviceObject, V_ InputBuffer, ULONG InputBufferLength, V_ OutputBuffer, ULONG OutputBufferLength );

NTSTATUS FsctlMountVolume     ( ICB_ );
NTSTATUS FsctlVerifyVolume    ( ICB_ );
NTSTATUS FsctlLockVolume      ( ICB_ );
NTSTATUS FsctlUnlockVolume    ( ICB_ );
NTSTATUS FsctlDismountVolume  ( ICB_ );
NTSTATUS FsctlMarkVolumeDirty ( ICB_ );
NTSTATUS FsctlIsVolumeDirty   ( ICB_ );

void PurgeVolumesFiles ( VCB_, B1 FlushBeforePurge );

void SetVpbBit   ( PVPB, USHORT Flag );
void ClearVpbBit ( PVPB, USHORT Flag );

const char* GetFileInformationClassAsText ( FILE_INFORMATION_CLASS );
const char* GetDispositionAsText          ( U1 Disposition );
const char* GetStatusAsText               ( NTSTATUS );
const char* GetFsControlCodeAsText        ( ULONG FsControlCode, S1_ SuppliedBuffer );
const char* GetFsControlCodeMinorAsText   ( ULONG FsControlCodeMinor, S1_ SuppliedBuffer );
const char* GetMajorFunctionName          ( UCHAR MajorFunction );

void     UnlockAllForProcess ( PEPROCESS, LOCK_ *RootHandle );
NTSTATUS UnlockRange         ( PEPROCESS, LOCK_ *RootHandle, U8 Fm, U8 To, ULONG Key );
NTSTATUS LockRangeShared     ( PEPROCESS, LOCK_ *RootHandle, U8 Fm, U8 To, ULONG Key );
NTSTATUS LockRangeExclusive  ( PEPROCESS, LOCK_ *RootHandle, U8 Fm, U8 To, ULONG Key );
NTSTATUS Readable            ( PEPROCESS, LOCK_ *RootHandle, U8 Fm, U8 To );
NTSTATUS Writable            ( PEPROCESS, LOCK_ *RootHandle, U8 Fm, U8 To );

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


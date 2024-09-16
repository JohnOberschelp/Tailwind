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

LARGE_INTEGER  PerformanceCounterFrequencyInTicksPerSecond = {0,0};

U8 LastIrpMicrosecondStart = 0;

LARGE_INTEGER  DriverEntryTime;
UNICODE_STRING DeviceName;
UNICODE_STRING SymbolicName;

PDRIVER_OBJECT Volume_DriverObject;
PDEVICE_OBJECT Volume_TailwindDeviceObject;
PDEVICE_OBJECT Volume_PhysicalDeviceObject;
U4             Volume_NumSpaceBytesOnVolume;
U4             Volume_NumSpaceRangesOnVolume;
MULTISET_NODE_ Volume_SpaceByNumBytes;
SET_NODE_      Volume_SpaceByAddress;
U4             Volume_SpaceNumNodes;
U8             Volume_SpaceNumDifferencesFromVolume;
U8             Volume_SpaceNumBytes;
CHAIN          Volume_PendingReadsChain;
CHAIN          Volume_PendingLocksChain;
U4             Volume_SpaceNodeMaxNumBytes = 16 * 1024 * 1024;  //  16MB TODO what is the best number?
///            Volume_SpaceNodeMaxNumBytes =  1 * 1024 * 1024;  //  16MB TODO what is the best number?
U4             Volume_BlockSize = 4096;
U8             Volume_OverviewStart;
U8             Volume_OverviewNumBytes;
U8             Volume_EntriesStart;
U8             Volume_EntriesNumBytes;
U8             Volume_DataStart;
U8             Volume_DataNumBytes;
U8             Volume_TotalNumBytes;
U8             Volume_ConservativeMetadataUpdateCount;
U8             Volume_LastMetadataOkCount;
SPINLOCK       Volume_MetadataLock;
U4             Volume_TotalNumberOfEntries;
ID             Volume_WhereTableMaxId;
U4             Volume_WhereTableTotalAllocation;
ID             Volume_WhereTableLastRecycledID;       //  i.e. No recycled IDs.
ID             Volume_WhereTableFirstUnusedBottomID;  //  Index of zero reserved for errors like not found.

int ChatVariable = 0;

U1_     EntriesBytes = 0;
U4      EntriesTotalAllocation;
U4      EntriesFirstFreeByte;
U4      EntriesNumBytesAvailable;
ENTRY_* Entries = 0;  //  Table to convert an entry ID to and entry's address.

//////////////////////////////////////////////////////////////////////

void ZeroVolumeGlobals()

{
    Volume_DriverObject = 0;
    Volume_TailwindDeviceObject = 0;
    Volume_PhysicalDeviceObject = 0;
    DriverEntryTime.QuadPart = 0;
    Volume_NumSpaceBytesOnVolume = 0;
    Volume_NumSpaceRangesOnVolume = 0;
    Volume_SpaceByNumBytes = 0;
    Volume_SpaceByAddress = 0;
    Volume_SpaceNumNodes = 0;
    Volume_SpaceNumDifferencesFromVolume = 0;
    Volume_SpaceNumBytes = 0;
    Volume_ConservativeMetadataUpdateCount = 0;
    Volume_LastMetadataOkCount = 0;

    Zero( &Volume_PendingReadsChain, sizeof( CHAIN ) );
    Zero( &Volume_PendingLocksChain, sizeof( CHAIN ) );

    CacheStartup();
}

//////////////////////////////////////////////////////////////////////

void SleepForMilliseconds( int Milliseconds )

{
    const S8 RELATIVE_TIME = -1;
    const BOOLEAN IsWaitAlertable = TRUE;  //  https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-kedelayexecutionthread
////const BOOLEAN IsWaitAlertable = FALSE;  //  fastfat uses FALSE
    LARGE_INTEGER HundredsOfNanoseconds;
    int Microseconds = Milliseconds * 1'000;
    HundredsOfNanoseconds.QuadPart = RELATIVE_TIME * Microseconds * 10;
    KeDelayExecutionThread( KernelMode, IsWaitAlertable, &HundredsOfNanoseconds );

}

//////////////////////////////////////////////////////////////////////

int Utf8StringToWideString( S1_ Utf8, WCHAR * Wchar )

{
    WCHAR * StartWchar = Wchar;
    unsigned int CodePoint = 0;

    for ( ;; )
    {
        U1 c = ( U1 ) ( *Utf8++ );
        if ( !c ) break;

        if      ( c <= 0x7F ) CodePoint = c;
        else if ( c <= 0xBF ) CodePoint = ( CodePoint << 6 ) | ( c & 0x3F );
        else if ( c <= 0xDF ) CodePoint = c & 0x1F;
        else if ( c <= 0xEF ) CodePoint = c & 0x0F;
        else                  CodePoint = c & 0x07;

        if ( ( ( *Utf8 & 0xC0 ) != 0x80 ) && ( CodePoint <= 0x10FFFF ) )
        {
            if ( CodePoint > 0xFFFF )
            {
                CodePoint -= 0x10000;
                *Wchar++ = ( WCHAR ) ( 0xD800 + ( CodePoint >> 10 ) );
                *Wchar++ = ( WCHAR ) ( 0xDC00 + ( CodePoint & 0x03FF ) );
            }
            else
            if ( CodePoint < 0xD800 || CodePoint >= 0xE000 )
            {
                *Wchar++ = ( WCHAR ) CodePoint;
            }
        }
    }

    *Wchar = 0;

    return ( int ) ( Wchar - StartWchar );
}
//////////////////////////////////////////////////////////////////////

int WideStringToUtf8String( WCHAR * Wchar, S1_ Utf8 )

{
    S1_ StartUtf8 = Utf8;
    unsigned int CodePoint = 0;

    for ( ; *Wchar;  Wchar++ )
    {
        if ( *Wchar >= 0xD800 && *Wchar <= 0xDBFF )
            CodePoint = ( ( *Wchar - 0xD800 ) << 10 ) + 0x10000;
        else
        {
            if ( *Wchar >= 0xDC00 && *Wchar <= 0xDFFF )
                CodePoint |= *Wchar - 0xDC00;
            else
                CodePoint = *Wchar;

            if ( CodePoint <= 0x7F )
            {
                *Utf8++ = ( S1 ) CodePoint;
            }
            else
            if ( CodePoint <= 0x7FF )
            {
                *Utf8++ = ( 0xC0 | ( ( CodePoint >> 6 ) & 0x1F ) );
                *Utf8++ = ( 0x80 | (   CodePoint        & 0x3F ) );
            }
            else
            if ( CodePoint <= 0xFFFF )
            {
                *Utf8++ = ( 0xE0 | ( ( CodePoint >> 12 ) & 0x0F ) );
                *Utf8++ = ( 0x80 | ( ( CodePoint >>  6 ) & 0x3F ) );
                *Utf8++ = ( 0x80 | (   CodePoint         & 0x3F ) );
            }
            else
            {
                *Utf8++ = ( 0xF0 | ( ( CodePoint >> 18 ) & 0x07 ) );
                *Utf8++ = ( 0x80 | ( ( CodePoint >> 12 ) & 0x3F ) );
                *Utf8++ = ( 0x80 | ( ( CodePoint >>  6 ) & 0x3F ) );
                *Utf8++ = ( 0x80 | (   CodePoint         & 0x3F ) );
            }
            CodePoint = 0;
        }
    }

    *Utf8 = 0;

    return ( int ) ( Utf8 - StartUtf8 );
}

//////////////////////////////////////////////////////////////////////

int WideCharactersToUtf8String( int NumBytesIn, WCHAR * WideCharacters, S1_ Utf8String )

{
    S1_ StartUtf8 = Utf8String;
    unsigned int CodePoint = 0;
    const WCHAR * StopWchar = WideCharacters + ( NumBytesIn/2 );

    for ( ; WideCharacters < StopWchar;  WideCharacters++ )
    {
        if ( *WideCharacters >= 0xD800 && *WideCharacters <= 0xDBFF )
            CodePoint = ( ( *WideCharacters - 0xD800 ) << 10 ) + 0x10000;
        else
        {
            if ( *WideCharacters >= 0xDC00 && *WideCharacters <= 0xDFFF )
                CodePoint |= *WideCharacters - 0xDC00;
            else
                CodePoint = *WideCharacters;

            if ( CodePoint <= 0x7F )
            {
                *Utf8String++ = ( S1 ) CodePoint;
            }
            else
            if ( CodePoint <= 0x7FF )
            {
                *Utf8String++ = ( 0xC0 | ( ( CodePoint >> 6 ) & 0x1F ) );
                *Utf8String++ = ( 0x80 | (   CodePoint        & 0x3F ) );
            }
            else
            if ( CodePoint <= 0xFFFF )
            {
                *Utf8String++ = ( 0xE0 | ( ( CodePoint >> 12 ) & 0x0F ) );
                *Utf8String++ = ( 0x80 | ( ( CodePoint >>  6 ) & 0x3F ) );
                *Utf8String++ = ( 0x80 | (   CodePoint         & 0x3F ) );
            }
            else
            {
                *Utf8String++ = ( 0xF0 | ( ( CodePoint >> 18 ) & 0x07 ) );
                *Utf8String++ = ( 0x80 | ( ( CodePoint >> 12 ) & 0x3F ) );
                *Utf8String++ = ( 0x80 | ( ( CodePoint >>  6 ) & 0x3F ) );
                *Utf8String++ = ( 0x80 | (   CodePoint         & 0x3F ) );
            }
            CodePoint = 0;
        }
    }

    *Utf8String = 0;

    return ( int ) ( Utf8String - StartUtf8 );
}

//////////////////////////////////////////////////////////////////////

int WidePathAndNameCharactersToUtf8String( int NumBytesIn, WCHAR* WideCharacters, S1_ Utf8String )

{
    //  Convert any forward slashes to backslashes.
    WCHAR   BackSlashed[512];
    for ( int i = 0; i < NumBytesIn/2; i++ )  //  TODO wont work on 4 byte chars
    {
        WCHAR c = WideCharacters[i];
        BackSlashed[i] = ( c == L'/' ) ? L'\\' : c;
    }

    int NumBytes = WideCharactersToUtf8String( NumBytesIn, BackSlashed, Utf8String );

    return NumBytes;
}

//////////////////////////////////////////////////////////////////////
//
//  Compare a char string to a pattern
//  Patterns may contain:
//    '?': match any single character
//    '*': match zero or more characters

B1 WildcardCompare( char* pattern, int patternLen, char* string, int stringLen )

{

  top:

    if ( stringLen == patternLen && _strnicmp( pattern, string, stringLen ) == 0 ) return TRUE;

    if ( stringLen == 0 || string == 0 )
    {
        if ( pattern == 0 ) return TRUE;
        for ( int i = 0; i < patternLen; i++ ) if ( pattern[i] != '*' ) return FALSE;
        return TRUE;
    }

    if ( patternLen == 0 )
    {
        return FALSE;
    }

    if ( pattern[0] == '?' )
    {
        pattern++; patternLen--; string++; stringLen--; goto top;
    }

    if ( pattern[patternLen - 1] == '?' )
    {
        patternLen--; stringLen--; goto top;
    }

    if ( pattern[0] == '*' )
    {
        if ( WildcardCompare( pattern+1, patternLen-1, string, stringLen ) )
        {
            return TRUE;
        }
        string++; stringLen--; goto top;
    }

    if ( pattern[patternLen - 1] == '*' )
    {
        if ( WildcardCompare( pattern,  patternLen - 1, string, stringLen ) )
        {
            return TRUE;
        }
        stringLen--; goto top;
    }

    B1 charsMatch;
    char p = pattern[0];
    char s = string[0];
    if ( p == s ) charsMatch = TRUE;
    else
    {  //  Upper case p and s.  TODO yes i know this aint gunna fly for utf8
        if ( p >= 'a' && p <= 'z' ) p -= 32;
        if ( s >= 'a' && s <= 'z' ) s -= 32;
        charsMatch = ( p == s );
    }
    if ( charsMatch )
    {
        pattern++; patternLen--; string++; stringLen--; goto top;
    }

    return FALSE;
}

//////////////////////////////////////////////////////////////////////

B1 RemoveTrailingNameFromPath( S1_ PathAndName )

{
    S1_ LastSeperator = 0;
    for ( S1_ b = PathAndName; *b; b++ )
    {
        if ( *b == '\\' || *b == '/' )                  //  If we are at a \ or /
            if ( b[1] && b[1] != '\\' && b[1] != '/' )  //  and the next is a regular char
                LastSeperator = b;                      //  then we are at the last seperatorso far.
    }
    if ( ! LastSeperator ) return FALSE;
    *LastSeperator = 0;
    return TRUE;
}
//////////////////////////////////////////////////////////////////////

int WideFullPathAndName( ENTRY_ Entry, WCHAR *WidePathAndNameOut )

{
    //  Special case root.
    if ( ! Entry->ParentId )
    {
        WidePathAndNameOut[0] = L'\\';
        WidePathAndNameOut[1] = 0;
        return 1;
    }

    char  Buffer[1024];  //  TODO
    char* b = Buffer + 1024 - 1;  //  Start at the end of the buffer.

    //  Build the full path and name from right to left.
    *b = 0;
    do {
        //  Prepend the name.
        int NameNumBytes = Entry->NameNumBytes;
        b -= NameNumBytes;
        memcpy( b, Entry->Name, NameNumBytes );
        //  Prepend the name with a slash.
        b--;
        *b = '\\';
        //  Repeat for each parent.
        Entry = Entries[ Entry->ParentId ];
    } while ( Entry->ParentId );

    int NumWideChars = Utf8StringToWideString( b, WidePathAndNameOut );
    WidePathAndNameOut[NumWideChars] = 0;  //  Add a trailing zero.

//LogFormatted( "WideFullPathAndName is \"%S\"\n", WidePathAndNameOut );

    return NumWideChars;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


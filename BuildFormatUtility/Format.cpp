//
//  Format: An Application for formatting a volume for the Tailwind File System
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

#include <windows.h>
#include <stdio.h>

//////////////////////////////////////////////////////////////////////

#pragma warning( disable : 4101 )  //  Warning	C4101	'WriteError': unreferenced local variable	Format	C:\Users\jober\source\repos\Format\working\Format.cpp	181	
#pragma warning( disable : 4996 )  //  Error	C4996	'stricmp': The POSIX name for this item is deprecated. Instead, use the ISO C and C++ conformant name: _stricmp. See online help for details.	Format	C:\Users\jober\source\repos\Format\working\Format.cpp	278	

//  https://www.google.com/search?client=firefox-b-1-d&q=unmount+a+volume+msdn

//  FSCTL_DISMOUNT_VOLUME
//  https://learn.microsoft.com/en-us/windows/win32/debug/system-error-codes--0-499-
//  https://learn.microsoft.com/en-us/windows/win32/fileio/obtaining-file-system-recognition-information
//  https://en.wikipedia.org/wiki/Disk_formatting
//  ERROR_INVALID_NAME 123

/*

https://superuser.com/questions/1566558/is-a-boot-sector-considered-to-be-a-partition
The Wikipedia entry for Master boot record (aka "boot sector") says:
    "The MBR is not located in a partition; it is located at the first sector
    of the device (physical offset 0), preceding the first partition.
    (The boot sector present on a non-partitioned device or within an individual
    partition is called a volume boot record instead.)"
But the Wikipedia entry for Volume boot record says:
    "On partitioned devices, it is the first sector of an individual partition on the device."
https://en.wikipedia.org/wiki/Master_boot_record
https://en.wikipedia.org/wiki/Volume_boot_record
... (lots more good stuff)

https://en.wikipedia.org/wiki/Partition_type#:~:text=The%20partition%20type%20(or%20partition,mappings%2C%20LBA%20access%2C%20logical%20mapped
mbr vs gpt
old vs new


$C0'0000 : F A T 3 2 Z E R O E D 08 00 00 00 00
== 12MB

//  https://www.google.com/search?client=firefox-b-1-d&q=osr+write+raw

*/

//////////////////////////////////////////////////////////////////////

/*

Created a workable volume by:
1 ) Formatted a ( blue 32gb ) USB drive as ExFAT ( not quick == zeros unused bytes )
2 ) Ran HxD ( 2.5.0.0 ) as admin
3 ) Tools > Open Disk > uncheck "Open as Readonly"
4 ) > Physical Disk > Removable disk 1 > OK
5 ) edit      10: t a i l w i n d   i m a g e 00 00              <- tailwind
    edit     1fe: 55 bb ( from 55 aa )
    edit 10'01fe: 55 bb ( from 55 aa )
6 ) Save, exit, unmount, and reboot.
Note: after the format, $200 thru $F'FFFF were 00, $10'0000 was eb 76 90 EXFAT

Also did:
    edit    400: t a i l w i n d 00           <- tailwind
    edit    410: 1 1 1 1
    edit    420: T a i l w i n d 0 00
    edit    430: 2 2 2 2

Want to put in:

*/

//////////////////////////////////////////////////////////////////////
//
//  Typedefs
//

typedef          void        *V_;
typedef unsigned __int8  B1, *B1_;  //  One byte boolean
typedef          __int8  S1, *S1_;
typedef          __int16 S2, *S2_;
typedef          __int32 S4, *S4_;
typedef          __int64 S8, *S8_;
typedef unsigned __int8  U1, *U1_;
typedef unsigned __int16 U2, *U2_;
typedef unsigned __int32 U4, *U4_;
typedef unsigned __int64 U8, *U8_;

//////////////////////////////////////////////////////////////////////

inline void cr() { printf( "\n" ); }
inline V_   Zero( V_ Address, size_t NumBytes ) { return memset( Address, 0, NumBytes ); }

//////////////////////////////////////////////////////////////////////

DWORD Read( HANDLE FileHandle, V_ Buffer, U8 Where, U4 NumBytes )

{
    LONG  DistanceToMoveLow  = Where & 0xFFFFFFFF;
    LONG  DistanceToMoveHigh = Where >> 32;
    DWORD Result = SetFilePointer( FileHandle, DistanceToMoveLow, &DistanceToMoveHigh, FILE_BEGIN );
    if ( Result == INVALID_SET_FILE_POINTER )
    {
        DWORD SeekError = GetLastError();
        printf( "Read Seek Error is %d \n", SeekError );
        return SeekError;
    }

    unsigned long BytesRead = 0;
    BOOL ReadOK = ReadFile( FileHandle, Buffer, NumBytes, &BytesRead, NULL );
    if ( ! ReadOK )
    {
        DWORD ReadError = GetLastError();
        printf( "Read Error is %d \n", ReadError );
        return ReadError;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////

DWORD Write( HANDLE FileHandle, V_ Buffer, U8 Where, U4 NumBytes )

{
    LONG  DistanceToMoveLow  = Where & 0xFFFFFFFF;
    LONG  DistanceToMoveHigh = Where >> 32;
    DWORD Result = SetFilePointer( FileHandle, DistanceToMoveLow, &DistanceToMoveHigh, FILE_BEGIN );
    if ( Result == INVALID_SET_FILE_POINTER )
    {
        DWORD SeekError = GetLastError();
        printf( "Write Seek Error is %d \n", SeekError );
        return SeekError;
    }

    unsigned long BytesWritten = 0;
    BOOL WriteOK = WriteFile( FileHandle, Buffer, NumBytes, &BytesWritten, NULL );
    if ( ! WriteOK )
    {
        DWORD WriteError = GetLastError();
        printf( "Write Error is %d \n", WriteError );
        if ( WriteError == 5 ) printf( "5 == ERROR_ACCESS_DENIED \n" );
        return WriteError;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////

void Dump( const void* DataAddressAsVoid,  U8 DisplayAddress, int NumBytes, const char* Before, const char* After )

{
    U1_ DataAddress    = ( U1_ ) DataAddressAsVoid;
    while ( NumBytes > 0 )
    {
        //               "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................";
        char  Output[] = "                                                                 ";
        for ( int Column = 0; Column < min( 16, NumBytes ); Column++ )
        {
            U1 Byte = DataAddress[Column];
            Output[3*Column  ] = "0123456789ABCDEF"[Byte>>4];
            Output[3*Column+1] = "0123456789ABCDEF"[Byte&0xf];
            Output[3*16 + 1 + Column] = ( Byte >= ' ' && Byte <= '~' ) ? Byte : '.';
        }

        printf( "%s %p : %s %s\n", Before, ( V_ ) DisplayAddress, Output, After );

        NumBytes       -= 16;
        DataAddress    += 16;
        DisplayAddress += 16;
    }
}

//////////////////////////////////////////////////////////////////////

int main( int argc, char *argv[] )

{
    if ( argc != 2 )
    {
        printf( "\n\n" );
        for ( int a = 0; a < argc; a++ ) printf( "%d %s \n", a, argv[a] );
        printf( "syntax is %s <drive>   (ex) E:\n", argv[0] );
        printf( "\n\n" );
        return __LINE__;
    }
    char* Volume = argv[1];
    if ( ( Volume[0] == 'c' || Volume[0] == 'C' ) && ( Volume[1] == ':' || Volume[0] == ':' ) )
    {
        printf( "Modifying C: is not allowed.\n" );
        return __LINE__;
    }
    char Volume2[256];
    sprintf( Volume2, "\\\\.\\%s", Volume );

    HANDLE FileHandle;
    BOOL Status = TRUE;

    FileHandle = CreateFileA( Volume2,  //  "\\\\.\\e:",
        ( GENERIC_READ | GENERIC_WRITE ),
        ( FILE_SHARE_READ | FILE_SHARE_WRITE ),
        NULL,
        OPEN_EXISTING,
        0,
        NULL );
    if ( FileHandle == INVALID_HANDLE_VALUE )
    {
        DWORD OpenError = GetLastError();
        printf( "Open error is %d \n",OpenError );
        return __LINE__;
    }

    DWORD ReadError, WriteError;


    B1 Us;
    int NumFilled;
    char Proceed[80] = {0};
    unsigned char Buffer_000000[512];
    unsigned char Buffer_000400[512];
    unsigned char Buffer_100000[512];

    ReadError = Read( FileHandle, Buffer_000000, 0x000000, 512 ); if ( ReadError ) goto close;
    ReadError = Read( FileHandle, Buffer_000400, 0x000400, 512 ); if ( ReadError ) goto close;
    ReadError = Read( FileHandle, Buffer_100000, 0x100000, 512 ); if ( ReadError ) goto close;
    cr();

    Dump( Buffer_000000 + 0x000, 0x10, 16, "000000", "FS Name?" );
    Dump( Buffer_000000 + 0x010, 0x10, 16, "000000", "FS Name?" );
    Dump( Buffer_000000 + 0x1FE, 0x1FE, 2, "000000", "55 aa for others   55 BB for us?" );
    Dump( Buffer_100000 + 0x1FE, 0x1FE, 2, "100000", "55 aa for others   55 BB for us?" );
    cr();
    Dump( Buffer_000400 + 0x000, 0x400, 16, "000400", "a l p h a 00 ?" );
    Dump( Buffer_000400 + 0x010, 0x410, 16, "000400", "1 1 1 1 ?" );
    Dump( Buffer_000400 + 0x020, 0x420, 16, "000400", "A l p h a 0 00 ?" );
    Dump( Buffer_000400 + 0x030, 0x430, 16, "000400", "2 2 2 2 ?" );
    cr();

    Us =	
            0 == memcmp( Buffer_000000 + 0x03, "TAILWIND"  , 8 ) &&
            Buffer_000000[0x1FE] == 0x55 &&
            Buffer_000000[0x1FF] == 0xBB &&
            0 == memcmp( Buffer_000400 + 0x00, "tailwind\0\0\0\0\0\0\0\0"     , 16 ) &&
            0 == memcmp( Buffer_000400 + 0x10, "1111\0\0\0\0\0\0\0\0\0\0\0\0" , 16 ) &&
            0 == memcmp( Buffer_000400 + 0x20, "Tailwind0\0\0\0\0\0\0\0"      , 16 ) &&
            0 == memcmp( Buffer_000400 + 0x30, "2222\0\0\0\0\0\0\0\0\0\0\0\0" , 16 ) &&
            Buffer_100000[0x1FE] == 0x55 &&
            Buffer_100000[0x1FF] == 0xBB &&
            1;


    if ( Us ) printf( "Looks like us.\n" );
    else      printf( "Not us.\n" );


    printf( " ALL DATA will be lost from %s !\nProceed? ( yes or no )\n", Volume );
    NumFilled = scanf( "%79s", Proceed );
    if ( NumFilled != 1 )
    {
        printf( "Input buffer error.\n" );
        return __LINE__;
    }
    if ( stricmp( Proceed, "yes" ) )
    {
        printf( "Format canceled.\n" );
        return __LINE__;
    }


    printf( "\n\nGood to Go.\n\n\n" );

    Zero( Buffer_000000, 512 );
    Zero( Buffer_000400, 512 );
    Zero( Buffer_100000, 512 );

    memcpy( Buffer_000000 + 0x03, "TAILWIND"  , 8 );
    Buffer_000000[0x1FE] = 0x55;
    Buffer_000000[0x1FF] = 0xBB;
    memcpy( Buffer_000400 + 0x00, "tailwind\0\0\0\0\0\0\0\0"     , 16 );
    memcpy( Buffer_000400 + 0x10, "1111\0\0\0\0\0\0\0\0\0\0\0\0" , 16 );
    memcpy( Buffer_000400 + 0x20, "Tailwind0\0\0\0\0\0\0\0"      , 16 );
    memcpy( Buffer_000400 + 0x30, "2222\0\0\0\0\0\0\0\0\0\0\0\0" , 16 );
    Buffer_100000[0x1FE] = 0x55;
    Buffer_100000[0x1FF] = 0xBB;


    {
        DWORD WriteErrorA = Write( FileHandle, Buffer_000000, 0x000000, 512 );
        DWORD WriteErrorB = Write( FileHandle, Buffer_100000, 0x100000, 512 );
        DWORD WriteErrorC = Write( FileHandle, Buffer_000400, 0x000400, 512 );
        if ( WriteErrorA && WriteErrorB && WriteErrorC )
        {
            printf( "All formatting writes failed!\n" );
            printf( "%s is probably unaffected by this format attempt.\n", Volume );
            goto close;
        }
        if ( WriteErrorA || WriteErrorB || WriteErrorC )
        {
            printf( "One or more formatting writes failed!\n" );
            printf( "%s is not formatted correctly.\n", Volume );
            goto close;
        }
    }


    printf( "\n\nFormat complete.\n\n\n" );


  close:
    CloseHandle( FileHandle );

    return 0;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


//
//  Chat: An Application for communicating with the Tailwind File System
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

#define TAILWIND_BASE 0x800
#define FSCTL_TAILWIND_CHAT  CTL_CODE( FILE_DEVICE_FILE_SYSTEM, TAILWIND_BASE+1, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define equals( a, b ) ( strcmp( ( a ), ( b ) ) == 0 )

LARGE_INTEGER PerformanceCounterFrequency;

//////////////////////////////////////////////////////////////////////
//
//  Decoding I/O Control Codes
//  https://social.technet.microsoft.com/wiki/contents/articles/24653.decoding-io-control-codes-ioctl-fsctl-and-deviceiocodes-with-table-of-known-values.aspx
//
//  Defining I/O Control Codes
//  https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/defining-i-o-control-codes
//
//////////////////////////////////////////////////////////////////////

DWORD Call_DeviceIoControl( const char* FileName, DWORD ControlCode, V_ In, DWORD InSize, V_ Out, DWORD OutSize, DWORD* ReturnedSize_ )

{
	HANDLE Handle = CreateFileA( FileName,
			FILE_READ_ATTRIBUTES | FILE_READ_DATA,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if ( Handle == INVALID_HANDLE_VALUE ) return GetLastError();

    BOOL OK = DeviceIoControl( Handle, ControlCode, In, InSize, Out, OutSize, ReturnedSize_, NULL );
	DWORD Err = 0;
	if ( ! OK ) Err = GetLastError();

	CloseHandle( Handle );

	return Err;
}

//////////////////////////////////////////////////////////////////////

void PrintErr( DWORD Err )

{
	char Message[256];
	DWORD Flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
	FormatMessageA( Flags, NULL, GetLastError(), MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), Message, 256, NULL );
	printf( "$%04X : %s", ( int ) Err, Message );
}

//////////////////////////////////////////////////////////////////////

void Input( S1_ Buffer )

{
    int i = 0;

	for ( ; i < 255; i++ )
	{
		int c = getchar();
		if ( c == EOF || c == '\n' ) break;
        Buffer[i] = c;
    }

    Buffer[i] = '\0';
}

//////////////////////////////////////////////////////////////////////

int main()

{
	S1_   In;
	DWORD InSize;
	S1    Out[2560];
	DWORD OutSize = sizeof( Out );
	DWORD ReturnedSize;
	DWORD Err;

	QueryPerformanceFrequency( &PerformanceCounterFrequency );

	S1 User[256];
	for ( ;; )
	{
		printf( "\n: " );
		Input( User );
		if ( equals( User, "exit" ) ) break;
		if ( equals( User, "quit" ) ) break;

		In = User;
		InSize = ( U4 ) strlen( In ) + 1;
		Err = Call_DeviceIoControl( "\\\\.\\E:\\", FSCTL_TAILWIND_CHAT, In, InSize, Out, OutSize, &ReturnedSize );
		if ( Err )
		{
			PrintErr( Err );
		}
		else
		{
			Out[OutSize-1] = 0;
			printf( "%s\n", Out );
		}
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


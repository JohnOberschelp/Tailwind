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

#if defined(P) || defined(L) || defined(R) || defined(SetP) || defined(SetL) || defined(SetR)
static_assert(1, "need these to be undefined here");
#endif

#pragma warning(disable : 4706)  //  (ex) if (a = b)

#define P(Id) ( Entries[Id]->SiblingTreeParentId )
#define L(Id) ( Entries[Id]->SiblingTreeLeftId   )
#define R(Id) ( Entries[Id]->SiblingTreeRightId  )

#define SetP(Id,p) ( Entries[Id]->SiblingTreeParentId = p )
#define SetL(Id,l) ( Entries[Id]->SiblingTreeLeftId   = l )
#define SetR(Id,r) ( Entries[Id]->SiblingTreeRightId  = r )

//--------------------------------------------------------------------

inline void entryRotateL(ID_ rootHandle, ID x)

{
    ID y = R(x), p = P(x);
    if (SetR(x,L(y))) SetP(L(y), x);
    if (!SetP(y,p)) *rootHandle = y;
    else
    {
        if (x == L(p)) SetL(p,y);
        else           SetR(p,y);
    }
    SetP(SetL(y,x),y);
}

//--------------------------------------------------------------------

inline void entryRotateR(ID *rootHandle, ID y)

{
    ID x = L(y), p = P(y);
    if (SetL(y,R(x))) SetP(R(x),y);
    if (!SetP(x,p)) *rootHandle = x;
    else
    {
        if (y == L(p)) SetL(p, x);
        else           SetR(p, x);
    }
    SetP(SetR(x, y), x);
}

//--------------------------------------------------------------------

static void entrySplay(ID *rootHandle, ID x)

{
    ID p;
    while (p = P(x))
    {
        if (!P(p))
        {
            if (L(p) == x) entryRotateR(rootHandle, p);
            else           entryRotateL(rootHandle, p);
        }
        else
        {
            if (p == R(P(p)))
            {
                if (R(p) == x) entryRotateL(rootHandle, P(p));
                else           entryRotateR(rootHandle, p);
                entryRotateL(rootHandle, P(x));
            }
            else
            {
                if (L(p) == x) entryRotateR(rootHandle, P(p));
                else           entryRotateL(rootHandle, p);
                entryRotateR(rootHandle, P(x));
            }
        }
    }
}

//--------------------------------------------------------------------

inline int entrySeek(ID *rootHandle, ID *resultHandle, S1_ Name )

{
    int difference;
    ID x = *rootHandle;
    for (;;)
    {

        difference = _stricmp( Name, Entries[x]->Name );

        if (difference < 0)
        {
            ID l = L(x);
            if (!l) break;
            x = l;
        }
        else
        if (difference > 0)
        {
            ID r = R(x);
            if (!r) break;
            x = r;
        }
        else break;
    }

    *resultHandle = x;
    return difference;
}

//--------------------------------------------------------------------

ID EntryNext( ID x )

{
    ID l,p;
    if (R(x)) { x = R(x); while (l = L(x)) x = l; return x; }
    p = P(x);
    while (p && x == R(p)) { x = p; p = P(p); }
    return p;
}

//--------------------------------------------------------------------

ID EntryPrev( ID x )

{
    ID r,p;
    if (L(x)) { x = L(x); while (r = R(x)) x = r; return x; }
    p = P(x);
    while (p && x == L(p)) { x = p; p = P(p); }
    return p;
}

//--------------------------------------------------------------------

ID EntryFirst( ID x )

{
    ID l;
    if (x) while (l = L(x)) x = l;
    return x;
}

//--------------------------------------------------------------------

ID EntryLast( ID x )

{
    ID r;
    if (x) while (r = R(x)) x = r;
    return x;
}

//--------------------------------------------------------------------

ID EntryFind(ID *rootHandle, S1_ Name )

{
    if (! *rootHandle) return 0;
    ID x;
    int direction = entrySeek(rootHandle, &x, Name );
    entrySplay(rootHandle, x);
    return direction?0:x;
}

//--------------------------------------------------------------------

ID EntryNear(ID *rootHandle, S1_ Name, NEIGHBOR want)

{
    //  Return a node relative to (<, <=, ==, >=, or >) a key.
    if (! *rootHandle) return 0;
    ID x;
    int dir = entrySeek(rootHandle, &x, Name );  //  NearVolumeAddress);
    if ((dir == 0 && want == GT) || (dir > 0 && want >= GE)) x = EntryNext( x) ;
    else
    if ((dir == 0 && want == LT) || (dir < 0 && want <= LE)) x = EntryPrev( x );
    else
    if (dir != 0 && want == EQ) x = 0;


    //  Since Near is likely to be used to find where to attach (?),
    //  let's not splay on Near calls!?!  TODO?
    // if (x) entrySplay(rootHandle, x);


    return x;
}

//--------------------------------------------------------------------

int EntryAttach(ID *rootHandle, ID x)

{

    SetL(x,0); SetR(x,0);
    if (! *rootHandle)
    {
        *rootHandle = x;
        SetP(x,0);
    }
    else
    {
        ID f;

        int diff = entrySeek(rootHandle, &f, Entries[x]->Name );

        if (!diff) return 0;
        if (diff < 0) SetL(f, x); else SetR(f,x);
        SetP(x, f);
        entrySplay(rootHandle, x);
    }
    return 1;
}

//--------------------------------------------------------------------

void EntryDetach(ID *rootHandle, ID x)

{
    ID p = P(x);
    if (!L(x))
    {
        if (p) { if ( L(p) == x) SetL(p, R(x)); else SetR(p, R(x)); }
        else *rootHandle = R(x);
        if (R(x)) SetP(R(x), p);
    }
    else if (!R(x))
    {
        if (p) { if ( L(p) == x) SetL(p, L(x)); else SetR(p, L(x)); }
        else *rootHandle = L(x);
        if (L(x)) SetP(L(x), p);
    }
    else
    {
        ID y = L(x);
        if (R(y))
        {
            do { y = R(y); } while (R(y));
            if (SetR(P(y) , L(y))) SetP(L(y) , P(y));
            SetP((SetL(y , L(x))), y);
        }
        SetP((SetR(y , R(x))) , y);
        if (SetP(y , P(x))) { if (L(P(y)) == x) SetL(P(y) , y); else SetR(P(y) , y); }
        else *rootHandle = y;
    }
}

//--------------------------------------------------------------------

#undef P
#undef L
#undef R
#undef SetP
#undef SetL
#undef SetR

#pragma warning(default : 4706)  //  (ex) if (a = b)

//////////////////////////////////////////////////////////////////////

ID GetID()

{
    ID Id;
    if ( Volume_WhereTableLastRecycledID )
    {
        Id = Volume_WhereTableLastRecycledID;
        Volume_WhereTableLastRecycledID = ( ID ) ( U8 ) Entries[Volume_WhereTableLastRecycledID];
    }
    else
    {
        if ( Volume_WhereTableFirstUnusedBottomID >= Volume_WhereTableMaxId )
        {
ASSERT( 0 );
            return 0;  //  No more IDs.
        }
        Id = Volume_WhereTableFirstUnusedBottomID;
        Volume_WhereTableFirstUnusedBottomID++;
    }

    Volume_TotalNumberOfEntries++;

    return Id;
}

//////////////////////////////////////////////////////////////////////

void RecycleID( ID Id )

{
    Entries[Id] = ( ENTRY_ ) ( U8 ) Volume_WhereTableLastRecycledID;
    Volume_WhereTableLastRecycledID = Id;

    Volume_TotalNumberOfEntries--;

LogFormatted( "Just recycled %d. Volume_TotalNumberOfEntries now %d\n", Id, Volume_TotalNumberOfEntries );
}

//////////////////////////////////////////////////////////////////////

ID MakeEntry( ID ParentId, U4 DirectoryOrNumRanges, char* Name )

{
    size_t NewNameLen = strlen( Name );
    size_t size = offsetof( ENTRY, Name ) + NewNameLen + 1;
    U4 N = DirectoryOrNumRanges;
    if ( N == A_DIRECTORY ) size += sizeof( ID );
    else                    size += offsetof( FILE_DATA, DataRange ) + N * sizeof( DATA_RANGE );

    ENTRY_ Entry = EntriesAllocate( ( U4 ) size );

    if ( ! Entry ) return 0;

ASSERT( NewNameLen < 255 );
    Entry->NameNumBytes = ( U1 ) NewNameLen;
    strcpy( Entry->Name, Name );
    NTSTATUS Status = 0;
    if ( Status )
    {
        FreeMemory( Entry );
        return 0;
    }

    if ( N == A_DIRECTORY ) Entry->FileAttributes = FILE_ATTRIBUTE_DIRECTORY;
    else                    Entry->FileAttributes = FILE_ATTRIBUTE_NORMAL;  //  TODO temp should use OrNormal, right?


    //  Pick out an Entry ID.
    ID Id = GetID();
    if ( ! Id )
    {
ASSERT( 0 );
        return 0;
    }

    Entry->Id = Id;
    Entries[Id] = Entry;

    if ( ParentId ) AttachEntry( Entries[ParentId], Entry );

    return Id;
}

//////////////////////////////////////////////////////////////////////

U4 EntrySize( ENTRY_ Entry )

{
    size_t Size  = offsetof( ENTRY, Name ) + Entry->NameNumBytes + 1;
    if ( EntryIsADirectory( Entry ) )
    {
        Size += sizeof( ID );
    }
    else
    {
        FILE_DATA_ FileData  = Data( Entry );
        Size += offsetof( FILE_DATA, DataRange ) + FileData->NumRanges * sizeof( DATA_RANGE );
    }
    return ( U4 ) Size;
}

//////////////////////////////////////////////////////////////////////

NTSTATUS ResizeEntry( ID Id, S1_ NewNameOrZero, S4 DeltaToNumRanges )

{
    B1 D = IdIsADirectory( Id );

    ENTRY_     OldEntry     = Entries[Id];
    U1         OldNameLen   = OldEntry->NameNumBytes;
    FILE_DATA_ OldFileData  = D ? 0 : Data( OldEntry );
    U4         OldNumRanges = D ? 0 : OldFileData->NumRanges;
    U4         OldSize      = EntrySize( OldEntry );

    U1         NewNameLen   = ( U1 ) ( NewNameOrZero ? strlen( NewNameOrZero ) : OldNameLen );
    U4         NewNumRanges = OldNumRanges + DeltaToNumRanges;

    int NumRangeBytesAdding = D ? 0 : DeltaToNumRanges * sizeof( DATA_RANGE );
    int NumNameBytesAdding  = NewNameLen - OldNameLen;
    int NumBytesAdding      = NumNameBytesAdding + NumRangeBytesAdding;
    if ( NumBytesAdding > ( int ) EntriesNumBytesAvailable )
        return STATUS_INSUFFICIENT_RESOURCES;

    U4 NewSize = ( U4 ) ( OldSize + NumBytesAdding );


    //  Can we stay in place?


    //  Yes, if we don't need to resize at all.
    if ( NewNameLen == OldNameLen && ! DeltaToNumRanges )
    {
        if ( NewNameOrZero ) strcpy( OldEntry->Name, NewNameOrZero );
        return 0;
    }


    //  Yes, if we are just shedding some ranges.
    if ( NewNameOrZero == 0 && DeltaToNumRanges < 0 )
    {
        U4_ AddressOfShedding = ( U4_ ) ( ( ( U1_ ) OldFileData )
                              + offsetof( FILE_DATA, DataRange )
                              + NewNumRanges * sizeof( DATA_RANGE ) );
        OldFileData->NumRanges = NewNumRanges;
        AddressOfShedding[0] = 0;                                   //  Marker for unused
        AddressOfShedding[1] = ( U4 ) ( 0 - NumRangeBytesAdding );  //  Number of unused
        return 0;
    }


    //  Yes, if we are the last entry and we are just adding some ranges.
    if ( NewNameOrZero == 0 && DeltaToNumRanges > 0 )
    {
        U1_ AddressOfEndOfEntry    = ( U1_ ) OldEntry + OldSize;
        U1_ AddressOfFirstFreeByte = EntriesBytes + EntriesFirstFreeByte;
        B1 WeAreLast = AddressOfEndOfEntry == AddressOfFirstFreeByte;
        if ( WeAreLast )
        {
            OldFileData->NumRanges = NewNumRanges;
            EntriesFirstFreeByte     += ( U4 ) NumRangeBytesAdding;
            EntriesNumBytesAvailable -= ( U4 ) NumRangeBytesAdding;
            Zero( &OldFileData->DataRange[OldNumRanges], NumRangeBytesAdding );
            return 0;
        }
    }


    ENTRY_ NewEntry = EntriesAllocate( ( U4 ) NewSize );
ASSERT( NewEntry );

    if ( NewNameOrZero )
    {
        memcpy( NewEntry, OldEntry, offsetof( ENTRY, NameNumBytes ) );
        NewEntry->NameNumBytes = NewNameLen;
        strcpy( NewEntry->Name, NewNameOrZero );
    }
    else
    {
        memcpy( NewEntry, OldEntry, offsetof( ENTRY, Name ) + OldNameLen + 1 );
    }

    if ( D )
    {
        *ChildrenTree_( NewEntry ) = ChildrenTree( OldEntry );
    }
    else
    {
        FILE_DATA_ NewFileData  = Data( NewEntry );
        U4 MinNumRanges = min( OldNumRanges, NewNumRanges );
        U4 NumExtraToCopy = offsetof( FILE_DATA, DataRange ) + MinNumRanges * sizeof( DATA_RANGE );
        memcpy( NewFileData, OldFileData, NumExtraToCopy );
        NewFileData->NumRanges = NewNumRanges;
    }


    Entries[Id] = NewEntry;

    EntriesFree( OldEntry );

    return 0;
}

//////////////////////////////////////////////////////////////////////

void UnmakeEntry( ID Id )

{
    if ( Entries[Id]->ParentId ) DetachEntry( Entries[Id] );

ASSERT( ! HasChildren( Id ) );

    if ( IdIsAFile( Id ) )
    {
        NTSTATUS Status = DataResizeFile( Id, 0, DONT_FILL );
ASSERT( ! Status );
        Status = DataReallocateFile( Id, 0 );
ASSERT( ! Status );
    }

    EntriesFree( Entries[Id] );

    RecycleID( Id );
}

//////////////////////////////////////////////////////////////////////

NTSTATUS FindByPathAndName( ID StartId, S1_ PathAndName, S1_ *NameOut, ID *ParentIdOut, ID *EntryIdOut )

{
    //  Let's work on a private copy so we can clobber bytes for our evil ways.
    char ParsedName[MAX_PATH];
    CopyString( ParsedName, MAX_PATH, PathAndName );


    //  Skip any leading seperators.
    S1_ fm = ParsedName;
    while ( *fm == '/' || *fm == '\\' ) fm++;


    //  Special case root.
    if ( *fm == 0 )
    {
        if ( NameOut )   *NameOut   = PathAndName;
        if ( ParentIdOut ) *ParentIdOut = StartId;
        *EntryIdOut = StartId;
        return 0;
    }


    //  From the Start, for each ancestor...
    ID AncestorId = StartId;
    for ( ;; )
    {
        //  Isolate the next base name.
        S1_ to = fm;
        while ( *to != 0 && *to != '/' && *to != '\\' ) to++;   //  Advance to the next seperator.
        while ( *to == '/' || *to == '\\' ) { *to = 0; to++; }  //  Stomp any trailing seperators.


//  TODO process "." and ".." within a path ! ?
if ( fm[0] == '.' && fm[1] == 0 )  //  .
{
BreakToDebugger();
}
if ( fm[0] == '.' && fm[1] == '.' && fm[2] == 0 )  //  ..
{
BreakToDebugger();
}


        //  TODO process pattern characters ? !	
        //  https://community.osr.com/discussion/133381/irp-mj-create-with-wild-card-in-the-name
        for ( S1_ at = fm; *at; at++ )
        {
            if ( *at == '*' )
            {
//BreakToDebugger();
                return STATUS_OBJECT_NAME_INVALID;
            }
            if ( *at == '?' )
            {
BreakToDebugger();
                return STATUS_OBJECT_NAME_INVALID;
            }
        }


        //  Break out if it is the name, and not part of the path.
        if ( *to == 0 ) break;


        //  Find it.
        ID Id =  EntryFind( ChildrenTree_( Entries[AncestorId] ), fm );
        if ( ! Id )
        {
            if ( ParentIdOut ) *ParentIdOut = 0;
            *EntryIdOut = 0;
            return STATUS_OBJECT_PATH_NOT_FOUND;
        }

        AncestorId = Id;

        if ( IdIsAFile( AncestorId ) )
        {
            if ( ParentIdOut ) *ParentIdOut = 0;
            *EntryIdOut = 0;
            return STATUS_NOT_A_DIRECTORY;
        }

        fm = to;
    }

    if ( NameOut )
        *NameOut = PathAndName + ( ( ( UINT_PTR )fm - ( UINT_PTR )ParsedName  ) );

    if ( ParentIdOut )
        *ParentIdOut = AncestorId;

    //  Find the directory entry, if it exists.
    ID FoundId = EntryFind( ChildrenTree_( Entries[AncestorId] ), fm );
    if ( ! FoundId )
    {
        *EntryIdOut = 0;
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }
    *EntryIdOut = FoundId;
    return 0;
}

//////////////////////////////////////////////////////////////////////

void AttachEntry( ENTRY_ Parent, ENTRY_ Entry )

{
    EntryAttach( ChildrenTree_( Parent ), Entry->Id );

    Entry->ParentId = Parent->Id;
}

//////////////////////////////////////////////////////////////////////

void DetachEntry( ENTRY_ Entry )

{
    ENTRY_ Parent = Entries[ Entry->ParentId ];

    EntryDetach( ChildrenTree_( Parent ), Entry->Id );
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


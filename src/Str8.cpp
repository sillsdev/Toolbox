// Str8.cpp String class for UTF8 and 8 bit legacy encoded _data

// #include "stdafx.h"
#include "generic.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "Str8.h"
#include <cassert>

#undef GetChar

#ifndef ASSERT
#define ASSERT Str8Assert
#endif

BOOL Str8Assert( BOOL b )
	{
	return TRUE;
	}

void Str8Validate()
	{
	}

#define iStr8_Malloc_Increment 32 // 1.4ytb Change from 16 to 32 to make Str8 grow faster for higher speed

static BOOL bSpace( const char c ) // 1.4qzjh Fix U problem of jump not including all letters
	{
	return ( c == ' ' || c == '\n' );
	}

static int iMallocSize( int iSize )
	{
	int iMallocUnits = iSize / iStr8_Malloc_Increment + 1;
	return iMallocUnits * iStr8_Malloc_Increment;
	}

void Str8::Init()
	{
#ifdef UseCharStar
    int iMallocSiz = iMallocSize(0);
    m_psz = (char*)malloc( iMallocSiz ); // Allocate string buffer
    m_iAlloc = iMallocSiz; // Init allocated size
    m_iLen = 0; // Init length
    *m_psz = '\0'; // Terminate string
#else
    _data.clear();       // empty the string
    _data.shrink_to_fit(); // optional: release any extra capacity
#endif
	}

void Str8::MakeSpace( int iSize ) // Make room for possibly larger size
	{
#ifdef UseCharStar
	if ( iSize + 1 > m_iAlloc ) // If new size larger than allocated, move to bigger space
		{
		char* pszOld = m_psz; // Remember old place
		int iMallocSiz = iMallocSize( iSize * 2 ); // 1.4ytb Double to make Str8 grow faster for higher speed
		m_psz = (char*)malloc( iMallocSiz ); // Allocate string buffer
		m_iAlloc = iMallocSiz; // Init allocated size
		strcpy_s( m_psz, iMallocSiz, pszOld ); // Copy old to new place
		free( pszOld ); // Free old place
		}
#else
        // Ensure there's enough capacity for iSize characters (+1 for null terminator)
        if (static_cast<size_t>(iSize + 1) > _data.capacity())
        {
            // Grow roughly twice as large (to preserve the original intent)
            _data.reserve(iSize * 2);
        }
#endif
	}

#ifndef UseCharStar
Str8::Str8()
    : _data()  // default construct empty string
{
}

Str8::Str8(const char* pszInit, int iCount)
{
    if (!pszInit)
        pszInit = "";

    _data = pszInit;   // handles allocation automatically

    if (iCount >= 0 && static_cast<size_t>(iCount) < _data.size())
        _data.resize(iCount);  // truncate if needed
}

Str8::Str8(const Str8& other)
    : _data(other._data)
{
}

int Str8::GetLength() const // Length
{
    AssertValid();
    return _data.size();
}

void Str8::AssertValid() const noexcept // Assert that all is well
{
#ifndef NDEBUG
    assert(_data.c_str()[_data.size()] == '\0');  // sanity check
#endif
}

Str8& Str8::operator=(const Str8& sSource)
{
    _data = sSource._data;
    AssertValid();
    return *this;
}

Str8& Str8::operator=(const char* pszSource)
{
    _data = pszSource ? pszSource : "";
    AssertValid();
    return *this;
}

Str8 Str8::Mid(int iStart, int iCount) const
{
    if (iStart < 0)
        iStart = 0;
    if (iStart > static_cast<int>(_data.size()))
        iStart = static_cast<int>(_data.size());

    if (iCount < 0 || iCount > static_cast<int>(_data.size()) - iStart)
        iCount = static_cast<int>(_data.size()) - iStart;

    Str8 s;
    s._data = _data.substr(iStart, iCount);
    return s;
}

Str8& Str8::operator+=(const char c) // 1.4qzkb Add Str8 += char to prevent crash from default behaviour
{
    _data.push_back(c);
    return *this;
    AssertValid();
    return *this;
}

Str8& Str8::operator+=(const Str8& sSource)
{
    _data += sSource._data;
    AssertValid();
    return *this;
}

Str8& Str8::operator+=(const char* pszSource)
{
    if (!pszSource)
        pszSource = "";

    _data += pszSource;
    AssertValid();
    return *this;
}

Str8& Str8::operator+=(int iAdd) // 1.4tec Add a number, if numeric
{
    int i = std::atoi(_data.c_str());
    i += iAdd;

    _data += std::to_string(i);  // convert and append
    return *this;
}

void Str8::Truncate(int iCount) // Cut off end at iCount
{
    if (iCount < 0)
        iCount = 0;
    if (iCount < static_cast<int>(_data.size()))
        _data.resize(iCount);
}

int Str8::Find(const char c, int iStart) const
{
    if (iStart < 0 || iStart >= static_cast<int>(_data.size()))
        return -1;

    size_t pos = _data.find(c, static_cast<size_t>(iStart));
    return pos == std::string::npos ? -1 : static_cast<int>(pos);
}

int Str8::Find(const char* psz, int iStart) const
{
    if (!psz)
        return -1;
    if (iStart >= static_cast<int>(_data.size()))
        return -1;

    size_t pos = _data.find(psz, iStart);
    return (pos == std::string::npos) ? -1 : static_cast<int>(pos);
}

char* Str8::GetBuffer(int iSize) // Write access to buffer
{
    _data.resize(iSize);  // ensures buffer has at least iSize chars
    AssertValid();
    return _data.data();  // writable buffer
}

void Str8::ReleaseBuffer(int iLen) // Release buffer after writing
{
    if (iLen >= 0 && iLen < static_cast<int>(_data.size()))
        _data.resize(iLen);
    else
        _data.resize(std::strlen(_data.c_str()));  // recalc if unknown length
    AssertValid();
}

void Str8::SetAt(int iPos, const char c)
{
    if (iPos < 0 || iPos >= static_cast<int>(_data.size()))
        return;
    _data[iPos] = c;
    AssertValid();
}

// redo
int Str8::ReverseFind(const char c) const
{
    size_t pos = _data.rfind(c);
    return (pos == std::string::npos) ? -1 : static_cast<int>(pos);
}

void Str8::Replace(const char* pszFrom, const char* pszTo, BOOL bFeed) // 1.4vyt 
{
    if (!pszFrom || !pszTo)
        return;

    const std::string from(pszFrom);
    const std::string to(pszTo);

    if (from.empty())
        return;

    size_t start = 0;
    while (true)
    {
        size_t pos = _data.find(from, start);
        if (pos == std::string::npos)
            break;

        _data.replace(pos, from.length(), to);
        start = bFeed ? pos : pos + to.length();
    }
    AssertValid();
}

Str8::Str8(const char c) // Constructor from char
{
    _data.assign(1, c);
    AssertValid();
}

void Str8::TrimLeft()
{
    size_t first = _data.find_first_not_of(" \t\r\n");
    if (first != std::string::npos)
        _data.erase(0, first);
    else
        _data.clear();
    AssertValid();
}

void Str8::TrimRight()
{
    size_t last = _data.find_last_not_of(" \t\r\n");
    if (last != std::string::npos)
        _data.erase(last + 1);
    else
        _data.clear();
    AssertValid();
}

void Str8::Insert(int iPos, const char* psz)
{
    if (!psz)
        return;

    if (iPos < 0)
        iPos = 0;
    if (iPos > static_cast<int>(_data.size()))
        iPos = static_cast<int>(_data.size());

    _data.insert(static_cast<size_t>(iPos), psz);
    AssertValid();
}

void Str8::Delete(int iPos, int iCount)
{
    if (iPos < 0)
        iPos = 0;
    if (iCount < 0)
        iCount = 0;

    if (iPos >= static_cast<int>(_data.size()))
        return;

    _data.erase(static_cast<size_t>(iPos), static_cast<size_t>(iCount));
    AssertValid();
}

Str8& Str8::Append(const char* psz) // Append string
{
    _data.append(psz ? psz : "");
    AssertValid();
    return *this;
}

int Str8::FindAtEndOfWord(const char* psz, int iStart) const // 1.4ytc Add find at end of word (followed by sp or nl)
{
    Str8 sFind = psz;
    Str8 sSp = sFind + " ";
    Str8 sNl = sFind + "\n";
    int iFindSp = Find(sSp, iStart); // Find first with space
    int iFindNl = Find(sNl, iStart); // Find first with nl
    if (iFindSp < 0) // If none found with space, return find with nl (maybe no find of either)
        return iFindNl;
    if (iFindNl < 0) // If none found with nl, return find with sp
        return iFindSp;
    if (iFindSp < iFindNl) // If both found, return earliest
        return iFindSp;
    else
        return iFindNl;
}
#else
Str8::Str8() // Default constructor // 1.4qzfv Start Str8
	{
	Init();
	AssertValid();
	}

Str8::Str8( const char* pszInit, int iCount ) // Constructor with initializing string
	{
	if ( !pszInit ) // 1.4qzpf Don't crash if null passed in as empty string
		pszInit = "";
	Init();
	int iLen = strlen( pszInit );
	MakeSpace( iLen ); // Init to large enough size (= m_iAlloc)
	strcpy_s( m_psz, m_iAlloc, pszInit ); // Copy initializing string to buffer
	m_iLen = iLen;
	if ( iCount >= 0 ) // If limited count, then shorten to that length
		Truncate( iCount );
	AssertValid();
	}

Str8::Str8( const Str8& s ) // Copy constructor
	{
	Init();
	MakeSpace( s.GetLength() );
	strcpy_s( m_psz, m_iAlloc, (const char*)s ); // Copy initializing string to buffer (= m_iAlloc)
	m_iLen = s.GetLength();
	AssertValid();
	}

Str8::Str8(const char c) // Constructor from char
{
    Init(); // Init to large enough size
    MakeSpace(1);
    *m_psz = c;
    *(m_psz + 1) = '\0';
    m_iLen = 1;
    AssertValid();
}

Str8::~Str8() // Destructor
	{
	AssertValid();
	free( m_psz ); // Free the string buffer
	}

void Str8::AssertValid() const // Assert that all is well
{
    ASSERT(m_iLen >= 0); // Length greater than zero
    ASSERT(m_iLen < m_iAlloc); // Length less than alloc size (allows for terminating null)
    ASSERT(*(m_psz + m_iLen) == '\0'); // Terminating null at length
}

int Str8::GetLength() const // Length
{
    AssertValid();
    return m_iLen;
}

Str8::operator const char* () const  // Read access to buffer
{
    AssertValid();
    return m_psz;
}

Str8& Str8::operator=(const char* pszSource)
{
    if (!pszSource) // 1.4qzpf Don't crash if null passed in as empty string
        pszSource = "";
    int iNewLen = strlen(pszSource);
    MakeSpace(iNewLen);
    m_iLen = iNewLen;
    strcpy_s(m_psz, m_iAlloc, pszSource);
    AssertValid();
    return *this;
}

Str8& Str8::operator=(const Str8& sSource)
{
    MakeSpace(sSource.GetLength());
    m_iLen = sSource.GetLength();
    strcpy_s(m_psz, m_iAlloc, (const char*)sSource);
    AssertValid();
    return *this;
}

Str8& Str8::operator+=(const char* pszSource)
{
    if (!pszSource) // 1.4qzpf Don't crash if null passed in as empty string
        pszSource = "";
    Append(pszSource);
    AssertValid();
    return *this;
}

Str8& Str8::operator+=(const char c) // 1.4qzkb Add Str8 += char to prevent crash from default behaviour
{
    Str8 s(c);
    Append(s);
    AssertValid();
    return *this;
}

Str8& Str8::operator+=(int iAdd) // 1.4tec Add a number, if numeric
{
    int i = atoi(*this);
    i += iAdd;
    char buffer[20];
    _itoa_s(i, buffer, (int)sizeof(buffer), 10);
    //	this->Empty(); // 1.6.1ck Fix bug of bad write of settings
    this->Append(buffer);
    return *this;
}

Str8& Str8::Append(const char* psz) // Append string
{
    if (!psz) // 1.4qzpf Don't crash if null passed in as empty string
        psz = "";
    int iNewSize = m_iLen + strlen(psz); // Calculate new length
    MakeSpace(iNewSize); // Make enough space (= m_iAlloc)
    strcpy_s(m_psz + m_iLen, m_iAlloc, psz); // Copy onto end
    m_iLen = iNewSize; // Set new length
    AssertValid();
    return *this;
}

Str8& Str8::Prepend(const char* psz) // Prepend string
{
    if (!psz) // 1.4qzpf Don't crash if null passed in as empty string
        psz = "";
    *this = psz + *this;
    AssertValid();
    return *this;
}

int Str8::FindAtEndOfWord(const char* psz, int iStart) const // 1.4ytc Add find at end of word (followed by sp or nl)
{
    Str8 sFind = psz;
    Str8 sSp = sFind + " ";
    Str8 sNl = sFind + "\n";
    int iFindSp = Find(sSp, iStart); // Find first with space
    int iFindNl = Find(sNl, iStart); // Find first with nl
    if (iFindSp < 0) // If none found with space, return find with nl (maybe no find of either)
        return iFindNl;
    if (iFindNl < 0) // If none found with nl, return find with sp
        return iFindSp;
    if (iFindSp < iFindNl) // If both found, return earliest
        return iFindSp;
    else
        return iFindNl;
}

int Str8::ReverseFind(const char c) const
{
    const char* pszFound = strrchr(m_psz, c);
    if (!pszFound)
        return -1;
    else
        return pszFound - m_psz;
}

Str8 Str8::Mid(int iStart, int iCount) const
{
    if (iStart < 0) // Protect against negative
        iStart = 0;
    if (iStart > m_iLen) // If start beyond end, start at end // ab 01
        iStart = m_iLen;
    if (iCount < 0 || iCount > m_iLen - iStart) // If count too many, do end
        iCount = m_iLen - iStart;
    Str8 s;
    char* psz = s.GetBuffer(iCount);
    memcpy(psz, m_psz + iStart, iCount);
    *(psz + iCount) = '\0';
    s.ReleaseBuffer();
    return s;
}

void Str8::SetAt(int iPos, const char c)
{
    if (iPos < 0 || iPos > m_iLen)
        return;
    *(m_psz + iPos) = c;
    AssertValid();
}

void Str8::Truncate(int iCount) // Cut off end at iCount
{
    if (iCount < m_iLen) // 1.4qzhx Fix U crash on jump (from Truncate)
    {
        m_iLen = iCount;
        *(m_psz + iCount) = '\0';
    }
}

void Str8::Replace(const char* pszFrom, const char* pszTo, BOOL bFeed) // 1.4vyt 
{
    int iFromLen = strlen(pszFrom);
    int iToLen = strlen(pszTo);
    if (iFromLen == 0)
        return;
    int iStart = 0;
    while (TRUE)
    {
        int iFind = Find(pszFrom, iStart);
        if (iFind == -1)
            break;
        *this = Mid(0, iFind) + pszTo + Mid(iFind + iFromLen);
        iStart = iFind; // 1.4vyt 
        if (!bFeed) // 1.4vyt 
            iStart += iToLen; // 1.4vyt 
    }
    AssertValid();
}

int Str8::ReverseFind(const char c) const
{
    const char* pszFound = strrchr(m_psz, c);
    if (!pszFound)
        return -1;
    else
        return pszFound - m_psz;
}

void Str8::TrimLeft()
{
    while (bSpace(*m_psz)) // 1.4qzjh
        *this = Mid(1);
    AssertValid();
}

void Str8::TrimRight()
{
    while (m_iLen > 0 && bSpace(*(m_psz + m_iLen - 1))) // 1.4qzjh
        Truncate(m_iLen - 1);
    AssertValid();
}

void Str8::Insert(int iPos, const char* psz)
{
    if (iPos < 0)
        iPos = 0;
    if (iPos > m_iLen)
        iPos = m_iLen;
    *this = Left(iPos) + psz + Mid(iPos);
    AssertValid();
}

void Str8::Delete(int iPos, int iCount)
{
    if (iPos < 0)
        iPos = 0;
    if (iCount > m_iLen)
        iCount = m_iLen;
    if (iPos + iCount > m_iLen)
        iCount = m_iLen - iPos;
    *this = Left(iPos) + Mid(iPos + iCount);
    AssertValid();
}

int Str8::Find(const char* psz, int iStart) const
{
    if (!psz) // 1.4qzpf Don't crash if null passed in as empty string
        psz = "";
    if (iStart >= m_iLen)
        return -1;
    const char* pszFound = strstr(m_psz + iStart, psz);
    if (!pszFound)
        return -1;
    else
        return pszFound - m_psz;
}

Str8& Str8::operator+=(const Str8& sSource)
{
    Append(sSource);
    AssertValid();
    return *this;
}

char* Str8::GetBuffer(int iSize) // Write access to buffer
{
    MakeSpace(iSize); // Make sure enough space
    AssertValid();
    return m_psz;
}

void Str8::ReleaseBuffer(int iLen) // Release buffer after writing
{
    ASSERT(iLen < m_iAlloc); // Should have made large enough space at GetBuffer call
    if (iLen >= 0) // If iLen given, null terminate string in case it was not terminated
        *(m_psz + iLen) = '\0';
    m_iLen = strlen(m_psz); // Set correct length of string
    AssertValid();
}
#endif

void Str8::Format(const char* pszFormat, ...)
	{
	char* psz = GetBuffer( 5000 ); // Get plenty of space, no loss because this is probably a temp string	
    va_list argptr;
    va_start( argptr, pszFormat );
	vsprintf_s( psz, 5000, pszFormat, argptr );
    va_end(argptr);
	ReleaseBuffer();
	}

BOOL Str8::bNextWord( Str8& sWord, int& iPos ) // Find next space delimited word, return in sWord, return end of word in iPos
	{
	if ( iPos >= GetLength() ) // If iPos past end, return false
		return FALSE;
	while ( iPos < GetLength() && bSpace( GetChar( iPos ) ) ) // Skip spaces // 1.4qzjh
		iPos++;
	if ( iPos == GetLength() ) // If no more non-spaces, return false
		return FALSE;
	int iStart = iPos;
	while ( iPos < GetLength() && !bSpace( GetChar( iPos ) ) ) // Look for next space or end // 1.4qzjh
		iPos++;
	sWord = Mid( iStart, iPos - iStart ); // Collect word in return value
	return TRUE;
	}

#define STR8BUFMAX 10000

BOOL Str8::bReadFile( Str8 sFileName ) // 1.4yta Add function to read file into Str8
	{
	char* pszStr8Buffer = (char*)malloc( STR8BUFMAX );
	FILE* pfil = nullptr;
	errno_t err = fopen_s(&pfil, sFileName, "r");
	if (err != 0 || pfil == nullptr) {
		// handle error
		return FALSE;
	}
	while ( fgets( pszStr8Buffer, STR8BUFMAX, pfil ) )
		Append( pszStr8Buffer );
	fclose( pfil );
	free( pszStr8Buffer );
	return TRUE;
	}

BOOL Str8::bGetSettingsTagSection( Str8& sTagSection, const char* pszTag, int& iStart, BOOL bIncludeTags ) // 1.4ytc Add functions to get tag section from file string // 1.5.0fe Add option to not include tags
	{
	Str8 sBeginTag = "\\+";
	sBeginTag += pszTag; // Make begin tag
	Str8 sEndTag = "\\-";
	sEndTag += pszTag; // Make end tag
	int iFindStart = FindAtEndOfWord( sBeginTag, iStart ); // Find begin tag
	if ( iFindStart < 0 ) // If not found, return false
		return FALSE;
	int iFindEnd = FindAtEndOfWord( sEndTag, iFindStart ); // Find end tag
	if ( iFindEnd < 0 ) // Should be found, but if not, set to end of string
		iFindEnd = GetLength();
	else
		iFindEnd += sEndTag.GetLength() + 1; // Set end to nl after end tag
	iStart = iFindEnd; // Update start of next find
	if ( !bIncludeTags ) // 1.5.0fe If not include tags, change begin and end to skip them
		{
		iFindStart += sBeginTag.GetLength(); // 1.5.0fe 
		iFindEnd -= sEndTag.GetLength() + 1; // 1.5.0fe 
		}
	sTagSection = Mid( iFindStart, iFindEnd - iFindStart ); // Collect tag section
	sTagSection.Trim(); // 1.5.0fe Get rid of any extra space and nl on end
	return TRUE; // Return success
	}

BOOL Str8::bGetSettingsTagContent( Str8& sTagContent, const char* pszTag, int iStart ) // 1.4ytc Add functions to get tag section from file string
	{
	Str8 sFullTag = "\\";
	sFullTag += pszTag; // Make full tag
	int iFindStart = FindAtEndOfWord( sFullTag, iStart ); // Find tag
	if ( iFindStart < 0 ) // If not found, return false
		return FALSE;
	int iFindEnd = Find( "\n", iFindStart ); // Find end of tag
	if ( iFindEnd < 0 ) // Should be found, but if not, set to length of tag
		iFindEnd = sFullTag.GetLength();
	iFindStart += sFullTag.GetLength(); // Don't include tag in content
	sTagContent = Mid( iFindStart, iFindEnd - iFindStart ); // Collect tag and content
	sTagContent.Trim(); // Trim off possible space or nl
	return TRUE; // Return success
	}

BOOL Str8::bDeleteSettingsTagSection( const char* pszTag ) // 1.5.0fe Add function to delete settings tag section from settings file string
	{
	Str8 sBeginTag = "\\+";
	sBeginTag += pszTag; // Make begin tag
	Str8 sEndTag = "\\-";
	sEndTag += pszTag; // Make end tag
	int iFindStart = FindAtEndOfWord( sBeginTag, 0 ); // Find begin tag
	if ( iFindStart < 0 ) // If not found, return false
		return FALSE;
	int iFindEnd = FindAtEndOfWord( sEndTag, iFindStart ); // Find end tag
	if ( iFindEnd < 0 ) // Should be found, but if not, set to end of string
		iFindEnd = GetLength();
	else
		iFindEnd += sEndTag.GetLength() + 1; // Set end to nl after end tag
	Delete( iFindStart, iFindEnd - iFindStart ); // 1.5.0fe 
	return TRUE; // Return success
	}

BOOL operator==( const Str8& s, const Str8& s2 )  { return strcmp( s, s2 ) == 0; }
BOOL operator==( const Str8& s, const char* psz ) { return strcmp( s, psz ) == 0; }
BOOL operator==( const char* psz, const Str8& s ) { return strcmp( s, psz ) == 0; }
BOOL operator==( const Str8& s, const char c )    { return strcmp( s, Str8( c ) ) == 0; }
BOOL operator!=( const Str8& s, const Str8& s2 )  { return strcmp( s, s2 ) != 0; }
BOOL operator!=( const Str8& s, const char* psz ) { return strcmp( s, psz ) != 0; }
BOOL operator!=( const char* psz, const Str8& s ) { return strcmp( s, psz ) != 0; }
BOOL operator!=( const Str8& s, const char c )    { return strcmp( s, Str8( c ) ) != 0; }
BOOL operator>(  const Str8& s, const Str8& s2 )  { return strcmp( s, s2 ) > 0; }
BOOL operator>(  const Str8& s, const char* psz ) { return strcmp( s, psz ) > 0; }
BOOL operator>(  const char* psz, const Str8& s ) { return strcmp( psz, s ) > 0; }
BOOL operator<(  const Str8& s, const Str8& s2 )  { return strcmp( s, s2 ) < 0; }
BOOL operator<(  const Str8& s, const char* psz ) { return strcmp( s, psz ) < 0; }
BOOL operator<(  const char* psz, const Str8& s ) { return strcmp( psz, s ) < 0; }
BOOL operator>=( const Str8& s, const Str8& s2 )  { return strcmp( s, s2 ) >= 0; }
BOOL operator>=( const Str8& s, const char* psz ) { return strcmp( s, psz ) >= 0; }
BOOL operator>=( const char* psz, const Str8& s ) { return strcmp( psz, s ) >= 0; }
BOOL operator<=( const Str8& s, const Str8& s2 )  { return strcmp( s, s2 ) <= 0; }
BOOL operator<=( const Str8& s, const char* psz ) { return strcmp( s, psz ) <= 0; }
BOOL operator<=( const char* psz, const Str8& s ) { return strcmp( psz, s ) <= 0; }

Str8 operator+(const Str8& s1, const Str8& s2)
{
	Str8 s( s1 );
	s.Append(s2);
	s.AssertValid();
	return s;
}

Str8 operator+(const Str8& s1, const char* psz)
{
	Str8 s( s1 );
	s.Append(psz);
	s.AssertValid();
	return s;
}

Str8 operator+(const char* psz, const Str8& s2)
{
	Str8 s( psz );
	s.Append(s2);
	s.AssertValid();
	return s;
}

Str8 operator+(const Str8& s1, const char c)
{
	Str8 s( s1 );
	s.Append( Str8( c ) );
	s.AssertValid();
	return s;
}

Str8 operator+(const char c, const Str8& s2)
{
	Str8 s( c );
	s.Append(s2);
	s.AssertValid();
	return s;
}

Str8 sTestCopy( const char* psz )
	{
	Str8 s = psz;
	return s;
	}

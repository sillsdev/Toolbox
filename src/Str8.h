// Str8.h String class for UTF8 and 8 bit legacy encoded data

#ifndef Str8_H
#define Str8_H

#include <string> // For std::string

class Str8
	{
private:
#ifndef UseCharStar
    std::string _data;
#else
    char* m_psz; // Pointer to allocated string data
    int m_iLen; // Length of string data
    int m_iAlloc; // Amount of space allocated
#endif
private:
	void Init(); // Initialize
	void MakeSpace( int iSize ); // Make room for possibly larger size
public:
	Str8();	// Default constructor // 1.4qzfv Start Str8
    Str8( const char* pszInit, int iCount = -1 ); // Constructor with initializing string
	Str8( const char c ); // Constructor from char
	Str8( const Str8& s ); // Copy constructor
#ifdef UseCharStar
    ~Str8(); // Destructor
#endif
	void AssertValid() const noexcept; // Assert that all is well

    operator const char* () const { return _data.c_str(); } // Read access to buffer
	Str8& operator=( const Str8& sSource ); // Assign sSource
	Str8& operator=( const char* pszSource ); // Assign pszSource
	Str8& operator+=( const Str8& sSource ); // Append sSource
	Str8& operator+=( const char* pszSource ); // Append pszSource
	Str8& operator+=( const char c ); // Append c // 1.4qzkb
    Str8& Str8::operator +=( int iAdd ); // 1.4tec Add a number, if numeric
	char* GetBuffer( int iSize = 0 ); // Write access to buffer
	void ReleaseBuffer( int iLen = -1 ); // Release buffer after writing
	Str8& Append( const char* psz ); // Append psz
	Str8& Prepend( const char* psz ); // Prepend psz
	int GetLength() const; // Length of string in chars
	int Find( const char* psz, int iStart = 0 ) const; // Find next psz after iStart
	int Find( const char c, int iStart = 0 ) const; // Find next char c after iStart
	int FindAtEndOfWord( const char* psz, int iStart = 0 ) const; // 1.4ytc Add find at end of word (followed by sp or nl)
	int ReverseFind( const char c ) const; // Find rightmost char c
	Str8 Mid( int iStart, int iCount = -1 ) const; // Next iCount chars starting at iStart, default is rest of string
	Str8 Left( int iCount ) const { return Mid( 0, iCount ); } // Leftmost iCount chars
	Str8 Right( int iCount ) const { return Mid( GetLength() - iCount, iCount ); } // Rightmost iCount chars
#ifdef UseCharStar
    char GetChar(int iPos) const { return *(m_psz + iPos); } // Get char at iPos
    BOOL IsEmpty() const { return m_iLen == 0; } // True if empty
#else
    char GetChar( int iPos ) const { return _data.at(iPos); } // Get char at iPos
    BOOL IsEmpty() const { return _data.empty(); }
#endif
	void SetAt( int iPos, const char c ); // Set char at iPos to c
	void Empty() { Truncate( 0 ); } // Make empty
	void Truncate( int iCount ); // Cut off end at iCount
	void Replace( const char* pszFrom, const char* pszTo, BOOL bFeed = FALSE ); // Replace pszFrom with pszTo everywhere // 1.4vyt Add bFeed option for self-feeding replace
	void TrimLeft(); // Trim whitespace on left
	void TrimRight(); // Trim whitespace on right
	void Trim() { TrimLeft(); TrimRight(); } // 1.4wh Trim both sides
	void Insert( int iPos, const char* psz ); // Insert psz at iPos
	void Delete( int iPos, int iCount = 1 ); // Delete iCount chars at iPos
	void Format(const char* pszFormat, ...); // Format like printf
	BOOL bNextWord( Str8& sWord, int& iPos ); // Find next space delimited word, return in sWord, return start of next word in iPos

	BOOL bReadFile( Str8 sFileName ); // 1.4yta Add function to read file into string
	BOOL bGetSettingsTagSection( Str8& sTagSection, const char* pszTag, int& iStart, BOOL bIncludeTags = TRUE ); // 1.4ytc Add function to get settings tag section from settings file string // 1.5.0fe Add option to not include tags
	BOOL bGetSettingsTagContent( Str8& sTagContent, const char* pszTag, int iStart = 0 ); // 1.4ytc Add function to get tag content from file string
	BOOL bDeleteSettingsTagSection( const char* pszTag ); // 1.5.0fe Add function to delete settings tag section from settings file string
	};
	// Compatibility with CStringEx, wait till later
// #define iFindFrom Find

Str8 sTestCopy( const char* psz ); // Copy for testing

BOOL operator==( const Str8& s, const Str8& s2 );
BOOL operator==( const Str8& s, const char* psz );
BOOL operator==( const char* psz, const Str8& s );
BOOL operator==( const Str8& s, const char c );
BOOL operator!=( const Str8& s, const Str8& s2 );
BOOL operator!=( const Str8& s, const char* psz );
BOOL operator!=( const char* psz, const Str8& s );
BOOL operator!=( const Str8& s, const char c );
BOOL operator>(  const Str8& s, const Str8& s2 );
BOOL operator>(  const Str8& s, const char* psz );
BOOL operator>(  const char* psz, const Str8& s );
BOOL operator<(  const Str8& s, const Str8& s2 );
BOOL operator<(  const Str8& s, const char* psz );
BOOL operator<(  const char* psz, const Str8& s );
BOOL operator>=( const Str8& s, const Str8& s2 );
BOOL operator>=( const Str8& s, const char* psz );
BOOL operator>=( const char* psz, const Str8& s );
BOOL operator<=( const Str8& s, const Str8& s2 );
BOOL operator<=( const Str8& s, const char* psz );
BOOL operator<=( const char* psz, const Str8& s );

Str8 operator+(const Str8& s1, const Str8& s2);
Str8 operator+(const Str8& s1, const char* psz);
Str8 operator+(const char* psz, const Str8& s2);
Str8 operator+(const Str8& s1, const char c);
Str8 operator+(const char c, const Str8& s2);

#endif // Str8_H
// lng.h Interface for language encodings (1995-02-25)


#ifndef LNG_H
#define LNG_H

#ifndef _UNICODE
#define LNG_DOUBLE_BYTE
#endif

#ifdef ESCES
// ESCES Language encoding function library
typedef int TLength;  // len
typedef unsigned short LngSortCode;  // cod
#endif

#include "set.h"  // classes CSet and CSetEl
#include "srt.h"  // 1995-04-03
#include "var.h"
#include "kbd.h"  // CKeyboardSet
#include "ptr.h"  // CKeyboardPtr

#include "update.h"  // CUpdate
#include "cbo.h"  // CSetComboBox
#include "lbo.h"  // CSetListBox
#include "ChangeTable.h"  // class ChangeTable

class Object_istream;  // obstream.h // 1.6.1cb 
class Object_ostream;  // obstream.h // 1.6.1cb 

class CMarkerUpdate;
class CFilterUpdate;


// **************************************************************************

#ifndef SHW
#define SHW
#endif  // not SHW

#ifdef SHW
extern const char Shw_chLMeta;  // left meta element delimiter, i.e. chevron
extern const char Shw_chRMeta;  // right ...
extern const char Shw_pszLMeta[2];
extern const char Shw_pszRMeta[2];
#endif  // SHW


// **************************************************************************

// A multibyte designator represents a character in a language encoding.
// It is the unit into which multilingual marked strings may be segmented.
// It may be a member of various variables (i.e. character classes).
// It has a unique identity code, by which it is associated with its
// sorting attributes in each sort order in its language encoding.

class CMChar : public CSetEl  // Hungarian: mch
{
private:
    BOOL m_bUpperCase;
    CMChar* m_pmchOtherCase;  // counterpart of this in other case

    // 1999-05-18 TLB: Moved all old-style subset stuff out of base class
    friend class CMCharsInVarSubSet;
    long m_lSubMCharsInVar;  // 1998-01-30 MRP: Variables that match this

    UIdMChar m_uch;  // unique identity for sort orders and keys
    
    Str8 m_sMCharDraw;

    // Multichars are instantiated by being added to a language encoding.
    friend class CLangEnc;
    CLangEnc* m_plngMyOwner;
        
    CMChar(const char* pszMChar, UIdMChar uch, CLangEnc* plngMyOwner);
    
    // Prevent use of the copy constructor and assignment operator
    // by making them private and omitting their definitions.
    CMChar(const CMChar& mch);
    CMChar& operator=(const CMChar& mch);
    
public:
    ~CMChar();  // destructor
    
    const Str8& sMChar() const { return sKey(); }
    const Str8& sMCharDraw() const { return m_sMCharDraw; }
        // Multi-char with two spaces prepended so that overstriking
        // diacritics will not be clipped when drawn.
    
    CLangEnc* plngMyOwner() const { return m_plngMyOwner; }
    virtual CSet* psetMyOwner();
    
    void WriteMetaMChar(std::ostream& ios) const;
        // Write the multichar, enclosed by chevrons, if a multi-byte.
        
    BOOL bUpperCase() const { return m_bUpperCase; }    
    BOOL bHasOtherCase() const { return (m_pmchOtherCase != NULL); }                        
    CMChar* pmchOtherCase() const { return m_pmchOtherCase; }
    BOOL bCaseAssociateOf(const CMChar* pmch) const;

    BOOL bWordChar() const;

    UIdMChar uch() const { return m_uch; }
    
#ifdef _DEBUG
    void AssertValid() const;
#endif  // _DEBUG

private:
    BOOL bSetUpperCase(CMChar* pmchLowerCaseCounterpart);
    BOOL bSetLowerCase(CMChar* pmchUpperCaseCounterpart);
    void ClearCase();
};  // class CMChar


// --------------------------------------------------------------------------

// A language encoding consists of a set of multiple byte designators
// representing characters, and their sorting behaviors.
// One sort order, which is distinguished as the default,
// provides the multichar attributes used in text pattern matching.
// Any of the sort orders may be chosen to sort a database (cf. \_sh, CIndex).
// The multichars of a language encoding are associated with their
// corresponding sort order attributes via their unique identity codes (uch).
class CLangEncSet; // Forward reference
class CLangEncUpdate; // Forward reference
class CFontDef; // forward reference

typedef int WINAPI iInitFunc( const char* pszFileName );
typedef int WINAPI iEncodeFunc(int iTable, const char* pszInput, int iMax, char* pszOutput);
typedef int WINAPI iUnderPosFunc( int iTable, const char* pszInput, int iSurfacePos );
typedef int WINAPI iSurfacePosFunc( int iTable, const char* pszInput, int iUnderPos );
// 1998-09-28 MRP: Extended functions are for rendering substrings
typedef int WINAPI iEncodeFuncEx(int iTable, int iInputLen, const char* pszInput, int iMax, char* pszOutput);
typedef int WINAPI iUnderPosFuncEx( int iTable, int iInputLen, const char* pszInput, int iSurfacePos );
typedef int WINAPI iSurfacePosFuncEx( int iTable, int iInputLen, const char* pszInput, int iUnderPos );

class CLangEnc : public CSet // CFastSet  // Hungarian: lng
{
private:
    Str8 m_sDescription;
    Str8 m_sCase;
    Str8 m_sPunct; // 1.4qa Add punc to lng
	Str8 m_sKeyDefs; // 1.5.0fe 
	BOOL m_bUseInternalKeyboard; // 1.5.1mc 
	Str8 m_sLng; // 1.5.0fe 
	Str8 m_sUnrecognizedSettingsInfo; // 1.0cp Don't lose settings info from later versions

    CSortOrderSet* m_psrtset;  // sort orders
    CVarInstanceSet* m_pvinset;

    UIdMChar m_uchNext;  // unique identity of next multichar to be added
    CMChar* m_pmchUndefined;  // represents chars not in language encoding
    
    BOOL m_bModified;
    
    CFont m_fnt;    // default font for markers based on this language encoding
    COLORREF m_rgbColor; // color of m_fnt
    int m_iLineHeight; // height of a line in pixels
    int m_iAscent; // space between base line and top of the character cell
    int m_iOverhang; // for italic: amount the top is skewed past the bottom
    CFont m_fntDlg; // font to be used in dialog controls, status bar, etc
    CFont m_fntPropertiesDlg;  // User's unconfirmed choice

    Str8 m_sKeyboardWin32;  // 1999-09-29 MRP
    Str8 m_sKeyboardWin16;
    CKeyboardPtr m_pkbd;
    BOOL m_bPropertiesDlgKeyboard;  // User has selected a keyboard
    CKeyboard* m_pkbdPropertiesDlg;  // User's unconfirmed selection

    BOOL m_bRightToLeft; // if TRUE, language is written right to left  e.g. Arabic
	BOOL m_bRTLRightJustify; // 1.2bd If TRUE, right to left should display right justified
	BOOL m_bUnicodeLang;	// true if language is encoded in Unicode UTF8
    Str8 m_sRenDLL; // Name of rendering DLL, used for right to left
    Str8 m_sRenTable; // Name of rendering table for DLL
    HINSTANCE m_hRenDLL; // Handle to DLL with which to free it when done
    iInitFunc* m_iInit; // DLL functions, see above for details
    iEncodeFunc* m_iEncode;
    iUnderPosFunc* m_iUnderPos;
    iSurfacePosFunc* m_iSurfacePos;
    // 1998-09-28 MRP: Extended functions are for rendering substrings
    iEncodeFuncEx* m_iEncodeEx;
    iUnderPosFuncEx* m_iUnderPosEx;
    iSurfacePosFuncEx* m_iSurfacePosEx;
    int m_iTable; // Table number for rendering DLL
    BOOL m_bRenDLL; // True if a valid rendering dll has been loaded

    // What is a "character" when editing text in this encoding?    
    // These first two are properties in the .lng file
    BOOL m_bMultiChar;  // According to the language encoding's characters
    BOOL m_bDBCS;  // Double-byte character set
#ifdef LNG_DOUBLE_BYTE
    BOOL m_bDoubleByteChar;  // DBCS and actually running on Far East Windows
#endif
    BOOL m_bSingleChar;  // Default

    friend class CLangEncProxy;
    CLangEncProxy* m_plrxMyProxy;  // owns this, and also the variables

    CLangEnc(CLangEncProxy* plrxMyProxy);  // constructor
    
    // Prevent use of the copy constructor and assignment operator
    // by making them private and omitting their definitions.
    CLangEnc(const CLangEnc& lng);
    CLangEnc& operator=(const CLangEnc& lng);

public:
    ~CLangEnc();  // destructor

    void IncrementNumberOfRefs();
    void DecrementNumberOfRefs();

    void SetDefaultProperties();
    BOOL bReadProperties(CNoteList& notlst);
    BOOL bReadProperties(Object_istream& obs);
#ifdef lngWritefstream // 1.6.4ab 
    void WriteProperties(Object_ofstream& obs) const;
#else
    void WriteProperties(Object_ostream& obs) const;
#endif
    void WritePropertiesIfModified( BOOL bForceWrite=FALSE );
    
#ifdef prjWritefstream // 1.6.4aa 
   void WriteRef(Object_ofstream& obs, const char* pszQualifier) const;
#else
    void WriteRef(Object_ostream& obs, const char* pszQualifier) const;
#endif
    BOOL bReadSortOrderRef(Object_istream& obs, const char* pszQualifier,
            CSortOrder*& psrt);

    const Str8& sName() const;
    const Str8& sDescription() const { return m_sDescription; }
    const Str8& sCase() const { return m_sCase; }
	BOOL bHasCase() const { return m_sCase != ""; } // 1.4vyz Add test for case
	// Punctuation characters
    void SetPunct( const char* pszPunctuation ) { m_sPunct = pszPunctuation; } // 1.4qa Add punc to lng
    const Str8& sPunct() const { return m_sPunct; } // 1.4qa Add punc to lng
    CSortOrderSet* psrtset() const { return m_psrtset; }    
    CSortOrder* psrtDefault() const;
    CVarInstanceSet* pvinset() const { return m_pvinset; }
    CLangEncProxy* plrxMyProxy() const { return m_plrxMyProxy; }
    CLangEncSet* plngsetMyOwner() const;
    
    BOOL bSeparateDiacriticsAfter() const
        { return psrtDefault()->bSeparateDiacriticsAfter(); }

    // font functions
    const CFont* pfnt() const { return &m_fnt; }
	Str8 sFontName() const; // 1.4ywp 
    int iLineHeight() const { return m_iLineHeight; } // return height of font in pixels
    int iAscent() const { return m_iAscent; }
    int iOverhang() const { return m_iOverhang; }
    void SetFont( CFontDef* pfntdef );  // Change fonts using parameters in plf
    COLORREF rgbColor() const { return m_rgbColor; } // return color used to display language font
    void SetColor( COLORREF rgbColor ) { m_rgbColor = rgbColor; } // Set color of font
    const CFont* pfntDlg() const; // trimmed font to be used in dialog controls, status bar
    void CreateCtrlFont( CFontDef* pfntdef ); // create a trimmed font (if main font too big) for language controls
    void RecalcCtrlFont(); // recalculate language control font size
    

    // ------------------------------------------------------------------------
    // Context-sensitive rendering
private:
    void LoadRenDLL();
        // If there's a rendering DLL, load it and its table.
        // Sets m_bRenDLL if sucessfully loaded and ready to use.
    int iEncode(const char* pszUnderlying, int maxlen1, char* pszSurface) const;
        // Apply context-sensitive rendering changes to underlying string.
    int iUnderPos(const char* pszUnderlying, int iSurfacePos) const;
        // Return the character position in the underlying string
        // that corresponds to the given surface string position.
    int iSurfacePos(const char* pszUnderlying, int iUnderPos) const;
        // Return the character position in a surface string
        // that corresponds to the given underlying string position.
    // 1998-09-28 MRP: Extended functions are for rendering substrings
    int iEncodeEx(int lenUnderlying, const char* pszUnderlying,
    		int maxlen1, char* pszSurface) const;
    int iUnderPosEx(int lenUnderlying, const char* pszUnderlying,
    		int iSurfacePos) const;
    int iSurfacePosEx(int lenUnderlying, const char* pszUnderlying,
    		int iUnderPos) const;

public:
    BOOL bRightToLeft() const { return m_bRightToLeft; }
    void SetRightToLeft( BOOL b ) { m_bRightToLeft = b; }
    BOOL bRTLRightJustify() const { return m_bRTLRightJustify; } // 1.2bd Make option for rtl right justify
    void SetRTLRightJustify( BOOL b ) { m_bRTLRightJustify = b; }
	BOOL bUnicodeLang() const { return m_bUnicodeLang; }
    void SetUnicodeLang( BOOL bUnicodeLang ) { m_bUnicodeLang = bUnicodeLang; }
    BOOL bUseInternalKeyboard() // True if internal keyboard in use // 1.5.1mb 
            { return m_bUseInternalKeyboard; }     
    void SetUseInternalKeyboard( BOOL b ) // Set or clear // 1.5.1mc 
        { m_bUseInternalKeyboard = b; }
	void ToggleUseInternalKeyboard()  // 1.5.1mc Toggle internal keyboard on or off
		{ m_bUseInternalKeyboard = !m_bUseInternalKeyboard; }

    BOOL bRenDLL() const { return m_bRenDLL; }
    // File name of Rendering DLL
    const Str8& sRenDLL() const { return m_sRenDLL; }
    void SetRenDLL( const char* pszRenDLL ) { m_sRenDLL = pszRenDLL; }
    // File name of .SDF table
    const Str8& sRenTable() const { return m_sRenTable; }
    void SetRenTable( const char* pszRenTable ) { m_sRenTable = pszRenTable; }

    int GetTextWidth(const CDC* pDC, const char* pszUnderlying,
            int lenUnderlying, int lenContext = 0) const;
        // Return the width that the underlying substring will require
        // when it is displayed in its surface form using pDC's font.
        // Use in place of CDC::GetTextExtent to account for rendering.
    CSize GetTextExtent(const CDC* pDC, const char* pszUnderlying,
            int lenUnderlying, int lenContext = 0) const;
        // Return the extent, i.e. the width and height ...

    int lenRenSubstring(const char* pchUnderlying, int lenUnderlying,
            int lenContext, BOOL bReverseIfRightToLeft,
            char* pszRenBuf, int maxlenRenBuf,
            const char** ppchSurface, int* plenSurface) const;
        // Render a substring in underlying form so that it can be displayed
        // in its surface form. If this is a right-to-left script and
        // bReverseIfRightToLeft, then the substring also gets reversed.
        // The caller provides a character buffer pchRenBuf for rendering.
        // NOTE: Because string "length" don't count the terminating null,
        // be sure to allocate pszRenBuf as char[maxlenRenBuf + 1].
        //
        // The surface substring is returned via arguments *ppchSurface
        // and *plenSurface (which will simply be the underlying substring,
        // if this encoding doesn't have any context-sensitive rendering).
        // The return value is the actual length of the surface substring.
        // If it exceeds maxlenRenBuf, a rendered result gets truncated.

    BOOL bClipAlignedSubstring(const CDC* pDC, BOOL bRightAligned,
            const char** ppszUnderlying, int* plenUnderlying,
            int* pixTextWidth) const;
        // Returns whether the substring in underlying form needs to be
        // clipped to fit in an available space of *pixTextWidth units
        // when displayed in its surface form using pDC's font.
        // If it needs to be clipped, return via *ppszUnderlying and
        // *plenUnderlying the substring that will fit. If bRightAligned:
        // clip off the beginning for left-to-right scripts, but
        // clip off the (underlying) end for right-to-left scripts.
        // To left-align the substring, it does the opposite.
        // ALWAYS returns via *pixTextWidth the actual width of the
        // (possibly clipped) substring as it will be displayed.

    // Replacements for standard device context display functions.
    CFont* SelectFontIntoDC(CDC* pDC) const;
        // Select this language encoding's font into the
        // device context. Returns the font being replaced.
    CFont* SelectDlgFontIntoDC(CDC* pDC) const;
        // Select this language encoding's dialog box font into the
        // device context. Returns the font being replaced.
        // Use this font when the display height is restricted.
    void SetTextAndBkColorInDC(CDC* pDC, BOOL bHighlight) const;
        // Set the device context's text and background colors
        // to those chosen in this language encoding's font properties.
        // If bHighlight, use the appropriate system-defined colors.
    void ExtTextOutLng(CDC* pDC, int ixBegin, int iyBegin,
            int nOptions, const CRect* prect,
            const char* pszUnderlying, int lenUnderlying,
            int lenContext) const;
        // Display a substring rendered in its surface form
        // using the device context's current font and color.

	void WritePaths( std::ofstream& ostr )
		{
		if ( m_sRenTable.GetLength() > 0 )
			ostr << m_sRenTable << "\n";
#if UseCct
		for ( CSortOrder* psrt = psrtset()->psrtFirst(); psrt; psrt = psrtset()->psrtNext( psrt ) )
			psrt->WritePaths( ostr );
#endif
		};

    // ------------------------------------------------------------------------

	void CleanUpKeyDefs(); // 1.5.0ff Clean up the key defs for case and correct nl's
	void ApplyKeyDefs( Str8 &strChar ); // 1.5.0ff Implement key defs

    CKeyboard* pkbd() const { return m_pkbd; }
    void SetKeyboard(CKeyboard* pkbd);

    // The following member functions are for special use in the
    // Language Encoding Properties sheet, so that the user will see
    // a font choice or keyboard selection on the Options page
    // immediately affect language-related controls on the other pages.
    // The normal member data and functions cannot be used for this purpose,
    // since the user has not yet chosen OK or Cancel.
    
    const CFont* pfntPropertiesDlg() const;
    void SetPropertiesDlgFont( CFontDef* pfntdef );
    void ClearPropertiesDlgFont();
    
    CKeyboard* pkbdPropertiesDlg() const;
    void SetPropertiesDlgKeyboard(CKeyboard* pkbd);
    void ClearPropertiesDlgKeyboard();

    // Search for multichararacter designator
    CMChar* pmchSearch( const char* pszMChar ) const
        { return (CMChar*)pselSearch( pszMChar ); }
    CMChar* pmchSearch_AddIfNew( const char* pszMChar );
        
    // Multichars are sorted plain ASCII/ANSI.
    CMChar* pmchFirst() const  // First multi-char
        { return (CMChar*)pselFirst(); }
    CMChar* pmchLast() const  // Last multi-char
        { return (CMChar*)pselLast(); }
    CMChar* pmchNext( const CMChar* pmchCur ) const  // after current
        { return (CMChar*)pselNext( pmchCur ); }
    CMChar* pmchPrev( const CMChar* pmchCur ) const  // before current
        { return (CMChar*)pselPrev( pmchCur ); }

    // NOTE: InsertBefore/After omitted, since each multichar must be unique.   
    void Delete( CMChar** ppmch )  // Delete multi-char, set pointer to NULL
	{ CSet::Delete( (CSetEl**) ppmch ); } // CFastSet
    
    BOOL bMChar(const char** ppsz, const CMChar** ppmch) const;
        // Return whether there is a character at the pointer (i.e. FALSE at the
        // end of the string). If not at end, update the pointer to the
        // position after the longest matching multi-character in the
        // language encoding, and return a pointer to its definition.
        // If no match, increment the pointer by one and return m_pmchUndefined.
    const CMChar* pmchNextSortingUnit(const char** ppchr) const;

    // Move by editing units
    // If \MultiChar, then move by language encoding multicharacters
    // If \DBCS and running Far East Windows, move according to CharNext/Prev
    // Otherwise move by single characters
    void NextEditingUnit(const Str8& s, int* poff) const;
        // Return offset to beginning of next unit in s via poff.
        // NOTE: Don't call this function when already at the end of s.
    void PrevEditingUnit(const Str8& s, int* poff) const;
        // Return offset to beginning of previous unit in s via poff.
        // NOTE: Don't call this function when already at the beginning of s.

    BOOL bEqualMChars(const CMChar* pmchA, const CMChar* pmchB,
                                                MatchSetting matset) const
        { return (psrtDefault()->iCompareMChars(pmchA, pmchB, matset) == 0); }
        // Return whether multichars match at the specified setting according to
        // the default sort order for this language encoding.

    // Return multichar attributes in default sort order.   
    BOOL bIncludeInMatch(const CMChar* pmch, MatchSetting matset) const
        { return psrtDefault()->bIncludeInMatch(pmch, matset); }
    const CMCharOrder* pmchordDefault(const CMChar* pmch) const;

    static BOOL s_bMatchMetaMCharAt(const char** ppsz, Str8& sMChar,
            CNote** ppnot);
        // Match a "meta" multichar, i.e. the representation of a character
        // in a definition (e.g. sort order or variable).
        // A multiple byte designator will be enclosed in chevrons.

    BOOL bMatchMetaMCharAt_AddIfNew(const char** ppsz,
            CMChar** ppmch, CNote** ppnot);

	bool bNextWord( Str8 sLine, Str8& sWord, int& iPos ) const; // Find next space delimited word, return in sWord, return end of word in iPos // 1.4teb 
	Str8 sLowerCase( Str8 s ); // 1.4tgk Return lower case equivalent of string

	BOOL bIsWdSep( Str8 s, int i ) const // 1.4vyv Return true if pos i of string s is word separator
		{
		const char* psz = ((const char*)s) + i;
		return bIsWdSepMove( &psz );
		}

	BOOL bInPuncList( const char* psz, int& iNumBytes ) const; // 1.4zbc Return 0 if no punct list, or first char not in punc list, return num bytes if first char is in punc list
#ifdef LATER // 1.4zbc 
	BOOL CLangEnc::bIsWdChar(const char* psz, int& iNumBytes ) const;  // Return true if word char, else return false, return number of bytes in next char in iNumBytes
	BOOL CLangEnc::bIsWdSep(const char* psz, int& iNumBytes ) const;  // Return true if word separator, else return false, return number of bytes in next char in iNumBytes
#endif // LATER
    BOOL bIsWdCharMove(const char** ppsz) const;  // Return true and advance pointer if in the default sort order, but not space
    BOOL bIsWdSepMove(const char** ppsz) const;  // Return true and advance pointer if space, or if not in the default sort order

    int iWdChar( const char* psz ) const // Return 0 if not word char, len of char if it is
        {
        const char* pszEnd = psz; // Temp for end
        if ( !bIsWdCharMove( &pszEnd ) ) // If not word char, return fail
            return 0;
        else
            return pszEnd - psz; // Else return len of match
        }

    int iWdSep( const char* psz ) const // Return 0 if not word char, len of char if it is
        {
        const char* pszEnd = psz; // Temp for end
        if ( !bIsWdSepMove( &pszEnd ) ) // If not word char, return fail
            return 0;
        else
            return pszEnd - psz; // Else return len of match
        }

    BOOL bIsSpace( const char* psz ) const // Return true if space or char defined as space in language, but not nl, which is included below in bIsWhite
        {
        return ( *psz && ( *psz == ' ' ) ); // !!! AB Later allow for special chars defined as space in language // 1996-02-01 MRP: WHAT ABOUT USER-DEFINED SPACE, IF ANY???
        }

    BOOL bIsSpace( char ch ) const // Return true if space or char defined as space in language, but not nl, which is included below in bIsWhite
        {
        return ( ch == ' ' ); // !!! AB Later allow for special chars defined as space in language // 1996-02-01 MRP: WHAT ABOUT USER-DEFINED SPACE, IF ANY???
        }

    BOOL bIsWhite( const char* psz ) const // Return true if space, nl, or char defined as space in language
        {
        return ( *psz && ( bIsSpace( psz ) || *psz == '\n' ) );
        }

    BOOL bIsWhite( char ch ) const // Return true if space, nl, or char defined as space in language
        {
        return ( bIsSpace( ch ) || ch == '\n' );
        }

    BOOL bIsNonWhite( const char* psz ) const // Return true if at char that is not whitespace
        {
        return *psz && !bIsWhite( psz );
        }

    const char* pszSkipSpace( const char* psz ) const // Find first non-space
        {
        while ( bIsSpace( psz ) ) // Hunt for non-space
            psz++;
        return psz;
        }

    const char* pszSkipWhite( const char* psz ) const // Find start of next word
        {
        while ( bIsWhite( psz ) ) // Hunt for non-space
            psz++;
        return psz;
        }

    const char* pszSkipNonWhite( const char* psz ) const // Find end of word
        {
        while ( bIsNonWhite( psz ) ) // Hunt for space
            psz++;
        return psz;
        }

    Str8 sTrimWhitespace( const char* psz ) const; // Trim leading and trailing whitespace

    // To check for embedded whitespace, first call sTrimWhitespace, then pass result
    // to this function.
    BOOL bContainsWhite( const char* psz ) const
        {
        return (BOOL) *pszSkipNonWhite( psz );
        }

	int iCharNumBytes( const char* psz ) const // Return number of bytes in UTF8 char, helper for move and delete primitives
		{
		const char* pszStart = psz; // Remember starting pos
		psz++; // Move at least one
		if ( *(psz - 1) == '\r' && *psz == '\n' ) // Move past return line feed as one
			psz++;
		else if ( m_bUnicodeLang ) // If UTF8, move past whole character
			if ( (const unsigned char)*pszStart >= 0xC0 ) // If byte was first byte of multi-byte sequence, move past all bytes of sequence
				while ( (const unsigned char)*psz >= 0x80 && (const unsigned char)*psz < 0xC0 ) // While later byte of sequence, move past
					psz++;
		return psz - pszStart; // Return number of bytes moved
	};

	int iCharLeftNumBytes( const char* psz, int iChar ) const // Return number of bytes in UTF8 char, helper for move and delete primitives // 1.4gz Fix bug of crash on left arrow in Unicode
		{
		const char* pszStart = psz; // Remember starting pos
		psz--; // Move at least one
		if ( *psz == '\n' && *(psz - 1) == '\r' ) // Move past return line feed as one
			psz--;
		else if ( m_bUnicodeLang ) // If UTF8, move past whole character
			if ( (const unsigned char)*psz >= 0x80 && (const unsigned char)*psz < 0xC0 ) // If byte was last byte of multi-byte sequence, move past all bytes of sequence
				{
				while ( (const unsigned char)*psz >= 0x80 && (const unsigned char)*psz < 0xC0 )
					{
					if ( psz <= pszStart - iChar ) // 1.4gz Prevent move past beginning of string
						break;
					else
						psz--; // Move past latter bytes of sequence
					}
				}
		return pszStart - psz; // Return number of bytes moved
		}

    BOOL bModifyProperties(const char* pszName,
            const char* pszDescription, const char* pszCase, const char* pszPunct, // 1.4qb
            CSortOrder* psrtDefaultOriginal, CNote** ppnot);
    BOOL bSetProperties(const char* pszName,
            const char* pszDescription, const char* pszCase, const char* pszPunct, // 1.4qb 
			CNote** ppnot);
    void Update(CUpdate& up);
    void LangEncUpdated(CLangEncUpdate& lup);

    BOOL bViewProperties(int iActivePage = 0);
    BOOL bViewProperties(int iActivePage, CSortOrder* psrt, CVarInstance* pvin);

#ifdef _DEBUG
    void AssertValid() const;
#endif  // _DEBUG

    // ------------------------------------------------------------------------
#ifdef ESCES
    // ESCES: External Scripture Checking Exchange Standard
    BOOL bNextWord(const char** ppchr, TLength* plenWord) const;
    int iCompareStrings(const char* pchrA, const char* pchrB) const;
    BOOL bMatchStrings(const char* pchrA, const char* pchrB,
            BOOL bDistinguishCase, BOOL bWhole) const;
        // QUESTION: Provide all four character matching options?
    int iCompareSubstrings(const char* pchrA, TLength lenA,
            const char* pchrB, TLength lenB) const;
    BOOL bMatchSubstrings(const char* pchrA, TLength lenA,
            const char* pchrB, TLength lenB,
            BOOL bDistinguishCase, BOOL bWhole) const;
    int iCompareSortKeys(const LngSortCode* pcodA,
            const LngSortCode* pcodB) const;
    BOOL bMatchSortKeys(const LngSortCode* pcodA,
            const LngSortCode* pcodB,
            BOOL bDistinguishCase, BOOL bWhole) const;
    TLength lenMakeSortKey(const char* pchr, TLength len,
            LngSortCode* pcod, TLength maxlenKey) const;
        // QUESTION: Tell the caller if any chars are undefined or unordered?
        // QUESTION: Version which takes care of leading/trailing white space?
    TLength lenCopySortKey(const LngSortCode* pcodFrom,
            LngSortCode* pcodTo, TLength maxlenKeyTo) const;
#endif
    // ------------------------------------------------------------------------
                                                                     
private:
    CMChar* pmchAdd( const char* pszMChar );
    
    BOOL bSetCase(const char* pszCase, BOOL bReading, CNote** ppnot);
    BOOL bSetCase(const char* pszCase, CNote** ppnot);
    BOOL bSetCase(CMChar* pmchUpper, CMChar* pmchLower, CNote** ppnot);
    void ClearCase();
};  // class CLangEnc

class CCharSetListBox : public CSetListBox
{
public:
    CCharSetListBox(CLangEnc* plng, CMChar** ppmch);

protected:
    int m_xChar;
    int m_xOtherCase;
    int m_xDec;
    int m_xHex;
    int m_xStandardChar;
    
    CLangEnc* m_plng;
    CFont m_fntStandard;
    
    virtual void InitLabels();
    virtual int iItemHeight();
    virtual void DrawElement(CDC& cDC, const RECT& rcItem,
            const CDblListEl *pel);
};  // class CCharSetListBox

// --------------------------------------------------------------------------

class CLangEncPtr  // Hungarian: plng
{
private:
    CLangEnc* m_plng;
    
    void IncrementNumberOfRefs(CLangEnc* plng);
    void DecrementNumberOfRefs();
    
public:
    CLangEncPtr(CLangEnc* plng = NULL)
        { IncrementNumberOfRefs(plng); }
    CLangEncPtr(const CLangEncPtr& plng)  // copy constructor
        { IncrementNumberOfRefs(plng.m_plng); }
    
    // Decrement the reference count of the index to which
    // this currently refers and increment plng's reference count
    const CLangEncPtr& operator=(CLangEnc* plng);
    const CLangEncPtr& operator=(const CLangEncPtr& plng)
        { return operator=(plng.m_plng); }

    ~CLangEncPtr() { DecrementNumberOfRefs(); }
    
    CLangEnc* operator->() const { return m_plng; }
    operator CLangEnc*() const { return m_plng; } 
};  // class CLangEncPtr


// --------------------------------------------------------------------------

class CLangEnc;
class CDatabaseType;
class CShwDoc;
class CShwView;

class CLangEncUpdate : public CUpdate  // Hungarian: lup
{
private:
    CLangEnc* m_plng;
    BOOL m_bModified;
    BOOL m_bNameModified;
    BOOL m_bDefaultSortOrderChanged;
    
public:
    CLangEncUpdate(CLangEnc* plng);
    CLangEncUpdate(CLangEnc* plng, const char* pszName,
            const char* pszCase, const char* pszPunct, // 1.4qb
			CSortOrder* psrtDefault);
    
    CLangEnc* plng() const { return m_plng; }

    BOOL bModified() const { return m_bModified; }  
    BOOL bNameModified() const { return m_bNameModified; }
    BOOL bDefaultSortOrderChanged() const { return m_bDefaultSortOrderChanged; }

    // NOTE: Implementated in update.cpp rather than in srt.cpp
    virtual void UpdateLangEnc(CLangEnc* plng);
    virtual void UpdateDatabaseType(CDatabaseType* ptyp);
    virtual void UpdateShwDoc(CShwDoc* pdoc);
    virtual void UpdateShwView(CShwView* pview);
};  // class CLangEncUpdate


// --------------------------------------------------------------------------

class CLangEncProxy : public CSetEl  // Hungarian: lrx
{
private:
    Str8 m_sFileName;
    Str8 m_sVersion;  // 1998-03-12 MRP
    CLangEnc* m_plng;

    friend class CLangEncSet;
    CLangEncSet* m_plngsetMyOwner;
    
    CLangEncProxy(const char* pszName, const char* pszFileName,
            CLangEncSet* plngsetMyOwner);
    
    // Prevent use of the copy constructor and assignment operator
    // by making them private and omitting their definitions.
    CLangEncProxy(const CLangEncProxy& lrx);
    CLangEncProxy& operator=(const CLangEncProxy& lrx);
    
public:
    ~CLangEncProxy();

    const Str8& sFileName() const { return m_sFileName; }
    Str8 sFullPathName() const;
    const Str8& sVersion() const { return m_sVersion; }
    CLangEncSet* plngsetMyOwner() const { return m_plngsetMyOwner; }
    virtual CSet* psetMyOwner();
    
    CLangEnc* plng(CNoteList& notlst);
    void DecrementNumberOfRefs();
    Str8 sPath();
    
    void Update(CUpdate& up);
    
//    virtual BOOL bCopy(CSetEl** ppselNew); // 1.6.1cb 
    virtual BOOL bModify();

    BOOL bModifyProperties(const char* pszName,
            const char* pszDescription, const char* pszCase, const char* pszPunct, // 1.4qb
            CSortOrder* psrtDefaultOriginal, CNote** ppnot);
    BOOL bSetProperties(const char* pszName,
            const char* pszDescription, const char* pszCase, const char* pszPunct, // 1.4qb
			CNote** ppnot);

private:
    static BOOL s_bReadProperties(Object_istream& obs, const char* pszName,
            const char* pszFileName, CLangEncSet* plngsetMyOwner,
            CLangEnc** pplng);
    static CLangEnc* s_plngDefaultProperties(const char* pszName,
            const char* pszFileName, CLangEncSet* plngsetMyOwner);
    CLangEnc* plngNew();

    void WritePropertiesIfModified( BOOL bForceWrite=FALSE );

    static BOOL s_bReadProxy(const char* pszPath, CLangEncSet* plngset,
            CLangEncProxy** pplrx);

#ifdef _DEBUG
    void AssertValid() const;
#endif  // _DEBUG
};  // class CLangEncProxy


// --------------------------------------------------------------------------

// In Shw there is a single set containing all language encodings,
// which will be a member of the CShwApp instance.

class CLangEncSet : public CSet  // Hungarian: lngset
{
#ifdef TEST_BUILD
    friend class CLangEncSet_Test;
#endif
private:
    CKeyboardSet m_kbdset;
    CVarSet m_varset;  // variable names are shared by all language encodings
    Str8 m_sSettingsVersion;  // 1998-03-12 MRP
    Str8 m_sSettingsDirPath;
#ifdef LNG_DOUBLE_BYTE
    BOOL m_bDoubleByteSystem;  // Actually running on Far East Windows?
#endif
    BOOL m_bReadFromString;
    CLangEncPtr m_plngDefault;
    
public: 
    CLangEncSet(const char* pszSettingsVersion);
#ifdef _DEBUG
    CLangEncSet(const char* pszSettingsVersion, const char* pszProperties);
        // Read language encodings from a string rather than from .enc files.
        // To be used in internal tests so that they contain their own data.
#endif  // _DEBUG
    
    // Prevent use of the copy constructor and assignment operator
    // by making them private and omitting their definitions.
    CLangEncSet(const CLangEncSet& lngset);
    CLangEncSet& operator=(const CLangEncSet& lngset);

    virtual ~CLangEncSet();

    void ReadAllProxies(CNoteList& notlst);
    void WriteAllModified( BOOL bForceWrite=FALSE );
    BOOL bReadLangEncRef(Object_istream& obs, const char* pszQualifier,
            CLangEnc*& plng);

    CKeyboardSet* pkbdset() { return &m_kbdset; }   
    CVarSet* pvarset() { return &m_varset; }
    const Str8& sSettingsVersion() const { return m_sSettingsVersion; }
    const Str8& sSettingsDirPath() const { return m_sSettingsDirPath; }
    void SetDirPath( const char* pszPath ) { m_sSettingsDirPath = pszPath; }
    BOOL bReadFromString() const { return m_bReadFromString; }
    CLangEnc* plngDefault() const { return m_plngDefault; }

#ifdef LNG_DOUBLE_BYTE
    BOOL bDoubleByteSystem() const { return m_bDoubleByteSystem; }
#endif
    
    CLangEnc* plngSearch_AddIfNew(const char* pszName, CNoteList& notlst);
    CLangEnc* plngSearch(const char* pszName);

    CLangEncProxy* plrxFirst() const
        { return (CLangEncProxy*)pselFirst(); }
    CLangEncProxy* plrxLast() const
        { return (CLangEncProxy*)pselLast(); }
    CLangEncProxy* plrxNext( const CLangEncProxy* plrxCur ) const
        { return (CLangEncProxy*)pselNext( plrxCur ); }
    CLangEncProxy* plrxPrev( const CLangEncProxy* plrxCur ) const
        { return (CLangEncProxy*)pselPrev( plrxCur ); }
        
    BOOL bIsMember(CLangEnc* plng) const
        { return CDblList::bIsMember(plng->plrxMyProxy()); }

//  void Add( CLangEnc* plngNew );  // Add new encoding in correct order    
    // NOTE: InsertBefore/After omitted, since each encoding must be unique.    
    // NOTE: Cannot delete a encoding to which any CMarker instances refer.
    void Delete( CLangEnc** pplng )  // Delete encoding, set pointer to next
        { CSet::Delete( (CSetEl**)pplng ); }
            
    void Update(CUpdate& up);

    BOOL bViewElements(CLangEncProxy* plrxToSelect = NULL);
    BOOL bViewElements(CLangEnc* plngToSelect);
    // 1996-02-15 MRP: Obsolete and to be removed soon
    void ViewElements(CLangEncProxy* plrxToSelect = NULL)
        { (void) bViewElements(plrxToSelect); }
    void ViewElements(CLangEnc* plngToSelect)
        { (void) bViewElements(plngToSelect); }

    void RecalcCtrlFonts(); // adjusts fonts used in language controls for all languages in use

    BOOL bAdd(CSetEl** ppselNew);
    BOOL bAddLangEnc(const char* pszName,
            const char* pszDescription, const char* pszCase, const char* pszPunct, // 1.4qb
            CSortOrder* psrtDefaultOriginal,
            CLangEnc* plngToBeAdded, CNote** ppnot);
    virtual void Delete(CSetEl** ppsel);
        
#ifdef _DEBUG
    void AssertValid() const;
#endif  // _DEBUG

private:
    CLangEncProxy* plrxSearch(const char* pszName);
    void AddAndWrite(CLangEnc* plng);
};  // class CLangEncSet


// ==========================================================================

class CLangEncSetComboBox : public CSetComboBox  // Hungarian: cbo
{
public:
    CLangEncSetComboBox(CLangEncSet* plngset, BOOL bUseAnyItem = FALSE);
    
    void UpdateElements(CLangEnc* plngToSelect);
    CLangEnc* plngSelected();
    void SelectElement(CLangEnc* plngToSelect);  // TLB: 06/08/99 - Added for Jump dlg
};  // class CLangEncSetComboBox

class CLangEncSetListBox : public CSetListBox
{
public:
    CLangEncSetListBox(CLangEncSet* plngset, CLangEncProxy** pplrx);

protected:
    int m_xName;
    int m_xFileName;
    
    virtual void InitLabels();
    virtual void DrawElement(CDC& cDC, const RECT& rcItem,
            const CDblListEl *pel);
};  // class CLangEncSetListBox

// **************************************************************************

#endif  // LNG_H

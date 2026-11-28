// obstream.cpp  Implementation of textual iostreams of objects (1995-07-22)


#include "stdafx.h"
#include "toolbox.h"
#include "obstream.h"
#include "project.h"
// #undef Object_ostream // 1.6.1cb  // 1.6.4aa Never do full redefine with FileStream
// #undef Object_istream // 1.6.1cm  // 1.6.4aa Never do full redefine with FileStream
#include <iostream>
using namespace std;  // classes istream, ostream
#ifdef OBSTREAM_NOTES   // 1.6.4aa Not defined
#include "not.h"  // class CNoteList
#endif  // 1.6.4aa 

void Object_ofstream::WriteProjectPath(const char* marker, const Str8& path)
{
    const CProject* project = Shw_pProject();
    WriteString(marker, project ? project->sStoredPath(path) : path);
}

void Object_ostream::WriteProjectPath(const char* marker, const Str8& path)
{
    const CProject* project = Shw_pProject();
    WriteString(marker, project ? project->sStoredPath(path) : path);
}

BOOL Object_istream::bReadProjectPath(const char* marker, Str8& path)
{
    if (!bReadString(marker, path))
        return FALSE;
    if (const CProject* project = Shw_pProject())
        path = project->sResolvedPath(path, bRelocateLegacyPaths());
    return TRUE;
}

static const char s_pszWhiteSpace[] = " \n\t";  // space, newline, tab
static const char s_pszOtherWhiteSpace[] = "\n\t";  // besides space

// **************************************************************************
 // 1.6.4aa All ofstream seems to compile

void Object_ofstream::WriteUTF8BOM() // 1.4qnh Write UTF8 bom Byte Order Mark
	{
	Str8 sOut; // 1.6.1bg 
	sOut+= (char)0xef; // 1.4qnh Write bom
	sOut+= (char)0xbb; // 1.4qnh Write bom
	sOut+= (char)0xbf; // 1.4qnh Write bom
	fputs( sOut, m_pf ); // 1.6.1bg 
	}

void Object_ofstream::WriteBeginMarker(const char* pszMarker, const char* pszName)
{
    ASSERT( pszName );
	Str8 sOut; // 1.6.1bg 
    sOut+= "\\+"; 
	sOut+= pszMarker; 
	sOut+= ' '; 
	sOut+= pszName; 
	sOut+= '\n';
	fputs( sOut, m_pf ); // 1.6.1bg 
}

void Object_ofstream::WriteEndMarker(const char* pszMarker)
{
	Str8 sOut; // 1.6.1bg 
    sOut+= "\\-"; 
	sOut+= pszMarker; 
	sOut+= '\n';
	fputs( sOut, m_pf ); // 1.6.1bg 
}

void Object_ofstream::WriteBeginMarkerWithQualifier(const char* pszMarker, const char* pszQualifier,
        const char* pszName)
{
	Str8 sOut; // 1.6.1bg 
    if ( !pszQualifier )
        {
        WriteBeginMarker( pszMarker, pszName );
        return;
        }

    ASSERT( pszName );
    sOut+= "\\+"; 
	sOut+= pszMarker; 
	sOut+= pszQualifier; 
	sOut+= ' '; 
	sOut+= pszName; 
	sOut+= '\n';
	fputs( sOut, m_pf ); // 1.6.1bg 
}

void Object_ofstream::WriteEndMarkerWithQualifier(const char* pszMarker, const char* pszQualifier)
{
	Str8 sOut; // 1.6.1bg 
    if ( !pszQualifier )
        {
        WriteEndMarker( pszMarker );
        return;
        }

    sOut+= "\\-"; 
	sOut+= pszMarker; 
	sOut+= pszQualifier; 
	sOut+= '\n';
	fputs( sOut, m_pf ); // 1.6.1bg 
}


void Object_ofstream::WriteString(const char* pszMarker, const char* pszQualifier,
        const char* psz)
{
    ASSERT( psz );
	Str8 sOut; // 1.6.1bg 
    if ( !*psz )
        return;

    sOut+= '\\'; 
	sOut+= pszMarker; 
	sOut+= pszQualifier; 
	sOut+= ' ';
	fputs( sOut, m_pf ); // 1.6.1ck Fix bug of writing settings in wrong order
    WriteContents(psz);
	WriteNewline(); // 1.6.1ck 
}

void Object_ofstream::WriteString(const char* pszMarker, const char* psz)
{
    ASSERT( psz );
	Str8 sOut; // 1.6.1bg 
    if ( !*psz )
        return;

    sOut+= '\\'; 
	sOut+= pszMarker; 
	sOut+= ' ';
	fputs( sOut, m_pf ); // 1.6.1ck Fix bug of writing settings in wrong order
    WriteContents(psz);
	WriteNewline(); // 1.6.1ck 
}

void Object_ofstream::WriteObligatoryString(const char* pszMarker,
        const Str8& s)
{
	Str8 sOut; // 1.6.1bg 
    sOut+= '\\'; 
	sOut+= pszMarker;
	fputs( sOut, m_pf ); // 1.6.1ck Fix bug of writing settings in wrong order
    if ( !s.IsEmpty() )  // 1999-04-01 MRP: Replace incorrect condition
        {
        sOut = ' '; // 1.6.1ck 
		fputs( sOut, m_pf ); // 1.6.1ck Fix bug of writing settings in wrong order
        WriteContents(s);
        }
	WriteNewline(); // 1.6.1ck 
}

void Object_ofstream::WriteObligatoryString(const char* pszMarker,
        const char* pszQualifier, const char* psz)
{
	Str8 sOut; // 1.6.1bg 
    sOut+= '\\'; 
	sOut+= pszMarker; 
	sOut+= pszQualifier;
	fputs( sOut, m_pf ); // 1.6.1ck Fix bug of writing settings in wrong order
    ASSERT( psz );
    if ( *psz )
        {
        sOut = ' '; // 1.6.1ck 
		fputs( sOut, m_pf ); // 1.6.1ck Fix bug of writing settings in wrong order
        WriteContents(psz);
        }
	WriteNewline(); // 1.6.1ck 
}

void Object_ofstream::WriteContents(const char* pszContents)
{
	Str8 sOut; // 1.6.1bg 
    const char* psz = pszContents;
    ASSERT( psz );
    const char* pszBackslashAtBOL = NULL;
    while ( (pszBackslashAtBOL = strstr(psz, "\n\\")) != NULL )
        {
        pszBackslashAtBOL += 1;  // Point to the backslash;
        // Write the substring up to and including the newline
		Str8 s = psz; // 1.6.1bg 
		sOut+= s.Left( pszBackslashAtBOL - psz ); // 1.6.1bg 
//        m_ios.write(psz, pszBackslashAtBOL - psz);
        sOut+= ' ';  // Write a space before the backslash
        psz = pszBackslashAtBOL;
        ASSERT( *psz == '\\' );
        }
    sOut+= psz;  // Write the rest of the field
	fputs( sOut, m_pf ); // 1.6.1bg 
}

void Object_ofstream::WriteChar(const char* pszMarker, char c)
{
	Str8 sOut; // 1.6.1bg 
    sOut+= '\\'; 
	sOut+= pszMarker ; 
	sOut+= ' '; 
	sOut+= c; 
	sOut+= '\n';
	fputs( sOut, m_pf ); // 1.6.1bg 
}

void Object_ofstream::WriteBool(const char* pszMarker, BOOL b)
{
	Str8 sOut; // 1.6.1bg 
    if ( b )
		{
        sOut+= '\\'; 
		sOut+= pszMarker; 
		sOut+= '\n';
		}
	fputs( sOut, m_pf ); // 1.6.1bg 
}

void Object_ofstream::WriteUnrecognizedSettingsInfo( const char* psz ) // 1.0cm Write unrecognized settings info
{
	Str8 sOut; // 1.6.1bg 
    sOut+= psz;
	fputs( sOut, m_pf ); // 1.6.1bg 
}

void Object_ofstream::WriteCOLORREF(const char* pszMarker, COLORREF rgb)
{
	Str8 sOut; // 1.6.1bg 
    sOut+= '\\'; 
	sOut+= pszMarker ; 
	sOut+= ' '; 
	sOut+= (int)GetRValue(rgb); 
	sOut+= ','; 
	sOut+= (int)GetGValue(rgb); 
	sOut+= ','; 
	sOut+= (int)GetBValue(rgb); 
	sOut+= '\n';
	fputs( sOut, m_pf ); // 1.6.1bg 
}

void Object_ofstream::WriteInteger(const char* pszMarker, int i)
{
	Str8 sOut; // 1.6.1bg 
    sOut+= '\\'; 
	sOut+= pszMarker; 
	sOut+= ' '; 
	sOut+= i; 
	sOut+= '\n';
	fputs( sOut, m_pf ); // 1.6.1bg 
}

void Object_ofstream::WriteNewline()
{
	Str8 sOut; // 1.6.1bg 
    sOut+= '\n';
	fputs( sOut, m_pf ); // 1.6.1bg 
}

void Object_ofstream::WriteWindowPlacement(const char* pszMarker, const CWnd* pwnd)
{
	Str8 sOut; // 1.6.1bg 
    WINDOWPLACEMENT wpl;
    wpl.length = sizeof(wpl);  // Windows API expects this to be initialized
    if ( !pwnd->GetWindowPlacement(&wpl) )
        return;

    sOut+= '\\'; 
	sOut+= pszMarker; 
	sOut+= ' ';

    switch ( wpl.showCmd )
        {
        case SW_SHOWMINIMIZED:
            sOut+= "min";
            break;

        case SW_SHOWMAXIMIZED:
            sOut+= "max";
            break;

        case SW_SHOWNORMAL:
        default:
            sOut+= "normal";
            break;
        }

    // 1998-02-05 MRP: Rewritten to eliminate use of sprintf.
    sOut+= ' '; 
	sOut+= wpl.ptMinPosition.x;
    sOut+= ' '; 
	sOut+= wpl.ptMinPosition.y;
    sOut+= ' '; 
	sOut+= wpl.ptMaxPosition.x;
    sOut+= ' '; 
	sOut+= wpl.ptMaxPosition.y;
    sOut+= ' '; 
	sOut+= wpl.rcNormalPosition.left;
    sOut+= ' '; 
	sOut+= wpl.rcNormalPosition.top;
    sOut+= ' '; 
	sOut+= wpl.rcNormalPosition.right;
    sOut+= ' '; 
	sOut+= wpl.rcNormalPosition.bottom;

    sOut+= '\n';
	fputs( sOut, m_pf ); // 1.6.1bg 
}


#ifdef FileStream // 1.6.1cm  // 1.6.4aa Never define FileStream  // 1.6.4aa ifstream won't compile
// ==========================================================================

static const Length maxsizMString = 32000001; // 1.4qzhz Fix bug of crash on too many file names in corpus

Object_ifstream::Object_ifstream(FILE* pf, CNoteList& notlst) : // 1.6.1da 
    m_notlst(notlst)
{
    m_pf = pf; // 1.6.1da 
    m_pszMStringBuf = new char[maxsizMString];
    m_pszEnd = NULL;
    m_pszMarker = NULL;
    m_pszString = NULL;
    m_bUnRead = FALSE;
    m_chEndOfLine = '\n';  // Default for Mac and Windows
    m_bStringTooLong = FALSE;  //26-07-1997

    // Safely check for optional UTF-8 BOM: check eof first then compare bytes.
    auto c = m_ios.peek();
    if (c != std::char_traits<char>::eof())
    {
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc == 0xEF)
        {
            (void)m_ios.get(); // consume
            c = m_ios.peek();
            if (c != std::char_traits<char>::eof() && static_cast<unsigned char>(c) == 0xBB)
                (void)m_ios.get();
            c = m_ios.peek();
            if (c != std::char_traits<char>::eof() && static_cast<unsigned char>(c) == 0xBF)
                (void)m_ios.get();
        }
    }
}

Object_ifstream::~Object_ifstream()
{
    delete [] m_pszMStringBuf;
}

BOOL Object_ifstream::bAtEnd()
{
    // MRP: TO DO!!! Check whether the following (and bReadField) is right...
    // 1. If marker or contents at EOF with no newline;
    // 2. If explicit Ctrl-Z (code 26) in file.
    
    // Have read to end if no "unread" field, and all chars have been read
    // from the stream. NOTE: Because no attempt has yet been made actually
    // to get chars from beyond the end of the stream, eof() will NOT be true.
    //return ( !m_bUnRead && m_ios.peek() == EOF );
    //return (!m_bUnRead && !m_ios.good());
    // Have read to end if no "unread" field and peek() indicates EOF.
    return (!m_bUnRead && (m_ios.peek() == std::char_traits<char>::eof()));
}

BOOL Object_ifstream::bAtBackslash()
{
    // Stream represents Standard Format if a field has already been read
    // successfully, or if the first char of the stream is a backslash.
    if (m_bUnRead)
        return TRUE;
    int c = m_ios.peek();
    if (c == std::char_traits<char>::eof())
        return FALSE;
    return (c == '\\');
}

BOOL Object_ifstream::bAtBeginMarker()
{
    const char* pszMarkerRead = NULL;
    const char* pszStringRead = NULL;
    ReadMarkedString(&pszMarkerRead, &pszStringRead);

    BOOL bBeginMarker = ( *pszMarkerRead == '+' );
    UnReadMarkedString();
    
    return bBeginMarker;
}

/* 1999-11-29 TLB: No longer needed
BOOL Object_ifstream::bAtBeginOrEndMarker()
{
    const char* pszMarkerRead = NULL;
    const char* pszStringRead = NULL;
    ReadMarkedString(&pszMarkerRead, &pszStringRead);

    BOOL bBeginOrEndMarker = ( *pszMarkerRead == '+' || *pszMarkerRead == '-' );
    UnReadMarkedString();
    
    return bBeginOrEndMarker;
}
*/

BOOL Object_ifstream::bAtBeginMarker(const char* pszMarker, const char* pszName )
{                                                                                                                               
    Str8 sName;
    if ( !bReadBeginMarker(pszMarker, sName) )
        return FALSE;                                           

    BOOL b = !pszName || !strcmp( pszName, sName ); // If name given, it must match
    UnReadMarkedString();       
    return b;
}

BOOL Object_ifstream::bAtBeginMarker(const char* pszMarker, Str8& sName)
{
    if ( !bReadBeginMarker(pszMarker, sName) )
        return FALSE;                                           

    UnReadMarkedString();       
    return TRUE;
}

BOOL Object_ifstream::bAtEndMarker(const char* pszMarker)
{
    if ( !bReadEndMarker(pszMarker) )
        return FALSE;

    UnReadMarkedString();       
    return TRUE;
}


BOOL Object_ifstream::bReadBeginMarker(const char* pszMarker, Str8& sName)
{
    const char* pszMarkerRead = NULL;
    const char* pszStringRead = NULL;
    ReadMarkedString(&pszMarkerRead, &pszStringRead);

    BOOL bBeginMarker = (*pszMarkerRead == '+') && bEqual(pszMarker, pszMarkerRead+1);
    if ( !bBeginMarker )
        {
        UnReadMarkedString();
        return FALSE;
        }

    sName = pszStringRead;      
    return TRUE;
}

BOOL Object_ifstream::bReadMarker(char cFirstChar, const char* pszMarker)
{
    const char* pszMarkerRead = NULL;
    const char* pszStringRead = NULL;
    ReadMarkedString(&pszMarkerRead, &pszStringRead);

    BOOL bMarkerFound = (*pszMarkerRead == cFirstChar) && bEqual(pszMarker, pszMarkerRead+1);
    if ( !bMarkerFound )
        {
        UnReadMarkedString();
        return FALSE;
        }
        
    return TRUE;
}

#define MAXMKRSIZE 50

BOOL Object_ifstream::bReadBeginMarkerWithQualifier(const char* pszMarker, const char* pszQualifier)
{
    if ( !pszQualifier )
        return bReadBeginMarker( pszMarker );

    char buf[MAXMKRSIZE];
    ASSERT( strlen(pszMarker) + strlen(pszQualifier) + 1 < MAXMKRSIZE );
    strcpy( buf, pszMarker );
    strcat( buf, pszQualifier );
    return bReadMarker('+', buf);
}

BOOL Object_ifstream::bReadEndMarkerWithQualifier(const char* pszMarker, const char* pszQualifier)
{
    if ( !pszQualifier )
        return bReadEndMarker( pszMarker );

    char buf[MAXMKRSIZE];
    ASSERT( strlen(pszMarker) + strlen(pszQualifier) + 1 < MAXMKRSIZE );
    strcpy( buf, pszMarker );
    strcat( buf, pszQualifier );
    return bReadMarker('-', buf);
}

BOOL Object_ifstream::bReadString(const char* pszMarker, const char* pszQualifier,
        Str8& s)
{
    const char* pszMarkerRead;
    const char* pszStringRead = NULL;
    ReadMarkedString(&pszMarkerRead, &pszStringRead);

    // 07-26-1997 If the string is longer then 32001 Bytes -> return FALSE 
    if (m_bStringTooLong) return FALSE;

    Length lenMarker = strlen(pszMarker);
    if ( strncmp(pszMarker, pszMarkerRead, lenMarker) != 0 ||
            !bEqual(pszQualifier, pszMarkerRead+lenMarker) )
        {
        UnReadMarkedString();
        return FALSE;
        }
        
    s = pszStringRead;
    return TRUE;
}

BOOL Object_ifstream::bReadString(const char* pszMarker, Str8& s)
{
    const char* pszMarkerRead;
    const char* pszStringRead = NULL;
    ReadMarkedString(&pszMarkerRead, &pszStringRead);

    // 07-26-1997 If the string is longer then 32001 Bytes -> return FALSE 
    if (m_bStringTooLong) return FALSE;
    
    if ( !bEqual(pszMarker, pszMarkerRead) )
        {
        UnReadMarkedString();
        return FALSE;
        }
        
    s = pszStringRead;
    return TRUE;
}

BOOL Object_ifstream::bReadBool(const char* pszMarker, BOOL& b)
{
    Str8 s;
    if ( !bReadString(pszMarker, s) )
        return FALSE;
        
    b = TRUE;
    return TRUE;
}

BOOL Object_ifstream::bReadCOLORREF(const char* pszMarker, COLORREF& rgb)
{
    Str8 s;
    if ( !bReadString(pszMarker, s) )
        return FALSE;
    
    short r = GetRValue(rgb);
    short g = GetGValue(rgb);
    short b = GetBValue(rgb);

    long lColor = 0;
    const char* psz = s;
    char* pszEnd = NULL;
    lColor = strtol(psz, &pszEnd, 10);
    if ( psz != pszEnd )
        r = (short)lColor;

    psz = pszEnd;
    if ( *psz == ',' )
        psz++;
    lColor = strtol(psz, &pszEnd, 10);
    if ( psz != pszEnd )
        g = (short)lColor;

    psz = pszEnd;
    if ( *psz == ',' )
        psz++;
    lColor = strtol(psz, &pszEnd, 10);
    if ( psz != pszEnd )
        b = (short)lColor;

    rgb = RGB(r,g,b);
    return TRUE;
}

BOOL Object_ifstream::bReadInteger(const char* pszMarker, int& i)
{
    Str8 s;
    if ( !bReadString(pszMarker, s) )
        return FALSE;
        
    i = atoi(s);
    return TRUE;
}

BOOL Object_ifstream::bReadDecimal(const char* pszMarker, double& d)
{
    Str8 s;
    if ( !bReadString(pszMarker, s) )
        return FALSE;

    const char* psz = s;
    char* pszEnd = NULL;
    d = strtod(psz, &pszEnd);        
    return TRUE;
}

BOOL Object_ifstream::bUnrecognizedSettingsInfo( const char* pszMarker, Str8& s ) // 1.0cm Store any unrecognized settings info so as not to lose it
{
    if ( bAtEndMarker( pszMarker ) ) // If our own end marker, return false as not unrecognized
        return FALSE;
    else if ( bAtBeginMarker( pszMarker ) ) // If our own begin marker, return false as not unrecognized
        return FALSE;
    else // Else, unrecognized marker, save line and return true to say we ate something
        {           
		if ( m_bUnRead ) // 1.0cn Fix assert, allow for unread from before
			{
			s += '\\'; // 1.0cn Include backslash that was removed from unread stuff
			s += m_pszMarker; // 1.0cn Append unread marker to unrecognized info
			s += ' '; // Append space
			s += m_pszString; // Append unread string to unrecognized info
//			*ppszMarker = m_pszMarker;
//			*ppszString = m_pszString;
			m_bUnRead = FALSE;
			}
		else
			{
			ReadLine(); // 1.0cm Read line of unrecognized marker
			s += m_pszMStringBuf; // 1.0cm Append line to unrecognized info
			}
		s += '\n'; // 1.0cm Append line break
        return TRUE; // Return true to say we ate something
        }
}

BOOL Object_ifstream::bEnd( const char* pszMarker ) // Read given end marker, stop any begin marker, for error recovery on objects with unrecognized names AB 1-18-95
{
    if ( bReadEndMarker( pszMarker ) ) // If desired end marker, return true for successfully found end
        return TRUE;
// 1999-11-29 TLB: The following code was designed to deal with a missing end marker, but it causes problems when
//                 converting from newer to older settings files if the newer settings include a new level
//                 of embedded settings. Since this is the much more common case, we'll allow for unexpected
//                 stuff to be "eaten" line-by-line. This means that a missing end marker will probably result
//                 in the rest of the settings file being eaten. We can still check for an embedded begin
//                 marker for the same object and consider that to be an implicit end since we never recursively
//                 embed objects in a settings file.
    else if ( bAtBeginMarker( pszMarker ) ) // If our own begin marker, return true for found end
        return TRUE;
    else // Else, unrecognized marker, eat marker and return false for not at end yet
        {           
        const char* pszMarkerRead = NULL;
        const char* pszStringRead = NULL;
        ReadMarkedString(&pszMarkerRead, &pszStringRead);
        return FALSE;
        }
}

BOOL Object_ifstream::bEndWithQualifier( const char* pszMarker, const char* pszQualifier )
{
    if ( !pszQualifier )
        return bEnd( pszMarker );

    char buf[MAXMKRSIZE];
    ASSERT( strlen(pszMarker) + strlen(pszQualifier) + 1 < MAXMKRSIZE );
    strcpy( buf, pszMarker );
    strcat( buf, pszQualifier );
    return bEnd( buf );
}

BOOL Object_ifstream::bReadNewObjectProperties(const char* pszMarker,
        Str8& sProperties)
{
    if ( !bAtBeginMarker(pszMarker) )
        return FALSE;

    AppendFieldTo(sProperties);
    BOOL bEndMarker = FALSE;
    while ( !bEndMarker && !bAtEnd() )
        {
        bEndMarker = bAtEndMarker(pszMarker);
        AppendFieldTo(sProperties);
        }

    return TRUE;
}

void Object_ifstream::AppendFieldTo(Str8& sProperties)
{
    const char* pszMarkerRead = NULL;
    const char* pszStringRead = NULL;
    ReadMarkedString(&pszMarkerRead, &pszStringRead);
    sProperties += '\\';
    sProperties += pszMarkerRead;
    if ( *pszStringRead )
        {
        sProperties += ' ';
        sProperties += pszStringRead;
        }
    sProperties += '\n';
}


BOOL Object_ifstream::bReadMainWindowPlacement(const char* pszMarker, CWnd* pwnd)
{
    WINDOWPLACEMENT wpl;        
    if ( bReadWindowPlacement(pszMarker, wpl) )
        {
        if ( wpl.showCmd == SW_SHOWMINIMIZED )
            wpl.showCmd = SW_SHOWNORMAL;
        wpl.ptMinPosition.x = -1;
        wpl.ptMinPosition.y = -1;
        
        pwnd->SetWindowPlacement(&wpl);
        return TRUE;
        }
    else
        return FALSE;
}

// NOTE! This function keeps the window hidden, returning the desired show state in eShowCmd for later use
// This is used by the CShwView class to defer view painting until the view contents are ready,
// after indexing, filtering.
BOOL Object_ifstream::bReadWindowPlacement(const char* pszMarker, CWnd* pwnd, BOOL& eShowCmd)
{
    WINDOWPLACEMENT wpl;        
    if ( bReadWindowPlacement(pszMarker, wpl) )
        {
        eShowCmd = wpl.showCmd;
        wpl.showCmd = SW_HIDE; // keep hidden for now
        pwnd->SetWindowPlacement(&wpl);
        return TRUE;
        }
    else
        return FALSE;
}

BOOL Object_ifstream::bReadWindowPlacement(const char* pszMarker, WINDOWPLACEMENT& wpl)
{
    Str8 sPlacement;
    if ( !bReadString(pszMarker, sPlacement) )
        return FALSE;

    wpl.length = sizeof(wpl);
    wpl.flags = WPF_SETMINPOSITION;
    char pszShowState[7];
    int nItems = sscanf(sPlacement, "%s %d %d %d %d %d %d %d %d",
        pszShowState,
        &wpl.ptMinPosition.x, &wpl.ptMinPosition.y,
        &wpl.ptMaxPosition.x, &wpl.ptMaxPosition.y,
        &wpl.rcNormalPosition.left, &wpl.rcNormalPosition.top,
        &wpl.rcNormalPosition.right, &wpl.rcNormalPosition.bottom);

    if ( nItems != 9 )
        return FALSE;

    if ( bEqual(pszShowState, "normal") )
        wpl.showCmd = SW_SHOWNORMAL;
    else if ( bEqual(pszShowState, "min") )
        wpl.showCmd = SW_SHOWMINIMIZED;
    else if ( bEqual(pszShowState, "max") )
        wpl.showCmd = SW_SHOWMAXIMIZED;
    else
        wpl.showCmd = SW_SHOWNORMAL;;

    return TRUE;        
}


void Object_ifstream::ReadMarkedString(const char** ppszMarker,
        const char** ppszString, BOOL bTrimWhiteSpace)
{
    if (m_bUnRead)
    {
        *ppszMarker = m_pszMarker;
        *ppszString = m_pszString;
        m_bUnRead = FALSE;
        return;
    }

    ReadMarkedLine(ppszMarker, ppszString);

    // Read any additional lines of the marked string
    while (!m_bStringTooLong)
    {
        int ich = m_ios.peek();
        if (ich == std::char_traits<char>::eof())
            break;

        if (ich == '\\')
            break;

        *m_pszEnd++ = '\n';
        ReadLine();
    }
    *m_pszEnd = '\0'; // 1.4qzgj Fix problem of not terminating string on Read
    if ((bTrimWhiteSpace) && (!m_bStringTooLong))
    {
        // Trim white space from the beginning and end of the string
        m_pszString += strspn(m_pszString, s_pszWhiteSpace);      //BDW 9-2-97
        if (*m_pszString)  // string contains non-ws
        {
            char* psz = m_pszString + strlen(m_pszString) - 1;  // last char
            while (strchr(s_pszWhiteSpace, *psz) != NULL)
            {
                ASSERT(psz != m_pszString);  // have not moved past beginning
                *psz-- = '\0';  // remove last char
            }
        }
    }

    *ppszString = m_pszString;
}

void Object_ifstream::UnReadMarkedString()
{
    ASSERT( !m_bUnRead );
    m_bUnRead = TRUE;
}


void Object_ifstream::ReadMarkedLine(const char** ppszMarker, const char** ppszString)
{
    ASSERT( !m_bUnRead );
    char* psz = m_pszEnd = m_pszMStringBuf;
    *psz = '\0';
    ReadLine();
	*m_pszEnd = '\0'; // 1.4qzgj
    
    // Caller is responsible to not to read to end, and also to check that
    // the file is indeed Standard Format (i.e. it begins with a backslash)
    ASSERT( *psz == '\\' ); 
    psz += 1; // move past the backslash

    m_pszMarker = psz;
    Length lenMarker = strcspn(psz, " \t\n");
    // 1996-11-04 MRP: This temporary patch will skip the rest of a field
    // that contains a backslash at the beginning of one of its lines.
    // The real fix is to use the read-line-ahead approach in sfstream.cpp
    // ASSERT( lenMarker != 0 ); // RECOVERY
    psz += lenMarker;

    if ( *psz == ' ' )
        {
        // A space delimiting the marker is not considered part of the string
        *psz = '\0'; // so the marker's null termination can take its place
        psz += 1;
        }
    else
        {
        // 1995-10-04 MRP: At first I thought this shifting was redundant
        // if no white space followed the marker. If the field consisted
        // of just that one line that would be true. If, however, the
        // field's content follows in additional lines we must do it.
        //
        // Move the marker left one position (covering the backslash)
        m_pszMarker = m_pszMStringBuf;
        memcpy(m_pszMarker, m_pszMStringBuf + 1, lenMarker);
        ASSERT( m_pszMarker + lenMarker == psz - 1 );
        m_pszMarker[lenMarker] = '\0'; // making a place for its null
        }

    *ppszMarker = m_pszMarker; // beginning of marker       
    *ppszString = m_pszString = psz; // beginning of string
}

void Object_ifstream::ReadLine()
{
    ASSERT(!m_bUnRead);
    // Remaining space in buffer including room for terminating NUL
    Length sizRemaining = maxsizMString - (m_pszEnd - m_pszMStringBuf);
    // Max characters we may copy (leave room for terminating NUL as original did)
    size_t maxCopy = (sizRemaining > 0) ? (size_t)(sizRemaining - 1) : 0;

    std::string line;
    std::getline(m_ios, line, m_chEndOfLine); // consumes the delimiter

    // Strip possible trailing CR when reading CRLF text files
    if (!line.empty() && line.back() == '\r')
        line.pop_back();

    // Detect if the line exceeds our buffer capacity
    if (line.size() > maxCopy)
        m_bStringTooLong = TRUE;

    // Copy up to maxCopy characters into buffer
    size_t copyLen = (line.size() <= maxCopy) ? line.size() : maxCopy;
    if (copyLen > 0)
    {
        memcpy(m_pszEnd, line.data(), copyLen);
        m_pszEnd += copyLen;
    }
    *m_pszEnd = '\0';
}

// **************************************************************************

#endif // FileStream // 1.6.1cm  // 1.6.4aa Never define FileStream

// **************************************************************************

void Object_ostream::WriteUTF8BOM() // 1.4qnh Write UTF8 bom Byte Order Mark
	{
	m_ios << (char)0xef; // 1.4qnh Write bom
	m_ios << (char)0xbb; // 1.4qnh Write bom
	m_ios << (char)0xbf; // 1.4qnh Write bom
	}

void Object_ostream::WriteBeginMarker(const char* pszMarker, const char* pszName)
{
    ASSERT( pszName );
//  ASSERT( *pszName );
    m_ios << "\\+" << pszMarker << ' ' << pszName << '\n';
}

void Object_ostream::WriteEndMarker(const char* pszMarker)
{
    m_ios << "\\-" << pszMarker << '\n';
}

void Object_ostream::WriteBeginMarkerWithQualifier(const char* pszMarker, const char* pszQualifier,
        const char* pszName)
{
    if ( !pszQualifier )
        {
        WriteBeginMarker( pszMarker, pszName );
        return;
        }

    ASSERT( pszName );
    m_ios << "\\+" << pszMarker << pszQualifier << ' ' << pszName << '\n';
}

void Object_ostream::WriteEndMarkerWithQualifier(const char* pszMarker, const char* pszQualifier)
{
    if ( !pszQualifier )
        {
        WriteEndMarker( pszMarker );
        return;
        }

    m_ios << "\\-" << pszMarker << pszQualifier << '\n';
}


void Object_ostream::WriteString(const char* pszMarker, const char* pszQualifier,
        const char* psz)
{
    ASSERT( psz );
    if ( !*psz )
        return;

    m_ios << '\\' << pszMarker << pszQualifier << ' ';
    WriteContents(psz);
    m_ios << '\n';
}

void Object_ostream::WriteString(const char* pszMarker, const char* psz)
{
    ASSERT( psz );
    if ( !*psz )
        return;

    m_ios << '\\' << pszMarker << ' ';
    WriteContents(psz);
    m_ios << '\n';
}

void Object_ostream::WriteObligatoryString(const char* pszMarker,
        const Str8& s)
{
    m_ios << '\\' << pszMarker;
    if ( !s.IsEmpty() )  // 1999-04-01 MRP: Replace incorrect condition
        {
        m_ios << ' ';
        WriteContents(s);
        }
    m_ios << '\n';
}

void Object_ostream::WriteObligatoryString(const char* pszMarker,
        const char* pszQualifier, const char* psz)
{
    m_ios << '\\' << pszMarker << pszQualifier;
    ASSERT( psz );
    if ( *psz )
        {
        m_ios << ' ';
        WriteContents(psz);
        }
    m_ios << '\n';
}

void Object_ostream::WriteContents(const char* pszContents)
{
    const char* psz = pszContents;
    ASSERT( psz );
    const char* pszBackslashAtBOL = NULL;
    while ( (pszBackslashAtBOL = strstr(psz, "\n\\")) != NULL )
        {
        pszBackslashAtBOL += 1;  // Point to the backslash;
        // Write the substring up to and including the newline
        m_ios.write(psz, pszBackslashAtBOL - psz);
        m_ios << ' ';  // Write a space before the backslash
        psz = pszBackslashAtBOL;
        ASSERT( *psz == '\\' );
        }
    m_ios << psz;  // Write the rest of the field
}

void Object_ostream::WriteChar(const char* pszMarker, char c)
{
    m_ios << '\\' << pszMarker << ' ' << c << '\n';
}

void Object_ostream::WriteBool(const char* pszMarker, BOOL b)
{
    if ( b )
        m_ios << '\\' << pszMarker << '\n';
}

void Object_ostream::WriteUnrecognizedSettingsInfo( const char* psz ) // 1.0cm Write unrecognized settings info
{
    m_ios << psz;
}

void Object_ostream::WriteCOLORREF(const char* pszMarker, COLORREF rgb)
{
    m_ios << '\\' << pszMarker << ' ' <<
        (int)GetRValue(rgb) << ',' << (int)GetGValue(rgb) << ',' << (int)GetBValue(rgb) << '\n';
}

void Object_ostream::WriteInteger(const char* pszMarker, int i)
{
    m_ios << '\\' << pszMarker << ' ' << i << '\n';
}
    
void Object_ostream::WriteNewline()
{
    m_ios << '\n';
}

void Object_ostream::WriteWindowPlacement(const char* pszMarker, const CWnd* pwnd)
{
    WINDOWPLACEMENT wpl;
    wpl.length = sizeof(wpl);  // Windows API expects this to be initialized
    if ( !pwnd->GetWindowPlacement(&wpl) )
        return;

    m_ios << '\\' << pszMarker << ' ';

    switch ( wpl.showCmd )
        {
        case SW_SHOWMINIMIZED:
            m_ios << "min";
            break;

        case SW_SHOWMAXIMIZED:
            m_ios << "max";
            break;

        case SW_SHOWNORMAL:
        default:
            m_ios << "normal";
            break;
        }

    // 1998-02-05 MRP: Rewritten to eliminate use of sprintf.
    m_ios << ' ' << wpl.ptMinPosition.x;
    m_ios << ' ' << wpl.ptMinPosition.y;
    m_ios << ' ' << wpl.ptMaxPosition.x;
    m_ios << ' ' << wpl.ptMaxPosition.y;
    m_ios << ' ' << wpl.rcNormalPosition.left;
    m_ios << ' ' << wpl.rcNormalPosition.top;
    m_ios << ' ' << wpl.rcNormalPosition.right;
    m_ios << ' ' << wpl.rcNormalPosition.bottom;

    m_ios << '\n';
}


// ==========================================================================

static const Length maxsizMString = 32000001; // 1.4qzhz Fix bug of crash on too many file names in corpus

#ifdef OBSTREAM_NOTES
Object_istream::Object_istream(std::istream& ios, CNoteList& notlst) :
    m_ios(ios),
    m_notlst(notlst)
{
    m_pszMStringBuf = new char[maxsizMString];
    m_pszEnd = NULL;
    m_pszMarker = NULL;
    m_pszString = NULL;
    m_bUnRead = FALSE;
    m_chEndOfLine = '\n';  // Default for Mac and Windows
    m_bStringTooLong = FALSE;  //26-07-1997

    // Safely check for optional UTF-8 BOM: check eof first then compare bytes.
    auto c = m_ios.peek();
    if (c != std::char_traits<char>::eof())
    {
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc == 0xEF)
        {
            (void)m_ios.get(); // move past possible UTF-8 BOM before first \ in first line
            c = m_ios.peek();
            if (c != std::char_traits<char>::eof() && static_cast<unsigned char>(c) == 0xBB)
                (void)m_ios.get(); // move past possible UTF-8 BOM before first \ in first line
            c = m_ios.peek();
            if (c != std::char_traits<char>::eof() && static_cast<unsigned char>(c) == 0xBF)
                (void)m_ios.get(); // move past possible UTF-8 BOM before first \ in first line
        }
    }
}
#else
Object_istream::Object_istream(std::istream& ios) :
    m_ios(ios)
{
    m_pszMStringBuf = new char[maxsizMString];
    m_pszEnd = NULL;
    m_pszMarker = NULL;
    m_pszString = NULL;
    m_bUnRead = FALSE;
    m_chEndOfLine = '\n';  // Default for Mac and Windows
    m_bStringTooLong = FALSE;    // 26-07-1997
}
#endif

Object_istream::~Object_istream()
{
    delete [] m_pszMStringBuf;
}



BOOL Object_istream::bAtEnd()
{
    // MRP: TO DO!!! Check whether the following (and bReadField) is right...
    // 1. If marker or contents at EOF with no newline;
    // 2. If explicit Ctrl-Z (code 26) in file.
    
    // Have read to end if no "unread" field, and all chars have been read
    // from the stream. NOTE: Because no attempt has yet been made actually
    // to get chars from beyond the end of the stream, eof() will NOT be true.
    // return ( !m_bUnRead && m_ios.peek() == EOF );
    //return ( !m_bUnRead && !m_ios.good() );
    return (!m_bUnRead && (m_ios.peek() == std::char_traits<char>::eof()));
}

BOOL Object_istream::bAtBackslash()
{
    // Stream represents Standard Format if a field has already been read
    // successfully, or if the first char of the stream is a backslash.
    if (m_bUnRead)
        return TRUE;
    int c = m_ios.peek();
    if (c == std::char_traits<char>::eof())
        return FALSE;
    return (c == '\\');
}

BOOL Object_istream::bAtBeginMarker()
{
    const char* pszMarkerRead = NULL;
    const char* pszStringRead = NULL;
    ReadMarkedString(&pszMarkerRead, &pszStringRead);

    BOOL bBeginMarker = ( *pszMarkerRead == '+' );
    UnReadMarkedString();
    
    return bBeginMarker;
}

/* 1999-11-29 TLB: No longer needed
BOOL Object_istream::bAtBeginOrEndMarker()
{
    const char* pszMarkerRead = NULL;
    const char* pszStringRead = NULL;
    ReadMarkedString(&pszMarkerRead, &pszStringRead);

    BOOL bBeginOrEndMarker = ( *pszMarkerRead == '+' || *pszMarkerRead == '-' );
    UnReadMarkedString();
    
    return bBeginOrEndMarker;
}
*/

BOOL Object_istream::bAtBeginMarker(const char* pszMarker, const char* pszName )
{                                                                                                                               
    Str8 sName;
    if ( !bReadBeginMarker(pszMarker, sName) )
        return FALSE;                                           

    BOOL b = !pszName || !strcmp( pszName, sName ); // If name given, it must match
    UnReadMarkedString();       
    return b;
}

BOOL Object_istream::bAtBeginMarker(const char* pszMarker, Str8& sName)
{
    if ( !bReadBeginMarker(pszMarker, sName) )
        return FALSE;                                           

    UnReadMarkedString();       
    return TRUE;
}

BOOL Object_istream::bAtEndMarker(const char* pszMarker)
{
    if ( !bReadEndMarker(pszMarker) )
        return FALSE;

    UnReadMarkedString();       
    return TRUE;
}


BOOL Object_istream::bReadBeginMarker(const char* pszMarker, Str8& sName)
{
    const char* pszMarkerRead = NULL;
    const char* pszStringRead = NULL;
    ReadMarkedString(&pszMarkerRead, &pszStringRead);

    BOOL bBeginMarker = (*pszMarkerRead == '+') && bEqual(pszMarker, pszMarkerRead+1);
    if ( !bBeginMarker )
        {
        UnReadMarkedString();
        return FALSE;
        }

    sName = pszStringRead;      
    return TRUE;
}

BOOL Object_istream::bReadMarker(char cFirstChar, const char* pszMarker)
{
    const char* pszMarkerRead = NULL;
    const char* pszStringRead = NULL;
    ReadMarkedString(&pszMarkerRead, &pszStringRead);

    BOOL bMarkerFound = (*pszMarkerRead == cFirstChar) && bEqual(pszMarker, pszMarkerRead+1);
    if ( !bMarkerFound )
        {
        UnReadMarkedString();
        return FALSE;
        }
        
    return TRUE;
}

#define MAXMKRSIZE 50

BOOL Object_istream::bReadBeginMarkerWithQualifier(const char* pszMarker, const char* pszQualifier)
{
    if ( !pszQualifier )
        return bReadBeginMarker( pszMarker );

    char buf[MAXMKRSIZE];
    ASSERT( strlen(pszMarker) + strlen(pszQualifier) + 1 < MAXMKRSIZE );
    strcpy_s( buf, sizeof(buf), pszMarker );
    strcat_s( buf, sizeof(buf), pszQualifier );
    return bReadMarker('+', buf);
}

BOOL Object_istream::bReadEndMarkerWithQualifier(const char* pszMarker, const char* pszQualifier)
{
    if ( !pszQualifier )
        return bReadEndMarker( pszMarker );

    char buf[MAXMKRSIZE];
    ASSERT( strlen(pszMarker) + strlen(pszQualifier) + 1 < MAXMKRSIZE );
    strcpy_s( buf, sizeof(buf), pszMarker );
    strcat_s( buf, sizeof(buf), pszQualifier );
    return bReadMarker('-', buf);
}

BOOL Object_istream::bReadString(const char* pszMarker, const char* pszQualifier,
        Str8& s)
{
    const char* pszMarkerRead;
    const char* pszStringRead = NULL;
    ReadMarkedString(&pszMarkerRead, &pszStringRead);

    // 07-26-1997 If the string is longer then 32001 Bytes -> return FALSE 
    if (m_bStringTooLong) return FALSE;

    Length lenMarker = strlen(pszMarker);
    if ( strncmp(pszMarker, pszMarkerRead, lenMarker) != 0 ||
            !bEqual(pszQualifier, pszMarkerRead+lenMarker) )
        {
        UnReadMarkedString();
        return FALSE;
        }
        
    s = pszStringRead;
    return TRUE;
}

BOOL Object_istream::bReadString(const char* pszMarker, Str8& s)
{
    const char* pszMarkerRead;
    const char* pszStringRead = NULL;
    ReadMarkedString(&pszMarkerRead, &pszStringRead);

    // 07-26-1997 If the string is longer then 32001 Bytes -> return FALSE 
    if (m_bStringTooLong) return FALSE;
    
    if ( !bEqual(pszMarker, pszMarkerRead) )
        {
        UnReadMarkedString();
        return FALSE;
        }
        
    s = pszStringRead;
    return TRUE;
}

BOOL Object_istream::bReadBool(const char* pszMarker, BOOL& b)
{
    Str8 s;
    if ( !bReadString(pszMarker, s) )
        return FALSE;
        
    b = TRUE;
    return TRUE;
}

BOOL Object_istream::bReadCOLORREF(const char* pszMarker, COLORREF& rgb)
{
    Str8 s;
    if ( !bReadString(pszMarker, s) )
        return FALSE;
    
    short r = GetRValue(rgb);
    short g = GetGValue(rgb);
    short b = GetBValue(rgb);

    long lColor = 0;
    const char* psz = s;
    char* pszEnd = NULL;
    lColor = strtol(psz, &pszEnd, 10);
    if ( psz != pszEnd )
        r = (short)lColor;

    psz = pszEnd;
    if ( *psz == ',' )
        psz++;
    lColor = strtol(psz, &pszEnd, 10);
    if ( psz != pszEnd )
        g = (short)lColor;

    psz = pszEnd;
    if ( *psz == ',' )
        psz++;
    lColor = strtol(psz, &pszEnd, 10);
    if ( psz != pszEnd )
        b = (short)lColor;

    rgb = RGB(r,g,b);
    return TRUE;
}

BOOL Object_istream::bReadInteger(const char* pszMarker, int& i)
{
    Str8 s;
    if ( !bReadString(pszMarker, s) )
        return FALSE;
        
    i = atoi(s);
    return TRUE;
}

BOOL Object_istream::bReadDecimal(const char* pszMarker, double& d)
{
    Str8 s;
    if ( !bReadString(pszMarker, s) )
        return FALSE;

    const char* psz = s;
    char* pszEnd = NULL;
    d = strtod(psz, &pszEnd);        
    return TRUE;
}

BOOL Object_istream::bUnrecognizedSettingsInfo( const char* pszMarker, Str8& s ) // 1.0cm Store any unrecognized settings info so as not to lose it
{
    if ( bAtEndMarker( pszMarker ) ) // If our own end marker, return false as not unrecognized
        return FALSE;
    else if ( bAtBeginMarker( pszMarker ) ) // If our own begin marker, return false as not unrecognized
        return FALSE;
    else // Else, unrecognized marker, save line and return true to say we ate something
        {           
		if ( m_bUnRead ) // 1.0cn Fix assert, allow for unread from before
			{
			s += '\\'; // 1.0cn Include backslash that was removed from unread stuff
			s += m_pszMarker; // 1.0cn Append unread marker to unrecognized info
			s += ' '; // Append space
			s += m_pszString; // Append unread string to unrecognized info
//			*ppszMarker = m_pszMarker;
//			*ppszString = m_pszString;
			m_bUnRead = FALSE;
			}
		else
			{
			ReadLine(); // 1.0cm Read line of unrecognized marker
			s += m_pszMStringBuf; // 1.0cm Append line to unrecognized info
			}
		s += '\n'; // 1.0cm Append line break
        return TRUE; // Return true to say we ate something
        }
}

BOOL Object_istream::bEnd( const char* pszMarker ) // Read given end marker, stop any begin marker, for error recovery on objects with unrecognized names AB 1-18-95
{
    if ( bReadEndMarker( pszMarker ) ) // If desired end marker, return true for successfully found end
        return TRUE;
// 1999-11-29 TLB: The following code was designed to deal with a missing end marker, but it causes problems when
//                 converting from newer to older settings files if the newer settings include a new level
//                 of embedded settings. Since this is the much more common case, we'll allow for unexpected
//                 stuff to be "eaten" line-by-line. This means that a missing end marker will probably result
//                 in the rest of the settings file being eaten. We can still check for an embedded begin
//                 marker for the same object and consider that to be an implicit end since we never recursively
//                 embed objects in a settings file.
    else if ( bAtBeginMarker( pszMarker ) ) // If our own begin marker, return true for found end
        return TRUE;
    else // Else, unrecognized marker, eat marker and return false for not at end yet
        {           
        const char* pszMarkerRead = NULL;
        const char* pszStringRead = NULL;
        ReadMarkedString(&pszMarkerRead, &pszStringRead);
        return FALSE;
        }
}

BOOL Object_istream::bEndWithQualifier( const char* pszMarker, const char* pszQualifier )
{
    if ( !pszQualifier )
        return bEnd( pszMarker );

    char buf[MAXMKRSIZE];
    ASSERT( strlen(pszMarker) + strlen(pszQualifier) + 1 < MAXMKRSIZE );
    strcpy_s( buf, sizeof(buf), pszMarker );
    strcat_s( buf, sizeof(buf), pszQualifier );
    return bEnd( buf );
}

BOOL Object_istream::bReadNewObjectProperties(const char* pszMarker,
        Str8& sProperties)
{
    if ( !bAtBeginMarker(pszMarker) )
        return FALSE;

    AppendFieldTo(sProperties);
    BOOL bEndMarker = FALSE;
    while ( !bEndMarker && !bAtEnd() )
        {
        bEndMarker = bAtEndMarker(pszMarker);
        AppendFieldTo(sProperties);
        }

    return TRUE;
}

void Object_istream::AppendFieldTo(Str8& sProperties)
{
    const char* pszMarkerRead = NULL;
    const char* pszStringRead = NULL;
    ReadMarkedString(&pszMarkerRead, &pszStringRead);
    sProperties += '\\';
    sProperties += pszMarkerRead;
    if ( *pszStringRead )
        {
        sProperties += ' ';
        sProperties += pszStringRead;
        }
    sProperties += '\n';
}


BOOL Object_istream::bReadMainWindowPlacement(const char* pszMarker, CWnd* pwnd)
{
    WINDOWPLACEMENT wpl;        
    if ( bReadWindowPlacement(pszMarker, wpl) )
        {
        if ( wpl.showCmd == SW_SHOWMINIMIZED )
            wpl.showCmd = SW_SHOWNORMAL;
        wpl.ptMinPosition.x = -1;
        wpl.ptMinPosition.y = -1;
        
        pwnd->SetWindowPlacement(&wpl);
        return TRUE;
        }
    else
        return FALSE;
}

// NOTE! This function keeps the window hidden, returning the desired show state in eShowCmd for later use
// This is used by the CShwView class to defer view painting until the view contents are ready,
// after indexing, filtering.
BOOL Object_istream::bReadWindowPlacement(const char* pszMarker, CWnd* pwnd, BOOL& eShowCmd)
{
    WINDOWPLACEMENT wpl;        
    if ( bReadWindowPlacement(pszMarker, wpl) )
        {
        eShowCmd = wpl.showCmd;
        wpl.showCmd = SW_HIDE; // keep hidden for now
        pwnd->SetWindowPlacement(&wpl);
        return TRUE;
        }
    else
        return FALSE;
}

BOOL Object_istream::bReadWindowPlacement(const char* pszMarker, WINDOWPLACEMENT& wpl)
{
    Str8 sPlacement;
    if ( !bReadString(pszMarker, sPlacement) )
        return FALSE;

    wpl.length = sizeof(wpl);
    wpl.flags = WPF_SETMINPOSITION;
    char pszShowState[7];
    int nItems = sscanf_s(sPlacement, "%s %d %d %d %d %d %d %d %d",
        pszShowState, sizeof(pszShowState),
        &wpl.ptMinPosition.x, &wpl.ptMinPosition.y,
        &wpl.ptMaxPosition.x, &wpl.ptMaxPosition.y,
        &wpl.rcNormalPosition.left, &wpl.rcNormalPosition.top,
        &wpl.rcNormalPosition.right, &wpl.rcNormalPosition.bottom);

    if ( nItems != 9 )
        return FALSE;

    if ( bEqual(pszShowState, "normal") )
        wpl.showCmd = SW_SHOWNORMAL;
    else if ( bEqual(pszShowState, "min") )
        wpl.showCmd = SW_SHOWMINIMIZED;
    else if ( bEqual(pszShowState, "max") )
        wpl.showCmd = SW_SHOWMAXIMIZED;
    else
        wpl.showCmd = SW_SHOWNORMAL;;

    return TRUE;        
}


void Object_istream::ReadMarkedString(const char** ppszMarker,
        const char** ppszString, BOOL bTrimWhiteSpace)
{
    if (m_bUnRead)
    {
        *ppszMarker = m_pszMarker;
        *ppszString = m_pszString;
        m_bUnRead = FALSE;
        return;
    }

    ReadMarkedLine(ppszMarker, ppszString);

    // Read any additional lines of the marked string
    while (!m_bStringTooLong)
    {
        int ich = m_ios.peek();

        if (ich == std::char_traits<char>::eof())
            break;

        if (ich == '\\')
            break;

        *m_pszEnd++ = '\n';
        ReadLine();
    }
    *m_pszEnd = '\0'; // 1.4qzgj Fix problem of not terminating string on Read
    if ((bTrimWhiteSpace) && (!m_bStringTooLong))
    {
        // Trim white space from the beginning and end of the string
        m_pszString += strspn(m_pszString, s_pszWhiteSpace);      //BDW 9-2-97
        if (*m_pszString)  // string contains non-ws
        {
            char* psz = m_pszString + strlen(m_pszString) - 1;  // last char
            while (strchr(s_pszWhiteSpace, *psz) != NULL)
            {
                ASSERT(psz != m_pszString);  // have not moved past beginning
                *psz-- = '\0';  // remove last char
            }
        }
    }

    *ppszString = m_pszString;
}

void Object_istream::UnReadMarkedString()
{
    ASSERT( !m_bUnRead );
    m_bUnRead = TRUE;
}


void Object_istream::ReadMarkedLine(const char** ppszMarker, const char** ppszString)
{
    ASSERT( !m_bUnRead );
    char* psz = m_pszEnd = m_pszMStringBuf;
    *psz = '\0';
    ReadLine();
	*m_pszEnd = '\0'; // 1.4qzgj
    
    // Caller is responsible to not to read to end, and also to check that
    // the file is indeed Standard Format (i.e. it begins with a backslash)
    ASSERT( *psz == '\\' ); 
    psz += 1; // move past the backslash

    m_pszMarker = psz;
    Length lenMarker = strcspn(psz, " \t\n");
    // 1996-11-04 MRP: This temporary patch will skip the rest of a field
    // that contains a backslash at the beginning of one of its lines.
    // The real fix is to use the read-line-ahead approach in sfstream.cpp
    // ASSERT( lenMarker != 0 ); // RECOVERY
    psz += lenMarker;

    if ( *psz == ' ' )
        {
        // A space delimiting the marker is not considered part of the string
        *psz = '\0'; // so the marker's null termination can take its place
        psz += 1;
        }
    else
        {
        // 1995-10-04 MRP: At first I thought this shifting was redundant
        // if no white space followed the marker. If the field consisted
        // of just that one line that would be true. If, however, the
        // field's content follows in additional lines we must do it.
        //
        // Move the marker left one position (covering the backslash)
        m_pszMarker = m_pszMStringBuf;
        memcpy(m_pszMarker, m_pszMStringBuf + 1, lenMarker);
        ASSERT( m_pszMarker + lenMarker == psz - 1 );
        m_pszMarker[lenMarker] = '\0'; // making a place for its null
        }

    *ppszMarker = m_pszMarker; // beginning of marker       
    *ppszString = m_pszString = psz; // beginning of string
}

void Object_istream::ReadLine()
{
    ASSERT(!m_bUnRead);
    // Remaining space in buffer including room for terminating NUL
    Length sizRemaining = maxsizMString - (m_pszEnd - m_pszMStringBuf);
    // Max characters we may copy (leave room for terminating NUL as original did)
    size_t maxCopy = (sizRemaining > 0) ? (size_t)(sizRemaining - 1) : 0;

    std::string line;
    std::getline(m_ios, line, m_chEndOfLine); // consumes the delimiter

    // Strip possible trailing CR when reading CRLF text files
    if (!line.empty() && line.back() == '\r')
        line.pop_back();

    // Detect if the line exceeds our buffer capacity
    if (line.size() > maxCopy)
        m_bStringTooLong = TRUE;

    // Copy up to maxCopy characters into buffer
    size_t copyLen = (line.size() <= maxCopy) ? line.size() : maxCopy;
    if (copyLen > 0)
    {
        memcpy(m_pszEnd, line.data(), copyLen);
        m_pszEnd += copyLen;
    }
    *m_pszEnd = '\0';
}

// **************************************************************************

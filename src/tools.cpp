#ifdef _MAC
#include <files.h> // needed for FSSpec stuff
#endif

#include "stdafx.h"
#include "toolbox.h"

#include "tools.h"
#include "shw.h"
#include "resource.h"
#include <ctype.h>  // isalnum
#include <io.h>  // _access
#include <sys\stat.h> // _stat  
#include <windowsx.h>
#include <malloc.h>
#include <afxpriv.h> // 1.5.0gd For USES_CONVERSION
#include <atlconv.h>

extern Str8 g_sVersion;

// #ifndef _DEBUG // 1.1sd Allow MyAssertFailed to be defined in debug version
void MyAssertFailedLine(LPCSTR lpszFileName, int nLine)
{
    char pszLine[11];
    _itoa_s(nLine, pszLine, (int)sizeof(pszLine), 10);
	Str8 sFileName = sGetFileName(lpszFileName, TRUE);
    Str8 sLineInFile;
    Str8 sMessage = "Internal error."; // 1.5.0fg 
	sMessage = sMessage + " " + "Toolbox" + " " + g_sVersion + " " + "File:" + " " + sFileName + " " + "Line:" + " " + pszLine; // 1.5.0fg 
    AfxMessageBox(sMessage, MB_ICONEXCLAMATION);
}
// #endif  // ifndef _DEBUG

Str8 g_sCurrentExternalJumpRef; // 1.5.9pc Fix bug of wrong jump from Paratext

extern UINT NEAR WM_SANTA_FE_FOCUS;
extern LPARAM FROM_TOOLBOX; // 1.2md Signal that this focus message is from Tbx
void SendParallelMoveRefExternal( Str8 sFocus ) // 1.4kr
	{
	Str8 sRef = sFocus; // 1.5.9pc Set ref to trim spaces
	sRef.Trim(); // 1.5.9pc Trim extra spaces
	if ( sRef == g_sCurrentExternalJumpRef ) // 1.5.9pc If same ref as most recent, don't jump
		return; // 1.5.9pc 
	g_sCurrentExternalJumpRef = sRef; // 1.5.9pc Remember current external ref 
	sFocus.Replace( ".", " " ); // 1.2md Change reference from MAT.1:1 to TW and Paratext style MAT 1:17 // 1.4heb Change Replace args to string
	CString swFocus = swUTF16( sFocus ); // 1.4qto Upgrade RetSetValue for Unicode build
	Str8 sRegPath = "Software\\SantaFe\\Focus\\ScriptureReference"; // 1.4qto
	CString swRegPath = swUTF16( sRegPath ); // 1.4qto
	::RegSetValue( HKEY_CURRENT_USER, swRegPath, REG_SZ, swFocus, swFocus.GetLength() ); // 1.2md Put ref into registry // 1.4qto
	::PostMessage( HWND_BROADCAST, WM_SANTA_FE_FOCUS, 1, FROM_TOOLBOX); // 1.2md Post ref message for TW and Paratext to follow
	}

BOOL bValidFldLen( int iLen ) // return TRUE if iLen is a valid field length
{
    return TRUE;
}


#ifdef _WINDOWS
LONG lFileSize( Str8 sFile ) // 1.5.9mj Change to Str8
{
    struct _stat buf;
    if ( _wstat( swUTF16( sFile ), &buf ) ) // returns 0 if successful // 1.5.9mj Change to wstat
        return 0;
    else
        return buf.st_size;
}

BOOL bFileExists( Str8 sFile ) // 1.5.9mj Change to Str8
{
    return _waccess( swUTF16( sFile ), 0) == 0; // _access returns 0 on success // 1.5.9mj Change to waccess
}

BOOL bFileReadOnly( Str8 sPath ) // 1.5.9mf Test file for read only // 1.5.9mj Change to Str8
{
	return _waccess( swUTF16( sPath ), 2) != 0; // 1.5.9mf _access returns 0 on success // 1.5.9mj Change to waccess
}

Str8 sChangeFileExt(const char* pszFile, const char* pszExt)
{
#ifdef _MAC
    char dir[256];
    char fname[256];
    char ext[6];

    MacSplitPath( pszFile, dir, fname, ext );
    Str8 sNewFile = dir;
    if ( *dir )
        sNewFile += ":";
    sNewFile += fname;
    if ( *pszExt != '.' )
        sNewFile += ".";
    sNewFile += pszExt;
#else
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    char fname[_MAX_FNAME];
    char ext[_MAX_EXT];

	_splitpath_s(pszFile, drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT);
	std::string newPath = std::string(drive) + dir + fname + pszExt;
	Str8 sNewFile = newPath.c_str();
#endif // _MAC

    return sNewFile;
}

#ifdef _MAC
// Mac equivalent (more or less) to _splitpath()
void MacSplitPath( const char* pszSpec, char* pszPath, char* pszFName, char* pszExt )
{
    ASSERT( strlen(pszSpec) < 256 );
    const char* pszDirPath = NULL;

    const char* pszName = pszSpec;
    for (const char* psz = pszSpec; *psz != '\0'; psz++)
        {
        // remember last directory/drive separator
        if (*psz == ':')
            {
            pszName = psz+1;
            pszDirPath = pszSpec;
            }
        }
    if ( pszPath ) // caller wants path
        {
        if ( pszDirPath )
            {
            int iLen = pszName - pszDirPath - 1;
            memcpy( pszPath, pszDirPath, iLen ); // copy path component
            *( pszPath + iLen ) = '\0';
            }
        else
            *pszPath = '\0'; // no path found
        }

    const char* p = strrchr( pszName, '.' ); // look for extension
    if ( p && p > pszName && strlen(p) <= 4 ) // don't count extensions longer than 3 chars
        {
        if ( pszExt )
            strcpy( pszExt, p ); // copy extension to caller
        if ( pszFName )
            {
            int iLen = p - pszName;
            memcpy( pszFName, pszName, iLen ); // copy filename to caller
            *( pszFName + iLen ) = '\0';
            }
        }
    else // extension not found
        {
        if ( pszExt )
            *pszExt = '\0';
        if ( pszFName )
            strcpy( pszFName, pszName ); // copy filename to caller
        }
}
#endif // _MAC

#ifdef _MAC

Str8 sGetFullPath( const char* pszFormattedBuffer ) // convert formatted buffer to full pathname
{
    Str8 sResult;
    char* psz;
    ::GetFullPathName(pszFormattedBuffer, MAX_PATH, sResult.GetBuffer(MAX_PATH), &psz); // 1.4qzfq GetBuffer OK because written immediately
    sResult.ReleaseBuffer();
    return sResult;
}

Str8 sGetFormattedBuffer( const char* pszFullPath ) // convert full pathname to formatted buffer suitable for WPL functions
{
    if ( strlen( pszFullPath ) < 256 )
        {
        unsigned char pszPath[256];
        strcpy( (char*)pszPath, pszFullPath );
        _c2pstr( (char*)pszPath ); // convert to pascal string
        struct FSSpec fss;
        FSMakeFSSpec( 0, 0, pszPath, &fss ); // convert pathname to FSSpec
        char pszFBuf[256];
        WrapFile( &fss, pszFBuf, sizeof(pszFBuf) ); // convert FSSpec to formatted buffer
        return Str8( pszFBuf );
        }
    ASSERT(FALSE);
    return Str8( pszFullPath );  
}

void SetFileType( const char* pszFullPath, OSType filetype, OSType creator ) // set the filetype and creator for specified file
{
    struct FSSpec fss;
    if ( strlen( pszFullPath ) < 256 )
        {
        unsigned char pszPath[256];
        strcpy( (char*)pszPath, pszFullPath );
        _c2pstr( (char*)pszPath ); // convert to pascal string
        FSMakeFSSpec( 0, 0, pszPath, &fss ); // convert pathname to FSSpec
        }
    else
        {
        ASSERT(FALSE);
        return;
        }

    FInfo fndrInfo;
    int iErr = FSpGetFInfo( &fss, &fndrInfo ); // get file info for file
    if ( iErr || ( fndrInfo.fdType == filetype && fndrInfo.fdCreator == creator ) )
        return;
    fndrInfo.fdType = filetype;
    fndrInfo.fdCreator = creator;
    FSpSetFInfo( &fss, &fndrInfo ); // set to specified filetype and creator
}

void SetFileTypeRTF( const char* pszFullPath ) // set file to type 'RTF ' and creator 'MSWD'
{
    SetFileType( pszFullPath, 'RTF ', 'MSWD' ); // set creator and type for Rich Text Format
}

void SetFileTypeSF( const char* pszFullPath ) // set file to type 'TEXT' and creator '????'
{
    SetFileType( pszFullPath, 'TEXT', '????' ); // set creator and type for Standard Format
}

void SetDocFileType( const char* pszFullPath ) // set file to type 'TEXT' and creator '????'
{
    // 1999-10-11 MRP: On the Macintosh, database files are now saved with
    // creator SHBX and type TEXT so that when Shoebox is already running,
    // you can open them by double-clicking their icons and aliases.
    SetFileType( pszFullPath, 'TEXT', 'SHBX' ); // set creator and type for database
}

void SetProjectFileType( const char* pszFullPath ) // set file to Shoebox identifier type and creator
{
    SetFileType( pszFullPath, 'SHBX', 'SHBX' ); // set creator and type for project file
}


void XferFileLabel( const char* pszFromPath, const char* pszToPath) 
// get a Mac file label attribute (color) from one file and set it in another   6/98 BDW
{
	// get the file label bits from the first file
    struct FSSpec fss;
    if ( strlen( pszFromPath ) < 256 )
        {
        unsigned char pszPath[256];
        strcpy( (char*)pszPath, pszFromPath );
        _c2pstr( (char*)pszPath ); // convert to pascal string
        FSMakeFSSpec( 0, 0, pszPath, &fss ); // convert pathname to FSSpec
        }
    else
        {
        ASSERT(FALSE);
        return;
        }

    FInfo fndrInfo;
    int iErr = FSpGetFInfo( &fss, &fndrInfo ); // get file info for file
	if (iErr)
		return;

	unsigned short usLabelFlags = fndrInfo.fdFlags & 0x000e;
		// ref Inside Macintosh vol IV  Finder Information pg 9-37
	/*Str8 sMessage;
	sMessage.Format("%4x",usLabelFlags);
	AfxMessageBox(sMessage);*/

	// set the label bits for the second file
    if ( strlen( pszToPath ) < 256 )
        {
        unsigned char pszPath[256];
        strcpy( (char*)pszPath, pszToPath );
        _c2pstr( (char*)pszPath ); // convert to pascal string
        FSMakeFSSpec( 0, 0, pszPath, &fss ); // convert pathname to FSSpec
        }
    else
        {
        ASSERT(FALSE);
        return;
        }

    iErr = FSpGetFInfo( &fss, &fndrInfo ); // get file info for file
	if (iErr)
		return;

	fndrInfo.fdFlags = (fndrInfo.fdFlags & ~0x000e) | usLabelFlags;
    FSpSetFInfo( &fss, &fndrInfo ); // set to specified filetype and creator
	/*sMessage.Format("%4x",fndrInfo.fdFlags);
	AfxMessageBox(sMessage);*/
}

#endif // _MAC


// Returns the drive+folder portion of the given path
Str8 sGetDirPath(const char* pszFile)
{
    char dir[_MAX_PATH];
#ifdef _MAC
    MacSplitPath( pszFile, dir, NULL, NULL );
    return Str8(dir);
#else
    char drive[_MAX_DRIVE];
	_splitpath_s(pszFile, drive, _MAX_DRIVE, dir, _MAX_DIR, NULL, 0, NULL, 0);
	std::string newPath = std::string(drive) + dir;
	Str8 sNewFile = newPath.c_str();
	return sNewFile;
#endif
}

Str8 sGetFileName(const char* pszFile, BOOL bIncludeExt)
{
#ifdef _MAC
    char fname[256];
    char ext[6];
    MacSplitPath( pszFile, NULL, fname, bIncludeExt ? ext : NULL );
#else
	char fname[_MAX_FNAME];
    char ext[_MAX_EXT];
	_splitpath_s(pszFile, NULL, 0, NULL, 0, fname, _MAX_FNAME, bIncludeExt ? ext : NULL, bIncludeExt ? _MAX_EXT : 0);
#endif
    Str8 sName(fname);
    if (bIncludeExt)
        sName += ext;
    return sName;
}

Str8 sGetFileExt(const char* pszFile)
{
#ifdef _MAC
    char ext[6];
    MacSplitPath( pszFile, NULL, NULL, ext );
#else
    char ext[_MAX_EXT];
	_splitpath_s(pszFile, NULL, 0, NULL, 0, NULL, 0, ext, _MAX_EXT);
#endif
    Str8 sName(ext);
    if ( sName.GetLength() > 0 )
        sName = sName.Mid( 1 ); // Cut off the dot!
    return sName;
}

Str8 sGetDrive(const char* pszFile)
{
#ifdef _MAC
    char dir[_MAX_PATH];
    MacSplitPath( pszFile, dir, NULL, NULL );
    return Str8(dir);
#else
    char drive[_MAX_DRIVE];
	_splitpath_s(pszFile, drive, _MAX_DRIVE, NULL, 0, NULL, 0, NULL, 0);
    return Str8(drive);
#endif
}

// sPath() functions return a full path built from the components provided
Str8 sPath(const char* pszDirPath, const char* pszName, const char* pszExt)
{
	// pszDirPath: directory path including the drive (see next func)
	// pszName: the name part, cannot be empty.
	// pszExt: the file extension (including the dot, if non-empty)
	ASSERT( pszName );
	ASSERT( *pszName );
	Str8 sNameExt = pszName;
	ASSERT( pszExt );
	ASSERT( *pszExt == '\0' || *pszExt == '.' );  // caller must give the dot
	sNameExt += pszExt;
	return sPath(pszDirPath, sNameExt);
}
Str8 sPath(const char* pszDirPath, const char* pszNameExt)
{
	// pszDirPath: directory path including the drive;
	//     no trailing backslash unless it's the root directory,
	//     empty or relative directory path is permitted,
	//     e.g. "A:", "B:\", "C:\SHW\SETTINGS", "", "SETTINGS".
	// pszNameExt: the name and ext part, cannot be empty.
	ASSERT( pszDirPath );
	Str8 s = pszDirPath;
	if ( !s.IsEmpty() )
		{
		const char chLast = s.GetChar(s.GetLength() - 1);
		if ( chLast != ':' && chLast != '\\' )
#ifdef _MAC
			s += ':';
#else
			s += '\\';
#endif
		}
	ASSERT( pszNameExt );
	ASSERT( *pszNameExt );  // caller is responsible that it not be empty
	s += pszNameExt;
	return s;
}


BOOL bXMLFileType( Str8 sPath ) // 1.5.9ge Return true if XML file type xml or xhtml
{
	if ( ( sPath.Find( ".xml" ) > 0 ) || ( sPath.Find( ".xht" ) > 0 ) ) // 1.5.9ge Check for xml and xhtml // 1.5.9gf Include xht
		return TRUE;
	else
		return FALSE;
}

void RemoveExtraSeparatorInFolderPath(Str8& sFolderPath)
{
    // 1999-03-11 MRP: If there is an extra separator at the end
    // of a folder's path, remove it (e.g. C:\Program Files\Shoebox\MDF\).
    int len = sFolderPath.GetLength();
    if ( len == 0 )
        return;

    char chUltimate = sFolderPath.GetChar(len - 1);
#ifdef _MAC
    if ( chUltimate == ':' )
        sFolderPath = sFolderPath.Left(len - 1);
#else
    if ( chUltimate == '\\' )
        {
        // In Windows, the root folder's backslash isn't extra (e.g. C:\).
        char chPenUltimate = '\0';
        if ( 2 <= len )
            chPenUltimate = sFolderPath.GetChar(len - 2);
        if ( chPenUltimate != ':' )
            sFolderPath = sFolderPath.Left(len - 1);
        }
#endif
}


// Returns TRUE if the file name/path strings are equivalent.
// The given strings may be two DirPaths, two simple file names, 
//  or two full paths, etc.
// Logic may be enhanced in the future to accommodate non-US operating 
//  system conventions.
BOOL bEquivFilePath(const char* psz1, const char* psz2)
	{
	if (_stricmp(psz1,psz2) == 0) // lower case comparison of strings
		return TRUE;
	else 
		return FALSE;
	// notes: 
	//	If non-US Win or Mac file names allow upper-128 characters like 
	//	accented vowels, use of _stricmp() will not properly equivalence
	//  them if their case is different.
	// Using lstrcmpi() is not acceptable because in Win it will
	//  do a "word sort" and ignore hyphens. Neither is it acceptable
	//  for file names on Mac (see Mac Porting Reference- lstrcmpi).
	// Str8::CompareNoCase() uses lstrcmpi()
	}

void Path1632DLL(Str8& sPathDLL)
{
#ifndef _MAC
    char pszDrive[_MAX_DRIVE];
    char pszDir[_MAX_DIR];
    char pszFileName[_MAX_FNAME];
    char pszExt[_MAX_EXT];
	_splitpath_s(sPathDLL, pszDrive, _MAX_DRIVE, pszDir, _MAX_DIR, pszFileName, _MAX_FNAME, pszExt, _MAX_EXT);
    int lenFileName = strlen(pszFileName);
    if ( 1 < lenFileName )
        {
        const char* psz = pszFileName + lenFileName - 2;
        if ( bEqual(psz, "16") || bEqual(psz, "32") )
            pszFileName[lenFileName - 2] = '\0';
        }
	std::string newPath = std::string(pszDrive) + pszDir + pszFileName + pszExt;
    sPathDLL = newPath.c_str();
#endif
}

Str8 sOnlyAlphanumeric( Str8 s ) // 1.4vzc Make a string only alphanumeric
	{
	int i = 0;
	while ( i < s.GetLength() )
		{
		int c = s.GetChar( i );
		if ( !( __isascii( c ) && isalnum( c ) ) )
			s.Delete( i );
		else
			i++;
		}
	return s;
	}

BOOL bAllASCII( Str8 s ) // 1.4vzc Check string for all ASCII chars
	{
	int i = 0;
	while ( i < s.GetLength() )
		{
		int c = s.GetChar( i );
		if ( !( __isascii( c ) ) )
			return FALSE;
		else
			i++;
		}
	return TRUE;
	}

BOOL bAllASCIIComplain( Str8 sPath ) // 1.4vze Check string for all ASCII chars, complain and return false if not all
	{
	if ( !bAllASCII( sPath ) ) // 1.4vze 
		{
		Str8 sMessage = _("Toolbox cannot handle path with non-Roman character:"); // 1.5.0fg 
		sMessage = sMessage + " " + sPath; // 1.5.0fg 
		AfxMessageBox( sMessage ); // 1.4vze 
		return FALSE; // 1.4vze 
		}
	else
		return TRUE; // 1.4vze 
	}

Str8 sUniqueFileName(const char* pszDirPath, const char* pszName,
        const char* pszExt, BOOL bIncludeExt, BOOL bLongFN, BOOL bAllChars)
{
    // Convert the name to a valid file name by removing invalid characters
    // and truncating it to the proper length.
    const int maxFNBuf = 28; // max len for buffer allocation
    int maxlenFileName = 8;
    if (bLongFN)            // allow longer file name 3/7/98
        maxlenFileName = 28;    // 32 (Mac limit) less 4 for ".abc" extension
    ASSERT (maxFNBuf >= maxlenFileName);
    char pszBaseName[maxFNBuf+1];
    int lenRemaining = maxlenFileName;
    char* pszB = pszBaseName;
    const char* pszN = pszName;
    ASSERT( pszN );
    for ( ; *pszN && lenRemaining != 0; pszN += 1 )
            // idea: isalnum() test could be done in separate function  
            //  (called when pszName is not already a valid file name)
            //  instead of using bAllChars param here
        if ( (bAllChars && *pszN) || ( __isascii(*pszN) && isalnum(*pszN) ) )  // bAllChars mode added 3/7/98 // 1.4vzc Fix U bug of keeping upper ANSI chars in file name
            {                 
            *pszB++ = *pszN;
            lenRemaining -= 1;
            }
    *pszB = '\0';
    
    // Make the file name unique in this directory by appending numbers and
    // shortening the base name, if needed
    int lenBaseName = strlen(pszBaseName);
    ASSERT( lenBaseName <= maxlenFileName );
    unsigned short i = 1;
    char pszFileName[maxFNBuf+1];
    strcpy_s(pszFileName, sizeof(pszFileName), pszBaseName);
    Str8 s = sPath(pszDirPath, pszFileName, pszExt);
    while ( bFileExists(s) )  // The path exists
        {
        char pszNumber[6];
		sprintf_s(pszNumber, sizeof(pszNumber), "%u", ++i);
        int lenNumber = strlen(pszNumber);
        if ( maxlenFileName < lenBaseName + lenNumber )
            {
            lenBaseName = maxlenFileName - lenNumber;
            ASSERT( 0 <= lenBaseName );
            ASSERT( lenBaseName + lenNumber <= maxlenFileName );
            pszBaseName[lenBaseName] = '\0';  // Shorten the base name
            }
        // Append the number to the base name and make a path from it
		strcpy_s(pszFileName, sizeof(pszFileName), pszBaseName);
		strcat_s(pszFileName, sizeof(pszFileName), pszNumber);
        s = sPath(pszDirPath, pszFileName, pszExt);
        }
    Str8 sFileName = pszFileName;
    if ( bIncludeExt )
        sFileName += pszExt;
    
    return sFileName;
}
#endif  // _WINDOWS
// For each length of lead byte, the rest of the bytes be trail bytes
// If an out of range byte is found, then the sequence up to the out of range byte is invalid
// If a trail byte is found without a lead byte, it is invalid
// 
// Functions to convert to and from UFT8 and UTF32 // 1.4hk Move UTF8 to/from UFT32 converters to tools
LONG lUTF8ToUTF32( const char* psz, int& iNumBytes )
	{
	iNumBytes = 0;
	LONG lUTF32 = 0;
	unsigned char c1 = *psz;
	if ( c1 >= 0xF0 ) // If 4 byte UTF-8
		{
		unsigned char c2 = *(psz+1);
		if ( c2 > 0 ) // Protect against invalid data at end of string
			{
			unsigned char c3 = *(psz+2);
			if ( c3 > 0 )
				{
				unsigned char c4 = *(psz+3);
				if ( c4 > 0 )
					{
					lUTF32 = (c1 - 240) * 262144 + (c2 - 128) * 4096 + (c3 - 128) * 64 + c4 - 128;
					iNumBytes = 4;
					}
				}
			}
		}
	else if ( c1 >= 0xE0 ) // If 3 byte UTF-8
		{
		unsigned char c2 = *(psz+1);
		if ( c2 > 0 )
			{
			unsigned char c3 = *(psz+2);
			if ( c3 > 0 )
				{
				lUTF32 = (c1 - 224) * 4096 + (c2 - 128) * 64 + c3 - 128;
				iNumBytes = 3;
				}
			}
		}
	else if ( c1 >= 0xC0 ) // If 2 byte UTF-8
		{
		unsigned char c2 = *(psz+1);
		if ( c2 > 0 )
			{
			lUTF32 = (c1 - 192) * 64 + c2 - 128;
			iNumBytes = 2;
			}
		}
	if ( iNumBytes == 0 ) // If none of the above built a valid sequence, do a single byte
		{
		lUTF32 = c1;
		iNumBytes = 1;
		}
	return lUTF32;
	}

// Unicode convert from UTF-32 to UTF-8, algorithm from NRTC 2001 book pg 202
Str8 sUTF32ToUTF8( UINT lUTF32 )
	{
	Str8 sUTF8;
	if ( lUTF32 <= 0x007F )
		sUTF8 += (UCHAR)lUTF32;
	else if ( lUTF32 <= 0x07FF )
		{
		sUTF8 += (UCHAR)( lUTF32 / 64 + 192 );
		sUTF8 += (UCHAR)( lUTF32 % 64 + 128 );
		}
	else if ( lUTF32 <= 0xD7FF || ( 0xE000 <= lUTF32 && lUTF32 <= 0xFFFF ) )
		{
		sUTF8 += (UCHAR)( lUTF32 / 4096 + 224 );
		sUTF8 += (UCHAR)( ( lUTF32 % 4096 ) / 64 + 128 );
		sUTF8 += (UCHAR)( lUTF32 % 64 + 128 );
		}
	else
		{
		sUTF8 += (UCHAR)( lUTF32 / 262144 + 240 );
		sUTF8 += (UCHAR)( ( lUTF32 % 262144 ) / 4096 + 128 );
		sUTF8 += (UCHAR)( ( lUTF32 % 4096 ) / 64 + 128 );
		sUTF8 += (UCHAR)( lUTF32 % 64 + 128 );
		}
	return sUTF8;
	}
// Numbers in hex
// 262144 = 0x40000
// 4096 = 0x1000
// 240 = 0xF0
// 224 = 0xE0
// 192 = 0xC0
// 128 = 0x80
// 64 = 0x40

//1997-7-1 Josh Kelley
//Functions to handle local settings for date, time, and measurements.
//Macs handle locale completely differently (if they support it at all -
//I really don't know), so these functions do very little on a Mac.

BOOL bIsLocaleMetric()
{
#ifdef _MAC
    return FALSE;
#else
//    LPTSTR swzBuffer[2]; // 1.4qts
    LCID lcid = GetUserDefaultLCID();
	CString swText; // 1.4qts
    LPTSTR pswzBuffer = swText.GetBuffer(5); // 1.4qts // 1.4qzfq GetBuffer OK because CString, not Str8
    GetLocaleInfo (lcid, LOCALE_IMEASURE, pswzBuffer, 2); // 1.4qts Upgrade GetLocaleInfo for Unicode build
	BOOL bMetric = ( pswzBuffer[0] == '0' ); // 1.4qts
    swText.ReleaseBuffer(); // 1.4qts
	return bMetric; // 1.4qts
#endif
}

BOOL bIsLocaleEnglish()
{
    return !bIsLocaleMetric();
}

void LocaleFormatTime (CTime& timToFormat, Str8& sRetVal)
{
#ifdef _MAC
    sRetVal = timToFormat.Format("%H:%M");
#else
    // Using the COleDateTime class here would also be an option, but that
    // seems like overkill.
    LCID lcid = GetUserDefaultLCID();
    SYSTEMTIME st;
    int nStrLen;
    st.wHour = timToFormat.GetHour();
    st.wMinute = timToFormat.GetMinute();
    st.wSecond = timToFormat.GetSecond();
    st.wMilliseconds = 0;
    nStrLen = GetTimeFormat (lcid, 0, &st, NULL, NULL, 0);
	CString swText; // 1.4qtq Upgrade GetTimeFormat for Unicode build
    LPTSTR pswzBuffer = swText.GetBuffer(nStrLen); // 1.4qtq // 1.4qzfq GetBuffer OK because CString, not Str8
    GetTimeFormat (lcid, 0, &st, NULL, pswzBuffer, nStrLen); // 1.4qtq Upgrade GetTimeFormat for Unicode build
    swText.ReleaseBuffer(); // 1.4qtq
	sRetVal = sUTF8( swText ); // 1.4qtq
#endif
}

void LocaleFormatDate (CTime& timToFormat, Str8& sRetVal, 
                       BOOL bLongDate /*= FALSE*/)
{
#ifdef _MAC
    if (bLongDate)
        sRetVal = timToFormat.Format("%A, %B %d, %Y");
    else
        sRetVal = timToFormat.Format("%m/%d/%y");
#else
    // Using the COleDateTime class here would also be an option, but that
    // seems like overkill.
    LCID lcid = GetUserDefaultLCID();
    SYSTEMTIME st;
    int nStrLen;
    DWORD dwFlags;
    if (bLongDate)
        dwFlags = DATE_LONGDATE;
    else
        dwFlags = DATE_SHORTDATE;
    st.wYear = timToFormat.GetYear();
    st.wMonth = timToFormat.GetMonth();
    st.wDay = timToFormat.GetDay();
    nStrLen = GetDateFormat (lcid, dwFlags, &st, NULL, NULL, 0);
	CString swText; // 1.4qtr Upgrade GetTimeFormat for Unicode build
    LPTSTR pswzBuffer = swText.GetBuffer(nStrLen); // 1.4qtr // 1.4qzfq GetBuffer OK because CString, not Str8
    GetDateFormat(lcid, dwFlags, &st, NULL, pswzBuffer, nStrLen); // 1.4qtr Upgrade GetDateFormat for Unicode build
    swText.ReleaseBuffer(); // 1.4qtr
	sRetVal = sUTF8( swText ); // 1.4qtr
#endif
}

/*
 * bWinExecWait(char* szCmdLine, UINT);
 *
 *  This function executes szCmdLine and if bWait is TRUE, it doesn't return until szCmdLine is done. 
 * Used by Run Batch File command, and by Tools, Show using external program.
 * Written by Doug Eberman Aug 97
 */
BOOL bWinExecWait(const char* szCmdLine, UINT nCmdShow, BOOL bWait) // 1.5.0aj 
{
#ifdef _MAC
    ASSERT(FALSE);
    // TODO - Write a Mac solution
    return FALSE;
#else
  STARTUPINFO si;
  PROCESS_INFORMATION pi;

  si.cb = sizeof(si);
  // DebugMessage ("%i", (int) sizeof(si));
  si.lpReserved = NULL;
  si.lpDesktop = NULL;
  si.lpTitle = NULL;
  si.dwFlags = STARTF_USESHOWWINDOW;
  si.wShowWindow = (WORD) nCmdShow;
  si.cbReserved2 = 0;
  si.lpReserved2 = 0;
  CString swCmdLine = swUTF16( szCmdLine ); // 1.4qpu
  LPTSTR pswzCmdLine = swCmdLine.GetBuffer( 1000 ); // 1.4qzfq GetBuffer OK because CString, not Str8 // 1.4qpu Make non-const string pointer because required by CreateProcess
  if (!CreateProcess(NULL, pswzCmdLine, NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE, NULL, NULL, &si, &pi)) // 1.4qpu Fix WinExecWait for Unicode
  {
//    AfxMessageBox ("WinExecWait failed.", MB_OK); // 1.4qpr // 1.5.0aj Eliminate meaningless message if WinExecWait fails
    return FALSE;
  } 

  if( bWait )
	  WaitForSingleObject (pi.hProcess, INFINITE);
  return TRUE;

#endif
}

#define TRYINTERVAL 100 // Save time by not getting time except at intervals
static int s_iTryNum = 0; // Maximum number of times to try on failures
static int s_iMaxTime = 5; // 1.3bg Limit total time parsing to 5 seconds
static int s_bMessage = TRUE; // 1.4qzjm Add option to prevent message for interlinear check
static CTime s_tStartTime; // 1.2rg Limit parse time to avoid apparent hang
static BOOL s_bTimeLimitExceededMessageGiven = FALSE; // 1.2rg Note if parse time limit was exceeded

void InitTimeLimit( int iTimeLimit, BOOL bMessage ) // 1.3bh Make separate timer init function // 1.4qzjm Add argument to prevent message for interlinear check
	{
	s_iMaxTime = iTimeLimit;
	s_bMessage = bMessage; // 1.4qzjm
	s_iTryNum = 0; // Initialize number of tries
	s_bTimeLimitExceededMessageGiven = FALSE; // 1.2rg Note if parse time limit was exceeded
	s_tStartTime = CTime::GetCurrentTime(); // 1.2rg Note starting time of parse
	}

BOOL bTimeLimitExceeded() // Return true if time limit has been exceeded // 1.3bh Make time limit test a separate function
	{
	if ( s_bTimeLimitExceededMessageGiven ) // If time limit already exceeded, return true
		return TRUE;
	if ( ( ++s_iTryNum % TRYINTERVAL ) == 0 ) // At regular intervals, check elapsed time
		{
		CTimeSpan tsElapsedParseTime = CTime::GetCurrentTime() - s_tStartTime; // Get elapsed time
		if ( tsElapsedParseTime.GetSeconds() > s_iMaxTime ) // If elapsed parse time > 5 seconds, return TRUE
			{
			if ( !s_bTimeLimitExceededMessageGiven )
				{
				if ( s_bMessage ) // 1.4qzjm If no timeout message desired, don't give message
					AfxMessageBox( _("Parse time exceeded limit. Not all possibilities could be tried.") ); // 1.2rg Give user warning that time limit was exceeded
				s_bTimeLimitExceededMessageGiven = TRUE; //
				}
			return TRUE;
			}
		}
	return FALSE;
	}

void WaitSeconds( int iSeconds ) // 1.4kzc Wait for a number of seconds
	{
	CTime tStartTime = CTime::GetCurrentTime(); // Note starting time
	CTimeSpan tsElapsedParseTime = CTime::GetCurrentTime() - tStartTime; // Get elapsed time
	while ( tsElapsedParseTime.GetSeconds() < iSeconds ) // If elapsed parse time < required number of seconds, wait
		tsElapsedParseTime = CTime::GetCurrentTime() - tStartTime; // Update elapsed time
	}

void RemoveExtraSpaces( Str8& s ) // Remove all multiple spaces and trailing spaces to prepare for comparison // 1.3br move to tools
{
	s.TrimRight();
	int iSpace = 0;
	while ( ( iSpace = s.Find( "  " ) ) >= 0 ) // Eat out all extra spaces
		{
		Str8 sLeft = s.Left( iSpace );
		Str8 sRight = s.Mid( iSpace + 1 );
		s = sLeft + sRight;
		}
}

void TrimBothEnds( Str8& s ) // 1.4hdc Trim both ends of string
	{
	s.TrimLeft();
	s.TrimRight();
	}

void RemoveBackslashAtFront( Str8& s ) // 1.4hdj Trim and remove any backslash at front
	{
	TrimBothEnds( s ); // Trim first
    if ( s.GetLength() != 0 && s.GetChar( 0 ) == '\\' )
        s = s.Mid(1);  // If the first char is a backslash, remove it
	}

int FindAfter( Str8 s, Str8 sFind, int iStart ) // 1.4hen Make Find with start pos, not in Str8
	{
	int iFullFind = -1; // Default to no find
	Str8 sTail = s.Mid( iStart ); // Make tail string to find in
	int iFind = sTail.Find( sFind ); // Do find in tail
	if ( iFind >= 0 ) // If found in tail, calculate equivalent place in full string
		iFullFind = iStart + iFind;
	return iFullFind;
	}

Str8 sGetNextRef( Str8 s, int& iStart ) // 1.4qng Get next reference from a string of refs
	{
	// While not ref, move forward
	Str8 sRef = ""; // 1.4qng Temp so it will compile
	return sRef; // 1.4qng Temp so it will compile
	}

BOOL bFindRef( Str8 s, Str8 sRef ) // 1.4qng Find a reference in a string of references
	{
	int i = s.Find( sRef ); // See if ref found
	if ( i != -1 ) // If found, make sure whole ref is covered
		{
		int iLen = sRef.GetLength(); // Get length of ref found // MAT.1:1 End, or middle, or false (1:11)
		int iEnd = i + iLen; // Calculate end of ref match
		if ( iEnd == s.GetLength() ) // If matched all the way to the end, it is a good match
			return TRUE;
		if ( isdigit( s[iEnd] ) ) // If next after match is digit, it is only a partial match
			return FALSE;
		else
			return TRUE; // Good match
		}
	else
		return FALSE; // If not found, no match
	}

void WriteProtect( const char* pszPath ) // 1.4qzhg Write protect a file
	{
	CString swPath = swUTF16( Str8( pszPath ) ); // Change to UTF16 for Unicode build
	_tchmod( swPath, _S_IREAD ); // Set write protect
	}

void UnWriteProtect( const char* pszPath ) // 1.4qzhg Clear write protect of a file
	{
	CString swPath = swUTF16( Str8( pszPath ) ); // Change to UTF16 for Unicode build
	_tchmod( swPath, _S_IWRITE ); // Clear write protect to set writable
	}

Str8 sGetClipBoardData( CWnd* w, BOOL bUnicode ) // 1.4rbc Encapsulate get clipboard data
	{
	Str8 sClip;
    if ( !w->OpenClipboard() ) // If nothing available on clipboard, return empty string
        return sClip;
    HGLOBAL hData = GetClipboardData( Shw_papp()->TlbxClipBdFormat() ); // Toolbox internal format is top priority
    if( hData )
        sClip = (LPCSTR)GlobalLock( hData ); // get a pointer to contents
    else if ( bUnicode && ( hData = GetClipboardData( CF_UNICODETEXT ) ) ) // If pasting into a Unicode field, convert to Unicode
		{
		LPCWSTR pwStr = (LPCWSTR)GlobalLock( hData );
		sClip = sUTF8(pwStr); // manually convert this to UTF-8 // 1.4qzhm
		}
    else if( ( hData = GetClipboardData( CF_TEXT ) ) ) // If pasting into a non-Unicode field, paste as text
        sClip = (LPCSTR)GlobalLock( hData ); // get a pointer to contents
	if ( hData )
	    GlobalUnlock( hData );  // release the memory
    CloseClipboard();
    sClip.Replace( "\t", " " ); // Don't let paste insert hard tabs
	return sClip;
	}

char caBigBufferA[BIGBUFMAX+1]; // 1.4kg Fix bug of losing part of large word formula
char* pszBigBuffer = (char*)caBigBufferA; // 1.4kg
wchar_t caBigBufferW[BIGBUFMAX+1]; // 1.4kg Fix bug of losing part of large word formula
LPWSTR pswzBigBuffer = (LPWSTR)caBigBufferW; // 1.4qpt Make unsigned short pointer to buffer

void AfxFormatString1( Str8& s, LPCSTR pszFormat, LPCSTR pszArg1 ) // 1.3ag Make string form of AfxFormatStrng
	{
//	s = pszFormat; // 1.4rag Fix U bug of not showing marker in some MDF error messages // 1.4raj
//	s.Replace( "%1", pszArg1 ); // 1.4rag // 1.4raj
//	char buffer[1000]; // 1.4rag
	sprintf_s( pszBigBuffer, BIGBUFMAX, pszFormat, pszArg1 ); // Use sprintf to format string // 1.4rag // 1.4raj
	s = pszBigBuffer; // Copy formatted result to return string // 1.4rag // 1.4raj
	}

void AfxFormatString2( Str8& s, LPCSTR pszFormat, LPCSTR pszArg1, LPCSTR pszArg2 ) // 1.3ag Make string form of AfxFormatStrng
	{
//	s = pszFormat; // 1.4rag Fix U bug of not showing marker in some MDF error messages // 1.4raj Fix bug of showing %s in messages
//	s.Replace( "%1", pszArg1 ); // 1.4rag // 1.4raj
//	s.Replace( "%2", pszArg2 ); // 1.4rag // 1.4raj
//	char buffer[1000]; // 1.4rag
	sprintf_s( pszBigBuffer, BIGBUFMAX, pszFormat, pszArg1, pszArg2 ); // Use sprintf to format string // 1.4rag // 1.4raj
	s = pszBigBuffer; // Copy formatted result to return string // 1.4rag // 1.4raj
	}

#ifdef _UNICODE
int AfxMessageBox( const char* psz, UINT nType ) // 1.4qpq Make message box for Unicode
	{
	USES_CONVERSION; // 1.5.0gd 
	return AfxMessageBox( A2CW( psz ), nType ); // 1.5.0gd Make message box use code page, for translatability
	
//	Str8 s = psz; // 1.5.0gd 
//	return AfxMessageBox( swUTF16( s ), nType ); // 1.4qzhc Clean up message box code // 1.5.0gd 
	}
#endif

Str8 sUTF8( CString swUTF16 ) // 1.4qpt Make narrow string UTF8 from UTF16
	{
	Str8 s;
#ifdef _UNICODE // 1.4qpt
	LPCWSTR pwszUTF16 = swUTF16;
	int iNumChars = WideCharToMultiByte( CP_UTF8, 0, pwszUTF16, -1, pszBigBuffer, BIGBUFMAX, NULL, NULL );
	s = pszBigBuffer;
#else
	s = (const char*)swUTF16;
#endif
	return s;
	}

CString swUTF16( Str8 sUTF8 ) // 1.4qpt Make wide string UTF16 from UTF8
	{
	CString sw;
	const char* pszUTF8 = sUTF8;
#ifdef _UNICODE // 1.4qpt
	int iNumChars = MultiByteToWideChar( CP_UTF8, 0, pszUTF8, -1, pswzBigBuffer, BIGBUFMAX );
	sw = pswzBigBuffer;
#else
	sw = pszUTF8;
#endif
	return sw;
	}

CString swUTF16( const char* psz ) // 1.4qxj Make string conversion from char* directly to UTF16
	{
	Str8 s;
	if ( psz ) // 1.4qzhr Fix U crash on RTF export
		s = psz; // Load string into Str8
	return swUTF16( s ); // Convert Str8 to UTF16 and return it
	}

void SetRegPossibleCrash() // 1.5.8wa Set registry to watch for possible crash
	{
	CString swCrash = swUTF16( "Yes" );
	Str8 sRegPath = "Applications\\Toolbox.exe\\FileDialogCrash";
	CString swRegPath = swUTF16( sRegPath ); // 1.4qto
	::RegSetValue( HKEY_CLASSES_ROOT, swRegPath, REG_SZ, swCrash, swCrash.GetLength() );
	}

void ClearRegPossibleCrash() // 1.5.8wa Clear registry to watch for possible crash
	{
	CString swCrash = swUTF16( "No" );
	Str8 sRegPath = "Applications\\Toolbox.exe\\FileDialogCrash";
	CString swRegPath = swUTF16( sRegPath ); // 1.4qto
	::RegSetValue( HKEY_CLASSES_ROOT, swRegPath, REG_SZ, swCrash, swCrash.GetLength() );
	}

int iDlgDoModalSetReg( CFileDialog* pdlg ) // 1.5.8wa Do dialog DoModal with reg set and clear for crash detect
	{
	SetRegPossibleCrash(); // 1.5.8wa 
	int iRet = pdlg->DoModal(); // 1.5.8wa 
	ClearRegPossibleCrash(); // 1.5.8wa 
	return iRet; // 1.5.8wa 
	}

BOOL bRegPossibleCrash() // 1.5.8wa Read registry to see if possible crash
	{
	CString swCrash = swUTF16( "No" );
	Str8 sRegPath = "Applications\\Toolbox.exe\\FileDialogCrash\\";
	CString swRegPath = swUTF16( sRegPath ); // 1.4qto
	LONG lLen; // 1.5.8wa 
	::RegQueryValue( HKEY_CLASSES_ROOT, swRegPath, pswzBigBuffer, &lLen ); // 1.5.8wa 
	Str8 sRegValue = sUTF8( pswzBigBuffer );
	if ( sRegValue == "Yes" )
		return TRUE;
	else
		return FALSE;
	}

BOOL bReadFileIntoString( Str8 sPath, Str8& sIn ) // 1.5.9pd Read file into string
	{
	Str8 sMode = "r"; // Set up mode string
	FILE* pf = nullptr;
	errno_t err = _wfopen_s(&pf, swUTF16( sPath ), swUTF16( sMode ));
	if (err != 0 || pf == nullptr) {
		// handle error
		return FALSE;
	}
	Str8 sLine; // Str8 form of line for searching and processing
	const char* pszSucc = fgets( pszBigBuffer, BIGBUFMAX, pf ); // 1.5.9pd Read a line
	sLine = pszBigBuffer; // 1.5.9pd Get first line
	Str8 sBOM = "﻿"; // 1.5.9pd UTF8 Byte Order Mark
	if ( sLine.Find ( sBOM ) == 0 ) // 1.5.9pd If Byte Order Mark, delete it 
		sLine = sLine.Mid( 3 ); // 1.5.9pd
	sIn += sLine; // 1.5.9pd Load file into CC buffer
	while ( pszSucc = fgets( pszBigBuffer, BIGBUFMAX, pf ) ) // 1.5.9pd While another line, put it in buffer
		{
		sLine = pszBigBuffer;
		sIn += sLine; // 1.5.9pd Append line to buffer
		}
	fclose( pf ); // 1.5.9pd Close file
	return TRUE; // 1.5.9pd 
	}

BOOL bWriteFileFromString( Str8 sPath, Str8 sOut ) // 1.5.9pd Write file from string
	{
	Str8 sMode = "w"; // Set up mode string to write // 1.5.9pd 
	FILE* pf = nullptr;
	_wfopen_s(&pf, swUTF16( sPath ), swUTF16( sMode ));
	fputs( sOut, pf ); // 1.5.9pd Write modified file
	fclose( pf ); // 1.5.9pd Close file
	return TRUE; // 1.5.9pd 
	}

FILE* pfFileOpenWrite( const char* pszPath ) // 1.6.4ad Open FILE for write
	{
	UnWriteProtect( pszPath ); // 1.2af Unwrite protect project file so it can be written // 1.4qzhg
	Str8 sPath = pszPath; // 1.6.1cb 
	CString swPath = swUTF16( sPath ); // 1.6.1cb 
	Str8 sMode = "w"; // Set up mode string  // 1.6.1cb 
	CString swMode = swUTF16( sMode ); // Convert mode string to CString  // 1.6.1cb 
	FILE* pf = nullptr;
	_wfopen_s(&pf, swPath, swMode);
	return pf;
	}

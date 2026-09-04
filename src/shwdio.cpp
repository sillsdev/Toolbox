// shwdio.cpp : contains read and write functions for the document.
// Note: does not include MFC On... member funcs -- they're in shwdoc.cpp
// Rod Early 1995-04-19

#include "stdafx.h"
#include "toolbox.h"
#include "shw.h"
#include "shwdoc.h"
#include "shwview.h"
#include "project.h"  // File Open (Import) properties

#include "dlgimpt.h"
#include "tools.h"

#include "resource.h"

#if UseCct
#include "Change_ostream.h" // For change table stream classes
#include "Change_istream.h"
#endif
#include "sfstream.h"  // class SF_istream, SF_ostream
#include "progress.h"

#include <ctype.h>
#include <fstream>  // classes ifstream, ofstream
#include <stdlib.h> // Needed for _MAX_PATH
#include <io.h> // _tell
#include <errno.h>
#include <fcntl.h>

//08-11-1997
#include "nlstream.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

extern Str8 g_sVersion; // 1.5.9ee Allow access to version

#define TEXTSTRING_MAX  32765

BOOL bApplyCCT( Str8 sCCT, Str8 sPath ) // 1.5.9mt Apply CC table to file, write file to same name and place
	{
	if ( bFileReadOnly( sPath ) ) // If file is read only, return failure
		return FALSE;
	CString swPath = swUTF16( sPath );
	Str8 sMode = "r"; // Set up mode string
	CString swMode = swUTF16( sMode ); // Convert mode string to CString
	FILE* pf = nullptr;
	errno_t err = _wfopen_s(&pf, swPath, swMode);
	if (err != 0 || pf == nullptr) {	// If file open failed, return failure
		return FALSE;
	}
	int fh = _fileno( pf ); // 1.5.9mh Get file handle
	int iFileLen = _filelength( fh ); // 1.5.9mh 
	Str8 sCCInBuff; // 1.5.9mh CC input buffer
	Str8 sCCOutBuff; // 1.5.9mh CC output buffer
	int iCCInBuffLen = iFileLen + 1000; // 1.5.9mh 
	int iCCOutBuffLen = 5 * iCCInBuffLen + 500000; // 1.5.9mh  // 1.5.9ne Enlarge CC output buffer
	char* pszCCInBuff = sCCInBuff.GetBuffer( iCCInBuffLen ); // 1.5.9mh Allow room for read
	char* pszCCOutBuff = sCCOutBuff.GetBuffer( iCCOutBuffLen ); // 1.5.9mh Allow plenty of room for CC expansion
	// Load file into CC buffer
	Str8 sLine; // 1.5.9mh 
	const char* pszSucc; // 1.5.9mt 
	while ( pszSucc = fgets( pszBigBuffer, BIGBUFMAX, pf ) ) // 1.5.9mh While another line, put it in buffer
		{
		sLine = pszBigBuffer;
		sCCInBuff += sLine; // 1.5.9mh Append line to buffer
		}
	fclose( pf ); // 1.5.9mh Close file
	// Do CC on file
	ChangeTable cct; // 1.5.9mh Change table
	BOOL bCCSucc = TRUE;
	bCCSucc = cct.bLoadFromFile( sCCT ); // 1.5.9mh Load change table
	int iCCOutLen = iCCOutBuffLen; // 1.5.9mh 
	if ( bCCSucc ) // 1.5.9mh If succesful load, make changes
		bCCSucc = cct.bMakeChanges( pszCCInBuff, sCCInBuff.GetLength() + 1, 
			pszCCOutBuff, &iCCOutLen ); // 1.5.9mh Make changes
	if ( !bCCSucc ) // 1.5.9mh If not successful change, return fail
		return FALSE; // 1.5.9mh 
	sCCOutBuff.ReleaseBuffer( iCCOutLen ); // 1.5.9mh Reset sring
	// Write out CC'd file
	sMode = "w"; // Set up mode string to write // 1.5.9mh 
	swMode = swUTF16( sMode ); // Convert mode string to CString // 1.5.9mh 
	pf = nullptr;
	_wfopen_s(&pf, swPath, swMode); // 1.5.9mh Open modified file
	fputs( sCCOutBuff, pf ); // 1.5.9mh Write modified file
	fclose( pf ); // 1.5.9mh Close file
	return TRUE; // 1.5.9mt Return success
	}

int iXMLFind( Str8 s, Str8 &sTag ) // 1.5.9ea Find first XML tag in string, return -1 if no tag found
	{
	int iOpen = s.Find( "<" ); // 1.5.9ea Find wedge
	if ( iOpen < 0 ) // 1.5.9ea If no open wedge found, fail
		return -1;
	int iClose = s.Find( ">" ); // 1.5.9ea Find close wedge
	if ( iClose < 0 ) // 1.5.9ea If no close found, return fail
		return -1;
	sTag = s.Mid( iOpen, iClose - iOpen + 1 ); // 1.5.9ea Collect tag
	return iOpen; // 1.5.9ea Return starting position of tag
	}

BOOL CShwDoc::bRead(const char* pszPath, Str8& sMessage)
{
#define NEWFILEREAD // 1.5.9hb Turn on new file read
	Str8 sPath = pszPath; // 1.5.9da Put path into Str8 to check it
	CString swPath = swUTF16( sPath ); // 1.4qug
#ifdef NEWFILEREAD // 1.5.9hb Code for new file read
	Str8 sMode = "r"; // Set up mode string
	CString swMode = swUTF16( sMode ); // Convert mode string to CString
	FILE* pf = nullptr;
	errno_t err = _wfopen_s(&pf, swPath, swMode);
	if (err != 0 || pf == nullptr) 
	{	// If file open failed, return failure
		sMessage = sMessage + _("Cannot open file:") + " " + sPath; // 1.5.9da 
		return FALSE;
	}
	if ( bFileReadOnly(pszPath) ) // 1.4ysa Make sure read only gets written to project file
		SetReadOnly( TRUE ); // 1.4ysa 
	BOOL bReadOnly = m_bReadOnly || bFileReadOnly(pszPath);
	Str8 sHeadAndTailInfo; // 1.5.9gc Head and tail info is all before first rec mkr
	Str8 sLine; // Str8 form of line for searching and processing
#endif // 1.5.9hb NEWFILEREAD
	if ( bXMLFileType( sPath ) ) // 1.5.9da If xml file, read as xml // 1.5.9ge Include xhtml file type as xml
		{
#ifndef NEWFILEREAD // 1.5.9hb Code for before new file read
		Str8 sMode = "r"; // Set up mode string
		CString swMode = swUTF16( sMode ); // Convert mode string to CString
		FILE* pf = _wfopen( swPath, swMode ); // 1.5.9da Open file
		if ( !pf ) // If file open failed, return failure with message
			{
			sMessage = sMessage + _("Cannot open file:") + " " + sPath; // 1.5.9da 
			return FALSE;
			}
		if ( bFileReadOnly(pszPath) ) // 1.4ysa Make sure read only gets written to project file
			SetReadOnly( TRUE ); // 1.4ysa 
		BOOL bReadOnly = m_bReadOnly || bFileReadOnly(pszPath);
		Str8 sLine; // Str8 form of line for searching and processing
		Str8 sHeadAndTailInfo; // 1.5.9gc Head and tail info is all before first rec mkr
#endif // 1.5.9hb ifndef NEWFILEREAD
		sHeadAndTailInfo = "\\xmlhead\n"; // 1.5.9gc Head and tail info is all xml before and after database
		const char* sSucc = NULL; // 1.5.9da Successfully read a line
		while ( TRUE ) // 1.5.9da Read lines from file until start of database is found
			{
			sSucc = fgets( pszBigBuffer, BIGBUFMAX, pf ); // 1.5.9da Read a line
			if ( sSucc == NULL ) // If end of file, stop reading
				break;
			sLine = pszBigBuffer;
			if ( sLine.Find( "<?xml version" ) >= 0 ) // 1.5.9eh If main declaration line, skip it // 1.5.9gd Fix bug of doubling xml header if bom before it
				continue;
			if ( sLine.Find( "<database " ) >= 0 ) // 1.5.9da If database start found, break from this read loop
				{
				if ( sLine.Find( "toolboxwrite=\"save\"" ) < 0 ) // 1.5.9ej If opening exported xml file, give warning
					{ // 1.5.9ej Give warning
					int iAns = AfxMessageBox( "Warning: Opening an exported XML file loses interlinear layout", MB_OKCANCEL ); // 1.5.9pe XML read exported file only warn about interlinear layout
					if ( iAns == IDCANCEL ) // 1.5.9ej If user cancels, don't load the file
						return FALSE;
					}
				break;
				}
			sHeadAndTailInfo += sLine; // 1.5.9da Put all lines before database start into head info
			}
		if ( !sSucc ) // 1.5.9ea If no database header found, this is not a Toolbox xml file
			{
			sMessage = sMessage + _("Not a Toolbox XML file:") + " " + sPath; // 1.5.9da 
			return FALSE;
			}

		Str8 sDBTypeTag = "toolboxdatabasetype="; // 1.5.9ea Store database type tag for use and measuring
		int iFind = sLine.Find( sDBTypeTag ); // 1.5.9ea Find database type
		if ( iFind < 0 ) // 1.5.9ea If not found, fail
			return FALSE;
		int iDBTypeStart = iFind + sDBTypeTag.GetLength() + 1; // 1.5.9ea Remember start of database type
		iFind = sLine.Find ( '\"', iDBTypeStart ); // 1.5.9ea Find end of database type
		Str8 sDBType = sLine.Mid( iDBTypeStart, iFind - iDBTypeStart ); // 1.5.9ea Collect database type

		Str8 sWrapTag = "wrap="; // 1.5.9ea Store wrap tag for use and measuring
		int iWrap = 400; // 1.5.9ea Set wrap to default
		iFind = sLine.Find( sWrapTag ); // 1.5.9ea Find wrap margin
		if ( iFind > 0 ) // 1.5.9ea If wrap given, store it
			{
			int iWrapStart = iFind + sWrapTag.GetLength() + 1; // 1.5.9ea Remember start of wrap
			iFind = sLine.Find ( '\"', iWrapStart ); // 1.5.9ea Find end of wrap
			Str8 sWrap = sLine.Mid( iWrapStart, iFind - iWrapStart ); // 1.5.9ea Collect wrap
			iWrap = atoi( sWrap ); // 1.5.9ea Get wrap as number
			if ( iWrap < 1 ) // 1.5.9ea If invalid wrap, set to default
				iWrap = 400;
			}

		CNoteList notlst;
		CDatabaseType* ptyp=NULL; // 1.5.9ea Get database type struct by finding database type
		if ( !Shw_ptypset()->bSearch(sDBType, &ptyp, notlst) ) // 1.5.9ea If database type does not exist, give error
			{
			sMessage = sMessage + _("Database type not found:") + " " + sDBType; // 1.5.9da 
			return FALSE; 
			}
		m_pindset = new CIndexSet(ptyp, iWrap, bReadOnly); // 1.5.9ea Make index set
		
		Str8 sRecMkr = pmkrRecord()->sName(); // 1.5.9ea Get record marker
		Str8 sRecMkrXML = "<" + sRecMkr + ">"; // 1.5.9ea Make xml record marker
		Str8 sRecMkrGroupStartXML = "<" + sRecMkr + "Group"; // 1.5.9ec Make xml record marker group start
		Str8 sRecMkrGroupEndXML = "</" + sRecMkr + "Group>"; // 1.5.9ea Make xml record marker group end
		Str8 sField; // 1.5.9ea String to collect field
		Str8 sRecMkrGroupAttributes; // 1.5.9ec Attributes from rec mkr group start tag
		BOOL bInRec = FALSE; // 1.5.9ea True if in a record
		CField* pfldKey = new CField( ptyp->pmkrRecord() ); // 1.5.9ea Make new empty field to satisfy record
		CRecord* prec = new CRecord( pfldKey ); // 1.5.9ea Make new record to fill in
		Str8 sTag; // 1.5.9ea XML tag found in line
		int iTag = 0; // 1.5.9ea Location of XML tag found in line
		Str8 sLines; // 1.5.9ea Collection of lines

		while ( TRUE ) // 1.5.9ea Collect and store records
			{
			while ( sLines.Find( "</" ) < 0 ) // 1.5.9ea Collect lines until XML end tag found
				{
				sSucc = fgets( pszBigBuffer, BIGBUFMAX, pf ); // 1.5.9ea Read a line
				if ( sSucc == NULL ) // 1.5.9ea If end of file, break to stop reading lines
					break;
				sLines += pszBigBuffer; // 1.5.9ea If successful read, put line into string
				}
			if ( sSucc == NULL ) // 1.5.9ea If end of file, break to stop reading records
				{
				delete prec; // 1.5.9ea Delete empty record, deletes key field as well
				break;
				}
			if ( sLines.Find( "</database>" ) >= 0 ) // 1.5.9ed Watch for end of database to store tail info
				{
				delete prec; // 1.5.9ed Delete empty record, deletes key field as well
				break; // 1.5.9ed At end of database, go store tail info
				}
			while ( ( iTag = iXMLFind( sLines, sTag ) ) >= 0 ) // 1.5.9ea While XML tag found in line
				{
				if ( sTag == sRecMkrGroupEndXML ) // 1.5.9ea If end of record marker group, make record
					{
					m_pindset->AddRecord(prec); // 1.5.9ea Add record to index set
					pfldKey = new CField( ptyp->pmkrRecord() ); // 1.5.9ea Make new empty field to satisfy record
					prec = new CRecord( pfldKey ); // 1.5.9ea Make new record to fill in
					}
				if ( sTag.Find( "</" ) > 0 ) // 1.5.9ea If end tag, delete it and continue
					{
					sLines = sLines.Mid( iTag + sTag.GetLength() ); // 1.5.9ea Delete tag
					continue;
					}
				int iStartRecMkrGroupTag = sTag.Find( sRecMkrGroupStartXML ); // 1.5.9ec See if rec mkr group start tag
				if ( iStartRecMkrGroupTag >= 0 ) // 1.5.9ec If rec mkr group start tag, collect attributes
					{
					int iStartAttributes = sRecMkrGroupStartXML.GetLength(); // 1.5.9ec Start of possible attributes
					int iEndRecMkrGroupTag = sTag.Find( ">", iStartRecMkrGroupTag ); // 1.5.9ec Find end of tag
					int iLenAttributes = iEndRecMkrGroupTag - iStartAttributes; // 1.5.9ec Length of attributes
					if ( iLenAttributes > 0 ) // 1.5.9he If XML attributes in sf record, store them
						{
						sRecMkrGroupAttributes = sTag.Mid( iStartAttributes, iLenAttributes ); // 1.5.9ec Get attributes
						prec->m_sXMLAttributes = sRecMkrGroupAttributes; // 1.5.9ec Set attributes in record
						}
					}
				Str8 sEndTag = "</" + sTag.Mid( 1 ); // 1.5.9ea Make matching end tag
				int iEndTag = sLines.Find( sEndTag ); // 1.5.9ea Look for matching end tag
				if ( iEndTag < 0 ) // 1.5.9ea If matching end tag not found, delete tag and continue (eg group tag)
					{
					sLines = sLines.Mid( iTag + sTag.GetLength() ); // 1.5.9ea Delete tag
					continue;
					}
				int iStartTagEnd = iTag + sTag.GetLength(); // 1.5.9ea End of start tag
				Str8 sContent = sLines.Mid( iStartTagEnd, iEndTag - iStartTagEnd ); // 1.5.9ea Get content of XML field
				sContent.Replace( "&amp;", "&" ); // 1.5.9fa Undo escaped XML characters
				sContent.Replace( "&quot;", "\"" ); // 1.5.9fa Undo escaped XML characters
				sContent.Replace( "&apos;", "'" ); // 1.5.9fa Undo escaped XML characters
				sContent.Replace( "&lt;", "<" ); // 1.5.9fa Undo escaped XML characters
				sContent.Replace( "&gt;", ">" ); // 1.5.9fa Undo escaped XML characters
				if ( sTag == sRecMkrXML ) // 1.5.9da If record marker, put content into key field already in record
					{
					pfldKey->SetContent( sContent ); // 1.5.9ea Put content into key field
					}
				else
					{
					Str8 sMkr = sTag.Mid( 1, sTag.GetLength() - 2 ); // 1.5.9ea Get marker string
					CMarker* pmkr = ptyp->pmkrset()->pmkrSearch_AddIfNew( sMkr ); // 1.5.9ea Get marker of string
					CField* pfld = new CField( pmkr, sContent ); // 1.5.9ea Make field
					prec->Add( pfld ); // 1.5.9ea Add field to record
					}
				sLines = sLines.Mid( iEndTag + sEndTag.GetLength() ); // 1.5.9ea Delete XML field
				}
			}
		sHeadAndTailInfo += "\\xmltail\n"; // 1.5.9gc Store tail information separator
		while ( sSucc = fgets( pszBigBuffer, BIGBUFMAX, pf ) ) // 1.5.9ed Store tail info
			{
			sHeadAndTailInfo += pszBigBuffer; // 1.5.9ed If successful read, put line into tail info // 1.5.9gc 
			}
		m_pindset->SetHeadInfo( sHeadAndTailInfo ); // 1.5.9da Store head info on xml file read // 1.5.9gc Change to head and tail
		fclose( pf ); // 1.5.9ec Close file after reading it
		}
	else // 1.5.9da Else (not xml file) read as sf
		{
#ifdef NEWFILEREAD // 1.5.9hb Code for new file read
		CDatabaseType* ptyp = NULL; // 1.5.9ea Get database type struct by finding database type
		BOOL bImport = FALSE; // 1.5.9hb True if importing file
		Str8 sWrap = "400"; // 1.5.9hb String of wrap margin, set to default // 1.5.9mt Move to be used for import
		int iWrap = 400; // 1.5.9hf Init wrap to default
		Str8 sDBType; // 1.5.9hf Database type
		CNoteList notlst;
		const char* pszSucc = NULL; // 1.5.9da Successfully read a line
		pszSucc = fgets( pszBigBuffer, BIGBUFMAX, pf ); // 1.5.9hb Read a line
		sLine = pszBigBuffer; // 1.5.9hb Get first line to see if it is \sh line
		Str8 sBOM = "﻿"; // 1.5.9mh UTF8 Byte Order Mark // 1.5.9mv Remove BOM at beginning of file read
		if ( sLine.Find ( sBOM ) == 0 ) // 1.5.9mh If Byte Order Mark, delete it // 1.5.9mv 
			sLine = sLine.Mid( 3 ); // 1.5.9mh  // 1.5.9mv 
		if ( sLine.Find( "\\_sh" ) == 0 ) // 1.5.9my If sh header, read normally, even if export chain
			{
			int iPos = 10; // 1.5.9hb Start position for wrap margin
			sLine.bNextWord( sWrap, iPos ); // 1.5.9hb Find wrap margin
			iWrap = atoi( sWrap ); // 1.5.9hb Get wrap as number
			sDBType = sLine.Mid( iPos ); // 1.5.9ea Collect database type
				sDBType.Trim(); // 1.5.9hb Get rid of extra spaces
			if ( !Shw_ptypset()->bSearch(sDBType, &ptyp, notlst) ) // 1.5.9ea If database type does not exist, give error
				{
				sMessage = sMessage + _("Database type not found:") + " " + sDBType; // 1.5.9da 
				fclose( pf ); // 1.5.9mg Close file opened for import if cancel
				return FALSE; 
				}
			}
		else if ( Shw_pProject()->m_bExportProcessAutoOpening ) // 1.1bh On export process auto open set to same type as file doing export process // 1.5.9mf If export auto open, set type
			{
			bImport = TRUE; // 1.5.9hb Note this is import for slightly different behavior than normal read // 1.5.9mf 
			ptyp = Shw_pProject()->m_ptypExportProcessAutoOpening; // 1.5.9mf 
			}
		else // 1.5.9hf If no sh header and not a chained export, import
			{
			if ( bFileReadOnly( sPath ) ) // 1.5.9mf Refuse to import read only file
				{
				sMessage = sMessage + _("Cannot import read-only file:") + " " + pszPath; // 1.5.0fd  // 1.5.9mf 
				fclose( pf ); // 1.5.9mg Close file opened for import if cancel
				return FALSE; // 1.5.9mf 
				}
			bImport = TRUE; // 1.5.9hb Note this is import for slightly different behavior than normal read
			Str8 sImportCCT; // 1.5.9mf Import CC table, empty if none
			// Set up for import dialog box
			Str8 sFileText = sLine; // 1.5.9mf Set text to show in dialog box
			sFileText.TrimRight(); // 1.5.9mf Trim nl off line
			int nMode = 0; // 1.5.9mf Set up mode to normal, allow CC table
			Str8 sMsg = ""; // 1.5.9mf Message at top of box is default in dialog box
			CDatabaseTypeProxy* ptrx = NULL; // 1.5.9mf Database type
			BOOL bMakeOri = FALSE; // 1.5.9mf Make .ori option not shown in dialog box // 1.5.9mr Don't make .ori on import
		    CImportDlg dlg( pszPath, sFileText, nMode, sMsg, &ptrx, &bMakeOri ); // 1.5.9mf Show user import dialog box
			if ( dlg.DoModal() != IDOK )  // 1.5.9mf Show dialog box
				{
				fclose( pf ); // 1.5.9mg Close file opened for import if cancel
				return FALSE; // 1.5.9mf Cancel
				}
			ptyp = ptrx->ptyp(notlst); // load in type if not already loaded // 1.5.9mf 
			// If CC, do CC
            if ( Shw_pProject()->bUseImportCCT() ) // set by CImportDlg
				{
				// Set up for CC
                sImportCCT = Shw_pProject()->pszImportCCT();
				int fh = _fileno( pf ); // 1.5.9mh Get file handle
				int iFileLen = _filelength( fh ); // 1.5.9mh 
				Str8 sCCInBuff; // 1.5.9mh CC input buffer
				Str8 sCCOutBuff; // 1.5.9mh CC output buffer
				int iCCInBuffLen = iFileLen + 1000; // 1.5.9mh 
				int iCCOutBuffLen = 2 * iCCInBuffLen; // 1.5.9mh 
				char* pszCCInBuff = sCCInBuff.GetBuffer( iCCInBuffLen ); // 1.5.9mh Allow room for read
				char* pszCCOutBuff = sCCOutBuff.GetBuffer( iCCOutBuffLen ); // 1.5.9mh Allow plenty of room for CC expansion
				// Load file into CC buffer
				sCCInBuff += sLine; // 1.5.9mh 
				while ( pszSucc = fgets( pszBigBuffer, BIGBUFMAX, pf ) ) // 1.5.9mh While another line, put it in buffer
					{
					sLine = pszBigBuffer;
					sCCInBuff += sLine; // 1.5.9mh Append line to buffer
					}
				fclose( pf ); // 1.5.9mh Close file
				// Do CC on file
				ChangeTable cctImport; // 1.5.9mh Change table
				BOOL bCCSucc = TRUE;
				bCCSucc = cctImport.bLoadFromFile( sImportCCT ); // 1.5.9mh Load change table
				int iCCOutLen = iCCOutBuffLen; // 1.5.9mh 
				if ( bCCSucc ) // 1.5.9mh If succesful load, make changes
					bCCSucc = cctImport.bMakeChanges( pszCCInBuff, sCCInBuff.GetLength() + 1, 
						pszCCOutBuff, &iCCOutLen ); // 1.5.9mh Make changes
				if ( !bCCSucc ) // 1.5.9mh If not successful change, give message and fail
					{
					sMessage = sMessage + _("Consistent Changes failed");
					return FALSE; // 1.5.9mh 
					}
				sCCOutBuff.ReleaseBuffer( iCCOutLen ); // 1.5.9mh Reset sring
				// Write out CC'd file
				sMode = "w"; // Set up mode string to write // 1.5.9mh 
				swMode = swUTF16( sMode ); // Convert mode string to CString // 1.5.9mh 
				pf = nullptr;
				_wfopen_s(&pf, swPath, swMode);
				fputs( sCCOutBuff, pf ); // 1.5.9mh Write modified file
				fclose( pf ); // 1.5.9mh Close file
				// Open CC'd file for import
				sMode = "r"; // Set up mode string to read // 1.5.9mh 
				swMode = swUTF16( sMode ); // Convert mode string to CString // 1.5.9mh 
				pf = nullptr;
				_wfopen_s(&pf, swPath, swMode);
				pszSucc = fgets( pszBigBuffer, BIGBUFMAX, pf ); // 1.5.9mh Read a line
				sLine = pszBigBuffer; // 1.5.9mh Get first line to prepare for import
				if ( sLine.Find ( sBOM ) == 0 ) // 1.5.9mh If Byte Order Mark, delete it
					sLine = sLine.Mid( 3 ); // 1.5.9mh 
				}
			}
		m_pindset = new CIndexSet(ptyp, iWrap, bReadOnly); // 1.5.9ea Make index set
		Str8 sRecMkr = pmkrRecord()->sName(); // 1.5.9hb Get record marker
		sRecMkr = "\\" + sRecMkr; // 1.5.9hb Add backslash for search
		if ( !bImport ) // 1.5.9hf If not import, read next line after sh header line
			{
			pszSucc = fgets( pszBigBuffer, BIGBUFMAX, pf ); // 1.5.9hb Read next line
			sLine = pszBigBuffer;
			if ( sLine == "\n" ) // 1.5.9hf If next line is blank, skip it
				{
				pszSucc = fgets( pszBigBuffer, BIGBUFMAX, pf ); // 1.5.9hb Read next line
				sLine = pszBigBuffer;
				}
			}
		while ( TRUE ) // 1.5.9hb Read lines from file until start of database is found
			{
			int iMkrPos = 0; // 1.5.9ka 
			Str8 sMkrFromLine; // 1.5.9ka 
			sLine.bNextWord( sMkrFromLine, iMkrPos ); // 1.5.9ka Fix bug in 1.5.9k of finding rec mkr in longer mkr
			if ( sMkrFromLine == sRecMkr ) // 1.5.9hb If rec mkr found, break from this read loop // 1.5.9ka 
				break; // 1.5.9hb 
			sHeadAndTailInfo += sLine; // 1.5.9hb Put all lines before database start into head info
			pszSucc = fgets( pszBigBuffer, BIGBUFMAX, pf ); // 1.5.9hb Read a line
			sLine = pszBigBuffer; // 1.5.9hb Store line, NULL for eof stores empty line
			if ( !pszSucc ) // 1.5.9hb If end of file, stop reading
				break;
			}
		sHeadAndTailInfo.Replace( "\\_DateStampHasFourDigitYear", "" ); // 1.5.9my Don't keep DateStampHasFourDigitYear if input
		sHeadAndTailInfo.TrimLeft(); // 1.5.9my Eat extra white space at top of head info
		m_pindset->SetHeadInfo( sHeadAndTailInfo ); // 1.5.9hf Store head info on sf file read
		if ( !pszSucc ) // 1.5.9hb If no rec mkr found, deal with it
			{
			fclose( pf ); // 1.5.9he Close file after reading it
			if ( bImport ) // 1.5.9hb If no rec mkr found on import, give message
				{
				sMessage = sMessage + _("No record marker found in file:") + " " + sPath; // 1.5.9hb 
				fclose( pf ); // 1.5.9mg Close file opened for import if cancel
				return FALSE;
				}
			else
				{
				return TRUE; // 1.5.9hb If no rec mkr found on normal file, it is an empty db
				}
			}
		Str8 sField; // 1.5.9ea String to collect field
		Str8 sRecMkrGroupAttributes; // 1.5.9ec Attributes from rec mkr group start tag
		BOOL bInRec = FALSE; // 1.5.9ea True if in a record
		CField* pfldKey = NULL; // 1.5.9hb Key fld
		CRecord* prec = NULL; // 1.5.9hb Record
//		Str8 sTag; // 1.5.9ea XML tag found in line
//		int iTag = 0; // 1.5.9ea Location of XML tag found in line
		Str8 sLines; // 1.5.9ea Collection of lines
		CField* pfldCur = NULL; // 1.5.9hb Current field being built
		if ( sLine.Right( 1 ) == "\n" ) // 1.5.9hb If nl at end of one, remove it
			sLine = sLine.Left( sLine.GetLength() - 1 ); // 1.5.9hb Remove nl
		while ( TRUE ) // 1.5.9hb Collect and store records
			{
			int iMkrPos = 0; // 1.5.9ka 
			Str8 sMkrFromLine; // 1.5.9ka 
			sLine.bNextWord( sMkrFromLine, iMkrPos ); // 1.5.9ka Fix bug in 1.5.9k of finding rec mkr in longer mkr
			if ( !pszSucc || sMkrFromLine == sRecMkr ) // 1.5.9hb If rec mkr or eof, finish and store previous rec // 1.5.9ka 
				{
				if ( prec ) // 1.5.9hb If not first rec mkr, finish and store previous rec
					{
					Str8 sContentFixed = pfldCur->sContents();  // 1.5.9hb Remove extra nl from last fld of rec
					if ( sContentFixed.Right( 1 ) == "\n" ) // 1.5.9hb If nl at end of fld, remove it
						{
						sContentFixed = sContentFixed.Left( sContentFixed.GetLength() - 1 ); // 1.5.9hb Remove nl
						pfldCur->SetContent( sContentFixed ); // 1.5.9hb Store contentn with nl removed
						}
					m_pindset->AddRecord(prec); // 1.5.9hb Add record to index set
					}
				if ( !pszSucc ) // 1.5.9hb If end of file, stop reading
					break;
				pfldKey = new CField( ptyp->pmkrRecord() ); // 1.5.9hb Make new empty field to satisfy record
				pfldCur = pfldKey; // 1.5.9hb Remember key as current field
				prec = new CRecord( pfldKey ); // 1.5.9hb Make new record to fill in
				}
			if ( sLine.Find( "\\" ) == 0 )  // 1.5.9hb If mkr, if rec mkr, put content into key fld, else make new fld and put content into it
				{
				Str8 sMkr; // 1.5.9hb Mkr string
				int iPos = 1; // 1.5.9hb Pos of next word, start of marker string after backslash
				sLine.bNextWord( sMkr, iPos ); // 1.5.9hb Collect marker
				if ( iPos < sLine.GetLength() ) // 1.5.9he If not nl immediately, skip space
					iPos++; // 1.5.9he Move past space
				Str8 sContent = sLine.Mid( iPos ); // 1.5.9hb Collect content part of line
				if ( sMkrFromLine == sRecMkr ) // 1.5.9hb If rec mkr, put content into key fld
					pfldKey->SetContent( sContent ); // 1.5.9ea Put content into key field
				else // 1.5.9hb Else (not rec mkr) make field
					{
					if ( sMkr == "xmlattr" ) // 1.5.9he If xmlid, save as that
						prec->m_sXMLAttributes = " " + sContent;
					else // 1.5.9he Else (normal mkr), make new fld and store in rec
						{
						CMarker* pmkr = ptyp->pmkrset()->pmkrSearch_AddIfNew( sMkr ); // 1.5.9hb Get marker of string
						CField* pfld = new CField( pmkr, sContent ); // 1.5.9hb Make field
						pfldCur = pfld; // 1.5.9hb Remember this as current field
						prec->Add( pfld ); // 1.5.9hb Add field to record
						}
					}
				}
			else // 1.5.9hb If not mkr add line to fld
				{
				pfldCur->SetContent( pfldCur->sContents() + "\n" + sLine ); // 1.5.9hb Add nl and line to fld
				}
			pszSucc = fgets( pszBigBuffer, BIGBUFMAX, pf ); // 1.5.9hb Read next line
			sLine = pszBigBuffer; // 1.5.9hb Store line, NULL for eof stores empty line
			if ( sLine.Right( 1 ) == "\n" ) // 1.5.9hb If nl at end of one, remove it
				sLine = sLine.Left( sLine.GetLength() - 1 ); // 1.5.9hb Remove nl
			}
		fclose( pf ); // 1.5.9he Close file after reading it
		if ( m_pindset->bSort() ) // 1.5.9mp Fix bug of not sorting imported file
			SetModifiedFlag(TRUE); // 1.5.9mp Fix bug of not sorting imported file
		if ( bImport ) // 1.5.9mh If Import, save modified file
			{
			CProject* pprj = Shw_papp()->pProject(); // 1.5.9ms 
			BOOL bBackupSetting = pprj->bNoBackup(); // 1.5.9ms Remember backup setting
			pprj->SetNoBackup( TRUE ); // 1.5.9ms Don't make backup file on export chain
			OnSaveDocument( sPath ); // 1.5.9mh Save file
			pprj->SetNoBackup( bBackupSetting ); // 1.5.9ms Restore backup setting
			}
		}
#endif // NEWFILEREAD // 1.5.9hb 
#ifndef NEWFILEREAD // 1.5.9hb 
		std::ifstream ios(pszPath);
		if ( ios.fail() )
			{
			sMessage = sMessage + _("Cannot open file:") + " " + pszPath; // 1.5.0fd 
			return FALSE;
			}

		// If the file begins with an Sh3 database header line, use its properties to construct the index set;
		// otherwise let the user choose a database type and import the file.
		if ( bFileReadOnly(pszPath) ) // 1.4ysa Make sure read only gets written to project file
			SetReadOnly( TRUE ); // 1.4ysa 
		BOOL bReadOnly = m_bReadOnly || bFileReadOnly(pszPath);
		BOOL bStringTooLong = FALSE;   // 07-26-1997
		Str8 sMissingDBType;     // 02-12-98
		{
		//The class Newline_istream provides datas from both, Mac and PC 11-08-1997
		Newline_istream iosnl(ios);

		if ( !CIndexSet::s_bReadPropertiesFromSh3Header(iosnl, bReadOnly, &m_pindset, &bStringTooLong, sMissingDBType ) )
			{       //not able to open as a database
			if (bStringTooLong)  // 1) String is to long -> stop "File-open" procedure
				return FALSE; 
			else if (!sMissingDBType.IsEmpty() ) 
				{               // 2) Special import due to missing db type  02-12-1998
				ios.close();
				CDBTypeNFDlg dlg (pszPath, sMissingDBType);
				MessageBeep(0);
				if ( dlg.DoModal() != IDOK )        // DB Type Not Found dialog
					return FALSE;
				return bImport(pszPath, iDBTypeNotFound, sMissingDBType, sMessage);
				}
			else 
				{                // 3) Normal Import
				ios.close();  // 1996-07-26 MRP
				return bImport(pszPath, iNormal, "", sMessage);
				}
			}

		BOOL bOpeningProject = Shw_papp()->bOpeningProject();
		if ( bFileReadOnly(pszPath) && !bOpeningProject ) // 5.96zb Don't ask about read-only if project is opening it
			{
			Str8 sMessage2;  // NOTE: Does NOT get returned to caller
			sMessage2 = _("Open read-only file?"); // 1.5.0fd
			if ( AfxMessageBox(sMessage2, MB_ICONQUESTION | MB_YESNO ) != IDYES ) // 5.96za Change default to yes // 1.4qpr
				return FALSE;
			}

		}  // 1998-04-22 MRP: End of scope for first occurrence of iosnl
		// This is an emergency patch to make automatic conversion
		// of datestamp fields work in Shoebox 4. Here's the problem:
		// a Shoebox 3 file has the \_sh header but doesn't have the new
		// internal property marker \_DateStampHasFourDigitYear, therefore
		// s_ bReadPropertiesFromSh3Header has read and cached the first
		// data field. So, we reread from the beginning and let SF_istream
		// skip over all internal fields.
		ios.close();
		ios.open(pszPath);
	// ????? AB Collaborate 65 reports that this assert fires on Mac in rare long-line situations, with loss of data
		ASSERT( !ios.fail() );
		Newline_istream iosnl(ios);

		// Proceed to load the database
		// 08-11-1997 Changed Paramterlist (Newline_istream)
		SF_istream sfs(iosnl, m_pindset->ptyp()->pmkrset(), m_pindset->ptyp()->pmkrRecord());
		Str8 sHeadInfo = sfs.sSkippedFields(); // 1.4pcg Collect head info on normal file read
		m_pindset->SetHeadInfo( sHeadInfo ); // 1.4pch Store head info on normal file read
		if ( !bReadRecords(pszPath, ios, sfs) ) 
			return FALSE;
		}
#endif // ifndef NEWFILEREAD // 1.5.9hb 

    // 1998-04-22 MRP: Moved to OnOpenDocument
    // Set the path and file name.
    // SetPathName(pszPath);
    // SetTitle(pszPath);

    CFileStatus fileStat;    
    if (!CFile::GetStatus( swPath, fileStat)) // 1.4qug Upgrade GetStatus for Unicode build
		return FALSE;
    m_timeLastLoaded = fileStat.m_mtime;

    return TRUE;
}


static const char* s_pszTempExt = ".tmp";
static const char* s_pszOriginalExt = ".ori";
static const char* s_pszBackupExt = ".bak";

BOOL CShwDoc::bImport(const char* pszPath, const int nMode, const Str8& sMissingDBType, Str8& sMessage)
{
    ASSERT( pszPath );
    if ( bFileReadOnly(pszPath) )
        {
		sMessage = sMessage + _("Cannot import read-only file:") + " " + pszPath; // 1.5.0fd 
        return FALSE;
        }
    
    // Set up for Import Dialog
    Str8 sFileText;
	if ( !Shw_pProject()->m_bExportProcessAutoOpening ) // 1.1bh Not on export process auto open
		{
		int fh = -1;
		errno_t err = _sopen_s(&fh, pszPath, _O_RDONLY | _O_BINARY, _SH_DENYNO, 0);
		if (err != 0 || fh == -1)
		{
			sMessage = sMessage + _("Cannot open file:") + " " + pszPath; // 1.5.0fd 
			return FALSE;
		}
		unsigned int bytesread;
		char* pszBuf = sFileText.GetBuffer(TEXTSTRING_MAX); // 1.4qzft GetBuffer OK because written immediately // 1.4hc Fix possibility of not releasing string buffer
		bytesread = _read( fh, pszBuf, TEXTSTRING_MAX ); // read first 32k of file
		_close( fh );
		sFileText.ReleaseBuffer(bytesread);
		}
	if ( sFileText.Find( "\\+ShProjectSettings" ) == 0 ) // 1.2mk If File, Open of project file, give warning // 1.5.0fh Fix problem of thinking message file is project file
        {
		sMessage = _("This file is a Toolbox project file. It must be opened using Project, Open."); // 1.5.0fg 
        return FALSE;
        }

    BOOL bMakeOri = FALSE; // 5.98p Default to not make .ori
    CDatabaseTypeProxy* ptrx = NULL;

    CImportDlg dlg( pszPath, sFileText, nMode, sMissingDBType, &ptrx, &bMakeOri );

    // Provide a loop to re-try the import if certain user-correctable errors occur
    BOOL bLoop = FALSE;     // normally, never repeat this do loop 
    do {
        bLoop = FALSE;      // reset if we do repeat

        //Present the Import Dialog
		if ( !Shw_pProject()->m_bExportProcessAutoOpening ) // 1.1bh Not on export process auto open
			if ( dlg.DoModal() != IDOK ) 
				return FALSE;

        const char* pszCCTable = "";
        BOOL bRemoveSpaces = FALSE;
        if (nMode != iDBTypeNotFound && !Shw_pProject()->m_bExportProcessAutoOpening ) // 1.1bh Not on export process auto open
            {
            if ( Shw_pProject()->bUseImportCCT() ) // set by CImportDlg
                pszCCTable = Shw_pProject()->pszImportCCT();
            bRemoveSpaces = Shw_pProject()->bRemoveSpaces();
            }
    
        CNoteList notlst;
        CDatabaseTypePtr ptyp = NULL;
		if ( Shw_pProject()->m_bExportProcessAutoOpening ) // 1.1bh On export process auto open set to same type as file doing export process
			ptyp = Shw_pProject()->m_ptypExportProcessAutoOpening;
		else
			ptyp = ptrx->ptyp(notlst); // load in type if not already loaded

        // Set up Input Stream
        std::ifstream ios(pszPath);
        if ( ios.fail() )
            {
			sMessage = sMessage + _("Cannot open file:") + " " + pszPath; // 1.5.0fd
            return FALSE;
            } 
        // 1997-10-28 MRP: We have to test the ifstream's fail bit ABOVE
        // because the Newline_istream constructor reads ahead and
        // that can hit end-of-file of a small file (which sets the fail bit).
        // 08-11-1997
        //The class Newline_istream provieds datas from both, Mac and PC
        Newline_istream iosnl(ios);

        //Add Changes to input stream if specified
        std::istream* pios = &iosnl;     // a pointer to the actual input stream
        if ( *pszCCTable )
            {
            // 08-11-1997 - Changed parameterlist (Newline_istream)
            pios = new Change_istream ( pszCCTable, iosnl ); // open input stream using cc table
            if ( pios->fail() )
                {       // if failure to load valid CC table
                delete pios;
                bLoop = TRUE;
                continue;           // rerun do-while loop
                //return FALSE;
                }
            }

        // Set up the SF Reader object
        SF_istream sfr(*pios, ptyp->pmkrset(), ptyp->pmkrRecord(), TRUE, bRemoveSpaces);
        long lFieldsSkipped = sfr.lFieldsSkipped();
        long lLinesSkipped = sfr.lUsefulLinesSkipped();

        // Handle if record marker not found
        if (sfr.bAtEnd())
            {
            Str8 sMsg = _("Record marker not found"); // 1.5.0fg 
            AfxMessageBox (sMsg, MB_OK | MB_ICONEXCLAMATION);
            if ( *pszCCTable )  
				delete pios;
			return FALSE; // 1.4aa Fix bug of crash when record marker not found in import
            }

        // Proceed to import the file as a ShwDoc
        m_pindset = new CIndexSet(ptyp, 400, FALSE); // default margin setting of 400 pixels
        
		Str8 sHeadInfo = sfr.sSkippedFields(); // 1.4pcm Store head info on import
		m_pindset->SetHeadInfo( sHeadInfo ); // 1.4pcm Store head info on import

        // Read the records.
        if ( !bReadRecords(pszPath, ios, sfr) ) //within loop because sfr in scope
            {               //if errors reading records
            if ( *pszCCTable )  delete pios;
            return FALSE;
            }
        
        if ( *pszCCTable )  delete pios;    //clean up before proceeding
        ios.close();
    } while (bLoop);    // We loop back only from continue statements above on a user-correctable error.

    CIndex* pind = pindset()->pindRecordOwner(); // 1.0-ab Change tab to space on import
    for ( CNumberedRecElPtr pnrl = pind->pnrlFirst(); pnrl; pnrl = pind->pnrlNext( pnrl ) )
        {
        CRecord* prec = pnrl->prec();
		for ( CField* pfld = prec->pfldFirst(); pfld; pfld = prec->pfldNext( pfld ) )
			{
			CRecPos rpsFirstTab( 0, pfld, prec ); // 1.4qrh On import, if tabs in interlinear, align the interlinear
			if ( rpsFirstTab.bInterlinear() && ( pfld->Find( '\t' ) != -1 ) ) // 1.4qrh If field contains a tab and is interlinear
				{
				CRecPos rps = rpsFirstTab;
				for ( ; ; rps.pfld = rps.pfldNext() ) // 1.4qrh For the rest of fields in the bundle
					{
					rps.pfld->OverlayAll( ' ', '\a' ); // 1.4qrh Overlay all space with temp ctrl char \a (bell)
					rps.pfld->OverlayAll( '\t', ' ' ); // 1.4qrh Overlay all tab with space
					if ( rps.bLastInBundle() ) // 1.4qrh 
						break;
					}
				rpsFirstTab.bAlignWholeBundle(); // 1.4qrh Do standard Align, from first line in which tab was found
				for ( rps = rpsFirstTab; ; rps.pfld = rps.pfldNext() ) // 1.4qrh For the rest of fields in the bundle
					{
					rps.pfld->OverlayAll( '\a', ' ' ); // 1.4qrh Overlay all temp ctrl char \a with space
					if ( rps.bLastInBundle() ) // 1.4qrh 
						break;
					}
				pfld = rps.pfld; // 1.4qrh Move past the interlinear fields that were aligned
				}
			else // 1.4qrh
				pfld->OverlayAll( '\t', ' ' ); // 1.0-ab Change tab to space on import
			}
		}

    // 1. Write the contents of the imported database to a temporary file.
    Str8 sDirPath = sGetDirPath(pszPath);
    Str8 sFileName = sGetFileName(pszPath, FALSE);
    Str8 sTempFileName = sUniqueFileName(sDirPath, sFileName, s_pszTempExt, FALSE, TRUE, TRUE);
    Str8 sTempPath = sPath(sDirPath, sTempFileName, s_pszTempExt);

    BOOL bWritten = bWrite(sTempPath);
    BOOL bTempExists = ( bFileExists(sTempPath) );
    if ( !bWritten )
        {
        if ( bTempExists )
            remove(sTempPath);
		sMessage = sMessage + _("Cannot open file:") + " " + pszPath; // 1.5.0fd
        return FALSE;
        }
	#ifdef _MAC
	XferFileLabel(pszPath, sTempPath); //set label of sTempPath   6/98
	#endif
        
    // 2. Rename the current Standard Format file as an original copy,
    //      or delete it if user requested so.
    if ( bMakeOri )
        {
        Str8 sOriFileName = sUniqueFileName(sDirPath, sFileName, s_pszOriginalExt, FALSE, TRUE, TRUE);
        Str8 sOriPath = sPath(sDirPath, sOriFileName, s_pszOriginalExt);
        if ( rename(pszPath, sOriPath) != 0 )
            {
            remove(sTempPath);
			sMessage = sMessage + _("Cannot open file:") + " " + pszPath; // 1.5.0fd
            return FALSE;
            }
        }
    else if ( remove(pszPath) != 0 )
        {
        remove(sTempPath);
		sMessage = sMessage + _("Cannot open file:") + " " + pszPath; // 1.5.0fd
        return FALSE;
        }
        
    // 3. Rename the temporary file as the database file.
    if ( rename(sTempPath, pszPath) != 0 )
        {
		sMessage = sMessage + _("Cannot open file:") + " " + pszPath; // 1.5.0fd
        return FALSE;
        }
    SetModifiedFlag(FALSE);  // 1996-04-16
    return TRUE;
}



BOOL CShwDoc::bReadRecords(const char* pszPath, std::ifstream& ios, SF_istream& sfs)
{
    ASSERT( pszPath );
    LONG lSizFile = lFileSize(pszPath);
    {
    CProgressIndicator prg(lSizFile, NULL, pszPath, TRUE); // 1.4ad Eliminate resource messages sent to progress bar
    while ( !sfs.bAtEnd() )
        {
        CRecord* prec = NULL;
        if (!sfs.bReadRecord(&prec)) 
            return FALSE;
        m_pindset->AddRecord(prec);  // 1995-05-26 MRP
		LONG lPos = static_cast<LONG>(ios.tellg());
        if ( !prg.bUpdateProgress(lPos) )
            return FALSE;
        }
    }  // 1998-04-22 MRP: Cause prg object to be deleted so that progress message for datestamp conversion or sorting shows correctly.

    BOOL bSkipped = FALSE;  // 1998-04-22 MRP
    
    BOOL bResorted = m_pindset->bSort();
    if ( bSkipped || bResorted )
        SetModifiedFlag(TRUE);
    
    return TRUE;
}  // CShwDoc::bReadRecords

BOOL CShwDoc::bSave(const char* pszPath, Str8& sMessage)
{
	CProject* pprj = Shw_papp()->pProject();
	if (pprj->bExerciseNoSave())
		return TRUE;

	if (!bReadOnly())
	{
		Str8 sPath = sGetDirPath(pszPath) + sGetFileName(pszPath, TRUE);
		Str8 sTempPath = sPath + ".tmp";
		Str8 sBackupPath = sPath + ".bak";

		CString swPath = swUTF16(sPath);
		CString swTempPath = swUTF16(sTempPath);
		CString swBackupPath = swUTF16(sBackupPath);

		// Clean up old backups or unprotect existing backup for ReplaceFileW
		if (bFileExists(sBackupPath))
		{
			UnWriteProtect(sBackupPath);
			if (pprj->bNoBackup())
				DeleteFileW(swBackupPath);
		}

		// Write the temp file, retry on transient lock/access errors
		UnWriteProtect(sTempPath); // clear any stale leftover from a crash
		BOOL bWroteTemp = FALSE;
		DWORD dwErr = ERROR_SUCCESS;
		const int kMaxWriteTries = 8;
		for (int i = 0; i < kMaxWriteTries; ++i)
		{
			bWroteTemp = bWrite(sTempPath, FALSE, NULL, NULL, FALSE, NULL, &dwErr);
			if (bWroteTemp)
				break;
			if (dwErr == ERROR_SHARING_VIOLATION || dwErr == ERROR_LOCK_VIOLATION || dwErr == EACCES)
				Sleep(100);
			else
				break; // permanent failure (e.g. disk full, bad path) — don't waste time retrying
		}
		if (!bWroteTemp)
		{
			sMessage.Format("Failed to write file (code: %lu)", dwErr);
			DeleteFileW(swTempPath);
			return FALSE;
		}

		// Atomically swap temp into place
		UnWriteProtect(sPath);
		BOOL bSuccess = FALSE;
		LPCWSTR pwszBackup = NULL;
		if (!pprj->bNoBackup())
			pwszBackup = swBackupPath;
		const int kMaxSwapTries = 8;
		for (int i = 0; i < kMaxSwapTries; ++i)
		{
			bSuccess = ReplaceFileW(swPath, swTempPath, pwszBackup, REPLACEFILE_IGNORE_MERGE_ERRORS, NULL, NULL);
			if (bSuccess)
				break;

			dwErr = GetLastError();
			if (dwErr == ERROR_FILE_NOT_FOUND) // first save — nothing to swap into yet
			{
				bSuccess = MoveFileExW(swTempPath, swPath, MOVEFILE_REPLACE_EXISTING);
				if (bSuccess)
					break;
				dwErr = GetLastError();
			}

			if (dwErr == ERROR_SHARING_VIOLATION || dwErr == ERROR_LOCK_VIOLATION)
				Sleep(100);
			else
				break;
		}

		if (!bSuccess)
		{
			sMessage.Format("Failed to write file (Win32 code: %lu)", dwErr);
			DeleteFileW(swTempPath);
			return FALSE;
		}
	}

	if (ptyp()->bCCTable())
	{
		POSITION pos = GetFirstViewPosition();
		if (pos != NULL)
		{
			CShwView* pView = (CShwView*)GetNextView(pos);
			pView->OnFileExport();
		}
	}
	SetModifiedFlag(FALSE);
	return TRUE;
}

#ifdef NotYetInterlinear // 1.5.9pa 
void WriteInterlinearGroup( const CRecord* prec, CField*& pfld, FILE* pf, CMarkerSubSet* psubsetMarkersToExport ) // 1.5.9eb Write mkr group tag and fields under the mkr // 1.5.9ec Add rec mkr group attributes
	{
	CMarker* pmkr = pfld->pmkr(); // 1.5.9eb Get marker
	Str8 sInterlinearBeginTag = "<Interlinear>\n"; // 1.5.9eb Make begin tag // 1.5.9ec Add rec mkr attributes
	Str8 sInterlinearEndTag = "</Interlinear>\n"; // 1.5.9eb Make end tag, include line break
	fputs( sInterlinearBeginTag, pf ); // 1.5.9eb Output group begin tag
	while ( pfld ) // 1.5.9eb For each field in group, write the field
		{
		if ( !pfld->pmkr()->bInterlinear() ) // 1.5.9nd If end of interlinear, stop
			break;
		WriteField( pfld, pf, psubsetMarkersToExport ); // 1.5.9eb Write next field of group
		pfld = prec->pfldNext(pfld) ;
		}
	fputs( sInterlinearEndTag, pf ); // 1.5.9eb Output group end tag
	}

	// 1.5.9pa Maybe someday will go below
		if ( pfld->pmkr()->bFirstInterlinear() ) // 1.5.9nd If start of interlinear, write interlin
			{
			WriteInterlinearGroup( prec, pfld, pf, psubsetMarkersToExport ); // 1.5.9nd Write interlin
			continue; // 1.5.9nd Write group has moved pfld forward
			}
		if ( !bOver( pmkrStart, pfld->pmkr() ) ) // 1.5.9eb If next mkr not under group mkr, group is done
			break;
#endif

void WriteField( const CField* pfld, FILE* pf, CMarkerSubSet* psubsetMarkersToExport ) // 1.5.9eb Write a field
	{
	if ( !psubsetMarkersToExport || psubsetMarkersToExport->bIncludes( pfld->pmkr() ) ) // 1.5.9mq Export subset of fields
		{
		Str8 sMkr = pfld->pmkr()->sMarker(); // 1.5.9eb Get marker
		Str8 sBeginTag = "<" + sMkr + ">"; // 1.5.9eb Make begin tag
		Str8 sEndTag = "</" + sMkr + ">\n"; // 1.5.9eb Make end tag, include line break
		Str8 sCont = pfld->sContents(); // 1.5.9eb Get contents
		sCont.Replace( "&", "&amp;" ); // 1.5.9fa Do escaped XML characters
		sCont.Replace( "\"", "&quot;" ); // 1.5.9fa Do escaped XML characters
		sCont.Replace( "'", "&apos;" ); // 1.5.9fa Do escaped XML characters
		sCont.Replace( "<", "&lt;" ); // 1.5.9fa Do escaped XML characters
		sCont.Replace( ">", "&gt;" ); // 1.5.9fa Do escaped XML characters
		fputs( sBeginTag, pf ); // 1.5.9eb Output begin tag
		fputs( sCont, pf ); // 1.5.9eb Output content
		fputs( sEndTag, pf ); // 1.5.9eb Output end tag and line break
		}
	}

void WriteGroupBeginTag( CMarker* pmkr, FILE* pf, Str8 sXMLAttributes = "" ) // 1.6.1ce Write group begin tag for mkr
	{
	Str8 sGroupMkr = pmkr->sMarker(); // 1.5.9eb Get marker string // 1.6.1ce Build group marker from mkr
	Str8 sGroupBeginTag = "<" + sGroupMkr + "Group" + sXMLAttributes + ">\n"; // 1.5.9eb Make begin tag // 1.5.9ec Add rec mkr attributes
	fputs( sGroupBeginTag, pf ); // 1.5.9eb Output group begin tag
	}

void WriteGroupEndTag( CMarker* pmkr, FILE* pf ) // 1.6.1ce Write group end tag for mkr
	{
	Str8 sGroupMkr = pmkr->sMarker(); // 1.5.9eb Get marker string // 1.6.1ce Build group marker from mkr
	Str8 sGroupEndTag = "</" + sGroupMkr + "Group>\n"; // 1.5.9eb Make end tag, include line break
	fputs( sGroupEndTag, pf ); // 1.5.9eb Output group end tag
	}

BOOL bGroup( const CMarker* pmkr ) // 1.5.9eb See if mkr needs a group because it has markers under it
{
    ASSERT( pmkr );
    CMarkerSet* pmkrset = pmkr->pmkrsetMyOwner();
    CMarker* pmkrUnder = pmkrset->pmkrFirst();
    for ( ; pmkrUnder; pmkrUnder = pmkrset->pmkrNext(pmkrUnder) )
        if ( pmkrUnder->pmkrOverThis() == pmkr )
            return TRUE;
    return FALSE;
}

BOOL bOver( CMarker* pmkrOver, CMarker* pmkrUnder ) // 1.5.9eb Return true if pmkrOver is somewhere over pmkrUnder
	{
	while ( pmkrUnder ) // 1.5.9eb While more to climb, check for over
		{
		if ( pmkrUnder->pmkrOverThis() == pmkrOver ) // 1.5.9eb If over is found, return true
			return TRUE;
		pmkrUnder = pmkrUnder->pmkrOverThis(); // 1.5.9eb Try next one up
		}
	return FALSE; // 1.5.9eb If over not found above under, return false
	}

CMarker* pmkrGroupIntermediate( CMarker* pmkrOver, CMarker* pmkrUnder ) // 1.6.1ce See if intermediate group between two markers
	{
	if ( !bOver( pmkrOver, pmkrUnder ) ) // 1.6.1ce 
		return NULL; // 1.6.1ce Return null for none between
	if ( pmkrUnder->pmkrOverThis() == pmkrOver ) // 1.6.1ce If none between, return null
		return NULL; // 1.6.1ce Return null for none between
	CMarker* pmkrIntermediate = pmkrUnder->pmkrOverThis(); // 1.6.1ce Get an intermediate marker
	while ( pmkrIntermediate->pmkrOverThis() != pmkrOver ) // 1.6.1ce While next up is not the over
		pmkrIntermediate = pmkrIntermediate->pmkrOverThis(); // 1.6.1ce Try next one up until the one directly under the over mkr is found
	return pmkrIntermediate; // 1.6.1ce 
	}

extern void WriteFields( const CRecord* prec, CField*& pfld, FILE* pf, CMarker* pmkrGroup, CMarkerSubSet* psubsetMarkersToExport ); // 1.6.1cf Write fields at this level

void WriteGroup( const CRecord* prec, CField*& pfld, FILE* pf, CMarker* pmkrGroup, CMarkerSubSet* psubsetMarkersToExport, Str8 sXMLAttributes = "" ) // 1.6.1cf Write one group
	{
	WriteGroupBeginTag( pmkrGroup, pf, sXMLAttributes ); // 1.6.1cf Write begin tag
	WriteFields( prec, pfld, pf, pmkrGroup, psubsetMarkersToExport ); // 1.6.1cf Write fields in group
	WriteGroupEndTag( pmkrGroup, pf ); // 1.6.1cf Write end tag
	}

void WriteFields( const CRecord* prec, CField*& pfld, FILE* pf, CMarker* pmkrGroup, CMarkerSubSet* psubsetMarkersToExport ) // 1.6.1cf Write fields at this level
	{
	while( TRUE ) // 1.6.1cf For each field in current group, write the field, or its group if needed
		{
		CMarker* pmkrIntermediate = pmkrGroupIntermediate( pmkrGroup, pfld->pmkr() ); // 1.6.1cf Get possible intermediate group
		while ( pmkrIntermediate )  // 1.6.1cf If intermediate group, write the group
			{
			WriteGroup( prec, pfld, pf, pmkrIntermediate, psubsetMarkersToExport ); // 1.6.1cf Write fields in group
			if ( !pfld || !bOver( pmkrGroup, pfld->pmkr() ) ) // 1.6.1cf If end of rec or next mkr not under group mkr, group is done
				break;
			pmkrIntermediate = pmkrGroupIntermediate( pmkrGroup, pfld->pmkr() ); // 1.6.1cf Get possible intermediate group // 1.6.1cg Fix bug of group close one field too late
			}
		if ( !pfld || ( pmkrGroup != pfld->pmkr() && !bOver( pmkrGroup, pfld->pmkr() ) ) ) // 1.6.1cf If end of rec or next mkr not under group mkr, group is done
			break;
		if ( bGroup( pfld->pmkr() ) && ( pmkrGroup != pfld->pmkr() ) ) // 1.6.1cf If another bgroup (mkr has mkrs under it in hierarchy), write it
			WriteGroup( prec, pfld, pf, pfld->pmkr(), psubsetMarkersToExport ); // 1.6.1cf Write group, move pfld forward
		else
			{
			WriteField( pfld, pf, psubsetMarkersToExport ); // 1.6.1cf Write this field of group
			pfld = prec->pfldNext(pfld); // 1.6.1cf Move to next field
			}
		if ( !pfld || !bOver( pmkrGroup, pfld->pmkr() ) ) // 1.6.1cf If end of rec or next mkr not under group mkr, group is done
			break;
		}
	}

#ifdef OldWayWriteGroup // 1.6.1cf 
void WriteGroup( const CRecord* prec, CField*& pfld, FILE* pf, CMarkerSubSet* psubsetMarkersToExport, Str8 sXMLAttributes = "", CMarker* pmkrGroupAbove = NULL ) // 1.5.9eb Write mkr group tag and fields under the mkr // 1.5.9ec Add rec mkr group attributes // 1.6.1ce Make attributes optional, only used for top call
	{ // 1.5.9eb Called recursively for group under group
	CMarker* pmkrGroupCur = pfld->pmkr(); // 1.5.9eb Get marker from fld
	if ( !pmkrGroupAbove ) // 1.6.1ce If no mkr above, use current, starts top field correctely
		pmkrGroupAbove = pmkrGroupCur; // 1.6.1ce 
	CMarker* pmkrIntermediate = pmkrGroupIntermediate( pmkrGroupAbove, pmkrGroupCur ); // 1.6.1ce Get possible intermediate group
	if ( pmkrIntermediate )  // 1.6.1ce If intermediate group, write its tags
		{
		WriteGroupBeginTag( pmkrIntermediate, pf ); // 1.6.1ce Write intermediate group begin tag
		WriteGroup( prec, pfld, pf, psubsetMarkersToExport, "", pmkrIntermediate ); // 1.6.1ce Write group inderneath
		WriteGroupEndTag( pmkrIntermediate, pf ); // 1.6.1ce Write intermediate group end tag
		}
	WriteGroupBeginTag( pmkrGroupCur, pf, sXMLAttributes ); // Write group begin tag // 1.6.1ce Change to function
	WriteField( pfld, pf, psubsetMarkersToExport ); // 1.5.9eb Write first field of group
	pfld = prec->pfldNext(pfld); // 1.5.9eb Move to next field
	while ( pfld ) // 1.5.9eb For each field in group, write the field or the field group
		{
		if ( !bOver( pmkrGroupCur, pfld->pmkr() ) ) // 1.5.9nd If next mkr not under group mkr, group is done
			break;
		if ( bGroup( pfld->pmkr() ) ) // 1.5.9eb If another bgroup (mkr has mkrs under it in hierarchy), write it
			WriteGroup( prec, pfld, pf, psubsetMarkersToExport, "", pmkrGroupAbove ); // 1.5.9eb Write group, move pfld forward
		else
			{
			WriteField( pfld, pf, psubsetMarkersToExport ); // 1.5.9eb Write next field of group
			pfld = prec->pfldNext(pfld);
			}
		}
	WriteGroupEndTag( pmkrGroupCur, pf ); // Write group end tag // 1.6.1ce Change to function
	}
#endif // OldWayWriteGroup // 1.6.1cf 

BOOL CShwDoc::bWrite(const char* pszPath, BOOL bExport, CIndex* pindExport, 
		CMarkerSubSet* psubsetMarkersToExport, BOOL bCurrentRecord, const CRecLookEl* prelCur, DWORD* pdwError)
{
    ASSERT( pszPath );
	if (pdwError) *pdwError = ERROR_SUCCESS;
	Str8 sPath = pszPath; // 1.5.9eb Put path into string
	CString swPath = swUTF16( sPath ); // 1.5.9eb Make utf16 path for file open
	Str8 sMode = "w"; // Set up mode string // 1.5.9eb 
	CString swMode = swUTF16( sMode ); // Convert mode string to CString // 1.5.9eb 
	FILE* pf = nullptr;
	errno_t err = _wfopen_s(&pf, swPath, swMode);	// 1.5.9eb Open file
	if (err != 0 || pf == nullptr) {	// If file open failed, return failure // 1.5.9eb 
		if (pdwError) // Capture error immediately, before anything else can overwrite it
		{
			DWORD dwWin32 = GetLastError(); // _wfopen_s calls CreateFileW internally, GetLastError often still valid
			*pdwError = (dwWin32 != ERROR_SUCCESS) ? dwWin32 : (DWORD)err; // Fall back to errno_t if no Win32 code was set
		}
		return FALSE;
	}
	Str8 sTyp = ptyp()->sName(); // 1.5.9eb Get database type name
	CShwView* pview = Shw_papp()->pviewActive();
	int iMargin = 100; // 1.6.1cc 
	if ( pview ) // 1.6.1cc Fix bug of crash on file open on new empty project
		iMargin = pview->iMargin(); // 1.5.9eb Get margin
	Str8 sMargin;
	sMargin += iMargin;
	Str8 sHeadInfo = pindset()->sGetHeadInfo(); // 1.5.9gc Get head info
	if ( bXMLFileType( sPath ) ) // 1.5.9da If xml file, write as xml // 1.5.9ge Include xhtml file type as xml
		{
		Str8 sXMLHead; // 1.5.9gc XML head info
		Str8 sXMLTail; // 1.5.9gc XML tail info
		int iXMLHead = sHeadInfo.Find( "\\xmlhead\n" ); // 1.5.9gc Find xmlhead marker
		int iXMLTail = sHeadInfo.Find( "\\xmltail\n" ); // 1.5.9gc Find xmltail marker
		if ( iXMLTail > 0 ) // 1.5.9gc If xmltail marker found, collect xml tail info
			{
			sXMLTail = sHeadInfo.Mid( iXMLTail + 9 ); // 1.5.9gc xml tail info is from tail mkr to end
			}
		if ( iXMLHead >= 0 ) // 1.5.9gc If xmlhead marker found, collect xml head info
			{
			int iXMLHeadLen = iXMLTail - ( iXMLHead + 9 ); // 1.5.9gc Calculate end of head mkr, if none, produces neg
			sXMLHead = sHeadInfo.Mid( iXMLHead + 9, iXMLHeadLen ); // 1.5.9gc xml head info is from head mkr tail mkr
			}
		fputs( "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n", pf ); // 1.5.9eh Always output main declaration on xml save
		fputs( sXMLHead, pf ); // 1.5.9gc Write head info
		Str8 sDBBeginTag = "<database toolboxdatabasetype=\"" + sTyp + "\"" 
			+ " wrap=\"" + sMargin + "\""
			+ " toolboxversion=\"" + g_sVersion + "\"" // 1.5.9ee Add Toolbox version to xml attributes
			+ " toolboxwrite=\"save\"" // 1.5.9ef Add Toolbox write as file save to xml attributes
			+ ">\n"; // 1.5.9eb Make database begin tag
		fputs( sDBBeginTag, pf ); // 1.5.9eb Write database begin tag
		CIndexPtr pind = pindset()->pindRecordOwner(); // 1.5.9eb Get record owning index
		if ( pindExport ) // 1.5.9mn If export sorted by index of current window, use it
			pind = pindExport; // 1.5.9mn 
		const CRecLookEl* prel = pind->prelFirst();
		if ( bCurrentRecord ) // 1.5.9mu If export current record, select current rec
			prel = prelCur; // 1.5.9mu Select current rec
		for ( ; prel; prel = pind->prelNext(prel) ) // 1.5.9eb For each record, write the record
			{
			const CRecord* prec = prel->prec();
			CField* pfld = prec->pfldFirst();
			WriteGroup( prec, pfld, pf, pfld->pmkr(), psubsetMarkersToExport, prec->m_sXMLAttributes ); // 1.5.9eb Write key field group // 1.5.9ec Write rec mkr group attributes
			if ( bCurrentRecord ) // 1.5.9mu If export current record, stop after record is output
				break; // 1.5.9mu 
			fputs( "\n", pf ); // 1.5.9eb Output line break between records
			}
		Str8 sDBEndTag = "</database>\n"; // 1.5.9eb Make database end tag
		fputs( sDBEndTag, pf ); // 1.5.9eb Write database end tag
		fputs( sXMLTail, pf ); // 1.5.9gc Write tail info
		fclose( pf ); // 1.5.9eb Close file to finish write
		}
	else // 1.5.9ga Else (not xml), write as sf
		{
		Str8 sMarginTrailingSpaces = " "; // 1.5.9ga Set up trailing spaces, margin plus spaces must equal 5 spaces total
		int iMarginLength = sMargin.GetLength(); // 1.5.9ga Get length of margin
		int iSpacesNeeded = 4 - iMarginLength; // 1.5.9ga Calculate number of spaces needed
		for ( int i = 0; i < iSpacesNeeded; i++ ) // 1.5.9ga For each spacd needed, add a space
			sMarginTrailingSpaces += " ";
		Str8 sSFHeaderLine = "\\_sh v3.0  " + sMargin + sMarginTrailingSpaces + sTyp + "\n\n"; // 1.5.9ga Make database header line
		if ( !bExport ) // 1.5.9mm If export, don't write SF database header line
			fputs( sSFHeaderLine, pf ); // 1.5.9ga Write database header line
		fputs( sHeadInfo, pf ); // 1.5.9gc Write head info
		CIndexPtr pind = pindset()->pindRecordOwner(); // 1.5.9eb Get record owning index
		if ( pindExport ) // 1.5.9mn If export sorted by index of current window, use it
			pind = pindExport; // 1.5.9mn 
		const CRecLookEl* prel = pind->prelFirst();
		if ( bCurrentRecord ) // 1.5.9mu If export current record, select current rec
			prel = prelCur; // 1.5.9mu Select current rec
		for ( ; prel; prel = pind->prelNext(prel) ) // 1.5.9eb For each record, write the record
			{
			const CRecord* prec = prel->prec();
			CField* pfld = prec->pfldFirst();
			for ( ; pfld; pfld = prec->pfldNext(pfld) ) // 1.5.9ga For each field, write field
				{
				if ( !psubsetMarkersToExport || psubsetMarkersToExport->bIncludes( pfld->pmkr() ) ) // 1.5.9mq Export subset of fields
					{
					Str8 sMkr = pfld->pmkr()->sMarker(); // 1.5.9eb Get marker
					Str8 sMkrWrite = "\\" + sMkr; // 1.5.9ga Make marker form to write // 1.5.9mw 
					fputs( sMkrWrite, pf ); // 1.5.9ga Output marker
					if ( pfld->sContents().GetLength() > 0 ) // 1.5.9mw Write space after marker only if contents
						fputs( " ", pf ); // 1.5.9mw 
					if ( pfld->sContents().Find( "\n\\" ) ) // 1.5.9hd If fld contains backslash at begin line, output with space before it
						{
						Str8 sContentFixed = pfld->sContents();
						sContentFixed.Replace( "\n\\", "\n \\" ); // 1.5.9hd Replace to add space 
						fputs( sContentFixed, pf ); // 1.5.9hd Output field
						}
					else
						fputs( pfld->sContents(), pf ); // 1.5.9ga Output field
						fputs( "\n", pf ); // 1.5.9ga Output line break between fields
					}
				}
			if ( !prec->m_sXMLAttributes.IsEmpty() ) // 1.5.9ha If xml attribute for record, write it out
				{
				Str8 sXMLAttributeFld = "\\xmlattr" + prec->m_sXMLAttributes + "\n"; // 1.5.9ha Make fld // 1.5.9he Adjust for XML attributes always having leading space
				fputs( sXMLAttributeFld, pf ); // 1.5.9ha Write fld
				}
			if ( bCurrentRecord ) // 1.5.9mu If export current record, stop after record is output
				break; // 1.5.9mu 
			if ( pind->prelNext(prel) ) // 1.5.9mx Don't output extra line break at end of file
				fputs( "\n", pf ); // 1.5.9eb Output line break between records
			}
		fclose( pf ); // 1.5.9ga Close file
#ifdef OLDOSTREAM // 1.5.9ga Disable old iostream write
		std::ofstream ios(pszPath);
		if ( ios.fail() )
			return FALSE;
		SF_ostream sfs(ios, FALSE);
		long lnElements = m_pindset->pindRecordOwner()->lNumRecEls();
		ASSERT( pszProgressPath );
		CProgressIndicator prg(lnElements, NULL, pszProgressPath); // 1.4ad Eliminate resource messages sent to progress bar
		m_pindset->Write( sfs, prg ); // 1.4pcd Fix bug (1.4pcb) of failing to write file correctly
		BOOL bWriteFailed = ios.fail();     
		ios.close();  // Flushes any waiting output
		if ( bWriteFailed || ios.fail() )
			return FALSE;
#endif // OLDIOSTREAM
        }
    return TRUE;
}

BOOL CShwDoc::bExport(CIndex* pindCur, const CRecLookEl* prelCur)
{
    Str8 sMessage;
    if ( !ptyp()->bExport(pindCur, prelCur,  sUTF8( GetPathName() ), sMessage) ) // 1.4quh Upgrade GetPathName for Unicode build
        {
        MessageBeep(0);
		if ( sMessage != "" ) // 1.5.8n Give more messages for MDF problems
			AfxMessageBox(sMessage); // 1.5.4e Don't give empty message
		return FALSE;
        }
	return TRUE;
}  // CShwDoc::bExport

    
/*
 * CShwDoc::bNeedsRefresh - determines if this document needs to be refreshed
 * from the data on disk.  Returns TRUE if the document needs a refresh.
 */
BOOL CShwDoc::bNeedsRefresh()
{
    BOOL bRefreshMe = FALSE;
// 2000-01-10 TLB: If date is beyond 18 Jan 2038, we need to display a one-time warning.
    static BOOL s_bMessageShown = FALSE;
    
    CFileStatus tempFStat;
    if (!CFile::GetStatus(GetPathName(), tempFStat))
        {
        // couldn't even get status - say we need a refresh and 
        // let code for reading file handle it.
        bRefreshMe = TRUE;
        }
    else
        { 
        // we need a refresh if the disk date is not our last loaded in date.
        bRefreshMe = (tempFStat.m_mtime != m_timeLastLoaded);
        if ( tempFStat.m_mtime == -1 )
            {
            if ( !s_bMessageShown )
                {
	            AfxMessageBox(_("Please check your system date. Toolbox cannot handle a year higher than 2038."), MB_ICONINFORMATION | MB_OK);
                s_bMessageShown = TRUE;
                }
            bRefreshMe = TRUE; // Assume we need a refresh.
            }
        }
    
    return bRefreshMe;
}



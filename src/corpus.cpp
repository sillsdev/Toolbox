//File: Corpus.cpp  CCorpus and CCorpusSet classes   Josh Kelly  July 1997

// Change History
//    July 97 JK First draft, designed with input from Alan B
// 25 Sept 97 BW default Corpus files path to cwd instead of settings folder
//               chgd names: WordListFile > WordIndexFile, TextListFile > WordListOutput
// 24 Apr  98 BW removed use of m_sFilesPath and sFilesPath()

#include "stdafx.h"
#include "toolbox.h"
#include "Corpus.h"

#include "Tools.h"
#include "Shwnotes.h"
#include <CType.h>

#include "Corpus_d.h"
#include "lbo.h"
#include "Project.h"
#include "Not.h"
#include "Shw.h"
#include "Shwdoc.h"
#include "set_d.h"  // CSetDlg

static const char psz_CorpusSet[] = "CorpusSet";
static const char psz_Corpus[] = "Corpus";
static const char psz_cor[] = "cor";
static const char psz_mkrFilesList[] = "FilesList";
static const char psz_mkrWordMarkers[] = "WordMarkers";
static const char psz_mkrBookMarker[] = "BookMarker";
static const char psz_mkrChapMarker[] = "ChapMarker";
static const char psz_mkrVerseMarker[] = "VerseMarker";
static const char psz_mkrWordIndex[] = "WordIndex";
static const char psz_mkrWordListOutput[] = "WordListOutput";
static const char psz_mkrWordLstCorpus[] = "WordLstCorpus";
static const char psz_mkrNoWordLstCounts[] = "NoWordLstCounts";
static const char psz_mkrNoWordLstRefs[] = "NoWordLstRefs";
static const char psz_mkrWordLstLimitCount[] = "WordLstLimitCount";
static const char psz_mkrWordLstMaxCount[] = "WordLstMaxCount";
static const char psz_mkrConcordanceCorpus[] = "ConcordanceCorpus";
static const char psz_mkrConcordanceSearch[] = "ConcordanceSearch";
static const char psz_mkrConcordanceSearchType[] = "ConcordanceSearchType";
static const char psz_mkrConcordanceLimitCount[] = "ConcordanceLimitCount";
static const char psz_mkrConcordanceMaxCount[] = "ConcordanceMaxCount";
static const char psz_mkrNoConcordanceIgnoreCase[] = "ConcordanceMatchCase";
static const char psz_mkrConcordanceAlignMatchedStrings[] = "ConcordanceAlignMatchedStrings"; // 1.4vyx 
static const char psz_mkrConcordanceOutput[] = "ConcordanceOutput";
static const char psz_WordIndexExt[] = ".wi";

// --------------------------------------------------------------------------

CCorpus::CCorpus (const char* pszName, CCorpusSet* pcorsetMyOwner) : 
	CSetEl(pszName)
{
	m_pcorsetMyOwner = pcorsetMyOwner;
	ASSERT( m_pcorsetMyOwner );
	m_sWordListOutput = "wordlist.db"; //default name for word list output file
	m_plng = Shw_plngset()->plngDefault();
	m_pfilCorpusCurFile = NULL; // 1.4tb
	m_iFileNameStart = 0; // 1.4tb
}

CCorpus::CCorpus (const CCorpus& corSrc) : 
	CSetEl(corSrc.sKey()), 
	m_sFilesList(corSrc.m_sFilesList),
	m_sWordMarkers(corSrc.m_sWordMarkers),
	m_sBookMarker(corSrc.m_sBookMarker),
	m_sChapMarker(corSrc.m_sChapMarker),
	m_sVerseMarker(corSrc.m_sVerseMarker),
	m_sWordIndexPath(corSrc.m_sWordIndexPath),
	m_sWordListOutput(corSrc.m_sWordListOutput),
	m_pcorsetMyOwner(corSrc.m_pcorsetMyOwner),
	m_plng(corSrc.m_plng)
{
	m_pfilCorpusCurFile = NULL; // 1.4tb
	m_iFileNameStart = 0; // 1.4tb
}

CCorpus::~CCorpus()
{
}

CSet* CCorpus::psetMyOwner()
{
    return m_pcorsetMyOwner; 
}

#ifdef prjWritefstream // 1.6.4aa 
void CCorpus::WriteProperties(Object_ofstream& obs) const
#else
void CCorpus::WriteProperties(Object_ostream& obs) const
#endif
{
	obs.WriteBeginMarker(psz_Corpus, sKey());
	obs.WriteString(psz_mkrFilesList, Shw_pProject()->sStoredPathList(m_sFilesList));
	m_plng->WriteRef(obs, "");
	obs.WriteString(psz_mkrWordMarkers, m_sWordMarkers);
	obs.WriteString(psz_mkrBookMarker, m_sBookMarker);
	obs.WriteString(psz_mkrChapMarker, m_sChapMarker);
	obs.WriteString(psz_mkrVerseMarker, m_sVerseMarker);
	obs.WriteProjectPath(psz_mkrWordIndex, m_sWordIndexPath);
	obs.WriteProjectPath(psz_mkrWordListOutput, m_sWordListOutput);
	obs.WriteEndMarker(psz_Corpus);
}

BOOL CCorpus::bReadProperties(Object_istream& obs)
{
	Str8 sName;
	CLangEnc* plng = NULL;

	if (!obs.bReadBeginMarker(psz_Corpus, sName)) // If not a setup, fail
		return FALSE;

	SetKey(sName);
	while (!obs.bAtEnd()) 
		{
		if (obs.bReadString(psz_mkrFilesList, m_sFilesList))
            m_sFilesList = Shw_pProject()->sResolvedPathList(m_sFilesList, obs.bRelocateLegacyPaths());
		else if (obs.bReadString(psz_mkrWordMarkers, m_sWordMarkers))
			;
		else if (obs.bReadString(psz_mkrBookMarker, m_sBookMarker))
			;
		else if (obs.bReadString(psz_mkrChapMarker, m_sChapMarker))
			;
		else if (obs.bReadString(psz_mkrVerseMarker, m_sVerseMarker))
			;
		else if (obs.bReadProjectPath(psz_mkrWordIndex, m_sWordIndexPath))
			;
		else if (obs.bReadProjectPath(psz_mkrWordListOutput, m_sWordListOutput))
			;
		else if (Shw_plngset()->bReadLangEncRef(obs, "", plng))
			;
		else if (obs.bEnd(psz_Corpus)) // If end or begin something else, break
			break;
		} // while

	if ( !plng )
		plng = Shw_plngset()->plngDefault();

	m_plng = plng;
	return TRUE;
}

BOOL CCorpus::s_bReadProperties(Object_istream& obs,
                                CCorpusSet* pcorsetMyOwner, CCorpus** ppcor)
{
    Str8 sName;
    *ppcor = new CCorpus("",pcorsetMyOwner);
    if ((*ppcor)->bReadProperties(obs))
        return TRUE;
    else 
        {
        delete *ppcor;
        *ppcor = NULL;
        return FALSE;
        }
}

CCorpus* CCorpus::s_pcorDefaultMarkers(const char* pszName,
                                          CCorpusSet* pcorsetMyOwner)
{
    CCorpus *pcorNew = new CCorpus(pszName, pcorsetMyOwner);
    pcorNew->SetDefaultMarkers1();
    return pcorNew;
}

void CCorpus::SetDefaultMarkers1()
{
    m_sWordMarkers = "\\vt \\s"; // 5.98n Change default corpus markers to \vt, \s with \vref for refs
    m_sBookMarker = "\\vref";
    m_sChapMarker = "";
    m_sVerseMarker = "";
}

// return TRUE if corpus uses \id \c \v etc
BOOL CCorpus::bHasBibleMkrs()
{
	if ( (m_sBookMarker == "\\id") &&
	     (m_sChapMarker == "\\c")  &&
		 (m_sVerseMarker == "\\v") &&
		 (m_sWordMarkers.Find("\\v") != -1) )
		 return TRUE;
	else
		return FALSE;
}

void CCorpus::SetWordIndexPath (const char* pszWordIndexPath)
{
    m_sWordIndexPath = pszWordIndexPath;
}

void CCorpus::ClearWordIndexPath()
{
    m_sWordIndexPath.Empty();
}

// Sets word index file path & creates an empty file if needed
void CCorpus::PrepareWordIndexFile()
{
	// Delete old index file if it's not where we can use it now
	if (!m_sWordIndexPath.IsEmpty())
		{
		if ( !bEquivFilePath(sGetDirPath(m_sWordIndexPath), sCorpusFolder()) )
			{	// if old index is not in current corpus folder
            remove(m_sWordIndexPath);	// delete old index file
			m_sWordIndexPath.Empty();
			}
		}
	
	// If we have no path for existing index, make a new index
	if (m_sWordIndexPath.IsEmpty())
		{
		// set the new name & path
		Str8 sFileName = sUniqueFileName (sCorpusFolder(), sName(), psz_WordIndexExt);  // use corpus name
		SetWordIndexPath (sPath (sCorpusFolder(), sFileName, psz_WordIndexExt));
		// create a zero-size file so future corpora won't use this file
		CStdioFile csfWordIndex( swUTF16( sWordIndexFullPath() ),  // 1.4qvt Upgrade csfWordIndex or Unicode build
						   CFile::modeWrite | CFile::modeCreate);
		}

}

void CCorpus::SetWordListOutput (const char* pszWordListOutput)
{
    m_sWordListOutput = pszWordListOutput;
}

// Returns the default folder for data files related to the corpus.
//  Currently the folder of the first corpus file
Str8 CCorpus::sCorpusFolder() const
{
	if (!m_sFilesList.IsEmpty())
		{
		Str8 sFile;
		m_sFilesList.iExtractNext(0, sFile, '\n');	// get first file path
		return sGetDirPath(sFile);		// get the dir+path
		}
	else
		return "";
}

// 1.4tb Functions for new IME wordlist
BOOL CCorpus::bInitCorpusRead() // 1.4td Used in first and next line functions
	{
	if ( m_sWordMarkers.Right( 1 ) != " " ) // 1.4td If marker list doesn't end with space, add one
		m_sWordMarkers += " ";

	m_iBookMarkerPos = 0; // 1.4td Set up ref marker search string and start positions
	m_sRefMarkers = m_sBookMarker;
	m_sRefMarkers += " ";

	m_iChapMarkerPos = m_sRefMarkers.GetLength(); // 1.4tea Fix bug of number wrong
	m_sRefMarkers += m_sChapMarker;
	m_sRefMarkers += " ";

	m_iVerseMarkerPos = m_sRefMarkers.GetLength();
	m_sRefMarkers += m_sVerseMarker + " ";
	m_sRefMarkers += " ";

	m_sBookRef = ""; // 1.4tea Clear refs at start of word list
	m_sChapRef = "";
	m_sVerseRef = "";

	m_iFileNameStart = 0; // 1.4td Set to start at first file name
	m_pfilCorpusCurFile = NULL; // 1.4td Clear file pointer to be prepared for possible fail
	return bOpenNextFile(); // 1.4td Return true if a file opens
	}

BOOL CCorpus::bOpenNextFile() // 1.4td 
	{
	if ( m_pfilCorpusCurFile ) // 1.4td If there was a file open, close it
		fclose( m_pfilCorpusCurFile ); // 1.4td 
	Str8 sFileName;
	BOOL bSucc = FALSE;
    while ( m_iFileNameStart < m_sFilesList.GetLength() ) // 1.4tb Find the first file and open it
        {
        m_iFileNameStart = m_sFilesList.iExtractNext( m_iFileNameStart, sFileName, '\n' ); // 1.4tb Get first line from file name string
        if (!sFileName.IsEmpty())  // 1.4tb If line not empty, it is a file name
            {
				m_pfilCorpusCurFile = nullptr;
				errno_t err = fopen_s(&m_pfilCorpusCurFile, sFileName, "r"); // 1.4tb Open file
				if (err != 0 || m_pfilCorpusCurFile == nullptr) {
					// handle error
				}
				else // 1.4tb If open succeeded, use it, else try another
				{
					bSucc = TRUE;
					break;
				}
			}
		}
	return bSucc; // 1.4tb If no file was opened, return fail
	}

Str8 sPrevLine; // 1.4vye Previous line saved if not used

BOOL bGetLine( Str8& s, FILE* pfil ) // Get a whole line from a file 1.4tc
	{
	if ( sPrevLine != "" ) // 1.4vye If previous line saved, return it
		{
		s = sPrevLine; // 1.4vye 
		sPrevLine = ""; // 1.4vye 
		return TRUE; // 1.4vye 
		}
	s = ""; // 1.4tc Clear string
	int iGrow = 1000; // 1.4tc Amount to grow by if buffer not big enough
	int iSize = iGrow; // 1.4tc Starting size
	if ( !pfil ) // 1.4td If no file open, don't try to read
		return FALSE;
	char* pszReadStart = s.GetBuffer( iSize + 1 ); // 1.4tc Get initial buffer
	const char* pszReadSucc = fgets( pszReadStart, iSize, pfil ); // 1.4tc Read line, or part of line
	s.ReleaseBuffer(); // 1.4tc Release buffer to get it back to normal
	if ( !pszReadSucc ) // 1.4tc If first read hit end of file, return fail
		return FALSE;
	while ( s.Right( 1 ) != "\n" ) // 1.4tc While we didn't get a whole line, read more
		{
		iSize += iGrow; // 1.4tc Enlarge size
		int iLen = s.GetLength(); // 1.4tc Find current end
		char* pszReadStart = s.GetBuffer( iSize + 1 ); // 1.4tc Get start of buffer
		pszReadStart += iLen; // 1.4tc Calculate place to start appending read
		const char* pszReadSucc = fgets( pszReadStart, iGrow, pfil ); // 1.4tc Read line, or part of line
		s.ReleaseBuffer(); // 1.4tc Release buffer to get it back to normal
		if ( !pszReadSucc ) // 1.4tc If read failed, don't read further
			break;
		}
	if ( s.Right( 1 ) == "\n" ) // 1.4td If line includes nl, cut it off
		{
		int iLen = s.GetLength(); // 1.4td Find current end
		s.GetBuffer(); // 1.4td Get buffer as efficient way to cut off end
		s.ReleaseBuffer( iLen - 1 ); // 1.4td Release buffer one shorter to cut off end
		}
	return TRUE; // 1.4tc Return successful read
	}

BOOL CCorpus::bGetFileLine( Str8& s, BOOL bWholeField ) // 1.4td
	{
	BOOL bInGoodField = TRUE; // 1.4td If first line is continuation line, it is in corpus
	while ( bGetLine( s, m_pfilCorpusCurFile ) )
		{
		if ( !( s.Left( 1 ) == "\\" ) ) // 1.4td If continuation line, see if in good field
			{
			if ( bInGoodField ) // 1.4td If continuation line in good field, it is success
				return TRUE; // 1.4td Return success
			}
		else // Else, new field, see if is in the corpus
			{
			Str8 sMarker;
			int iDataStart = 0; // 1.4td Pos for search has to be var
			s.bNextWord( sMarker, iDataStart ); // 1.4td Get the the marker
			sMarker += " "; // 1.4td Add space after marker to make sure it can't match short
			while ( iDataStart < s.GetLength() && s.GetChar( iDataStart ) == ' ' ) // 1.4td Pass by spaces
				iDataStart++;
			int iRefMarker = m_sRefMarkers.Find( sMarker ); // 1.4td See if ref marker
			if ( iRefMarker >= 0 ) // 1.4td If marker is in the reference list, get the reference
				{
				Str8 sRefWord;
				s.bNextWord( sRefWord, iDataStart ); // 1.4td Get reference
				if ( iRefMarker == m_iVerseMarkerPos ) // 1.5.3h If verse ref, stop at space in case data is in same field
					{
					while ( iDataStart < s.GetLength() && s.GetChar( iDataStart ) == ' ' ) // 1.4td Pass by spaces after reference
						iDataStart++;
					}
				else // 1.5.3h Else (not verse ref), put whole field in ref
					{
					Str8 sRefWordExtra; // 1.5.8p Don't put whole book name field in ref if scripture ref
					while ( iDataStart < s.GetLength() )  // 1.5.3h Include whole field as ref
						{
						sRefWordExtra += s.Mid( iDataStart, 1 );  // 1.5.3h Copy each char into ref // 1.5.8p 
						iDataStart++;
						}
					if ( !( iRefMarker == m_iBookMarkerPos && !m_sVerseMarker.IsEmpty() ) ) // 1.5.8p If book marker and there is a verse marker, don't use whole field
						sRefWord += sRefWordExtra; // 1.5.8p If not book marker, or no verse marker, use whole field
					}
				if ( iRefMarker == m_iBookMarkerPos ) // 1.4td Save as appropriate reference
					{
					m_sBookRef = sRefWord;
					m_sChapRef = ""; // 1.4td On book ref, clear lower refs
					m_sVerseRef = "";
					}
				else if ( iRefMarker == m_iChapMarkerPos )
					{
					m_sChapRef = sRefWord; // 1.4td On chap ref, clear lower ref
					m_sVerseRef = "";
					}
				else if ( iRefMarker == m_iVerseMarkerPos )
					m_sVerseRef = sRefWord;
				}
			if ( m_sWordMarkers.Find( sMarker ) >= 0 ) // 1.4td If marker is in the corpus, remove it and return success
				{
				s = s.Mid( iDataStart ); // 1.4td Remove marker, also remove ref if this is both ref and data
				if ( bWholeField ) // 1.4vye If whole field, read rest of field
					{
					Str8 sLine; // 1.4vye 
					while ( bGetLine( sLine, m_pfilCorpusCurFile ) ) // 1.4vye Read lines till next field is found
						{
						if ( sLine.Left( 1 ) == "\\" ) // 1.4vye If start of new field, save it and stop reading
							{
							sPrevLine = sLine; // 1.4vye Save line for next call
							break; // 1.4vye 
							}
						s += " "; // 1.4vye Add space between lines
						s += sLine; // 1.4vye Append line to prev
						}
					s.Replace( "  ", " ", TRUE ); // 1.4wd Fix U bug of conc not matching phrase across line break
					}
				return TRUE; // 1.4td Return success
				}
			else // 1.4td Else, marker not in corpus, not in good field, skip rest of field
				bInGoodField = FALSE; // 1.4td Set to skip rest of field
			}
		}
	return FALSE; // 1.4td If no more good line found, return fail
	}

BOOL CCorpus::bGetNextLineOfCorpus( Str8& s, BOOL bWholeField ) // 1.4tb // 1.4vye Include whole field in concordance line
	{
	BOOL bSucc = bGetFileLine( s, bWholeField ); // 1.4tc Read a line // 1.4vye 
	while ( !bSucc ) // 1.4td If no more in file, try next file
		{
		BOOL bFile = bOpenNextFile(); // 1.4td Get next file
		if ( !bFile ) // 1.4td If no more files, return fail
			return FALSE;
		bSucc = bGetFileLine( s, bWholeField ); // 1.4tc Read a line
		}
	return bSucc; // 1.4tb
	}

Str8 CCorpus::sCurRef() // 1.4td Current reference
	{
	Str8 sFullRef = m_sBookRef; // 1.4td Start with book ref
	if ( sFullRef.IsEmpty() ) // 1.4td If no primary ref, show question marks
		sFullRef = "???";
	if ( !m_sChapMarker.IsEmpty() ) // 1.4td If there is a chap ref, build it
		{
		Str8 sChapRef = m_sChapRef;
		if ( m_sChapRef.IsEmpty() ) // 1.4td If no chap ref found, use 0
			sChapRef = "0";
		sFullRef += ".";
		sFullRef += sChapRef;
		}
	if ( !m_sVerseMarker.IsEmpty() ) // 1.4td If there is a verse ref, build it
		{
		Str8 sVerseRef = m_sVerseRef;
		if ( m_sVerseRef.IsEmpty() ) // 1.4td If no verse ref found, use 0
			sVerseRef = "0";
		sFullRef += ":";
		sFullRef += sVerseRef;
		}
	return sFullRef;
	}

// Confirms that all the files in the files list exist.  If bCheckForDirty
// is TRUE, also confirms that all the files in the files list are not open
// and dirty (modified).  The two notes, on exit, contain a list of all
// files that fail the test (NULL if no such files found).  If NO_INTERFACE
// is defined, bCheckForDirty is ignored and ppnotDirty is always NULL.
void CCorpus::ValidateFilesList (CNote **ppnotExist, CNote **ppnotDirty, 
                                 BOOL bCheckForDirty)
{
    int iFileStart = 0;
    Str8 sFile, sNoExistList, sDirtyList;
    CFileStatus filestat;

    CShwDoc *pdoc;

    while (iFileStart < m_sFilesList.GetLength()) //11-5-97 changed from do-while because empty m_sFilesList causes iExtractNext to fail assert
        {
        iFileStart = m_sFilesList.iExtractNext(iFileStart, sFile, '\n');
        if (!sFile.IsEmpty()) 
            {
            if (!CFile::GetStatus( swUTF16( sFile ), filestat))  // 1.4qvx
                {		// GetStatus returns zero if file does not exist
                sNoExistList += "\n";
                sNoExistList += sFile;
                }
			else		// while we have the file status, 
				{		//	check for illegal date beyond 2038+
				// 2000-01-10 BW: If date is beyond 18 Jan 2038, we need to display a one-time warning.
				static BOOL s_bMessageShown = FALSE;
				if ( filestat.m_mtime == -1 )
					if ( !s_bMessageShown )
						{
			            AfxMessageBox(_("Please check your system date. Toolbox cannot handle a year higher than 2038."), MB_ICONINFORMATION | MB_OK);
						s_bMessageShown = TRUE;
						}
				}
            pdoc = Shw_app().pdocGetDocByPath(sFile);
            if ((pdoc != NULL) && (pdoc->IsModified())) 
                {
                sDirtyList += "\n";
                sDirtyList += sFile;
                }
            } // if sFile has something
        }  // while

    if (sNoExistList.IsEmpty())
        *ppnotExist = NULL;
    else
        *ppnotExist = new CNote(_("The following file(s) do not exist or cannot be found:"), sNoExistList);
    
    if (sDirtyList.IsEmpty())
        *ppnotDirty = NULL;
    else
        *ppnotDirty = new CNote(_("The following file(s) are open and have been modified:"), sDirtyList);
}



BOOL CCorpus::bCopy(CSetEl** ppselNew)
{
    CCorpus* pcorNew = new CCorpus(*this);

    pcorNew->ChangeNameTo(m_pcorsetMyOwner->sUniqueName(sName()));
    pcorNew->ClearWordIndexPath();
    {
    CCorpusDlg dlg(pcorNew);
    if ( dlg.DoModal() == IDOK )
        {
        m_pcorsetMyOwner->Add(pcorNew);
        *ppselNew = pcorNew;
        return TRUE;
        }
    }
    delete pcorNew;
    return FALSE;
}

BOOL CCorpus::bModify()
{
    CCorpusDlg dlg(this);
    return (dlg.DoModal() == IDOK);
}

BOOL CCorpus::bModifyProperties(const char* pszName, const char* pszFilesList,
                                const char* pszWordMarkers, 
                                const char* pszBookMarker, 
                                const char* pszChapMarker, 
                                const char* pszVerseMarker, CLangEnc* plng, 
                                CNote** ppnot)
{
    // The word list file and text list file are omitted from this function
    // because they are not set from within CCorpusDlg.
    if (pszName[0] == '\x0') 
        {
        *ppnot = new CNote(_("Expecting a name"));
        return FALSE;
        }

    if ((pszName != sKey()) && !pcorsetMyOwner()->bValidNewName(pszName, ppnot))
        return FALSE;
    
    SetKey(pszName);
    m_sFilesList = pszFilesList;
    m_sWordMarkers = pszWordMarkers;
    m_sBookMarker = pszBookMarker;
    m_sChapMarker = pszChapMarker;
    m_sVerseMarker = pszVerseMarker;
    m_plng = plng;
    return TRUE;
}


#ifdef _DEBUG
void CCorpus::AssertValid() const
{
}
#endif

// --------------------------------------------------------------------------

CCorpusSet::CCorpusSet() : CSet()
{
    m_bWordLstCounts = TRUE;
    m_bWordLstRefs = TRUE;
    m_bWordLstLimitCount = FALSE; // 1.1sa Change default of new wordlist to unlimited references
    m_nWordLstMaxCount = 100; // 1.4vnj Change default limit on wordlist refs from 10 to 100
    m_nConcordanceSearchType = 0; // CCharSet::matchWhole; // 1.4yta 
    m_bConcordanceLimitCount = TRUE;
    m_nConcordanceMaxCount = 1000;
    m_bConcordanceIgnoreCase = TRUE;
	m_bConcordanceAlignMatchedStrings = FALSE; // 1.4vyx 
    m_sConcordanceOutput = "lookup.db";
}

CCorpusSet::~CCorpusSet()
{
}


void CCorpusSet::ViewElements(CCorpus** ppcorSelect)
{
    CSetListBox lboSet(this, (CSetEl**) ppcorSelect);
    Str8 sSettingsDirLabel = Shw_pProject()->sSettingsDirLabel();
    CSetDlg dlg(lboSet, _("Text Corpora"), sSettingsDirLabel, IDD_corset);
    (void) dlg.DoModal();
}

void CCorpusSet::ViewElements()
{
    CCorpus *pcor = NULL;
    ViewElements(&pcor);
}

BOOL CCorpusSet::bAdd(CSetEl** ppselNew)
{
    Str8 sName = sUniqueName("added");
    CCorpus* pcorNew = CCorpus::s_pcorDefaultMarkers(sName, this);
    ASSERT( pcorNew );
    
    {
    CCorpusDlg dlg(pcorNew);
    
    /* lpszTemplateName = MAKEINTRESOURCE(432);
    HINSTANCE hInst = AfxFindResourceHandle(lpszTemplateName, RT_DIALOG);
    HRSRC hResource = ::FindResource(hInst, lpszTemplateName, RT_DIALOG);
    DWORD dwOuch = GetLastError();
    Str8 sResult;
    sResult.Format("%lu", dwOuch);
    AfxMessageBox(sResult);
    //HANDLE hDialogTemplate = LoadResource(hInst, hResource);*/
    
    if ( dlg.DoModal() == IDOK )
        {
        Add(pcorNew);
        *ppselNew = pcorNew;
        return TRUE;
        }
    }
    delete pcorNew;
    return FALSE;
}

void CCorpusSet::Delete(CSetEl** ppsel)
{
    if ( !((CCorpus*) *ppsel)->m_sWordIndexPath.IsEmpty() )
        {
        Str8 sFullFile = ((CCorpus*) *ppsel)->sWordIndexFullPath();
        if (!sFullFile.IsEmpty())
            remove(sFullFile);
        }
    CSet::Delete(ppsel);
}


#ifdef prjWritefstream // 1.6.4aa 
void CCorpusSet::WriteProperties(Object_ofstream& obs) const
#else
void CCorpusSet::WriteProperties(Object_ostream& obs) const
#endif
{
    CCorpus *pcor = (CCorpus*) pselFirst();
    obs.WriteBeginMarker(psz_CorpusSet);
    obs.WriteString(psz_mkrWordLstCorpus, m_sWordLstCorpus);
    obs.WriteBool(psz_mkrNoWordLstCounts, !m_bWordLstCounts);
    obs.WriteBool(psz_mkrNoWordLstRefs, !m_bWordLstRefs);
    obs.WriteBool(psz_mkrWordLstLimitCount, m_bWordLstLimitCount);
    obs.WriteInteger(psz_mkrWordLstMaxCount, m_nWordLstMaxCount);
    obs.WriteString(psz_mkrConcordanceCorpus, m_sConcordanceCorpus);
    obs.WriteString(psz_mkrConcordanceSearch, m_sConcordanceSearch);
    obs.WriteInteger(psz_mkrConcordanceSearchType, m_nConcordanceSearchType);
    obs.WriteBool(psz_mkrConcordanceLimitCount, m_bConcordanceLimitCount);
    obs.WriteInteger(psz_mkrConcordanceMaxCount, m_nConcordanceMaxCount);
    obs.WriteBool(psz_mkrNoConcordanceIgnoreCase, !m_bConcordanceIgnoreCase);
    obs.WriteBool(psz_mkrConcordanceAlignMatchedStrings, m_bConcordanceAlignMatchedStrings); // 1.4vyx 
    obs.WriteProjectPath(psz_mkrConcordanceOutput, m_sConcordanceOutput);
    while (pcor != NULL) 
        {
        pcor->WriteProperties(obs); // 1.6.4aa 
        pcor = (CCorpus*) pselNext(pcor);
        }
    obs.WriteEndMarker(psz_CorpusSet);
}

BOOL CCorpusSet::bReadProperties(Object_istream& obs)
{
    if ( !obs.bReadBeginMarker( psz_CorpusSet ) )
        return FALSE;

    while (!obs.bAtEnd()) 
        {
        CCorpus* pcor = NULL;
        if (CCorpus::s_bReadProperties(obs, this, &pcor))  
            {
            // If we read a corpus, add it to list
            if (pcor) // If a valid corpus, add it to the list
                Add(pcor);
            }
        else if (obs.bReadString(psz_mkrWordLstCorpus, m_sWordLstCorpus))
            ;
        else if (obs.bReadBool(psz_mkrNoWordLstCounts, m_bWordLstCounts))
            m_bWordLstCounts = !m_bWordLstCounts;
        else if (obs.bReadBool(psz_mkrNoWordLstRefs, m_bWordLstRefs))
            m_bWordLstRefs = !m_bWordLstRefs;
        else if (obs.bReadBool(psz_mkrWordLstLimitCount, m_bWordLstLimitCount))
            ;
        else if (obs.bReadInteger(psz_mkrWordLstMaxCount, m_nWordLstMaxCount))
            ;
        else if (obs.bReadString(psz_mkrConcordanceCorpus, m_sConcordanceCorpus))
            ;
        else if (obs.bReadString(psz_mkrConcordanceSearch, m_sConcordanceSearch))
            ;
        else if (obs.bReadInteger(psz_mkrConcordanceSearchType, m_nConcordanceSearchType))
            ;
        else if (obs.bReadBool(psz_mkrConcordanceLimitCount, m_bConcordanceLimitCount))
            ;
        else if (obs.bReadInteger(psz_mkrConcordanceMaxCount, m_nConcordanceMaxCount))
            ;
        else if (obs.bReadBool(psz_mkrNoConcordanceIgnoreCase, m_bConcordanceIgnoreCase))
            m_bConcordanceIgnoreCase = !m_bConcordanceIgnoreCase;
        else if (obs.bReadBool(psz_mkrConcordanceAlignMatchedStrings, m_bConcordanceAlignMatchedStrings)) // 1.4vyx 
            ;
        else if (obs.bReadProjectPath(psz_mkrConcordanceOutput, m_sConcordanceOutput))
            ;
        else if (obs.bEnd(psz_CorpusSet)) // If end of list, stop
            break;
        } // while
    return TRUE;
}


CCorpus* CCorpusSet::pcorWordLstCorpus() const
{
    return (CCorpus*) pselSearch(m_sWordLstCorpus);
}

CCorpus* CCorpusSet::pcorConcordanceCorpus() const
{
    return (CCorpus*) pselSearch(m_sConcordanceCorpus);
}

void CCorpusSet::SetWordLstCorpus(CCorpus* pcor)
{
    m_sWordLstCorpus = pcor->sName();
}

void CCorpusSet::SetConcordanceCorpus(CCorpus* pcor)
{
    m_sConcordanceCorpus = pcor->sName();
}


// --------------------------------------------------------------------------

  
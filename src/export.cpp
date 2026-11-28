// export.cpp

#include "stdafx.h"
#include "toolbox.h"
#include "export.h"
#include "export_d.h"  // CExportProcessDlg
#include "mdf_d.h"  // CMDFDlg
#include "obstream.h"  // Object_ostream, Object_istream
#include "pgs.h"  // RtfPageSetup
#include "typ.h"  // CDatabaseTypeSet
#include <fstream>  // ofstream
#include "rtf.h"  // RTF_ostream
#include "xml.h"  // XML_ostream
#include "mdf.h"  // CExportProcessMDF
#include "tools.h"  // sGetFileName, SetFileTypeRTF
#include "progress.h"
#include "resource.h"
#include "mainfrm.h"
#include "shwview.h"
#include "shw.h"  // Shw_papp()
#include "cck.h"  // CFieldConsistencyChecker
#include "shwdoc.h" // 1.1bg Open export file
#include "doclist.h"
#include <io.h>  // _tell
#include <errno.h>
#include <cderr.h>  // FNERR_INVALIDFILENAME
#include "alan.h" // 1.5.8h 
#include "Change_ostream.h" // For change table stream classes

extern BOOL bDebugKaren; // 1.4bk
extern Str8 g_sVersion; // 1.5.9eh 

CExportProcess::CExportProcess(const char* pszName,
        CExportProcessSet* pexpsetMyOwner) :
    CSetEl(pszName),
    m_pexpsetMyOwner(pexpsetMyOwner),
    m_subsetMarkersToExport(pexpsetMyOwner->pmkrset()),
    m_bCheckConsistency(FALSE),
    m_bOverwriteOutputFile(FALSE),
    m_bAddAlphDiv(FALSE),
    m_bChainedTo(FALSE),
    m_pdocExported(NULL)
{
    ASSERT( m_pexpsetMyOwner );
}

CExportProcess::CExportProcess(const char* pszName,
        CExportProcessSet* pexpsetMyOwner,
        const CMarkerSubSet& subsetMarkersToExport,
        BOOL bCheckConsistency) :
    CSetEl(pszName),
    m_pexpsetMyOwner(pexpsetMyOwner),
    m_subsetMarkersToExport(subsetMarkersToExport),
    m_bCheckConsistency(bCheckConsistency),
    m_bOverwriteOutputFile(FALSE), // 1.2mg Fix bug of losing new export process if used before closing dialog box
    m_bAddAlphDiv(FALSE), // 1.2mg
    m_bChainedTo(FALSE), // 1.2mg
    m_pdocExported(NULL) // 1.2mg
{
    ASSERT( m_pexpsetMyOwner );
}

CExportProcess::~CExportProcess()
{
}

CSet* CExportProcess::psetMyOwner()
{
    return m_pexpsetMyOwner;
}

BOOL CExportProcess::bCheckConsistency(CIndex* pindCur)
{
    CShwView* pview = Shw_papp()->pviewActive();
    CFieldConsistencyChecker cck(pview);
    return cck.bCheckConsistencyBeforeExporting(pindCur);
}

BOOL CExportProcess::bAutoOpen(const Str8& sPath, Str8& sMessage)
{
#ifndef _MAC  // If this fails to compile on Macintosh, please define it out
    HWND hwnd = Shw_papp()->m_pMainWnd->m_hWnd;
    // 1998-03-17 MRP: Replacing SW_SHOW by SW_SHOWMAXIMIZED causes
    // an already-running instance of Microsoft Word to come to the front.
    // However, it doesn't force Word to be maximized, if it wasn't already.
    HINSTANCE hinst = ShellExecute(hwnd, NULL,  swUTF16( sPath ), NULL,  swUTF16( "" ), SW_SHOWMAXIMIZED); // 1.4qxw Upgrade ShellExecute for Unicode build
    int iReturn = (int)hinst;
    if ( iReturn <= 32 )
       {
	   sMessage = sMessage + _("Failed to automatically open file:") + " " + sPath; // 1.5.0fd
       return FALSE;
       }
#endif  // not _MAC

    return TRUE;
}


void CExportProcess::FilterUpdated(CFilterUpdate& fup)
{
}

BOOL CExportProcess::s_bExportedFileDlg(const char* pszDefaultExt, BOOL bOverwritePrompt, const char* pszFileFilter, int* piFilterIndex, Str8& sPath)
{
    // 1996-05-24 MRP: In this case our normal approach of deriving
    // a class from CFileDialg doesn't accomplish encapsulation
    // of the implementation details, because some of the arguments
    // for the base class constructor, such as the file filter,
    // wouldn't be available early enough in the initialization process.
    const char* pszFileName = NULL;
    if ( !sPath.IsEmpty() )
        pszFileName = sPath;
    DWORD dwFlags = OFN_HIDEREADONLY |  // defaults
        OFN_PATHMUSTEXIST |  // Dialog requires that the path be valid
        OFN_NOREADONLYRETURN;  // Dialog requires that the file
            // not be read-only, nor be in a write-protected directory
    // 1999-01-22 MRP: Prompt for SF and RTF export, but not for MDF HTML.
    if ( bOverwritePrompt )
        dwFlags |= OFN_OVERWRITEPROMPT;
    Str8 sFileFilter = pszFileFilter; // 1.4ar Bring exported file dialog filters in from resources
    
	if ( !Shw_papp()->bUseCrashFixFileEditDlg() ) // 1.5.8h 
		{
		CFileDialog dlg(FALSE, swUTF16( pszDefaultExt ), swUTF16( pszFileName ), dwFlags, swUTF16( sFileFilter ) ); // File Save As-type common dialog // If NULL, no default extension // 1.4qrd // 1.5.8h 

		ASSERT( piFilterIndex );    
		dlg.m_ofn.nFilterIndex = *piFilterIndex;
		dlg.m_ofn.lpstrTitle = _T(" "); // 1.4qzhs Fix bad letters in file browse box title

		dlg.SetHelpID(IDD_ExportedFile);  // Help topic only works for Win3.1x
		if ( iDlgDoModalSetReg( &dlg ) != IDOK )
			return FALSE;
		sPath =  sUTF8( dlg.GetPathName() ); // 1.4qra
		*piFilterIndex = (int)dlg.m_ofn.nFilterIndex;
		}
	else // 1.5.8h 
		if ( !bCrashFixBrowseForFileName(sPath, sPath) ) // 1.5.8h 
			return FALSE; // 1.5.8h 
	if ( !bAllASCIIComplain( sPath ) ) // 1.4vze 
		return FALSE; // 1.4vze 
    return TRUE;
}


BOOL CExportProcess::s_bBrowseForFile(const char* pszTitle, const char* pszDefaultExt, const char* pszFilter, const Str8& sDirPath, Str8& sFilePath, UINT idHelp) // 1.4ap Bring export process titles in from resource file // 1.4ar Bring export process filters in from resources
{
    long lFlags = (OFN_HIDEREADONLY | OFN_NOCHANGEDIR |
        OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST);
    Str8 sFilter = pszFilter; // 1.4as Bring in filters from resource file
	Str8 sPath; // 1.5.8h 
	if ( !Shw_papp()->bUseCrashFixFileEditDlg() ) // 1.5.8h 
		{
		CFileDialog dlgOpen(TRUE, swUTF16( pszDefaultExt), NULL, lFlags, swUTF16( sFilter ) ); // 1.4qrd // 1.5.8h 
		dlgOpen.m_ofn.lpstrTitle = _T(" "); // 1.4qzhs Fix bad letters in file browse box title
		const char chDirSeparator = '\\';
		sPath = sDirPath;
		int len = sPath.GetLength();
		if ( len > 1 && sPath.GetChar(len - 1) == chDirSeparator )
			sPath = sPath.Left(len - 1);
		dlgOpen.m_ofn.lpstrInitialDir =  swUTF16( sPath ); // 1.4qra
		if ( idHelp != 0 )
			dlgOpen.SetHelpID(idHelp);  // Help topic only works for Win3.1x
		if ( iDlgDoModalSetReg( &dlgOpen ) != IDOK )
			return FALSE;
		sPath =  sUTF8( dlgOpen.GetPathName() ); // 1.4qra
		sFilePath =  sUTF8( dlgOpen.GetPathName() ); // 1.4qra
		}
	else // 1.5.8h 
		if ( !bCrashFixBrowseForFileName(sPath, sPath) ) // 1.5.8h 
			return FALSE; // 1.5.8h 
	if ( !bAllASCIIComplain( sPath ) ) // 1.4vze 
		return FALSE; // 1.4vze 
    return TRUE;
}

void AddSettingsPathIfNone( Str8& sPath ) // 1.4bf Add settings dir path if no dir path 
	{
	if ( sPath == "" ) // 1.4bf Don't add dir path if no file name
		return;
	Str8 sDirPath = sGetDirPath( sPath );
	if (sDirPath.IsEmpty())
		{
		sDirPath = Shw_pProject()->sSettingsDirPath();
		sPath = sDirPath + sPath;
		}
	}

// --------------------------------------------------------------------------

CExportProcessRTF::CExportProcessRTF(const char* pszName,
        CExportProcessSet* pexpsetMyOwner) :
    CExportProcess(pszName, pexpsetMyOwner)
{
    m_bPriKeyFieldFirst = FALSE;  // 1998-03-06 MRP
	m_bAddAlphDiv = FALSE; // 1.1va Fix uninitialized export process vars // 1.1cm Add alphabetic dividers
    m_bMarkerFont = FALSE;
    // 1999-02-01 MRP: Default is tab-aligned in version 4.1 (and later)
    m_bInterlinearTabAligned = TRUE;
    m_twxInterlinearSpacing = s_twxDefaultInterlinearSpacing;
    m_ppgs = new RtfPageSetup();
}

CExportProcessRTF::CExportProcessRTF(const char* pszName,
        CExportProcessSet* pexpsetMyOwner,
        const Str8& sPathCCT, const Str8& sPathDOT,
        BOOL bMarkerFont,
        const CMarkerSubSet& subsetMarkersToExport,
        BOOL bCheckConsistency,
        RtfPageSetup* ppgs,
        BOOL bInterlinearTabAligned, RtfTwips twxInterlinearSpacing) :
    CExportProcess(pszName, pexpsetMyOwner, subsetMarkersToExport,
        bCheckConsistency),
    m_sPathCCT(sPathCCT),
    m_sPathDOT(sPathDOT)
{
    m_bPriKeyFieldFirst = FALSE;  // 1998-03-06 MRP
	m_bAddAlphDiv = FALSE; // 1.1va Fix uninitialized export process vars // 1.1cm Add alphabetic dividers
    m_bMarkerFont = bMarkerFont;
    m_ppgs = ( ppgs != NULL ? new RtfPageSetup(*ppgs) : new RtfPageSetup() );

    // 1999-01-30 MRP
    m_bInterlinearTabAligned = bInterlinearTabAligned;
    m_twxInterlinearSpacing = twxInterlinearSpacing;
    SetValidInterlinearSpacing();
}

CExportProcessRTF::~CExportProcessRTF()
{
    delete m_ppgs;
}


static const char* s_pszDefaultNameRTF = "Rich Text Format";

static const char* psz_expRTF = "expRTF";
static const char* psz_PriKeyFieldFirst = "PriKeyFieldFirst";
static const char* psz_cct = "cctFile";
static const char* psz_dot = "dotFile";
static const char* psz_MarkerFont = "MarkerFont";
static const char* psz_rtf = "exportedFile";
static const char* psz_mrfset = "mkrsubsetIncluded";
static const char* psz_CheckConsistency = "CheckConsistency";
static const char* psz_OverwriteOutputFile = "OverwriteOutputFile";
static const char* psz_AddAlphDiv = "AddAlphDiv";

// 1999-01-30 MRP: Tab-alignment was an experimental feature in 4.0,
// but in 4.1 (and later) it's considered the default for interlinear text.
static const char* psz_InterlinearSpaceAligned = "InterlinearSpaceAligned";
static const char* psz_InterlinearSpacing = "InterlinearSpacing";

#ifdef typWritefstream // 1.6.4ac
void CExportProcessRTF::WriteProperties(Object_ofstream& obs) const
#else
void CExportProcessRTF::WriteProperties(Object_ostream& obs) const
#endif
{
    obs.WriteBeginMarker(psz_expRTF, sName());

    obs.WriteBool(psz_PriKeyFieldFirst, m_bPriKeyFieldFirst);  // 1998-03-06 MRP
    obs.WriteProjectPath(psz_cct, m_sPathCCT);
    obs.WriteProjectPath(psz_dot, m_sPathDOT);
    obs.WriteProjectPath(psz_rtf, m_sPathRTF);
    obs.WriteBool(psz_MarkerFont, m_bMarkerFont);
    if ( m_pexpsetMyOwner->bInterlinear() )
        {
        obs.WriteBool(psz_InterlinearSpaceAligned, !m_bInterlinearTabAligned);
        if ( m_bInterlinearTabAligned && m_twxInterlinearSpacing != 0 )
            obs.WriteInteger(psz_InterlinearSpacing, m_twxInterlinearSpacing);
        }
    obs.WriteBool(psz_CheckConsistency, m_bCheckConsistency);
    obs.WriteBool(psz_OverwriteOutputFile, m_bOverwriteOutputFile);
    obs.WriteBool(psz_AddAlphDiv, m_bAddAlphDiv);
    m_subsetMarkersToExport.WriteProperties(obs);
    m_ppgs->WriteProperties(obs);
	obs.WriteUnrecognizedSettingsInfo( m_sUnrecognizedSettingsInfo ); // 1.0cp Write unrecognized settings info
    
    obs.WriteEndMarker(psz_expRTF);
}

BOOL CExportProcessRTF::s_bReadProperties(Object_istream& obs,
        CExportProcessSet* pexpsetMyOwner, CExportProcess** ppexp)
{
    ASSERT( pexpsetMyOwner );
    ASSERT( ppexp );
    ASSERT( !*ppexp );

    Str8 sName;
    if ( !obs.bReadBeginMarker(psz_expRTF, sName) )
        return FALSE;

    Str8 sValidName;
    if ( !pexpsetMyOwner->bValidNameRead(sName, s_pszDefaultNameRTF, sValidName) )
        sName = sValidName;

    CExportProcessRTF* pexp = new CExportProcessRTF(sName, pexpsetMyOwner);
    pexp->ReadProperties(obs);    
    *ppexp = pexp;
        
    return TRUE;
}

void CExportProcessRTF::ReadProperties(Object_istream& obs)
{
    BOOL bInterlinearSpaceAligned = FALSE;  // 1999-01-30 MRP
    RtfPageSetup* ppgs = NULL;

    while ( !obs.bAtEnd() )
        {
        if ( obs.bReadBool(psz_PriKeyFieldFirst, m_bPriKeyFieldFirst) )
            ;
        else if ( obs.bReadProjectPath(psz_cct, m_sPathCCT) )
            ;
        else if ( obs.bReadProjectPath(psz_dot, m_sPathDOT) )
            ;
        else if ( obs.bReadProjectPath(psz_rtf, m_sPathRTF) )
            ;
        else if ( obs.bReadBool(psz_MarkerFont, m_bMarkerFont) )
            ;
        else if ( obs.bReadBool(psz_InterlinearSpaceAligned, bInterlinearSpaceAligned) )
            m_bInterlinearTabAligned = FALSE;  // 1999-01-30 MRP
        else if ( obs.bReadInteger(psz_InterlinearSpacing, m_twxInterlinearSpacing) )
            ;
        else if ( obs.bReadBool(psz_CheckConsistency, m_bCheckConsistency) )
            ;
        else if ( obs.bReadBool(psz_OverwriteOutputFile, m_bOverwriteOutputFile) )
            ;
        else if ( obs.bReadBool(psz_AddAlphDiv, m_bAddAlphDiv) )
            ;
        else if ( m_subsetMarkersToExport.bReadProperties(obs) )
            ;
        else if ( RtfPageSetup::s_bReadProperties(obs, FALSE, &ppgs) )
            {
            if ( ppgs ) // 2000-06-15 TLB
                {
                delete m_ppgs;
                m_ppgs = ppgs;
                ppgs = NULL;
                }
            }
		else if ( obs.bUnrecognizedSettingsInfo( psz_expRTF, m_sUnrecognizedSettingsInfo  ) ) // 1.0cp Store any unrecognized settings info so as not to lose it
			;
        else if ( obs.bEnd( psz_expRTF ) )
            break;
        }
	AddSettingsPathIfNone( m_sPathCCT );
	AddSettingsPathIfNone( m_sPathDOT );
	AddSettingsPathIfNone( m_sPathRTF );
    SetValidInterlinearSpacing();  // 1999-01-30 MRP
}


const RtfTwips CExportProcessRTF::s_twxDefaultInterlinearSpacing = 120;

void CExportProcessRTF::SetValidInterlinearSpacing()
{
    // 1999-01-30 MRP: Set Spacing to the same valid range, no matter whether
    // the alignment is currently tab or space aligned. That way,
    // if the user changes the alignment option in the RTF Export
    // dialog box, the initial value shown in the edit box will be valid.
    // Valid range of values: 24 per inch to 6 per inch.
    // Default value: 12 per inch
    if ( m_twxInterlinearSpacing < 20 || 2000 < m_twxInterlinearSpacing ) // 6.0y Increase range of possible values
        m_twxInterlinearSpacing = s_twxDefaultInterlinearSpacing;
}


static const char s_pszTypeRTF[] = "RTF";

const char* CExportProcessRTF::pszType() const
{
    return s_pszTypeRTF;
}

BOOL CExportProcessRTF::bMDF(CFilter** ppfil) const
{
    return FALSE;
}

BOOL CExportProcessRTF::bCanAutoOpen() const
{
    return TRUE;
}

static void s_Export(Field_ostream& sfs, CDatabaseType* ptyp, BOOL bCurrentRecord, const CRecLookEl* prelCur, // 1.4qpb
        BOOL bCurrentWindow, CIndex* pindCur, const CMarkerSubSet& subsetMarkersToExport,
        BOOL bPriKeyFieldFirst, const Str8& sPath, 
		BOOL bAddAlphDiv = FALSE )
{
    if ( bCurrentRecord )
        {
        ASSERT( prelCur );
        // 1996-05-25 MRP: Might need to get owning index's element
        pindCur->WriteElement(sfs, ptyp, prelCur, subsetMarkersToExport,
            FALSE, FALSE);
        }
    else
        {
        ASSERT( pindCur );
        CIndex* pind = pindCur;
        if ( !bCurrentWindow )
            pind = ((CIndex*)pindCur)->pindsetMyOwner()->pindRecordOwner();
        CProgressIndicator prg(pind->lNumRecEls(), NULL, sPath); // 1.4ad Eliminate resource messages sent to progress bar
        pind->WriteElements(sfs, ptyp, prg, subsetMarkersToExport,
            bPriKeyFieldFirst, FALSE, bAddAlphDiv);
        }
}

static const char* s_pszExtTMP = ".tmp";

BOOL CExportProcessRTF::bWriteChanges(const Str8& sPathTMP,
        BOOL bCurrentRecord, const CRecLookEl* prelCur,
        BOOL bCurrentWindow, CIndex* pindCur,
        Str8& sMessage)
{
    std::ofstream iosTMP(sPathTMP);
    ASSERT( !m_sPathCCT.IsEmpty() );
    {
    Change_ostream iosChange(m_sPathCCT, iosTMP);
    if ( iosChange.fail() )
        {
        int iRemoved = remove(m_sPathRTF);
        iRemoved = remove(sPathTMP);
        return FALSE;
        }
            
    SF_ostream sfs(iosChange, FALSE);
	sfs.WriteString( pindCur->pindsetMyOwner()->pszHeadInfo() ); // 1.4pcc Export head extra in RTF if change table
    s_Export(sfs, m_pexpsetMyOwner->ptypMyOwner(), bCurrentRecord, prelCur, bCurrentWindow, pindCur, // 1.4qpb
        m_subsetMarkersToExport, m_bPriKeyFieldFirst, sPathTMP, 
		m_bAddAlphDiv ); // 1.1ed Fix bug of failing to output alphabetic dividers if CC table is used
    }
    
    BOOL bWriteFailed = iosTMP.fail();
    iosTMP.close();  // Flush any remaining output
    if ( bWriteFailed || iosTMP.fail() )
        {
		sMessage = sMessage + _("Failed to write file:") + " " + sPathTMP; // 1.5.0fd
        int iRemoved = remove(m_sPathRTF);
        iRemoved = remove(sPathTMP);
        return FALSE;
        }

    return TRUE;
}

BOOL CExportProcessRTF::bReadChanges(const Str8& sPathTMP, RTF_ostream& sfs,
        CMarkerSet* pmkrset, CMarker* pmkrRecord,
        Str8& sMessage)
{
    std::ifstream iosTMP(sPathTMP);
    if ( iosTMP.fail() )
        {
		sMessage = sMessage + _("Cannot open file:") + " " + sPathTMP; // 1.5.0fd 
        int iRemoved = remove(m_sPathRTF);
        iRemoved = remove(sPathTMP);
        return FALSE;
        }

    ASSERT( pmkrset );
    ASSERT( pmkrRecord );
    SF_istream sfsTMP(iosTMP, pmkrset, pmkrRecord, FALSE);

    LONG lSizTMP = lFileSize(sPathTMP);
    CProgressIndicator prg(lSizTMP, NULL, m_sPathRTF); // 1.4ad Eliminate resource messages sent to progress bar
	BOOL bFirstAlphDiv = TRUE; // 1.2dt
    while ( !sfsTMP.bAtEnd() )
        {
        CField* pfld = NULL;
        if ( !sfsTMP.bReadField(&pfld) )
            return FALSE;
		if ( pfld->pmkr()->sName() == "zzzAlphDiv" ) // 1.2dt Make rts alph div section break output work with CCT
			{
			sfs.WriteAlphDiv( pfld->sContents(), pfld->pmkr()->pmkrsetMyOwner()->ptyp(), bFirstAlphDiv ); // 1.2dt
			bFirstAlphDiv = FALSE; // 1.2dt
			}
		else // 1.2dt
			sfs.WriteField(pfld, FALSE);
        delete pfld;
		LONG lPos = static_cast<LONG>(iosTMP.tellg());

        (void) prg.bUpdateProgress(lPos);
        }
	sfs.WriteMultipleColumnEnd(); // 1.2du For rtf export double column, even up columns on last page

    // 1997-12-10 MRP: Win and Mac require that the stream
    // be closed before removing the file, although Win16 doesn't.
    iosTMP.close();
    int iRemoved = remove(sPathTMP);
    
    return TRUE;
}

BOOL CExportProcessRTF::bExport(CIndex* pindCur, const CRecLookEl* prelCur,
        const Str8& sPathDatabase, Str8& sMessage)
{
    ASSERT( m_pexpsetMyOwner );
    BOOL bCurrentRecord = m_pexpsetMyOwner->bCurrentRecord();
    BOOL bCurrentWindow = m_pexpsetMyOwner->bCurrentWindow();
    ASSERT( pindCur );
    ASSERT( !bCurrentRecord || prelCur );

    // 1998-03-10 MRP: The user chooses whether or not to see the properties
    // and be able to modify them (except the name) when exporting.
    if ( m_pexpsetMyOwner->bShowProperties() && !m_bChainedTo ) // 1.1cd Don't show properties box if the process was chained to
        {
        CExportProcessRTFDlg dlg(this, FALSE);
        if ( dlg.DoModal() != IDOK )
            return TRUE;
        }

    // 1998-03-06 MRP: Let the user choose how to order the exported fields
    // when the primary sorting marker isn't the record marker.
    const CMarker* pmkrPriKey = pindCur->pmkrPriKey();
    const CMarker* pmkrRecord = pindCur->pindsetMyOwner()->
        pindRecordOwner()->pmkrPriKey();
    if ( bCurrentWindow && pmkrPriKey != pmkrRecord )
        {
        CExportCurrentWindowDlg dlg(pmkrPriKey, &m_bPriKeyFieldFirst);
        if ( dlg.DoModal() != IDOK )
            return TRUE;
        }

    BOOL bCCT = !m_sPathCCT.IsEmpty();
    if ( bCCT && !bFileExists(m_sPathCCT) )
        {
		sMessage = sMessage + _("Cannot find file:") + " " + m_sPathCCT; // 1.5.0fd 
        return FALSE;
        }

    if ( !m_sPathDOT.IsEmpty() && !bFileExists(m_sPathDOT) )
        {
		sMessage = sMessage + _("Cannot find file:") + " " + m_sPathDOT; // 1.5.0fd 
        return FALSE;
        }

    if ( m_bCheckConsistency && !bCheckConsistency(pindCur) )
        return TRUE;

    if ( !m_bChainedTo )
	    (void) Shw_app().bSaveAllFiles(); // 1.1ga Save all before export

    Str8 sFileFilter = _("Rich Text Format"); // 1.5.0fz 
	sFileFilter = sFileFilter + " (*.rtf)|*.rtf||";
    int iFilterIndex = 1;
	Str8 sFolderPath = sGetDirPath( m_sPathRTF ); // 1.2da Fix bug of RTF export not using given file name if it doesn't exist
	if ( sFolderPath != "" // 1.2pf Fix bug of losing export file name if no path given
			&& bFileReadOnly( sFolderPath ) ) // 1.1ua If export saved file path not valid, don't try to use it
		m_sPathRTF = ""; // 1.1ua Clear saved path so it will ask user for path

	BOOL bAskForFileName = !( m_bOverwriteOutputFile && m_sPathRTF != "" ); // 1.3dd Make not overwrite always ask for file name
	if ( bAskForFileName )
		{
		if ( !CExportProcess::s_bExportedFileDlg("rtf", TRUE, sFileFilter, &iFilterIndex, m_sPathRTF) )
			return TRUE;
		}
    
    {
	UnWriteProtect( m_sPathRTF ); // 1.2fv Turn off write protect before writing // 1.4qzhg
    std::ofstream iosExport(m_sPathRTF);
    // 1997-11-15 MRP: In MDF's export we don't return from failures here

    Str8 sPathTMP;
    if ( bCCT )
        {
        Str8 sDirPath = sGetDirPath(m_sPathRTF);
        Str8 sFileNameTMP = sUniqueFileName(sDirPath, "rtf", s_pszExtTMP);
        sPathTMP = sPath(sDirPath, sFileNameTMP, s_pszExtTMP);
        if ( !bWriteChanges(sPathTMP, bCurrentRecord, prelCur,
                bCurrentWindow, pindCur, sMessage) )
            return FALSE;
        }

    CMarkerSet* pmkrset = m_pexpsetMyOwner->pmkrset();
    CMarker* pmkrRecord = pmkrset->pmkrRecord();
    Str8 sFileNameExt = sGetFileName(m_sPathRTF);

    {
    Str8 sPathDOT;
    if ( !m_bMarkerFont )
        sPathDOT = m_sPathDOT;
    RTF_ostream sfs(iosExport, pmkrset,
        m_pexpsetMyOwner->bAutoUpdateStyles(), m_bMarkerFont,
        m_bInterlinearTabAligned, m_twxInterlinearSpacing,  // 1998-07-11
        "", "", sFileNameExt, TRUE, sPathDOT, m_ppgs);

    if ( bCCT )
        {
        if ( !bReadChanges(sPathTMP, sfs, pmkrset, pmkrRecord, sMessage) )
            return FALSE;
        }
    else
			// 1.4pcc Note, we don't export head extra in RTF if no change table, doing so would produce invalid RTF code
        s_Export(sfs, m_pexpsetMyOwner->ptypMyOwner(), bCurrentRecord, prelCur, bCurrentWindow, pindCur, // 1.4qpb
            m_subsetMarkersToExport, m_bPriKeyFieldFirst, m_sPathRTF, m_bAddAlphDiv );
    }

#ifdef _MAC
    SetFileTypeRTF(m_sPathRTF); // set file type and creator
#endif

    BOOL bWriteFailed = iosExport.fail();
    iosExport.close();  // Flush any remaining output
    if ( bWriteFailed || iosExport.fail() )
        {
		sMessage = sMessage + _("Cannot write file:") + " " + m_sPathRTF; // 1.5.0fd 
        int iRemoved = remove(m_sPathRTF);
        return FALSE;
        }
    }

    if ( m_pexpsetMyOwner->bAutoOpen() )
        {
        if ( !bAutoOpen(m_sPathRTF, sMessage) )
            return FALSE;
        }

	if ( m_bChainedTo && m_pdocExported ) // 1.1ce If this process was chained to, unload the file
		{
		Str8 sPathFile =  sUTF8( m_pdocExported->GetPathName() ); // 1.4bj Delete temporary files after chain is finished // 1.4qxs
		m_pdocExported->SetModifiedFlag( FALSE ); // 1.1ce Clear modified to prevent write before close
		m_pdocExported->OnCloseDocument(); // 1.1ce Close file
		m_pdocExported = NULL; // 1.1cj Clear file so it can't be referred to again
		if ( !bDebugKaren ) // 1.4bk Allow Ctrl+Alt+Shift+K to keep chained export files
			int i = remove(sPathFile); // 1.4bm Delete chained files after RTF export is finished
		}

    return TRUE;
}

BOOL CExportProcessRTF::bCopy(CSetEl** ppselNew)
{
    CExportProcessRTFDlg dlg(this, (CExportProcessRTF**)ppselNew);
    return (dlg.DoModal() == IDOK);
}

BOOL CExportProcessRTF::bModify()
{
    CExportProcessRTFDlg dlg(this, TRUE);
    return (dlg.DoModal() == IDOK);
}


// --------------------------------------------------------------------------

CExportProcessSF::CExportProcessSF(const char* pszName,
        CExportProcessSet* pexpsetMyOwner) :
    CExportProcess(pszName, pexpsetMyOwner)
{
    m_bPriKeyFieldFirst = FALSE;  // 1998-03-06 MRP
	m_bOpenExportedFile = FALSE; // 1.1bd Automatically open exported file
	m_bAddAlphDiv = FALSE; // 1.1cm Add alphabetic dividers
}

CExportProcessSF::CExportProcessSF(const char* pszName,
        CExportProcessSet* pexpsetMyOwner,
        const Str8& sPathCCT,
        const CMarkerSubSet& subsetMarkersToExport,
        BOOL bCheckConsistency) :
    CExportProcess(pszName, pexpsetMyOwner, subsetMarkersToExport,
        bCheckConsistency),
    m_sPathCCT(sPathCCT)
{
    m_bPriKeyFieldFirst = FALSE;  // 1998-03-06 MRP
	m_bOpenExportedFile = FALSE; // 1.1bd Automatically open exported file
	m_bAddAlphDiv = FALSE; // 1.1cm Add alphabetic dividers
}

CExportProcessSF::~CExportProcessSF()
{
}


static const char* s_pszDefaultNameSF = "Standard Format";

static const char* psz_expSF = "expSF";
static const char* psz_sf = "exportedFile";
static const char* psz_OpenExportedFile = "OpenExportedFile";
static const char* psz_ChainProcess = "ChainProcess";

#ifdef typWritefstream // 1.6.4ac
void CExportProcessSF::WriteProperties(Object_ofstream& obs) const
#else
void CExportProcessSF::WriteProperties(Object_ostream& obs) const
#endif
{
    obs.WriteBeginMarker(psz_expSF, sName());

    obs.WriteBool(psz_PriKeyFieldFirst, m_bPriKeyFieldFirst);  // 1998-03-06 MRP
    obs.WriteProjectPath(psz_cct, m_sPathCCT);
    obs.WriteProjectPath(psz_sf, m_sPathSF);
    obs.WriteString(psz_ChainProcess, m_sChainProcess);
    obs.WriteBool(psz_CheckConsistency, m_bCheckConsistency);
    obs.WriteBool(psz_OverwriteOutputFile, m_bOverwriteOutputFile);
    obs.WriteBool(psz_AddAlphDiv, m_bAddAlphDiv);
    obs.WriteBool(psz_OpenExportedFile, m_bOpenExportedFile);
    m_subsetMarkersToExport.WriteProperties(obs);
	obs.WriteUnrecognizedSettingsInfo( m_sUnrecognizedSettingsInfo ); // 1.0cp Write unrecognized settings info
    
    obs.WriteEndMarker(psz_expSF);
}

BOOL CExportProcessSF::s_bReadProperties(Object_istream& obs,
        CExportProcessSet* pexpsetMyOwner, CExportProcess** ppexp)
{
    ASSERT( pexpsetMyOwner );
    ASSERT( ppexp );
    ASSERT( !*ppexp );

    Str8 sName;
    if ( !obs.bReadBeginMarker(psz_expSF, sName) )
        return FALSE;

    Str8 sValidName;
    if ( !pexpsetMyOwner->bValidNameRead(sName, s_pszDefaultNameSF, sValidName) )
        sName = sValidName;

    CExportProcessSF* pexp = new CExportProcessSF(sName, pexpsetMyOwner);
    pexp->ReadProperties(obs);    
    *ppexp = pexp;
        
    return TRUE;
}

void CExportProcessSF::ReadProperties(Object_istream& obs)
{
    while ( !obs.bAtEnd() )
        {
        if ( obs.bReadBool(psz_PriKeyFieldFirst, m_bPriKeyFieldFirst) )
            ;
        else if ( obs.bReadProjectPath(psz_cct, m_sPathCCT) )
            ;
        else if ( obs.bReadProjectPath(psz_sf, m_sPathSF) )
            ;
        else if ( obs.bReadString(psz_ChainProcess, m_sChainProcess) )
            ;
        else if ( obs.bReadBool(psz_CheckConsistency, m_bCheckConsistency) )
            ;
        else if ( obs.bReadBool(psz_OverwriteOutputFile, m_bOverwriteOutputFile) )
            ;
        else if ( obs.bReadBool(psz_AddAlphDiv, m_bAddAlphDiv) ) // 1.1cm Disable alphabetic dividers in SF process
            ;
        else if ( obs.bReadBool(psz_OpenExportedFile, m_bOpenExportedFile) )
            ;
        else if ( m_subsetMarkersToExport.bReadProperties(obs) )
            ;
		else if ( obs.bUnrecognizedSettingsInfo( psz_expSF, m_sUnrecognizedSettingsInfo  ) ) // 1.0cp Store any unrecognized settings info so as not to lose it
			;
        else if ( obs.bEnd( psz_expSF ) )
            break;
        }
	AddSettingsPathIfNone( m_sPathCCT );
	AddSettingsPathIfNone( m_sPathSF );
}


static const char s_pszTypeSF[] = "SF";

const char* CExportProcessSF::pszType() const
{
    return s_pszTypeSF;
}

BOOL CExportProcessSF::bMDF(CFilter** ppfil) const
{
    return FALSE;
}

BOOL CExportProcessSF::bCanAutoOpen() const
{
    return FALSE;
}


extern BOOL bApplyCCT( Str8 sCCT, Str8 sPath ); // 1.5.9mt Apply CC table to file, write file to same name and place

BOOL CExportProcessSF::bExport(CIndex* pindCur, const CRecLookEl* prelCur,
        const Str8& sPathDatabase, Str8& sMessage)
{
    BOOL bCurrentRecord = m_pexpsetMyOwner->bCurrentRecord();
    BOOL bCurrentWindow = m_pexpsetMyOwner->bCurrentWindow();
    ASSERT( pindCur );
    ASSERT( !bCurrentRecord || prelCur );

    // 1998-03-10 MRP: The user chooses whether or not to see the properties
    // and be able to modify them (except the name) when exporting.
    if ( m_pexpsetMyOwner->bShowProperties() && !m_bChainedTo ) // 1.1cd Don't show properties box if the process was chained to
        {
        CExportProcessSFDlg dlg(this, FALSE);
        if ( dlg.DoModal() != IDOK )
            return TRUE;
        }

    // 1998-03-06 MRP: Let the user choose how to order the exported fields
    // when the primary sorting marker isn't the record marker.
    const CMarker* pmkrPriKey = pindCur->pmkrPriKey();
    const CMarker* pmkrRecord = pindCur->pindsetMyOwner()->
        pindRecordOwner()->pmkrPriKey();
//    if ( bCurrentWindow && pmkrPriKey != pmkrRecord ) // 1.5.9mn  // 1.6.1dr Try enable dialog to ask if move field on export // 1.6.1dr Test enable move field on export, not implemented after 1.6.0
	if ( FALSE ) // 1.5.9mn Disable primary key field first dialog box // 1.6.1dr Disable ask because move field not implemented in new code
        {
        CExportCurrentWindowDlg dlg(pmkrPriKey, &m_bPriKeyFieldFirst);
        if ( dlg.DoModal() != IDOK )
            return TRUE;
        }

    if ( m_bCheckConsistency && !bCheckConsistency(pindCur) )
        return TRUE;

    if ( !m_sPathCCT.IsEmpty() && !bFileExists(m_sPathCCT) )
        {
		sMessage = sMessage + _("Cannot find file:") + " " + m_sPathCCT; // 1.5.0fd 
        return FALSE;
        }

	CDatabaseType* ptyp = m_pexpsetMyOwner->ptypMyOwner(); // 1.4qmj Prevent loop in export of CCTable file
    if ( !m_bChainedTo )
		{
		if ( !ptyp->bCCTable() ) // 1.4qmj Prevent loop in export of CCTable // 1.4qzff
			(void) Shw_app().bSaveAllFiles(); // 1.1ga Save all before export
		}
	if ( ptyp->bCCTable() ) // 1.4qmm // 1.4qzff
		{
		int iDot = sPathDatabase.ReverseFind('.'); // 1.4qmn Fix problem if no file type of CCTable file
		if ( iDot == -1 ) // 1.4qmn
			iDot = sPathDatabase.GetLength(); // 1.4qmn
		m_sPathSF = sPathDatabase.Mid( 0, iDot ); // 1.4qmm Export CCTable to same file name as source
		m_sPathSF = m_sPathSF + ".cct"; // 1.4qmm
		}

	if ( sGetFileName( sPathDatabase ) == sGetFileName( m_sPathSF ) ) // 1.1bk Don't allow export to same name // 1.2bj Simplify get file name by using tools function
		{
        sMessage = _("Can't export to same name as source file");
		return FALSE;
		}

	Str8 sFileFilter = _("All files"); // 1.5.0fz 
	sFileFilter += " (*.*)|*.*||"; // 1.5.0fz 
    int iFilterIndex = 1;
	Str8 sFolderPath = sGetDirPath( m_sPathSF ); // 1.2bj Fix bug of SF export not using given file name if it doesn't exist
	if ( sFolderPath != "" // 1.2pf Fix bug of losing export file name if no path given
			&& bFileReadOnly( sFolderPath ) ) // 1.1ua If export saved file path not valid, don't try to use it // 1.2bj
		{
		if ( !ptyp->bCCTable() ) // 1.4qmm // 1.4qzff
			m_sPathSF = ""; // 1.1ua Clear saved path so it will ask user for path
		}

	BOOL bAskForFileName = !( m_bOverwriteOutputFile && m_sPathSF != "" ); // 1.3dd Make not overwrite always ask for file name
	if ( bAskForFileName )
		{
		if ( !CExportProcess::s_bExportedFileDlg(NULL, TRUE, sFileFilter, &iFilterIndex, m_sPathSF) )
			return TRUE;
		}

	if ( m_bOverwriteOutputFile ) // 1.1kv Move this up to fix bug of not reloading output file in all cases // 1.1bk If output file to overwrite is loaded, unload before export
		{
		CDocList doclst; // list of currently open docs
	    for ( CShwDoc* pdoc = doclst.pdocFirst(); pdoc; pdoc = doclst.pdocNext() ) // 1.1bk If output file to overwrite is loaded, unload before export
			{
		    Str8 sFullPath =  sUTF8( pdoc->GetPathName() ); // 1.1kv Fix bug of not always reloading file on export // 1.4qxs
			Str8 sFileName = sFullPath.Mid(sFullPath.ReverseFind('\\') + 1); // Find file name only, to fix bug of not recognizing it as the same file
			if ( sFileName == m_sPathSF ) // 1.1bk If file is loaded, unload
				{
				pdoc->SetModifiedFlag( FALSE ); // 1.1bk Clear modified to prevent write before close
				pdoc->OnCloseDocument(); // 1.1bk Close file so load of exported file will not find it
				}
			}
		}

	UnWriteProtect( m_sPathSF ); // 1.2fv Turn off write protect before writing // 1.4qzhg

	// Find CShwDoc for this file by searching for doc that owns this index set // 1.5.9mm 
	CDocList doclst; // list of currently open docs // 1.5.9mm 
	CShwDoc* pdoc = doclst.pdocFirst(); // 1.5.9mm 
	for ( ; pdoc; pdoc = doclst.pdocNext() ) // 1.5.9mm 
		{
		if ( pdoc->pindset() == pindCur->pindsetMyOwner() ) // 1.5.9mm If found the doc, break
			break; // 1.5.9mm 
		}
	CIndex* pindExport = NULL; // 1.5.9mn 
	if ( bCurrentWindow ) // 1.5.9mn 
		pindExport = pindCur; // 1.5.9mn 
	pdoc->bWrite( m_sPathSF, TRUE, pindExport, &m_subsetMarkersToExport, bCurrentRecord, prelCur ); // 1.5.9mm Write exported file
    if ( !m_sPathCCT.IsEmpty() ) // 1.5.9mt If CCT, apply CCT to exported file
		{
		if ( !bApplyCCT( m_sPathCCT, m_sPathSF ) ) // 1.5.9mt Apply CC table to file
			{
			sMessage = _("CC failed"); // 1.5.9mt If CC failed, give message and return fail
			return FALSE;
			}
		}


#ifdef OLDIOSTREAMEXPORT
    std::ofstream iosExport(m_sPathSF);
    // 1997-11-15 MRP: In MDF's export we don't return from failures here

    if ( !m_sPathCCT.IsEmpty() )
        {
        Change_ostream* piosChange = new Change_ostream(m_sPathCCT, iosExport); // 1.2fh
        if ( piosChange->fail() )
            {
            int iRemoved = remove(m_sPathSF);
            return FALSE;
            }
        
		SF_ostream* psfs = new SF_ostream(*piosChange, FALSE); // 1.2fh Try heap instead of stack for CC stream
		psfs->WriteString( pindCur->pindsetMyOwner()->pszHeadInfo() ); // 1.4pcc
        s_Export(*psfs, m_pexpsetMyOwner->ptypMyOwner(), bCurrentRecord, prelCur, bCurrentWindow, pindCur, // 1.4qpb
            m_subsetMarkersToExport, m_bPriKeyFieldFirst, m_sPathSF, m_bAddAlphDiv);
		delete psfs; // 1.2fh
		delete piosChange; // 1.2fh
        }
    else  // Not making changes
        {
        SF_ostream sfs(iosExport, FALSE);
		sfs.WriteString( pindCur->pindsetMyOwner()->pszHeadInfo() ); // 1.4pcc
        s_Export(sfs, m_pexpsetMyOwner->ptypMyOwner(), bCurrentRecord, prelCur, bCurrentWindow, pindCur, // 1.4qpb
            m_subsetMarkersToExport, m_bPriKeyFieldFirst, m_sPathSF, m_bAddAlphDiv);
        }

    BOOL bWriteFailed = iosExport.fail();
    iosExport.close();  // Flush any remaining output
    if ( bWriteFailed || iosExport.fail() )
        {
		sMessage = sMessage + _("Cannot write file:") + " " + m_sPathSF; // 1.5.0fd 
        int iRemoved = remove(m_sPathSF);
        return FALSE;
        }
#endif // OLDIOSTREAMEXPORT
	if ( m_bOpenExportedFile )
		{
		Shw_pProject()->m_bExportProcessAutoOpening = TRUE; // 1.1bh Assign db type on auto open exported file
		CExportProcessSet* pset = (CExportProcessSet*)psetMyOwner();
		Shw_pProject()->m_ptypExportProcessAutoOpening = pset->ptypMyOwner(); // 1.1bh Assign db type on auto open exported file
        CShwDoc *pdoc = Shw_app().pdocGetDocByPath( m_sPathSF ); // 1.4qmd If auto load exported file already open, refresh it
		if ( pdoc ) // 1.4qmd If open, refresh
			{
			Shw_app().bRefreshADoc(pdoc); // 1.4qmd Refresh
			pdoc = Shw_app().pdocGetDocByPath( m_sPathSF );
			POSITION pos = pdoc->GetFirstViewPosition(); // 1.4qmp Find a view of this document
			if (pos != NULL)
				{
				Shw_app().SetZ(); // 1.4qmp Fix bug (1.4qmd) of refreshed exported file in focus, but not in front
				CShwView* pView = (CShwView*)pdoc->GetNextView(pos); // 1.4qmp Find a view of this document
				Shw_app().pviewTop()->ShowInitialTopState(); // 1.4qmp
//				pView->SetZ( 100 ); // 1.4qmp Fix bug (1.4qmd) of not bringing refreshed export file to front, this tried to bring it to the front, but it didn't work
//				pView->ShowInitialTopState(); // 1.4qmp Didn't bring to front
				}
			}
		else
			pdoc = (CShwDoc*)AfxGetApp()->OpenDocumentFile( swUTF16( m_sPathSF )); // 1.4qmd If file not already open, open it // 1.1bg Open exported file // 1.4qxe
		Shw_pProject()->m_bExportProcessAutoOpening = FALSE; // 1.1bh Assign db type on auto open exported file
		if ( m_sChainProcess != "" && pdoc ) // 1.1br Chain to next process // 1.1cf Fix bug of crash on chain if output same name as current
			{
			Str8 sHeadInfo = pdoc->pindset()->sGetHeadInfo(); // 1.4pe Add \_chain option in head info
			int iChain = sHeadInfo.Find(  "\\_chain " ); // 1.4pe See if \_chain is in head info
			if ( iChain >= 0 ) // 1.4pe If chain found, use it instead of the standard one of this export process
				{
				Str8 sChain = sHeadInfo.Mid( iChain +  8 ); // 1.4pe Get string starting at chain name
				int iChainEnd = sChain.Find( "\n" ); // 1.4pe Find end of chain name
				if ( iChainEnd >= 0 ) // 1.4pe
					{
					sChain = sChain.Left( iChainEnd ); 
					m_sChainProcess = sChain; // 1.4pe Substitute head info chain for standard one of this process
					Str8 sBeforeChain = sHeadInfo.Left( iChain ); // 1.4pg Delete chain from head info after it is used
					Str8 sAfterChain = sHeadInfo.Mid( iChain + iChainEnd + 9 ); // 1.4pg Get part after chain field
					sHeadInfo = sBeforeChain + sAfterChain; // 1.4pg
					pdoc->pindset()->SetHeadInfo( sHeadInfo ); // 1.4pg
					}
				}
			CDatabaseType* ptyp = pdoc->ptyp(); // 1.1bu Chain to next process
			CExportProcessSet* pexpset = ptyp->pexpset(); // 1.1bu Chain to next process
		    CExportProcess* pexp = pexpset->pexpSearch( m_sChainProcess ); // 1.1bu Chain to next process
			if ( pexp )
				{
				CShwView* pview = Shw_pmainframe()->pviewGetActive(); // 1.1bu Find view of newly opened doc
				CIndex* pind = pview->pind(); // 1.1bu Use view to get pindCur and prelCur
				CNumberedRecElPtr prelCur = pview->prelCur();
				pexp->m_bChainedTo = TRUE; // 1.1cd Notify process that it is being chained to
				pexp->m_pdocExported = pdoc; // 1.1ce Notify process of the doc being exported so it can unload it after the chain is complete
				sMessage = sMessage + _("Cannot write file:") + " " + m_sPathSF; // 1.5.0fd 
				if ( !pexp->bExport( pind, prelCur, m_sPathSF, sMessage ) ) // 1.1bu Chain to export process
					{
					MessageBeep(0);
					AfxMessageBox(sMessage);
					pexp->m_pdocExported = NULL; // 1.1cj Clear file so it can't be referred to again		
					}
				}
			}
		}

	if ( m_bChainedTo && m_pdocExported ) // 1.1ce If this process was chained to, unload the file // 1.4bj Delete the file also
		{
		Str8 sPathFile =  sUTF8( m_pdocExported->GetPathName() ); // 1.4bj Delete temporary files after chain is finished // 1.4qxs
		m_pdocExported->SetModifiedFlag( FALSE ); // 1.1ce Clear modified to prevent write before close
		m_pdocExported->OnCloseDocument(); // 1.1ce Close file
		m_pdocExported = NULL; // 1.1cj Clear file so it can't be referred to again
		if ( !bDebugKaren ) // 1.4bk Allow Ctrl+Alt+Shift+K to keep chained export files
			int i = remove(sPathFile); // 1.4bj Delete temporary files after chain is finished
		}

	return TRUE;
}

BOOL CExportProcessSF::bCopy(CSetEl** ppselNew)
{
    CExportProcessSFDlg dlg(this, (CExportProcessSF**)ppselNew);
    return (dlg.DoModal() == IDOK);
}

BOOL CExportProcessSF::bModify()
{
    CExportProcessSFDlg dlg(this, TRUE);
    return (dlg.DoModal() == IDOK);
}


// --------------------------------------------------------------------------

CExportProcessXML::CExportProcessXML(const char* pszName,
        CExportProcessSet* pexpsetMyOwner) :
    CExportProcess(pszName, pexpsetMyOwner)
{
    m_bPriKeyFieldFirst = FALSE;  // 1998-03-06 MRP
	m_bAddAlphDiv = FALSE; // 1.1va Fix uninitialized export process vars // 1.1cm Add alphabetic dividers
    m_bCheckConsistency = TRUE;  // 1999-06-14 MRP: Override base class default
    m_sRootNode = "database";
    m_bUTF8 = pexpsetMyOwner->ptypMyOwner()->bUnicode();
    const Str8& sTypeFileName = sGetFileName(m_pexpsetMyOwner->ptypMyOwner()->ptrxMyProxy()->sFileName(), FALSE);
    m_sDTDName = sTypeFileName;
}

CExportProcessXML::CExportProcessXML(const char* pszName,
        CExportProcessSet* pexpsetMyOwner,
        const Str8& sPathCCT, const Str8& sPathStylesheet,
        const CMarkerSubSet& subsetMarkersToExport,
        BOOL bCheckConsistency) :
    CExportProcess(pszName, pexpsetMyOwner, subsetMarkersToExport,
        bCheckConsistency),
    m_sPathCCT(sPathCCT),
    m_sPathStylesheet(sPathStylesheet)
{
    m_bPriKeyFieldFirst = FALSE;  // 1998-03-06 MRP
	m_bAddAlphDiv = FALSE; // 1.1va Fix uninitialized export process vars // 1.1cm Add alphabetic dividers
    m_sRootNode = "database";
    m_bUTF8 = pexpsetMyOwner->ptypMyOwner()->bUnicode();
    const Str8& sTypeFileName = sGetFileName(m_pexpsetMyOwner->ptypMyOwner()->ptrxMyProxy()->sFileName(), FALSE);
    m_sDTDName = sTypeFileName;
}

CExportProcessXML::~CExportProcessXML()
{
}


static const char* s_pszDefaultNameXML = "Extensible Markup Language";

static const char* psz_expXML = "expXML";
static const char* psz_DontCheckConsistency = "DontCheckConsistency";
static const char* psz_UTF8 = "UTF8";
static const char* psz_stylesheetFile = "stylesheetFile";
static const char* psz_exportedFile = "exportedFile";

#ifdef typWritefstream // 1.6.4ac
void CExportProcessXML::WriteProperties(Object_ofstream& obs) const
#else
void CExportProcessXML::WriteProperties(Object_ostream& obs) const
#endif
{
    obs.WriteBeginMarker(psz_expXML, sName());

    obs.WriteBool(psz_PriKeyFieldFirst, m_bPriKeyFieldFirst);  // 1998-03-06 MRP
    obs.WriteBool(psz_UTF8, m_bUTF8);
    obs.WriteProjectPath(psz_cct, m_sPathCCT);
    obs.WriteProjectPath(psz_stylesheetFile, m_sPathStylesheet);
    obs.WriteProjectPath(psz_exportedFile, m_sPathXML);
    obs.WriteBool(psz_DontCheckConsistency, !m_bCheckConsistency);
    obs.WriteBool(psz_OverwriteOutputFile, m_bOverwriteOutputFile);
    obs.WriteBool(psz_AddAlphDiv, m_bAddAlphDiv);
    m_subsetMarkersToExport.WriteProperties(obs);
	obs.WriteUnrecognizedSettingsInfo( m_sUnrecognizedSettingsInfo ); // 1.0cp Write unrecognized settings info
    
    obs.WriteEndMarker(psz_expXML);
}

BOOL CExportProcessXML::s_bReadProperties(Object_istream& obs,
        CExportProcessSet* pexpsetMyOwner, CExportProcess** ppexp)
{
    ASSERT( pexpsetMyOwner );
    ASSERT( ppexp );
    ASSERT( !*ppexp );

    Str8 sName;
    if ( !obs.bReadBeginMarker(psz_expXML, sName) )
        return FALSE;

    Str8 sValidName;
    if ( !pexpsetMyOwner->bValidNameRead(sName, s_pszDefaultNameXML, sValidName) )
        sName = sValidName;

    CExportProcessXML* pexp = new CExportProcessXML(sName, pexpsetMyOwner);
    pexp->ReadProperties(obs);    
    *ppexp = pexp;
        
    return TRUE;
}

void CExportProcessXML::ReadProperties(Object_istream& obs)
{
    while ( !obs.bAtEnd() )
        {
        BOOL b = FALSE;
        if ( obs.bReadBool(psz_PriKeyFieldFirst, m_bPriKeyFieldFirst) )
            ;
        else if ( obs.bReadBool(psz_UTF8, m_bUTF8) )
            ;
        else if ( obs.bReadProjectPath(psz_cct, m_sPathCCT) )
            ;
        else if ( obs.bReadProjectPath(psz_stylesheetFile, m_sPathStylesheet) )
            ;
        else if ( obs.bReadProjectPath(psz_exportedFile, m_sPathXML) )
            ;
        else if ( obs.bReadBool(psz_DontCheckConsistency, b) )
            m_bCheckConsistency = FALSE;
        else if ( obs.bReadBool(psz_OverwriteOutputFile, m_bOverwriteOutputFile) )
            ;
        else if ( obs.bReadBool(psz_AddAlphDiv, m_bAddAlphDiv) )
            ;
        else if ( m_subsetMarkersToExport.bReadProperties(obs) )
            ;
		else if ( obs.bUnrecognizedSettingsInfo( psz_expXML, m_sUnrecognizedSettingsInfo  ) ) // 1.0cp Store any unrecognized settings info so as not to lose it
			;
        else if ( obs.bEnd( psz_expXML ) )
            break;
        }
	AddSettingsPathIfNone( m_sPathCCT );
	AddSettingsPathIfNone( m_sPathStylesheet );
	AddSettingsPathIfNone( m_sPathXML );
}


static const char s_pszTypeXML[] = "XML";

const char* CExportProcessXML::pszType() const
{
    return s_pszTypeXML;
}

BOOL CExportProcessXML::bMDF(CFilter** ppfil) const
{
    return FALSE;
}

BOOL CExportProcessXML::bCanAutoOpen() const
{
    return FALSE;
}


BOOL CExportProcessXML::bExport(CIndex* pindCur, const CRecLookEl* prelCur,
        const Str8& sPathDatabase, Str8& sMessage)
{
    BOOL bCurrentRecord = m_pexpsetMyOwner->bCurrentRecord();
    BOOL bCurrentWindow = m_pexpsetMyOwner->bCurrentWindow();
    ASSERT( pindCur );
    ASSERT( !bCurrentRecord || prelCur );

    // 1998-03-10 MRP: The user chooses whether or not to see the properties
    // and be able to modify them (except the name) when exporting.
    if ( m_pexpsetMyOwner->bShowProperties() && !m_bChainedTo ) // 1.1cd Don't show properties box if the process was chained to
        {
        CExportProcessXMLDlg dlg(this, FALSE);
        if ( dlg.DoModal() != IDOK )
            return TRUE;
        }

    // 1998-03-06 MRP: Let the user choose how to order the exported fields
    // when the primary sorting marker isn't the record marker.
    const CMarker* pmkrPriKey = pindCur->pmkrPriKey();
    const CMarker* pmkrRecord = pindCur->pindsetMyOwner()->
        pindRecordOwner()->pmkrPriKey();
    if ( bCurrentWindow && pmkrPriKey != pmkrRecord )
        {
        CExportCurrentWindowDlg dlg(pmkrPriKey, &m_bPriKeyFieldFirst);
        if ( dlg.DoModal() != IDOK )
            return TRUE;
        }

    if ( !m_sPathCCT.IsEmpty() && !bFileExists(m_sPathCCT) )
        {
		sMessage = sMessage + _("Cannot find file:") + " " + m_sPathCCT; // 1.5.0fd 
        return FALSE;
        }

    if ( m_bCheckConsistency && !bCheckConsistency(pindCur) )
        return TRUE;

    if ( !m_bChainedTo )
	    (void) Shw_app().bSaveAllFiles(); // 1.1ga Save all before export

	Str8 sFileFilter = _("All files"); // 1.5.0fz 
	sFileFilter += " (*.*)|*.*||"; // 1.5.0fz 
    int iFilterIndex = 1;
	Str8 sFolderPath = sGetDirPath( m_sPathXML ); // 1.2da Fix bug of XML export not using given file name if it doesn't exist
	if ( sFolderPath != "" // 1.2pf Fix bug of losing export file name if no path given
			&& bFileReadOnly( sFolderPath ) ) // 1.1ua If export saved file path not valid, don't try to use it
		m_sPathXML = ""; // 1.1ua Clear saved path so it will ask user for path

	BOOL bAskForFileName = !( m_bOverwriteOutputFile && m_sPathXML != "" ); // 1.3dd Make not overwrite always ask for file name
	if ( bAskForFileName )
		{
		if ( !CExportProcess::s_bExportedFileDlg("xml", TRUE, sFileFilter, &iFilterIndex, m_sPathXML) )
			return TRUE;
		}

    CMarkerSet* pmkrset = m_pexpsetMyOwner->ptypMyOwner()->pmkrset();

	UnWriteProtect( m_sPathXML ); // 1.2fv Turn off write protect before writing // 1.4qzhg
    std::ofstream iosExport(m_sPathXML);
    // 1997-11-15 MRP: In MDF's export we don't return from failures here

	CDatabaseType* ptyp = m_pexpsetMyOwner->ptypMyOwner(); // 1.5.8a 
	Str8 sTyp = ptyp->sName(); // 1.5.8a 
	CShwView* pview = Shw_papp()->pviewActive(); // 1.5.8a 
	int iMargin = pview->iMargin(); // 1.5.8a 
	Str8 sMargin; // 1.5.8a 
	sMargin += iMargin; // 1.5.8a 
	Str8 sRootNodeWithType = m_sRootNode + " toolboxdatabasetype=\"" + sTyp + "\"" 
		+ " wrap=\"" + sMargin + "\"" // 1.5.8a Output database type and wrap margin in XML export
		+ " toolboxversion=\"" + g_sVersion + "\"" // 1.5.9eg Add Toolbox version to xml attributes
		+ " toolboxwrite=\"export\""; // 1.5.9eg Add Toolbox write as export to xml attributes

	CIndexSet* pindset = pindCur->pindsetMyOwner(); // 1.5.9pa Get XML head and tail info
	Str8 sHeadInfo = pindset->sGetHeadInfo(); // 1.5.9pa Get head info
	Str8 sXMLHead; // 1.5.9pa XML head info
	Str8 sXMLTail; // 1.5.9pa XML tail info
	int iXMLHead = sHeadInfo.Find( "\\xmlhead\n" ); // 1.5.9pa Find xmlhead marker
	int iXMLTail = sHeadInfo.Find( "\\xmltail\n" ); // 1.5.9pa Find xmltail marker
	if ( iXMLTail > 0 ) // 1.5.9pa If xmltail marker found, collect xml tail info
		{
		sXMLTail = sHeadInfo.Mid( iXMLTail + 9 ); // 1.5.9pa xml tail info is from tail mkr to end
		}
	if ( iXMLHead >= 0 ) // 1.5.9pa If xmlhead marker found, collect xml head info
		{
		int iXMLHeadLen = iXMLTail - ( iXMLHead + 9 ); // 1.5.9pa Calculate end of head mkr, if none, produces neg
		sXMLHead = sHeadInfo.Mid( iXMLHead + 9, iXMLHeadLen ); // 1.5.9pa xml head info is from head mkr tail mkr
		}

    if ( !m_sPathCCT.IsEmpty() )
        {
        Change_ostream iosChange(m_sPathCCT, iosExport);
        if ( iosChange.fail() )
            {
            int iRemoved = remove(m_sPathXML);
            return FALSE;
            }
        
        XML_ostream sfs(iosChange, pmkrset, sRootNodeWithType, m_bUTF8, m_sDTDName, m_sPathStylesheet);
		sfs.WriteString( sXMLHead ); // 1.5.9pa Export head extra in XML if change table
        s_Export(sfs, ptyp, bCurrentRecord, prelCur, bCurrentWindow, pindCur, // 1.4qpb
            m_subsetMarkersToExport, m_bPriKeyFieldFirst, m_sPathXML, m_bAddAlphDiv);
//		sfs.WriteString( sXMLTail ); // 1.5.9pa Export tail extra in XML if change table
        }
    else  // Not making changes
        {
        XML_ostream sfs(iosExport, pmkrset, sRootNodeWithType, m_bUTF8, m_sDTDName, m_sPathStylesheet);
		sfs.WriteString( sXMLHead ); // 1.5.9pa Export head extra in XML if change table
        s_Export(sfs, ptyp, bCurrentRecord, prelCur, bCurrentWindow, pindCur, // 1.4qpb
            m_subsetMarkersToExport, m_bPriKeyFieldFirst, m_sPathXML, m_bAddAlphDiv);
//		iosExport.flush(); // 1.5.9pb 
//		sfs.WriteString( sXMLTail ); // 1.5.9pa Export tail extra in XML if change table
        }

#ifdef _MAC
    // SetDocFileTypeXML(m_sPathXML);  // 1999-10-11 MRP
#endif

    BOOL bWriteFailed = iosExport.fail();
    iosExport.close();  // Flush any remaining output
    if ( bWriteFailed ) // 1.5.9pb 
        {
		sMessage = sMessage + _("Cannot write file:") + " " + m_sPathXML; // 1.5.0fd 
        int iRemoved = remove(m_sPathXML);
        return FALSE;
        }

	Str8 sExportedFile; // 1.5.9pd Add xml tail info
	if ( sXMLTail.GetLength() > 0 && bReadFileIntoString( m_sPathXML, sExportedFile ) ) // 1.5.9pd If xml tail info
		{
		sExportedFile += sXMLTail; // 1.5.9pd Add xml tail info
		bWriteFileFromString( m_sPathXML, sExportedFile ); // 1.5.9pd write file with added tail info
		}

#ifdef XML_DTD
    Str8 sDirPath = sGetDirPath(m_sPathXML);
    Str8 sDTDPath = sPath(sDirPath, m_sDTDName, ".dtd");
	UnWriteProtect( sDTDPath ); // 1.2fv Turn off write protect before writing // 1.4qzhg
    std::ofstream iosDTD(sDTDPath);
    XML_ostream::s_WriteDTD(iosDTD, pmkrset, m_sRootNode);
#endif

	if ( m_bChainedTo && m_pdocExported ) // 1.1ce If this process was chained to, unload the file
		{
		Str8 sPathFile =  sUTF8( m_pdocExported->GetPathName() ); // 1.4bj Delete temporary files after chain is finished // 1.4qxs
		m_pdocExported->SetModifiedFlag( FALSE ); // 1.1ce Clear modified to prevent write before close
		m_pdocExported->OnCloseDocument(); // 1.1ce Close file
		m_pdocExported = NULL; // 1.1cj Clear file so it can't be referred to again
		if ( !bDebugKaren ) // 1.4bk Allow Ctrl+Alt+Shift+K to keep chained export files
			int i = remove(sPathFile); // 1.4bj Delete temporary files after chain is finished
		}

    return TRUE;
}

BOOL CExportProcessXML::bCopy(CSetEl** ppselNew)
{
    CExportProcessXMLDlg dlg(this, (CExportProcessXML**)ppselNew);
    return (dlg.DoModal() == IDOK);
}

BOOL CExportProcessXML::bModify()
{
    CExportProcessXMLDlg dlg(this, TRUE);
    return (dlg.DoModal() == IDOK);
}

// ==========================================================================

CExportProcessSet::CExportProcessSet(CDatabaseType* ptypMyOwner,
        BOOL bAddDefaultProcesses)
{
    m_ptypMyOwner = ptypMyOwner;
    ASSERT( m_ptypMyOwner );

    // 1999-03-11 MRP
    m_sSettingsFolder = m_ptypMyOwner->ptypsetMyOwner()->sSettingsDirPath();
    RemoveExtraSeparatorInFolderPath(m_sSettingsFolder);

    m_pexpDefault = NULL;
    m_bCurrentRecord = FALSE;
    m_bCurrentWindow = FALSE;
    m_bAutoOpen = FALSE;
    m_bAutoUpdateStyles = TRUE;  // 1997-12-10 MRP: Change default value
    m_bShowProperties = FALSE;  // 1.1cb Make Show properties button default to no 
    m_bMDF = FALSE;
    m_bShowConvertOlderMDF = FALSE;  // 1999-03-02 MRP
    m_bNoMDFReminder = FALSE;  // 1998-01-21 MRP

    if ( bAddDefaultProcesses )
        {
        Add(new CExportProcessSF(s_pszDefaultNameSF, this));
        Add(new CExportProcessRTF(s_pszDefaultNameRTF, this));
        }
}


CExportProcessSet::~CExportProcessSet()
{
}


static const char* psz_expset = "expset";
static const char* psz_expDefault = "expDefault";
static const char* psz_CurrentRecord = "CurrentRecord";
static const char* psz_CurrentWindow = "CurrentWindow";
static const char* psz_AutoOpen = "AutoOpen";
static const char* psz_NotAutoUpdateStyles = "NotAutoUpdateStyles";
static const char* psz_SkipProperties = "SkipProperties";  // 1998-03-10 MRP
static const char* psz_MDF = "MDF";
// 1999-03-02 MRP: Move MDF version to the export process set.
static const char* psz_verMDF = "verMDF";  // 1999-03-02 MRP
static const char* psz_ShowConvertOlderMDF = "ShowConvertOlderMDF";
static const char* psz_NoMDFReminder = "NoMDFReminder";  // 1998-01-21 MRP

#ifdef typWritefstream // 1.6.4ac
void CExportProcessSet::WriteProperties(Object_ofstream& obs) const
#else
void CExportProcessSet::WriteProperties(Object_ostream& obs) const
#endif
{
    obs.WriteBeginMarker(psz_expset);
    // 1999-03-02 MRP: Move MDF version to the export process set.
    // Write these properties *before* the export processes so that
    // they can determine whether to their convert properties
    // from an older version.
    obs.WriteBool(psz_MDF, m_bMDF);
    if ( m_bMDF )
        obs.WriteString(psz_verMDF, m_sVersionMDF);
    obs.WriteBool(psz_ShowConvertOlderMDF, m_bShowConvertOlderMDF);
    obs.WriteBool(psz_NoMDFReminder, m_bNoMDFReminder);  // 1998-01-21 MRP
    obs.WriteNewline();

    const CExportProcess* pexp = pexpFirst();
    for ( ; pexp; pexp = pexpNext(pexp) )
        {
        pexp->WriteProperties(obs);
        obs.WriteNewline();
        }

    if ( m_pexpDefault )
        obs.WriteString(psz_expDefault, m_pexpDefault->sName());

    obs.WriteBool(psz_CurrentRecord, m_bCurrentRecord);    
    obs.WriteBool(psz_CurrentWindow, m_bCurrentWindow);
    obs.WriteBool(psz_AutoOpen, m_bAutoOpen);
    obs.WriteBool(psz_NotAutoUpdateStyles, !m_bAutoUpdateStyles);
    obs.WriteBool(psz_SkipProperties, !m_bShowProperties);  // 1998-03-10 MRP

    obs.WriteEndMarker(psz_expset);
}

BOOL CExportProcessSet::s_bReadProperties(Object_istream& obs,
        CDatabaseType* ptyp, CExportProcessSet** ppexpset)
{
    if ( !obs.bReadBeginMarker(psz_expset) )
        return FALSE;

    ASSERT( ptyp );
    CExportProcessSet* pexpset = new CExportProcessSet(ptyp, FALSE);
    pexpset->ReadProperties(obs);

    ASSERT( ppexpset );
    if ( *ppexpset )
        delete *ppexpset;
    *ppexpset = pexpset;

    return TRUE;
}

void CExportProcessSet::ReadProperties(Object_istream& obs)
{
    Str8 sDefaultName;
    Str8 sFieldsToHideName;
	m_bShowProperties = TRUE; // 1.4qzfh Fix bug of export losing show properties setting

    while ( !obs.bAtEnd() )
        {
        BOOL b = FALSE;
        CExportProcess* pexp = NULL;
        if ( CExportProcessSF::s_bReadProperties(obs, this, &pexp) )
            {
            if ( pexp )
                Add(pexp);
            }
        else if ( CExportProcessRTF::s_bReadProperties(obs, this, &pexp) )
            {
            if ( pexp )
                Add(pexp);
            }
        else if ( CExportProcessMDF::s_bReadProperties(obs, this, &pexp) )
            {
            if ( pexp )
                {
                Add(pexp);
                m_bMDF = TRUE;
                }
            }
        else if ( CExportProcessXML::s_bReadProperties(obs, this, &pexp) )
            {
            if ( pexp )
                Add(pexp);
            }
        else if ( obs.bReadString(psz_expDefault, sDefaultName) )
            ;
        else if ( obs.bReadBool(psz_CurrentRecord, m_bCurrentRecord) )
            ;
        else if ( obs.bReadBool(psz_CurrentWindow, m_bCurrentWindow) )
            ;
        else if ( obs.bReadBool(psz_AutoOpen, m_bAutoOpen) )
            ;
        else if ( obs.bReadBool(psz_NotAutoUpdateStyles, b) )
            m_bAutoUpdateStyles = FALSE;  // 1997-12-10 MRP: Change default value
        else if ( obs.bReadBool(psz_SkipProperties, b) )
            m_bShowProperties = FALSE;  // 1998-03-10 MRP
        else if ( obs.bReadBool(psz_MDF, m_bMDF) )
            ;
        // 1999-03-02 MRP: Move MDF version to the export process set.
        else if ( obs.bReadString(psz_verMDF, m_sVersionMDF) )
            ;
        else if ( obs.bReadBool(psz_ShowConvertOlderMDF, m_bShowConvertOlderMDF) )
            ;
        else if ( obs.bReadBool(psz_NoMDFReminder, m_bNoMDFReminder) )  // 1998-01-21 MRP
            ;
        else if ( obs.bEnd(psz_expset) )
            break;
        }

    // 1998-03-18 MRP: Provide two default processes for "one-shot" jobs
    // and to avoid showing the user an empty list in the Export dialog box.
    if ( !pexpSearch(s_pszDefaultNameSF) )
        Add(new CExportProcessSF(s_pszDefaultNameSF, this));
    if ( !pexpSearch(s_pszDefaultNameRTF) )
        Add(new CExportProcessRTF(s_pszDefaultNameRTF, this));

    m_pexpDefault = pexpSearch(sDefaultName);

    // 1999-03-02 MRP: Move MDF version to the export process set.
    if ( m_bMDF )
        {
        if ( strcmp(m_sVersionMDF, CExportProcessMDF::s_pszVersion) < 0 )
            m_bShowConvertOlderMDF = TRUE;
        m_sVersionMDF = CExportProcessMDF::s_pszVersion;
        }
}

BOOL CExportProcessSet::bInterlinear() const
{
    return !m_ptypMyOwner->pintprclst()->bIsEmpty();
}

CMarkerSet* CExportProcessSet::pmkrset()
{
    return m_ptypMyOwner->pmkrset();
}

void CExportProcessSet::SetMultiDictionaryFormatter(BOOL bMDF)
{
    if ( m_bMDF == bMDF )
        return;

    m_bMDF = bMDF;
    if ( m_bMDF )
        {
        CExportProcessMDF* pexp =
            new CExportProcessMDF("Multi-Dictionary Formatter", this, TRUE);
        Add(pexp);
        }
    else
        {
        CFilter* pfil = NULL;
        CExportProcess* pexp = pexpFirst();
        while ( pexp )
            if ( pexp->bMDF(&pfil) )
                {
                if ( m_pexpDefault == pexp )
                    m_pexpDefault = NULL;
                CSet::Delete((CSetEl**)&pexp);
                }
            else
                pexp = pexpNext(pexp);
                
        }
}

void CExportProcessSet::SetNoMDFReminder(BOOL bNoMDFReminder)
{
    m_bNoMDFReminder = bNoMDFReminder;  // 1998-01-21 MRP
}


void CExportProcessSet::WritePaths( std::ofstream& ostr ) // Write paths of supporting files
{
    for ( CExportProcess* pexp = pexpFirst(); pexp; pexp = pexpNext(pexp) )
        pexp->WritePaths( ostr );
}

BOOL CExportProcessSet::bExport(CIndex* pindCur, const CRecLookEl* prelCur,
        const Str8& sPathDatabase, Str8& sMessage)
{
    if ( !m_pexpDefault )
        m_pexpDefault = pexpFirst();
	CDatabaseType* ptyp = ptypMyOwner(); // 1.4qmk
	if ( !ptyp->bCCTable() || m_bShowProperties ) // 1.4qmk If CCTable, do export without asking for export process // 1.4qzff // 1.4qzfg Show dialog box on CCT export if show properties is on
		{
		CExportProcessDlg dlg(this, pindCur, prelCur, sPathDatabase);
		if ( dlg.DoModal() != IDOK )
			return TRUE;
		}
	if ( !m_pexpDefault )
		return TRUE;
    return m_pexpDefault->bExport(pindCur, prelCur, sPathDatabase, sMessage);
}

void CExportProcessSet::FilterUpdated(CFilterUpdate& fup)
{
    CExportProcess* pexp = pexpFirst();
    for ( ; pexp; pexp = pexpNext(pexp) )
        pexp->FilterUpdated(fup);
}

void CExportProcessSet::Delete(CSetEl** ppsel)
{
    ASSERT( ppsel );
    CExportProcess* pexp = (CExportProcess*)*ppsel;
    const Str8& sName = pexp->sName();
    Str8 sMessage = _("Delete export process?"); // 1.5.0fd

    MessageBeep(0);
    if ( AfxMessageBox(sMessage, MB_YESNO | MB_ICONQUESTION) != IDYES )
        return;

    CSet::Delete(ppsel);

    // 1997-11-15 MRP: If the default export process just got deleted
    // we must change it right now. Otherwise it's left dangling
    // if we choose Cancel from the Export dialog box.
    if ( pexp == m_pexpDefault )
        m_pexpDefault = (CExportProcess*)*ppsel;
}

BOOL CExportProcessSet::bAdd(CSetEl** ppselNew)
{
    BOOL bMDF = FALSE;
    BOOL bXML = FALSE;  // 1999-05-21 MRP
    BOOL bRTF = FALSE;
    CExportProcessTypeDlg dlgType(m_bMDF, &bMDF, &bXML, &bRTF);
    if ( dlgType.DoModal() != IDOK )
        return FALSE;

    if ( bMDF )
        {
        CMDFDlg dlg(this, FALSE, (CExportProcessMDF**)ppselNew);  // 1999-05-21 MRP
        if ( dlg.DoModal() != IDOK )
            return FALSE;
        }
    else if ( bXML )  // 1999-05-21 MRP
        {
        CExportProcessXMLDlg dlg(this, (CExportProcessXML**)ppselNew);
        if ( dlg.DoModal() != IDOK )
            return FALSE;
        }
    else if ( bRTF )
        {
        CExportProcessRTFDlg dlg(this, (CExportProcessRTF**)ppselNew);
        if ( dlg.DoModal() != IDOK )
            return FALSE;
        }
    else
        {
        CExportProcessSFDlg dlg(this, (CExportProcessSF**)ppselNew);
        if ( dlg.DoModal() != IDOK )
            return FALSE;
        }

    return TRUE;
}

BOOL CExportProcessSet::bValidNameDlg(const Str8& sName,
        const CExportProcess* pexpModify, Str8& sMessage)
{
    if ( sName.IsEmpty() )
        {
        sMessage = _("Please give a name."); // 1.5.0fd 
        return FALSE;
        }

    CExportProcess* pexp = pexpSearch(sName);
    if ( pexp != NULL && pexp != pexpModify )
        {
		sMessage = sMessage + _("Name is already used:") + " " + sName; // 1.5.0fd 
        return FALSE;
        }

    return TRUE;    
}

BOOL CExportProcessSet::bValidNameRead(const Str8& sName,
        const char* pszDefaultName, Str8& sValidName)
{
    if ( sName.IsEmpty() )
        {
        ASSERT( pszDefaultName );
        sValidName = sUniqueName(pszDefaultName);
        return FALSE;
        }

    if ( pexpSearch(sName) )
        {
        sValidName = sUniqueName(sName);
        return FALSE;
        }

    return TRUE;    
}


// --------------------------------------------------------------------------

COldExportProcessSet::COldExportProcessSet(CDatabaseType* ptypMyOwner) :
    CExportProcessSet(ptypMyOwner, FALSE)
{
    m_psubsetMarkersToExport = new CMarkerSubSet(ptypMyOwner->pmkrset());
    m_bUseExportCCT = FALSE;
    m_bMarkerFont = FALSE;
    m_ppgs = NULL;
    m_bExportMDF = FALSE;
    m_bExportRTF = FALSE;
    m_pexpMDF = NULL;
}

COldExportProcessSet::~COldExportProcessSet()
{
    delete m_psubsetMarkersToExport;
    delete m_ppgs;
    ASSERT( !m_pexpMDF );
    delete m_pexpMDF;
}


static const char* psz_ExportMDF = "ExportMDF";
static const char* psz_ExportRTF = "ExportRTF";
static const char* psz_UseExportCCT = "MakeChangesExportSF";
static const char* psz_ExportCCT = "cctExportSF";
static const char* psz_IncludeFormatting = "IncludeFormatting";
static const char* psz_mrfsetExport = "mrfsetExportFieldsExcluded";

BOOL COldExportProcessSet::bReadProperties(Object_istream& obs)
{
    if ( obs.bReadBool(psz_CurrentRecord, m_bCurrentRecord) )
        return TRUE;
    if ( obs.bReadBool(psz_CurrentWindow, m_bCurrentWindow) )
        return TRUE;
    if ( obs.bReadBool(psz_AutoOpen, m_bAutoOpen ) )
        return TRUE;
    if ( m_psubsetMarkersToExport->bReadOldProperties(obs, psz_mrfsetExport) )
        return TRUE;
    if ( obs.bReadBool(psz_UseExportCCT, m_bUseExportCCT) )
        return TRUE;
    if ( obs.bReadProjectPath(psz_ExportCCT, m_sExportCCT ) )
        return TRUE;
    if ( obs.bReadBool(psz_IncludeFormatting, m_bMarkerFont ) )
        return TRUE;
    if ( RtfPageSetup::s_bReadProperties(obs, FALSE, &m_ppgs) )
        return TRUE;
    if ( CExportProcessMDF::s_bReadOldProperties(obs, this, &m_pexpMDF) )
        return TRUE;
    if ( obs.bReadBool(psz_ExportMDF, m_bExportMDF) )
        return TRUE;
    if ( obs.bReadBool(psz_ExportRTF, m_bExportRTF) )
        return TRUE;

    return FALSE;
}

void COldExportProcessSet::ConvertOldProperties()
{
    Str8 sPathCCT;
    if ( m_bUseExportCCT )
        sPathCCT = m_sExportCCT;
    CExportProcessSF* pexpSF = new CExportProcessSF(s_pszDefaultNameSF, this,
        sPathCCT, *m_psubsetMarkersToExport, FALSE);
    Add(pexpSF);

    CExportProcessRTF* pexpRTF = new CExportProcessRTF(s_pszDefaultNameRTF, this,
        "", "", m_bMarkerFont, *m_psubsetMarkersToExport, FALSE, m_ppgs);
    Add(pexpRTF);

    if ( m_pexpMDF )
        {
        Add(m_pexpMDF);
        m_bMDF = TRUE;
        }

    if ( m_bExportMDF && m_pexpMDF )
        m_pexpDefault = m_pexpMDF;
    else if ( m_bExportRTF )
        m_pexpDefault = pexpRTF;
    else
        m_pexpDefault = pexpSF;

    delete m_psubsetMarkersToExport;
    m_psubsetMarkersToExport = NULL;
    delete m_ppgs;
    m_ppgs = NULL;
    m_pexpMDF = NULL;

    // 1999-03-02 MRP: Move MDF version to the export process set.
    if ( m_bMDF )
        {
        if ( strcmp(m_sVersionMDF, CExportProcessMDF::s_pszVersion) < 0 )
            m_bShowConvertOlderMDF = TRUE;
        m_sVersionMDF = CExportProcessMDF::s_pszVersion;
        }
}

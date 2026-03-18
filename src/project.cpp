// project.cpp : project class implementation
//

#include "stdafx.h"
#include "toolbox.h"
#include "project.h"
#include "shw.h"
#include "mainfrm.h"
#include "lng.h"
#include "typ.h"
#include "kmn.h"
#include <fstream>
#include "find_d.h"
#include "tools.h"  // sPath
#include "font.h"
#include "obstream.h"  // Object_istream, Object_ostream
#include "corpus.h"
#include "rundos.h"
#include "alan.h" // 1.5.8h 

// 11-08-1997
#include "nlstream.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProject construction

CProject::CProject( const char* pszProjectPath, const char* pszSettingsVersion )
{
    ASSERT( pszProjectPath );
    ASSERT( pszSettingsVersion );
    m_sSettingsVersion = pszSettingsVersion;  // 1998-03-12 MRP
    
    m_bClosing = FALSE;
    m_bWarnDocClose = FALSE;
    m_bShowWholePath = FALSE;
    m_bAutoWrap = TRUE;
    m_bAutoSave = TRUE; // 5.96h ALB Fix bug of forgetting setting of autosave
    m_iLockProject = 0; // 1.2bp Lock settings option
	m_bSaved = FALSE; // 1.1tu
    m_bExerciseNoSave = FALSE; // 5.99f
    m_bNoBackup = FALSE; // 1.5.3p 
    m_bSaveWithoutNewlines = FALSE; // 1.5.4b 
    m_bUseImportCCT = FALSE;
    m_bRemoveSpaces = TRUE;
    m_bMovedPath = FALSE;  // 1997-08-13
    m_iLangCtlHeight = eNormalLangCtrl;
    m_bCheckConsistencyWhenEditing = TRUE;
    m_bAutomaticInterlinearUpdate = FALSE; // 5.98w Make automatic interlinear update optional
	m_bDontAskChangeMarker = FALSE; // 1.6.1ch Default to ask change marker
	m_bExportProcessAutoOpening = FALSE; // 1.1bh Export process auto open
	m_ptypExportProcessAutoOpening = NULL; // 1.1bh Export process auto open

    // Be SURE to delete anything allocated here in the destructor!!!
    m_pfndset = new CFindSettings();
    m_plngset = new CLangEncSet( m_sSettingsVersion );
    m_ptypset = new CDatabaseTypeSet( m_plngset, m_sSettingsVersion );
    m_pcorset = new CCorpusSet();
    m_pkmn = new CKeyboardManager( m_plngset->pkbdset() );
    m_pDosRunner = new CRunDos;

    SetPath( pszProjectPath );
}

CProject::~CProject()
{
    m_bClosing = TRUE;
    // 1998-03-24 MRP: Removing the dangling reference to this project's
    // find text list seems like a good idea. But the following line of code
    // has always been commented out. Did it cause a problem???
    // Shw_pcboFind()->ChangeSet( NULL ); // tell find combo project is closing
    // 2000-08-29 TLB & MRP : CFindCombo now accesses CProject to get current CFindTextList
    // so there is no need to do the above function call (which failed because the Main Frame
    // window was destroyed before the project was destroyed).
    // m_pkmn->DeActivateKeyboards();  // 1998-07-07 MRP

    delete m_pkmn;
    delete m_ptypset;
    delete m_pcorset;
    delete m_plngset;
    delete m_pfndset;
    delete m_pDosRunner;
}

BOOL bAnotherInstanceFound() // 1.2mm See if another instance of Toolbox is running
	{
	Str8 sTitle = Shw_papp()->sLastWndTitle(); // 1.2dg If another copy of Toolbox already open, just activate it
	if ( !sTitle.IsEmpty() )
		{
//		HWND hwnd = ::FindWindow( NULL, swUTF16( sTitle  ) ); // 1.4qun Upgrade FindWindow for Unicode build // 1.4vwb 
//		if ( hwnd ) // is another instance of Shoebox already running? // 1.4vwb 
		CWnd* pwnd = CWnd::FindWindow( NULL, swUTF16( sTitle  ) ); // 1.4qun Upgrade FindWindow for Unicode build // 1.4vwb 
		if ( pwnd ) // is another instance of Shoebox already running? // 1.4vwb Try to fix bug of sometimes not opening project after crash
			{
			CString sTitleFound; // 1.4vwb
			pwnd->GetWindowText( sTitleFound ); // 1.4vwb
			WINDOWPLACEMENT* lpwndpl = NULL; // 1.4vwd
			pwnd->GetWindowPlacement( lpwndpl );  // 1.4vwd
//			::SetForegroundWindow( hwnd ); // activate previous instance // 1.4su Don't flash window if another instance found // 1.4vwa 
			Shw_papp()->SetAnotherInstanceAlreadyRunning( TRUE );
			return TRUE;
			}
		}
	return FALSE;
	}

extern void BringIn(); // 1.4vwe 

BOOL CProject::s_bConstruct(const char* pszProjectPath, BOOL bOpening,
        const char* pszSettingsVersion, CProject** ppprj)
{
    ASSERT( pszProjectPath );
    ASSERT( pszSettingsVersion );
    ASSERT( ppprj );
    ASSERT( !*ppprj );  // If it isn't NULL now, there will be a leak.

    CProject* pprj = new CProject(pszProjectPath, pszSettingsVersion);
    
    // 1998-03-12 MRP: It's abnormal to assign the object so soon,
    // but right now CLangEnc::bReadProperties depends on CShwApp::m_pProject.
    // Once the global dependencies are removed, clean this up.
    *ppprj = pprj;

    // Create proxies for all .lng and .typ files in the settings directory.
    CNoteList notlst;
    CLangEncSet* plngset = pprj->m_plngset;
    plngset->ReadAllProxies(notlst);
    CDatabaseTypeSet* ptypset = pprj->m_ptypset;
    ptypset->ReadAllProxies(notlst);
    BOOL bConverting = FALSE;
    BOOL bOlder = FALSE;
    
    if ( bConverting )
        {
        // Read and convert all language encodings.
        CLangEncProxy* plrx = plngset->plrxFirst();
        for ( ; plrx; plrx = plngset->plrxNext(plrx) )
            plrx->plng(notlst);  // Causes file to be read
            
        // Read and convert all database types
        BOOL bDateStamp = FALSE;
        CDatabaseTypeProxy* ptrx = ptypset->ptrxFirst();
        for ( ; ptrx; ptrx = ptypset->ptrxNext(ptrx) )
            {
            CDatabaseType* ptyp = ptrx->ptyp(notlst);
            if ( ptyp->pmkrDateStamp() )
                bDateStamp = TRUE;
            }
        }

    if ( !pprj->bInitializeProperties(bOpening) )  // 1998-03-24 MRP
        {
        delete pprj;
        // 1998-03-14 MRP: And here we have to delay assigning NULL
        // because of another global dependency: yuck, Yuck, YUCK!!!
        *ppprj = NULL;
        return FALSE;
        }

    if ( bConverting )
        // Rewrite all the settings files
        pprj->SaveSettings(TRUE);  // 1998-03-24 MRP

    // 1998-05-07 MRP: If the project has been moved or copied
    // to a different folder, update the jump and interlinear paths
    // in the database types that are actually in use.
    // 1998-06-27 MRP: ONLY update paths in types that are in use,
    // even if they all got converted from older/newer format.
    if ( pprj->bProjectMoved() )
        {
        CDatabaseTypeProxy* ptrx = ptypset->ptrxFirst();
        for ( ; ptrx; ptrx = ptypset->ptrxNext(ptrx) )
            {
            CNoteList notlst;
            CDatabaseType* ptyp = ptrx->ptyp(notlst);
            ptyp->UpdatePaths();
            }
		if ( !pprj->bExerciseNoSave() ) // 1.4vwg Don't bring in window of moved exercise project
			BringIn(); // 1.4vwe If project moved, bring Toolbox window into home position
        }
    if ( pprj->bProjectMoved() ) // 1.2mj If project moved and ambig window outside 640x480, move it in
        {
//		if ( pprj->m_rectAmbig.left > 400 || pprj->m_rectAmbig.left < 0 ) // 1.2mj If left or above main monitor, move it in also // 1.4vwf 
//			pprj->m_rectAmbig.left = 0; // 1.4vwf Not needed because included in BringIn
//		if ( pprj->m_rectAmbig.top > 300 || pprj->m_rectAmbig.top < 0) // 1.4vwf 
//			pprj->m_rectAmbig.top = 20; // 1.4vwf 
		}

    *ppprj = pprj;

    return TRUE;
}

BOOL CProject::bInitializeProperties(BOOL bOpening)
{
    if ( bOpening )
        {
        CNoteList notlst;
        if ( !bReadProperties(m_sProjectPath, notlst) )
            return FALSE;
		m_pcorset->UpdatePaths();
        }
    else {
#ifdef _MAC
    CFontDef fntdef("Helvetica", -11); // on Mac point size == height in pixels
#elif defined(TLB_07_18_2000_FONT_HANDLING_BROKEN)
//    CFontDef fntdef("MS Sans Serif", -13);
#else
    CFontDef fntdef("MS Sans Serif", -10, TRUE /* Points */);
#endif
        fntdef.iCreateFont( &m_fntMkrs ); // create a default font
    
        if ( !bWriteProperties(m_sProjectPath) )
            {   
            MessageBeep(0);
            AfxMessageBox( _("Error writing project file.") );
            return FALSE;
            }
        }           

    // tell find combo to use this project's settings
    // 2000-08-29 TLB & MRP : CFindCombo now accesses CProject to get current CFindTextList
    // Shw_pcboFind()->ChangeSet( m_pfndset->pftxlst() );

    return TRUE;
}

// --------------------------------------------------------------------------

void CProject::SetPath( const char* pszPath )
{
    m_sProjectPath = pszPath;
    m_sSettingsDirPath = sGetDirPath( pszPath );
    m_sProjectName = sGetFileName( pszPath );

    plngset()->SetDirPath( m_sSettingsDirPath );
    ptypset()->SetDirPath( m_sSettingsDirPath );
    pcorset()->SetDirPath( m_sSettingsDirPath );
}

void CProject::SetLangCtrlHeight( int iHeight ) // set size of language controls - normal size or enlarged
{
    if ( iHeight != m_iLangCtlHeight )
        {
        m_iLangCtlHeight = min(eLargeLangCtrl+15, max(eNormalLangCtrl, iHeight)); // set reasonable limits in case settings file gets garbled
        plngset()->RecalcCtrlFonts(); // make new control fonts if necessary
        }
    CShwStatusBar* pStatusBar = (CShwStatusBar*)Shw_pmainframe()->GetMessageBar(); // get pointer to status bar
    ASSERT(pStatusBar);
    if (pStatusBar != nullptr)
    {
        pStatusBar->SetFontAndSize(); // resize status bar on the fly
    }
}

void CProject::DoAutoSave()
{
    plngset()->WriteAllModified();
    ptypset()->WriteAllModified();
}

void CProject::SaveSettings( BOOL bForceWrite )
{
    // 1998-03-14 MRP: Write .lng, .typ, then .prj
    plngset()->WriteAllModified( bForceWrite );
    ptypset()->WriteAllModified( bForceWrite );

    BOOL bWritten = bWriteProperties(m_sProjectPath, bForceWrite );
	SetSaved( TRUE ); // 1.1tu Note that something has been saved
}

extern Str8 g_sVersion; // 1.1tr Write program version in project file, for information only

static const char* psz_ShProjectSettings = "ShProjectSettings";
static const char* psz_ProgramVersion = "ProgramVersion"; // 1.1tr Write program version in project file, for information only
static const char* psz_ver = "ver";
static const char* psz_ProjectPath = "ProjectPath";
static const char* psz_placementMain = "placementMain";
static const char* psz_WarnDocClose = "WarnDocClose";
static const char* psz_ShowWholePath = "ShowWholePath";
static const char* psz_AutoWrap = "AutoWrap";
static const char* psz_NoAutoWrap = "NoAutoWrap";  // 1999-08-06 MRP
static const char* psz_NoAutoSave = "NoAutoSave";
static const char* psz_LockProject = "LockProject";
static const char* psz_ExerciseNoSave = "ExerciseNoSave";
static const char* psz_NoBackup = "NoBackup"; // 1.5.3p 
static const char* psz_SaveWithoutNewlines = "SaveWithoutNewlines"; // 1.5.4b 
static const char* psz_Markers = "Markers";
static const char* psz_Interlinearize = "Interlinearize";
static const char* psz_Adapt = "Adapt";
static const char* psz_FindSettings = "FindSettings";
static const char* psz_UseImportCCT = "UseImportCCT";
static const char* psz_ImportCCT = "ImportCCT";
static const char* psz_KeepSpaces = "KeepSpaces";

static const char* psz_StatusBarHidden = "StatusBarHidden";
static const char* psz_ToolBarHidden = "ToolBarHidden";
static const char* psz_LangCtrlHeight = "LangCtrlHeight";
static const char* psz_DontCheckConsistencyWhenEditing = "DontValidateOnEdit";
static const char* psz_AutomaticInterlinearUpdate = "AutomaticInterlinearUpdate"; // 1.1an Save automatic interlinear update setting in prj file
static const char* psz_AmbigBoxLeft = "AmbigBoxLeft"; // 1.2mj
static const char* psz_AmbigBoxTop = "AmbigBoxTop"; // 1.2mj

static const char* psz_DontAskChangeMarker = "DontAskChangeMarker"; // 1.6.1ch Save status of ask change mkr in prj

BOOL CProject::bWriteProperties(const char* pszPath, BOOL bForceWrite)
{
	CProject* pprj = Shw_papp()->pProject();
	if ( pprj->bExerciseNoSave() && !bForceWrite ) // 5.99f Don't save if exercises, unless forced by the toggle
		return TRUE;

	UnWriteProtect( pszPath ); // 1.2af Unwrite protect project file so it can be written // 1.4qzhg
#ifdef prjWritefstream // 1.6.1cb  // 1.6.4aa Never define FileStream // 1.6.4aa Use prjWritefstream
	Str8 sPath = pszPath; // 1.6.1cb 
	CString swPath = swUTF16( sPath ); // 1.6.1cb 
	Str8 sMode = "w"; // Set up mode string  // 1.6.1cb 
	CString swMode = swUTF16( sMode ); // Convert mode string to CString  // 1.6.1cb 
	FILE* pf = nullptr;
	errno_t err = _wfopen_s(&pf, swPath, swMode);
	if (err != 0 || pf == nullptr) {
		return FALSE; // 1.6.1cb 
	}
    Object_ofstream obs(pf); // 1.6.1cb 
    WriteProperties(obs); // 1.6.4aa 
#else
    std::ofstream ios(pszPath); // 1.6.1cb 
    if ( ios.fail() ) // 1.6.1cb 
        return FALSE; // 1.6.1cb 
    Object_ostream obs(ios); // 1.6.1cb 
    WriteProperties(obs); // 1.6.4aa 
#endif  // 1.6.1cb 
#ifdef _MAC
    SetProjectFileType(pszPath); // set file type and creator
#endif
	if ( !Shw_papp()->bGetClosingProjectOrMainFrame() && !bExerciseNoSave() ) // 1.2ms Fix bug of write protecting exercise project
		WriteProtect( pszPath ); // 1.2af If project not closing, write protect it again as signal that it is in use // 1.4qzh // 1.4st Fix U bug of clearing write protect of prj on save all

    return TRUE;
}

#ifdef prjWritefstream // 1.6.4aa Use prjWritefstream
void CProject::WriteProperties(Object_ofstream& obs)
#else
void CProject::WriteProperties(Object_ostream& obs)
#endif // 1.6.4aa 
{
    obs.WriteBeginMarker(psz_ShProjectSettings);

    // 1998-03-12 MRP: Always write the current version
    obs.WriteString(psz_ver, m_sSettingsVersion);  

    obs.WriteString( psz_ProgramVersion, g_sVersion ); // 1.1tr Write program version in project file, for information only

    obs.WriteString(psz_ProjectPath, m_sProjectPath);

    obs.WriteInteger(psz_LockProject, m_iLockProject); // 1.2fb If project is not locked, don't let any views be locked

    m_plngset->pkbdset()->WriteAutoSwitching(obs);  // 1999-09-30 MRP // 1.6.4aa 
    pcorset()->WriteProperties(obs); // 1.6.4aa 

    obs.WriteBool(psz_WarnDocClose, m_bWarnDocClose);
    obs.WriteBool(psz_ShowWholePath, m_bShowWholePath);
    obs.WriteBool(psz_NoAutoWrap, !m_bAutoWrap);  // 1999-08-06 MRP
    obs.WriteBool(psz_NoAutoSave, !m_bAutoSave);
    obs.WriteBool(psz_ExerciseNoSave, m_bExerciseNoSave); // 5.99f
    obs.WriteBool(psz_NoBackup, m_bNoBackup); // 1.5.3p 
    obs.WriteBool(psz_SaveWithoutNewlines, m_bSaveWithoutNewlines); // 1.5.4b 
    obs.WriteBool(psz_UseImportCCT, m_bUseImportCCT);
    obs.WriteString(psz_ImportCCT, m_sImportCCT);
    obs.WriteBool(psz_KeepSpaces, !m_bRemoveSpaces);

    // bReadProperties() depends on these two properties being written before the marker font
    obs.WriteBool(psz_StatusBarHidden, !Shw_pmainframe()->bStatusBarVisible());
    obs.WriteBool(psz_ToolBarHidden, !Shw_pmainframe()->bToolBarVisible());
    obs.WriteInteger(psz_LangCtrlHeight, m_iLangCtlHeight);
    obs.WriteBool(psz_DontCheckConsistencyWhenEditing, !m_bCheckConsistencyWhenEditing);
    obs.WriteBool(psz_AutomaticInterlinearUpdate, m_bAutomaticInterlinearUpdate); // 1.1an Save automatic interlinear update setting in prj file
    obs.WriteBool(psz_DontAskChangeMarker, m_bDontAskChangeMarker); // 1.6.1ch Save status of ask change marker

    CFontDef fntdefAmbig( &m_fntMkrs );
    fntdefAmbig.WriteProperties(obs, psz_Markers); // write font used to display markers // 1.6.4aa 
    if (m_pDosRunner) 
		{
		m_pDosRunner->WriteProperties(obs, psz_Markers); // 1.6.4aa 
		}
    
    // The main frame window's placement    
    obs.WriteWindowPlacement(psz_placementMain, Shw_papp()->m_pMainWnd);
    Shw_papp()->WriteMRUList(obs); // 1.6.4aa 
    Shw_papp()->WriteDbList(obs); // 1.6.4aa 
    pfndset()->WriteProperties(obs); // Find... settings // 1.6.4aa 
	obs.WriteInteger(psz_AmbigBoxLeft, m_rectAmbig.left); // 1.2mj
	obs.WriteInteger(psz_AmbigBoxTop, m_rectAmbig.top); // 1.2mj
	obs.WriteUnrecognizedSettingsInfo( m_sUnrecognizedSettingsInfo ); // 1.0cp Write unrecognized settings info
        
    obs.WriteEndMarker(psz_ShProjectSettings);
}

BOOL CProject::bReadProperties(const char* pszPath, CNoteList& notlst)
{
    ASSERT( pszPath );
    ASSERT( *pszPath );
	std::ifstream ios(pszPath);
	if (!ios) {
		// handle file-not-found
		return FALSE;
	}
	// 08-11-1997
    //The class Newline_istream provieds datas from both, Mac and PC
    Newline_istream iosnl(ios);
    Object_istream obs(iosnl, notlst);
    ReadProperties(obs);
    return TRUE;
}

BOOL CProject::s_bIsProjectFile(const char* pszPath)
{
    Str8 sVersion;
    return s_bIsProjectFile(pszPath, sVersion);
}

BOOL CProject::s_bIsProjectFile(const char* pszPath, Str8& sVersion)
{
	std::ifstream ios(pszPath);
	if (!ios) {
		// handle file-not-found
		return FALSE;
	}
        
    CNoteList notlst;
    
    //08-11-1997
    //The class Newline_istream provieds datas from both, Mac and PC
    Newline_istream iosnl(ios);
    Object_istream obs(iosnl, notlst);
    if ( !obs.bAtBackslash() || !obs.bReadBeginMarker(psz_ShProjectSettings) )
        return FALSE;

    BOOL b = obs.bReadString(psz_ver, sVersion);

    return TRUE;
}
        
void CProject::ReadProperties(Object_istream& obs)
{
    BOOL b = ( obs.bAtBackslash() && obs.bReadBeginMarker(psz_ShProjectSettings) );
    ASSERT( b );

    Str8 sVersion;
    Str8 sProgramVersion; // 1.1tr Write program version in project file, for information only
    BOOL bAutoWrap = FALSE;  // 1999-08-06 MRP
    BOOL bNoAutoWrap = FALSE;  // 1999-08-06 MRP
    BOOL bNoAutoSave = FALSE;
    BOOL bExerciseNoSave = FALSE;
    BOOL bNoBackup = FALSE; // 1.5.3p 
    BOOL bSaveWithoutNewlines = FALSE; // 1.5.4b
    BOOL bStatusBarHidden = FALSE;
    BOOL bToolBarHidden = FALSE;
    int iLangCtlHeight = eNormalLangCtrl;
	int iAmbigLeft = 0; // 1.2mj
	int iAmbigTop = 20; // 1.2mj
    CFontDef fntdef("MS Sans Serif", -10, TRUE );
    BOOL bFontCreated = FALSE;

    ASSERT(m_pDosRunner);

    Str8 s;  
    while ( !obs.bAtEnd() )
        {
        if ( obs.bReadString(psz_ver, sVersion) )
            ;
        else if ( obs.bReadString(psz_ProgramVersion, sProgramVersion) ) // 1.1tr Write program version in project file, for information only
            ;
        else if ( obs.bReadString(psz_ProjectPath, m_sPrevProjectPath) ) // Read saved project path
            ;
        else if ( m_plngset->pkbdset()->bReadAutoSwitching(obs) )  // 1999-09-30 MRP
            ;
        else if ( obs.bReadMainWindowPlacement(psz_placementMain, Shw_papp()->m_pMainWnd) )
            ; // The main frame window's placement
        else if ( Shw_papp()->bReadMRUList(obs) ) // read MRU list
            ;
        else if ( Shw_papp()->bReadDbList(obs) ) // read autoload list
            ;
        else if ( pcorset()->bReadProperties(obs))  // read text corpora
            ;
        else if ( obs.bReadBool(psz_WarnDocClose, m_bWarnDocClose) )
            ; // warn if closing doc used in a jump path
        else if ( obs.bReadBool(psz_ShowWholePath, m_bShowWholePath) )
            ; // show whole path of filenames in listboxes
        else if ( obs.bReadBool(psz_AutoWrap, bAutoWrap) )
            ;
        else if ( obs.bReadBool(psz_NoAutoWrap, bNoAutoWrap) )
            ;
        else if ( obs.bReadBool(psz_NoAutoSave, bNoAutoSave) )
            ;
        else if ( obs.bReadInteger(psz_LockProject, m_iLockProject) )
            ;
        else if ( obs.bReadBool(psz_ExerciseNoSave, bExerciseNoSave) )
            ;
        else if ( obs.bReadBool(psz_NoBackup, bNoBackup) ) // 1.5.3p 
            ;
        else if ( obs.bReadBool(psz_SaveWithoutNewlines, bSaveWithoutNewlines) ) // 1.5.4b 
            ;
        else if ( obs.bReadBool(psz_UseImportCCT, m_bUseImportCCT) )
            ;
        else if ( obs.bReadString(psz_ImportCCT, m_sImportCCT ) )
            ;
        else if ( obs.bReadBool(psz_KeepSpaces, b) )
            m_bRemoveSpaces = FALSE;
        else if ( obs.bReadBool(psz_StatusBarHidden, bStatusBarHidden ) )
            ;
        else if ( obs.bReadBool(psz_ToolBarHidden, bToolBarHidden ) )
            ;
        else if ( obs.bReadInteger(psz_LangCtrlHeight, iLangCtlHeight) ) // height of language controls
            ;
        else if ( obs.bReadBool(psz_DontCheckConsistencyWhenEditing, b) )
            m_bCheckConsistencyWhenEditing = FALSE;
        else if ( obs.bReadBool(psz_AutomaticInterlinearUpdate, m_bAutomaticInterlinearUpdate ) ) // 1.1an Save automatic interlinear update setting in prj file
            ;
        else if ( obs.bReadBool(psz_DontAskChangeMarker, m_bDontAskChangeMarker ) ) // 1.6.1ch Read status of don't ask change marker
            ;
        else if ( fntdef.bReadProperties(obs, psz_Markers) ) // marker font info
            {
            // the following three items come before marker font in settings file
            // We should have read them and we need to use them before mainframe
            // gets displayed in bReadDbList()
            Shw_pmainframe()->ShowStatusBar( !bStatusBarHidden );
            Shw_pmainframe()->ShowToolBar( !bToolBarHidden );
            SetLangCtrlHeight( iLangCtlHeight );

            fntdef.iCreateFont( &m_fntMkrs ); // create now for use by bReadDbList()
            bFontCreated = TRUE;
            }
        else if ( m_pDosRunner->bReadProperties(obs, psz_Markers) ) 
               ;
        else if ( pfndset()->bReadProperties(obs) ) // Find... settings
            ;
		else if ( obs.bReadInteger(psz_AmbigBoxLeft, iAmbigLeft) ) // 1.2mj
			;
		else if ( obs.bReadInteger(psz_AmbigBoxTop, iAmbigTop) ) // 1.2mj
			;
		else if ( obs.bUnrecognizedSettingsInfo( psz_ShProjectSettings, m_sUnrecognizedSettingsInfo  ) ) // 1.0cp Store any unrecognized settings info so as not to lose it
			;
        else if ( obs.bEnd(psz_ShProjectSettings) )
            break;
        }
    
    if (!bFontCreated)
        {
        fntdef.iCreateFont( &m_fntMkrs ); // create the default font
        SetLangCtrlHeight( eNormalLangCtrl );
        }

    if ( bNoAutoWrap )
        m_bAutoWrap = FALSE;
    if ( bNoAutoSave )
        m_bAutoSave = FALSE;
    if ( bExerciseNoSave )
        m_bExerciseNoSave = TRUE;
    if ( bNoBackup ) // 1.5.3p 
        m_bNoBackup = TRUE; // 1.5.3p 
    if ( bSaveWithoutNewlines ) // 1.5.4b 
        m_bSaveWithoutNewlines = TRUE; // 1.5.4b 
	m_rectAmbig.left = iAmbigLeft; // 1.2mj
	m_rectAmbig.top = iAmbigTop; // 1.2mj
	m_rectAmbig.right = 0; // 1.2mj
	m_rectAmbig.bottom = 0; // 1.2mj
}

#ifdef _DEBUG
void CProject::AssertValidity() const
{
    // markup types and language encodings
//    Shw_pGlobal->AssertValid();
    m_plngset->AssertValid();
    m_ptypset->AssertValid();
}
#endif  // _DEBUG

BOOL CProject::bProjectMoved() // Return true if project moved
{
    Str8 sPrevProjPath = Shw_pProject()->pszPrevPath(); //path where the prj was
    Str8 sProjPath = Shw_pProject()->pszPath(); //path where the prj is now opened
    return !bEquivFilePath( sGetDirPath( sPrevProjPath ), sGetDirPath( sProjPath ) );
}

void CutOffLastFolder( Str8& s ) // 5.99v Cut off last folder from a path
	{
	int iLen = s.GetLength();
	int iLoc = s.ReverseFind( '\\' ); // Find last slash
	if ( iLoc == ( iLen - 1 ) ) // If it ends with slash, cut it off and find previous slash
		{
		Str8 sShortenedPath = s.Left( iLoc ); // Cut off last slash
		iLoc = sShortenedPath.ReverseFind( '\\' ); // Find previous slash
		if ( iLoc != -1 ) // If previous slash found, cut of all after it
			{
			sShortenedPath = s.Left( iLoc + 1 );
			s = sShortenedPath; // Return shortened path
			}
		}
	}

void CProject::UpdatePath( Str8& sPathToUpdate ) 
// Update db path from old project path to new, if applicable
{
    // 1998-08-05 MRP: *** Temporary fix for assertion Line 353 in Tools.cpp ***
    // CLookupProc::UpdatePaths() in interlin.h calls CProject::UpdatePath( m_sCCT )
    // even if m_sCCT is empty (i.e. no CCT is selected). This is a problem
    // when sPrevProjPath is also empty, which it will be for pre-version 4 projects.
    // *** TO DO After the 4.0 release:
    // 1. Add an if ( !m_sCCT.IsEmpty ) condition in interlin.h
    // 2. Analyze whether there are other places where this function can be called
    // with an empty file path, e.g. change the return to an assertion.
    if ( sPathToUpdate.IsEmpty() )
        return;
        
    if (  bProjectMoved() && Shw_papp()->bOpeningProject() )
        {
        Str8 sPrevProjPath = Shw_pProject()->pszPrevPath(); //path where the prj was
		sPrevProjPath = sGetDirPath( sPrevProjPath ); // 5.98m Get folder part of path
        Str8 sProjPath = Shw_pProject()->pszPath(); //path where the prj is now opened
		sProjPath = sGetDirPath( sProjPath);
		if ( sPathToUpdate[1] == ':' && islower( sPathToUpdate[0] ) ) // 5.98m If lower case device name, match lower case
			{
			if ( sPrevProjPath[1] == ':' && isupper( sPrevProjPath[0] ) )
				{
				char cDevice = sPrevProjPath[0]; // Remember drive letter
				cDevice = tolower( cDevice ); // 1.4gn Fix problem that won't compile in Str8
				Str8 sDevice( cDevice ); // Make lower case 
				sPrevProjPath = sPrevProjPath.Mid( 1 ); // Cut off drive letter
				sPrevProjPath = sDevice + sPrevProjPath;
				}
			}
		if ( sPathToUpdate.Find( sPrevProjPath ) == 0 )
			sPathToUpdate.Replace( sPrevProjPath, sProjPath ); // 5.98m Replace upper part of path
		else // 5.99v If path is one up from settings, update it
			{
			CutOffLastFolder( sPrevProjPath ); // Cut off last folder from prev and current paths
			CutOffLastFolder( sProjPath );
			if ( sPrevProjPath.GetLength() > 4 ) // 1.3dc Fix bug of not correctly updating path if project moved
				sPathToUpdate.Replace( sPrevProjPath, sProjPath ); // Replace upper part of path
			}
        }
}

Str8 CProject::sSettingsDirLabel() const
{
    Str8 sDirPath = m_sSettingsDirPath;
#ifdef _MAC
    const char chDirSeparator = ':';
#else
    const char chDirSeparator = '\\';
#endif
    int len = sDirPath.GetLength();
    if ( len != 0 && sDirPath.GetChar(len - 1) == chDirSeparator )
        sDirPath = sDirPath.Left(len - 1);
    Str8 sLabel = _("Settings folder:"); // 1.5.0fg  // 1.5.0ja Make 'Settings folder' translatable
	sLabel = sLabel + " " + sDirPath; // 1.5.0fg 
    return sLabel;
}

// 2000-08-29 TLB & MRP : to avoid CFindComboBox accessing freed memory, provide
// access to pftxlst via project.
CFindTextList* CProject::pftxlst()
{
    ASSERT(m_pfndset);
    ASSERT(!m_bClosing);
    return m_pfndset->pftxlst();
}

CProject* Shw_pProject()
{
    return ((CShwApp*)AfxGetApp())->pProject(); // return currently open project
}

CDatabaseTypeSet* Shw_ptypset()
{
    return Shw_pProject()->ptypset();
}

CLangEncSet* Shw_plngset()
{
    return Shw_pProject()->plngset();
}

CCorpusSet* Shw_pcorset()
{
    return Shw_pProject()->pcorset();
}

void CProject::ViewMarkerFont() // let user set font used to display markers in record view
{
    CFontDef fntdef( &m_fntMkrs );
    if ( fntdef.bModalView_Properties( FALSE ) )
        {
        fntdef.iCreateFont( &m_fntMkrs );
        Shw_UpdateAllViews(NULL);
        }
}


BOOL CProject::bOpenMovedFile(Str8* psPath)
{
    ASSERT( psPath );
    Str8 sMovedPath;
    Str8 sOriginalPath = *psPath;

    // If the project has moved, try new project place for file *** Later check for whether it was in old project place
    if ( !m_bMovedPath && m_sProjectPath != m_sPrevProjectPath )
        m_sMovedFileDirPath = sGetDirPath( m_sProjectPath );

    // If the user has already chosen a directory to open a moved file,
    // then attempt to open this file in that same directory.   
    Str8 sFileNameExt = sGetFileName(*psPath, TRUE);
    if ( !m_sMovedFileDirPath.IsEmpty() )
        {
        sMovedPath = sPath(m_sMovedFileDirPath, sFileNameExt);
        if ( bFileExists(sMovedPath) )  // If the file exists
            {
            *psPath = sMovedPath;
            m_bMovedPath = TRUE;
            return TRUE;
            }
        }

    // 08-12-1997
    // It could be, that the path is a Mac-path on the PC or 
    // a PC-path on the Mac.
    // The next funktions (bPathFromOwnSystem) detect and handle it
    if (!bPathFromOwnSystem(psPath))
        {
        if (!m_bMovedPath) 
            {
            Str8 sMessage;
			sMessage = sMessage + _("Cannot find file:") + " " + sOriginalPath; // 1.5.0fd 
            MessageBeep(0);
            AfxMessageBox(sMessage);
            }
        }
    else if (!m_bMovedPath)
        {
        Str8 sMessage;
		sMessage = sMessage + _("Cannot find file:") + " " + sOriginalPath; // 1.5.0fd 
        MessageBeep(0);
        // 1999-04-01 TLB: Had to use AfxGetMainWnd and MessageBox instead of AfxMessageBox to avoid crash when no child windows are open
        AfxGetMainWnd()->MessageBox( swUTF16( sMessage ), NULL, MB_ICONINFORMATION); // 1.4qpr  // 1.4qzhz Fix bug (1.4qpr) of crash on not find file message
        }
    m_bMovedPath = TRUE;

    // Let the user choose the directory to which this file has been moved.
    long lFlags = (OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST);
	Str8 sFileFilter = _("All files"); // 1.5.0fz 
	sFileFilter += " (*.*)|*.*||"; // 1.5.0fz 
	Str8 sPath; // 1.5.8h 
	if ( !Shw_papp()->bUseCrashFixFileEditDlg() ) // 1.5.8h 
		{
		CFileDialog dlg(TRUE, NULL, swUTF16( sFileNameExt ), lFlags, swUTF16( sFileFilter ), AfxGetMainWnd()); // 1.4qrd // 1.5.8h 
		dlg.m_ofn.lpstrTitle = _T(" "); // 1.4qzhs Fix bad letters in file browse box title
		dlg.SetHelpID(IDD_FILE_HASMOVED);
		if ( iDlgDoModalSetReg( &dlg ) != IDOK )
			return FALSE;
		sPath = sUTF8( dlg.GetPathName() ); // 1.4qra
		}
	else // 1.5.8h 
		if ( !bCrashFixBrowseForFileName(sPath, sFileNameExt) ) // 1.5.8h 
			return FALSE; // 1.5.8h 
//	if ( !bAllASCIIComplain( sPath ) ) // 1.4vze  // 1.5.9ka Allow Unicode file names
//		return FALSE; // 1.4vze  // 1.5.9ka Allow Unicode file names
    sMovedPath =  sPath; // 1.4qra // 1.5.8h 
    m_sMovedFileDirPath = sGetDirPath(sMovedPath);  // Remember the directory
    *psPath = sMovedPath;
    return TRUE;
}


/*
 *   CProject::bDoRunDos - bring up dialog to run a dos command.
 */
BOOL CProject::bDoRunDos()
{
    if (!m_pDosRunner) {
        return FALSE;
    }
    m_pDosRunner->DoModal();           
    
    return m_pDosRunner->DidACommand();
}


// 08-12-1997
BOOL CProject::bPathFromOwnSystem(Str8* psPath)
{

  // Mac and PC uses different Symbols in their path
#ifdef _MAC
    const char csep = '\\';
#else
    const char csep = ':';
#endif
    
    int item = 0; 

    // Search for the first ':'
    while ((item != psPath->GetLength()) &&(psPath->GetChar(item) != ':') ) 
        item++;

    if (item == psPath->GetLength() )
        // There is no ':' -> we can't say anything, so step further in the program
        return TRUE;
    
#ifdef _MAC
    // If the first ":" is not followed by an "\" -> return without changing path
    // If we are on the end of the sting, we can't say anything -> go ahead in the program
    if ( (item+1) == psPath->GetLength() || psPath->GetChar(item+1) != '\\' )
        return TRUE;
#else
    // If the first ":" is followed by an "\" -> return without changing path
    // If we are on the end of the sting, we can't say anything -> go ahead in the program
    if ( (item + 1) == psPath->GetLength() || psPath->GetChar(item+1) == '\\' )
        return TRUE;
#endif

    //psPath from other System -> change psPath (return only the file name)
    //Search for the last \ in String
    item = (psPath->GetLength()) - 1;
    while (psPath->GetChar(item) != csep ) 
        item--;
    //Extract filename
    *psPath= psPath->Right(psPath->GetLength() - item -1);   
    return FALSE;
}

//-----------------------------------------------------------------------------
//Implementation of CNoProjectDlg
//08-26-1997

CNoProjectDlg::CNoProjectDlg(int* iSelected) :
    // 1999-04-01 TLB: Added AfxGetMainWnd to get non-NULL parent (avoids crash)
                           CDialog(CNoProjectDlg::IDD, AfxGetMainWnd()),
                           m_iSelected(0)                           
{
    //{{AFX_DATA_INIT(CNoProjectDlg)
    m_piSelected=iSelected;
    //}}AFX_DATA_INIT
}

BOOL CNoProjectDlg::OnInitDialog()
{
    BOOL bTemp = CDialog::OnInitDialog();
    return bTemp;  // return TRUE  unless you set the focus to a control
}


void CNoProjectDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CNoProjectDlg)    
    DDX_Radio(pDX, IDC_No_Project_Selection, m_iSelected);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CNoProjectDlg,CDialog)
    //{{AFX_MSG_MAP(CNoProjectDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CNoProjectDlg::OnOK()
{
    //TODO: Add extra validation here
    CDialog::OnOK();
    *m_piSelected=m_iSelected;
}

// --------------------------------------------------------------------------

class CSettingsFile : public CDblListEl  // Hungarian: sef
{
private:
    Str8 m_sFileName;
    Str8 m_sName;
    Str8 m_sVersion;
    
    friend class CSettingsFileList;
    CSettingsFile(const Str8& sFileName, const Str8& sName,
            const Str8& sVersion);

public:
    const Str8& sFileName() const { return m_sFileName; }
    const Str8& sName() const { return m_sName; }
    const Str8& sVersion() const { return m_sVersion; }
};  // class CSettingsFile

class CSettingsFileList : public CDblList  // Hungarian: seflst
{
public:
    void AddSettingsFile(const Str8& sFileName, const Str8& sName,
            const Str8& sVersion);
};
                         

// --------------------------------------------------------------------------

class CSettingsFileListBox : public CDblListListBox  // Hungarian: lbo
{
public:
    CSettingsFileListBox(CSettingsFileList* pseflst);

    void InitListBox();
    
protected:
    int m_xFileName;
    int m_xName;
    int m_xVersion;
    
    virtual void InitLabels();
    virtual void DrawElement(CDC& cDC, const RECT& rcItem,
            const CDblListEl *pel);
};  // class CSettingsFileListBox


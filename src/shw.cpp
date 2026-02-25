// shw.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "toolbox.h"
#include "shw.h"

#include "mainfrm.h"
#include "shwdoc.h"
#include "shwview.h"
#include "shwdtplt.h"
#include <fstream>
#include "tools.h"  // sPath
#include "font.h"
#include "find_d.h"
#include "status.h"
#include "project.h"
#if UseCct
#include "ChangeTable.h"  // class ChangeTable
#endif
#include "doclist.h"
#include "obstream.h"  // Object_istream, Object_ostream
#include "nlstream.h"
#include "wrdlst_d.h"
#include "corpus.h"
#include "progress.h"
#include "rundos.h"
#include "help_ids.h" // HT_VERSION_INFORMATION
#include "VersionDlg.h"
#include "alan.h" // 1.5.8h 

#ifdef _MAC
#include "resmac.h" // Mac specific resources
#else
#include "respc.h"
#include <shlobj.h>
#endif

#include <direct.h> // _mkdir
#include <sstream>

#ifdef _MAC
#include <mprof.h>
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CShwApp

BEGIN_MESSAGE_MAP(CShwApp, CWinApp)
    //{{AFX_MSG_MAP(CShwApp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_COMMAND(ID_FILE_SAVE_ALL, OnFileSaveAll)
    ON_COMMAND(ID_FILE_SAVE_SETTINGS, OnFileSaveSettings)
    ON_COMMAND(ID_VIEW_MARKER_FONT, OnViewMarkerFont)
    ON_COMMAND(ID_PROJECT_LANGUAGE_ENCODINGS, OnFileLanguageEncodings)
    ON_COMMAND(ID_PROJECT_DATABASE_TYPES, OnFileDatabaseTypes)
    ON_COMMAND(ID_PROJECT_TEXT_CORPUS, OnFileTextCorpus)
    ON_UPDATE_COMMAND_UI(ID_FILE_NEW, OnUpdateFileNew)
    ON_UPDATE_COMMAND_UI(ID_FILE_OPEN, OnUpdateFileOpen)
    ON_UPDATE_COMMAND_UI(ID_PROJECT_LANGUAGE_ENCODINGS, OnUpdateFileLanguageEncodings)
    ON_UPDATE_COMMAND_UI(ID_PROJECT_DATABASE_TYPES, OnUpdateFileDatabaseTypes)
    ON_UPDATE_COMMAND_UI(ID_PROJECT_TEXT_CORPUS, OnUpdateFileTextCorpus)
    ON_COMMAND(ID_PROJECT_NEW, OnProjectNew)
    ON_COMMAND(ID_PROJECT_OPEN, OnProjectOpen)
    ON_COMMAND(ID_PROJECT_CLOSE, OnProjectClose)
    ON_UPDATE_COMMAND_UI(ID_PROJECT_CLOSE, OnUpdateProjectClose)
    ON_COMMAND(ID_PROJECT_SAVE, OnProjectSave)
    ON_UPDATE_COMMAND_UI(ID_PROJECT_SAVE, OnUpdateProjectSave)
    ON_COMMAND(ID_PROJECT_SAVEAS, OnProjectSaveas)
    ON_UPDATE_COMMAND_UI(ID_PROJECT_SAVEAS, OnUpdateProjectSaveas)
    ON_UPDATE_COMMAND_UI(ID_FILE_MRU_FILE1, OnUpdateRecentFileMenu)
    ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
    ON_COMMAND(ID_Project_AutoSave, OnAutoSave)
    ON_UPDATE_COMMAND_UI(ID_Project_AutoSave, OnUpdateAutoSave)
    ON_COMMAND(ID_Project_LockProject, OnLockProject)
    ON_UPDATE_COMMAND_UI(ID_Project_LockProject, OnUpdateLockProject)
    ON_COMMAND(ID_VIEW_LARGE_CONTROLS, OnViewLargeControls)
    ON_UPDATE_COMMAND_UI(ID_VIEW_LARGE_CONTROLS, OnUpdateViewLargeControls)
	ON_COMMAND(ID_TOOLS_CHECK_CONSISTENCY_WHEN_EDITING, OnToolsCheckConsistencyWhenEditing)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_CHECK_CONSISTENCY_WHEN_EDITING, OnUpdateToolsCheckConsistencyWhenEditing)
	ON_COMMAND(ID_TOOLS_AUTOMATICINTERLINEARUPDATE, OnToolsAutomaticInterlinearUpdate)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_AUTOMATICINTERLINEARUPDATE, OnUpdateToolsAutomaticInterlinearUpdate)
    ON_COMMAND(ID_TOOLS_WORDLIST, OnToolsWordList)
    ON_COMMAND(ID_TOOLS_CONCORDANCE, OnToolsConcordance)
    //}}AFX_MSG_MAP
    // Standard file based document commands
    ON_COMMAND(ID_FILE_NEW, CShwApp::OnFileNew)
    ON_COMMAND(ID_FILE_OPEN, CShwApp::OnFileOpen)
    // Standard print setup command
    ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
#ifndef _MAC
    ON_COMMAND(ID_DORUNDOS, OnRunDos)
#endif
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CShwApp construction

CShwApp::CShwApp()
{
    m_pProject = NULL;
	m_bOpeningProject = FALSE; // 12md Initialize
	m_bWriteProtectedProject = FALSE; // 12md Initialize
    m_bClosing = FALSE;
	m_bWriteProtectedSettings = FALSE; // 1.0df Fix bug of not noticing write protected typ or lng file if prj file is not write protected
	m_bProjectOpenedPortable = FALSE; // 1.5.0de Remember if project opened in portable way (from program folder, don't save in registry)
    m_bClosingProjectOrMainFrame = FALSE;
    m_bAnotherInstanceAlreadyRunning = FALSE;  // 1998-05-11 MRP
	m_pviewJumpedFrom = NULL; // 12md Initialize
	m_pviewJumpedFromWordParse = NULL; // 12md Initialize // 1.6.4v 
	m_bNoHelpMessageGiven = FALSE; // 1.4ppa Remember if no help message already given
	m_bUseCrashFixFileEditDlg = FALSE; // 1.5.8h If true, use crash fix file navigation dialog box // 1.5.8wa Test reg
}

// CShwApp destruction

CShwApp::~CShwApp()
{
    if ( bProjectOpen() )
        delete m_pProject;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CShwApp object

CShwApp NEAR theApp;

/////////////////////////////////////////////////////////////////////////////
// CShwApp initialization

// SHW.INI section of registry and entries (NOTE: .INI files do not distinguish case but registry )
static const char* psz_ShSection = "The Linguist's Shoebox";
static const char* psz_ProjectLastClosed = "ToolboxProjectLastClosed"; // 1.0ce Make Toolbox project last closed separate from Shoebox project last closed
static const char* psz_ShoeboxProjectLastClosed = "ProjectLastClosed"; // 1.0ce Make Toolbox project last closed separate from Shoebox project last closed
static const char* psz_LastWndTitle = "LastWndTitle";
static const char* psz_DefaultSettingsFolder = "DefaultSettingsFolder";
static const char* psz_DefaultSettingsFolder2 = "DefaultSettingsFolder2";
static const char* psz_AppPath = "AppPath";
static const char* psz_AppPath2 = "AppPath2";
#ifdef _MAC
static const char* psz_IniFile = "Shoebox Preferences";
#else
static const char* psz_IniFile = "shoebox.ini";  // 1998-03-18 MRP
static Str8 s_sIniPath;
static const char* psz_HelpFile = "Toolbox.hlp";
static Str8 s_sHelpFilePath;
#endif


// First number is major version. Tenths represent a full external test release.
// Letters represent a minor internal or external test with limited distribution.
// static char s_pszProgramVersion[] = "1.4gzp Apr 2005"; // Also in history in VersionDlg.cpp // 1.4gzp Move version number string to VersDlg.cpp

static char s_pszSettingsVersion[] = "5.0"; // 1.0cd Change settings version to 1.0 as requested, changed back to 5.0 at 1.0ch

extern Str8 g_sVersion;  // 1997-08-05

HBRUSH CShwApp::GetAppDialogBrush(CDC* pDC)
{
    pDC->SetBkColor(g_clrDialogBk);
    pDC->SetTextColor(g_clrDialogText);
    static CBrush brushBk(g_clrDialogBk);
    return brushBk;
}

BOOL CShwApp::InitInstance()
{
    Str8 sPlatform = "";
#ifdef MacWine // 1.5.1kb Make different version for Mac Windows emulator
#ifdef WineFix
    sPlatform = " for Mac Wine Windows emulators"; // 1.5.9rd Make different version for Mac Wine Windows emulator
#else
    sPlatform = " for Mac Windows emulators"; // 1.5.1kb Make different version for Mac Windows emulator
#endif
#else
#ifdef WineFix // 1.5.9rd Make different version for Linux Wine Windows emulator
    sPlatform = " for Linux Wine Windows emulator"; // 1.5.9rd Make different version for Mac Windows emulator
#endif
#endif
	g_sVersion += sPlatform; // 1.5.9rd 

// 1.2ac Allow multiple instances of Toolbox to run at the same time
	m_sAppPath = sGetDirPath(  sUTF8( m_pszHelpFilePath ) ); // 1.5.0dd Add app path to app class, get it from help path
    s_sHelpFilePath = sGetDirPath(  sUTF8( m_pszHelpFilePath ) ) + psz_HelpFile; // force correct help filename // 1.4qvu Upgrade sGetDirPath for Unicode build
	free((void*)m_pszHelpFilePath);
	m_pszHelpFilePath = _tcsdup( swUTF16( s_sHelpFilePath) ); // 1.4qyj

	// if this is truly needed:
    // SetDialogBkColor( ::GetSysColor(COLOR_BTNFACE), ::GetSysColor(COLOR_BTNTEXT) );
	// Then in each dialog’s OnCtlColor() (by adding ON_WM_CTLCOLOR() in their BEGIN_MESSAGE_MAP(CMyDialog, CDialog)...END_MESSAGE_MAP())
	//if (nCtlColor == CTLCOLOR_DLG)
	//	return GetAppDialogBrush(pDC);

    // Register the application's document templates.  Document templates
    //  serve as the connection between documents, frame windows and views.
    CShwMultiDocTemplate* pDocTemplate;
    pDocTemplate = new CShwMultiDocTemplate(
        IDR_SHWTYPE,
        RUNTIME_CLASS(CShwDoc),
        RUNTIME_CLASS(CShwViewFrame),
        RUNTIME_CLASS(CShwView));
    AddDocTemplate(pDocTemplate);

    // Create main MDI Frame window
    CMainFrame* pMainFrame = new CMainFrame;
    if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
        return FALSE;
    m_pMainWnd = pMainFrame;  // Must delete before any return FALSE

    // register a new 'internal' clipboard format for internal (and with Paratext) c/c/p  operations
    m_uiTlbxIntrlClipBrdFormat = RegisterClipboardFormat( swUTF16( "CF_UTF8TEXT") ); // 1.4qxt Upgrade RegisterClipboardFormat for Unicode build
   
	g_clrDialogBk = ::GetSysColor(COLOR_BTNFACE);
	g_clrDialogText = ::GetSysColor(COLOR_BTNTEXT);

#ifndef _MAC
    // Parse command line arguments.
    // On the Mac: see OpenDocumentFile and CreateInitialDocument below.
    Str8 sProjectPath = DetermineWinProjectPath();
    if ( !sProjectPath.IsEmpty() ) // no project on command line or in .ini file
        {
		if ( sProjectPath[0] == '\"' ) // If path has quotes, remove them
			sProjectPath = sProjectPath.Mid( 1, sProjectPath.GetLength() - 2 );
        Str8 sDirPath = sGetDirPath( sProjectPath ); // see if whole path was specified
		if ( !bAllASCIIComplain( sDirPath ) ) // 1.4vze 
			return FALSE; // 1.4vze 
        if ( sDirPath.IsEmpty() )
            {
            Str8 sMsg = _("You must specify the full pathname of the project file:"); // 1.5.0fg 
			sMsg = sMsg + " " + sProjectPath; // 1.5.0fg 
            AfxMessageBox( sMsg, MB_ICONINFORMATION);
            }
        else if ( !bFileExists(sProjectPath) ) // see if directory exists
            {
            Str8 sMsg = _("Cannot find file:"); // 1.5.0fg 
			sMsg = sMsg + " " + sProjectPath; // 1.5.0fg 
            AfxMessageBox( sMsg, MB_ICONINFORMATION);
            }
        else if ( CProject::s_bIsProjectFile( sProjectPath ) )
			{
            BOOL bSucc = bOpenProject( sProjectPath, TRUE, s_pszSettingsVersion ); // 1.4vwa 
			if ( !bSucc ) // 1.4vwa Try to fix bug of sometimes not opening project after crash
				{
//				if ( Shw_papp()->bAnotherInstanceAlreadyRunning() ) // 1.4vwa 
				if ( m_bWriteProtectedProject ) // 1.4ytd If project was write protected, exit this run
					CloseShoebox(); // 1.4vwa 
				}
			}
        }

    pMainFrame->OnUpdateFrameTitle(TRUE); // make sure window title gets written to .ini file
#endif // _MAC

    // The main window has been initialized, so show and update it.
    int nCmdShow = ( m_nCmdShow == SW_SHOWMINNOACTIVE ?
        SW_SHOWMINNOACTIVE :  // [X] Run Minimized in Program Manager icon
        SW_SHOW  // According to project settings (if any)
        );
    pMainFrame->ShowWindow(nCmdShow);
    pMainFrame->UpdateWindow();

	m_bUseCrashFixFileEditDlg = bRegPossibleCrash(); // 1.5.8h If true, use crash fix file navigation dialog box // 1.5.8wa Test reg

#ifndef _MAC
//    if ( !m_pProject && ( m_bWriteProtectedProject || !bOpenOrNewProject() ) ) // 1.2ak If user says another copy of Toolbox may have this project open, exit // 1.4vwa 
//        CloseShoebox();  // 1998-03-16 MRP // 1.4vwa 
#endif

    return TRUE;
}

#ifdef _MAC
// Visual C++ Porting Application to the Macintosh, p. 174
// With Windows, MFC applications commonly have code in InitInstance
// that examines the command line; the application creates a new
// document if the command line is empty, or opens a file if a file
// is given on the command line.

// On the Macintosh, there is no equivalent to the command line.
// Instead, Apple events are used to communicate filename paramenters
// to an application. When a Macintosh application is launched,
// it may be sent an Open Document Apple event with the location
// of the file to open, in which case MFC automatically handles
// this Apple event with the need for any application code.

CDocument* CShwApp::OpenDocumentFile(const char* pszFileName)
{
    Str8 sFullPath = sGetFullPath( pszFileName );
    BOOL bIsProjectFile = CProject::s_bIsProjectFile( sFullPath );

    if ( bProjectOpen() )
        {
        if ( bIsProjectFile )
            // If the user double-clicks a project file,
            // when a project is already open, ignore it.
            return NULL;
        else
            // The ordinary case is opening a database file.
            return CWinApp::OpenDocumentFile( pszFileName );
        }
    
    // The user started Shoebox by double-clicking a project file.
    if ( bIsProjectFile )
        bOpenProject( sFullPath, TRUE, s_pszSettingsVersion );
    else
        AfxMessageBox( _("This is not a valid project file.") );

    // 1998-03-18 MRP: If the file wasn't a valid project,
    // show the No Project Open dialog box, not an "empty Shoebox".
    if ( !m_pProject && !bOpenOrNewProject() )
        CloseShoebox();

    return NULL;
}

// On the Macintosh, MFC also adds the CWinApp::CreateInitialDocument
// member function. If no files are specified when an application is
// launched, this function is called and automatically creates a new
// empty document by default. If your Windows MFC application has code
// in InitInstance that calls OnFileNew when the command line is empty,
// you can remove it, because the default CreateInitialDocument takes
// care of this on the Macintosh. On the other hand, if your Windows
// application does not create an empty document by default, override
// CreateInitialDocument on the Macintosh to prevent an empty document
// from being created automatically.

BOOL CShwApp::CreateInitialDocument()
{
    // The user started Shoebox by double-clicking the application.
    // If possible, reopen the last project they used.
    Str8 sProject = sGetLastProjectFromIni();
    if ( !sProject.IsEmpty() )
        bOpenProject( sProject, TRUE, s_pszSettingsVersion );

    // 1998-03-18 MRP: If that isn't possible,
    // show the No Project Open dialog box, not an "empty Shoebox".
    if ( !m_pProject && !bOpenOrNewProject() )
        CloseShoebox();
    
    return TRUE;
}
#endif

int CShwApp::ExitInstance()
{
    // 1998-05-05 MRP: The program has been leaving the title "Shoebox"
    // in the .ini file when it closes, but that can allow a false match
    // with an Explorer window on a folder named "Shoebox". This prevents
    // Shoebox from running even when there aren't any other instances.
    // Blanking out the item when Shoebox exits avoids the confusion.
    if ( !m_bAnotherInstanceAlreadyRunning )  // 1998-05-11 MRP
        WriteToIni_Title("");
    
    return 0;  // The application's exit code: no errors
}


static Str8 s_sChangeDirPath(Str8 sDirPath)
{
	// Change every single backslash to double for _chdir
#ifdef OLDWAY
	int iSlash = 0;
	while ( TRUE )
		{
		iSlash = sDirPath.Find( '\\', iSlash + 2 );
		if ( iSlash == -1 )
			break;
		sDirPath.Insert( iSlash, '\\' );
		}
#endif
	sDirPath.Replace( "\\", "\\\\" ); // 1.4hef Use Replace to change single backslash to double
    return sDirPath;
}

void CShwApp::OnFileNew() // Set default folder
{
    CWinApp::OnFileNew();
}

void CShwApp::OnFileOpen() // Set default folder
{
	Str8 sPath; // 1.4hed Use CStrg to pass in to MFC function
	CString swPath; // 1.5.8h 
    CShwView* pView = Shw_papp()->pviewActive();
    if ( pView ) // get folder of active window if present
        {
        sPath = sGetDirPath( sUTF8( pView->GetDocument()->GetPathName() ) ); // 1.4qvu
		}
	if ( !Shw_papp()->bUseCrashFixFileEditDlg() ) // 1.5.8h 
		{
		sPath += "*.*"; // 1.1ec Default to path of current directory, from Bob // 1.4qze Correct U path failure
		swPath = swUTF16( sPath ); // 1.4qze Correct U path failure
		SetRegPossibleCrash(); // 1.5.8wa 
		BOOL bSucc = m_pDocManager->DoPromptFileName( swPath, AFX_IDS_OPENFILE, OFN_FILEMUSTEXIST, TRUE, NULL); // 1.5.8wa 
		ClearRegPossibleCrash(); // 1.5.8wa 
		if ( !bSucc ) // 1.4et OK, doesn't use CStrg // 1.4qzc Correct CStrg that called wrong override // 1.4qze
			return; // open cancelled
		sPath = sUTF8( swPath ); // 1.4vzc 
		}
	else // 1.5.8h 
		{
		CCrashFixFileEdit dlg; // 1.5.8h 
		dlg.m_sName = sPath; // 1.5.8h Initialize file name to current name
		if ( dlg.DoModal() != IDOK)
			return; // 1.5.8h 
		sPath =  dlg.m_sName; // 1.5.8h 
		swPath = swUTF16( sPath ); // 1.5.8h 
		}
//	if ( !bAllASCIIComplain( sPath ) ) // 1.4vze  // 1.5.9ka Allow Unicode file names
//		return; // 1.4vze  // 1.5.9ka Allow Unicode file names
    OpenDocumentFile( swPath ); // 1.1ec Default to path of current directory, from Bob // 1.4qzd // 1.4qze
#ifdef DoesntWork // 1.1ec _chdir doesn't do any good, different approach required
        sPath = s_sChangeDirPath(sPath);  // 1998-08-04 MRP
        int i = _chdir( sPath ); // Set current working directory
        }
    CWinApp::OnFileOpen();
#endif // DoesntWork
}

void CShwApp::WriteToIni_Title( const char* pszTitle )
{
    WritePrivateProfileString(
        swUTF16(psz_ShSection),
        swUTF16(psz_LastWndTitle),
        swUTF16(pszTitle),
        swUTF16(s_sIniPath));
}

void CShwApp::WriteToIni_ProjectPath(const char* pszProjectPath, const char* pszKey)
{
    if (!pszKey)
        pszKey = psz_ProjectLastClosed;
    if (m_bProjectOpenedPortable)
        // If project opened from Toolbox.exe folder, don't save in registry.
        return;
    WritePrivateProfileString(
        swUTF16(psz_ShSection),
        swUTF16(pszKey),
        swUTF16(pszProjectPath),
        swUTF16(s_sIniPath)
    );
}

Str8 sGetPrivateProfileString( const char* pszDesiredProfile )
	{
    CString swProfile;
    LPTSTR pswzProfile = swProfile.GetBufferSetLength(4096);
    DWORD dw = ::GetPrivateProfileString(
        swUTF16(psz_ShSection),
        swUTF16(pszDesiredProfile),
        swUTF16(""),
        pswzProfile,
        4096,
        swUTF16(s_sIniPath));
    ASSERT(dw < 4095);
	swProfile.ReleaseBuffer();
    return sUTF8( swProfile );
	}

Str8 CShwApp::sLastWndTitle()
	{
	return sGetPrivateProfileString( psz_LastWndTitle ); // 1.4xgh
	}

BOOL CShwApp::SaveAllModified()
{
    m_bClosing = TRUE;
    m_bClosingProjectOrMainFrame = TRUE;  //08-28-1997
	if ( !bProjectOpen() ) // 1.4sa Fix bug (1.4rac) of crash on open same project second time
		return TRUE; // 1.4sa

	if ( Shw_pProject()->bExerciseNoSave() ) // 1.4rac On exercise, don't save name of project last closed
		return TRUE;
	(void) Shw_app().bSaveAllFiles( true, !Shw_pProject()->bAutoSave() ); // 1.4qzhk Fix U problem of writing emtpy output file

    if ( CWinApp::SaveAllModified() )
        {
        // This is the last chance to save the properties of all the
        // open databases and windows before the framework closes them.
        return TRUE;
        }
    m_bClosing = FALSE;
    m_bClosingProjectOrMainFrame = FALSE;  //08-28-1997
    return FALSE;  // the user cancelled closing Shoebox
}

Str8 CShwApp::DetermineWinProjectPath()
{
    Str8 sProjectPath;

    // Look for standard .prj in folder with program, i.e. portable setup.
    m_bProjectOpenedPortable = FALSE;
    for (const char* name : { "Toolbox.prj", "Toolbox Project.prj" })
    {
        sProjectPath = sAppPath() + name;
        if (bFileExists(sProjectPath))
        {
            m_bProjectOpenedPortable = TRUE;
            return sProjectPath;
        }
    }
    // Keep INI file in APPDATA instead of C:\Windows or the registry.
    TCHAR szAppData[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, szAppData)))
    {
        Str8 folderPath = sUTF8(szAppData) + "\\Toolbox";
        s_sIniPath = folderPath + "\\" + psz_IniFile;
        CreateDirectory(swUTF16(folderPath), nullptr);
    }
    else
    {
        s_sIniPath = sAppPath() + psz_IniFile;
    }

    // Command-line argument
    Str8 sProjectName = sUTF8(m_lpCmdLine);

    // A .SET file contains Shoebox's project/workspace settings.
    // The name of the settings file for this session is:
    // 1. the contents of the command line (e.g. "David"); otherwise
    // 2. the ProjectLastClosed entry in SHW.INI section of registry
    if (sProjectName.IsEmpty()) {
        sProjectName = sGetLastProjectFromIni();
        if (!sProjectName.IsEmpty() && !bFileExists(sProjectName))
        {
            // Path doesn’t exist, clear the cached value
            WriteToIni_ProjectPath("");
            sProjectName = "";
        }
    }
    sProjectPath = sProjectName;
    return sProjectPath;
}

Str8 CShwApp::sGetLastProjectFromIni()
{
	Str8 sProjectLastClosed = sGetPrivateProfileString( psz_ProjectLastClosed ); // 1.4qxk Upgrade GetPrivateProfileString for Unicode build
	if ( sProjectLastClosed == "" ) // 1.0ce If Toolbox last project not found, check for Shoebox last project
		{
		sProjectLastClosed = sGetPrivateProfileString( psz_ShoeboxProjectLastClosed );
        // 1.0ce If using Shoebox last project, clear it so Shoebox won't accidentally open Toolbox project
        WriteToIni_ProjectPath("", psz_ShoeboxProjectLastClosed);
		}
	return sProjectLastClosed;
}

BOOL CShwApp::OnIdle( LONG lCount )
{
    BOOL bMore = CWinApp::OnIdle(lCount);
    return bMore;
}

void CShwApp::WinHelp( DWORD dwData, UINT nCmd ) // 1.4pp Make help check file existence to solve problem on Linux Wine
	{
		AfxMessageBox( _("Help is available on the web or from toolbox@sil.org.") ); // 1.4pp // 1.6.2b Always give help on web message
//	if ( m_bNoHelpMessageGiven ) // 1.4ppa If no help message already given, don't give it again // 1.6.2b 
		return; // 1.4ppa
	if ( !bFileExists(  sUTF8( m_pszHelpFilePath ) ) ) // 1.4pp If help file not found, don't try to call help // 1.4qym
		{
		m_bNoHelpMessageGiven = TRUE; // 1.4ppa
//		AfxMessageBox( _("Sorry, help is not available.") ); // 1.4pp // 1.6.2b 
		}
	else
		CWinApp::WinHelp( dwData, nCmd ); // 1.4pp If file OK, call standard help
	}


BOOL CShwApp::bValidateAllViews()
{
    CMDIChildWnd* pwnd = Shw_pmainframe()->MDIGetActive(); // remember active child window

    CDocList doclst; // list of currently open docs
    for ( CShwDoc* pdoc = doclst.pdocFirst(); pdoc; pdoc = doclst.pdocNext() )
        if ( !pdoc->bValidateViews(FALSE) ) // validate all views of this doc, FALSE says don't autosave at each one
            return FALSE;

    if ( pwnd != Shw_pmainframe()->MDIGetActive() ) // was another view activated because of validation problem?
        Shw_pmainframe()->MDIActivate(pwnd); // re-activate current view

    return TRUE; // all views validated
}

void CShwApp::SendUpdateToAllViews(LPARAM lHint, CObject* pHint)
{
    CDocList doclst; // list of currently open docs
    for ( CShwDoc* pdoc = doclst.pdocFirst(); pdoc; pdoc = doclst.pdocNext() )
        pdoc->UpdateAllViews(NULL, lHint, pHint); // Update all the views of this document.
}


CShwView* CShwApp::pviewActive()
{
    CMDIFrameWnd* pwndMainFrame = (CMDIFrameWnd*)m_pMainWnd;
    ASSERT( pwndMainFrame->IsKindOf(RUNTIME_CLASS(CMDIFrameWnd)) );
    CMDIChildWnd* pwndActiveChildFrame = pwndMainFrame->MDIGetActive();
    
    CShwView* pview = ( pwndActiveChildFrame ?
        CShwView::s_pviewActiveChild(pwndActiveChildFrame) :
        NULL
        );
        
    return pview;
}

CShwView* CShwApp::pviewTop()
{
    return pviewEnd(GW_HWNDPREV);
}

CShwView* CShwApp::pviewBottom()
{
    return pviewEnd(GW_HWNDNEXT);
}

CShwView* CShwApp::pviewAbove(CShwView* pviewCur)
{
    return pviewNeighbor(pviewCur, GW_HWNDPREV);  
}

CShwView* CShwApp::pviewBelow(CShwView* pviewCur)
{
    return pviewNeighbor(pviewCur, GW_HWNDNEXT);
}

void CShwApp::SetZ()
{
    int z = 0;
    CShwView* pview = pviewBottom();
    for ( ; pview; pview = pviewAbove(pview) )
        pview->SetZ(z++);
}


CShwView* CShwApp::pviewEnd(UINT uNextOrPrev)
{
    CShwView* pview = pviewActive();
    CShwView* pviewN = ( pview ? pviewNeighbor(pview, uNextOrPrev) : NULL);
    for ( ; pviewN; pviewN = pviewNeighbor(pviewN, uNextOrPrev) )
        pview = pviewN;
        
    return pview;
}

CShwView* CShwApp::pviewNeighbor(CShwView* pviewCur, UINT uNextOrPrev)
{
    ASSERT( pviewCur );
    CMDIChildWnd* pwndChildFrame = pviewCur->pwndChildFrame();

    ASSERT( uNextOrPrev == GW_HWNDNEXT || uNextOrPrev == GW_HWNDPREV ); 
    CWnd* pwndNext = pwndChildFrame->GetNextWindow(uNextOrPrev);
    for ( ; pwndNext; pwndNext = pwndNext->GetNextWindow(uNextOrPrev) )
        if ( pwndNext->IsKindOf(RUNTIME_CLASS(CMDIChildWnd)) )
            // NOTE: This test is to skip minimized icon windows,
            // because they are *in addition to* the corresponding
            // MDI child frame window which contains the view object.
            break;
            
    CShwView* pview = ( pwndNext ?
        CShwView::s_pviewActiveChild((CMDIChildWnd*)pwndNext) :
        NULL
        );
        
    return pview;
}


static const char* psz_mrulst = "mrulst";
static const char* psz_mru = "mru";
static const char* psz_dblst = "dblst";

CProject* CShwApp::pProject() const // 1.4ytf Try to fix bug of false assert on init project
		{ 
		ASSERT( bProjectOpen() );
		return m_pProject; 
		}

#ifdef prjWritefstream // 1.6.4aa 
void CShwApp::WriteDbList(Object_ofstream& obs)
#else
void CShwApp::WriteDbList(Object_ostream& obs)
#endif
{
    // The open databases and windows
    SetZ();  // Set the current z-order of all views
    obs.WriteBeginMarker(psz_dblst);
    obs.WriteNewline();

    CDocList doclst; // get list of currently open documents
    for ( CShwDoc* pdoc = doclst.pdocFirst(); pdoc; pdoc = doclst.pdocNext() )
        pdoc->WriteProperties(obs);

    obs.WriteEndMarker(psz_dblst);
}

#ifdef prjWritefstream // 1.6.4aa 
void CShwApp::WriteMRUList(Object_ofstream& obs)
#else
void CShwApp::WriteMRUList(Object_ostream& obs)
#endif
{
}

// read the open databases and windows
BOOL CShwApp::bReadDbList(Object_istream& obs)
{
    if ( !obs.bReadBeginMarker(psz_dblst) )
        return FALSE;
        
    while ( !obs.bAtEnd() )
        {
        if ( CShwDoc::s_bReadProperties(obs) )
            ;
        else if ( obs.bEnd(psz_dblst) )
            break;
        }
    
    // Now that documents and views have been opened, activate the top window
    //, and maximize it if it had been when Shoebox was closed.
    CShwView* pviewT = pviewTop();
    if ( pviewT )
        pviewT->ShowInitialTopState();
    // 1996-09-05 MRP: I used to assert in the _DEBUG version
    // that the z-order was correct, but it gets changed if there are
    // aliased files (if the project has been moved or upgraded)

    return TRUE;
}

// Read in Most Recently Used file list on the File menu
BOOL CShwApp::bReadMRUList(Object_istream& obs)
{
    if ( !obs.bReadBeginMarker(psz_mrulst) )
        return FALSE;

    while ( !obs.bAtEnd() )
        {
        Str8 sFile;
        if ( obs.bReadString(psz_mru, sFile) )
#ifdef BJY_5_23_96
            AddToRecentFileList(sFile);
#else
            ;
#endif
        else if ( obs.bEnd(psz_mrulst) )
            break;
        }

    return TRUE;
}

#ifdef _DEBUG
void CShwApp::AssertValidity() const
{
    if ( bProjectOpen() )
        m_pProject->AssertValidity();

    // documents and views    
    AssertValid();  // MFC base class

    CDocList doclst; // list of currently open docs
    for ( CShwDoc* pdoc = doclst.pdocFirst(); pdoc; pdoc = doclst.pdocNext() )
        pdoc->AssertValidity();
}
#endif  // _DEBUG


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
    CAboutDlg();

// Dialog Data
    //{{AFX_DATA(CAboutDlg)
    enum { IDD = IDD_ABOUTBOX };
    //}}AFX_DATA

// Implementation
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //{{AFX_MSG(CAboutDlg)
	afx_msg void OnVersionInformation();
    //}}AFX_MSG
	BOOL OnInitDialog();
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
    //{{AFX_DATA_INIT(CAboutDlg)
    //}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAboutDlg)
//    DDX_Txt(pDX, IDC_Version, g_sVersion);
    //}}AFX_DATA_MAP
}

BOOL CAboutDlg::OnInitDialog()
	{
    SetDlgItemText( IDC_Version, swUTF16( g_sVersion ) ); // 1.4qpv
	return TRUE;
	}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
    //{{AFX_MSG_MAP(CAboutDlg)
    ON_BN_CLICKED(IDC_btnVersionInfo, OnVersionInformation)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CAboutDlg::OnVersionInformation()
{
	CVersionDlg dlg;
	dlg.DoModal();
}

// App command to run the dialog
void CShwApp::OnAppAbout()
{
    CAboutDlg aboutDlg;
    aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CShwApp commands

void Shw_UpdateAllViews(CHint* pHint)
{
    Shw_papp()->SendUpdateToAllViews(0L, pHint);
}

CShwApp* Shw_papp()
{
    return (CShwApp*)AfxGetApp();
}

CShwApp& Shw_app()
{
    return (CShwApp&)*AfxGetApp();
}

CMainFrame* Shw_pmainframe()
{
    CMainFrame* pMainFrame = (CMainFrame*)Shw_papp()->m_pMainWnd;
    ASSERT( pMainFrame->IsKindOf(RUNTIME_CLASS(CMainFrame)) );
    return pMainFrame;
}

#ifdef ToolbarFindCombo // 1.4yq
CFindComboBox* Shw_pcboFind()
{
    return Shw_pmainframe()->pcboFind();
}
#endif // ToolbarFindCombo // 1.4yq

void Shw_Update(CUpdate& up)
{
    Shw_papp()->Update(up);
}

void CShwApp::Update(CUpdate& up)
{
    Shw_plngset()->Update(up);
    Shw_ptypset()->Update(up);
    
    CDocList doclst; // list of currently open docs
    for ( CShwDoc* pdoc = doclst.pdocFirst(); pdoc; pdoc = doclst.pdocNext() )
        pdoc->Update(up); // Update all documents (and their views)
}

CShwDoc* CShwApp::pdoc(const char* pszDatabasePath)
{
     char pszFullDatabasePath[_MAX_PATH];
     _fullpath(pszFullDatabasePath, pszDatabasePath,
        sizeof(pszFullDatabasePath));
    
    CDocList doclst; // list of currently open docs
    for ( CShwDoc* pdoc = doclst.pdocFirst(); pdoc; pdoc = doclst.pdocNext() )
        {
        char pszFullDocumentPath[_MAX_PATH];
        _fullpath(pszFullDocumentPath,  sUTF8( pdoc->GetPathName() ), sizeof(pszFullDocumentPath)); // 1.4qyn
        // NOTE: To be safe, we match the full paths
        if ( !_stricmp(pszFullDocumentPath, pszFullDatabasePath) )
            return pdoc;
        }
        
    return NULL;
}

CShwDoc* CShwApp::pdocOpen(const char* pszDatabasePath) // Same as pdoc, but tries to open if not open, returns null if path cannot be opened
{
    CShwDoc* pdoc = CShwApp::pdoc( pszDatabasePath );
    if ( !pdoc )
        {
        if ( bFileExists( sUTF8( pszDatabasePath ) ) )
            {
            CShwView* pviewPrev = Shw_papp()->pviewActive(); // Remember previous active view
			Str8 sDatabasePath = pszDatabasePath; // 1.4qxe
            pdoc = (CShwDoc*)OpenDocumentFile( swUTF16( sDatabasePath ) ); // Open document, moves focus to a new view on doc // 1.4qxe
            if ( pdoc )
                {
                CShwView* pview = Shw_papp()->pviewActive(); // Get view
                ASSERT( pview );
                ASSERT( pview->GetDocument() == pdoc );
                CMDIChildWnd* pwnd = pview->pwndChildFrame(); // Get frame window of view
                pwnd->ShowWindow( SW_SHOWMINIMIZED ); // Minimize view
                if ( pviewPrev )
                    {
                    // Restore previous active MDI child window
                    CMDIChildWnd* pwndPrev = pviewPrev->pwndChildFrame(); // Get frame window of view
                    CMDIFrameWnd* pwndMainFrame = (CMDIFrameWnd*)Shw_papp()->m_pMainWnd; // Get main frame
                    ASSERT( pwndMainFrame->IsKindOf(RUNTIME_CLASS(CMDIFrameWnd)) ); 
                    pwndMainFrame->MDIActivate(pwndPrev); // Move focus to previous active view
                    }
                }
            }
        }
    return pdoc;    
}   

#ifdef _DEBUG
void Shw_AssertValid()
{
    theApp.AssertValidity();
}
#endif  // _DEBUG

// If the file has been changed externally, reload it and refresh the views.
// This happens even if they have made changes to the file internally as well.
// But it is fairly safe because files are almost never modified externally,
// so refresh won't accidentally reload and lose their changes.
void CShwApp::OnViewRefresh()
{
    CShwDoc *pshwdoc = (CShwDoc*) ((CMainFrame*) AfxGetMainWnd())->GetActiveFrame()->GetActiveDocument();
    SetZ();
    bRefreshADoc( pshwdoc );
    CShwView* pviewT = pviewTop();
    if ( pviewT )
        pviewT->ShowInitialTopState();
}

void CShwApp::OnFileSaveAll()
{
    (void) bSaveAllFiles( true, false ); // 1.4qzhy Fix U problem of asking whether to save file on Save All
	Shw_UpdateAllViews(NULL); // 1.4sm Fix bug (1.4ae) of not showing date stamp on save
}

BOOL CShwApp::bSaveAllFiles( BOOL bWriteProtect, bool bAsk ) // 1.2gz Unwrite protect data files when running batch file
{
    // Save all documents
    CDocList doclst; // list of currently open docs
    for ( CShwDoc* pdoc = doclst.pdocFirst(); pdoc; pdoc = doclst.pdocNext() )
        {
        if ( !pdoc->bValidateViews(FALSE) ) // validate each document's views
            return FALSE;
        if ( pdoc->IsModified() )  // 1996-07-25 MRP
			{
			int iReply = IDYES; // 1.4qzhy Default to yes, so if bAsk is false, it will save
			if ( bAsk ) // 1.4qzhy Fix U problem  of asking whether to save file on Save All
				{
				Str8 sFileName = _("Save changes?"); // 1.4qzhk Fix U problem of writing empty file on close // 1.5.0fg 
				sFileName = sFileName + " " + sUTF8( pdoc->GetPathName() ); // 1.5.0fg 
				iReply = AfxMessageBox( sFileName, MB_YESNOCANCEL ); // 1.1tu Ask if user wants to save changes // 1.4qzhk
				}
			if ( iReply == IDCANCEL ) // 1.4qzhk If cancel, don't close
				return FALSE;
			else if ( iReply == IDYES ) // 1.4qzhk If save, save now so framework won't need to
					pdoc->OnSaveDocument( sUTF8( pdoc->GetPathName() ) ); // 1.4qxd Upgrade OnSaveDocument for Unicode build
			else // 1.4qzhk If don't save, clear modified flag so framework won't save
				pdoc->SetModifiedFlag( FALSE ); // 1.4qzhk
			}
		if ( !pdoc->bReadOnly() ) // 1.5.3n If read only, don't change write protect, fixes bug of leaving file falsely read only if two projects open same file
			{
			if ( bWriteProtect && !pProject()->bExerciseNoSave() ) // 1.2ms Fix bug of write protecting exercise project
				WriteProtect( sUTF8( pdoc->GetPathName() ) ); // 1.2an Write protect data file as signal that it is open by an instance of Toolbox // 1.4qxa // 1.4qzhh
			else
				UnWriteProtect( sUTF8( pdoc->GetPathName() ) ); // 1.2gz Unwrite protect data files when running batch file // 1.4qxa // 1.4qzhg
			}
        }
    pProject()->SaveSettings();
    // Update the name of the project in SHW.INI section of registry
    Str8 sProjectPath = pProject()->pszPath();
    WriteToIni_ProjectPath(sProjectPath);
    return TRUE;
}

void CShwApp::OnFileSaveSettings()
{
    pProject()->SaveSettings();
}

void CShwApp::OnViewMarkerFont() // let user set font used to display markers in record view
{
    pProject()->ViewMarkerFont();
}

void CShwApp::OnFileLanguageEncodings()
{
    Shw_plngset()->ViewElements();
}

void CShwApp::OnFileDatabaseTypes()
{
    Shw_ptypset()->ViewElements();
}

#include "corpus_d.h"

void CShwApp::OnFileTextCorpus()
{
    Shw_pcorset()->ViewElements();
}

void CShwApp::OnUpdateFileNew(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( bProjectOpen() );
}

void CShwApp::OnUpdateFileOpen(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( bProjectOpen() );
}

void CShwApp::OnUpdateFileLanguageEncodings(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( bProjectOpen() );
}

void CShwApp::OnUpdateFileDatabaseTypes(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( bProjectOpen() );
}

void CShwApp::OnUpdateFileTextCorpus(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( bProjectOpen() );
}

void CShwApp::OnProjectNew()
{
	Str8 sMessage = _("To start a new project, you need to install the required files. Do you want to do that?"); // 1.4vna Make Project, New recommend install project files // 1.4ywe 
    if ( AfxMessageBox(sMessage, MB_YESNO | MB_ICONQUESTION) == IDYES ) // 1.4vna 
		{
		AfxMessageBox( _("After Toolbox exits, install the New Project package.") ); // 1.5.8ya Update wording of new project message
		CloseShoebox(); // 1.4vna Close Toolbox so user can open new project
		}
	else
		{
		NewProject();
		if ( !m_pProject && !bOpenOrNewProject() )
			CloseShoebox();  // 1998-03-16 MRP
		}
}

// 1999-02-27 MRP: Used to be a static function in shw.cpp,
// but now it's also used by CMDFConvertOlderDlg.
Str8 sGetDefaultSettingsFolder()
// helper function
// returns a useable Default Settings Folder from the INI/Preferences, if possible
{
	return sGetPrivateProfileString( psz_DefaultSettingsFolder ); // 1.4qxm Upgrade GetPrivateProfileString for Unicode build
}

// 1999-03-1 MRP: Used to be part of a static function in shwdoc.cpp,
// but now it's also used by CMDFConvertOlderDlg.
Str8 sGetAppPath()
// helper function
// returns a useable App Path from the INI/Preferences, if possible
{
	Str8 sPath = sGetPrivateProfileString( psz_AppPath ); // 1.4qxm
	if ( sPath == "" || !bFileExists(sPath) )  // If nothing useful, likely on Mac // 1.4qxm
		sPath = "Toolbox"; // give something readable is about all we can do // 1.4qxm
	return sPath; // 1.4qxm
}

void CShwApp::NewProject()
{
    long lFlags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    Str8 sFilter;
    Str8 sPath;
    if ( bProjectOpen() )
        sPath = Shw_pProject()->sSettingsDirPath();
    else
        sPath = sGetDefaultSettingsFolder();
    sFilter = _("Project files");
	sFilter = sFilter + " (*.prj)|*.prj|" + _("All files") + " (*.*)|*.*||";
	if ( !Shw_papp()->bUseCrashFixFileEditDlg() ) // 1.5.8h 
		{
		CFileDialog dlgNew(FALSE, swUTF16( "prj"), NULL, lFlags, swUTF16( sFilter ), m_pMainWnd); // 1.4qrd // 1.5.8h 
		sPath = s_sChangeDirPath(sPath);  // 1998-08-04 MRP
		dlgNew.m_ofn.lpstrInitialDir =  swUTF16( sPath ); // 1.4qra
		dlgNew.m_ofn.lpstrTitle = _T(" "); // 1.4qzhs Fix bad letters in file browse box title
		if ( iDlgDoModalSetReg( &dlgNew ) != IDOK )
			return;
		sPath = sUTF8( dlgNew.GetPathName() ); // 1.4qra
		}
	else // 1.5.8h 
		if ( !bCrashFixBrowseForFileName(sPath, sPath) ) // 1.5.8h 
			return; // 1.5.8h 
	if ( !bAllASCIIComplain( sPath ) ) // 1.4vze 
		return; // 1.4vze 
    if ( bProjectOpen() && !bCloseProject() ) // try to close currently open project
        return;

    BOOL b = bOpenProject( sPath, FALSE, s_pszSettingsVersion );
}

void CShwApp::OnProjectOpen()
{
    OpenProject();
    if ( !m_pProject && !bOpenOrNewProject() )
        CloseShoebox();  // 1998-03-16 MRP
    m_bClosingProjectOrMainFrame = FALSE;  // rde 52tb fix bug of not resetting this
}

void CShwApp::OpenProject()
{
    long lFlags = (OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR);
    Str8 sFilter;
    Str8 sPath;
    if ( bProjectOpen() )
        sPath = Shw_pProject()->sSettingsDirPath();
    else
        sPath = sGetDefaultSettingsFolder();
    sFilter = _("Project files");
	sFilter = sFilter + " (*.prj)|*.prj|" + _("All files") + " (*.*)|*.*||";
	if ( !Shw_papp()->bUseCrashFixFileEditDlg() ) // 1.5.8h 
		{
		CFileDialog dlgOpen(TRUE, swUTF16( "prj" ), NULL, lFlags, swUTF16( sFilter ), m_pMainWnd); // 1.4qrd // 1.5.8h 
		sPath = s_sChangeDirPath(sPath);  // 1998-08-04 MRP
		dlgOpen.m_ofn.lpstrInitialDir =  swUTF16( sPath ); // 1.4qra
		dlgOpen.m_ofn.lpstrTitle = _T(" "); // 1.4qzhs Fix bad letters in file browse box title
		if ( iDlgDoModalSetReg( &dlgOpen ) != IDOK )
			return;
		sPath = sUTF8( dlgOpen.GetPathName() ); // 1.4qra
		}
	else // 1.5.8h 
		if ( !bCrashFixBrowseForFileName(sPath, sPath) ) // 1.5.8h 
			return; // 1.5.8h 
	if ( !bAllASCIIComplain( sPath ) ) // 1.4vze 
		return; // 1.4vze 
    if ( !CProject::s_bIsProjectFile( sPath ) )
        {
        AfxMessageBox( _("This is not a valid project file.") );
        return;
        }

    if ( bProjectOpen() && !bCloseProject() ) // try to close currently open project
        return;

    BOOL b = bOpenProject( sPath, TRUE, s_pszSettingsVersion );
}

extern BOOL bAnotherInstanceFound();

BOOL CShwApp::bOpenProject( const char* pszProjectPath, BOOL bOpening,
        const char* pszSettingsVersion )
{
	m_bOpeningProject = TRUE; // 5.96zb
	m_bWriteProtectedProject = bFileReadOnly( pszProjectPath ); // 1.2ae Note if project is write protected
	if ( !bOpening ) // 1.2gw Fix bug (1.2b) of project new not working
		m_bWriteProtectedProject = FALSE;
//	if ( m_bWriteProtectedProject && bAnotherInstanceFound() ) // 1.4vwc Try to fix bug of sometimes not opening project after crash // 1.4ytd Don't check for another instance
	if ( m_bWriteProtectedProject ) // 1.4ytd If write protected project, clear to recover from previous crash
		{
#ifndef _DEBUG // 1.4yte Don't wait on reopen project that was killed in the debugger
		WaitSeconds( 2 ); // 1.4ytd Wait a few seconds to give another instance that may be in process time to come up
#endif // 1.4ytd 
		Str8 sProjectPath = pszProjectPath; // 1.4yte 
		Str8 sPrjFile; // 1.4ytd 
		if ( sPrjFile.bReadFile( sProjectPath ) )
			{
			Str8 sDbListInfo;
			int iDbList = 0;
			if ( sPrjFile.bGetSettingsTagSection( sDbListInfo, "dblst", iDbList ) )
				{
				Str8 sDbInfo;
				int iDb = 0;
				while ( sDbListInfo.bGetSettingsTagSection( sDbInfo, "db", iDb ) )
					{
					Str8 sDbPath;
					sDbInfo.bGetSettingsTagContent( sDbPath, "+db" );
					Str8 sReadOnly;
					BOOL bReadOnly = sDbInfo.bGetSettingsTagContent( sReadOnly, "ReadOnly" );
					if ( !bReadOnly ) // If db was read only, leave it that way
						UnWriteProtect( sDbPath ); // Clear write protect of db file
					}
				}
			UnWriteProtect( pszProjectPath ); // Clear write protect of project
#ifndef _DEBUG // 1.4yte Don't wait on reopen project that was killed in the debugger
			return FALSE; // Return false to tell Toolbox to exit this time, next time it will run
#endif // 1.4yte 
			}
		}
	WriteProtect( pszProjectPath ); // 1.2af Write protect project file as signal that it is open by an instance of Toolbox // 1.4qzhh
    if ( !CProject::s_bConstruct(pszProjectPath, bOpening, pszSettingsVersion, &m_pProject) )
        {
		if ( !m_bWriteProtectedProject ) // 1.2ag If project wasn't write protected, leave it writable
			UnWriteProtect( pszProjectPath ); // 1.2ag Turn off write protect of project to say no longer in use // 1.4qzhg
        Shw_pbarStatus()->SetRecordInfo("","",NULL,NULL); //09-22-1997 Update the language pointer in the statusbarclass
        Shw_pbarStatus()->SetPriKeyInfo("","",NULL,NULL);
        return FALSE;
        }
	ASSERT( m_pProject );
	if ( pProject()->bExerciseNoSave() ) // 1.2ms If exercise, don't leave project or data files write protected
		{
		UnWriteProtect( pszProjectPath ); // 1.2ms If exercise, clear write protect of project file // 1.4qzhg
		CDocList doclst; // list of currently open docs
		for ( CShwDoc* pdoc = doclst.pdocFirst(); pdoc; pdoc = doclst.pdocNext() )
			UnWriteProtect( sUTF8( pdoc->GetPathName() ) ); // 1.2ms If exercise, clear write protect of data files // 1.4qxa // 1.4qzhg
		}
    
	Str8 sPath = sGetDirPath(pszProjectPath);
    sPath = s_sChangeDirPath(sPath);  // 1998-08-04 MRP
    int i = _chdir( sPath ); // Set current working directory 5-6-98
            // setting the CWD is not required for anything in SH, but will
            // probably be nice for Mac users who don't have DefaultSettingFolder preferences

	SetWriteProtectedSettings( FALSE ); // 1.2am Clear write protected settings after file open sequence is complete

	if ( m_pProject->iLockProject() >= 2 ) // 1.2hb At level 2, remove Project menu
		{
		CMenu* pMenu = Shw_pmainframe()->GetMenu();
		pMenu->DeleteMenu( Shw_pmainframe()->iAdjustForMaximized( 3 ), MF_BYPOSITION ); // Remove Project menu
		if ( Shw_papp()->pProject()->iLockProject() >= 3 ) // 1.2hb At level 3, remove Tools menu
			pMenu->DeleteMenu( Shw_pmainframe()->iAdjustForMaximized( 3 ), MF_BYPOSITION ); // Remove Tools menu
		if ( Shw_papp()->pProject()->iLockProject() >= 4 ) // 1.2hb At level 4, remove View menu
			pMenu->DeleteMenu( Shw_pmainframe()->iAdjustForMaximized( 4 ), MF_BYPOSITION ); // Remove View menu // 1.4qzpe Adjust project lock for new Checks menu
		if ( Shw_papp()->pProject()->iLockProject() >= 5 ) // 1.2hb At level 5, remove Window menu
			pMenu->DeleteMenu( Shw_pmainframe()->iAdjustForMaximized( 4 ), MF_BYPOSITION ); // Remove Window menu // 1.4qzpe Adjust project lock for new Checks menu
		if ( Shw_papp()->pProject()->iLockProject() >= 7 ) // 1.4qzpe At level 7, remove Checks menu (level 6 prevents resize of window)
			pMenu->DeleteMenu( Shw_pmainframe()->iAdjustForMaximized( 3 ), MF_BYPOSITION ); // Remove Checks menu // 1.4qzpe Adjust project lock for new Checks menu
		if ( Shw_papp()->pProject()->iLockProject() >= 10 ) // 1.2hb At level 10, remove File menu
			pMenu->DeleteMenu( Shw_pmainframe()->iAdjustForMaximized( 0 ), MF_BYPOSITION ); // Remove File menu
		Shw_pmainframe()->DrawMenuBar(); // 1.2hb
		}

	CShwView* pviewT = pviewTop(); // 1.2pc Send external reference to TW on project open
    if ( pviewT )
        pviewT->TextParallelJump(); // 1.2pc Send external reference to TW on project open

	m_bOpeningProject = FALSE;
    return TRUE;
}

void CShwApp::OnProjectClose() // 1.4vzf Put Project, Close back on menu to allow close without saving
{	
	int iReply = AfxMessageBox( _("Save changes to project?"), MB_YESNO ); // 1.4vyp Add Ctrl+Alt+Shift+Q as quit without saving
	if ( iReply != IDYES )
		{
		pProject()->SetExerciseNoSave( TRUE ); // 1.1tu Set exercise no save so nothing will be saved
		CDocList doclst; // list of currently open docs
		for ( CShwDoc* pdoc = doclst.pdocFirst(); pdoc; pdoc = doclst.pdocNext() )
			{
			UnWriteProtect( sUTF8( pdoc->GetPathName() ) ); // 1.4qzhj Unwrite protect files on close project without saving
			pdoc->SetModifiedFlag( FALSE ); // 1.4qzhj Clear modified flag so the framework won't ask about saving
			}
		UnWriteProtect( m_pProject->pszPath() ); // Turn off write protect of project to say no longer in use // 1.4qzhj
		CloseShoebox(); // 1.4qzha On close project without saving, exit immediately if desired
		return; // 1.4qzhe Fix problem (1.4qzha) in close project
		}

    (void) bSaveAllFiles(); // 1.4qzmf On project close and save, don't ask about saving files

    m_bClosingProjectOrMainFrame = TRUE;   //08-27-1997
    BOOL bClosed = bCloseProject();
    m_bClosingProjectOrMainFrame = FALSE;  //08-27-1997

    // 1998-03-14 MRP: When a database has unsaved changes, the user gets
    // a Yes/No/Cancel choice. If they choose Cancel, don't close.
    if ( !bClosed )
        return;
    Shw_pbarStatus()->OnUpdateCmdUI(Shw_pmainframe()->GetActiveFrame(), FALSE);
	CloseShoebox();
}

BOOL CShwApp::bCloseProject()
{
	if (!SaveAllModified()) // If user cancels at a save question, don't close project
		return FALSE;    
    CloseAllDocuments(FALSE);
	UnWriteProtect( m_pProject->pszPath() ); // 1.2ag Turn off write protect of project to say no longer in use // 1.4qzhg
    delete m_pProject; // destroy project 

    Shw_pbarStatus()->SetRecordInfo("","",NULL,NULL); //09-22-1997 Update the language pointer in the statusbarclass
    Shw_pbarStatus()->SetPriKeyInfo("","",NULL,NULL);

    m_pProject = NULL;
    return TRUE;
}

void CShwApp::CloseShoebox()
{
    theApp.GetMainWnd( )->PostMessage(WM_CLOSE);
}

void CShwApp::OnUpdateProjectClose(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( bProjectOpen() );
}

void CShwApp::OnProjectSave()
{
    (void) bSaveAllFiles();
	Shw_UpdateAllViews(NULL); // 1.4sm Fix bug (1.4ae) of not showing date stamp on save
}

void CShwApp::OnUpdateProjectSave(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( bProjectOpen() );
}

void CShwApp::OnProjectSaveas()
{
    long lFlags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
	Str8 sFileFilter = _("All files"); // 1.5.0fz 
	sFileFilter += " (*.*)|*.*||"; // 1.5.0fz 
	Str8 sPath; // 1.5.8h 
	if ( !Shw_papp()->bUseCrashFixFileEditDlg() ) // 1.5.8h 
		{
		CFileDialog dlgSaveas(FALSE, swUTF16( "prj" ), swUTF16( sGetFileName( pProject()->pszPath() ) ), lFlags, swUTF16( sFileFilter ) ); // 1.4qrd // 1.5.8h 
		dlgSaveas.m_ofn.lpstrInitialDir =  swUTF16( pProject()->sSettingsDirPath() ); // 1.4qra
		dlgSaveas.m_ofn.lpstrTitle = _T(" "); // 1.4qzhs Fix bad letters in file browse box title
		if ( iDlgDoModalSetReg( &dlgSaveas ) != IDOK )
			return;
		sPath = sUTF8( dlgSaveas.GetPathName() ); // 1.4qra
		}
	else // 1.5.8h 
		if ( !bCrashFixBrowseForFileName(sPath, sPath) ) // 1.5.8h 
			return; // 1.5.8h 
	if ( !bAllASCIIComplain( sPath ) ) // 1.4vze 
		return; // 1.4vze 
    Str8 sNewPath =  sPath; // 1.4qxs // 1.5.8h 
    BOOL bSameDir = !_stricmp( sGetDirPath(sNewPath), sGetDirPath(pProject()->pszPath()) );
    pProject()->SaveSettings(); // save current project first
	UnWriteProtect( pProject()->pszPath() ); // 1.2ah Turn off write protect of old project to say no longer in use // 1.4qzhg
    pProject()->SetPath( sNewPath );
    pProject()->SaveSettings( !bSameDir ); // force settings files to be written if saving to different directory
}

void CShwApp::OnUpdateProjectSaveas(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( bProjectOpen() );
}

void CShwApp::OnUpdateRecentFileMenu(CCmdUI* pCmdUI)
{
    ASSERT(FALSE);
#ifdef BJY_5_23_96
    CWinApp::OnUpdateRecentFileMenu( pCmdUI );
    pCmdUI->Enable( bProjectOpen() );
#endif
}

void CShwApp::OnAutoSave()
{
	Shw_pProject()->SetAutoSave( !Shw_pProject()->bAutoSave() );
}

void CShwApp::OnUpdateAutoSave(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck( Shw_pProject()->bAutoSave() );
}

void CShwApp::OnLockProject()
{
	if ( !Shw_pProject()->bLockProject() ) // 1.2bn If project is being locked, lock all current views
		{
		CDocList doclst; // list of currently open docs
		for ( CShwDoc* pdoc = doclst.pdocFirst(); pdoc; pdoc = doclst.pdocNext() )
    		{
			POSITION pos = pdoc->GetFirstViewPosition();
			while ( pos )
				{
				CShwView* pshv = (CShwView*)pdoc->GetNextView(pos); // increments pos
				pshv->SetViewLocked( TRUE ); // 1.2bn Lock every active view
				}
    		}
		Shw_pProject()->SetLockProject( 1 );
		if ( Shw_pProject()->bDontAskChangeMarker() ) // 1.6.1ch If project says don't ask change marker, change to ask
			Shw_pProject()->ToggleDontAskChangeMarker(); // 1.6.1ch 
		}
	else // 1.2bn Else, project is being unlocked, unlock all views
		{
		CDocList doclst; // list of currently open docs
		for ( CShwDoc* pdoc = doclst.pdocFirst(); pdoc; pdoc = doclst.pdocNext() )
    		{
			POSITION pos = pdoc->GetFirstViewPosition();
			while ( pos )
				{
				CShwView* pshv = (CShwView*)pdoc->GetNextView(pos); // increments pos
				pshv->SetViewLocked( FALSE ); // 1.2bn Unlock every active view
				}
    		}
		Shw_pProject()->SetLockProject( 0 );
		}
}

void CShwApp::OnUpdateLockProject(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck( Shw_pProject()->bLockProject() );
}

void CShwApp::OnViewLargeControls()
{
    if ( pProject()->iLangCtrlHeight() == CProject::eNormalLangCtrl )
        pProject()->SetLangCtrlHeight( CProject::eLargeLangCtrl );
    else
        pProject()->SetLangCtrlHeight( CProject::eNormalLangCtrl );
}

void CShwApp::OnUpdateViewLargeControls(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck( pProject()->iLangCtrlHeight() != CProject::eNormalLangCtrl );
}

void CShwApp::OnToolsCheckConsistencyWhenEditing()
{
    Shw_pProject()->ToggleCheckConsistencyWhenEditing();
}

void CShwApp::OnUpdateToolsCheckConsistencyWhenEditing(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck( Shw_pProject()->bCheckConsistencyWhenEditing() );
}

void CShwApp::OnToolsAutomaticInterlinearUpdate()
{
    Shw_pProject()->ToggleAutomaticInterlinearUpdate();
}

void CShwApp::OnUpdateToolsAutomaticInterlinearUpdate(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck( Shw_pProject()->bAutomaticInterlinearUpdate() );
}

void CShwApp::OnToolsWordList()
{
//	AfxMessageBox( "Sorry, wordlist is unavailable in this test version." ); // 1.4rbe // 1.4tb
//	return; // 1.4rbe // 1.4tb
    CWordListDlg dlg(Shw_pcorset());
    dlg.DoWordList();  // Displays the dialog and does the work
//    if ( dlg.DoModal() != IDOK ) // 1.4rbb  // 1.4tb
        return; // 1.4rbb // 1.4tb
	CCorpus* pcor = dlg.pcor(); // 1.4rbb // 1.4tb
}

void CShwApp::OnToolsConcordance()
{
    // get selected text or word at caret position, if applicable
    // 1999-08-12 MRP: The Tools menu now appears even when
    // there are no databases open.
    Str8 sWord;
    CShwView *pview = (CShwView*) ((CMainFrame*) AfxGetMainWnd())->GetActiveFrame()->GetActiveView();
    if ( pview && !pview->bBrowsing() )
        {
        sWord = pview->sGetCurText(CShwView::cttWord); // Get single word even if field has multi-word item
//        if (sWord.GetLength())        now use sWord as param 4/2/98
//            dlg.m_sSearch = sWord;
        }

    CConcordanceDlg dlg(Shw_pcorset());
    dlg.DoConcordance(sWord);  // Displays the dialog and does the work
}

// Given the path of a currently open document, returns a pointer to that
// document.  Returns NULL if no such document is open.
CShwDoc* CShwApp::pdocGetDocByPath (const char* pszDocPath)
{
    CDocList doclst; // list of currently open docs
    for ( CShwDoc* pdoc = doclst.pdocFirst(); pdoc; pdoc = doclst.pdocNext() )
        if (pdoc->GetPathName().CompareNoCase( swUTF16( pszDocPath )) == 0) // 1.4qyp
            return pdoc;
    return NULL;
}


/*
 *   CShwApp::bRefreshADoc - refresh a document from
 *  the data on disk if neccessary.  Recommended that you call SetZ()
 *  before calling this method and call pviewTop()->ShowInitialTopState()
 *  afterwards.
 */
BOOL CShwApp::bRefreshADoc(CShwDoc* pshwdoc, BOOL bForceRefresh)
{                     
    // If already up to date and not forced do nothing.
    if ( !pshwdoc->bNeedsRefresh() && !bForceRefresh )
        return TRUE;  

#ifdef ostrWritefstream // 1.6.4ad
	Str8 sFile = Shw_pProject()->sSettingsDirPath(); // 1.6.4ad Get settings folder
	sFile += "Temp.txt"; // 1.6.4ad Add file name
	FILE* pf = pfFileOpenWrite( sFile ); // 1.6.4ad Open FILE for write
    Object_ofstream obs(pf); // 1.6.4ad 
    pshwdoc->WriteProperties(obs); // 1.6.4ad 
    pshwdoc->OnCloseDocument(); // 1.6.4ad 
	fclose( pf ); // 1.6.4ad 

    CNoteList notlst;
    std::ifstream ios(sFile);
    Newline_istream iosnl(ios);
    Object_istream obsIn(iosnl, notlst);
    if (!CShwDoc::s_bReadProperties(obsIn)) 
        return FALSE;
    remove( sFile ); // 1.6.4ad delete file
#else
    // Create a buffer in memory to write the document's properties to.
    ostrstream iosOut;
    {
        Object_ostream obsOut(iosOut);
        // Save the document's properties and close it.
        pshwdoc->WriteProperties(obsOut);
        pshwdoc->OnCloseDocument();
		Str8 sProps = iosOut.str(); // 1.6.4ad 
    }
    {
        // Prepare the input stream.
        CNoteList notlst;
        istrstream iosIn(iosOut.str(), iosOut.pcount());
        Object_istream obsIn(iosIn, notlst);
    
        // Reopen the document to its original settings.
        if (!CShwDoc::s_bReadProperties(obsIn)) 
            return FALSE;
    }

    iosOut.rdbuf()->freeze(0);
#endif
    return TRUE;
}

/* CShwApp::RefreshAllDocs - goes through every document and refreshes if from
 * disk if neccessary.
 */
void CShwApp::RefreshAllDocs()
{
    SetZ();
    CDocList doclst; // list of currently open docs
    CShwDoc* pdoc = doclst.pdocFirst();
    CShwDoc* pdocLast = NULL;

    // 2000-01-10 TLB: First get the last document so we know where to stop
    for (; pdoc; pdoc = doclst.pdocNext() )
        pdocLast = pdoc;

    // Now spin through the list of docs, but stop when you get the last one.
    // Refreshing can cause "new" documents to be created, but we only care about
    // the ones that were in existence initially.
    for ( pdoc = doclst.pdocFirst(); pdoc; pdoc = doclst.pdocNext() )
        {
        bRefreshADoc(pdoc);
        if ( pdoc == pdocLast )
            break;
        }
    CShwView* pviewT = pviewTop();
    if ( pviewT )
        pviewT->ShowInitialTopState();
}   

/*
 * CShwApp::OnRunDos - user wants to run a dos command.
 * Do so using the CRunDos class.
 */    
void CShwApp::OnRunDos()
{                 
    if (Shw_pProject()->bDoRunDos()) 
		{
        // a dos command was run which may affect the files
        // currently in memory, therefore refresh them all.
        RefreshAllDocs();
		}
	bSaveAllFiles(); // 1.2gz Redo write protect of data files after running batch file
}

//08-27-1997
BOOL CShwApp::bOpenOrNewProject()
{
    // 1998-03-14 MRP: Need a loop because the user can choose Cancel.
    // 1998-03-16 MRP: To avoid recursive calls to this function, use
    // OpenProject and NewProject instead of OnProjectOpen and OnProjectNew.
    ASSERT( !m_pProject );
    do
        {
        int iselected = 0;
        CNoProjectDlg dlg(&iselected);
        if ( dlg.DoModal() == IDOK )
            switch (iselected)
                {
                case 0:
                    {
                    OpenProject();
                    break;
                    }
                case 1:
                    {
                    NewProject();
                    break;
                    }
                default:
                    return FALSE;  // Exit Shoebox
                }
        }
        while ( !m_pProject );

    return TRUE;  // A project is open
}


BOOL CShwApp::bExternalJump(const char* pszWordFocusRef, CShwView* pshvFrom, BOOL bFailMessage, BOOL bJump ) // Do external jump // 1.4qzjf Fix problem (1.4qzje) of window flash on parallel move
{
	BOOL bSucc = FALSE;
    // 1998-09-24 MRP
    CDocList doclst; // list of currently open docs
    for ( CShwDoc* pdoc = doclst.pdocFirst(); pdoc; pdoc = doclst.pdocNext() )
    	{
	    POSITION pos = pdoc->GetFirstViewPosition();
	    while ( pos )
	        {
	        CShwView* pshv = (CShwView*)pdoc->GetNextView(pos); // increments pos
			if ( pshvFrom )
				{
				if ( pshvFrom == pshv ) // If jump is from this internal view, skip it
					continue;
				if ( pshvFrom->GetDocument() == pdoc // 1.0cr If jump is from a duplicate view of same doc sorted the same, skip it
						&& pshvFrom->pind()->pmkrPriKey() == pshv->pind()->pmkrPriKey() 
						&& pshvFrom->pind()->bSortPriKeyFromEnd() == pshv->pind()->bSortPriKeyFromEnd() // 1.0dd Fix bug of parallel move failing if same window sorted from opposite end
						&& !pshvFrom->bBrowsing() ) // 1.0de Make parallel move active to same database from browse view
					continue;
				}
			CNumberedRecElPtr prel = NULL; // 1.5.2aa 
			if ( pshvFrom ) // 1.5.2aa 
				prel = pshvFrom->prelCur(); // 1.5.2aa Fix bug (1.5.2nc) of crash on external jump
			if ( pshv->bExternalJump( pszWordFocusRef, prel, bFailMessage, bJump ) ) // 6.0zf Don't ask multiple matches on parallel jump // 1.4qzjf // 1.5.1nc  // 1.5.2aa 
		        bSucc = TRUE;
	        }
    	}
	if ( !bSucc && bFailMessage ) // 6.0za Eliminate No Matches message from BookRef jump
		{
		if ( !pshvFrom && pviewActive()->OpenClipboard() ) // 1.1ab Don't put nomatch onto clipboard if from internal jump
			{
			::EmptyClipboard();
			Str8 sSel = pszWordFocusRef;
			int iEnd = sSel.Find( "\n" );
			if ( iEnd > 0 )
				sSel = sSel.Left( iEnd );
			iEnd = sSel.Find( " " );
			if ( iEnd > 0 )
				sSel = sSel.Left( iEnd );
			int len = sSel.GetLength()+1;
			HGLOBAL hData = ::GlobalAlloc( GMEM_DDESHARE, len ); // get a chunk of memory (+1 for null termination)
			ASSERT( hData );
			char* pmem = (char* )::GlobalLock( hData ); // get a pointer to allocated memory
			ASSERT( pmem );
			strcpy_s( pmem, len, sSel);
			::SetClipboardData( CF_TEXT, hData );
			::CloseClipboard();
			}
		AfxMessageBox( _("No matches for jump") );
		}
	return bSucc;
}

BOOL CShwApp::bViewValid( CShwView* pview ) // Return true if view is valid, false if it has been closed, used for return from jump
	{
    CDocList doclst; // list of currently open docs
    for ( CShwDoc* pdoc = doclst.pdocFirst(); pdoc; pdoc = doclst.pdocNext() )
    	{
	    POSITION pos = pdoc->GetFirstViewPosition();
	    while ( pos )
	        {
	        CShwView* pshv = (CShwView*)pdoc->GetNextView(pos); // increments pos
			if ( pshv == pview ) // If view is found, then it is valid, so return true
		        return TRUE;
	        }
    	}
	return FALSE; // If view not found among the active views, return false
	}

#ifdef _MAC
const char* pNL = "\n"; // system dependent newline strings
#else
const char* pNL = "\r\n";
#endif


// shw.h : main header file for the SHW application

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols 

// #define WineFix // 1.5.9mb Disable rendering for Wine display problem fix // 1.5.9na // 1.5.9rd Move to here

class CUpdate; // update.h
class CHint; // hint.h
class Object_istream; // obstream.h
class Object_ostream;

class CShwDoc; //?? May be restructured later
class CShwView;
class CProject; // project.h
class CDatabaseType;  // typ.h

/////////////////////////////////////////////////////////////////////////////
// CShwApp:
// See shw.cpp for the implementation of this class
//
class CShwApp : public CWinApp
{
private:
    CProject* m_pProject; // currently open project or NULL if none open

    BOOL m_bOpeningProject; // TRUE if project is in process of being opened
    BOOL m_bWriteProtectedProject; // TRUE if project file is write protected
    BOOL m_bClosing; // TRUE if app is in process of being closed
	BOOL m_bWriteProtectedSettings; // TRUE if settings file or files are write protected
	Str8 m_sAppPath; // 1.5.0dd Add app path to app class, get it from help path
	BOOL m_bProjectOpenedPortable; // 1.5.0de Remember if project opened in portable way (from program folder, don't save in registry)

      //08-28-1997 This variable will become TRUE if the user selected
      //Project,close or the user selected to shut down Shoebox.
      //In the first case, the variable will become FALSE again, when
      //the Project,close work is done. The variabe is intialized at FALSE.
      //The flag is necessary to detect a user started File,close job.
      //Only in this case a messagebox should appear if it is the last
      //file of a project.
    BOOL m_bClosingProjectOrMainFrame;
    BOOL m_bAnotherInstanceAlreadyRunning;  // 1998-05-11 MRP
    UINT    m_uiTlbxIntrlClipBrdFormat;
	CShwView* m_pviewJumpedFrom; // View most recently jumped from
	CShwView* m_pviewJumpedFromWordParse; // View most recently jumped from // 1.6.4v 
	BOOL m_bNoHelpMessageGiven; // 1.4ppa Remember if no help message already given
	BOOL m_bUseCrashFixFileEditDlg; // 1.5.8h If true, use crash fix file navigation dialog box // 1.5.8q Move to app from project to fix crash

	COLORREF g_clrDialogBk;
	COLORREF g_clrDialogText;

public:
    CShwApp();
    ~CShwApp();

	Str8 sAppPath() const { return m_sAppPath; } // 1.5.0dd 

    BOOL bProjectOpen() const { return m_pProject != NULL; } // return TRUE if a project is open
    CProject* pProject() const; // 1.4ytf Try to fix bug of assert on init project
//		{ 
//		ASSERT( bProjectOpen() ); // 1.4ytd Disable to prevent false assert in ShwApp::InitInstance
//		return m_pProject; 
//		}

    void OnFileNew(); // Set default folder
    void OnFileOpen(); // Set default folder
	HBRUSH GetAppDialogBrush(CDC* pDC);

	BOOL bExternalJump(const char* pszWordFocusRef, CShwView* pshvFrom, BOOL bFailMessage = TRUE, BOOL bJump = TRUE ); // 1.4qzjf
    	// 1998-09-24 MRP: Forward focus synchronization requests
        //from other programs (e.g. Paratext) to all open windows.
	CShwView* pviewJumpedFrom() { return m_pviewJumpedFrom; }; // View most recently jumped from
	CShwView* pviewJumpedFromWordParse() { return m_pviewJumpedFromWordParse; }; // View most recently jumped from
	void SetViewJumpedFrom( CShwView* pview ) { m_pviewJumpedFrom = pview; }; // Set view jumped from
	void SetViewJumpedFromWordParse( CShwView* pview ) { m_pviewJumpedFromWordParse = pview; }; // Set view jumped from

	BOOL bViewValid( CShwView* pview ); // Return true if view is valid, false if it has been closed, used for return from jump

    void Update(CUpdate& up);
    CShwDoc* pdoc(const char* pszDatabasePath); // Access to document from string of path, returns null if not open
    CShwDoc* pdocOpen(const char* pszDatabasePath); // Same as pdoc, but tries to open if not open, returns null if path cannot be opened
    BOOL bValidateAllViews(); // call bValidateCur() for every view
    void SendUpdateToAllViews(LPARAM lHint = 0L, CObject* pHint = NULL);
    UINT    TlbxClipBdFormat() const  { return m_uiTlbxIntrlClipBrdFormat; };

    CShwView* pviewActive();
        // Return the active MDI child record window;
        // otherwise NULL, if there are no windows open.
    CShwView* pviewTop();
    CShwView* pviewBottom();
    CShwView* pviewBelow(CShwView* pviewCur);
    CShwView* pviewAbove(CShwView* pviewCur);
        // Operations on the MDI child view [window] list, ordered by z-order.
    void SetZ();
        // Set the current z-order of all MDI child views.
        
    BOOL bOpeningProject() { return m_bOpeningProject; }
    BOOL bWriteProtectedProject() { return m_bWriteProtectedProject; }
    BOOL bClosing() { return m_bClosing; }
	BOOL bWriteProtectedSettings() { return m_bWriteProtectedSettings; }
	void SetWriteProtectedSettings( BOOL b ) { m_bWriteProtectedSettings = b; }

    //08-28-1997
    //The following functions permit access to a private Membervariable
    void SetClosingProjectOrMainFrame(BOOL bclosing) 
        { m_bClosingProjectOrMainFrame = bclosing; }
    BOOL bGetClosingProjectOrMainFrame()
        { return m_bClosingProjectOrMainFrame; }

    void SetAnotherInstanceAlreadyRunning(BOOL b) 
        { m_bAnotherInstanceAlreadyRunning = b; }
    BOOL bAnotherInstanceAlreadyRunning()
        { return m_bAnotherInstanceAlreadyRunning; }
    
    void WriteToIni_Title( const char* pszTitle ); // write current window title to .ini file
    void WriteToIni_ProjectPath(const char* pszProjectPath,
        const char* pszKey = nullptr); // write path of currently opened project, optionally specify key
    Str8 sLastWndTitle(); // get window title from .ini file to see if another instance is running
// Overrides
    virtual BOOL InitInstance();
    virtual int ExitInstance();
    virtual BOOL SaveAllModified();
    virtual BOOL OnIdle( LONG lCount );
	virtual void WinHelp( DWORD dwData, UINT nCmd = HELP_CONTEXT ); // 1.4pp Make help check file existence to solve bug on Linux Wine
    
    CShwDoc* pdocGetDocByPath(const char* pszDocPath);
    BOOL bRefreshADoc(CShwDoc * pshwdoc, BOOL bForceRefresh = FALSE);
    // virtual CDocument* OpenDocumentFile( LPCSTR lpszFileName );
    BOOL bSaveAllFiles( BOOL bWriteProtect = TRUE, bool bAsk = FALSE );
        // Returns whether all records currently being viewed are valid.
        // For each current record (which has been modified) validity checks
        // are done (e.g. range sets, CARLA field syntax).
        // If any one is not valid, this function makes its view active,
        // gives feedback and returns FALSE; the caller should allow
        // the user to continue to edit that record.
        // If all are valid, their keys are recomputed and, if changed
        // the record elements are positioned correctly.

    BOOL bUseCrashFixFileEditDlg() const { return m_bUseCrashFixFileEditDlg; } // 1.5.8h 
    void ToggleUseCrashFixFileEditDlg() // 1.5.8h 
        { m_bUseCrashFixFileEditDlg = !m_bUseCrashFixFileEditDlg; } // 1.5.8h 
    
#ifdef _DEBUG
    void AssertValidity() const;
        // This function asserts the validity of all persistant objects
        // starting from the following main collection objects:
        // markup types, language encodings, documents and views.
        //
        // NOTE: This function is called by the Alt+Ctrl+Shift+V hotkey.
        // We expect that automated external tests will want to use it.
        //
        // NOTE: The AssertValid of an MFC base class is called before
        // the second phase of object construction occurs. Furthermore,
        // the application framework assumes it is very cheap to call,
        // Therefore, we must check validity of Shoebox-specific data
        // in derived classes using a separate function that only we call.
#endif  // _DEBUG

// Implementation

    //{{AFX_MSG(CShwApp)
    afx_msg void OnAppAbout();
    afx_msg void OnFileSaveAll();
    afx_msg void OnFileSaveSettings();
    afx_msg void OnViewMarkerFont();
    afx_msg void OnFileLanguageEncodings();
    afx_msg void OnFileDatabaseTypes();
    afx_msg void OnFileTextCorpus();
    afx_msg void OnUpdateFileNew(CCmdUI* pCmdUI);
    afx_msg void OnUpdateFileOpen(CCmdUI* pCmdUI);
    afx_msg void OnUpdateFileLanguageEncodings(CCmdUI* pCmdUI);
    afx_msg void OnUpdateFileDatabaseTypes(CCmdUI* pCmdUI);
    afx_msg void OnUpdateFileTextCorpus(CCmdUI* pCmdUI);
    afx_msg void OnProjectNew();
    afx_msg void OnProjectOpen();
    afx_msg void OnProjectClose();
    afx_msg void OnUpdateProjectClose(CCmdUI* pCmdUI);
    afx_msg void OnProjectSave();
    afx_msg void OnUpdateProjectSave(CCmdUI* pCmdUI);
    afx_msg void OnProjectSaveas();
    afx_msg void OnUpdateProjectSaveas(CCmdUI* pCmdUI);
    afx_msg void OnUpdateRecentFileMenu(CCmdUI* pCmdUI);
    afx_msg void OnViewRefresh();
    afx_msg void OnAutoSave();
    afx_msg void OnUpdateAutoSave(CCmdUI* pCmdUI);
    afx_msg void OnLockProject();
    afx_msg void OnUpdateLockProject(CCmdUI* pCmdUI);
    afx_msg void OnViewLargeControls();
    afx_msg void OnUpdateViewLargeControls(CCmdUI* pCmdUI);
    afx_msg void OnToolsCheckConsistencyWhenEditing();
    afx_msg void OnUpdateToolsCheckConsistencyWhenEditing(CCmdUI* pCmdUI);
    afx_msg void OnToolsAutomaticInterlinearUpdate();
    afx_msg void OnUpdateToolsAutomaticInterlinearUpdate(CCmdUI* pCmdUI);
    afx_msg void OnToolsWordList();
    afx_msg void OnToolsConcordance();
    afx_msg void OnRunDos();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    CShwView* pviewEnd(UINT uNextOrPrev);
    CShwView* pviewNeighbor(CShwView* pviewCur, UINT uNextOrPrev);
        // NOTE NOTE NOTE: In the MDI window list, the next direction
        // means toward the bottom; prev means toward the top.

    Str8 CShwApp::DetermineWinProjectPath();
    Str8 sGetLastProjectFromIni(); // get ProjectLastClosed setting from .ini file

    void NewProject();  // 1998-03-16 MRP
    void OpenProject();  // 1998-03-16 MRP
    BOOL bOpenProject( const char* pszProjectPath, BOOL bOpening,
            const char* pszSettingsVersion );
        // Starting Shoebox, Project Open, Project New
    BOOL bCloseProject(); // close currently open project
    void CloseShoebox();  // 1998-03-16 MRP
    BOOL bOpenOrNewProject();
        // 1997-08-27: Show the No Project Open dialog to let the user
        // choose Open, New, or Exit Shoebox.
        // 1998-03-16 MRP: Return TRUE if a project is opened;
        // FALSE if the user chose Exit.

#ifdef _MAC
    BOOL CreateInitialDocument(); // override of virtual function on Mac
    CDocument* OpenDocumentFile(const char* pszFileName); // open named file
#endif

public:
    void RefreshAllDocs();

    BOOL bReadDbList(Object_istream& obs); // read the open databases and windows
    BOOL bReadMRUList(Object_istream& obs); // Read in Most Recently Used file list
#ifdef prjWritefstream // 1.6.4aa 
    void WriteDbList(Object_ofstream& obs); // write the open databases and windows
#else
    void WriteDbList(Object_ostream& obs); // write the open databases and windows
#endif
#ifdef prjWritefstream // 1.6.4aa 
    void WriteMRUList(Object_ofstream& obs); // write the most recently used file list
#else
    void WriteMRUList(Object_ostream& obs); // write the most recently used file list
#endif
};  // class CShwApp


CShwApp& Shw_app();
CShwApp* Shw_papp(); // The Shoebox application object.

class CMainFrame; // forward reference - mainfrm.h
CMainFrame* Shw_pmainframe(); // mainframe - should be called only when a db is open

#ifdef ToolbarFindCombo // 1.4yq Eliminate find combo box from toolbar, can't work right
class CFindComboBox; // forward reference - find_d.h
CFindComboBox* Shw_pcboFind(); // access to find combo
#endif // ToolbarFindCombo // 1.4yq

void Shw_Update(CUpdate& up);

void Shw_UpdateAllViews(CHint* pHint);
    // Broadcast notification to each view of each document that an element
    // of "settings" data upon which it may depend has been modified (or
    // a new one has been added, or one has been deleted).

Str8 sGetDefaultSettingsFolder();
Str8 sGetAppPath();

#ifdef _DEBUG
void Shw_AssertValid();
#endif  // _DEBUG

/////////////////////////////////////////////////////////////////////////////

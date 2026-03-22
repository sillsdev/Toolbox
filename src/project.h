// project.h : CProject class

#ifndef PROJECT_H
#define PROJECT_H

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif

class CFindSettings; // forward reference - find_d.h
class CFindTextList; // forward reference - find_d.h
class CShwDoc; //?? May be restructured later
class CShwView;
class CLangEncSet;
class CDatabaseTypeSet;
class CDatabaseType;
class CCorpusSet;
class CKeyboardManager;
class Object_ostream;  // obstream.h
class Object_ofstream;  // obstream.h // 1.6.4aa 
class Object_istream;  // obstream.h
class CNoteList;
class CSetListBox;  // set_d.h
class CRunDos;

#include "resource.h"


/////////////////////////////////////////////////////////////////////////////
// CProject
class CProject
{
#ifdef TEST_BUILD
    friend class CProject_Test;
#endif
private:
    Str8 m_sSettingsVersion;  // 1998-03-12 MRP
    Str8 m_sSettingsDirPath;
    Str8 m_sProjectName;  // remember user's actual choice of case
    Str8 m_sProjectPath;
    Str8 m_sPrevProjectPath; // Path where project was saved in previous run, detects moved project AB 3-10-98
    Str8 m_sMovedFileDirPath;
	Str8 m_sUnrecognizedSettingsInfo; // 1.0cp Don't lose settings info from later versions

    // NOTE: Language encoding set is declared before proxy set,
    // since it must be constructed before and destroyed after. 
    CLangEncSet* m_plngset;  // the set of all language encodings
    CDatabaseTypeSet* m_ptypset; // The set of all database types
    CCorpusSet* m_pcorset;  // the set of all text corpuses
//  CInterlinearSetupSet m_intsupset; // The set of all interlinear setups // AB 2-28-96 Still under consideration
    
    CRunDos * m_pDosRunner;

    CKeyboardManager* m_pkmn;

    CFindSettings* m_pfndset; // Settings for Find dialog including list of previous find texts
    BOOL m_bClosing; // TRUE if project is being closed
    BOOL m_bWarnDocClose; // if TRUE, display warning when doc closing that is in a jump path
    CFont m_fntMkrs; // font used to display markers in record view
    BOOL m_bShowWholePath; // show whole path name in listboxes showing filenames
    BOOL m_bAutoWrap; // state of auto wrap feature
	BOOL m_bAutoSave; // state of auto save feature
	int m_iLockProject; // 1.2bp State of project lock feature
	BOOL m_bSaved; // 1.1tu Note whether any save has happened, so as to allow project close without saving
	BOOL m_bExerciseNoSave; // 5.99f If true, this is exercise, never to be saved
	BOOL m_bNoBackup; // 1.5.3p If true, no automatic backup files should be created
	BOOL m_bSaveWithoutNewlines; // 1.5.4b If true, save fields with no embedded newlines
    BOOL m_bUseImportCCT; // use conversion table when importing files
    Str8 m_sImportCCT; // name of conversion table for importing
    BOOL m_bRemoveSpaces;
    BOOL m_bMovedPath; // Becomes TRUE when the first time a moved File get opend
    
    int m_iLangCtlHeight; // height of edit boxes, list box items, status bar, etc.

    BOOL m_bCheckConsistencyWhenEditing;
	BOOL m_bAutomaticInterlinearUpdate; // 5.98w
	BOOL m_bDontAskChangeMarker; // 1.6.1ch Project remember don't ask change marker

    // 08-12-1997 Functions return TRUE, when the path matches to their own System
    BOOL bPathFromOwnSystem(Str8* psPath); // Modify psPath if it is from an other System

    CProject( const char* pszProjectPath, const char* pszSettingsVersion );

public:
	CRect m_rectAmbig; // 1.2mj Remember position of ambig box
	void SetAmbigPos( int iLeft, int iTop ) { m_rectAmbig.left = iLeft; m_rectAmbig.top = iTop; }

#ifdef _MAC
    enum { eNormalLangCtrl = 15, eLargeLangCtrl = 21 }; // normal and enlarged sizes for language controls
#else
    enum { eNormalLangCtrl = 24, eLargeLangCtrl = 54 };
#endif
	BOOL m_bExportProcessAutoOpening; // 1.1bh Assign db type automatically on export process auto open
	CDatabaseType* m_ptypExportProcessAutoOpening; // 1.1bh Db type to assign on export process auto open

public:
    static BOOL s_bConstruct(const char* pszProjectPath, BOOL bOpening,
            const char* pszSettingsVersion, CProject** ppprj);
    ~CProject(); // destructor
    
    void SetPath( const char* pszPath ); // set full path for project file
    const char* pszPath() { return m_sProjectPath; } // return full pathname to project file
    const char* pszPrevPath() { return m_sPrevProjectPath; } // return full pathname to project file
    BOOL CProject::bProjectMoved(); // Return true if project moved
    void UpdatePath( Str8& sPath ); // Update path from old project path to new, if applicable
    const Str8& sSettingsDirPath() const { return m_sSettingsDirPath; }
    Str8 sSettingsDirLabel() const;
    const Str8& sProjectName() const { return m_sProjectName; }
    const Str8& sSettingsVersion() const { return m_sSettingsVersion; }
    
    CFindSettings* pfndset() { return m_pfndset; } // access to Find... settings
    // 2000-08-29 TLB & MRP : to avoid CFindComboBox accessing freed memory, provide
    // access to pftxlst via project.
    CFindTextList* pftxlst();

    CFont* pfntMkrs() { return &m_fntMkrs; }

    CDatabaseTypeSet* ptypset() { return m_ptypset; }   
    CLangEncSet* plngset() { return m_plngset; }
    CCorpusSet* pcorset() { return m_pcorset; }
    CKeyboardManager* pkmnManager() { return m_pkmn; }

	void DoAutoSave(); // save db type & lang, if modified
    void SaveSettings( BOOL bForceWrite=FALSE ); // save db type, language and project settings
    BOOL bClosing() { return m_bClosing; }
    BOOL bDoRunDos();
    
    // interface for setting that determines whether we warn the user if they
    // try to close a document referenced in a jump path
    void SetWarnDocClose( BOOL bWarn ) { m_bWarnDocClose = bWarn; }
    BOOL bWarnDocClose() { return m_bWarnDocClose; }
    
    // determines whether whole path is displayed in listboxes that contain filenames
    void ShowWholePath(BOOL bShowWholePath) { m_bShowWholePath = bShowWholePath; }
    BOOL bShowWholePath() { return m_bShowWholePath; }

    BOOL bAutoWrap() { return m_bAutoWrap; } // if TRUE, auto wrap feature is enabled
    void SetAutoWrap( BOOL bEnable ) { m_bAutoWrap = bEnable; }

	BOOL bAutoSave() { return m_bAutoSave; }
    void SetAutoSave( BOOL bEnable ) { m_bAutoSave = bEnable; }

	int iLockProject() { return m_iLockProject; }
    void SetLockProject( int i ) { m_iLockProject = i; }
	BOOL bLockProject() { return m_iLockProject > 0; }

	BOOL bSaved() { return m_bSaved; }
    void SetSaved( BOOL b ) { m_bSaved = b; }

	BOOL bExerciseNoSave() { return m_bExerciseNoSave; } // 5.99f
    void SetExerciseNoSave( BOOL bEnable ) { m_bExerciseNoSave = bEnable; }

	BOOL bNoBackup() { return m_bNoBackup; } // 1.5.3p 
    void SetNoBackup( BOOL bEnable ) { m_bNoBackup = bEnable; } // 1.5.3p 

	BOOL bSaveWithoutNewlines() { return m_bSaveWithoutNewlines; } // 1.5.4b
    void SetSaveWithoutNewlines( BOOL bEnable ) { m_bSaveWithoutNewlines = bEnable; } // 1.5.4b

    // interface for import conversion table settings
    BOOL bUseImportCCT() const { return m_bUseImportCCT; }
    const char* pszImportCCT() const { return m_sImportCCT; }
    void SetImportCCT( const char* pszImportCCT, BOOL bUseImportCCT )
        { m_sImportCCT = pszImportCCT;  m_bUseImportCCT = bUseImportCCT; }
    BOOL bRemoveSpaces() const { return m_bRemoveSpaces; }
    void SetRemoveSpaces( BOOL bRemoveSpaces )
        { m_bRemoveSpaces = bRemoveSpaces; }

    int iLangCtrlHeight() const { return m_iLangCtlHeight; } // return height of language controls
    void SetLangCtrlHeight( int iHeight ); // set size of language controls - normal size or enlarged

    BOOL bCheckConsistencyWhenEditing() const { return m_bCheckConsistencyWhenEditing; }
    void ToggleCheckConsistencyWhenEditing()
        { m_bCheckConsistencyWhenEditing = !m_bCheckConsistencyWhenEditing; }

    BOOL bAutomaticInterlinearUpdate() const { return m_bAutomaticInterlinearUpdate; }
    void ToggleAutomaticInterlinearUpdate() // 5.98w
        { m_bAutomaticInterlinearUpdate = !m_bAutomaticInterlinearUpdate; }

    BOOL bDontAskChangeMarker() const { return m_bDontAskChangeMarker; } // 1.6.1ch Read don't ask change marker
    void ToggleDontAskChangeMarker() // 1.6.1ch Clear or set don't ask change marker
        { m_bDontAskChangeMarker = !m_bDontAskChangeMarker; } // 1.6.1ch 


    static BOOL s_bIsProjectFile(const char* pszPath);
        // return TRUE if pszPath is a project file

    void ViewMarkerFont(); // let user set font used to display markers in record view
    
    BOOL bOpenMovedFile(Str8* psPath);
        // Whenever a database file in the project, or referred to
        // in a Database Type's jump path or interlinear settings
        // does not exist, let the user choose the directory
        // to which it has been moved.
        // If the user chooses OK, this function returns TRUE and
        // via *psPath the path for opening the file;
        // otherwise it returns FALSE and the file should not be opened.

#ifdef _DEBUG
    void AssertValidity() const;
#endif

private:
    static BOOL s_bIsProjectFile(const char* pszPath, Str8& sVersion);
        // return TRUE if pszPath is a project file
    BOOL bInitializeProperties(BOOL bOpening);  // 1998-03-24 MRP
    BOOL bReadProperties(const char* pszPath, CNoteList& notlst);
    void ReadProperties(Object_istream& obs);
    BOOL bWriteProperties(const char* pszPath, BOOL bForceWrite = FALSE);
#ifdef prjWritefstream // 1.6.4aa Use prjWritefstream
    void WriteProperties(Object_ofstream& obs);
#else
    void WriteProperties(Object_ostream& obs);
#endif // 1.6.4aa 

};  // class CProject


CProject* Shw_pProject();
CDatabaseTypeSet* Shw_ptypset();
CLangEncSet* Shw_plngset();
CCorpusSet* Shw_pcorset();


//-----------------------------------------------------------------------------

//08-26-1997
//This dialogbox appears when no project-file is open
class CNoProjectDlg : public CDialog
{
private:
    int* m_piSelected;

public:
    //constructor
    CNoProjectDlg(int* iSelected);

//Dialog Date
    //{{AFX_DATA(CNoProjectDlg)
    enum { IDD = IDD_No_Project };
    int m_iSelected;
    //}}AFX_DATA

//Implementation
protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

                
    //Generated message map functions
    //{{AFX_MSG(CNoProjectDlg)
    virtual void OnOK();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};  //class CNoProjectDlg

#endif  // not PROJECT_H

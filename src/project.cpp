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
#include <filesystem>
#include <vector>
#include "doclist.h"
#include "shwdoc.h"
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
#ifdef TEST_BUILD
    #undef ASSERT
    #define ASSERT(f) ((void)0)
#endif

namespace {
Str8 AbsoluteProjectFolder(const Str8& path);
}

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
    
        m_sSavingProjectRoot = sInferProjectRoot();
        const BOOL saved = bWriteProperties(m_sProjectPath);
        if (saved)
            m_sSavedProjectRoot = m_sSavingProjectRoot;
        m_sSavingProjectRoot.Empty();
        if (!saved)
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
    if (m_sSettingsDirPath != sGetDirPath(pszPath))
        m_sSavedProjectRoot.Empty();
    m_sProjectPath = pszPath;
    m_sProjectFolder = AbsoluteProjectFolder(m_sProjectPath);
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
    bSaveSettings(bForceWrite);
}

BOOL CProject::bSaveSettings( BOOL bForceWrite )
{
    if (bExerciseNoSave() && !bForceWrite)
        return TRUE;

    m_sSavingProjectRoot = sInferProjectRoot();
    plngset()->WriteAllModified(bForceWrite);
    const BOOL rewriteTypes = !m_sPrevProjectPath.IsEmpty() ||
        (!m_sSavedProjectRoot.IsEmpty() && m_sSavedProjectRoot != m_sSavingProjectRoot);
    if (rewriteTypes)
        {
        if (!ptypset()->bSaveProjectPaths(bForceWrite))
            {
            m_sSavingProjectRoot.Empty();
            return FALSE;
            }
        }
    else
        ptypset()->WriteAllModified(bForceWrite);

    const BOOL saved = bWriteProperties(m_sProjectPath, bForceWrite);
    if (saved)
        {
        m_sSavedProjectRoot = m_sSavingProjectRoot;
        m_sPrevProjectPath.Empty();
        m_sOldProjectRoot.Empty();
        m_sRelocatedProjectRoot.Empty();
        SetSaved(TRUE);
        }
    else
        AfxMessageBox(Str8(_("Cannot write file:")) + " " + m_sProjectPath);
    m_sSavingProjectRoot.Empty();
    return saved;
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

    WCHAR temporary[MAX_PATH] = {};
    if (!GetTempFileNameW(swUTF16(m_sProjectFolder), L"prj", 0, temporary))
        return FALSE;

    BOOL written = FALSE;
#ifdef prjWritefstream
    FILE* file = nullptr;
    if (_wfopen_s(&file, temporary, L"w") == 0 && file)
        {
        Object_ofstream obs(file);
        WriteProperties(obs);
        written = fflush(file) == 0 && !ferror(file);
        }
#else
    {
        std::ofstream file(temporary);
        if (file)
            {
            Object_ostream obs(file);
            WriteProperties(obs);
            file.flush();
            written = file.good();
            }
    }
#endif
    if (written)
        {
        const BOOL wasReadOnly = bFileReadOnly(pszPath);
        UnWriteProtect(pszPath);
        written = MoveFileExW(temporary, swUTF16(pszPath), MOVEFILE_REPLACE_EXISTING);
        if (!written && wasReadOnly)
            WriteProtect(pszPath);
        }
    if (!written)
        {
        DeleteFileW(temporary);
        return FALSE;
        }
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
    obs.WriteString(psz_ImportCCT, sStoredPath(m_sImportCCT));
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

namespace
{
namespace fs = std::filesystem;

fs::path NormalizedPath(const Str8& path)
{
    fs::path result(static_cast<LPCWSTR>(swUTF16(path)));
    result.make_preferred();
    result = result.lexically_normal();
    if (result.has_relative_path() && !result.has_filename())
        result = result.parent_path();
    return result;
}

Str8 PathString(const fs::path& path)
{
    return sUTF8(path.c_str());
}

Str8 AbsoluteProjectFolder(const Str8& path)
{
    std::error_code error;
    const fs::path absolute = fs::absolute(NormalizedPath(path), error);
    return error ? Str8() : PathString(absolute.lexically_normal().parent_path());
}

bool PathWithin(const fs::path& path, const fs::path& directory, fs::path& suffix)
{
    if (directory.empty())
        return false;
    auto part = path.begin();
    for (const auto& dirPart : directory)
        {
        if (part == path.end() ||
            CompareStringOrdinal(part->c_str(), -1, dirPart.c_str(), -1, TRUE) != CSTR_EQUAL)
            return false;
        ++part;
        }
    suffix.clear();
    for (; part != path.end(); ++part)
        suffix /= *part;
    return true;
}

bool UsesParentRoot(const fs::path& settings, const fs::path& file)
{
    const fs::path parent = settings.parent_path();
    fs::path suffix;
    // A UNC share is the top-level folder for this comparison.
    if (parent == settings || (parent == parent.root_path() && parent.root_name().native().size() > 2))
        return false;
    return PathWithin(file, parent, suffix) &&
        !PathWithin(file, settings, suffix);
}
}

void CProject::ReadProjectLayout(Object_istream& obs)
{
    if (!obs.bReadBeginMarker(psz_ShProjectSettings))
        return;

    std::vector<Str8> dataFiles;
    while (!obs.bAtEnd())
        {
        if (obs.bReadString(psz_ProjectPath, m_sPrevProjectPath))
            continue;
        if (obs.bReadBeginMarker("dblst"))
            {
            Str8 file;
            while (!obs.bAtEnd())
                {
                if (obs.bReadBeginMarker("db", file))
                    {
                    dataFiles.push_back(file);
                    while (!obs.bAtEnd() && !obs.bEnd("db"))
                        {}
                    }
                else if (obs.bEnd("dblst"))
                    break;
                }
            }
        else if (obs.bEnd(psz_ShProjectSettings))
            break;
        }

    const fs::path currentSettings = NormalizedPath(m_sProjectFolder);
    const fs::path oldSettings = NormalizedPath(m_sPrevProjectPath).parent_path();
    const fs::path& layoutSettings = oldSettings.is_absolute() ? oldSettings : currentSettings;
    bool useParent = false;
    for (const Str8& file : dataFiles)
        {
        if (file.Find('<') == 0)
            continue;
        const fs::path path = NormalizedPath(file);
        if ((path.is_absolute() && UsesParentRoot(layoutSettings, path)) ||
            (!path.has_root_path() && UsesParentRoot(currentSettings, (currentSettings / path).lexically_normal())))
            {
            useParent = true;
            break;
            }
        }

    m_sSavedProjectRoot = PathString(useParent ? currentSettings.parent_path() : currentSettings);
    if (oldSettings.is_absolute())
        {
        m_sOldProjectRoot = PathString(useParent ? oldSettings.parent_path() : oldSettings);
        m_sRelocatedProjectRoot = m_sSavedProjectRoot;
        }
}

Str8 CProject::sInferProjectRoot() const
{
    const fs::path settings = NormalizedPath(m_sProjectFolder);
    CDocList docs;
    for (CShwDoc* doc = docs.pdocFirst(); doc; doc = docs.pdocNext())
        {
        if (UsesParentRoot(settings, NormalizedPath(sUTF8(doc->GetPathName()))))
            return PathString(settings.parent_path());
        }
    return PathString(settings);
}

Str8 CProject::sStoredPath(const Str8& value) const
{
    if (value.IsEmpty() || value.Find('<') == 0)
        return value;

    const fs::path settings = NormalizedPath(m_sProjectFolder);
    if (!settings.is_absolute())
        return value;

    fs::path path = NormalizedPath(value);
    if (!path.has_root_path() && value.Find(':') < 0)
        path = (settings / path).lexically_normal();
    if (!path.is_absolute())
        return value;

    // Standalone settings writes can run while the project is closing.
    // Use the same root as the saved open-file list during those writes.
    const Str8& snapshotRoot = m_sSavingProjectRoot.IsEmpty() ? m_sSavedProjectRoot : m_sSavingProjectRoot;
    const fs::path root = NormalizedPath(snapshotRoot.IsEmpty() ? sInferProjectRoot() : snapshotRoot);
    fs::path suffix;
    if (!PathWithin(path, root, suffix))
        return PathString(path);
    if (PathWithin(path, settings, suffix))
        return PathString(fs::path(L".") / suffix);
    return PathString(fs::path(L"..") / suffix);
}

Str8 CProject::sResolvedPath(const Str8& value, BOOL bRelocateLegacy) const
{
    if (value.IsEmpty() || value.Find('<') == 0)
        return value;

    const fs::path path = NormalizedPath(value);
    if (!path.has_root_path() && value.Find(':') < 0)
        {
        const fs::path settings = NormalizedPath(m_sProjectFolder);
        if (settings.is_absolute())
            return PathString((settings / path).lexically_normal());
        }
    else if (path.is_absolute() && bRelocateLegacy && !m_sOldProjectRoot.IsEmpty())
        {
        fs::path suffix;
        if (PathWithin(path, NormalizedPath(m_sOldProjectRoot), suffix))
            return PathString(NormalizedPath(m_sRelocatedProjectRoot) / suffix);
        }
    return value;
}

static Str8 ConvertPathList(const Str8& paths, const CProject& project, bool storing, BOOL relocate = FALSE)
{
    Str8 result;
    int start = 0;
    while (start < paths.GetLength())
        {
        const int end = paths.Find('\n', start);
        const Str8 path = paths.Mid(start, end < 0 ? -1 : end - start);
        result += storing ? project.sStoredPath(path) : project.sResolvedPath(path, relocate);
        if (end < 0)
            break;
        result += '\n';
        start = end + 1;
        }
    return result;
}

Str8 CProject::sStoredPathList(const Str8& paths) const
{
    return ConvertPathList(paths, *this, true);
}

Str8 CProject::sResolvedPathList(const Str8& paths, BOOL bRelocateLegacy) const
{
    return ConvertPathList(paths, *this, false, bRelocateLegacy);
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
    {
        Newline_istream layoutStream(ios);
        Object_istream layout(layoutStream, notlst);
        ReadProjectLayout(layout);
    }
    ios.clear();
    ios.seekg(0);
	// 08-11-1997
    //The class Newline_istream provieds datas from both, Mac and PC
    Newline_istream iosnl(ios);
    Object_istream obs(iosnl, notlst);
    obs.SetProjectPathContext(TRUE);
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
    Str8 sLegacyProjectPath;
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
        else if ( obs.bReadString(psz_ProjectPath, sLegacyProjectPath) )
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
            m_sImportCCT = sResolvedPath(m_sImportCCT, TRUE);
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


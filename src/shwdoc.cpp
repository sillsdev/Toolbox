// shwdoc.cpp : implementation of the CShwDoc class
//

#include "stdafx.h"
#include "toolbox.h"
#include "shw.h"
#include "project.h"
#include "shwdoc.h"
#include "shwview.h"  // bValidateCur()
#include "newdoc_d.h"
#include "obstream.h"  // classes Object_istream, Object_ostream
#include "progress.h"
#include "status.h" // Shw_pbarStatus()
#include "Doclist.h"  //08-28-1997
#include "alan.h" // 1.5.8h 

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CShwDoc

#undef new
IMPLEMENT_DYNCREATE(CShwDoc, CDocument)
#define new DEBUG_NEW

BEGIN_MESSAGE_MAP(CShwDoc, CDocument)
    //{{AFX_MSG_MAP(CShwDoc)
    ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
    ON_COMMAND(ID_FILE_SAVE, OnFileSave)
    ON_COMMAND(ID_FILE_CLOSE, OnFileClose)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CShwDoc construction/destruction

CShwDoc::CShwDoc() :
    m_pindset(NULL),
    m_bReadOnly(FALSE)
{
    // NOTE NOTE NOTE: The File New, File Open, and Autoload operations
    // *must* set the markup type and index set (before any window is shown).
    // (This is the two-phase construction protocol which the MFC
    // application framework requires for documents and views.)  
}

CShwDoc::~CShwDoc()
{
    delete m_pindset;
}


static const char* psz_db = "db";
static const char* psz_ReadOnly = "ReadOnly";
static const char* psz_TextFile = "TextFile";
static const char* psz_wndlst = "wndlst";
static const char* psz_wnd = "wnd";
static const char* psz_Margin = "Margin";

#ifdef prjWritefstream // 1.6.4aa 
void CShwDoc::WriteProperties(Object_ofstream& obs, BOOL bProjectList)
#else
void CShwDoc::WriteProperties(Object_ostream& obs, BOOL bProjectList)
#endif
{

    const Str8 path = sUTF8(GetPathName());
    obs.WriteBeginMarker(psz_db, bProjectList ? Shw_pProject()->sStoredPath(path) : path);
    obs.WriteBool(psz_ReadOnly, m_bReadOnly);
    
    obs.WriteBeginMarker(psz_wndlst);
    POSITION pos = GetFirstViewPosition();
    while ( pos )
        {
        CShwView* pshv = (CShwView*)GetNextView(pos); // increments pos
        pshv->WriteProperties(obs);
        }
    obs.WriteEndMarker(psz_wndlst);
    
    obs.WriteEndMarker(psz_db);
    obs.WriteNewline();
}


// UpdateAppPath() helper function might belong in shw.cpp
// 1999-03-01 MRP: Moved part of this function to sGetAppPath() in shw.cpp
static void UpdateAppPath (Str8& sPath)
// if sPath begins with <AppPath> token, replaces token with the Shoebox folder
{
    const char* pszAppPathToken = "<AppPath>";
    if (sPath.Find(pszAppPathToken)==0)
        {
        Str8 sAppPath = sGetAppPath();  // 1999-03-01 MRP
        sPath = sAppPath + sPath.Mid(strlen(pszAppPathToken));
        }
}


static BOOL s_bDocumentWasAlreadyOpen = FALSE;  // 1996-09-05 MRP

BOOL CShwDoc::s_bReadProperties(Object_istream& obs)
{
    Str8 sPath;
    if ( !obs.bReadBeginMarker(psz_db, sPath) )
        return FALSE;
        
    BOOL bReadOnly = FALSE;
	BOOL bTextFile = FALSE; // 1.1ef
    
    if (obs.bProjectPaths())
        sPath = Shw_pProject()->sResolvedPath(sPath, obs.bRelocateLegacyPaths());
    UpdateAppPath(sPath);

    // 1995-11-10; 1996-07-24 MRP
    if ( !bFileExists(sPath) ) // || !bAllASCIIComplain( sPath ) )  // If the file does not exist // 1.4vze Complain and refuse if path contains upper ANSI // 1.5.9ka Allow Unicode file names
        {
        if ( !Shw_pProject()->bOpenMovedFile(&sPath) )
            {
            // Read the rest of the non-existent database's properties
            while ( !obs.bAtEnd() )
                {
                if ( obs.bEnd(psz_db) )
                    break;
                }
            return TRUE;  // NOTE: FALSE means no database properties were read
            }
        }
        
    CShwView::s_SetObjectStream(obs);  // 1996-09-05 MRP
    BOOL bDatabaseEndMarker = FALSE;
    CShwDoc* pdoc = NULL;
    while ( !bDatabaseEndMarker && !obs.bAtEnd() )
        {
        if ( obs.bReadBeginMarker(psz_wndlst) )
            {
            BOOL bHaveReadFirstWindow = FALSE;
            
            while ( !obs.bAtEnd() )
                {
                if ( obs.bAtBeginMarker(psz_wnd) )
                    {
                    if ( bHaveReadFirstWindow )
                        {
                        ASSERT( pdoc );
                        CDocTemplate* pTemplate = pdoc->GetDocTemplate();
                        ASSERT_VALID(pTemplate);
                        CFrameWnd* pwndFrame = pTemplate->CreateNewFrame(pdoc, NULL);
                        if ( pwndFrame )
                            pTemplate->InitialUpdateFrame(pwndFrame, pdoc);
                        else
                            {
                            ASSERT( FALSE );  // 1995-09-05 MRP: DO WHAT???
                            }
                        }
                    else
                        {
                        ASSERT( !pdoc );

                        // We need to allow for the case where <sPath> no longer exists
                        // or is invalid.
                        if (bFileExists(sPath))
                            {
                            // 1996-09-05 MRP: OnOpenDocument clears this flag
                            s_bDocumentWasAlreadyOpen = TRUE;
                            pdoc = (CShwDoc*)AfxGetApp()->OpenDocumentFile( swUTF16( sPath) ); // 1.4qxe Upgrade OpenDocumentFile for Unicode build
                            // If the project files have been moved or
                            // upgraded from DOS, files from different folders
                            // but with the same name and extension get aliased.
                            // For now we just skip any after the first.
                            if ( s_bDocumentWasAlreadyOpen )
                                {
                                pdoc = NULL;
                                }
                            }

                        if (pdoc)
                            {
//                            pdoc->SetReadOnly(bReadOnly); // 1.4ysa Fix bug of falsely clearing read only
                            bHaveReadFirstWindow = TRUE;
                            }
                        else
                            {   // File did not exist
                                // Therefore read to the end of the information
                                // about this database file then try next file                          
                            while ( !obs.bAtEnd() )
                                {
                                if ( obs.bEnd(psz_db) )
                                    {
                                    bDatabaseEndMarker = TRUE; 
                                    break;
                                    }
                                }
                                break;
                            }
                        }
                        
                    // We let the MFC application framework calls above
                    // create the view in its normal size and on top,
                    // then minimize it (if necessary) and set its z-order
                    // to what it had been when Shoebox was closed.                     
                    CShwView* pview = Shw_papp()->pviewActive();
                    ASSERT( pview );
                    ASSERT( pview->GetDocument() == pdoc );
                    pview->ShowInitialStateAndZ();
                    }
                else if ( obs.bEnd(psz_wndlst) )
                    break;
                }
            }
        else if ( obs.bReadBool(psz_ReadOnly, bReadOnly) )
            ;
        else if ( obs.bReadBool(psz_TextFile, bTextFile) ) // 1.1ef
            ;
        else if ( obs.bEnd(psz_db) )
            bDatabaseEndMarker = TRUE; 
        }       
    CShwView::s_ClearObjectStream();  // 1996-09-05 MRP

    return TRUE;
}


CMarkerSet* CShwDoc::pmkrset() { return m_pindset->ptyp()->pmkrset(); }
CFilterSet* CShwDoc::pfilset() { return m_pindset->ptyp()->pfilset(); }
CInterlinearProcList* CShwDoc::pintprclst()
        { return m_pindset->ptyp()->pintprclst(); }

void CShwDoc::RecordModified( CRecLookEl* prel, BOOL bUpdateDateStamp ) // record modified (not thru view), update indexes if necessary
{
    if ( bUpdateDateStamp )
        {
        CMarker* pmkrDateStamp = ptyp()->pmkrDateStamp();
        if ( pmkrDateStamp )
            prel->prec()->ApplyDateStamp( pmkrDateStamp ); // freshen or add datestamp field
        }
    // Update the sort keys of this record in all indexes of this database.
    // Phase I-- Add record elements which correspond to added or modified
    // primary key fields. Return a list of those record elements which
    // correspond to modified or deleted fields.
    pindset()->bUpdateSortKeys(prel);

    // Phase IIa-- Cause any other views of this document which are showing
    // this same record to clear their modified flags and to insure that
    // their current record element is up-to-date.
    CRecordHint hint(prel->prec(), CRecordHint::Validated);
    UpdateAllViews(NULL, 0L, &hint);

    // Phase III-- Now that we are sure that no view has an obsolete
    // current record element, delete obsolete elements from all indexes.
    CIndexUpdate iup( pindset(), prel->prec(), INDEX_UPDATE_MODIFY );
    extern void Shw_Update(CUpdate& up);        
    Shw_Update(iup);  // Notify interlinear processes that this record changed

    // 1998-03-26 MRP: Merging a marker with another must be considered
    // a change to all databases that contain it.
    SetModifiedFlag(TRUE);
}

void CShwDoc::MergeMarker( CMarker* pmkrTo, CMarker* pmkrFrom )
{
    CIndex* pind = pindset()->pindRecordOwner(); // ramble thru owning index
    CNumberedRecElPtr prel = pind->pnrlFirst();
    for ( ; prel; prel = pind->pnrlNext( prel ) ) // check each record for occurences of pmkrFrom
        { // NOTE: most of this function should probably be pushed down to the index and record
        BOOL bRecordChanged = FALSE;
        CField* pfld = prel->prec()->pfldFirst();
        for ( ; pfld; pfld = prel->prec()->pfldNext( pfld ) )
            {
            if ( pfld->pmkr() == pmkrFrom ) // this field uses the marker we're getting rid of!
                {
                pfld->SetMarker( pmkrTo ); // change to marker we're merging with
                bRecordChanged = TRUE;
                }
            }
        if ( bRecordChanged )
            RecordModified( prel ); // reindex and such as necessary
        }
}

void CShwDoc::Update(CUpdate& up)
{
    if ( !m_pindset )  // If reading its type before its data has been read
        return;
        
    // 1. Update the data
    up.UpdateShwDoc(this);
    
    // 2. Update all views of the data  
    //    (Documents take responsibility for updating their views)
    POSITION pos = GetFirstViewPosition();
    while ( pos )
        {
        CView* pView = GetNextView(pos);
        ASSERT_VALID( pView );
        CShwView* pShwView = (CShwView*) pView;
        ASSERT( pShwView->IsKindOf(RUNTIME_CLASS(CShwView)) );
        pShwView->Update(up);
        }
    
    // 3. Delete obsolete data entities: fields and record elements
    //    (This step may be encapsulated later by index iterators)
    m_pindset->DoneUpdatingViews();
}

void CShwDoc::MarkerUpdated(CMarkerUpdate& mup)
{
    if ( !mup.bModified() )
        return;

    CIndex* pindRecordOwner = m_pindset->pindRecordOwner();

    if ( mup.pmkrMerge() ) // if not NULL, then we're merging markers
        MergeMarker( mup.pmkr(), mup.pmkrMerge() ); // merge 'em
    else if ( pindRecordOwner->bMarkerUpdateModifiesDatabase(mup) )
        SetModifiedFlag(TRUE);
    else if ( pindRecordOwner->bMarkerUpdateRequiresReSorting(mup) )
        SetModifiedFlag(TRUE);
        
    // Delegate any actual resorting to the document's index set.
    m_pindset->MarkerUpdated(mup);
}

void CShwDoc::FilterUpdated(CFilterUpdate& fup)
{
// 2000/04/28 TLB & MRP - Remove this optimization because the fup now has multiple conditions that are of interest to the indexes
//    if ( !fup.bModified() )
//        return;

    // Delegate any actual refiltering to the document's index set.
    m_pindset->FilterUpdated(fup);
}

void CShwDoc::LangEncUpdated(CLangEncUpdate& lup)
{
    if ( !lup.bModified() )
        return;

    CIndex* pindRecordOwner = m_pindset->pindRecordOwner();
    if ( pindRecordOwner->bLangEncUpdateRequiresReSorting(lup) )
        SetModifiedFlag(TRUE);  // Affects database content
        
    // Delegate any actual resorting to the document's index set.
    m_pindset->LangEncUpdated(lup);
}

void CShwDoc::SortOrderUpdated(CSortOrderUpdate& sup)
{
    if ( !sup.bModified() )
        return;

    CIndex* pindRecordOwner = m_pindset->pindRecordOwner();
    const CSortOrder* psrt = sup.psrt();
    if ( pindRecordOwner->psrtPriKey() == psrt && sup.bNameModified() )
        SetModifiedFlag(TRUE);  // Affects database header
    else if ( pindRecordOwner->bSortOrderUpdateRequiresReSorting(sup) )
        SetModifiedFlag(TRUE);  // Affects database content
        
    // Delegate any actual resorting to the document's index set.
    m_pindset->SortOrderUpdated(sup);
}


void CShwDoc::SetTitle(const char* pszDocumentPath)
{
    // Let the document title consist of the filename only.
    // Omit the drive and directory.
    CDocument::SetTitle(  swUTF16( sGetFileName(pszDocumentPath) ) ); // 1.4qxc Upgrade SetTitle for Unicode build
}

CShwView* CShwDoc::pviewJumpTarget( CIndex* pind, CNumberedRecElPtr* pprel )
{
    // Return a jump target view based on pind, else return NULL.
    // If the view has a different index than the jump path,
    // also return an appropriate record element via *pprel.
    ASSERT( pprel );  // NOTE: *pprel is NULL if it's a Jump Insert

    // 1998-06-18 MRP: First prefer an index that matches exactly,
    // e.g. no filtering, no secondary sorting. Because it's an
    // exact match, the record element *pprel in a normal Jump
    // is appropriate, so it doesn't need to be updated.
    POSITION pos = GetFirstViewPosition();
    while (pos != NULL)
        {
        CShwView* pview = (CShwView*)GetNextView(pos);
        ASSERT_VALID(pview);
        if ( pview->pind() == pind && pview->bIsJumpTarget() )
            return pview;
        }

    // 1998-03-27 MRP: A jump target must have the appropriate
    // primary sorting marker, but other index properties
    // don't have to be identical, especially secondary sorting.
    // Fixes Jump Insert bug for MDF's \lx \hm sorting.
    pos = GetFirstViewPosition();
    while (pos != NULL)
        {
        CShwView* pview = (CShwView*)GetNextView(pos);
        ASSERT_VALID(pview);
        const CMarker* pmkrPriKey = pind->pmkrPriKey();
        CIndex* pindView = (CIndex*)pview->pind();
        const CMarker* pmkrPriKeyView = pindView->pmkrPriKey();
        if ( pmkrPriKeyView == pmkrPriKey && pview->bIsJumpTarget() )
            {
            // 1998-08-03 MRP: If it's a Jump Insert, there's no
            // record element yet, so the calling code doesn't
            // need to get back a corresponding element.
            if ( !*pprel )
                return pview;
                
            // 1998-06-18 MRP: Exchange the record element in the index
            // specified in the jump path for one in this view's index.
            // Because it may be filtered or sorted differently,
            // the record number may be different, or there may not even be
            // a corresponding record element. If there isn't, the calling
            // function will have to open a new window with the jump index.
            CNumberedRecElPtr prelView = pindView->prelCorrespondingTo(*pprel);
            if ( (CRecLookEl*)prelView != NULL )  // 1998-12-02 MRP
                {
                *pprel = prelView;
                return pview;
                }
            }
        }

    return NULL; // No such view: a new window will be opened
}

BOOL CShwDoc::SaveModified()
{
    if ( !bValidateViews() )
        return FALSE;
    if ( IsModified() ) // 1.4tee Fix bug (1.4t) of writing empty file on close window
		{
		int iReply = IDYES;
		Str8 sFileName = _("Save changes?"); // 1.5.0fg 
		sFileName = sFileName + " " + sUTF8( GetPathName() ); // 1.5.0fg 
		iReply = AfxMessageBox( sFileName, MB_YESNOCANCEL ); // Ask if user wants to save changes
		if ( iReply == IDCANCEL ) // If cancel, don't close
			return FALSE;
		else if ( iReply == IDYES ) // 1.4tee If save, save now so framework won't need to
				OnSaveDocument( sUTF8( GetPathName() ) ); // 1.4qxd Upgrade OnSaveDocument for Unicode build
		else // 1.4tee If don't save, clear modified flag so framework won't save
			SetModifiedFlag( FALSE ); // 1.4tee
		}

      //08-29-1997
      //if the last file is going to be closed (but not the mainframe or
      //the whole application) a messagebox should appear
    if (!(Shw_papp()->bGetClosingProjectOrMainFrame()))
        { 
        CDocList doclst; // list of currently open docs
        doclst.pdocFirst();
        if (!doclst.pdocNext())
            //there is only one document open -> Dialog
            {
			AfxMessageBox( _("You cannot close the last file of a project.") ); // 1.4vnb Don't allow close of last file of project
            return FALSE;
            }
        }

	CProject* pprj = Shw_papp()->pProject();
	if ( pprj->bExerciseNoSave() ) // 5.99f Don't save if exercises
		return TRUE;

    BOOL bClosing = Shw_papp()->bClosing() || Shw_pProject()->bClosing() ||
                    !Shw_pProject()->bWarnDocClose() || Shw_ptypset()->bOkToClose( this );
    if (bClosing)
		{
		Str8 sDocPath =  sUTF8( GetPathName() ); // 1.4qxf
		if ( !bReadOnly() ) // 1.4aj Fix bug of not preserving read-only attribute of data file
			UnWriteProtect( sDocPath ); // 1.3db Fix bug of falsely asking for Save As on close file // 1.4qzhg
        bClosing = CDocument::SaveModified();
		}
    return bClosing;
}

/////////////////////////////////////////////////////////////////////////////

BOOL CShwDoc::OnNewDocument()
{
    if (!CDocument::OnNewDocument())
        return FALSE;

    DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT |  // defaults
        OFN_PATHMUSTEXIST |  // Dialog requires that the path be valid
        OFN_NOREADONLYRETURN;  // Dialog requires that the file
            // not be read-only, nor be in a write-protected directory
	Str8 sFileFilter = _("All files"); // 1.5.0fz 
	sFileFilter += " (*.*)|*.*||"; // 1.5.0fz 
	Str8 sPath; // 1.5.8h 
	if ( !Shw_papp()->bUseCrashFixFileEditDlg() ) // 1.5.8h 
		{
		CFileDialog dlgNew(FALSE, NULL, GetPathName(), dwFlags, swUTF16( sFileFilter ) ); // 1.4qrd // 1.4yp  // 1.5.8h 
		dlgNew.m_ofn.lpstrTitle = _T(" "); // 1.4qzhs Fix bad letters in file browse box title
		dlgNew.SetHelpID(IDD_NEWDATABASE);
		if ( iDlgDoModalSetReg( &dlgNew ) != IDOK )
			return FALSE;
		sPath =  sUTF8( dlgNew.GetPathName() ); // 1.4qxf
		}
	else // 1.5.8h 
		if ( !bCrashFixBrowseForFileName(sPath, sPath) ) // 1.5.8h 
			return FALSE; // 1.5.8h 
//	if ( !bAllASCIIComplain( sPath ) ) // 1.4vze  // 1.5.9ka Allow Unicode file names
//		return FALSE; // 1.4vze  // 1.5.9ka Allow Unicode file names

    CDatabaseTypeProxy* ptrx = NULL;
    CNewDocDlg dlgNewDoc( sGetFileName(sPath, TRUE), &ptrx );
    dlgNewDoc.SetHelpID(IDD_NEWDB_SELTYPE);
    if ( dlgNewDoc.DoModal() != IDOK )
        return FALSE;

    CNoteList notlst;
    CDatabaseTypePtr ptyp = ptrx->ptyp(notlst); // load in type if not already loaded
        
    m_pindset = new CIndexSet(ptyp, 400, FALSE); // default margin setting of 400 pixels
    m_pindset->pindRecordOwner()->prelInsertRecord("");
    m_pindset->DoneUpdatingViews();  // Must empty nrllstInserted

    // write it immediately even though it's empty;
    // otherwise we'll get a Save As box when we try to save or close later
    if ( !bWrite(sPath) )
        {
        if ( bFileExists(sPath) )
            remove(sPath);
        // Note: The common dialog notifies the user of write-protected disks.
        Str8 sMessage;
		sMessage = sMessage + _("Cannot open file:") + " " + sPath; // 1.5.0fd 
        MessageBeep(0);
        AfxMessageBox(sMessage, MB_ICONEXCLAMATION);
        return FALSE;
        }

    // Set the document title and path
    SetPathName( swUTF16( sPath) ); // 1.4qxb
    SetTitle( sGetFileName( sPath, TRUE ) );
    
    return TRUE;
}
    
BOOL CShwDoc::OnOpenDocument( const char* pszPath )
{
    ASSERT( pszPath );
    // 1998-04-22 MRP: Set the path and file name here so it's available
    // for the converting datestamps progress message.
    SetPathName( swUTF16( pszPath) ); // 1.4qxb Upgrade SetPathName for Unicode build
    SetTitle(pszPath);

	if ( Shw_papp()->bWriteProtectedSettings() )
		UnWriteProtect( sUTF8( GetPathName() ) ); // 1.2ap Turn off write protect of data files if settings were write protected, probably came from CD // 1.4qxa // 1.4qzhg
    Str8 sMessage;    
    if ( !bRead(pszPath, sMessage) )
        {
        // Note: If the user Cancels the Import dialog,
        // the message will be empty because no feedback is required.
        if ( !sMessage.IsEmpty() )
            {
            MessageBeep(0);
            AfxMessageBox(sMessage, MB_ICONEXCLAMATION);
            }
        return FALSE;
        }
	if ( !(Shw_papp()->pProject()->iLockProject() >= 10) && !Shw_papp()->pProject()->bExerciseNoSave() ) // 1.2ms If exercise, don't write protect data file // 1.2gy Don't write protect data files at level 10 lock
		WriteProtect( pszPath ); // 1.2an Write protect data file as signal that it is open by an instance of Toolbox // 1.4qzhh

    s_bDocumentWasAlreadyOpen = FALSE;
    return TRUE;
}

void CShwDoc::OnCloseDocument()
{
	if ( !bReadOnly() )
		UnWriteProtect( sUTF8( GetPathName() ) ); // 1.2an Turn off write protect of data file to say it is no longer open by Toolbox // 1.4qxa // 1.4qzhg
    // need to notify all search paths that we're closing
    Shw_ptypset()->OnDocumentClose( this );

      //10-24-1997 Update the language pointer in the statusbarclass
    Shw_pbarStatus()->SetRecordInfo("","",NULL,NULL);
    Shw_pbarStatus()->SetPriKeyInfo("","",NULL,NULL);

    CDocument::OnCloseDocument();
}

BOOL CShwDoc::OnSaveDocument( const char* pszPath )
{
    ASSERT( pszPath );
	if ( !bReadOnly() )
		UnWriteProtect( sUTF8( GetPathName() ) ); // 1.2an Turn off write protect of data file while writing it // 1.4qzhg
    Str8 sMessage;    
    if ( !bSave(pszPath, sMessage) )
        {
        MessageBeep(0);
        AfxMessageBox(sMessage, MB_ICONEXCLAMATION);
        return FALSE;
        }

	if ( !Shw_papp()->pProject()->bExerciseNoSave() ) // 1.2ms If exercise, don't write protect data file
		WriteProtect( pszPath ); // 1.2an Write protect data file as signal that it is open by an instance of Toolbox // 1.4qzhh
    ResetUndoForAllViews();

	Shw_app().pProject()->SetSaved( TRUE ); // 1.1tu Note that something has been saved
    return TRUE;
}

BOOL CShwDoc::bSaveDocument()
{
    if ( !bValidateViews() )
        return FALSE;

	OnFileSave();

	if ( IsModified() ) // Save failed
		return FALSE;

	ResetUndoForAllViews();

    return TRUE;
}

void CShwDoc::ResetUndoForAllViews()
{
	// When an non-undoable action has occurred that affects multiple
	// records in the database, call this function to reset the Undo
	// and Undo All options for all views of the document.
    POSITION pos = GetFirstViewPosition(); // reset undo buffers in views
    while (pos != NULL)
        {
        CShwView* pView = (CShwView*)GetNextView(pos);
        ASSERT_VALID(pView);
        pView->ResetUndoAll();
        pView->ResetUndo();
        }
}

void CShwDoc::ResetUndoForAllViews( CRecord* prec )
{
	// When an non-undoable action has occurred that affects a single
	// record in the database, call this function to reset the Undo
	// and Undo All options for any views of the document where this record
	// is currently being edited.
	ASSERT( prec );
    POSITION pos = GetFirstViewPosition(); // reset undo buffers in views
    while (pos != NULL)
        {
        CShwView* pView = (CShwView*)GetNextView(pos);
        ASSERT_VALID(pView);
        pView->ResetUndo( prec );
        }
}



BOOL CShwDoc::bValidateViews( BOOL bAutoSave, CShwView* pviewDontValidate )
{
    POSITION pos = GetFirstViewPosition();
    while (pos != NULL)
        {
        CShwView* pView = (CShwView*)GetNextView(pos);
        ASSERT_VALID(pView);
        if ( pviewDontValidate != pView
                && !pView->bValidate() ) // validate any modified view
            return FALSE;
        }

	if ( Shw_pProject()->bAutoSave() && bAutoSave ) // If autosaving, save doc and settings
		{
		if ( IsModified() )
			OnSaveDocument( sUTF8( GetPathName() ) ); // 1.4qxd
		Shw_pProject()->DoAutoSave();
		}
    
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CShwDoc diagnostics

#ifdef _DEBUG
void CShwDoc::AssertValidity() const
{
    AssertValid();  // MFC base class
    m_pindset->AssertValid();
    POSITION posView = GetFirstViewPosition();
    while ( posView )
        {
        CShwView* pview = (CShwView*)GetNextView(posView);  // updates pos
        ASSERT( pview->IsKindOf(RUNTIME_CLASS(CShwView)) );
//      pview->AssertValidity();
        }
}

void CShwDoc::AssertValid() const
{
    CDocument::AssertValid();
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CShwDoc commands

void CShwDoc::OnFileSaveAs()
{
    if ( !bValidateViews() )
        return;
    BOOL bModified = IsModified();
    Str8 sName =  sUTF8( GetPathName() ); // save current name and title // 1.4qxf
	Str8 sPath = sGetDirPath( sName ); // 1.4yp
    Str8 sTitle =  sUTF8( GetTitle() ); // 1.4qxf
	if ( !Shw_papp()->bUseCrashFixFileEditDlg() ) // 1.5.8h 
		{
		CFileDialog dlgSaveAs( FALSE, NULL, GetPathName() ); // 1.4vze  // 1.5.8h 
		dlgSaveAs.m_ofn.lpstrInitialDir =  swUTF16( sPath ); // 1.4yp Make File, Save As default to current folder
		dlgSaveAs.m_ofn.lpstrTitle = _T(" "); // 1.4vze 
		if ( iDlgDoModalSetReg( &dlgSaveAs ) != IDOK)  // 1.4vze Do separate file dialog on SaveAs so file name can be checked
			return; // 1.4vze 
		sPath =  sUTF8( dlgSaveAs.GetPathName() ); // 1.4qxf
		}
	else // 1.5.8h 
		if ( !bCrashFixBrowseForFileName(sPath, sPath) ) // 1.5.8h 
			return; // 1.5.8h 
//	if ( !bAllASCIIComplain( sPath ) ) // 1.4vze  // 1.5.9ka Allow Unicode file names
//		return; // 1.4vze  // 1.5.9ka Allow Unicode file names
	SetPathName( swUTF16( sPath ), FALSE ); // 1.4vze  // 1.5.8h 
	if ( !bReadOnly() ) // 1.4raf
		UnWriteProtect( sName ); // 1.4raf Fix bug of File, Save As leaving old file write protected
//	Str8 sNewName =  sUTF8( dlgSaveAs.GetPathName() ); // 1.4vze 
//	SetReadOnly( FALSE ); // 1.4wb Fix bug of File, Save As leaving new file write protected
//	if ( m_pindset ) // 1.4wb 
//		m_pindset->SetReadOnly( FALSE ); // 1.4wb 
//	UnWriteProtect( sNewName ); // 1.4wb Fix bug of File, Save As leaving new file write protected
//    CDocument::OnFileSaveAs();
//	sName =  sUTF8( GetPathName() ); // get new name // 1.4raf // 1.4rah
//		UnWriteProtect( sName ); // 1.4raf Fix bug of File, Save As leaving old file write protected // 1.4rah
	OnSaveDocument( sUTF8( GetPathName() ) ); // 1.4qxd Upgrade OnSaveDocument for Unicode build // 1.4rah Fix bug of File, Save As not writing file
    SetModifiedFlag( bModified );
}

// override to do validation
void CShwDoc::OnFileSave()
{
    if ( !bValidateViews() )
        return;

    (void) Shw_app().bSaveAllFiles(); // AB 2-Apr-2002 Change to save all so button will save all
	Shw_UpdateAllViews(NULL); // 1.4sm Fix bug (1.4ae) of not showing date stamp on save
//    CDocument::OnFileSave();
}

BOOL CShwDoc::bNumber(CShwView* pview, CRecLookEl* prelCur, BOOL bDoBreak)
{
    if ( !ptyp()->bNumber(pview, prelCur, bDoBreak) )
        return FALSE;

    UpdateAllViews(NULL, 0L, 0);
    return TRUE;
}

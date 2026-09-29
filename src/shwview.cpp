// shwview.cpp : implementation of the CShwView class
//

#include "stdafx.h"
#include "toolbox.h"
#include "dlgs.h"
#include "shw.h"

#include "mainfrm.h"
#include "shwdoc.h"
#include "shwview.h"
#include "project.h"
#include "typ.h"
#include "find_d.h"
#include "progress.h"
#include "obstream.h"  // classes Object_istream, Object_ostream
#include "kmn.h"
#include "mkr_d.h"
#include "wav.h"
#include "doclist.h" // CTypedDocList, 03/29/99 - TLB
#include "cck.h"  // CFieldConsistencyChecker
#include "SHConv.h" // 1.4qzhm // 1.4zam 
// #ifndef _MAC
#include <mmsystem.h>   // MM_MCINOTIFY
// #endif

#ifdef _MAC
#include "resmac.h" // Mac specific resources
#else
#include "respc.h"
#endif
#include "hide_d.h"

#include <fstream>

#include <afxpriv.h> // For WM_COMMANDHELP
#include <atlconv.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CShwView

#undef new
IMPLEMENT_DYNCREATE(CShwView, CShwScrollView)
#define new DEBUG_NEW

BEGIN_MESSAGE_MAP(CShwView, CShwScrollView)
    //{{AFX_MSG_MAP(CShwView)
    ON_COMMAND(ID_DATABASE_NEXT_RECORD, OnDatabaseNextRecord)
    ON_COMMAND(ID_DATABASE_PREVIOUS_RECORD, OnDatabasePreviousRecord)
    ON_COMMAND(ID_DATABASE_FIRST_RECORD, OnDatabaseFirstRecord)
    ON_COMMAND(ID_DATABASE_LAST_RECORD, OnDatabaseLastRecord)
    ON_COMMAND(ID_DATABASE_SEARCH, OnDatabaseSearch)
    ON_COMMAND(ID_DATABASE_INSERT_RECORD, OnDatabaseInsertRecord)
    ON_COMMAND(ID_DATABASE_DELETE_RECORD, OnDatabaseDeleteRecord)
    ON_COMMAND(ID_DATABASE_COPY_RECORD, OnDatabaseCopyRecord)
    ON_COMMAND(ID_DATABASE_MOVE_RECORD, OnDatabaseMoveRecord)
    ON_COMMAND(ID_DATABASE_MERGE_DB, OnDatabaseMergeDb)
    ON_COMMAND(ID_DATABASE_FILTERING, OnDatabaseFiltering)
    ON_UPDATE_COMMAND_UI(ID_DATABASE_FILTERING, OnUpdateDatabaseFiltering)
    ON_COMMAND(ID_DATABASE_SORTING, OnDatabaseSorting)
    ON_COMMAND(ID_DATABASE_PROPERTIES, OnDatabaseProperties)
    ON_UPDATE_COMMAND_UI(ID_DATABASE_PROPERTIES, OnUpdateDatabaseProperties)
    ON_COMMAND(ID_DATABASE_MARKERS, OnDatabaseMarkers)
    ON_UPDATE_COMMAND_UI(ID_DATABASE_NEXT_RECORD, OnUpdateDatabaseNextRecord)
    ON_UPDATE_COMMAND_UI(ID_DATABASE_PREVIOUS_RECORD, OnUpdateDatabasePreviousRecord)
    ON_UPDATE_COMMAND_UI(ID_DATABASE_FIRST_RECORD, OnUpdateDatabaseFirstRecord)
    ON_UPDATE_COMMAND_UI(ID_DATABASE_LAST_RECORD, OnUpdateDatabaseLastRecord)
    ON_UPDATE_COMMAND_UI(ID_DATABASE_INSERT_RECORD, OnUpdateDatabaseInsertRecord)
    ON_UPDATE_COMMAND_UI(ID_DATABASE_DELETE_RECORD, OnUpdateDatabaseDeleteRecord)
    ON_UPDATE_COMMAND_UI(ID_DATABASE_COPY_RECORD, OnUpdateDatabaseCopyRecord)
    ON_UPDATE_COMMAND_UI(ID_DATABASE_MOVE_RECORD, OnUpdateDatabaseMoveRecord)
    ON_UPDATE_COMMAND_UI(ID_DATABASE_MERGE_DB, OnUpdateDatabaseMergeDb)
    ON_COMMAND(ID_WINDOW_DUPLICATE, OnWindowDuplicate)
    ON_COMMAND(ID_FILE_OUTPUT, OnFileExport)
    ON_UPDATE_COMMAND_UI(ID_FILE_OUTPUT, OnUpdateFileExport)
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_AS, OnUpdateFileSaveAs) // 1.4wb 
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_ALL, OnUpdateFileSaveAll)
    ON_UPDATE_COMMAND_UI(ID_FILE_CLOSE, OnUpdateFileClose)
    ON_WM_CHAR()
    ON_WM_CREATE()
    ON_WM_SETFOCUS()
    ON_WM_KILLFOCUS()
    ON_WM_KEYDOWN()
    ON_WM_LBUTTONDOWN()
    ON_COMMAND(ID_VIEW_MARKERS, OnViewMarkers)
    ON_COMMAND(ID_VIEW_FIELDNAMES, OnViewFieldNames)
    ON_UPDATE_COMMAND_UI(ID_VIEW_FIELDNAMES, OnUpdateViewFieldNames)
    ON_UPDATE_COMMAND_UI(ID_VIEW_MARKERS, OnUpdateViewMarkers)
    ON_COMMAND(ID_EDIT_MARKER, OnEditMarker)
    ON_UPDATE_COMMAND_UI(ID_EDIT_MARKER, OnUpdateEditMarker)
    ON_COMMAND(ID_EDIT_INSERT_FROM_RANGE_SET, OnEditInsertFromRangeSet)
    ON_WM_MOUSEMOVE()
    ON_WM_SETCURSOR()
    ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
    ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
    ON_COMMAND(ID_EDIT_CUT, OnEditCut)
    ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)
    ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
    ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
    ON_COMMAND(ID_EDIT_UNDOALL, OnEditUndoAll)
    ON_UPDATE_COMMAND_UI(ID_EDIT_UNDOALL, OnUpdateEditUndoAll)
    ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
    ON_WM_LBUTTONUP()
    ON_COMMAND(ID_TOOLS_CHECK_UNICODE_VALIDITY, OnToolsCheckUnicodeValidity)
    ON_COMMAND(ID_TOOLS_COMPARE, OnToolsCompareFiles)
    ON_COMMAND(ID_EDIT_INTERLINEARIZE, OnEditInterlinearize)
    ON_COMMAND(ID_INTERLIN_SHOW_FAIL, OnEditInterlinShowFail)
    ON_COMMAND(ID_EDIT_ADAPT, OnEditAdapt)
    ON_COMMAND(ID_EDIT_ADAPT_CONT, OnEditAdaptCont)
    ON_COMMAND(ID_EDIT_SPELLCHECK, OnEditSpellCheck)
	ON_COMMAND(ID_TOOLS_BREAK_NBR_TXT, OnToolsNumbering)
    ON_COMMAND(ID_TOOLS_RENUMBER_TXT, OnToolsReNumber)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_BREAK_NBR_TXT, OnUpdateToolsNumbering)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_RENUMBER_TXT, OnUpdateToolsReNumber)
	ON_COMMAND(ID_TOOLS_CHECK_CONSISTENCY, OnToolsCheckConsistency)
	ON_COMMAND(ID_TOOLS_NEW_CONSISTENCY_CHECK, OnToolsNewConsistencyCheck)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_NEW_CONSISTENCY_CHECK, OnUpdateToolsNewConsistencyCheck)
	ON_COMMAND(ID_TOOLS_CONTINUE_CONSISTENCY_CHECK, OnToolsContinueConsistencyCheck)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_CONTINUE_CONSISTENCY_CHECK, OnUpdateToolsContinueConsistencyCheck)
    ON_WM_SIZE()
    ON_WM_MOVE()
	ON_WM_WINDOWPOSCHANGED()
	ON_WM_WINDOWPOSCHANGING()
    ON_WM_LBUTTONDBLCLK()
    ON_COMMAND(ID_VIEW_BROWSE, OnViewBrowse)
    ON_COMMAND(ID_VIEW_SIDEBROWSE, OnViewSideBrowse)
    ON_UPDATE_COMMAND_UI(ID_VIEW_BROWSE, OnUpdateViewBrowse)
    ON_COMMAND(ID_VIEW_BROWSEFIELDS, OnViewBrowsefields)
    ON_COMMAND(ID_VIEW_HIDEFIELDS, OnViewHideFields)
    ON_COMMAND(ID_VIEW_FIELDS_TO_HIDE, OnViewFieldsToHide)
    ON_COMMAND(ID_VIEW_REAPPLY_FILTER, OnViewReapplyFilter)
    ON_UPDATE_COMMAND_UI(ID_VIEW_REAPPLY_FILTER, OnUpdateViewReapplyFilter)
    ON_UPDATE_COMMAND_UI(ID_EDIT_INSERT_FROM_RANGE_SET, OnUpdateEditInsertFromRangeSet)
    ON_UPDATE_COMMAND_UI(ID_EDIT_INTERLINEARIZE, OnUpdateEditInterlinearize)
    ON_UPDATE_COMMAND_UI(ID_INTERLIN_SHOW_FAIL, OnUpdateEditInterlinShowFail)
    ON_UPDATE_COMMAND_UI(ID_EDIT_ADAPT, OnUpdateEditAdapt)
    ON_UPDATE_COMMAND_UI(ID_EDIT_ADAPT_CONT, OnUpdateEditAdaptCont)
    ON_UPDATE_COMMAND_UI(ID_EDIT_INT_WORD, OnUpdateEditInterlinearizeWord)
    ON_UPDATE_COMMAND_UI(ID_EDIT_SPELLCHECK, OnUpdateEditSpellCheck)
    ON_COMMAND(ID_VIEW_MARKER_HIERARCHY, OnViewMarkerHierarchy)
    ON_COMMAND(ID_VIEW_JUMP_TARGET, OnViewJumpTarget)
    ON_UPDATE_COMMAND_UI(ID_VIEW_JUMP_TARGET, OnUpdateViewJumpTarget)
    ON_COMMAND(ID_VIEW_FOCUS_TARGET, OnViewFocusTarget)
    ON_UPDATE_COMMAND_UI(ID_VIEW_FOCUS_TARGET, OnUpdateViewFocusTarget)
    ON_UPDATE_COMMAND_UI(ID_VIEW_HIDEFIELDS, OnUpdateViewHideFields)
    ON_UPDATE_COMMAND_UI(ID_VIEW_MARKER_HIERARCHY, OnUpdateViewMarkerHierarchy)
    ON_COMMAND(ID_EDIT_USE_AS_TEMPLATE, OnEditUseAsTemplate)
    ON_UPDATE_COMMAND_UI(ID_EDIT_USE_AS_TEMPLATE, OnUpdateEditUseAsTemplate)
    ON_COMMAND(ID_EDIT_JUMPTO, OnEditJumpto)
    ON_UPDATE_COMMAND_UI(ID_EDIT_JUMPTO, OnUpdateEditJumpto)
    ON_COMMAND(ID_EDIT_JUMPINSERT, OnEditJumpinsert)
    ON_UPDATE_COMMAND_UI(ID_EDIT_JUMPINSERT, OnUpdateEditJumpinsert)
    ON_COMMAND(ID_PROJECT_LANGUAGE_ENCODINGS, OnFileLanguageEncodings)
    ON_UPDATE_COMMAND_UI(ID_PROJECT_LANGUAGE_ENCODINGS, OnUpdateFileLanguageEncodings)
    ON_COMMAND(ID_PROJECT_DATABASE_TYPES, OnFileDatabaseTypes)
    ON_UPDATE_COMMAND_UI(ID_PROJECT_DATABASE_TYPES, OnUpdateFileDatabaseTypes)
    ON_UPDATE_COMMAND_UI(ID_PROJECT_TEXT_CORPUS, OnUpdateFileTextCorpus)
    ON_COMMAND(ID_VIEW_AUTOWRAP, OnViewAutowrap)
    ON_COMMAND(ID_VIEW_SET_MARGIN, OnViewSetMargin)
    ON_UPDATE_COMMAND_UI(ID_VIEW_AUTOWRAP, OnUpdateViewAutowrap)
    ON_COMMAND(ID_EDIT_RESHAPE, OnEditReshape)
    ON_COMMAND(ID_EDIT_RESHAPE_DB, OnEditReshapeDb)
    ON_WM_ERASEBKGND()
    ON_UPDATE_COMMAND_UI(ID_EDIT_RESHAPE, OnUpdateEditReshape)
    ON_UPDATE_COMMAND_UI(ID_EDIT_RESHAPE_DB, OnUpdateEditReshapeDb)
    ON_WM_RBUTTONDOWN()
    ON_COMMAND(ID_EDIT_FIND, OnEditFind)
    ON_UPDATE_COMMAND_UI(ID_EDIT_FIND, OnUpdateEditFind)
    ON_COMMAND(ID_EDIT_REPLACE, OnEditReplace)
    ON_UPDATE_COMMAND_UI(ID_EDIT_REPLACE, OnUpdateEditReplace)
    ON_COMMAND(ID_DEBUG_TOM, OnExerciseToggle)
    ON_COMMAND(ID_EXERCISE_UPDATE, OnExerciseUpdate)
//    ON_COMMAND(ID_DATABASE_TYPE_UPDATE, OnDatabaseTypeUpdate) // 1.2ra Add option to upgrade database type
    ON_COMMAND(ID_DEBUG_ALAN, OnDebugAlan)
    ON_COMMAND(ID_ALAN_DEBUG2, AlanDebug2)
    ON_COMMAND(ID_ALAN_DEBUG3, AlanDebug3)
    ON_COMMAND(ID_TOGGLE_INTERNAL_KEYBOARD, ToggleInternalKeyboard)
//    ON_COMMAND(ID_TOOLS_MACROS, OnToolsMacros) // 1.5.0ah 
    ON_COMMAND(ID_ACTIVATE_FILTER_COMBO, ActivateFilterCombo)
    ON_COMMAND(ID_DEBUG_KAREN, OnDebugKaren)
    ON_COMMAND(ID_VIEW_BOTHMARKERSANDNAMES, OnViewBothmarkersandnames)
    ON_UPDATE_COMMAND_UI(ID_VIEW_BOTHMARKERSANDNAMES, OnUpdateViewBothmarkersandnames)
    ON_COMMAND(ID_FILE_PAGE_SETUP, OnFilePageSetup)
    ON_COMMAND(ID_EDIT_CTRL_SPACE, OnEditCtrlSpace)
    ON_COMMAND(ID_EDIT_SELECT_ALL, OnEditSelectAll)
    ON_UPDATE_COMMAND_UI(ID_EDIT_SELECT_ALL, OnUpdateEditSelectAll)
    ON_COMMAND(ID_EDIT_DELWORD_LEFT, OnEditDelwordLeft)
    ON_COMMAND(ID_EDIT_DELWORD_RIGHT, OnEditDelwordRight)
    ON_COMMAND(ID_EDIT_SELECT_LINE, OnEditSelectLine)
    ON_COMMAND(ID_EDIT_SELECT_FIELD, OnEditSelectField)    
	//}}AFX_MSG_MAP
#ifndef _MAC
    ON_COMMAND(ID_EDIT_PLAYSOUND, OnEditPlaysound)
    ON_UPDATE_COMMAND_UI(ID_EDIT_PLAYSOUND, OnUpdateEditPlaysound)
    ON_COMMAND(ID_EDIT_PLAYFILE, OnEditPlayFile)
    ON_UPDATE_COMMAND_UI(ID_EDIT_PLAYFILE, OnUpdateEditPlayFile)
#endif // _MAC
    // Standard printing commands
    ON_COMMAND(ID_FILE_PRINT, CShwScrollView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_PREVIEW, CShwScrollView::OnFilePrintPreview)
#ifdef _MAC
    ON_COMMAND(ID_WINDOW_CLOSE, OnWindowClose)
#endif
    ON_MESSAGE(MM_MCINOTIFY, OnMCNotify)
#ifndef _MAC
	ON_MESSAGE(WM_UNICHAR, OnUnicodeChar)
//    ON_MESSAGE(WM_INPUTLANGCHANGE,OnInputLangChange) // 1.4qzhm
#endif _MAC
    ON_COMMAND(ID_EDIT_INSERT_FIELD, OnEditInsertField)
    ON_UPDATE_COMMAND_UI(ID_EDIT_INSERT_FIELD, OnUpdateEditInsertField)
    ON_COMMAND(ID_EDIT_RETURN_FROM_JUMP, OnEditReturnFromJump)
    ON_UPDATE_COMMAND_UI(ID_EDIT_RETURN_FROM_JUMP, OnUpdateEditReturnFromJump)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CShwView construction/destruction
static int s_iFldStartColForMarker = 50; // Field start column if showing marker only
static int s_iFldStartColForMarkerHierarchy = 15;
static int s_iFldStartColForFieldName = 100; // Field start column if showing field name only
                        // If showing both, field start is set to the sum of the two

CShwView::CShwView()
{
    m_iMaxLineWd = 100; // Scroll width, init to minimum
    m_prelCur.Set( NULL, 0 ); // Current record element
    m_iSelection = 0; // status of any text selection
    m_iTracking = 0; // state of mouse capture
    m_iFldStartCol = s_iFldStartColForMarker; // Default starting column for field data
    m_bViewMarkers = TRUE; // Show marker in display
    m_bViewMarkerHierarchy = FALSE; // Indent marker to show hierarchy
    m_bViewFieldNames = FALSE; // Show field name in display
    m_bModifiedSinceValidated = FALSE; // Current record element modified
    m_bCanUndoAll = FALSE;
    m_pszUndoAllBuffer = NULL;
    m_precUndoAll = NULL;
    m_eInitialShowCmd = SW_SHOWNORMAL;
    m_z = 0;
    m_lVPosUL = 0;
    m_lTotalHt = 0;
    m_bScrollOverflow = FALSE;       // BW 2/3/98
    m_bScrollOverflowWarning = FALSE;
    m_bBrowsing = FALSE;
    m_bIsJumpTarget = TRUE; // AB 12-14-01 Change default to jump target
    m_bFocusTarget = TRUE; // 1.4qzjb
    m_bViewLocked = FALSE; // 1.2bn
	m_iVerifyPassNumber = 1; // 5.97x Default to pass 1
	m_bVerifySkippedFail = FALSE; // Verify skipped at least one failure
    m_pind = NULL;
    m_iWhatChanged = eNothing;
    m_pindUnfiltered = NULL;
    m_keyInsert.SetKey("", TRUE, CMCharOrder::matsetSamePrimaryGroup); // record insert defaults to matching whole word
//    m_keyJump.SetKey("", TRUE, CMCharOrder::matsetSamePrimaryGroup); // jump defaults to matching whole word
//    m_keyJumpInsert.SetKey("", TRUE, CMCharOrder::matsetSamePrimaryGroup); // jump insert defaults to matching whole word
    m_bDontCallSetCaretPosAdj = FALSE;
    m_iInterlinCharWd = 0;
	m_bRecalculatePixelDisplayStarts = TRUE;
    m_rpsPixelDisplayStarts.SetPos(0, NULL, NULL);
    memset(m_iPixelDisplayStarts, 0, sizeof(m_iPixelDisplayStarts));
    m_iCaretHt = 0;  // 1998-01-26 MRP: Initialize
    m_bCaretHidden = FALSE;
    m_bHaveFocus = FALSE;
    m_iMaxLineLength = 1000;  // 07-31-1997
    m_bLineLengthMessageBoxShowed = FALSE;  // 07-31-1997
    m_iInitLine = 0; // Line number
    m_iInitLineScroll = 0; // Number of lines from current line to upper left
    m_iInitChar = 0; // Character number
    m_iXWidth = 0;
    m_iBrowseHeaderHeight = 0;
    m_pcck = NULL;  // 1999-06-11 MRP: Constructed when-if needed
#ifndef _UNICODE // 1.4zam 
    m_bConvertNext = FALSE; // 1.4qzhm // 1.4zam 
    m_cpIM = CP_ACP; // 1.4qzhm // 1.4zam 
#endif  // 1.4zam 
	m_bUnicodeValidityCheckInProcess = FALSE; // 1.4hr
	m_pdocDefaultForMove = NULL; // 1.5.3c
    m_hWavDevice = 0;
    m_ppat = NULL;
    m_prcplst = NULL;
    m_prcpCur = NULL;
}

CShwView* CShwView::s_pViewToDuplicate = NULL;
CRecLookEl* CShwView::s_prelJumpTo = NULL;
const char* CShwView::s_pszJumpInsert = NULL;
CShwDoc* CShwView::s_pdocJumpTo = NULL;
CIndex* CShwView::s_pindJumpTo = NULL;

void CShwView::Duplicate()
{
    ASSERT( s_pViewToDuplicate );

    // Window Duplicate: Analogous to a copy constructor, but delayed
    // as required by the application framework's "two-phase" construction.
    // NOTE NOTE NOTE: Whenever a new member variable is added to this class,
    // it must be accounted for here as well as in the default constructor.
    
    m_pind = s_pViewToDuplicate->m_pind;
    m_pindUnfiltered = s_pViewToDuplicate->m_pindUnfiltered;
    m_keySearch.SetKey(s_pViewToDuplicate->m_keySearch);
    m_keyInsert.SetKey(s_pViewToDuplicate->m_keyInsert);
//    m_keyJump.SetKey(s_pViewToDuplicate->m_keyJump);
//    m_keyJumpInsert.SetKey(s_pViewToDuplicate->m_keyJumpInsert);
        
    m_prelCur = s_pViewToDuplicate->m_prelCur;
    m_rpsCur = s_pViewToDuplicate->m_rpsCur;
    m_rpsSelOrigin = s_pViewToDuplicate->m_rpsSelOrigin;
    m_iSelection = s_pViewToDuplicate->m_iSelection;
    m_rpsUpperLeft = s_pViewToDuplicate->m_rpsUpperLeft;

    m_pntCaretLoc = s_pViewToDuplicate->m_pntCaretLoc;
    m_lTotalHt = s_pViewToDuplicate->m_lTotalHt;
    m_lVPosUL = s_pViewToDuplicate->m_lVPosUL;

    m_iFldStartCol = s_pViewToDuplicate->m_iFldStartCol;
    m_iMaxLineWd = s_pViewToDuplicate->m_iMaxLineWd;
    m_iInterlinCharWd = s_pViewToDuplicate->m_iInterlinCharWd;
	m_bRecalculatePixelDisplayStarts = s_pViewToDuplicate->m_bRecalculatePixelDisplayStarts;
    
    m_iXWidth = s_pViewToDuplicate->m_iXWidth;

    m_bBrowsing = s_pViewToDuplicate->m_bBrowsing;
    m_iBrowseHeaderHeight = s_pViewToDuplicate->m_iBrowseHeaderHeight; //09-12-1997
    m_bIsJumpTarget = s_pViewToDuplicate->m_bIsJumpTarget; // 1.4zaf Make Window, Duplicate keep jump and parallel move target settings
    m_bFocusTarget = s_pViewToDuplicate->m_bFocusTarget; // 1.4zaf Make Window, Duplicate keep jump and parallel move target settings
    m_bViewLocked = FALSE; // 1.2bn
    m_prelUL = s_pViewToDuplicate->m_prelUL;
    m_brflst = s_pViewToDuplicate->m_brflst;
   
    m_bViewMarkers = s_pViewToDuplicate->m_bViewMarkers;
    m_bViewMarkerHierarchy = s_pViewToDuplicate->m_bViewMarkerHierarchy;
    m_bViewFieldNames = s_pViewToDuplicate->m_bViewFieldNames;
                                                            
    m_bModifiedSinceValidated = s_pViewToDuplicate->m_bModifiedSinceValidated;
    
    m_eInitialShowCmd = s_pViewToDuplicate->m_eInitialShowCmd;
        
    s_pViewToDuplicate = NULL;
}

CShwView::~CShwView()
{
    delete m_pcck;  // 1999-06-11 MRP
    if ( m_pszUndoAllBuffer )
        delete m_pszUndoAllBuffer;
    // m_pind->DecrementNumRefs();
    ASSERT( !s_pViewToDuplicate );
}

void CShwView::SetIndex(CIndex* pind, CIndex* pindUnfiltered /*,
        CNumberedRecElPtr* pprelCur */)
{
    if ( pind == m_pind )
        return;

    // Access the record element first, since it will be deleted below
    // if its index's reference count becomes zero.
    CRecord* precCur = m_prelCur ? m_prelCur->prec() : NULL; // may be switching from empty record set

    m_pind = pind;
    ASSERT( m_pind );
    m_pindUnfiltered = pindUnfiltered;
    ASSERT( m_pindUnfiltered );

    // Return the first record element which represents the _record_.
    CNumberedRecElPtr prel = m_pind->prelFind(precCur);
    if ( prel )
        ASSERT( prel->prec() == precCur );
    else
        {
        ASSERT( m_pind->bUsingFilter() || !precCur );
        prel = m_pind->pnrlFirst();
        }

    SetCur(prel);

    // 2000/05/01 TLB & MRP - If we've chosen an index that has pending updates (because of a non-unique
    // filter), we should force the index to reapply the filter now. This will clean up the m_prelCur for
    // this view and all other views that use this index, so we won't need to look for the corresponding
    // rel if we do this.
    if ( m_pind->bModifiedSinceFiltered() )
        {
        CFilterUpdate fup(m_pind);
        Shw_Update(fup);
        }
}


static const char* psz_wnd = "wnd";
static const char* psz_placement = "placement";
static const char* psz_z = "z";
static const char* psz_ViewMarkers = "ViewMarkers";
static const char* psz_ViewMarkerHierarchy = "ViewMarkerHierarchy";
static const char* psz_ViewFieldNames = "ViewFieldNames";
static const char* psz_widthMarkerArea = "widthMarkerArea";
static const char* psz_Cur = "Cur";
static const char* psz_browse = "Browse";
static const char* psz_JumpTarget = "JumpTarget";
static const char* psz_FocusTarget = "FocusTarget";  // 1998-09-24 MRP
static const char* psz_ViewLocked = "ViewLocked";
static const char* psz_Search = "Search";
static const char* psz_Insert = "Insert";
static const char* psz_Jump = "Jump";
static const char* psz_JumpInsert = "JumpInsert";
static const char* psz_CaretLine = "CaretLine";
static const char* psz_CaretLineScroll = "CaretLineScroll";
static const char* psz_CaretChar = "CaretChar";

#ifdef prjWritefstream // 1.6.4aa 
void CShwView::WriteProperties(Object_ofstream& obs)
#else
void CShwView::WriteProperties(Object_ostream& obs)
#endif
{
    obs.WriteBeginMarker(psz_wnd);
    CWnd* pwndFrame = GetParent();
    obs.WriteWindowPlacement(psz_placement, pwndFrame);
    obs.WriteInteger(psz_z, m_z);
    obs.WriteBool(psz_ViewMarkers, m_bViewMarkers);
    obs.WriteBool(psz_ViewMarkerHierarchy, m_bViewMarkerHierarchy);
    obs.WriteBool(psz_ViewFieldNames, m_bViewFieldNames);
    obs.WriteInteger(psz_widthMarkerArea, m_iFldStartCol);
    obs.WriteBool(psz_browse, m_bBrowsing);
    obs.WriteBool(psz_JumpTarget, m_bIsJumpTarget);
    obs.WriteBool(psz_FocusTarget, m_bFocusTarget);  // 1998-09-24 MRP
    obs.WriteBool(psz_ViewLocked, m_bViewLocked);
    m_brflst.WriteProperties(obs);
    
    m_pind->WriteProperties(obs);
    m_pind->WriteRecordRef(obs, psz_Cur, m_prelCur);

    m_keySearch.WriteProperties(obs, psz_Search);
    m_keyInsert.WriteProperties(obs, psz_Insert);
//    m_keyJump.WriteProperties(obs, psz_Jump);
//    m_keyJumpInsert.WriteProperties(obs, psz_JumpInsert);

    // If a current record has not been set, don't try to use it
    // 1998-02-10 MRP: When browsing, there's no reason to save the caret
    // position, since it will change when the view changes to record mode.
    // Attempting to do so caused the caret to disappear at File Save All,
    // if the active window wasn't in browse mode.
    if ( m_rpsCur.pfld && !m_bBrowsing )
        {
        if ( m_prelCur ) // Handle empty view from filter // AB 3.0.8r added section below to fix bug of GPF on Save All when cursor in inactive window is in a deleted line
            { // resync upper left visible position in case stuff was deleted further back record
            m_lVPosUL = -pntPosToPnt( CRecPos( 0, m_prelCur->prec()->pfldFirst(), m_prelCur->prec() ), TRUE ).y;
            if ( bPositionValid( m_rpsCur ) )
                m_pntCaretLoc = pntPosToPnt( m_rpsCur ); // set caret to previous record position
            else
                m_pntCaretLoc = pntPntToPos( m_pntCaretLoc ); // set caret to previous pixel position
            }
        CRecPos rps = m_rpsCur; // Find out line position of current to save it
        int iChar = rps.iChar; // Remember char number
        bHome( rps ); // Move to home
        int iLineScroll = 0; // Number of lines scroll top is above current line
		int iLine = 0;
        for ( ; ; iLine++ ) // Count lines to top of file
            {
            if ( rps.pfld == m_rpsUpperLeft.pfld ) // If we come to the scroll upper left field, remember how far up it was
                iLineScroll = iLine;
            if ( !iUp( rps ) )
                break;
            }
        obs.WriteInteger( psz_CaretLine, iLine );
        obs.WriteInteger( psz_CaretLineScroll, iLineScroll );
        obs.WriteInteger( psz_CaretChar, iChar );
        if ( m_pcck )
		    m_pcck->WriteProperties(obs);  // 1999-08-30 TLB
        }
	obs.WriteUnrecognizedSettingsInfo( m_sUnrecognizedSettingsInfo ); // 1.0cp Write unrecognized settings info

    obs.WriteEndMarker(psz_wnd);
}

BOOL CShwView::bReadProperties(Object_istream& obs)
{
    if ( !obs.bReadBeginMarker(psz_wnd) )
        return FALSE;

    BOOL bViewMarkers = FALSE;
    BOOL bViewMarkerHierarchy = FALSE;
    BOOL bViewFieldNames = FALSE;
	m_bIsJumpTarget = FALSE; // 6.0zb Fix bug of not remembering that jump target has been turned off for view
	m_bFocusTarget = FALSE; // 6.0zb Fix bug of not remembering that jump target has been turned off for view
	m_bViewLocked = FALSE; // 1.2bn

    CKey keyOldSettings; // dummy object for discarding older settings
        
    CWnd* pwndFrame = GetParent();
    CShwDoc* pdoc = GetDocument();
    CIndexSet* pindset = pdoc->pindset();

    while ( !obs.bAtEnd() )
        {
        if ( obs.bReadWindowPlacement(psz_placement, pwndFrame, m_eInitialShowCmd) )
            ;
        else if ( obs.bReadInteger(psz_z, m_z) )
            ;
        else if ( obs.bReadBool(psz_ViewMarkers, bViewMarkers) )
            ;
        else if ( obs.bReadBool(psz_ViewMarkerHierarchy, bViewMarkerHierarchy) )
            ;
        else if ( obs.bReadBool(psz_ViewFieldNames, bViewFieldNames) )
            ;
        else if ( obs.bReadInteger(psz_widthMarkerArea, m_iFldStartCol) )
            ;
        else if ( obs.bReadBool(psz_browse, m_bBrowsing) )
            ;
        else if ( obs.bReadBool(psz_JumpTarget, m_bIsJumpTarget) )
            ;
        else if ( obs.bReadBool(psz_FocusTarget, m_bFocusTarget) )  // 1998-09-24 MRP
            ;
        else if ( obs.bReadBool(psz_ViewLocked, m_bViewLocked) )
            ;
        else if ( m_brflst.bReadProperties(obs, GetDocument()->pmkrset() ) )
            ;
            // NOTE NOTE This needs work.
            
        else if ( !m_pind && pindset->bReadIndexProperties(obs, m_pind, m_pindUnfiltered) )
            ;
        else if ( m_pind && m_pind->bReadRecordRef(obs, psz_Cur, m_prelCur) )
            ;       
        else if ( m_keySearch.bReadProperties(obs, psz_Search) )
            ;
        else if ( m_keyInsert.bReadProperties(obs, psz_Insert) )
            ;
//        else if ( m_keyJump.bReadProperties(obs, psz_Jump) )
//            ;
//        else if ( m_keyJumpInsert.bReadProperties(obs, psz_JumpInsert) )
//            ;
        else if ( obs.bReadInteger( psz_CaretLine, m_iInitLine ) )
            ;
        else if ( obs.bReadInteger( psz_CaretLineScroll, m_iInitLineScroll ) )
            ;
        else if ( obs.bReadInteger( psz_CaretChar, m_iInitChar ) )
            ;
        else if ( CFieldConsistencyChecker::s_bReadProperties(obs, this, &m_pcck) )
            ;
        else if ( keyOldSettings.bReadProperties(obs, psz_Jump) )
            ;
        else if ( keyOldSettings.bReadProperties(obs, psz_JumpInsert) )
            ;
		else if ( obs.bUnrecognizedSettingsInfo( psz_wnd, m_sUnrecognizedSettingsInfo  ) ) // 1.0cp Store any unrecognized settings info so as not to lose it
			;
        else if ( obs.bEnd(psz_wnd) ) // Look for end marker; skip unexpected field
            break;
        }

	if ( !Shw_papp()->pProject()->bLockProject() ) // 1.2fb If project is not locked, don't let any views be locked
		m_bViewLocked = FALSE;

    // Set defaults if these properties were not read
    if ( bViewMarkers || bViewFieldNames )
        {
        m_bViewMarkers = bViewMarkers;
        m_bViewMarkerHierarchy = bViewMarkerHierarchy;
        m_bViewFieldNames = bViewFieldNames;
        }
    // else defaults to View Markers and Marker Hierarchy set in constructor

    if ( !m_pind )    
        m_pind = m_pindUnfiltered = pindset->pindRecordOwner();
    if ( !m_prelCur )   
        m_prelCur = m_pind->pnrlFirst();

    return TRUE;
}

Object_istream* CShwView::s_pobsAutoload = NULL;

void CShwView::s_SetObjectStream(Object_istream& obs)
{
    ASSERT( !s_pobsAutoload );
    s_pobsAutoload = &obs;
}


void CShwView::s_ClearObjectStream()
{
    s_pobsAutoload = NULL;
}


void CShwView::ShowInitialStateAndZ()
{
    CMDIChildWnd* pwnd = pwndChildFrame();

    // Minimize this window if it had been when Shoebox was closed.
    if ( m_eInitialShowCmd == SW_SHOWMINIMIZED )
        pwnd->ShowWindow(m_eInitialShowCmd);

    // Find the MDI child window which corresponds to the one --out of those
    // opened so far-- just above this when Shoebox was closed.
    int zThis = z();
    CShwView* pviewAboveThis = NULL;
    CShwView* pview = Shw_papp()->pviewBelow(this);
#ifdef _MAC
    CShwView* pviewTop = pview;
#endif
    for ( ; pview; pview = Shw_papp()->pviewBelow(pview) )
        if ( zThis < pview->z() )       
            pviewAboveThis = pview;

    // Set this window's z-order. (The framework has opened it on top.)
    if ( pviewAboveThis )
        {
        BOOL bSetZ = pwnd->SetWindowPos(pviewAboveThis->pwndChildFrame(),
            0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        ASSERT( bSetZ );
#ifdef _MAC
        Shw_pmainframe()->MDIActivate(pviewTop->pwndChildFrame()); // make it stick
#endif
        }
}

void CShwView::ShowInitialTopState()
{
    CMDIChildWnd* pwnd = pwndChildFrame();

    // Make this the active MDI child window, since it had been
    // the top when Shoebox was closed.
    CMDIFrameWnd* pwndMainFrame = (CMDIFrameWnd*)Shw_papp()->m_pMainWnd;
    ASSERT( pwndMainFrame->IsKindOf(RUNTIME_CLASS(CMDIFrameWnd)) );
    pwndMainFrame->MDIActivate(pwnd);

    // Maximize this window if it had been when Shoebox was closed.
    if ( m_eInitialShowCmd == SW_SHOWMAXIMIZED )
        pwnd->ShowWindow(m_eInitialShowCmd);
	ResynchCaretAndCursor(); // 1.5.1ra Fix bug of cursor sometimes wrong on startup
}


CMDIChildWnd* CShwView::pwndChildFrame() const
{
    CMDIChildWnd* pwnd = (CMDIChildWnd*)GetParent();
    ASSERT( pwnd );
    ASSERT( pwnd->IsKindOf(RUNTIME_CLASS(CMDIChildWnd)) );
    
    return pwnd;
}

CShwView* CShwView::s_pviewActiveChild(CMDIChildWnd* pwnd)
{
    ASSERT( pwnd->IsKindOf(RUNTIME_CLASS(CMDIChildWnd)) );
    CShwView* pview = (CShwView*)pwnd->GetActiveView();
//    ASSERT( pview ); // 1.4zba Fix U bug of possible error message on mouse click
//    ASSERT( pview->IsKindOf(RUNTIME_CLASS(CShwView)) ); // 1.4zba 
    
    return pview;
}


/////////////////////////////////////////////////////////////////////////////
// CShwView drawing



void CShwView::OnDraw(CDC* pDC)
{
    if ( m_bBrowsing )
        {
        HideCaret();
        DrawBrowse( pDC ); // Draw browse view
        }
    else
        {
        if ( !m_prelCur ) // If empty record set, don't draw
            return;
        HideCaret();
        DrawRec( pDC ); // Draw the record
        }
    ShowCaret();
}

void CShwView::OnInitialUpdate()
{
    InitializeRec();
}

// BJY - Calling bPositionValid() unconditionally may cause a real performance hit for records
// with a large number of fields.  The hint system should probably handle passing on
// information such as what field was deleted or that a multiple field deletion occurred.

void CShwView::OnUpdate( CView* pSender, LPARAM lHint, CObject* poHint )
{
    if ( m_bBrowsing )
        {
        }
    else if (m_prelCur)
        {
        if ( !bPositionValid( m_rpsUpperLeft ) ) // our frame of reference may have been deleted by another view
            ResetUpperLeft();
        if ( bSelecting( eAnyText ) && ( !bPositionValid( m_rpsCur ) || !bPositionValid( m_rpsSelOrigin ) ) )
            {
            ClearSelection();
            Invalidate(FALSE);
            }
        }
    CHint* pHint = (CHint*)poHint;
    Update(pHint);
}


/////////////////////////////////////////////////////////////////////////////
// CShwView printing

void CShwView::OnPrepareDC( CDC* pDC, CPrintInfo* pInfo)
// Prepares DC for printing.
{
    if (pDC->IsPrinting())
        {
        CPrintProperties* pprtp = (CPrintProperties*)pInfo->m_lpUserData;

        m_nMapMode = MM_LOENGLISH;

        // If we reached the end of the browse records on the last page and are trying to go to
        // the next page, then stop printing.
        if (m_bBrowsing && pprtp->bEndOfBrowse() 
                && ((UINT)(pprtp->pptplst()->lGetCount() + 1) <= pInfo->m_nCurPage) )
            {
            pInfo->m_bContinuePrinting = FALSE;
            pprtp->SetEndOfBrowse(FALSE);   // Set it back for the sake of print preview.
            return;
            }

        // Move to the proper page.
        Print_MoveToPage(pInfo, pDC);
        // This will set pInfo->m_bContinuePrinting FALSE if the user asked to
        // print beyond the end of the printed pages.

        if (!pInfo->m_bContinuePrinting)
            return;

        // First, check to see if we should continue printing.

        // In the case of printing a record, no remaining fields means stop printing.
        // If we're printing records, then also factor in the record.
        if (m_bBrowsing)
            {
            if ((CRecLookEl*)pprtp->prel() == NULL)  // 1998-12-02 MRP
                {
                pInfo->m_bContinuePrinting = FALSE;
                return;
                }
            }
        else
            {
            if (pprtp->bPrintingAllRecords())
                {
                if ( pprtp->rps().pfld == NULL)
                    {
                    // 1998-12-02 MRP: casts
                    if (((CRecLookEl*)pprtp->prel() == NULL) ||
                            (CRecLookEl*)pind()->pnrlNext(pprtp->prel()) == NULL)
                        {
                        pInfo->m_bContinuePrinting = FALSE;
                        return;
                        }
                    else // go to next element
                        {
                        pprtp->prel() = pind()->pnrlNext(pprtp->prel());
                        pprtp->rps().SetPos(0, pprtp->prel()->prec()->pfldFirst(), pprtp->prel()->prec());
                        }
                    }
                }
            else
                {
                if ( pprtp->rps().pfld == NULL)
                    {
                    pInfo->m_bContinuePrinting = FALSE;
                    return;
                    }
                }
            }

        CShwScrollView::OnPrepareDC(pDC, pInfo);    // This sets the mapmode and viewport origin.

        pDC->SetTextAlign( TA_UPDATECP );
        pInfo->m_bContinuePrinting = TRUE;
        }
    else
        {
        m_nMapMode = MM_TEXT;
        CShwScrollView::OnPrepareDC(pDC, pInfo);
        }
}


/////////////////////////////////////////////////////////////////////////////
// CShwView diagnostics

#ifdef _DEBUG
void CShwView::AssertValid() const
{
    CShwScrollView::AssertValid();
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CShwView message handlers

void CShwView::OnDatabaseNextRecord()
{
    // RNE 1995-11-09 Note.  Calling SetStatusRecordAndPrimaryKey(m_prelCur)
    // only for the browse view.  In the normal view, SetCur calls
    // SetStatusRecordAndPrimaryKey(m_prelCur) in the process of moving among
    // the record elements.
    if ( m_bBrowsing )
        {
        bBrowseDown();
        SetStatusRecordAndPrimaryKey(m_prelCur);
        }
    else
        DatabaseNextRecord();
	ParallelJump(); // 6.0y Book reference jump
	if ( !ptyp()->bTextFile() ) // 1.1ky Let top of record show in text file
		FixCursorPosForHidden(); // 1.5.0fp Change to better way of positioning cursor after jump
// 		PlaceCursorAtTopOfWindow(); // 6.0zh In long record, place cursor at top of window // 1.5.0fp 
}

void CShwView::OnDatabasePreviousRecord()
{
    if ( m_bBrowsing )
        {
        bBrowseUp();
        SetStatusRecordAndPrimaryKey(m_prelCur);
        }
    else
        DatabasePreviousRecord();
	ParallelJump(); // 6.0y Book reference jump
	if ( !ptyp()->bTextFile() ) // 1.1ky Let top of record show in text file
		FixCursorPosForHidden(); // 1.5.0fp Change to better way of positioning cursor after jump
// 		PlaceCursorAtTopOfWindow(); // 6.0zh In long record, place cursor at top of window // 1.5.0fp 
}
void CShwView::OnDatabaseFirstRecord()
{
    if ( m_bBrowsing )
        {
        BrowseCtrlHome();
        SetStatusRecordAndPrimaryKey(m_prelCur);
        }
    else
        DatabaseFirstRecord();
	ParallelJump(); // 6.0y Book reference jump
	if ( !ptyp()->bTextFile() ) // 1.1ky Let top of record show in text file
		FixCursorPosForHidden(); // 1.5.0fp Change to better way of positioning cursor after jump
// 		PlaceCursorAtTopOfWindow(); // 6.0zh In long record, place cursor at top of window // 1.5.0fp 
}
void CShwView::OnDatabaseLastRecord()
{
    if ( m_bBrowsing )
        {
        BrowseCtrlEnd();
        SetStatusRecordAndPrimaryKey(m_prelCur);
        }
    else
        DatabaseLastRecord();
	ParallelJump(); // 6.0y Book reference jump
	if ( !ptyp()->bTextFile() ) // 1.1ky Let top of record show in text file
		FixCursorPosForHidden(); // 1.5.0fp Change to better way of positioning cursor after jump
// 		PlaceCursorAtTopOfWindow(); // 6.0zh In long record, place cursor at top of window // 1.5.0fp 
}

void CShwView::OnDatabaseSearch() 
{ 
	if ( !bDatabaseSearch() ) // 1.4wp Fix bug of moving cursor on cancel of search
		return;  // 1.4wp 
	ParallelJump(); // 6.0y Book reference jump
	FixCursorPosForHidden(); // 1.5.0fp Change to better way of positioning cursor after jump
// 	PlaceCursorAtTopOfWindow(); // 6.0zh In long record, place cursor at top of window // 1.5.0fp 
}

void ActivateWindow( CMDIChildWnd* pwnd ); // Activate window after jump

void CShwView::TextParallelJump() // 1.1me ParallelJump from text file, used on mouse click and cursor movements
	{
	if ( ptyp()->bTextFile() && m_pind->pmkrPriKey() == ptyp()->pmkrTextRef() ) // 1.1me If text file sorted by reference, do parallel move
		{ // 1.5.0gu Fix bug of auto save happening too often on mouse clicks
		ParallelJump();
		if ( m_rpsCur.pfld && m_rpsCur.pfld->pmkr() == ptyp()->pmkrTextRef() && Shw_pProject()->bAutoSave() ) // If autosaving, save doc and settings // 1.4vys Do auto save more often on text // 1.5.0gu Fix bug of auto save happening too often on mouse clicks, do auto save only if click is in ref field // 1.5.0je Fix bug (1.5.0gu) of crash on open project with text file in browse
			{
			if ( pdoc()->IsModified() ) // 1.4vys 
				pdoc()->OnSaveDocument( sUTF8( pdoc()->GetPathName() ) ); // 1.4qxd // 1.4vys 
			Shw_pProject()->DoAutoSave(); // 1.4vys 
			}
		}
	}

// extern UINT NEAR WM_SANTA_FE_FOCUS;
// extern LPARAM FROM_TOOLBOX; // 1.2md Signal that this focus message is from Tbx

void CShwView::ParallelJump() // 6.0y Book reference jump
{
	if ( !m_rpsCur.prec ) // 1.0ea Fix bug of crash when last filtered record disappears
		return;
    BOOL bActiveChildIsMaximized;
    Shw_pmainframe()->MDIGetActive(&bActiveChildIsMaximized);
	Str8 sSortSeq = pind()->psrtPriKey()->sName();
	CField* pfld = m_prelCur->pfldPriKey(); // 6.0zc Generalize to parallel jump any language encoding
	if ( ptyp()->bTextFile() && !bBrowsing() && m_pind->pmkrPriKey() == ptyp()->pmkrTextRef() ) // 1.1pb Fix bug of parallel move not responding to text in browse view // 1.1me If text file sorted by reference, find nearest reference
		{
		BOOL bFound = FALSE;
		for ( pfld = m_rpsCur.pfld; pfld; pfld = m_rpsCur.prec->pfldPrev( pfld ) ) // Search from current field upward for reference
			if ( pfld->pmkr() == ptyp()->pmkrTextRef() )
				{
				bFound = TRUE;
				break;
				}
		if ( !bFound ) // If no reference found above, search below
			for ( pfld = m_rpsCur.pfld; pfld; pfld = m_rpsCur.prec->pfldNext( pfld ) ) // Search from current field upward for reference
				if ( pfld->pmkr() == ptyp()->pmkrTextRef() )
					{
					bFound = TRUE;
					break;
					}
		if ( !bFound )
			pfld = NULL;
		}
	if ( pfld )
		{
		BOOL bJumped = FALSE; // 1.4qzfc Note whether any window did a parallel move
		Str8 sWord = pfld->sContents();
		Str8 sLang = pfld->plng()->sName();
		Str8 sFocus( sWord + " \n\n" + sLang ); // 1.1me Fix to use sWord instead of m_sKeyFld so as to correctly do parallel jump from text file // 1.4ak Add space after ref to make TW obey parallel move even if no space after reference
		if ( !bActiveChildIsMaximized ) // 1.4kq Fix bug of not sending ref to Paratext if window maximized // 1.0ck Don't do parallel movement if window is maximized
			bJumped = Shw_papp()->bExternalJump( sFocus, this, FALSE, FALSE ); // Pass in this view so it can be skipped // 6.0za Eliminate No Matches message from BookRef jump // 1.4qzfc // 1.4qzjf
		if ( pfld->plng()->sName() == "Book References" ) // 1.2md If jumping from book reference, broadcast it to other apps
			{
			SendParallelMoveRefExternal( sWord + "\n\n" ); // 1.4kr Make generalized external ref broadcast func // 1.5.4d Don't send language encoding on external parallel move
			}
		if ( !bJumped && bBrowsing() ) // 1.4qzfc If no window moved, and this view is browse, try parallel move with record key
			{
			sWord = m_prelCur->prec()->sKey();
			sFocus = sWord + " \n\n" + sLang; // 1.1me Fix to use sWord instead of m_sKeyFld so as to correctly do parallel jump from text file // 1.4ak Add space after ref to make TW obey parallel move even if no space after reference
			if ( !bActiveChildIsMaximized ) // 1.4kq Fix bug of not sending ref to Paratext if window maximized // 1.0ck Don't do parallel movement if window is maximized
				bJumped = Shw_papp()->bExternalJump( sFocus, this, FALSE, FALSE ); // Pass in this view so it can be skipped // 6.0za Eliminate No Matches message from BookRef jump // 1.4qzfc // 1.4qzjf
			}
		// Make this the active view, since succesful jump will have left it at another view
		CMDIChildWnd* pwnd = pwndChildFrame();
		ActivateWindow( pwnd );
		}
	// 1.6.4a If AGNT, jump lexicon and show highlighting
	Str8 sTyp = ptyp()->sName(); // 1.6.4a Get database type name
	if ( sTyp == "BARTtxt" ) // 1.6.4a If AGNT file, look for lexicon word
		{
		Str8 sWord;
		for ( pfld = m_prelCur->prec()->pfldFirst(); pfld; pfld = m_rpsCur.prec->pfldNext( pfld ) ) // 1.6.4a Search from first field
			{
			if ( pfld->pmkr()->sMarker() == "lg" ) // 1.6.4a Find lex word
				{
				sWord = pfld->sContents(); // 1.6.4a Store lex word for lex search
				break;
				}
			}
		CShwView* pshvLex = NULL;
		CShwView* pshvLexNoNum = NULL;
		CDocList doclst; // list of currently open docs
		for ( CShwDoc* pdoc = doclst.pdocFirst(); pdoc; pdoc = doclst.pdocNext() ) // 1.6.4a Look for lex doc
    		{
			if ( pdoc->ptyp()->sName() == "ANLEX" )
				{
				Str8 sLexFileName = sUTF8( pdoc->GetTitle() ); // 1.6.4a Get file name
				BOOL bNoNum = ( sLexFileName.Find( "NoNum" ) > 0 ); // 1.6.4a See if no numbers file
				POSITION pos = pdoc->GetFirstViewPosition();
				if ( pos )
					{
					CShwView* pshv = (CShwView*)pdoc->GetNextView(pos); // 1.6.4a Get first view of lex
					if ( !bNoNum ) // 1.6.4a If not the NoNumbers file
						{
						if ( !pshvLex ) // 1.6.4a If no lex saved
							pshvLex = pshv; // 1.6.4a Save this as lex
						}
					else // 1.6.4a Else, NoNumbers file save this as lex NoNumbers
						{
						if ( !pshvLexNoNum ) // 1.6.4a If no lex saved
							pshvLexNoNum = pshv; // 1.6.4a Save this as lex
						}
					}
				}
    		}
		if ( pshvLex ) // 1.6.4g fix 1.6.4f bug of not refreshing on same word or sort by lg
			{
			BOOL bJump = pshvLex->bExternalJump( sWord, NULL, FALSE, FALSE ); // 1.6.4a If lex found, search for word // 1.6.4g Test jump after check for db found
			// SetFocus(); // 1.6.4a Put focus back on AGNT window // 1.6.4b Fix 1.6.4a problem of focus left on lex // 1.6.4f Test focus change
			pshvLex->Invalidate(FALSE); // redraw to show result // 1.6.4f Test focus change // 1.6.4g fix bug of not refresh on same word or sort by lg
			CRecLookEl* prelLex = pshvLex->prelCur(); // 1.6.4a Get record jumped to in view
			CRecord* precLex = prelLex->prec();
			CField* pfldLex = precLex->pfldFirst(); // 1.6.4a Get first field
			CField* pfldDef = pfldLex; // 1.6.4a Remember def field
			while ( pfldLex ) // 1.6.4a For each field
				{
				Str8 sMkr = pfldLex->sMarker(); // 1.6.4a Get marker
				if ( sMkr == "def" ) // 1.6.4a If def, remember it
					{
					pfldDef = pfldLex; // 1.6.4a Remember def
					pfldLex = precLex->pfldNext( pfldLex ); // 1.6.4a Move to next field
					break;
					}
				if ( sMkr == "defl" || sMkr == "defd" ) // 1.6.4a If defl or defd change to def
					{
					CMarker* pmkrDef = pshvLex->GetDocument()->pmkrset()->pmkrSearch( "def" ); // 1.6.4a Get def mkr
					pfldLex->SetMarker( pmkrDef ); // 1.6.4a Change defl or defd to def
					pfldDef = pfldLex; // 1.6.4a Remember def
					pfldLex = precLex->pfldNext( pfldLex ); // 1.6.4a Move to next field
					break;
					}
				pfldLex = precLex->pfldNext( pfldLex ); // 1.6.4a Move to next field
				}
			while ( pfldLex ) // 1.6.4a For each field
				{
				Str8 sMkr = pfldLex->sMarker(); // 1.6.4a Get marker
				if ( sMkr == "def" || sMkr == "defl" || sMkr == "defd" ) // 1.6.4a If defl or defd combine into def
					{
					Str8 sCont = pfldLex->sContents();
					pfldDef->Append( sCont ); // 1.6.4a Append content to def
					CField* pfldDelete = pfldLex; // 1.6.4a Remember field to delete
					pfldLex = precLex->pfldNext( pfldLex ); // 1.6.4a Move to next field
					precLex->Delete( pfldDelete ); // 1.6.4a Delete field
					}
				else // 1.6.4a If not any kind of def, move to next
					pfldLex = precLex->pfldNext( pfldLex ); // 1.6.4a Move to next field
				}
			CField* pfldAlink = NULL; // 1.6.4a Remember Alink field
			CField* pfldPlink = NULL; // 1.6.4a Remember Plink field
			Str8 sAlink; // 1.6.4a Alink field contents
			Str8 sPlink; // 1.6.4a Plink field contents
			Str8 sLink; // 1.6.4a Combined link field contents
			for ( pfld = m_prelCur->prec()->pfldFirst(); pfld; pfld = m_rpsCur.prec->pfldNext( pfld ) ) // 1.6.4a Search from first field
				{
				if ( pfld->pmkr()->sMarker() == "A/Alink" ) // 1.6.4a Find Alink field
					{
					pfldAlink = pfld;
					sAlink = pfldAlink->sContents(); // 1.6.4a Alink field contents
					}
				if ( pfld->pmkr()->sMarker() == "a/aPERGlink" ) // 1.6.4a Find Plink field
					{
					pfldPlink = pfld;
					sPlink = pfldPlink->sContents(); // 1.6.4a Plink field contents
					}
				}
			sLink = sAlink + "; " + sPlink; // 1.6.4a Combine links into a single string to ease searching
			int iLen = sLink.GetLength(); // 1.6.4a Length of string
			BOOL bDark = TRUE; // 1.6.4a Remember dark or light most recent
			int iStart = 0; // 1.6.4a Start of highlight range
			int iEnd = 0; // 1.6.4a End of highlight range
			int iLowestStartDone = 5000; // 1.6.4a Lowest done so far, start high
			int iHighestEnd = 0; // 1.6.4a End of highest done so far
			int iHighStart = 0; // 1.6.4a Highest found so far
			int iHighEnd = 0; // 1.6.4a End of highest found so far
			BOOL bHighDark = TRUE; // 1.6.4a Dark or light of highest found so far
			BOOL bMore = TRUE; // 1.6.4a More to check
			while ( bMore ) // 1.6.4a While more to check
				{
				iHighStart = 0; // 1.6.4a Reset high numbers
				iHighEnd = 0;
				BOOL bFound = FALSE; // 1.6.4a Note if a usable formula found
				for ( int iCur = 0; iCur < iLen; iCur++ ) // 1.6.4a For each char
					{
					char cCur = sLink.GetChar( iCur );
					if ( cCur == 'D' )
						bDark = TRUE;
					if ( cCur == 'L' )
						bDark = FALSE;
					if ( cCur == '*' )
						{
						Str8 sDigits; // 1.6.4a String to collect digits
						iCur++; // 1.6.4a Move to first digit
						cCur = sLink.GetChar( iCur );
						while ( isdigit( cCur ) ) // 1.6.4a Collect digits
							{
							sDigits += cCur; // 1.6.4a Collect digit
							iCur++; // 1.6.4a Move to next
							cCur = sLink.GetChar( iCur );
							}
						iStart = atoi( sDigits ); // 1.6.4a Make digit string into int
						iEnd = iStart + 1; // 1.6.4a Default end to one more in case formula not complete
						sDigits = ""; // 1.6.4a Clear num string
						if ( cCur == '-' && sLink.GetChar( iCur + 1 ) == '*' )
							{
							iCur += 2; // 1.6.4a Move past hyphen and star
							cCur = sLink.GetChar( iCur );
							while ( isdigit( cCur ) ) // 1.6.4a Collect digits
								{
								sDigits += cCur; // 1.6.4a Collect digit
								iCur++; // 1.6.4a Move to next
								cCur = sLink.GetChar( iCur );
								}
							iEnd = atoi( sDigits ); // 1.6.4a Make digit string into int
							iCur++; // 1.6.4a Move to next
							cCur = sLink.GetChar( iCur );
							}
						if ( iStart < iLowestStartDone && iStart > iHighStart ) // 1.6.4a If higher than prev highest, remember
							{
							iHighStart = iStart;
							iHighEnd = iEnd;
							bHighDark = bDark;
							bFound = TRUE; // 1.6.4a Found a highlight to do
							}
						}
					}
				if ( bFound ) // 1.6.4a If a highlight is found, do it
					{
					iLowestStartDone = iHighStart; // 1.6.4a Remember this as lowest one done so far
					Str8 sStart; // 1.6.4a Make number into string to search for it
					sStart += iHighStart; // 1.6.4a 
					sStart = "*" + sStart + "*"; // 1.6.4a Put stars into search
					Str8 sEnd; // 1.6.4a Make number into string to search for it
					sEnd += iHighEnd; // 1.6.4a 
					sEnd = "*" + sEnd + "*"; // 1.6.4a Put stars into search
					int iLexEnd = pfldDef->sContents().Find( sEnd ); // 1.6.4a Find end
					if ( iLexEnd < 0 )
						iLexEnd = pfldDef->sContents().GetLength(); // 1.6.4a If not found, set to end
					if ( iLexEnd < pfldDef->sContents().GetLength() - sEnd.GetLength() ) // 1.6.4a If not at end, break off end as def
						{
						CField* pfldNewDef = new CField( pfldDef->pmkr() ); // 1.6.4a Make new def field to insert
						precLex->InsertAfter( pfldDef, pfldNewDef ); // 1.6.4a Insert new def field
						pfldNewDef->SetContent( pfldDef->Mid( iLexEnd ) ); // 1.6.4a Set new field to tail
						pfldDef->Delete( iLexEnd, pfldDef->GetLength() - iLexEnd ); // 1.6.4a Remove tail
						}
					Str8 sDefdl = "defl"; // 1.6.4a Marker for defl or defd
					if ( bHighDark )
						sDefdl = "defd";
					CMarker* pmkrDefdl = pshvLex->GetDocument()->pmkrset()->pmkrSearch( sDefdl ); // 1.6.4a Get defl or defd mkr
					CField* pfldNewDefdl = new CField( pmkrDefdl ); // 1.6.4a Make new field for highlighted
					precLex->InsertAfter( pfldDef, pfldNewDefdl ); // 1.6.4a Insert new def field
					int iLexStart = pfldDef->sContents().Find( sStart ); // 1.6.4a Find start
					if ( iLexStart >= 0 ) // 1.6.4a If found (should be there)
						{
						pfldNewDefdl->SetContent( pfldDef->Mid( iLexStart ) ); // 1.6.4a Set new field to tail
						pfldDef->Delete( iLexStart, pfldDef->GetLength() - iLexStart ); // 1.6.4a Remove tail
						}
					}
				else // 1.6.4a If no more found, we are done
					bMore = FALSE; // 1.6.4a Stop loop
				}
			// 1.6.4a Here the main lex display is done, next do the NoNum display in pshvLexNoNum
			if ( pshvLexNoNum ) // 1.6.4a If there is a no numbers lex
				{
				CRecLookEl* prelLexNoNum = pshvLexNoNum->prelCur(); // 1.6.4a Get record in view
				CRecord* precLexNoNum = prelLexNoNum->prec();
				CField* pfldLexNoNum = precLexNoNum->pfldFirst(); // 1.6.4a Get first field of NoNum
				pfldLex = precLex->pfldFirst(); // 1.6.4a Get first field of main lex
				pfldLexNoNum->SetContent( pfldLex->sContents() ); // 1.6.4a Set key field of NoNum to same as main lex
				CField* pfldDelete = precLexNoNum->pfldNext( pfldLexNoNum ); // 1.6.4a Delete all fields after first
				while ( pfldDelete )
					{
					precLexNoNum->Delete( pfldDelete ); // 1.6.4a Delete field
					pfldDelete = precLexNoNum->pfldNext( pfldLexNoNum ); // 1.6.4a Get next field if there is one
					}
				pfldLex = precLex->pfldNext( pfldLex ); // 1.6.4a Get second field of main lex record
				while ( pfldLex ) // 1.6.4a While another field, copy it to NoNum
					{
					CField* pfldNew = new CField( pfldLex->pmkr() ); // 1.6.4a Make new def field to insert
					pfldNew->SetContent( pfldLex->sContents() ); // 1.6.4a Set new field contents
					precLexNoNum->InsertAfter( pfldLexNoNum, pfldNew ); // 1.6.4a Insert new def field
					pfldLexNoNum = precLexNoNum->pfldNext( pfldLexNoNum ); // 1.6.4a Move insertion place to inserted fld
					pfldLex = precLex->pfldNext( pfldLex ); // 1.6.4a Get next field if there is one
					}
				pfldLexNoNum = precLexNoNum->pfldFirst(); // 1.6.4a Get first field of NoNum
				while ( pfldLexNoNum ) // 1.6.4a For each field in NoNum, remove numbers
					{
					Str8 sMkr = pfldLexNoNum->sMarker(); // 1.6.4a Get marker
					if ( sMkr.Find( "def" ) >= 0 ) // 1.6.4a If it is a def field, look for stars
						{
						Str8 sCont = pfldLexNoNum->sContents();
						BOOL bStar = TRUE; // 1.6.4a More to find
						while ( bStar ) // 1.6.4a While more to find, look for star
							{
							int iStar = sCont.Find( '*' ); // 1.6.4a Find star
							if ( iStar >= 0 ) // 1.6.4a If star found, get number and remove it
								{
								int iStarEnd = sCont.Find( '*', iStar + 1 ); // 1.6.4a Find star at end of number
								if ( iStarEnd < 0 ) // 1.6.4a If closing star not found, error
									iStarEnd = iStar; // 1.6.4a If closing star not found, just delete one star
								if ( iStarEnd - iStar > 4 ) // 1.6.4a If distance between stars too long, error
									iStarEnd = iStar; // 1.6.4a If error, just delete one star
								sCont = sCont.Mid( 0, iStar ) + sCont.Mid( iStarEnd + 1 ); // 1.6.4a Remove number
								}
							else
								bStar = FALSE;
							}
						pfldLexNoNum->SetContent( sCont ); // 1.6.4a Put nonumber content into field
						}
					pfldLexNoNum = precLexNoNum->pfldNext( pfldLexNoNum ); // 1.6.4a Move to next field
					}
				pshvLexNoNum->Invalidate(FALSE); // redraw to show result // 1.6.4f Test focus change
				}
			}
		SetFocus(); // 1.6.4a Put focus back on AGNT window // 1.6.4b Fix 1.6.4a problem of focus left on lex // 1.6.4f 
		// Make this the active view, since succesful jump will have left it at another view
		CMDIChildWnd* pwnd = pwndChildFrame(); // 1.6.4f 
		ActivateWindow( pwnd ); // 1.6.4f Fix 1.6.4a  problem of focus left on lex
		}
	}

void CShwView::OnEditInsertField() // 5.96y
	{
	if ( m_bBrowsing ) // 1.1tb Fix possible crash on insert field in browse view
		return;
	EditEnd(); // Move to end of current line
	while ( !m_rpsCur.bEndOfField() ) // Move to end of field
		{
		bEditRight(); // Move to next line
		EditEnd(); // Move to end of line
		}
	if ( m_rpsCur.bInterlinear() ) // 1.4yb If interlinear field, trim off possible nl
		{
        m_rpsCur.pfld->Trim(); // Trim trailing nl's and spaces // 1.4yb Fix bug of insert field sometimes messing up interlinear
		m_rpsCur.iChar = m_rpsCur.pfld->GetLength(); // 1.4yb Fix recpos for possible trim
		}
    CharInsert( '\n' ); // 5.98c Fix bug of falsely inserting following field
    bEditRight(); // 5.98c Move cursor right
	OnChar( '\\', 1, 0 ); // Open new marker box as if user had pressed backslash
	}

void CShwView::OnDatabaseInsertRecord() 
	{ 
	DatabaseInsertRecord(); 
	ParallelJump(); // 6.0ze Do parallel jump on insert record
	}
void CShwView::OnDatabaseDeleteRecord() 
	{ 
	DatabaseDeleteRecord(); 
	if ( m_prelCur ) // 1.5.5c 
		ParallelJump(); // 6.0ze Do parallel jump on delete record
	}
void CShwView::OnDatabaseCopyRecord() { DatabaseCopyRecord(FALSE); }
void CShwView::OnDatabaseMoveRecord() { DatabaseCopyRecord(TRUE); }
void CShwView::OnDatabaseMergeDb() { DatabaseMergeDb(); }

void CShwView::OnDatabaseFiltering()
	{
	ViewDatabaseProperties( 1 ); // 6.0zn Go direct to filter page from Database, Filtering menu
// DatabaseFiltering(); 
	}

void CShwView::OnUpdateDatabaseFiltering(CCmdUI* pCmdUI)
	{
    pCmdUI->Enable( !Shw_papp()->pProject()->bLockProject() );
	}

void CShwView::OnDatabaseSorting() { DatabaseSorting(); }
void CShwView::OnDatabaseMarkers() { DatabaseMarkers(); }

void CShwView::OnUpdateDatabaseProperties(CCmdUI* pCmdUI)
	{
    pCmdUI->Enable( !Shw_papp()->pProject()->bLockProject() );
	}

// Menu update handlers to disable options appropriately
void CShwView::OnUpdateDatabaseNextRecord(CCmdUI* pCmdUI)
{   // Disable Next if no current record
    pCmdUI->Enable((CRecLookEl*)m_prelCur!=NULL);  // 1998-12-02 MRP
}
void CShwView::OnUpdateDatabasePreviousRecord(CCmdUI* pCmdUI)
{   // Disable Prev if no current record
    pCmdUI->Enable((CRecLookEl*)m_prelCur!=NULL);  // 1998-12-02 MRP
}
void CShwView::OnUpdateDatabaseFirstRecord(CCmdUI* pCmdUI)
{   // Disable Top if no current record
    pCmdUI->Enable((CRecLookEl*)m_prelCur!=NULL);  // 1998-12-02 MRP
}
void CShwView::OnUpdateDatabaseLastRecord(CCmdUI* pCmdUI)
{   // Disable Bottom if no current record
    pCmdUI->Enable((CRecLookEl*)m_prelCur!=NULL);  // 1998-12-02 MRP
}
void CShwView::OnUpdateEditInsertField(CCmdUI* pCmdUI)
{   // Disable Insert Field if document is read-only
    pCmdUI->Enable(!pdoc()->bReadOnly());
	if ( Shw_papp()->pProject()->iLockProject() >= 10 ) // 1.2ca At project lock level 10, disable anything that can modify a file
		pCmdUI->Enable( FALSE );
}
void CShwView::OnUpdateEditReturnFromJump(CCmdUI* pCmdUI)
{  // Disable Return from Jump if no valid jump available
	CShwView* pview = Shw_papp()->pviewJumpedFrom(); // Get last view jumped from
    pCmdUI->Enable( pview && Shw_papp()->bViewValid( pview) );
}
void CShwView::OnUpdateDatabaseInsertRecord(CCmdUI* pCmdUI)
{   // Disable Insert Record if document is read-only
    pCmdUI->Enable(!pdoc()->bReadOnly());
	if ( Shw_papp()->pProject()->iLockProject() >= 10 ) // 1.2ca At project lock level 10, disable anything that can modify a file
		pCmdUI->Enable( FALSE );
}
void CShwView::OnUpdateDatabaseDeleteRecord(CCmdUI* pCmdUI)
{  // Disable Delete if no current record or if document is read-only
    // 1998-12-02 MRP: cast
    pCmdUI->Enable((CRecLookEl*)m_prelCur!=NULL && !pdoc()->bReadOnly());
	if ( Shw_papp()->pProject()->iLockProject() >= 10 ) // 1.2ca At project lock level 10, disable anything that can modify a file
		pCmdUI->Enable( FALSE );
}
void CShwView::OnUpdateDatabaseCopyRecord(CCmdUI* pCmdUI)
{   // Disable Copy if no current record
    pCmdUI->Enable((CRecLookEl*)m_prelCur!=NULL);  // 1998-12-02 MRP
	if ( Shw_papp()->pProject()->iLockProject() >= 10 ) // 1.2ca At project lock level 10, disable anything that can modify a file
		pCmdUI->Enable( FALSE );
}
void CShwView::OnUpdateDatabaseMoveRecord(CCmdUI* pCmdUI)
{   // Disable Move if no current record or if no other DB of same type is open or if
    // current DB is read-only
    if ( (CRecLookEl*)m_prelCur != NULL )
        {
        CTypedDocList doclst(GetDocument()->ptyp());
        pCmdUI->Enable(doclst.pdocFirst(GetDocument())!=NULL && !pdoc()->bReadOnly());
        }
    else
        pCmdUI->Enable(FALSE);
	if ( Shw_papp()->pProject()->iLockProject() >= 10 ) // 1.2ca At project lock level 10, disable anything that can modify a file
		pCmdUI->Enable( FALSE );
}
void CShwView::OnUpdateDatabaseMergeDb(CCmdUI* pCmdUI)
{   // Disable Merge Database command if no valid sources or if current DB is read-only
    CTypedDocList doclst(GetDocument()->ptyp());
    pCmdUI->Enable(doclst.pdocFirst(GetDocument())!=NULL && !pdoc()->bReadOnly());
	if ( Shw_papp()->pProject()->iLockProject() >= 10 ) // 1.2ca At project lock level 10, disable anything that can modify a file
		pCmdUI->Enable( FALSE );
}


void CShwView::SetHideFields( BOOL bHide ) // 1.0bg Set hide fields on or off
{
	CDatabaseType* ptyp = this->ptyp(); // 1.0bn Get database type into local for convenience
	ptyp->SetHideFields( bHide ); // 1.0be Set hide fields on or off
	if ( !m_bBrowsing && m_rpsUpperLeft.pfld ) // 1.1se Fix bug of assert and crash on hide fields on or off in browse view // 1.5.0ak Fix bug of possible crash on hide fields in browse view // 1.5.0kg 
		{
		if ( ptyp->bHideFields() ) // 1.4px If changing to hide, check for upper left becoming hidden
			while ( m_rpsUpperLeft.pfld && m_rpsUpperLeft.pfld->pmkr()->bHidden() && m_rpsUpperLeft.pfldPrev() ) // 1.4px Fix bug of cursor wrong if top field in view becomes hidden
				m_rpsUpperLeft.PrevField(0); // 1.4px If upper left is hidden, move to non-hidden by setting to current
		if ( bHide && m_rpsUpperLeft.pfld && m_rpsCur.pfld->pmkr()->bHidden() ) // 1.2kt Don't move to top of record on hide or unhide fields
			bMoveUpField( TRUE ); // 1.2kt If cursor in hidden field, move up to non-hidden
		SetScroll( TRUE ); // 1.2kt Set scroll position after hide or unhide
		ResynchCaretAndCursor(); // 1.1mg Fix bug of cursor not always right after hide fields
		SetCaretPosAdj(); // 1.0bj Set caret pos to avoid wrong pos after show or hide
		}
//    GetDocument()->UpdateAllViews(NULL); // 1.0bj Repaint all windows showing this database // 1.4xg 
    Shw_UpdateAllViews(NULL); // 1.4xg Fix bug of other db's not updating view on hide feilds // make all views redraw 
}

void CShwView::OnWindowDuplicate()
{ // This function is excerpted from CMDIFrameWnd::OnWindowNew
    if ( !bValidateCur() )
        return;
    CDocument* pDocument = GetDocument();
    CDocTemplate* pTemplate = pDocument->GetDocTemplate();
    ASSERT_VALID(pTemplate);
    CFrameWnd* pFrame = pTemplate->CreateNewFrame(pDocument, NULL); // Make a new frame
    if (pFrame == NULL)
        return;     // command failed
    ASSERT( !s_prelJumpTo ); // we're not jumping, this needs to be NULL
    ASSERT( !s_pViewToDuplicate );
    s_pViewToDuplicate = this;
    pTemplate->InitialUpdateFrame(pFrame, pDocument); // causes OnInitialUpdate of new view, based on s_pViewToDuplicate
}

// Routines for setting up windows and focus changes
int CShwView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CShwScrollView::OnCreate(lpCreateStruct) == -1)
        return -1;

    m_pntCaretLoc.x = m_iFldStartCol;
    m_pntCaretLoc.y = 0;
    return 0;
}

// BJY - Calling pntPosToPnt() takes a lot of time on big records. The hint system should be improved
//  to give us information about field deletions, etc. so we know if we have to call pntPosToPnt().
// (It only has to be called if the field containing the caret or upper left visible position are
//  shortened or deleted)
void CShwView::ResynchCaretAndCursor( BOOL bSetKbd ) // 03/01/2000 TLB : broke of most of functionality from OnSetFocus to create this // 1.5.1kh 
                                       //                  helper function to try to fix problem after renumbering.
{
	if ( m_bBrowsing ) // 1.1tb Fix possible crash on insert field in browse view
		return;
    if ( m_prelCur ) // Handle empty view from filter
        { // resync upper left visible position in case stuff was deleted further back record
        m_lVPosUL = -pntPosToPnt( CRecPos( 0, m_prelCur->prec()->pfldFirst(), m_prelCur->prec() ), TRUE ).y;
        if ( bPositionValid( m_rpsCur ) )
            {
            m_pntCaretLoc = pntPosToPnt( m_rpsCur ); // set caret to previous record position
            }
        else
            {
            m_pntCaretLoc = pntPntToPos( m_pntCaretLoc ); // set caret to previous pixel position
            }
        }
    m_iMaxLineWd = 100; // Scroll width, init to minimum
    Invalidate(FALSE);
    SetScroll( TRUE );
    
    // Select the proper keyboard in the new focus.
    if ( bSetKbd && m_rpsCur.pfld ) // 1.5.1kh 
        {
        Shw_pProject()->pkmnManager()->ActivateKeyboard(m_rpsCur.pmkr());
        }
}

void CShwView::OnSetFocus(CWnd* pOldWnd)
{
    CPoint pntScrollPosSaved = GetScrollPosition(); // 1.1na Fix bug of horizontal scroll lost on gain focus
    CShwScrollView::OnSetFocus(pOldWnd);
	MessageStatusLine( "" ); // 1.1ch Restore normal status message on set focus

    m_bHaveFocus = TRUE;

    if ( m_bBrowsing )
        return;

	if ( m_rpsCur.pfld && bHiddenField( m_rpsCur.pfld ) ) // 1.2dp Fix crash (in 1.2) when hide fields is on and filter shows no records  // 1.0bg If cursor in hidden field, move to visible field
		bMoveUpField( TRUE ); // 1.4xf Fix bug of possible crash on hide fields
    ResynchCaretAndCursor(); // 03/01/2000 TLB : broke of most of functionality to create helper
                             //                  function to try to fix problem after renumbering.

    // Note: the remainder of this functionality used to preceed the Keyboard handling in the
    // original OnSetFocus function. The Keyboard stuff seemed useful in the Resynch helper function,
    // and the change of order doesn't seem to break anything. TLB.
	ScrollToPosition( pntScrollPosSaved ); // 1.1na Fix bug of horizontal scroll lost on gain focus
    SetCaretSizeCreate( TRUE );
    SetCaretLoc();
    if ( !bSelecting( eAnyText ) ) 
        ShowCaret();     // Make caret appear in this window
    m_bDontCallSetCaretPosAdj = TRUE; // signal DrawRec() to NOT call SetCaretPosAdj()
    UpdateWindow();
    m_bDontCallSetCaretPosAdj = FALSE;
}

void CShwView::OnKillFocus(CWnd* pNewWnd)
{
    CShwScrollView::OnKillFocus(pNewWnd);
    
    m_bHaveFocus = FALSE;

    // RNE:  Do not deactivate Keyman when the Shoebox app itself is losing focus.
    if ( !CMainFrame::s_bIsAppLosingFocus() )
        Shw_pProject()->pkmnManager()->DeActivateKeyboards();
    
    if ( !m_bBrowsing )
        DestroyCaret();

    if (m_iTracking)
        StopTracking(); // yes, we can lose focus while tracking!
}

void CShwView::UpdateBasedOnChange()
{
    int iRedraw;

    if (m_iWhatChanged == eNothing)
        return; // nothing actually changed
    else if ((m_iWhatChanged & eMultipleLines) || m_rpsCur.bInterlinear())
        iRedraw = CRecordHint::eAll;
    else if (m_iWhatChanged & eSingleLine)
		{
		if ( ptyp()->bHideFields() ) // 1.0bh Temp force full repaint to make insert and delete char show result
	        iRedraw = CRecordHint::eAll;
		else
			iRedraw = CRecordHint::eLine;
		}
    else
        ASSERT(FALSE);

    CRecordHint hint(m_rpsCur, iRedraw);
    GetDocument()->UpdateAllViews(NULL, 0L, &hint);
}

#ifndef _MAC
// Unicode char input routine
LRESULT CShwView::OnUnicodeChar( WPARAM wParam, LPARAM lParam )
{
	if ( wParam == UNICODE_NOCHAR )
		return 1;
	Str8 sUTF8 = sUTF32ToUTF8( wParam );
    // since we've made OnCharEx support a string of chars (rather than one at a time), just
    //  call that directly with the whole thing.
    OnCharEx(sUTF8, 0, 0);
	return 1;
}

// This section of code is using the thread locale version of USES_CONVERSION, so that 
//  _CONVERSION_USES_THREAD_LOCALE must be define at this point. Currently, it is defined 
//  in MyMLang.h (for unicode c/c/p), so you shouldn't remove it from there or this section
//  of code will break.
#ifndef _CONVERSION_USES_THREAD_LOCALE
// #pragma message("This code must use the thread locale version of USES_CONVERSION, but _CONVERSION_USES_THREAD_LOCALE isn't defined!!!")
#endif  
#endif _MAC

// Editing routines
void CShwView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	USES_CONVERSION; // 1.4xe 
	CLangEnc* plng = NULL; // 1.5.0ff 
    if( m_rpsCur.pfld ) // 1.5.0ff 
		plng = m_rpsCur.pfld->plng(); // 1.5.0ff 
	// CString sw = nChar; // 1.4qyu
	CString sw((wchar_t)nChar);
    Str8 strChar; // 1.4ua Fix U bug of legacy keyboard input not working, didn't work because of code page conversion
    if( plng && plng->bUnicodeLang() ) // 1.4ua  // 1.5.0ff 
		strChar =  sUTF8( sw ); // 1.4qyu
	else
	    strChar = W2CA( sw );  // 1.4xe Fix U bug of incorrect chars from legacy Keyman
	if ( plng )
		plng->ApplyKeyDefs( strChar ); // 1.5.0ff Implement key defs
	if ( strChar != "" ) // 1.4qmx
	    OnCharEx(strChar, nRepCnt, nFlags);
}

void CShwView::ReadOnlyMessage() // 1.1ch Give message in status bar when user tries to modify read-only file
	{
	MessageStatusLine( _("") ); // 1.5.0ha Remove unused messages
    MessageBeep(0);
	}

void CShwView::OnCharEx(Str8& strChar, UINT nRepCnt, UINT nFlags)
{
    UINT nChar = strChar[0];    // for the operator==() testing for control chsr below
    CRecPos rpsBefore = m_rpsCur;

    if ( bTracking( eAnything ) )   // don't use keys if mouse button down
        return;

    if ( !m_prelCur ) // Handle empty view from filter
        return;

    if ( m_bBrowsing )
        {
        BrowseOnChar( nChar, nRepCnt, nFlags );
        return;
        }

    if ( pdoc()->bReadOnly() || (Shw_papp()->pProject()->iLockProject() >= 10) ) // 1.2gx Fix bug (1.2ca) of project lock level 10 allowing insert char
        {
		ReadOnlyMessage(); // 1.1ch Give message in status bar when user tries to modify read-only file
        return;
        }

    HideCaret();
    ASSERT( m_rpsCur.pfld );
    m_iWhatChanged = eNothing; // clear flag set by edit routines
    BOOL bSelectionDeleted = bSelecting(eAnyText);
    if ( nChar == VK_ESCAPE )
        {
        ClearSelection();
        m_iWhatChanged = eMultipleLines;
        }
    else
        DeleteSelection(); // trash selected text, if any
    if ( nChar == 8 ) // If backspace, delete char to left
        {
        if (!bSelectionDeleted)
            bEditBackspace();
        }
	else if ( nChar == VK_TAB ) // 5.99za Don't try to insert TAB key
		;
    else // Else, normal character, insert or overlay it
        {
        CRecPos rpsBefore = m_rpsCur;
        EditCharInsert( strChar );
        m_undlst.Add( rpsBefore, m_rpsCur ); // add change to undo list
        }

    if ( nChar != '\r' && nChar != '\n' ) // wrap line(s) if necessary
        LineWrap(); // 2000-09-07 TLB: Do auto-wrap or emergency line breaking to 1000-chars max

    SetCaretPosAdj(); // Move caret
    UpdateBasedOnChange();
    ShowCaret();
    
    if ( (rpsBefore.pfld && m_rpsCur.pfld) 
//            && (rpsBefore.pfld->pmkr() != m_rpsCur.pfld->pmkr()) )
            && (rpsBefore.pmkr() != m_rpsCur.pmkr()) )
        {
        Shw_pProject()->pkmnManager()->ActivateKeyboard(m_rpsCur.pmkr());
        }

}

void CShwView::CheckShiftKey( UINT nChar )
{
    if ( ::GetKeyState( VK_SHIFT ) < 0 )    // shift pressed?
        {
        switch ( nChar )
            {
            case VK_RIGHT:
            case VK_LEFT:
            case VK_HOME:
            case VK_END:
            case VK_UP:
            case VK_DOWN:
            case VK_PRIOR:
            case VK_NEXT:
                ShiftSelectText();
                Invalidate( FALSE );
				TextParallelJump(); // 1.5mh Do parallel jump from text file
            }
        }
    else if ( bSelecting( eAnyText ) )
        {
        switch ( nChar )
            {
            case VK_LEFT:
            case VK_HOME:
            case VK_UP:
            case VK_PRIOR:
                ClearSelection(-1); // put caret at beginning of selection
                Invalidate( FALSE );
				TextParallelJump(); // 1.5mh Do parallel jump from text file
                break;
            case VK_RIGHT:
            case VK_END:
            case VK_DOWN:
            case VK_NEXT:
                ClearSelection(1); // put caret at end of selection
                Invalidate( FALSE );
				TextParallelJump(); // 1.5mh Do parallel jump from text file
            }
        }
}

void CShwView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    CRecPos rpsBefore = m_rpsCur;
	CField* pfldBefore = m_rpsCur.pfld;
    
    if ( !m_prelCur ) // Handle empty view from filter
        return;

    if ( m_bBrowsing ) // handle keys for browse view elsewhere
        {
        BrowseOnKeyDown( nChar, nRepCnt, nFlags );
        return;
        }

    ASSERT( m_rpsCur.pfld );
    ASSERT( m_rpsCur.iChar >= 0 );
    ASSERT( m_rpsCur.iChar <= m_rpsCur.pfld->GetLength() );
    
    if ( bTracking( eAnything ) )   // don't use keys if mouse button down
        return;

    BOOL bSetCaretPos = TRUE; // don't set caret pos if no movement happens

    HideCaret();
    BOOL bSelectionCleared = bSelecting( eAnyText ); // left, right don't move when cancelling selection
    CheckShiftKey( nChar );
    if ( bSelecting( eAnyText ) )
        bSelectionCleared = FALSE;
	RememberInterlinearWidths( TRUE ); // 6.0j Speed up cursor movements in interlinear
    if ( ::GetKeyState( VK_CONTROL ) >= 0 ) // If control key is up, do normal movements
        {
#ifdef _MAC
        // powerbooks don't have Home, End keys.  This is an alternate means
        // of navigating on the Mac.
        if ( ::GetKeyState( VK_OPTION ) < 0 ) // If Command key down
            {
            switch( nChar )
                {
                case VK_RIGHT: EditEnd(); break;
                case VK_LEFT: bEditHome(); break;
                case VK_UP: 
					EditCtrlHome(); 
					TextParallelJump(); // 1.1mf Do parallel jump from text file
					break;
                case VK_DOWN: 
					EditCtrlEnd(); 
					TextParallelJump(); // 1.1mf Do parallel jump from text file
					break;
                default: bSetCaretPos = FALSE;
                }
            }
        else
            {
#endif
        switch ( nChar )
            {                       
            case VK_RIGHT:
                if ( !bSelectionCleared )
                    if ( m_rpsCur.pfld->plng()->bRightToLeft() )
                        bEditLeft();
                    else
                        bEditRight();
				TextParallelJump(); // 1.5mh Do parallel jump from text file
                break;
            case VK_LEFT:
                if ( !bSelectionCleared )
                    if ( m_rpsCur.pfld->plng()->bRightToLeft() )
                        bEditRight();
                    else
                        bEditLeft();
				TextParallelJump(); // 1.5mh Do parallel jump from text file
                break;
            case VK_UP: 
				{
				BOOL bOutOfTextRef = ( ptyp()->bTextFile() && m_rpsCur.pfld->pmkr() == ptyp()->pmkrTextRef() ); // 1.1me Do parallel jump if moved up out of a ref field
				bEditUp(); 
				if ( bOutOfTextRef ) // 1.1mf Do parallel jump if moved up out of a ref field
					TextParallelJump(); // 1.1mf Do parallel jump from text file
				break;
				}
            case VK_DOWN:
				bEditDown(); 
				if ( ptyp()->bTextFile() && m_rpsCur.pfld->pmkr() == ptyp()->pmkrTextRef() ) // 1.1mf Do parallel jump if moved down into a ref field
					TextParallelJump(); // 1.1mf Do parallel jump from text file
				break;
            case VK_HOME: bEditHome(); break;
            case VK_END: EditEnd(); break;
            case VK_PRIOR: 
				bEditPgUp(); 
				TextParallelJump(); // 1.1mf Do parallel jump from text file
				break;
            case VK_NEXT: 
				bEditPgDn(); 
				TextParallelJump(); // 1.1mf Do parallel jump from text file
				break;   
            case VK_INSERT:
                if ( pdoc()->bReadOnly() || (Shw_papp()->pProject()->iLockProject() >= 10) ) // 1.2gx Fix bug (1.2ca) of project lock level 10 allowing insert char
					ReadOnlyMessage(); // 1.1ch Give message in status bar when user tries to modify read-only file
                else
					{
					RememberInterlinearWidths( FALSE ); // 6.0j Turn off remember because record is changing
                    EditInsert();
					}
                break;
            case VK_DELETE:
                if ( pdoc()->bReadOnly() || (Shw_papp()->pProject()->iLockProject() >= 10) ) // 1.2gx Fix bug (1.2ca) of project lock level 10 allowing insert char
					ReadOnlyMessage(); // 1.1ch Give message in status bar when user tries to modify read-only file
                else
                    {
					RememberInterlinearWidths( FALSE ); // 6.0j Turn off remember because record is changing
                    m_iWhatChanged = eNothing; // clear flag set by bEditDel()
                    bEditDel();
                    LineWrap(); // 2000-09-07 TLB: Do auto-wrap or emergency line breaking to 1000-chars max
                    UpdateBasedOnChange();
                    }
                break;
			case VK_TAB: // 5.97u Tab move to next field, shift tab to prev
				bEditHome();
			    if ( ::GetKeyState( VK_SHIFT ) < 0 )    // shift pressed?
					{
					while ( bEditUp() && m_rpsCur.pfld == pfldBefore ) // Move up to end of prev field
						;
					pfldBefore = m_rpsCur.pfld;
					while ( bEditUp() && m_rpsCur.pfld == pfldBefore ) // Move up to field before prev
						;
					if ( m_rpsCur.pfld != pfldBefore ) // If this wasn't the first record, move down again
						bEditDown();
					bEditHome(); // Make sure it ends up at start of field
					}
				else
					{
					while ( bEditDown() && m_rpsCur.pfld == pfldBefore )
						;
					bEditHome(); // Make sure it ends up at start of field
					if ( m_rpsCur.pfld == pfldBefore ) // 1.4qzmc If it didn't move down, it was on last field
						{
						EditEnd(); // 1.4qzmc Move to end of line to show it is done
						OnEditReturnFromJump(); // 1.4qzmc If came from interlinear jump, go back and continue interlinear
						}
					}
				break;
            default: bSetCaretPos = FALSE;
            }
#ifdef _MAC
            }
#endif
        }
    else // Else, control key is down, do control movements
        {
        switch ( nChar )
            {                       
            case VK_RIGHT: 
				bEditCtrlRight(); 
				TextParallelJump(); // 1.5mh Do parallel jump from text file
				break;
            case VK_LEFT: 
				bEditCtrlLeft(); 
				TextParallelJump(); // 1.5mh Do parallel jump from text file
				break;
            case VK_UP: 
				bEditCtrlUp(); 
				TextParallelJump(); // 1.5mh Do parallel jump from text file
				break;
            case VK_DOWN: 
				bEditCtrlDown(); 
				TextParallelJump(); // 1.5mh Do parallel jump from text file
				break;
            case VK_HOME: 
				EditCtrlHome(); 
				TextParallelJump(); // 1.1mf Do parallel jump from text file
				break;
            case VK_END: 
				EditCtrlEnd(); 
				TextParallelJump(); // 1.1mf Do parallel jump from text file
				break;
            case VK_PRIOR: 
				EditCtrlPgUp(); 
				TextParallelJump(); // 1.5mh Do parallel jump from text file
				break;
            case VK_NEXT: 
				EditCtrlPgDn(); 
				TextParallelJump(); // 1.5mh Do parallel jump from text file
				break;
            default: bSetCaretPos = FALSE;
            }
        }   
    if ( bSelecting( eText ) && m_rpsCur == m_rpsSelOrigin ) // back to no selection
        ClearSelection();
    if (bSetCaretPos) // any movement happen?
        SetCaretPosAdj(); // Move caret
	RememberInterlinearWidths( FALSE );
    ShowCaret();
    
    if ( (rpsBefore.pfld && m_rpsCur.pfld) 
//            && (rpsBefore.pfld->pmkr() != m_rpsCur.pfld->pmkr()) )
            && (rpsBefore.pmkr() != m_rpsCur.pmkr()) )
        {
        Shw_pProject()->pkmnManager()->ActivateKeyboard(m_rpsCur.pmkr());
        }
}

void CShwView::StartTracking( int type ) // capture mouse input
{
    m_iTracking = type;
    SetCapture(); // capture mouse input so we get buttonup message for sure

    ASSERT( type == eDivider || type == eText || type == eLines || type == eFields ); 
}

void CShwView::StopTracking() // release mouse input
{
    ASSERT( m_iTracking );
    m_iTracking = 0;
    if ( GetCapture() == this ) // verify we have the mouse captured
        ReleaseCapture();
}

void CShwView::OnLButtonDown(UINT nFlags, CPoint point)
{
#ifdef _MAC
    // make option-leftclick equivalent to right click on Mac
    if ( (::GetKeyState(VK_OPTION) & 0x8000) != 0 ) // is option key pressed?
        {
        OnRButtonDown( nFlags, point ); // turn it into right click
        return;
        }
#endif

    if ( Shw_papp()->pviewActive() == this && GetFocus() != this ) // means find combo has focus
        SetFocus(); // force focus back to current view

    if ( !m_prelCur ) // Handle empty view from filter
        return;
    
    if ( m_bBrowsing ) // Handle button down in browse elsewhere
        {
        BrowseOnLButtonDown( nFlags, point );
        return;
        }

    HideCaret();

    CRecPos rpsBefore = m_rpsCur;

    if ( bPtInDivider( point ) )
        {
        StartTracking( eDivider );
        StartMovingDivider( point );
        }
    else if ( bPtInMarker( point ) ) // user wants to select lines
        {
        StartTracking( eLines );
        ClearSelection();

        CPoint pntOrg = GetScrollPosition(); // Allow for scrolling
        point.x += pntOrg.x;
        point.y += pntOrg.y;
        EditMouseLeftClick( point );
        m_rpsSelOrigin = m_rpsCur; // select whole line
        if ( m_rpsCur.pfld->plng()->bRightToLeft() ) // If right to left, need to move home to get to right end
            m_rpsCur.bMoveHome();
        else
            m_rpsCur.bMoveEnd();
    
        StartSelection( eLines );
        SetCaretPosAdj(); // Move caret
        Invalidate( FALSE );
        }
    else
        {
        StartTracking( eText );
    
        BOOL bHadSelection = bSelecting( eAnyText ); // remember if anything was selected
        BOOL bShift = ::GetKeyState( VK_SHIFT ) < 0; // remember state of shift key
        if ( bShift )
            ShiftSelectText();
        else
            ClearSelection();

        CPoint pntOrg = GetScrollPosition(); // Allow for scrolling
        point.x += pntOrg.x;
        point.y += pntOrg.y;
        EditMouseLeftClick( point );
        SetCaretPosAdj(); // Move caret
        if ( !bShift )
            StartSelection( eText );
        if ( bHadSelection || bShift )    // need to redraw if selection disappeared or changed
            Invalidate( FALSE );
		TextParallelJump(); // 1.1me Do parallel jump from text file
        }
    ShowCaret();

    if ( (rpsBefore.pfld && m_rpsCur.pfld) 
            && (rpsBefore.pmkr() != m_rpsCur.pmkr()) )
        {
        Shw_pProject()->pkmnManager()->ActivateKeyboard(m_rpsCur.pmkr());
        }
}


void CShwView::OnFileExport()
{
	if ( !ptyp()->bCCTable() ) // 1.4qmq Prevent loop in export of CCTable // 1.4qzff
	    if ( !bValidateCur() )
		    return;

    GetDocument()->bExport(m_pind, m_prelCur); // 1.1ra Remove set hide fields to fix bug of going to top of record
}

void CShwView::OnUpdateFileExport(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( TRUE );
}

void CShwView::OnUpdateFileSave(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( !pdoc()->bReadOnly() );
	if ( Shw_papp()->pProject()->iLockProject() >= 10 ) // 1.2ca At project lock level 10, disable anything that can modify a file
		pCmdUI->Enable( FALSE );
	if ( Shw_papp()->pProject()->bExerciseNoSave() ) // 1.3av Disable File Save and Save All in exercise project
		pCmdUI->Enable( FALSE );
}

void CShwView::OnUpdateFileSaveAs(CCmdUI* pCmdUI) // 1.4wb 
{
    pCmdUI->Enable( !pdoc()->bReadOnly() );
	if ( Shw_papp()->pProject()->iLockProject() >= 10 ) // 1.2ca At project lock level 10, disable anything that can modify a file
		pCmdUI->Enable( FALSE );
	if ( Shw_papp()->pProject()->bExerciseNoSave() ) // 1.3av Disable File Save and Save All in exercise project
		pCmdUI->Enable( FALSE );
}

void CShwView::OnUpdateFileSaveAll(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( !( Shw_papp()->pProject()->iLockProject() >= 10 ) );
	if ( Shw_papp()->pProject()->bExerciseNoSave() ) // 1.3av Disable File Save and Save All in exercise project
		pCmdUI->Enable( FALSE );
}

void CShwView::OnUpdateFileClose(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( !Shw_papp()->pProject()->bLockProject() ); // 1.2bv Disable file close if project is locked
}

  //09-12-1997 Did some changes in order to provide the visible
  //markers in the Browseview 
void CShwView::OnViewMarkers()
{
    m_bViewMarkers = TRUE;
    m_bViewFieldNames = FALSE;
    if (!m_bBrowsing)
        {
        AdjustLeftColumn();
#if 0        
           // 1995-12-05 MRP: Should factor out the following paragraph
        m_iFldStartCol = 0; // Set up start column
        if ( m_bViewMarkers )
            {
            m_iFldStartCol += s_iFldStartColForMarker;
            if ( m_bViewMarkerHierarchy )
                m_iFldStartCol += s_iFldStartColForMarkerHierarchy;
            }
        if ( m_bViewFieldNames )
            m_iFldStartCol += s_iFldStartColForFieldName;
        if (m_prelCur)
            {
            bEditHome(); // Do Home to reset caret position
            SetCaretPosAdj(); // Move caret
            }
        ShowCaret();
        Invalidate(FALSE); // Repaint window     
#endif
        }
    else 
        Invalidate(FALSE);
}

void CShwView::OnViewMarkerHierarchy()
{
    if (!m_bBrowsing)
        {
        m_bViewMarkerHierarchy = !m_bViewMarkerHierarchy; // Invert flag

        AdjustLeftColumn();
        
#if 0
        m_iFldStartCol = 0; // Set up start column
        if ( m_bViewMarkers )
            {
            m_iFldStartCol += s_iFldStartColForMarker;
            if ( m_bViewMarkerHierarchy )
                m_iFldStartCol += s_iFldStartColForMarkerHierarchy;
            }
        if ( m_bViewFieldNames )
            m_iFldStartCol += s_iFldStartColForFieldName;
        if (m_prelCur)
            {
            bEditHome(); // Do Home to reset caret position
            SetCaretPosAdj(); // Move caret
            }
        ShowCaret();
        Invalidate(FALSE); // Repaint window     
#endif        
        }
}

  //09-12-1997 Did some changes in order to provide the visible
  //markers in the Browseview 
void CShwView::OnViewFieldNames()
{
    m_bViewFieldNames = TRUE;
    m_bViewMarkers = FALSE;
    if (!m_bBrowsing)
        {
        m_iFldStartCol = 0; // Set up start column
        if ( m_bViewMarkers )
            {
            m_iFldStartCol += s_iFldStartColForMarker;
            if ( m_bViewMarkerHierarchy )
                m_iFldStartCol += s_iFldStartColForMarkerHierarchy;
            }
        if ( m_bViewFieldNames )
            m_iFldStartCol += s_iFldStartColForFieldName;
        if (m_prelCur)
            {
            bEditHome(); // Do Home to reset caret position
            SetCaretPosAdj(); // Move caret
            }
        ShowCaret();
        Invalidate(FALSE); // Repaint window     
        }
    else
        Invalidate(FALSE); // Repaint window 
}

void CShwView::OnUpdateViewFieldNames(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck( m_bViewFieldNames && !m_bViewMarkers );
   
    //09-12-1997 Deleted the following line in order to provide
    //visible markers in the Browseview
    //pCmdUI->Enable(!m_bBrowsing);
    pCmdUI->Enable(TRUE);
}

void CShwView::OnUpdateViewMarkers(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck( m_bViewMarkers && !m_bViewFieldNames);
    //09-12-1997 Replaced the following line in order to provide the visible
    //markers in  the Browseview
    //pCmdUI->Enable(!m_bBrowsing);
    pCmdUI->Enable(TRUE);
}

void CShwView::OnUpdateViewMarkerHierarchy(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck( m_bViewMarkerHierarchy );
    pCmdUI->Enable(!m_bBrowsing);
}

void CShwView::OnEditMarker()
{
//    EditInsertMarker();
//    SetCaretPosAdj(); // Move caret
//    ShowCaret();
}

void CShwView::OnUpdateEditMarker(CCmdUI* pCmdUI)
{   // Disable if no current record or browsing or read-only
    // 1998-12-02 MRP: cast
    pCmdUI->Enable((CRecLookEl*)m_prelCur!=NULL && !m_bBrowsing && !pdoc()->bReadOnly());
}

void CShwView::OnViewJumpTarget()
{
    m_bIsJumpTarget = !m_bIsJumpTarget;
}

void CShwView::OnUpdateViewJumpTarget(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck( m_bIsJumpTarget );
}

void CShwView::OnViewFocusTarget()
{
	m_bFocusTarget = !m_bFocusTarget;
}

void CShwView::OnUpdateViewFocusTarget(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck( m_bFocusTarget );
}

void CShwView::OnEditInsertFromRangeSet()
{
    EditInsertFromRangeSet();   
}

void CShwView::OnMouseMove(UINT nFlags, CPoint point)
{
    if ( !m_prelCur ) // Handle empty view from filter
        {
        ::SetCursor( AfxGetApp()->LoadStandardCursor( IDC_ARROW ) );
        return;
        }
    
    if ( bTracking( eDivider ) ) // moving divider bars
        {
        MoveDivider( point );
        }
    else if ( bTracking( eText ) )
        {
        HideCaret();
        CRecPos rpsOld = m_rpsCur;
        CPoint pntOrg = GetScrollPosition(); // Allow for scrolling
        point.x += pntOrg.x;
        point.y += pntOrg.y;
        EditMouseLeftClick( point );
        if ( rpsOld != m_rpsCur )   // any movement actually happen?
            {
            SetCaretPosAdj(); // Move caret
            if ( bSelecting( eText ) )
                Invalidate( FALSE );
            }
        ShowCaret();
        }
    else if ( bTracking( eLines ) )
        {
        HideCaret();
        CRecPos rpsOld = m_rpsCur;
        CPoint pntOrg = GetScrollPosition(); // Allow for scrolling
        point.x += pntOrg.x;
        point.y += pntOrg.y;
        EditMouseLeftClick( point );
        m_rpsSelOrigin.bMoveHome(); // fancy footwork to make so at least one line is always selected
        BOOL bAfter = m_rpsCur >= m_rpsSelOrigin;
        if ( bAfter )
            m_rpsCur.bMoveEnd(); // at end of line
        else
            {
            m_rpsCur.bMoveHome(); // beginning of line
            m_rpsSelOrigin.bMoveEnd();
            }
        if ( rpsOld != m_rpsCur )   // any movement actually happen?
            {
            SetCaretPosAdj(); // Move caret
            Invalidate( FALSE );
            }
        ShowCaret();
        }
    else if ( bTracking( eFields ) )
        {
        HideCaret();
        CRecPos rpsOld = m_rpsCur;
        CPoint pntOrg = GetScrollPosition(); // Allow for scrolling
        point.x += pntOrg.x;
        point.y += pntOrg.y;
        EditMouseLeftClick( point );
        m_rpsSelOrigin.SetPos(0); // fancy footwork to make so at least one line is always selected
        BOOL bAfter = m_rpsCur >= m_rpsSelOrigin;
        if ( bAfter )
            {
            while ( bMoveDownLine() ) // make sure m_rpsCur is on last line of field
                ;
            m_rpsCur.bMoveEnd(); // put at end of line
            }
        else
            {
            while ( bMoveUpLine() ) // put m_rpsCur at beginning of field
                ;
            m_rpsCur.bMoveHome(); // beginning of line
            m_rpsSelOrigin.SetPos( m_rpsSelOrigin.pfld->GetLength() ); // end of field
            }
        if ( rpsOld != m_rpsCur )   // any movement actually happen?
            {
            SetCaretPosAdj(); // Move caret
            Invalidate( FALSE );
            }
        ShowCaret();
        }
    else // mouse input not captured
        {
        if ( m_bBrowsing )
            {
            if ( bPtInDivider( point ) )
                ::SetCursor( AfxGetApp()->LoadStandardCursor( IDC_SIZEWE ) );
            else
                ::SetCursor( AfxGetApp()->LoadStandardCursor( IDC_ARROW ) );
            }
        else
            {
            if ( bPtInMarker( point ) ) // If in marker area, use right pointer
#ifdef _MAC
                ::SetCursor( AfxGetApp()->LoadCursor( IDC_RIGHTARROW_MAC ) );
#else
                ::SetCursor( AfxGetApp()->LoadCursor( IDC_RIGHTARROW ) );
#endif
            else if ( bPtInDivider( point ) )
                ::SetCursor( AfxGetApp()->LoadStandardCursor( IDC_SIZEWE ) );
            else
                ::SetCursor( AfxGetApp()->LoadStandardCursor( IDC_IBEAM ) );
            }
        }
}

BOOL CShwView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    if ( nHitTest == HTCLIENT ) // we'll set the cursor ourselves in OnMouseMove()
        return TRUE;
    return CShwScrollView::OnSetCursor( pWnd, nHitTest, message );
}

void CShwView::OnEditCopy()
{
#ifdef ToolbarFindCombo // 1.4yq
    if ( Shw_pcboFind()->bHasFocus() ) // find combo edit has focus
        {
        Shw_pcboFind()->Copy();
        return;
        }
#endif // ToolbarFindCombo // 1.4yq
    bEditCopy();
}

void CShwView::OnUpdateEditCopy(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( ( m_prelCur && bSelecting( eAnyText ) && !bTracking( eAnyText ) && !m_bBrowsing )
#ifdef ToolbarFindCombo // 1.4yq
		|| Shw_pcboFind()->bHasFocus() 
#endif // ToolbarFindCombo // 1.4yq
					);
}

void CShwView::OnEditCut()
{
#ifdef ToolbarFindCombo // 1.4yq
    if ( Shw_pcboFind()->bHasFocus() ) // find combo edit has focus
        {
        Shw_pcboFind()->Cut();
        return;
        }
#endif // ToolbarFindCombo // 1.4yq
    if ( bEditCopy() ) // make sure copy succeeds before deleting selection
        {
        HideCaret();
        DeleteSelection();
        SetCaretPosAdj(); // Move caret
        LineWrap(); // 2000-09-07 TLB: Do auto-wrap or emergency line breaking to 1000-chars max
        UpdateBasedOnChange();
        ShowCaret();
        }
}

void CShwView::OnUpdateEditCut(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( ( m_prelCur && !m_bBrowsing && !pdoc()->bReadOnly() &&
                            bSelecting( eAnyText ) && !bTracking( eAnyText ) )
#ifdef ToolbarFindCombo // 1.4yq
							|| Shw_pcboFind()->bHasFocus() 
#endif // ToolbarFindCombo // 1.4yq
						);
	if ( Shw_papp()->pProject()->iLockProject() >= 10 ) // 1.2ca At project lock level 10, disable anything that can modify a file
		pCmdUI->Enable( FALSE );
}

void CShwView::OnEditPaste()
{
#ifdef ToolbarFindCombo // 1.4yq
    if ( Shw_pcboFind()->bHasFocus() ) // find combo edit has focus
        {
        Shw_pcboFind()->Paste();
        return;
        }
#endif // ToolbarFindCombo // 1.4yq
    HideCaret();
    bEditPaste();
    LineWrap(); // 2000-09-07 TLB: Do auto-wrap or emergency line breaking to 1000-chars max
    UpdateBasedOnChange();
    ShowCaret();
}

void CShwView::OnUpdateEditPaste(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( ::IsClipboardFormatAvailable( CF_TEXT ) &&
                    ( ( m_prelCur && !m_bBrowsing && !pdoc()->bReadOnly() )
#ifdef ToolbarFindCombo // 1.4yq
                    || Shw_pcboFind()->bHasFocus() 
#endif // ToolbarFindCombo // 1.4yq
					) );
	if ( Shw_papp()->pProject()->iLockProject() >= 10 ) // 1.2ca At project lock level 10, disable anything that can modify a file
		pCmdUI->Enable( FALSE );
}

void CShwView::OnEditUndoAll()
{
    ASSERT( m_bCanUndoAll && m_pszUndoAllBuffer );
#ifdef ToolbarFindCombo // 1.4yq
    if ( Shw_pcboFind()->bHasFocus() ) // find combo edit has focus
        return;
#endif // ToolbarFindCombo // 1.4yq
    bEditUndoAll();
    GetDocument()->UpdateAllViews(NULL, 0L, 0);  // ??AB Speed this up later, don't paint the whole screen on current view, send hint showing changing record
}

void CShwView::OnUpdateEditUndoAll(CCmdUI* pCmdUI)
{
    ASSERT( !( m_bBrowsing && m_bCanUndoAll ) );
    ASSERT( m_pszUndoAllBuffer || !m_bCanUndoAll );
    pCmdUI->Enable( m_prelCur && m_bCanUndoAll );
}

void CShwView::OnEditUndo()
{
    ASSERT( !m_undlst.bIsEmpty() );
#ifdef ToolbarFindCombo // 1.4yq
    if ( Shw_pcboFind()->bHasFocus() ) // find combo edit has focus
        return;
#endif // ToolbarFindCombo // 1.4yq
    HideCaret();
    bEditUndo();
    GetDocument()->UpdateAllViews(NULL, 0L, 0);  // ??AB Speed this up later, don't paint the whole screen on current view, send hint showing changing record
    ShowCaret();
}

void CShwView::OnUpdateEditUndo(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( m_prelCur && !m_bBrowsing && !m_undlst.bIsEmpty() );
}

void CShwView::OnLButtonUp(UINT nFlags, CPoint point)
{
    if ( !m_prelCur ) // Handle empty view from filter
        return;

    if ( bTracking( eAnything ) )
        StopTracking();

    if ( m_bBrowsing )
        return;

    HideCaret();
    if ( bSelecting( eText ) && m_rpsSelOrigin == m_rpsCur )
        ClearSelection();   // clear selection if back at starting point
    ShowCaret();
}

BOOL CShwView::bMoveDownRecord() // 1.4hn Add new function to move through records
	{
	CNumberedRecElPtr prel = m_prelCur; // 1.2fj Move through all records
	if ( !ptyp()->bTextFile() ) // 1.2hf Make compare files move correctly through text files
		prel = m_pind->pnrlNext( prel );
	else 
		{
		CRecord* precCur = m_rpsCur.prec; 
		do { 
			prel = m_pind->pnrlNext( prel );
			} while ( prel && prel->prec() == precCur );
		}
    if ( prel )
		{
        SetCur(prel);
		return TRUE;
		}
	return FALSE;
	}

void CShwView::OnToolsCheckUnicodeValidity() // 1.4hp Add Unicode validity check
	{
	if ( m_pind->pmkrPriKey() != pdoc()->pmkrRecord() // If not sorted by record marker or text marker, then complain
			&& !( ptyp()->bTextFile() && m_pind->pmkrPriKey() == ptyp()->pmkrTextRef() ) )
		{
		AfxMessageBox(_("View must be sorted by the record marker or text reference marker."));
		return;
		}
	if ( m_bBrowsing )
		SwitchToRecordView();
	SetHideFields( FALSE ); // 1.4hq Clear hide fields before Unicode validity check
	if ( !m_bUnicodeValidityCheckInProcess ) // 1.4hr If not in process, start at top of database
		DatabaseFirstRecord(); // 1.4hr Start Unicode validity check at top of file
	BOOL bValid = TRUE;
	while ( TRUE ) // While more to check
		{
		if ( !m_rpsCur.pfld->plng()->bUnicodeLang() || m_rpsCur.bEndOfField() ) // If not Unicode field, or at end of field, try to move to next field
			{
			BOOL bMore = bMoveDownField( TRUE );
			if ( !bMore) // If no more fields, try to move to next record
				bMore = bMoveDownRecord();
			if ( !bMore ) // If no more fields, we are done
				break;
			}
		else
			{
			int iNumBytes = m_rpsCur.pfld->plng()->iCharNumBytes( m_rpsCur.psz() ); // Move forward one char and see if it was a valid char
			unsigned char c1 = *m_rpsCur.psz();
			int iCorrectLen = 1;
			if ( c1 >= 0xC0 && c1 < 0xE0 )
				iCorrectLen = 2;
			else if ( c1 >= 0xE0 && c1 < 0xF0 )
				iCorrectLen = 3;
			else if ( c1 >= 0xF0 && c1 <= 0xFF )
				iCorrectLen = 4;
			bValid = ( iNumBytes == iCorrectLen ); // If incorrect length, not valid
			if ( iNumBytes == 1 && c1 >= 0x80 ) // If single char over 0x80, not valid
				bValid = FALSE;
			if ( !bValid ) // If char not valid, tell user
				{
				m_bUnicodeValidityCheckInProcess = TRUE; // 1.4hr Remember that validity check was in process
				SetCaretPosAdj(); // Make sure cursor will be in view
				Invalidate();
				char ac[20]; // 1.4pk Show code of invalid Unicode char
				_itoa_s(c1, ac, (int)sizeof(ac), 10);
				Str8 sMessage = _("Invalid Unicode character found:"); // 1.5.0fg 
				sMessage = sMessage + " " + (const char*)ac + " " + _("Remove it?"); // 1.5.0fg 
				int iAns = AfxMessageBox( sMessage, MB_YESNO );
				if ( iAns == IDYES )  // If user says remove char, delete it and search further
					{
					if ( iNumBytes > iCorrectLen ) // If too many trail bytes, eat one of extras
						m_rpsCur.iChar += iCorrectLen; // Move to end of good character before delete
					bEditDel();
					}
				else // If user says don't remove char, then cancel further search
					{
					m_rpsCur.iChar += iNumBytes; // Move forward past the char so further search won't find it again
					break;
					}
				}
			else
				m_rpsCur.iChar += iNumBytes; // If valid, move forward
			}
		}
	SetCaretPosAdj(); // Make sure cursor will be in view
	Invalidate();
	if ( bValid )
		{
		if ( m_bUnicodeValidityCheckInProcess ) // 1.4hr If check was in process, then say no more, otherwise, say none
			{
			AfxMessageBox(_("No more invalid Unicode characters found."), MB_ICONINFORMATION ); // 1.4xa 
			m_bUnicodeValidityCheckInProcess = FALSE; // 1.4hr Check is no longer in process
			}
		else
			AfxMessageBox(_("No invalid Unicode characters found."), MB_ICONINFORMATION ); // 1.4hr If check ran the whole file and found none, report that // 1.4xa Change message icon when Unicode validity check finds no problems
		}
	}

static CShwView* pviewFromCompare = NULL; // 1.4qd Compare files handle continue from wrong window
static CShwView* pviewToCompare = NULL; // 1.2fa Compare files

void CShwView::OnToolsCompareFiles() // 1.2fa Compare files
{
	if ( m_rpsCur.prec == pind()->pnrlFirst()->prec() // 1.2fd If starting at top of file, start new compare
			&& ( m_rpsCur.pfld == m_rpsCur.prec->pfldFirst() 
				|| ( ptyp()->bTextFile() && m_rpsCur.pfld->pmkr() == ptyp()->pmkrTextRef() ) )
			&& m_rpsCur.iChar == 0 )
		{
		pviewFromCompare = this; // 1.4qd Set current view as From view
		pviewToCompare = NULL; // Clear view to compare to trigger search for new one
		}
	if ( pviewToCompare == this ) // 1.4qd Compare files handle continue from wrong window, if this is the To view, swap them
		{
		pviewToCompare = pviewFromCompare; // 1.4qd Set To view to former From view
		pviewFromCompare = this; // 1.4qd Set From view to this
		}
	if ( !pviewFromCompare ) // 1.4qg Fix bug (1.4qe) of false message when continuing compare
		pviewFromCompare = this; // 1.4qg Set current view as From view
	if ( pviewToCompare && pviewFromCompare != this ) // 1.4qe Check for compare trying to continue from unrelated file
		{
		AfxMessageBox( _("No compare in process from this file.") ); // 1.4qe Tell user this view does not have a compare in process // 1.5.0fj 
		return; // 1.4qe
		}
	if ( pviewToCompare ) // Protect against closing the view that was being compared
		{
		BOOL bFoundCompareView = FALSE;
		CDocList doclst; // list of currently open docs
		for ( CShwDoc* pdoc = doclst.pdocFirst(); pdoc; pdoc = doclst.pdocNext() )
			{
			POSITION pos = pdoc->GetFirstViewPosition();
			while ( pos )
				{
				CShwView* pshv = (CShwView*)pdoc->GetNextView(pos); // increments pos
				if ( pshv == pviewToCompare )
					bFoundCompareView = TRUE;
				}
			}
		if ( !bFoundCompareView )
			{ 
			AfxMessageBox( _("View to compare is no longer available.") ); // Tell user the compare view is no longer available
			pviewFromCompare = NULL; // 1.4qd Clear From view to prevent possible confusion
			pviewToCompare = NULL; // Reset so next time it will hunt for a new one
			return;
			}
		}
	if ( !pviewToCompare ) // 1.2fa If no file to compare, find one
		{
		BOOL bSamePrimarySort = FALSE; // 1.4qf Compare give message if files differ only by secondary sort
		BOOL bSameFirstField = FALSE; // 1.4qj Make compare work if first field not the same
		CDocList doclst; // list of currently open docs
		for ( CShwDoc* pdoc = doclst.pdocFirst(); pdoc; pdoc = doclst.pdocNext() )
			{
			POSITION pos = pdoc->GetFirstViewPosition();
			while ( pos )
				{
				CShwView* pshv = (CShwView*)pdoc->GetNextView(pos); // increments pos
				if ( pshv == this ) // Don't compare to self
					continue;
				if ( pdoc->ptyp() != ptyp() ) // Must be same database type
					continue;
				if ( pshv->pind()->pmkrPriKey() != pind()->pmkrPriKey() ) // Must be sorted the same
					continue;
				bSamePrimarySort = TRUE; // 1.4qf Compare give message if files differ only by secondary sort
				CMarkerRef* pmrfComp = pshv->pind()->pmrflstSecKeys()->pmrfFirst();
				CMarkerRef* pmrf = pind()->pmrflstSecKeys()->pmrfFirst();
				if ( pmrfComp && !pmrf )
					continue;
				if ( pmrf && ! pmrfComp )
					continue;
				if ( pmrfComp && pmrf && ( pmrfComp->pmkr() != pmrf->pmkr() ) ) // First secondary key must be the same
					continue;
				if ( !bSameFirstField ) // 1.4qj If we don't already have a better one, remember this as candidate
					pviewToCompare = pshv; // 1.4qj We have a candidate view, use the last one we find
				if ( pshv->pind()->pnrlFirst()->prec()->pfldFirst()->sContents()
						!= pind()->pnrlFirst()->prec()->pfldFirst()->sContents() ) // First field must be the same
					continue;
				bSameFirstField = TRUE; // 1.4qj Remember that we found one with same first field
				pviewToCompare = pshv; // 1.4qj We have a candidate view, use the last one we find
				}
			}
		if ( !pviewToCompare ) // If no candidate file found, tell user the criteria
			{
//			if ( bSamePrimarySort ) // 1.4qf Compare give message if files differ only by secondary sort // 1.5.9rb 
				AfxMessageBox( _("No file with same database type and sorting was found.") ); // 1.5.0fj // 1.5.9rb Include file compare criteria in message
//			else // 1.5.9rb 
//				AfxMessageBox( _("No file with same database type and sorting was found.") ); // 1.5.0fj  // 1.5.9rb 
			pviewFromCompare = NULL; // 1.4qd Clear From view to prevent possible confusion
			return;
			}
		if ( !( pind()->pmkrPriKey() == ptyp()->pmkrRecord() // 1.4qk Warn if compare on files not sorted by record mark
				|| ( ptyp()->bTextFile() && pind()->pmkrPriKey() == ptyp()->pmkrRef() ) ) ) // 1.4qk
			AfxMessageBox( _("View is not sorted by record marker or reference. Compare may not show all differences.") ); // 1.4qk
		if ( ptyp()->bHideFields() ) // 1.4qg Warn that hidden fields won't be compared
			AfxMessageBox( _("Some fields are hidden. Hidden fields will not be compared.") );
		if ( m_pind->bUsingFilter() ) // 1.4qh Warn that filtered records won't be compared
			AfxMessageBox( _("A filter is active. Records excluded by a filter will not be compared.") );
		DatabaseFirstRecord(); // 1.2hk Start current view at top
		pviewToCompare->DatabaseFirstRecord(); // Start new view to compare at top
		if ( ptyp()->bTextFile() ) // 1.2hk If text type, start both views at top of first record
			{
			EditCtrlHome(); // Set main view to top of record
			pviewToCompare->EditCtrlHome(); // Set compare view to top of record
			}
		CMDIChildWnd* pwndCompare = pviewToCompare->pwndChildFrame(); // 1.2fk
		WINDOWPLACEMENT wplstruct;
		wplstruct.length = sizeof(wplstruct);
		pwndCompare->GetWindowPlacement( &wplstruct );
		BOOL bWasMinimized = wplstruct.showCmd == SW_SHOWMINIMIZED;
		if ( bWasMinimized )
			{
			ActivateWindow( pwndCompare ); // 1.2fk If compare window minimized, change to normal view so both windows can be seen
			ActivateWindow( pwndChildFrame() );
			}
		}
	BOOL bDiff = FALSE;
	if ( bBrowsing() ) // Make sure not browsing
		SwitchToRecordView();
	if ( pviewToCompare->bBrowsing() )
		pviewToCompare->SwitchToRecordView();

	BOOL bAllSelected = FALSE; // 1.2sb Note whether whole record is selected
	if ( bSelecting( eText ) ) // 1.2sb See if text selected
		{
		CRecPos rpsBeg = rpsSelBeg(); // 1.2sb See where selection is
		CRecPos rpsEnd = rpsSelEnd(); // 1.2sb See where selection is
		if ( rpsBeg.pfld == rpsBeg.prec->pfldFirst() && rpsBeg.iChar == 0
				&& rpsEnd.pfld == rpsEnd.prec->pfldLast() )
			{
			ClearSelection( 1 ); // Clear any previous selection // 1.5.0km 
			bAllSelected = TRUE;
			CNumberedRecElPtr prel = m_pind->pnrlNext( m_prelCur );	// 1.2sb If all selected it is inserted record, so step to next record	
            if ( prel )
                SetCur(prel);
			}
		}
	ClearSelection( 1 ); // Clear any previous selection // 1.5.0km Start compare at end of selection

	BOOL bCompareAllSelected = FALSE; // 1.2sb Note whether whole record is selected
	if ( pviewToCompare->bSelecting( eText ) ) // 1.2sb See if text selected
		{
		CRecPos rpsBeg = pviewToCompare->rpsSelBeg(); // 1.2sb See where selection is
		CRecPos rpsEnd = pviewToCompare->rpsSelEnd(); // 1.2sb See where selection is
		if ( rpsBeg.pfld == rpsBeg.prec->pfldFirst() && rpsBeg.iChar == 0
				&& rpsEnd.pfld == rpsEnd.prec->pfldLast() )
			{
			pviewToCompare->ClearSelection( 1 ); // Clear any previous selection // 1.5.0km 
			bCompareAllSelected = TRUE;
			CNumberedRecElPtr prelCompare = pviewToCompare->m_pind->pnrlNext( pviewToCompare->m_prelCur );	// 1.2sb If all selected it is inserted record, so step to next record	
            if ( prelCompare )
                pviewToCompare->SetCur(prelCompare);
			}
		}
	pviewToCompare->ClearSelection( 1 ); // 1.5.0km Start compare at end of selection

	BOOL bFirstTime = TRUE; // 1.5.0ce Make compare continue from cursor position, not next field
	if ( m_rpsCur.pfld->pmkr() != pviewToCompare->m_rpsCur.pfld->pmkr() ) // 1.5.0km If previous positions not in same field, move to next field first time
		bFirstTime = FALSE; // 1.5.0km 
	while ( TRUE ) // 1.2fc Compare until difference found, or end
		{
		BOOL bMore = TRUE; // 1.5.0ce 
		BOOL bMoreCompare = TRUE; // 1.5.0ce 
		if ( !bFirstTime ) // 1.5.0ce If first time through, don't move down to next field
			{
			bMore = bMoveDownField( TRUE );
			bMoreCompare = pviewToCompare->iDnField( pviewToCompare->m_rpsCur, TRUE );
			}
		if ( !bMore && !bMoreCompare ) // If both at end, we are done with this record
			{ // 1.4hj Should be able to use bDownRecord here once it is tested
			CNumberedRecElPtr prel = m_prelCur; // 1.2fj Move through all records
			if ( !ptyp()->bTextFile() ) // 1.2hf Make compare files move correctly through text files
				prel = m_pind->pnrlNext( prel );
			else 
				{
				CRecord* precCur = m_rpsCur.prec; 
				do { 
					prel = m_pind->pnrlNext( prel );
					} while ( prel && prel->prec() == precCur );
				}
            if ( prel )
				{
                SetCur(prel);
				bMore = TRUE;
				}
			CNumberedRecElPtr prelCompare = pviewToCompare->m_prelCur; // 1.4hj Should be able to use bDownRecord here once it is tested
			if ( !ptyp()->bTextFile() ) // 1.2hj Make compare files move correctly through text files
				prelCompare = pviewToCompare->m_pind->pnrlNext(prelCompare);
			else 
				{
				CRecord* precCur = prelCompare->prec(); 
				do { 
					prelCompare = pviewToCompare->m_pind->pnrlNext( prelCompare );
					} while ( prelCompare && prelCompare->prec() == precCur );
				}
            if ( prelCompare )
				{
                pviewToCompare->SetCur(prelCompare);
				bMoreCompare = TRUE;
				}
			if ( !bMore && !bMoreCompare ) // 1.2fj If both at end, we are done with the file
				break;
			if ( bMore && bMoreCompare // 1.2sb If record field different, see if inserted or deleted record
					&& m_rpsCur.pfld->sContents() != pviewToCompare->m_rpsCur.pfld->sContents() )
				{
				CNumberedRecElPtr prelNext = m_pind->pnrlNext( prel );
				if ( prelNext ) // 1.2sb Make sure there is another record
					{
					CField* pfldNext = prelNext->prec()->pfldFirst();
					if ( pfldNext->sContents() == pviewToCompare->m_rpsCur.pfld->sContents() ) // 1.2sb If next record field matches current record of compare, this is an inserted record
						{
						bDiff = TRUE;
						EditCtrlEnd();
						ShiftSelectText();
						EditCtrlHome();
						Invalidate();
						break;
						}
					}
				CNumberedRecElPtr prelCompareNext = pviewToCompare->m_pind->pnrlNext(prelCompare);
				if ( prelCompareNext ) // 1.2sb Make sure there is another record
					{
					CField* pfldCompareNext = prelCompareNext->prec()->pfldFirst();
					if ( pfldCompareNext->sContents() == m_rpsCur.pfld->sContents() ) // 1.2sb If next record field matches current record of compare, this is an inserted record
						{
						bDiff = TRUE;
						pviewToCompare->EditCtrlEnd();
						pviewToCompare->ShiftSelectText();
						pviewToCompare->EditCtrlHome();
						pviewToCompare->Invalidate();
						break;
						}
					}
				}
			}
		if ( !bMore ) // If only main at end, select the rest of record in compare file
			{
			bDiff = TRUE;
			ClearSelection(); // Clear any previous selection
			Invalidate();
			EditSetCaret( m_rpsCur.pfld, m_rpsCur.pfld->GetLength() ); // Put cursor of finished file at end of file
			pviewToCompare->ClearSelection();
			pviewToCompare->Invalidate();
			pviewToCompare->EditSetCaret( pviewToCompare->m_rpsCur.pfld, pviewToCompare->m_rpsCur.pfld->GetLength() );
			pviewToCompare->bMoveUpField( TRUE );
			pviewToCompare->EditEnd();
			pviewToCompare->ShiftSelectText();
			while ( pviewToCompare->bMoveDownField( TRUE ) )
				;
			pviewToCompare->EditEnd();
			pviewToCompare->Invalidate();
			break;
			}
		if ( !bMoreCompare ) // If only compare file at end, select the rest of record in main file // 1.2ff
			{
			bDiff = TRUE;
			pviewToCompare->ClearSelection(); // Clear any previous selection
			pviewToCompare->Invalidate();
			pviewToCompare->EditSetCaret( pviewToCompare->m_rpsCur.pfld, pviewToCompare->m_rpsCur.pfld->GetLength() ); // Put cursor if finished file at end of file
			ClearSelection();
			Invalidate();
			EditSetCaret( m_rpsCur.pfld, m_rpsCur.pfld->GetLength() );
			bMoveUpField( TRUE );
			EditEnd();
			ShiftSelectText();
			while ( bMoveDownField( TRUE ) )
				;
			EditEnd();
			Invalidate();
			break;
			}
		if ( m_rpsCur.pfld->pmkr() != pviewToCompare->m_rpsCur.pfld->pmkr() ) // 1.2fm If marker is different, select whole field
			{
			bDiff = TRUE;
			BOOL bAdded = FALSE; 
			if ( bMoveDownField( TRUE ) ) // 1.2gv Fix bug of not highlighting correctly on added field
				{
				if ( m_rpsCur.pfld->pmkr() == pviewToCompare->m_rpsCur.pfld->pmkr() )
					bAdded = TRUE;
				bMoveUpField( TRUE );
				}
			BOOL bCompareAdded = FALSE; 
			if ( pviewToCompare->bMoveDownField( TRUE ) ) // 1.2gv Fix bug of not highlighting correctly on added field
				{
				if ( m_rpsCur.pfld->pmkr() == pviewToCompare->m_rpsCur.pfld->pmkr() )
					bCompareAdded = TRUE;
				pviewToCompare->bMoveUpField( TRUE );
				}
			ClearSelection();
			bMoveUpField( TRUE );
			EditEnd();
			EditSetCaret( m_rpsCur.pfld, m_rpsCur.pfld->GetLength() );
			Invalidate();
			pviewToCompare->ClearSelection();
			pviewToCompare->bMoveUpField( TRUE );
			pviewToCompare->EditEnd();
			pviewToCompare->EditSetCaret( pviewToCompare->m_rpsCur.pfld, pviewToCompare->m_rpsCur.pfld->GetLength() );
			pviewToCompare->Invalidate();
			if ( !bCompareAdded || ( bAdded && bCompareAdded ) ) // 1.2ne Fix bug of compare not displaying correctly if two fields exchanged // 1.2gv Fix bug of not highlighting correctly // 1.2ft In file compare, if extra field, highlight it
				{
				ShiftSelectText();
				bMoveDownField( TRUE );
				EditEnd();
				Invalidate();
				}
			if ( !bAdded || ( bAdded && bCompareAdded ) ) // 1.2ne Fix bug of compare not displaying correctly if two fields exchanged  // 1.2gv Fix bug of not highlighting correctly // 1.2ft In file compare, if extra field, highlight it
				{
				pviewToCompare->ShiftSelectText();
				pviewToCompare->bMoveDownField( TRUE );
				pviewToCompare->EditEnd();
				pviewToCompare->Invalidate();
				}
			break;
			}
		BOOL bDifferenceInField = FALSE; // 1.5.0ec 
		if ( m_rpsCur.iChar > 0 || pviewToCompare->m_rpsCur.iChar > 0 ) // 1.5.0ec If cursors not at beginning of field, compare rest of field
			{
			Str8 sTail = m_rpsCur.pfld->sContents().Mid( m_rpsCur.iChar ); // 1.5.0ec Get tail of each string
			Str8 sCompareTail = pviewToCompare->m_rpsCur.pfld->sContents().Mid( pviewToCompare->m_rpsCur.iChar ); // 1.5.0ec Get tail of each string
			if ( sTail != sCompareTail ) // 1.5.0ec 
				bDifferenceInField = TRUE; // 1.5.0ec 
			}
		else
			{
			if ( m_rpsCur.pfld->sContents() != pviewToCompare->m_rpsCur.pfld->sContents() )
				bDifferenceInField = TRUE; // 1.5.0ec 
			}
		if ( bDifferenceInField ) // 1.5.0ec 
			{
			bDiff = TRUE;
			int iDiffStartMain = m_rpsCur.iChar; // 1.5.0ce Use iChar of each view as compare position
			int iDiffStartCompare = pviewToCompare->m_rpsCur.iChar; // 1.5.0ce Use iChar of each view as compare position
			Str8 sMain = m_rpsCur.pfld->sContents();
			Str8 sToCompare = pviewToCompare->m_rpsCur.pfld->sContents();
//			int iLenShortest = __min( sMain.GetLength(), sToCompare.GetLength() ); // 1.5.0ce 
			while ( iDiffStartMain < sMain.GetLength() 
					&& iDiffStartCompare < sToCompare.GetLength() 
					&& sMain.GetChar( iDiffStartMain ) == sToCompare.GetChar( iDiffStartCompare ) ) // 1.5.0ce 
				{
				iDiffStartMain++; // 1.5.0ce 
				iDiffStartCompare++; // 1.5.0ce 
				}
			int iDiffEndMain = m_rpsCur.pfld->GetLength();
			int iDiffEndCompare = pviewToCompare->m_rpsCur.pfld->GetLength();
			while ( iDiffEndMain > iDiffStartMain
					&& iDiffEndCompare > iDiffStartCompare
					&& sMain.GetChar( iDiffEndMain - 1 ) == sToCompare.GetChar( iDiffEndCompare - 1 ) )
				{
				iDiffEndMain--;
				iDiffEndCompare--;
				}
			if ( m_rpsCur.bInterlinear() ) // 1.2sa If file compare sees space difference in interlinear, don't try to highlight it
				{
				if ( iDiffEndMain == iDiffStartMain && sMain.GetChar( iDiffStartMain ) == ' ' ) // 1.2sa
					continue;
				if ( iDiffEndCompare == iDiffStartCompare && sToCompare.GetChar( iDiffStartCompare ) == ' ' ) // 1.2sa
					continue;
				}
			if ( iDiffEndMain - iDiffStartMain > 15 ) // 1.5ma If long difference, try to find first short difference
				{
				Str8 sDiffMain = m_rpsCur.pfld->Mid( iDiffStartMain, iDiffEndMain - iDiffStartMain ); // 1.5ma Get difference strings
				Str8 sDiffCompare = pviewToCompare->m_rpsCur.pfld->Mid( iDiffStartCompare, iDiffEndCompare - iDiffStartCompare ); // 1.5ma 
				Str8 sMainWord;
				int iMain = 0;
				int iMainWordsCompared = 0;
				sDiffMain.bNextWord( sMainWord, iMain ); // 1.5ma Move past first word
				while ( sDiffMain.bNextWord( sMainWord, iMain ) && iMainWordsCompared < 5 ) // 1.5ma Look for match of second and later words, limit to 5 words
					{
					iMainWordsCompared++; // 1.5ma 
					Str8 sCompareWord;
					int iCompare = 0; // 1.5ma Reset compare 
					int iCompareWordsCompared = 0;
					sDiffCompare.bNextWord( sCompareWord, iCompare ); // 1.5ma 
					while( sDiffCompare.bNextWord( sCompareWord, iCompare ) && iCompareWordsCompared < 5 ) // 1.5ma If word matches, we have found shorter difference
						{
						iCompareWordsCompared++; // 1.5ma 
						if ( sMainWord == sCompareWord ) // 1.5ma If matching word found, back up till difference found
							{
							while ( iMain > 0
									&& iCompare > 0
									&& sDiffMain.GetChar( iMain - 1 ) == sDiffCompare.GetChar( iCompare - 1 ) )
								{
								iMain--;
								iCompare--;
								}
							iDiffEndMain = iDiffStartMain + iMain; // 1.5ma Adjust for working in shortened piece
							iDiffEndCompare = iDiffStartCompare + iCompare; // 1.5ma 
							iMainWordsCompared = 10; // 1.5ma Set high number to break out of loop
							iCompareWordsCompared = 10; // 1.5ma Set high number to break out of loop
							}
						}
					}
				}
			EditSetCaret( m_rpsCur.pfld, iDiffStartMain, iDiffEndMain - iDiffStartMain ); // Select different field contents
			Invalidate();
			pviewToCompare->EditSetCaret( pviewToCompare->m_rpsCur.pfld, iDiffStartCompare, iDiffEndCompare - iDiffStartCompare );
			pviewToCompare->Invalidate();
			break;
			}
		bFirstTime = FALSE; // 1.5.0ce 
		}
	if ( !bDiff )
		{
		ClearSelection(); // Clear any previous selection
		Invalidate();
		pviewToCompare->ClearSelection();
		pviewToCompare->Invalidate();
		AfxMessageBox( _("No more differences found.") );
		pviewFromCompare = NULL; // 1.4qd Clear From view to prevent possible confusion
		pviewToCompare = NULL; // 1.2hm Reset file to compare when no more differences are found
		}
	else
		{
		FixCursorPosForHidden(); // 1.5mp Fix bug of compare highlight sometimes not visible
		pviewToCompare->FixCursorPosForHidden(); // 1.5mp Fix bug of compare highlight not visible
		}
}

extern BOOL bShowAllParses; // Show parses that fail the formulas

void CShwView::UnHideInterlinearFields() // Unhide interlinear fields before interlinearizing
{
	if ( ptyp()->bHideFields() ) // 1.0bw Unhide interlinear fields before interlinearizing
		{
		CMarkerSet* pmkrset = ptyp()->pmkrset();
		CMarker* pmkr = pmkrset->pmkrFirst();
		for ( ; pmkr; pmkr = pmkrset->pmkrNext(pmkr) )
			if ( pmkr->bHidden() && pmkr->bInterlinear() )
				pmkr->SetHidden( FALSE );
		}

}

void CShwView::OnEditInterlinShowFail()
{
	bShowAllParses = TRUE; // Turn on show fails for parse, it is turned off when used
	OnEditInterlinearize(); // Do the parse
	bShowAllParses = FALSE; // Turn off show fails after one time
	FixCursorPosForHidden( FALSE ); // 1.0bv Fix cursor pos if hidden fields
}

void CShwView::OnEditInterlinearize()
{
    ASSERT(m_prelCur);
	CShwView* pviewFrom = Shw_papp()->pviewJumpedFrom(); // Get last view jumped from
	UnHideInterlinearFields(); // 1.0bw Unhide interlinear fields before interlinearizing
	CDocument* pdoc = GetDocument();
//    m_bModifiedSinceValidated = FALSE; // 1.6.4c Fix interlinear cursor jump problem // 1.6.4e Undo 1.6.4c temp fix
    bInterlinearize( FALSE );   
    SetCaretPosAdj(); // Move caret
    ShowCaret();
    pdoc->UpdateAllViews(NULL, 0L, 0);
	FixCursorPosForHidden( FALSE ); // 1.0bv Fix cursor pos if hidden fields
}

void CShwView::OnEditAdapt()
{
    ASSERT(m_prelCur);
	UnHideInterlinearFields(); // 1.0bw Unhide interlinear fields before interlinearizing
	CDocument* pdoc = GetDocument();
    BOOL bAdapt = TRUE;
//    m_bModifiedSinceValidated = FALSE; // 1.6.4d Fix interlinear cursor jump problem // 1.6.4e Undo 1.6.4c temp fix
    bInterlinearize( bAdapt );   
    SetCaretPosAdj(); // Move caret
    ShowCaret();
    pdoc->UpdateAllViews(NULL, 0L, 0);
	FixCursorPosForHidden( FALSE ); // 1.0bv Fix cursor pos if hidden fields
}

void CShwView::OnEditAdaptCont()
{
    ASSERT(m_prelCur);
	UnHideInterlinearFields(); // 1.0bw Unhide interlinear fields before interlinearizing
    BOOL bAdapt = TRUE;
	for ( BOOL bMore = TRUE; ; DatabaseNextRecord() )
		{
		while( bInterlinearize( bAdapt, TRUE ) ) // Do whole record
			;
		CNumberedRecElPtr prel = m_prelCur; // 1.2hf
		if ( !ptyp()->bTextFile() ) // 1.2hf Make adapt all move correctly through text files
			prel = m_pind->pnrlNext( prel );
		else 
			{
			CRecord* precCur = m_rpsCur.prec; 
			do { 
				prel = m_pind->pnrlNext( prel );
				} while ( prel && prel->prec() == precCur );
			}
		if ( !prel ) // If no more records, we are done
			break;
		}
    SetScroll( TRUE ); // 1.2hh Adjust scroll size if necessary after Adapt All
    SetCaretPosAdj(); // Move caret
    ShowCaret();
    GetDocument()->UpdateAllViews(NULL, 0L, 0);  // ??AB Speed this up later, don't paint the whole screen on current view, send hint showing changing record
    ResetUndo(); // clear undo buffer
	FixCursorPosForHidden( FALSE ); // 1.0bv Fix cursor pos if hidden fields
}

// If this field is spell checkable, return interlinear process to be used to check it
CInterlinearProc* pintprcSpellCheckProcForField( CRecPos rpsCur, CShwView* pview ) 
{
    CInterlinearProcList* pintprclst = pview->GetDocument()->pintprclst(); // Get interlinear proc list
	CInterlinearProc* pintprc = pintprclst->pintprcFirst(); // Get first process
	if ( !pintprc )
		return NULL; // If no interlinear processes, spell check is not possible
	if ( !rpsCur.bInterlinear() ) // If not interlinear, spell check if in language of first proc
		{
		CLangEnc* plng = pintprc->pmkrFrom()->plng();
		if ( rpsCur.pfld->plng() == plng ) 
			return pintprc;
		else
			return NULL;
		}
	// If interlinear, if this field came from a lookup or parse process, return the process
	for ( pintprc = pintprclst->pintprcFirst(); pintprc; pintprc = pintprclst->pintprcNext( pintprc ) )
		if ( pintprc->pmkrTo() == rpsCur.pfld->pmkr() 
				&& ( pintprc->bLookupProc() || pintprc->bParseProc() ) )
			return pintprc;
	return NULL;
}

BOOL CShwView::bSpellCheckRecord( CRecPos& rpsCur, CRecPos rpsEnd ) // Spell check current record, starting at rpsCur
	{
	while ( TRUE ) // For each field
		{
		CInterlinearProc* pintprc = pintprcSpellCheckProcForField( rpsCur, this ); // Find out if this is a spell checkable field, and what process should be used for it
		if ( pintprc ) // If spell checkable, check it
			if ( !pintprc->bSpellCheckField( rpsCur, rpsEnd ) ) // If it fails, return fail
				return FALSE;
		if ( !rpsCur.pfldNext() ) // If more fields in record, move to next field
			break; // If no more fields in record, break to return success
		rpsCur.NextField( 0 );
		}
	if ( rpsEnd.pfld ) // If selection, sent cursor to end of selection
		rpsCur = rpsEnd;
	else
		rpsCur.iChar = rpsCur.pfld->GetLength(); // Set cursor to end to indicate completion
	FixCursorPosForHidden( FALSE ); // 1.0bv Fix cursor pos if hidden fields
	return TRUE; // Success if it didn't return a failure from inside the loop
	}

void CShwView::OnEditSpellCheck()
{
    ASSERT(m_prelCur);
	UnHideInterlinearFields(); // 1.0bw Unhide interlinear fields before interlinearizing
    if ( !bValidate() ) // validate current view first
        return;
    if ( !Shw_papp()->bValidateAllViews() ) // then validate everyone
        return;
	if ( m_rpsCur.prec == pind()->pnrlFirst()->prec() // 5.97e If starting at top of file, set to first pass
			&& ( m_rpsCur.pfld == m_rpsCur.prec->pfldFirst() 
				|| ( ptyp()->bTextFile() && m_rpsCur.pfld->pmkr() == ptyp()->pmkrTextRef() ) ) // 1.1pg Start verify pass 1 correctly at first record of text file
			&& m_rpsCur.iChar == 0 )
		{
		m_iVerifyPassNumber = 1;
		m_bVerifySkippedFail = FALSE;
		}
	BOOL bAutoSave = Shw_pProject()->bAutoSave(); // 1.1ae Disable autosave during automatic interlinear update of pass 2 failures
	if ( m_iVerifyPassNumber <= 2 && Shw_pProject()->bAutomaticInterlinearUpdate() ) // 1.1af Disable autosave during automatic interlinear update of lookups in pass 1
		Shw_pProject()->SetAutoSave( FALSE ); // 1.1ae Disable autosave during automatic interlinear update of pass 2 failures
	BOOL bRepeatPass2 = TRUE; // 1.1ac If automatic interlinear update, do all pass 2 failures automatically
	while ( bRepeatPass2 ) // 1.1ac If automatic interlinear update, do all pass 2 failures automatically
		{
		bRepeatPass2 = FALSE; // 1.1ac If automatic interlinear update, do all pass 2 failures automatically
		if ( bSelecting( eAnyText ) ) // If selection is a single word, assume it is leftover from spell check and go on
			{
			if ( rpsSelEnd().pfld == rpsSelBeg().pfld ) // If in same field
				{
				CRecPos rpsTemp = rpsSelBeg();
				rpsTemp.bSkipNonWhite(); // See if white space in selection
				if ( rpsTemp.iChar >= rpsSelEnd().iChar ) // If no white space in selection, clear it
					ClearSelection();
				if ( m_iVerifyPassNumber == 2 ) // 1.0dc If failure from pass 2, interlinearize to fix it before continuing spell check
					{
					OnEditInterlinearize();
					if ( Shw_pProject()->bAutomaticInterlinearUpdate() ) // 1.1ac If automatic interlinear update, do all pass 2 failures automatically
						bRepeatPass2 = TRUE;
					}
				}
			}
		CRecPos rpsEnd; 
		if ( bSelecting( eAnyText ) )
			{
			m_iVerifyPassNumber = 3; // If doing a selection, don't do multiple passes, just check it
			rpsEnd = rpsSelEnd();
			m_rpsCur = rpsSelBeg();
			ClearSelection(); // Prevent odd selection results
			if ( m_rpsCur.bIsWdChar() ) // If in or touching a word
				m_rpsCur.bSkipNonWhiteLeft(); // Move back to beginning of word
			if ( !bSpellCheckRecord( m_rpsCur, rpsEnd ) ) // Run spell check processes on selection
				{
				Shw_pProject()->SetAutoSave( bAutoSave ); // 1.1ae Disable autosave during automatic interlinear update of pass 2 failures
				return;
				}
			}
		else
			{
			if ( m_rpsCur.bInterlinear() ) // If in interlinear, start at beginning of current bundle, this makes sure nothing is skipped over if the user has reinterlinearized a word to fix it
				{
				m_rpsCur = m_rpsCur.rpsFirstInBundle( m_rpsCur.iChar );
				m_rpsCur.iChar = 0; // Start at front of line
				}
			else if ( m_rpsCur.bIsWdChar() ) // If in or touching a word
				m_rpsCur.bSkipNonWhiteLeft(); // Move back to beginning of word
			m_rpsCur.bSkipWhite(); // If before white space, move to beginning of word

			CShwStatusBar* pStatus = (CShwStatusBar*)
			AfxGetApp()->m_pMainWnd->GetDescendantWindow(IDW_SHW_STATUS_BAR); // 1.0dab Show record number in status bar during spell check
			CNumberedRecElPtr prel = m_prelCur; // 1.1pf Fix bug of hang on interlinear verify of text file
			while ( TRUE )
				{
				if ( !bSpellCheckRecord( m_rpsCur, rpsEnd ) ) // Run spell check processes!
					{
					if ( bRepeatPass2 )  // 1.1ac If automatic interlinear update, do all pass 2 failures automatically
						break;
					else
						{
						Shw_pProject()->SetAutoSave( bAutoSave ); // 1.1ae Disable autosave during automatic interlinear update of pass 2 failures
						return;
						}
					}
				if ( !ptyp()->bTextFile() ) // If not text file, move to next index
					prel = m_pind->pnrlNext( prel );
				else // If text file, move forward until start of next record is found
					{
					CRecord* precCur = m_rpsCur.prec; // 1.1 pf Fix bug of hang on interlinear verify of text file
					do { // 1.1 pf Fix bug of hang on interlinear verify of text file
						prel = m_pind->pnrlNext( prel );
						} while ( prel && prel->prec() == precCur );
					}
				if ( !prel ) // If no more records, we are done
					{
					if ( pdoc()->pintprclst()->bWordParse() ) // 1.6.4zb If word parse, stop interlinear check after first pass
						break; // 1.6.4zb 
					if ( m_iVerifyPassNumber < 3 && m_bVerifySkippedFail ) // If not checking failures, and at least one was skipped, check them now
						{
						m_iVerifyPassNumber++; // Move to next pass
						DatabaseFirstRecord(); // Start again from top of file
						prel = m_prelCur; // 1.1pf Fix bug of crash on momve to second pass of interlinear verify of text file
						}
					else
						break;
					}
				else
					{
					SetCur(prel); // 1.1pf Move current record
//					SetCaretPosAdj(); // 1.1ph Fix bug of messed up display during pass 2 of verify (try after test version)
					}
				long lCurrentRec = m_prelCur ? lGetCurrentRecNum() : 0L;
				if ( lCurrentRec % 10 == 0 || ptyp()->bTextFile() ) // 1.1pe Fix bug of interlinear check not showing progress on text file
					pStatus->SetRecordPosInfo( lCurrentRec, lGetRecordCount() ); // 1.0db Show record number in status bar during spell check
				}
			}
		}
	Shw_pProject()->SetAutoSave( bAutoSave ); // 1.1ae Disable autosave during automatic interlinear update of pass 2 failures
	// End of file or selection reached, give completion message
    AfxMessageBox( _("Spell Check is complete") );   
    SetCaretPosAdj(); // Move caret
    ShowCaret();
    GetDocument()->UpdateAllViews(NULL, 0L, 0);
}
#ifdef JUNK // 1.5.1fb 
void CShwView::OnMove( int x, int y ) // 1.5.1ea 
{
	CView::OnMove( x, y ); // 1.5.1ea 
	CShwView* pviewSide = pviewSideBrowse(); // 1.5.1ea See if there is a side view
	if ( pviewSide ) // 1.5.1ea 
		{
	    WINDOWPLACEMENT wp; // 1.5.1ea 
	    this->GetParent()->GetWindowPlacement(&wp); // 1.5.1ea 
		int iTop = wp.rcNormalPosition.top; // 1.5.1ea 
		int iBottom = wp.rcNormalPosition.bottom; // 1.5.1ea 
		int iLeft = wp.rcNormalPosition.left; // 1.5.1ea 
		int iRight = wp.rcNormalPosition.right; // 1.5.1ea 
	    WINDOWPLACEMENT wpSide; // 1.5.1ea 
		pviewSide->GetParent()->GetWindowPlacement(&wpSide); // 1.5.1ea Get position of side browse view
		int iLeftSide = wpSide.rcNormalPosition.left; // 1.5.1ea 
		int iRightSide = wpSide.rcNormalPosition.right; // 1.5.1ea 
		int iWidthSide = iRightSide - iLeftSide; // 1.5.1ea Get current width
		CWnd* pWnd = pviewSide->GetParent(); // 1.5.1ea 
		pWnd->SetWindowPos(&wndTop, iLeft - iWidthSide + 5, iTop, iWidthSide, iBottom - iTop, 0 ); // 1.5.1ea 
		}
}
#endif // 1.5.1fb 

void CShwView::OnSize(UINT nType, int cx, int cy)
{
    CShwScrollView::OnSize(nType, cx, cy);
    
    if ( !m_prelCur ) // Handle empty view from filter
        return;

    if ( m_bBrowsing )
        {
        if ( m_pind->lNumRecEls() ) // BJY 10-18-95 I Don't think this makes sense anymore
            BrowseSetScroll( TRUE ); // recalc for scroll bars, etc.
        }
    else
        {
        if ( m_lTotalHt )
            SetScroll( TRUE );
        }
#ifdef JUNK // 1.5.1fb 
	WINDOWPLACEMENT wp; // 1.5.1ea 
	this->GetWindowPlacement(&wp); // 1.5.1ea 
	int iTop = wp.rcNormalPosition.top; // 1.5.1ea 
	int iBottom = wp.rcNormalPosition.bottom; // 1.5.1ea 
	int iLeft = wp.rcNormalPosition.left; // 1.5.1ea 
	int iRight = wp.rcNormalPosition.right; // 1.5.1ea 
	CShwView* pviewSide = pviewSideBrowse(); // 1.5.1ea See if there is a side view
	if ( pviewSide ) // 1.5.1ea 
		{
	    WINDOWPLACEMENT wp; // 1.5.1ea 
	    this->GetParent()->GetWindowPlacement(&wp); // 1.5.1ea 
		int iTop = wp.rcNormalPosition.top; // 1.5.1ea 
		int iBottom = wp.rcNormalPosition.bottom; // 1.5.1ea 
		int iLeft = wp.rcNormalPosition.left; // 1.5.1ea 
		int iRight = wp.rcNormalPosition.right; // 1.5.1ea 
	    WINDOWPLACEMENT wpSide; // 1.5.1ea 
		pviewSide->GetParent()->GetWindowPlacement(&wpSide); // 1.5.1ea Get position of side browse view
		int iLeftSide = wpSide.rcNormalPosition.left; // 1.5.1ea 
		int iRightSide = wpSide.rcNormalPosition.right; // 1.5.1ea 
		int iWidthSide = iRightSide - iLeftSide; // 1.5.1ea Get current width
		CWnd* pWnd = pviewSide->GetParent(); // 1.5.1ea 
		pWnd->SetWindowPos(&wndTop, iLeft - iWidthSide + 5, iTop, iWidthSide, iBottom - iTop, 0 ); // 1.5.1ea 
		}
#endif // 1.5.1fb 
}

void CShwView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    CRecPos rpsBefore = m_rpsCur;

    if ( !m_prelCur ) // Handle empty view from filter
        return;

    if ( bPtInDivider( point ) )
        return;

    if ( m_bBrowsing )
        {
        BrowseOnLButtonDblClk( nFlags, point );
        return;
        }

    if ( bPtInMarker( point ) ) // user wants to select fields
        {
        ASSERT( !bTracking( eAnything ) );
        if ( !bSelecting( eLines ) || m_rpsSelOrigin.pfld != m_rpsCur.pfld )
            return;  // should have a line selected at this point - if not, something's fishy
        StartTracking( eFields );
        m_rpsSelOrigin.SetPos(0); // select whole field
        while ( bMoveDownLine() ) // put m_rpsCur at end field
            ;
        m_rpsCur.bMoveEnd();
        StartSelection( eFields );
        SetCaretPosAdj(); // Move caret
        Invalidate( FALSE );
        return;
        }

    HideCaret();
    ClearSelection();
        
    EditMouseDblClk( point );
    Invalidate( FALSE );
    ShowCaret();
    
    if ( (rpsBefore.pfld && m_rpsCur.pfld) 
//            && (rpsBefore.pfld->pmkr() != m_rpsCur.pfld->pmkr()) )
            && (rpsBefore.pmkr() != m_rpsCur.pmkr()) )
        {
        Shw_pProject()->pkmnManager()->ActivateKeyboard(m_rpsCur.pmkr());
        }

}

void CShwView::OnViewBrowse() // toggle between browse and normal edit view
{
    if ( !m_prelCur ) // Handle empty view from filter
        return;

    if ( !m_bBrowsing )
        SwitchToBrowseView();
    else 
        SwitchToRecordView();
}

CShwView* CShwView::pviewSideBrowse() // 1.5.1ea Find side browse of this window, if it has one
{
    WINDOWPLACEMENT wp; // 1.5.1ea 
    this->GetParent()->GetWindowPlacement(&wp); // 1.5.1ea 
    if ( wp.showCmd == SW_SHOWMAXIMIZED ) // 1.5.1ea Don't do if maximized
		return NULL; // 1.5.1ea 
	int iTop = wp.rcNormalPosition.top; // 1.5.1ea 
	int iBottom = wp.rcNormalPosition.bottom; // 1.5.1ea 
	int iLeft = wp.rcNormalPosition.left; // 1.5.1ea 
	int iRight = wp.rcNormalPosition.right; // 1.5.1ea 
	POSITION pos = pdoc()->GetFirstViewPosition();
	while ( pos )
		{
		CShwView* pviewSide = (CShwView*)pdoc()->GetNextView(pos); // increments pos
		if ( pviewSide == this ) // 1.5.1ea Skip if this is self
			continue; // 1.5.1ea 
	    WINDOWPLACEMENT wpSide; // 1.5.1ea 
		pviewSide->GetParent()->GetWindowPlacement(&wpSide); // 1.5.1ea Get position of possible side browse view
		if ( wpSide.showCmd == SW_SHOWMAXIMIZED ) // 1.5.1ea Don't do if maximized
			continue; // 1.5.1ea 
		int iTopSide = wpSide.rcNormalPosition.top; // 1.5.1ea 
		int iBottomSide = wpSide.rcNormalPosition.bottom; // 1.5.1ea 
		int iLeftSide = wpSide.rcNormalPosition.left; // 1.5.1ea 
		int iRightSide = wpSide.rcNormalPosition.right; // 1.5.1ea 
		int iTopDiff = abs( iTop - iTopSide ); // 1.5.1ea 
		int iBottomDiff = abs( iBottom - iBottomSide ); // 1.5.1ea 
		int iEdgeDiff = abs( iLeft - iRightSide ); // 1.5.1ea 
		int iDiff = iTopDiff + iBottomDiff + iEdgeDiff; // 1.5.1ea See how close it is to correct pos
		if ( pviewSide->bBrowsing() && iDiff < 30 ) // 1.5.1ea If near to side browse pos, window might be moving
			return pviewSide; // 1.5.1ea If browse in right position, return as side browse
		}
	return NULL; // 1.5.1ea 
}

void CShwView::OnViewSideBrowse() // toggle between browse and normal edit view // 1.5.1ea 
{ // Some of this code is copied from OnWindowDuplicate
	if ( bBrowsing() ) // 1.5.1ea Don't do if already browsing 1.5.1fc Make side browse even if browsing
		SwitchToRecordView(); // 1.5.1fc Switch to non-browse view if browsing
    WINDOWPLACEMENT wp; // 1.5.1ea 
    this->GetParent()->GetWindowPlacement(&wp); // 1.5.1ea 
    if ( wp.showCmd == SW_SHOWMAXIMIZED ) // 1.5.1ea Don't do if maximized
		return; // 1.5.1ea 
	CShwView* pviewSide = pviewSideBrowse(); // 1.5.1fb Check for existing side view
	if ( pviewSide ) // 1.5.1fb If existing side view, close it
		{
	    CWnd* pwndFrame = pviewSide->GetParent();
		pwndFrame->DestroyWindow(); // 1.5.1fb 
		pviewSide = NULL; // 1.5.1fb 
		}
	int iTop = wp.rcNormalPosition.top; // 1.5.1ea 
	int iBottom = wp.rcNormalPosition.bottom; // 1.5.1ea 
	int iLeft = wp.rcNormalPosition.left; // 1.5.1ea 
	int iRight = wp.rcNormalPosition.right; // 1.5.1ea 
	int iLeftSide = wp.rcNormalPosition.left - 150; // 1.5.1ea 
	int iRightSide = wp.rcNormalPosition.left; // 1.5.1ea 
	CDocument* pDocument = GetDocument();
	CDocTemplate* pTemplate = pDocument->GetDocTemplate();
	CFrameWnd* pFrame = pTemplate->CreateNewFrame(pDocument, NULL); // Make a new frame
	if (pFrame == NULL)
		return;     // command failed
	if ( iLeftSide < 0 ) // 1.5.1ea If side browse won't fit, see if main window can move right
		{
		int iLeftOver = -iLeftSide; // 1.5.1ea Get amount iLeft is over
		WINDOWPLACEMENT wpMain; // 1.5.1ea 
		Shw_papp()->m_pMainWnd->GetWindowPlacement( &wpMain ); // 1.5.1ea 
		int iRightAvailable = wpMain.rcNormalPosition.right - iRight; // 1.5.1ea See how much main window could move right
		if ( iRightAvailable < iLeftOver ) // 1.5.1ea If not enough room, make main window narrower
			iRight = wpMain.rcNormalPosition.right; // 1.5.1ea Set right to right edge of main window
		else
			iRight += iLeftOver; // 1.5.1ea Else (enough room) move right side over
		iLeft += iLeftOver; // 1.5.1ea Move left enough to make room
		this->GetParent()->SetWindowPos(&wndTop, iLeft, iTop, iRight - iLeft - 10, iBottom - iTop, 0 ); // 1.5.1ea 
		iLeftSide += iLeftOver; // 1.5.1ea 
		iRightSide += iLeftOver; // 1.5.1ea 
		}
	pFrame->SetWindowPos(&wndTop, iLeftSide, iTop, iRightSide - iLeftSide + 5, iBottom - iTop, 0 ); // 1.5.1ea 
	s_pViewToDuplicate = this;
	pTemplate->InitialUpdateFrame(pFrame, pDocument); // causes OnInitialUpdate of new view, based on s_pViewToDuplicate
	pviewSide = (CShwView*)pFrame->GetActiveView(); // 1.5.1ea 
	pviewSide->pbrflst()->DeleteAll(); // 1.5.1ea Clear browse to start with only key
	CBrowseField* pbrfSrt = new CBrowseField( m_pind->pmkrPriKey() ); // 1.5.1ea Browse key mkr
	pviewSide->pbrflst()->Add( pbrfSrt ); // 1.5.1fb Sort field in left column
    pviewSide->ResetBrowseFieldWidths(); // 1.5.1fd Fix odd appearance on side browse from side browse
	pviewSide->SwitchToBrowseView(); // 1.5.1ea
	m_bFocusTarget = TRUE; // 1.5.1gb Side browse turn on parallel move target
	}

void CShwView::OnUpdateViewBrowse(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( (CRecLookEl*)m_prelCur != NULL );  // 1998-12-02 MRP
    pCmdUI->SetCheck( m_bBrowsing );
}

void CShwView::OnViewBrowsefields() // allow user to modify the fields displayed in a browse view
{
    if ( m_brflst.bModalView_Elements( GetDocument()->pmkrset() ) )
        {
        if (m_brflst.bIsEmpty())
            ResetBrowseFields();
        else
            { 
            BrowseJustificationDefault();   //09-05-1997
            m_iXWidth = 0;  //09-09-1997 After leaving the dialogbox, the Window should be recalculated
            ResetBrowseFieldWidths();   
            }
        if ( m_prelCur && m_bBrowsing )
            InitBrowse(); // recalculate scroll bars, etc.

        if ( m_prelCur && !m_bBrowsing )
            SwitchToBrowseView();
        else
            Invalidate(FALSE);
        }
}

void CShwView::OnViewHideFields()
{
	SetHideFields( !ptyp()->bHideFields() ); // 1.0bp Toggle hide fields
	if ( !m_bBrowsing ) // 1.1se Fix bug of assert and crash on set hide fields if browsing
		SetScroll( TRUE ); // 1.1kf Fix bug of scroll bar too short after hide is turned off
}

void CShwView::OnUpdateViewHideFields(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck( ptyp()->bHideFields() ); // 1.0bp Toggle hide fields
}

void CShwView::OnViewFieldsToHide()
{
	CMarkerSet* pmkrset = GetDocument()->pmkrset();
    CHideFieldsDlg dlg( pmkrset );
    if ( dlg.DoModal() == IDOK )
		{
		BOOL bHiddenFields = FALSE; // 1.1rc Set hide fields off if none are selected in fields to hide
		for ( CMarker* pmkr = pmkrset->pmkrFirst(); pmkr; pmkr = pmkrset->pmkrNext( pmkr ) )
			{
			if ( pmkr->bHidden() )
				{
				bHiddenFields = TRUE; // If a hidden field found, note that there are hidden fields
				break;
				}
			}
		if ( bHiddenFields ) // 1.1rc If there are hidden fields, then set hide fields on
			SetHideFields( TRUE );
		else if ( ptyp()->bHideFields() ) // 1.1rc If hide fields was on, but there are none to hide now, set it off
			SetHideFields( FALSE );
		}
}

// 2000/04/27 TLB & MRP - Added new function to support lazy application of some filters.
void CShwView::OnViewReapplyFilter()
{
    // 2000/05/02 TLB 7 MRP - bValidateCur now takes only one argument, which defaults to dirSameRecord
    if ( !bValidateCur(/*eSameRecord, &m_prelCur*/) )
        return;

    ASSERT(m_pind);
    CFilterUpdate fup(m_pind);
    Shw_Update(fup);
}

// 2000/04/27 TLB & MRP - Added new function to support lazy application of some filters.
void CShwView::OnUpdateViewReapplyFilter(CCmdUI* pCmdUI)
{
      pCmdUI->Enable(pind()->bModifiedSinceFiltered());
}

void CShwView::OnUpdateEditInsertFromRangeSet(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!m_bBrowsing && m_prelCur && !pdoc()->bReadOnly() &&
        m_rpsCur.pfld->pmkr()->bRangeSetEnabled() );  // 1999-06-07 MRP
    
	if ( Shw_papp()->pProject()->iLockProject() >= 10 ) // 1.2ca At project lock level 10, disable anything that can modify a file
		pCmdUI->Enable( FALSE );
}

void CShwView::OnUpdateEditInterlinearize(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( !m_bBrowsing && m_prelCur && !pdoc()->bReadOnly() &&
        !pdoc()->pintprclst()->bIsEmpty() );
	if ( Shw_papp()->pProject()->iLockProject() >= 10 ) // 1.2ca At project lock level 10, disable anything that can modify a file
		pCmdUI->Enable( FALSE );
}

void CShwView::OnUpdateEditInterlinShowFail(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( !m_bBrowsing && m_prelCur && !pdoc()->bReadOnly() &&
        !pdoc()->pintprclst()->bIsEmpty() );
	if ( Shw_papp()->pProject()->iLockProject() >= 10 ) // 1.2ca At project lock level 10, disable anything that can modify a file
		pCmdUI->Enable( FALSE );
}

void CShwView::OnUpdateEditInterlinearizeWord(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( !m_bBrowsing && m_prelCur && !pdoc()->bReadOnly() &&
        !pdoc()->pintprclst()->bIsEmpty() );
	if ( Shw_papp()->pProject()->iLockProject() >= 10 ) // 1.2ca At project lock level 10, disable anything that can modify a file
		pCmdUI->Enable( FALSE );
}

void CShwView::OnUpdateEditAdapt(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( !m_bBrowsing && m_prelCur && !pdoc()->bReadOnly() &&
        !pdoc()->pintprclst()->bIsEmpty() ); // 1.3df Reenable adapt prevent greying of interlinearize button // && pdoc()->pintprclst()->bAdapt() ); // 1.3au Disable Adapt if no adapt processes
	if ( Shw_papp()->pProject()->iLockProject() >= 10 ) // 1.2ca At project lock level 10, disable anything that can modify a file
		pCmdUI->Enable( FALSE );
}

void CShwView::OnUpdateEditAdaptCont(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( !m_bBrowsing && m_prelCur && !pdoc()->bReadOnly() &&
        !pdoc()->pintprclst()->bIsEmpty() && pdoc()->pintprclst()->bAdapt() ); // 1.3au Disable Adapt All if no adapt processes
	if ( Shw_papp()->pProject()->iLockProject() >= 10 ) // 1.2ca At project lock level 10, disable anything that can modify a file
		pCmdUI->Enable( FALSE );
}

void CShwView::OnUpdateEditSpellCheck(CCmdUI* pCmdUI)
{
    // 1996-10-11 MRP: Could enable for read-only documents
    // AS LONG AS spelling corrections are disabled
    pCmdUI->Enable( !m_bBrowsing && m_prelCur && !pdoc()->bReadOnly() &&
        !pdoc()->pintprclst()->bIsEmpty() );
}

void CShwView::OnEditUseAsTemplate()
{
    ASSERT( m_prelCur );
    CopyTemplate();
}

void CShwView::OnUpdateEditUseAsTemplate(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( !m_bBrowsing && m_prelCur && !Shw_papp()->pProject()->bLockProject() );
}

void CShwView::OnDatabaseProperties()
{
    ViewDatabaseProperties();
}

void CShwView::ViewDatabaseProperties(int iPage)
{
    // 2000/05/02 TLB 7 MRP - bValidateCur now takes only one argument, which defaults to dirSameRecord
    if ( !bValidateCur(/*eSameRecord, &m_prelCur*/) )
        return;
                     
    CMarker* pmkr=NULL;
    if (m_prelCur && m_rpsCur.pfld)
        pmkr = m_rpsCur.pfld->pmkr();
    CFilter* pfil = m_pind->pfil();
    GetDocument()->ptyp()->bViewProperties(iPage, pmkr, pfil);
}

void CShwView::OnEditJumpto()
{
    EditJumpTo(FALSE);
}

void CShwView::OnUpdateEditJumpto(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( !m_bBrowsing ); // 1.4vxd Disable jump and jump insert on menu if browsing
}

void CShwView::OnEditJumpinsert()
{
    EditJumpTo(TRUE);
}

void CShwView::OnUpdateEditJumpinsert(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( !m_bBrowsing && !( Shw_papp()->pProject()->iLockProject() >= 10 ) ); // 1.4vxd Disable jump and jump insert on menu if browsing
}

void CShwView::OnFileLanguageEncodings()
{
    // 2000/05/02 TLB 7 MRP - bValidateCur now takes only one argument, which defaults to dirSameRecord
    if ( !bValidateCur(/*eSameRecord, &m_prelCur*/) )
        return;
                     
    CLangEnc* plng = NULL;
    if (m_prelCur && m_rpsCur.pfld)
        plng = m_rpsCur.pfld->pmkr()->plng();
    Shw_plngset()->ViewElements(plng);
}

void CShwView::OnUpdateFileLanguageEncodings(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( !Shw_papp()->pProject()->bLockProject() ); // 1.2bv Disable language encodings if project locked
}

void CShwView::OnFileDatabaseTypes()
{
    // 2000/05/02 TLB 7 MRP - bValidateCur now takes only one argument, which defaults to dirSameRecord
    if ( !bValidateCur(/*eSameRecord, &m_prelCur*/) )
        return;
                     
    CDatabaseType* ptyp = GetDocument()->ptyp();
    Shw_ptypset()->ViewElements(ptyp);
}

void CShwView::OnUpdateFileDatabaseTypes(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( !Shw_papp()->pProject()->bLockProject() ); // 1.2bv Disable language encodings if project locked
}

void CShwView::OnUpdateFileTextCorpus(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( TRUE ); // !Shw_papp()->pProject()->bLockProject() ); // 1.2dm Leave Project, Text Corpora active when project is locked
}

void CShwView::OnViewAutowrap()
{
    Shw_pProject()->SetAutoWrap( !Shw_pProject()->bAutoWrap() ); // toggle auto wrap setting
}

void CShwView::OnViewSetMargin()
{
    CRect rect;
    GetClientRect(&rect); // get current size of view
    CShwDoc* pdocCur = GetDocument();
	int iNewMargin = rect.right - m_iFldStartCol - 10; // subtract marker column from allowable line width
	if ( iNewMargin <= 0 ) // 1.4nb Protect against wrap margin setting less than zero
		iNewMargin = 1; // 1.4nb
    m_pind->pindsetMyOwner()->SetMargin( iNewMargin );
	pdocCur->SetModifiedFlag(); // 1.1be Fix bug of not saving modified wrap margin
//	AfxMessageBox(_("The margin has been set to the width of the current view."), MB_OK | MB_ICONINFORMATION ); // give user a little feedback // 1.5.8vc Remove message about setting wrap margin
}

void CShwView::OnUpdateViewSetMargin(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!m_bBrowsing && m_prelCur && !pdoc()->bReadOnly());
}

void CShwView::OnUpdateViewAutowrap(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck( Shw_pProject()->bAutoWrap() );
}

void CShwView::OnEditReshape()
{
    ASSERT( m_prelCur ); // should be disabled

    EditReshape();
    UpdateBasedOnChange();
}

void CShwView::OnEditReshapeDb()
{
    ASSERT( m_prelCur ); // should be disabled

    if ( AfxMessageBox(_("Set wrap to width of current window and reshape entire file?"), // 1.4vne Shorten question about reshaping entire file // 1.5.8yb Expand message to mention set wrap margin
            MB_YESNO | MB_ICONQUESTION) != IDYES ) // warn 'em
        return;
	OnViewSetMargin();
    EditReshapeDocument();
    GetDocument()->UpdateAllViews(NULL, 0L, 0);
}

BOOL CShwView::OnEraseBkgnd(CDC* pDC)
{
    if ((CRecLookEl*)m_prelCur != NULL)  // 1998-12-02 MRP
        return TRUE;
    else
        return CWnd::OnEraseBkgnd(pDC);
}

void CShwView::OnUpdateEditReshape(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!m_bBrowsing && m_prelCur && !pdoc()->bReadOnly() &&
        !m_rpsCur.pfld->pmkr()->bNoWordWrap());  // 1999-08-06 MRP
}

void CShwView::OnUpdateEditReshapeDb(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!m_bBrowsing && m_prelCur && !pdoc()->bReadOnly());
}

void CShwView::OnRButtonDown(UINT nFlags, CPoint pnt)
{
    if ( Shw_papp()->pviewActive() == this && GetFocus() != this ) // means find combo has focus
        SetFocus(); // force focus back to current view

    if ( !m_prelCur ) // Handle empty view from filter
        return;

    if ( m_bBrowsing )
        {
        BrowseOnRButtonDown(nFlags, pnt);
        return;
        }

    if ( bPtInDivider( pnt ) )
        return;

    if ( bPtInMarker( pnt ) )
        {
		CRecPos rps = m_rpsCur;
		pnt.x += GetScrollPosition().x;
		pntPntToPos( pnt, TRUE, &rps ); // get record position at point where mouse clicked
#ifdef MacWine // 1.5.1ke Use shift click to move to browse only on Mac
		if ( (::GetKeyState( VK_SHIFT ) < 0 ) ) // 1.2bq On ctrl right click, add to browse list // 1.5.1fg Change add marker to browse to shift click, works on Mac
#else
		if ( (::GetKeyState( VK_CONTROL ) < 0 ) ) // 1.2bq On ctrl right click, add to browse list // 1.5.1fg Change add marker to browse to shift click, works on Mac
#endif
			{
			BOOL bAlreadyIn = FALSE; // 1.5.1fh Make Ctrl+right click always switch to browse
			CBrowseFieldList* pbrflstT = pbrflst();
			for ( CBrowseField* pbrf = pbrflst()->pbrfFirst(); pbrf; pbrf = pbrflst()->pbrfNext( pbrf ) )
				{
				if ( pbrf->pmkr() == rps.pfld->pmkr() ) // 1.2bq If marker already in browse list, don't add it
					{
					bAlreadyIn = TRUE; // 1.5.1fh Remember mkr already in browse
//					return; // 1.5.1fh 
					}
				}
			if ( !bAlreadyIn ) // 1.5.1fh If already in, don't change browse layout
				{
				CMarker* pmkr = rps.pfld->pmkr();
				pbrflstT->Add( pmkr );
				BrowseJustificationDefault(); // 1.2bq Keep right justification correct
				InitBrowse(); // 1.2bx recalculate scroll bars, etc.			
				ResizeBrowse(); // 1.2bq Adjust browse width if needed
				ResetBrowseFieldWidths(); // 1.2bq Adjust browse widths to show new marker
				}
			SwitchToBrowseView(); // 1.2bx Switch to browse view on ctrl rt click on marker, also fixes bug of losing scroll bar
			}
		else
			{
			if ( !Shw_papp()->pProject()->bLockProject() ) // 1.2bs Don't show marker properties if project is locked
				{
				CMarkerPropertiesSheet dlg( rps.pfld->pmkr() );
				dlg.DoModal(); // allow user to modify this marker's properties
				}
			}
        return;
        }

    HideCaret();
    BOOL bSelectionCleared = FALSE;
    
    if ( bSelecting( eAnyText ) ) // if there was a selection, attempt to use selected text
        {
        CPoint pntBeg = pntPosToPnt( rpsSelBeg(), TRUE );
        CPoint pntEnd = pntPosToPnt( rpsSelEnd(), TRUE );
		if ( pntBeg.y == pntEnd.y // 1.4qrc Fix bug of not jumping to selection that crosses line end
				&& ( pnt.x < pntBeg.x - 20 || pnt.x > pntEnd.x + 20 || pnt.y < pntBeg.y - 14 || pnt.y > pntBeg.y + 45 ) ) // 1.4mb Fix bug (1.4k) of right click low in selection failing // 1.4dc If mouse not in selection on right click, don't jump to selection
			{
            ClearSelection();
            bSelectionCleared = TRUE;
			}
        }
        
    if ( !bSelecting( eAnyText ) )
        {
        CPoint pntOrg = GetScrollPosition(); // Allow for scrolling
        pnt.x += pntOrg.x;
        pnt.y += pntOrg.y;
        EditMouseLeftClick( pnt );
        SetCaretPosAdj(); // Move caret
        }
    ShowCaret();

    BOOL bDontShowDlg = TRUE; // The right-click Jump only works in context-sensistive
    BOOL bJumpEvenIfEmptyString = FALSE; // The right-click Jump is stricter than other flavors of EditJumpTo
#ifdef MacWine // 1.5.1ke Use shift click for jump insert only on Mac
    EditJumpTo( (::GetKeyState( VK_SHIFT ) & 0x8000) != 0, // 1.5.1ga Change Ctrl+click to Shift+click for jump insert
                bDontShowDlg, bJumpEvenIfEmptyString ); // do jump (jump insert if control pressed)
#else
    EditJumpTo( (::GetKeyState( VK_CONTROL ) & 0x8000) != 0, // 1.5.1ga Change Ctrl+click to Shift+click for jump insert
                bDontShowDlg, bJumpEvenIfEmptyString ); // do jump (jump insert if control pressed)
#endif
    if ( bSelectionCleared )
        Invalidate( FALSE );
}

void CShwView::OnToolsNumbering()
{
	ASSERT(m_prelCur);
    // 2000/05/02 TLB 7 MRP - bValidateCur now takes only one argument, which defaults to dirSameRecord
    if ( !bValidateCur(/*eSameRecord, &m_prelCur*/) )
        return;
    if ( !m_prelCur )
        {
        MessageBeep(0);
        return;
        }
    if ( GetDocument()->bNumber(this, m_prelCur, TRUE) && !m_bBrowsing )
        {
        // Invalidate( FALSE ); // TLB 01/06/2000 : Added to try to make it repaint the changed record (doesn't seem to help)
        // SetScroll( TRUE ); // TLB 03/01/2000 TLB : replaced with following (to fix cursor when it was in a \ref that has been deleted)
        ResynchCaretAndCursor();
        }
}

void CShwView::OnToolsReNumber()
{
	ASSERT(m_prelCur);
    // 2000/05/02 TLB 7 MRP - bValidateCur now takes only one argument, which defaults to dirSameRecord
    if ( !bValidateCur(/*eSameRecord, &m_prelCur*/) )
        return;
    if ( !m_prelCur )
        {
        MessageBeep(0);
        return;
        }
    if ( GetDocument()->bNumber(this, m_prelCur, FALSE) && !m_bBrowsing )
        {
        // Invalidate( FALSE ); // TLB 01/06/2000 : Added to try to make it repaint the changed record (doesn't seem to help)
        //SetScroll( TRUE ); // TLB 01/06/2000 : Added (to match Break & Number above) so
                           // scroll bar will be recalculated if renumber deletes bogus ref lines
        // TLB 03/01/2000 TLB : replaced SetScroll with following (to fix cursor when it was in a \ref that has been deleted)
        ResynchCaretAndCursor();
        }
}

void CShwView::OnUpdateToolsNumbering(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(ptyp()->bNumberingEnabled() && m_prelCur);
}

void CShwView::OnUpdateToolsReNumber(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(ptyp()->bNumberingEnabled() && m_prelCur);
}

//void CShwView::OnToolsMacros() // 1.5.0ah  // 1.5.9rc Remove all code related to macro recorder
//	{
//	if ( !bWinExecWait( "RECORDER.EXE TOOLBOX.REC", SW_SHOWNORMAL, FALSE ) ) // 1.5.0ah Add Tools, Macros command to activate macro recorder // 1.5.0aj 
//		AfxMessageBox ( _("Unable to find Recorder.exe."), MB_OK); // 1.4qpr // 1.5.0aj 
//	}

void CShwView::OnToolsCheckConsistency()
{
    CheckConsistency();
}

void CShwView::OnToolsNewConsistencyCheck()
{
#ifdef WordIdentifcationLater  // 1.5.1fa Disable start of word identification code, allow consistency check on word list
	Str8 sTyp = ptyp()->sName(); // 1.4qng Get database type name
	if ( sTyp.Find( "Word List" ) == 0 ) // 1.4qng On consistency check of word list, do word identification
		{
		CDocList doclst; // list of currently open docs
		for ( CShwDoc* pdocT = doclst.pdocFirst(); pdocT; pdocT = doclst.pdocNext() )
			{ // 1.4qng For each doc, see if word list and not this one
			Str8 sTypT = pdocT->ptyp()->sName(); // 1.4qng Get database type name
			if ( sTyp.Find( "Word List" ) == 0  // 1.4qng If word list and not this one, do word identification
					&& pdocT != pdoc() )
				{
				Str8 s = "";
				}
			}
		}
	else
#endif // 1.5.1fa 
	    RecheckConsistency();
}

void CShwView::OnUpdateToolsNewConsistencyCheck(CCmdUI* pCmdUI)
{
#ifdef JUNK // 1.5mn Messes up translation
    Str8 s;
    if ( m_pcck && m_pcck->bCheckInProgress() )
        s = _("New Consistenc&y Check...");
    else
        s = _("Consistenc&y Check...\tAlt+Y");
    pCmdUI->SetText( swUTF16( s) ); // 1.4qwu Upgrade SetText for Unicode build
#endif // JUNK // 1.5mn 
    pCmdUI->Enable(TRUE);
}

void CShwView::OnToolsContinueConsistencyCheck()
{
    CheckConsistency();
}

void CShwView::OnUpdateToolsContinueConsistencyCheck(CCmdUI* pCmdUI)
{
    Str8 s;
    if ( m_pcck && m_pcck->bCheckInProgress() )
        {
//        s = _("Continue Consistency Check\tAlt+Y"); // 1.5mn 
        pCmdUI->Enable(TRUE);
        }
    else
        {
//        s = _("Continue Consistency Check"); // 1.5mn 
        pCmdUI->Enable(FALSE);
        }
#ifdef JUNK // 1.5mn Messes up translation
    pCmdUI->SetText( swUTF16( s) ); // 1.4qwu Upgrade SetText for Unicode build
#endif // JUNK // 1.5mn 
}

void CShwView::OnEditFind()
{
	const char* pszDefault = NULL; // 1.4xd If selection, use it as default for find
    Str8 sText; // 1.4xd 
	Str8 sLastMkr; // 1.5.8b If last mkr same as this, keep field setting
    if ( m_prelCur && !m_bBrowsing && bSelecting(eText)) // 1.4wq On search, if selection, use it as default // 1.4xd 
		{
        sText = sGetCurText(); // use selected text or word/item at caret position for search // 1.4xd 
		pszDefault = sText; // 1.4xd 
		sLastMkr = Shw_pProject()->pfndset()->pszMkr(); // 1.5.8b 
		const char* pszMkr = m_rpsCur.pfld->pmkr()->sMarker(); // 1.4yd Make Find box show default in correct language encoding
		Shw_pProject()->pfndset()->SetMkr( pszMkr ); // 1.4yd 
		Shw_pProject()->pfndset()->SetLng( m_rpsCur.pfld->pmkr()->plng() ); // 1.4zaj Set language for find dialog box
		if ( sLastMkr != pszMkr ) // 1.5.8b If selection in different field, change setting to language
			Shw_pProject()->pfndset()->SetSingleField( FALSE ); // 1.4yd  // 1.4zaj Change find of selected text default to all fields of language
		}
    if ( Shw_pProject()->pfndset()->bModalView_Properties( GetDocument()->ptyp(), pszDefault, FALSE ) ) // bring up Find dialog // 1.4xd 
		{
	    CFindSettings* pfndset = Shw_pProject()->pfndset();
		if ( !pfndset->bSingleRecord() // 1.4am Allow find in whole database if not sorted by record marker, give warning
				&& ( m_pind->pmkrPriKey() != pdoc()->pmkrRecord() // 1.1 sj Fix bug of find not working correctly when not sorted by record marker
					&& !( ptyp()->bTextFile() && m_pind->pmkrPriKey() == ptyp()->pmkrTextRef() ) ) ) // 1.1tp Allow find whole database if text file sorted by ref
			AfxMessageBox( _("Database is not sorted by record marker. Find in whole database may find the same string multiple times.") ); // 1.4am Allow find in whole database if not sorted by record marker, give warning
		pfndset->ClearReplace(); // 1.4wr Fix U bug of find trying to continue replace
        bEditFind( TRUE );                          
		}
}

void CShwView::OnUpdateEditFind(CCmdUI* pCmdUI)
{
    pCmdUI->Enable((CRecLookEl*)m_prelCur != NULL);  // 1998-12-02 MRP
}

void CShwView::OnEditReplace()
{
	const char* pszDefault = NULL; // 1.4xd If selection, use it as default for find
    Str8 sText; // 1.4xd 
	Str8 sLastMkr; // 1.5.8b If last mkr same as this, keep field setting
    if ( m_prelCur && !m_bBrowsing && bSelecting(eText)) // 1.4wq On search, if selection, use it as default // 1.4xd 
		{
        sText = sGetCurText(); // use selected text or word/item at caret position for search // 1.4xd 
		pszDefault = sText; // 1.4xd 
		sLastMkr = Shw_pProject()->pfndset()->pszMkr(); // 1.5.8b 
		const char* pszMkr = m_rpsCur.pfld->pmkr()->sMarker(); // 1.4yd Make Find box show default in correct language encoding
		Shw_pProject()->pfndset()->SetMkr( pszMkr ); // 1.4yd 
		Shw_pProject()->pfndset()->SetLng( m_rpsCur.pfld->pmkr()->plng() ); // 1.4zaj Set language for find dialog box
		if ( sLastMkr != pszMkr ) // 1.5.8b If selection in different field, change setting to language
			Shw_pProject()->pfndset()->SetSingleField( FALSE ); // 1.4yd // 1.4zaj Change find of selected text default to all fields of language 
		}
    if ( Shw_pProject()->pfndset()->bModalView_Properties( GetDocument()->ptyp(), pszDefault, TRUE ) ) // bring up Find dialog // 1.4xd 
		{
	    CFindSettings* pfndset = Shw_pProject()->pfndset();
		if ( !pfndset->bSingleRecord() // 1.4am Allow find in whole database if not sorted by record marker, give warning
				&& ( m_pind->pmkrPriKey() != pdoc()->pmkrRecord() // 1.1 sj Fix bug of find not working correctly when not sorted by record marker
					&& !( ptyp()->bTextFile() && m_pind->pmkrPriKey() == ptyp()->pmkrTextRef() ) ) ) // 1.1tp Allow find whole database if text file sorted by ref
			{
			AfxMessageBox( _("Replace in whole database requires sort by record marker.") ); // 1.4am Don't allow replace in whole database if not sorted by record marker, even though find is allowed
			return;
			}
        bEditFind( TRUE );                          
		}
}

void CShwView::OnUpdateEditReplace(CCmdUI* pCmdUI)
{
    // 1998-12-02 MRP: cast
    pCmdUI->Enable((CRecLookEl*)m_prelCur != NULL && !pdoc()->bReadOnly());
	if ( Shw_papp()->pProject()->iLockProject() >= 10 ) // 1.2ca At project lock level 10, disable anything that can modify a file
		pCmdUI->Enable( FALSE );
}

void CShwView::OnEditSelectAll()
{
    HideCaret();
    StartSelection( eFields );
    MoveCtrlHome(); // start at top and select whole record
    m_rpsSelOrigin = m_rpsCur;
    EditCtrlEnd();
    Invalidate( FALSE );
    ShowCaret();
    Shw_pProject()->pkmnManager()->ActivateKeyboard(m_rpsSelOrigin.pmkr());
}

void CShwView::OnUpdateEditSelectAll(CCmdUI* pCmdUI)
{
    pCmdUI->Enable((CRecLookEl*)m_prelCur != NULL);  // 1998-12-02 MRP
}

void CShwView::OnEditDelwordLeft()
{
    if ( !m_prelCur || m_bBrowsing )
        {
        MessageBeep(0);
        return;
        }
    if ( !bSelecting( eAnything ) )
        {
        StartSelection( eText );
        if ( !bEditCtrlLeft() )
            {
            ClearSelection();
            MessageBeep(0);
            return;
            }
        }
    HideCaret();
    DeleteSelection();
    SetCaretPosAdj(); // Move caret
    LineWrap(); // 2000-09-07 TLB: Do auto-wrap or emergency line breaking to 1000-chars max
    UpdateBasedOnChange();
    ShowCaret();
    Shw_pProject()->pkmnManager()->ActivateKeyboard(m_rpsCur.pmkr());
}

void CShwView::OnEditDelwordRight()
{
    if ( !m_prelCur || m_bBrowsing )
        {
        MessageBeep(0);
        return;
        }
    if ( !bSelecting( eAnything ) )
        {
        StartSelection( eText );
        if ( !bEditCtrlRight() )
            {
            ClearSelection();
            MessageBeep(0);
            return;
            }
        }
    HideCaret();
    DeleteSelection();
    SetCaretPosAdj(); // Move caret
    LineWrap(); // 2000-09-07 TLB: Do auto-wrap or emergency line breaking to 1000-chars max
    UpdateBasedOnChange();
    ShowCaret();
    Shw_pProject()->pkmnManager()->ActivateKeyboard(m_rpsCur.pmkr());
}

void CShwView::OnEditSelectField()
{
    if ( !m_prelCur || m_bBrowsing )
        {
        MessageBeep(0);
        return;
        }
    HideCaret();
    if ( bSelecting( eAnything ) && !bSelecting( eFields ) )
        ClearSelection();

    if ( bSelecting( eFields ) ) // at least one field already selected
        bMoveDownField();
    else
        {
        m_rpsSelOrigin = m_rpsCur;
        m_rpsSelOrigin.SetPos(0); // select whole field
        }
    while ( bMoveDownLine() ) // put m_rpsCur at end of field
        ;
    m_rpsCur.bMoveEnd();
    StartSelection( eFields );

    SetCaretPosAdj(); // Move caret
    Invalidate( FALSE );
    ShowCaret();
    Shw_pProject()->pkmnManager()->ActivateKeyboard(m_rpsSelOrigin.pmkr());
}

void CShwView::OnEditSelectLine()
{
    if ( !m_prelCur || m_bBrowsing )
        {
        MessageBeep(0);
        return;
        }
    HideCaret();
    if ( bSelecting( eAnything ) && !bSelecting( eLines ) )
        ClearSelection();

    if ( bSelecting( eLines ) ) // at least one field already selected
        bMoveDown();
    else
        {
        m_rpsSelOrigin = m_rpsCur;
        m_rpsSelOrigin.bMoveHome(); // select whole line
        }
    m_rpsCur.bMoveEnd();
    StartSelection( eLines );

    SetCaretPosAdj(); // Move caret
    Invalidate( FALSE );
    ShowCaret();
    Shw_pProject()->pkmnManager()->ActivateKeyboard(m_rpsSelOrigin.pmkr());
}

void CShwView::OnEditCtrlSpace()
{
    if ( !m_prelCur || m_bBrowsing ||
            bSelecting( eAnything ) ||
            !m_rpsCur.bMoveRightPastSubfieldTag() )
        {
        MessageBeep(0);
        return;
        }
    Shw_pProject()->pkmnManager()->ActivateKeyboard(m_rpsCur.pmkr());
}

// 1998-10-13 MRP: Moved CShwView:OnDebugMark() to mark.cpp

void CShwView::OnDebugAlan()
{
// #ifdef _DEBUG  // Activate
    extern void DebugAlan(CShwView*); 
    DebugAlan(this);
// #endif  // _DEBUG
}

  //09-12-1997 Did some changes in order to provide the visible
  //markers in the Browseview 
void CShwView::OnViewBothmarkersandnames()
{
    m_bViewMarkers = TRUE;
    m_bViewFieldNames = TRUE;
    if (!m_bBrowsing)
        AdjustLeftColumn();
    else
        Invalidate(FALSE);           
}

void CShwView::OnUpdateViewBothmarkersandnames(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck( m_bViewFieldNames && m_bViewMarkers );
    //09-12-1997 Replaced the following line in order to provide the visible
    //markers in  the Browseview
    //pCmdUI->Enable(!m_bBrowsing);
    pCmdUI->Enable(TRUE);
}


void CShwView::AdjustLeftColumn()
{
        HideCaret();
        m_iFldStartCol = 0; // Set up start column
        if ( m_bViewMarkers )
            {
            m_iFldStartCol += s_iFldStartColForMarker;
            if ( m_bViewMarkerHierarchy )
                m_iFldStartCol += s_iFldStartColForMarkerHierarchy;
            }
        if ( m_bViewFieldNames )
            m_iFldStartCol += s_iFldStartColForFieldName;
        if (m_prelCur)
            {
            bEditHome(); // Do Home to reset caret position
            SetCaretPosAdj(); // Move caret
            }
        ShowCaret();
        Invalidate(FALSE); // Repaint window     
}


void CShwView::OnFilePageSetup()
{
    // TODO: Add your command handler code here
    PageSetup();    
}


void CShwView::ResetUndo( CRecord* prec )
{
	ASSERT ( prec );
	if ( m_prelCur->prec() == prec )
		{
		ResetUndo();
		ResetUndoAll();
		}
}

#ifdef _MAC
void CShwView::OnWindowClose()
{
    CWnd* pwndFrame = GetParent();
    pwndFrame->SendMessage( WM_CLOSE ); // give user a way to close a single view.  equivalent to Ctrl-F4 in Windows
}
#endif

LRESULT CShwView::OnMCNotify(WPARAM wParam, LPARAM lParam)
{
    Str8 sErr;
#ifndef _MAC
//    bCloseWaveDevice(m_hWavDevice, sErr); // 1.5.0af Fix bug of not starting new play file if play in process Doesn't seem to be needed, since starting a new play closes it, This was preventing the next play from starting if started while prev was in process
#endif // not _MAC
    return 0L;
}

BOOL CShwView::bPlayableFile( CField* pfld, Str8& sFilePath, Str8& sType, Str8& sFrom, Str8& sTo ) // 1.2mn For Play Sound or Play File, see if field is file name (dot followed by non-space) // 1.4sg
	{
    if ( !bParseWaveLine( pfld->sContents(), sFilePath, sFrom, sTo ) ) // 1.4sf
		return FALSE; // 1.4sf
	sFilePath.TrimRight(); // 1.4sf
	int iDot = sFilePath.ReverseFind( '.' ); // See if dot in this field // 1.4sf
	if ( iDot >= 0 ) // If dot found, collect the file type // 1.4sf
		sType = sFilePath.Mid( iDot ); // 1.4sf
	Str8 sFilePathToFolder; // 1.5.0jd Remember path of play sound or file as default 
	Str8 sFileName = sFilePath; // 1.5.0jd 
	int iColon = sFilePath.Find( ':' ); // 1.5.0kf Require colon to save path for play sound
	int iSlash = sFilePath.ReverseFind( '\\' ); // 1.5.0jd 
	if ( iColon > 0 && iSlash >= 0 ) // 1.5.0jd  // 1.5.0kf Require colon to save path for play sound
		{
		sFilePathToFolder = sFilePath.Left( iSlash + 1 ); // 1.5.0jd 
		sFileName = sFilePath.Mid( iSlash + 1 ); // 1.5.0jd 
		}
	Str8 sDocPath = sUTF8( pdoc()->GetPathName() ); // 1.4sd Fix bug U of play file not working
	iSlash = sDocPath.ReverseFind( '\\' );
	if ( iSlash >= 0 )
		sDocPath = sDocPath.Left( iSlash + 1 );
	if ( bFileExists(sFilePath) ) // 1.5.0jd If file exists, accept it
		{
		if ( sFilePathToFolder != "" ) // 1.5.0jd If path given, remember it
			ptyp()->SetPlaySoundPath( sFilePathToFolder ); // 1.5.0hc Remember play sound path for file names with no path given
		return TRUE;
		}
	if ( sFilePathToFolder == "" ) // 1.5.0jd If file not found and no path given, try remembered path
		{
		sFilePath = ptyp()->sPlaySoundPath() + sFileName; // 1.5.0jc Add previously rememberd path
		if ( bFileExists(sFilePath) ) // 1.5.0jd If file exists, accept it
			return TRUE;
		}
	if ( sFilePathToFolder == "" ) // 1.5.0jd If remembered path not found, try doc path
		{
		sFilePath = sDocPath + sFileName; // 1.5.0jc Add doc path
		if ( bFileExists( sFilePath ) ) // 1.5.0jd If file exists, accept it
			return TRUE;
		}
	return FALSE; // 1.5.0jd If no file path exists, return FALSE
	}

BOOL CShwView::bFindPlayableFile( Str8& sPath, Str8& sType, Str8& sFrom, Str8& sTo  ) // 1.2mn For Play Sound or Play File, find file name in record
	{
	CRecord* prec = m_rpsCur.prec;
	CField* pfld = m_rpsCur.pfld;
	if ( ptyp()->bTextFile() ) // 1.5.0jc Make play sound in text find correct sound file
		{
		BOOL bFound = FALSE; // 1.5.0jc 
		for ( pfld = m_rpsCur.pfld; pfld; pfld = m_rpsCur.prec->pfldPrev( pfld ) ) // Search from current field upward for reference // 1.5.0jc 
			if ( pfld->pmkr() == ptyp()->pmkrTextRef() ) // 1.5.0jc 
				{
				bFound = TRUE; // 1.5.0jc 
				break;
				}
		if ( !bFound ) // If no ref found, use current field // 1.5.0jc 
			pfld = m_rpsCur.pfld; // 1.5.0jc 
		}
	for ( ; pfld; pfld = prec->pfldNext( pfld ) ) // 1.2mn First look below
		if ( bPlayableFile( pfld, sPath, sType, sFrom, sTo ) )
			{
			return TRUE;
			}
	for ( pfld = prec->pfldFirst(); pfld; pfld = prec->pfldNext( pfld ) ) // 1.2mn If not found below, try above
		if ( bPlayableFile( pfld, sPath, sType, sFrom, sTo ) )
			{
			return TRUE;
			}
	return FALSE;
	}

#ifndef _MAC
void CShwView::OnEditPlayFile() // 1.2kw Add menu item for Tools, Play File
{
	if (m_bBrowsing) // 1.5.0cb 
		{
		SwitchToRecordView(); // 1.5.0cb 
		SwitchToBrowseView(); // 1.5.0cb 
		}
    Str8 sPath;
	Str8 sType;
	Str8 sFrom; // 1.4sg
	Str8 sTo; // 1.4sg
	if ( !bFindPlayableFile( sPath, sType, sFrom, sTo ) ) // 1.2mn On Play Sound, search for field with file name  // 1.4sg
		{
		AfxMessageBox( _("No playable file found.") ); // 1.2rc On Play File, give message if no playable file found
		return;
		}
    Str8 sErr; // 1.5mg 
    bCloseWaveDevice(m_hWavDevice, sErr); // 1.5mg Fix bug (1.5.0af) of not closing wav file before play file
    HKEY hkey = 0; 
	const int maxlenBuffer = 1000; // 1.4sd
	_TCHAR pszFileType[maxlenBuffer]; // 1.4sd
	long lenFileType = maxlenBuffer;
	long lReturn = ::RegQueryValue(HKEY_CLASSES_ROOT,  swUTF16( sType ), pszFileType, &lenFileType); // 1.2mp Get file type for this extension // 1.4sd
	if ( lReturn != ERROR_SUCCESS )
		return;
	_TCHAR pszCommand[maxlenBuffer]; // 1.4sd
	long lenCommand = maxlenBuffer;
	Str8 sFileTypeEdit = sUTF8( pszFileType ); // 1.2rh Make Play File look for Edit command as well as Open // 1.4sd
	sFileTypeEdit += "\\shell\\Edit\\command"; 
	lReturn = ::RegQueryValue(HKEY_CLASSES_ROOT,  swUTF16( sFileTypeEdit ), pszCommand, &lenCommand); // 1.2mp Get command for this file type
	if ( lReturn != ERROR_SUCCESS ) // 1.2rh If no Edit command, look for Open
		{
		lenCommand = maxlenBuffer;
		Str8 sFileTypeOpen =  sUTF8( pszFileType ); // 1.4sd
		sFileTypeOpen += "\\shell\\Open\\command";
		lReturn = ::RegQueryValue(HKEY_CLASSES_ROOT,  swUTF16( sFileTypeOpen ), pszCommand, &lenCommand); // 1.2mp Get command for this file type // 1.4sd
		if ( lReturn != ERROR_SUCCESS )
			return;
		}

	Str8 sCommand =  sUTF8( pszCommand ); // 1.2mp Clean up the command
	int iPercent = sCommand.Find( '%' ); // 1.2mp If there is a percent arg, remove it
	if ( iPercent > 0 )
		sCommand = sCommand.Left( iPercent - 1 );
	sCommand += " "; // 1.2mp Append space and file name to be played
	sPath = '"' + sPath; // 1.2mp Put quotes around path
	sPath += '"';
	sCommand += sPath;
	bWinExecWait( sCommand.GetBuffer(1000), SW_SHOW, FALSE ); // 1.2mp Perform the command
	sCommand.ReleaseBuffer();
}

void CShwView::OnEditPlaysound()
{
	if (m_bBrowsing) // 1.5.0cb 
		{
		SwitchToRecordView(); // 1.5.0cb 
		SwitchToBrowseView(); // 1.5.0cb 
		}
    Str8 sPath;
	Str8 sType;
	Str8 sFrom; // 1.4sg
	Str8 sTo; // 1.4sg
	if ( !bFindPlayableFile( sPath, sType, sFrom, sTo ) ) // 1.2mn On Play Sound, search for field with file name 
		return;
    bPlaySound( sPath, sFrom, sTo );
}

BOOL CShwView::bPlaySound( const char* lpszFilePath, Str8 sFrom, Str8 sTo )
{
    Str8 sFilePath = lpszFilePath;
    BOOL bHasFromTo = FALSE; // 1.4sg
    DWORD dwFrom = 0; // 1.4sg
	DWORD dwTo = 0; // 1.4sg
	sFrom.Trim(); // 1.4zak 
	if ( !sFrom.IsEmpty() ) // 1.4sg
		{
		bHasFromTo = TRUE;
		double dblFrom = atof( sFrom ); // 1.4zak Don't assume exactly 3 digits after decimal point on sound numbers
		dblFrom = dblFrom * 1000; // 1.4zak 
		dwFrom = (DWORD)dblFrom; // 1.4zak 
		}
	sTo.Trim(); // 1.4zak 
	if ( !sTo.IsEmpty() ) // 1.4sg
		{
		bHasFromTo = TRUE;
		double dblTo = atof( sTo ); // 1.4zak Don't assume exactly 3 digits after decimal point on sound numbers
		dblTo = dblTo * 1000; // 1.4zak 
		dwTo = (DWORD)dblTo; // 1.4zak 
		}
    Str8 sErr;
    DWORD dwErr = dwPlayWAVEFile(sFilePath, GetSafeHwnd(), &m_hWavDevice, bHasFromTo, dwFrom, dwTo, sErr);
    if (dwErr != 0)
        {
        Str8 sMsg = _("Wave sound file error:"); // 1.5.0fg 
		sMsg = sMsg + " " + sFilePath + " " + sErr; // 1.5.0fg 
        AfxMessageBox(sMsg);
        return FALSE;
        }
    return TRUE;
}

void CShwView::OnUpdateEditPlaysound(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(TRUE && m_prelCur); // !m_bBrowsing && m_prelCur); // 1.5.0cb Allow play sound in browse view
}

void CShwView::OnUpdateEditPlayFile(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(TRUE && m_prelCur); // !m_bBrowsing && m_prelCur); // 1.5.0cb Allow play file in browse view
}
#endif // not _MAC

void CShwView::ActivateFilterCombo() // 1.5.0ad Add Ctrl+Shift+F to activate filter combo box
	{
	Shw_pmainframe()->pcboFilterSet()->SetFocus();
	Shw_pmainframe()->pcboFilterSet()->ShowDropDown();
	}


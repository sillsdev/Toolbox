// mainfrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "toolbox.h"
#include "shw.h"
#include "project.h"

#include "mainfrm.h"
#include "shwdoc.h"
#include "shwview.h"
#include "kmn.h"
#include "ind.h"
#include "fil.h"

#ifdef _MAC
#include "resmac.h" // Mac specific resources
#else
#include "respc.h"
#endif

#include <afxpriv.h>    // For WM_COMMANDHELP, WM_SETMESSAGESTRING
#include <afxwin.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

#ifndef _MAC
// 1998-10-20 MRP: See Technical Note #6 in MFCNotes.hlp
UINT NEAR WM_SANTA_FE_FOCUS = RegisterWindowMessage( swUTF16( "SantaFeFocus") ); // 1.4qya Upgrade RegisterWindowMessage for Unicode build
static UINT NEAR WM_SAVE_ALL = RegisterWindowMessage( swUTF16( "ShbxSaveAll") ); // 1.4qya
static UINT NEAR WM_REFRESH_ALL = RegisterWindowMessage( swUTF16( "ShbxRefreshAll") ); // 1.4qya
#endif  // not _MAC

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
    //{{AFX_MSG_MAP(CMainFrame)
    ON_WM_CREATE()
    ON_COMMAND(ID_DEBUG_ALAN, OnDebugAlan)
    ON_COMMAND(ID_LOCK_LEVEL, OnDebugMark)
    ON_COMMAND(ID_DEBUG_TOM, OnExerciseToggle)
    ON_COMMAND(ID_DEBUG_ROD, OnDebugRod)
    ON_COMMAND(ID_DEBUG_SNAPSHOT, OnDebugSnapshot)
    ON_COMMAND(ID_DEBUG_ASSERT_VALID, OnDebugAssertValid)
    ON_COMMAND(ID_DEBUG_BRIAN, OnBugReport)
    ON_COMMAND(ID_DEBUG_BRINGIN, OnBringIn)
    ON_COMMAND(ID_VIEW_STATUS_BAR, OnViewStatusBar)
    ON_UPDATE_COMMAND_UI(ID_VIEW_STATUS_BAR, OnUpdateViewStatusBar)
    ON_WM_INITMENU()
    ON_WM_ACTIVATEAPP()
    ON_COMMAND(ID_EDIT_FINDNEXT, OnEditFindnext)
    ON_UPDATE_COMMAND_UI(ID_EDIT_FINDNEXT, OnUpdateEditFindnext)
    ON_COMMAND(ID_EDIT_FINDPREV, OnEditFindprev)
    ON_UPDATE_COMMAND_UI(ID_EDIT_FINDPREV, OnUpdateEditFindprev)
    ON_COMMAND(ID_HELP_CONTENTS, OnHelpContents)
    ON_COMMAND(ID_HELP_SEARCH, OnHelpSearch)
    ON_WM_ENDSESSION()
    //}}AFX_MSG_MAP
    // Other commands 
    ON_WM_MENUSELECT()
    ON_MESSAGE(WM_SETMESSAGESTRING, OnSetMessageString)
    // Global help commands
    ON_COMMAND(ID_HELP_INDEX, CMDIFrameWnd::OnHelpIndex)
    ON_COMMAND(ID_HELP_USING, CMDIFrameWnd::OnHelpUsing)
    ON_COMMAND(ID_HELP, CMDIFrameWnd::OnHelp)
    ON_COMMAND(ID_DEFAULT_HELP, CMDIFrameWnd::OnHelpIndex)
    ON_COMMAND(ID_CONTEXT_HELP, CMDIFrameWnd::OnContextHelp)
    // toolbar combo messages
    ON_CBN_SELENDOK(IDC_FILSEL_FILTERCOMBO, OnSelEndOkFiltersetCombo)
    ON_CBN_SELENDCANCEL(IDC_FILSEL_FILTERCOMBO, OnSelEndCancelFiltersetCombo)
    ON_CBN_DBLCLK(IDC_FILSEL_FILTERCOMBO, OnDblClkFiltersetCombo)
#ifdef ToolbarFindCombo // 1.4yq
    ON_CBN_SELCHANGE(IDC_FIND_COMBO, OnSelChangeFindCombo)
    ON_COMMAND(IDOK, OnOK)
    ON_COMMAND(IDCANCEL, OnCancel)
#endif // ToolbarFindCombo // 1.4yq
#ifndef _MAC
    ON_REGISTERED_MESSAGE(WM_SANTA_FE_FOCUS, OnSantaFeFocus)
    ON_REGISTERED_MESSAGE(WM_SAVE_ALL, OnShbxSaveAll)
    ON_REGISTERED_MESSAGE(WM_REFRESH_ALL, OnShbxRefreshAll)

#endif  // not _MAC
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// arrays of IDs used to initialize control bars


#ifdef ToolbarFindCombo // 1.4yq
#define IDX_FIND_COMBO 16
#define IDX_FILTER_COMBO 22  // position on toolbar // 1.4ywh 
#else // 1.4yq
#define IDX_FILTER_COMBO 20  // position on toolbar // 1.4ywh 
#endif // ToolbarFindCombo // 1.4yq

// toolbar buttons - IDs are command buttons
static UINT BASED_CODE buttons[] =
{
    // same order as in the bitmap 'toolbar.bmp'
    ID_FILE_OPEN,
    ID_FILE_SAVE,
        ID_SEPARATOR,
    ID_EDIT_CUT,
    ID_EDIT_COPY,
    ID_EDIT_PASTE,
        ID_SEPARATOR,
    ID_DATABASE_PREVIOUS_RECORD,
    ID_DATABASE_NEXT_RECORD,
    ID_DATABASE_FIRST_RECORD,
    ID_DATABASE_LAST_RECORD,
        ID_SEPARATOR,
    ID_EDIT_INTERLINEARIZE, // 1.6.4zk Change toolbar button to interlinearize instead of adapt
        ID_SEPARATOR,
    ID_VIEW_BROWSE,
        ID_SEPARATOR,
#ifdef ToolbarFindCombo // 1.4yq
	ID_SEPARATOR, // find combo
		ID_SEPARATOR,
#endif // ToolbarFindCombo // 1.4yq
    ID_EDIT_FINDPREV, // 1.4ywh 
    ID_EDIT_FIND, // 1.4ywh Add Find button to toolbar
    ID_EDIT_FINDNEXT, // 1.4ywh 
        ID_SEPARATOR,
    ID_SEPARATOR, // filter combo
        ID_SEPARATOR,
    ID_VIEW_REAPPLY_FILTER, // 2000/04/27 TLB
#if 0    
    ID_HELP_INDEX_, //ID_APP_ABOUT,
    ID_CONTEXT_HELP,
#endif    
};


// 2000/04/28 TLB Let the toolbar set its own buttons. Now two possible states.
BOOL CShwToolBar::SetButtons()
{
    int iButtonCount = sizeof(buttons)/sizeof(UINT);
    if ( !m_bShowingReapplyFilter )
        iButtonCount--;
    BOOL bRetVal = CToolBar::SetButtons(buttons, iButtonCount);

//    SetButtonInfo(IDX_FIND_COMBO, ID_SEPARATOR, TBBS_SEPARATOR, 6 ); //5.95e
//    SetButtonInfo(IDX_FIND_COMBO-1, ID_SEPARATOR, TBBS_SEPARATOR, 1 ); //5.95e
//    SetButtonInfo(IDX_FIND_COMBO+1, ID_SEPARATOR, TBBS_SEPARATOR, 1 ); //5.95e Replaced next 2 lines with these 3
#ifdef ToolbarFindCombo // 1.4yq
    SetButtonInfo(IDX_FIND_COMBO, IDC_FIND_COMBO, TBBS_SEPARATOR, 145 );
    SetButtonInfo(IDX_FIND_COMBO-1, ID_SEPARATOR, TBBS_SEPARATOR, 12 );
#endif // ToolbarFindCombo // 1.4yq
    SetButtonInfo(IDX_FILTER_COMBO, IDC_FILSEL_FILTERCOMBO, TBBS_SEPARATOR, 160 ); // 6.0zp Widen filter combo control
    SetButtonInfo(IDX_FILTER_COMBO-1, ID_SEPARATOR, TBBS_SEPARATOR, 12 );

    return bRetVal;
}

void CShwToolBar::OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler)
{
    // 2000/04/28 TLB - Make sure toolbar is in correct state first: hide/show Reapply Filter button as needed.
    CMainFrame* pfrm = (CMainFrame*)pTarget->GetActiveFrame();
    CShwView* pview = NULL;

    if ( pfrm )
        {
        pview = (CShwView*)pfrm->GetActiveView();

        if ( pview )
            {
            if ( m_bShowingReapplyFilter )
                {
                // Should we hide this button?
                if ( !pview->pind() || !pview->pind()->pfil() || !pview->pind()->pfil()->bFilterDependsOnRecordContext() )
                    {
                    m_bShowingReapplyFilter = FALSE;
                    SetButtons();
                    }
                }
            else
                {
                // Should we show this button?
                if ( pview->pind() && pview->pind()->pfil() && pview->pind()->pfil()->bFilterDependsOnRecordContext() )
                    {
                    m_bShowingReapplyFilter = TRUE;
                    SetButtons();
                    }
                }
            }
        else if ( m_bShowingReapplyFilter )
            {
                m_bShowingReapplyFilter = FALSE;
                SetButtons();
            }
        }

    CToolBar::OnUpdateCmdUI(pTarget, bDisableIfNoHndler);

    if (GetFocus() == &m_cboFilterSet)
        return; // don't update if user is selecting from combo
    
    if (pfrm == NULL)
        return;

    if (pview == NULL)
        {
        m_cboFilterSet.ResetContent();
        m_cboFilterSet.EnableWindow(FALSE);
#ifdef ToolbarFindCombo // 1.4yq
        m_cboFind.EnableWindow(FALSE);
#endif // ToolbarFindCombo // 1.4yq
        return;
        }
        
    // If it's a Shoebox database view, then
    if (!pview->IsKindOf(RUNTIME_CLASS( CShwView ) ) )
        return;

    pview->UpdateFilterSetCombo(&m_cboFilterSet);
    m_cboFilterSet.EnableWindow(TRUE);
    
#ifdef ToolbarFindCombo // 1.4yq
    if ( m_cboFind.bHasChanged( Shw_pProject()->pfndset()->pftxlst() ) ) // 5.95e removed these 3 lines
        m_cboFind.UpdateElements();
    m_cboFind.EnableWindow( pview->lGetRecordCount() != 0 );
#endif // ToolbarFindCombo // 1.4yq
}


/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction
BOOL CMainFrame::s_bAppLosingFocus = FALSE;

CMainFrame::CMainFrame() :
    m_pkbdOnMenuSelect(NULL),
    m_bMenuSelected(FALSE),
    m_hOldAccel(NULL),
    m_bShowingReapplyFilter(TRUE)
{
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    // PostMessage(WM_SETMESSAGESTRING, (WPARAM)NULL, 0L);
    
    DWORD dwStyle = WS_CHILD|WS_VISIBLE|CBRS_TOP;   // default
    dwStyle |= CBRS_TOOLTIPS;     

    if (!m_wndToolBar.Create(this, dwStyle) ||
        !m_wndToolBar.LoadBitmap(IDR_SHWTYPE) ||
        !m_wndToolBar.SetButtons())
        {
        return -1;      // fail to create
        }

    CRect rect;
#ifdef ToolbarFindCombo // 1.4yq
    // Create the find combo box
//    m_wndToolBar.SetButtonInfo(IDX_FIND_COMBO, IDC_FIND_COMBO, TBBS_SEPARATOR, 145 );

    // Design guide advises 12 pixel gap between combos and buttons
//    m_wndToolBar.SetButtonInfo(IDX_FIND_COMBO-1, ID_SEPARATOR, TBBS_SEPARATOR, 12 );
    m_wndToolBar.GetItemRect( IDX_FIND_COMBO, &rect);
    rect.top = 1;
    rect.bottom = rect.top + 150;   
    if (!pcboFind()->Create( 
			CBS_DROPDOWN | CBS_AUTOHSCROLL | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL, // 5.95e removed visible, tabstop, and vscroll
            rect, &m_wndToolBar, IDC_FIND_COMBO)) // 5.96x Change to dropdownlist
        {
        return FALSE;
        }
    pcboFind()->SetExtendedUI(); // make so down arrow in combo drops the listbox // 5.95e removed this line
    pcboFind()->Init(); // initialize find combo
#endif // ToolbarFindCombo // 1.4yq

    // Create the filter combo box
    // Design guide advises 12 pixel gap between combos and buttons
    m_wndToolBar.GetItemRect( IDX_FILTER_COMBO, &rect);
    rect.top = 1;
    rect.bottom = rect.top + 400;
    if (!pcboFilterSet()->Create(CBS_DROPDOWNLIST | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL,
            rect, &m_wndToolBar, IDC_FILSEL_FILTERCOMBO))
        {
        return FALSE;
        }

    if (!(m_wndStatusBar.Create(this,WS_CHILD|WS_VISIBLE|CBRS_BOTTOM, IDW_SHW_STATUS_BAR)) ||
        !m_wndStatusBar.UseNormalIndicators())
        {
        return -1;      // fail to create
        }
	
    return 0;
}

// 2000/04/28 TLB Function where we handle hiding/showing Reapply Filter menu item, and removing menus for reduced menu set
void CMainFrame::OnInitMenu(CMenu* pMenu)
{
    CShwView* pview = (CShwView*)Shw_papp()->pviewActive();
	if ( !pview ) // 1.2hc Fix bug of crash on File, Open after Project, New
		return;

	if ( Shw_papp()->pProject()->iLockProject() == 0 ) // 1.2hb Don't mess with menus if some might be removed
		{
		CMenu* pMenuView = pMenu->GetSubMenu( 5 );
        if ( m_bShowingReapplyFilter )
            {
            // Should we hide this menu item?
            if ( !pview->pind() || !pview->pind()->pfil() || !pview->pind()->pfil()->bFilterDependsOnRecordContext() )
                {
                // Is this the first time we're removing it? Then store state info.
                if ( m_sReapplyFilterMenuText.IsEmpty() )
                    {
                    for (m_nViewMenu = 0; (int)m_nViewMenu < pMenu->GetMenuItemCount(); m_nViewMenu++)
                        {
                        pMenuView = pMenu->GetSubMenu(m_nViewMenu);
                        for (m_nReapplyFilter = 0; (int)m_nReapplyFilter < pMenuView->GetMenuItemCount(); m_nReapplyFilter++)
                            {
                            if ( pMenuView->GetMenuItemID(m_nReapplyFilter) == ID_VIEW_REAPPLY_FILTER )
                                {
//								char buffer[DLGBUFMAX+1]; // 1.4heg Use char buffer for GetMenuString // 1.4qyh
//                              pMenuView->GetMenuString(m_nReapplyFilter, (char*)buffer, DLGBUFMAX, MF_BYPOSITION); // 1.4qyh
								CString sw; // 1.4qyh
                                pMenuView->GetMenuString(m_nReapplyFilter, sw, MF_BYPOSITION); // 1.4qyh Upgrade ReapplyFilter for Unicode build
								m_sReapplyFilterMenuText =  sUTF8( sw ); // 1.4qyh
                                ASSERT(!m_sReapplyFilterMenuText.IsEmpty());
                                break;
                                }
                            }
                        if ( !m_sReapplyFilterMenuText.IsEmpty() )
                            {
                            // 2000/05/05 TLB - The member m_nViewMenu stores the zero-based index of View
                            // menu, excluding system menu if maximized.
#ifndef _MAC
                            BOOL bActiveChildIsMaximized;
                            MDIGetActive(&bActiveChildIsMaximized);
                            if ( bActiveChildIsMaximized )
                                m_nViewMenu--;
#endif
                            break;
                            }
                        }
                    }
                else
                    {
                    pMenuView = pMenu->GetSubMenu(nViewMenuActual());
                    }
                pMenuView->DeleteMenu(m_nReapplyFilter, MF_BYPOSITION); // Delete command
                pMenuView->DeleteMenu(m_nReapplyFilter, MF_BYPOSITION); // Delete separator
                m_bShowingReapplyFilter = FALSE;
                }
            }
        else
            {
            // Should we show this menu item?
            if ( pview->pind() && pview->pind()->pfil() && pview->pind()->pfil()->bFilterDependsOnRecordContext() )
                {
                m_bShowingReapplyFilter = TRUE;
                ASSERT(!m_sReapplyFilterMenuText.IsEmpty());
                CMenu* pMenuView = pMenu->GetSubMenu(nViewMenuActual());
                pMenuView->InsertMenu(m_nReapplyFilter, MF_SEPARATOR | MF_BYPOSITION);
                pMenuView->InsertMenu(m_nReapplyFilter, MF_STRING | MF_BYPOSITION, ID_VIEW_REAPPLY_FILTER,  swUTF16( m_sReapplyFilterMenuText) ); // 1.4qyh
                }
            }
		}

}

int CMainFrame::iAdjustForMaximized( int i ) // 1.2bv Find correct menu item, allowing for maximize changing item number
{
    BOOL bActiveChildIsMaximized;
    MDIGetActive(&bActiveChildIsMaximized);
    if ( bActiveChildIsMaximized )
		i += 1;
	return i;
}

// 2000/05/05 TLB - The member m_nViewMenu stores the zero-based index of View
// menu, excluding system menu if maximized.  This function returns the actual
// index, taking the system menu into consideration.
int CMainFrame::nViewMenuActual() const
{
#ifndef _MAC
    BOOL bActiveChildIsMaximized;
    MDIGetActive(&bActiveChildIsMaximized);
    if ( bActiveChildIsMaximized )
        return m_nViewMenu + 1;
    else
        return m_nViewMenu;
#else
    return m_nViewMenu;
#endif
}

void CMainFrame::OnEditFindnext()
{
	CFindSettings* pfndset = Shw_pProject()->pfndset(); // 1.4kn Fix bug of not finding after typing into toolbar box
	pfndset->ClearReplace(); // 1.4wr Fix U bug of find trying to continue replace
#ifdef ToolbarFindCombo // 1.4yq
	Str8 sLastFind; // 1.4sq
	CFindText* pftxLastFind = pfndset->pftxlst()->pftxFirst(); // 1.4sq
	if ( pftxLastFind ) // 1.4sq Fix U bug of possible crash on find button
		sLastFind = pftxLastFind->pszText(); // 1.4sq
	Str8 sFind; // 1.4sq
	CFindText* pftxFind = pcboFind()->pftxSelected(); // pfndset->pftxlst()->pftxFirst(); // 1.4sq // 1.4ym 
	if ( pftxFind ) // 1.4sq Fix U bug of possible crash on find button
		sFind = pftxFind->pszText(); // 1.4sq // 1.4ym 
	if ( sFind.IsEmpty() ) // 1.4ym 
		sFind = pcboFind()->elc()->sGetEditLngText(); // 1.4ym 
	if( sFind != sLastFind ) // 1.4kn
		{
		CLangEnc* plng = Shw_plngset()->plngDefault(); // pfndset->pftxlst()->pftxFirst()->plng(); // 1.4kn // 1.4sq
		if ( pftxLastFind ) // 1.4sq
			plng = pftxLastFind->plng(); // 1.4sq
		pfndset->pftxlst()->Add( sFind, plng ); // 1.4kn
		}
#endif // ToolbarFindCombo // 1.4yq
    Find( TRUE );
}

void CMainFrame::OnUpdateEditFindnext(CCmdUI* pCmdUI)
{
    CShwView* pview = pviewGetActive();
    pCmdUI->Enable( pview && pview->lGetRecordCount() ); // disable if no view or view empty
}

void CMainFrame::OnEditFindprev()
{
	CFindSettings* pfndset = Shw_pProject()->pfndset(); // 1.4kn Fix bug of not finding after typing into toolbar box
	pfndset->ClearReplace(); // 1.4wr Fix U bug of find trying to continue replace
#ifdef ToolbarFindCombo // 1.4yq
	Str8 sLastFind; // 1.4sq
	CFindText* pftxLastFind = pfndset->pftxlst()->pftxFirst(); // 1.4sq
	if ( pftxLastFind ) // 1.4sq Fix U bug of possible crash on find button
		sLastFind = pftxLastFind->pszText(); // 1.4sq
	Str8 sFind; // 1.4sq
	CFindText* pftxFind = pcboFind()->pftxSelected(); // pfndset->pftxlst()->pftxFirst(); // 1.4sq // 1.4ym 
	if ( pftxFind ) // 1.4sq Fix U bug of possible crash on find button
		sFind = pftxFind->pszText(); // 1.4sq // 1.4ym 
	if ( sFind.IsEmpty() ) // 1.4ym 
		sFind = pcboFind()->elc()->sGetEditLngText(); // 1.4ym 
	if( sFind != sLastFind ) // 1.4kn
		{
		CLangEnc* plng = Shw_plngset()->plngDefault(); // pfndset->pftxlst()->pftxFirst()->plng(); // 1.4kn // 1.4sq
		if ( pftxLastFind ) // 1.4sq
			plng = pftxLastFind->plng(); // 1.4sq
		pfndset->pftxlst()->Add( sFind, plng ); // 1.4kn
		}
#endif // ToolbarFindCombo // 1.4yq
    Find( FALSE ); // find in reverse direction
}

void CMainFrame::OnUpdateEditFindprev(CCmdUI* pCmdUI)
{
    CShwView* pview = pviewGetActive();
    pCmdUI->Enable( pview && pview->lGetRecordCount() ); // disable if no view or view empty
}

#ifdef ToolbarFindCombo // 1.4yq
void CMainFrame::OnSelChangeFindCombo()
{
    CFindText* pftx = pcboFind()->pftxSelected();
    if ( !pftx )
        return;
    pcboFind()->SetLangEnc( pftx->plng() ); // set edit control to language of selected item
	pcboFind()->elc()->SetEditLngText( pftx->pszText() ); // 1.4ym 
    Shw_pProject()->pkmnManager()->ActivateKeyboard( pftx->plng() );
}

void CMainFrame::OnOK() // executes when Enter pressed in Find combo
{
	CFindSettings* pfndset = Shw_pProject()->pfndset(); // 1.4kn Fix bug of not finding after typing into toolbar box
	pfndset->ClearReplace(); // 1.4wr Fix U bug of find trying to continue replace
	CFindText* pftx = pfndset->pftxlst()->pftxFirst(); // 1.4pj Fix bug (1.4kn) of crash on enter in empty find combo
	CLangEnc* plng = Shw_pProject()->plngset()->plngDefault(); // 1.4ym 
	if ( pftx )
		plng = pftx->plng(); // 1.4pj // 1.4kn
	Str8 sFind = pcboFind()->elc()->sGetEditLngText(); // 1.4ym Try to make find combo on Toolbar work with legacy
	pfndset->pftxlst()->Add( sFind, plng ); // 1.4kn
    Find( TRUE );
}
#endif // ToolbarFindCombo // 1.4yq

void CMainFrame::Find( BOOL bForward )
{
    CShwView* pview = pviewGetActive();
    ASSERT( pview != NULL ); // Find Combo should be disabled if no view open
    pview->bEditFind( bForward ); // have view do find operation
    pview->SetFocus(); // force focus back to view
}

void CMainFrame::OnCancel() // executes when Escape pressed in Find combo
{
    CShwView* pview = Shw_papp()->pviewActive();
    ASSERT( pview ); // if no active view, parent combo should be disabled
    pview->SetFocus(); // user is aborting find text combo, force focus back to view
}

CShwView* CMainFrame::pviewGetActive() // Get active view - returns NULL if no active view
{
    CMDIChildWnd* pActiveChild = MDIGetActive();
    if (pActiveChild == NULL)
        return NULL;
    return (CShwView*)pActiveChild->GetActiveView(); // return active view
}

void CMainFrame::OnSelEndCancelFiltersetCombo()
{
    CShwView* pview = pviewGetActive();
    if (pview)
        pview->SetFocus();
}

void CMainFrame::OnSelEndOkFiltersetCombo()
{
    CShwView* pview = pviewGetActive();
    if (!pview)
        return;
    pview->UseFilter(pcboFilterSet()->pfilSelected()); // change view's filter
    pview->SetFocus(); // force focus back to view (it doesn't happen correctly otherwise)
}

void CMainFrame::OnDblClkFiltersetCombo() // allow user to edit current filter's properties
{
    // 1998-02-03 MRP: Fix bugs #187 and #219 where validation doesn't get done
    // before modifying a filter that causes reindexing of the active view.
    // NOTE: When there aren't any MDI child windows, the filter drop-down list
    // should be disabled, therefore this function shouldn't get called.
    CMDIChildWnd* pwndActiveChildFrame = MDIGetActive();
    ASSERT( pwndActiveChildFrame );
    CShwView* pview = CShwView::s_pviewActiveChild(pwndActiveChildFrame);

    // 1998-02-03 MRP: THERE'S A SUBTLE BUG HERE
    // If a filter that gets modified is used by *other* non-active windows
    // (perhaps even in other databases), they are not getting validated
    // before the filter update causes them to be reindexed.
    if ( pview && !pview->bValidateCur() ) // 1.4zba Protect against possibile null view
        return;
    
    CFilter* pfil = pcboFilterSet()->pfilSelected();
    if (pfil)
        {
        pfil->bModify();
        pcboFilterSet()->ResetContent(); // make so combo gets reloaded in idle loop as names may have changed
        }
    else
        MessageBeep(0);
}

void CMainFrame::OnUpdateFrameTitle( BOOL bAddToTitle ) // override to save current window title to .ini file
{
    CMDIFrameWnd::OnUpdateFrameTitle( bAddToTitle );
#ifndef _MAC
	CString sDlgItem; // 1.4qpx
	GetWindowText( sDlgItem ); // 1.4ec Change to buffer // 1.4qpx
	Str8 sText = sUTF8( sDlgItem ); // 1.4qpx
    if ( sText != m_sCurrentTitle ) // if title has changed, update it in .ini file
        {
        m_sCurrentTitle = sText;
        Shw_papp()->WriteToIni_Title( sText ); // save in .ini file to keep another instance of Shoebox from running
        }
#endif
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
    CMDIFrameWnd::AssertValid();
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

void CMainFrame::OnDebugAlan()
{
#ifdef _DEBUG
    extern void DebugAlan();  // alan.cpp
    DebugAlan();
#endif  // _DEBUG
}

// 1998-10-13 MRP: Moved CMainFrame:OnDebugMark() to mark.cpp

void CMainFrame::OnDebugRod()
{
#ifdef _DEBUG
    extern void DebugRod(CFrameWnd* pFrameWnd);  // 1995-08-02
    DebugRod(this);
#endif  // _DEBUG
}

void CMainFrame::OnDebugSnapshot()
{
#ifdef _DEBUG
#endif  // _DEBUG
}

void CMainFrame::OnDebugAssertValid()
{
#ifdef _DEBUG
    Shw_AssertValid();
#endif  // _DEBUG
}

void CMainFrame::OnBugReport()
{
    extern void BugReport();  // brian.cpp
    BugReport();
}

void BringIn() // 1.4db Add Ctrl+Shift+B to bring in window that is off screen // 1.4vwe 
{
	CWnd* pwnd = Shw_papp()->m_pMainWnd;
	WINDOWPLACEMENT wpl;
	pwnd->GetWindowPlacement(&wpl);
    wpl.showCmd = SW_SHOWNORMAL;
//    wpl.ptMinPosition.x = -1; // 1.4tgd 
//    wpl.ptMinPosition.y = -1; // 1.4tgd 
	CRect rect = wpl.rcNormalPosition;
	int iHt = rect.bottom - rect.top; // 1.4tgd Find current height
	if ( iHt < 10 ) // 1.4tgd If height is negative or too small, make it positive
		iHt = 500;
	int iWid = rect.right - rect.left; // 1.4tgd Find current width
	if ( iWid < 10 ) // 1.4tgd If width is negative or too small, make it positive
		iWid = 500;
	rect.top = 0; // 1.4tgd Force top to location 0
	rect.bottom = iHt; // 1.4tgd Keep same height as before
	rect.left = 0; // 1.4tgd Force left to location 0
	rect.right = iWid; // 1.4tgd Keep same width as before
    wpl.rcNormalPosition = rect; // CRect( 0, 0, 640, 480 ); // 1.4tgd Don't resize window on Ctrl+Shift+B bring into screen
	pwnd->SetWindowPlacement(&wpl);
	Shw_papp()->pProject()->SetAmbigPos( 0, 20 ); // 1.4vwf Make bring in command (Ctrl+Alt+B) bring in ambig box
}

void CMainFrame::OnBringIn() // 1.4db Add Ctrl+Shift+B to bring in window that is off screen // 1.4vwe 
	{
	BringIn(); // 1.4vwe Separate function so it can be called from elsewhere
	}

void CMainFrame::OnViewStatusBar()
{
    OnBarCheck(IDW_SHW_STATUS_BAR);
}

void CMainFrame::OnUpdateViewStatusBar(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck( bStatusBarVisible() );
}

void CMainFrame::OnActivateApp(BOOL bActive, DWORD hTask)
{
#ifdef DISABLE // 1.5.0dc Disable save on lose focus (1.4zae) because of crash
	if ( !bActive && Shw_papp()->bProjectOpen() ) // 1.4zae When Toolbox loses focus, if auto save, save all modified // 1.4zbd Fix bug (1.4zae) of crash on close project
		{
		if ( Shw_pProject()->bAutoSave() ) // 1.4zae If auto save, save all modified
			(void) Shw_app().bSaveAllFiles( true, FALSE ); // 1.4zae Maintain write protect, save without asking
		}
#endif // DISABLE // 1.5.0dc 

    CMDIFrameWnd::OnActivateApp(bActive, hTask);

    // Prevent CShwView::OnKillFocus from deactivating keyboards.
    if ( !bActive )
        s_bAppLosingFocus = TRUE;
}


// RNE 1996-02-20
// OnMenuSelect captures the WM_MENUSELECT message that is sent every time the user
// moves to a menu item or leaves the menu system (as a whole).
void CMainFrame::OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hSysMenu)
{
    if ( Shw_papp()->bProjectOpen() )
        {
        if ( (nFlags == 0xFFFF) && (hSysMenu == 0) )    // Leaving menu system.
            {
            m_bMenuSelected = FALSE;
            
            // Reactivate the keyboard that was in use (if any).
            if ( m_pkbdOnMenuSelect )
                Shw_pProject()->pkmnManager()->ActivateKeyboard(m_pkbdOnMenuSelect);
            m_pkbdOnMenuSelect = NULL;  // 1998-07-07 MRP
            }
        else    // Menu item highlighted.
            {
            // Note:  OnMenuSelect is called every time we go to a new menu item.
            //Therefore we want action only the first time we go into the menus.
            if  (!m_bMenuSelected)
                {
                m_bMenuSelected = TRUE;
                
                // Remember the current keyboard, then disable keyman.
                m_pkbdOnMenuSelect = Shw_pProject()->pkmnManager()->pkbdGetCurrent();
                if ( m_pkbdOnMenuSelect )  // 1998-07-07 MRP
                    Shw_pProject()->pkmnManager()->DeActivateKeyboards();
                }
            }
        }
        
    CMDIFrameWnd::OnMenuSelect(nItemID, nFlags, hSysMenu);  // Call base.
}

#ifndef _MAC
int i = RAND_MAX;
int iTbxRand() // Get random number so that messages from this instance of Toolbox will be different from those from another instance
	{
	srand( (unsigned)time( NULL ) );
	return rand() / 2 + 10;
	}

extern Str8 g_sCurrentExternalJumpRef; // 1.5.9pc Fix bug of wrong behavior on jump from Paratext

LPARAM FROM_TOOLBOX = iTbxRand(); // 1.2me Make number different for each instance of Toolbox // 1.2md Signal that this focus message is from Tbx
LRESULT CMainFrame::OnSantaFeFocus(WPARAM wParam, LPARAM lParam)
{
	if ( lParam == FROM_TOOLBOX ) // 1.2md If focus message is from this Tbx, ignore it
		return 0;
	const int maxlenBuffer = 256;
	char pszBuffer[maxlenBuffer];
	DWORD len = maxlenBuffer;
    if ( wParam == 4 )  // 1998-10-20 MRP: Word focus
		{
		HKEY hkey = 0; // 1.2rj Change registry query to try to fix failure of jump from Paratext on certain Unicode characters, didn't help
		long lReturn = ::RegOpenKeyEx(HKEY_CURRENT_USER,  swUTF16( "Software\\SantaFe\\Focus\\Word" ), 0, KEY_READ, &hkey); // 1.4qyf
		if ( lReturn != ERROR_SUCCESS )
			return 0;
        DWORD dwType = 0;
        lReturn = ::RegQueryValueExA(hkey, NULL, NULL, &dwType, (unsigned char*)pszBuffer, &len); // 1.4whg Fix U bug of external word focus not working on Unicode
        ::RegCloseKey(hkey);
		if ( lReturn != ERROR_SUCCESS )
			return 0;
		// rde 52w // Make Toolbox get focus on external jump, min & restore is kludge, but works on all OS versions
		CWnd* pWnd = AfxGetMainWnd();
		pWnd->ShowWindow(SW_MINIMIZE);
		pWnd->ShowWindow(SW_RESTORE);
		Shw_papp()->bExternalJump(pszBuffer, NULL ); // Do external jump
		}
	else if ( wParam == 1 ) // 1.2mc Do reference follow
		{
		HKEY hkey = 0;  // 1.4qyg
		long lReturn = ::RegOpenKeyEx(HKEY_CURRENT_USER,  swUTF16( "Software\\SantaFe\\Focus\\ScriptureReference" ), 0, KEY_READ, &hkey); // 1.4qyg Upgrade RegOpenKeyEx for Unicode build
		if ( lReturn != ERROR_SUCCESS )
			return 0;
        DWORD dwType = 0;
        lReturn = ::RegQueryValueExA(hkey, NULL, NULL, &dwType, (unsigned char*)pszBuffer, &len); // 1.4qyg // 1.4xb Fix U bug of external ref focus not working on Unicode
        ::RegCloseKey(hkey);
		char* pszSpace = strchr( pszBuffer, ' ' ); // 1.2mc Change reference from MAT 1:1 to Toolbox style MAT.1:17
		if ( pszSpace ) // 1.2md Make sure space is found
			*pszSpace = '.';
		Str8 sRef = pszBuffer; // 1.5.9pc Get ref for compare
		sRef.Trim(); // 1.5.9pc Trim extra spaces
		if ( sRef == g_sCurrentExternalJumpRef ) // 1.5.9pc If ref same as current, don't jump
			return 0; // 1.5.9pc 
		g_sCurrentExternalJumpRef = sRef; // 1.5.9pc Remember current external ref
		Shw_papp()->bExternalJump(pszBuffer, NULL, FALSE ); // Do external jump
		}
    return 0;
}

LRESULT CMainFrame::OnShbxSaveAll(WPARAM, LPARAM)
{
    Shw_app().bSaveAllFiles( FALSE ); // 1.4whd Make files writable after external program does save all
    return 0;
}

LRESULT CMainFrame::OnShbxRefreshAll(WPARAM, LPARAM)
{
    Shw_app().RefreshAllDocs();
    return 0;
}
#endif  // not _MAC

// CShwViewFrame is specified in the document template in InitInstance (shw.cpp).

#undef new
IMPLEMENT_DYNCREATE(CShwViewFrame, CMDIChildWnd)
#define new DEBUG_NEW

BEGIN_MESSAGE_MAP(CShwViewFrame, CMDIChildWnd)
    //{{AFX_MSG_MAP(CShwViewFrame)
    ON_WM_CLOSE()
    ON_WM_NCLBUTTONDOWN()
    ON_WM_MDIACTIVATE()
	ON_WM_SYSCOMMAND()
    //}}AFX_MSG_MAP
    ON_MESSAGE(WM_SETMESSAGESTRING, OnSetMessageString)
END_MESSAGE_MAP()

void CShwViewFrame::OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd) 
{
// 1998-02-04 MRP: Calling CShwView::UpdateIndexes from CShwViewFrame::OnMDIActivate
// appears to cause bug #127 "Range set on jump not checked".
// I think we can remove both these functions.
// Jump, Adapt, Interlinearize, and Spell Check all call ValidateAllViews
// and this step gets in the way by incorrectly clearing bModifiedSinceValidated.

    if ( !bActivate ) // we are losing focus!
        {
        CShwView* pview = (CShwView*)GetActiveView();
        ASSERT( pview );
        pview->UpdateIndexes();
        }

    CMDIChildWnd::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);
}

// catch attempted view close and keep it open if validation of view fails
void CShwViewFrame::OnClose()
{
    CShwView* pview = (CShwView*)GetActiveView();
    ASSERT( pview );
    if ( !pview->bValidate() )
        return; // don't destroy
	if ( pview->bViewLocked() && !Shw_papp()->pProject()->bClosing() ) // 1.2bn Don't let user close view if it is locked
		return;
    CMDIChildWnd::OnClose();
}

void CShwViewFrame::OnSysCommand( UINT nID, LPARAM lParam ) // 1.2kr Capture minimize and maximize for project lock
	{ // this->GetWindowPlacement (Allow restore or maximize if minimized
	if ( Shw_papp()->pProject()->iLockProject() >= 6 ) // 1.2kr At level 6, remove Window menu and prevent minimize and maximize
		{
		UINT nIDTmp = nID & 0xFFF0; // Trim off lower 4 bits 
		if ( nIDTmp == SC_MINIMIZE // 1.2kr At project lock level 6 block max, min & restore
				|| nIDTmp == SC_MAXIMIZE
				|| nIDTmp == SC_SIZE
				|| nIDTmp == SC_MOVE
				|| nIDTmp == SC_RESTORE )
			{
			return;
			}
		}
	CWnd::OnSysCommand( nID, lParam );
	}

void CShwViewFrame::OnNcLButtonDown(UINT nFlags, CPoint point)
{
    CShwView* pview = (CShwView*)GetActiveView();
    if ( pview && pview != GetFocus() ) // means find combo doesn't want to give up focus
        pview->SetFocus(); // force focus from find combo back to view
    
    CMDIChildWnd::OnNcLButtonDown(nFlags, point);
}

afx_msg LRESULT CShwViewFrame::OnSetMessageString(WPARAM wParam, LPARAM lParam)
{
    // strip any bubble help text from the resource ID strings
  
    // are we processing a ID request?
    if (wParam)
        {
        // yes
        // load the resource string
//		HINSTANCE hinst = AfxGetInstanceHandle( ); // 1.4gc Change status bar loadstring to not use CStrg // 1.4qyb
//		char buffer[DLGBUFMAX+1]; // 1.4gc // 1.4qyb
//		LoadString( hinst, wParam, (char*)buffer, DLGBUFMAX ); // 1.4gc // 1.4qyb
		CString sw; // 1.4qyb Upgrade LoadString for Unicode build
        sw.LoadString( wParam ); // 1.4gc // 1.4qyb
        Str8  str =  sUTF8( sw ); // 1.4qyb
    
        // check for the bubble text
        int nNewLine = str.ReverseFind('\n');
  
        // do we have a delimiter?
        if(nNewLine != -1)
            {
            // strip the newline delimited text from the string
            str = str.Left(nNewLine); // up to the delimited part
      
            // copy to a temporary char buffer for Str8 safety
			int len = str.GetLength()+1;
            char* pstrTmp = new char[len];
            strcpy_s(pstrTmp, len, str);
      
            // call the base class to handle the modified message
            // CFrameWnd::OnSetMessageString(0, (LONG)(char FAR*)pstrTmp);
            Shw_pbarStatus()->SetStatusMessage(pstrTmp);
            //CMDIFrameWnd::OnSetMessageString(0, (LONG)(char FAR*)pstrTmp);
            // free the temporary buffer
            delete pstrTmp;
      
            m_nIDLastMessage = (UINT)wParam;    // new ID (or 0)
            m_nIDTracking = (UINT)wParam;       // so F1 on toolbar buttons work

            // return that we processed this message
            return TRUE;
            }
        else    // Print the non-delimited message
            {
            Shw_pbarStatus()->SetStatusMessage(str);
            //CMDIFrameWnd::OnSetMessageString(wParam, 0L);
            }
        }
    else
        {
        ASSERT(wParam == 0);    // can't have both an ID and a string
        // set an explicit string
        LPCSTR lpsz = (LPCSTR)lParam;
        Shw_pbarStatus()->SetStatusMessage(lpsz);
        //CMDIFrameWnd::OnSetMessageString(0, lParam);
        }

    m_nIDLastMessage = (UINT)wParam;    // new ID (or 0)
    m_nIDTracking = (UINT)wParam;       // so F1 on toolbar buttons work

    return TRUE;
}


void CMainFrame::OnHelpContents()
{
    CWinApp* papp = AfxGetApp();  
    papp->WinHelp(0, HELP_CONTENTS);
}

void CMainFrame::OnHelpSearch()
{
    CWinApp* papp = AfxGetApp();
    static char* pszEmpty = "";
    
    papp->WinHelp((DWORD)pszEmpty, HELP_PARTIALKEY);
}

afx_msg LRESULT CMainFrame::OnSetMessageString(WPARAM wParam, LPARAM lParam)
{
    // strip any bubble help text from the resource ID strings
  
    // are we processing a ID request?
    if (wParam)
        {
        // yes
        // load the resource string
//		HINSTANCE hinst = AfxGetInstanceHandle( ); // 1.4gc Change status bar loadstring to not use CStrg // 1.4qyb
//		char buffer[DLGBUFMAX+1]; // 1.4gc // 1.4qyb
//		LoadString( hinst, wParam, (char*)buffer, DLGBUFMAX ); // 1.4gc // 1.4qyb
		CString sw; // 1.4qyb Upgrade LoadString for Unicode build
        sw.LoadString( wParam ); // 1.4gc // 1.4qyb
        Str8  str =  sUTF8( sw ); // 1.4qyb
    
        // check for the bubble text
        int nNewLine = str.ReverseFind('\n');
  
        // do we have a delimiter?
        if(nNewLine != -1)
            {
            // strip the newline delimited text from the string
            str = str.Left(nNewLine); // up to the delimited part
      
            // copy to a temporary char buffer for Str8 safety
			int len = str.GetLength()+1;
            char* pstrTmp = new char[len];
            strcpy_s(pstrTmp, len, str);
      
            // call the base class to handle the modified message
            // CFrameWnd::OnSetMessageString(0, (LONG)(char FAR*)pstrTmp);
            Shw_pbarStatus()->SetStatusMessage(pstrTmp);
            //CMDIFrameWnd::OnSetMessageString(0, (LONG)(char FAR*)pstrTmp);
            // free the temporary buffer
            delete pstrTmp;
      
            m_nIDLastMessage = (UINT)wParam;    // new ID (or 0)
            m_nIDTracking = (UINT)wParam;       // so F1 on toolbar buttons work

            // return that we processed this message
            return TRUE;
            }
        else    // Print the non-delimited message
            {
            Shw_pbarStatus()->SetStatusMessage(str);
            //CMDIFrameWnd::OnSetMessageString(wParam, 0L);
            }
        }
    else
        {
        ASSERT(wParam == 0);    // can't have both an ID and a string
        // set an explicit string
        LPCSTR lpsz = (LPCSTR)lParam;
        Shw_pbarStatus()->SetStatusMessage(lpsz);
        //CMDIFrameWnd::OnSetMessageString(0, lParam);
        }

    m_nIDLastMessage = (UINT)wParam;    // new ID (or 0)
    m_nIDTracking = (UINT)wParam;       // so F1 on toolbar buttons work

    return TRUE;
}



CWnd* CMainFrame::GetMessageBar()
{
    return GetDescendantWindow(IDW_SHW_STATUS_BAR, TRUE);
}

void CMainFrame::OnEndSession(BOOL bEnding)
{
    if (bEnding)
        {
//      AfxOleSetUserCtrl(TRUE);    // keeps from randomly shutting
        Shw_papp()->HideApplication();
        Shw_papp()->CloseAllDocuments(FALSE); // fake normal close so cleanup happens properly
        // allow application to save settings, etc.
        Shw_papp()->ExitInstance();
        }
}

void CMainFrame::EnableAccelerators()
{
    if (!m_hAccelTable) {
        m_hAccelTable = m_hOldAccel;
        m_hOldAccel = NULL;
    }
}

void CMainFrame::DisableAccelerators()
{
    if (m_hAccelTable) {
        m_hOldAccel = m_hAccelTable;
        m_hAccelTable = NULL;
    }
}

void CMainFrame::OnSetPreviewMode( BOOL bPreview, CPrintPreviewState* pModeStuff )
{
    // Josh Kelley 7/2/97
    // The child's accelerators and system menu remain active by default
    // during the print preview.  Accessing any of these can cause
    // unhandled exceptions, faults, etc.  The following code disables
    // accelerators; I don't know how to safely and thoroughly disable the
    // items on the system menu, but that is difficult to access, so it is
    // less of a problem.  
    CShwViewFrame *pChild = (CShwViewFrame*) GetActiveFrame();
    if (bPreview)
        pChild->DisableAccelerators();
    else
        pChild->EnableAccelerators();
    CMDIFrameWnd::OnSetPreviewMode (bPreview, pModeStuff);
}

void CShwViewFrame::EnableAccelerators()
{
    if (!m_hAccelTable) {
        m_hAccelTable = m_hOldAccel;
        m_hOldAccel = NULL;
    }
}

void CShwViewFrame::DisableAccelerators()
{
    if (m_hAccelTable) {
        m_hOldAccel = m_hAccelTable;
        m_hAccelTable = NULL;
    }
}

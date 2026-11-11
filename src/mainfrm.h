// mainfrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "kbd.h"
#include "status.h"
#include "filsel_d.h"
#include "find_d.h"

///////////////////////////////////////////////////////////////
class CShwToolBar : public CToolBar
{
public:
    CShwToolBar() : CToolBar(), m_cboFilterSet(NULL), m_bShowingReapplyFilter(FALSE) {}
    
#ifdef ToolbarFindCombo // 1.4yq
    CFindComboBox m_cboFind;
#endif // ToolbarFindCombo // 1.4yq
    CFilterSetComboBox m_cboFilterSet;
private:
    // 2000/04/28 TLB - Member for hiding/showing Reapply Filter icon
    BOOL m_bShowingReapplyFilter;
public: 
    virtual void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);
    BOOL SetButtons();
};

///////////////////////////////////////////////////////////////
class CMainFrame : public CMDIFrameWnd
{
    DECLARE_DYNAMIC(CMainFrame)
public:
    CMainFrame();

// Attributes
private:
    BOOL m_bMenuSelected;
    CKeyboard* m_pkbdOnMenuSelect;
        // This is the keyboard that was active (if any were)
        // when the menu system is entered. Shoebox deactivates
        // Keyman's keyboards when you use the menu system
        // and then reactivates the appropriate keyboard.
        // These two members are for the exclusive use of OnMenuSelect.
    
    static BOOL s_bAppLosingFocus;  // See CShwView::OnKillFocus
    Str8 m_sKeyboardOnActivateApp;  // 1998-07-07 MRP
        // This is the keyboard that was active (if any were)
        // when you switched to or from Shoebox. Shoebox automatically
        // reactivates it when you exit or switch applications.
        // This member is for the exclusive use of OnActivateApp.

    Str8 m_sCurrentTitle; // cache our window title so we don't write it to .ini unnecessarily

    HACCEL m_hOldAccel;  // Stores the application's accelerator table in case we 
        // need to temporarily disable accelerators.

    // 2000/04/28 TLB - Members for hiding/showing View, Reapply Filter menu item
    BOOL m_bShowingReapplyFilter;
    Str8 m_sReapplyFilterMenuText;
    // 2000/05/05 TLB - Use nViewMenuActual to get the real-time index of the View menu, taking the
    // system menu into consideration (it is added to the menu bar when the active MDI child is maximized).
    UINT m_nViewMenu; // zero-based index of View menu (excluding system menu if maximized)
    UINT m_nReapplyFilter; // zero-based index of reapply filter menu item on View menu
    int nViewMenuActual() const;

// Operations
public:
    virtual CWnd* GetMessageBar();

// Implementation
public:
    virtual ~CMainFrame();

    CFilterSetComboBox* pcboFilterSet() { return &m_wndToolBar.m_cboFilterSet; }
#ifdef ToolbarFindCombo // 1.4yq
    CFindComboBox* pcboFind() { return &m_wndToolBar.m_cboFind; }
#endif // ToolbarFindCombo // 1.4yq

	int CMainFrame::iAdjustForMaximized( int i ); // 1.2bv Find correct menu item, allowing for maximize changing item number

#ifdef _DEBUG
    virtual void AssertValid() const;
#endif

protected:  // control bar embedded members
    CShwStatusBar  m_wndStatusBar;
    CShwToolBar    m_wndToolBar;

public:
    CShwView* pviewGetActive(); // Get active view - returns NULL if no active view

    virtual void OnUpdateFrameTitle( BOOL bAddToTitle ); // override to save current window title to .ini file

    BOOL bStatusBarVisible() const // return TRUE if status bar is visible
        { return (m_wndStatusBar.GetStyle() & WS_VISIBLE) != 0; }
    BOOL bToolBarVisible() const // return TRUE if tool bar is visible
        { return (m_wndToolBar.GetStyle() & WS_VISIBLE) != 0; }
    void ShowStatusBar( BOOL bShow ) // show or hide status bar
        { if ( bStatusBarVisible() == !bShow ) OnBarCheck(IDW_SHW_STATUS_BAR); }
    void ShowToolBar( BOOL bShow ) // show or hide toolbar
        { if ( bToolBarVisible() == !bShow ) OnBarCheck(AFX_IDW_TOOLBAR); }
        
    static BOOL s_bIsAppLosingFocus()
        { BOOL b = s_bAppLosingFocus; s_bAppLosingFocus = FALSE; return b; }
    // NOTE:  s_bIsAppLosingFocus always resets the app losing focus flag
    // to FALSE upon being called.

    void EnableAccelerators();  // Enable or disable the keyboard accelerator
    void DisableAccelerators(); // table (should be enabled most of the time).

protected:
    void Find( BOOL bForward ); // do Find operation

// Generated message map functions
    //{{AFX_MSG(CMainFrame)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDropFiles(HDROP hDrop);
    afx_msg void OnDebugAlan();
    afx_msg void OnDebugMark();
    afx_msg void OnExerciseToggle();
    afx_msg void OnDebugRod();
    afx_msg void OnDebugSnapshot();
    afx_msg void OnDebugAssertValid();
    afx_msg void OnBugReport();
    afx_msg void OnBringIn();
    afx_msg void OnViewStatusBar();
    afx_msg void OnUpdateViewStatusBar(CCmdUI* pCmdUI);
    afx_msg void OnInitMenu(CMenu* pMenu);
    afx_msg void OnActivateApp(BOOL bActive, DWORD hTask);
    afx_msg void OnSelEndCancelFiltersetCombo();
    afx_msg void OnSelEndOkFiltersetCombo();
    afx_msg void OnDblClkFiltersetCombo();
    afx_msg void OnSelChangeFindCombo();
    afx_msg void OnEditFindnext();
    afx_msg void OnUpdateEditFindnext(CCmdUI* pCmdUI);
    afx_msg void OnEditFindprev();
    afx_msg void OnUpdateEditFindprev(CCmdUI* pCmdUI);
    afx_msg void OnHelpContents();
    afx_msg void OnHelpSearch();
    afx_msg void OnEndSession(BOOL bEnding);
    //}}AFX_MSG
    afx_msg void OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hSysMenu);
    afx_msg LRESULT OnSetMessageString(WPARAM wParam, LPARAM lParam);
    afx_msg void OnOK();
    afx_msg void OnCancel();
    afx_msg void OnSetPreviewMode( BOOL bPreview, CPrintPreviewState* pModeStuff );
#ifndef _MAC
    afx_msg LRESULT OnSantaFeFocus(WPARAM wParam, LPARAM lParam);  // 1998-10-20 MRP
    afx_msg LRESULT OnShbxSaveAll(WPARAM, LPARAM);
    afx_msg LRESULT OnShbxRefreshAll(WPARAM, LPARAM);
#endif  // not _MAC
    DECLARE_MESSAGE_MAP()
    
};


/////////////////////////////////////////////////////////////////////////////
// This class is specified in the document template in InitInstance (shw.cpp).
class CShwViewFrame : public CMDIChildWnd
{
public:
    void EnableAccelerators();
    void DisableAccelerators();
protected:
    HACCEL m_hOldAccel;
    DECLARE_DYNCREATE(CShwViewFrame)
    //{{AFX_MSG(CShwViewFrame)
    afx_msg void OnClose();
	afx_msg void OnSysCommand( UINT nID, LPARAM lParam );
    afx_msg void OnNcLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);
    //}}AFX_MSG
    afx_msg LRESULT OnSetMessageString(WPARAM wParam, LPARAM lParam);
    DECLARE_MESSAGE_MAP()
};

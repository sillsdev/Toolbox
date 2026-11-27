// ambig_d.cpp Choose among ambiguities

#include "stdafx.h"
#include "toolbox.h"
#include "shw.h"
#include "typ.h"
#include "font.h"
#include "ambig_d.h"
#include "project.h"  // Shw_pProject()->iLangCtrlHeight()
#include "wdf.h"  // CWordFormulaSet

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


// --------------------------------------------------------------------------

CAmbigListBox::CAmbigListBox( CFieldList* pfldlst ) : CDblListListBox( pfldlst )
{
    m_pfldlst = pfldlst;
}

void CAmbigListBox::InitLabels()
{
    m_xWord = xSubItem_Show();
}

void CAmbigListBox::DrawElement(CDC& cDC, const RECT& rcItem,
            const CDblListEl *pel)
{
    CField* pfld = (CField*)pel;
    // CFont* pfnt = (CFont*)pfld->pmkr()->pfntDlg();
    // DrawSubItemJustify(cDC, rcItem, m_xWord, 0,
    //     pfld->sContents(), pfnt);
#ifdef NOTYET // 1.5.1fs 
	if ( Shw_papp()->pProject()->m_rgbColorAmbig == -1 ) // 1.5.1fs If font not set, set it to lng fnt
		{
		CFontDef fntdefAmbig( "MS Sans Serif", -10, TRUE ); // 1.5.1fs 
		fntdefAmbig.Set( pfld->plng()->pfnt(), pfld->plng()->rgbColor() ); // 1.5.1fs 
		fntdefAmbig.iCreateFont( &Shw_papp()->pProject()->m_fntAmbig, &Shw_papp()->pProject()->m_rgbColorAmbig ); // 1.5.1fs 
		}
#endif
    DrawSubItemJustify(cDC, rcItem, m_xWord, 0,
        pfld->sContents(), pfld->plng());  // 1998-02-13 MRP
}

void CAmbigListBox::InitListBox()
{
    InitLabels();
    SetItemHeight(iItemHeight());
    UpdateElements( m_pfldlst->pfldFirst() );
}

int CAmbigListBox::iItemHeight()
{
    return Shw_pProject()->iLangCtrlHeight();
}

// --------------------------------------------------------------------------
void CAmbigDlg::DeleteAllUnselected() // Delete all not selected
{
    CField* pfldKeep = m_lboAmbigs.pfldSelected();
    for ( CField* pfld = m_pfldlst->pfldFirst(); pfld; )
        {
        CField* pfldNext = m_pfldlst->pfldNext( pfld ); // Get next before delete
        if ( pfld != pfldKeep ) // If not selected, delete it
            m_pfldlst->Delete( &pfld );
        pfld = pfldNext;    
        }       
    ASSERT( !m_pfldlst->bIsEmpty() );       
}

void CAmbigDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAmbigDlg)
    DDX_Control(pDX, IDC_AMBIG_LIST, m_lboAmbigs);
    DDX_Control(pDX, IDC_wdf_btnFormulas, m_btnFormulas);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAmbigDlg, CDialog)
    //{{AFX_MSG_MAP(CAmbigDlg)
    ON_LBN_DBLCLK(IDC_AMBIG_LIST, OnDblclkAmbigList)
    ON_WM_MEASUREITEM()
    ON_BN_CLICKED(IDC_wdf_btnFormulas, OnFormulas)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

CAmbigDlg::CAmbigDlg( CFieldList* pfldlst ) :
    CDialog(IDD), m_lboAmbigs( pfldlst )
{
    m_pfldlst = pfldlst;
    m_pfldlstExcluded = NULL;
    m_pwdfset = NULL;
}

CAmbigDlg::CAmbigDlg( CFieldList* pfldlst, CFieldList* pfldlstExcluded,
        CWordFormulaSet* pwdfset ) :
    CDialog(IDD), m_lboAmbigs( pfldlst )
{
    m_pfldlst = pfldlst;
    m_pfldlstExcluded = pfldlstExcluded;
    ASSERT( m_pfldlstExcluded );
    m_pwdfset = pwdfset;
}

static CRect s_rectAmbig( 0, 20, 0, 0 );

static void RestoreAmbigDialogPosition(CDialog& dialog)
{
    CWnd* mainWindow = Shw_papp()->m_pMainWnd;
    const CRect& savedPosition = Shw_papp()->pProject()->m_rectAmbig;
    CRect windowRect;
    dialog.GetWindowRect(&windowRect);

    TITLEBARINFO titleBar = { sizeof(TITLEBARINFO) };
    CRect captionRect = windowRect;
    captionRect.bottom = captionRect.top + GetSystemMetrics(SM_CYCAPTION);
    if (::GetTitleBarInfo(dialog.GetSafeHwnd(), &titleBar))
        captionRect = titleBar.rcTitleBar;
    captionRect.OffsetRect(savedPosition.left - windowRect.left,
                           savedPosition.top - windowRect.top);

    // An oversized dialog is still usable if enough of its title bar can be grabbed.
    const int grabWidth = min(captionRect.Width(), 2 * GetSystemMetrics(SM_CXSIZE));
    HMONITOR savedMonitor = MonitorFromRect(&captionRect, MONITOR_DEFAULTTONULL);
    MONITORINFO monitor = { sizeof(MONITORINFO) };
    CRect visibleCaption;
    CPoint position = savedPosition.TopLeft();
    if (!(savedMonitor && GetMonitorInfo(savedMonitor, &monitor) &&
          visibleCaption.IntersectRect(&captionRect, &monitor.rcWork) &&
          visibleCaption.Width() >= grabWidth &&
          visibleCaption.Height() == captionRect.Height()))
        {
        HMONITOR mainMonitor = MonitorFromWindow(mainWindow->GetSafeHwnd(), MONITOR_DEFAULTTONEAREST);
        if (!GetMonitorInfo(mainMonitor, &monitor))
            return;
        const CRect workArea(monitor.rcWork);
        position.x = workArea.left + max(0, (workArea.Width() - windowRect.Width()) / 2);
        position.y = workArea.top + max(0, (workArea.Height() - windowRect.Height()) / 2);
        }

    dialog.SetWindowPos(NULL, position.x, position.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

BOOL CAmbigDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    RestoreAmbigDialogPosition(*this);
    m_lboAmbigs.InitListBox();
    m_btnFormulas.EnableWindow( m_pwdfset && m_pwdfset->bEnabled() );  // 1999-09-04 MRP
    return TRUE;  // input focus will be set to the dialog's first control
}

void CAmbigDlg::OnOK()
{
    // Delete all unselected ambiguities from the list
    DeleteAllUnselected();
    GetWindowRect( &s_rectAmbig ); // Remember the window position
	Shw_papp()->pProject()->m_rectAmbig = s_rectAmbig; // 1.2mj Remember position of ambig box
    CDialog::OnOK();
}

void CAmbigDlg::OnDblclkAmbigList()
{
    OnOK();
}

void CAmbigDlg::OnFormulas()
{
    ASSERT( m_pwdfset );
    ASSERT( m_pwdfset->bEnabled() );
    if ( !m_pwdfset->bModify() )
        return;

    // Recheck the ambiguities using the modified formulas
    m_pwdfset->MatchParses( m_pfldlst, m_pfldlstExcluded, TRUE );
    CField* pfldSelected = m_lboAmbigs.pfldSelected();
    m_lboAmbigs.UpdateElements(pfldSelected);
}

/////////////////////////////////////////////////////////////////////////////
// CLookFailDlg dialog


CLookFailDlg::CLookFailDlg( const char* pszKey, const CLangEnc* plng, CWnd* pParent /*=NULL*/)
    : CDialog(CLookFailDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CLookFailDlg)
	//}}AFX_DATA_INIT
    m_sKey = pszKey;
    m_plng = (CLangEnc*)plng;
}

const char* CLookFailDlg::pszKey() // Return modified key
{
	return m_sKey;
}

BEGIN_MESSAGE_MAP(CLookFailDlg, CDialog)
    //{{AFX_MSG_MAP(CLookFailDlg)
    ON_BN_CLICKED(IDYES, OnYes)
    ON_BN_CLICKED(IDNO, OnNo)
	ON_BN_CLICKED(IDC_WholeWord, OnWholeWord)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CLookFailDlg message handlers

BOOL CLookFailDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

//	SetDlgItemTxt( IDC_LookupFailureKey, m_sKey); // 1.4qpn
	m_elcKey.SetLangEnc(m_plng);
	m_elcKey.SetEditLngText( m_sKey ); // 1.4qpn
    RestoreAmbigDialogPosition(*this);
    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CLookFailDlg::OnYes()
{
	UpdateData(TRUE);  // from dialog controls to member variables
	m_sKey = m_elcKey.sGetEditLngText(); // 1.4qpn

    GetWindowRect( &s_rectAmbig ); // Remember the window position
	Shw_papp()->pProject()->m_rectAmbig = s_rectAmbig; // 1.2mj Remember position of ambig box
    EndDialog(IDYES);
}

void CLookFailDlg::OnNo()
{
    GetWindowRect( &s_rectAmbig ); // Remember the window position
	Shw_papp()->pProject()->m_rectAmbig = s_rectAmbig; // 1.2mj Remember position of ambig box
    EndDialog(IDNO);
}

void CLookFailDlg::OnWholeWord() 
{
	UpdateData(TRUE);  // from dialog controls to member variables
	m_sKey = m_elcKey.sGetEditLngText(); // 1.4qpn

    EndDialog(IDC_WholeWord);
}

void CLookFailDlg::OnCancel()
{
    GetWindowRect( &s_rectAmbig ); // Remember the window position
	Shw_papp()->pProject()->m_rectAmbig = s_rectAmbig; // 1.2mj Remember position of ambig box
    CDialog::OnCancel();
}

void CLookFailDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LookupFailureKey, m_elcKey);
    //{{AFX_DATA_MAP(CLookFailDlg)
//	DDX_Txt(pDX, IDC_LookupFailureKey, m_sKey);
	//}}AFX_DATA_MAP
}


// mdf_d.cpp  Multi-Dictionary Formatter property sheet and pages

#include "stdafx.h"
#include "toolbox.h"
#include "mdf_d.h"
#include "mrfset_d.h"  // CSelectFieldsDlg
#include "mdf.h"  // CExportProcessMDF
#include "pgs.h"  // RtfPageSetup
#include "typ.h"  // CDatabaseType
#include "shw.h"  // sGetDefaultSettingsFolder(), sGetAppPath()


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


// --------------------------------------------------------------------------

// Add button
CMDFDlg::CMDFDlg(CExportProcessSet* pexpset,
		BOOL bWebPages,  // 1998-10-16 MRP
        CExportProcessMDF** ppexpNew) :
    CDialog(CMDFDlg::IDD),
    m_pmdf(new CExportProcessMDF("", pexpset, TRUE, bWebPages)),
    m_ppexpNew(ppexpNew),
    m_pexpset(pexpset),
    m_bEnableName(TRUE),
    m_cboFilter(m_pexpset->ptypMyOwner()->pfilset()),
    m_bCheckConsistency(FALSE),
    m_subsetMarkersToExport(m_pmdf->m_subsetMarkersToExport)
{
    Init();
}

// Copy button
CMDFDlg::CMDFDlg(const CExportProcessMDF* pexpToCopyFrom,
        CExportProcessMDF** ppexpNew) :
    CDialog(CMDFDlg::IDD),
    m_pmdf(new CExportProcessMDF(*pexpToCopyFrom)),
    m_ppexpNew(ppexpNew),
    m_pexpset(pexpToCopyFrom->m_pexpsetMyOwner),
    m_bEnableName(TRUE),
    m_cboFilter(pexpToCopyFrom->m_pexpsetMyOwner->ptypMyOwner()->pfilset()),
    m_bCheckConsistency(pexpToCopyFrom->m_bCheckConsistency),
    m_subsetMarkersToExport(pexpToCopyFrom->m_subsetMarkersToExport)
{
    Init();
}

// Modify button
CMDFDlg::CMDFDlg(CExportProcessMDF* pmdf, BOOL bEnableName) :
    CDialog(CMDFDlg::IDD),
    m_pmdf(pmdf),
    m_ppexpNew(NULL),
    m_pexpset(m_pmdf->m_pexpsetMyOwner),
    m_bEnableName(bEnableName),
    m_sName(m_pmdf->sName()),
    m_cboFilter(m_pmdf->m_pexpsetMyOwner->ptypMyOwner()->pfilset()),
    m_bCheckConsistency(m_pmdf->m_bCheckConsistency),
    m_subsetMarkersToExport(m_pmdf->m_subsetMarkersToExport)
{
    Init();
}

void CMDFDlg::Init()
{
    ASSERT( m_pmdf );

    // 1999-01-25 MRP: Finderlist language is independent of audience
    m_iAudience = m_pmdf->m_iAudience;
    m_iFormat = m_pmdf->m_iFormat;
    m_aasTitle[0][0] = m_pmdf->m_aasTitle[0][0];
    m_aasTitle[0][1] = m_pmdf->m_aasTitle[0][1];
    m_aasTitle[0][2] = m_pmdf->m_aasTitle[0][2];
    m_aasTitle[0][3] = m_pmdf->m_aasTitle[0][3];
    m_aasTitle[0][4] = m_pmdf->m_aasTitle[0][4];
    m_aasTitle[1][0] = m_pmdf->m_aasTitle[1][0];
    m_aasTitle[1][1] = m_pmdf->m_aasTitle[1][1];
    m_aasTitle[1][2] = m_pmdf->m_aasTitle[1][2];
    m_aasTitle[1][3] = m_pmdf->m_aasTitle[1][3];
    m_aasTitle[1][4] = m_pmdf->m_aasTitle[1][4];
    m_pfil = m_pmdf->m_pfil;

    CMarkerSet* pmkrset = m_pmdf->m_pexpsetMyOwner->pmkrset();
    CLangEnc* plngDefault = pmkrset->plngDefault();
    CMarker* pmkr_re = pmkrset->pmkrSearch("re");
    m_plngEnglish = (pmkr_re ? pmkr_re->plng() : plngDefault);
    CMarker* pmkr_rn = pmkrset->pmkrSearch("rn");
    m_plngNational = (pmkr_rn ? pmkr_rn->plng() : plngDefault);

    m_iExportedFileType = (m_pmdf->m_bWebPages ? 1 : 0);  // 1999-02-01 MRP

    m_bSortByCitationForm = m_pmdf->m_bSortByCitationForm;
    m_bIncludeLexemeWithCitationForm = m_pmdf->m_bIncludeLexemeWithCitationForm;
    m_bIncludeExamples = m_pmdf->m_bIncludeExamples;
    m_bIncludeNotes = m_pmdf->m_bIncludeNotes;
    m_bIncludeRegionalWithNational = m_pmdf->m_bIncludeRegionalWithNational;
    m_bIncludeNonMDF = m_pmdf->m_bIncludeNonMDF;
    m_bIncludePartOfSpeech = m_pmdf->m_bIncludePartOfSpeech;
    m_sCopyright = m_pmdf->m_sCopyright;  // 1999-01-25 MRP
    m_bHeader = m_pmdf->m_bHeader;
    m_bDate = m_pmdf->m_bDate;
    m_bTotal = m_pmdf->m_bTotal;
    m_ppgs = new RtfPageSetup(*m_pmdf->m_ppgs);

    m_sEnglishLabelsCct = m_pmdf->m_sEnglishLabelsCct;
    m_sEnglishDot = m_pmdf->m_sEnglishDot;
    m_sNationalLabelsCct = m_pmdf->m_sNationalLabelsCct;
    m_sNationalDot = m_pmdf->m_sNationalDot;
}

CMDFDlg::~CMDFDlg()
{
    delete m_ppgs;
}

void CMDFDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_MDF_edtTitle, m_elcTitle);
    
    //{{AFX_DATA_MAP(CMDFDlg)
    DDX_Control(pDX, IDC_Export_edtName, m_edtName);
    DDX_Control(pDX, IDC_MDF_cboFilter, m_cboFilter);
    DDX_Radio(pDX, IDC_MDF_optDocument, m_iExportedFileType);
    DDX_Check(pDX, IDC_Export_chkCheckConsistency, m_bCheckConsistency);
    //}}AFX_DATA_MAP
//    DDX_Txt(pDX, IDC_MDF_edtChainProcess, m_sChainProcess); // 1.4bh
//    m_pexpset->DDX_Name(pDX, IDC_Export_edtName, m_sName); // 1.4hdf Replace DDX_Name in MDF export proc dlg

    // 1996-08-13 MRP: The order of the following two data exchanges
    // is significant if the title edit box is to be updated correctly
    // when the selection in the list box changes.
//    DDX_Txt(pDX, IDC_MDF_edtTitle, m_aasTitle[m_iAudience][m_iFormat]);
    DDX_Control(pDX, IDC_MDF_optEnglish, m_optEnglish);
    DDX_Radio(pDX, IDC_MDF_optEnglish, m_iAudience);
    DDX_Radio(pDX, IDC_MDF_optDiglot, m_iFormat);
    if ( pDX->m_bSaveAndValidate )
        m_pfil = m_cboFilter.pfilSelected();
    else
        m_cboFilter.UpdateElements(m_pfil);
}

BOOL CMDFDlg::OnInitDialog()
{
    BOOL bRet = CDialog::OnInitDialog();
    SetDlgItemText( IDC_Export_edtName, swUTF16( m_sName ) ); // 1.4hdf // 1.4qpv

    m_elcTitle.SetLangEnc(m_iAudience == 0 ?
        m_plngEnglish : m_plngNational);
	m_elcTitle.SetEditLngText( m_aasTitle[m_iAudience][m_iFormat] ); // 1.4qpn

    m_edtName.EnableWindow(m_bEnableName);
    if ( !m_ppexpNew )
        {
        m_optEnglish.SetFocus();
        bRet = FALSE;
        }
    
    return bRet;
}

BEGIN_MESSAGE_MAP(CMDFDlg, CDialog)
    //{{AFX_MSG_MAP(CMDFDlg)
    ON_BN_CLICKED(IDC_MDF_optEnglish, OnAudienceOrFormatChanged)
    ON_BN_CLICKED(IDC_MDF_optNational, OnAudienceOrFormatChanged)
    ON_BN_CLICKED(IDC_MDF_optDiglot, OnAudienceOrFormatChanged)
    ON_BN_CLICKED(IDC_MDF_optTriglot, OnAudienceOrFormatChanged)
    ON_BN_CLICKED(IDC_MDF_optEnglishGlossIndex, OnAudienceOrFormatChanged)
    ON_BN_CLICKED(IDC_MDF_optNationalGlossIndex, OnAudienceOrFormatChanged)
    ON_BN_CLICKED(IDC_MDF_optRegionalGlossIndex, OnAudienceOrFormatChanged)
    ON_BN_CLICKED(IDC_MDF_btnAudienceSetup, OnAudienceSetup)
    ON_BN_CLICKED(IDC_MDF_btnOptions, OnOptions)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CMDFDlg::OnOK()
{
    UpdateData(TRUE);  // from dialog controls to dialog object member data
//    GetDlgItemText( IDC_Export_edtName, (char*)buffer, DLGBUFMAX ); // 1.4hdf
	CString sDlgItem; // 1.4qpw
	GetDlgItemText( IDC_Export_edtName, sDlgItem ); // 1.4qpw
	m_sName = sUTF8( sDlgItem ); // 1.4qpw
	TrimBothEnds( m_sName ); // 1.4hdf
	m_aasTitle[m_iAudience][m_iFormat] = m_elcTitle.sGetEditLngText(); // 1.4qpn

    Str8 sMessage;
    if ( !m_pexpset->bValidNameDlg(m_sName, m_pmdf, sMessage) )
        {
        MessageBeep(0);
        AfxMessageBox(sMessage);
        m_edtName.SetFocus();
        m_edtName.SetSel(0, -1);  // Select the entire name
        return;
        }

    if ( m_ppexpNew )
        {
        m_pexpset->Add(m_pmdf);
        *m_ppexpNew = m_pmdf;
        }
    
    if ( strcmp(m_sName, m_pmdf->sName()) != 0 )
        m_pmdf->ChangeNameTo(m_sName);

    // 1999-01-25 MRP: Finderlist language is independent of audience
    m_pmdf->m_iAudience = m_iAudience;
    m_pmdf->m_iFormat = m_iFormat;
    m_pmdf->m_aasTitle[0][0] = m_aasTitle[0][0];
    m_pmdf->m_aasTitle[0][1] = m_aasTitle[0][1];
    m_pmdf->m_aasTitle[0][2] = m_aasTitle[0][2];
    m_pmdf->m_aasTitle[0][3] = m_aasTitle[0][3];
    m_pmdf->m_aasTitle[0][4] = m_aasTitle[0][4];
    m_pmdf->m_aasTitle[1][0] = m_aasTitle[1][0];
    m_pmdf->m_aasTitle[1][1] = m_aasTitle[1][1];
    m_pmdf->m_aasTitle[1][2] = m_aasTitle[1][2];
    m_pmdf->m_aasTitle[1][3] = m_aasTitle[1][3];
    m_pmdf->m_aasTitle[1][4] = m_aasTitle[1][4];

    m_pmdf->m_pfil = m_pfil;
    
    m_pmdf->m_bWebPages = (m_iExportedFileType == 1);  // 1999-02-01 MRP
    m_pmdf->m_bCheckConsistency = m_bCheckConsistency;  // 1999-06-14 MRP
    
    m_pmdf->m_bSortByCitationForm = m_bSortByCitationForm;
    m_pmdf->m_bIncludeLexemeWithCitationForm = m_bIncludeLexemeWithCitationForm;
    m_pmdf->m_bIncludeExamples = m_bIncludeExamples;
    m_pmdf->m_bIncludeNotes = m_bIncludeNotes;
    m_pmdf->m_bIncludeNonMDF = m_bIncludeNonMDF;
    m_pmdf->m_bIncludeRegionalWithNational = m_bIncludeRegionalWithNational;
    m_pmdf->m_bIncludeNonMDF = m_bIncludeNonMDF;
    m_pmdf->m_bIncludePartOfSpeech = m_bIncludePartOfSpeech;
    m_pmdf->m_sCopyright = m_sCopyright;  // 1999-01-25 MRP
    m_pmdf->m_bHeader = m_bHeader;
    m_pmdf->m_bDate = m_bDate;
    m_pmdf->m_bTotal = m_bTotal;
    m_pmdf->m_subsetMarkersToExport = m_subsetMarkersToExport;
    *m_pmdf->m_ppgs = *m_ppgs;

    m_pmdf->m_sEnglishLabelsCct = m_sEnglishLabelsCct;
    m_pmdf->m_sEnglishDot = m_sEnglishDot;
    m_pmdf->m_sNationalLabelsCct = m_sNationalLabelsCct;
    m_pmdf->m_sNationalDot = m_sNationalDot;

    CDialog::OnOK();
}

void CMDFDlg::OnCancel()
{
    if ( m_ppexpNew )
        delete m_pmdf;

    CDialog::OnCancel();
}

void CMDFDlg::OnAudienceSetup()
{
    CMDFAudienceSetupDlg dlg(this);
    dlg.DoModal();
}

void CMDFDlg::OnOptions()
{
    CMDFOptionsDlg dlg(this);
    dlg.DoModal();
}

void CMDFDlg::OnAudienceOrFormatChanged()
{
	m_aasTitle[m_iAudience][m_iFormat] = m_elcTitle.sGetEditLngText(); // 1.4qpn
    UpdateData(TRUE);  // from dialog controls to dialog object member data // 1.4dx Fix bug of MDF box not refreshing footers correctly on change audience
    UpdateData(FALSE);  // from dialog object member data to dialog controls // 1.4kub Verify that this one is needed
	m_elcTitle.SetEditLngText( m_aasTitle[m_iAudience][m_iFormat] ); // 1.4qpn

	m_elcTitle.SetLangEnc(m_iAudience == 0 ? m_plngEnglish : m_plngNational);
	m_elcTitle.Invalidate(); // 1.4qpn
}

// --------------------------------------------------------------------------

CMDFOptionsDlg::CMDFOptionsDlg(CMDFDlg* pdlg) :
    CDialog(CMDFOptionsDlg::IDD),
    m_pdlg(pdlg),
    m_sChainProcess(m_pdlg->m_pmdf->m_sChainProcess), // 1.4kk Process to chain to
    m_subsetMarkersToExport(m_pdlg->m_subsetMarkersToExport)
{
    ASSERT( m_pdlg );

    Str8 sMarkers = m_pdlg->m_pmdf->sHierarchyMarkers();
	m_sMarkerHierarchy = "Marker hierarchy:";
	m_sMarkerHierarchy = m_sMarkerHierarchy + " " + sMarkers; // 1.5.0fg 

    //{{AFX_DATA_INIT(CMDFOptionsDlg)
    m_bSortByCitationForm = m_pdlg->m_bSortByCitationForm;
    m_bIncludeLexemeWithCitationForm = m_pdlg->m_bIncludeLexemeWithCitationForm;
    m_bIncludeExamples = m_pdlg->m_bIncludeExamples;
    m_bIncludeNotes = m_pdlg->m_bIncludeNotes;
    m_bIncludeRegionalWithNational = m_pdlg->m_bIncludeRegionalWithNational;
    m_bIncludeNonMDF = m_pdlg->m_bIncludeNonMDF;
    m_bIncludePartOfSpeech = m_pdlg->m_bIncludePartOfSpeech;
    m_sCopyright = m_pdlg->m_sCopyright;  // 1999-01-25 MRP
    m_bHeader = m_pdlg->m_bHeader;
    m_bDate = m_pdlg->m_bDate;
    m_bTotal = m_pdlg->m_bTotal;
    //}}AFX_DATA_INIT

    m_ppgs = new RtfPageSetup(*m_pdlg->m_ppgs);
}

CMDFOptionsDlg::~CMDFOptionsDlg()
{
    delete m_ppgs;
}

void CMDFOptionsDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CMDFOptionsDlg)
    DDX_Check(pDX, IDC_MDF_chkSortByCitationForm, m_bSortByCitationForm);
    DDX_Check(pDX, IDC_MDF_chkIncludeLexemeWithCitationForm, m_bIncludeLexemeWithCitationForm);
    DDX_Check(pDX, IDC_MDF_chkIncludeExamples, m_bIncludeExamples);
    DDX_Check(pDX, IDC_MDF_chkIncludeNotes, m_bIncludeNotes);
    DDX_Check(pDX, IDC_MDF_chkIncludeRegionalWithNational, m_bIncludeRegionalWithNational);
    DDX_Check(pDX, IDC_MDF_chkIncludeNonMDF, m_bIncludeNonMDF);
    DDX_Check(pDX, IDC_MDF_chkIncludePartOfSpeech, m_bIncludePartOfSpeech);
    // 1999-01-25 MRP: Added copyright and moved hierarchy from MDF dialog box.
//    DDX_Txt(pDX, IDC_MDF_edtCopyright, m_sCopyright);
//    DDX_Txt(pDX, IDC_MDF_lblMarkerHierarchy, m_sMarkerHierarchy);
    DDX_Check(pDX, IDC_MDF_chkHeader, m_bHeader);
    DDX_Check(pDX, IDC_MDF_chkDate, m_bDate);
    DDX_Check(pDX, IDC_MDF_chkTotal, m_bTotal);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMDFOptionsDlg, CDialog)
    //{{AFX_MSG_MAP(CMDFOptions)
    ON_BN_CLICKED(IDC_MDF_btnSelectFields, OnSelectFields)
    ON_BN_CLICKED(IDC_MDF_btnPageSetup, OnPageSetup)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CMDFOptionsDlg::OnInitDialog()
{
    BOOL bRet = CDialog::OnInitDialog();
    SetDlgItemText( IDC_MDF_edtCopyright, swUTF16( m_sCopyright ) ); // 1.4qpv
    SetDlgItemText( IDC_MDF_lblMarkerHierarchy, swUTF16( m_sMarkerHierarchy ) ); // 1.4qpv
    SetDlgItemText( IDC_MDF_edtChainProcess, swUTF16( m_sChainProcess ) ); // 1.4kk // 1.4qpv

    return bRet;
}


void CMDFOptionsDlg::OnOK()
{
    UpdateData(TRUE);  // from dialog controls to dialog object member data
//    GetDlgItemText( IDC_MDF_edtCopyright, (char*)buffer, DLGBUFMAX );
	CString sDlgItem; // 1.4qpw
	GetDlgItemText( IDC_MDF_edtCopyright, sDlgItem ); // 1.4qpw
	m_sCopyright = sUTF8( sDlgItem ); // 1.4qpw
//    GetDlgItemText( IDC_MDF_lblMarkerHierarchy, (char*)buffer, DLGBUFMAX );
	GetDlgItemText( IDC_MDF_lblMarkerHierarchy, sDlgItem ); // 1.4qpw
	m_sMarkerHierarchy = sUTF8( sDlgItem ); // 1.4qpw
//    GetDlgItemText( IDC_MDF_edtChainProcess, (char*)buffer, DLGBUFMAX );
	GetDlgItemText( IDC_MDF_edtChainProcess, sDlgItem ); // 1.4qpw
	m_sChainProcess = sUTF8( sDlgItem ); // 1.4qpw

    m_pdlg->m_bSortByCitationForm = m_bSortByCitationForm;
    m_pdlg->m_bIncludeLexemeWithCitationForm = m_bIncludeLexemeWithCitationForm;
    m_pdlg->m_bIncludeExamples = m_bIncludeExamples;
    m_pdlg->m_bIncludeNotes = m_bIncludeNotes;
    m_pdlg->m_bIncludeRegionalWithNational = m_bIncludeRegionalWithNational;
    m_pdlg->m_bIncludeNonMDF = m_bIncludeNonMDF;
    m_pdlg->m_bIncludePartOfSpeech = m_bIncludePartOfSpeech;
    m_pdlg->m_sCopyright = m_sCopyright;  // 1999-01-25 MRP
    m_pdlg->m_subsetMarkersToExport = m_subsetMarkersToExport;
    m_pdlg->m_bHeader = m_bHeader;
    m_pdlg->m_bDate = m_bDate;
    m_pdlg->m_bTotal = m_bTotal;
    *m_pdlg->m_ppgs = *m_ppgs;

    m_pdlg->m_pmdf->m_sChainProcess = m_sChainProcess; // 1.4kk

    CDialog::OnOK();
}

void CMDFOptionsDlg::OnSelectFields()
{
    CSelectFieldsDlg dlg(m_subsetMarkersToExport, FALSE ); // , IDSx_SelectMDFExcludeFields); // 1.4gb
    dlg.DoModal();
}

void CMDFOptionsDlg::OnPageSetup()
{
    m_ppgs->bViewProperties();
}


// --------------------------------------------------------------------------

CMDFAudienceSetupDlg::CMDFAudienceSetupDlg(CMDFDlg* pdlg) :
    CDialog(CMDFAudienceSetupDlg::IDD),
    m_pdlg(pdlg)
{
    ASSERT( m_pdlg );

    //{{AFX_DATA_INIT(CMDFAudienceSetupDlg)
    m_sEnglishLabelsCct = m_pdlg->m_sEnglishLabelsCct;
    m_sEnglishDot = m_pdlg->m_sEnglishDot;
    m_sNationalLabelsCct = m_pdlg->m_sNationalLabelsCct;
    m_sNationalDot = m_pdlg->m_sNationalDot;
    //}}AFX_DATA_INIT
}

CMDFAudienceSetupDlg::~CMDFAudienceSetupDlg()
{
}

void CMDFAudienceSetupDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CMDFAudienceSetupDlg)
//    DDX_Txt(pDX, IDC_MDF_edtEnglishLabelsCct, m_sEnglishLabelsCct);
//    DDX_Txt(pDX, IDC_MDF_edtEnglishDot, m_sEnglishDot);
//    DDX_Txt(pDX, IDC_MDF_edtNationalLabelsCct, m_sNationalLabelsCct);
//    DDX_Txt(pDX, IDC_MDF_edtNationalDot, m_sNationalDot);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMDFAudienceSetupDlg, CDialog)
    //{{AFX_MSG_MAP(CMDFAudienceOptions)
    ON_BN_CLICKED(IDC_MDF_btnBrowseEnglishLabelsCct, OnBrowseEnglishLabelsCct)
    ON_BN_CLICKED(IDC_MDF_btnBrowseEnglishDot, OnBrowseEnglishDot)
    ON_BN_CLICKED(IDC_MDF_btnBrowseNationalLabelsCct, OnBrowseNationalLabelsCct)
    ON_BN_CLICKED(IDC_MDF_btnBrowseNationalDot, OnBrowseNationalDot)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CMDFAudienceSetupDlg::OnInitDialog()
{
    BOOL bRet = CDialog::OnInitDialog();
    SetDlgItemText( IDC_MDF_edtEnglishLabelsCct, swUTF16( m_sEnglishLabelsCct ) ); // 1.4qpv
    SetDlgItemText( IDC_MDF_edtEnglishDot, swUTF16( m_sEnglishDot ) ); // 1.4qpv
    SetDlgItemText( IDC_MDF_edtNationalLabelsCct, swUTF16( m_sNationalLabelsCct ) ); // 1.4qpv
    SetDlgItemText( IDC_MDF_edtNationalDot, swUTF16( m_sNationalDot ) ); // 1.4qpv

    return bRet;
}


void CMDFAudienceSetupDlg::OnOK()
{
    UpdateData(TRUE);  // from dialog controls to dialog object member data
//    GetDlgItemText( IDC_MDF_edtEnglishLabelsCct, (char*)buffer, DLGBUFMAX );
	CString sDlgItem; // 1.4qpw
	GetDlgItemText( IDC_MDF_edtEnglishLabelsCct, sDlgItem ); // 1.4qpw
	m_sEnglishLabelsCct = sUTF8( sDlgItem ); // 1.4qpw
//    GetDlgItemText( IDC_MDF_edtEnglishDot, (char*)buffer, DLGBUFMAX );
	GetDlgItemText( IDC_MDF_edtEnglishDot, sDlgItem ); // 1.4qpw
	m_sEnglishDot = sUTF8( sDlgItem ); // 1.4qpw
//    GetDlgItemText( IDC_MDF_edtNationalLabelsCct, (char*)buffer, DLGBUFMAX );
	GetDlgItemText( IDC_MDF_edtNationalLabelsCct, sDlgItem ); // 1.4qpw
	m_sNationalLabelsCct = sUTF8( sDlgItem ); // 1.4qpw
//    GetDlgItemText( IDC_MDF_edtNationalDot, (char*)buffer, DLGBUFMAX );
	GetDlgItemText( IDC_MDF_edtNationalDot, sDlgItem ); // 1.4qpw
	m_sNationalDot = sUTF8( sDlgItem ); // 1.4qpw

    m_pdlg->m_sEnglishLabelsCct = m_sEnglishLabelsCct;
    m_pdlg->m_sEnglishDot = m_sEnglishDot;
    m_pdlg->m_sNationalLabelsCct = m_sNationalLabelsCct;
    m_pdlg->m_sNationalDot = m_sNationalDot;

    CDialog::OnOK();
}


void CMDFAudienceSetupDlg::OnBrowseEnglishLabelsCct()
{
    UpdateData(TRUE);  // from dialog controls to object members
	Str8 sFileFilter = _("Change Table"); // 1.5.0fz 
	sFileFilter += " (*.cct)|*.cct||"; // 1.5.0fz 
    Str8 sProjectPath = m_pdlg->pmdf()->sSettingsFolder();  // 1999-03-10 MRP

    if ( CExportProcess::s_bBrowseForFile(_("Change Table for English Audience Labels"), NULL, sFileFilter, sProjectPath, m_sEnglishLabelsCct, IDD) )
        {
	    SetDlgItemText( IDC_MDF_edtEnglishLabelsCct, swUTF16( m_sEnglishLabelsCct ) ); // 1.4qzmg Fix U bug of not updating MDF export template from browse button
//        UpdateData(FALSE);  // from dialog object members to controls // 1.4qzmg
        }
}

static const char* s_pszDOT = "dot";  // 1998-10-14 MRP

void CMDFAudienceSetupDlg::OnBrowseEnglishDot()
{
    UpdateData(TRUE);  // from dialog controls to object members
    const char* pszDefaultExt = s_pszDOT;
    Str8 sFileFilter = _("Document Templates"); // 1.5.0fz 
	sFileFilter = sFileFilter + " (*.dot)|*.dot||";
    Str8 sDirPath = sGetDirPath(m_sEnglishDot);
    if ( sDirPath.IsEmpty() )
        sDirPath = m_pdlg->pmdf()->sSettingsFolder();  // 1999-03-10 MRP

    if ( CExportProcess::s_bBrowseForFile(_("Document Template for English Audience"), pszDefaultExt, sFileFilter, sDirPath, m_sEnglishDot, IDD) )
	    SetDlgItemText( IDC_MDF_edtEnglishDot, swUTF16( m_sEnglishDot ) ); // 1.4qzmg Fix U bug of not updating MDF export template from browse button
//        UpdateData(FALSE);  // from dialog object members to controls // 1.4qzmg
}

void CMDFAudienceSetupDlg::OnBrowseNationalLabelsCct()
{
    UpdateData(TRUE);  // from dialog controls to object members
	Str8 sFileFilter = _("Change Table"); // 1.5.0fz 
	sFileFilter += " (*.cct)|*.cct||"; // 1.5.0fz 
    Str8 sProjectPath = m_pdlg->pmdf()->sSettingsFolder();  // 1999-03-10 MRP

    if ( CExportProcess::s_bBrowseForFile(_("Change Table for National Audience Labels"), NULL, sFileFilter, sProjectPath, m_sNationalLabelsCct, IDD) )
        {
	    SetDlgItemText( IDC_MDF_edtNationalLabelsCct, swUTF16( m_sNationalLabelsCct ) ); // 1.4qzmg Fix U bug of not updating MDF export template from browse button
        UpdateData(FALSE);  // from dialog object members to controls // 1.4qzmg
        }
}

void CMDFAudienceSetupDlg::OnBrowseNationalDot()
{
    UpdateData(TRUE);  // from dialog controls to object members
    const char* pszDefaultExt = s_pszDOT;
    Str8 sFileFilter = _("Document Templates"); // 1.5.0fz 
	sFileFilter = sFileFilter + " (*.dot)|*.dot||";
    Str8 sDirPath = sGetDirPath(m_sNationalDot);
    if ( sDirPath.IsEmpty() )
        sDirPath = m_pdlg->pmdf()->sSettingsFolder();  // 1999-03-10 MRP

    if ( CExportProcess::s_bBrowseForFile(_("Document Template for National Audience"), pszDefaultExt, sFileFilter, sDirPath, m_sNationalDot, IDD) )
	    SetDlgItemText( IDC_MDF_edtNationalDot, swUTF16( m_sNationalDot ) ); // 1.4qzmg Fix U bug of not updating MDF export template from browse button
//        UpdateData(FALSE);  // from dialog object members to controls // 1.4qzmg
}


// --------------------------------------------------------------------------

CMDFReminderDlg::CMDFReminderDlg(CExportProcessSet* pexpset) :
    CDialog(CMDFReminderDlg::IDD),
    m_pexpset(pexpset)
{
    ASSERT( m_pexpset );
    ASSERT( !m_pexpset->bNoMDFReminder() );

    //{{AFX_DATA_INIT(CMDFReminderDlg)
    m_bNoMDFReminder = m_pexpset->bNoMDFReminder();
    //}}AFX_DATA_INIT
}

CMDFReminderDlg::~CMDFReminderDlg()
{
}

void CMDFReminderDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CMDFReminderDlg)
    DDX_Control(pDX, IDC_MDF_lblToPrint, m_lblToPrint);
    DDX_Check(pDX, IDC_MDF_chkNoMDFReminder, m_bNoMDFReminder);
    //}}AFX_DATA_MAP
}


BOOL CMDFReminderDlg::OnInitDialog()
{
    BOOL bRet = CDialog::OnInitDialog();

    if ( m_pexpset->bAutoOpen() )
        {
        Str8 sToPrintSwitchToWord;
        sToPrintSwitchToWord = _("To complete the formatting process in Word, choose Finish MDF Export.");
        m_lblToPrint.SetWindowText( swUTF16( sToPrintSwitchToWord) ); // 1.4qpy
        }

    return bRet;
}


void CMDFReminderDlg::OnOK()
{
    CDialog::OnOK();
    m_pexpset->SetNoMDFReminder(m_bNoMDFReminder);
}

void CMDFReminderDlg::OnCancel()
{
    UpdateData(TRUE);  // from dialog controls to dialog object member data
    m_pexpset->SetNoMDFReminder(m_bNoMDFReminder);
    CDialog::OnCancel();
}


// --------------------------------------------------------------------------

CMDFUpdateOlderDlg::CMDFUpdateOlderDlg(CExportProcessSet* pexpset) :
    CDialog(IDD),
    m_pexpset(pexpset)
{
    ASSERT( m_pexpset );
    extern Str8 g_sVersion;  // shw.cpp
    m_sShoeboxVersion = g_sVersion;
    m_sProjectSettings = m_pexpset->sSettingsFolder();  // 1999-03-11 MRP

#ifdef _MAC
    // On the Macintosh, the Shoebox installer doesn't write information
    // to Shoebox preferences.
    m_sDefaultSettings = "My Shoebox Settings";

    // 1999-03-12 MRP: Need to find a way to determine the default drive.
    // For now, disregard it.
    int len = m_sDefaultSettings.GetLength();
    if ( m_sProjectSettings.GetLength() < len )
        len = m_sProjectSettings.GetLength();
    Str8 sProjectFolder = m_sProjectSettings.Right(len);
    BOOL bDifferent = ( _stricmp(sProjectFolder, m_sDefaultSettings) != 0 );
#else
    // In Windows, Shoebox Setup writes this information to Shoebox.ini.
    m_sDefaultSettings = ::sGetDefaultSettingsFolder();
    Str8 sAppPath = ::sGetAppPath();
    m_sNewerMDF = ::sPath(sAppPath, "MDF");

    // 1999-03-09 MRP: Compare settings folders and check for customized files.
    BOOL bDifferent = ( _stricmp(m_sProjectSettings, m_sDefaultSettings) != 0 );
#endif  // _MAC

    m_bShow = ( bDifferent /* || customized change tables??? */ );
}

void CMDFUpdateOlderDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    
    //{{AFX_DATA_MAP(CMDFUpdateOlderDlg)
//    DDX_Txt(pDX, IDC_MDF_lblShoeboxVersion, m_sShoeboxVersion);
//    DDX_Txt(pDX, IDC_MDF_lblProjectSettings, m_sProjectSettings);
//    DDX_Txt(pDX, IDC_MDF_lblDefaultSettings, m_sDefaultSettings);
//    DDX_Txt(pDX, IDC_MDF_lblNewerMDF, m_sNewerMDF);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMDFUpdateOlderDlg, CDialog)
    //{{AFX_MSG_MAP(CMDFUpdateOlderDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CMDFUpdateOlderDlg::OnInitDialog()
{
    BOOL b = CDialog::OnInitDialog();
    SetDlgItemText( IDC_MDF_lblShoeboxVersion, swUTF16( m_sShoeboxVersion ) ); // 1.4qpv
    SetDlgItemText( IDC_MDF_lblProjectSettings, swUTF16( m_sProjectSettings ) ); // 1.4qpv
    SetDlgItemText( IDC_MDF_lblDefaultSettings, swUTF16( m_sDefaultSettings ) ); // 1.4qpv
    SetDlgItemText( IDC_MDF_lblNewerMDF, swUTF16( m_sNewerMDF ) ); // 1.4qpv
    
    return b;
}

// **************************************************************************

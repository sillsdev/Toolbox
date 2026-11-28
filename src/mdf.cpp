// mdf.cpp  Multi-Dictionary Formatter

#include "stdafx.h"
#include "toolbox.h"
#include "mdf.h"
#include "ind.h"  // CIndexSet, CIndex, etc.
#include "typ.h"  // CDatabaseType
#include "obstream.h"  // Object_istream, Object_ostream
#if UseCct
#include "Change_ostream.h" // For change table stream classes
#include "Change_istream.h" 
#endif
#include "sfstream.h"  // SF_istream, SF_ostream
#include "rtf.h"  // RTF_ostream
#include "pgs.h"  // RtfPageSetup
#include "html.h"  // HTML_ostream
#include "progress.h"
#include "date.h"  // CDateCon::s_DateStamp for initializing copyright
#include <fstream>  // ifstream, ofstream
#include <io.h>  // _tell
#include <cderr.h>  // FNERR_INVALIDFILENAME
#include <afxpriv.h>  // HID_BASE_PROMPT

// 1998-10-26 MRP: See also HTML.cpp
#define HTML_LINK_SORT
#ifdef HTML_LINK_SORT
#include <ctype.h>  // __isascii, isprint
#endif

#include "strstream_d255.h"

// #define abmdf2011  // 1.5.9b 

#include "mdf_d.h"  // CMDFDlg

extern BOOL bDebugKaren; // 1.4bk Allow Ctrl+Alt+Shift+K to keep chained export files // 1.4qzpd Change to Ctrl+Shift+K

const char* CExportProcessMDF::s_pszVersion = "4.1";

// Change the version number only when there is a change to
// MDF's settings files (CCTs, DOTs, TYPs) and processing logic
// that's incompatible with earlier versions of Shoebox.

// 3.09 October 1997
//      * Allow \lx and \lc to have different language encodings,
//        e.g. when \lc is a romanized transliteration of the lexeme
// 4.0  December 1997
//      * changed special symbols notation to |{tab} etc.
//      * allow "sub-bundles" under \lf or \pdl when value would be repeated
//      * named Export Processes and AutoUpdateStyles


CExportProcessMDF::CExportProcessMDF(const char* pszName,
        CExportProcessSet* pexpsetMyOwner, BOOL bInitDefaultProperties,
        BOOL bWebPages) :
    CExportProcess(pszName, pexpsetMyOwner)
{
    m_bWebPages = bWebPages;  // 1998-09-16 MRP
    m_bHTML_XML = FALSE;  // 1998-12-05 MRP
    m_bUTF8 = pexpsetMyOwner->ptypMyOwner()->bUnicode(); // 1.2kf Fix bug of MDF HTML not outputting Unicode correctly

    m_iAudience = 0;
    m_iFormat = 0;
    
    m_sEnglishLabelsCct = "mdf_eng.cct";
    m_sNationalLabelsCct = "mdf_inz.cct";
	m_sEnglishDot = "mdf_e.dot";
	m_sNationalDot = "mdf_n.dot";

    m_pfil = NULL;

    m_bSortByCitationForm = TRUE;
    m_bIncludeLexemeWithCitationForm = FALSE;
    m_bIncludeExamples = TRUE;
    m_bIncludeNotes = TRUE;
    m_bIncludeRegionalWithNational = FALSE;
    m_bIncludeNonMDF = FALSE;
    m_bIncludePartOfSpeech = TRUE;  
    m_bDoubleColumn = TRUE;
    m_bHeader = TRUE;
    m_bDate = TRUE;
    m_bTotal = TRUE;
    
    m_ppgs = NULL;

    // 1998-12-03 MRP: User-defined index
    m_bUserDefined = FALSE;
    m_pmkrIndexSelect = NULL;
    m_pmkrIndexPriKey = NULL;
    m_bIndexingCategories = FALSE;

    if ( bInitDefaultProperties )
        InitDefaultProperties();
}

CExportProcessMDF::CExportProcessMDF(const CExportProcessMDF& exp) :
    CExportProcess(exp.sName(), exp.m_pexpsetMyOwner)
{
    m_bCheckConsistency = exp.m_bCheckConsistency;  // 1999-06-14 MRP

    m_bWebPages = exp.m_bWebPages;  // 1998-09-16 MRP
    m_bHTML_XML = exp.m_bHTML_XML;  // 1998-12-05 MRP
    m_bUTF8 = exp.m_bUTF8;  // 1999-02-16 MRP
    m_sCopyright = exp.m_sCopyright;  // 1998-12-05 MRP
    
    m_iAudience = exp.m_iAudience;
    m_iFormat = exp.m_iFormat;

    // 1999-01-25 MRP: Finderlist language is independent of audience
    m_aasTitle[0][0] = exp.m_aasTitle[0][0];
    m_aasTitle[0][1] = exp.m_aasTitle[0][1];
    m_aasTitle[0][2] = exp.m_aasTitle[0][2];
    m_aasTitle[0][3] = exp.m_aasTitle[0][3];
    m_aasTitle[0][4] = exp.m_aasTitle[0][4];
    m_aasTitle[1][0] = exp.m_aasTitle[1][0];
    m_aasTitle[1][1] = exp.m_aasTitle[1][1];
    m_aasTitle[1][2] = exp.m_aasTitle[1][2];
    m_aasTitle[1][3] = exp.m_aasTitle[1][3];
    m_aasTitle[1][4] = exp.m_aasTitle[1][4];

    m_aasExportedRtf[0][0] = exp.m_aasExportedRtf[0][0];
    m_aasExportedRtf[0][1] = exp.m_aasExportedRtf[0][1];
    m_aasExportedRtf[0][2] = exp.m_aasExportedRtf[0][2];
    m_aasExportedRtf[0][3] = exp.m_aasExportedRtf[0][3];
    m_aasExportedRtf[0][4] = exp.m_aasExportedRtf[0][4];
    m_aasExportedRtf[1][0] = exp.m_aasExportedRtf[1][0];
    m_aasExportedRtf[1][1] = exp.m_aasExportedRtf[1][1];
    m_aasExportedRtf[1][2] = exp.m_aasExportedRtf[1][2];
    m_aasExportedRtf[1][3] = exp.m_aasExportedRtf[1][3];
    m_aasExportedRtf[1][4] = exp.m_aasExportedRtf[1][4];

    m_asExportedHtm[0] = exp.m_asExportedHtm[0];  // 1998-10-16 MRP
    m_asExportedHtm[1] = exp.m_asExportedHtm[1];

    m_sEnglishLabelsCct = exp.m_sEnglishLabelsCct;
    m_sEnglishDot = exp.m_sEnglishDot;
    m_sNationalLabelsCct = exp.m_sNationalLabelsCct;
    m_sNationalDot = exp.m_sNationalDot;

    m_pfil = exp.m_pfil;

    m_bSortByCitationForm = exp.m_bSortByCitationForm;
    m_bIncludeLexemeWithCitationForm = exp.m_bIncludeLexemeWithCitationForm;
    m_bIncludeExamples = exp.m_bIncludeExamples;
    m_bIncludeNotes = exp.m_bIncludeNotes;
    m_bIncludeRegionalWithNational = exp.m_bIncludeRegionalWithNational;
    m_bIncludeNonMDF = exp.m_bIncludeNonMDF;
    m_bIncludePartOfSpeech = exp.m_bIncludePartOfSpeech;
    m_bHeader = exp.m_bHeader;
    m_bDate = exp.m_bDate;
    m_bTotal = exp.m_bTotal;
    
    m_sDatabaseTypeRTF = exp.m_sDatabaseTypeRTF;
    m_sDatabaseTypeHTML = exp.m_sDatabaseTypeHTML;  // 1998-10-16 MRP
    
    m_ppgs = new RtfPageSetup(*exp.m_ppgs);

    // 1999-03-10 MRP: Customized versions of the change tables.
    m_sFileNameDict1 = exp.m_sFileNameDict1;
    m_sFileNameDict2 = exp.m_sFileNameDict2;
    m_sFileNameDictH = exp.m_sFileNameDictH;
    m_sFileNameLinkH = exp.m_sFileNameLinkH;
    m_sFileNameList1 = exp.m_sFileNameList1;
    m_sFileNameList2 = exp.m_sFileNameList2;
    m_sFileNameListH = exp.m_sFileNameListH;

    // 1998-12-03 MRP: User-defined index
    m_bUserDefined = exp.m_bUserDefined;
    m_pmkrIndexSelect = exp.m_pmkrIndexSelect;
    m_sOption = exp.m_sOption;
    m_sFileNameIndex1 = exp.m_sFileNameIndex1;
    m_pmkrIndexPriKey = exp.m_pmkrIndexPriKey;
    m_mrflstIndexSecKeys = exp.m_mrflstIndexSecKeys;
    m_bIndexingCategories = exp.m_bIndexingCategories;
    m_sFileNameIndex2 = exp.m_sFileNameIndex2;
    m_sFileNameIndexH = exp.m_sFileNameIndexH;
}

CExportProcessMDF::~CExportProcessMDF()
{
    delete m_ppgs;
}


static const char* s_pszDefaultNameMDF = "Multi-Dictionary Formatter";

static const char* psz_MDF = "mdf";
static const char* psz_expMDF = "expMDF";  // 1997-11-18 MRP

static const char* psz_National = "National";
static const char* psz_Triglot = "Triglot";
// 1999-01-25 MRP: Finderlist language independent of audience
static const char* psz_Finderlist = "Finderlist";
static const char* psz_EnglishGlossIndex = "EnglishGlossIndex";
static const char* psz_NationalGlossIndex = "NationalGlossIndex";
static const char* psz_RegionalGlossIndex = "RegionalGlossIndex";
static const char* psz_EnglishDiglot = "titleEnglishDiglot";
static const char* psz_EnglishTriglot = "titleEnglishTriglot";
static const char* psz_EnglishFinderlist = "titleEnglishFinderlist";
static const char* psz_GlossIndexEE = "titleGlossIndexEE";
static const char* psz_GlossIndexEN = "titleGlossIndexEN";
static const char* psz_GlossIndexER = "titleGlossIndexER";
static const char* psz_NationalDiglot = "titleNationalDiglot";
static const char* psz_NationalTriglot = "titleNationalTriglot";
static const char* psz_NationalFinderlist = "titleNationalFinderlist";
static const char* psz_GlossIndexNE = "titleGlossIndexNE";
static const char* psz_GlossIndexNN = "titleGlossIndexNN";
static const char* psz_GlossIndexNR = "titleGlossIndexNR";

static const char* psz_exportedEnglishDiglot = "exportedEnglishDiglot";
static const char* psz_exportedEnglishTriglot = "exportedEnglishTriglot";
//static const char* psz_exportedEnglishFinderlist = "exportedEnglishFinderlist";
static const char* psz_exportedGlossIndexEE = "exportedGlossIndexEE";
static const char* psz_exportedGlossIndexEN = "exportedGlossIndexEN";
static const char* psz_exportedGlossIndexER = "exportedGlossIndexER";
static const char* psz_exportedNationalDiglot = "exportedNationalDiglot";
static const char* psz_exportedNationalTriglot = "exportedNationalTriglot";
//static const char* psz_exportedNationalFinderlist = "exportedNationalFinderlist";
static const char* psz_exportedGlossIndexNE = "exportedGlossIndexNE";
static const char* psz_exportedGlossIndexNN = "exportedGlossIndexNN";
static const char* psz_exportedGlossIndexNR = "exportedGlossIndexNR";

// 1998-10-16 MRP
static const char* psz_exportedEnglishWebPage = "exportedEnglishWebPage";
static const char* psz_exportedNationalWebPage = "exportedNationalWebPage";

static const char* psz_cctEnglishLabels = "cctEnglishLabels";
static const char* psz_dotEnglish = "dotEnglish";
static const char* psz_cctNationalLabels = "cctNationalLabels";
static const char* psz_dotNational = "dotNational";

static const char* psz_SortOnlyByLexeme = "SortOnlyByLexeme";
static const char* psz_IncludeLexemeWithCitationForm = "IncludeLexemeWithCitationForm";
static const char* psz_ExcludeExamples = "ExcludeExamples";
static const char* psz_ExcludeNotes = "ExcludeNotes";
static const char* psz_IncludeNonMDF = "IncludeNonMDF";
static const char* psz_IncludeRegionalWithNational = "IncludeRegionalWithNational";
static const char* psz_ExcludePartOfSpeech = "ExcludePartOfSpeech";
static const char* psz_SingleColumn = "SingleColumn";
static const char* psz_NoHeader = "NoHeader";
static const char* psz_NoDate = "NoDate";
static const char* psz_NoTotal = "NoTotal";

static const char* psz_typRTF = "typRTF";
static const char* psz_typHTML = "typHTML";  // 1998-10-14 MRP
// 1998-10-22 MRP: If these properties aren't in the .typ file,
// reinitialize them with their default values.
static const char* s_pszDatabaseTypeRTF = "MDF Rich Text Format";
static const char* s_pszDatabaseTypeHTML = "MDF SF-to-HTML";

static const char* psz_CheckConsistency = "CheckConsistency";
static const char* psz_mrfset = "mrfsetFieldsExcluded";

static const char* psz_WebPages = "WebPages";  // 1998-09-21 MRP
static const char* psz_HTML_XML = "HTML_XML";  // 1998-12-05 MRP
static const char* psz_UTF8 = "UTF8";  // 1998-02-16 MRP
static const char* psz_copyright = "copyright";  // 1998-12-05 MRP
static const char* psz_ChainProcess = "ChainProcess";

// 1999-03-10 MRP: Customized versions of the change tables.
static const char* psz_cctDict1 = "cctDict1";
static const char* psz_cctDict2 = "cctDict2";
static const char* psz_cctDictH = "cctDictH";
static const char* psz_cctLinkH = "cctLinkH";
static const char* psz_cctList1 = "cctList1";
static const char* psz_cctList2 = "cctList2";
static const char* psz_cctListH = "cctListH";

// 1998-12-03 MRP: User-defined index
static const char* psz_UserDefined = "UserDefined";
static const char* psz_IndexSelect = "IndexSelect";
static const char* psz_option = "option";
static const char* psz_cctIndex1 = "cctIndex1";
static const char* psz_IndexPriKey = "IndexPriKey";
static const char* psz_IndexSecKey = "IndexSecKey";
static const char* psz_IndexingCategories = "IndexingCategories";
static const char* psz_cctIndex2 = "cctIndex2";
static const char* psz_cctIndexH = "cctIndexH";  // 1999-03-10 MRP

#ifdef typWritefstream // 1.6.4ac
void CExportProcessMDF::WriteProperties(Object_ofstream& obs) const
#else
void CExportProcessMDF::WriteProperties(Object_ostream& obs) const
#endif
{
    obs.WriteBeginMarker(psz_expMDF, sName());
    
    obs.WriteString(psz_ChainProcess, m_sChainProcess);
    obs.WriteBool(psz_WebPages, m_bWebPages);  // 1998-09-21 MRP
    obs.WriteBool(psz_HTML_XML, m_bHTML_XML);  // 1998-12-05 MRP
    obs.WriteBool(psz_UTF8, m_bUTF8);  // 1999-02-16 MRP
    obs.WriteObligatoryString(psz_copyright, m_sCopyright);  // 1999-04-01 MRP

    obs.WriteBool(psz_National, bNational());
    obs.WriteBool(psz_Triglot, bTriglot());
    // 1999-01-25 MRP: Finderlist language independent of audience
    obs.WriteBool(psz_EnglishGlossIndex, bEnglishGlossIndex());
    obs.WriteBool(psz_NationalGlossIndex, bNationalGlossIndex());
    obs.WriteBool(psz_RegionalGlossIndex, bRegionalGlossIndex());

    // 1999-03-10 MRP: Customized versions of the change tables.
    obs.WriteProjectPath(psz_cctDict1, m_sFileNameDict1);
    obs.WriteProjectPath(psz_cctDict2, m_sFileNameDict2);
    obs.WriteProjectPath(psz_cctDictH, m_sFileNameDictH);
    obs.WriteProjectPath(psz_cctLinkH, m_sFileNameLinkH);
    obs.WriteProjectPath(psz_cctList1, m_sFileNameList1);
    obs.WriteProjectPath(psz_cctList2, m_sFileNameList2);
    obs.WriteProjectPath(psz_cctListH, m_sFileNameListH);

    // 1998-12-03 MRP: User-defined index
    obs.WriteBool(psz_UserDefined, m_bUserDefined);
    if ( m_pmkrIndexSelect )
        m_pmkrIndexSelect->WriteRef(obs, psz_IndexSelect);
    obs.WriteString(psz_option, m_sOption);
    obs.WriteProjectPath(psz_cctIndex1, m_sFileNameIndex1);
    if ( m_pmkrIndexPriKey )
        m_pmkrIndexPriKey->WriteRef(obs, psz_IndexPriKey);
    const CMarkerRef* pmrf = m_mrflstIndexSecKeys.pmrfFirst();
    for ( ; pmrf; pmrf = m_mrflstIndexSecKeys.pmrfNext(pmrf) )
        pmrf->pmkr()->WriteRef(obs, psz_IndexSecKey);
    obs.WriteBool(psz_IndexingCategories, m_bIndexingCategories);
    obs.WriteProjectPath(psz_cctIndex2, m_sFileNameIndex2);
    obs.WriteProjectPath(psz_cctIndexH, m_sFileNameIndexH);

    obs.WriteString(psz_EnglishDiglot, m_aasTitle[0][0]);
    obs.WriteString(psz_EnglishTriglot, m_aasTitle[0][1]);
    //obs.WriteString(psz_EnglishFinderlist, m_aasTitle[0][2]);
    obs.WriteString(psz_GlossIndexEE, m_aasTitle[0][2]);
    obs.WriteString(psz_GlossIndexEN, m_aasTitle[0][3]);
    obs.WriteString(psz_GlossIndexER, m_aasTitle[0][4]);
    obs.WriteString(psz_NationalDiglot, m_aasTitle[1][0]);
    obs.WriteString(psz_NationalTriglot, m_aasTitle[1][1]);
    //obs.WriteString(psz_NationalFinderlist, m_aasTitle[1][2]);
    obs.WriteString(psz_GlossIndexNE, m_aasTitle[1][2]);
    obs.WriteString(psz_GlossIndexNN, m_aasTitle[1][3]);
    obs.WriteString(psz_GlossIndexNR, m_aasTitle[1][4]);
    
    obs.WriteProjectPath(psz_exportedEnglishDiglot, m_aasExportedRtf[0][0]);
    obs.WriteProjectPath(psz_exportedEnglishTriglot, m_aasExportedRtf[0][1]);
    //obs.WriteProjectPath(psz_exportedEnglishFinderlist, m_aasExportedRtf[0][2]);
    obs.WriteProjectPath(psz_exportedGlossIndexEE, m_aasExportedRtf[0][2]);
    obs.WriteProjectPath(psz_exportedGlossIndexEN, m_aasExportedRtf[0][3]);
    obs.WriteProjectPath(psz_exportedGlossIndexER, m_aasExportedRtf[0][4]);
    obs.WriteProjectPath(psz_exportedNationalDiglot, m_aasExportedRtf[1][0]);
    obs.WriteProjectPath(psz_exportedNationalTriglot, m_aasExportedRtf[1][1]);
    //obs.WriteProjectPath(psz_exportedNationalFinderlist, m_aasExportedRtf[1][2]);
    obs.WriteProjectPath(psz_exportedGlossIndexNE, m_aasExportedRtf[1][2]);
    obs.WriteProjectPath(psz_exportedGlossIndexNN, m_aasExportedRtf[1][3]);
    obs.WriteProjectPath(psz_exportedGlossIndexNR, m_aasExportedRtf[1][4]);

	// 1998-10-16 MRP
    obs.WriteProjectPath(psz_exportedEnglishWebPage, m_asExportedHtm[0]);
    obs.WriteProjectPath(psz_exportedNationalWebPage, m_asExportedHtm[1]);

    obs.WriteProjectPath(psz_cctEnglishLabels, m_sEnglishLabelsCct);
    obs.WriteProjectPath(psz_dotEnglish, m_sEnglishDot);
    obs.WriteProjectPath(psz_cctNationalLabels, m_sNationalLabelsCct);
    obs.WriteProjectPath(psz_dotNational, m_sNationalDot);

    if ( m_pfil )
        m_pfil->WriteRef(obs);

    // Multi-Dictionary Formatter Options
    obs.WriteBool(psz_SortOnlyByLexeme, !m_bSortByCitationForm);
    obs.WriteBool(psz_IncludeLexemeWithCitationForm, m_bIncludeLexemeWithCitationForm);
    obs.WriteBool(psz_ExcludeExamples, !m_bIncludeExamples);
    obs.WriteBool(psz_ExcludeNotes, !m_bIncludeNotes);
    obs.WriteBool(psz_IncludeNonMDF, m_bIncludeNonMDF);
    obs.WriteBool(psz_IncludeRegionalWithNational, m_bIncludeRegionalWithNational);
    obs.WriteBool(psz_ExcludePartOfSpeech, !m_bIncludePartOfSpeech);
    obs.WriteBool(psz_SingleColumn, !m_bDoubleColumn);
    obs.WriteBool(psz_NoHeader, !m_bHeader);
    obs.WriteBool(psz_NoDate, !m_bDate);
    obs.WriteBool(psz_NoTotal, !m_bTotal);
    
    obs.WriteString(psz_typRTF, m_sDatabaseTypeRTF);
    obs.WriteString(psz_typHTML, m_sDatabaseTypeHTML);  // 1998-10-16 MRP

    obs.WriteBool(psz_CheckConsistency, m_bCheckConsistency);  // 1999-06-14 MRP
    m_subsetMarkersToExport.WriteProperties(obs);
    
    m_ppgs->WriteProperties(obs);
	obs.WriteUnrecognizedSettingsInfo( m_sUnrecognizedSettingsInfo ); // 1.0cp Write unrecognized settings info

    obs.WriteEndMarker(psz_expMDF);
}

BOOL CExportProcessMDF::s_bReadProperties(Object_istream& obs,
        CExportProcessSet* pexpsetMyOwner, CExportProcess** ppexp)
{
    Str8 sName;
    if ( !obs.bReadBeginMarker(psz_expMDF, sName) )
        return FALSE;

    Str8 sValidName;
    if ( !pexpsetMyOwner->bValidNameRead(sName, s_pszDefaultNameMDF, sValidName) )
        sName = sValidName;

    ASSERT( pexpsetMyOwner );
    ASSERT( ppexp );
    ASSERT( !*ppexp );
    CExportProcessMDF* pexp = new CExportProcessMDF(sName, pexpsetMyOwner, FALSE);
    pexp->ReadProperties(obs);    

    *ppexp = pexp;
    return TRUE;
}

void CExportProcessMDF::ReadProperties(Object_istream& obs)
{
    BOOL bCopyright = FALSE;  // 1999-04-01 MRP
    BOOL bNational = FALSE;
    BOOL bTriglot = FALSE;
    BOOL bFinderlist = FALSE;
    BOOL bEnglishGlossIndex = FALSE;
    BOOL bNationalGlossIndex = FALSE;
    BOOL bRegionalGlossIndex = FALSE;
    CMarker* pmkrIndexSelect = NULL;  // 1998-12-03 MRP
    CMarker* pmkrIndexPriKey = NULL;
    CMarker* pmkrIndexSecKey = NULL;
    CMarkerSet* pmkrset = m_pexpsetMyOwner->ptypMyOwner()->pmkrset();
    CFilterSet* pfilset = m_pexpsetMyOwner->ptypMyOwner()->pfilset();
    BOOL bExcludedFieldsRead = FALSE;

    // 1999-01-26 MRP: Read old properties for gloss index (finderlist) titles.
    // But disregard old properties for their exported file paths,
    // since the defaults have changed from list_e.rtf to List_ee.rtf, etc.

    while ( !obs.bAtEnd() )
        {
        BOOL b = FALSE;
        if ( obs.bReadBool(psz_WebPages, m_bWebPages) )
        	;
        else if ( obs.bReadString(psz_ChainProcess, m_sChainProcess) )
            ;
        else if ( obs.bReadBool(psz_HTML_XML, m_bHTML_XML) )
        	;
        else if ( obs.bReadBool(psz_UTF8, m_bUTF8) )
        	;
        else if ( obs.bReadString(psz_copyright, m_sCopyright) )
            bCopyright = TRUE;
        else if ( obs.bReadBool(psz_National, bNational) )
            ;
        else if ( obs.bReadBool(psz_Triglot, bTriglot) )
            ;
        // 1999-01-25 MRP: Finderlist language independent of audience
        else if ( obs.bReadBool(psz_Finderlist, bFinderlist) )
            ;
        else if ( obs.bReadBool(psz_EnglishGlossIndex, bEnglishGlossIndex) )
            ;
        else if ( obs.bReadBool(psz_NationalGlossIndex, bNationalGlossIndex) )
            ;
        else if ( obs.bReadBool(psz_RegionalGlossIndex, bRegionalGlossIndex) )
            ;
        // 1999-03-10 MRP: Customized versions of the change tables.
        else if ( obs.bReadProjectPath(psz_cctDict1, m_sFileNameDict1) )
            ;
        else if ( obs.bReadProjectPath(psz_cctDict2, m_sFileNameDict2) )
            ;
        else if ( obs.bReadProjectPath(psz_cctDictH, m_sFileNameDictH) )
            ;
        else if ( obs.bReadProjectPath(psz_cctLinkH, m_sFileNameLinkH) )
            ;
        else if ( obs.bReadProjectPath(psz_cctList1, m_sFileNameList1) )
            ;
        else if ( obs.bReadProjectPath(psz_cctList2, m_sFileNameList2) )
            ;
        else if ( obs.bReadProjectPath(psz_cctListH, m_sFileNameListH) )
            ;
        // 1998-12-03 MRP: User-defined index
        else if ( obs.bReadBool(psz_UserDefined, m_bUserDefined) )
            ;
        else if ( pmkrset->bReadMarkerRef(obs, psz_IndexSelect, pmkrIndexSelect) )
            ;
        else if ( obs.bReadString(psz_option, m_sOption) )
            ;
        else if ( obs.bReadProjectPath(psz_cctIndex1, m_sFileNameIndex1) )
            ;
        else if ( pmkrset->bReadMarkerRef(obs, psz_IndexPriKey, pmkrIndexPriKey) )
            ;
        else if ( pmkrset->bReadMarkerRef(obs, psz_IndexSecKey, pmkrIndexSecKey) )
            {
            if ( pmkrIndexSecKey )
                m_mrflstIndexSecKeys.AddMarkerRef(pmkrIndexSecKey);
            pmkrIndexSecKey = NULL;
            }
        else if ( obs.bReadBool(psz_IndexingCategories, m_bIndexingCategories) )
            ;
        else if ( obs.bReadProjectPath(psz_cctIndex2, m_sFileNameIndex2) )
            ;
        else if ( obs.bReadProjectPath(psz_cctIndexH, m_sFileNameIndexH) )
            ;
        else if ( obs.bReadString(psz_EnglishDiglot, m_aasTitle[0][0]) )
            ;
        else if ( obs.bReadString(psz_EnglishTriglot, m_aasTitle[0][1]) )
            ;
        else if ( obs.bReadString(psz_EnglishFinderlist, m_aasTitle[0][2]) )
            ;
        else if ( obs.bReadString(psz_GlossIndexEE, m_aasTitle[0][2]) )
            ;
        else if ( obs.bReadString(psz_GlossIndexEN, m_aasTitle[0][3]) )
            ;
        else if ( obs.bReadString(psz_GlossIndexER, m_aasTitle[0][4]) )
            ;
        else if ( obs.bReadString(psz_NationalDiglot, m_aasTitle[1][0]) )
            ;
        else if ( obs.bReadString(psz_NationalTriglot, m_aasTitle[1][1]) )
            ;
        else if ( obs.bReadString(psz_NationalFinderlist, m_aasTitle[1][3]) )
            ;  // 1999-01-26 MRP: Used to be [1][2]
        else if ( obs.bReadString(psz_GlossIndexNE, m_aasTitle[1][2]) )
            ;
        else if ( obs.bReadString(psz_GlossIndexNN, m_aasTitle[1][3]) )
            ;
        else if ( obs.bReadString(psz_GlossIndexNR, m_aasTitle[1][4]) )
            ;
        else if ( obs.bReadProjectPath(psz_exportedEnglishDiglot, m_aasExportedRtf[0][0]) )
            ;
        else if ( obs.bReadProjectPath(psz_exportedEnglishTriglot, m_aasExportedRtf[0][1]) )
            ;
        //else if ( obs.bReadProjectPath(psz_exportedEnglishFinderlist, m_aasExportedRtf[0][2]) )
        //    ;
        else if ( obs.bReadProjectPath(psz_exportedGlossIndexEE, m_aasExportedRtf[0][2]) )
            ;
        else if ( obs.bReadProjectPath(psz_exportedGlossIndexEN, m_aasExportedRtf[0][3]) )
            ;
        else if ( obs.bReadProjectPath(psz_exportedGlossIndexER, m_aasExportedRtf[0][4]) )
            ;
        else if ( obs.bReadProjectPath(psz_exportedNationalDiglot, m_aasExportedRtf[1][0]) )
            ;
        else if ( obs.bReadProjectPath(psz_exportedNationalTriglot, m_aasExportedRtf[1][1]) )
            ;
        //else if ( obs.bReadProjectPath(psz_exportedNationalFinderlist, m_aasExportedRtf[1][2]) )
        //    ;
        else if ( obs.bReadProjectPath(psz_exportedGlossIndexNE, m_aasExportedRtf[1][2]) )
            ;
        else if ( obs.bReadProjectPath(psz_exportedGlossIndexNN, m_aasExportedRtf[1][3]) )
            ;
        else if ( obs.bReadProjectPath(psz_exportedGlossIndexNR, m_aasExportedRtf[1][4]) )
            ;
        else if ( obs.bReadProjectPath(psz_exportedEnglishWebPage, m_asExportedHtm[0]) )
            ;
        else if ( obs.bReadProjectPath(psz_exportedNationalWebPage, m_asExportedHtm[1]) )
            ;
        else if ( obs.bReadProjectPath(psz_cctEnglishLabels, m_sEnglishLabelsCct) )
            ;
        else if ( obs.bReadProjectPath(psz_dotEnglish, m_sEnglishDot) )
            ;
        else if ( obs.bReadProjectPath(psz_cctNationalLabels, m_sNationalLabelsCct) )
            ;
        else if ( obs.bReadProjectPath(psz_dotNational, m_sNationalDot) )
            ;
        else if ( pfilset->bReadFilterRef(obs, m_pfil) )
            ;
        else if ( obs.bReadBool(psz_SortOnlyByLexeme, b) )
            m_bSortByCitationForm = FALSE;
        else if ( obs.bReadBool(psz_IncludeLexemeWithCitationForm, m_bIncludeLexemeWithCitationForm) )
            ;
        else if ( obs.bReadBool(psz_ExcludeExamples, b) )
            m_bIncludeExamples = FALSE;
        else if ( obs.bReadBool(psz_ExcludeNotes, b) )
            m_bIncludeNotes = FALSE;
        else if ( obs.bReadBool(psz_IncludeNonMDF, m_bIncludeNonMDF) )
            ;
        else if ( obs.bReadBool(psz_IncludeRegionalWithNational, m_bIncludeRegionalWithNational) )
            ;
        else if ( obs.bReadBool(psz_ExcludePartOfSpeech, b) )
            m_bIncludePartOfSpeech = FALSE;
        else if ( obs.bReadBool(psz_SingleColumn, b) )
            m_bDoubleColumn = FALSE;
        else if ( obs.bReadBool(psz_NoHeader, b) )
            m_bHeader = FALSE;
        else if ( obs.bReadBool(psz_NoDate, b) )
            m_bDate = FALSE;
        else if ( obs.bReadBool(psz_NoTotal, b) )
            m_bTotal = FALSE;
        else if ( obs.bReadString(psz_typRTF, m_sDatabaseTypeRTF) )
            ;
        else if ( obs.bReadString(psz_typHTML, m_sDatabaseTypeHTML) )  // 1998-10-16 MRP
            ;
        else if ( obs.bReadBool(psz_CheckConsistency, m_bCheckConsistency) )
            ;
        else if ( m_subsetMarkersToExport.bReadProperties(obs) )
            bExcludedFieldsRead = TRUE;
        else if ( RtfPageSetup::s_bReadProperties(obs, TRUE, &m_ppgs) )    
            ;
		else if ( obs.bUnrecognizedSettingsInfo( psz_expMDF, m_sUnrecognizedSettingsInfo  ) ) // 1.0cp Store any unrecognized settings info so as not to lose it
			;
        else if ( obs.bEnd(psz_expMDF) )
            break;
        }

    // 1999-01-25 MRP: CHECK COMMON CODE WITH ReadOldProperties!!!

    // 1999-04-01 MRP: Initialize the web copyright with the current year
    // when the database type is used the first time.
    if ( m_sCopyright.IsEmpty() && !bCopyright )
        InitCopyright();

    // 1999-01-25 MRP: Finderlist language independent of audience
    m_iAudience = (bNational ? 1 : 0);
    m_iFormat = 0;
    if ( bTriglot )
        m_iFormat = 1;
    else if ( bFinderlist )
        m_iFormat = 2;
    else if ( bEnglishGlossIndex )
        m_iFormat = 2;
    else if ( bNationalGlossIndex )
        m_iFormat = 3;
    else if ( bRegionalGlossIndex )
        m_iFormat = 4;
    if ( !bExcludedFieldsRead )  // If database type lacks this property
        InitExcludedFields();  // Then initialize it to the defaults
    if ( !m_ppgs )  // If ungrading from version 3.07 or earlier
        {
        int numColumns = ( m_bDoubleColumn ? 2 : 1 );
        m_ppgs = new RtfPageSetup(TRUE, 0.75, 1.25, 0.75, 0.75, 0.5, FALSE,
            0.375, 0.875, numColumns, 0.25);
        }

    // 1998-10-22 MRP: If these properties aren't in the .typ file,
    // reinitialize them with their default values.
    if ( m_sDatabaseTypeRTF.IsEmpty() )
        m_sDatabaseTypeRTF = s_pszDatabaseTypeRTF;
    if ( m_sDatabaseTypeHTML.IsEmpty() )
        m_sDatabaseTypeHTML = s_pszDatabaseTypeHTML;

    m_pmkrIndexSelect = pmkrIndexSelect;  // 1998-12-03 MRP
    m_pmkrIndexPriKey = pmkrIndexPriKey;  // 1998-12-03 MRP

    ConvertOlderProperties();  // 1999-01-25 MRP
}

static const char* psz_rtfEnglishDiglot = "rtfEnglishDiglot";
static const char* psz_rtfEnglishTriglot = "rtfEnglishTriglot";
//static const char* psz_rtfEnglishFinderlist = "rtfEnglishFinderlist";
static const char* psz_rtfNationalDiglot = "rtfNationalDiglot";
static const char* psz_rtfNationalTriglot = "rtfNationalTriglot";
//static const char* psz_rtfNationalFinderlist = "rtfNationalFinderlist";

BOOL CExportProcessMDF::s_bReadOldProperties(Object_istream& obs,
        CExportProcessSet* pexpsetMyOwner, CExportProcessMDF** ppexp)
{
    if ( !obs.bReadBeginMarker(psz_MDF) )
        return FALSE;
        
    ASSERT( pexpsetMyOwner );
    ASSERT( ppexp );
    ASSERT( !*ppexp );
    CExportProcessMDF* pexp = new CExportProcessMDF(s_pszDefaultNameMDF, pexpsetMyOwner, FALSE);
    pexp->ReadOldProperties(obs);

    *ppexp = pexp;
    return TRUE;
}

void CExportProcessMDF::ReadOldProperties(Object_istream& obs)
{
    BOOL bNational = FALSE;
    BOOL bFinderlist = FALSE;
    BOOL bTriglot = FALSE;
    CFilterSet* pfilset = m_pexpsetMyOwner->ptypMyOwner()->pfilset();
    BOOL bExcludedFieldsRead = FALSE;

    // 1999-01-26 MRP: Read old properties for gloss index (finderlist) titles.
    // But disregard old properties for their exported file paths,
    // since the defaults have changed from list_e.rtf to List_ee.rtf, etc.

    while ( !obs.bAtEnd() )
        {
        BOOL b = FALSE;
        if ( obs.bReadBool(psz_National, bNational) )
            ;
        else if ( obs.bReadBool(psz_Finderlist, bFinderlist) )
            ;
        else if ( obs.bReadBool(psz_Triglot, bTriglot) )
            ;
        else if ( obs.bReadString(psz_EnglishDiglot, m_aasTitle[0][0]) )
            ;
        else if ( obs.bReadString(psz_EnglishTriglot, m_aasTitle[0][1]) )
            ;
        else if ( obs.bReadString(psz_EnglishFinderlist, m_aasTitle[0][2]) )
            ;
        else if ( obs.bReadString(psz_NationalDiglot, m_aasTitle[1][0]) )
            ;
        else if ( obs.bReadString(psz_NationalTriglot, m_aasTitle[1][1]) )
            ;
        else if ( obs.bReadString(psz_NationalFinderlist, m_aasTitle[1][3]) )
            ;  // 1999-01-26 MRP: Used to be [1][2]
        else if ( obs.bReadProjectPath(psz_rtfEnglishDiglot, m_aasExportedRtf[0][0]) )
            ;
        else if ( obs.bReadProjectPath(psz_rtfEnglishTriglot, m_aasExportedRtf[0][1]) )
            ;
        //else if ( obs.bReadProjectPath(psz_rtfEnglishFinderlist, m_aasExportedRtf[0][2]) )
        //    ;
        else if ( obs.bReadProjectPath(psz_rtfNationalDiglot, m_aasExportedRtf[1][0]) )
            ;
        else if ( obs.bReadProjectPath(psz_rtfNationalTriglot, m_aasExportedRtf[1][1]) )
            ;
        //else if ( obs.bReadProjectPath(psz_rtfNationalFinderlist, m_aasExportedRtf[1][2]) )
        //    ;
        else if ( obs.bReadProjectPath(psz_cctEnglishLabels, m_sEnglishLabelsCct) )
            ;
        else if ( obs.bReadProjectPath(psz_dotEnglish, m_sEnglishDot) )
            ;
        else if ( obs.bReadProjectPath(psz_cctNationalLabels, m_sNationalLabelsCct) )
            ;
        else if ( obs.bReadProjectPath(psz_dotNational, m_sNationalDot) )
            ;
        else if ( pfilset->bReadFilterRef(obs, m_pfil) )
            ;
        else if ( obs.bReadBool(psz_SortOnlyByLexeme, b) )
            m_bSortByCitationForm = FALSE;
        else if ( obs.bReadBool(psz_IncludeLexemeWithCitationForm, m_bIncludeLexemeWithCitationForm) )
            ;
        else if ( obs.bReadBool(psz_ExcludeExamples, b) )
            m_bIncludeExamples = FALSE;
        else if ( obs.bReadBool(psz_ExcludeNotes, b) )
            m_bIncludeNotes = FALSE;
        else if ( obs.bReadBool(psz_IncludeNonMDF, m_bIncludeNonMDF) )
            ;
        else if ( obs.bReadBool(psz_IncludeRegionalWithNational, m_bIncludeRegionalWithNational) )
            ;
        else if ( obs.bReadBool(psz_ExcludePartOfSpeech, b) )
            m_bIncludePartOfSpeech = FALSE;
        else if ( obs.bReadBool(psz_SingleColumn, b) )
            m_bDoubleColumn = FALSE;
        else if ( obs.bReadBool(psz_NoHeader, b) )
            m_bHeader = FALSE;
        else if ( obs.bReadBool(psz_NoDate, b) )
            m_bDate = FALSE;
        else if ( obs.bReadBool(psz_NoTotal, b) )
            m_bTotal = FALSE;
        else if ( obs.bReadString(psz_typRTF, m_sDatabaseTypeRTF) )
            ;
        else if ( m_subsetMarkersToExport.bReadOldProperties(obs, psz_mrfset) )
            bExcludedFieldsRead = TRUE;
        else if ( RtfPageSetup::s_bReadProperties(obs, TRUE, &m_ppgs) )    
            ;
        else if ( obs.bEnd(psz_MDF) )
            break;
        }

    InitCopyright();  // 1999-04-01 MRP

    m_iAudience = (bNational ? 1 : 0);
    m_iFormat = 0;
    if ( bFinderlist )
        m_iFormat = 2;
    else if ( bTriglot )
        m_iFormat = 1;
    if ( !bExcludedFieldsRead )  // If database type lacks this property
        InitExcludedFields();  // Then initialize it to the defaults
    if ( !m_ppgs )  // If ungrading from version 3.07 or earlier
        {
        int numColumns = ( m_bDoubleColumn ? 2 : 1 );
        m_ppgs = new RtfPageSetup(TRUE, 0.75, 1.25, 0.75, 0.75, 0.5, FALSE,
            0.375, 0.875, numColumns, 0.25);
        }

    // 1998-10-22 MRP: If these properties aren't in the .typ file,
    // reinitialize them with their default values.
    if ( m_sDatabaseTypeRTF.IsEmpty() )
        m_sDatabaseTypeRTF = s_pszDatabaseTypeRTF;
    if ( m_sDatabaseTypeHTML.IsEmpty() )
        m_sDatabaseTypeHTML = s_pszDatabaseTypeHTML;

    ConvertOlderProperties();  // 1999-01-25 MRP
}


void CExportProcessMDF::ConvertOlderProperties()
{
    const Str8& sVersion = m_pexpsetMyOwner->sVersionMDF();
    if ( strcmp(sVersion, "4.1") < 0 )
        {
        // 1999-01-25 MRP: Finderlist language independent of audience
        if ( bFinderlist() )
            {
            if ( bNational() )
                m_iFormat = 3;
            else
                m_iFormat = 2;
            }
        InitNewTitles();  // 1999-01-26 MRP
        }
}

void CExportProcessMDF::InitDefaultProperties()
{
    // NOW WE ALWAYS ENABLE MDF FORMATTING AND PROVIDE DIAGNOSTIC MESSAGES
    // DURING FILE EXPORT IF ANYTHING IS WRONG OR MISSING

    InitCopyright();  // 1999-01-25 MRP

    m_sDatabaseTypeRTF = s_pszDatabaseTypeRTF;
    m_sDatabaseTypeHTML = s_pszDatabaseTypeHTML;  // 1998-10-22

    InitOldTitles();  // 1999-01-26 MRP
    InitNewTitles();  // 1999-01-26 MRP

    InitExcludedFields();

    m_ppgs = new RtfPageSetup(TRUE, 0.75, 1.25, 0.75, 0.75, 0.5, FALSE,
        0.375, 0.875, 2, 0.25);
}

void CExportProcessMDF::InitCopyright()
{
    // 1999-01-22 MRP: Fill in the copyright with the current year.
    // Note: The copyright symbol has the same code on Windows and Mac!
    Str8 sDateStamp;
    CDateCon::s_DateStamp(sDateStamp);
    m_sCopyright = "Copyright (c)"; // 1.5.0ft Maybe translate? copyright word and symbol for mdf output
	m_sCopyright = m_sCopyright + " " + sDateStamp.Right(4); // 1.5.0ft 
}

void CExportProcessMDF::InitOldTitles()
{
    // 1999-01-25 MRP: Finderlist language independent of audience
    // 1999-01-26 MRP: Titles in version 4.0x and earlier.
    // Note: Em-dash character code is d209 on Macintosh, d151 in ANSI
	// 1.5.0ed Eliminate ANSI em dash in MDF default titles
    m_aasTitle[0][0] = "vernacular - English Dictionary";
    m_aasTitle[0][1] = "vernacular - English - national Dictionary";
    m_aasTitle[0][2] = "English - vernacular";
    m_aasTitle[1][0] = "vernacular - national Dictionary";
    m_aasTitle[1][1] = "vernacular - national - English Dictionary";
    m_aasTitle[1][3] = "national - vernacular";
}

void CExportProcessMDF::InitNewTitles()
{
    // 1999-01-25 MRP: Finderlist language independent of audience
    // 1999-01-26 MRP: Titles in version 4.1 and later.
    // Note: Em-dash character code is d209 on Macintosh, d151 in ANSI
	// 1.5.0ed Eliminate ANSI em dash in MDF default titles
    m_aasTitle[0][3] = "national - vernacular";
    m_aasTitle[0][4] = "regional - vernacular";
    m_aasTitle[1][2] = "English - vernacular";
    m_aasTitle[1][4] = "regional - vernacular";
}

void CExportProcessMDF::InitExcludedFields()
{
    // 1997-11-01 MRP: Removed xg, since it was never implemented
    // and now it's being dropped from the official MDF set of markers.
    strstream_d255 ios;
    ios <<
        "\\+mkrsubsetExcluded\n"
        "\\mkr dt\n"
        "\\mkr ec\n"
        "\\mkr es\n"
        "\\mkr is\n"
        "\\mkr re\n"
        "\\mkr rn\n"
        "\\mkr rr\n"
        "\\mkr sd\n"
        "\\mkr so\n"
        "\\mkr st\n"
        "\\mkr th\n"
        "\\mkr we\n"
        "\\mkr wn\n"
        "\\mkr wr\n"
        "\\-mkrsubsetExcluded\n";
    CNoteList notlst;
    Object_istream obs(ios, notlst);
    BOOL b = m_subsetMarkersToExport.bReadProperties(obs);
    ASSERT( b );
}


static const char s_pszTypeMDF_RTF[] = "MDF RTF";
static const char s_pszTypeMDF_HTML[] = "MDF HTML";  // 1998-10-14 MRP

const char* CExportProcessMDF::pszType() const
{
    return m_bWebPages ? s_pszTypeMDF_HTML : s_pszTypeMDF_RTF;
}

BOOL CExportProcessMDF::bMDF(CFilter** ppfil) const
{
    ASSERT( ppfil );
    *ppfil = m_pfil;
    return TRUE;
}

BOOL CExportProcessMDF::bCanAutoOpen() const
{
    // 1999-10-11 MRP: Because the user might change the Exported file type
    // to RTF, the AutoOpen check box should always be enabled for MDF.
    // return !m_bWebPages;  // 1999-01-22 MRP
    return TRUE;
}

Str8 CExportProcessMDF::sHierarchyMarkers() const
{
    // 1998-04-16 MRP: Generalize the "alternate" hierarchy.
    CMarkerSet* pmkrset = m_pexpsetMyOwner->pmkrset();
    CMarker* pmkr_se = pmkrset->pmkrSearch("se");
    CMarker* pmkr_ps = pmkrset->pmkrSearch("ps");
    CMarker* pmkr_sn = pmkrset->pmkrSearch("sn");

    Str8 s;
    if ( !pmkr_se || !pmkr_ps || !pmkr_sn ||
            pmkr_se->bOver(pmkr_ps) && pmkr_ps->bOver(pmkr_sn) )
        s = "\\lx \\se \\ps \\sn";  // Standard hierarchy
    else if ( pmkr_se->bOver(pmkr_sn) && pmkr_sn->bOver(pmkr_ps) )
        s = "\\lx \\se \\sn \\ps";
    else if ( pmkr_sn->bOver(pmkr_se) && pmkr_se->bOver(pmkr_ps) )
        s = "\\lx \\sn \\se \\ps";  // Malaysia hierarchy
    else if ( pmkr_sn->bOver(pmkr_ps) && pmkr_ps->bOver(pmkr_se) )
        s = "\\lx \\sn \\ps \\se";  // Philippines hierarchy
    else if ( pmkr_ps->bOver(pmkr_se) && pmkr_se->bOver(pmkr_sn) )
        s = "\\lx \\ps \\se \\sn";
    else if ( pmkr_ps->bOver(pmkr_sn) && pmkr_sn->bOver(pmkr_se) )
        s = "\\lx \\ps \\sn \\se";

    return s;
}

static const char* psz_se_ps_sn = "se_ps_sn";
static const char* psz_se_sn_ps = "se_sn_ps";
static const char* psz_sn_se_ps = "sn_se_ps";
static const char* psz_sn_ps_se = "sn_ps_se";
static const char* psz_ps_se_sn = "ps_se_sn";
static const char* psz_ps_sn_se = "ps_sn_se";

void CExportProcessMDF::WriteHierarchy(SF_ostream& sfs) const
{
    CMarkerSet* pmkrset = m_pexpsetMyOwner->pmkrset();
    CMarker* pmkr_sn = pmkrset->pmkrSearch("sn");
    if ( !pmkr_sn )
        return;
    CMarker* pmkr_se = pmkrset->pmkrSearch("se");
    if ( !pmkr_se )
        return;
    CMarker* pmkr_ps = pmkrset->pmkrSearch("ps");
    if ( !pmkr_ps )
        return;

    const char* psz = NULL;
    if ( pmkr_se->bOver(pmkr_ps) && pmkr_ps->bOver(pmkr_sn) )
        psz = psz_se_ps_sn;
    else if ( pmkr_se->bOver(pmkr_sn) && pmkr_sn->bOver(pmkr_ps) )
        psz = psz_se_sn_ps;
    else if ( pmkr_sn->bOver(pmkr_se) && pmkr_se->bOver(pmkr_ps) )
        psz = psz_sn_se_ps;
    else if ( pmkr_sn->bOver(pmkr_ps) && pmkr_ps->bOver(pmkr_se) )
        psz = psz_sn_ps_se;
    else if ( pmkr_ps->bOver(pmkr_se) && pmkr_se->bOver(pmkr_sn) )
        psz = psz_ps_se_sn;
    else if ( pmkr_ps->bOver(pmkr_sn) && pmkr_sn->bOver(pmkr_se) )
        psz = psz_ps_sn_se;

    // Let the change tables generalize their hierarchy logic.
    if ( psz )
        sfs.WriteInternalField(psz, "");
}


Str8 CExportProcessMDF::sSettingsFolder() const
{
    return m_pexpsetMyOwner->sSettingsFolder();    
}


BOOL CExportProcessMDF::bCustomizedChangeTables() const
{
    if ( !m_sFileNameDict1.IsEmpty() )
        return TRUE;
    else if ( !m_sFileNameDict2.IsEmpty() )
        return TRUE;
    else if ( !m_sFileNameDictH.IsEmpty() )
        return TRUE;
    else if ( !m_sFileNameLinkH.IsEmpty() )
        return TRUE;
    else if ( !m_sFileNameList1.IsEmpty() )
        return TRUE;
    else if ( !m_sFileNameList2.IsEmpty() )
        return TRUE;
    else if ( !m_sFileNameListH.IsEmpty() )
        return TRUE;

    return FALSE;
}

Str8 CExportProcessMDF::sChangeTableFolder() const
{
    // 1999-03-10 MRP: Same as the settings folder in version 4.1.
    return m_pexpsetMyOwner->sSettingsFolder();    
}

Str8 CExportProcessMDF::sChangeTablePath(const Str8& sFileName) const
{
    if (!sGetDirPath(sFileName).IsEmpty())
        return sFileName;
    return ::sPath(sChangeTableFolder(), sFileName);
}

Str8 CExportProcessMDF::sFileNameChangeTable1() const
{
    Str8 sFileName;
    if ( bFinderlist() )
        {
        if ( m_bUserDefined )
            sFileName = m_sFileNameIndex1;
        else if ( !m_sFileNameList1.IsEmpty() )
            sFileName = m_sFileNameList1;
        else
            sFileName = "MDFList1.cct";
        }
    else
        {
        if ( !m_sFileNameDict1.IsEmpty() )
            sFileName = m_sFileNameDict1;
        else
            sFileName = "MDFDict1.cct";
        }

    return sFileName;
}

Str8 CExportProcessMDF::sFileNameChangeTable2() const
{
    Str8 sFileName;
    if ( bFinderlist() && m_bUserDefined )
            {
            if ( m_bWebPages )
                sFileName = m_sFileNameIndexH;
            else
                sFileName = m_sFileNameIndex2;
            }
    else if ( bFinderlist() )  // Gloss index
        {
        if ( m_bWebPages )
            {
            if ( !m_sFileNameListH.IsEmpty() )
                sFileName = m_sFileNameListH;
            else
                sFileName = "MDFListH.cct";
            }
        else  // Document (RTF)
            {
            if ( !m_sFileNameList2.IsEmpty() )
                sFileName = m_sFileNameList2;
            else
                sFileName = "MDFList2.cct";
            }
        }
    else  // Dictionary
        {
        if ( m_bWebPages )
            {
            if ( !m_sFileNameDictH.IsEmpty() )
                sFileName = m_sFileNameDictH;
            else
                sFileName = "MDFDictH.cct";
            }
        else  // Document (RTF)
            {
            if ( !m_sFileNameDict2.IsEmpty() )
                sFileName = m_sFileNameDict2;
            else
                sFileName = "MDFDict2.cct";
            }
        }

    return sFileName;
}

Str8 CExportProcessMDF::sFileNameChangeTableLinkH() const
{
    Str8 sFileName;
    if ( !m_sFileNameLinkH.IsEmpty() )
        sFileName = m_sFileNameLinkH;
    else
        sFileName = "MDFLinkH.cct";

    return sFileName;
}

Str8 CExportProcessMDF::sFileNameChangeTableLabels() const
{
    Str8 sFileName;
    if ( bNational() )
        sFileName = m_sNationalLabelsCct;
    else
        sFileName = m_sEnglishLabelsCct;

    return sFileName;
}


static const char* s_aapszFileNameRtf[2][5] =
    {
    "Dict_ed.rtf",
    "Dict_et.rtf",
    "List_ee.rtf",
    "List_en.rtf",
    "List_er.rtf",
    "Dict_nd.rtf",
    "Dict_nt.rtf",
    "List_ne.rtf",
    "List_nn.rtf",
    "List_nr.rtf"
    };

static const char* s_apszSubDirHtml[2] =  // 1999-01-22 MRP
    {
    "English",
    "National"
    };
static const char s_pszFileNameHtml[] ="Index.html";  // 1998-12-02 MRP

Str8 CExportProcessMDF::sExportedFilePath() const
{
    Str8 s;
    if ( m_bWebPages )  // 1998-10-16 MRP
        s = m_asExportedHtm[m_iAudience];
    else  // RTF
        s = m_aasExportedRtf[m_iAudience][m_iFormat];

    return s;
}

Str8 CExportProcessMDF::sDocumentPath(const Str8& sDatabasePath) const
{
    Str8 sDirPath = sGetDirPath(sDatabasePath);
    Str8 s;
    if ( m_bWebPages )  // 1998-10-16 MRP
    	{
    	if ( m_asExportedHtm[m_iAudience].IsEmpty() )
            {
            // 1999-01-22 MRP
	    	s = sPath(sDirPath, s_apszSubDirHtml[m_iAudience]);
            s = sPath(s, s_pszFileNameHtml);
            }
	    else
		    s = m_asExportedHtm[m_iAudience];
		}
    else  // RTF
    	{
    	if ( m_aasExportedRtf[m_iAudience][m_iFormat].IsEmpty() )
	    	s = sPath(sDirPath, s_aapszFileNameRtf[m_iAudience][m_iFormat]);
	    else
		    s = m_aasExportedRtf[m_iAudience][m_iFormat];
		}

    return s;
}

void CExportProcessMDF::SetDocumentPath(const Str8& sDocumentPath)
{
    if ( m_bWebPages )  // 1998-10-16 MRP
	    m_asExportedHtm[m_iAudience] = sDocumentPath;
    else  // RTF
	    m_aasExportedRtf[m_iAudience][m_iFormat] = sDocumentPath;
}

static const char* s_pszTempExt = ".tmp";

BOOL CExportProcessMDF::bExportedFileDlg(const Str8& sDatabasePath)
{
    const char* pszDefaultExt = ( m_bWebPages ? "html" : "rtf" );
    // 1999-01-22 MRP: Prompt for RTF export, but not for MDF HTML.
    BOOL bPromptOverwrite = !m_bWebPages;
	Str8 sFileFilter;
	if ( m_bWebPages )
		sFileFilter = sFileFilter + _("Web pages") + " (*.html;*.htm)|*.html;*.htm||"; // 1.5.0fz 
	else
		sFileFilter = sFileFilter + _("Rich Text Format") + " (*.rtf)|*.rtf||"; // 1.5.0fz 
    int iFilterIndex = 1;
    Str8 sPath = sDocumentPath(sDatabasePath);
	if ( !CExportProcess::s_bExportedFileDlg(pszDefaultExt, bPromptOverwrite, sFileFilter, &iFilterIndex, sPath) )
	    return FALSE;

    SetDocumentPath(sPath);
    return TRUE;
}


BOOL CExportProcessMDF::bExport(CIndex* pindCur, const CRecLookEl* prelCur,
        const Str8& sPathDatabase, Str8& sMessage)
{
#ifndef EXPORT_WEB_PAGES
    // 1998-12-02 MRP: Web page format isn't implemented in the 16-bit version,
    // mainly because there would be problems with long file names and
    // the four-letter .html file name extension.
    //
    // DESIGN QUESTION: Should these processes be disabled? How?
    //
    if ( m_bWebPages )
        {
        sMessage = "MDF HTML export processes are disabled in Toolbox for 16-bit Windows";
        return FALSE;
        }
#endif

    // 1999-01-25 MRP: Shoebox converts properties from an older version
    // right when it reads them. The first time that this export process
    // is used after the conversion, Shoebox shows a message reminding
    // the user to copy newer versions of MDF's files.
    // 1999-03-02 MRP: Move MDF version to the export process set.
    // 1999-03-05 MRP: Use the term "Update" instead of "Convert"
    if ( m_pexpsetMyOwner->bShowConvertOlderMDF() )
        {
        CMDFUpdateOlderDlg dlg(m_pexpsetMyOwner);  // 1999-02-27 MRP
        MessageBeep(0);
        int i = dlg.DoModal();
        m_pexpsetMyOwner->ClearShowConvertOlderMDF();
        return TRUE;
        }

    // 1998-03-10 MRP: The user chooses whether or not to see the properties
    // and be able to modify them (except the name) when exporting.
    if ( m_pexpsetMyOwner->bShowProperties() )
        {
        CMDFDlg dlg(this, FALSE);
        if ( dlg.DoModal() != IDOK )
            return TRUE;
        }

    if ( m_bCheckConsistency && !bCheckConsistency(pindCur) )
        return TRUE;

    if ( !bExportedFileDlg(sPathDatabase) )  // 1999-01-22 MRP
        return TRUE;
    
    if ( !bSelectSortFormat(pindCur->pindsetMyOwner(), sPathDatabase, sMessage) )
        return FALSE;

    return TRUE;
}

// 1999-02-26 MRP: Temporary file for links in web pages (HTML).
static Str8 s_sTempPathWebPageLinks;
static Str8 s_sChangePathWebPageLinks;

BOOL CExportProcessMDF::bSelectSortFormat(CIndexSet* pindset,
        const Str8& sPathIn, Str8& sMessage)
{
    // 1997-12-08 MRP: Make sure the record marker is \lx
    const Str8& sRecordMarker = m_pexpsetMyOwner->ptypMyOwner()->
        pmkrRecord()->sName();
    if ( strcmp(sRecordMarker, "lx") != 0 )
        {
		sMessage = sMessage + _("MDF record marker must be:") + " \\lx"; // 1.5.0fd 
        return FALSE;
        }
    
    CDatabaseTypeSet* ptypset = m_pexpsetMyOwner->ptypMyOwner()->ptypsetMyOwner();
    Str8 sProjectPath = sSettingsFolder();  // 1999-03-10 MRP
    CDatabaseType* ptyp = NULL;
    CNoteList notlst;
    if ( m_bWebPages )  // 1998-10-16 MRP
    	{
	    if ( !ptypset->bSearch(m_sDatabaseTypeHTML, &ptyp, notlst) )
	        {
			sMessage = sMessage + _("Cannot find database type:") + " " + m_sDatabaseTypeHTML; // 1.5.0fd 
	        return FALSE;
	        }
    	}
    else
    	{
	    if ( !ptypset->bSearch(m_sDatabaseTypeRTF, &ptyp, notlst) )
	        {
			sMessage = sMessage + _("Cannot find database type:") + " " + m_sDatabaseTypeRTF; // 1.5.0fd 
	        return FALSE;
	        }
	    }

    ASSERT( pindset );
    ASSERT( pindset->ptyp() == m_pexpsetMyOwner->ptypMyOwner() );

    const Str8& sPathOut = sExportedFilePath();  // 1999-01-22 MRP
    Str8 sDirPath = sGetDirPath(sPathOut);
    Str8 sTempFileName = sUniqueFileName(sDirPath, "mdf", s_pszTempExt);
    Str8 sTempPath = sPath(sDirPath, sTempFileName, s_pszTempExt);

    // 1999-02-26 MRP: Temporary file for links in web pages (HTML).
    Str8 sTempFileNameWebPageLinks = sUniqueFileName(sDirPath, "html", s_pszTempExt);
    s_sTempPathWebPageLinks = sPath(sDirPath, sTempFileNameWebPageLinks, s_pszTempExt);
    s_sChangePathWebPageLinks = sChangeTablePath(sFileNameChangeTableLinkH());

    //                       Select  Sort     Format Label RTF     DOT
    // English  diglot:      dict1   \lx \hm  dict2  _eng  dict_ed mdf_e
    // English  triglot:     dict1   \lx \hm  dict2  _eng  dict_et mdf_e
    // English  finderlist:  list1   \re \ps  list2  _eng  list_e  mdf_e
    // National diglot:      dict1   \lx \hm  dict2  _inz  dict_nd mdf_n
    // National triglot:     dict1   \lx \hm  dict2  _inz  dict_nt mdf_n
    // National finderlist:  list1   \rn \ps  list2  _inz  list_n  mdf_n

#ifdef abmdf2011
	sTempPath = sPath(sDirPath,"MDFTemp1", ".txt"); // 1.5.9b 
#endif

    if ( !bSelect(pindset, sPathIn, sTempPath, sProjectPath, sMessage) )
        return FALSE;

	if ( m_bWebPages )  // 1998-09-16 MRP
		{
#ifdef EXPORT_WEB_PAGES
		if ( !bSortAndFormatHTML(sTempPath, ptyp, sPathOut,
				sProjectPath, sMessage) )
		    return FALSE;
#else
		ASSERT( FALSE );  // 1998-12-02 MRP: Disabled in 16-bit version
		return TRUE;
#endif
		}
	else
		{
		if ( !bSortAndFormatRTF(sTempPath, ptyp, sPathOut,
				sProjectPath, sMessage) )
		    return FALSE;
		}

    // 1998-03-17 MRP: Show the reminder before auto-opening.
    if ( !m_pexpsetMyOwner->bNoMDFReminder() && !m_bWebPages )  // 1998-01-21 MRP
        {
        CMDFReminderDlg dlgMDFReminder(m_pexpsetMyOwner);
        MessageBeep(0);
        int i = dlgMDFReminder.DoModal();
        }

    if ( bCanAutoOpen() && m_pexpsetMyOwner->bAutoOpen() )  // 1999-01-22 MRP
        {
        if ( !bAutoOpen(sPathOut, sMessage) )
            return FALSE;
        }
        
    return TRUE;
}

BOOL CExportProcessMDF::bSelect(CIndexSet* pindset, const Str8& sPathIn,
        const Str8& sTempPath,
        const Str8& sProjectPath, Str8& sMessage) const
{
    std::ofstream ios(sTempPath);  // We check for ofstream failures below

    {
    // 1999-03-10 MRP: Customized versions of the change tables.
    Str8 sTableFileName = sFileNameChangeTable1();
    Str8 sTablePath = sChangeTablePath(sTableFileName);

    if ( !bFileExists(sTablePath) )
        {
        int iRemoved = remove(sTempPath);
		sMessage = sMessage + _("Cannot find file:") + " " + sTablePath; // 1.5.0fd 
        return FALSE;
        }

    Change_ostream ccs( sTablePath, ios );
    if ( ccs.fail() )
        {
        int iRemoved = remove(sTempPath);
        return FALSE;
        }
        
    SF_ostream sfs(ccs, TRUE);  // 1998-12-07 MRP: Trim surrounding WS
    
    if ( bNational() )
        sfs.WriteInternalField(psz_National, "");
    if ( bFinderlist() )
        {
        // 1999-01-25 MRP: Finderlist language independent of audience
        if ( bEnglishGlossIndex() )
            sfs.WriteInternalField(psz_EnglishGlossIndex, "");
        if ( bNationalGlossIndex() )
            sfs.WriteInternalField(psz_NationalGlossIndex, "");
        if ( bRegionalGlossIndex() )
            sfs.WriteInternalField(psz_RegionalGlossIndex, "");
        if ( !m_bIncludePartOfSpeech )
            sfs.WriteInternalField(psz_ExcludePartOfSpeech, "");
        // if ( bAlternateHierarchy() )
        //     sfs.WriteInternalField(psz_AlternateHierarchy, "");
        WriteHierarchy(sfs);  // 1998-04-15 MRP
        // 1998-12-04 MRP: User-defined index
        if ( !m_sOption.IsEmpty() )
            sfs.WriteInternalField(m_sOption, "");
        }
    else  // Dictionary
        {
        if ( bTriglot() )
            sfs.WriteInternalField(psz_Triglot, "");
        if ( !m_bSortByCitationForm )
            sfs.WriteInternalField(psz_SortOnlyByLexeme, "");
        if ( !m_bIncludeExamples )
            sfs.WriteInternalField(psz_ExcludeExamples, "");
        if ( !m_bIncludeNotes )
            sfs.WriteInternalField(psz_ExcludeNotes, "");
        if ( m_bIncludeNonMDF )
            sfs.WriteInternalField(psz_IncludeNonMDF, "");
        if ( m_bIncludeRegionalWithNational )
            sfs.WriteInternalField(psz_IncludeRegionalWithNational, "");
        WriteHierarchy(sfs);  // 1998-04-15 MRP
        }

    ASSERT( pindset );  
    // 1997-01-27 MRP
    // const CIndex* pind = pindset->pindRecordOwner();
    CIndex* pindRecordOwner = pindset->pindRecordOwner();

    // 1998-12-03 MRP: User-defined index
//    CIndexPtr pind = pindset->pindFind_AddIfNew(
//        pindRecordOwner->pmkrPriKey(), pindRecordOwner->psrtPriKey(),
//        pindRecordOwner->pmrflstSecKeys(), 0, FALSE,
//        m_pfil);
    CMarkerRefList mrflstSecKeys;
    CIndexPtr pind;
    if ( bFinderlist() && m_bUserDefined && m_pmkrIndexSelect )
        pind = pindset->pindFind_AddIfNew(
            m_pmkrIndexSelect, m_pmkrIndexSelect->plng()->psrtDefault(),
            &mrflstSecKeys, 0, FALSE, m_pfil);
    else
        pind = pindset->pindFind_AddIfNew(
            pindRecordOwner->pmkrPriKey(), pindRecordOwner->psrtPriKey(),
            pindRecordOwner->pmrflstSecKeys(), 0, FALSE, m_pfil);

    CMarkerSubSet subsetMarkersInFinderlist(m_pexpsetMyOwner->pmkrset());
    CProgressIndicator prg(pind->lNumRecEls(), NULL, sPathIn); // 1.4ad Eliminate resource messages sent to progress bar
    long lWritten = 0;
    const CRecLookEl* prel = pind->prelFirst();
    for ( ; prel; prel = pind->prelNext(prel) )
        {
        if ( bFinderlist() )
            pind->WriteElement(sfs, m_pexpsetMyOwner->ptypMyOwner(), prel, subsetMarkersInFinderlist, // 1.4qpb
                m_bUserDefined, TRUE);
        else
            pind->WriteElement(sfs, m_pexpsetMyOwner->ptypMyOwner(), prel, m_subsetMarkersToExport, // 1.4qpb
                FALSE, TRUE);
        (void) prg.bUpdateProgress(++lWritten);
        }
    }

    BOOL bWriteFailed = ios.fail();     
    ios.close();  // Flushes any waiting output
    if ( bWriteFailed || ios.fail() )
        {
        int iRemoved = remove(sTempPath);
		sMessage = sMessage + _("Failed to write file:") + " " + sTempPath; // 1.5.0fd
        return FALSE;
        }

    return TRUE;
}  // CExportProcessMDF::bSelect

BOOL CExportProcessMDF::bSortAndFormatRTF(const Str8& sTempPath,
		CDatabaseType* ptyp, const Str8& sOutputPath,
        const Str8& sProjectPath, Str8& sMessage) const
{
    // Read record(s) into a temporary index set
    std::ifstream iosInput(sTempPath);
    if ( iosInput.fail() )
        {
		sMessage = sMessage + _("Cannot open file:") + " " + sTempPath; // 1.5.0fd 
        return FALSE;
        }

    // 1997-12-18 MRP: Emergency patch to provide compatibility with MDF-DOS
    // sorting of a reversed index (finderlist).
    MatchSetting matset = CMCharOrder::matsetSamePrimaryGroup;
    // 2000-04-20 MRP: Use special sorting for both dictionary and finderlist.
    matset &= (~CMCharOrder::mchtypUndefined);
    CIndexSet indset(m_pexpsetMyOwner->ptypMyOwner(), 300, FALSE, TRUE);
    
    LONG lSizFile = lFileSize(sTempPath);
    CMarkerSet* pmkrset = m_pexpsetMyOwner->pmkrset();
    CMarker* pmkrRecord = m_pexpsetMyOwner->ptypMyOwner()->pmkrRecord();
    
    {
    CProgressIndicator prg(lSizFile, NULL); // 1.4ad Eliminate resource messages sent to progress bar
    SF_istream sfsInput(iosInput, pmkrset, pmkrRecord);
    while ( !sfsInput.bAtEnd() )
        {
        CRecord* prec = NULL;
        if ( !sfsInput.bReadRecord(&prec) )
            return FALSE;

        indset.AddRecord(prec);
		LONG lPos = static_cast<LONG>(iosInput.tellg());
        (void) prg.bUpdateProgress(lPos);
        }
    }
        
    iosInput.close();
	if ( !bDebugKaren ) // Allow Ctrl+Alt+Shift+K to keep chained export files // 1.5.9b 
    if ( remove(sTempPath) != 0 )
        {
        return FALSE;
        }
        
    // Sort the records
    CIndex* pindRecordOwner = indset.pindRecordOwner();
    BOOL bSort = pindRecordOwner->bSort();
    const Str8& sDatabaseTypeName = m_pexpsetMyOwner->ptypMyOwner()->sName();
    
    CMarker* pmkrPriKey = NULL;
    CSortOrder* psrtPriKey = NULL;
    CMarker* pmkrSecKey = NULL;
    CMarkerRefList mrflstSecKeys;
    if ( bFinderlist() )
        {
        // 1998-12-03 MRP: User-defined index
        if ( m_bUserDefined && m_pmkrIndexPriKey )
            {
            pmkrPriKey = m_pmkrIndexPriKey;
            psrtPriKey = pmkrPriKey->plng()->psrtDefault();
            mrflstSecKeys = m_mrflstIndexSecKeys;
            }
        else
            {
            // 1999-01-25 MRP: Finderlist language independent of audience
            const char psz_re[] = "re";
            const char psz_rn[] = "rn";
            const char psz_rr[] = "rr";
            const char* pszPriKeyMarker = NULL;
            if ( bEnglishGlossIndex() )
                pszPriKeyMarker = psz_re;
            else if ( bNationalGlossIndex() )
                pszPriKeyMarker = psz_rn;
            else if ( bRegionalGlossIndex() )
                pszPriKeyMarker = psz_rr;
            pmkrPriKey = pmkrset->pmkrSearch(pszPriKeyMarker);
            if ( !pmkrPriKey )
                {
                return FALSE;
                }

            // 1997-12-08 MRP: Make sure the reversal and gloss fields
            // have the same language encoding.
            const char psz_ge[] = "ge";
            const char psz_gn[] = "gn";
            const char psz_gr[] = "gr";
            const char* pszGlossMarker = NULL;
            if ( bEnglishGlossIndex() )
                pszGlossMarker = psz_ge;
            else if ( bNationalGlossIndex() )
                pszGlossMarker = psz_gn;
            else if ( bRegionalGlossIndex() )
                pszGlossMarker = psz_gr;
            const CMarker* pmkrGloss = pmkrset->pmkrSearch(pszGlossMarker);
            if ( pmkrGloss && pmkrGloss->plng() != pmkrPriKey->plng() )
                {
				sMessage = _("Reversal and gloss must be same language encoding"); // 1.5.0fg 
                return FALSE;
                }

            psrtPriKey = pmkrPriKey->plng()->psrtDefault();
            const char pszSecKeyMarker[] = "ps";
            pmkrSecKey = pmkrset->pmkrSearch(pszSecKeyMarker);
            if ( !pmkrSecKey )
                {
                return FALSE;
                }

            mrflstSecKeys.AddMarkerRef(pmkrSecKey);
            // 1997-09-26 MRP: mdflist1.cct now preserves \ps and \pn
            // distinction so that user can translate \ps using the
            // national labels change table or use explicit \pn fields
            // in the lexical database file. So we sort by either field.
            if ( bNational() )
                {
                const char pszTerKeyMarker[] = "pn";
                CMarker* pmkrTerKey = pmkrset->pmkrSearch(pszTerKeyMarker);
                if ( !pmkrTerKey )
                    {
                    return FALSE;
                    }
    
                mrflstSecKeys.AddMarkerRef(pmkrTerKey);
                }
            }
        }
    else  // Dictionary
        {
        if ( m_bSortByCitationForm )  // 1997-09-26 MRP
            {
            const char psz_lc[] = "lc";
            pmkrPriKey = pmkrset->pmkrSearch(psz_lc);
            if ( !pmkrPriKey )
                {
                return FALSE;
                }

            psrtPriKey = pmkrPriKey->plng()->psrtDefault();
            }
        else
            {
            pmkrPriKey = pindRecordOwner->pmkrPriKey();
            psrtPriKey = pindRecordOwner->psrtPriKey();
            }
        const char pszSecKeyMarker[] = "hm";
        pmkrSecKey = pmkrset->pmkrSearch(pszSecKeyMarker);
        if ( !pmkrSecKey )
            {
            return FALSE;
            }

        mrflstSecKeys.AddMarkerRef(pmkrSecKey);
        }

    CIndex* pind = indset.pindFind_AddIfNew(pmkrPriKey, psrtPriKey,
        &mrflstSecKeys, 0, FALSE, NULL);
    
    // Write the sorted records and make formatting changes
    std::ofstream iosOutput(sTempPath);  // We check for ofstream failures below

    {
    // 1999-03-10 MRP: Customized versions of the change tables.
    Str8 sTableFileName = sFileNameChangeTable2();
    Str8 sTablePath = sChangeTablePath(sTableFileName);

    if ( !bFileExists(sTablePath) )
        {
        int iRemoved = remove(sTempPath);
		sMessage = sMessage + _("Cannot find file:") + " " + sTablePath; // 1.5.0fd 
        return FALSE;
        }

    Change_ostream ccs( sTablePath, iosOutput );
    if ( ccs.fail() )
        {
        int iRemoved = remove(sTempPath);
        return FALSE;
        }
        
    SF_ostream sfsOutput(ccs, TRUE);  // 1998-12-07 MRP: Trim surrounding WS
    
    // Write the formatting options
    if ( bNational() )
        sfsOutput.WriteInternalField(psz_National, "");
    if ( bFinderlist() )
        {
        // 1999-01-25 MRP: Finderlist language independent of audience
        if ( bEnglishGlossIndex() )
            sfsOutput.WriteInternalField(psz_EnglishGlossIndex, "");
        if ( bNationalGlossIndex() )
            sfsOutput.WriteInternalField(psz_NationalGlossIndex, "");
        if ( bRegionalGlossIndex() )
            sfsOutput.WriteInternalField(psz_RegionalGlossIndex, "");
        // 1998-12-04 MRP: User-defined index
        if ( !m_sOption.IsEmpty() )
            sfsOutput.WriteInternalField(m_sOption, "");
        }
    else
        {
        if ( bTriglot() )
            sfsOutput.WriteInternalField(psz_Triglot, "");
        if ( !m_bSortByCitationForm )
            sfsOutput.WriteInternalField(psz_SortOnlyByLexeme, "");
        if ( m_bIncludeLexemeWithCitationForm )
            sfsOutput.WriteInternalField(psz_IncludeLexemeWithCitationForm, "");
        // if ( bAlternateHierarchy() )
        //     sfsOutput.WriteInternalField(psz_AlternateHierarchy, "");
        }
    WriteHierarchy(sfsOutput);  // 1998-04-15 MRP
    // if ( !m_bDoubleColumn )
    if ( m_ppgs->numColumns() == 1 )
        sfsOutput.WriteInternalField(psz_SingleColumn, "");
    if ( !m_bTotal )
        sfsOutput.WriteInternalField(psz_NoTotal, "");
    
    // Order ordPrev = CMCharOrder::ordSecOnly;
    const CMChar* pmchPrev = NULL;  // 2000-04-17 MRP
    CLangEnc* plngPriKey = pmkrPriKey->plng();

    CProgressIndicator prg(pind->lNumRecEls(), NULL); // 1.4ad Eliminate resource messages sent to progress bar
    long lWritten = 0;
    const CRecLookEl* prel = pind->prelFirst();
    for ( ; prel; prel = pind->prelNext(prel) )
        {
        // Output the \LP Letter Paragraph field 
        // 2000-04-17 MRP: Rewrite to account for changes made to the sort key
        // using a CCT or DLL (optional step for non-Roman scripts).
        const CField* pfldHeading = prel->pfldPriKey();
        ASSERT( pfldHeading );
        const Str8& sHeading = pfldHeading->sContents();

        // 2000-07-24 MRP: If the primary sorting field is empty or
        // if it doesn't contain any primary sorting characters,
        // then the logic for outputting a heading is skipped.
        // Note: The second part of the if condition skips
        // outputting of a heading in the following unusual case:
        // the first primary sorting character is a space
        // (e.g., \gn ??? alerte).
        const CMChar* pmch = NULL;
        if ( psrtPriKey->bFirstPrimary(sHeading, &pmch) &&
                !bEqual(pmch->sMChar(), " ") )
            {
            if ( pmch != pmchPrev )
                {
                pmchPrev = pmch;  // 2000-04-17 MRP: Remember before resetting
                const CMChar* pmchUpper = (pmch->bUpperCase() ?
                    pmch :
                    pmch->pmchOtherCase()
                    );
                if ( pmch->bUpperCase() )
                    pmch = pmch->pmchOtherCase();
                Str8 sLetter;
                if ( pmchUpper )
                    {
                    sLetter = pmchUpper->sMChar();
                    sLetter += "  -  ";  // String will become an option
                    }
                ASSERT( pmch );
                sLetter += pmch->sMChar();
                sfsOutput.WriteField("LP", sLetter);  // Letter Paragraph
                }
            }

        pind->WriteElement(sfsOutput, m_pexpsetMyOwner->ptypMyOwner(), prel); // 1.4qpb
        (void) prg.bUpdateProgress(++lWritten);
        }
    }

    BOOL bWriteFailed = iosOutput.fail();       
    iosOutput.close();  // Flushes any waiting output
    if ( bWriteFailed || iosOutput.fail() )
        {
        int iRemoved = remove(sTempPath);
		sMessage = sMessage + _("Failed to write file:") + " " + sTempPath; // 1.5.0fd
        return FALSE;
        }

    if ( !bFormat(sTempPath, ptyp, sOutputPath, sProjectPath, sMessage) )
        return FALSE;
    
    return TRUE;
}  // CExportProcessMDF::bSortAndFormatRTF

#ifdef HTML_LINK_SORT
const CSortOrder* g_psrtHTML = NULL;  // 1998-10-26 MRP
#endif

#ifdef EXPORT_WEB_PAGES
BOOL CExportProcessMDF::bSortAndFormatHTML(const Str8& sTempPath,
		CDatabaseType* ptyp, const Str8& sOutputPath,
        const Str8& sProjectPath, Str8& sMessage) const
{
    // Read record(s) into a temporary index set
    std::ifstream iosInput(sTempPath);
    if ( iosInput.fail() )
        {
		sMessage = sMessage + _("Cannot open file:") + " " + sTempPath; // 1.5.0fd 
        return FALSE;
        }

    // 1997-12-18 MRP: Emergency patch to provide compatibility with MDF-DOS
    // sorting of a reversed index (finderlist).
    MatchSetting matset = CMCharOrder::matsetSamePrimaryGroup;
    // 2000-07-24 MRP: This change should have been added for HTML
    // at the same time that it was added for RTF!
    // 2000-04-20 MRP: Use special sorting for both dictionary and finderlist.
    matset &= (~CMCharOrder::mchtypUndefined);
    CIndexSet indset(m_pexpsetMyOwner->ptypMyOwner(), 300, FALSE, TRUE);
    
    LONG lSizFile = lFileSize(sTempPath);
    CMarkerSet* pmkrset = m_pexpsetMyOwner->pmkrset();
    CMarker* pmkrRecord = m_pexpsetMyOwner->ptypMyOwner()->pmkrRecord();
    
    {
    CProgressIndicator prg(lSizFile, NULL); // 1.4ad Eliminate resource messages sent to progress bar
    SF_istream sfsInput(iosInput, pmkrset, pmkrRecord);
    while ( !sfsInput.bAtEnd() )
        {
        CRecord* prec = NULL;
        if ( !sfsInput.bReadRecord(&prec) )
            return FALSE;

        indset.AddRecord(prec);
		LONG lPos = static_cast<LONG>(iosInput.tellg());
        (void) prg.bUpdateProgress(lPos);
        }
    }
        
    iosInput.close();
    if ( remove(sTempPath) != 0 )
        {
        return FALSE;
        }
        
    // Sort the records
    CIndex* pindRecordOwner = indset.pindRecordOwner();
    BOOL bSort = pindRecordOwner->bSort();
    const Str8& sDatabaseTypeName = m_pexpsetMyOwner->ptypMyOwner()->sName();
    
    CMarker* pmkrPriKey = NULL;
    CSortOrder* psrtPriKey = NULL;
    CMarker* pmkrSecKey = NULL;
    CMarkerRefList mrflstSecKeys;
    if ( bFinderlist() )
        {
        // 1998-12-03 MRP: User-defined index
        if ( m_bUserDefined && m_pmkrIndexPriKey )
            {
            pmkrPriKey = m_pmkrIndexPriKey;
            psrtPriKey = pmkrPriKey->plng()->psrtDefault();
            mrflstSecKeys = m_mrflstIndexSecKeys;
            }
        else
            {
            // 1999-03-15 MRP: Finderlist language independent of audience
            const char psz_re[] = "re";
            const char psz_rn[] = "rn";
            const char psz_rr[] = "rr";
            const char* pszPriKeyMarker = NULL;
            if ( bEnglishGlossIndex() )
                pszPriKeyMarker = psz_re;
            else if ( bNationalGlossIndex() )
                pszPriKeyMarker = psz_rn;
            else if ( bRegionalGlossIndex() )
                pszPriKeyMarker = psz_rr;
            pmkrPriKey = pmkrset->pmkrSearch(pszPriKeyMarker);
            if ( !pmkrPriKey )
                {
                return FALSE;
                }

            // 1997-12-08 MRP: Make sure the reversal and gloss fields
            // have the same language encoding.
            const char psz_ge[] = "ge";
            const char psz_gn[] = "gn";
            const char psz_gr[] = "gr";
            const char* pszGlossMarker = NULL;
            if ( bEnglishGlossIndex() )
                pszGlossMarker = psz_ge;
            else if ( bNationalGlossIndex() )
                pszGlossMarker = psz_gn;
            else if ( bRegionalGlossIndex() )
                pszGlossMarker = psz_gr;
            const CMarker* pmkrGloss = pmkrset->pmkrSearch(pszGlossMarker);
            if ( pmkrGloss && pmkrGloss->plng() != pmkrPriKey->plng() )
                {
				sMessage = _("Reversal and gloss must be same language encoding"); // 1.5.0fg 
                return FALSE;
                }

            psrtPriKey = pmkrPriKey->plng()->psrtDefault();
            const char pszSecKeyMarker[] = "ps";
            pmkrSecKey = pmkrset->pmkrSearch(pszSecKeyMarker);
            if ( !pmkrSecKey )
                {
                return FALSE;
                }

            mrflstSecKeys.AddMarkerRef(pmkrSecKey);
            // 1997-09-26 MRP: mdflist1.cct now preserves \ps and \pn
            // distinction so that user can translate \ps using the
            // national labels change table or use explicit \pn fields
            // in the lexical database file. So we sort by either field.
            if ( bNational() )
                {
                const char pszTerKeyMarker[] = "pn";
                CMarker* pmkrTerKey = pmkrset->pmkrSearch(pszTerKeyMarker);
                if ( !pmkrTerKey )
                    {
                    return FALSE;
                    }
    
                mrflstSecKeys.AddMarkerRef(pmkrTerKey);
                }
            }
        }
    else  // Dictionary
        {
        if ( m_bSortByCitationForm )  // 1997-09-26 MRP
            {
            const char psz_lc[] = "lc";
            pmkrPriKey = pmkrset->pmkrSearch(psz_lc);
            if ( !pmkrPriKey )
                {
                return FALSE;
                }

            psrtPriKey = pmkrPriKey->plng()->psrtDefault();
            }
        else
            {
            pmkrPriKey = pindRecordOwner->pmkrPriKey();
            psrtPriKey = pindRecordOwner->psrtPriKey();
            }
        const char pszSecKeyMarker[] = "hm";
        pmkrSecKey = pmkrset->pmkrSearch(pszSecKeyMarker);
        if ( !pmkrSecKey )
            {
            return FALSE;
            }

        mrflstSecKeys.AddMarkerRef(pmkrSecKey);
        }

    CIndex* pind = indset.pindFind_AddIfNew(pmkrPriKey, psrtPriKey,
        &mrflstSecKeys, 0, FALSE, NULL);

#ifdef HTML_LINK_SORT
	// 1998-10-26 MRP
    const CMarker* pmkrLink = pmkrset->pmkrSearch("lc");
    if ( pmkrLink )
    	g_psrtHTML = pmkrLink->plng()->psrtDefault();
    else
    	g_psrtHTML = pindRecordOwner->psrtPriKey();
#endif

    // Write the sorted records and make formatting changes

    // 1999-03-10 MRP: Customized versions of the change tables.
    Str8 sTableFileName = sFileNameChangeTable2();
    Str8 sTablePath = sChangeTablePath(sTableFileName);

    if ( !bFileExists(sTablePath) )
        {
        int iRemoved = remove(sTempPath);
		sMessage = sMessage + _("Cannot find file:") + " " + sTablePath; // 1.5.0fd 
        return FALSE;
        }

	// 1998-09-16 MRP: Output each section as a separate file.
	ofstream* piosOutput = NULL;
	Change_ostream* pccsOutput = NULL;
	SF_ostream* psfsOutput = NULL;

    // 1999-01-21 MRP: Via the Exported File dialog box,
    // the user has already selected the appropriate subfolder
    // for the audience. Unless this is a user-defined index,
    // there is a fixed name for the subfolder that contains
    // the exported web pages. This is necessary for the
    // hypertext links to work right.
	Str8 sDirPath = sGetDirPath(sOutputPath);
    BOOL bSubDir = !( bFinderlist() && m_bUserDefined );
    if ( bSubDir )
        {
        Str8 sSubDir;
        if ( m_iFormat == 0 || m_iFormat == 1 )
            sSubDir = "Dict";
        // 1999-03-15 MRP: Finderlist language is independent of audience
        else if ( m_iFormat == 2 )
            sSubDir = "ge";
        else if ( m_iFormat == 3 )
            sSubDir = "gn";
        else
            sSubDir = "gr";
        sDirPath = sPath(sDirPath, sSubDir);
        }
	Str8 sPathOut;
    
    // Order ordPrev = CMCharOrder::ordSecOnly;
    const CMChar* pmchPrev = NULL;  // 2000-04-17 MRP
    CLangEnc* plngPriKey = pmkrPriKey->plng();

    long lWritten = 0;
    const CRecLookEl* prelPrev = NULL;
    const CRecLookEl* prel = pind->prelFirst();
    for ( ; prel; prelPrev = prel, prel = pind->prelNext(prel) )
        {
        BOOL bWrite = FALSE;

        // 1998-12-04 MRP: User-defined indexing
        if ( bFinderlist() && m_bUserDefined && m_bIndexingCategories )
            {
            if ( prel->pfldPriKey() == NULL )
                continue;

            const Str8& sPriKey = prel->pfldPriKey()->sContents();  // Trim spaces!!!

            BOOL bEqual = FALSE;
            if ( prelPrev )
                {
                int i = pind->iCompare(prel, prelPrev,
                    CMCharOrder::matsetExactDisregardCase, TRUE);
                bEqual = ( i == 0 );
                }
            if ( !bEqual )
                {
                // If there was a file open, close it
                if ( psfsOutput )
                    {
                    delete psfsOutput;
                    delete pccsOutput;
				    BOOL bWriteFailed = piosOutput->fail();       
				    piosOutput->close();  // Flushes any waiting output
				    if ( bWriteFailed || piosOutput->fail() )
					    {
					    int iRemoved = remove(sTempPath);
						sMessage = sMessage + _("Cannot write file:") + " " + sTempPath; // 1.5.0fd
					    return FALSE;
					    }
				    delete piosOutput;

                    // 1999-03-02 MRP: Convert |fvlink{...} in dictionary web pages
                    if ( !bFormatHTML(sTempPath, ptyp, sPathOut, sProjectPath, sMessage) )
					    return FALSE;
                    }

                sPathOut = sPath(sDirPath, sPriKey, ".html");

                // 1998-10-16 MRP: Confirm overwriting the first web page???
			    piosOutput = new ofstream(sTempPath);
			    pccsOutput = new Change_ostream(sTablePath, *piosOutput);
			    if ( pccsOutput->fail() )
				    {
				    int iRemoved = remove(sTempPath);
				    return FALSE;
				    }				        
                // 1998-12-07 MRP: Trim surrounding WS
			    psfsOutput = new SF_ostream(*pccsOutput, TRUE);
            
			    // Write the formatting options
			    if ( bNational() )
				    psfsOutput->WriteInternalField(psz_National, "");
			    if ( bFinderlist() )
                    {
                    // 1999-01-25 MRP: Finderlist language independent of audience
                    if ( bEnglishGlossIndex() )
                        psfsOutput->WriteInternalField(psz_EnglishGlossIndex, "");
                    if ( bNationalGlossIndex() )
                        psfsOutput->WriteInternalField(psz_NationalGlossIndex, "");
                    if ( bRegionalGlossIndex() )
                        psfsOutput->WriteInternalField(psz_RegionalGlossIndex, "");
                    // 1998-12-04 MRP: User-defined index
                    if ( !m_sOption.IsEmpty() )
                        psfsOutput->WriteInternalField(m_sOption, "");
                    }
                else
				    {
				    if ( bTriglot() )
					    psfsOutput->WriteInternalField(psz_Triglot, "");
				    if ( !m_bSortByCitationForm )
					    psfsOutput->WriteInternalField(psz_SortOnlyByLexeme, "");
				    if ( m_bIncludeLexemeWithCitationForm )
					    psfsOutput->WriteInternalField(psz_IncludeLexemeWithCitationForm, "");
				    }
			    WriteHierarchy(*psfsOutput);  // 1998-04-15 MRP
                if( !m_sCopyright.IsEmpty() )  // 1998-12-05 MRP
                    psfsOutput->WriteInternalField(psz_copyright, m_sCopyright);
                }

            bWrite = TRUE;
            }
        else
            {
            // Output the \LP Letter Paragraph field 
            // 2000-04-17 MRP: Rewrite to account for changes made to the sort key
            // using a CCT or DLL (optional step for non-Roman scripts).
            const CField* pfldHeading = prel->pfldPriKey();
            ASSERT( pfldHeading );
            const Str8& sHeading = pfldHeading->sContents();
            const CMChar* pmch = NULL;
            if ( !psrtPriKey->bFirstPrimary(sHeading, &pmch) )
                pmch = psrtPriKey->plngMyOwner()->pmchSearch(" ");
            if ( pmch != pmchPrev )
                {
                pmchPrev = pmch;  // 2000-04-17 MRP: Remember before resetting
                // If there was a file open, close it
                if ( psfsOutput )
                    {
                    delete psfsOutput;
                    delete pccsOutput;
					BOOL bWriteFailed = piosOutput->fail();       
					piosOutput->close();  // Flushes any waiting output
					if ( bWriteFailed || piosOutput->fail() )
					    {
					    int iRemoved = remove(sTempPath);
						sMessage = sMessage + _("Cannot write file:") + " " + sTempPath; // 1.5.0fd
					    return FALSE;
					    }
					delete piosOutput;

                    // 1999-03-02 MRP: Convert |fvlink{...} in dictionary web pages
					if ( !bFormatHTML(sTempPath, ptyp, sPathOut, sProjectPath, sMessage) )
					    return FALSE;
                    }

                const CMChar* pmchUpper = (pmch->bUpperCase() ?
                    pmch :
                    pmch->pmchOtherCase()
                    );
                if ( pmch->bUpperCase() )
                    pmch = pmch->pmchOtherCase();
                Str8 sLetter;
                if ( pmchUpper )
                    {
                    sLetter = pmchUpper->sMChar();
                    sLetter += "  -  ";  // String will become an option
                    }
                ASSERT( pmch );
                sLetter += pmch->sMChar();

                // Open a new file for this section.
                // Note: ofstream failures are detected when it is closed.
                Str8 sFileName = pmch->sMChar();
                // 2000-04-20 MRP: Space cannot be used as the file name;
                // however, it is a section heading only if it was preceded
                // in the field by undefined characters, e.g., \gn ??? alerte
                if ( bEqual(sFileName, " ") )
                    {
                    sFileName = "undefined";
                    sLetter = "";  // 2000-07-24 MRP
                    }
                Str8 sConvertedFileName = HTML_ostream::s_sConvertURL(sFileName);
                // 1998-12-02 MRP: html
                sPathOut = sPath(sDirPath, sConvertedFileName, ".html");
                // 1998-10-16 MRP: Confirm overwriting the first web page???
				piosOutput = new ofstream(sTempPath);
				pccsOutput = new Change_ostream(sTablePath, *piosOutput);
				if ( pccsOutput->fail() )
				    {
				    int iRemoved = remove(sTempPath);
				    return FALSE;
				    }				        
                // 1998-12-07 MRP: Trim surrounding WS
				psfsOutput = new SF_ostream(*pccsOutput, TRUE);
            
				// Write the formatting options
				if ( bNational() )
					psfsOutput->WriteInternalField(psz_National, "");
				if ( bFinderlist() )
                    {
                    // 1998-12-04 MRP: User-defined index
                    if ( !m_sOption.IsEmpty() )
                        psfsOutput->WriteInternalField(m_sOption, "");
                    }
                else
					{
					if ( bTriglot() )
					    psfsOutput->WriteInternalField(psz_Triglot, "");
					if ( !m_bSortByCitationForm )
					    psfsOutput->WriteInternalField(psz_SortOnlyByLexeme, "");
					if ( m_bIncludeLexemeWithCitationForm )
					    psfsOutput->WriteInternalField(psz_IncludeLexemeWithCitationForm, "");
					// if ( bAlternateHierarchy() )
					//     psfsOutput->WriteInternalField(psz_AlternateHierarchy, "");
					}
				WriteHierarchy(*psfsOutput);  // 1998-04-15 MRP
				// if ( !m_bDoubleColumn )
                if( !m_sCopyright.IsEmpty() )  // 1998-12-05 MRP
                    psfsOutput->WriteInternalField(psz_copyright, m_sCopyright);

                psfsOutput->WriteField("LP", sLetter);  // Letter Paragraph
                }
            bWrite = TRUE;
            }

		if ( bWrite )        
        	pind->WriteElement(*psfsOutput, m_pexpsetMyOwner->ptypMyOwner(), prel); // 1.4qpb
//        (void) prg.bUpdateProgress(++lWritten);
        }

    if ( psfsOutput )
    	{
    	delete psfsOutput;
    	delete pccsOutput;
	    BOOL bWriteFailed = piosOutput->fail();       
	    piosOutput->close();  // Flushes any waiting output
	    if ( bWriteFailed || piosOutput->fail() )
	        {
	        int iRemoved = remove(sTempPath);
			sMessage = sMessage + _("Cannot write file:") + " " + sTempPath; // 1.5.0fd
	        return FALSE;
	        }
	    delete piosOutput;

        // 1999-03-02 MRP: Convert |fvlink{...} in dictionary web pages
	    if ( !bFormatHTML(sTempPath, ptyp, sPathOut, sProjectPath, sMessage) )
	        return FALSE;
    	}
    
    return TRUE;
}  // CExportProcessMDF::bSortAndFormatHTML

BOOL CExportProcessMDF::bFormatHTML(const Str8& sTempPath, CDatabaseType* ptyp,
        const Str8& sOutputPath,
        const Str8& sProjectPath, Str8& sMessage) const
{
    // 1999-03-02 MRP: Convert |fvlink{...} only in dictionary web pages.
    if ( !bFinderlist() )
        {
        if ( !bFileExists(s_sChangePathWebPageLinks) )
            {
            int iRemoved = remove(sTempPath);
			sMessage = sMessage + _("Cannot find file:") + " " + s_sChangePathWebPageLinks; // 1.5.0fd 
            return FALSE;
            }

        ChangeTable cctWebPageLinks;
        if ( !cctWebPageLinks.bLoadFromFile(s_sChangePathWebPageLinks) )
			{
			int iRemoved = remove(sTempPath);
			return FALSE;
			}				        

        // 1999-03-02 MRP: Use ASSERT below until I can determine
        // a repeatable way to trigger the following failure condition.
        int i = rename(sTempPath, s_sTempPathWebPageLinks);
        ASSERT( i == 0 );
        BOOL b = cctWebPageLinks.bMakeChanges(s_sTempPathWebPageLinks, sTempPath);
        ASSERT( b );
        i = remove(s_sTempPathWebPageLinks);
        ASSERT( i == 0 );
        }
				
	return bFormat(sTempPath, ptyp, sOutputPath, sProjectPath, sMessage);
}
#endif  // EXPORT_WEB_PAGES

BOOL CExportProcessMDF::bFormat(const Str8& sTempPath, CDatabaseType* ptyp,
        const Str8& sOutputPath,
        const Str8& sProjectPath, Str8& sMessage) const
{
    std::ifstream iosIn(sTempPath);
    if ( iosIn.fail() )
        {
        int iRemoved = remove(sTempPath);
		sMessage = sMessage + _("Cannot open file:") + " " + sTempPath; // 1.5.0fd 
        return FALSE;
        }

    // 1999-03-10 MRP: Customized versions of the change tables.
    Str8 sTableFileName = sFileNameChangeTableLabels();
    Str8 sTablePath = sChangeTablePath(sTableFileName);
    if ( !bFileExists(sTablePath) )
        {
        int iRemoved = remove(sTempPath);
		sMessage = sMessage + _("Cannot find file:") + " " + sTablePath; // 1.5.0fd 
        return FALSE;
        }

    Change_istream ccsIn( sTablePath, iosIn );
    if ( ccsIn.fail() )
        {
        int iRemoved = remove(sTempPath);
        return FALSE;
        }

    // 1998-06-10 MRP: Only prepend the path to the settings folder
    // if the document template doesn't already have an explicit path.
    Str8 sPathDOT = ( bNational() ? m_sNationalDot : m_sEnglishDot );
	if ( !m_bWebPages )  // 1998-09-21 MRP
		{
	    if ( sGetDirPath(sPathDOT).IsEmpty() )
	        sPathDOT = sPath(sProjectPath, sPathDOT);
	    if ( !bFileExists(sPathDOT) )
	        {
	        int iRemoved = remove(sTempPath);
			sMessage = sMessage + _("Cannot find file:") + " " + sPathDOT; // 1.5.0fd 
	        return FALSE;
	        }
	    }

    CMarkerSet* pmkrset = ptyp->pmkrset();
    CMarker* pmkrRecord = ptyp->pmkrRecord();
    SF_istream sfsIn(ccsIn, pmkrset, pmkrRecord, FALSE);

    std::ofstream iosOut(sOutputPath);  // We check for ofstream failures below
	Field_ostream* pfosOut = NULL;  // 1998-09-21 MRP
	if ( m_bWebPages )
#ifdef EXPORT_WEB_PAGES
		pfosOut = new HTML_ostream(iosOut, pmkrset, m_bHTML_XML, m_bUTF8,
			m_aasTitle[m_iAudience][m_iFormat],
			// 1998-11-21 MRP
			"Dict",  // 1999-04-02 MRP: Change from dict to Dict
			bNational() ? "../MDF_n.css" : "../MDF_e.css",  // 1998-10-25 MRP
            m_sCopyright,  // 1998-12-11 MRP
            "Multi-Dictionary Formatter",  // 1999-02-04 MRP: generator
            TRUE);  // 1999-02-15 MRP: bNoRobots
#else
		ASSERT( FALSE );  // 1998-12-02 MRP: Disable in 16-bit version
#endif  // EXPORT_WEB_PAGES
	else
		{
	    CMarker* pmkrHeadword = NULL;
	    Str8 sHeaderLeft;
	    Str8 sHeaderRight;
	    if ( bFinderlist() )
	        {
            // 1999-01-25 MRP: Finderlist language is independent of audience
	        if ( bEnglishGlossIndex() )
	            pmkrHeadword = pmkrset->pmkrSearch("HE");  // Headword (E)
	        else if ( bNationalGlossIndex() )
	            pmkrHeadword = pmkrset->pmkrSearch("HN");  // Headword (n)
	        else if ( bRegionalGlossIndex() )
	            pmkrHeadword = pmkrset->pmkrSearch("HR");  // Headword (r)
            else
                ASSERT( FALSE );
	        }
	    else  // Dictionary
	        pmkrHeadword = pmkrset->pmkrSearch("lx");
	    if ( m_bHeader && pmkrHeadword )
	        {
	        const Str8& sHeadwordStyleName = pmkrHeadword->sFieldName();
	        sHeaderLeft = "{STYLEREF \"";
	        sHeaderLeft += sHeadwordStyleName;
	        sHeaderLeft += "\"}";
	        sHeaderRight = "{STYLEREF \"";
	        sHeaderRight += sHeadwordStyleName;
	        sHeaderRight += "\"\\l}";
	        }
	    // int nCols = (m_bDoubleColumn ? 2 : 1);
	    int nCols = m_ppgs->numColumns();
	    pfosOut = new RTF_ostream(iosOut, pmkrset,
	        m_pexpsetMyOwner->bAutoUpdateStyles(), FALSE,
	        FALSE, 0,  // 1998-07-11 MRP
	        sHeaderLeft, sHeaderRight,
	        m_aasTitle[m_iAudience][m_iFormat], m_bDate,
	        sPathDOT, m_ppgs);
	        // nCols, 360, 540, 1260, m_ppgs);
	    }
	ASSERT( pfosOut );
        
    {
    LONG lSizFile = lFileSize(sTempPath);
    CProgressIndicator prg(lSizFile, NULL, sOutputPath); // 1.4ad Eliminate resource messages sent to progress bar
    while ( !sfsIn.bAtEnd() )
        {
        CField* pfld = NULL;
        if ( !sfsIn.bReadField(&pfld) )
            {
            delete pfosOut;
            return FALSE;
            }
        pfosOut->WriteField(pfld, FALSE);
        delete pfld;
		LONG lPos = static_cast<LONG>(iosIn.tellg());
        (void) prg.bUpdateProgress(lPos);
        }
    }  // Cause progress indicator to be destroyed

	delete pfosOut;  // 1998-09-21 MRP

#ifdef _MAC
	if ( !m_bWebPages )
		{
        // 1997-04-01 MRP: Must set creator BEFORE closing the file
        // in order to set it to MSWD instead of SHBX, it seems.
        SetFileTypeRTF(sOutputPath); // set file type and creator
        }
#endif

    BOOL bWriteFailed = iosOut.fail();      
    iosOut.close();  // Flushes any waiting output
    if ( bWriteFailed || iosOut.fail() )
        {
        int iRemoved = remove(sTempPath);
        iRemoved = remove(sOutputPath);
		sMessage = sMessage + _("Failed to write file:") + " " + sOutputPath; // 1.5.0fd
        return FALSE;
        }

    // 1997-12-10 MRP: Win and Mac require that the stream
    // be closed before removing the file, although Win16 doesn't.
    iosIn.close();
	if ( !bDebugKaren ) // Allow Ctrl+Alt+Shift+K to keep chained export files // 1.5.9b 
    int iRemoved = remove(sTempPath);
    
    return TRUE;
}  // CExportProcessMDF::bFormat

void CExportProcessMDF::FilterUpdated(CFilterUpdate& fup)
{
    if ( m_pfil == fup.pfil() && fup.bToBeDeleted() )
        m_pfil = NULL;
}

BOOL CExportProcessMDF::bCopy(CSetEl** ppselNew)
{
    CMDFDlg dlgMDF(this, (CExportProcessMDF**)ppselNew);
    return (dlgMDF.DoModal() == IDOK);
}

BOOL CExportProcessMDF::bModify()
{
    CMDFDlg dlgMDF(this, TRUE);
    return (dlgMDF.DoModal() == IDOK);
}

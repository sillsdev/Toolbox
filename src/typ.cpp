// typ.cpp  Implementation of Standard Format text markup type (1995-08-10)

#include "stdafx.h"
#include "toolbox.h"
#include "shw.h"
#include "typ.h"
#include "tools.h"  // sPath
#include "font.h"
#include "spath_d.h"
#include "project.h"
#include <fstream>  // ifstream
#include "mdf.h"  // CExportProcessMDF
#include "pgs.h"  // RtfPageSetup
#include "obstream.h"  // Object_istream, Object_ostream
#include "export.h"  // CExportProcessSet

#include "typ_d.h"  // CDatabaseTypeSheet
#include "lbo.h"  // CSetDlg

//11-08-1997
#include "nlstream.h"

#include "dirlist.h"

//#ifdef _DEBUG
#include <sstream>
//#endif  // _DEBUG


// **************************************************************************

CDatabaseType::CDatabaseType(CLangEncSet* plngset,
        CDatabaseTypeProxy* ptrxMyProxy, BOOL bNew) :
    m_mkrset(plngset),
    m_filset(&m_mkrset),
    m_ptrxMyProxy(ptrxMyProxy),
    m_bModified(bNew),
    m_jmpset(&m_mkrset)
{
    ASSERT( m_ptrxMyProxy );
    m_pmkrRecord = NULL;
    m_pmkrDateStamp = NULL;
    m_pmkrTextRef = NULL; // 1.1ma Add text ref marker to database type
    m_intprclst.SetOwner( this );
    m_mkrset.SetOwner( this );
    m_iInterlinCharWd = 8; // Give Char width a default in case not in settings file
    m_pexpset = new CExportProcessSet(this, TRUE);  // 1997-11-18 MRP
	m_bPreventNumbering = FALSE;
	m_bNumberingDisabled = FALSE;
	m_pnbrproc = NULL;
	m_bHideFields = FALSE;
}

CDatabaseType::~CDatabaseType()
{
    delete m_pexpset;  // 1997-11-15 MRP
	delete m_pnbrproc;  // 1999-02-03 TLB
}

CDatabaseTypeSet* CDatabaseType::ptypsetMyOwner() const
{
    return ptrxMyProxy()->ptypsetMyOwner();
}

static const char* psz_DatabaseTypeExt = ".typ"; 
static const char* psz_DatabaseType = "DatabaseType";
static const char* psz_ver = "ver";
static const char* psz_desc = "desc";
static const char* psz_Template = "template";
static const char* psz_fld = "fld";
static const char* psz_mkrDateStamp = "mkrDateStamp";
static const char* psz_mkrTextRef = "mkrTextRef";
static const char* psz_Default = "Default";
static const char* psz_iInterlinCharWd = "iInterlinCharWd";
static const char* psz_mkrMultipleMatchShow = "mkrMultipleMatchShow";
static const char* psz_mkrRecord = "mkrRecord";
static const char* psz_bPreventNumbering = "PreventNumbering";
static const char* psz_bDisableNumbering = "DisableNumbering";
static const char* psz_bHideFields = "HideFields";
static const char* psz_bTextFile = "TextFile";
static const char* psz_sPlaySoundPath = "PlaySoundPath"; // 1.5.0hc 

static const char* psz_SecKey = "SecKey";

BOOL CDatabaseType::bWriteProperties(const char* path, BOOL bForceWrite) const
{
    if (Shw_pProject()->bExerciseNoSave() && !bForceWrite)
        return TRUE;

#ifdef typWritefstream
    FILE* file = nullptr;
    if (_wfopen_s(&file, swUTF16(path), L"w") != 0 || !file)
        return FALSE;
    Object_ofstream obs(file);
    WriteProperties(obs);
    return fflush(file) == 0 && !ferror(file);
#else
    std::ofstream file(path);
    if (!file)
        return FALSE;
    Object_ostream obs(file);
    WriteProperties(obs);
    file.flush();
    return file.good();
#endif
}
void CDatabaseType::WritePaths( std::ofstream& ostr ) // Update paths that point to the project if the project moved
{
	m_intprclst.WritePaths( ostr ); // Update interlinearization references
	m_pexpset->WritePaths( ostr ); // 1.4pr Include export process files in list of active files // Update export process references // 5.98t Don't write these, they are external and tend to be provided, and there can be lots of them
}

#ifdef typWritefstream // 1.6.4ac
void CDatabaseType::WriteProperties(Object_ofstream& obs) const
#else
void CDatabaseType::WriteProperties(Object_ostream& obs) const
#endif
{
    const Str8& sName = m_ptrxMyProxy->sName();
    obs.WriteBeginMarker(psz_DatabaseType, sName);

    // 1998-03-12 MRP: Always write the current version
    obs.WriteString(psz_ver, ptypsetMyOwner()->sSettingsVersion());
    obs.WriteBool("ProjectRelativePaths", TRUE);

    obs.WriteString(psz_desc, m_sDescription);
    m_mkrset.WriteProperties(obs);
    obs.WriteInteger( psz_iInterlinCharWd, m_iInterlinCharWd );
    m_intprclst.WriteProperties(obs);
    m_filset.WriteProperties(obs);
    m_jmpset.WriteProperties(obs);
    TemplateWriteProperties(obs);
    obs.WriteString(psz_mkrRecord, m_pmkrRecord->sName());
    const CMarkerRef* pmrf = m_mrflstSecKeys.pmrfFirst();
    for ( ; pmrf; pmrf = m_mrflstSecKeys.pmrfNext(pmrf) )
        pmrf->pmkr()->WriteRef(obs, psz_SecKey);
    if ( m_pmkrDateStamp )
        obs.WriteString(psz_mkrDateStamp, m_pmkrDateStamp->sName());
    if ( m_pmkrTextRef )
        obs.WriteString(psz_mkrTextRef, m_pmkrTextRef->sName());
    // 2000-06-22 MRP: Use a Str8 instead of a CMarkerPtr,
    // so that users are not prevented from deleting the marker.
    if ( m_mkrset.pmkrSearch(m_sMarkerMultipleMatchShow) )
        obs.WriteString(psz_mkrMultipleMatchShow, m_sMarkerMultipleMatchShow);
    m_prtp.WriteProperties(obs);

    ASSERT( m_pexpset );
    m_pexpset->WriteProperties(obs);  // 1997-11-15 MRP

	obs.WriteBool(psz_bPreventNumbering, m_bPreventNumbering);
	if ( m_bNumberingDisabled )
		obs.WriteBool(psz_bDisableNumbering, m_bNumberingDisabled);
	else if ( m_pnbrproc )
		m_pnbrproc->WriteProperties(obs);  // 1999-02-03 TLB
	obs.WriteBool(psz_bHideFields, m_bHideFields); // 1.1ef
    obs.WriteProjectPath(psz_sPlaySoundPath, m_sPlaySoundPath);
	obs.WriteUnrecognizedSettingsInfo( m_sUnrecognizedSettingsInfo ); // 1.0cp Write unrecognized settings info

    obs.WriteEndMarker(psz_DatabaseType);
}

#ifdef typWritefstream // 1.6.4ac
void CDatabaseType::TemplateWriteProperties(Object_ofstream& obs) const
#else
void CDatabaseType::TemplateWriteProperties(Object_ostream& obs) const
#endif
{
    obs.WriteBeginMarker(psz_Template);
    
    Str8 sTmp = m_sTemplate;
    while ( sTmp.GetLength() )
        {
        Str8 sFld;
        int iIdx = sTmp.Find( Str8(pNL) + "\\"); // find start of next field
        if ( iIdx >= 0 )
            {
            sFld = sTmp.Left( iIdx ); // grab a copy of this field
            sTmp = sTmp.Mid( iIdx+strlen(pNL) ); // throw away the \r\n and the field just found
            }
        else // must be last field
            {
            sFld = sTmp;
            sTmp = "";
            }

#ifdef _MAC
        const char* p = sFld;
        for ( p = strchr( p+1, '\n' ); p; p = strchr( p+1, '\n' ) )
            sFld = sFld.Left(p-sFld) + "\\n" + sFld.Mid(p-sFld+1);
#else
        const char* p = sFld; // convert \r\n to \\n in field contents
        for ( p = strstr( p+1, pNL ); p; p = strstr( p+1, pNL ) )
            {
            int i = p - sFld;
            sFld.SetAt( i, '\\' );
            sFld.SetAt( i+1, 'n' );
            }
#endif
        obs.WriteString(psz_fld, sFld);
        }

    obs.WriteEndMarker(psz_Template);
}

BOOL CDatabaseType::bReadProperties(CNoteList& notlst, BOOL bLocateMissing)
{
    const Str8& sPath = m_ptrxMyProxy->sPath();
	if ( !bAllASCIIComplain( sPath ) ) // 1.4vze 
		return FALSE; // 1.4vze 
	std::ifstream ios(sPath);
	if (!ios)
        return FALSE;

    //11-08-1997
    //The class Newline_istream provides data from both Mac and PC
    Newline_istream iosnl(ios);
        
    Object_istream obs(iosnl, notlst);
    obs.SetProjectPathContext(TRUE);
    obs.SetLocateMissingReferences(bLocateMissing);
    return bReadProperties(obs);
}

BOOL CDatabaseType::bReadProperties(Object_istream& obs)
{
    if ( !obs.bAtBackslash() || !obs.bReadBeginMarker(psz_DatabaseType) )
        return FALSE;

    BOOL bRelativePaths = FALSE;
    Str8 s;
    m_iInterlinCharWd = 8; // Give a default in case not in the settings file
    Str8 sRecordMarker;
    CMarker* pmkrSecKey = NULL;
    COldExportProcessSet* pexpsetOld = new COldExportProcessSet(this);
    delete m_pexpset;
    m_pexpset = NULL;
    CDatabaseRefList drflstOldJumpPath;
    while ( !obs.bAtEnd() )
        {
        if ( obs.bReadString(psz_ver, s) )
            ;
        else if ( obs.bReadBool("ProjectRelativePaths", bRelativePaths) )
            obs.SetProjectPathContext(FALSE);
        else if ( obs.bReadString(psz_desc, m_sDescription) )
            ;
        else if ( m_mkrset.bReadProperties(obs) )
            ;
        // 3/30/2000 TLB : Now catch any new-style multi-word range sets or empty range sets.
        // These are written wrapped in \+rngset ... \-rngset form so that older
        // versions of the program will ignore them.
        else if ( CRangeSet::s_bReadProperties(obs, &m_mkrset) )
            ;
        else if ( obs.bReadInteger( psz_iInterlinCharWd, m_iInterlinCharWd ) )
            ;
        else if ( m_intprclst.bReadProperties(obs) )
            ;
        else if ( m_filset.bReadProperties(obs) )
            ;
        else if ( m_jmpset.bReadProperties(obs) )
            ;
        else if ( drflstOldJumpPath.bReadProperties(obs) )
            ;
        else if ( m_prtp.bReadProperties(obs) )
            ;
        else if ( bTemplateReadProperties(obs) )
            ;
        else if ( obs.bReadString( psz_mkrDateStamp, s ) )
            m_pmkrDateStamp = m_mkrset.pmkrSearch_AddIfNew( (const char*)s );
        else if ( obs.bReadString( psz_mkrTextRef, s ) )
            m_pmkrTextRef = m_mkrset.pmkrSearch_AddIfNew( (const char*)s );
        else if ( obs.bReadString( psz_mkrMultipleMatchShow, m_sMarkerMultipleMatchShow ) )
            ;  // 2000-06-22 MRP
        else if ( obs.bReadString( psz_mkrRecord, sRecordMarker ) )
            ;
        else if ( m_mkrset.bReadMarkerRef(obs, psz_SecKey, pmkrSecKey) )
            {
            if ( pmkrSecKey )
                m_mrflstSecKeys.AddMarkerRef(pmkrSecKey);
            pmkrSecKey = NULL;
            }
        else if ( CExportProcessSet::s_bReadProperties(obs, this, &m_pexpset) )
            ;
		else if ( obs.bReadBool(psz_bPreventNumbering, m_bPreventNumbering) )
			;
		else if ( obs.bReadBool(psz_bDisableNumbering, m_bNumberingDisabled) )
			;
        else if ( CNumberProcess::s_bReadProperties(obs, this, &m_pnbrproc) )
            ;
        else if ( pexpsetOld->bReadProperties(obs) )
            ;
		else if ( obs.bReadBool(psz_bHideFields, m_bHideFields) )
			;
        else if ( obs.bReadProjectPath(psz_sPlaySoundPath, m_sPlaySoundPath) )
            ;
		else if ( obs.bUnrecognizedSettingsInfo( psz_DatabaseType, m_sUnrecognizedSettingsInfo  ) ) // 1.0cp Store any unrecognized settings info so as not to lose it
			;
        else if ( obs.bEnd( psz_DatabaseType ) )
            break;       
        }
    SetInterlinearMkrs(); // Set up interlinear properties of markers
    if ( sRecordMarker.IsEmpty() ) // old settings file or user somehow trashed record marker setting
        {
        CDbTypRecMkrDlg dlg(this);
        dlg.DoModal(); // get record marker
        }
    else
        SetRecordMarker( sRecordMarker );  // 1996-11-05 MRP
    ASSERT( m_pmkrRecord );
    
    // Upgrade old Sh2 Key-String conditions to Sh3 Marker-Text elements
    m_filset.SetRecordMarker(m_pmkrRecord);  // 1996-08-28 MRP
        
    if ( m_pexpset )
        delete pexpsetOld;
    else
        {
        pexpsetOld->ConvertOldProperties();
        m_pexpset = pexpsetOld;
        }

	m_bNumberingDisabled |= m_bPreventNumbering;

    // Take care of old-style jump path - 1999-05-05 TLB
    if ( !drflstOldJumpPath.bIsEmpty() )
        {
        m_jmpset.bAddDefaultJumpPath( drflstOldJumpPath );
        }

	char cForcedGlossStart = pintprclst()->cForcedGlossStartChar(); // 1.5.3r If forced gloss chars not in punc, add them
	char cForcedGlossEnd = pintprclst()->cForcedGlossEndChar(); // 1.5.3r 
	CInterlinearProc* pintprc = pintprclst()->pintprcFirst(); // 1.5.3r 
	if ( pintprc && pintprc->bParseProc() ) // 1.5.3r 
		{
		CLangEnc* plng = pintprc->pmkrFrom()->plng(); // 1.5.3r 
		Str8 sPunct = plng->sPunct(); // 1.5.3r 
		if ( sPunct != "" ) // 1.5.3r If punctuation list
			{
			if ( sPunct.Find( cForcedGlossStart ) < 0 ) // 1.5.3r If forced gloss char not in punc, add it
				{
				plng->SetPunct( sPunct + " " + cForcedGlossStart ); // 1.5.3r 
				sPunct = plng->sPunct(); // 1.5.3r 
				}
			if ( sPunct.Find( cForcedGlossEnd ) < 0 ) // 1.5.3r If forced gloss char not in punc, add it
				plng->SetPunct( sPunct + " " + cForcedGlossEnd ); // 1.5.3r 
			}
		}
    
	return TRUE;
}

BOOL CDatabaseType::bTemplateReadProperties(Object_istream& obs)
{
    if ( !obs.bReadBeginMarker(psz_Template) )
        return FALSE;

    Str8 sFld;
    while ( !obs.bAtEnd() )
        {
        if ( obs.bReadString(psz_fld, sFld) )
            {
            if ( m_sTemplate.GetLength() ) // put a \r\n before all fields except first one
                m_sTemplate += pNL;
            const char* p = sFld;
            for ( p = strstr( p+1, "\\n" ); p; p = strstr( p+1, "\\n" ) )
                {
#ifdef _MAC
                sFld = sFld.Left(p-sFld) + pNL + sFld.Mid(p-sFld+2);
#else
                int i = p - sFld;
                sFld.SetAt( i, '\r' ); // convert \\n to \r\n
                sFld.SetAt( i+1, '\n' );
#endif
                }
            m_sTemplate += sFld;
            }
        else if ( obs.bEnd(psz_Template) )
            break;      
        }

    return TRUE;
}

BOOL CDatabaseType::bMultiDictionaryFormatter() const
{
    ASSERT( m_pexpset );
    return m_pexpset->bMultiDictionaryFormatter();
}

void CDatabaseType::SetMultiDictionaryFormatter(BOOL bMDF)
{
    ASSERT( m_pexpset );
    m_pexpset->SetMultiDictionaryFormatter(bMDF);
}

BOOL CDatabaseType::bExport(CIndex* pindCur, const CRecLookEl* prelCur,
        const Str8& sPathDatabase, Str8& sMessage)
{
    ASSERT( m_pexpset );
    return m_pexpset->bExport(pindCur, prelCur, sPathDatabase, sMessage);
}


// 2000-06-22 MRP: Use a Str8 instead of a CMarkerPtr,
// so that users are not prevented from deleting the marker.
void CDatabaseType::SetMultipleMatchMkr(CMarker* pmkr)
{
    if ( pmkr )
        m_sMarkerMultipleMatchShow = pmkr->sName();
    else
        m_sMarkerMultipleMatchShow.Empty();  // User selected [None] in the list
}

CMarker* CDatabaseType::pmkrMultipleMatchShow()
{
    CMarker* pmkr = m_mkrset.pmkrSearch(m_sMarkerMultipleMatchShow);
    if ( !pmkr && !m_sMarkerMultipleMatchShow.IsEmpty() )
        m_sMarkerMultipleMatchShow.Empty();  // User has deleted the marker

    return pmkr;
}


void CDatabaseType::DisableNumbering()
{
	m_bNumberingDisabled = TRUE;
    if ( m_pnbrproc ) // Need to delete numbering process object and "forget" settings
		{
		delete m_pnbrproc;
		m_pnbrproc = NULL;
		}
}

void CDatabaseType::SetupNumbering(CMarker* pmkrRef, CMarker* pmkrTxt)
{
	CNumberProcess::s_CreateInstance(this, &m_pnbrproc, pmkrRef, pmkrTxt);
}

BOOL CDatabaseType::bNumber(CShwView* pview, CRecLookEl* prelCur, BOOL bDoBreak)
{
	ASSERT ( !m_bNumberingDisabled );
    if ( !m_pnbrproc )  // User has never before done numbering with this DB type.
		{
		CNumberProcess::s_CreateInstance(this, &m_pnbrproc);
		if ( !m_pnbrproc ) // User cancelled out of DBT type sheet or failed to set up numbering
			return FALSE;
		}
	ASSERT ( pview );
    return m_pnbrproc->bNumber(pview, prelCur, bDoBreak);
}

void CDatabaseType::IncrementNumberOfRefs()
{
    m_ptrxMyProxy->IncrementNumRefs();
}

void CDatabaseType::DecrementNumberOfRefs()
{
    m_ptrxMyProxy->DecrementNumberOfRefs();
}

BOOL CDatabaseType::bUnicode() // Return true if db contains Unicode langs
{
	CMarker* pmkr = pmkrset()->pmkrFirst();
	for ( ; pmkr; pmkr = pmkrset()->pmkrNext( pmkr ) ) // For each marker
		if ( pmkr->plng()->bUnicodeLang() ) // If it is a Unicode lang, return TRUE
			return TRUE;
	return FALSE; // If no Unicode langs found, return FALSE
}


void CDatabaseType::WritePropertiesIfModified( BOOL bForceWrite)
{
    if ( !m_bModified && !bForceWrite )
        return;
        
    ASSERT( !m_ptrxMyProxy->ptypsetMyOwner()->bReadFromString() );
    const Str8& sPath = m_ptrxMyProxy->sPath();
    WriteProperties( sPath );
}

void CDatabaseType::Update(CUpdate& up)
{
    up.UpdateDatabaseType(this);
}

void CDatabaseType::MarkerUpdated(CMarkerUpdate& mup)
{
    m_bModified = m_bModified || mup.pmkr()->pmkrsetMyOwner() == pmkrset();
    m_intprclst.MarkerUpdated(mup); // notify interlinear processes
    m_jmpset.MarkerUpdated(mup); // notify jump path set
	if ( m_pnbrproc && mup.pmkr()->pmkrsetMyOwner() == pmkrset() )
		m_pnbrproc->MarkerUpdated(mup, &m_pnbrproc); // notify numbering process
}

void CDatabaseType::FilterUpdated(CFilterUpdate& fup)
{
    m_bModified = m_bModified || fup.pfil()->pfilsetMyOwner() == pfilset();
    m_pexpset->FilterUpdated(fup);
}

void CDatabaseType::LangEncUpdated(CLangEncUpdate& lup)
{
    if ( m_bModified || !lup.bNameModified() )
        return;
        
    const CLangEnc* plng = lup.plng();
    const CMarker* pmkr = m_mkrset.pmkrFirst();
    for ( ; pmkr; pmkr = m_mkrset.pmkrNext(pmkr) )
        if ( pmkr->plng() == plng )
            break;
    m_bModified = ( pmkr != NULL );
}

void CDatabaseType::SortOrderUpdated(CSortOrderUpdate& sup)
{
}

void CDatabaseType::IndexUpdated(CIndexUpdate& iup)
{
    m_intprclst.IndexUpdated(iup); // Send update info to all processes
}

void CDatabaseType::OnDocumentClose( CShwDoc* pdoc ) // NULL out indexes of jump paths referring to sDoc
{
    m_jmpset.OnDocumentClose( pdoc ); // notify jump path set that doc is closing
    m_intprclst.OnDocumentClose( pdoc ); // notify interlinear processes that doc is closing
}

BOOL CDatabaseType::bViewProperties(int iActivePage)
{
    CDatabaseTypeSheet dlg(this, iActivePage);
    return ( dlg.DoModal() == IDOK );  // show a modal view of the marker
}

BOOL CDatabaseType::bViewProperties(int iActivePage, CMarker* pmkr, CFilter* pfil)
{
    CDatabaseTypeSheet dlg(this, iActivePage, pmkr, pfil);
    return ( dlg.DoModal() == IDOK );  // show a modal view of the marker
}


// Cannot inline these because they contain forward references to member data
const Str8& CDatabaseType::sName() const
{
    return m_ptrxMyProxy->sName();
}

void CDatabaseType::SetRecordMarker( const char* pszMarker )
{
    ASSERT( !m_pmkrRecord );
    m_pmkrRecord = m_mkrset.pmkrSearch_AddIfNew( pszMarker );
    ASSERT( m_pmkrRecord );
    m_mkrset.SetRecordMarker( m_pmkrRecord );
}

void CDatabaseType::ChangeRecordMarker( const char* pszMarker )
{
    ASSERT( m_pmkrRecord );
    CMarker* pmkrRecordFrom = m_pmkrRecord;
    m_pmkrRecord = m_mkrset.pmkrSearch_AddIfNew( pszMarker );
    ASSERT( m_pmkrRecord );
    m_mkrset.ChangeRecordMarker( pmkrRecordFrom, m_pmkrRecord );
}


#ifdef _DEBUG
void CDatabaseType::AssertValid() const
{
    m_mkrset.AssertValid();
    m_filset.AssertValid();
}
#endif  // _DEBUG

// --------------------------------------------------------------------------
CDatabaseTypeRef::CDatabaseTypeRef( const char* psz ) // Constructor
{
    SetType( psz ); // Set type name and clear pointer
}

CDatabaseType* CDatabaseTypeRef::ptyp() // Return the type pointer, looking up the name if necessary
{
    CNoteList notlst;
    if ( !m_ptyp &&  !m_sType.IsEmpty() ) // If type not loaded and name available, look it up
        m_ptyp = Shw_ptypset()->ptypSearch_AddIfNew( m_sType, notlst );
    return m_ptyp; // Return type, possibly null
}

void CDatabaseTypeRef::SetType( const char* psz ) // Set the type name and clear the pointer
{
    if ( strcmp( m_sType, psz ) ) // Don't change and null out pointer if same as before
        {
        m_sType = psz;
        m_ptyp = NULL;
        }
}

// --------------------------------------------------------------------------
void CDatabaseTypePtr::IncrementNumberOfRefs(CDatabaseType* ptyp)
{
    m_ptyp = ptyp;
    if ( m_ptyp )
        m_ptyp->IncrementNumberOfRefs();
}

void CDatabaseTypePtr::DecrementNumberOfRefs()
{
    if ( m_ptyp )
        m_ptyp->DecrementNumberOfRefs();
}


const CDatabaseTypePtr& CDatabaseTypePtr::operator=(CDatabaseType* ptyp)
{
    if ( m_ptyp != ptyp )
        {
        DecrementNumberOfRefs();
        IncrementNumberOfRefs(ptyp);
        }
        
    return *this;
}


// --------------------------------------------------------------------------

CDatabaseTypeProxy::CDatabaseTypeProxy(const char* pszName,
        const char* pszFileName, CDatabaseTypeSet* ptypsetMyOwner) :
    CSetEl(pszName)
{
    ASSERT( pszFileName );
    m_sFileName = pszFileName;
    m_ptyp = NULL;
    m_ptypsetMyOwner = ptypsetMyOwner;
    ASSERT( m_ptypsetMyOwner );
    m_sVersion = m_ptypsetMyOwner->sSettingsVersion();  // 1998-03-12 MRP
}

CDatabaseTypeProxy::~CDatabaseTypeProxy()
{
    delete m_ptyp;
    m_ptyp = NULL;
}


CSet* CDatabaseTypeProxy::psetMyOwner()
{
    return m_ptypsetMyOwner;
}


void CDatabaseTypeProxy::DecrementNumberOfRefs()
{
    ASSERT( m_ptyp );
    CSetEl::DecrementNumRefs();
    if ( !bHasRefs() )
        {
        m_ptyp->WritePropertiesIfModified();
        CDatabaseType* ptyp = m_ptyp; // BJY 4-16-96 Have to use temporary and clear m_ptyp before
        m_ptyp = NULL;              // deleting m_ptyp to avoid compiler bug
        delete ptyp;
        }
}


CDatabaseType* CDatabaseTypeProxy::ptyp(CNoteList& notlst)
{
    if ( !m_ptyp )
        {
        if ( !ptypNew()->bReadProperties(notlst) )
            { // user must have trashed .typ file while Shw was running or something else equally naughty
            CDbTypRecMkrDlg dlg(m_ptyp);
            dlg.DoModal(); // get record marker
            
            // Write the markup type now.
		    m_ptyp->WriteProperties( sPath() ); // 5.99f Encapslute write of database type file
            }
        }
        
    return m_ptyp;
}

CDatabaseType* CDatabaseTypeProxy::ptypNew()
{
    ASSERT( !m_ptyp );
    CLangEncSet* plngset = m_ptypsetMyOwner->plngset();     
    m_ptyp = new CDatabaseType(plngset, this, TRUE);
    
    return m_ptyp;
}

BOOL CDatabaseTypeProxy::s_bReadProperties(Object_istream& obs, const char* pszName,
        const char* pszFileName, CDatabaseTypeSet* ptypsetMyOwner,
        CDatabaseType** pptyp)
{
    CDatabaseTypeProxy* ptrxRead = new CDatabaseTypeProxy(pszName, pszFileName,
        ptypsetMyOwner);
    CDatabaseType* ptypRead = ptrxRead->ptypNew();
    
    if ( !ptypRead->bReadProperties(obs) )
        {
        delete ptrxRead;
        return FALSE;
        }
        
    *pptyp = ptypRead;
    return TRUE;
}

void CDatabaseTypeProxy::WritePropertiesIfModified( BOOL bForceWrite)
{
    if ( m_ptyp )
        m_ptyp->WritePropertiesIfModified( bForceWrite );
}

BOOL CDatabaseTypeProxy::s_bReadProxy(const char* pszPath,
        CDatabaseTypeSet* ptypset, CDatabaseTypeProxy** pptrx)
{
    ASSERT( pszPath );
    ASSERT( ptypset );
    ASSERT( pptrx );
    ASSERT( !*pptrx );  // If it isn't NULL now, there will be a leak.

    std::ifstream ios( pszPath );
    if ( ios.fail() )
        return FALSE;

    Newline_istream iosnl(ios);  // 1997-08-11
    CNoteList notlst;
    Object_istream obs(iosnl, notlst);
    Str8 sName;
    // Make sure the file begins with the proper marker.
    if ( !obs.bAtBackslash() || !obs.bReadBeginMarker(psz_DatabaseType, sName) )
        return FALSE;

    Str8 sFileName = sGetFileName(pszPath, TRUE); // get extension too
    // if ( sName.IsEmpty() )
    //     sName = sFileName;

    Str8 sVersion = "3.0";  // If no \ver field in the file, assume this
    obs.bReadString(psz_ver, sVersion);  // 1998-03-12 MRP
    
    CDatabaseTypeProxy* ptrx = new CDatabaseTypeProxy(sName, sFileName, ptypset);
    ptrx->m_sVersion = sVersion;  // MRP: NEED TO REVISE THE CONSTRUCTOR!!!

    *pptrx = ptrx;
    return TRUE;
}

Str8 CDatabaseTypeProxy::sPath()
{
    return ::sPath(m_ptypsetMyOwner->sSettingsDirPath(), m_sFileName);
}
    

void CDatabaseTypeProxy::Update(CUpdate& up)
{
    if ( m_ptyp )
        m_ptyp->Update(up);
}


// 1997-12-02 MRP: Use a temporary file instead of a string stream
// because it's so much faster for medium-sized objects.
// For now this is a static function instead of a member function
// to avoid changes to the .h file
static BOOL s_bCopyTMP(const CDatabaseType* ptypToCopyFrom,
        const Str8& sPathTMP, CDatabaseType* ptypNew,
        Str8& sMessage)
{
    // 1. Write properties to a temporary file
    ptypToCopyFrom->WriteProperties( sPathTMP ); // 5.99f Encapslute write of database type file

    // 2. Read properties from the temporary file
    {  // Scope of iosTMP and obsTMP for reading
    std::ifstream iosTMP(sPathTMP);
    if ( iosTMP.fail() )
        {
        int i = remove(sPathTMP);
		sMessage = sMessage + _("Cannot open file:") + " " + sPathTMP; // 1.5.0fd 
        return FALSE;
        }
        
    CNoteList notlst;
    Object_istream obsTMP(iosTMP, notlst);
    if ( !ptypNew->bReadProperties(obsTMP) )
        {
        int i = remove(sPathTMP);
        return FALSE;
        }
    }  // Delete iosTMP and obsTMP for reading

    // 3. Remove the temporary file
    int i = remove(sPathTMP);
    return TRUE;
}

static const char* s_pszExtTMP = ".tmp";

BOOL CDatabaseTypeProxy::bCopy(CSetEl** ppselNew)
{
    CNoteList notlst;
    CDatabaseType* ptypToCopyFrom = ptyp(notlst);
    // 1996-03-13 MRP: CAN THIS EVER FAIL???

    const Str8& sDirPath = m_ptypsetMyOwner->sSettingsDirPath();
    const Str8& sName = ptypToCopyFrom->sName();
    Str8 sNewName = m_ptypsetMyOwner->sUniqueName(sName);
    Str8 sRecordMkr = ptypToCopyFrom->pmkrRecord()->sName();
    
    CNewDbTypDlg dlg( m_ptypsetMyOwner, &sNewName, &sRecordMkr, ptypToCopyFrom->pmkrset() );
    if ( dlg.DoModal() != IDOK )
        return FALSE;

    Str8 sFileName = sUniqueFileName(sDirPath, sNewName, psz_DatabaseTypeExt, TRUE);
    CDatabaseTypeProxy* ptrxNew = new CDatabaseTypeProxy(sName, sFileName, m_ptypsetMyOwner);
    CDatabaseType* ptypNew = ptrxNew->ptypNew();
    ASSERT( ptypNew );

    Str8 sFileNameTMP = sUniqueFileName(sDirPath, "typ", s_pszExtTMP);
    Str8 sPathTMP = ::sPath(sDirPath, sFileNameTMP, s_pszExtTMP);

    Str8 sMessage;    
    if ( !s_bCopyTMP(ptypToCopyFrom, sPathTMP, ptypNew, sMessage) )
        {
        delete ptrxNew;
        MessageBeep(0);
        AfxMessageBox(sMessage);
        return FALSE;
        }

    // NOTE NOTE NOTE: We must temporarily name the new object identical to
    // the one from which we are copying in order to read its properties,
    // but then we rename it to a new unique name.
    ptrxNew->ChangeNameTo(sNewName);
    // ptypNew->SetRecordMkr(sRecordMkr);
    ptypNew->ChangeRecordMarker(sRecordMkr);  // 1996-11-05 MRP
    
    {
    CDatabaseTypeSheet dlg(ptypNew, CDatabaseTypeSheet::eOptionsPage);
    if ( dlg.DoModal() == IDOK )
        {
        m_ptypsetMyOwner->Add(ptrxNew);
        ptrxNew->WritePropertiesIfModified();  // This clears the flag
        *ppselNew = ptrxNew;
        return TRUE;
        }
    }
        
    delete ptrxNew;
    return FALSE;
}

BOOL CDatabaseTypeProxy::bModify()
{
    CNoteList notlst;
    CDatabaseType* ptypView = ptyp(notlst);
    return ptypView->bViewProperties();
//  return FALSE;
}


#ifdef _DEBUG
void CDatabaseTypeProxy::AssertValid() const
{
    ASSERT( !m_sFileName.IsEmpty() );
    if ( m_ptyp )
        m_ptyp->AssertValid();
}
#endif  // _DEBUG


// --------------------------------------------------------------------------

CDatabaseTypeSet::CDatabaseTypeSet(CLangEncSet* plngset,
        const char* pszSettingsVersion)
{
    m_plngset = plngset;
    ASSERT( m_plngset );
    ASSERT( pszSettingsVersion );
    m_sSettingsVersion = pszSettingsVersion;  // 1998-03-12 MRP
    m_bReadFromString = FALSE;
}

#ifdef _DEBUG
CDatabaseTypeSet::CDatabaseTypeSet(CLangEncSet* plngset,
        const char* pszSettingsVersion, const char* pszProperties)
{
    m_plngset = plngset;
    ASSERT( m_plngset );
    ASSERT( pszSettingsVersion );
    m_sSettingsVersion = pszSettingsVersion;  // 1998-03-12 MRP

    // Read the properties of zero or more markup types
    // concatenated together in pszProperties.
    m_bReadFromString = TRUE;   
    ASSERT( pszProperties );
    std::istringstream ios((char*)pszProperties);
    CNoteList notlst;
    Object_istream obs(ios, notlst);
    while ( !obs.bAtEnd() )
        {
        Str8 sName;
        BOOL bName = obs.bAtBeginMarker(psz_DatabaseType, sName);
        ASSERT( bName );
        CDatabaseTypeProxy* ptrx = ptrxAdd( sName, "" );
        BOOL bRead = ptrx->ptypNew()->bReadProperties(obs);
        ASSERT( bRead );
        }
    ASSERT( obs.bAtEnd() );
    ASSERT( notlst.bIsEmpty() );
}
#endif  // _DEBUG

CDatabaseTypeSet::~CDatabaseTypeSet()
{
    // 1996-05-24 AB and MRP: A database type deletes itself when it sees
    // its reference count go to zero. Interlinear processes hold references
    // to other database types, so when they are deleted, they can cause
    // other database types, or even their own type, to no longer be referenced.
    // To avoid circular calling of destructors, we must first delete
    // the interlinear processes of all the database types so that they no longer
    // point to other databases. While doing so, we hold pointers to 
    // all database types so that their reference counts won't go to zero.
    int n = 0;
    for ( CDatabaseTypeProxy* ptrx = ptrxFirst(); ptrx; ptrx = ptrxNext(ptrx) )
        if ( ptrx->bOpen() )
            n++;
    if ( n == 0 )
        return;
        
    CDatabaseTypePtr* aptyp = new CDatabaseTypePtr[n];
    int i = 0;
    for ( CDatabaseTypeProxy* ptrx = ptrxFirst(); ptrx; ptrx = ptrxNext(ptrx) )
        if ( ptrx->bOpen() )
            {
            CNoteList notlst;
            aptyp[i++] = ptrx->ptyp(notlst); // Hold a pointer so the database type number of references won't go to zero
            }
    for ( i = 0; i < n; i++ )
        aptyp[i]->ClearProcListRefs(); // clear marker and dbtyp ref ptrs
    delete [] aptyp; // Delete all the type pointers, causing their destructors to be called    
}


void CDatabaseTypeSet::ReadAllProxies(CNoteList& notlst)
{
    // Read all markup type files in the setting directory.
    // It should be an _absolute_ directory path, e.g. C:\SHW\SETTINGS.
    ASSERT( m_sSettingsDirPath.GetLength() > 0 );

    // Get list of .lng files in settings directory.
    CFileList filelst( m_sSettingsDirPath, psz_DatabaseTypeExt );
    CFileListEl* pfle = filelst.pfleFirst();
    for ( ; pfle; pfle = filelst.pfleNext( pfle ) )
        {
        CDatabaseTypeProxy* ptrx = NULL;
		if ( bFileReadOnly(pfle->pszPath()) )
			{
			Shw_papp()->SetWriteProtectedSettings( TRUE );
			UnWriteProtect( pfle->pszPath() ); // 1.2aj Always clear write protect of settings files // 1.4qzhg
			}
		Str8 sPath = pfle->pszPath(); // 1.4vze 
		if ( !bAllASCII( sPath ) ) // 1.4vze 
			continue; // 1.4vze Don't load database type files with non-ANSI in file name
        if ( CDatabaseTypeProxy::s_bReadProxy(pfle->pszPath(), this, &ptrx) )
            CSet::Add(ptrx);
        }
}

CDatabaseTypeProxy* CDatabaseTypeSet::ptrxAdd( const char* pszName, const char* pszFileName )
{
    Str8 sName = pszName;
    if ( sName.IsEmpty() )
        sName = pszFileName;
    ASSERT( !sName.IsEmpty() );
//  sName.MakeUpper();
    CDatabaseTypeProxy* ptrx = new CDatabaseTypeProxy(sName, pszFileName, this);
    CSet::Add(ptrx);
    return ptrx;
}

void CDatabaseTypeSet::WriteAllModified( BOOL bForceWrite )
{
    CDatabaseTypeProxy* ptrx = ptrxFirst();
    for ( ; ptrx; ptrx = ptrxNext(ptrx) )
        ptrx->WritePropertiesIfModified( bForceWrite );
}


BOOL CDatabaseTypeSet::bSaveProjectPaths(BOOL bForceWrite)
{
    CNoteList notes;
    for (CDatabaseTypeProxy* proxy = ptrxFirst(); proxy; proxy = ptrxNext(proxy))
        {
        const Str8 path = proxy->sPath();
        if (!proxy->m_ptyp && !proxy->ptypNew()->bReadProperties(notes, FALSE))
            {
            delete proxy->m_ptyp;
            proxy->m_ptyp = nullptr;
            AfxMessageBox(Str8(_("Cannot read file:")) + " " + path);
            return FALSE;
            }

        // Keep the original type file intact if migration cannot write the replacement.
        WCHAR temporary[MAX_PATH] = {};
        if (GetTempFileNameW(swUTF16(m_sSettingsDirPath), L"typ", 0, temporary) &&
            proxy->m_ptyp->bWriteProperties(sUTF8(temporary), bForceWrite) &&
            MoveFileExW(temporary, swUTF16(path), MOVEFILE_REPLACE_EXISTING))
            continue;

        if (*temporary)
            DeleteFileW(temporary);
        AfxMessageBox(Str8(_("Cannot write file:")) + " " + path);
        return FALSE;
        }
    return TRUE;
}

BOOL CDatabaseTypeSet::bSearch(const char* pszName,
        CDatabaseType** pptyp, CNoteList& notlst)
{
    ASSERT( pszName );
    Str8 sName = pszName;
//  sName.MakeUpper();
    CSetEl* psel = pselSearch(sName);
    if ( !psel )
        return FALSE;
        
    CDatabaseTypeProxy* ptrx = (CDatabaseTypeProxy*)psel;
    
    *pptyp = ptrx->ptyp(notlst);
    return TRUE;
}


CDatabaseType* CDatabaseTypeSet::ptypSearch_AddIfNew(const char* pszName, CNoteList& notlst)
{
    CDatabaseType* ptyp = NULL;
    CDatabaseTypeProxy* ptrx = ptrxSearch(pszName);
    if ( ptrx )
        ptyp = ptrx->ptyp(notlst);
    else
        {
        ASSERT( !m_bReadFromString );

        Str8 sFileName = sUniqueFileName(m_sSettingsDirPath, pszName, psz_DatabaseTypeExt, TRUE);
        ptrx = ptrxAdd( pszName, sFileName );
        ptyp = ptrx->ptypNew();
        
        CDbTypRecMkrDlg dlg(ptyp);
        dlg.DoModal(); // get record marker
        
        // Write the new markup type now.
        Str8 sPathNew = sPath(m_sSettingsDirPath, sFileName);
	    ptyp->WriteProperties( sPathNew ); // 5.99f Encapslute write of database type file
        }

    ASSERT( ptyp );
    return ptyp;
}

// Used by word list and concordance to add a new type,
//    BUT ONLY IF it doesn't already exist.
CDatabaseType* CDatabaseTypeSet::ptypNew(const char* pszName,
        const char* pszRecordMarker, const char* pszFileNameBase /*=NULL*/)
{	// this func must be called with params already valid;
    ASSERT( pszName );
    ASSERT( *pszName );
    ASSERT( pszRecordMarker );
    ASSERT( *pszRecordMarker );
    CNote* pnot = NULL;
    ASSERT( CMarker::s_bValidName(pszRecordMarker, &pnot) );
    CDatabaseType* ptyp = NULL;
    CDatabaseTypeProxy* ptrx = ptrxSearch(pszName);
    ASSERT( !ptrx );
    ASSERT( !m_bReadFromString );
	if (pszFileNameBase)
		ASSERT( *pszFileNameBase );	// a special base for file name
	else
		pszFileNameBase = pszName;	// or use the type name as usual

    Str8 sFileName = sUniqueFileName(m_sSettingsDirPath, pszFileNameBase, psz_DatabaseTypeExt, TRUE);
    ptrx = ptrxAdd( pszName, sFileName );
    ptyp = ptrx->ptypNew();
    ASSERT( ptyp );
    ptyp->SetRecordMarker(pszRecordMarker);

    return ptyp;
}

CDatabaseTypeProxy* CDatabaseTypeSet::ptrxSearch(const char* pszName)
{
    ASSERT( pszName );
    Str8 sNameDisregardCase = pszName;
//  sNameDisregardCase.MakeUpper();
    
    return (CDatabaseTypeProxy*)pselSearch(sNameDisregardCase);
}
    
void CDatabaseTypeSet::OnDocumentClose( CShwDoc* pdoc ) // NULL out indexes of jump paths referring to sDoc
{
    CDatabaseTypeProxy* ptrx = ptrxFirst();
    for ( ; ptrx; ptrx = ptrxNext(ptrx) )
        if (ptrx->m_ptyp) // don't bother if not in use
            ptrx->m_ptyp->OnDocumentClose( pdoc ); // notify search path and interlinear processes that doc is closing
}

BOOL CDatabaseTypeSet::bOkToClose( CShwDoc* pdoc ) // check for any jump path references to this doc
{
    CDatabaseTypeProxy* ptrx = ptrxFirst();
    for ( ; ptrx; ptrx = ptrxNext(ptrx) )
        {
        if (ptrx->m_ptyp) // don't bother if not in use
            {
#ifdef NOTYET
            if ( ptrx->m_ptyp->pintprclst()->bRefersTo( pdoc ) ) // see if it's referenced in interlinear process
                {
                CDocCloseInterlinWarnDlg dlg;
                return dlg.DoModal() == IDYES;
                }
#endif                  
            if (ptrx->m_ptyp->pjmpset()->bRefersTo( pdoc ) ) // see if it's referenced in any jump paths
                {
                CDocCloseWarnDlg dlg;
                return dlg.DoModal() == IDYES;
                }
            }   
        }
    return TRUE;
}

void CDatabaseTypeSet::Update(CUpdate& up)
{
    CDatabaseTypeProxy* ptrx = ptrxFirst();
    for ( ; ptrx; ptrx = ptrxNext(ptrx) )
        ptrx->Update(up);
}

void CDatabaseTypeSet::ViewElements(CDatabaseTypeProxy** pptrxToSelect)
{
    CDatabaseTypeSetListBox lboSet(this, pptrxToSelect);
    Str8 sSettingsDirLabel = Shw_pProject()->sSettingsDirLabel();
    CSetDlg dlg(lboSet, _("Database Types"), sSettingsDirLabel, IDD_dbtypset);
    (void) dlg.DoModal();
}

void CDatabaseTypeSet::ViewElements(CDatabaseType* ptypToSelect)
{
    CDatabaseTypeProxy* ptrxToSelect = ( ptypToSelect ?
        ptypToSelect->ptrxMyProxy() :
        NULL
        );
    ViewElements(&ptrxToSelect);
}

BOOL CDatabaseTypeSet::bAdd(CSetEl** ppselNew)
{
    Str8 sName = sUniqueName("added");
    Str8 sRecordMkr;
    CMarkerSet mkrset( Shw_plngset() ); // have to pass something
    CNewDbTypDlg dlg( this, &sName, &sRecordMkr, &mkrset );
    if ( dlg.DoModal() != IDOK )
        return FALSE;

    Str8 sFileName = sUniqueFileName(m_sSettingsDirPath, sName, psz_DatabaseTypeExt, TRUE);

    CDatabaseTypeProxy* ptrxNew = new CDatabaseTypeProxy(sName, sFileName, this);
    CDatabaseType* ptypNew = ptrxNew->ptypNew();
    // ptypNew->SetRecordMkr( sRecordMkr );
    ptypNew->SetRecordMarker( sRecordMkr );
    
    {
    CDatabaseTypeSheet dlg(ptypNew, CDatabaseTypeSheet::eOptionsPage);
    if ( dlg.DoModal() == IDOK )
        {
        Add(ptrxNew);
        ptrxNew->WritePropertiesIfModified();  // This clears the flag
        *ppselNew = ptrxNew;
        return TRUE;
        }
    }  // Let dlg be destroyed and decrement reference to ptrxNew
    // Perhaps the OnCancel should do that explicitly
        
    delete ptrxNew;
    return FALSE;
}

void CDatabaseTypeSet::Delete(CSetEl** ppsel)
{
    ASSERT( ppsel );
    CDatabaseTypeProxy* ptrxToBeDeleted = (CDatabaseTypeProxy*)*ppsel;

    const Str8& sName = ptrxToBeDeleted->sName();
    const Str8& sFileName = ptrxToBeDeleted->sFileName();    
    Str8 sMessage = _("Delete database type?"); // 1.5.0fg 
    if ( AfxMessageBox(sMessage, MB_YESNO | MB_ICONQUESTION) != IDYES )
        return;

    Str8 sPathToBeDeleted = sPath(m_sSettingsDirPath, sFileName);
    int iResult = remove(sPathToBeDeleted);
    CSet::Delete(ppsel);
}


#ifdef _DEBUG
void CDatabaseTypeSet::AssertValid() const
{
    const CDatabaseTypeProxy* ptrx = ptrxFirst();
    for ( ; ptrx; ptrx = ptrxNext(ptrx) )
        ptrx->AssertValid();
}
#endif  // _DEBUG

void CDatabaseTypeSet::AddAndWrite(CDatabaseType* ptyp)
{
    ASSERT( ptyp );
    CDatabaseTypeProxy* ptrx = ptyp->ptrxMyProxy();
    ASSERT( ptrx->ptypsetMyOwner() == this );
    Add(ptrx);
        
//  CDatabaseTypeUpdate tup(ptyp);
//  extern void Shw_Update(CUpdate& up);        
//  Shw_Update(tup);  // This causes its modified flag to be set
        
    ptrx->WritePropertiesIfModified(TRUE);  // For now, force the write
}


void CDatabaseTypeSetListBox::InitLabels()
{
    m_xName = xSubItem_Show(IDC_SET_NAME);
    m_xFileName = xSubItem_Show(IDC_SET_FILE_NAME);
}

void CDatabaseTypeSetListBox::DrawElement(CDC& cDC, const RECT& rcItem,
            const CDblListEl *pel)
{
    CDatabaseTypeProxy* ptrx = (CDatabaseTypeProxy*)pel;
    DrawSubItem(cDC, rcItem, m_xName, m_xFileName, ptrx->sName());
    DrawSubItem(cDC, rcItem, m_xFileName, 0, ptrx->sFileName());
}

BOOL CDatabaseTypeSetListBox::bIsBold(CSetEl* psel)
{
    CDatabaseTypeProxy* ptrx = (CDatabaseTypeProxy*)psel;
    return ptrx->bHasRefs();
}

// **************************************************************************

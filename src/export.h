// export.h  File Export "processes" for SF, RTF, MDF

#ifndef EXPORT_H
#define EXPORT_H

#include "set.h"  // CSet, CSetEl
#include "mkr.h"  // CMarker, CMarkerSet, CMarkerSubSet
#include "project.h" // For update path
#include <fstream>
#include "Change_ostream.h" // For change table stream classes

class CIndex;  // ind.h
class CRecLookEl;
class CDatabaseType;  // typ.h
class RtfPageSetup;  // pgs.h
class RTF_ostream;  // rtf.h
class CFilter;  // fil.h

class CFilterUpdate;  // fil.h

// ==========================================================================

// Abstract base class for deriving export processes

class CExportProcess : public CSetEl  // Hungarian: exp
{
protected:
    friend class CExportProcessSet;
    CExportProcessSet* m_pexpsetMyOwner;
    CMarkerSubSet m_subsetMarkersToExport;
    BOOL m_bCheckConsistency;  // 1999-06-14 MRP: Move to base class
    BOOL m_bOverwriteOutputFile; // 1.1bc Add option to overwrite output file without asking
    BOOL m_bAddAlphDiv;

    CExportProcess(const char* pszName, CExportProcessSet* pexpsetMyOwner);
    CExportProcess(const char* pszName, CExportProcessSet* pexpsetMyOwner,
                   const CMarkerSubSet& subsetMarkersToExport,
                   BOOL bCheckConsistency);

    // Prevent use of the copy constructor and assignment operator
    // by making them private and omitting their definitions.
    CExportProcess(const CExportProcess& exp);
    CExportProcess& operator=(const CExportProcess& exp);
public:
    BOOL m_bChainedTo; // 1.1cd Flag to note that this process was chained to
	CShwDoc* m_pdocExported; // 1.1ce Note doc being exported by this process so it can be unloaded if chained to

public:
    virtual ~CExportProcess();

    CSet* psetMyOwner();

#ifdef typWritefstream // 1.6.4ac
    virtual void WriteProperties(Object_ofstream& obs) const = 0;
#else
    virtual void WriteProperties(Object_ostream& obs) const = 0;
#endif

    virtual const char* pszType() const = 0;
    virtual BOOL bMDF(CFilter** ppfil) const = 0;
    virtual BOOL bCanAutoOpen() const = 0;
    CMarkerSubSet subsetMarkersToExport() { return m_subsetMarkersToExport; } // 1.0bn Access for hide fields


    virtual BOOL bExport(CIndex* pindCur, const CRecLookEl* prelCur,
            const Str8& sPathDatabase, Str8& sMessage) = 0;

    BOOL bAutoOpen(const Str8& sPath, Str8& sMessage);

	virtual void WritePaths( std::ofstream& ostr ) {}; // Update paths if project moved

    virtual void FilterUpdated(CFilterUpdate& fup);

    virtual BOOL bCopy(CSetEl** ppselNew) = 0;
    virtual BOOL bModify() = 0;

    static BOOL s_bExportedFileDlg(const char* pszDefaultExt, BOOL bOverwritePrompt, const char* pszFileFilter, int* piFilterIndex, Str8& sPath);
    static BOOL s_bBrowseForFile(const char* pszTitle, const char* pszDefaultExt, const char* pszFilter, const Str8& sDirPath, Str8& sFilePath, UINT idHelp = 0); // 1.4ap Bring export process titles in from resource file // 1.4ar Bring export filters in from resource file

protected:
    BOOL bCheckConsistency(CIndex* pindCur);
};  // class CExportProcess


// --------------------------------------------------------------------------

class CExportProcessSF : public CExportProcess  // Hungarian: exp
{
private:
    BOOL m_bPriKeyFieldFirst;  // 1998-03-06 MRP
        // If you've selected Current Window's Records and
        // the primary sorting marker isn't the record marker,
        // move the relevant sorting field to the top of the record.
    Str8 m_sPathCCT;
    Str8 m_sPathSF;
	BOOL m_bOpenExportedFile; // 1.1bd Automatically open exported file
	BOOL m_bAddAlphDiv; // 1.1cm Add alphabetic dividers
    Str8 m_sChainProcess; // 1.1br Process to chain to

	Str8 m_sUnrecognizedSettingsInfo; // 1.0cp Don't lose settings info from later versions

    // Prevent use of the copy constructor and assignment operator
    // by making them private and omitting their definitions.
    CExportProcessSF(const CExportProcessSF& exp);
    CExportProcessSF& operator=(const CExportProcessSF& exp);

public:
    CExportProcessSF(const char* pszName, CExportProcessSet* pexpsetMyOwner);
    CExportProcessSF(const char* pszName, CExportProcessSet* pexpsetMyOwner,
            const Str8& sPathCCT,
            const CMarkerSubSet& subsetMarkersToExport,
            BOOL bCheckConsistency);
    ~CExportProcessSF();

#ifdef typWritefstream // 1.6.4ac
    void WriteProperties(Object_ofstream& obs) const;
#else
    void WriteProperties(Object_ostream& obs) const;
#endif
    static BOOL s_bReadProperties(Object_istream& obs,
            CExportProcessSet* pexpsetMyOwner, CExportProcess** ppexp);

    const char* pszType() const;
    BOOL bMDF(CFilter** ppfil) const;
    BOOL bCanAutoOpen() const;

    BOOL bExport(CIndex* pindCur, const CRecLookEl* prelCur,
            const Str8& sPathDatabase, Str8& sMessage);


	void WritePaths( std::ofstream& ostr ) // Write paths 
		{
		if ( m_sPathCCT.GetLength() > 0 )
			ostr << m_sPathCCT << "\n";
		}

    friend class CExportProcessSFDlg;
    BOOL bCopy(CSetEl** ppselNew);
    BOOL bModify();

private:
    void ReadProperties(Object_istream& obs);
};  // CExportProcessSF


// --------------------------------------------------------------------------

typedef int RtfTwips;  // pgs.h - Hungarian: tw, twx, twy

class CExportProcessRTF : public CExportProcess  // Hungarian: exp
{
private:
    BOOL m_bPriKeyFieldFirst;  // 1998-03-06 MRP
        // If you've selected Current Window's Records and
        // the primary sorting marker isn't the record marker,
        // move the relevant sorting field to the top of the record.
    Str8 m_sPathCCT;
    Str8 m_sPathDOT;
    Str8 m_sPathRTF;
    BOOL m_bMarkerFont;
    RtfPageSetup* m_ppgs;

	Str8 m_sUnrecognizedSettingsInfo; // 1.0cp Don't lose settings info from later versions
    
    // 1999-01-30 MRP: Tab-alignment was an experimental feature in 4.0,
    // but in 4.1 (and later) it's considered the default for interlinear text.
    BOOL m_bInterlinearTabAligned;
    RtfTwips m_twxInterlinearSpacing;  // Nominal character width in twips

    // Prevent use of the copy constructor and assignment operator
    // by making them private and omitting their definitions.
    CExportProcessRTF(const CExportProcessRTF& exp);
    CExportProcessRTF& operator=(const CExportProcessRTF& exp);

public:
    CExportProcessRTF(const char* pszName, CExportProcessSet* pexpsetMyOwner);
    CExportProcessRTF(const char* pszName, CExportProcessSet* pexpsetMyOwner,
            const Str8& sPathCCT, const Str8& sPathDOT,
            BOOL bMarkerFont,
            const CMarkerSubSet& subsetMarkersToExport,
            BOOL bCheckConsistency,
            RtfPageSetup* ppgs,
            BOOL bInterlinearTabAligned = TRUE,
            RtfTwips twxInterlinearSpacing = s_twxDefaultInterlinearSpacing);
    ~CExportProcessRTF();

#ifdef typWritefstream // 1.6.4ac
    void WriteProperties(Object_ofstream& obs) const;
#else
    void WriteProperties(Object_ostream& obs) const;
#endif
    static BOOL s_bReadProperties(Object_istream& obs,
            CExportProcessSet* pexpsetMyOwner, CExportProcess** ppexp);

    static const RtfTwips s_twxDefaultInterlinearSpacing;  // 1999-02-01 MRP

    const char* pszType() const;
    BOOL bMDF(CFilter** ppfil) const;
    BOOL bCanAutoOpen() const;

    BOOL bExport(CIndex* pindCur, const CRecLookEl* prelCur,
            const Str8& sPathDatabase, Str8& sMessage);


	void WritePaths( std::ofstream& ostr ) // Write paths 
		{
		if ( m_sPathCCT.GetLength() > 0 )
			ostr << m_sPathCCT << "\n";
		if ( m_sPathDOT.GetLength() > 0 )
			ostr << m_sPathDOT << "\n";
		}

    friend class CExportProcessRTFDlg;
    BOOL bCopy(CSetEl** ppselNew);
    BOOL bModify();

private:
    void ReadProperties(Object_istream& obs);

    void SetValidInterlinearSpacing();  // 1999-01-30 MRP
        // After properties are read from the database type file or
        // modifyed through the visual user interface, this function
        // is called to set the Spacing to its default value,
        // if it's out of the valid range.

    BOOL bWriteChanges(const Str8& sPathTMP,
            BOOL bCurrentRecord, const CRecLookEl* prelCur,
            BOOL bCurrentWindow, CIndex* pind,
            Str8& sMessage);
    BOOL bReadChanges(const Str8& sPathTMP, RTF_ostream& sfs,
            CMarkerSet* pmkrset, CMarker* pmkrRecord,
            Str8& sMessage);
};  // CExportProcessRTF


// --------------------------------------------------------------------------

class CExportProcessXML : public CExportProcess  // Hungarian: exp
{
private:
    BOOL m_bPriKeyFieldFirst;  // 1998-03-06 MRP
        // If you've selected Current Window's Records and
        // the primary sorting marker isn't the record marker,
        // move the relevant sorting field to the top of the record.
    Str8 m_sPathCCT;
    Str8 m_sPathStylesheet;
    Str8 m_sPathXML;

	Str8 m_sUnrecognizedSettingsInfo; // 1.0cp Don't lose settings info from later versions

    Str8 m_sRootNode;
    BOOL m_bUTF8;
    Str8 m_sDTDName;

    // Prevent use of the copy constructor and assignment operator
    // by making them private and omitting their definitions.
    CExportProcessXML(const CExportProcessXML& exp);
    CExportProcessXML& operator=(const CExportProcessXML& exp);

public:
    CExportProcessXML(const char* pszName, CExportProcessSet* pexpsetMyOwner);
    CExportProcessXML(const char* pszName, CExportProcessSet* pexpsetMyOwner,
            const Str8& sPathCCT, const Str8& sPathStylesheet,
            const CMarkerSubSet& subsetMarkersToExport,
            BOOL bCheckConsistency);
    ~CExportProcessXML();

#ifdef typWritefstream // 1.6.4ac
    void WriteProperties(Object_ofstream& obs) const;
#else
    void WriteProperties(Object_ostream& obs) const;
#endif
    static BOOL s_bReadProperties(Object_istream& obs,
            CExportProcessSet* pexpsetMyOwner, CExportProcess** ppexp);

    const char* pszType() const;
    BOOL bMDF(CFilter** ppfil) const;
    BOOL bCanAutoOpen() const;

    BOOL bExport(CIndex* pindCur, const CRecLookEl* prelCur,
            const Str8& sPathDatabase, Str8& sMessage);


	void WritePaths( std::ofstream& ostr ) // Write paths 
		{
		if ( m_sPathCCT.GetLength() > 0 )
			ostr << m_sPathCCT << "\n";
		if ( m_sPathStylesheet.GetLength() > 0 )
			ostr << m_sPathStylesheet << "\n";
		}

    friend class CExportProcessXMLDlg;
    BOOL bCopy(CSetEl** ppselNew);
    BOOL bModify();

private:
    void ReadProperties(Object_istream& obs);
};  // CExportProcessXML


// ==========================================================================

class CExportProcessSet : public CSet  // Hungarian: expset
{
protected:
    CDatabaseType* m_ptypMyOwner;
    Str8 m_sSettingsFolder;  // 1999-03-11 MRP

    CExportProcess* m_pexpDefault;
    BOOL m_bCurrentRecord;
    BOOL m_bCurrentWindow;
    BOOL m_bAutoOpen;
    BOOL m_bAutoUpdateStyles;  // 1997-11-24 MRP
    BOOL m_bShowProperties;  // 1998-03-10 MRP
    BOOL m_bMDF;  // 1997-11-18 MRP
    // 1999-03-02 MRP: Move MDF version from individual export processes
    // to the set to avoid showing the Convert Older MDF Files message
    // more than once per project.
    Str8 m_sVersionMDF;
    BOOL m_bShowConvertOlderMDF;
        // Shoebox has converted the MDF properties from an older version,
        // but the user hasn't yet used this export process since then.
        // The first time that it is used after the conversion, Shoebox shows
        // a message reminding the user to copy newer versions of MDF's files.
    BOOL m_bNoMDFReminder;  // 1998-01-21 MRP
    friend class CExportProcessDlg;

    // Prevent use of the copy constructor and assignment operator
    // by making them private and omitting their definitions.
    CExportProcessSet(const CExportProcessSet& expset);
    CExportProcessSet& operator=(const CExportProcessSet& expset);

public:
    CExportProcessSet(CDatabaseType* ptypMyOwner, BOOL bAddDefaultProcesses);
    virtual ~CExportProcessSet();

    CDatabaseType* ptypMyOwner() { return m_ptypMyOwner; }
    Str8 sSettingsFolder() const { return m_sSettingsFolder; }

#ifdef typWritefstream // 1.6.4ac
    void WriteProperties(Object_ofstream& obs) const;
#else
    void WriteProperties(Object_ostream& obs) const;
#endif
    static BOOL s_bReadProperties(Object_istream& obs, CDatabaseType* ptyp,
            CExportProcessSet** ppexpset);

    CExportProcess* pexpFirst() const
        { return (CExportProcess*) pselFirst(); }
    CExportProcess* pexpLast() const
        { return (CExportProcess*) pselLast(); }
    CExportProcess* pexpNext( const CExportProcess* pexpCur ) const
        { return (CExportProcess*) pselNext( pexpCur ); }
    CExportProcess* pexpPrev( const CExportProcess* pexpCur ) const
        { return (CExportProcess*) pselPrev( pexpCur ); }

    CExportProcess* pexpSearch( const char* pszName ) const
        { return (CExportProcess*) pselSearch( pszName ); }

    BOOL bCurrentWindow() const { return m_bCurrentWindow; }
    BOOL bCurrentRecord() const { return m_bCurrentRecord; }
    BOOL bAutoOpen() const { return m_bAutoOpen; }
    BOOL bAutoUpdateStyles() const { return m_bAutoUpdateStyles; }
    BOOL bShowProperties() const { return m_bShowProperties; }

    BOOL bMultiDictionaryFormatter() const { return m_bMDF; }
    void SetMultiDictionaryFormatter(BOOL bMDF);
    // 1999-03-02 MRP: Move MDF version to the export process set.
    const Str8& sVersionMDF() const { return m_sVersionMDF; }
    BOOL bShowConvertOlderMDF() const { return m_bShowConvertOlderMDF; }
    void ClearShowConvertOlderMDF() { m_bShowConvertOlderMDF = FALSE; }
    BOOL bNoMDFReminder() const { return m_bNoMDFReminder; }
    void SetNoMDFReminder(BOOL bNoMDFReminder);

    BOOL bInterlinear() const;  // 1999-01-30 MRP

    CExportProcess* pexpDefault() { return m_pexpDefault; } // 1.0bm Access to default export process

    CMarkerSet* pmkrset();
            
    BOOL bExport(CIndex* pindCur, const CRecLookEl* prelCur,
            const Str8& sPathDatabase, Str8& sMessage);

	void WritePaths( std::ofstream& ostr ); // Write paths in use

    void FilterUpdated(CFilterUpdate& fup);

    void Delete(CSetEl** ppsel);
    BOOL bAdd(CSetEl** ppselNew);
    BOOL bValidNameDlg(const Str8& sName,
            const CExportProcess* pexpModify, Str8& sMessage);

    BOOL bValidNameRead(const Str8& sName,
            const char* pszDefaultName, Str8& sValidName);

private:
    void ReadProperties(Object_istream& obs);    
};  // class CExportProcessSet


// --------------------------------------------------------------------------

class CExportProcessMDF;  // mdf.h

class COldExportProcessSet : public CExportProcessSet
{
private:
    CMarkerSubSet* m_psubsetMarkersToExport;
    BOOL m_bUseExportCCT; // use conversion table when exporting files
    Str8 m_sExportCCT; // name of conversion table for exporting
    BOOL m_bMarkerFont;
    RtfPageSetup* m_ppgs;
    BOOL m_bExportMDF;
    BOOL m_bExportRTF;
    CExportProcessMDF* m_pexpMDF;

    // Prevent use of the copy constructor and assignment operator
    // by making them private and omitting their definitions.
    COldExportProcessSet(const COldExportProcessSet& expset);
    COldExportProcessSet& operator=(const COldExportProcessSet& expset);

public:
    COldExportProcessSet(CDatabaseType* ptypMyOwner);
    ~COldExportProcessSet();

    BOOL bReadProperties(Object_istream& obs);
    void ConvertOldProperties();
};  // class COldExportProcessSet

#endif  // not EXPORT_H

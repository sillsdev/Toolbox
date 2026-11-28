//File: Corpus.h  CCorpus and CCorpusSet classes  Josh Kelly  July 1997

// Change History
//    Jul 97 JK First draft, designed with input from Alan B
// 18 Sep 97 BW Added some documentation
// 25 Sep 97 BW chgd names: WordListFile > WordIndexFile, TextListFile > WordListOutput
// 24 Apr 98 BW removed use of m_sFilesPath and sFilesPath(), 
// 27 Apr 98 BW replaced m_sWordIndexFile with m_sWordIndexPath, CreateWordIndexFile() with PrepareWordIndexFile()

#ifndef CORPUS_H
#define CORPUS_H

#include "set.h"
#include "obstream.h"
#include "lng.h"
#include "stringex.h"

class CCorpusSet;

// --------------------------------------------------------------------------

class CCorpus : public CSetEl
{
private:	
	Str8Ex m_sFilesList;  // list of SF corpus files, each separated by newline
	Str8 m_sWordMarkers;	 // list of SF markers (each separated by space) of fields containing words to be indexed
	CLangEncPtr m_plng;		 // language encoding of the corpus	texts
	Str8 m_sBookMarker;	 // SF marker of fields starting with 3-letter Book abbrev
	Str8 m_sChapMarker;	 // SF marker of fields with chapter number
	Str8 m_sVerseMarker;	 // SF marker of fields starting with verse number
	Str8 m_sRefMarkers;		// All reference markers together for quick search
	int m_iBookMarkerPos;	// Position of book marker in ref markers
	int m_iChapMarkerPos;	// Position of chap marker in ref markers
	int m_iVerseMarkerPos;	// Position of verse marker in ref markers
	Str8 m_sWordListOutput; // filename & ext entered for SF word list output
	Str8 m_sWordIndexPath; // full file path of binary Word Index file
	int m_iFileNameStart;	// Start of current file name in file list 1.4tb
	FILE* m_pfilCorpusCurFile;	// Currently open file for read of corpus 1.4tb
	Str8 m_sBookRef;	 // 1.4td Current book reference
	Str8 m_sChapRef;	 // 1.4td Current chap reference
	Str8 m_sVerseRef;	 // 1.4td Current verse reference

	friend class CCorpusSet;
	CCorpusSet* m_pcorsetMyOwner; // CorpusSet that owns this corpus

	// Construction
	CCorpus(const char* pszName, CCorpusSet *pcorsetMyOwner);
	CCorpus(const CCorpus& corSrc);

public:
	~CCorpus();


	CCorpusSet* pcorsetMyOwner() const { return m_pcorsetMyOwner; }
	virtual CSet* psetMyOwner();
	const Str8Ex& sFilesList() const { return m_sFilesList; }
	int iCurFileNameStart() const { return m_iFileNameStart; } // 1.4ya Information for progress bar
	const Str8& sWordMarkers() const { return m_sWordMarkers; }
	const Str8& sBookMarker() const { return m_sBookMarker; }
	const Str8& sChapMarker() const { return m_sChapMarker; }
	const Str8& sVerseMarker() const { return m_sVerseMarker; }
	CLangEncPtr plng() const { return m_plng; }
	void SetLangEnc( CLangEncPtr plng ) { m_plng = plng; } // 1.1rd Allow change of language encoding

#ifdef prjWritefstream // 1.6.4aa 
	void WriteProperties(Object_ofstream& obs) const;
#else
	void WriteProperties(Object_ostream& obs) const;
#endif
	static CCorpus* s_pcorDefaultMarkers(const char* pszName,
										CCorpusSet* pcorsetMyOwner);
	void SetDefaultMarkers1();
	BOOL bHasBibleMkrs();	// T if using \id \c \v etc

	void SetWordIndexPath(const char* pszWordIndexPath);
	void ClearWordIndexPath();
	void PrepareWordIndexFile();	// sets path & creates an empty file
    const Str8& sWordIndexFullPath() const { return m_sWordIndexPath; }
private:
	BOOL bOpenNextFile(); // 1.4td 
	BOOL bGetFileLine( Str8& s, BOOL bWholeField ); // 1.4td
public:
	BOOL bInitCorpusRead(); // 1.4td Used in first and next line functions
	BOOL bGetNextLineOfCorpus( Str8& s, BOOL bWholeField = FALSE ); // 1.4td // 1.4vye 
	Str8 sCurRef(); // 1.4td Current reference

	//	BOOL SetUpWordIndex(CWordList& wrdlst, int& nFiles, BOOL& bCancel); // 1.4tb

	//for word lists in SH, the file name is stored here with the corpus
	//  but maintained and used by CWordListDlg class
	const Str8& sWordListOutput() const { return m_sWordListOutput; }
	void SetWordListOutput(const char* pszWordListOutput);

	Str8 sCorpusFolder() const;	//returns folder of the first corpus file

	void ValidateFilesList(CNote **ppnotExist, CNote **ppnotDirty, 
						   BOOL bCheckForDirty);

	virtual BOOL bCopy(CSetEl** ppselNew);
	virtual BOOL bModify();
	BOOL bModifyProperties(const char* pszName, const char* pszFilesList, 
				   const char* pszWordMarkers, 
				   const char* pszBookMarker, const char* pszChapMarker, 
				   const char* pszVerseMarker, CLangEnc* plng, 
				   CNote** ppnot);

private:
	static BOOL s_bReadProperties(Object_istream& obs,
						  CCorpusSet* pcorsetMyOwner, CCorpus** ppcor);
	BOOL bReadProperties(Object_istream& obs);

#ifdef _DEBUG
	void AssertValid() const;
#endif  // _DEBUG

};

// --------------------------------------------------------------------------

// CCorpusSet doubles as a location where the word list and concordance 
// dialog boxes can store their settings between invocations.
class CCorpusSet : public CSet   // Hungarian: corset
{
protected:
	// Settings for the word list and concordance dialog boxes.
	Str8 m_sWordLstCorpus;	  // last corpus used to make word list
	BOOL m_bWordLstCounts;		  // flag: output count of occurences of each word
	BOOL m_bWordLstRefs;		  // flag: output references where word was found
	BOOL m_bWordLstLimitCount;	  // flag: limit the number of references output
	int m_nWordLstMaxCount;		  // max number of refs to output
	Str8 m_sConcordanceCorpus; // last corpus used to do concordance lookup
	Str8 m_sConcordanceSearch; // last search string used for conc lookup
	int m_nConcordanceSearchType; // last type of search match (part vs whole word)
	BOOL m_bConcordanceLimitCount;// flag: limit the number of matches output
	int m_nConcordanceMaxCount;	  // max number of matches to output
	BOOL m_bConcordanceIgnoreCase;// flag: ignore case during search matching
	BOOL m_bConcordanceAlignMatchedStrings; // flag: align the matched strings, as opposed to whole word in target // 1.4vyx 
	Str8 m_sConcordanceOutput; // last filename & ext for SF conc lookup output
	Str8 m_sSettingsDirPath;	  // copy of SH project settings path

public:
	CCorpusSet();
	~CCorpusSet();


#ifdef prjWritefstream // 1.6.4aa 
	void WriteProperties(Object_ofstream& obs) const;
#else
	void WriteProperties(Object_ostream& obs) const;
#endif
	BOOL bReadProperties(Object_istream& obs);

	const Str8& sSettingsDirPath() const { return m_sSettingsDirPath; }
	void SetDirPath( const char* pszPath ) { m_sSettingsDirPath = pszPath; }

	void ViewElements(CCorpus** ppcorToSelect);
	void ViewElements();
	virtual BOOL bAdd(CSetEl** ppselNew);
	virtual void Delete(CSetEl** ppsel);

	// Access to settings for the word list and concordance dialog boxes.
	CCorpus* pcorWordLstCorpus() const;
	BOOL bWordLstCounts() const { return m_bWordLstCounts; }
	BOOL bWordLstRefs() const { return m_bWordLstRefs; }
	BOOL bWordLstLimitCount() const { return m_bWordLstLimitCount; }
	int nWordLstMaxCount() const { return m_nWordLstMaxCount; }
	CCorpus* pcorConcordanceCorpus() const;
	const Str8& sConcordanceSearch() const { return m_sConcordanceSearch; }
	int nConcordanceSearchType() const { return m_nConcordanceSearchType; }
	BOOL bConcordanceLimitCount() const { return m_bConcordanceLimitCount; }
	int nConcordanceMaxCount() const { return m_nConcordanceMaxCount; }
	BOOL bConcordanceIgnoreCase() const { return m_bConcordanceIgnoreCase; }
	BOOL bConcordanceAlignMatchedStrings() const { return m_bConcordanceAlignMatchedStrings; } // 1.4vyx 
	const Str8& sConcordanceOutput() const { return m_sConcordanceOutput; }
	void SetWordLstCorpus(CCorpus* pcor);
	void SetWordLstCounts(BOOL bCounts)
		{ m_bWordLstCounts = bCounts; }
	void SetWordLstRefs(BOOL bRefs)
		{ m_bWordLstRefs = bRefs; }
	void SetWordLstLimitCount(BOOL bLimitCount)
		{ m_bWordLstLimitCount = bLimitCount; }
	void SetWordLstMaxCount(int nMaxCount)
		{ m_nWordLstMaxCount = nMaxCount; }
	void SetConcordanceCorpus(CCorpus* pcor);
	void SetConcordanceSearch(const char* pszSearch)
		{ m_sConcordanceSearch = pszSearch; }
	void SetConcordanceSearchType(int nSearchType)
		{ m_nConcordanceSearchType = nSearchType; }
	void SetConcordanceLimitCount(BOOL bLimitCount)
		{ m_bConcordanceLimitCount = bLimitCount; }
	void SetConcordanceMaxCount(int nMaxCount)
		{ m_nConcordanceMaxCount = nMaxCount; }
	void SetConcordanceIgnoreCase(BOOL bIgnoreCase)
		{ m_bConcordanceIgnoreCase = bIgnoreCase; }
	void SetConcordanceAlignMatchedStrings(BOOL b) // 1.4vyx 
		{ m_bConcordanceAlignMatchedStrings = b; } // 1.4vyx 
	void SetConcordanceOutput(const char* pszOutput)
		{ m_sConcordanceOutput = pszOutput; }
};

// --------------------------------------------------------------------------


class CCorpusComboBox : public CSetComboBox
{
public:
	CCorpusComboBox (CCorpusSet *pcorset) : CSetComboBox(pcorset) {}
	CCorpus* pcorSelected() { return (CCorpus*) pselSelected(); }
	void UpdateElements(CCorpus* pcorToSelect) 
		{ CSetComboBox::UpdateElements(pcorToSelect); }
};


#endif

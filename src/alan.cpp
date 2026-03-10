// Alan.cpp  Alan's Shw debugging/development code

#include "stdafx.h"
#include "toolbox.h"

#include "shw.h"
#include "alan.h"
#include "shwview.h" 
#include "typ.h"
#include "mainfrm.h"
#include "tools.h"
#include <malloc.h>

#include "CorGuess.h"

// extern BOOL bReleaseTrace; // Debug routine sets this for debugging release versions // 1.4qtp Disable release trace code
#ifndef _DEBUG
// void Trace( char* format, ...); // for debugging release versions // 1.4qtp Disable release trace code
#endif

void CShwView::AlanDebug2() // ID_ALAN_DEBUG2 Ctrl+Alt+Shift+C Debug2 // 1.5.0ab Add three more Alan debug functions
	{
	AfxMessageBox( "Toggle UseCrashFixFileEdit and reg setting" ); // 1.5.8h 
	Shw_papp()->ToggleUseCrashFixFileEditDlg(); // 1.5.8h 
	if ( Shw_papp()->bUseCrashFixFileEditDlg() ) // 1.5.8wa Set or clear Ret Setting
		SetRegPossibleCrash(); // 1.5.8wa 
	else
		ClearRegPossibleCrash(); // 1.5.8wa 
	BOOL b = bRegPossibleCrash(); // 1.5.8wa 
	}

void CShwView::AlanDebug3() // ID_ALAN_DEBUG3 Ctrl+Alt+Shift+D Debug3 // 1.5.0ab 
	{
//	AfxMessageBox( "AlanDebug3" );
	}

void CShwView::ToggleInternalKeyboard() // ID_TOGGLE_INTERNAL_KEYBOARD Ctrl+Shift+K Toggle internal keyboard // 1.5.1mc 
	{
	m_rpsCur.pfld->plng()->ToggleUseInternalKeyboard(); // 1.5.1mc 
	}

extern void CorrTest(CShwView* pview); // Body is below

void DebugAlan(CShwView* pview) // ID_DEBUG_ALAN Triggered by Ctrl+Alt+Shift+Z
{
	int iAns = IDNO;
	Str8 sDbType = pview->ptyp()->sName();
	if ( sDbType != "kb" ) // 1.4be Disable DebugAlan if not knowledge base db type
		return;
	CorrTest(pview); // Do correspondence test on current file +++++++++
}

// ===============================================
CField* pfldFindOrAdd( CRecord* prec, CMarker* pmkr ) // Find field in record, or add if it doesn't exist
	{
	ASSERT( prec );
	CField* pfld = prec->pfldFirstWithMarker( pmkr );
	if ( !pfld )
		{
		prec->Add( new CField( pmkr, "" ) );
		pfld = prec->pfldFirstWithMarker( pmkr );
		}
	return pfld;
	}

int iDiff( const char* psz1, const char* psz2 ) // Return amount of difference of 2 strings
	{ // If same, returns 0. If one char mismatch or one char extra in one string, returns 1. More mismatch returns higher number.
	int iLen1 = strlen( psz1 );	int iLen2 = strlen( psz2 ); // Get lengths of strings
	int i1 = 0;	int i2 = 0;	int iDiff = 0; // Initialize position counters and difference measure
	while ( i1 < iLen1 && i2 < iLen2 )
		{
		if ( *(psz1+i1) != *(psz2+i2) ) // For each mismatch, increment difference
			{
			iDiff++;
			if ( iLen1 == iLen2 || i1 != i2 ) // If same length or indexes already different, move both
				{ i1++; i2++; }
			else if ( iLen1 > iLen2 ) // If one longer than the other, increment longest only
				i1++;
			else
				i2++;
			}
		else // Else (match), increment both
			{ i1++; i2++; }
		}
	if ( i1 != iLen1 || i2 != iLen2 ) // If not all length covered, increment difference
		iDiff++;
	return iDiff;
	}

Str8 sQuality( const char* psz1, const char* psz2 ) // Return measure of difference of 2 strings, A is perfect, B is one difference, C is more than one difference
	{
	int iQuality = iDiff( psz1, psz2 ); // See how different guess is from correct
	Str8 sQuality = "A"; // Set quality to A for perfect, B one diff, C two or more diff
	if ( iQuality > 0 )
		sQuality = "B";
	if ( iQuality > 1 )
		sQuality = "C";
	return sQuality;
	}

class Corresp;

#define pmkrS(s) pview->pdoc()->pmkrset()->pmkrSearch_AddIfNew( s ) // Find a marker
#define pfldM(pmkr) pfldFindOrAdd( prec, pmkr ) // Find a field
#define pfldM2(pmkr) pfldFindOrAdd( prec2, pmkr ) // Find a field
#define sContS(pmkr) pfldM(pmkr)->sContents() // Content of a field

BOOL bCalcCorr = 1; // Global to remember status of check box
BOOL bCollectCorr = 1; // Global to remember status of check box
Str8 sComment = ""; // Global to remember knowledge base name
Str8 sName = ""; // Global to remember comment
int iRequiredSuccessPercent = 30; // Global to remember required success percent parameter
int iTestSequenceNumber = 10; // Global to remember test sequence number
int iGuessLevel = 50; // 1.5.8va 

class GuesserTest : public Guesser
	{
public:
	int iGetGuessLevel() { return iGuessLevel; }; // 1.5.8va 
	int iGetRequiredSuccessPercent() { return iRequiredSuccessPercent; }; // 1.5.8va 
	void SetRequiredSuccessPercent( int i ) { iRequiredSuccessPercent = i; }; // 1.4vyd 
//	int iGetMaxSuffLen() { return iMaxSuffLen; }; // 1.5.8va 
//	void SetMaxSuffLen( int i ) { iMaxSuffLen = i; }; // 1.4vyd 
	int iGetMinSuffExamples() { return iMinSuffExamples; }; // 1.5.8va 
	void SetMinSuffExamples( int i ) { iMinSuffExamples = i; }; // 1.4vyd 
	CorrespList* pcorlst() { return &corlstKB; }; // Raw correspondences given to guesser
	CorrespList* pcorlstSuffGuess() { return &corlstSuffGuess; }; // Guessed suffixes
	CorrespList* pcorlstRootGuess() { return &corlstRootGuess; }; // Guessed roots
	CorrespList* pcorlstPrefGuess() { return &corlstPrefGuess; }; // Guess prefixes
	};

void WriteStatistics( CShwView* pview, GuesserTest* pgue )
	{
	UnWriteProtect( "Statistics.txt" ); // 1.4qzhg
	std::ofstream str( "Statistics.txt", ios::app );
	CIndex* pind = pview->pind();
	CMarker* pmkrLx = pind->pmkrPriKey(); // Source word
	CMarker* pmkrGe = pmkrS( "ge" ); // Target word
	CMarker* pmkrGu = pmkrS( "gu" ); // Guess
	CMarker* pmkrQo = pmkrS( "qo" ); // Quality of original
	CMarker* pmkrQu = pmkrS( "qg" ); // Quality of guess
	CMarker* pmkrQc = pmkrS( "qc" ); // Quality change, better or worse
	CRecLookEl* prel = pind->prelFirst();
	int iTotalEntries = 0;
	int iTotalChanged = 0;
	int iTotalGuesses = 0;
	int iTotalCorrect = 0;
	int iTotalBetter = 0;
	int iTotalSimilar = 0;
	int iTotalWorse = 0;
	for ( ; prel; prel = pind->prelNext(prel) ) // Collect statistics
		{
		CRecord* prec = prel->prec();
		Str8 sLx = sContS(pmkrLx);
		Str8 sGe = sContS(pmkrGe);
		Str8 sQo = sQuality( sGe, sLx );
		Str8 sGu = sContS(pmkrGu);
		iTotalEntries++;
		if ( sLx != sGe ) // If user changed, count as change
			iTotalChanged++;
		if ( sGu != "" ) // If guess, count as guess
			{
			iTotalGuesses++;
			if ( sGu == sGe ) // If guess is correct, count as correct
				iTotalCorrect++;
			else
				{
				Str8 sQu = sQuality( sGe, sGu );
				if ( sQu < sQo ) // If guess better than original, count as better
					iTotalBetter++;
				else if ( sQu == sQo ) // If guess similar to original, count as similar
					iTotalSimilar++;
				else
					iTotalWorse++; // If guess worse than original, count as worse
				}
			}
		}
	int iBetterMinusWorse = iTotalCorrect + iTotalBetter - iTotalWorse;
	if ( iTotalChanged == 0 )
		iTotalChanged = 1;
	int iPercentBetter = iBetterMinusWorse * 1000 / iTotalChanged;
	char buffer[20];
	_itoa_s(iTestSequenceNumber, buffer, (int)sizeof(buffer), 10);
	str << "\\seq " << buffer << "\n";
	str << "\\nam " << sName << "\n";
	str << "\\com " << sComment << "\n"; // 1.4bn Add comment to guess test
	_itoa_s(iTotalEntries, buffer, (int)sizeof(buffer), 10);
	str << "\\tot " << buffer << "\n";
	_itoa_s(iTotalChanged, buffer, (int)sizeof(buffer), 10);
	str << "\\tchg " << buffer << "\n";
	_itoa_s(iTotalGuesses, buffer, (int)sizeof(buffer), 10);
	str << "\\tgue " << buffer << "\n";
	_itoa_s(iTotalCorrect, buffer, (int)sizeof(buffer), 10);
	str << "\\tcor " << buffer << "\n";
	_itoa_s(iTotalBetter, buffer, (int)sizeof(buffer), 10);
	str << "\\tbtr " << buffer << "\n";
	_itoa_s(iTotalSimilar, buffer, (int)sizeof(buffer), 10);
	str << "\\tsim " << buffer << "\n";
	_itoa_s(iTotalWorse, buffer, (int)sizeof(buffer), 10);
	str << "\\twor " << buffer << "\n";
	_itoa_s(iBetterMinusWorse, buffer, (int)sizeof(buffer), 10);
	str << "\\bminw " << buffer << "\n";
	_itoa_s(iPercentBetter, buffer, (int)sizeof(buffer), 10);
	str << "\\pcbtr " << buffer << "\n";
	_itoa_s(pgue->iGetGuessLevel(), buffer, (int)sizeof(buffer), 10);
	str << "\\guelev " << buffer << "\n";
	_itoa_s(pgue->iGetRequiredSuccessPercent(), buffer, (int)sizeof(buffer), 10);
	str << "\\sucpct " << buffer << "\n";
//	_itoa( pgue->iGetMaxSuffLen(), buffer, 10 );
//	str << "\\maxsuflen " << buffer << "\n";
	_itoa_s(pgue->iGetMinSuffExamples(), buffer, (int)sizeof(buffer), 10);
	str << "\\minsufex " << buffer << "\n";

	iTestSequenceNumber++; // Increment test sequence number for next test
	}

void OutputCorrespondences( CorrespList* pcorlst, std::ofstream& str, const char* pszType )
	{
	for ( Corresp* pcor = pcorlst->pcorFirst; pcor; pcor = pcor->pcorNext )
		{
		str << "\\lx " << pcor->pszSrc << "\n";
		str << "\\ge " << pcor->pszTar << "\n";
		str << "\\qu ";
		str.width(4);
		str.fill( '0' );
		str << pcor->iNumInstances << "\n";
		str << "\\qx " << pcor->iNumExceptions  << "\n";
		str << "\\typ " << pszType  << "\n";
		str << "\n";
		}
	}

void OutputCorrespondences( GuesserTest* pgue ) // Output correspondences to file for testing
	{
	UnWriteProtect( "Corresp.txt" ); // 1.4qzhg
	std::ofstream str( "Corresp.txt" );
	str << "\\_sh v3.0  400  kb\n\n";
	OutputCorrespondences( pgue->pcorlstSuffGuess(), str, "sf" );
	OutputCorrespondences( pgue->pcorlstRootGuess(), str, "rt" );
	OutputCorrespondences( pgue->pcorlstPrefGuess(), str, "pf" );
	str.close();
	}

void CorrTest(CShwView* pview) // Run test of guesser and output results +++++++
	{
	Shw_app().bSaveAllFiles(); // Save all so refresh won't lose changes
	CAlanAdaptIt dlg;
	dlg.m_bCalcCorr = bCalcCorr;
	dlg.m_bCollectCorr = bCollectCorr;
	dlg.m_sComment = sComment;
	sName = sUTF8( pview->pdoc()->GetPathName() ); // 1.5.8va 
	sName = sGetFileName( sName ); // 1.5.8va 
	sName = sName.Left( 6 ); // 1.5.8va 
	dlg.m_sName = sName;
	dlg.m_iRequiredSuccessPercent = iGuessLevel; // 1.5.8va 
	dlg.m_iTestSequenceNumber = iTestSequenceNumber;
    if (dlg.DoModal() != IDOK)
		return;
	bCalcCorr = dlg.m_bCalcCorr;
	bCollectCorr = dlg.m_bCollectCorr;
	sComment = dlg.m_sComment;
//	sName = dlg.m_sName; // 1.5.8va 
	iGuessLevel = dlg.m_iRequiredSuccessPercent; // 1.5.8va 
	iTestSequenceNumber = dlg.m_iTestSequenceNumber;
	CIndex* pind = pview->pind();
//	CMarker* pmkrLx = pind->pmkrPriKey(); // Source word // 1.6.1ah 
	CMarker* pmkrLx = pmkrS( "lx" ); // Source word
	CMarker* pmkrGe = pmkrS( "ge" ); // Target word
	CMarker* pmkrGu = pmkrS( "gu" ); // Guess
	CMarker* pmkrQo = pmkrS( "qo" ); // Quality of original
	CMarker* pmkrQu = pmkrS( "qg" ); // Quality of guess
	CMarker* pmkrQc = pmkrS( "qc" ); // Quality change, better or worse

	GuesserTest gue; // 1.4br Change main class name to Guesser
	gue.Init( iGuessLevel ); // 1.4vyd  // 1.5.8u Change ClearAll to Init // 1.5.8va 
//	gue.SetRequiredSuccessPercent( iRequiredSuccessPercent );
	char* pszGuess = (char*)_alloca( MAX_GUESS_LENGTH );
	CRecLookEl* prel = pind->prelFirst();
	CRecLookEl* prelFirst = prel;
	if ( bCalcCorr )
		{
		for ( ; prel; prel = pind->prelNext(prel) ) // Calculate correspondences and insert into fields
			{
			CRecord* prec = prel->prec();
			Str8 sLx = pfldM(pmkrLx)->sContents();
			Str8 sGe = pfldM(pmkrGe)->sContents();
			if ( ( sLx.Left( 1 ) == "-" ) && ( sLx.Right( 1 ) == "-" ) ) // 1.6.1ah Allow root as hyphen both sides
				{
				sLx = sLx.Mid( 1, sLx.GetLength() - 2 ); // 1.6.1ah Remove hyphens
				sGe = sGe.Mid( 1, sGe.GetLength() - 2 ); // 1.6.1ah Remove hyphens
				gue.AddCorrespondence( sLx, sGe, 0 ); // 1.6.1ah Store as root
				}
			else if ( sLx.Left( 1 ) == "-" ) // 1.6.1ah Suffix
				{
				sLx = sLx.Mid( 1 ); // 1.6.1ah Remove hyphen
				sGe = sGe.Mid( 1 ); // 1.6.1ah Remove hyphen
				gue.AddCorrespondence( sLx, sGe, -2 ); // 1.6.1ah Store as suffix
				}
			else if ( sLx.Right( 1 ) == "-" ) // 1.6.1ah Prefix
				{
				sLx = sLx.Mid( 0, sLx.GetLength() - 1 ); // 1.6.1ah Remove hyphen
				sGe = sGe.Mid( 0, sGe.GetLength() - 1 ); // 1.6.1ah Remove hyphen
				gue.AddCorrespondence( sLx, sGe, -1 ); // 1.6.1ah Store as prefix
				}
			else
				gue.AddCorrespondence( sLx, sGe, 1 ); // 1.6.1ah Normal kb pair
			}
		gue.bTargetGuess( "dogys", &pszGuess ); // Ask for a guess to trigger calculate correspondences ++++++
		OutputCorrespondences( &gue ); // Output correspondences to file for testing
		}
	if ( bCollectCorr && pview )
		{
		pind = pview->pind();
		prel = pind->prelFirst();
		for ( ; prel; prel = pind->prelNext(prel) ) // Collect correspondences
			{
			CRecord* prec = prel->prec();
			Str8 sLx = pfldM(pmkrLx)->sContents();
			Str8 sGe = pfldM(pmkrGe)->sContents();
			Str8 sQo = sQuality( sGe, sLx );
			pfldM(pmkrQo)->SetContent( sQo ); // Store quality of original in every entry
			pfldM(pmkrGu)->SetContent( "" ); // Clear the guess
			pfldM(pmkrQu)->SetContent( "" ); // Clear quality of guess
			pfldM(pmkrQc)->SetContent( "" ); // Clear change in quality
			if ( gue.bTargetGuess( sLx, &pszGuess ) ) // Try a guess on this entry +++++++
				{
				pfldM(pmkrGu)->SetContent( pszGuess ); // Store the guess
				Str8 sQu = sQuality( sGe, pszGuess );
				pfldM(pmkrQu)->SetContent( sQu ); // Store quality of guess
				Str8 sQc;
				if ( sQu == sQo ) // If guess same quality as original, store B for no quality change
					sQc = "B";
				else if ( sQu < sQo ) // If guess better than original, store A for improvement
					sQc = "A";
				else
					sQc = "C"; // If guess worse than original, store C for worsen
				pfldM(pmkrQc)->SetContent( sQc ); // Store change in quality
				}
			}
		WriteStatistics(pview, &gue); // Write statistics, append to statistics file
		pview->Invalidate(FALSE); // redraw to show result
		pview->pdoc()->SetModifiedFlag(); // Note modified
		pview->pdoc()->bSaveDocument(); // Try to force save // 1.6.1dp 
		}
	Shw_app().RefreshAllDocs();
	}


// Member functions for dialog box
CAlanAdaptIt::CAlanAdaptIt() :
    CDialog(IDD)
	{
	}

BOOL CAlanAdaptIt::OnInitDialog()
{
	CDialog::OnInitDialog();
	CheckDlgButton( IDC_CalcCorr, m_bCalcCorr ); // 1.4bc Test direct way of accessing dialog box controls
	CheckDlgButton( IDC_CollectCorr, m_bCollectCorr );
	SetDlgItemText( IDC_EDIT_Name, swUTF16( m_sName ) ); // 1.4qpv Fix SetDlgText in CEdit controls for Unicode
	SetDlgItemText( IDC_EDIT_Comment, swUTF16( m_sComment  ) ); // 1.4qpv
	SetDlgItemInt( IDC_EDIT_iTestSequenceNumber, m_iTestSequenceNumber );
	SetDlgItemInt( IDC_EDIT_iSuccPercent, m_iRequiredSuccessPercent );
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CAlanAdaptIt::OnOK()
{
	m_bCalcCorr = IsDlgButtonChecked( IDC_CalcCorr );
	m_bCollectCorr = IsDlgButtonChecked( IDC_CollectCorr );
//	GetDlgItemText( IDC_EDIT_Name, (char*)buffer, 1000  );
	CString sDlgItem; // 1.4qpw
	GetDlgItemText( IDC_EDIT_Name, sDlgItem ); // 1.4qpw
	m_sName = sUTF8( sDlgItem ); // 1.4qpw
//	GetDlgItemText( IDC_EDIT_Comment, (char*)buffer, 1000  );
	GetDlgItemText( IDC_EDIT_Comment, sDlgItem ); // 1.4qpw
	m_sComment = sUTF8( sDlgItem ); // 1.4qpw
	m_iRequiredSuccessPercent = GetDlgItemInt( IDC_EDIT_iSuccPercent );
	m_iTestSequenceNumber = GetDlgItemInt( IDC_EDIT_iTestSequenceNumber );
	LPSTR sNarrow = "Test"; // 1.4dt Test narrow and wide strings, they don't cross over
	LPWSTR wWide = L"Test"; // 1.4dt Test narrow and wide strings
	CDialog::OnOK();
}

// Member functions for dialog box
CCrashFixFileEdit::CCrashFixFileEdit() : // 1.5.8h 
    CDialog(IDD)
	{
	}

BOOL CCrashFixFileEdit::OnInitDialog() // 1.5.8h 
{
	CDialog::OnInitDialog();
	SetDlgItemText( IDC_EDIT_Name, swUTF16( m_sName ) );  // 1.5.8h Set initial file name
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CCrashFixFileEdit::OnOK() // 1.5.8h 
{
	CString sDlgItem; // 1.4qpw
	GetDlgItemText( IDC_EDIT_Name, sDlgItem );
	m_sName = sUTF8( sDlgItem );
	CDialog::OnOK();
}

BOOL bCrashFixBrowseForFileName(Str8 &sOutPath, Str8 sInPath) // 1.5.8h Browse using crash fix dialog box
	{
	CCrashFixFileEdit dlg; // 1.5.8h 
	dlg.m_sName = sInPath; // 1.5.8h Initialize file name to current name
	if (dlg.DoModal() != IDOK)
		return FALSE; // 1.5.8h 
	sOutPath =  dlg.m_sName; // 1.5.8h 
	return TRUE; // 1.5.8h 
	}

//	bReleaseTrace = TRUE; // Sample of how to use release trace

void DebugAlan() // Called only if no files are open, almost impossible in v5+
{
}


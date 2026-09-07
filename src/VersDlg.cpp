// VersionDlg.cpp : implementation file

#include "stdafx.h"
#include "toolbox.h"
#include "shw.h"
#include "VersionDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// These strings get shown in the About dialog (IDD_ABOUTBOX).
// First number is major version. Tenths represent a full external test release.
// Letters represent a minor internal or external test with limited distribution.

Str8 g_sVersion = "2.0.0 TBD 2026";
Str8 g_sCopyright = "Copyright \u00A9 2001-2026 SIL International";

/////////////////////////////////////////////////////////////////////////////
// CVersionDlg dialog
//
// This never gets called because IDC_btnVersionInfo in the About dialog
// is hidden in Shw.rc.
// The information below is an archive of pre-2.0 release notes
// and internal build histories. Future major release notes can be kept in
// project documentation (e.g. PROVENANCE.md).

CVersionDlg::CVersionDlg(CWnd* pParent /*=NULL*/) : CDialog(CVersionDlg::IDD, pParent)
{
Str8 sVersionHistory = "\
2.0.0 First version using VS 2022 \r\n";

sVersionHistory += "\r\n\
1.6.4s Test version for large corpus WordParse features. \r\n\
1.6.4zn Fix bug reinterlinearize word parse moving to wrong place. \r\n\
1.6.4zn Fix bug of return from jump finding cursor at top of text window. \r\n\
1.6.4zm If first word in field, lower case of word. \r\n\
1.6.4r Test version for large corpus WordParse features. \r\n\
1.6.4zk Change toolbar button to interlinearize instead of adapt. \r\n\
1.6.4q Test version for large corpus WordParse features. \r\n\
1.6.4zh Fix bug of interlinear check fail in lookup if keep cap & punc. \r\n\
1.6.4zg Fix bug of interlinear check fail in parse if keep cap & punc. \r\n\
1.6.4k Test version for large corpus WordParse features. \r\n\
1.6.4zf Lower first letter case on word inserted into WordParse file. \r\n";

sVersionHistory += "\r\n\
1.6.4j Test version for large corpus WordParse features. \r\n\
1.6.4ze Fix bug of WordParse overwriting text line if no interlinear. \r\n\
1.6.4zd Fix bug of interlinear check fail on ps n after << on later word. \r\n\
1.6.4zc Fix bug of interlinear check fail on ps n after << on first word. \r\n\
1.6.4zb If word parse, stop interlinear check after first pass. \r\n\
1.6.4za Remove 1.6.4y causes hang on some wrong combinations. \r\n\
1.6.4y Fix bug of interlinear check fail if << on first word. \r\n\
1.6.4w Fix bug of deleting aligning spaces from word parse. \r\n\
1.6.4v Make return from jump work from WordParse after jump to dic. \r\n\
1.6.4u Fix bug of not interlinearizing when at end of dic entry. \r\n\
1.6.4t Jump insert into dictionary from WordParse interlinearize without asking. \r\n\
1.6.4t Jump insert into WordParse file from text interlinearize without asking. \r\n\
1.6.4s Fix bug of interlinear not aligning from WordParse. \r\n\
1.6.4p Try to fix bug of interlinear not aligning from WordParse, no success. \r\n\
1.6.4n Fix bug of false forced gloss not found message on lookup with spaces. \r\n\
1.6.4m Fix bug of messed up cursor on delete back into interlinear. \r\n\
1.6.4k Fix bug of false forced gloss not found message on lookup with spaces. \r\n\
1.6.4h Recode write settings files to ofstream, no functional change. \r\n\
1.6.4ad Recode 2 places where ostrstream needs to be recoded to ofstream. \r\n\
1.6.4ac Start recode of ostream typ write into FILE. \r\n\
1.6.4ab Start recode of ostream lng write into FILE. \r\n\
1.6.4aa Start recode of ostream prj write into FILE. \r\n\
1.6.4g Fix 1.6.4f bug of highlight not refreshing on same word or sort by lg. \r\n\
1.6.4f Fix 1.6.4a highlight problem of focus left on lex. \r\n\
1.6.4e Undo 1.6.4c temp fix. \r\n\
1.6.4d Fix 1.6.4a interlinear cursor jump to top problem on button. \r\n\
1.6.4c Fix 1.6.4a interlinear cursor jump to top problem. \r\n\
1.6.4b Fix 1.6.4a problem of leaving highlight on lex. \r\n\
1.6.4a Show lexicon highlighting for AGNT databases. \r\n";

sVersionHistory += "\r\n\
1.6.4 May 2019 Change version number for release version, 1.6.4.0. \r\n\
1.6.3a Eliminate making Backup of file name. \r\n\
1.6.3a Fix bug of sometimes making Backup of Backup on upper ANSI file name. \r\n\
1.6.3a Fix bug of sometimes leaving Temp of instead of real file name. \r\n\
1.6.3 Dec 2017 Change version number for release version, 1.6.3.0. \r\n\
1.6.2b Fix help not working on Win 10, remove help buttons. \r\n\
1.6.2a Fix bug of save as non-Latin not working. \r\n\
1.6.2 Jul 2015 Change version number for release version, 1.6.2.0. \r\n\
1.6.1dr Test enable move field on export, not implemented after 1.6.0. \r\n\
1.6.1dp Guesser final fixes for release in Adapt It. \r\n\
1.6.1dn Guesser change to function to use for both suff & pref. \r\n\
1.6.1dm Guesser don't replace if it leaves less than 2 letters of root. \r\n\
1.6.1dk Guesser add max pref and suff args to Init. \r\n\
1.6.1dj Guesser remove max suffix length. \r\n\
1.6.1dh In guesser, remove 1.6.1df calculate at new word, didn't work. \r\n\
1.6.1dg In guesser, fix bug in sort longest last. \r\n\
1.6.1df Change guesser, if kb is small, calculate at each new word. \r\n\
1.6.1de Change guesser to avoid crash from bug in one compiler. \r\n\
1.6.1dd Change guesser to allow single char suffixes. \r\n\
1.6.1dc Fix guesser bug of no guess if no root list. \r\n\
1.6.1db Change guesser to require only 15% instead of 30% at 50+. \r\n\
1.6.1da Change guesser to allow single example of change at 50+. \r\n";

sVersionHistory += "\r\n\
1.6.1d Aug 2014 Change version for test version. 1.6.1.2. \r\n\
1.6.1cn In merge single rec into all, don't insert extra empty fields \r\n\
1.6.1cm Start work on file stream read settings. \r\n\
1.6.1ck Fix 1.6.1cd bug of writing settings in wrong order. \r\n\
1.6.1cj Fix bug of insert rec not asking about existing if not sorted by key. \r\n\
1.6.1ch Project remember don't ask change marker. \r\n\
1.6.1cg Fix 1.6.1cf bug of XML write group close one field too late. \r\n\
1.6.1cf XML write groups at all levels. \r\n\
1.6.1ce Work on XML write groups at all levels. \r\n\
1.6.1cd Work on new file stream. \r\n\
1.6.1cc Fix bug of crash on file open on new empty project. \r\n\
1.6.1cb Set up define for activating new file stream. \r\n\
1.6.1ca Change to def approach for eliminating streams. \r\n\
1.6.1bh Fix assert that stayed in infinite loop if mkr over same as mkr. \r\n\
1.6.1bg Start process of eliminating streams. \r\n\
1.6.1bf In guesser, sort longest last so it will be first in guess list. \r\n\
1.6.1be Change some guesser places back to pointer arithmetic. \r\n\
1.6.1bd In guesser, sort given affixes and roots longest first. \r\n\
1.6.1bc Make separate KB class for guesser. \r\n\
1.6.1bb Eliminate setting chars to 0 to terminate strings in guesser. \r\n\
1.6.1ba Update guesser to test _MCBS for non wxWidgets. \r\n\
1.6.1b Change version for test version of updated guesser. 1.6.1.1. \r\n\
1.6.1aj In guesser, make more shared functions. \r\n\
1.6.1ah In guesser, add testing code for hyphens to signal given. \r\n\
1.6.1ag In guesser, add given affixes to guess list. \r\n\
1.6.1af In guesser, always add correspondence to top of list. \r\n\
1.6.1ae In guesser, if higher frequency correspondence, replace previous. \r\n\
1.6.1ad In guesser, change bCount arg to iFreq, if zero, count. \r\n\
1.6.1ac Add place to store given affixes in guesser. \r\n\
1.6.1ab Distinguish guessed affixes from given in guesser. \r\n\
1.6.1aa Add frequency arg to guesser AddCorrespondence. \r\n";

sVersionHistory += "\r\n\
1.6.1 Jun 2013 Change version number for release version, 1.6.1.0. \r\n\
1.6.0b Mar 2013 Change version number for test version, 1.6.0.1. \r\n\
1.6.0ae Fix bug 1.6.0ac of parse not stopping at next a. \r\n\
1.6.0ad Update CorGuess.cpp for compiling in Adapt It. \r\n\
1.6.0ac Fix bug of interlinear not finding output of variant. \r\n\
1.6.0ab Fix bug of repeated records on print of text. \r\n\
1.6.0aa Fix bug of find repeating same record in text file. \r\n";

sVersionHistory += "\r\n\
1.6.0 Apr 2013 Change version number for release version, 1.6.0.0. \r\n\
1.5.9rd Move WineFix definition for wine display problem fix. \r\n\
1.5.9rc Remove all code related to macro recorder. \r\n\
1.5.9rb Include file compare criteria in message. \r\n\
1.5.9ra Restore ambiguity dialog box width to before 1.5.9qa. \r\n\
1.5.9r Mar 2013 Change version number for test version, 1.5.9.23. \r\n\
1.5.9qa Widen ambiguity dialog box for one user with long ambiguities. \r\n\
1.5.9q Feb 2013 Change version number for test version, 1.5.9.22. \r\n\
1.5.9pf Fix bug 1.5.9nc of wrong position of project path in dialog boxes. \r\n\
1.5.9pe XML read exported file only warn about interlinear layout. \r\n\
1.5.9pd Output XML head and tail infoon XML export. \r\n\
1.5.9pc Fix bug of wrong behavior on jump from Paratext. \r\n\
1.5.9p Nov 2012 Change version number for test version, 1.5.9.21. \r\n\
1.5.9ne Enlarge CC output buffer to allow for large expansion. \r\n\
1.5.9nd Fix bug of xml write not closing ref group in interlinear. \r\n\
1.5.9nc Enlarge all dialog boxes with paths, names, or lists. \r\n\
1.5.9nb Widen dialog box for merge database to show more of long path. \r\n\
1.5.9na Make special Wine version. \r\n";

sVersionHistory += "\r\n\
1.5.9n Aug 2012 Change version number for test version, 1.5.9.19. \r\n\
1.5.9mz Don't put deleted record in paste buffer. \r\n\
1.5.9my If sh header, read normally, even if export chain. \r\n\
1.5.9mx Don't output extra line break at end of file. \r\n\
1.5.9mw Don't write space after marker on empty field. \r\n\
1.5.9mv Remove BOM at beginning of file read. \r\n\
1.5.9mu Implement export current record. \r\n\
1.5.9mt If CCT, apply CCT to exported file. \r\n\
1.5.9ms Don't make backup file on export chain. \r\n\
1.5.9mr Don't make .ori on import. \r\n\
1.5.9mq Export subset of fields. \r\n\
1.5.9mp Fix bug of not sorting imported file. \r\n\
1.5.9mn Continue changes to write exported SF file with new bWrite. \r\n\
1.5.9mm Begin changes to write exported SF file with new bWrite. \r\n\
1.5.9mk Replace all calls to access with bFileExists and bFileReadOnly. \r\n\
1.5.9mj Change some file name tools functions from psz to Str8. \r\n\
1.5.9mh Finish recode of import, including CC. \r\n\
1.5.9mg Close file opened for import if cancel. \r\n\
1.5.9mf Recode of import, except for CC. \r\n\
1.5.9me Undo 1.5.9mc and 1.5.9md, didn't help for Wine display bug. \r\n\
1.5.9md Make ssa non-local for Wine display bug test, didn't help. \r\n\
1.5.9mc Disable ScriptStringFree for Wine display bug test, didn't help. \r\n\
1.5.9mb Disable rendering for Wine display problem fix. \r\n\
1.5.9ma Disable rendering in Mac version for Wine problem test. \r\n";

sVersionHistory += "\r\n\
1.5.9m Apr 2012 Change version number for test version, 1.5.9.9. \r\n\
1.5.9ka Fix bug in 1.5.9k of finding rec mkr in longer mkr. \r\n\
1.5.9k Mar 2012 Change version number for test version, 1.5.9.8. \r\n\
1.5.9he If XML attributes in sf record being read, store them. \r\n\
1.5.9hd If fld contains backslash at begin line, output with space before. \r\n\
1.5.9hb Recode sf read to use FILE class. \r\n\
1.5.9ha If xml attribute for record, write it in sf with xmlattr mkr. \r\n\
1.5.9h Feb 2012 Change version number for test version, 1.5.9.7. \r\n\
1.5.9gf Include xht file type as xml. \r\n\
1.5.9ge Include xhtml file type as xml. \r\n\
1.5.9gd Fix bug of doubling xml header if bom before it. \r\n\
1.5.9gc Change internal to store xml head and tail info with markers. \r\n\
1.5.9gb Use xmlhead and xmltail mkrs for head and tail info in sf write. \r\n\
1.5.9ga Recode sf write to use FILE class. \r\n\
1.5.9g Nov 2011 Change version number for test version, 1.5.9.6. \r\n\
1.5.9fa Handle escaped XML characters amp, quot, apos, lt, gt. \r\n";

sVersionHistory += "\r\n\
1.5.9f Nov 2011 Change version number for test version, 1.5.9.5. \r\n\
1.5.9ej If opening exported xml file, give warning. \r\n\
1.5.9eh Always output main declaration on xml save. \r\n\
1.5.9eg Add Toolbox write as export to xml attributes. \r\n\
1.5.9ef Add Toolbox write as file save to xml attributes. \r\n\
1.5.9ee Add Toolbox version to xml attributes. \r\n\
1.5.9ed Store and preserve xml tail info after database end. \r\n\
1.5.9ec Store and preserve on write xml attributes in rec mkr group tag. \r\n\
1.5.9eb Write xml file. \r\n\
1.5.9ea Start read of xml file. \r\n\
1.5.9e Fix bug of data link check not handling number at end of word. \r\n\
1.5.9da Start read xml, open file, read and store head info. \r\n\
1.5.9d Eliminate path from style sheet in XML export. \r\n\
1.5.9c Allow .css cascading style sheet in XML export. \r\n\
1.5.9b Remove header information from database end tag of XML export. \r\n\
1.5.9a Make interlinear lookup not show all output fields in record. \r\n";

sVersionHistory += "\r\n\
1.5.9 Apr 2011 Change version number for release version, 1.5.9.0. \r\n\
1.5.8yc Fix bug (1.5.8wb) of assert on XML export. \r\n\
1.5.8yb Fix bug (1.5.8vn) of crash on set wrap, remove message on set wrap. \r\n\
1.5.8ya Update wording of new project message. \r\n\
1.5.8y Test version uses registry to remember file browsing crash 1.5.8.9. \r\n\
1.5.8wb Fix bug of XML export no group for empty key fld. \r\n\
1.5.8wa Fix possible crash on parallel move. \r\n\
1.5.8w Non crash fix test version 1.5.8.8. \r\n\
1.5.8vc Set wrap margin on reshape entire file. \r\n\
1.5.8vb Fix bug of concordance lines of same ref in reverse order. \r\n\
1.5.8va Allow change of guess level in Guesser. \r\n\
1.5.8u Non crash fix test version, Ctrl+Alt+Shift+C clears reg 1.5.8.7. \r\n\
1.5.8t Fix bug of concordance trailing context too long. Test 1,5,8,6 \r\n\
1.5.8s Adjust header comments in guesser. \r\n\
1.5.8r Test version with crash fix turned on. \r\n\
1.5.8q Fix bug (1.5.8k) of crash on project open. \r\n\
1.5.8p In conc of scripture with verse ref, don't use whole book field. \r\n\
1.5.8n Give more messages for MDF problems. \r\n\
1.5.8m Add XML bool to doc for reading XML. \r\n\
1.5.8k Test version fixes all file browsing crashes. \r\n\
1.5.8h Test version fixes crash on File, Open and MDF export. \r\n\
1.5.8g Try non-MFC file box on WordList Browse, still crashed. \r\n\
1.5.8e If interlinear opposite rtl direction, don't add hyphens. \r\n\
1.5.8d Try to fix crash on File, Open. \r\n\
1.5.8c Fix bug of losing specific field on cancel of find dialog box. \r\n\
1.5.8b Fix bug of not remembering find in specific field. \r\n\
1.5.8a Output database type and wrap margin in XML export. \r\n";

sVersionHistory += "\r\n\
1.5.8 Feb 2010 Change version number for release version, 1.5.8.0. \r\n\
1.5.7d xSet up for XML export option to not escape wedges. \r\n\
1.5.7c Fix bug of XML export not escaping wedges. \r\n\
1.5.7b Fix bug of double word output on unannotated interlinear. \r\n\
1.5.7a Fix bug of output root guess not staying off. \r\n\
1.5.7 Nov 2009 Change version number for release version, 1.5.7.0. \r\n\
1.5.6a Fix bug of false message on insert record if filter already off. \r\n\
1.5.6  (Not released) Change version number for release version, 1.5.6.0. \r\n\
1.5.5d Remove non-functional HTML option from MDF export. Test 1.5.5.2 \r\n\
1.5.5c Fix bug of crash on modify or delete last record that matches filter. \r\n\
1.5.5b Fix bug of inserting duplicate record without asking if filtered. \r\n\
1.5.5a Fix bug of not allowing add to range set. Test 1.5.5.1. \r\n";

sVersionHistory += "\r\n\
1.5.5 Jan 2009 Change version number for release version, 1.5.5.0. \r\n\
1.5.4h Make ambiguity box a little narrower to fit in 1024x768. \r\n\
1.5.4g Fix bug of consistency check replace gray in legacy lng. \r\n\
1.5.4f Fix bug of crash on move if only one database available. \r\n\
1.5.4e Give error message if CCT path in export too long, max 126. \r\n\
1.5.4d Don't send language encoding on external parallel move. \r\n\
1.5.4c Portability look for Toolbox Project.prj as well as Toolbox.prj. \r\n\
1.5.4b Add SaveWithoutNewlines option in prj file. \r\n\
1.5.4a Don't merge records with different homonym numbers. 1.5.4.1. \r\n";

sVersionHistory += "\r\n\
1.5.4 Sep 2008 Change version number for release version, 1.5.4.0. \r\n\
1.5.3r If forced gloss chars not in punc, add them. \r\n\
1.5.3q Improve wording of matching criteria descriptions. \r\n\
1.5.3p Add NoBackup option in prj file. \r\n\
1.5.3n Fix bug of leaving file read only if two prjs open file. \r\n\
1.5.3m Fix bug of fail to write output if Backup write protected. \r\n\
x1.5.4  Change version number for release version, 1.5.4.0. \r\n\
1.5.3k Undo release prep, go back to test version. 1.5.3.3. \r\n\
1.5.3h Conc include whole field as ref, except verse ref. \r\n\
1.5.3g Allow empty fields in text record to be deleted. \r\n\
1.5.3f Fix problem (1.5.3b) of false message on delete record. \r\n\
1.5.3e Fix bug of possible crash on hit tab in last field. \r\n\
1.5.3d Fix bug of spell check fail if punc at end of line. \r\n\
1.5.3c Remember db as default for next move or copy. \r\n\
1.5.3b Allow delete record, highlight first. \r\n\
1.5.3a Widen text box in Marker Text filter dialog, 1.5.3.0. \r\n";

sVersionHistory += "\r\n\
1.5.3 Feb 2008 Change version number for release version, 1.5.3.0. \r\n\
1.5.2b Fix bug (1.5.1nc) of crash on external jump. \r\n";

sVersionHistory += "\r\n\
1.5.2 Jan 2008 Change version number for release version, 1.5.2.0. \r\n\
1.5.1s  Change version letter for test version, 1.5.1.3. \r\n\
1.5.1ra Fix bug of cursor sometimes wrong on startup. \r\n\
1.5.1r  Change version letter for test version. \r\n\
1.5.1pa Don't include all of long field in concordance line. \r\n";

sVersionHistory += "\r\n\
1.5.1p  Change version letter for test version. \r\n\
1.5.1ng If lookup not ambiguous, don't remove extra spaces. \r\n\
1.5.1nf Fix bug of interlinear not clearing if Given before lookup. \r\n\
1.5.1ne Make XML output valid by not escaping wedges. \r\n\
1.5.1nd Make XML output valid by adding a before numeric marker. \r\n\
1.5.1nc Make parallel move show same record if multiple same key. \r\n\
1.5.1nb Delete Record require delete content first. \r\n\
1.5.1na Adjustments for portability. \r\n\
1.5.1n  Change version letter for test version. \r\n\
1.5.1mc Add option to turn off internal keyboard. \r\n\
1.5.1mb Add option to allow multiple infixes. \r\n\
1.5.1ma Fix bug of odd display of sort cct in lng. \r\n\
1.5.1m  Change version letter for test version. \r\n\
1.5.1kh Don't reset keyboard on auto wrap. \r\n\
1.5.1kg Change Mac version tooltips to Mac shortcuts. \r\n\
1.5.1kf Change Mac version shortcuts to match Mac standards. \r\n\
1.5.1ke Use Shift+click instead of Ctrl+click only on Mac version. \r\n\
1.5.1kd Change Mac version to Quit, Alt+Q. \r\n\
1.5.1kc Change Windows version back to Exit, remove Alt+Q. \r\n\
1.5.1kb Make different version for Mac Windows emulator. \r\n\
1.5.1ka Don't write ProjectFilesList.txt file name into list of files. \r\n";

sVersionHistory += "\r\n\
1.5.1k  Change version letter for test version. \r\n\
1.5.1hb Remove Ctrl+Alt+Shift+A debug shortcut, UK kbd accent cap A. \r\n\
1.5.1ha Allow space to be defined in keydefs. \r\n\
1.5.1h  Change version letter for test version. \r\n\
1.5.1gb Side browse turn on parallel move target. \r\n\
1.5.1ga Change Ctrl+click to Shift+click for jump insert . \r\n\
1.5.1g  Change version letter for test version. \r\n\
1.5.1fs Cancel add separate font for ambiguity box. \r\n\
1.5.1fr Adjust width of ambiguity box. \r\n\
1.5.1fp Start add separate font for ambiguity box. \r\n\
1.5.1fn Widen ambiguity box to show longer line. \r\n\
1.5.1fm Fix bug of possible data loss on power fail during write. \r\n\
1.5.1fh Make Ctrl+right click always switch to browse. \r\n\
1.5.1fg Change add marker to browse to shift click, works on Mac. \r\n\
1.5.1ff Change browse sort from end to shift click, works on Mac. \r\n\
1.5.1fd Fix odd appearance on side browse from side browse. \r\n\
1.5.1fc Make side browse even if browsing. \r\n\
1.5.1fb Set side browse to show sort fields. \r\n\
1.5.1fa Allow consistency check on word list. \r\n";

sVersionHistory += "\r\n\
1.5.1f  Change version letter for test version. \r\n\
1.5.1ea Add Side Browse menu option. \r\n\
1.5.1e  Change version letter for test version. \r\n\
1.5.1da Fix problem of rtl interlinear wrap too wide. \r\n\
1.5.1d  Change version letter for test version. \r\n\
1.5.1cf Continue rtl interlinear, make cursor work. \r\n\
1.5.1ce Fix problem (1.5cc) of showing ltr interlinear rtl. \r\n\
1.5.1cd Continue rtl interlinear, make mouse right click work. \r\n\
1.5.1cc Begin rtl interlinear, display rtl. \r\n\
1.5.1cb Adjust guesser code for better portability. \r\n\
1.5.1ca Fix two export box strings that weren't being translated. \r\n\
1.5.1b  Enlarge button in export dialog box for translation. \r\n\
1.5.1a  Fix bug of possible crash on chained export. \r\n";

sVersionHistory += "\r\n\
1.5.1 Feb 2007 Change version letter for release version. \r\n\
1.5n  Change version letter for test version. \r\n\
1.5mr Undo 1.5mm, restore search box title and label. \r\n\
1.5mp Fix bug of compare highlight sometimes not visible. \r\n\
1.5mn Change Consistency Check menu for localization. \r\n\
1.5mm Change search box title for localization. \r\n\
1.5mk Enlarge keyboard name box in language encoding options. \r\n\
1.5mh Fix bug of text parallel jump failing on some movement keys. \r\n\
1.5mg Fix bug (1.5.0af) of not closing wav file before play file. \r\n\
1.5mf Fix bug of keydef leaving cursor wrong on multichar insert. \r\n\
1.5me Keydef, if dead combo no match, do plain char. \r\n\
1.5md Fix bug of keydef failing if dead combo after plain char. \r\n\
1.5mc Undo 1.5.0kc, caused a major bug. Do consistency check on jump. \r\n\
1.5mb Merge into all records (1.5.0jg) apply to filtered subset. \r\n\
1.5ma Make file compare show multiple differences in field. \r\n";

sVersionHistory += "\r\n\
1.5m  Change version letter for test version. \r\n\
1.5kr Remove Windows XP from version number. \r\n\
1.5kp Change version macr to two digits. \r\n\
1.5kn Change back to two digit version number. \r\n\
1.5.0km Fix bug (1.5.0ce) of compare sometimes not moving forward. \r\n\
1.5.0kh If last field in rec is date, insert merge field before it. \r\n\
1.5.0kg Fix possible crash on hide fields. \r\n\
1.5.0kf Require colon to save path for play sound. \r\n\
1.5.0ke Use updated CC lib to allow long file names. \r\n\
1.5.0kd Don't ask change marker if accepted once. \r\n\
1.5.0kc Don't do consistency check on jump. \r\n\
1.5.0kb Enlarge some controls in dialog boxes for French. \r\n\
1.5.0ka Fix bug (1.5.0jg) of merge single record inserting key field. \r\n";

sVersionHistory += "\r\n\
1.5.0k  Test version for French version. \r\n\
1.5.0jg If merge single record with empty key, merge into all records. \r\n\
1.5.0jf Fix bug (1.5.0jb) of paste field wrong in empty field. \r\n\
1.5.0je Fix bug (1.5.0gu) of crash on open prj with text file browse. \r\n\
1.5.0jd Remember path of play sound or file as default. \r\n\
1.5.0jc Make play sound in text find correct sound file. \r\n\
1.5.0jb If paste field at start of field, put before field. \r\n\
1.5.0ja Make 'Settings folder' translatable. \r\n\
1.5.0j  Test version for French version. \r\n\
1.5.0hc Start remember path of play sound or file as default. \r\n\
1.5.0hb Fix bug of key definition failing if it has extra spaces. \r\n\
1.5.0ha Remove unused messages. \r\n\
1.5.0h  Given to one person as test version. \r\n\
1.5.0gu Fix bug of auto save happening too often on mouse clicks. \r\n\
1.5.0gt Clarify that XML export CC table applies to XML output. \r\n\
1.5.0gs Note possible fix for no trailing nl in XML output. \r\n\
1.5.0gq Try two digit version number, but keep three digits for now. \r\n";

sVersionHistory += "\r\n\
1.5.0gp Make new db, record & field classes for Mac port. \r\n\
1.5.0gn Make new double linked list class for Mac port. \r\n\
1.5.0gm If jump to text file ref, place at top of window. \r\n\
1.5.0gk Clean up for portability. \r\n\
1.5.0gh Clean up for portability. \r\n\
1.5.0gg Clean up for portability. \r\n\
1.5.0ge Clean up messages for translatability. \r\n\
1.5.0gd Make message box use code page, for translatability. \r\n\
1.5.0gc Clean up messages for translatability. \r\n\
1.5.0gb Clean up messages for translatability. \r\n\
1.5.0ga Clean up messages for translatability. \r\n\
1.5.0fz Clean up messages for translatability. \r\n\
1.5.0fx Clean up messages for translatability. \r\n\
1.5.0fv Clean up messages for translatability. \r\n\
1.5.0ft Clean up messages for translatability. \r\n\
1.5.0fs Clean up messages for translatability. \r\n\
1.5.0fr Clean up messages for translatability. \r\n\
1.5.0fp Scroll up after jump. \r\n\
1.5.0fn Scroll up after turn off browse. \r\n\
1.5.0fm Make key definitions work in edit control. \r\n\
1.5.0fk Fix bug of hang on interlinearize from multi-line gloss. \r\n\
1.5.0fj Clean up messages for translatability. \r\n\
1.5.0fh Fix problem of thinking message file is project file. \r\n\
1.5.0fg Clean up messages for translatability. \r\n\
1.5.0ff Implement key definitions in lng file. \r\n\
1.5.0fe Add KeyDefs section to lng file. \r\n\
1.5.0fd Clean up messages for translatability. \r\n\
1.5.0fc Merge insert new field after previous field. \r\n\
1.5.0fb Delete dump and junk code, had lots of odd strings. \r\n\
1.5.0fa Remove cct test code, it had lots of odd strings. \r\n";

sVersionHistory += "\r\n\
1.5.0f Change version number for test version. \r\n\
1.5.0ek Change information in merge dialog box. \r\n\
1.5.0ek Don't merge hidden fields into records. \r\n\
1.5.0ej Fix bug (1.5.0eh) of crash on merge. \r\n\
1.5.0eh Don't merge empty fields. \r\n\
1.5.0eg Fix bug (1.4.zad) of not auto creating Default lng on new prj. \r\n\
1.5.0ef Clean up odd char in space string. \r\n\
1.5.0ed Change ANSI em dash in MDF default titles to hyphen. \r\n\
1.5.0ec Fix bug (1.5.0ce) of compare stopping at end of field. \r\n\
1.5.0eb On backspace into marker, ask if change marker. \r\n\
1.5.0eb Undo 1.50cb, allow backspace into marker. \r\n\
1.5.0ea Make Database, Merge do smart merge. \r\n";

sVersionHistory += "\r\n\
1.5.0e Change version number for test version. \r\n\
1.5.0de Portable, if Toolbox.prj in folder with program, open it. \r\n\
1.5.0dd Add app path to app class, get it from help path. \r\n\
1.5.0dc Undo save on lose focus (1.4zae) because of crash. \r\n\
1.5.0db Undo auto change lng to Unicode (1.4vxv) because of bugs. \r\n\
1.5.0da Adjust comments about ini file to show correct behaviour. \r\n";

sVersionHistory += "\r\n\
1.5.0d Change version number for test version. \r\n\
1.5.0ce Make compare continue from cursor position, not next field. \r\n\
1.5.0cd Undo remaximize after jump (1.4bzh) because of problems. \r\n\
1.5.0cc Start to improve compare in long fields. \r\n\
1.5.0cb Don't allow backspace to go into marker area, confuses users. \r\n\
1.5.0ca Allow play sound in browse view. \r\n";

sVersionHistory += "\r\n\
1.5.0c Change version number for test version. \r\n\
1.5.0ak Fix bug of possible crash on hide fields in browse view. \r\n\
1.5.0aj Eliminate meaningless message if WinExecWait fails. \r\n\
1.5.0ah Add Tools, Macros command to activate macro recorder. \r\n\
1.5.0ag Undo 1.5.0ae, better to use a macr. \r\n\
1.5.0af Fix bug of not starting new play file if play in process. \r\n\
1.5.0ae Auto play sound on next and prev record move. \r\n\
1.5.0ad Add Ctrl+Shift+F to activate filter combo box. \r\n\
1.5.0ac Experiment with making custom window. \r\n\
1.5.0ab Add three more Alan debug functions. \r\n\
1.5.0aa Make print not print hidden fields. \r\n";

#ifdef JUNK // 1.5.0ft 
sVersionHistory += "\r\n\
1.5.0 Aug 2006 Change version number for release. \r\n\
1.4zbk Remove version information button from About box for release. \r\n\
1.4zbj Make file version as shown by Windows show program name. \r\n\
1.4zbh Keep maximized when a maximized view does external jump. \r\n\
1.4zbg Make file version as shown by Windows match program version. \r\n\
1.4zbf Raise limit of sort order lines from 2500 to 10,000. \r\n\
1.4zbe Fix bug of text numbering including nl in ref. \r\n\
1.4zbd Fix bug (1.4zae) of crash on close project. \r\n\
1.4zbc Fix U bug of language encoding with only punc not working right. \r\n\
1.4zbb Tried to fix U bug of Win98 bad Keyman input, didn't work. \r\n\
1.4zba Fix U bug of possible error message on mouse click. \r\n";

sVersionHistory += "\r\n\
1.4zb Sent out as 1.5B3 Beta 3. \r\n\
1.4zam Fix U bug of Win98 version not handling Keyman Unicode. \r\n\
1.4zak Don't assume exactly 3 digits after decimal point on sound numbers. \r\n\
1.4zaj Change find of selected text default to all fields of language. \r\n\
1.4zah Fix bug of jump target doing parallel move when off. \r\n\
1.4zag Make new window opened by jump not default to parallel move target. \r\n\
1.4zaf Make Window, Duplicate keep jump and parallel move target settings. \r\n\
1.4zae When Toolbox loses focus, if auto save, save all modified. \r\n\
1.4zad Give message if making temporary language encoding. \r\n\
1.4zac Add Temporary to file name of temporary langauge encoding. \r\n\
1.4zab Fix U bug of possible assert message on mouse click. \r\n\
1.4zaa Remove def of Ctrl+Alt+Shif+Q (was temp close project). \r\n\
1.4zaa Remove duplicate def of Ctrl+Shif+F, still does select field. \r\n";

sVersionHistory += "\r\n\
1.4yx Sent out as 1.5B2 Beta 2. \r\n\
1.4ywp Change interlinear to copy down same morph break char in Unicode lng. \r\n\
1.4ywn Fix bug of extra ambiguity box from duplicate gloss. \r\n\
1.4ywm Improve appearance of binoculars on Find button. \r\n\
1.4ywk Change RTF export to only write uc0 once. \r\n\
1.4ywj Fix U bug of no space between fields on RTF export. \r\n\
1.4ywh Add Find button to toolbar. \r\n\
1.4ywg Start on code to keep interlinear alignment on replace. \r\n\
1.4ywf Fix U bug of crash if word list path does not exist. \r\n\
1.4ywe Improve wording of instructions for installing new project. \r\n\
1.4ywd Fix U bug of conc including extra word in target field. \r\n\
1.4ywc Fix bug of new conc not making ref lng same as text ref lng. \r\n\
1.4ywb Fix bug (1.4vxv) of setting rtl legacy lng to Unicode. \r\n\
1.4ywa Fix U bug of lookup inserting into lexicon without asking. \r\n";

sVersionHistory += "\r\n\
1.4yu Sent out as 1.5B1 Beta 1. \r\n\
1.4yth Change version to put Test or Beta right after version number. \r\n\
1.4ytg Try to fix possible export file overwrite bug. \r\n\
1.4ytf Try to fix bug of false assert on init project. \r\n\
1.4yte In debugger, open write protected project without delay. \r\n\
1.4ytd Fix bug of sometimes refusing to open project after crash. \r\n\
1.4ytc Add functions to get tag section from file string. \r\n\
1.4ytb Make Str8 grow faster for higher speed. \r\n\
1.4yta Add function to read file into Str8. \r\n\
1.4ysb Avoid writing settings files twice on close. \r\n\
1.4ysa Make sure read only gets written to project file. \r\n\
1.4yr Fix U bug of jump from word list ref failing to highlight upper case. \r\n\
1.4yq Eliminate find combo box from toolbar, can't work right. \r\n\
1.4yp Make File, Save As default to current folder. \r\n\
1.4ym Try to make find combo on Toolbar work with legacy. \r\n\
1.4yk Fix bug (1.4yc) of conc not limiting to whole word. \r\n\
1.4yj Change quick setup parse default to show root guess. \r\n\
1.4yh Change default gloss marker of quick setup to ge. \r\n\
1.4yg Fix U bug of interlinear quick setup giving blank markers. \r\n\
1.4yf Don't use selection for search default if wrong language . \r\n\
1.4ye If selection, make it default for insert record. \r\n\
1.4yd Make Find box show default in correct language encoding. \r\n\
1.4yc Fix U bug of conc sometimes failing to find a whole word. \r\n\
1.4yb Fix bug of insert field sometimes messing up interlinear. \r\n\
1.4ya Make word list progress bar show progress. \r\n";

sVersionHistory += "\r\n\
1.4y  Sent out as test version. \r\n\
1.4xg Fix bug of other db's not updating view on hide feilds. \r\n\
1.4xf Fix bug of possible crash on hide fields. \r\n\
1.4xe Fix U bug of incorrect chars from legacy Keyman. \r\n\
1.4xd If selection, use it as default for find. \r\n\
1.4xc Change title of export box and change export button to OK. \r\n\
1.4xb Fix U bug of external ref focus not working on Unicode. \r\n\
1.4xa Change message icon when Unicode validity check finds no problems. \r\n";

sVersionHistory += "\r\n\
1.4x  Sent out as test version. \r\n\
1.4wr Fix U bug of find trying to continue replace. \r\n\
1.4wq On search, if selection, use it as default. \r\n\
1.4wp Fix bug of moving cursor on cancel of search. \r\n\
1.4wn Fix U bug of possible hang on conc request. \r\n\
1.4wm Improve wording of new project recommend install. \r\n\
1.4wk Fix U bug of conc not always showing correct word. \r\n\
1.4wj Widen status bar pane that shows project file name. \r\n\
1.4whg Fix U bug of external word focus not working on Unicode. \r\n\
1.4whf Fix bug of cursor going below bottom of window on auto wrap . \r\n\
1.4whe Fix U bug of possible crash on run batch file. \r\n\
1.4whd Make files writable after external program does save all. \r\n\
1.4whc Fix U bug of double click selecting two words across nl. \r\n\
1.4whb Fix U bug of conc fail if extra space or nl in search. \r\n\
1.4wha Fix U bug of conc allowing space at begin and end of search. \r\n\
1.4wg Fix U bug of conc wrong if accented char before in field. \r\n\
1.4wf Fix U bug of conc not showing phrase as target. \r\n\
1.4we Fix U bug of conc not matching upper case search string. \r\n\
1.4wd Fix U bug of conc not matching phrase across line break. \r\n\
1.4wc Fix bug of replace skipping records if filtered on replaced string. \r\n\
1.4wb Disable File, Save As on read only file. \r\n\
1.4wa Change label on export button to Export file. \r\n";

sVersionHistory += "\r\n\
1.4w  Sent out as test version. \r\n\
1.4vzp Bring back interlinear quick setup button. \r\n\
1.4vzn Fix U bug of jump from concordance failing to highlight. \r\n\
1.4vzm Undo 1.4vzj and 1.4vzk, neither one worked. \r\n\
1.4vzk Fix bug of cursor going below bottom of window on auto wrap. \r\n\
1.4vzj Fix U bug of skipping records on replace with filter. \r\n\
1.4vzh Fix bug of possibly doing replace all on find next. \r\n\
1.4vzg Fix U bug of not setting keyboard in edit cntrl. \r\n\
1.4vzf Put Project, Close back on menu to allow close without saving. \r\n\
1.4vze Complain and refuse if any path has upper ANSI. \r\n\
1.4vzd Add conc dialog shortcuts for middle, begin, end, whole. \r\n\
1.4vzc Fix U bug of keeping upper ANSI chars in file name. \r\n\
1.4vzb Fix problem (1.4vyz) of word list missing end of some lines. \r\n\
1.4vza Write cz count with zeroes in word list like previous did. \r\n";

sVersionHistory += "\r\n\
1.4vyz Fix U bug of word list not collapsing case. \r\n\
1.4vyy Change conc to use better test for word separator. \r\n\
1.4vyx Add option for conc to align matched strings. \r\n\
1.4vyw Add extra space to separate word in concordance if aligned. \r\n\
1.4vyv Change conc target to show whole word like previous version. \r\n\
1.4vyu Fix U bug of not conc start to limiting to start. \r\n\
1.4vyt Delete extra spaces from concordance line. \r\n\
1.4vys Do auto save more often on text database. \r\n\
1.4vyr Fix cosmetic problem of (c) not showing in About box. \r\n\
1.4vyq Fix bug (1.4vyp) of assert on interlinearize new line. \r\n\
1.4vyp Add Ctrl+Alt+Shift+Q as quit without saving. \r\n\
1.4vyn Fix U bug of not scrolling multi-line language edit control. \r\n\
1.4vym Interlinearize align Given don't add nl unless already had one. \r\n\
1.4vyk When editing interlinear, don't add new nl at end of bundle. \r\n\
1.4vyj Change Find, Find Next shortcuts on menu to say F3, Shift+F3. \r\n\
1.4vyh Improve layout of buttons in Export dialog. \r\n\
1.4vyg Remove Modify button from Insert from Range Set dialog. \r\n\
1.4vyf Don't set wrap margin of other open docs of same type. \r\n\
1.4vye Include whole field in concordance line. \r\n\
1.4vyd Add ClearAll function to correspondence guesser. \r\n\
1.4vyc Disable Str8 validate, seems fine. \r\n\
1.4vyb Add progress bar during word list build. \r\n\
1.4vya Restore default find and replace in dialog box. \r\n";

sVersionHistory += "\r\n\
1.4vy  Local test version. \r\n\
1.4vxy Make mouse click work in single line legacy edit controls. \r\n\
1.4vxw Fix U bug of not selecting text in filter dialog. \r\n\
1.4vxv If lng has no upper Ansi chars, make it Unicode. \r\n\
1.4vxu Fix bug (1.4) of inappropriate assert in word formulas. \r\n\
1.4vxt Fix problem (1.4vxr) of crash on find dialog box. \r\n\
1.4vxs Change to use standard CEdit for Unicode. \r\n\
1.4vxr Fix problem of not inserting variable into marker text. \r\n\
1.4vxq Start change to use standard CEdit for Unicode. \r\n\
1.4vxp Change name of CLangEditCtrl to CEditLngCombo to reflect use. \r\n\
1.4vxn Eliminate Resize from CEditLng. \r\n\
1.4vxm Eliminate SetMarker from CEditLng. \r\n\
1.4vxk Use SetLangEnc for CEditLng instead of SetMarker. \r\n\
1.4vxj Fix U bug of added chars in sort not working till reload. \r\n\
1.4vxh Experiment with substitution of edit control. \r\n\
1.4vxg Fix problem with word formula edit control line ends. \r\n\
1.4vxf Change word formula edit control to standard ctrl. \r\n\
1.4vxe Make new language encoding default to Unicode. \r\n\
1.4vxd Disable jump and jump insert on menu if browsing. \r\n\
1.4vxc Remove default from search and insert record edit controls. \r\n\
1.4vxb Remove default from find and replace edit controls. \r\n\
1.4vxa Put nl in wordlist refs after every 10 refs. \r\n";

sVersionHistory += "\r\n\
1.4vwg Don't bring in window of moved exercise project. \r\n\
1.4vwf Make bring in command (Ctrl+Alt+B) bring in ambig box. \r\n\
1.4vwe If project moved, bring Toolbox window into home position. \r\n\
1.4vwd Try to fix bug of sometimes not opening project after crash. \r\n\
1.4vwc Try to fix bug of sometimes not opening project after crash. \r\n\
1.4vwb Try to fix bug of sometimes not opening project after crash. \r\n\
1.4vwa Try to fix bug of sometimes not opening project after crash. \r\n\
1.4vu Add possibility of simulating a crash (removed later). \r\n\
1.4vt Restore English files. \r\n\
1.4vs Translated to Chinese. \r\n\
1.4vp Version change and full for Chinese translation. \r\n\
1.4vnk Allow word list to limit number of references. \r\n\
1.4vnj Change default limit on wordlist refs from 10 to 100. \r\n\
1.4vnh Fix bug of CC buffer not big enough for Thai sort. \r\n\
1.4vng Clean up other places like 1.4vnf. \r\n\
1.4vnf Fix U bug of writing jnk in prj file as filSecKey. \r\n\
1.4vne Shorten question about reshaping entire file. \r\n\
1.4vnd Eliminate Project, Save, should be done with File, Save All. \r\n\
1.4vnc Eliminate Project, Close, it is not useful (back in later). \r\n\
1.4vnb Don't allow close of last file of project. \r\n\
1.4vna Make Project, New recommend install project files. \r\n\
1.4vm Undo 1.4vj as too strong. \r\n\
1.4vk Fix U bug of legacy keyboard input not working. \r\n\
1.4vj Fix bug of crash on change marker (didn't fix). \r\n\
1.4vh Remove unused string resource defs from resource.h. \r\n\
1.4vg Fix bug (1.4vb) of a few string resources still needed. \r\n\
1.4vf Fix bug of Failed to save message on read only file. \r\n\
1.4ve Fix messages with extra quote marks. \r\n\
1.4vc Fix bug of four messages with %1 instead of %s. \r\n\
1.4vb Delete all unused string resources to simplify translation. \r\n";

sVersionHistory += "\r\n\
1.4v  Sent out as test version. \r\n\
1.4ua Fix U bug of legacy keyboard input not working. \r\n";

sVersionHistory += "\r\n\
1.4u  Sent out as test version. \r\n\
1.4tgn Don't add extra space on concordance. \r\n\
1.4tgm If concordance search limited to begin or end, check it. \r\n\
1.4tgk Make concordance match case option work. \r\n\
1.4tgj Disable old word list code. \r\n\
1.4tgh Don't do concordance of empty string. \r\n\
1.4tgf Activate previous code to refresh or open concordance. \r\n\
1.4tge Fix U bug of add marker losing none for marker following. \r\n\
1.4tgd Don't resize window on Ctrl+Shift+B bring into screen. \r\n\
1.4tgc Continue conc port, fix to use search instead of cur word. \r\n\
1.4tgb Continue concordance port to IME, write concordance file. \r\n\
1.4tga Start concordance port to IME, collect concordance lines. \r\n\
1.4tf Local test version. \r\n\
1.4tej Save wordlist after it is loaded and sorted. \r\n\
1.4teh Remove word list options to turn off count and limit references. \r\n\
1.4teg Save all files before making wordlist. \r\n\
1.4tef Activate previous code to refresh or open word list. \r\n\
1.4tee Fix bug (1.4t) of writing empty file on close window. \r\n\
1.4ted Continue wordlist port, write word list file. \r\n\
1.4tec Continue wordlist port, do count and refs. \r\n\
1.4teb Continue wordlist port, collect words. \r\n\
1.4tea Continue wordlist port, fix bugs (1.4te) in corpus read. \r\n\
1.4te Continue wordlist port to IME, finish corpus read. \r\n\
1.4td Continue wordlist port to IME. \r\n\
1.4tc Continue wordlist port to IME. \r\n\
1.4tb Start wordlist port to IME. \r\n\
1.4ta Fix bug of not saving paths in db on moved exercise. \r\n";

sVersionHistory += "\r\n\
1.4t  Sent out as alpha test version. \r\n\
1.4sw Fix U bug of incorrect count of width of upper ANSI chars. \r\n\
1.4sv Fix bug of crash on build range set from empty fields. \r\n\
1.4su Wait before checking for same project already running. \r\n\
1.4st Fix U bug of clearing write protect of prj on save all. \r\n\
1.4ss Fix U bug of escape showing as box in edit control. \r\n\
1.4sr Fix U bug of bad display in find box on toolbar. \r\n\
1.4sq Fix U bug of possible crash on find button. \r\n\
1.4sp Fix U bug of range set buttons not enabled. \r\n\
1.4sn Fix U bug of interlin number using punc instead of name. \r\n\
1.4sm Fix bug (1.4ae) of not showing date stamp on save. \r\n\
1.4sk Hide Interlinear Quick Setup button, probably going. \r\n\
1.4sj Fix U bug of problem in Interlinear Quick Setup. \r\n\
1.4sh Fix U bug of crash on Interlinear Quick Setup. \r\n\
1.4sg Change parse wave line From and To numbers to strings. \r\n\
1.4sf Detect wave file names followed by decimal numbers. \r\n\
1.4se Fix bug of error on play sound while playing. \r\n\
1.4sd Fix bug U of play file not working. \r\n\
1.4sc Turn Unicode off for play sound file. \r\n\
1.4sb Clean up unnecessary uses of Unicode symbol. \r\n\
1.4sa Fix bug (1.4rac) of crash on open same project second time. \r\n";

sVersionHistory += "\r\n\
1.4s   Sent out as alpha test version. \r\n\
1.4rbe Disable wordlist and concordance for test version. \r\n\
1.4rbd Fix U bug of failing to paste Unicode into edit controls. \r\n\
1.4rbc Encapsulate get clipboard data. \r\n\
1.4rbb Start change to new wordlist system. \r\n\
1.4rba Re-enable (1.4qzhu) wordlist and concordance. \r\n";

sVersionHistory += "\r\n\
1.4rb  Local test version. \r\n\
1.4raj Fix U bug of showing %s or %1 in some messages. \r\n\
1.4rah Fix U bug of File, Save As not writing file. \r\n\
1.4rag Fix U bug of not showing marker in some MDF error messages. \r\n\
1.4raf Fix U bug of File, Save As leaving old file write protected. \r\n\
1.4rae Move reshape options from Tools menu to Database. \r\n\
1.4rad Add bar square bracket for raw output to export. \r\n\
1.4rac On exercise, don't save name of project last closed. \r\n\
1.4rab Fix U bug of crash on long line. \r\n\
1.4raa Restore 1.4qzra open previous project. \r\n";

sVersionHistory += "\r\n\
1.4r    Sent out as alpha test version. \r\n\
1.4qzra For alpha test, disable open previous project. \r\n\
1.4qzr  Local test version. \r\n\
1.4qzqb Fix problem (1.4qzpc) of language encodings changing to Unicode. \r\n\
1.4qzqa Undo 1.4qzpc, it made legacy language encodings change to Unicode. \r\n\
1.4qzq  Local test version. \r\n\
1.4qzph Fix bug of Code2000 font regular changing to bold. \r\n\
1.4qzpg Fix another U bug of crash on ambig box Formula button. \r\n\
1.4qzpf Fix U bug of crash on ambiguity box Formula button. \r\n\
1.4qzpe Adjust project lock for new Checks menu. \r\n\
1.4qzpd Clean up and regularize shortcut combos of Ctrl, Alt, and Shift. \r\n\
1.4qzpc Make new language encoding default to Unicode. \r\n\
1.4qzpb On close project without saving, ask if they want to exit. \r\n\
1.4qzpa Fix U bug of bad write of file path in ProjectFilesList.txt. \r\n\
1.4qzp  Local test version. \r\n\
1.4qzna Remove detailed change history before 1.4q, too much caused problems. \r\n\
1.4qzn  Local test version. \r\n\
1.4qzmg Fix U bug of not updating MDF export template from browse button. \r\n\
1.4qzmf On project close and save, don't ask about saving files. \r\n\
1.4qzme Fix U bug of skipping word on interlinearize return from jump. \r\n\
1.4qzmd On interlin lookup insert word into lexicon, do lower case. \r\n\
1.4qzmc On Tab in last field of record, do return from jump. \r\n\
1.4qzmb On interlinear lookup, if insert word, insert without asking. \r\n\
1.4qzma Fix U bug of wordlist jump failing to find some unicode words. \r\n";

sVersionHistory += "\r\n\
1.4qzm  Local test version. \r\n\
1.4qzkc Disable wordlist and concordance in Win98 test version. \r\n\
1.4qzkb Fix U problem of crash on wrap interlinear text. \r\n\
1.4qzka Fix bug of trying to write read-only file after set wrap margin. \r\n";

sVersionHistory += "\r\n\
1.4qzk  Local test version. \r\n\
1.4qzjm Prevent parse timeout message on interlinear check. \r\n\
1.4qzjk Change compiler optimizations to minimize size. \r\n\
1.4qzjh Fix U problem of jump not including all letters. \r\n\
1.4qzjg Allow wordlist and concordance in Win98 test version. \r\n\
1.4qzjf Fix problem (1.4qzje) of window flash on parallel move. \r\n\
1.4qzje Fix problem (1.4qzjd) of fail to jump to current ref. \r\n\
1.4qzjd Fix problem (1.4qzjb) of jumping to non-jump targets. \r\n\
1.4qzjc Make menu wording Parallel Movement Target. \r\n\
1.4qzjb Use focus target as target for parallel move. \r\n\
1.4qzja Local test version. \r\n";

sVersionHistory += "\r\n\
1.4qzhz Fix bug of crash on too many file names in corpus. \r\n\
1.4qzhy Fix U problem of asking whether to save file on Save All. \r\n\
1.4qzhx Fix U crash on jump (from Truncate). \r\n\
1.4qzhw Work on U problem of play file failing. \r\n\
1.4qzhv Eliminate unused Internet Options dialog box. \r\n\
1.4qzhu Disable wordlist and concordance for first Unicode test version. \r\n\
1.4qzht Clean up menu when no file is open. \r\n\
1.4qzhs Fix bad letters in file browse box title. \r\n\
1.4qzhr Fix U crash on RTF export. \r\n\
1.4qzhq Get rid of Str8 folder and Str8.cpp file. \r\n\
1.4qzhp Eliminate status bar messages. \r\n\
1.4qzhn Fix U problem of not recognizing keyboard names. \r\n\
1.4qzhm Eliminate code page input for U. \r\n\
1.4qzhk Fix U problem of writing empty file on close. \r\n\
1.4qzhj Make project close without saving remember project. \r\n\
1.4qzhh Use WriteProtect instead of chmod. \r\n\
1.4qzhg Use UnWriteProtect instead of chmod. \r\n\
1.4qzhf Work on problem of not playing sound with start and stop times. \r\n\
1.4qzhe Make Str8::bNextWord for parsing file info for play sound. \r\n\
1.4qzhd Fix U problem of writing empty file on close. \r\n\
1.4qzhc Clean up message box code. \r\n\
1.4qzhb Fix problem of auto save on interlinear in U build. \r\n\
1.4qzha On close project without saving, exit immediately. \r\n\
1.4qzh Test version for K and A local testing. \r\n\
1.4qzgk Fix _doserrno not defined in U release build. \r\n\
1.4qzgj Make PtrArray and derive Str8Array from it. \r\n\
1.4qzgh Temporarily stop using map in wordlist. \r\n\
1.4qzgg Start Str8Array. \r\n\
1.4qzgf Start Str8Array. \r\n\
1.4qzge Add Str8 Format function. \r\n\
1.4qzgd Add Str8 Insert and Delete functions. \r\n\
1.4qzgc Eliminate Cmp and CmpNoCase. \r\n\
1.4qzgb Add more Str8 functions. \r\n\
1.4qzga Use Str8 for Str8Ex. \r\n";

sVersionHistory += "\r\n\
1.4qzfz Make Str8 char operators. \r\n\
1.4qzfy Make Str8 append operators. \r\n\
1.4qzfx Make Str8 addition operators. \r\n\
1.4qzfw Make Str8 comparison operators. \r\n\
1.4qzfv Start Str8, make constructors. \r\n\
1.4qzfu Eliminate use of GetBuffr. \r\n\
1.4qzft Eliminate use of GetBuffr. \r\n\
1.4qzfs Make WinExecWait arg const to eliminate GetBuffer. \r\n\
1.4qzfr Upgrade GetBuffr for Unicode build. \r\n\
1.4qzfq Note places GetBuffer OK because immediately written. \r\n\
1.4qzfp Don't need Prepend in Unicode because Str8 includes it. \r\n\
1.4qzfn Make sure the content is available after GetBuffr. \r\n\
1.4qzfm Upgrade GetBuffr for Unicode build. \r\n\
1.4qzfk Upgrade GetBuffr for Unicode build. \r\n\
1.4qzfj Update TrimContents to TrimRight for Unicode build. \r\n\
1.4qzfh Fix bug of export losing show properties setting. \r\n\
1.4qzfg Show dialog box on CCT export if show properties is on. \r\n\
1.4qzff Generalize CCTable db type to any name that starts that way. \r\n\
1.4qzfe Fix problem (1.4qta) of displaying too much in list box. \r\n\
1.4qzfd Fix problem (1.4qta) of showing garbage in blank line. \r\n\
1.4qzfc If no window did parallel move, and view is browse, try with key. \r\n\
1.4qzfb Fix bug of not changing keyboard when coming out of browse. \r\n\
1.4qzfa Version info distinguish Win 98 build from Unicode build. \r\n\
1.4qze Correct U path failure, successfully loads file. \r\n\
1.4qzd Upgrade U pDocument for Unicode build. \r\n\
1.4qzc Correct U Cstrg that called wrong function. \r\n\
1.4qzb Correct U problem of crash on file open. \r\n\
1.4qza Upgrade debug Project entry point for Unicode build. \r\n";

sVersionHistory += "\r\n\
1.4qyzg Upgrade more sVarPattern for Unicode build. \r\n\
1.4qyzf Upgrade more SetAtGrow for Unicode build. \r\n\
1.4qyze Upgrade more SetAtGrow for Unicode build. \r\n\
1.4qyzd Upgrade SetAtGrow for Unicode build. \r\n\
1.4qyzc Upgrade sVarCharList for Unicode build. \r\n\
1.4qyzb Upgrade SetAt for Unicode build. \r\n\
1.4qyza Upgrade SetSize for Unicode build. \r\n\
1.4qyz Upgrade CStringArray for Unicode build. \r\n\
1.4qyy Upgrade RemoveAll for Unicode build. \r\n\
1.4qyx Upgrade Lookup for Unicode build. \r\n\
1.4qyw Upgrade GetLocaleInfo for Unicode build. \r\n\
1.4qyv Upgrade TextOut for Unicode build. \r\n\
1.4qyu Upgrade OnChar for Unicode build. \r\n\
1.4qyt Add Str8 Find with starting place for CStrg compatibility. \r\n\
1.4qys Upgrade strcpy for Unicode build. \r\n\
1.4qyr Upgrade lfFaceName for Unicode build. \r\n\
1.4qyp Upgrade CompareNoCase for Unicode build. \r\n\
1.4qyn Upgrade _fullpath for Unicode build. \r\n\
1.4qym Upgrade bFileExists for Unicode build. \r\n\
1.4qyk Upgrade CommandLine_Args for Unicode build. \r\n\
1.4qyj Upgrade _tcsdup for Unicode build. \r\n\
1.4qyh Upgrade ReapplyFilter for Unicode build. \r\n\
1.4qyg Upgrade RegOpenKeyEx for Unicode build. \r\n\
1.4qyf Upgrade RegOpenKeyEx for Unicode build. \r\n\
1.4qye Upgrade LoadLibrary for Unicode build. \r\n\
1.4qyd Upgrade GetLBText for Unicode build. \r\n\
1.4qyc Upgrade Dir for Unicode build. \r\n\
1.4qyb Upgrade LoadString for Unicode build. \r\n\
1.4qya Upgrade RegisterWindowMessage for Unicode build. \r\n";

sVersionHistory += "\r\n\
1.4qxz Upgrade AddFilesWithExt for Unicode build. \r\n\
1.4qxy Upgrade FindFirstFile for Unicode build. \r\n\
1.4qxw Upgrade ShellExecute for Unicode build. \r\n\
1.4qxv Upgrade stricmp for Unicode build. \r\n\
1.4qxu Upgrade Add for Unicode build. \r\n\
1.4qxt Upgrade RegisterClipboardFormat for Unicode build. \r\n\
1.4qxs Upgrade GetPathName for Unicode build. \r\n\
1.4qxr Upgrade CPropertySheet for Unicode build. \r\n\
1.4qxp Upgrade GetDlgItemText for Unicode build. \r\n\
1.4qxn Upgrade WritePrivateProfileString for Unicode build. \r\n\
1.4qxm Upgrade GetPrivateProfileString for Unicode build. \r\n\
1.4qxk Upgrade GetPrivateProfileString for Unicode build. \r\n\
1.4qxj Make string conversion from char* directly to UTF16. \r\n\
1.4qxh Make sPrivateProfileString for Unicode build. \r\n\
1.4qxg Upgrade GetPrivateProfileString for Unicode build. \r\n\
1.4qxf Upgrade GetPathName for Unicode build. \r\n\
1.4qxe Upgrade OpenDocumentFile for Unicode build. \r\n\
1.4qxd Upgrade OnSaveDocument for Unicode build. \r\n\
1.4qxc Upgrade SetTitle for Unicode build. \r\n\
1.4qxb Upgrade SetPathName for Unicode build. \r\n\
1.4qxa Upgrade _chmod for Unicode build. \r\n";

sVersionHistory += "\r\n\
1.4qwy Upgrade CStdioFile for Unicode build. \r\n\
1.4qwx Upgrade GetLocaleInfo for Unicode build. \r\n\
1.4qww Upgrade RegQueryValue for Unicode build. \r\n\
1.4qwv Upgrade GetPathName for Unicode build. \r\n\
1.4qwu Upgrade SetText for Unicode build. \r\n\
1.4qwt Upgrade GetPathName for Unicode build. \r\n\
1.4qws Upgrade GetPathName for Unicode build. \r\n\
1.4qwr Upgrade TextOut for Unicode build. \r\n\
1.4qwp Upgrade wvsprintf for Unicode build. \r\n\
1.4qwn Upgrade GetTextExtent for Unicode build. \r\n\
1.4qwm qqe Temp upgrade Lookup for Unicode build. \r\n\
1.4qwk qqd Temp upgrade WriteOutputEntry for Unicode build. \r\n\
1.4qwj qqc Temp upgrade CStdioFileEx for Unicode build. \r\n\
1.4qwh qqb Temp upgrade SetAt for Unicode build. \r\n\
1.4qwg qqa Temp upgrade WriteString for Unicode build. \r\n\
1.4qwf Upgrade pdocGetDocByPath for Unicode build. \r\n\
1.4qwe Change wsprintf to wsprintfA for Unicode build. \r\n\
1.4qwd Undo 1.4qwb and 1.4qwc. \r\n\
1.4qwc Upgrade SetAtGrow for Unicode build. \r\n\
1.4qwb Upgrade CBookList for Unicode build. \r\n\
1.4qwa Upgrade AddTail for Unicode build. \r\n";

sVersionHistory += "\r\n\
1.4qvx Upgrade GetStatus for Unicode build. \r\n\
1.4qvw Upgrade Open for Unicode build. \r\n\
1.4qvv Upgrade Remove for Unicode build. \r\n\
1.4qvu Upgrade sGetDirPath for Unicode build. \r\n\
1.4qvt Upgrade csfWordIndex for Unicode build. \r\n\
1.4qvs Upgrade AddString for Unicode build. \r\n\
1.4qvr Upgrade FindStringExact for Unicode build. \r\n\
1.4qvp Upgrade GetText for Unicode build. \r\n\
1.4qvn Upgrade GetText for Unicode build. \r\n\
1.4qvm Upgrade GetText for Unicode build. \r\n\
1.4qvk Upgrade GetText and FindStringExact for Unicode build. \r\n\
1.4qvj Upgrade sGetFileName for Unicode build. \r\n\
1.4qvh Upgrade GetText for Unicode build. \r\n\
1.4qvg Upgrade GetLBText for Unicode build. \r\n\
1.4qvf Upgrade AddString for Unicode build. \r\n\
1.4qve Upgrade FindStringExact for Unicode build. \r\n\
1.4qvd Upgrade GetText for Unicode build. \r\n\
1.4qvc Upgrade Insert for Unicode build. \r\n\
1.4qvb Add Insert to Str8 for CStrg compatibility. \r\n\
1.4qva Add Delete to Str8 for CStrg compatibility. \r\n";

sVersionHistory += "\r\n\
1.4quy Upgrade GetPathName for Unicode build. \r\n\
1.4qux Upgrade AddString for Unicode build. \r\n\
1.4quw Upgrade CPropertySheet for Unicode build. \r\n\
1.4quv Upgrade SetTitle for Unicode build. \r\n\
1.4quu Upgrade GetTextExtent for Unicode build. \r\n\
1.4qut Upgrade wsprintf for Unicode build. \r\n\
1.4qus Upgrade SetTitle for Unicode build. \r\n\
1.4qur Upgrade CPropertySheet for Unicode build. \r\n\
1.4quq Upgrade GetFilePath for Unicode build. \r\n\
1.4qup Upgrade GetPathName for Unicode build. \r\n\
1.4qun Upgrade FindWindow for Unicode build. \r\n\
1.4qum Upgrade pfnmSearch_AddIfNew for Unicode build. \r\n\
1.4quk Upgrade AddString for Unicode build. \r\n\
1.4quj Upgrade GetWindowsDirectory for Unicode build. \r\n\
1.4quh Upgrade GetPathName for Unicode build. \r\n\
1.4qug Upgrade GetStatus for Unicode build. \r\n\
1.4quf Upgade OnOpenDocuument for Unicode build. \r\n\
1.4que Upgrade wsprintf for Unicode build. \r\n\
1.4qud Upgrade GetPathName for Unicode build. \r\n\
1.4quc Upgrade LoadLibrary for Unicode build. \r\n\
1.4qub Upgrade GetPathName for Unicode build. \r\n\
1.4qua Upgrade wsprintf for Unicode build. \r\n";

sVersionHistory += "\r\n\
1.4qtx Upgrade lstrlen for Unicode build. \r\n\
1.4qtw Upgrade CStdioFileEx for Unicode build. \r\n\
1.4qtv Upgrade SetTitle for Unicode build. \r\n\
1.4qtu Upgrade CPropertySheet for Unicode build. \r\n\
1.4qtt Get rid of calls to AfxTrace for Unicode build. \r\n\
1.4qts Upgrade GetLocaleInfo for Unicode build. \r\n\
1.4qtr Upgrade GetDateFormat for Unicode build. \r\n\
1.4qtq Upgrade GetTimeFormat for Unicode build. \r\n\
1.4qtp Disable release trace code. \r\n\
1.4qto Upgrade RetSetValue for Unicode build. \r\n\
1.4qtn Upgrade mciGetErrorString for Unicode build. \r\n\
1.4qtm Upgrade some GetTextExtent for Unicode build. \r\n\
1.4qtk Upgrade some GetTextExtent for Unicode build. \r\n\
1.4qtj Disable unused TextOut. \r\n\
1.4qth Upgrade LoadLibrary for Unicode build. \r\n\
1.4qtg Upgrade some GetTextExtent for Unicode build. \r\n\
1.4qtf Upgrade a FindString for Unicode build. \r\n\
1.4qte Upgrade a FindString for Unicode build. \r\n\
1.4qtd Upgrade some GetTextExtent for Unicode build. \r\n\
1.4qtc Undo 1.4qtb because it didn't work. \r\n\
1.4qtb Upgrade CFileList ref for Unicode build. \r\n\
1.4qta Upgrade all ExtTextOut for Uniocde build. \r\n";

sVersionHistory += "\r\n\
1.4qs Oct 2005 Test version to one user to change interlinear to Unicode. \r\n\
1.4qrh On import, if tabs in interlinear, align the interlinear. \r\n\
1.4qrg Add func to see if db type is CC Table. \r\n\
1.4qrf Align whole bundle start at current field, for tab align on import. \r\n\
1.4qre Upgrade all listbox InsertString for Unicode. \r\n\
1.4qrd Upgrade all CFileDialog for Unicode build. \r\n\
1.4qrc Fix bug (1.4m) of not jumping to selection that crosses line end. \r\n\
1.4qrb Fix bug (1.4) of not keeping following field marker on add marker. \r\n\
1.4qra Upgrade all mismatch = for Unicode build. \r\n";

sVersionHistory += "\r\n\
1.4qpz Upgrade time output Format call for Unicode build. \r\n\
1.4qpy Upgrade all SetWindowText for Unicode build. \r\n\
1.4qpx Upgrade all GetWindowText for Unicode build. \r\n\
1.4qpw Upgrade all GetDlgItemText for Unicode build. \r\n\
1.4qpv Upgrade all SetDlgItemText for Unicode build. \r\n\
1.4qpu Upgrade WinExecWait for Unicode build. \r\n\
1.4qpt Make string conversion to UTF16 and UTF8 for Unicode build. \r\n\
1.4qps Fix bug (1.4) of interlinear wrap wrong on very narrow wrap . \r\n\
1.4qpr Change regular message box to AfxMessageBox. \r\n\
1.4qpq Make AfxMessage box for Unicode build \r\n\
1.4qpp Use default language in language edit control if none assigned. \r\n\
1.4qpn Update set and get code for all edit language controls. \r\n\
1.4qpm Sample of changes needed for Unicode build SetWindowText. \r\n\
1.4qpk Sample of changes needed for SetDlgItemText of lng edit ctrl. \r\n\
1.4qpj Start to make reshape pull morphs together after XML import. \r\n\
1.4qph Fix bug (1.4qna) of not selecting Search string. \r\n";

sVersionHistory += "\r\n\
1.4qpg Fix bug (1.4qpf) of XML export repeating morphs multiple times. \r\n\
1.4qpf Fix bug (1.4qpe) of XML export annotation cut short at end of line. \r\n\
1.4qpe In XML export, for each morph in morphs of word, output XML. \r\n\
1.4qpd Continue to make XML export do structured interlinear. \r\n\
1.4qpc Find marker that is output of parse. \r\n\
1.4qpb Make database typ visible to XML export. \r\n\
1.4qpa Start to make XML export do structured interlinear. \r\n\
1.4qnk Add func to test whether this is XML export. \r\n\
1.4qnj Fix bug (1.4qpn) of not showing Gloss separator in Lookup dialog. \r\n\
1.4qnh If Unicode lng file, write UTF8 bom at top. \r\n\
1.4qng On consistency check word list, do word identification (partial). \r\n\
1.4qnf Temp use WindowText in edit ctrl until change for IME. \r\n\
1.4qne Fix bug (1.4qnd) of divide by zero in new browse. \r\n\
1.4qnd If view of same type found, use its browse for default on open. \r\n\
1.4qnc Change default browse fields for MDF to lx, ps, ge. \r\n\
1.4qnb Start use WindowText in edit control until change for IME. \r\n\
1.4qna Add funcs to get and set text from CEditLng. \r\n";

sVersionHistory += "\r\n\
1.4qmx Fix bug (1.4qmw) of assert after paste in edit control. \r\n\
1.4qmw Implement paste in edit control. \r\n\
1.4qmv Fix bug (1.4qmu) of not displaying text in edit control. \r\n\
1.4qmu CEditLng store text in member var, not wnd text. \r\n\
1.4qmt Change all CStrg in CEditLng to Str8. \r\n\
1.4qms Derive CEditLng from CEdit to fix combo drop. \r\n\
1.4qmq Prevent loop in export of CCTable. \r\n\
1.4qmp Fix bug (1.4qmd) refreshed exported file in focus, but not in front. \r\n\
1.4qmn Fix bug (1.4qmm) if no dot and file type on CCTable file. \r\n\
1.4qmm Export CCTable to same file name as source. \r\n\
1.4qmk If CCTable, do export without asking for export process. \r\n\
1.4qmj Prevent recursive call of save all files in export of CCTable. \r\n\
1.4qmh On save of CCTable source file, export the table as well. \r\n\
1.4qmf If punctuation list, use it to determine word chars. \r\n\
1.4qme Try to make edit control display Unicode correctly. \r\n\
1.4qmd If auto load exported file already open, refresh it. \r\n\
1.4qmc Change find CLangEditCtrl to CEditLng. \r\n\
1.4qmb Change most CLangEditCtrl to CEditLng. \r\n\
1.4qma Fix focus problem of new language edit control. \r\n\
1.4qk Warn if compare on files not sorted by record mark. \r\n\
1.4qj Make compare work if first field not the same. \r\n\
1.4qh Warn that filtered records won't be compared. \r\n\
1.4qg Warn that hidden fields won't be compared. \r\n\
1.4qf Compare give message if files differ only by secondary sort. \r\n\
1.4qe Check for compare trying to continue from unrelated file. \r\n\
1.4qd Compare files handle continue from wrong window. \r\n\
1.4qc Allow long file names for typ and lng files. \r\n\
1.4qb Add punctuation control to lang enc dialog box. \r\n\
1.4qa Add punctuation to lng file. \r\n";

sVersionHistory += "\r\n\
1.4q Aug 2005 Sent out as test version. \r\n\
1.4p  Aug 2005 Sent out as test version. \r\n\
1.4n  Aug 2005 Sent out as test version. \r\n\
1.4m  Aug 2005 Sent out as test version. \r\n\
1.4k  May 2005 Sent out as test version. \r\n\
1.4h  Apr 2005 Test version for Unicode sequence sort. \r\n\
1.4g  Apr 2005 Given to one person as test version for Unicode sort. \r\n\
1.4d  Feb 2005 Sent out as test version. \r\n\
1.4b  Nov 2004 Sent out as test version. \r\n";

sVersionHistory += "\r\n\
1.4   Nov 2004 Release. \r\n\
1.3   Oct 2004 Release. \r\n\
1.2   Apr 2004 Release. \r\n\
1.1   Oct 2003 Package as maintenance release. \r\n";
#endif // JUNK // 1.5.0ft 
Str8 sVersionInfo = "";
sVersionInfo += "\
Detailed version history: \r\n\r\n";

sVersionInfo += sVersionHistory; // Used to show version history for test versions
	//{{AFX_DATA_INIT(CVersionDlg)
	m_edVersion = "";
	//}}AFX_DATA_INIT
m_edVersion = sVersionInfo;
}


void CVersionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CVersionDlg)
//	DDX_Txt(pDX, IDC_Version, m_edVersion);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CVersionDlg, CDialog)
	//{{AFX_MSG_MAP(CVersionDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVersionDlg message handlers

BOOL CVersionDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

	SetDlgItemText( IDC_Version, swUTF16( m_edVersion ) ); // 1.4qpv

    return TRUE;  // input focus will be set to the dialog's first control
}

void CVersionDlg::OnOK() 
{
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}

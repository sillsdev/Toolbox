# Toolbox Version History

#### Version 1.6.4 May 2019 (Windows 10, Windows 8, Windows 7, Vista and XP, and Mac Windows emulators, Linux Wine)

Fix potential problems when working in Dropbox folders. Do not make
"Backup of ..." files.

#### Version 1.6.3 Jan 2018 (Windows 10, Windows 8, Windows 7, Vista and XP, and Mac Windows emulators, Linux Wine)

Make help not fail on Windows 10.

#### Version 1.6.2 Jul 2015 (Windows 10, Windows 8, Windows 7, Vista and XP, and Mac Windows emulators, Linux Wine)

XML write groups at all levels in save. XML export writes only groups
that have content.

#### Version 1.6.1 Jun 2013 (Windows 8, Windows 7, Vista and XP, and Mac Windows emulators, Linux Wine)

Enhancements and bug fixes.

#### Version 1.6.0 Apr 2013 (Windows 8, Windows 7, Vista and XP, and Mac Windows emulators, Linux Wine)

Add ability to save and read files as XML.

To change a file from standard format to XML, choose File, Save As, and
enter a file name with .xml, .xhtml or .xht as its type. Toolbox will
automatically write the file in XML format. Toolbox will continue to
read and write the file in XML format as long as the file name has .xml,
.xhtml or .xht as its type. Toolbox can only read XML files that it has
written, not files produced by other programs. Toolbox can read files
produced by XML export, but this is not encouraged. When Toolbox detects
this it gives a warning that interlinear layout will be lost.

Toolbox uses "database" start and end tags to mark its database. Users
can add extra information to a Toolbox XML file before and after the
database section, and Toolbox will preserve such information. Such
information can include additional structure, such as HTML5 above the
database start tag. It can also include XML header lines such as a css
style sheet reference.

Users are also allowed to add attributes inside the "lxGroup" tags
inside the database, where "lx" is the record marker. For example, an id
could be added to each dictionary entry as lxGroup id="abc" where the
id="abc" attribute is before the closing wedge of the lxgroup tag. Such
attributes will be preserved by Toolbox.

XML and HTML5 header and footer information and record marker attributes
are stored in save as standard format such that they are restored when
file is again saved as XML. Such information is preserved even when the
standard format file is edited with an older version of Toolbox.

#### Version 1.5.9 Apr 2011 (Windows 7, Vista and XP, and Mac Windows emulators, Linux Wine)

Enhancements and bug fixes.

#### Version 1.5.8 Feb 2010 (Windows 7, Vista and XP, and Mac Windows emulators, Linux Wine)

Enhancements and bug fixes.

#### Version 1.5.7 Nov 2009 (Windows XP and Vista, Mac Windows emulators, Linux Wine)

Enhancements and bug fixes.

#### Version 1.5.5 Jan 2009 (Windows XP and Vista, Mac Windows emulators, Linux Wine)

Enhancements and bug fixes.

#### Version 1.5.4 Sep 2008 (Windows XP and Vista, Mac Windows emulators, Linux Wine)

Enhancements and bug fixes.

#### Version 1.5.3 Feb 2008 (Windows XP and Vista, Mac Windows emulators, Linux Wine)

Enhancements and bug fixes.

#### Version 1.5.2 Jan 2008 (Windows XP and Vista, Mac Windows emulators, Linux Wine)

Enhancements and bug fixes.

#### Version 1.5.1 Feb 2007 (Windows XP and Vista)

Enhancements and bug fixes.

#### Version 1.5.0 Aug 2006 (Windows XP)

Enhancements and bug fixes.

#### Version 1.4 Nov 2004 (Windows 98/ME/XP)

Enhancements and bug fixes.

#### Version 1.3 Oct 2004 (Windows)

Enhancements and bug fixes.

#### Version 1.2 Apr 2004 (Windows)

Enhancements and bug fixes.

#### Version 1.1 Oct 2003 (Windows)

Enhancements and bug fixes.

#### Version 1.0 Sep 2003 (Windows)

This is the first release of Field Linguist's Toolbox.

## A Brief History of Shoebox/Toolbox

Shoebox was written as a DOS program in 1987 by an SIL field linguist
named John Wimbish. He enhanced it through 1991, producing versions 1.0,
and 1.2. From 1991-1993 SIL International enhanced DOS Shoebox,
producing version 2.0. (Programmers Brian Wussow, Mark Pedrotti, et al.)
From 1994-2000 SIL International produced Shoebox for Windows versions
3.0, 4.0, and 5.0. (Project manager Karen Buseman, programmers Alan
Buseman, Mark Pedrotti, Rod Early, Bryan Yoder, Tom Bogle, et al.) In
2002 Shoebox work was moved to an SIL field entity and the name was
changed to Field Linguist's Toolbox. This field entity continues to
enhance Toolbox, producing versions as above. (Programmer Alan Buseman,
support Karen Buseman.)

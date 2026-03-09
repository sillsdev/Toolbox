#pragma once

#define _HAS_STD_BYTE 0
#define _HAS_STD_BYTE_ 0

#include <winsock2.h>
#include <windows.h>
#include <afxwin.h>  // provides BOOL, CString, MFC base classes

typedef unsigned char byte_alias;
#define byte byte_alias

#define ASSERT(x)
#define AFX_DATA
#define AFX_EXT_CLASS
#define afx_msg
#define DECLARE_MESSAGE_MAP() private:

class CDC;
class CDblListEl;
class CComboBox;
class CFont;
class CListBox;
class CWnd;

#include "Shwdefs.h"
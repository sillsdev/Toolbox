// cdblel.cpp CDblListEl Double linked list element class implementation Alan Buseman 25 Jan 95

#include "stdafx.h"
#include "all.h"  // includes cdbllist.h

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void CDblListEl::SetNum(long lNum)
{
	ASSERT(!m_stableIndex.has_value());
	m_stableIndex = lNum;
}

void CDblListEl::ClearNum()
{
	m_stableIndex.reset();
}

#ifdef _DEBUG
	void CDblListEl::AssertValid() const
		{}
#endif		

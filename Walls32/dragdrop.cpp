#include "stdafx.h"

#if !defined(DRAGDROP_H)
#include "dragdrop.h"
#endif

UINT NEAR WM_CANDROP = ::RegisterWindowMessage("TEST_CANDROP");
UINT NEAR WM_DROP = ::RegisterWindowMessage("TEST_DROP");

IMPLEMENT_DYNAMIC(CDragQuery, CObject)
IMPLEMENT_DYNAMIC(CDragStruct, CDragQuery)

CDragQuery::CDragQuery(CWnd* pWnd, const CPoint& pt, BOOL bCopy, WORD type)
{
	m_pServerWnd = pWnd;
	m_ClientPt = pt;
	m_bDragCopy = bCopy;
	m_DragType = type;
}

CDragStruct::CDragStruct(CWnd* pWnd, const CPoint& pt, BOOL bCopy, WORD type, DWORD item)
	: CDragQuery(pWnd, pt, bCopy, type)
{
	m_DragItem = item;
}

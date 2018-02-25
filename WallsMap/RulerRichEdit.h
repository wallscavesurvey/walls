#if !defined(AFX_RULERRICHEDIT_H__E10A8ED3_2E1D_402E_A599_003214085F1A__INCLUDED_)
#define AFX_RULERRICHEDIT_H__E10A8ED3_2E1D_402E_A599_003214085F1A__INCLUDED_

// RulerRichEdit.h : header file
//
#include "FileDropTarget.h"

/////////////////////////////////////////////////////////////////////////////
// CRulerRichEdit window

class CRulerRichEdit : public CRichEditCtrl
{
public:
// Construction/creation/destruction
	CRulerRichEdit();
	virtual ~CRulerRichEdit();

	BOOL Create( DWORD style, CRect rect, CWnd* parent );

	CFileDropTarget m_dropTarget;

protected:
// Message handlers
	//afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnChange();
	afx_msg UINT OnGetDlgCode();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLink(NMHDR *,LRESULT *);

};

#endif // !defined(AFX_RULERRICHEDIT_H__E10A8ED3_2E1D_402E_A599_003214085F1A__INCLUDED_)

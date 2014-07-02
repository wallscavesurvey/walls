#if !defined(AFX_XEDIT_H__22DE2BCC_1D68_484C_9B71_26FAA6142B98__INCLUDED_)
#define AFX_XEDIT_H__22DE2BCC_1D68_484C_9B71_26FAA6142B98__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// XEdit.h : header file
//

void UpdateCaret(CEdit*, BOOL = TRUE);
void DeleteNextChar(CEdit*);

/////////////////////////////////////////////////////////////////////////////
// CXEdit window

class CXEdit : public CEdit
{
// Construction
public:
	CXEdit();
	virtual ~CXEdit();

	// Attributes
public:
	bool m_bReadOnly;

private:
	int m_Start, m_End;

	// Overrides
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);

protected:
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XEDIT_H__22DE2BCC_1D68_484C_9B71_26FAA6142B98__INCLUDED_)

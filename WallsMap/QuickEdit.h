#if !defined(AFX_QUICKEDITCELL_H__2EB671B5_0711_11D3_90AB_00E029355177__INCLUDED_)
#define AFX_QUICKEDITCELL_H__2EB671B5_0711_11D3_90AB_00E029355177__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EditCell.h : header file
//

//#include "quicklist.h"
class CQuickList;

#define MES_UNDO			_T("&Undo")
#define MES_CUT				_T("Cu&t")
#define MES_COPY			_T("&Copy")
#define MES_PASTE			_T("&Paste")
#define MES_DELETE			_T("&Delete")
#define MES_SELECTALL		_T("Select &All")
#define ME_SELECTALL		WM_USER + 0x7000

/*
	The code in this class is very much based on Lee Nowotny's article
	"Easy Navigation Through an Editable List View",
	http://www.codeproject.com/listctrl/listeditor.asp

	And that article is based on another article by Zafir Anjum, "Editable Subitems":
	http://www.codeguru.com/listview/edit_subitems.shtml

	However, currently most of it is now rewritten. But I thought it
	would be unfair to not mention it.
*/

/////////////////////////////////////////////////////////////////////////////
// CQuickEdit window
class CQuickEdit : public CEdit
{
public:
	CQuickEdit(CQuickList* pCtrl, int iItem, int iSubItem,
		HFONT hFont, CString sInitText, BYTE cTyp, bool endonlostfocus);
	virtual ~CQuickEdit();

	void SetEndOnLostFocus(bool autoend);
	void StopEdit(UINT endkey = 0);
	bool IsChanged();
	bool IsReadOnly() { return m_bReadOnly; }

	//{{AFX_VIRTUAL(CQuickEdit)
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

protected:
	CQuickList* pListCtrl;
	int			Item;
	int			SubItem;
	CString		InitText;
	HFONT		m_hFont;

	bool		m_endOnLostFocus;
	bool		m_bReadOnly;
	bool		m_isClosing;
	bool		m_bChanged;
	BYTE		m_bAccent;
	BYTE		m_cTyp;
	char		m_pYN[4], m_pTF[4];
	char		m_pLogStr[2];
	LPSTR		m_pLogChar;
	int			m_iLogChar;

	//{{AFX_MSG(CQuickEdit)
	afx_msg void OnChange();
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg UINT OnGetDlgCode();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//}}AFX_MSG
	//afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QUICKEDITCELL_H__2EB671B5_0711_11D3_90AB_00E029355177__INCLUDED_)

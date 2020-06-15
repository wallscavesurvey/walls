
#if !defined(_TABCOMBO_H)
#define _TABCOMBO_H
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

typedef void(*TABCOMBO_CB)(void);

class CComboHistItem
{
public:
	CComboHistItem(CComboHistItem *pNext, CString &str, WORD flags = 0) : m_pNext(pNext), m_wFlags(flags), m_Str(str) {};
	virtual ~CComboHistItem() {
		delete m_pNext;
		m_pNext = NULL;
	}
	CComboHistItem *m_pNext;
	WORD m_wFlags;
	CString m_Str;
};

class CTabComboHist
{
public:
	CTabComboHist(int maxItems) : m_nMaxItems(maxItems), m_nItems(0), m_pFirst(NULL)
	{
	}

	virtual ~CTabComboHist() {
		delete m_pFirst;
	}

	void InsertAsFirst(CString &str, WORD flags = 0);
	BOOL IsEmpty() { return m_nItems == 0; }
	LPCSTR GetFirstString() { return m_nItems ? m_pFirst->m_Str : ""; }
	WORD GetFirstFlags() { return m_nItems ? m_pFirst->m_wFlags : 0; }
	CComboHistItem *GetItem(int index);

	int m_nItems;
	int m_nMaxItems;
	CComboHistItem *m_pFirst;
};

class CTabCombo : public CComboBox
{
	// Construction
public:
	CTabCombo();

	// Attributes
public:

	// Operations
public:

	void LoadHistory(CTabComboHist *pHist);

	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CTabCombo)
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTabCombo();

	// Generated message map functions
protected:
	//{{AFX_MSG(CTabCombo)
	afx_msg UINT OnNcHitTest(CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

extern CTabComboHist hist_prjfind, hist_prjrepl;
void OnSelChangeFindStr(CDialog *pDlg, BOOL bGlobal);

#endif

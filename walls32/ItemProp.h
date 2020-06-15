#if !defined(AFX_ITEMPROP_H__3556F7A2_D2E4_11D1_AFA3_000000000000__INCLUDED_)
#define AFX_ITEMPROP_H__3556F7A2_D2E4_11D1_AFA3_000000000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ItemProp.h : header file
//

#include "DisCombo.h"

class CPrjDoc;
class CPrjListNode;
class CItemProp;
class CPrjView;

extern BOOL hPropHook;
extern CItemProp *pItemProp;
extern CPrjView *pPropView;

/////////////////////////////////////////////////////////////////////////////
// CChangeEdit window

class CChangeEdit : public CEdit
{
	// Construction
public:
	void Init(CDialog *pDlg, UINT idEdit, UINT idText);
	CDialog *m_pDlg;
	CChangeEdit() { m_pDlg = NULL; }

	// Attributes
	BOOL m_bAbsolute;
private:
	int m_idText;
	CString m_cBuf;

	// Operations
public:

	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CChangeEdit)
		//}}AFX_VIRTUAL

		// Generated message map functions
	//protected:
		//{{AFX_MSG(CChangeEdit)
	afx_msg void OnChange();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
#ifdef _DEBUG
	DECLARE_DYNAMIC(CChangeEdit)
#endif
};

class CItemPage : public CPropertyPage
{
	//	DECLARE_DYNCREATE(CItemPage)

public:
	CItemPage(UINT nIDTemplate) : CPropertyPage(nIDTemplate) {}
	~CItemPage() {}
	virtual BOOL ValidateData() = 0;
	virtual void Init(CItemProp *pSheet) = 0;
	virtual UINT * getids() = 0;

	CItemProp * Sheet();

	void Show(UINT idc, BOOL bShow)
	{
		GetDlgItem(idc)->ShowWindow(bShow ? SW_SHOW : SW_HIDE);
	}

	//static BOOL m_bChanged;
	//static BOOL m_bChangeAllow;

protected:
	UINT m_idLastFocus;

	// Overrides
	afx_msg LRESULT OnRetFocus(WPARAM, LPARAM);

	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CItemPage)
public:
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	virtual void OnCancel();
	virtual void OnOK();
	//}}AFX_VIRTUAL

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CItemGeneral dialog

class CItemGeneral : public CItemPage
{
	DECLARE_DYNCREATE(CItemGeneral)

	friend class CItemProp;

public:
	void UpdateGenCtrls(void);
	void UpdateSurveyOther();

	// Dialog Data
		//{{AFX_DATA(CItemGeneral)
	enum { IDD = IDD_ITEM_GENERAL };
	CString	m_szName;
	CString	m_szTitle;
	BOOL	m_bDefineSeg;
	CString	m_szPath;
	//}}AFX_DATA
	int m_iLaunch;

	// Construction
	CItemGeneral() : CItemPage(CItemGeneral::IDD) {}
	~CItemGeneral() {}

	BOOL m_bFeetUnits;
	BOOL m_bSurvey, m_bBook, m_bOther, m_bReadOnly;
	BOOL m_bFilesHaveMoved;
	BOOL m_bClickedReadonly;

private:
	CPrjListNode *m_pParent;
	CChangeEdit m_ePath;
	void Enable(UINT id, BOOL bEnable) { GetDlgItem(id)->EnableWindow(bEnable); }
	void Check(UINT id, BOOL bCheck) { ((CButton *)GetDlgItem(id))->SetCheck(bCheck); }
	void UpdateSizeDate(char *pathbuf);

	// Overrides

public:
	virtual BOOL ValidateData(void);
	virtual void Init(CItemProp *pSheet);
	virtual UINT * getids();
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CItemGeneral)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
	// Generated message map functions
	//{{AFX_MSG(CItemGeneral)
	afx_msg void OnSurvey();
	afx_msg void OnBrowse();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedReadonly();
};

/////////////////////////////////////////////////////////////////////////////
// CItemReference dialog

class CItemReference : public CItemPage
{
	DECLARE_DYNCREATE(CItemReference)

	// Construction
public:
	CItemReference() : CItemPage(CItemReference::IDD) {}
	~CItemReference() {}

	// Dialog Data
	PRJREF m_Ref;

	//{{AFX_DATA(CItemReference)
	enum { IDD = IDD_ITEM_REFERENCE };
	BOOL	m_bDateInherit;
	BOOL	m_bDateMethod;
	BOOL	m_bGridInherit;
	BOOL	m_bGridType;
	BOOL	m_bRefInherit;
	BOOL	m_bRefClear;
	//}}AFX_DATA

private:
	CPrjListNode *m_pParent;
	BOOL m_bConvUpdated;
	void SetText(UINT id, char *pText)
	{
		((CEdit *)GetDlgItem(id))->SetWindowText(pText);
	}
	int GetText(UINT id, char *pText, int sizbuf)
	{
		return ((CEdit *)GetDlgItem(id))->GetWindowText(pText, sizbuf);
	}
	void Enable(UINT id, BOOL bEnable) { GetDlgItem(id)->EnableWindow(bEnable); }
	void Check(UINT id, BOOL bCheck) { ((CButton *)GetDlgItem(id))->SetCheck(bCheck); }
	void UpdateRefCtrls();


	// Overrides
public:
	virtual BOOL ValidateData(void);
	virtual void Init(CItemProp *pSheet);
	virtual UINT * getids();

	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CItemReference)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
	// Generated message map functions
	//{{AFX_MSG(CItemReference)
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnDateInherit();
	afx_msg void OnGridInherit();
	afx_msg void OnRefChange();
	afx_msg void OnRefClear();
	afx_msg void OnRefInherit();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CItemOptions dialog

class CItemOptions : public CItemPage
{
	DECLARE_DYNCREATE(CItemOptions)

	// Construction
public:
	CItemOptions();
	~CItemOptions() {}

	// Dialog Data
		//{{AFX_DATA(CItemOptions)
	enum { IDD = IDD_ITEM_OPTIONS };
	CDisabledCombo	m_DisCombo;
	CString	m_szOptions;
	BOOL	m_bInheritLength;
	BOOL	m_bInheritVertical;
	BOOL	m_bPreserveLength;
	BOOL	m_bPreserveVertical;
	BOOL	m_bInheritView;
	int		m_iNorthEast;
	BOOL	m_bSVG_process;
	//}}AFX_DATA

	void Enable(UINT id, BOOL bEnable) { GetDlgItem(id)->EnableWindow(bEnable); }
	void Check(UINT id, BOOL bCheck) { ((CButton *)GetDlgItem(id))->SetCheck(bCheck); }
	void EnableView(BOOL bEnable);
private:
	void InsertComboOption(CPrjListNode *pNode);

	// Overrides
public:
	virtual BOOL ValidateData(void);
	virtual void Init(CItemProp *pSheet);
	virtual UINT * getids();

	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CItemOptions)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
	// Generated message map functions
	//{{AFX_MSG(CItemOptions)
	afx_msg void OnInheritLength();
	afx_msg void OnInheritVertical();
	afx_msg void OnInheritView();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnCbnSelchangeOptions();
};

/////////////////////////////////////////////////////////////////////////////
// CItemProp -- Property sheet declaration

class CItemProp : public CPropertySheet
{
	DECLARE_DYNAMIC(CItemProp)

	// Construction
public:
	CItemProp(CPrjDoc *pDoc, CPrjListNode *pNode, BOOL bNew);

	// Attributes
public:
	CPrjDoc *m_pDoc;
	CPrjListNode *m_pNode;
	CPrjListNode *m_pParent;
	BOOL m_bNew;
	UINT m_uFlags;

	CItemGeneral m_genDlg;
	CItemOptions m_optDlg;
	CItemReference m_refDlg;

	// Operations
	CItemPage * GetActivePage() { return (CItemPage *)CPropertySheet::GetActivePage(); }

public:
	CItemGeneral * GetGenDlg() { return &m_genDlg; }
	CItemOptions * GetOptDlg() { return &m_optDlg; }
	CItemReference * GetRefDlg() { return &m_refDlg; }
	void SetPropTitle();
	void SetLastIndex() { if (!m_bNew) m_pDoc->m_nActivePage = GetActiveIndex(); }
	void EndItemProp(int id);
	void UpdateReadOnly(BOOL bReadOnly) {
		m_genDlg.Enable(IDC_READONLY, TRUE);
		m_genDlg.Check(IDC_READONLY, bReadOnly);
		m_genDlg.m_bReadOnly = bReadOnly;
		m_genDlg.m_bClickedReadonly = FALSE;
	}

	// Overrides
	afx_msg LRESULT OnRetFocus(WPARAM, LPARAM);
	//***7.1 afx_msg LRESULT OnPropViewLB(CPrjDoc *pDoc,LPARAM);
	afx_msg LRESULT OnPropViewLB(WPARAM wParam, LPARAM);

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CItemProp)
public:
	virtual BOOL OnInitDialog();
protected:
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CItemProp() {}

	// Generated message map functions
protected:
	//{{AFX_MSG(CItemProp)
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ITEMPROP_H__3556F7A2_D2E4_11D1_AFA3_000000000000__INCLUDED_)

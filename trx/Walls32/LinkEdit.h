#if !defined(AFX_LINKEDIT_H__5CE719E5_5C0B_11D1_B46B_98049A72CF3A__INCLUDED_)
#define AFX_LINKEDIT_H__5CE719E5_5C0B_11D1_B46B_98049A72CF3A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// LinkEdit.h : header file
//

#ifndef __AFXCMN_H__
#include <afxcmn.h>
#endif

typedef int (* LINKEDIT_CB)(int fcn);

//controls context menu options --
enum {BTYP_INT,BTYP_FSEC,BTYP_FMIN,BTYP_FDEG,BTYP_FUTM};
#define ME_COPYCOORDS	(WM_USER+0x7000)
#define ME_PASTECOORDS	(WM_USER+0x7001)

#define MES_COPYCOORDS	_T("Copy coordinates")
#define MES_PASTECOORDS	_T("Paste coordinates")
#define MES_UNDO			_T("&Undo")
#define MES_CUT				_T("Cu&t")
#define MES_COPY			_T("&Copy")
#define MES_PASTE			_T("&Paste")
#define MES_DELETE			_T("&Delete")

/////////////////////////////////////////////////////////////////////////////
// CLinkEdit base class

class CLinkEditI;

class CLinkEdit : public CEdit
{
// Construction
public:
	static CLinkEdit *m_pFocus;
	static int m_bSetting;
	static bool m_bNoContext;

	LINKEDIT_CB m_pCB;
	CLinkEditI *m_pLinkL; //control on left
	CLinkEdit *m_pLinkR; //control on right (can change)
	CMagDlg *m_pMagDlg;

	CLinkEdit()
		: m_bType(BTYP_INT)
		, m_pLinkL(NULL)
		, m_pLinkR(NULL)
		, m_pMagDlg(NULL)
	{}

protected:
	BOOL m_bGrayed;
	BYTE m_bType;
	CString m_csSaved;
	DWORD m_dwSavedPos;

// Overrides
	BOOL PreTranslateMessage(MSG* pMsg);

// Operations
	virtual LPCSTR GetRangeErrMsg(char *buf)=0;
	void ShowRangeErrMsg();

public:
	virtual BOOL InRange()=0;
	virtual void UpdateText(BOOL bValid)=0;
	virtual BOOL RetrieveText()=0;
	virtual void SetMin()=0;
	
// Overrides

	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);

	// message map functions
private:
	afx_msg LRESULT OnCut(WPARAM, LPARAM);
	afx_msg LRESULT OnClear(WPARAM, LPARAM);
	afx_msg LRESULT OnPaste(WPARAM, LPARAM);

	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg LRESULT OnRetFocus(WPARAM,LPARAM);

	//{{AFX_MSG(CLinkEdit)
	afx_msg void OnChange();
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
#ifdef _DEBUG
	DECLARE_DYNAMIC(CLinkEdit)
#endif
};

class CLinkEditI : public CLinkEdit
{
// Construction
public:
	CLinkEditI() : m_bZoneType(false) {}
	void Init(CDialog *pdlg,int id,int min,int max,int len,
		int *pI,CLinkEditI *pLinkL,CLinkEdit *pLinkR,LINKEDIT_CB pCB,CMagDlg *pDlg=NULL);

// Attributes
protected:
	int *m_pI;
	int m_max,m_min;
	bool m_bZoneType; //check control key in IncSpin

// Operations
	virtual LPCSTR GetRangeErrMsg(char *buf);

public:
	void IncSpin(int dir);
	virtual BOOL InRange();
	virtual void UpdateText(BOOL bValid);
	virtual BOOL RetrieveText();
	virtual void SetMin();
	void IncVal(int dir);
	void SetZoneType() {m_bZoneType=true;}

protected:

	DECLARE_MESSAGE_MAP()
#ifdef _DEBUG
	DECLARE_DYNAMIC(CLinkEditI)
#endif
};

/////////////////////////////////////////////////////////////////////////////
class CLinkEditF : public CLinkEdit
{
// Construction
public:
	void Init(CDialog *pdlg,int id,double min,double max,int len,
		double *pI,BYTE bType,CLinkEditI *plinkL,LINKEDIT_CB pCB,CMagDlg *pDlg=NULL);

// Attributes

	double *m_pF;
	double m_fmin,m_fmax;
	int m_len;

/// Implementation
	virtual BOOL InRange();
	virtual void UpdateText(BOOL bValid);
	virtual BOOL RetrieveText();
	virtual void SetMin();
private:
	virtual LPCSTR GetRangeErrMsg(char *buf);

protected:
	DECLARE_MESSAGE_MAP()
#ifdef _DEBUG
	DECLARE_DYNAMIC(CLinkEditF)
#endif
};

/////////////////////////////////////////////////////////////////////////////
// CSpinEdit window

class CSpinEdit : public CSpinButtonCtrl
{
public:

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSpinEdit)
	//}}AFX_VIRTUAL

// Implementation
public:
	void Init(CDialog *pDlg,int id,CLinkEditI *pEdit)
	{
		SubclassDlgItem(id,pDlg);
		m_pEdit=pEdit;
	}

	// Generated message map functions
protected:
	CLinkEditI *m_pEdit;

	//{{AFX_MSG(CSpinEdit)
	afx_msg void OnDeltapos(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LINKEDIT_H__5CE719E5_5C0B_11D1_B46B_98049A72CF3A__INCLUDED_)

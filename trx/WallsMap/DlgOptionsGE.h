#pragma once
#include "afxwin.h"

#include "EditLabel.h"

// CDlgOptionsGE dialog

class CDlgOptionsGE : public CDialog
{
	DECLARE_DYNAMIC(CDlgOptionsGE)

public:
	CDlgOptionsGE(CShpLayer *pShp,VEC_DWORD &vRec,CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgOptionsGE();

// Dialog Data
	enum { IDD = IDD_OPTIONS_GE };

private:
	void InitLabelFields();
	void UpdateRecControls();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	CString m_pathName;
	bool m_bFlyEnabled;

private:
	bool m_bFldChg;
	CEdit m_ceDescription;
	CEditLabel m_cePathName;
	CShpLayer *m_pShp;
	VEC_DWORD &m_vRec;
	VEC_BYTE m_vFld;
	UINT m_uRecIdx;
	UINT m_uRecCount;

public:
	afx_msg void OnBnClickedSave();
	afx_msg void OnBnClickedClear();
	afx_msg void OnBnClickedAppend();
	afx_msg void OnBnClickedClrLast();
	afx_msg void OnBnClickedBrowse();
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
	afx_msg void OnBnClickedNext();
	afx_msg void OnBnClickedPrev();
	afx_msg void OnBnClickedRemoveRec();
};

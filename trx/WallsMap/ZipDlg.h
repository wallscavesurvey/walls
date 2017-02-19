#pragma once
#include "EditLabel.h"

// CZipDlg dialog
class CWallsMapDoc;

class CZipDlg : public CDialog
{
	DECLARE_DYNAMIC(CZipDlg)

public:
	CZipDlg(CWallsMapDoc *pDoc,CWnd* pParent = NULL);   // standard constructor
	virtual ~CZipDlg();

// Dialog Data
	enum { IDD = IDD_ZIPDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	void SetText(UINT id,LPCSTR text) {GetDlgItem(id)->SetWindowText(text);}
	afx_msg void OnBnClickedBrowse();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

	int AddFileToZip(LPCSTR pFilePath,int iTyp);
	int AddShpToZip(CShpLayer *pShp);

private:
	CEditLabel m_cePathName;
	CString m_pathName;
	CWallsMapDoc *m_pDoc;
	BOOL m_bShpOnly,m_bPtsOnly;
	BOOL m_bIncHidden;
	BOOL m_bIncLinked;
	LPSTR m_errmsg;
public:
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
};

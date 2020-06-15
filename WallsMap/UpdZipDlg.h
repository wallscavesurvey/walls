#pragma once

#ifndef _zip_H
#include "zip.h"
#endif

class CShpLayer;

class CUpdZipDlg : public CDialog
{
	DECLARE_DYNAMIC(CUpdZipDlg)

public:
	CUpdZipDlg(CShpLayer *pShp, LPCSTR pathName, int iFlags, CWnd* pParent = NULL)
		: m_pShp(pShp)
		, m_pathName(pathName)
		, m_iFlags(iFlags)
		, m_bAddShp(0)
		, m_bCompleted(false)
		, CDialog(CUpdZipDlg::IDD, pParent) {}

	virtual ~CUpdZipDlg();

	BOOL m_bAddShp;

	// Dialog Data
	enum { IDD = IDD_UPDZIP };

private:
	void OnOK();
	void OnCancel();
	LPCSTR FixPath(LPCSTR ext)
	{
		return ReplacePathExt(m_pathName, ext);
	}
	int AddShpComponent(HZIP hz, CString &path, LPCSTR pExt);
	int CopyShpComponent(HZIP hz, CString &path, LPCSTR pExt);

	void Enable(UINT id, BOOL bEnable) { GetDlgItem(id)->EnableWindow(bEnable); }
	void Show(UINT id, BOOL bShow) { GetDlgItem(id)->ShowWindow(bShow ? SW_SHOW : SW_HIDE); }
	bool IsVisible(UINT id) { return GetDlgItem(id)->IsWindowVisible() == TRUE; }
	void SetText(UINT id, LPCSTR s) { GetDlgItem(id)->SetWindowText(s); }
	void SetS3Test(CString &s0, bool bInit);

	int m_iFlags;
	UINT m_nNew;
	BOOL m_bCompleted;
	CShpLayer *m_pShp;
	CString m_pathName;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	virtual BOOL OnInitDialog();
};

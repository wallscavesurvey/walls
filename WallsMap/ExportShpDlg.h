#pragma once

// CExportShpDlg dialog

#ifndef _SHPLAYER_H
#include "ShpLayer.h"
#endif
#include "EditLabel.h"

class CExportShpDlg : public CDialog
{
	DECLARE_DYNAMIC(CExportShpDlg)

public:
	CExportShpDlg(CShpLayer *pShp,VEC_DWORD &vRec,bool bSelected,CWnd* pParent = NULL);   // standard constructor
	virtual ~CExportShpDlg();

// Dialog Data
	enum { IDD = IDD_EXPORT_SHAPE };

	bool m_bSelected;

private:

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	void SetText(UINT id,LPCSTR text) {GetDlgItem(id)->SetWindowText(text);}
	void GetText(UINT id,CString &text) {GetDlgItem(id)->GetWindowText(text);}
	void Enable(UINT id,BOOL bEnable) {GetDlgItem(id)->EnableWindow(bEnable);}
	void SetCheck(UINT id,BOOL bChk)  {((CButton *)GetDlgItem(id))->SetCheck(bChk);}
	bool Check_template_name(LPCSTR tmppath,LPCSTR shppath);

	DECLARE_MESSAGE_MAP()

	void CopyTemplate();

	CWallsMapDoc *m_pDoc;
	CShpLayer *m_pShp;
	VEC_DWORD &m_vRec;
	BOOL m_bUseViewCoord;
	CString m_pathName;
	CString m_tempName;
	CEditLabel m_cePathName,m_ceTempName;
	BOOL m_bRetainDeleted;

	afx_msg void OnBnClickedBrowseShpdef();
	afx_msg void OnBnClickedBrowse();
	virtual BOOL OnInitDialog();
	BOOL m_bInitEmpty;
	BOOL m_bExcludeMemos;
	BOOL m_bGenTemplate;
	BOOL m_bAddLayer;
	BOOL m_bUseLayout;
	afx_msg void OnBnClickedUseLayout();
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
public:
	BOOL m_bMake2D;
};

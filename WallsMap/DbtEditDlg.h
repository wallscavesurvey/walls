#pragma once

#ifndef _DBTEDITDLG_H
#define _DBTEDITDLG_H

#include "MemoEditDlg.h"
#include "RulerRichEditCtrl.h"
#include "FileDropTarget.h"

// CDbtEditDlg dialog

class CShpLayer;
class CShowShapeDlg;
class CWallsMapDoc;

extern UINT urm_DATACHANGED, urm_LINKCLICKED;

class CDbtEditDlg : public CMemoEditDlg
{
	DECLARE_DYNAMIC(CDbtEditDlg)

public:
	CDbtEditDlg(CDialog *pParentDlg, CShpLayer *pShp, EDITED_MEMO &memo, CWnd* pParent = NULL);   // standard constructor
	virtual ~CDbtEditDlg();

	//virtual bool CanReInit(CShpLayer *pShp,DWORD nRec,UINT nFld);
	virtual void ReInit(CShpLayer *pShp, EDITED_MEMO &memo);
	virtual bool DenyClose(BOOL bForceClose = FALSE);
	virtual bool HasEditedText();
	virtual void SetEditable(bool bEditable);

	static CString m_csLastSaved;

	void SaveEditedText();
	void Destroy()
	{
		m_rtf.CloseFindDlg();
		GetWindowRect(&m_rectSaved);
		DestroyWindow();
	}

	// Dialog Data
	enum { IDD = IDD_DBTEDITDLG };

private:
	void InitData();
	LPCSTR GetMemoFileName(CString &csPath);

	CFileDropTarget m_dropTarget;
	bool WriteToFile(CString &path);

	static CRect m_rectSaved;
	CStatic	m_placement;
	CRulerRichEditCtrl	m_rtf;

	void ChkToolbarButton(BOOL bVisible) { ((CButton *)GetDlgItem(IDC_CHK_TOOLBAR))->SetCheck(bVisible); }

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnOK();
	virtual void OnCancel();
	virtual void PostNcDestroy();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLinkToFile();
	afx_msg void OnSaveToFile();
	afx_msg LRESULT OnDataChanged(WPARAM bFormatRTF, LPARAM);
	afx_msg LRESULT OnLinkClicked(WPARAM, LPARAM lParam);
	afx_msg void OnBnClickedImageMode();
	afx_msg void OnBnClickedToolbar();
	afx_msg void OnCompareWithFile();
	afx_msg void OnReplaceWithFile();
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
};

extern CDbtEditDlg * app_pDbtEditDlg;
#endif



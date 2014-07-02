#pragma once

#ifndef _INFOEDITDLG_H
#define _INFOEDITDLG_H

#include "WallsMapDoc.h"
#include "ResizeDlg.h"
#include "RulerRichEditCtrl.h"
#include "FileDropTarget.h"

// CInfoEditDlg dialog

class CWallsMapDoc;

extern UINT urm_DATACHANGED,urm_LINKCLICKED;

class CInfoEditDlg : public CResizeDlg
{
	DECLARE_DYNAMIC(CInfoEditDlg)

public:
	CInfoEditDlg(UINT nID,CWallsMapDoc *pDoc,CString &csData,CWnd* pParent = NULL);
	virtual ~CInfoEditDlg();

	//virtual bool CanReInit(CShpLayer *pShp,DWORD nRec,UINT nFld);
	virtual void ReInit(CShpLayer *pShp,EDITED_MEMO &memo);
	virtual bool DenyClose(BOOL bForceClose=FALSE);

	bool HasEditedText();
	void SaveEditedText();
	void Destroy()
	{
		m_rtf.CloseFindDlg();
		GetWindowRect(&m_rectSaved);
		DestroyWindow();
	}

// Dialog Data
	enum { IDD = IDD_DBTEDITDLG };

	CWallsMapDoc *m_pDoc;

private:
	void InitData();

	//CFileDropTarget m_dropTarget;

	static CRect m_rectSaved;
	bool m_bEditable;
	CStatic	m_placement;
	CRulerRichEditCtrl	m_rtf;
	CString &m_csData;

	void ChkToolbarButton(BOOL bVisible) {((CButton *)GetDlgItem(IDC_CHK_TOOLBAR))->SetCheck(bVisible);}

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnOK();
	virtual void OnCancel();
	virtual void PostNcDestroy();

	void RefreshTitle();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLinkToFile();
	afx_msg void OnSaveToFile();
	afx_msg LRESULT OnDataChanged(WPARAM bFormatRTF, LPARAM);
	afx_msg LRESULT OnLinkClicked(WPARAM, LPARAM lParam);
	afx_msg void OnBnClickedToolbar();
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
};

extern CInfoEditDlg * app_pInfoEditDlg;
#endif



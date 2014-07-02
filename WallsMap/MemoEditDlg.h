// CMemoEditDlg -- A parent dialog with elements common
// to CDbtEditDlg and CImageViewDlg

 
#ifndef __MEMOEDITDLG__H
#define __MEMOEDITDLG__H
#pragma once

#include "ResizeDlg.h"
#include "ShowShapeDlg.h"

class CMemoEditDlg : public CResizeDlg
{
	DECLARE_DYNAMIC(CMemoEditDlg)

public:
	CMemoEditDlg(UINT nID,CDialog *pParentDlg,CShpLayer *pShp,EDITED_MEMO &memo,CWnd* pParent = NULL)
		: CResizeDlg(nID, pParent)
		, m_pParentDlg(pParentDlg)
		, m_pShp(pShp)
		, m_memo(memo)
	    , m_bEditable(false) {}

	~CMemoEditDlg() {}

	//Shared variables --
	EDITED_MEMO m_memo;
	CShpLayer *m_pShp;
	bool m_bEditable;
	CDialog *m_pParentDlg;

	//shared functions  -
	
	void ReturnFocus()
	{
		if(m_pParentDlg) {
			 if(m_pParentDlg==app_pShowDlg) {
				 app_pShowDlg->ReturnFocus();
				 //::SetFocus(app_pShowDlg->m_hWnd);
			 }
			 else if(m_pParentDlg==(CDialog *)m_pShp->m_pdbfile->pDBGridDlg) {
				::SetFocus(m_pParentDlg->m_hWnd);
			}
		}
	}

	void RefreshTitle()
	{
		CString title;
		SetWindowText(m_pShp->GetMemoTitle(title,m_memo));
	}

	bool IsMemoOpen(CShpLayer *pShp,DWORD rec,WORD fld)
	{
		return rec==m_memo.uRec && fld==m_memo.fld && m_pShp->m_pdb==pShp->m_pdb;
	}

	bool IsRecOpen(CShpLayer *pShp,DWORD rec)
	{
		return rec==m_memo.uRec && m_pShp->m_pdb==pShp->m_pdb;
	}

	bool IsEditable() {return m_bEditable;}

	//virtual functions for targeting CMemoEditDlg objects
	virtual bool DenyClose(BOOL bForceClose=FALSE)=0;
	virtual void ReInit(CShpLayer *pShp,EDITED_MEMO &memo)=0;
	virtual bool HasEditedText()=0;
	virtual void SetEditable(bool bEditable)=0;
};

#endif


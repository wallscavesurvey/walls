#if !defined(AFX_ThumbListCtrl_H__55F59001_AD5F_11D3_9DA4_0008C711C6B6__INCLUDED_)
#define AFX_ThumbListCtrl_H__55F59001_AD5F_11D3_9DA4_0008C711C6B6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define _USE_TT

class CImageViewDlg;

class CThumbListCtrl : public CListCtrl
{
public:
	CThumbListCtrl();
	virtual ~CThumbListCtrl();

	bool IsMovingSelected() {return m_bDragging;}

private:

	void DrawBar(bool bBlack);
	void DrawTheLines(int pointX);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void PreSubclassWindow();
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);

#ifdef _USE_TT
	BOOL OnToolTipText( UINT id, NMHDR * pNMHDR, LRESULT * pResult );
	virtual int OnToolHitTest(CPoint point, TOOLINFO * pTI) const;
	//WCHAR *m_pwchTip;
	//TCHAR *m_pchTip;
	//bool m_bToolTipCtrlCustomizeDone;
#endif

	bool m_bDragging;
	bool m_bInitDrag;
	bool m_bDropBelow;
	int m_nDragIndex,m_nDragX,m_nScrollOff;
	bool m_bBarDrawn;
	int m_nDropIndex;
	int m_nItemWidth;
	CRect m_crRange,m_crDrag;
	UINT_PTR m_nTimer;
	int m_nTimerDir;

	//CImageList *m_pImageList;
	CImageViewDlg *m_pDlg;

	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////

#endif // !defined(AFX_ThumbListCtrl_H__55F59001_AD5F_11D3_9DA4_0008C711C6B6__INCLUDED_)

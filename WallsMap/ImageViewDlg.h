#pragma once

#ifndef _IMAGEVIEWDLG_H
#define _IMAGEVIEWDLG_H

#define _ADJ_SCROLL

#include "MemoEditDlg.h"
#include "FileDropTarget.h"
#include "ThumbListCtrl.h"

#undef USE_CLIPBOARD     // Allow clipboard functions?

#define	THUMBNAIL_WIDTH		100
#define	THUMBNAIL_HEIGHT	75

class CShpLayer;
class CShowShapeDlg;
class CImageLayer;
class CWallsMapDoc;

/////////////////////////////////////////////////////////////////////////////
class CPreviewPane : public CStatic
{
public:
	CPreviewPane() : m_bInfoLoaded(false) {}
	virtual ~CPreviewPane();

#ifdef _ADJ_SCROLL
	CImageLayer *m_pLastLayer;
	long m_iLastClientWidth;
#endif

private:
	bool m_bInfoLoaded;
	CBitmap m_bmInfo;
	int m_bmWidth,m_bmHeight;

	afx_msg void OnPaint();
	//afx_msg void OnSize(UINT nType, int cx, int cy) ;
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
class CLVEdit : public CEdit
{
// Construction
public:
	CLVEdit() {}

	//int m_x;
	//int m_y;

// Implementation
public:
	virtual ~CLVEdit() {}

	// Generated message map functions
protected:
	afx_msg void OnWindowPosChanging(WINDOWPOS FAR* lpwndpos);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
class CImageData
{
public:
	CImageData() : pLayer(NULL) {}
	CImageData(CString &s) : csPath(s) {}
	LPCSTR InitFromText(LPCSTR p,CShpLayer *pShp);
	bool IsPathValid() {return !csPath.IsEmpty() && csPath[0]!='*';}
	bool IsOpen() {return pLayer!=NULL;}
	CString csPath;
	CString csTitle;
	CString csDesc;
	CImageLayer *pLayer;
};

// CImageViewDlg dialog

class CImageViewDlg : public CMemoEditDlg
{
	DECLARE_DYNAMIC(CImageViewDlg)

public:
	CImageViewDlg(CDialog *pParentDlg,CShpLayer *pShp,EDITED_MEMO &memo,CWnd* pParent = NULL);   // standard constructor
	virtual ~CImageViewDlg();
	virtual BOOL OnInitDialog();

	//virtual bool CanReInit(CShpLayer *pShp,DWORD uRec,UINT uFld);
	virtual void ReInit(CShpLayer *pShp,EDITED_MEMO &memo);
	virtual bool DenyClose(BOOL bForceClose=FALSE);
	virtual void SetEditable(bool bEditable);

	LPSTR GetTooltipText(int col,UINT code);
	void Destroy();
	static void NoDropMsg(CShpLayer *pShp);
	void OnDrop();
	void MoveSelected(int nDragIndex,int nDropIndex);


// Dialog Data
	enum { IDD = IDD_IMAGE_VIEWER };

	CThumbListCtrl m_ThumbList;
	//CListCtrl m_ThumbList;
	CPreviewPane m_PreviewPane;
	CImageList m_ImageList;
	std::vector<CImageData> m_vImageData;
	typedef std::vector<CImageData>::iterator it_ImageData;
	VEC_INT m_vIndex;

	int m_nImages;
	bool m_bResizing;
	bool m_bEmptyOnEntry;
	CImageLayer * GetSelectedImage();
	void AdjustControls();

	bool HasEditedText() {return m_bChanged;}

	static bool IsValidExt(LPCSTR pExt);
	bool IsDragAllowed() {
		if(!IsEditable() || !m_bThreadActive)
			return !m_bThreadActive;

		AfxMessageBox("Please wait for thumbnail geneneration to complete before any rearrangements.",MB_ICONINFORMATION);
		
		return false;
	}

private:
	//CWnd	*m_pParent;
	int	   m_iListSel;	//Currently selected m_ThumbList (or m_vIndex[]) index
	int	   m_iListImages; //Count of thumbs processed
	int	   m_iListContext;

	CDC m_dc;
	CFont *m_pOldFont;
	int m_iListHeight;
	int m_iLastIncHeight;
	int m_iTextHeight;
	int m_nBadLinks;

	LPCSTR m_pMemoText;
	int  m_iMaxDataHt;
	CString m_SetTitle;

	CString m_csToolTipText;
	WCHAR	*m_pwchTip;
	ULONG	m_wchTipSiz;

	bool m_bChanged;
	bool m_bThreadActive;			// Flag to whether thread is on running or not
	bool m_bInitFlag;
	bool m_bEditing;
	bool m_bOneLine;
	volatile bool m_bThreadTerminating;		// Flag to Thread to be terminated
	CWinThread *m_pThread;
	static CRect m_rectSaved;

	CLVEdit m_LVEdit;

	CFileDropTarget m_dropTarget;

	int FixIconTitle(CString &title);
	void RefreshIconTitleSize();
	void RefreshIconTitles();

	void ClearImageData();
	void WaitForThread();
	void TerminateThread();
	BOOL InitImageData();
	void InitThumbnails();
	bool ImageFound(LPCSTR path);
	bool InitThumbnail(CImageData &image);
	void DisplayData();
	void UpdateCounter();
	void SaveImageData();
	LPCSTR GetSetTitle();

	static UINT LoadThumbNails(LPVOID lpParam);

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void PostNcDestroy();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK();
	virtual void OnCancel();

	void EnableSave(bool bEnable);
	bool ScrollBarPresent();

	DECLARE_MESSAGE_MAP()

private:
	afx_msg void GetDispInfo(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNcLButtonDown(UINT nHitTest, CPoint point);
	afx_msg void OnNcMouseMove(UINT nHitTest, CPoint point);
	afx_msg LRESULT OnThumbThreadMsg(UINT wParam, LONG lParam);
	afx_msg void OnItemChangedList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemChanging(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void OnBnClickedButtonleft();
	afx_msg void OnBnClickedButtonright();
	afx_msg void OnEditPaste();
	afx_msg void OnImgProperties();
	afx_msg void OnImgRemove();
	afx_msg void OnImgInsert();
	afx_msg void OnUpdateImgCollection(CCmdUI *pCmdUI);
	afx_msg void OnBnClickedSaveandexit();
	afx_msg void OnRemoveall();
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
	afx_msg void OnOpenfolder();
	afx_msg void OnItemDblClick(NMHDR* pNMHDR, LRESULT* pResult);
	
	afx_msg void OnBeginLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
public:
	afx_msg void OnNcLButtonUp(UINT nHitTest, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
};

extern CImageViewDlg * app_pImageDlg;

#endif


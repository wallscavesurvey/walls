#if !defined(_RESOURCEBUTTON_H)
#define _RESOURCEBUTTON_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define baseCResourceButton CButton


class CResourceButton : public baseCResourceButton
{
// Construction
public:

CResourceButton() : m_index(0), m_imageCount(0), m_styleCount(0) {}

// Attributes

private:
	CBitmap m_Bitmap; // bitmap containing the images
	CSize m_ButtonSize; // width and height of the button
	int m_imageCount;
	int m_styleCount;
	int m_index;

// Operations
public:
	BOOL LoadBitmap(UINT bitmapid,int nCount,int nStyles);
	int GetImageCount() {return m_imageCount;}
	int GetImageIndex() {return m_index;}
	void Show(BOOL bShow) {
		ShowWindow(bShow?SW_SHOW:SW_HIDE);
	}
	void SetImageIndex(int n) {
		ASSERT(n<m_imageCount);
		m_index=n;
		Invalidate();
	}

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CResourceButton)
	public:
	//virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CResourceButton();

protected:

	DECLARE_MESSAGE_MAP()
};

#endif

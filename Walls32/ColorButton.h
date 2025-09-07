//***************************************************************************
//
// AUTHOR:  James White (feel free to remove or otherwise mangle any part)
//
// DESCRIPTION: This class is alarmingly similar to the CColorPicker control
//	created by Chris Maunder of www.codeproject.com. It is so as it was blatantly
//	copied from that class and is entirely dependant on his other great work
//  in CColorPopup. I was hoping for (cough.. gag..) a more Microsoft look
//  and I think this is pretty close. Hope you like it.
//
// ORIGINAL: http://www.codeproject.com/miscctrl/colour_picker.asp
//
//***************************************************************************
#ifndef COLORBUTTON_INCLUDED
#define COLORBUTTON_INCLUDED
#pragma once

#ifndef COLOURPOPUP_INCLUDED
#include "ColorPopup.h"
#endif//COLOURPOPUP_INCLUDED

void AFXAPI DDX_ColorCombo(CDataExchange *pDX, int nIDC, COLORREF& crColor);

#define MAX_TEXT_ELEMS 5
extern LPCTSTR dflt_pText[MAX_TEXT_ELEMS];

typedef struct {
	CDC *pDC;
	DWORD *pClr;
	RECT *pRect;
} CUSTOM_COLOR;

#define CUSTOM_COLOR_MASK     0xB0000000  //8+3
#define GET_CUSTOM_COLOR_MASK 0xC0000000  //8+4
#define GET_CUSTOM_COLOR_BIT  0x40000000
#define CUSTOM_COLOR_SHIFT	  28
#define IS_CUSTOM(clr) ((long)(CUSTOM_COLOR_MASK&(clr))>0)
#define IS_GET_CUSTOM(clr) ((long)(GET_CUSTOM_COLOR_MASK&(clr))>0)
#define CUSTOM_COLOR_IDX(clr) (((clr)>>CUSTOM_COLOR_SHIFT)-1)


class CColorCombo : public CButton
{
public:
	DECLARE_DYNCREATE(CColorCombo);

	//***********************************************************************
	// Name:		CColorCombo
	// Description:	Default constructor.
	// Parameters:	None.
	// Return:		None.	
	// Notes:		None.
	//***********************************************************************
	CColorCombo(void);

	//***********************************************************************
	// Name:		CColorCombo
	// Description:	Destructor.
	// Parameters:	None.
	// Return:		None.		
	// Notes:		None.
	//***********************************************************************
	virtual ~CColorCombo(void);

	//***********************************************************************
	//**                        Property Accessors                         **
	//***********************************************************************	
	__declspec(property(get = GetColor, put = SetColor))						COLORREF	Color;
	__declspec(property(get = GetDefaultColor, put = SetDefaultColor))		COLORREF	DefaultColor;
	__declspec(property(get = GetTrackSelection, put = SetTrackSelection))	BOOL		TrackSelection;
	__declspec(property(put = SetCustomText))								LPCTSTR		CustomText;
	__declspec(property(put = SetDefaultText))							LPCTSTR		DefaultText;

	//***********************************************************************
	// Name:		GetColor
	// Description:	Returns the current color selected in the control.
	// Parameters:	void
	// Return:		COLORREF 
	// Notes:		None.
	//***********************************************************************
	COLORREF GetColor(void) const;

	//***********************************************************************
	// Name:		SetColor
	// Description:	Sets the current color selected in the control.
	// Parameters:	COLORREF Color
	// Return:		None. 
	// Notes:		None.
	//***********************************************************************
	void SetColor(COLORREF Color);


	//***********************************************************************
	// Name:		GetDefaultColor
	// Description:	Returns the color associated with the 'default' selection.
	// Parameters:	void
	// Return:		COLORREF 
	// Notes:		None.
	//***********************************************************************
	COLORREF GetDefaultColor(void) const;

	//***********************************************************************
	// Name:		SetDefaultColor
	// Description:	Sets the color associated with the 'default' selection.
	//				The default value is COLOR_APPWORKSPACE.
	// Parameters:	COLORREF Color
	// Return:		None. 
	// Notes:		None.
	//***********************************************************************
	void SetDefaultColor(COLORREF Color);

	//***********************************************************************
	// Name:		SetCustomText
	// Description:	Sets the text to display in the CColorPicker control
	// Parameters:	LPCTSTR *pText
	// Return:		None. 
	// Notes:		None.
	//***********************************************************************
	void SetCustomText(int nTextElems, LPCTSTR *pText = dflt_pText);

	//***********************************************************************
	// Name:		SetTrackSelection
	// Description:	Turns on/off the 'Track Selection' option of the control
	//				which shows the colors during the process of selection.
	// Parameters:	BOOL bTrack
	// Return:		None. 
	// Notes:		None.
	//***********************************************************************
	void SetTrackSelection(BOOL bTrack);

	//***********************************************************************
	// Name:		GetTrackSelection
	// Description:	Returns the state of the 'Track Selection' option.
	// Parameters:	void
	// Return:		BOOL 
	// Notes:		None.
	//***********************************************************************
	BOOL GetTrackSelection(void) const;


	BOOL DoCustomColor(CUSTOM_COLOR *pCR);
	void AddToolTip(CToolTipCtrl *pTTCtrl);
	char * GetColorName(char *buf);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg BOOL OnToolTipNotify(UINT id, NMHDR * pNMHDR, LRESULT * pResult);

	//{{AFX_VIRTUAL(CColorCombo)
public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CColorCombo)
	afx_msg BOOL OnClicked();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	afx_msg LONG OnSelEndOK(UINT wParam, LONG lParam);
	afx_msg LONG OnSelEndCancel(UINT wParam, LONG lParam);
	afx_msg LONG OnSelChange(UINT wParam, LONG lParam);

	//***********************************************************************
	// Name:		DrawArrow
	// Description:	None.
	// Parameters:	CDC* pDC
	//				RECT* pRect
	//				int iDirection
	//					0 - Down
	//					1 - Up
	//					2 - Left
	//					3 - Right
	// Return:		static None. 
	// Notes:		None.
	//***********************************************************************
	static void DrawArrow(CDC* pDC,
		RECT* pRect,
		int iDirection = 0,
		COLORREF clrArrow = RGB(0, 0, 0));


	DECLARE_MESSAGE_MAP()

	COLORREF m_Color;
	COLORREF m_ClrElems[MAX_TEXT_ELEMS];
	LPCTSTR  *m_pText;
	int		 m_nTextElems;
	BOOL	m_bPopupActive;
	BOOL	m_bTrackSelection;

private:
	CToolTipCtrl *m_pToolTips;
	typedef CButton _Inherited;
};

#endif //!COLORBUTTON_INCLUDED


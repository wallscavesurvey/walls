#if !defined(AFX_RULERRICHEDITCTRL_H__4CD13283_82E4_484A_83B4_DBAD5B64F17C__INCLUDED_)
#define AFX_RULERRICHEDITCTRL_H__4CD13283_82E4_484A_83B4_DBAD5B64F17C__INCLUDED_

#include "RulerRichEdit.h"
#include "RRECRuler.h"
#include "RRECToolbar.h"
#include "EditLabel.h"
#include "FindDlgTbl.h"

#include "ids.h"

class CTableFillDlg;
class CEditImageDlg;

/////////////////////////////////////////////////////////////////////////////
// Helper structs

struct CharFormat : public CHARFORMAT
{
	CharFormat()
	{
		memset( this, 0, sizeof ( CharFormat ) );
		cbSize = sizeof( CharFormat );
	};

};

struct CharFormat2 : public CHARFORMAT2
{
	CharFormat2()
	{
		memset( this, 0, sizeof ( CharFormat2 ) );
		cbSize = sizeof( CharFormat2 );
	};

};

struct ParaFormat : public PARAFORMAT
{
	ParaFormat( DWORD mask )
	{
		memset( this, 0, sizeof ( ParaFormat ) );
		cbSize = sizeof( ParaFormat );
		dwMask = mask;
	}
};

/////////////////////////////////////////////////////////////////////////////
// CRulerFindDlg 
class CRulerFindDlg : public CFindReplaceDialog
{
public:
	CRulerFindDlg() {}
	virtual ~CRulerFindDlg() {}

	virtual void OnCancel()
	{
		CFindReplaceDialog::OnCancel();
	}
	CEditLabel m_ce;
	CEditLabel m_cr;
};

class CLogFont;/////////////////////////////////////////////////////////////////////////////
// CRulerRichEditCtrl window


class CRulerRichEditCtrl : public CWnd
{

public:
// Construction/creation/destruction
	CRulerRichEditCtrl();
	virtual ~CRulerRichEditCtrl();
	virtual BOOL Create( DWORD dwStyle, const RECT &rect, CWnd* pParentWnd,
		UINT nID, const CLogFont *pFont, COLORREF clrTxt);

// Attributes
	//Set mode to use, MODE_INCH or MODE_METRIC (default)
	void	SetMode( int mode ) {m_ruler.SetMode( mode );} 
	int		GetMode() const {m_ruler.GetMode();}

	void ShowToolbar( BOOL show = TRUE );
	void ShowRuler( BOOL show = TRUE );

	BOOL CanPaste() {return m_rtf.CanPaste();}

	BOOL IsToolbarVisible() const;
	BOOL IsRulerVisible() const;

	void CloseFindDlg() {
		if(m_pFindDlg) m_pFindDlg->PostMessage(WM_DESTROY);
		//if(m_pFindDlg) m_pFindDlg->DestroyWindow();
	}

	UINT GetTextLength() {return m_rtf.GetTextLengthEx(GTL_NUMCHARS|GTL_PRECISE);}

	CRichEditCtrl& GetRichEditCtrl( ) {return m_rtf;}

// Implementation

	CDialog *m_pParentDlg;
	UINT m_idContext;

	CString GetRTF();
	void GetRTF(CString *pStr);
	void SetRTF( const CString& rtf );
	void SetText(LPCSTR pText,BOOL bFormatRTF);
	void GetText(CString &s);
	BOOL IsFormatRTF() {return m_bFormatRTF;}

	BOOL	Save( CString& filename );
	BOOL	Load( CString& filename );

	void SetReadOnly( BOOL readOnly );
	BOOL GetReadOnly() const {return m_readOnly;}

	// To turn word wrap on - based on window width
	void SetWordWrap(BOOL bOn) {m_rtf.SetTargetDevice(NULL, bOn==FALSE);}

	// To turn word wrap on - based on target device (e.g. printer)
	// m_dcTarget is the device context, m_lineWidth is the line width 
	// SetTargetDevice(m_dcTarget, m_lineWidth);

// Formatting
	virtual void DoFont();
	virtual void DoColor();
	virtual void DoBold();
	virtual void DoItalic();
	virtual void DoUnderline();
	virtual void DoLeftAlign();
	virtual void DoCenterAlign();
	virtual void DoRightAlign();
	virtual void DoIndent();
	virtual void DoOutdent();
	virtual void DoBullet();

	void SetCurrentFontName( const CString& font );
	void SetCurrentFontSize( int points );
	void SetCurrentFontColor( COLORREF color );

	void SetBackgroundColor(COLORREF color) {m_rtf.SetBackgroundColor(FALSE,color);}

	afx_msg void OnFormatRTF();
	afx_msg void OnEditPaste(); //needed by TableFillDlg()

protected:

// Overrides
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);

	// Message handlers
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnButtonFont();
	//afx_msg void OnButtonColor();
	afx_msg void OnButtonBold();
	afx_msg void OnButtonItalic();
	afx_msg void OnButtonUnderline();
	afx_msg void OnButtonLeftAlign();
	afx_msg void OnButtonCenterAlign();
	afx_msg void OnButtonRightAlign();
	afx_msg void OnButtonIndent();
	afx_msg void OnButtonOutdent();
	afx_msg void OnButtonBullet();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg LRESULT OnDataChanged(WPARAM, LPARAM);
	afx_msg LRESULT OnSetText (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetText (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetTextLength (WPARAM wParam, LPARAM lParam);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnEditUndo();
	afx_msg void OnEditRedo();
	afx_msg void OnEditCut();
	afx_msg void OnEditCopy();
	afx_msg void OnEditSelectAll();
	//afx_msg void OnShowToolbar();
	afx_msg void OnShowRuler();
	afx_msg void OnFilePrint();
	afx_msg void OnEditFind();
	afx_msg void OnEditReplace();

	afx_msg LRESULT OnFindDlgMessage(WPARAM wParam, LPARAM lParam);
	afx_msg void OnLinkToFile();
	afx_msg void OnSaveToFile();
	afx_msg void OnCompareWithFile();
	afx_msg void OnReplaceWithFile();

	LRESULT OnTrackRuler(WPARAM mode, LPARAM pt);
	LRESULT OnGetScrollPos(WPARAM, LPARAM);
	LRESULT OnSetCurrentFontName(WPARAM font, LPARAM size);
	LRESULT OnSetCurrentFontSize(WPARAM font, LPARAM size);
	LRESULT OnSetCurrentFontColor(WPARAM font, LPARAM size);

	DECLARE_MESSAGE_MAP()

private:
	// Internal data
	int				m_rulerPosition;	// The x-position of the ruler line when dragging a tab
	CPen			m_pen;				// The pen to use for drawing the XORed ruler line

	CDWordArray		m_tabs;				// An array containing the tab-positions in device pixels
	int				m_margin;			// The margin to use for the ruler and buttons

	int				m_pxPerInch;		// The number of pixels for an inch on screen
	int				m_movingtab;		// The tab-position being moved, or -1 if none
	int				m_offset;			// Internal offset of the tab-marker being moved.

	BOOL			m_showToolbar;
	BOOL			m_showRuler;
	BOOL			m_readOnly;
	BOOL			m_bFormatRTF;

	// Sub-controls
	CRulerRichEdit	m_rtf;
	CRRECToolbar	m_toolbar;
	CRRECRuler		m_ruler;

	// Handle to the RTF 2.0 dll
	HINSTANCE		m_richEditModule;
	CRulerFindDlg *m_pFindDlg;
	CString m_csLastFind,m_csLastRepl;

	// Private helpers
	void	SetTabStops( LPLONG tabs, int size );
	void	UpdateTabStops();

	BOOL	CreateToolbar();
	BOOL	CreateRuler();
	BOOL	CreateRTFControl(const CLogFont *pFont,COLORREF clrTxt);
	void	CreateMargins();

	void	UpdateToolbarButtons();

	void	SetEffect( int mask, int effect );
	void	SetAlignment( int alignment );

	void	LayoutControls( int width, int height );
};

#endif // !defined(AFX_RULERRICHEDITCTRL_H__4CD13283_82E4_484A_83B4_DBAD5B64F17C__INCLUDED_)

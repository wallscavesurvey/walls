// prjfont.h : header file for global font utilities

#ifndef __PRJFONT_H
#define __PRJFONT_H

struct CLogFont : public LOGFONT
{
   void Init(char *FaceName,int Height,int Weight=FW_REGULAR,int Pitch=34,
        int Italic=FALSE,int Underline=FALSE);
};

struct PRJFONT
{
        CFont cf;
		int PointSize;
        int LineHeight;
        int AveCharWidth;
        LPSTR szTitle;
        BOOL bPrinter;
		BOOL bChanged;
        void Load(LPCSTR szText,BOOL bPrinter=FALSE,LOGFONT *pLF=NULL);
        BOOL Change(DWORD dwFlags);
	    void CalculateTextSize(CDC *pDC=NULL);
	    CFont *ScalePrinterFont(CDC *pDC,CFont *pcf);
        void Save();
		PRJFONT() {szTitle=NULL;}
		~PRJFONT();
};

// CTitleFontDialog dialog

class CTitleFontDialog : public CFontDialog
{
// Construction
public:
	CTitleFontDialog(LPLOGFONT pLF,DWORD dwFlags,LPCSTR szTitle);
	
// Dialog Data
	//{{AFX_DATA(CTitleFontDialog)
	//}}AFX_DATA

// Implementation
protected:
    LPCSTR m_szTitle;
    
    virtual BOOL OnInitDialog();
	// Generated message map functions
	//{{AFX_MSG(CTitleFontDialog)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//Global utitilties --
BOOL GetProfileFont(LPCSTR szSec, LOGFONT* plf); //<0 if not present, >0 if changed
      
#endif

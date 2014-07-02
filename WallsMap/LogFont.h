//CLogFont class definition
//

#ifndef _LOGFONT_H
#define _LOGFONT_H

class CLogFont
{
public:
	LOGFONT lf;//Stores this fonts LogFont for quick retrieval

	CLogFont();//Default Constructor

	HFONT Create();

	void GetStrFromFont(CString &Str) const;
	BOOL GetFontFromStr(PSTR str);

	BOOL GetFontFromDialog(DWORD dwFlags=CF_SCREENFONTS,COLORREF *pClr=NULL,
			CDC *pPrinterDC=NULL, CWnd *pParentWnd=NULL);
};

#endif
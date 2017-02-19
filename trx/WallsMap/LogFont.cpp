//CLogFont class implementation
#include "stdafx.h"
#include "trx_str.h"
#include "LogFont.h"

CLogFont::CLogFont()
{
	memset(&lf,0,sizeof(lf));
	lf.lfHeight=-11; //same as 8 pts(?)
	//lf.lfWidth=0;
	//lf.lfEscapement=0;
	//lf.lfOrientation=0;
	lf.lfWeight=FW_NORMAL;
	//lf.lfItalic=0;
	//lf.lfUnderline=0;
	//lf.lfStrikeOut=0;
	//lf.lfCharSet=ANSI_CHARSET;
	//lf.lfOutPrecision=OUT_DEFAULT_PRECIS;
	//lf.lfClipPrecision=CLIP_DEFAULT_PRECIS;
	lf.lfQuality=PROOF_QUALITY;
	lf.lfPitchAndFamily=VARIABLE_PITCH | FF_ROMAN;
	strcpy(lf.lfFaceName,"Times New Roman");
}

HFONT CLogFont::Create()
{
	return ::CreateFontIndirect(&lf);
}

void CLogFont::GetStrFromFont(CString &Str) const
{
	Str.Format("%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,\"%s\"",
		lf.lfHeight,
		lf.lfWidth,
		lf.lfEscapement,
		lf.lfOrientation,
		lf.lfWeight,
		lf.lfItalic,
		lf.lfUnderline,
		lf.lfStrikeOut,
		lf.lfCharSet,
		lf.lfOutPrecision,
		lf.lfClipPrecision,
		lf.lfQuality,
		lf.lfPitchAndFamily,
		lf.lfFaceName);
}

BOOL CLogFont::GetFontFromStr(PSTR str)
{
	int numArgs=cfg_GetArgv(str,CFG_PARSE_ALL);
	if(numArgs>=14) {
		lf.lfHeight=atol(cfg_argv[0]);
		lf.lfWidth=atol(cfg_argv[1]);
		lf.lfEscapement=atol(cfg_argv[2]);
		lf.lfOrientation=atol(cfg_argv[3]);
		lf.lfWeight=atol(cfg_argv[4]);
		lf.lfItalic=atoi(cfg_argv[5]);
		lf.lfUnderline=atoi(cfg_argv[6]);
		lf.lfStrikeOut=atoi(cfg_argv[7]);
		lf.lfCharSet=atoi(cfg_argv[8]);
		lf.lfOutPrecision=atoi(cfg_argv[9]);
		lf.lfClipPrecision=atoi(cfg_argv[10]);
		lf.lfQuality=atoi(cfg_argv[11]);
		lf.lfPitchAndFamily=atoi(cfg_argv[12]);
		strcpy(lf.lfFaceName,cfg_argv[13]);
		return TRUE;
	}
	return FALSE;
}

BOOL CLogFont::GetFontFromDialog(DWORD dwFlags,COLORREF *pClr,
				CDC *pPrinterDC, CWnd *pParentWnd)
{
	LOGFONT tlf=lf;

	CFontDialog dlg(&tlf,dwFlags,pPrinterDC, pParentWnd);
	if(pClr) {
		dlg.m_cf.rgbColors=*pClr;
		dwFlags|=CF_EFFECTS;
	}
	
	if (dlg.DoModal()==IDOK)
	{
		dlg.GetCurrentFont(&lf);
		if(pClr)
			*pClr=dlg.m_cf.rgbColors;
		return TRUE;
	}
	return FALSE;
}


//prjfont.cpp -- Global font utility fcns --

#include "stdafx.h"

static char BASED_CODE szHeight[] = "Height";
static char BASED_CODE szWeight[] = "Weight";
static char BASED_CODE szItalic[] = "Italic";
static char BASED_CODE szUnderline[] = "Underline";
static char BASED_CODE szPitchAndFamily[] = "PitchAndFamily";
static char BASED_CODE szFaceName[] = "FaceName";
static char BASED_CODE szSystem[] = "System";

/*Used by SuperPad --
static char BASED_CODE szSettings[] = "Settings";
static char BASED_CODE szTabStops[] = "TabStops";
static char BASED_CODE szPrintFont[] = "PrintFont";
static char BASED_CODE szWordWrap[] = "WordWrap";
*/

/////////////////////////////////////////////////////////////////////////////
// CTitleFontDialog dialog

CTitleFontDialog::CTitleFontDialog(LPLOGFONT pLF,DWORD dwFlags,LPCSTR szTitle)
	: CFontDialog(pLF,dwFlags)
{
  m_szTitle=szTitle;
}

BOOL CTitleFontDialog::OnInitDialog()
{
  CFontDialog::OnInitDialog();
  SetWindowText(m_szTitle);
  CenterWindow();
  return TRUE;
}

BEGIN_MESSAGE_MAP(CTitleFontDialog, CFontDialog)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//Global static functions --

BOOL GetProfileFont(LPCSTR szSec, LOGFONT* plf)
{
	memset(plf,0,sizeof(LOGFONT));
	
	CWinApp* pApp = AfxGetApp();
	int i = pApp->GetProfileInt(szSec, szHeight, 0);
	//Note: Pt * 10 units are used for lfHeight.
	if (i)
	{
	    plf->lfHeight = i; /*10ths of a point*/
		plf->lfWeight = pApp->GetProfileInt(szSec, szWeight, 0);
		plf->lfItalic = (BYTE)pApp->GetProfileInt(szSec, szItalic, 0);
		plf->lfUnderline = (BYTE)pApp->GetProfileInt(szSec, szUnderline, 0);
		plf->lfPitchAndFamily = (BYTE)pApp->GetProfileInt(szSec, szPitchAndFamily, 0);
		CString strFont = pApp->GetProfileString(szSec, szFaceName, szSystem);
		strncpy((char*)plf->lfFaceName, strFont, sizeof plf->lfFaceName);
		plf->lfFaceName[sizeof plf->lfFaceName-1] = 0;
	}
	return i != 0;
}

void CLogFont::Init(char *FaceName,int Height,int Weight,int Pitch,
        int Italic,int Underline)
{
	//NOTE: Height is assumed to be point size x 10.
    memset(this,0,sizeof(CLogFont));
    lfHeight = Height;
	lfWeight = Weight;
	lfItalic = Italic;
	lfUnderline = Underline;
	lfPitchAndFamily = Pitch;
	strcpy((char *)lfFaceName,FaceName);
}

BOOL CLogFont::GetFontFromStr(PSTR str)
{
	int numArgs=cfg_GetArgv(str, CFG_PARSE_ALL);
	if(numArgs>=14) {
		lfHeight=atol(cfg_argv[0]);
		lfWidth=atol(cfg_argv[1]);
		lfEscapement=atol(cfg_argv[2]);
		lfOrientation=atol(cfg_argv[3]);
		lfWeight=atol(cfg_argv[4]);
		lfItalic=atoi(cfg_argv[5]);
		lfUnderline=atoi(cfg_argv[6]);
		lfStrikeOut=atoi(cfg_argv[7]);
		lfCharSet=atoi(cfg_argv[8]);
		lfOutPrecision=atoi(cfg_argv[9]);
		lfClipPrecision=atoi(cfg_argv[10]);
		lfQuality=atoi(cfg_argv[11]);
		lfPitchAndFamily=atoi(cfg_argv[12]);
		strcpy(lfFaceName, cfg_argv[13]);
		return TRUE;
	}
	return FALSE;
}

PRJFONT::~PRJFONT()
{
	free(szTitle);
}

void PRJFONT::Save()
{
	CWinApp* pApp = AfxGetApp();

	if (bChanged)
	{
		LOGFONT lf;
   		cf.GetLogFont(&lf);
		pApp->WriteProfileInt(szTitle, szHeight, PointSize);
		pApp->WriteProfileInt(szTitle, szWeight, lf.lfWeight);
		pApp->WriteProfileInt(szTitle, szItalic, lf.lfItalic);
		pApp->WriteProfileInt(szTitle, szUnderline, lf.lfUnderline);
		pApp->WriteProfileInt(szTitle, szPitchAndFamily, lf.lfPitchAndFamily);
		pApp->WriteProfileString(szTitle, szFaceName, (LPCSTR)lf.lfFaceName);
	    bChanged=FALSE;
	}
}

void PRJFONT::CalculateTextSize(CDC *pDC)
{
	CDC dc;
	TEXTMETRIC tm;
	
	if(!pDC) {
	  dc.CreateCompatibleDC(NULL);
	  pDC=&dc;
	}
	CFont* pOldFont=pDC->SelectObject(&cf);
	pDC->GetTextMetrics(&tm);
	pDC->SelectObject(pOldFont);
    LineHeight=tm.tmHeight;
    AveCharWidth=tm.tmAveCharWidth;
} 

CFont * PRJFONT::ScalePrinterFont(CDC *pDC,CFont *pcf)
{
	CDC dc;
	dc.CreateCompatibleDC(NULL);
	LOGFONT lf;
	cf.GetLogFont(&lf);
	lf.lfHeight=PointSize;
    pcf->CreatePointFontIndirect(&lf,pDC);
    pcf=pDC->SelectObject(pcf);
    CalculateTextSize(pDC);
    return pcf;
}
 
void PRJFONT::Load(LPCSTR szText,BOOL bPtr,LOGFONT *pLF)
{
	LOGFONT lf;
	if(!GetProfileFont(szText,&lf)) {
      if(pLF) lf=*pLF;
      else {
		::GetObject(GetStockObject(SYSTEM_FONT), sizeof(LOGFONT), &lf);
		lf.lfHeight=100;
	  }
    }
	PointSize=lf.lfHeight;
    cf.CreatePointFontIndirect(&lf);
    if(szTitle=(LPSTR)malloc(strlen(szText)+1))	strcpy(szTitle,szText);
    bChanged=FALSE;
	//For printer fonts, we'll do this at the time of printing --
    if(!(bPrinter=bPtr)) CalculateTextSize();
}

static HDC GetDfltPrinterDC()
{
	//The follwing code was obtained directly from the SDK's
	//"Guide to Programming, Part 2" (minor changes for efficiency) --
	char * pTemp;
	char pPrintDevice[80];
	char * pPrintDriver;
	char * pPrintPort;
	
	::GetProfileString("windows","device","",(LPSTR)pPrintDevice,80);
	pTemp = pPrintDevice;
	pPrintDriver = pPrintPort = NULL;
	
	while (*pTemp) {
	    if (*pTemp == ',') {
	        *pTemp++ = 0;
	        while (*pTemp == ' ') pTemp++;
	        if (!pPrintDriver) pPrintDriver = pTemp;
	        else {
	            pPrintPort = pTemp;
	            break;
	        }
	    } 
	    else pTemp++;

	    //Due to what I consider a compiler bug, the cast, (PSTR)::AnsiNext(pTemp),
	    //doesn't avoid a "segment lost in conversion" warning, so we could
	    //instead use a more complicated cast or clumsy pragmas:
		//	#pragma warning(disable:4759)
	    //	pTemp = (PSTR)::AnsiNext(pTemp);
		//	#pragma warning(default:4759)
	    //which still don't work because of another compiler bug.
	    
	    //Anyway, for efficiency, let's postpone worrying about double-wide
	    //characters in WIN.INI (as we've done elsewhere) --
	}
	
	return ::CreateDC((LPSTR)pPrintDriver,(LPSTR)pPrintDevice,(LPSTR)pPrintPort,NULL);
}

BOOL PRJFONT::Change(DWORD dwFlags)
{
   BOOL bRet=FALSE;
   LOGFONT lf;
   HDC hDC;
   
   cf.GetLogFont(&lf);
   CTitleFontDialog dlg(&lf,dwFlags,szTitle);
   //dlg.m_cf.iPointSize=PointSize;
   //If this is a printer font we'll need a device context for the
   //current printer so the user will see the font sizes he'll actually
   //be getting --
   
   if(bPrinter) {
     dlg.m_cf.Flags|=CF_PRINTERFONTS;
	 dlg.m_cf.Flags&=~CF_SCREENFONTS;
     dlg.m_cf.hDC=hDC=GetDfltPrinterDC();
   }
   else hDC=NULL;
   
   if(dwFlags&CF_LIMITSIZE) {
      dlg.m_cf.nSizeMin=6;
      dlg.m_cf.nSizeMax=10;
   }

   if (dlg.DoModal() == IDOK) {
		cf.DeleteObject();
		//For the Review font, we'll always generate an 8 pt font here.
		//It will be rescaled to the proper width in CCompView::ScaleReviewFont().
		PointSize=lf.lfHeight=(dwFlags&CF_LIMITSIZE)?80:dlg.GetSize();
		cf.CreatePointFontIndirect(&lf);
	    if(!bPrinter) CalculateTextSize();
	    bRet=bChanged=TRUE;
   }
   
   if(hDC) ::DeleteDC(hDC);
   return bRet;
}

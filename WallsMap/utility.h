//Routines in utility.cpp --

#ifndef _UTILITY_H
#define _UTILITY_H

#define _USE_LYR_CULLING

#undef LYR_TIMING

#ifdef LYR_TIMING
extern bool bLYRTiming;
extern UINT nLayersSkipped;
#endif

#include "resource.h"
#ifdef _USE_MODELESS
#include "Modeless.h"
#endif
#include "GetElapsedSecs.h"

#ifndef _FLTPOINT_H
#include "FltPoint.h"
#endif

#ifndef __GEOREF_H
#include "georef.h"
#endif

#ifndef _LOGFONT_H
#include "LogFont.h"
#endif

#define LEN_TIMESTAMP 19

#define SWAPWORD(x) MAKEWORD(HIBYTE(x), LOBYTE(x))
#define SWAPLONG(x) MAKELONG(SWAPWORD(HIWORD(x)),SWAPWORD(LOWORD(x)))

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif

#define GetR_F(rgb) ((float)(GetRValue(rgb)/255.0))
#define GetG_F(rgb) ((float)(GetGValue(rgb)/255.0))
#define GetB_F(rgb) ((float)(GetBValue(rgb)/255.0))

#define GetR_f(rgb) (GetRValue(rgb)/255.0)
#define GetG_f(rgb) (GetGValue(rgb)/255.0)
#define GetB_f(rgb) (GetBValue(rgb)/255.0)

#define GetR(rgb) GetRValue(rgb)
#define GetG(rgb) GetGValue(rgb)
#define GetB(rgb) GetBValue(rgb)

#define D2D_CLR_F(c,o) D2D1::ColorF(GetR_F(c),GetG_F(c),GetB_F(c),o/100.0f)

//D2D1_COLOR_F values for ID2D1SolidColorBrush::SetColor() --
#define RGBAtoD2D(rgb,alpha) D2D1::ColorF(GetR_F(rgb),GetG_F(rgb),GetB_F(rgb),alpha)
#define RGBtoD2D(rgb) D2D1::ColorF(GetR_F(rgb),GetG_F(rgb),GetB_F(rgb))

template<class Interface>
inline void SafeRelease(Interface **ppInterfaceToRelease)
{
	if (*ppInterfaceToRelease != NULL)	{
		(*ppInterfaceToRelease)->Release();
		(*ppInterfaceToRelease) = NULL;
	}
}

//Initialized in CWallsMap::InitInstance() for now --
extern D2D1_RENDER_TARGET_PROPERTIES d2d_props;
extern ID2D1StrokeStyle *d2d_pStrokeStyleRnd;
extern ID2D1Factory *d2d_pFactory;
typedef std::vector<CD2DPointF> VEC_D2DPTF;

typedef std::vector<CPoint> VEC_CPOINT;
typedef std::vector<POINT> VEC_POINT;
typedef std::vector<int> VEC_INT;
typedef VEC_INT::iterator it_int;
typedef std::vector<BYTE> VEC_BYTE;
typedef VEC_BYTE::iterator it_byte;
typedef std::vector<WORD> VEC_WORD;
typedef VEC_WORD::iterator it_word;
typedef std::vector<DWORD> VEC_DWORD;
typedef VEC_DWORD::iterator it_dword;
typedef std::vector<LPSTR> VEC_LPSTR;
typedef VEC_LPSTR::iterator it_lpstr;
typedef std::vector<CString> VEC_CSTR;
typedef VEC_CSTR::iterator it_cstr;
typedef VEC_CSTR::const_iterator cit_cstr;
typedef std::vector<double> VEC_DOUBLE;

extern WORD wOSVersion;
extern bool bTS_OPT;
extern bool bIgnoreEditTS;

extern char _string128[128];

extern const UINT WM_FINDDLGMESSAGE;
extern const UINT WM_ADVANCED_SRCH;
extern const UINT WM_CLEAR_HISTORY;
extern const UINT uCF_RTF;

//bool __inline xisspace(char c) {return isspace((BYTE)c);}

bool	AddFileSize(const char *pszPathName,__int64 &size);
BOOL	AddFilter(CString &filter,int nIDS);
BOOL	BrowseFiles(CEdit *pw,CString &path,LPCSTR ext,UINT idType,UINT idBrowse); 
int		CheckAccess(LPCTSTR fname, int mode);
int 	CheckCoordinatePair(CString &csSrc,bool &bUtm,double *pf0=0,double *pf1=0);
int		CheckDirectory(CString &path,BOOL bOpen=FALSE);
BOOL	CheckFlt(const char *buf,double *pf,double fMin,double fMax,int dec,LPCSTR nam="");
BOOL	CheckZoomLevel(CDataExchange* pDX,UINT id,LPCSTR s);
BOOL	ClipboardCopy(const CString &strData,UINT cf=CF_TEXT);
BOOL	ClipboardPaste(CString &strData,UINT maxLen);
void	CMsgBox(LPCSTR format,...);
int		CMsgBox(UINT nType,UINT nIDS,...);
int		CMsgBox(UINT nType,LPCSTR format,...);
char *  CommaSep(char *outbuf,__int64 num);
LPCSTR  DatumStr(INT8 iNad);
BOOL	DoPromptPathName(CString& pathName,DWORD lFlags,
			int numFilters,CString &strFilter,BOOL bOpen,UINT ids_Title,LPCSTR defExt);
BOOL	DirCheck(LPSTR pathname,BOOL bPrompt);
bool	ForceExtension(CString &path,LPCSTR ext);
bool	GetConvertedPoint(CFltPoint &fpt,const int iNad,int *piZone,const int iNadSrc,const int iZoneSrc);
void	OpenContainingFolder(LPCSTR path);
void	OpenFileAsText(LPCSTR file);
bool	__inline geo_LatLon2UTM(CFltPoint &fp,int *zone,int datum)
{
	return geo_LatLon2UTM(fp.y,fp.x,zone,&fp.x,&fp.y,datum);
}
void	__inline geo_UTM2LatLon(CFltPoint &fp,int zone,int datum)
{
	geo_UTM2LatLon(zone,fp.x,fp.y,&fp.y,&fp.x,datum);
}
bool	GetCoordinate(CString &csSrc,double &fCoord,bool bFirst,bool bUtm);
char *	GetFloatStr(double f,int dec);
LPSTR	GetFullFromRelative(LPSTR buf,LPCSTR pPath,LPCSTR pRefPath);
char *  GetIntStr(__int32 i);
BOOL	GetKmlPath(CString &path,LPCSTR pLabel);
struct tm *GetLocalFileTime(const char *pszPathName,__int64 *pSize=NULL,BOOL *pbReadOnly=NULL);
LPCSTR  GetMediaTyp(LPCSTR sPath,BOOL *pbLocal=NULL);
int		GetNad(LPCSTR p);
//WORD	GetOSVersion();
char *  GetPercentStr(double f);
LPSTR   GetRelativePath(LPSTR buf,LPCSTR pPath,LPCSTR pRefPath,BOOL bNoNameOnPath);
LPSTR	GetLocalTimeStr();
BOOL	GetTempFilePathWithExtension(CString &csPath,LPCSTR pExt);
void	GetTimestamp(LPSTR pts,const struct tm &gmt);
char *	GetTimeStr(struct tm *ptm,__int64 *pSize=NULL);
int		GetZone(const double &lat,const double &lon);
int		__inline GetZone(const CFltPoint &fp) {return GetZone(fp.y,fp.x);}
void	AppendFileTime(CString &cs,LPCSTR lpszPathName,UINT uTyp=0);
BOOL	InitLabelDC(HDC hdc,CLogFont *pFont=NULL,COLORREF clr=0);
bool	IsNumeric(LPCSTR p);
LPSTR	LabelToValidFileName(LPCSTR pLabel,LPSTR pBuf,int sizBuf);
void	LastErrorBox();
BOOL	MakeFileDirectoryCurrent(LPCSTR pathname);
#ifdef _USE_MODELESS
void    ModelessOpen(LPCSTR title, UINT nDelay, LPCSTR format, ...);
#endif
int     MsgYesNoCancelDlg(HWND hWnd, LPCSTR msg, LPCSTR title, LPCSTR pYes, LPCSTR pNo, LPCSTR pCancel);
int     MsgCheckDlg(HWND hWnd, UINT mb, LPCSTR msg, LPCSTR title, LPCSTR pDoNotAsk);
bool    MsgOkCancel(HWND hWnd, LPCSTR msg, LPCSTR title);
void	Pause();
LPCSTR	ReplacePathExt(CString &path,LPCSTR pExt);
LPCSTR ReplacePathName(CString &path,LPCSTR name);
BOOL    SaveBitmapToFile(CBitmap *bitmap, CDC* pDC, LPCSTR lpFileName);
BOOL	SearchDirRecursively(LPSTR pRelPathToSearch,LPCSTR pFileName,LPCSTR pSkipDir);
BOOL	SearchRelativeDir(CString &sRelPathFound,LPCSTR pRelpathSearchd,LPCSTR pRefPath,LPCSTR pFileName);
BOOL	SetFileToCurrentTime(LPCSTR path);
void	SetWndTitle(CWnd *dlg,UINT nIDS,...);
LPSTR   String128(LPCSTR format,...);
void	ToolTipCustomize(int nInitial,int nAutopop,int nReshow,CToolTipCtrl* pToolTip);
BOOL	TrackMouse(HWND hWnd);

#ifdef _DEBUG
void	_TraceLastError();
#endif

#endif

#ifndef __UTILITY_H
#define __UTILITY_H

#ifndef _INC_CTYPE 
#include <ctype.h>
#endif

#ifndef _PCRE_H
#include <pcre.h>
#endif

#ifndef __COLORS_H
#include "colors.h"
#endif

#define PI 3.141592653589793
#define PI2 (PI/2.0)
#define U_DEGREES (PI/180.0)

enum {F_MATCHCASE=1,F_MATCHWHOLEWORD=2,F_SEARCHDETACHED=4,F_USEREGEX=8};

struct DPOINT {
public:
	DPOINT(double X,double Y) : x(X),y(Y) {};
	DPOINT() {x=0.0;y=0.0;};
	double x,y;
};

BOOL CheckInt(const char *buf,int *pi,int iMin,int iMax);
BOOL CheckFlt(const char *buf,double *pf,double fMin,double fMax,int dec);
BOOL SetCurrentDir(const char *path);
BOOL IsNumeric(LPCSTR p);

void PASCAL GetStrFromFld(char *s,const void *f,int len);
void PASCAL GetFldFromStr(void *f,const char *s,int len);
char * GetFloatStr(double f,int dec);
char * GetIntStr(int i);

void tok_init(char *p);
double tok_dbl();
int tok_int();
int tok_count();

void Pause();
int CDECL CMsgBox(UINT nType,char *format,...);
int CDECL CMsgBox(UINT nType,UINT nIDS,...);
int CDECL LoadCMsg(char *buf,int sizBuf,UINT nIDS,...);
void CDECL SetWndTitle(CWnd *pWnd,UINT nIDS,...);
void CDECL StatusMsg(UINT nIDS,...);
void StatusClear();
void CDECL UtlTextOut(CDC *dc,int x,int y,char *format,...);
BOOL PASCAL AddFilter(CString &filter,int nIDS);
BOOL PASCAL DoPromptPathName(CString& pathName,DWORD lFlags,int numFilters,CString &strFilter,BOOL bOpen,UINT ids_Title,char *defExt=NULL);
int PASCAL GetWinPos(int *pWinPos,int siz_pos,CRect &rect,CSize &size,int &cxIcon);
BOOL DirCheck(char *pathname,BOOL bPrompt);
CString GetWallsPath();
#ifdef _USE_ZONELETTER
char *ZoneStr(int zone,double abslat);
#else
char *ZoneStr(int zone);
#endif
BOOL IsAbsolutePath(const char *path);
BOOL GetOsVersion(RTL_OSVERSIONINFOEXW* pk_OsVer);
DWORD GetOsMajorVersion();
SYSTEMTIME *GetLocalFileTime(const char *pszPathName,long *pSize,BOOL *pbReadOnly=NULL);
char * GetTimeStr(SYSTEMTIME *ptm,long *pSize);
char * GetDateStr(int julday);
void GetCoordinateFormat(CString &s,DPOINT &coord,BOOL bProfile, BOOL bFeet);
void ElimNL(char *p);
void FixFilename(CString &name);
void FixPathname(CString &path,char *ext);
void DimensionStr(char *buf,double w,double h,BOOL bInches);
BOOL FilePatternExists(LPCSTR buf);
int parse_2names(char *names,BOOL bShowError);
int parse_name(LPCSTR name);
void GetStrFromDateFld(char *s,void *date,LPCSTR delim);
LPSTR FixFlagCase(char *name);
BOOL CheckRegEx(LPCSTR pattern,WORD wFlags);
pcre *GetRegEx(LPCSTR pattern,WORD wFlags);
//void InsertDriveLetter(char *pathbuf,const char *drvpath);
BOOL BrowseFiles(CEdit *pw,CString &path,char *ext,UINT idType,UINT idBrowse); 
int CheckDirectory(CString &path,BOOL bOpen=FALSE);
int CompareNumericLabels(LPCSTR p,LPCSTR p1);
int SetFileReadOnly(LPCSTR pPath,BOOL bReadOnly,BOOL bNoPrompt);
double GetElapsedSecs(BOOL bInit);
BOOL ClipboardCopy(LPCSTR strData,UINT cf=CF_TEXT);
BOOL ClipboardPaste(LPSTR strData,UINT maxLen);
BOOL OpenContainingFolder(HWND hwnd,LPCSTR path);
LPCSTR TrimPath(LPSTR path, int maxlen);
LPCSTR TrimPath(CString &path, int maxlen);
LPSTR GetRelativePath(LPSTR buf, LPCSTR pPath, LPCSTR pRefPath, BOOL bNoNameOnPath);
int _stat_fix(const char *pszPathName, DWORD *pSize=NULL, BOOL *pbReadOnly=NULL, BOOL *pbIsDir=NULL);

#ifdef _DEBUG
BOOL _TraceLastError();
#endif

//Support for centered stock dialogs --
class CCenterFileDialog : public CFileDialog
{
public:
	CCenterFileDialog(BOOL bOpen) : CFileDialog(bOpen) {}
protected:
virtual BOOL OnInitDialog();
    afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);

	// Generated message map functions
	//{{AFX_MSG(CCenterFileDialog)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

class CCenterColorDialog : public CColorDialog
{
public:
	CCenterColorDialog(COLORREF rgb=0,DWORD dwFlags=0,CWnd *pParent=NULL) :
	  CColorDialog(rgb,dwFlags,pParent) {}
	char *m_pTitle;
	static void LoadCustomColors();
	static void SaveCustomColors();

protected:
	static DWORD clrChecksum;
	virtual BOOL OnInitDialog();
    afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);

	// Generated message map functions
	//{{AFX_MSG(CCenterColorDialog)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

BOOL _inline IsWordDelim(char c)
{
	return !c || isspace((BYTE)c) || (c!='.' && c!='-' && ispunct((BYTE)c));
}

void TrackMouse(HWND hWnd);
#endif

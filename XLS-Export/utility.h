#ifndef __UTILITY_H
#define __UTILITY_H

#ifndef __TRX_TYPE_H
#include <trx_type.h>
#endif
int CheckAccess(LPCTSTR fname, int mode);
int CMsgBox(UINT nType,LPCSTR format,...);
void CMsgBox(LPCSTR format,...);
LPSTR GetFullPath(LPCSTR pathName,LPCSTR defExt);
long GetLong(CDaoRecordset &rs,int nFld);
double GetDouble(CDaoRecordset &rs,int nFld);
__int64 GetInt64(CDaoRecordset &rs,int nFld);
char * GetFloatStr(double f,int dec);
void GetString(CString &s,CDaoRecordset &rs,int nFld);
int GetStringTest(CString &s,CDaoRecordset &rs,int nFld);
char *GetDateUTC8();
int GetText(LPSTR dst,CDaoRecordset &rs,int nFld,int nLen);
BOOL GetVarText(LPSTR dst,CDaoRecordset &rs,int nFld,int nLen);
BOOL GetBoolCharVal(LPSTR dst,char c);
bool GetBool(CDaoRecordset &rs,int nFld);
void GetBoolStr(LPSTR s,CDaoRecordset &rs,int nFld);
void GetDateStr(LPSTR p,CDaoRecordset &rs,int nFld);
BOOL GetTempFilePathWithExtension(CString &csPath,LPCSTR pExt);
int     MsgCheckDlg(HWND hWnd, UINT mb, LPCSTR msg, LPCSTR title, LPCSTR pDoNotAsk);
int    MsgInfo(HWND hWnd, LPCSTR msg, LPCSTR title, UINT style=0);
bool    MsgOkCancel(HWND hWnd, LPCSTR msg, LPCSTR title);
int     MsgYesNoCancelDlg(HWND hWnd, LPCSTR msg, LPCSTR title, LPCSTR pYes, LPCSTR pNo, LPCSTR pCancel);
void OpenFileAsText(LPCSTR file);
#endif





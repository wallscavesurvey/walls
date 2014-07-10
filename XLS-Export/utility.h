#ifndef __UTILITY_H
#define __UTILITY_H

#ifndef __TRX_TYPE_H
#include <trx_type.h>
#endif

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
#endif





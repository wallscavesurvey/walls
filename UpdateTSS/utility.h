#ifndef __TRX_TYPE_H
#include <trx_type.h>
#endif

void CMsgBox(LPCSTR format,...);
int CMsgBox(UINT nType,LPCSTR format,...);
LPSTR GetFullPath(LPCSTR pathName,LPCSTR defExt);
long GetLong(CDaoRecordset &rs,int nFld);
double GetDouble(CDaoRecordset &rs,int nFld);
char * GetFloatStr(double f,int dec);
bool PutLong(CDaoRecordset &rs,int nFld,long n);
bool PutDouble(CDaoRecordset &rs,int nFld,double d,int dec);
bool PutDouble(CDaoRecordset &rs,int nFld,double d);
bool PutString(CDaoRecordset &rs,int nFld,LPCSTR s);
void GetString(CString &s,CDaoRecordset &rs,int nFld);
char *GetDateUTC8();
bool GetBool(CDaoRecordset &rs,int nFld);

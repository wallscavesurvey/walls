/* DBFPUTS.C -- Store field string at current position*/

#include <string.h>
#include "a__dbf.h"

int DBFAPI dbf_PutFldStr(DBF_NO dbf,LPCSTR pStr,UINT nFld)
{
  int len_dest,len_src;
  LPBYTE p=(LPBYTE)dbf_FldPtr(dbf,nFld);

  if(!p || dbf_Mark(dbf)) return dbf_errno;
  len_dest=nFld?_dbf_pFile->pFldDef[nFld-1].F_Len:1;
  if((len_src=strlen(pStr))>len_dest) len_src=len_dest;
  memcpy(p,pStr,len_src);
  if(len_src<len_dest) memset(p+len_src,' ',len_dest-len_src);
  return 0;
}

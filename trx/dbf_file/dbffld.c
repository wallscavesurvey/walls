/* DBFFLD.C -- Retrieve field at current position*/

#include <string.h>
#include "a__dbf.h"

LPVOID DBFAPI dbf_Fld(DBF_NO dbf,LPVOID pBuf,UINT nFld)
{
  DBF_pFLDDEF fp;
  LPBYTE p;

  if(!_GETDBFU || (_DBFU->U_Recno==0 && (dbf_errno=DBF_ErrEOF)!=0))
    return NULL;
  p=_DBFU->U_Rec;
  if(nFld) {
    if((fp=_dbf_GetFldDef(nFld))==NULL) return NULL;
    p+=fp->F_Off;
    nFld=fp->F_Len;
  }
  else nFld++;
  return memcpy(pBuf,p,nFld);
}

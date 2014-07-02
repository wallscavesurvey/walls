/* DBFGETFS.C -- Retrieve field string at current position*/

#include "a__dbf.h"

int DBFAPI dbf_GetFldStr(DBF_NO dbf,LPSTR pStr,UINT nFld)
{
  return dbf_FldStr(dbf,pStr,nFld)?0:dbf_errno;
}

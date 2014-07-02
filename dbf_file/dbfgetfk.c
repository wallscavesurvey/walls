/* DBFGETFK.C -- Retrieve field key at current position*/

#include "a__dbf.h"

int DBFAPI dbf_GetFldKey(DBF_NO dbf,LPSTR pStr,UINT nFld)
{
  return dbf_FldKey(dbf,pStr,nFld)?0:dbf_errno;
}

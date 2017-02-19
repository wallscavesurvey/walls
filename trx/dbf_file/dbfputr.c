/* DBFPUTR.C -- Replace record at current position*/

#include "a__dbf.h"
#include <string.h>

int DBFAPI dbf_PutRec(DBF_NO dbf,LPVOID pRec)
{
  LPVOID p=dbf_RecPtr(dbf);

  if(!p || dbf_Mark(dbf)) return dbf_errno;
  memcpy(p,pRec,_dbf_pFile->H.SizRec);
  return 0;
}

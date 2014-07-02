/* DBFGETR.C -- Retrieve record at current position*/

#include <string.h>
#include "a__dbf.h"

int DBFAPI dbf_GetRec(DBF_NO dbf,LPVOID pBuf)
{
  if(!_GETDBFU || (!_DBFU->U_Recno && (dbf_errno=DBF_ErrEOF)!=0))
    return dbf_errno;
  memcpy(pBuf,_DBFU->U_Rec,_dbf_pFile->H.SizRec);
  return 0;
}

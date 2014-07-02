/* DBFREC.C -- Retrieve record at current position*/

#include <string.h>
#include "a__dbf.h"

LPVOID DBFAPI dbf_Rec(DBF_NO dbf,LPVOID pRec)
{
  if(!_GETDBFU || (!_DBFU->U_Recno && (dbf_errno=DBF_ErrEOF)!=0))
    return NULL;
  return memcpy(pRec,_DBFU->U_Rec,_dbf_pFile->H.SizRec);
}

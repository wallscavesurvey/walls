/* DBFPREV.C -- Move current position to previous record*/

#include "a__dbf.h"

int DBFAPI dbf_Prev(DBF_NO dbf)
{
  if(!_GETDBFU || _dbf_CommitUsrRec()) return dbf_errno;

  if(_DBFU->U_Recno<=1) return dbf_errno=DBF_ErrEOF;

  _DBFU->U_Recno--;
  _DBFU->U_Recoff-=_dbf_pFile->H.SizRec;
  return _dbf_LoadUsrRec();
}

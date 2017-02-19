/* DBFSKIP.C --*/

/*Adjusts the user's position by n, where n is any integer
  (positive, negative, or zero). DBF_ErrEOF is returned if either a
  prior position was not established or if the new position doesn't
  exist. If this, or any other type of error occurs, the user's
  position is set to zero.*/

#include "a__dbf.h"

int DBFAPI dbf_Skip(DBF_NO dbf,long n)
{
  if(!_GETDBFU || _dbf_CommitUsrRec()) return dbf_errno;

  if(!_DBFU->U_Recno || !(n+=(long)_DBFU->U_Recno) ||
    (DWORD)n>_dbf_pFile->H.NumRecs) return dbf_errno=DBF_ErrEOF;

  _DBFU->U_Recno=(DWORD)n;
  _DBFU->U_Recoff=(DWORD)(n-1L)*_dbf_pFile->H.SizRec;
  return _dbf_LoadUsrRec();
}

/* DBFPOS.C -- Returns user's current position --*/

#include "a__dbf.h"

DWORD DBFAPI dbf_Position(DBF_NO dbf)
{
  DWORD n;
  if(!_GETDBFU || ((n=_DBFU->U_Recno)==0 && (dbf_errno=DBF_ErrEOF)!=0))
    return 0;
  return n;
}

/* DBFUNMAR.C -- Unmark user's record buffer*/

#include "a__dbf.h"

int DBFAPI dbf_UnMark(DBF_NO dbf)
{
  if(!_GETDBFU) return dbf_errno;
  if(!_DBFU->U_Recno) return dbf_errno=DBF_ErrEOF;
  _DBFU->U_RecChg=FALSE;
  return 0;
}

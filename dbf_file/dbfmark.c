/* DBFMARK.C -- Replace record at current position*/

#include "a__dbf.h"

int DBFAPI dbf_Mark(DBF_NO dbf)
{
  if(!_GETDBFU || _dbf_ErrReadOnly()) return dbf_errno;
  if(!_DBFU->U_Recno) return dbf_errno=DBF_ErrEOF;
  _DBFU->U_RecChg=TRUE;
  return 0;
}

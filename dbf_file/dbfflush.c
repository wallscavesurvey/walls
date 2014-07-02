/* DBFFLUSH.C -- Functions related to DBF file updates*/

#include "a__dbf.h"

int DBFAPI dbf_Flush(DBF_NO dbf)
{
  int e;

  if(!_GETDBFU) return DBF_ErrArg;
  if((_DBFU->U_Mode&DBF_ReadOnly)!=0) return 0;
  e=_dbf_CommitUsrRec();
  if(_dbf_Flush() && !e) e=dbf_errno;
  return e;
}

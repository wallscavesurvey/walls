/* DBFNEXT.C -- Advance current position one record*/

#include "a__dbf.h"

int DBFAPI dbf_Next(DBF_NO dbf)
{
  DBF_NO db=dbf;
  if(!_GETDBFU || _dbf_CommitUsrRec()) return dbf_errno;
  return dbf_Go(db,_DBFU->U_Recno+1);
}

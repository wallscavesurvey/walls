/* DBFLAST.C -- Move current position to last record*/

#include "a__dbf.h"

int DBFAPI dbf_Last(DBF_NO dbf)
{
  UINT nrecs=dbf_NumRecs(dbf);
  if(dbf_errno) return dbf_errno;
  if(!nrecs) return DBF_ErrEOF;
  return dbf_Go(dbf,nrecs);
}

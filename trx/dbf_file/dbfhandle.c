#include "a__dbf.h"

HANDLE DBFAPI dbf_Handle(DBF_NO dbf)
{
  return ((DOS_FP)(_dbf_GetDbfP(dbf)->Cf.Handle))->DosHandle;
}

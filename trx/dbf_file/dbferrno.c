#include "a__dbf.h"

int DBFAPI dbf_Errno(DBF_NO dbf)
{
  if(!_GETDBFU) return DBF_ErrArg;
  return _DBFU->U_Errno;
}

#include "a__dbf.h"

DWORD DBFAPI dbf_OpenFlags(DBF_NO dbf)
{
  return _GETDBFU?_DBFU->U_Mode:0;
}

#include "a__dbf.h"

UINT DBFAPI dbf_NumFlds(DBF_NO dbf)
{
  return _GETDBFP?_DBFP->NumFlds:0;
}

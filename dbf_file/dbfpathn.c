#include "a__dbf.h"

NPSTR DBFAPI dbf_PathName(DBF_NO dbf)
{
  return _GETDBFP?dos_PathName(_DBFP->Cf.Handle):"";
}

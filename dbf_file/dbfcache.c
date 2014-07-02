#include "a__dbf.h"

TRX_CSH DBFAPI dbf_Cache(DBF_NO dbf)
{
  return _GETDBFP?(TRX_CSH)_DBFP->Cf.Csh:0;
}

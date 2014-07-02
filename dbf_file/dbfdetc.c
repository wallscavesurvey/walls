#include "a__dbf.h"

int DBFAPI dbf_DetachCache(DBF_NO dbf)
{
  return dbf_errno=_csf_DetachCache((CSF_NO)_GETDBFP);
}

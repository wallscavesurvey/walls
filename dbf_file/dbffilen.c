#include "a__dbf.h"

NPSTR DBFAPI dbf_FileName(DBF_NO dbf)
{
	return _GETDBFP ? dos_FileName(_DBFP->Cf.Handle) : "";
}

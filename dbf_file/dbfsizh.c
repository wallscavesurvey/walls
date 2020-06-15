#include "a__dbf.h"

UINT DBFAPI dbf_SizHdr(DBF_NO dbf)
{
	return _GETDBFP ? _DBFP->Cf.Offset : 0;
}

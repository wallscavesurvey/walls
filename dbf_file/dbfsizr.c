#include "a__dbf.h"

UINT DBFAPI dbf_SizRec(DBF_NO dbf)
{
	return _GETDBFP ? _DBFP->H.SizRec : 0;
}

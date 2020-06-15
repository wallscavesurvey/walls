/* DBFHDRR.C - Returns pointer to DBF file header reserved area (16-bytes).*/

#include "a__dbf.h"

/*=================================================================*/

LPBYTE DBFAPI dbf_HdrRes16(DBF_NO dbf)
{
	if (!_GETDBFP) return 0;
	return (LPBYTE)_DBFP->H.Res2;
}

LPBYTE DBFAPI dbf_HdrRes2(DBF_NO dbf)
{
	if (!_GETDBFP) return 0;
	return (LPBYTE)&_DBFP->H.Res1;
}

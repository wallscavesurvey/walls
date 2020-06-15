/* DBFHDRD.C - Returns pointer to DBF file header date.*/

#include "a__dbf.h"

/*=================================================================*/

int DBFAPI dbf_GetHdrDate(DBF_NO dbf, DBF_HDRDATE *pDate)
{
	if (!_GETDBFP) return DBF_ErrArg;
	*pDate = _DBFP->H.Date;
	return 0;
}

int DBFAPI dbf_SetHdrDate(DBF_NO dbf, DBF_HDRDATE *pDate)
{
	int e = dbf_MarkHdr(dbf);
	if (!e) _dbf_pFile->H.Date = *pDate;
	return e;
}

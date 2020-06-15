/* DBFGETFD.C -- Copies part of DBF_FLD array.*/

#include <string.h>
#include "a__dbf.h"

/*=================================================================*/

int DBFAPI dbf_GetFldDef(DBF_NO dbf, DBF_FLDDEF *pF, UINT fst, int count)
{
	UINT cnt;
	DBF_pFLDDEF pf;

	if (!_GETDBFP || (pf = _dbf_GetFldDef(fst)) == 0) return dbf_errno;
	cnt = (UINT)_DBFP->NumFlds - (--fst);
	if ((UINT)count < cnt) cnt = (UINT)count;
	memcpy(pF, pf, cnt * sizeof(DBF_FLDDEF));
	return 0;
}

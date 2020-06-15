/* DBFFLDLE.C -- Retrieve length of specified field --*/

#include "a__dbf.h"

UINT DBFAPI dbf_FldLen(DBF_NO dbf, UINT nFld)
{
	DBF_pFLDDEF fp;

	if (!_dbf_GetDbfP(dbf)) return 0;
	if (!nFld) return 1;
	return (fp = _dbf_GetFldDef(nFld)) != 0 ? fp->F_Len : 0;
}

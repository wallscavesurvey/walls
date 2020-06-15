/* DBFFLDDE.C -- Retrieve decimal length of specified field --*/

#include "a__dbf.h"

UINT DBFAPI dbf_FldDec(DBF_NO dbf, UINT nFld)
{
	DBF_pFLDDEF fp;

	if (!_dbf_GetDbfP(dbf)) return 0;
	return (nFld && (fp = _dbf_GetFldDef(nFld)) != 0) ? fp->F_Dec : 0;
}

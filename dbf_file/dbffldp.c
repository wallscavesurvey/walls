/* DBFFLDP.C -- Retrieve pointer to field in user's record buffer*/

#include "a__dbf.h"

LPVOID DBFAPI dbf_FldPtr(DBF_NO dbf, UINT nFld)
{
	DBF_pFLDDEF fp;

	if (!_GETDBFU) return NULL;
	if (!nFld) return _DBFU->U_Rec;
	return (fp = _dbf_GetFldDef(nFld)) != NULL ? (_DBFU->U_Rec + fp->F_Off) : NULL;
}

/* DBFFLDKE.C -- Retrieve key string corresponding to field number --*/

#include <string.h>
#include "a__dbf.h"

LPSTR DBFAPI dbf_FldKey(DBF_NO dbf, LPSTR pStr, UINT nFld)
{
	if (!dbf_Fld(dbf, pStr + 1, nFld)) return 0L;
	*pStr = nFld ? ((char)_dbf_pFile->pFldDef[nFld - 1].F_Len) : 1;
	return pStr;
}

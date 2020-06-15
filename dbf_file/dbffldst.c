/* DBFFLDST.C -- Retrieve data string corresponding to field number --*/

#include <string.h>
#include "a__dbf.h"

LPSTR DBFAPI dbf_FldStr(DBF_NO dbf, LPSTR pStr, UINT nFld)
{
	if (!dbf_Fld(dbf, pStr, nFld)) return NULL;
	pStr[nFld ? _dbf_pFile->pFldDef[nFld - 1].F_Len : 1] = 0;
	return pStr;
}

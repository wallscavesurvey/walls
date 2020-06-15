/* DBFPUTF.C -- Store field at current position*/

#include <string.h>
#include "a__dbf.h"

int DBFAPI dbf_PutFld(DBF_NO dbf, LPVOID pFld, UINT nFld)
{
	LPVOID p = dbf_FldPtr(dbf, nFld);

	if (p == NULL || dbf_Mark(dbf)) return dbf_errno;
	memcpy(p, pFld, nFld ? _dbf_pFile->pFldDef[nFld - 1].F_Len : 1);
	return 0;
}

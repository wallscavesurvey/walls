/* DBFGETF.C -- Retrieve field at current position*/

#include "a__dbf.h"

int DBFAPI dbf_GetFld(DBF_NO dbf, LPVOID pBuf, UINT nFld)
{
	return dbf_Fld(dbf, pBuf, nFld) ? 0 : dbf_errno;
}

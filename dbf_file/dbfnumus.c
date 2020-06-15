#include "a__dbf.h"

UINT DBFAPI dbf_NumUsers(DBF_NO dbf)
{
	UINT i = 0;
	DBF_pFILEUSR usrp;

	if (_GETDBFP)
		for (usrp = _DBFP->pFileUsr; usrp; usrp = usrp->U_pNext) i++;
	return i;
}

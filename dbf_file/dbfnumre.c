#include "a__dbf.h"

DWORD DBFAPI dbf_NumRecs(DBF_NO dbf)
{
	int e;
	if (!_GETDBFU) return 0;
	if (!(_DBFU->U_Mode&DBF_Share)) return _PDBFF->H.NumRecs;
	return ((e = _dbf_RefreshNumRecs()) >= 0) ? (DWORD)e : 0;
}

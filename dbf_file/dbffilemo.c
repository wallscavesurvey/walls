#include "a__dbf.h"

DWORD DBFAPI dbf_FileMode(DBF_NO dbf)
{
	return _GETDBFU ? _DBFU->U_Mode : (DWORD)-1;
}

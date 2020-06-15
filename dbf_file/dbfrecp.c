/* DBFRECP.C -- Retrieve user's record pointer --*/

#include "a__dbf.h"

LPVOID DBFAPI dbf_RecPtr(DBF_NO dbf)
{
	return _GETDBFU ? _DBFU->U_Rec : NULL;
}

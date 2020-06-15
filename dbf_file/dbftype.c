#include "a__dbf.h"

UINT DBFAPI dbf_Type(DBF_NO dbf)
{
	return _GETDBFP ? (_DBFP->H.FileID&DBF_FILEID_MASK) : 0;
}

UINT DBFAPI dbf_FileID(DBF_NO dbf)
{
	return _GETDBFP ? (_DBFP->H.FileID) : 0;
}

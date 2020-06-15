#include "a__dbf.h"

int DBFAPI dbf_DeleteCache(DBF_NO dbf)
{
	TRX_CSH csh;

	if (_GETDBFP) {
		csh = _DBFP->Cf.Csh;
		if (!csh) dbf_errno = DBF_ErrNoCache;
		else {
			dbf_errno = _csf_DeleteCache((CSF_NO)_DBFP, (CSH_NO *)&dbf_defCache);
			if (!dbf_errno && csh == dbf_defCache) dbf_defCache = 0;
		}
	}
	return dbf_errno;
}

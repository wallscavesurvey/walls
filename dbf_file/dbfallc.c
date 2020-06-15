#include "a__dbf.h"

int DBFAPI dbf_AllocCache(DBF_NO dbf, UINT bufsiz, UINT numbufs, BOOL makedef)
{
	UINT e;
	CSH_NO csh;

	if ((e = dbf_SizRec(dbf)) == 0) return dbf_errno;

	/*Cache not allowed for shared file --*/
	if (_PDBFU->U_Mode&DBF_Share) return dbf_errno = DBF_ErrFileShared;

	if (!bufsiz) bufsiz = e * (1 + (DBF_MIN_BUFSIZ - 1) / e);
	else if (bufsiz < e) bufsiz = e;

	if ((e = csh_Alloc((TRX_CSH far *)&csh, bufsiz, numbufs)) == 0 &&
		(e = dbf_AttachCache(dbf, csh)) != 0) csh_Free(csh);

	if (!e && makedef) dbf_defCache = csh;

	return dbf_errno = e;
}

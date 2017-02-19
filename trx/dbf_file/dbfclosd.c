#include "a__dbf.h"

int DBFAPI dbf_CloseDel(DBF_NO dbf)
{
	int e;

	if(!_GETDBFP) return DBF_ErrArg;

	/*Deletion not allowed for shared file --*/
	if(_PDBFU->U_Mode&DBF_Share) return DBF_ErrFileShared;

	_csf_Purge((CSF_NO)_DBFP);
	e=_csf_DeleteCache((CSF_NO)_DBFP,(CSH_NO *)&dbf_defCache);
	if(_dbf_pFileUsr->U_Mode&DBF_ReadOnly) {
	  if(dos_CloseFile(_DBFP->Cf.Handle) && !e) e=DBF_ErrClose;
	  else if(!e) e=DBF_ErrReadOnly;
	}
	else if(dos_CloseDelete(_DBFP->Cf.Handle) && !e) e=DBF_ErrDelete;
	_dbf_FreeUsr(_dbf_pFileUsr);
	_dbf_FreeFile(_DBFP);
	return dbf_errno=e;
}

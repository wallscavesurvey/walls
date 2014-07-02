/* DBFGETDBC.C --
   Validates DBF_NO and sets various register and global variables.
*/
#include "a__dbf.h"

DBF_pFILE PASCAL _dbf_GetDbfP(DBF_NO dbf)
{
	if(dbf>DBF_MAX_USERS || (_dbf_pFileUsr=_dbf_usrMap[--dbf])==NULL ||
	  _dbf_pFileUsr->U_Index!=dbf || (_dbf_pFile=_dbf_pFileUsr->U_pFile)==NULL) return NULL;
	_dbf_pFileUsr->U_Errno=dbf_errno=0;
	return _dbf_pFile;
}

DBF_pFILEUSR PASCAL _dbf_GetDbfU(DBF_NO dbf)
{
	if(dbf>DBF_MAX_USERS || (_dbf_pFileUsr=_dbf_usrMap[--dbf])==NULL ||
	  _dbf_pFileUsr->U_Index!=dbf || (_dbf_pFile=_dbf_pFileUsr->U_pFile)==NULL) return NULL;
	_dbf_pFileUsr->U_Errno=dbf_errno=0;
	return _dbf_pFileUsr;
}
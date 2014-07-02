/* DBFFLDNA.C -- Retrieve name corresponding to specified field number--*/

#include <string.h>
#include "a__dbf.h"

PSTR DBFAPI dbf_FldNam(DBF_NO dbf,PSTR buffer,UINT nFld)
{
  DBF_pFLDDEF fp;

  return (_dbf_GetDbfP(dbf) && (fp=_dbf_GetFldDef(nFld))!=0) ?
    strcpy(buffer,fp->F_Nam):0;
}

LPCSTR DBFAPI dbf_FldNamPtr(DBF_NO dbf,UINT nFld)
{
  DBF_pFLDDEF fp;

  return (_dbf_GetDbfP(dbf) && (fp=_dbf_GetFldDef(nFld))!=0) ?
    fp->F_Nam:0;
}

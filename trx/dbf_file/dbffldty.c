/* DBFFLDTY.C -- Retrieve type of specified field --*/

#include "a__dbf.h"

int DBFAPI dbf_FldTyp(DBF_NO dbf,UINT nFld)
{
  DBF_pFLDDEF fp;

  if(!_dbf_GetDbfP(dbf)) return 0;
  if(!nFld) return 'C';
  return (fp=_dbf_GetFldDef(nFld))!=0?fp->F_Typ:0;
}

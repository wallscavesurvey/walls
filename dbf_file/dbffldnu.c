/* DBFFLDNU.C -- Retrieve number corresponding to specified field name--*/

#include <string.h>
#include "a__dbf.h"

UINT DBFAPI dbf_FldNum(DBF_NO dbf,LPCSTR pFldNam)
{
  int i;
  DBF_pFLDDEF fp;

  if(!_GETDBFP || (fp=_dbf_GetFldDef(1))==0) return 0;
  for(i=_DBFP->NumFlds;i;i--) if(!strcmp(fp[i-1].F_Nam,pFldNam)) break;
  return i;
}

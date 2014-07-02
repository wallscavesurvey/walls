#include "a__dbf.h"

/*Global line indicator for DBF file consistency tests --*/
#ifdef _DBF_TEST
int _dbf_errLine;
#endif

apfcn_i _dbf_FormatError(void)
{
  return _dbf_pFile->Cf.Errno=dbf_errno=DBF_ErrFormat;
}

#ifdef _DBF_TEST
apfcn_i _dbf_FormatErrorLine(int line)
{
  _dbf_errLine=line;
  return _dbf_FormatError();
}
#endif

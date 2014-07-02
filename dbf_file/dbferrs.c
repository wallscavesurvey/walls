/* DBFERRS.C -- DBF file error strings*/

#include <dbf_file.h>

/* Error codes defined in dbf_file.h - DBF_ERR_BASE=100 --
enum {DBF_ErrEOF=DBF_ERR_BASE,
      DBF_ErrNotClosed,
      DBF_ErrFldDef,
	  DBF_ErrRecChanged
     };
*/

LPSTR _dbf_errstr[]={
 "DBF empty or not positioned",
 "DBF not in closed form",
 "Bad field definition",
 "Record was updated by another user"
};

#define DBF_ERR_LIMIT (DBF_ERR_BASE+sizeof(_dbf_errstr)/sizeof(LPSTR))

LPSTR TRXAPI dbf_Errstr(UINT e)
{
  if(e<DBF_ERR_BASE||e>=DBF_ERR_LIMIT) return csh_Errstr(e);
  return _dbf_errstr[e-DBF_ERR_BASE];
}

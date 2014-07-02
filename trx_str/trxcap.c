#include <string.h>
#include <ctype.h>
#include <trx_str.h>

TRXFCN_S trx_CapitalizeKey(LPSTR pdest)
{
	char *p=pdest;
	UINT len=*(BYTE *)p;
	BOOL bNextCap=TRUE;
	
	while(len--) {
		if(isspace(*++p)) bNextCap=TRUE;
		else if(bNextCap) {
		  *p=toupper(*p);
		  bNextCap=FALSE;
		}
		else *p=tolower(*p);
	}
	return pdest;
}


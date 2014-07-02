#include <string.h>
#include <trx_str.h>

TRXFCN_S trx_Stcext(LPSTR buf,LPCSTR pathname,LPSTR ext,int buflen)
{
	LPSTR p;
	trx_Stncc(buf,pathname,buflen);
	p=trx_Stpext(buf);
	if(!*p && ext && p-buf<buflen-(int)strlen(ext)-1) {
	  *p++='.';
	  strcpy(p,ext);
	}
	return buf;
}

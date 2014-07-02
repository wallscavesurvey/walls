#include <string.h>
#include <trx_str.h>

TRXFCN_I trx_Mergekey(LPBYTE key,LPCSTR p)
{
	//return 0 or strlen(p)+1 --
	UINT len=(UINT)strlen(p);
	if(*key+len>254) return 0;
	key[*key+1]=0;
	memcpy(&key[*key+2],p,len);
	(*key)+=(BYTE)(++len);
	return (int)len;
}




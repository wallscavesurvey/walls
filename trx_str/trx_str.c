#include <string.h>
#include <trx_str.h>

TRXFCN_S trx_Stpnam(LPCSTR pathname)
{
	LPCSTR p = pathname + strlen(pathname);

	while (--p >= pathname && *p != '\\' && *p != '/' && *p != ':');
	return (LPSTR)p + 1;
}

TRXFCN_S trx_Stpext(LPCSTR pathname)
{
	int len = strlen(pathname);
	LPCSTR p = pathname + len;

	while (p > pathname && *--p != '.' && *p != '\\' && *p != '/' && *p != ':');
	if (*p != '.') p = pathname + len;
	return (LPSTR)p;
}

TRXFCN_S trx_Stxcp(LPSTR p)
{
	UINT len = strlen(p);
	if (len > 255) len = 255;
	memmove(p + 1, p, len);
	*p = len;
	return p;
}

TRXFCN_S trx_Stxcpz(LPSTR p)
{
	trx_Stxcp(p);
	p[(BYTE)*p + 1] = 0;
	return p;
}

TRXFCN_S trx_Stxpc(LPSTR p)
{
	UINT len;
	memcpy(p, p + 1, len = (BYTE)*p);
	p[len] = 0;
	return p;
}

TRXFCN_S trx_Stncc(LPSTR pdst, LPCSTR psrc, UINT sizdst)
{
	//sizdst is pdst *size* (includes space for null)
	//Assume sizdst>0!
	UINT len = strlen(psrc);
	if (len >= sizdst) len = sizdst - 1;
	pdst[len] = 0;
	memcpy(pdst, psrc, len);
	return pdst;
}

TRXFCN_S trx_Stncp(LPSTR pdst, LPCSTR csrc, UINT sizdst)
{
	UINT len = strlen(csrc);
	if (len >= sizdst) len = sizdst - 1;
	*pdst = len;
	memcpy(pdst + 1, csrc, len);
	return pdst;
}

TRXFCN_S trx_Stccp(LPSTR pdst, LPCSTR psrc)
{
	UINT len = strlen(psrc);
	if (len > 255) len = 255;
	*pdst = len;
	memcpy(pdst + 1, psrc, len);
	return pdst;
}

TRXFCN_S trx_Stcpc(LPSTR pdst, LPCSTR psrc)
{
	UINT len = (BYTE)*psrc;
	pdst[len] = 0;
	return memcpy(pdst, psrc + 1, len);
}

TRXFCN_S trx_Stcpp(LPSTR pdst, LPCSTR psrc)
{
	memcpy(pdst + 1, psrc + 1, *pdst = *psrc);
	return pdst;
}


#include <string.h>
#include <trx_str.h>

TRXFCN_I trx_Strcmpc(LPCSTR s1, LPCSTR s2)
{
	int i, len1, len2;

	len1 = strlen(s1);
	len2 = strlen(s2);

	if (len1 < len2) {
		i = memcmp(s1, s2, len1);
		if (!i) i = -1;
		else i = (i > 0) ? 2 : -2;
	}
	else {
		i = memcmp(s1, s2, len2);
		if (!i) i = (len1 == len2) ? 0 : 1;
		else i = (i < 0) ? -2 : 2;
	}
	return i;
}

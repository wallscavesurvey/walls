#include <string.h>
#include <trx_str.h>

TRXFCN_I trx_Strcmpp(LPCSTR s1, LPCSTR s2)
{
	int i;

	if (*s1 < *s2) {
		i = memcmp(s1 + 1, s2 + 1, *s1);
		if (!i) i = -1;
		else i = (i > 0) ? 2 : -2;
	}
	else {
		i = memcmp(s1 + 1, s2 + 1, *s2);
		if (!i) i = (*s1 == *s2) ? 0 : 1;
		else i = (i < 0) ? -2 : 2;
	}
	return i;
}

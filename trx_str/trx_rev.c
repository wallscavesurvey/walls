// Transpose bytes in 4-byte integer --
#include <trx_str.h>

#pragma warning(disable:4035)

TRXFCN_DW trx_RevDW(DWORD d)
{
	__asm {
		mov     eax, d
		bswap	eax
	}
}
/*
; BT_LNKID.C
; ================================================================
; Internal routine to access records in CSH_HDRP nodes
; Part of A_TRX library BTLIB.LIB.
;
*/
#include <a__trx.h>

#pragma warning(disable:4035)

/*
;--------------------------------------------------------------------
;       bt_link - Helper routine to move to position keypos
;       in node whose address is at [ESI]. Assumes keypos is EBX.
;       Sets EBX at keyPos positions from high end (N_SIZLINK<=0)
;       or low end (N_SIZLINK>0).
;
;       Returns ECX=|N_SIZLINK|,
;				ESI=node addr
;               EBX=addr of rec at keypos. Sets carry if EDX==0 or EDX>N_NUMKEYS.
;--------------------------------------------------------------------
*/
void __declspec(naked) bt_link(void)
{
	__asm {
		mov		esi, [esi]
		push    esi
		movsx   edx, word ptr[esi + N_SIZLINK]
		neg     edx
		or ebx, ebx
		jnz     _lnk0
		add     esi, N_RLINK
		jmp     short _err

		_lnk0 :
		movzx	eax, word ptr[esi + N_FSTLINK]
			movzx   ecx, word ptr[esi + N_NUMKEYS]
			add		esi, eax
			cmp     ecx, ebx
			jb      _errx
			xor     eax, eax

			_loop : cmp     ebx, ecx
			jnb     _errx
			add     esi, edx; skip over link
			lodsb; get unmatching suffix length
			inc     esi; skip matching prefix length
			add     esi, eax; skip over suffix
			loop    _loop

			_err : stc

			_errx : mov		ebx, esi
			mov		ecx, edx
			pop     esi
			ret
	}
}

/*
;--------------------------------------------------------------------
; ulong _TRX_BTLNKC(CSH_HDRP hp,UINT keyPos);
;
; Returns link at keyPos positions from high end of branch node.
;--------------------------------------------------------------------
*/

apfcn_ul _trx_Btlnkc(CSH_HDRP hp, UINT keyPos)
{
	__asm {
		mov     ebx, keyPos
		mov     esi, hp
		call    bt_link; ECX = size link, ESI = addr node, EBX = addr rec
		jb      lnk2
		cmp		ecx, 4
		jb		lnk1
		mov		eax, [ebx]
		jmp		short lnkx

		lnk1 : movzx	eax, word ptr[ebx]
			   jmp     short lnkx

			   lnk2 : xor     eax, eax
					  lnkx :
	}
}


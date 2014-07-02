; BT_SETID.C
; ================================================================
; Part of A_TRX library BTLIB.LIB.
; Last revised 01/17/90 - 03/17/91 -- dmck
;--------------------------------------------------------------------
; void pascal _TRX_BTSETC(CSH_HDRP NODE,word CNTI,ptr LINK);
;
; Replaces link _TRX_BTCNT positions from high end of buffer with *LINK.
; If CNTI># keys or CNTI==0, then _TRX_BTSETC does nothing.
;--------------------------------------------------------------------

;#include <a__trx.h>

BITS 32
GLOBAL _trx_Btsetc   	;fcn
EXTERN bt_link

SECTION .text
;       bt_link - Helper routine to move to position keypos
;       in node whose address is at [ESI]. Assumes keypos is EDX.
;       Sets ESI at keyPos positions from high end (N_SIZLINK<=0)
;       or low end (N_SIZLINK>0). Returns ECX=|N_SIZLINK|, EDX=keypos,
;       and EBX=node offset. Sets carry if EDX==0 or EDX>N_NUMKEYS.
;       Does NOT destroy EDI.

;apfcn_v _trx_Btsetc(CSH_HDRP hp,UINT keyPos,PVOID link)
_trx_Btsetc:
		push	ebp
		mov		ebp, esp
		push	ebx
		push	esi
		push	edi
        mov     ebx,[ebp+12]	;keyPos
        mov     esi,[ebp+8]		;hp
        call    bt_link		;ECX=size link,ESI=addr node,EBX=addr rec
        jb      _rcerr
        mov     edi,ebx         ;di = rec offset in node
        mov     esi,[ebp+16]	;Get rec pointer (link)
        shr     ecx,1
        rep     movsw
        adc     ecx,ecx
        rep     movsb
_rcerr:
		pop		edi
		pop		esi
		pop		ebx
		mov		esp,ebp
		pop		ebp
		ret

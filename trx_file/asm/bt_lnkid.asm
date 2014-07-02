; BT_LNKID.C
; ================================================================
; Internal routine to access records in CSH_HDRP nodes
; Part of A_TRX library BTLIB.LIB.
;
;#include <a__trx.h>

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

N_SIZLINK     equ	0
N_NUMKEYS     equ	2
N_FSTLINK     equ	4
N_FLAGS       equ	6
N_TREE        equ	7
N_THIS        equ	8
N_RLINK       equ	12
N_LLINK       equ	16
N_BUFFER      equ	20

BITS 32

GLOBAL bt_link   	;naked fcn
GLOBAL _trx_Btlnkc  ;fcn

SECTION .text

;void __declspec(naked) bt_link(void)
bt_link:
		mov		esi,[esi]
        push    esi
        movsx   edx,word [esi+N_SIZLINK]
        neg     edx
        or      ebx,ebx
        jnz     _lnk0
        add     esi,N_RLINK
        jmp     short _err

_lnk0:	
		movzx	eax,word [esi+N_FSTLINK]
		movzx   ecx,word [esi+N_NUMKEYS]
		add		esi,eax
        cmp     ecx,ebx
        jb      _errx
        xor     eax,eax

_loop:  cmp     ebx,ecx
        jnb     _errx
        add     esi,edx ;skip over link
        lodsb           ;get unmatching suffix length
        inc     esi     ;skip matching prefix length
        add     esi,eax ;skip over suffix
        loop    _loop

_err:   stc

_errx:	mov		ebx,esi
		mov		ecx,edx
		pop     esi
		ret

;--------------------------------------------------------------------
; ulong _TRX_BTLNKC(CSH_HDRP hp,UINT keyPos);
;
; Returns link at keyPos positions from high end of branch node.
;--------------------------------------------------------------------

;apfcn_ul _trx_Btlnkc(CSH_HDRP hp,UINT keyPos)
_trx_Btlnkc:
		push	ebp
		mov		ebp, esp
		push	ebx
		push	esi
		push	edi

        mov     ebx,[ebp+12]	;keyPos
        mov     esi,[ebp+8]		;hp
        call    bt_link		;ECX=size link,ESI=addr node,EBX=addr rec
        jb      lnk2
		cmp		ecx,4
		jb		lnk1
        mov		eax,[ebx]
		jmp		short lnkx

lnk1:	movzx	eax,word [ebx]
        jmp     short lnkx

lnk2:   xor     eax,eax
lnkx:
		pop		edi
		pop		esi
		pop		ebx
		mov		esp,ebp
		pop		ebp
		ret

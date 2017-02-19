; BT_KEYID.C -- Part of TRX_FILE library BTLIB.LIB
;-------------------------------------------------------------
; apfcn_i _trx_Btkey(CSH_HDRP Node,UINT KeyPos,PBYTE KeyBuf,UINT KeyBufLen);
;
; Retrieves key which is keyPos positions from the rightmost
; (high) end of node.
; Returns TRX_ErrFormat if keyPos is out of range.
; Returns TRX_ErrTruncate if a truncated key is copied.
;-------------------------------------------------------------

;#include <a__trx.h>

N_SIZLINK     equ	0
N_NUMKEYS     equ	2
N_FSTLINK     equ	4
N_FLAGS       equ	6
N_TREE        equ	7
N_THIS        equ	8
N_RLINK       equ	12
N_LLINK       equ	16
N_BUFFER      equ	20

TRX_ErrFormat	equ	15
TRX_ErrTruncate	equ	112

BITS 32
GLOBAL _trx_Btkey   	;DWORD

SECTION .text

;apfcn_i _trx_Btkey(CSH_HDRP Node,UINT KeyPos,PBYTE KeyBuf,UINT KeyBufLen)
_trx_Btkey:
		push	ebp
		mov		ebp, esp
		push	ebx
		push	esi
		push	edi
        sub     dword [ebp+20],1	;KeyBufLen--
        jb      _trnc00
        xor     eax,eax
        mov     edi,[ebp+16]	;Keybuf
        stosb
		cmp		dword [ebp+20],100h
		jb		_bt0
		mov		dword [ebp+20],0FFh	;Overflow not possible
_bt0:   mov     eax,[ebp+12]	;Keypos
        sub     eax,1
        jb      _err
        mov     esi,[ebp+8]		;Node
		mov		esi,[esi]
        movzx   edx,word [esi+N_NUMKEYS]
        sub     edx,eax
        jbe     _err
        movsx   ebx,word [esi+N_SIZLINK]
		movzx	ecx,word [esi+N_FSTLINK]
        add     esi,ecx
        xor     ecx,ecx

;At this point -- EDX = #keys remaining up to and incl target key
;		  DS:ESI = points to current link
;		  ES:EDI = points to destination string

_loop:  sub     esi,ebx         ;skip link
        lodsw                   ;AH:AL = prefix:suffix
        or      al,al
        jz      _nxt
        mov     cl,ah
        add     al,ah
        mov     edi,[ebp+16]	;Keybuf
        cmp     al,byte[ebp+20] ;KeyBufLen
        ja      _trnc
        stosb
        sub     al,ah
        add     edi,ecx
        mov     cl,al
        rep     movsb
_nxt:   dec     edx
        jnz     _loop
        xor     eax,eax
        jmp     short _ret

_err:   mov     eax,TRX_ErrFormat
        jmp     short _ret

_trnc00: jmp short _trnc0

		;Lines revised 7/26/98 to skip over truncated (but smaller) key (DMCK)
		;al=current key length (prefix+suffix) > KeyBufLen
		;ah=current prefix length
		;edi=&keybuf
		;esi=&suffix
		;edx=keys remaining including current

_trnc:	mov		cl,byte[ebp+20]	;Keybuflen
		cmp		cl,ah
		ja		_trnc1
		sub		al,ah	;al=suffix length
		jmp		short _trnc2

_trnc1:	mov		byte [edi],cl
		mov		cl,ah
		inc		edi
		add		edi,ecx
		mov		cl,byte[ebp+20]	;Keybuflen
		sub		cl,ah	;ecx=bytes to move from suffix
		sub		al,ah	;al=suffix length
		sub		al,cl	;al=remaining bytes in suffix
		rep		movsb

_trnc2:	mov		cl,al
		add		esi,ecx
		dec		edx
		jnz		_loop

_trnc0:	mov     eax,TRX_ErrTruncate
_ret:
		pop		edi
		pop		esi
		pop		ebx
		mov		esp,ebp
		pop		ebp
		ret

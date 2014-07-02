/*
; BT_KEYID.C -- Part of TRX_FILE library BTLIB.LIB
;-------------------------------------------------------------
; apfcn_i _trx_Btkey(CSH_HDRP Node,UINT KeyPos,PBYTE KeyBuf,UINT KeyBufLen);
;
; Retrieves key which is keyPos positions from the rightmost
; (high) end of node.
; Returns TRX_ErrFormat if keyPos is out of range.
; Returns TRX_ErrTruncate if a truncated key is copied.
;-------------------------------------------------------------
*/

#include <a__trx.h>

#pragma warning(disable:4035)

apfcn_i _trx_Btkey(CSH_HDRP Node,UINT KeyPos,PBYTE KeyBuf,UINT KeyBufLen)
{

	__asm {
        sub     KeyBufLen,1
        jb      _trnc0
        xor     eax,eax
        mov     edi,KeyBuf
        stosb
		cmp		KeyBufLen,100h
		jb		_bt0
		mov		KeyBufLen,0FFh	;Overflow not possible
_bt0:   mov     eax,KeyPos
        sub     eax,1
        jb      _err
        mov     esi,Node
		mov		esi,[esi]
        movzx   edx,word ptr[esi+N_NUMKEYS]
        sub     edx,eax
        jbe     _err
        movsx   ebx,word ptr[esi+N_SIZLINK]
		movzx	ecx,word ptr[esi+N_FSTLINK]
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
        mov     edi,KeyBuf
        cmp     al,byte ptr KeyBufLen
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

		;Lines revised 7/26/98 to skip over truncated (but smaller) key (DMCK)
		;al=current key length (prefix+suffix) > KeyBufLen
		;ah=current prefix length
		;edi=&keybuf
		;esi=&suffix
		;edx=keys remaining including current

_trnc:	mov		cl,byte ptr KeyBufLen
		cmp		cl,ah
		ja		_trnc1
		sub		al,ah	;al=suffix length
		jmp		short _trnc2

_trnc1:	mov		byte ptr[edi],cl
		mov		cl,ah
		inc		edi
		add		edi,ecx
		mov		cl,byte ptr KeyBufLen
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
	}
}

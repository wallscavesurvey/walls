; BT_RECID.C
; ================================================================
; Routines to access records in CSH_HDRP nodes
; Part of TRX_FILE library.
; #include <a__trx.h>

TRX_ErrFormat	equ	15
TRX_ErrTruncate	equ	112

BITS 32

EXTERN bt_link
GLOBAL _trx_Btrecp
GLOBAL _trx_Btkeyp
GLOBAL _trx_Btrec

SECTION .text

;--------------------------------------------------------------------
; vptr _TRX_BTRECP(CSH_HDRP hp,UINT keyPos);
;
; Returns pointer to link keyPos positions from high end of node.
; or from low end if N_SIZLINK>0.
;--------------------------------------------------------------------

;apfcn_vp  _trx_Btrecp(CSH_HDRP hp,UINT keyPos)
_trx_Btrecp:
		push	ebp
		mov		ebp, esp
		push	ebx
		push	esi
		push	edi
        mov     ebx,[ebp+12] ;keyPos
        mov     esi,[ebp+8]  ;hp
        call    bt_link		;ECX=size link,ESI=addr node,EBX=addr rec
        mov     eax,ebx
        jnb      _fnp1
		xor		eax,eax
_fnp1:
		pop		edi
		pop		esi
		pop		ebx
		mov		esp,ebp
		pop		ebp
		ret


;--------------------------------------------------------------------
; PBYTE _TRX_BTKEYP(CSH_HDRP hp,UINT keyPos);
;
; Returns byte pointer to key's suffix length byte, which is zero only
; in case of a duplicate or null key. The next byte is the matching prefix
; length. This is followed by the actual suffix bytes.
;--------------------------------------------------------------------
;apfcn_p _trx_Btkeyp(CSH_HDRP hp,UINT keyPos)
_trx_Btkeyp:
		push	ebp
		mov		ebp, esp
		push	ebx
		push	esi
		push	edi
        mov     ebx,[ebp+12] ;keyPos
        mov     esi,[ebp+8]  ;hp
        call    bt_link		;ECX=size link,ESI=addr node,EBX=addr rec
		jnb		_fn0
		xor		eax,eax
		jmp		short _fnp2
_fn0:   mov     eax,ebx
        add     eax,ecx    ;skip over record
_fnp2:
		pop		edi
		pop		esi
		pop		ebx
		mov		esp,ebp
		pop		ebp
		ret

;--------------------------------------------------------------------
; apfcn_i _TRX_BTREC(CSH_HDRP hp,UINT keyPos,vptr recbuf,UINT reclen);
;
; Retrieves link (or record) at node's keyPos position.
; Only min(|N_SIZLINK|,reclen) bytes are copied.
; Returns TRX_ErrFormat if keyPos is out of range.
; Returns TRX_ErrTruncate if a truncated record is copied.
;--------------------------------------------------------------------

;apfcn_i _trx_Btrec(CSH_HDRP hp,UINT keyPos,PVOID recbuf,UINT sizRecBuf)
_trx_Btrec:
		push	ebp
		mov		ebp, esp
		push	ebx
		push	esi
		push	edi
        mov     ebx,[ebp+12] ;keyPos
        mov     esi,[ebp+8]  ;hp
        call    bt_link		;ECX=size link,ESI=addr node,EBX=addr rec
        jb      _err
        xor     eax,eax
		mov     edi,[ebp+16] ;recbuf
		mov		esi,ebx
        mov     edx,[ebp+20] ;sizRecBuf
        cmp     ecx,edx
        jbe     _rmov
        mov     ecx,edx
        mov     eax,TRX_ErrTruncate
_rmov:  shr     ecx,1
        rep     movsw
        adc     ecx,ecx
        rep     movsb
		jmp		short _ex

_err:   mov     eax,TRX_ErrFormat
_ex:
		pop		edi
		pop		esi
		pop		ebx
		mov		esp,ebp
		pop		ebp
		ret

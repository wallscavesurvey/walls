/*
; BT_RECID.C
; ================================================================
; Routines to access records in CSH_HDRP nodes
; Part of TRX_FILE library.
*/

//Also includes a__csh.h and trx_file.h

#include <a__trx.h>

#pragma warning(disable:4035)

/*
;--------------------------------------------------------------------
; vptr _TRX_BTRECP(CSH_HDRP hp,UINT keyPos);
;
; Returns pointer to link keyPos positions from high end of node.
; or from low end if N_SIZLINK>0.
;--------------------------------------------------------------------
*/
apfcn_vp  _trx_Btrecp(CSH_HDRP hp,UINT keyPos)
{
	__asm {
        mov     ebx,keyPos
        mov     esi,hp
        call    bt_link		;ECX=size link,ESI=addr node,EBX=addr rec
        mov     eax,ebx
        jnb      _fnp
		xor		eax,eax
_fnp:
	}
}

/*
;--------------------------------------------------------------------
; PBYTE _TRX_BTKEYP(CSH_HDRP hp,UINT keyPos);
;
; Returns byte pointer to key's suffix length byte, which is zero only
; in case of a duplicate or null key. The next byte is the matching prefix
; length. This is followed by the actual suffix bytes.
;--------------------------------------------------------------------
*/
apfcn_p _trx_Btkeyp(CSH_HDRP hp,UINT keyPos)
{
	__asm {
        mov     ebx,keyPos
        mov     esi,hp
        call    bt_link		;ECX=size link,ESI=addr node,EBX=addr rec
		jnb		_fn0
		xor		eax,eax
		jmp		short _fnp
_fn0:   mov     eax,ebx
        add     eax,ecx    ;skip over record
_fnp:
	}
}

/*
;--------------------------------------------------------------------
; apfcn_i _TRX_BTREC(CSH_HDRP hp,UINT keyPos,vptr recbuf,UINT reclen);
;
; Retrieves link (or record) at node's keyPos position.
; Only min(|N_SIZLINK|,reclen) bytes are copied.
; Returns TRX_ErrFormat if keyPos is out of range.
; Returns TRX_ErrTruncate if a truncated record is copied.
;--------------------------------------------------------------------
*/

apfcn_i _trx_Btrec(CSH_HDRP hp,UINT keyPos,PVOID recbuf,UINT sizRecBuf)
{
	__asm {
        mov     ebx,keyPos
        mov     esi,hp
        call    bt_link		;ECX=size link,ESI=addr node,EBX=addr rec
        jb      _err
        xor     eax,eax
		mov     edi,recbuf
		mov		esi,ebx
        mov     edx,sizRecBuf
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
	}
}
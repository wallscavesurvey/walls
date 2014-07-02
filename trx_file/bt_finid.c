// BT_FINID.C
// ================================================================
// Part of TRX_FILE library BTLIB.LIB.
// Last revised 07/20/87 - 01/17/90 - 02/25/91 -- dmck
// 32-bit conversion 6/27/97 -- dmck

//Also includes a__csh.h and trx_file.h

#include <a__trx.h>

#pragma warning(disable:4035)

DWORD trx_findResult;
DWORD _trx_btcnt;
LPVOID _trx_pFindRec;

//----------------------------------------------------------
// bt_srch - Internal routine to search node for target key
//			 CAUTION: All __asm callers must also use EBX
//					  so it will be preserved upon entry
//
// Call with: EDI       - target key
//            ESI       - nodes's cache buffer handle
//
// Results:   EAX=0     - Target key exactly matches node key
//            EAX=xx00  - Target key is prefix of node key
//                        which has xx additional chars
//            EAX=00xx  - Unmatching suffix chars in target key
//            ECX       - # of new prefix chars in matched node key
//            EDX       - Node key count, 0 if target key > last node key
//            EBX       - Offset of prefix:suffix word of matched node
//                        key link if EDX!=0, else EBX=node size+link size
//            ESI       - Node's segment:offset
//            EDI       - Negative link size
//
// Counters used during processing:
//
//            CH  = matching prefix in target key
//            CL  = remaining unmatched suffix in target key
//            EDX = node key count
//            EBX = offset of current key prefix:suffix word
//            EBP = negative link size

void __declspec(naked) bt_srch(void)
{
	__asm {
		xor		eax,eax
		mov	    esi,[esi]
        movzx   edx,word ptr[esi+N_NUMKEYS]
        inc     edx                     ;EDX = number of node keys+1

        push    esi                     ;Save node offset
        push    ebp                     ;Preserve BP

        movsx   ebp,word ptr[esi+N_SIZLINK]  ;EBP = negative link size
        movzx   ecx,word ptr[esi+N_FSTLINK]  ;ESI = offset of first link
		add		esi,ecx
        movzx   ecx,byte ptr[edi]       ;Get target key length in ECX
        inc     edi                     ;Point to first byte
        jmp     short _nxt2

_nxt0:  mov     esi,ebx                 ;Position source pointer
        lodsw
_nxt1:  xor     ah,ah                   ;AX = node key suffix length
        add     esi,eax                 ;Skip over suffix
_nxt2:  sub     esi,ebp                 ;Skip over link
        mov     ebx,esi                 ;bx points to current node key
        dec     edx                      ;decrement node key counter
        jz      _excl                   ;Target key > last node key
        lodsw                           ;AL=suffix len, AH=prefix len
        cmp     ah,ch                   ;If prefix len > # matched chars
        ja      _nxt1                   ; in target key, try next node key
        jb      _excl                   ;Exit if prefix len < # matchd chars

;       Prefixes match - compare suffixes --
        or      al,al
        jnz     _cmp

;       Node key suffix is zero (duplicate key or NULL key) --
        or      cl,cl                   ;Any unmatched chars in search?
        jnz     _nxt2                   ;Next key if target suffix exists

;       Else both keys must be NULL, with AX=CX=0

_exnul: xchg    ah,al
        jmp     short _btex

;       Node key suffix nonzero - compare with target key suffix
_cmp:   xor     ah,ah
        cmp     cl,al
        jnb     _cmp1
        or      cl,cl                   ;If CL==0, target key must be NULL
        jz      _exnul                  ;If so, return AH=node key suffix
        mov     al,cl

_cmp1:  xchg    eax,ecx                 ;Set ecx = compare count
        repe    cmpsb                   ;Repeat while equal
        pushf                           ;Save flags
        mov     ecx,eax
        mov     eax,esi
        sub     eax,ebx
        sub     ax,3                    ;AL = iterations-1
        add     ch,al                   ;At least AL chars matched
        sub     cl,al                   ;AL+1 matches if z-flag set with cmpsb
        dec     edi
        popf
        jb      _nxt0                   ;Node subkey < target key
        je      _mtch

;       Node subkey > target key
        xchg    eax,ecx                 ;Set ECX=matches with node suffix
        xor     ah,ah                   ;Set EAX=target key suffix
		jmp		short _btex

;       One key is prefix of the other --
_mtch:  inc     edi                     ;DI at next target key char
        inc     ch                      ;Revise target key prefix
        dec     cl                      ;CL = target key chars left
        jnz     _nxt0                   ;If > 0, look at next node key

;       Subkey = target key + suffix --
        inc     al                      ;AL = good compares
        mov     ecx,eax                 ;CX = matches with node suffix
        xchg    al,ah
        sub     ah,byte ptr[ebx]        ;AH = unmatching node suffix
        jmp     short _btex

_excl:  xchg    eax,ecx                 ;Return EAX=CL, ECX=0
        xor     ecx,ecx
        mov     ah,ch

_btex:	mov     edi,ebp                 ;Set DI=negative link size
        pop     ebp
        pop     esi                     ;Set DS:SI=node buffer address
		ret
	}
}

//---------------------------------------------------------------
// apfcn_i _TRX_BTFND(CSH_HDRP hp,char far *Key);
//
// Searches node with cache buffer handle hp for the first key
// greater than or equal to the "length byte prefix" string at
// address Key. Returns:
//
//  EAX  _trx_btcnt >= 0 to the number of keys (>= Key) remaining
//       inclusive of the key found, or zero if there are no
//       keys in node >= Key.

//  EDX  trx_findResult=0000 if and only if an exact match is
//       found. DX = xx00 if target Key matches a node key's prefix
//       (but is xx chars shorter), or DX=00xx if there are
//       xx unmatching suffix chars in target key.
//		_trx_pFindRec = address of record at position _trx_btcnt,
//       or address(end of node)-record length if AX=0.
//       NOT USED, but may be used later.
//---------------------------------------------------------------

apfcn_i _trx_Btfnd(CSH_HDRP hp,PBYTE key)
{
	__asm {
		mov		edi,key
		mov		esi,hp
        call    bt_srch         ;EDX=count position of matched key
                                ;EDI=neg link size, EBX=pfx:sfx offset
        add     ebx,edi
        mov     _trx_pFindRec,ebx
        mov     _trx_btcnt,edx
        mov     trx_findResult,eax
		xchg	eax,edx
	}
}

//---------------------------------------------------------------
// apfcn_i _TRX_BTFNDN(CSH_HDRP hp,UINT KeyPos,char far *pKey);
//
// Compares pKey with node key at count position Keypos.
// Sets AX=trx_findResult as follows (compatible with _trx_Btfnd):
//
// FFFFh if pKey is larger than the node key at position KeyPos.
// 00xxh if pKey is less than the node key and it is not a prefix
//       (xxh is nonzero).
// xx00h if pKey is a prefix of the node key, but is not an exact match
//       (xxh is the length of the nonmatching suffix).
// 0000h if pKey exactly matches the node key.
//---------------------------------------------------------------

apfcn_i _trx_Btfndn(CSH_HDRP hp,UINT keyPos,PBYTE key)
{
	__asm {
		mov		edi,key			;EDI=addr of target key
	    mov     esi,hp			;ESI=buffer handle
        call    bt_srch         ;EDX=count position of matched key
                                ;EDI=neg link size, EBX=pfx:sfx offset
        mov     ecx,keyPos      ;ECX=count position of destination pfx:sfx
        cmp     ecx,edx         ;assumes ECX >= 1
        jz      _btfnx          ;If at destination, return rslt of bt_srch
        jb      _btfn0

; The target is larger than the key at position CX=KeyPos --
        mov     eax,0FFFFh
        jmp     short _btfnx

; The target is smaller than the key at position CX=KeyPos --
_btfn0: or      al,al
        jz      _btfn2

// The target is smaller, but NOT a prefix --
_btfn1: mov     eax,00FFh
        jmp     short _btfnx

// At this point: EDI=neg link size, CX=count of dest key,
// EBX=current pfx:sfx offset, DX=current count.
// The target at least matches the prefix of a smaller key.
// Advance, as required, to the destination pfx:sfx --

_btfn2: mov     esi,key
        mov     al,byte ptr[esi]
        mov     esi,ebx         ;SI=addr of current pfx:sfx in node
        mov     bl,al           ;BL=target key length

_btfn3:	xor		eax,eax
		lodsw                   ;AH:AL=current pfx:sfx
		xor		ah,ah
        add     esi,eax
        sub     esi,edi         ;SI=addr of next pfx:sfx
        cmp     bl,byte ptr[esi+1]
        ja      _btfn1          ;if the target is not a prefix of this key
        dec     edx             ;DX=count position of this key
        cmp     edx,ecx
        ja      _btfn3

; The target is a prefix of the key at the destination position.
; Set AX=xx00h, where xxh is the number of nonmatching suffix chars --

        lodsw                   ;AH:AL=pfx:sfx
        add     ah,al           ;AH=node key's length
        sub     ah,bl           ;AH=length of nonmatching suffix
        xor     al,al
_btfnx:
        mov     trx_findResult,eax
	}
}

//---------------------------------------------------------------
// apfcn_ul _TRX_BTLNK(CSH_HDRP hp,char far *Key);
//
// Similar to TRX_BTFND, but returns link as unsigned long, with zero
// padding or truncation as required. Sets _trx_btcnt and trx_findResult.

apfcn_ul _trx_Btlnk(CSH_HDRP hp,PBYTE key)
{
	__asm {
		mov		edi,key
		mov		esi,hp
        call    bt_srch					;EDX=count position of matched key
										;EDI=neg link size, EBX=pfx:sfx offset
        mov     trx_findResult,eax
		mov     _trx_btcnt,edx
        or      edx,edx
        jz      FND0
		add     ebx,edi                 ;EBX=link offset in node
		neg		edi
		cmp     edi,4
		jae		FNDDW
		movzx	eax,word ptr[ebx]
		jmp		short FNDX

FND0:   mov     ebx,N_RLINK
        add     ebx,esi                 ;BX=N_RLINK offset
FNDDW:	mov		eax,[ebx]
FNDX:
	}
}

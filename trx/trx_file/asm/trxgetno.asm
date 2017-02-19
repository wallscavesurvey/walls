; trxgetno.c
; ================================================================
; Part of A_TRX library TRX_FILE.LIB.
; Last revised 3/1/93 -- dmck
; Converted to 32-bits 6/29/97 -- dmck
;--------------------------------------------------------------------
; CSH_HDRP pascal _trx_GetNode(ulong nodeno)
;
; Obtains handle to buffer containing TRX file node number nodeno.
; If nodeno is zero, trx_errno is set to TRX_ErrEOF (the file's
; error flag is NOT set) and zero is returned. Similarly, if there
; is no cache assigned, TRX_ErrNoCache is returned with the file's
; error flag reset to zero. If nodeno is greater than the number of
; nodes in the file, the file's error flag is set to TRX_ErrFormat
; and zero is returned.
;
; If a (successful) disk read occurs (and _trx_noErrNodRec is zero),
; nodeno is checked against the value stored at offset N_THIS in the
; node. In case of a mismatch, the error flag is set to TRX_ErrFormat
; and zero is returned. (The buffer is not purged.)
;
; Assumes DS=DGROUP. Uses registers ES,AX,BX,CX,DX.
;--------------------------------------------------------------------

;#include <a__trx.h>
N_THIS        equ	8

TRX_ErrFormat	equ	15
TRX_ErrTruncate	equ	112
TRX_ErrEOF		equ 105
TRX_ErrNoCache	equ 6		

BITS 32

GLOBAL _trx_GetNode
GLOBAL _trx_hdrp

EXTERN	_csf_GetRecord
EXTERN	_trx_noErrNodRec
EXTERN	_trx_pFile
EXTERN	trx_errno
EXTERN	_csh_newRead

SECTION .text

;apfcn_hdrp _trx_GetNode(HNode nodeno)
_trx_GetNode:
		push	ebp
		mov		ebp, esp
		push	esi

        mov     esi,[_trx_pFile]
		mov		eax,[ebp+8]	;nodeno
		or		eax,eax
        mov     ecx,TRX_ErrEOF
        jz      _err0
        dec     eax
        mov     ecx,TRX_ErrFormat
		cmp		eax,[esi+60]	;TRX_FILE.NumNodes
        jb      _recOK

; Fatal errors: TRX_ErrFormat, TRX_ErrLocked, TRX_ErrRead, TRX_ErrWrite
; Non-fatal errors: TRX_ErrEOF, TRX_ErrNoCache

_error: mov     [esi+24],ecx	;TRX_FILE.Cf.Errno,ecx
_err0:  mov     [trx_errno],ecx
        xor     eax,eax
        jmp     short _ex

_recOK: push    dword 0
        push    eax
		push	esi
        call    _csf_GetRecord
		add		esp,12
        or      eax,eax
        jnz     _rdOK
        mov     ecx,[esi+24]	;TRX_FILE.Cf.Errno
        cmp     ecx,TRX_ErrNoCache
        jnz     _err0
        mov     [esi+24],eax	;TRX_FILE.Cf.Errno,eax  ;make it non-fatal
        jmp     short _err0

_rdOK:  cmp     dword [_csh_newRead],0
        jz      _ex
        cmp     dword [_trx_noErrNodRec],0
        jnz     _ex

;       Optionally check node's N_This value if it was read from disk --
        mov     ecx,TRX_ErrFormat
        mov     esi,eax
		mov		esi,[esi]
		mov		edx,[ebp+8]	;nodeno
        cmp     edx,[esi+N_THIS]
        jne     _error
_ex:
        mov     [_trx_hdrp],eax
		pop		esi
		mov		esp,ebp
		pop		ebp
		ret

SECTION	.bss

_trx_hdrp	resd 1

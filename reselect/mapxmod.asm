%macro GLOBAL@ 1
	%ifdef _MSC_
	GLOBAL _%1
	%define %1 _%1
	%else
	GLOBAL %1
	%endif
%endmacro

	BITS 32
	SECTION .text

	GLOBAL@ mapset
	GLOBAL@ mapclr
	GLOBAL@ maptst
	GLOBAL@ mapcnt

mapset:
	push	ebp
	mov		ebp,esp
	push	ebx
	mov		ebx,[ebp+12]  ;Bit number
	mov		cl,bl
	and		cl,7
	shr		ebx,1
	shr		ebx,1
	shr		ebx,1
	add		ebx,[ebp+8]
	mov		eax,1
	rol		al,cl
	mov		cl,al
	mov		al,[ebx]
	and		al,cl
	or		[ebx],cl
	pop		ebx
	mov		esp,ebp
	pop		ebp
	ret

maptst:
	push	ebp
	mov		ebp,esp
	push	ebx
	mov		ebx,[ebp+12]  ;Bit number
	mov		cl,bl
	and		cl,7
	shr		ebx,1
	shr		ebx,1
	shr		ebx,1
	add		ebx,[ebp+8]	  ;Map address
	mov		eax,1
	rol		al,cl
	and		al,[ebx]
	pop		ebx
	mov		esp,ebp
	pop		ebp
	ret

mapclr:
	push	ebp
	mov		ebp,esp
	push	ebx
	mov		ebx,[ebp+12]  ;Bit number
	mov		cl,bl
	and		cl,7
	shr		ebx,1
	shr		ebx,1
	shr		ebx,1
	add		ebx,[ebp+8]	  ;Map address
	mov		al,0FEh
	rol		al,cl
	and		[ebx],al
	pop		ebx
	mov		esp,ebp
	pop		ebp
	ret

mapcnt:
	push	ebp
	mov		ebp,esp
	push	esi
	push	edi
	push	ebx
	mov		esi,[ebp+8]  ;Map address
	mov		eax,[ebp+12] ;Map length in DWORDS
	shl		eax,1
	shl		eax,1
	add		eax,esi
	mov		edi,eax
	xor		eax,eax

cntl1:
	cmp		esi,edi
	jnb		cntl3
    mov     edx,[esi]
    mov     ecx,32
    xor     ebx,ebx
cntl2:
    rcr     edx,1
    adc     ebx,0
    loop    cntl2

	add		eax,ebx
	add		esi,4
	jmp		cntl1

cntl3:
	pop		ebx
	pop		edi
	pop		esi
	mov		esp,ebp
	pop		ebp
	ret

;EOF


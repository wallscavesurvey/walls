/*
; BT_DELID.C
;--------------------------------------------------------------------
; void _TRX_BTDEL(CSH_HDRP hp,UINT keyPos);
;
; Deletes key/link which is keyPos positions from right end
; of node. If keyPos = 0 or keyPos > N_NUMKEYS, no deletion occurs.
;-------------------------------------------------------------
*/
#include <a__trx.h>

#pragma warning(disable:4035)

apfcn_v _trx_Btdel(CSH_HDRP hp, UINT keyPos)
{
	__asm {
		mov     ebx, keyPos
		mov     esi, hp
		call    bt_link; ECX = size link, ESI = addr node, EBX = addr rec
		jnb     _del0
		jmp		_delx
		_del0 : xchg	ebx, esi
				push    ebx; Save node offset
				push    esi; Save link position
				mov     ebx, ecx; BX = link size
				mov     ecx, keyPos; CX = keypos

				// DS:SI=offset of keyPos link, BX=link size, CX=keyPos,
				// DX and DI available.

				// Calculate move parameters while advancing SI to next key --
				xor		eax, eax
				mov     edi, esi
				add     esi, ebx
				lodsw
				mov     dx, ax; save DH : DL = prefix : suffix
				xor ah, ah
				add     esi, eax
				dec     ecx; check keypos - 1
				jz      xmov0; If no next key

				// SI is now positioned at the next link.
				// DI is at the keypos link.
				// The next key's suffix must be expanded if its prefix
				// is larger than the deleted key's prefix.

				xor     ecx, ecx
				mov     ax, word ptr[esi + ebx]
				sub     ah, dh
				jbe     xmov0; If no adjustment required

				//       Move next key's link to position of deleted link --
				mov     ecx, ebx
				rep     movsb

				//       Next key's suffix must be expanded by ah bytes --
				add     byte ptr[esi], ah
				sub     byte ptr[esi + 1], ah

				//       Move the revised pfx:sfx word --
				movsw

				//       Prepare to shift revised link/prefix right to final position --
				mov     cl, ah
				add     edi, ecx; skip past new suffix bytes
				inc     ecx
				inc     ecx
				add     ecx, ebx; CX = additional bytes to shift right

				xmov0 : xchg    edi, esi

						; DI is at end - of node or at next key's suffix.
						; SI is at the keypos link or at final position of next key's suffix.
						; CX is zero, or additional bytes to shift right

						pop     edx; DX = segment offset of deleted link
						pop     ebx; BX = node segment offset
						sub     edx, ebx; DX = node relative offset of deleted link
						sub     dx, word ptr[ebx + N_FSTLINK]; DX = length of data left of deletion
						add     ecx, edx
						mov     eax, 2
						sub     esi, eax; SI = right end of source
						sub     edi, eax; DI = right end of destination
						std
						shr     ecx, 1
						rep     movsw
						jnb     xmov1; leave DI at AX = 2 bytes before N_FSTLINK
						inc     esi
						inc     edi
						dec     eax; leave DI at AX = 1 bytes before N_FSTLINK
						movsb
						xmov1 : cld
								add     eax, edi
								sub     eax, ebx; AX = new node - relative offset of first link
								mov     word ptr[ebx + N_FSTLINK], ax
								dec     word ptr[ebx + N_NUMKEYS]
								_delx:
	}
}
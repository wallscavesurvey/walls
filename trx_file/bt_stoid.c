/*
; BT_STOID.C
; ================================================================
; Part of A_TRX library BTLIB.LIB.
; Last revised  03/03/91 -- dmck
; Conversion to 32-bits 6/28/97 -- dmck
; Correct bug in _trx_Btcut 11/18/2003 -- dmck
;
*/

#include <a__trx.h>
#pragma warning(disable:4035)

// Private data initialized by _TRX_BTSTO() and used by _TRX_BTSCN() and
// _TRX_BTCUT() --
static DWORD _BTLNK, _BTKEY, _BTNOD, _BTOFF, _BTSIZ;

/*
;--------------------------------------------------------------------
; int pascal _TRX_BTSCN(void);
;
; Calls must immediately follow an unsuccessful _TRX_BTSTO() attempt.
; Examines next duplicate key set in node and stores computed results
; in _TRX_BTSCNDATA structure:
;
;_TRX_BTSCNDATA -
;	CntBound	- Count value of prospective upperbound key, which
;                 is externally set to zero before first function call.
;   SizDupLead  - Length of part prefixing last duplicate in a chain,
;                 which is always stored in the new node.
;   SizDupLast  - Length of part for last duplicate (or whole item),
;                 which is moved to the new node only at leaf level.
;   SpcRemain   - New relative offset of first link in old node, which is
;                 adjusted for suffix expansion and new key if present.
;   SizBoundKey - Length of upperbound key (may or may not be useful).
;   ScnOffset   - Node offset of next link/key to be examined.
;
; Returns AX=0 if no more sets remain.
;--------------------------------------------------------------------
*/
apfcn_i _trx_Btscn(void)
{
	__asm {
		push	ebp
		mov     ecx, _trx_btscnData.CntBound
		jcxz   _scn0
		dec     ecx
		jz      _scnx0
		mov     esi, _trx_btscnData.ScnOffset
		jmp     short _scn1

		_scnx0 : mov     eax, ecx
				 jmp		_scnex

				 //       Initializing --
				 _scn0 : mov     esi, _BTOFF
						 movzx   ecx, word ptr[esi + N_NUMKEYS]
						 inc     ecx
						 movzx   eax, word ptr[esi + N_FSTLINK]
						 add     esi, eax; ESI, ECX = first link offset, count
						 mov     _trx_btscnData.ScnOffset, esi

						 //       ECX=count position of first record of proposed bound
						 //       ESI=offset of first record of proposed bound, or of
						 //          the record following the new key if ECX=_trx_btcnt
						 //       BP=link size
						 //       First, set (DI,DX) to (position,count) of last duplicate,
						 //       or to (SI,CX) if the upper bound is not duplicated.

					 _scn1:  mov     ebp, _BTSIZ; Use EBP to hold link size
							 mov     _trx_btscnData.CntBound, ecx
							 mov     edi, esi
							 mov     edx, ecx
							 dec     ecx
							 cmp     edx, _trx_btcnt
							 jnz     _scn1a
							 mov     ebx, _BTKEY
							 movzx   eax, byte ptr[ebx]
							 mov     _trx_btscnData.SizBoundKey, eax
							 jmp     short _scn2

							 _scn1a : sub     esi, ebp
									  xor		eax, eax
									  lodsw; Get AH : AL = prefix : suffix
									  movzx	ebx, ah
									  xor		ah, ah
									  add		bx, ax
									  mov     _trx_btscnData.SizBoundKey, ebx
									  add     esi, eax

									  //       (SI,CX) = (position,count) of next potential duplicate,
									  //                 CX=0 or CX=_trx_btcnt if there are none
									  //       (DI,DX) = (position,count) of current key or duplicate
									  //                 DX=_trx_btcnt if current key is new key
									  //		 msw of EAX is zero

								  _scn2:  mov     ebx, esi; ebx = offset of next link, if not a dup
										  or ecx, ecx
										  jz      _scn4
										  cmp     ecx, _trx_btcnt
										  jz      _scn4
										  sub     esi, ebp
										  lodsw; Get AH : AL = prefix : suffix
										  or al, al
										  jz     _scn3; al = 0 implies this is a duplicate

										  cmp     edx, _trx_btcnt
										  jnz     _scn4
										  cmp     trx_findResult, 0
										  jnz     _scn4
										  xor     ah, ah
										  add     esi, eax

										  //       This is a duplicate --
										  _scn3 : mov     edi, ebx
												  mov     edx, ecx
												  dec     ecx
												  //       (DI,DX) points to a new current candidate, which is a duplicate
												  //       (SI,CX) has been advanced to the next link past the duplicate --
												  jmp     short _scn2

												  _scn4 : mov     esi, _trx_btscnData.ScnOffset
														  mov     ecx, _trx_btscnData.CntBound

														  //       (DI,DX) = (position,count) of proposed upper bound
														  //       (SI,CX) = (position,count) of prefix portion
														  //       BX      = Position of next link

														  mov     _trx_btscnData.ScnOffset, ebx
														  mov     _trx_btscnData.CntBound, edx

														  //       Compute SizDupLead, SizDupLast, and SpcRemain --

														  cmp     ecx, edx
														  jz     _uniq

														  //       Compute SizDupLast and SizDupLead for duplicate key --
														  mov     edi, 2
														  sub     edi, ebp
														  mov     _trx_btscnData.SizDupLast, edi
														  mov     eax, ebx
														  sub     eax, esi
														  sub     ecx, _trx_btcnt
														  jz     _dup1
														  sub     eax, edi
														  inc     ecx
														  jnz     _dup1
														  //       The new key is left-adjacent to the upper bound prefix portion.
														  //       Correct for increase in matching prefix --
														  sub     eax, _trx_btsfx
														  _dup1 : mov     _trx_btscnData.SizDupLead, eax
																  jmp     short _srem

																  //       Compute SizDupLead and SizDupLast for unique key --
																  _uniq : xor     eax, eax
																		  mov     _trx_btscnData.SizDupLead, eax
																		  mov     ecx, _trx_btcnt
																		  cmp     edx, ecx
																		  jz     _scn5

																		  //       Upper bound is not the new key --
																		  mov     eax, ebx
																		  sub     eax, edi
																		  dec     ecx
																		  cmp     edx, ecx
																		  jnz     _scn6

																		  //       The new key is left-adjacent to the upper bound.
																		  //       Correct for increase in matching prefix --
																		  sub     eax, _trx_btsfx
																		  jmp     short _scn6

																		  //       Upper bound is the new key --
																		  //       Its size in the new node is _trx_btinc+_trx_btsfx
																		  _scn5 : mov     eax, _trx_btinc
																				  add     eax, _trx_btsfx
																				  _scn6 : mov     _trx_btscnData.SizDupLast, eax

																						  //       Compute SpcRemain --

																						  _srem : xor     eax, eax
																								  dec     edx
																								  jz     _scn8
																								  cmp     edx, _trx_btcnt
																								  jnz     _scn7

																								  //       New key will be first in old node --
																								  mov     esi, _BTKEY
																								  lodsb
																								  sub     eax, ebp
																								  sub     eax, _trx_btsfx; Correct for next key's prefix increase
																								  inc     eax
																								  inc     eax
																								  jmp     short _scn8

																								  //       New key is not the first in the old node --
																								  _scn7 : mov     esi, ebx
																										  sub     esi, ebp
																										  mov     al, [esi + 1]; AX = matching prefix length
																										  cmp     edx, _trx_btcnt
																										  jb     _scn8
																										  add     eax, _trx_btinc

																										  _scn8 : sub     ebx, eax
																												  mov     eax, 1
																												  sub     ebx, _BTOFF
																												  mov     _trx_btscnData.SpcRemain, ebx

																												  _scnex :
		pop		ebp
	}
}
/*
;--------------------------------------------------------------------
; apfcn_v _TRX_BTLOP(CSH_HDRP Org_node,ptr bound_key);
;
; If possible, without increasing either its length or its matching prefix
; (PFX) with the first key in Org_node, bound_key is revised to be the
; least valued key of length PFX+1, or PFX+2, etc., having a value no less
; than the original value, but strictly less than that of the node key.
;--------------------------------------------------------------------
*/
apfcn_v _trx_Btlop(CSH_HDRP Org_node, PBYTE Bound_key)
{
	__asm {
		mov     esi, Org_node
		mov     esi, [esi]
		movsx   eax, word ptr[esi + N_SIZLINK]
		movzx   ebx, word ptr[esi + N_FSTLINK]
		add		esi, ebx
		sub     esi, eax
		lodsw; AH:AL = pfx : sfx(AH = 0)
		mov     edi, Bound_key
		mov     ebx, edi
		movzx   ecx, byte ptr[edi]
		inc     edi

		// EBX = Addr of length byte of bound key
		// EDI = Addr of first byte of Bound_key. ECX=length.
		// ESI = Addr of first byte of first key in Org_node. AX=length.

		cmp     al, cl
		ja      _lop0
		xchg    cl, al
		_lop0 : jcxz    _lopx; eliminate special case (null key)
				repz    cmpsb
				jz      _lopx; if Bound_key is a prefix, leave it alone
				dec     esi; SI = first non - matching char of node key
				dec     edi; DI = first non - matching char of Bound_key
				mov     eax, edi
				sub     eax, ebx; AX = PFX + 1 (new potential length of Bound_key)
				cmp     al, byte ptr[ebx]
				jnb     _lopx; if new length not smaller than current length, exit
				mov     cl, byte ptr[edi]
				inc     cl
				cmp     cl, byte ptr[esi]
				jnz     _lop2

				// Length PFX+1 would increase the matching prefix length.
				// Although this would work if there are remaining suffix
				// bytes in the node key, we prefer to lengthen the bound key.
				// Let's try PFX+2, etc., being careful not to increment a
				// byte in the bound key to zero.

			_lop1:  inc     al
					cmp     al, byte ptr[ebx]; compare new length with original length
					jnb     _lopx
					inc     edi
					mov     cl, byte ptr[edi]
					inc     cl
					jz      _lop1; skip to next byte if current byte is FF

					_lop2 : xchg    edi, ebx
							stosb
							mov     byte ptr[ebx], cl
							_lopx :
	}
}

/*
;--------------------------------------------------------------------
; apfcn_i _TRX_BTCUT(CSH_HDRP new_node,ptr high_key,
;                    word breakpoint);
;
; Split a filled node (handle _BTNOD), removing the portion left of and
; including the key at count position breakpoint. Part (or all) of the
; left portion is moved to an empty node, new_node. The key sequence is
; understood to include the key from a previous failed _TRX_BTSTO attempt
; (_BTKEY and _BTLNK). This key is at position _trx_btcnt.
;
; high_key is initialized to be an upperbound key for the new_node. For
; branch nodes (level>0) the upperbound is the largest key removed from
; the original node. It is not stored in the new node, but the
; corresponding link is stored as N_RLINK. For leaf nodes (level=0), all
; keys removed from the filled node are inserted in the new node, the
; largest also being stored as high_key.
;
; If breakpoint==0, the new key (_BTKEY) is simply stored in the new
; node. Flag TRX_N_DupCutleft is set in the old node and flag
; TRX_N_DupCutRight is set in the new node.
;
; For example, _TRX_BTCUT is designed to be used with _TRX_BTSTO somewhat
; as follows:
;
; while(_TRX_BTSTO(old_node,new_key,new_link)) {
;   (allocate new_node)
;   (scan old_node to determine breakpoint)
;   if(_TRX_BTCUT(new_node,high_key,breakpoint)
;     (bad breakpoint or file is corrupt)
;   (fix node links)
;   (new_key=high_key, or _TRX_BTLOP() result if at leaf level)
;   (new_link=addr of new_node's corresponding link)
;   (if(old_node==root) old_node=new root; else old_node=parent)
; }
;--------------------------------------------------------------------
*/

apfcn_i _trx_Btcut(CSH_HDRP New_node, PBYTE High_key, UINT Breakpoint)
{
	__asm {
		mov     esi, _BTOFF
		mov     ecx, Breakpoint
		or ecx, ecx
		jnz     _btc2
		call    newhi
		jmp     _sto1

		newhi : mov     eax, New_node
				mov     _BTNOD, eax
				push    esi
				mov     esi, _BTKEY
				mov     edi, High_key
				lodsb
				stosb
				movzx   ecx, al
				shr     ecx, 1
				rep     movsw
				adc     ecx, ecx
				rep     movsb
				pop		esi
				ret

				_btc1 : call    newhi
						jmp		short _nxt3

						// CX=breakpoint --
						_btc2 : xor     edi, edi
								cmp     ecx, _trx_btcnt
								jbe     _btc3
								inc     edi; Update high key req if bkpnt > btcnt
								_btc3 : movzx   edx, word ptr[esi + N_NUMKEYS]; DX = current # keys(at least one!)
										inc     edx
										movzx   ebx, word ptr[esi + N_FSTLINK]
										add     esi, ebx; SI = first link offset

										// SI=current link offset of candidate upper bound.
										// DX=count of keys to right, including current key.
										// ES=DGROUP, DS=original node's segment
										// DI=0 only while high key updating is not required.

										// Advance SI to next link, updating high key as required --

									_nxt0:  mov     ebx, esi; Save current link's offset
											cmp     edx, _trx_btcnt
											jz     _btc1; Move new key to high key, set DI > 0 to
											; start update mode, and jmp to _nxt3.
											sub     esi, _BTSIZ
											xor		eax, eax
											lodsw
											or al, al; Suffix = 0 if duplicate
											jz      _nxt3
											or edi, edi
											jnz     _nxt1
											xor     ah, ah
											add     esi, eax
											jmp     short _nxt3

											// Update high key with current key's suffix.
											// Leave DI>0: Update mode stays in effect.
											_nxt1 : mov     edi, High_key
													movzx   ecx, ax; save CH : CL = prefix : suffix
													add     al, ah
													stosb; set high key's new length
													xor     ah, ah
													mov     al, ch; Get prefix length
													add     edi, eax; Skip matching prefix
													xor     ch, ch
													shr     ecx, 1
													rep     movsw
													adc     ecx, ecx
													rep     movsb

													_nxt3 : dec     edx; move *past* potential breakpoint
															cmp     edx, Breakpoint
															jnb      _nxt0

															mov     edi, _BTOFF; DI = address of node
															mov     eax, esi; AX = address of next link
															sub     esi, edi
															//		High word of ESI now must be 0
															xchg    si, [edi + N_FSTLINK]; revise relative offset in node
															add     esi, edi; SI = segment offset of first link
															cmp     edx, _trx_btcnt
															jb      _mov1; if final cnt < new key's position
															dec     edx; DX = # actual keys past split point
															_mov1 : mov     ecx, edx
																	xchg    dx, [edi + N_NUMKEYS]; revise key count in old node
																	sub     edx, ecx; DX = # keys moved to new node
																	mov     ecx, eax; Get offset of next link or end of node
																	test    byte ptr[edi + N_FLAGS], TRX_N_Branch
																	jz      _mov2
																	//***Bug fixed 11/18/2003 (DMcK) -- When Breakpoint==_trx_btcnt, the last scanned
																	//   key must be moved since the new key will become the high key. The bug's effect
																	//	 occurs only rarely, when the added key of a full branch node is to become
																	//	 the high (non-inserted) key of the newly-created node. In that case,
																	//	 what should be the new node's rightmost key and link doesn't get moved
																	//	 from the old node. That key and link is effectively discarded, thereby
																	//	 orphaning a branch of the tree and causing some find operations to fail.
																	cmp		eax, ebx; Same only if Breakpoint == _trx_btcnt
#ifdef _DEBUG
																	jnz		_dbg
																	jmp		_mov2; Put breakpoint here to test when old version fails
																	_dbg :
#else
																	jz		_mov2
#endif
																	//***
																	mov     ecx, ebx; Get offset of breakpoint position
																	dec     edx; high key not moved to branch

																	_mov2 : mov     ebx, New_node
																			mov     ebx, [ebx]

																			// Move one or more existing keys to the new node --
																			// ES:BX = Address of new node
																			// DS:DI = Address of original node.
																			// DS:SI = Adress of first link in original node.
																			// AX = Current offset of first link remaining in original node.
																			// CX = Offset of breakpoint link if branch, else CX=AX
																			// DX = Actual keys to be moved to the new node.

																			push    edi; Save original node's offset
																			sub     ecx, esi; CX = #bytes to move
																			jz      _mov3
																			mov[ebx + N_NUMKEYS], dx

																			// For an empty node, assume N_FSTLINK is the node's size --

																			sub[ebx + N_FSTLINK], cx
																			movzx   edi, word ptr[ebx + N_FSTLINK]
																			add     edi, ebx; EDI = addr of new node's first link
																			shr     ecx, 1
																			rep     movsw
																			adc     ecx, ecx
																			rep     movsb

																			_mov3 : cmp     esi, eax; SI == AX implies leaf or branch with
																					jz      _mov4; breakpoint key = new key

																					// This is a branch node with SI=offset of upper bound's link.
																					// Move this link to the new node's N_RLINK --

																					sub     cx, word ptr[ebx + N_SIZLINK]
																					mov     edi, ebx
																					add     edi, N_RLINK
																					rep     movsb; Move just 2, 3, or 4 bytes
																					mov     esi, eax; SI = remaining first link in old node

																					// We are finished with the new node.
																					// Revise prefix of first key in old node if necessary.
																					// SI points to first remaining link --

																				_mov4:  pop     ebx; Restore BX = original node's offset
																						cmp     word ptr[ebx + N_NUMKEYS], cx; CX == 0
																						jz      _sto0; if no keys remaining
																						sub     cx, word ptr[ebx + N_SIZLINK]; CX = link size or record size
																						mov     edi, esi; DI = current position of link
																						add     esi, ecx
																						lodsw; AH:AL = prefix : suffix
																						xor edx, edx
																						or dl, ah; DX = prefix = shift distance
																						jz      _sto0; If no matching prefix, no adjustment needed

																						// We must restore a prefix of length DX to the remaining first key.
																						// First, shift link of first key DX bytes left--

																						mov     esi, edi; SI = Old link position
																						sub     edi, edx; DI = New link position
																						sub     word ptr[ebx + N_FSTLINK], dx
																						shr     ecx, 1
																						rep     movsw; move link or record
																						adc     ecx, ecx
																						rep     movsb
																						xchg    cl, ah; CX = original prefix size
																						add     al, cl
																						stosw; Set AH : AL = prefix : suffix
																						mov     esi, High_key
																						inc     esi
																						rep     movsb; Store matching prefix(usually short)

																						// The left portion of the key sequence has been moved.
																						// If required, store the new key in the appropriate node.
																						// DS:BX = address of original node.

																					_sto0:  mov     eax, Breakpoint
																							test    byte ptr[ebx + N_FLAGS], TRX_N_Branch
																							jz      _sto2

																							; This is a branch node.Do not store new key if it is
																							; the new node's upperbound key, in which case we only need
																							; to move its link to N_RLINK --

																							sub     eax, _trx_btcnt
																							jnz     _sto2
																							mov     edi, New_node
																							mov     edi, [edi]
																							add     edi, N_RLINK
																							mov     esi, _BTLNK
																							movsw
																							movsw
																							jmp		short _stret

																							; Set appropriate leaf flags if Breakpoint = 0.
																							; DS:SI = address of original node

																							_sto1 : or byte ptr[esi + N_FLAGS], TRX_N_DupCutLeft
																									mov     edi, New_node
																									mov		edi, [edi]
																									or byte ptr[edi + N_FLAGS], TRX_N_DupCutRight
																									_sto2 :
		push    _BTLNK
			push    _BTKEY
			push    _BTNOD
			call	_trx_Btsto
			_stret :
	}
}

/*
;--------------------------------------------------------------------
; int pascal _TRX_BTSTO(CSH_HDRP hp,char far *A,void far *L)
;
; Inserts record L with key A into node with buffer handle hp.
;
; Returns JZ, AX=0 if successful.
; Otherwise, returns JB, AX=attempted data size increase (_BTINC).
; The following global words are set in either case:
;
;    trx_findResult     - Result of bt_srch() -- 0 iff exact match.
;    _trx_btcnt         - Position of new key in total sequence (>0).
;
; In case of failure, the following additional variables are
; initialized for use by _TRX_BTSCN() and _TRX_BTCUT():
;
;    _trx_btsfx         - Additional matching prefix chars in next key.
;    _BTLNK             - Far pointer to new record (or link).
;    _BTKEY             - Far pointer to new key.
;    _BTNOD             - Node's handle.
;    _BTINC             - Additional space that the new data would have
;                         taken if the node had been large enough.
;--------------------------------------------------------------------
*/

apfcn_i _trx_Btsto(CSH_HDRP Old_node, PBYTE New_key, PVOID New_link)
{
	__asm {
		mov     edi, New_key
		mov     esi, Old_node
		call    bt_srch; EDX = count position of matched key
		; EDI = neg link size, EBX = pfx:sfx offset
		inc     edx
		mov     _trx_btcnt, edx; Save position of new key

		//////mov     edi,eax                 ;DI=Find result needed by _trx_Btscn
		//*** Added 2/24/05 -- see below
		mov     trx_findResult, eax; Useful to _trx_Btscn

		// Calculate AX = space required for insertion --

		push    ebp; Save local BP
		movsx   ebp, word ptr[esi + N_SIZLINK]; BP = negative link size
		add     ebx, ebp; BX = seg offset of matched key's link
		xor     ah, ah; AX = length of unmatching target suffix
		inc     eax; add 2 bytes for key suffix / prefix
		inc     eax
		sub     eax, ebp; AX = space needed
		mov     edx, N_BUFFER; get key buffer relative offset
		add     edx, eax; DX = lower bound for first link pos
		cmp     dx, [esi + N_FSTLINK]; comp with current first link offset
		ja      _ex0; Fail if not enough space

		inc     word ptr[esi + N_NUMKEYS]; revise key count
		sub[esi + N_FSTLINK], ax; Revise first link's node offset
		movzx	edi, word ptr[esi + N_FSTLINK]
		add     esi, edi
		mov     edi, esi; DI = new seg offset of fst link
		add     esi, eax; SI = original seg offset of fst link
		mov     eax, ecx; AX = matching chars in matched key
		mov     ecx, ebx; BX = seg offset of matched key's link
		sub     ecx, esi; CX = bytes to shift left
		jz      _shf1

		//       Insertion required - shift all lower keys left

		shr     ecx, 1
		rep     movsw
		adc     ecx, ecx
		rep     movsb

		//       DI now points to new link's position. BP=negative link size.
		//       SP points to local BP.
		//       If necessary shift the next higher key's link and prefix word
		//       to right, incrementing its prefix and decrementing its suffix --

	_shf1:  mov     edx, edi; Save new key's link position
			or al, al; AX = matching chars = shift distance
			jz      _shf2
			sub     ecx, ebp; CX = link size
			mov     esi, ebx
			add     esi, ecx; SI = old position of next prefix
			add     ebx, eax; BX = new position of next link
			mov     edi, ebx
			add     edi, ecx; DI = new position of next prefix
			sub     ah, al; AH:AL = # to decrement prefix : suffix
			sub     word ptr[esi], ax; dec suffix and inc prefix

			std
			inc     ecx
			inc     ecx
			shr     ecx, 1
			rep     movsw
			adc     ecx, ecx
			//*** BUG found 10/28/04: The following 2 instructions must be inserted for std moves!!
			inc		si
			inc		di
			//***
			rep     movsb; move link : prefix:suffix
			mov     edi, edx; restore DI = new key's link position
			cld

			//       Insert new link and key --
			_shf2 : sub     ecx, ebp; Set CX = link or record size
					pop     ebp; Restore local BP
					mov     esi, New_link
					shr     ecx, 1
					rep     movsw
					adc     ecx, ecx
					rep     movsb; Insert link or record
					mov     esi, New_key
					mov     ecx, ebx; Get next link's position
					sub     ecx, edi
					dec     ecx
					dec     ecx; CX = new key's suffix length
					xor		eax, eax
					lodsb; AX = new key's total length
					add     esi, eax
					sub     esi, ecx; SI = position of suffix
					mov     ah, al
					sub     ah, cl
					mov     al, cl
					stosw; Store prefix : suffix
					shr     ecx, 1
					rep     movsw
					adc     ecx, ecx
					rep     movsb; Store key's suffix portion
					xor     eax, eax
					jmp		short _shfx

					;       Node is full--
					;       Data useful to _TRX_BTSCN() and _TRX_BTCUT() when insertion fails : -
					_ex0 : mov     _BTSIZ, ebp; Save negative link size
						   pop		ebp; Restore local BP
						   xor     edx, edx
						   mov     _trx_btscnData.CntBound, edx; Initialize for _trx_Btscn()
						   mov     _BTOFF, esi
						   mov     _trx_btinc, eax; Save attempted increase in data
						   mov     _trx_btsfx, ecx; Added prefix chars in next key

						   ;		//*** Commented out 2/24/06 -- trx_findResult already initialized above
		;       //////mov     trx_findResult,edi       ;Useful to _trx_Btscn

		mov		ecx, New_link
			mov		_BTLNK, ecx
			mov		ecx, New_key
			mov		_BTKEY, ecx
			mov		ecx, Old_node
			mov		_BTNOD, ecx
			_shfx :
	}
}

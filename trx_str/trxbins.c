/*  TRXBINS.C - 32-bit Routines for Binary Search, Insertion, and Deletion
;
;  NOTE: This version works with an 4-byte index (DWORD).
;-------------------------------------------------------------------------
;  void trx_Bininit(DWORD *seqbuf,DWORD *seqlen,int (*compfcn)(DWORD));
;
;  Initialize for binary lookup, insert, and delete. (See other functions
;  below.) seqbuf[] is an int (or pointer) array of *seqlen elements.
;  The compare function, compfcn(seqbuf[i]), returns <0, 0, or >0 depending
;  on whether some target element it knows of is <, ==, or > the element
;  identified by seqbuf[i]. Function bininit() simply saves its parameters
;  for subsequent use by blookup(), binsert(), and bdelete().
;
;  For example, when preparing to create seqbuf[] with a sequence of
;  binsert() calls, function bininit() must be called AND *seqlen must be
;  initialized to zero. Then, before each binsert() call, it is necessary
;  to set up the target element (normally a global variable) for access
;  by compfcn().
;
;  The following code illustrates a simple case. Function sort_array()
;  creates a sorted list of pointers, index[], to integers in
;  array[]. Here, pointers to duplicate integers are not inserted.
;  ////////////////////////////////////////////////////////////////////////
;   static int target;
;
;   static int compfcn(int *i)
;   {
;     return(target-*i);
;   }
;
;   void sort_array(int *array,int *index,int *index_len,int array_len)
;   {
;     bininit(index,index_len,compfcn);
;     while(array_len--) {
;       target=*array;
;	    binsert(array++,TRX_DUP_NONE);
;     }
;   }
;
;-------------------------------------------------------------------------
;  DWORD * trx_Blookup(int dupflag);
;
;  Returns a pointer p to an element of seqbuf[] such that compfcn(*p)
;  is zero, or returns NULL if there are no such elements. If there is
;  more than one, dupflag controls which element is selected (see below).
;  A previous call to bininit() is required to specify the base position
;  of seqbuf[], its length, and the compare function, compfcn();
;
;  Handling of duplicates (compfcn()==0) is controlled by dupflag:
;
;  TRX_DUP_NONE       - Find any match in seqbuf array
;  TRX_DUP_IGNORE     - Same effect as TRX_DUP_NONE
;  TRX_DUP_FIRST      - Find first match in seqbuf array
;  TRX_DUP_LAST       - Find last match in seqbuf array
;-------------------------------------------------------------------------
;  DWORD * trx_Binsert(DWORD seqitem,int dupflag);
;
;  Insert the integer, seqitem, in the sorted array seqbuf[] using
;  the compare function, compfcn(), specified with a previous call
;  to bininit(). Although seqitem (usually a pointer) is presumed to
;  identify a target known to compfcn(), binsert() itself makes no use
;  of its value, but simply finds the appropriate position in seqbuf[]
;  and inserts the integer while shifting existing elements down if
;  necessary. The array length *seqlen is also incremented. The function
;  returns a pointer to the inserted element in seqbuf.
;
;  No insertion occurs in just one circumstance: if an exact match is
;  detected by compfcn(seqbuf[i])==0 (indicating an existing duplicate)
;  while dupflag=TRX_DUP_NONE. In this case, a pointer to the element
;  seqbuf[i] where the match occurred is returned, but *seqlen is not
;  incremented. A global flag, trx_binMatch, is set TRUE (1) when
;  this happens, otherwise it is set FALSE (0) to indicate an insertion.
;
;  Handling of duplicates (compfcn()==0) is controlled by dupflag:
;
;  TRX_DUP_NONE       - Do NOT insert duplicate
;  TRX_DUP_IGNORE     - Insert duplicate, order within set does not matter
;  TRX_DUP_FIRST      - Insert duplicate at beginning of existing set
;  TRX_DUP_LAST       - Insert duplicate at end of set (preserve order)
;-------------------------------------------------------------------------
;  void trx_Bdelete(DWORD *seqitem);
;
;  Delete integer, *seqitem, from seqbuf[], shifting remaining items
;  to close gap if necessary. Array length, *seqlen, is also decremented.
;  No lookup occurs since seqitem points to the element of seqbuf[] to
;  be deleted. This pointer is usually the return value of a previous call
;  to blookup() or binsert(). See also bininit().
;-------------------------------------------------------------------------
*/

#include <trx_str.h>

#pragma warning(disable:4035)

int trx_binMatch;

static TRXFCN_IF bincompfcn;
static DWORD *binseqbuf;
static DWORD *binseqlen;
static int bindupflag;

void _BTFIND(void);

TRXFCN_V trx_Bininit(DWORD *seqbuf,DWORD *seqlen,TRXFCN_IF seqfcn)
{
	__asm {
		mov		eax,seqfcn
		mov		bincompfcn,eax
		mov		eax,seqlen
		mov		binseqlen,eax
		mov		eax,seqbuf
		mov		binseqbuf,eax
	}
}

TRXFCN_V trx_Bdelete(DWORD *seqitem)
{
	__asm {
		mov		eax,seqitem
		mov		ebx,binseqlen
		mov		ecx,[ebx]
		sub		ecx,1
		jb		_dret
		mov		[ebx],ecx
		shl		ecx,2
		add		ecx,binseqbuf
		sub		ecx,eax
		jna		_dret

		mov		edi,eax
		mov		esi,eax
		add		esi,4
		shr		ecx,2
		rep		movsd
_dret:
	}
}

TRXFCN_DWP trx_Blookup(int dupflag)
{
	__asm {
        mov     esi,dupflag		;force saving of esi and edi in prolog
        mov     bindupflag,esi
		xor		edi,edi
        mov		trx_binMatch,edi
		call	_BTFIND
		jnz		_ex1
		xor		eax,eax
		jmp		short _ex0

_ex1:	cmp		bindupflag,TRX_DUP_LAST
		jnz		_ex0
		sub		eax,4			;position at last duplicate
_ex0:
	}
}

TRXFCN_DWP trx_Binsert(DWORD seqitem,int dupflag)
{
	__asm {
        mov     eax,dupflag
        mov     bindupflag,eax
		mov		trx_binMatch,0
        call    _BTFIND
		jz		_ins1
		cmp		bindupflag,TRX_DUP_NONE
		jz		_lookex

_ins1:	mov		edx,seqitem
		mov		edi,binseqlen
		mov		ecx,[edi]
		inc		dword ptr [edi]
		mov		edi,ecx
		shl		edi,2
		add		edi,binseqbuf
		cmp		eax,edi
		jnb		_ins2

		mov		esi,edi
		sub		esi,4
		mov		ecx,edi
		sub		ecx,eax
		shr		ecx,2 
		std	
		rep		movsd	
		cld	

	_ins2:
		mov		[edi],edx

_lookex:
	}
}

/*----------------------------------------------------------
;Local search routine used by BINSERT and BLOOKUP --
;Returns EAX = pointer to insertion position, leaves trx_Binmatch=0
;and sets Z-flag iff exact match is NOT found. Assumes esi and edi preserved!
*/

__declspec(naked) void _BTFIND(void)
{
	__asm {
		push	ebx
		mov		edi,binseqlen
		mov		edi,[edi]
		mov		esi,binseqbuf
		shl		edi,2			;esi=min points to seqbuf
		add		edi,esi			;edi=max points to seqbuf+seqlen
_cut:
		cmp	esi,edi
		jnb	_sexit

		mov		ebx,edi			;if (di=max)>(si=min)
		sub		ebx,esi
		shr		ebx,1
		and		bl,0FCh			;multiple of 4
		add		ebx,esi			;bx=cut=(max+min)/2

		push	ebx
		push	dword ptr [ebx]			
		call    bincompfcn		;ax=compfcn(seqbuf[cut])
		pop     ebx
		or		eax,eax
		js		_back
		jz		_match

_forw:	mov		esi,ebx
		add		esi,4			;if(target>=&seqbuf[cut])
		jmp		short _cut		;min=cut+1

_back:	mov		edi,ebx			;if(target<&seqbuf[cut])
		jmp		short _cut		;max=cut

_match: mov     trx_binMatch,1  ;an exact match is detected
		cmp		bindupflag,TRX_DUP_FIRST
		ja		_forw
		jz		_back
		mov		esi,ebx

_sexit: cmp     trx_binMatch,0
		mov		eax,esi
		pop		ebx
		ret
	}
}

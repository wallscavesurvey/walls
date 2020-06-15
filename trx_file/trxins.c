/* TRXINS.C - Insert key into TRX_FILE index -- */

#include "a__trx.h"

#undef _DBUG

#ifdef _DEBUG
#undef _DBUG
#endif

#if defined(_DBUG)
#include <stdio.h>
#include <trx_str.h>
#endif

#ifdef _SCN_TEST
UINT _trx_oldSpace, _trx_newSpace;
#endif

UINT _trx_btsfx;
UINT _trx_btinc;

// Global data referenced by _TRX_BreakPoint() and _TRX_BTSCN() --
typ_scnData _trx_btscnData;

/*Globals (in TRXPVAR.C) used to adjust user tree positions --*/
extern HNode _trx_p_RevLeaf; /*Original target node*/
extern HNode _trx_p_NewLeaf; /*New leaf node if target is split*/
extern UINT _trx_p_RevKeys;  /*Keys remaining in target node if split*/
extern UINT _trx_p_OldKeys;  /*Keys originally in target node*/
extern UINT _trx_p_KeyPos;   /*Position of new key in complete sequence*/

static void _trx_AddAdjust(void)
{
	TRX_pFILEUSR usrp;
	TRX_pTREEUSR tusr;
	UINT p;

	/*The new position for the calling process is at the inserted key.
	  All other user positions are shifted, if necessary, so that they
	  still point to the same key.*/

	  /***** NOTE: Bug fix 7/10/95 --
			if(_trx_p_RevKeys)... changed to
			if(_trx_p_RevKeys && _trx_p_KeyPos>_trx_p_RevKeys)...
	  */

	for (usrp = _trx_pFile->pFileUsr; usrp; usrp = usrp->pNextFileUsr)
		if (usrp == _trx_pFileUsr) {
			tusr = _trx_pTreeUsr;
			if (_trx_p_RevKeys && _trx_p_KeyPos > _trx_p_RevKeys) {
				tusr->KeyPos = _trx_p_KeyPos - _trx_p_RevKeys;
				tusr->NodePos = _trx_p_NewLeaf;
			}
			else {
				tusr->KeyPos = _trx_p_KeyPos;
				tusr->NodePos = _trx_p_RevLeaf;
			}
		}
		else {
			tusr = (TRX_pTREEUSR)&usrp->TreeUsr[_trx_tree];
			if ((p = tusr->KeyPos&KEYPOS_MASK) != 0 && tusr->NodePos == _trx_p_RevLeaf) {
				if (p == KEYPOS_MASK) p = _trx_p_OldKeys;
				if (p >= _trx_p_KeyPos) p++;
				if (p > _trx_p_RevKeys) {
					p -= _trx_p_RevKeys;
					tusr->NodePos = _trx_p_NewLeaf;
				}
				tusr->KeyPos = p | (tusr->KeyPos&KEYPOS_HIBIT);
			}
		}
}

#ifdef _DBUG
static int TRXAPI _trx_BreakPoint(CSH_HDRP hp, int level, UINT flags)
#else
static int TRXAPI _trx_BreakPoint(int level, UINT flags)
#endif
{
	/*Scan the existing node to determine the optimal count position for
	the new node's upperbound key in the complete sequence containing the
	new key. (Position 1 is the rightmost key position.) Return 0 only if
	the new key matches duplicates already filling an existing leaf. If no
	suitable breakpoint is found, -1 is returned (impossible unless the
	tree is corrupted).

	When not compacting, the value returned for leaves is the position
	that maximizes the minimum space remaining in the pair of nodes
	*after* the final insertion of the new key. For branches, especially,
	we might later want to revise this criterion by also considering the
	size of the upperbound key. This key will migrate to a higher tree
	level where prefix compression is less effective. Without further
	tests, it is difficult to know how to balance this effect against the
	need to equalize space remaining in the nodes at the current level.

	The operation of this function was originally part of _trx_Btcut, an
	assembly language (ASM) function that was becoming too complex. For
	clarity and ease if revision, the logic of selecting the breakpoint,
	which depends on branch/leaf status and compaction flags, was moved
	here. A separate ASM function, _trx_Btscn, parses the node's key
	sequence. Like _trx_Btcut, it has access to static data, initialized
	by _trx_Btsto, about the new key, its associated record (or link), and
	its position in the sequence.

	NOTE: This function must be called only after a failed _trx_Btsto()
	invocation. The latter function will have initialized global variables
	used by the sequence of _trx_Btscn calls below.

	The following global structure is referenced by this function and
	_trx_Btscn. It contains pertinent data for a possible upper bound key
	in the sequence being scanned. _trx_Btscn considers only unique keys
	and the last duplicate key of a set as possible upper bound
	breakpoints. Note that SizBoundKey is made available by _trx_Btscn for
	possible future use.*/

	int dmin;
	int newleaf_spc;
	int maxmin_spc = (int)KEYPOS_HIBIT;
	UINT maxmin_cnt = 0;

#ifdef _DBUG
	byte keybuf[256];

	if (level) {
		printf("\nCntB:  ---  Siz: ___  NewSp:%5u  OldSp:%5u    SFX:%4u",
			_trx_pFile->Fd.SizNode,
			_Node(hp)->N_FstLink - _trx_btinc,
			_trx_btsfx);
	}
#endif

	/*During the scan we will consider the node header part of the
	space remaining --*/
	newleaf_spc = _trx_pFile->Fd.SizNode;

	while (_trx_Btscn()) {

		/*The last position (1) is returned on the first pass only if the
		new key matches a duplicate key set already filling the node --*/

		if (_trx_btscnData.CntBound == 1) {
			if (level) break;
			if (maxmin_cnt) goto _rtn;
			return 0;
		}

		newleaf_spc -= _trx_btscnData.SizDupLead;
		if (!level) newleaf_spc -= _trx_btscnData.SizDupLast;

#ifdef _DBUG
		if (level) {
			dmin = _trx_btscnData.CntBound;
			if (dmin == (int)_trx_btcnt) strcpy(keybuf, "<new key>");
			else {
				_trx_Btkey(hp, dmin > (int)_trx_btcnt ? dmin - 1 : dmin, (ptr)keybuf, 256);
				trx_Stxpc(keybuf);
			}
			printf("\n%5u  Sz:%4u  NewS:%5u  OldS:%5u  NxtO:%5u  KeySz:%4u %s",
				_trx_btscnData.CntBound,
				_trx_btscnData.SizDupLast + _trx_btscnData.SizDupLead,
				newleaf_spc,
				_trx_btscnData.SpcRemain,
				_trx_btscnData.ScnOffset,
				_trx_btscnData.SizBoundKey,
				keybuf);
		}
#endif

		dmin = _trx_btscnData.SpcRemain;
		if (dmin > newleaf_spc) dmin = newleaf_spc;

		/*dmin is the minimum of the space remaining in the two nodes if
		this bound is chosen. As soon as it is no larger than the previous
		minimum, we stop the scan and accept the previous bound.*/

		if (dmin <= maxmin_spc) goto _rtn; /*impossible on first pass*/

		/*Otherwise, we can accept this bound if the old node has as much
		space available as the new node, or if we are compacting and have
		already advanced past the new key. The latter's position,
		_trx_btcnt, was saved by _trx_Btsto.*/

#ifdef _SCN_TEST
		_trx_oldSpace = _trx_btscnData.SpcRemain;
		_trx_newSpace = newleaf_spc;
#endif
		maxmin_spc = dmin;
		maxmin_cnt = _trx_btscnData.CntBound;
		if (dmin == newleaf_spc || maxmin_cnt == 2 ||
			((flags&TRX_KeyDescend) && maxmin_cnt <= _trx_btcnt)) goto _rtn;

		if (level) {
			newleaf_spc -= _trx_btscnData.SizDupLast;
		}
	}
	return -1;

_rtn:
	return maxmin_spc < sizeof(TRX_NODE) ? -1 : maxmin_cnt;
}

int TRXAPI trx_InsertKey(TRX_NO trx, vptr pRec, ptr pKey)
{
	/*Inserts a new record/key structure into the tree.*/

	/*NOTE: This function assumes an LRU cache design that guarantees at
	least two available buffers. In an environment where the request of a
	second buffer might cause the first buffer to be flushed and reused,
	we would need to revise the code to apply buffer locks whenever two
	nodes are accessed concurrently.*/

	enum e_typ {
		TYP_Ins, TYP_DelOrgLink_Ins, TYP_RepWithRlink_Ins,
		TYP_RepWithNewLink, TYP_RepWithNewKey
	};

	CSH_HDRP hp, hpnew;
	int level, branch_levels, bound_branch, breakpoint, lvl1typ = 0;
	UINT flags;
	BOOL bound_match;
	HNode lvlnod[TRX_MAX_BRANCH_LEVELS], newlink, rlink, llink;
	UINT lvlpos[TRX_MAX_BRANCH_LEVELS];
	byte keybuffer[2][256];

	if (!_GETTRXT || _trx_ErrReadOnly()) return trx_errno;

	if (_trx_pFile->Cf.Errno) return trx_errno = _trx_pFile->Cf.Errno;

	if (_TRXT->SizKey == 0)
		return trx_errno = _TRXT->SizRec ? TRX_ErrTreeType : TRX_ErrNoInit;

	if (*pKey > _TRXT->SizKey) return trx_errno = TRX_ErrSizKey;

	flags = (_trx_pTreeUsr->UsrFlags | _trx_pTreeVar->InitFlags);

	/*_trx_p_RevKeys will be set nonzero when a node is split --*/
	branch_levels = _trx_p_RevKeys = 0;
	bound_match = bound_branch = level = 0;

	if (_TRXT->Root == 0L) {
		/*Make leaf level node --*/
		if (_trx_IncPreWrite(1) || (hp = _trx_NewNode(0)) == 0) return trx_errno;
		_TRXT->NumKeys = 0L;
		_TRXT->Root = _TRXT->HighLeaf = _TRXT->LowLeaf = _trx_p_RevLeaf = _Node(hp)->N_This;
		branch_levels--;
	}
	else {
		/*Descend to leaf level --*/

		_trx_p_RevLeaf = _TRXT->Root;
		while (TRUE) {
			if ((hp = _trx_GetNode(_trx_p_RevLeaf)) == 0) return trx_errno;
			if ((lvl1typ = (_Node(hp)->N_Flags&TRX_N_Branch)) == 0) break;
			if (branch_levels == TRX_MAX_BRANCH_LEVELS) return TRX_ErrLevel;

			lvlnod[branch_levels] = _trx_p_RevLeaf;
			_trx_p_RevLeaf = _trx_Btlnk(hp, pKey);

			/*_trx_Btlnk sets _trx_btcnt and trx_findResult --*/
			if ((lvlpos[branch_levels++] = _trx_btcnt) > 0) {
				bound_branch = branch_levels;
				bound_match = trx_findResult == 0;
				level = 0;
			}
			else level = lvl1typ;
		}

		/*Consider the special case where a leaf is flagged as a placeholder
		node (flag N_EmptyRight set) for an unused upper bound. In this
		case, the tree level of the last branch node encountered will be
		level>0. We will complete the insertion by allocating a new leaf
		for the key and also, to maintain a uniform tree height, level-1
		new branch nodes leading to it.*/

		if (_Node(hp)->N_Flags&TRX_N_EmptyRight) {
			if (!level) return FORMAT_ERROR();
			if (_trx_IncPreWrite(level) || (hpnew = _trx_NewNode(0)) == 0)
				return trx_errno;
			if (_trx_Btsto(hpnew, pKey, pRec)) return FORMAT_ERROR();
			_Node(hpnew)->N_Llink = _trx_p_RevLeaf;
			_trx_p_RevLeaf = _Node(hpnew)->N_This;
			_Node(hp)->N_Flags &= ~TRX_N_EmptyRight;
			rlink = _Node(hp)->N_Rlink;
			_Node(hp)->N_Rlink = _trx_p_RevLeaf;
			_csh_MarkHdr(hp);

			if (!rlink) _TRXT->HighLeaf = _trx_p_RevLeaf;
			else {
				if ((hp = _trx_GetNode(_Node(hpnew)->N_Rlink = rlink)) == 0) return trx_errno;
				/*Check double linkage of leaves --*/
				_AVOID(_Node(hp)->N_Llink != _Node(hpnew)->N_Llink);
				_Node(hp)->N_Llink = _trx_p_RevLeaf;
				_csh_MarkHdr(hp);
			}
			rlink = _trx_p_RevLeaf;
			for (lvl1typ = 1; lvl1typ < level; lvl1typ++) {
				if ((hp = _trx_NewNode(lvl1typ)) == 0) return trx_errno;
				_Node(hp)->N_Rlink = rlink;
				rlink = _Node(hp)->N_This;
			}
			if ((hp = _trx_GetNode(lvlnod[branch_levels - 1])) == 0) return trx_errno;
			_Node(hp)->N_Rlink = rlink;
			_csh_MarkHdr(hp);
			_trx_p_KeyPos = 1;
			goto _IncKeys;
		}
	}

	/*branch_levels = -1 (new root), 0 (old root is leaf), >0 (tree levels-1)*/

	/*Should a duplicate key be rejected? If so, the user's new tree position
	  will be set at the matched key --*/

	_trx_p_OldKeys = _Node(hp)->N_NumKeys;

	if (_trx_p_OldKeys && (flags&TRX_KeyUnique)) {
		if (_trx_Btfnd(hp, pKey) == 0 && bound_match &&
			(_Node(hp)->N_Flags&TRX_N_DupCutRight) != 0) {
			/*This is possible, but *very* unlikely --*/
			_trx_btcnt = KEYPOS_MASK;
			_trx_p_RevLeaf = _Node(hp)->N_Rlink;
			trx_findResult = 0;
		}
		if (!trx_findResult) {
			_trx_pTreeUsr->NodePos = _trx_p_RevLeaf;
			_trx_pTreeUsr->KeyPos = _trx_btcnt;
			return trx_errno = TRX_ErrDup;
		}
	}

	level = 0;

	while (_trx_Btsto(hp, pKey, pRec)) {

		/*Abort if this is a new root - should not happen!*/
		if (level > branch_levels) return FORMAT_ERROR();

		/*Determine the position of the new node's upperbound key in the
		complete sequence containing the new key. breakpoint==0 only if the
		new key matches duplicates already filling an existing leaf.*/

#ifdef _DBUG
		if (level) {
			pKey[*pKey + 1] = 0;
			printf("\nOld spc:%5u _btinc:%4u Overflow key: |%s|",
				_Node(hp)->N_FstLink, _trx_btinc, (ptr)pKey + 1);
		}
		if ((breakpoint = _trx_BreakPoint(hp, level, flags)) == -1)
#else
		if ((breakpoint = _trx_BreakPoint(level, flags)) == -1)
#endif
			return FORMAT_ERROR();

		/*If at leaf level and MinPreWrite>0, we preallocate space for the
		new leaf plus the (branch_levels+1) additional nodes that will be
		needed if this operation leads to increasing the tree height by one.
		No additional nodes are needed if the new leaf is to inherit an
		existing upper bound, with the old leaf becoming an overflow node.*/

		if (!level) {

			/*Save position of new key in sequence for user-positioning
			purposes. Note that _trx_Btsto() set _trx_btcnt --*/
			_trx_p_KeyPos = _trx_btcnt;

#ifdef _SCN_TEST
			if (!breakpoint) {
				_trx_oldSpace = _Node(hp)->N_FstLink;
				_trx_newSpace = _trx_pFile->Fd.SizNode - _trx_btinc - _trx_btsfx;
			}
#endif
			if (_trx_IncPreWrite((bound_match && !breakpoint) ? 1 : branch_levels + 2))
				return trx_errno;
		}

		/*Allocate a new node for the split --*/
		if ((hpnew = _trx_NewNode(level)) == 0) return trx_errno;

		/*Insert the key while moving a specified part of the resulting key
		sequence to the new node. _trx_Btcut() also reinitializes pKey with
		the new node's upperbound key. If the original node is being made
		an overflow node (indicated by breakpoint==0), _trx_Btcut simply
		stores the new key as upperbound in the new node, setting flags
		TRX_N_DupCutLeft in the original node and TRX_N_DupCutRight in the
		new node.*/

		if (_trx_Btcut(hpnew, pKey = keybuffer[level & 1], breakpoint))
			return FORMAT_ERROR();

#ifdef _SCN_TEST
#ifdef _DBUG
		if (level) {
			printf("\nTotal Keys before split: %lu\n", _TRXT->NumKeys);
			printf("Node:%5lu  _trx_newSpace:%6u  Actual:%6u  Rlink: %lu\n"
				"Node:%5lu  _trx_oldSpace:%6u  Actual:%6u\n",
				_Node(hpnew)->N_This, _trx_newSpace, _Node(hpnew)->N_FstLink,
				_Node(hpnew)->N_Rlink,
				_Node(hp)->N_This, _trx_oldSpace, _Node(hp)->N_FstLink);
		}
#endif
#endif

		_AVOID(_Node(hpnew)->N_FstLink != _trx_newSpace ||
			_Node(hp)->N_FstLink != _trx_oldSpace);

		_csh_MarkHdr(hp);
		newlink = _Node(hpnew)->N_This;

		if (!level) { /*If leaf level --*/
		  /*Insert new leaf in doubly linked list --*/
			_Node(hpnew)->N_Llink = llink = _Node(hp)->N_Llink;
			_Node(hpnew)->N_Rlink = _Node(hp)->N_This;
			_Node(hp)->N_Llink = newlink;

			/*Save data necessary for repositioning in new node --*/
			_trx_p_NewLeaf = newlink;
			_trx_p_RevKeys = _Node(hp)->N_NumKeys;

			/*Node number in variable newlink will accompany subsequent
			key insertions --*/
			pRec = &newlink;

			if (breakpoint) {
				/*If possible, reduce the length of the upper bound key --*/
				_trx_Btlop(hp, pKey);
				/*The upper bound will be inserted at level 1 --*/
				lvl1typ = TYP_Ins;
			}

			/*Take care of the only major complication: If breakpoint==0, the
			original leaf is already filled with duplicates of the new key.
			This requires the flagging of a new overflow node with appropriate
			revisions made to nodes at level 1 (indicated by lvl1typ) --*/

			else if (!bound_match) {
				/*A new upper bound is required --*/
				if (bound_branch) {
					/*The filled leaf is not the rightmost leaf --*/
					if (_Node(hp)->N_Flags&TRX_N_DupCutRight) {
						/*We must modify a right-adjacent overflow node --*/
						_Node(hp)->N_Flags &= ~TRX_N_DupCutRight;
						if ((hp = _trx_GetNode(rlink = _Node(hp)->N_Rlink)) == 0)
							return trx_errno;
						lvl1typ = (_Node(hp)->N_Flags&TRX_N_EmptyRight) ?
							TYP_DelOrgLink_Ins : TYP_RepWithRlink_Ins;
						_Node(hp)->N_Flags &= ~(TRX_N_DupCutLeft + TRX_N_EmptyRight);
						_csh_MarkHdr(hp);
					}
					else lvl1typ = TYP_RepWithNewKey;
				}
				else {
					/* Flag rightmost leaf as a placeholder --*/
					_Node(hp)->N_Flags |= TRX_N_EmptyRight;
					lvl1typ = TYP_Ins;
				}
			}
			else lvl1typ = TYP_RepWithNewLink;

			/*Fix N_Rlink of the next lower leaf. If there are no lower
			leaves, set tree's LowLeaf variable to the new node --*/

			if (llink) {
				if ((hp = _trx_GetNode(llink)) == 0) return trx_errno;
				_Node(hp)->N_Rlink = newlink;
				_csh_MarkHdr(hp);
			}
			else _TRXT->LowLeaf = newlink;
		}

		/*Now retrieve parent node for modification --*/
		if (level++ < branch_levels) {
			if ((hp = _trx_GetNode(lvlnod[branch_levels - level])) == 0) return trx_errno;

			if (lvl1typ > TYP_Ins) {
				breakpoint = lvlpos[branch_levels - level];

				/*Bug fixed 7/5/2001. _trx_Btsetc() does NOT replace N_Rlink. This
				might be required when lvl1typ==TYP_RepWithNewLink*/

				if (lvl1typ == TYP_RepWithNewLink) {
					/*Should always be the case that breakpoint>0! */
					if (breakpoint) _trx_Btsetc(hp, breakpoint, pRec);
					else _Node(hp)->N_Rlink = newlink;
					break;
				}

				if (lvl1typ == TYP_RepWithNewKey) {

					/*Bug fixed 7/19/2001. _trx_Btsetc() does NOT replace N_Rlink. This
					might be required when lvl1typ==TYP_RepWithNewKey*/
					if (breakpoint) _trx_Btsetc(hp, breakpoint, pRec);
					else _Node(hp)->N_Rlink = newlink;

					bound_branch = branch_levels - bound_branch + 1;
					if (level != bound_branch) {
						_csh_MarkHdr(hp);
						level = bound_branch;
						breakpoint = lvlpos[branch_levels - level];
						if ((hp = _trx_GetNode(lvlnod[branch_levels - level])) == 0)
							return trx_errno;
						newlink = lvlnod[branch_levels - level + 1];
					}
					_trx_Btdel(hp, breakpoint);
				}
				else {
					if (lvl1typ == TYP_DelOrgLink_Ins) _trx_Btdel(hp, breakpoint);
					else {/*lvl1typ==TYP_RepWithRlink_Ins */
						if (breakpoint) _trx_Btsetc(hp, breakpoint, &rlink);
						else _Node(hp)->N_Rlink = rlink;
					}
				}
				lvl1typ = TYP_Ins; /*disable above code for next pass*/
			}
		}
		else {
			/*Increase tree level - Allocate a new root --*/
			if ((hp = _trx_NewNode(level)) == 0) return trx_errno;
			_Node(hp)->N_Rlink = _TRXT->Root;
			_TRXT->Root = _Node(hp)->N_This;
		}
	}

	/*Set update flag of last modified node --*/
	_csh_MarkHdr(hp);

	/*Save new key's position when no split was required --*/
	if (!_trx_p_RevKeys) _trx_p_KeyPos = _trx_btcnt;

_IncKeys:

	++_TRXT->NumKeys;
	_trx_pFile->HdrChg = TRUE;

	/*A flush error at this point would result in a corrupted tree;
	however, MinPreWrite>0 would have caused TRX_ErrNoSpace to be returned
	earlier if we had simply run out of disk space. The optional flush of
	revised nodes after each key addition reduces the probabilty of file
	corruption due to an abrupt computer shutdown or power failure.*/

	/*Flush nodes if file is in TRX_AutoFlush mode --*/
	if (_trx_ErrFlush()) return trx_errno;

	_trx_AddAdjust();  /*Adjust positions of all tree users*/

	return 0;
}

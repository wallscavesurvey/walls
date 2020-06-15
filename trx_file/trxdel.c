/* TRXDEL.C - Last revised 03/07/93 (DMcK)*/
/*====================================================================
int TRXAPI trx_DeleteKey(TRX_NO trx);

This function deletes the key and associated record at the caller's
current tree position. If the deletion empties a leaf node, the node is
placed on the file's free node list. This in turn leads to the possible
freeing of branch nodes as the tree is restructured.

After a tree's last key is deleted, it will no longer occupy node space
in the file. The tree can then be reinitialized with trx_InitTree().

Effect on Tree Position
=======================
If successful, the caller's tree position and the positions of other
users which happen to be set at the deleted key will remain invalid
until the next positioning function. Until then, functions that depend
on a valid position will return TRX_ErrDeleted, except that the relative
positioning functions, trx_Next(), trx_Prev(), and trx_Skip(), can be
used to establish a new position with respect to an invalidated one.

For example, if the key at some user's tree position is deleted by
any process, a subsequent trx_GetKey() by that user will return
TRX_ErrDeleted, while trx_Next() will position the user in a reasonable
fashion -- that is, at the key that was next in sequence at the time of
deletion. (If there had been no next key, trx_Next() will return
TRX_ErrEOF.) In other words, when any user's position is invalidated by
a key deletion, the next key serves as a placeholder for the invalidated
position until the user successfully repositions at a later time. If the
placeholder key itself is deleted by another user before this happens,
the key following that one (if it exists) becomes the new placeholder,
and so forth.

The treatment of user positions after a key deletion insures that a
series of low-to-high leaf traversal functions will always encounter a
non-decreasing sequence of key values -- even in a multiuser setting
where both deletions and additions by other users are occurring between
function calls. This is NOT necessarily true for a high-to-low leaf
traversal with functions like trx_Prev().

Possible Errors
===============
A key deletion will never increase the size of the file. Therefore, if
the file is in a flushed state, no errors should occur with this
function unless the file is found to be corrupted (TRX_ErrFormat) or
incomplete (TRX_ErrRead), or unless the user's tree position was never
established (TRX_ErrEOF) or was invalidated by a prior deletion
(TRX_ErrDeleted).

If the file is in a non-flushed state, trx_DeleteKey() is like any other
function requiring node access. Cache buffer reuse can cause an
TRX_ErrWrite error to be returned upon failure to flush previously
revised nodes to disk. This is unlikely if the file's MinPreWrite value
is at least one (the default), or if the file's open mode is either
TRX_UserFlush or TRX_AutoFlush.
======================================================================*/

#include <a__trx.h>

/*Globals (in TRXPVAR.C) used to adjust user tree positions --*/
extern HNode _trx_p_RevLeaf; /*Leaf node of deleted key*/
extern UINT  _trx_p_RevKeys; /*User's KeyPos if leaf was deleted, else zero*/
extern HNode _trx_p_NewLeaf; /*User's NodePos if leaf was deleted, or for
							   users whose position 1 was deleted*/
extern UINT _trx_p_KeyPos;   /*Position of deleted key in original leaf*/

static apfcn_v _trx_DelAdjust(void)
{
	TRX_pFILEUSR usrp;
	TRX_pTREEUSR tusr;
	UINT p;

	for (usrp = _trx_pFile->pFileUsr; usrp; usrp = usrp->pNextFileUsr) {
		tusr = (TRX_pTREEUSR)&usrp->TreeUsr[_trx_tree];
		if ((p = tusr->KeyPos&KEYPOS_MASK) >= _trx_p_KeyPos &&
			tusr->NodePos == _trx_p_RevLeaf) {
			if (_trx_p_RevKeys) {
				/*The node at the user's position was deleted --*/
				tusr->NodePos = _trx_p_NewLeaf;
				tusr->KeyPos = _trx_p_RevKeys;
			}
			else {
				if (p == _trx_p_KeyPos) {
					if (--p == 0 && _trx_p_NewLeaf) {
						tusr->NodePos = _trx_p_NewLeaf;
						p = (KEYPOS_MASK | KEYPOS_HIBIT);
					}
					else p |= KEYPOS_HIBIT;
				}
				else {
					if (p != KEYPOS_MASK) p--;
					p |= (tusr->KeyPos&KEYPOS_HIBIT);
				}
				tusr->KeyPos = p;
			}
		}
	}
}

static apfcn_v _trx_FreeNode(CSH_HDRP hp)
{
	/*Attach node to head of file's free node list --*/

	_Node(hp)->N_Tree = 0xFF; /*Flag as free node*/
	_Node(hp)->N_Rlink = _trx_pFile->FirstFreeNode;
	_trx_pFile->FirstFreeNode = _Node(hp)->N_This;
	_csh_MarkHdr(hp); /*Flag node buffer as updated*/
	_trx_pFile->NumFreeNodes++;
	_trx_pTreeVar->NumNodes--;
}

/*=================================================================*/
int TRXAPI trx_DeleteKey(TRX_NO trx)
{
	CSH_HDRP hp;
	UINT w, branch_index, revleaf_flags, llink_flags, rlink_flags;
	UINT lvlpos[TRX_MAX_BRANCH_LEVELS];
	HNode lvlnod[TRX_MAX_BRANCH_LEVELS];
	HNode link, rlink_node, llink_node;
	int rev_placeholder;
	BYTE keybuffer[256];

	if ((hp = _trx_GetNodePos(trx, 0)) == 0 || _trx_ErrReadOnly()) return trx_errno;
	trx = (TRX_NO)_trx_pTreeVar; /*Reuse trx*/
	if (_TRXT->SizKey == 0) return trx_errno = TRX_ErrTreeType;

	_trx_p_RevLeaf = _trx_pTreeUsr->NodePos;
	_trx_p_RevKeys = 0;

	if ((_trx_p_KeyPos = _trx_pTreeUsr->KeyPos) > 1 || _Node(hp)->N_NumKeys > 1) {

		/*The usual case: No node deletion is required. If the highest
		  key in a leaf is deleted, user positions will shift to the next
		  leaf (KeyPos=(KEYPOS_MASK|KEYPOS_HIBIT)) or to EOF (KeyPos=KEYPOS_HIBIT).*/

		_trx_Btdel(hp, _trx_p_KeyPos);
		if (_trx_p_KeyPos == 1) _trx_p_NewLeaf = _Node(hp)->N_Rlink;
		goto _update_hp;
	}

	/*A leaf deletion will be required. First, initialize local
	  variables characterizing the deleted leaf --*/

	revleaf_flags = _Node(hp)->N_Flags;
	llink_node = _Node(hp)->N_Llink;
	rlink_node = _Node(hp)->N_Rlink;

	/*Get target key when a tree traversal is required to find parent --*/
	if ((revleaf_flags&(TRX_N_DupCutLeft + TRX_N_EmptyRight)) != TRX_N_DupCutLeft)
		_trx_Btkey(hp, 1, keybuffer, 256);

	/*Delete the current leaf --*/
	_trx_FreeNode(hp);

	/*Initialize flag that will indicate need for placeholder revision --*/
	rev_placeholder = 0;

	/*Update the link and flags of the left adjacent leaf --*/

	if (llink_node) {
		if ((hp = _trx_GetNode(llink_node)) == 0) return trx_errno;
		llink_flags = _Node(hp)->N_Flags;
		_Node(hp)->N_Rlink = rlink_node;
		if (revleaf_flags&TRX_N_DupCutLeft) {
			_AVOID((llink_flags&TRX_N_DupCutRight) == 0);
			if ((revleaf_flags&TRX_N_DupCutRight) == 0) {
				/*We deleted a trailing overflow node --*/
				_Node(hp)->N_Flags &= ~TRX_N_DupCutRight;
				if ((revleaf_flags&TRX_N_EmptyRight) != 0) {
					rev_placeholder = 1;
					if ((llink_flags&TRX_N_DupCutLeft) != 0)
						/*llink_node takes over the role of placeholder --*/
						_Node(hp)->N_Flags = TRX_N_DupCutLeft + TRX_N_EmptyRight;
				}
			}
		}
		/*Else, if llink_node is a trailing overflow node, we will have to
		  make it a placeholder later if we determine that the deleted
		  node's upperbound key corresponds to N_Rlink in a non-empty
		  higher branch node!*/

		_csh_MarkHdr(hp);
	}
	else {
		_AVOID(_TRXT->LowLeaf != _trx_p_RevLeaf);
		_TRXT->LowLeaf = rlink_node;
		llink_flags = 0;
	}

	/*Update the link and flags of the right adjacent leaf --*/

	if (rlink_node) {

		if ((hp = _trx_GetNode(rlink_node)) == 0) return trx_errno;
		_Node(hp)->N_Llink = llink_node;
		rlink_flags = _Node(hp)->N_Flags;

		/*Set variables for user positioning --*/
		_trx_p_NewLeaf = rlink_node;
		_trx_p_RevKeys = (KEYPOS_MASK | KEYPOS_HIBIT);

		if (revleaf_flags == TRX_N_DupCutRight) {
			/*rlink_node is a first overflow node. Make it, instead,
			a normal leaf, or a duplicate chain's beginning node
			if flag TRX_DupCutRight is turned on --*/
			_AVOID((rlink_flags&TRX_N_DupCutLeft) == 0);
			_Node(hp)->N_Flags &= TRX_N_DupCutRight;
			/*If rlink_node was a placeholder, we will need to shift
			  keys right in a higher branch --*/
			if ((rlink_flags&TRX_N_EmptyRight) != 0) rev_placeholder = 2;
		}
		_csh_MarkHdr(hp);
	}
	else {
		/*Set variables for user positioning --*/
		_trx_p_NewLeaf = 0L;
		_trx_p_RevKeys = KEYPOS_HIBIT;

		_AVOID(_TRXT->HighLeaf != _trx_p_RevLeaf ||
			(revleaf_flags && revleaf_flags != TRX_N_EmptyRight + TRX_N_DupCutLeft));

		if ((_TRXT->HighLeaf = llink_node) == 0L) {
			/*We have deleted the last tree node --*/
			_AVOID(_TRXT->Root != _trx_p_RevLeaf);
			_TRXT->Root = 0L;
			goto _flush;
		}
		rlink_flags = 0;
	}

	/*At this point we have finished revising the adjacent leaves. (Except
	  that we may have to return to llink_node to make it a placeholder if
	  it is a trailing overflow node.)*/

	  /*If we deleted an overflow node that is not a placeholder, we
		are finished since the leaf has no parent --*/

	if ((revleaf_flags&(TRX_N_DupCutLeft + TRX_N_EmptyRight)) == TRX_N_DupCutLeft)
		goto _flush;

	/*One or more branch level nodes must now be modified. First, we
	  descend to branch level 1 from the root while saving node numbers
	  and link positions. If rev_placeholder==1, the deleted leaf was a
	  placeholder. If rev_placeholder==2, it was a beginning node with its
	  right-adjacent node a placeholder. In either case we will have to
	  revise the particular N_Rlink that points to the placeholder (which
	  may be at a branch level higher than 1).

	  If the deleted leaf was a placeholder (rev_placeholder==1), we
	  proceed as follows: If llink_node is an overflow node, we have
	  already made it the new placeholder, so we simply replace N_Rlink
	  with llink_node, finishing the deletion operation. Otherwise, we
	  replace llink_node's upper bound with a higher one by deleting the
	  key at position 1 and moving the associated link (which, if we are
	  at level 1, must be llink_node) to N_Rlink. We are then finished
	  unless this is the root and there are no remaining keys (at least
	  two children). In the latter case, we proceed to trim tree levels.

	  If rlink_node was a first overflow node and also a placeholder
	  (rev_placeholder==2), we replace rlink_node's upper bound with a
	  higher one by deleting the key at position 1 and moving the
	  associated link to N_Rlink. If this is the root, and there are no
	  remaining keys, we remember to return here to trim tree levels. We
	  then continue our descent to the level 1 parent (if we are not
	  already there), where we replace N_Rlink with rlink_node.*/

	branch_index = 0;
	link = _TRXT->Root;
	while (TRUE) {
		if ((hp = _trx_GetNode(link)) == 0) return trx_errno;
		if ((w = _Node(hp)->N_Flags&TRX_N_Branch) == 0) return FORMAT_ERROR();
		lvlnod[branch_index] = link;
		link = _trx_Btlnk(hp, keybuffer);
		if (rev_placeholder > 0 && _trx_btcnt == 1) {
			if (rev_placeholder == 1) {
				/*A placeholder node was deleted --*/
				if (_Node(hp)->N_Rlink == _trx_p_RevLeaf) {
					if ((llink_flags&TRX_N_DupCutLeft) != 0) _Node(hp)->N_Rlink = llink_node;
					else {
						_AVOID(w == 1 && link != llink_node);
						_Node(hp)->N_Rlink = link;
						if (!branch_index && _Node(hp)->N_NumKeys == 1) goto _trim_levels;
						_trx_Btdel(hp, 1);
					}
					goto _update_hp;
				}
			}
			else { /*rev_placeholder==2*/
			  /*rlink_node is a first overflow and a placeholder --*/
				if (_Node(hp)->N_Rlink == rlink_node) {
					_AVOID(w == 1 && link != _trx_p_RevLeaf);
					_Node(hp)->N_Rlink = link; /*will replace again if w==1*/
					_trx_Btdel(hp, 1);
					_csh_MarkHdr(hp);
					_trx_btcnt--;
					rev_placeholder = (!branch_index && _Node(hp)->N_NumKeys == 0) ? -2 : -1;
				}
			}
		}
		lvlpos[branch_index] = _trx_btcnt;
		if (w == 1) break;
		if (++branch_index == TRX_MAX_BRANCH_LEVELS) return FORMAT_ERROR();
	}

	if (rev_placeholder > 0) return FORMAT_ERROR();

	/*At this point, we are at the appropriate level 1 branch,
	  lvlnod[branch_index]=_Node(hp)->N_This --*/

	  /*If a beginning node of a duplicate chain was deleted --*/
	if ((revleaf_flags&TRX_N_DupCutRight) != 0) {
		if (_trx_btcnt) _trx_Btsetc(hp, _trx_btcnt, &rlink_node);
		else _Node(hp)->N_Rlink = rlink_node;
		if (rev_placeholder < 0) {
			if (_trx_btcnt != 0) return FORMAT_ERROR();
			if (rev_placeholder == -2) {
				if (branch_index) {
					_csh_MarkHdr(hp);
					if ((hp = _trx_GetNode(lvlnod[0])) == 0) return trx_errno;
				}
				goto _trim_levels;
			}
		}
		goto _update_hp;
	}

	/*At start of the following loop, a normal leaf node or a branch node
	  was deleted at the level just below the current branch,
	  lvlnod[branch_index]==_Node(hp)->N_This. We must either delete the
	  corresponding link or free the node if there would be no remaining
	  links to children. If less than two links (including N_Rlink) would
	  remain in a root, we proceed directly to _trim_levels.

	  Note that if llink_node is an overflow node, we cannot replace its
	  upperbound (by deleting a position 1 key and moving its link to
	  N_Rlink). Instead we must make it a placeholder by setting N_Rlink
	  to llink_node and retrieving llink_node to turn on its
	  TRX_N_EmptyRight flag.*/

	while (TRUE) {
		if ((w = lvlpos[branch_index]) > 0) {
			/*The branch node has at least one link in addition to the link
			  pointing to the deleted child.*/
			if (!branch_index && _Node(hp)->N_NumKeys == 1) goto _trim_levels;
			_trx_Btdel(hp, w);
			goto _update_hp;
		}

		/*The link is N_Rlink. Either a node must be deleted, an
		  upperbound replaced, or a placeholder created --*/

		if (_Node(hp)->N_NumKeys > 0) {
			/*Node deletion is unnecessary (apart from trimming levels) --*/
			if ((llink_flags&TRX_N_DupCutLeft) != 0) {
				/*Create a placeholder node --*/
				_AVOID((llink_flags&TRX_N_EmptyRight) != 0);
				_Node(hp)->N_Rlink = llink_node;
				_csh_MarkHdr(hp);
				if ((hp = _trx_GetNode(llink_node)) == 0) return trx_errno;
				_Node(hp)->N_Flags = TRX_N_DupCutLeft + TRX_N_EmptyRight;
				goto _update_hp;
			}
			_Node(hp)->N_Rlink = _trx_Btlnkc(hp, 1);
			if (!branch_index && _Node(hp)->N_NumKeys == 1) goto _trim_levels;
			_trx_Btdel(hp, 1);
			goto _update_hp;
		}

		/*delete node and move to parent --*/
		_trx_FreeNode(hp);
		if (!branch_index--) return FORMAT_ERROR();
		if ((hp = _trx_GetNode(lvlnod[branch_index])) == 0) return trx_errno;
	}

_trim_levels:

	/*At this point _Node(hp) is a root node whose only valid link is
	  N_Rlink. A root with only one child can be trimmed from the tree's
	  top. The child is then promoted to root status and checked to see if
	  it too can be trimmed, and so forth --*/

	do {
		link = _Node(hp)->N_Rlink;
		_AVOID(link == 0L);
		_trx_FreeNode(hp);
		if ((hp = _trx_GetNode(link)) == 0) return trx_errno;
	} while (_Node(hp)->N_NumKeys == 0);
	_TRXT->Root = link;

_update_hp:
	_csh_MarkHdr(hp);

_flush:
	_TRXT->NumKeys--;
	_trx_pFile->HdrChg = TRUE;

	/*If the file mode is TRX_AutoFlush, write revised nodes after each
	  key deletion to reduce probabilty of file corruption due to an
	  unexpected program termination (e.g., power failure).*/

	if (_trx_ErrFlush()) return trx_errno;

	/*Adjust positions of all users --*/
	_trx_DelAdjust();

	return 0;
}

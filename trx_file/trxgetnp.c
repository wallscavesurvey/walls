/*MODULE TRXGETNP*/
#include "a__trx.h"

apfcn_hdrp _trx_GetNodePos(TRX_NO trx, int offset)
{
	/*
	 Adjusts current tree position according to value of offset:

		  -2  - Moves directly to first record/key
		  +2  - Moves directly to last record/key
		   1  - Skips forward one position
		  -1  - Skips back one position
		   0  - No change

	  Also loads node corresponding to current position into cache and
	  returns a non-zero buffer handle if successful, in which case the
	  user variables NodePos and KeyPos are validated. If the current or
	  would-be adjusted position is nonexistent (e.g., KeyPos==0),
	  trx_errno is set to TRX_ErrEOF and 0 is returned.

	  If KeyPos==KEYPOS_HIBIT, the position identifies an invalidated key just
	  right of the current highest key in the tree. Otherwise, if
	  KeyPos==(KEYPOS_MASK|KEYPOS_HIBIT), it identifies an invalidated key just left of the
	  current leftmost (highest-count) key in node NodePos.

	  KeyPos>KEYPOS_MASK is possible only if the key at the current position
	  was previously deleted. In this case, offset==0 produces
	  trx_errno=TRX_ErrDeleted, offset==1 produces new position
	  KeyPos&KEYPOS_MASK, and offset==-1 produces new position ++KeyPos&KEYPOS_MASK
	  (with possible adjustments for a new leaf or EOF condition). In
	  effect, if offset!=0, the resulting position change will occur as
	  if the original key had not been deleted.

	  Note: The initial values of KeyPos and NodePos are not affected if
	  0 is returned due to an error or EOF condition. */

	CSH_HDRP hp;
	UINT cnti, numk;
	DWORD nxtnod;

	if (!_GETTRXU || (trx_errno = _trx_pFile->Cf.Errno) != 0) return 0;

	if (offset > 1) {
		/*go to last record/key*/
		nxtnod = _trx_pTreeVar->HighLeaf;
		cnti = 1;
		offset = 0;
	}
	else if (offset < -1) {
		/*go to first record/key*/
		nxtnod = _trx_pTreeVar->LowLeaf;
		cnti = KEYPOS_MASK;
		offset = 0;
	}
	else {
		cnti = _TRXU->KeyPos;
		nxtnod = cnti ? ((cnti == KEYPOS_HIBIT) ? _trx_pTreeVar->HighLeaf : _TRXU->NodePos) : 0L;
	}

	if ((hp = _trx_GetNode(nxtnod)) == 0) return 0;
#ifdef _TRX_TEST
	/*Confirm this is a leaf --*/
	if ((_Node(hp)->N_Flags&TRX_N_Branch) != 0) {
		FORMAT_ERROR();
		return 0;
	}
#endif

	if (cnti > KEYPOS_MASK) {
		/*A deletion occurred at the user's current position --*/
		if (!offset) {
			trx_errno = TRX_ErrDeleted;
			return 0;
		}
		cnti &= KEYPOS_MASK;
		if (offset > 0) offset = 0;
		/*else offset==-1*/
	}

	if (cnti > (numk = _Node(hp)->N_NumKeys)) cnti = numk;
	cnti -= offset;

	/*At this point cnti is in range, or cnti==(numk+1 or 0) --*/

	if (cnti && cnti <= numk) goto _rethp;

	nxtnod = cnti ? _Node(hp)->N_Llink : _Node(hp)->N_Rlink;
	if ((hp = _trx_GetNode(nxtnod)) == 0) return 0;
	if ((numk = _Node(hp)->N_NumKeys) == 0) {
		FORMAT_ERROR();
		return 0;
	}
	cnti = cnti ? 1 : numk;

_rethp:
	_TRXU->NodePos = nxtnod;
	_TRXU->KeyPos = cnti;
	return hp;
}

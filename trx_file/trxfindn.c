/* TRXFINDN.C - Find next matching key in TRX_FILE index -- */
#include <a__trx.h>

/*This function advances the current tree position to the next key and
  compares this key with a target key, pKey. Zero is returned if either
  pKey is an exact match or if pKey is a prefix of the tree's key and
  user flag TRX_KeyExact is turned off. Otherwise, if the repositioning is
  successful, TRX_ErrMatch is returned. If there is no next key,
  TRX_ErrEOF is returned.

  If zero or TRX_ErrMatch is returned, the global word trx_findResult is
  set to zero for an exact match, xx00h for a prefix match (xxh is the
  length of the tree's unmatching suffix), or 00xxh if the target has an
  unmatching suffix (xxh is nonzero). If the target is larger than the
  next key, trx_findResult is set to FFFFh.

  If a value other than zero or TRX_ErrMatch is returned, no repositioning
  occurs and  trx_findResult is set to FFFFh.
*/

int TRXAPI trx_FindNext(TRX_NO trx, LPBYTE pKey)
{
	CSH_HDRP hp;
	UINT frslt;

	trx_findResult = 0xFFFF;
	if ((hp = _trx_GetNodePos(trx, 1)) == 0) return trx_errno;
	if (!_trx_pTreeVar->SizKey) return trx_errno = TRX_ErrTreeType;
	frslt = _trx_Btfndn(hp, _trx_pTreeUsr->KeyPos, pKey); /*=trx_findResult*/
	return (frslt &&
		((BYTE)frslt || (_trx_pTreeUsr->UsrFlags&TRX_KeyExact))) ?
		(trx_errno = TRX_ErrMatch) : 0;
}

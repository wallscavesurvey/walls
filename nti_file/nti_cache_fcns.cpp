#include <nti_file.h>
#include <trx_str.h>
#include <sys/stat.h>
#include <assert.h>

apfcn_i	nti_ResizeCache(NTI_NO nti, UINT numbufs)
{
	CSH_NO csh = nti->Cf.Csh;
	UINT e = 0;
	if (csh) {
		CSF_NO csf = csh->FstFile;
		while (csf) {
			e++;
			csf = csf->NextFile;
		}
		if (e > 1) return nti_errno = NTI_ErrCacheResize;
		assert(e == 1);
		if (e = csh_Free(csh)) return e;
	}
	e = nti_AllocCache(nti, numbufs);
	if (csh && csh == nti_defCache) nti_defCache = (TRX_CSH)nti->Cf.Csh;
	return e;
}

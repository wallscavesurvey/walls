#include <windows.h>
#include <string.h>
#include <trx_str.h>
#include <dos_io.h>

#include "wnetlib.h"

static apfcn_i update_vectorsH(void)
{
	int i, idnxt;
	double varH = net_dbstrp->hv[0];

	//Obtain the correction vector --
	for (i = 0; i < 2; i++) P[i] = net_dbstrp->e_xyz[i] - net_dbstrp->xyz[i];
	GOTO_VEC(net_dbstrp->vec_id);
	idnxt = next_id(net_dbvecp->id[net_dbvecp->id[0] == net_dbstrp->id[0]]);

	while (TRUE) {
		idnxt = next_id(idnxt);
		if (net_dbvecp->hv[0] <= 0.0)
			memcpy(net_dbvecp->e_xyz, net_dbvecp->xyz, 2 * sizeof(double));
		else {
			/*varH has to be nonzero --*/
			Z = ((idnxt == net_dbvecp->id[1]) ? 1.0 : -1.0) * net_dbvecp->hv[0] / varH;
			for (i = 0; i < 2; i++) net_dbvecp->e_xyz[i] = (net_dbvecp->xyz[i] + Z * P[i]);
		}
		dbf_Mark(net_dbvec);
		if (net_dbvecp->str_nxt_vc == 0) break;
		GOTO_VEC(net_dbvecp->str_nxt_vc);
	}
	return 0;
}

static apfcn_i update_vectorsV(void)
{
	int idnxt;
	double varV = net_dbstrp->hv[1];

	//Obtain the correction vector --
	Z = net_dbstrp->e_xyz[2] - net_dbstrp->xyz[2];
	GOTO_VEC(net_dbstrp->vec_id);
	idnxt = next_id(net_dbvecp->id[net_dbvecp->id[0] == net_dbstrp->id[0]]);

	while (TRUE) {
		idnxt = next_id(idnxt);
		if (net_dbvecp->hv[1] <= 0.0) net_dbvecp->e_xyz[2] = net_dbvecp->xyz[2];
		else {
			/*varV has to be nonzero --*/
			net_dbvecp->e_xyz[2] = (net_dbvecp->xyz[2] +
				Z * (((idnxt == net_dbvecp->id[1]) ? 1.0 : -1.0) * net_dbvecp->hv[1] / varV));
		}
		dbf_Mark(net_dbvec);
		if (net_dbvecp->str_nxt_vc == 0) break;
		GOTO_VEC(net_dbvecp->str_nxt_vc);
	}
	return 0;
}

static apfcn_i update_flt_vectorsV(void)
{
	int i, idnxt;
	double varV = 0.0;
	int iFltV = 0;
	int iCnt = 0;

	//First, obtain total variances of floated vectors --
	i = net_dbstrp->vec_id;
	while (i) {
		GOTO_VEC(i);
		iCnt++;
		if (net_dbvecp->flag&NET_STR_FLTVECTV) {
			varV += net_dbvecp->hv[1];
			iFltV++;
		}
		i = net_dbvecp->str_nxt_vc;
	}

	if (!iFltV) varV = net_dbstrp->hv[1];

	P[2] = net_dbstrp->e_xyz[2] - net_dbstrp->xyz[2];

	GOTO_VEC(net_dbstrp->vec_id);
	idnxt = next_id(net_dbvecp->id[net_dbvecp->id[0] == net_dbstrp->id[0]]);

	while (TRUE) {
		idnxt = next_id(idnxt);
		if (iFltV && !(net_dbvecp->flag&NET_STR_FLTVECTV))
			net_dbvecp->e_xyz[2] = net_dbvecp->xyz[2];
		else {
			if (varV > 0.0) Z = net_dbvecp->hv[1] / varV;
			else Z = 1.0 / (iFltV ? iFltV : iCnt);
			net_dbvecp->e_xyz[2] = (net_dbvecp->xyz[2] +
				P[2] * ((idnxt == net_dbvecp->id[1]) ? 1.0 : -1.0) * Z);
		}
		dbf_Mark(net_dbvec);
		if (net_dbvecp->str_nxt_vc == 0) break;
		GOTO_VEC(net_dbvecp->str_nxt_vc);
	}
	return 0;
}

static apfcn_i update_flt_vectorsH(void)
{
	int i, idnxt;
	double varH = 0.0;
	int iFltH = 0;
	int iCnt = 0;

	//First, obtain total variances of floated vectors --
	i = net_dbstrp->vec_id;
	while (i) {
		GOTO_VEC(i);
		iCnt++;
		if (net_dbvecp->flag&NET_STR_FLTVECTH) {
			varH += net_dbvecp->hv[0];
			iFltH++;
		}
		i = net_dbvecp->str_nxt_vc;
	}

	if (!iFltH) varH = net_dbstrp->hv[0];

	for (i = 0; i < 2; i++) P[i] = net_dbstrp->e_xyz[i] - net_dbstrp->xyz[i];

	GOTO_VEC(net_dbstrp->vec_id);
	idnxt = next_id(net_dbvecp->id[net_dbvecp->id[0] == net_dbstrp->id[0]]);

	while (TRUE) {
		idnxt = next_id(idnxt);
		if (iFltH && !(net_dbvecp->flag&NET_STR_FLTVECTH))
			memcpy(net_dbvecp->e_xyz, net_dbvecp->xyz, 2 * sizeof(double));
		else {
			Z = ((idnxt == net_dbvecp->id[1]) ? 1.0 : -1.0);
			if (varH > 0.0) Z *= net_dbvecp->hv[0] / varH;
			else Z *= 1.0 / (iFltH ? iFltH : iCnt);
			for (i = 0; i < 2; i++) net_dbvecp->e_xyz[i] = (net_dbvecp->xyz[i] + Z * P[i]);
		}
		dbf_Mark(net_dbvec);
		if (net_dbvecp->str_nxt_vc == 0) break;
		GOTO_VEC(net_dbvecp->str_nxt_vc);
	}
	return 0;
}

static apfcn_i update_str_ntv(void)
{
	int f = net_dbstr_wflag;
	if (!net_i) {
		if ((f&NET_STR_FLTMSKH) != 0 && (f&NET_STR_FLTALLH) == 0) {
			f = update_flt_vectorsH();
		}
		else f = update_vectorsH();
	}
	else {
		if ((f&NET_STR_FLTMSKV) != 0 && (f&NET_STR_FLTALLV) == 0) {
			f = update_flt_vectorsV();
		}
		else f = update_vectorsV();
	}
	return f;
}

static apfcn_i update_sys_ntv(void)
{
	/*Called by net_Update(). Only the component(s) specified by net_i are affected.*/
	int s = sys_recfirst;
	int smax = s + sys_numstr;

	for (; s < smax; s++) {
		GOTO_STR(s);
		if (update_str_ntv()) return net_errno;
	}
	return 0;
}

static apfcn_i update_chain_ntv(rec0)
{
	int rec = rec0;
	while (TRUE) {
		if (update_str_ntv()) return net_errno;
		if ((rec = net_dbstrp->lnk_nxt) == rec0) break;
		GOTO_STR(rec);
	}
	return 0;
}

static apfcn_i update_chain_str(int rec0)
{
	int rec;
	double fdir, varsum, exy[2];
	double *pxy = net_dbstrp->e_xyz + 2 * net_i;
	double *pv = net_dbstrp->hv + net_i;
	int chndir0 = (net_dbstr_wflag&NET_STR_CHNDIR);
	int flg = (NET_STR_CHNFLTH << net_i);
	int nflt, bNewFlt;

	//Calculate the correction vector and variance sum --
	exy[0] = exy[1] = varsum = 0.0;
	nflt = bNewFlt = 0;
	rec = rec0;

	do {
		if (net_dbstr_wflag&flg) {
			if (rec == rec0) fdir = 1.0;
			else {
				nflt++;
				varsum += *pv;
				fdir = (chndir0 == (net_dbstr_wflag&NET_STR_CHNDIR)) ? 1.0 : -1.0;
			}
			for (rec = 0; rec < 2 - net_i; rec++) exy[rec] += fdir * (pxy[rec] - pxy[rec - 5]);
		}
		rec = net_dbstrp->lnk_nxt;
		GOTO_STR(rec);
	} while (rec != rec0);

	net_dbstr_wflag ^= flg; //toggle CHNFLT flag

	if (net_dbstr_wflag&flg) {
		//A new floated member --
		nflt++;
		varsum += *pv;
		net_dbstr_wflag &= ~(NET_STR_BRIDGEH << net_i);
	}
	else {
		//A new bridge --
		memcpy(pxy, pxy - 5, (2 - net_i) * sizeof(net_dbstrp->e_xyz[0]));
		if (net_dbstr_wflag&(NET_STR_FLOATH << net_i)) {
			net_dbstr_wflag &= ~(NET_STR_FLOATH << net_i);
			bNewFlt++;
		}
		net_dbstr_wflag |= (NET_STR_BRIDGEH << net_i);
	}

	//Now let's reassign the correction to the new floated set --

	if (varsum <= 0.0) varsum = 1.0;
	do {
		if (nflt < 2 && (net_dbstr_wflag&flg)) net_dbstr_wflag &= ~(NET_STR_CHNFLTXH << net_i);
		else net_dbstr_wflag |= (NET_STR_CHNFLTXH << net_i);

		if (net_dbstr_wflag&flg) {
			if (bNewFlt) {
				bNewFlt--;
				net_dbstr_wflag |= (NET_STR_FLOATH << net_i);
			}
			fdir = (chndir0 == (net_dbstr_wflag&NET_STR_CHNDIR)) ? 1.0 : -1.0;
			for (rec = 0; rec < 2 - net_i; rec++) {
				pxy[rec] = pxy[rec - 5] + fdir * exy[rec] * (*pv / varsum);
			}
		}
		dbf_Mark(net_dbstr);
		GOTO_STR(rec = net_dbstrp->lnk_nxt);
	} while (rec != rec0);
	return 0;
}

static apfcn_i update_net_ntp()
{
	int i;
	UINT ep, rec, lastrec;
	int endpts;
	double *pxyzVec;

	/*Note: To avoid roundoff, we are using doubles to accumulate
	  absolute coordinates.*/

#if _STACKNEAR
	DIM_DPOINT xyz;
	DIM_POINT xyzLast;
	double _huge *pxyz;
#else
	static DIM_DPOINT xyz;
	static DIM_POINT xyzLast;
	static double _huge *pxyz;
#endif

	//if(open_dbplt()) return net_errno;

	CHK_MEMH(pxyz, (DWORD)net_dbnetp->endpts * sizeof(DIM_DPOINT));

	endpts = 1;
	memset(xyz, 0, sizeof(DIM_DPOINT));

	lastrec = net_dbnetp->plt_id2;

	for (rec = net_dbnetp->plt_id1; rec <= lastrec; rec++) {
		if (GOTO_PLT_RTN(rec)) break;

		if ((ep = net_dbpltp->vec_id) != 0) {
			//DrawTo --
			if (net_dbpltp->flag == NET_REVISIT_FLAG) {
#ifdef _DEBUG
				if (endpts <= net_dbpltp->end_pt) break;
#endif
				for (i = 0, ep = Dim * net_dbpltp->end_pt; i < Dim;) xyz[i++] = pxyz[ep++];
			}
			else {
				//Retrieve point from vector database --
				//We can avoid this by checking if str_id==0 in which case displacement from
				//last point is unchanged!

				if (!net_dbpltp->str_id) {
					for (i = 0; i < Dim; i++) xyz[i] += net_dbpltp->xyz[i] - xyzLast[i];
				}
				else {

					if (GOTO_VEC_RTN((ep + 1) >> 1)) break;

					if (net_dbvecp->str_id) pxyzVec = net_dbvecp->e_xyz;
					else pxyzVec = net_dbvecp->xyz;

					if (ep & 1)
						for (i = 0; i < Dim; i++) xyz[i] -= pxyzVec[i];
					else
						for (i = 0; i < Dim; i++) xyz[i] += pxyzVec[i];
				}

				if (net_dbpltp->end_pt) {
#ifdef _DEBUG
					if (endpts != net_dbpltp->end_pt) break;
#endif
					for (i = 0, ep = Dim * endpts++; i < Dim;) pxyz[ep++] = xyz[i++];
				}
#ifdef _DEBUG
				else if (net_dbpltp->flag != NET_RESOLVE_FLAG) break;
#endif
			}
		}
		else {
			//MoveTo --
#ifdef _DEBUG
			if (endpts <= net_dbpltp->end_pt) break;
#endif
			for (i = 0, ep = Dim * net_dbpltp->end_pt; i < Dim;) xyz[i++] = pxyz[ep++];
		}

		memcpy(xyzLast, net_dbpltp->xyz, sizeof(DIM_POINT));
		for (i = 0; i < Dim; i++) net_dbpltp->xyz[i] = xyz[i];
		dbf_Mark(net_dbplt);
	}

	dos_FreeHuge(pxyz);

	FCHKP(dbf_Flush(net_dbplt));

	if (rec <= lastrec) {
		if (!net_errno) {
			net_errno = NET_ErrWork;
			free_resources();
		}
		return net_errno;
	}

	return 0;
}

int PASCAL net_Update(NET_DATA FAR *pData)
{
	UINT nts_rec;

	/*This function updates a database already created with a net_Adjust() call.
	  Only the system containing string record pData->s_id is readjusted. Also, only
	  the components (Hz or Vt) indicated by pData->nNetworks (0 or 1) are affected.*/

	net_pCB = 0;
	zero_resources();
	if (pData->version != NET_VERSION) CHK_ABORT(NET_ErrVersion);

	net_pData = pData;
	net_i = pData->net_id; /*0-Hz, or 1-Vt*/
	nts_rec = pData->sys_id;

	net_dbvecp = (vecptr)_NPTR(dbf_RecPtr(net_dbvec = pData->ntv_no));
	net_dbstrp = (strptr)_NPTR(dbf_RecPtr(net_dbstr = pData->nts_no));
	net_dbpltp = (pltptr)_NPTR(dbf_RecPtr(net_dbplt = pData->ntp_no));
	net_dbnetp = (netptr)_NPTR(dbf_RecPtr(net_dbnet = pData->ntn_no));
	vecopen = netopen = stropen = pltopen = FALSE;

	//if(open_dbvec(ntvpath)) return net_errno;

	//Upon opening the string database we ignore the header's Incomplete flag
	//since we assume WALLS has modified and flushed the file while still
	//keeping it open --

	//if(open_dbstr()) return net_errno;

	//if(open_dbnet()) return net_errno;

	/*Readjust component net_i of the system containing NTS record nts_rec --*/

	GOTO_STR(nts_rec);
	if (net_dbstr_wflag&(NET_STR_CHNFLTXH << net_i)) {
		//We are floating or unfloating a chained traverse in
		//the situation where at least one other member of the chain is
		//floated. Only a traverse chain adjustment is required --
		if (update_chain_str(nts_rec) ||
			update_chain_ntv(nts_rec) || position_dbnet(nts_rec)) return net_errno;
	}
	else {
		/*Toggle the Hz or Vt float flag --*/
		net_dbstr_wflag ^= (NET_STR_FLOATH << net_i);
		dbf_Mark(net_dbstr);

		if (sys_solve(nts_rec)) {
			GOTO_STR(nts_rec);
			net_dbstr_wflag ^= (NET_STR_FLOATH << net_i);
			dbf_Mark(net_dbstr);
			free_resources();
			return net_errno;
		}

		/*At this point sys_recfirst and sys_numstr have been computed.
		Update the string vectors of this system --*/
		if (update_sys_ntv()) return net_errno;
	}

	FCHKS(dbf_Flush(net_dbstr));

	/*At this point net_dbnetp points to this system's component, which in
	  turn has pointers to the portion of the plot file that needs updating --*/

	if (update_net_ntp()) return net_errno;

	FCHKV(dbf_Flush(net_dbvec));

	FCHKN(dbf_Flush(net_dbnet));

	//free_resources();
	net_dbvec = net_dbstr = net_dbplt = net_dbnet = 0;
	return 0;
}

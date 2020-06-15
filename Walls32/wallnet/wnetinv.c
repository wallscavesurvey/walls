/*Module WNETINV.C - Matrix modification for WALLNET.DLL */

#include <string.h>
#include "wnetlib.h"
#include <dos_io.h>

#undef _USE_CB_LEVEL

#ifdef _USE_CB_LEVEL
#define CB_LEVEL_INIT 1000
#define CB_LEVEL_INC 100
#endif

//#pragma optimize("",off)

/* Define DELETES if any string observations are to be processed while
   not requiring a solution for the corresponding displacements. In
   this case, certain rows of Q and elements of V need not be preserved.
   Such strings will have been flagged.

   NOTE: This feature has NOT yet been fully implemented or tested!
   The appearance of this macro is for future reference only.
   #undef DELETES
*/

#define Qtyp double
#define Qptr dlfptr

#define Qcol ((Qptr *)QCOL_ARRAY)
#define Q_(r,c) (*(Qcol[c]+r))

/*Globals referenced by UpdateQ() --*/
static int    Dm, qc, qr;
static double M;
static Qptr   Qcol_c;

static int    v_nod1, v_nod2, v_s, v_t;
static double F;
static Qptr V, v_v;
static BOOL no_alloc;

/*=============================================================*/
static apfcn_v UpdateQ(void)
{
	/*Revision of the Q matrix with a displacement/variance between
	  existing nodes. The first Dm rows of Q are the positions of
	  the border nodes with respect to the current reference node.*/

	int i, j;
	Qptr Qcol_j;

	for (j = Dm; j <= qc; j++) {
		ZZ = Qcol_c[row[j]] * M;
		Qcol_j = Qcol[j];
#ifdef DELETES
		for (i = 0; i <= qr; i++) Qcol_j[row[i]] -= ZZ * Qcol_c[row[i]];
#else
		for (i = 0; i <= qr; i++) *Qcol_j++ -= ZZ * Qcol_c[i];
#endif
	}
}

static apfcn_v reverse_vec(Qptr vv)
{
	Qptr qp;

	for (qp = vv + Dm; qp > vv; qp--) *qp = -*qp;
}

static _inline ExchangeInt(lnptr p1, lnptr p2)
{
	long i = *p1;
	*p1 = *p2;
	*p2 = i;
}

static _inline ExchangeQptr(Qptr *p1, Qptr *p2)
{
	Qptr p = *p1;
	*p1 = *p2;
	*p2 = p;
}

static apfcn_v DeleteCol(int c)
{
	/* Removes column of border node, making it an interior node,
	   Deletes a logical column of Q.*/

	if (c >= Dm) {
		ExchangeQptr(&Qcol[qc], &Qcol[c]);
		ExchangeInt(&row[qc], &row[c]);
		ExchangeInt(&colnod[qc], &colnod[c]);
		nodcol[colnod[qc]] = qc;
		nodcol[colnod[c]] = c;
		qc--;
	}
}

#ifdef DELETES
static apfcn_v DeleteRow(int r)
{
	/* Removes node for which there are no "Save" vectors adjacent */

	if (r >= Dm) {
		ExchangeInt(&row[r], &row[qr]);
		nodcol[colnod[r] = colnod[qr]] = r;
		qr--;
	}
}
static apfcn_v MoveCol(int j, int c)
{
	/*Moves components of col j to col c or zeroes col c
	  prior to updating or extending Q. */

	int i;
	Qptr Qcol1, Qcol2;

	Qcol2 = Qcol[c];

	if (j >= 0) {
		Qcol1 = Qcol[j];
		for (i = 0; i <= qr; i++) Qcol2[row[i]] = Qcol1[row[i]];
	}
	else for (i = 0; i <= qr; i++) Qcol2[row[i]] = 0.0;
}
#else
static apfcn_v MoveCol(int j, int c)
{
	/*Moves components of col j to col c or zeroes col c
	  prior to updating or extending Q. */

	  /*NOTE: This version assumes rows are not deleted (qr decremented)!*/

	if (j >= 0) memcpy(Qcol[c], Qcol[j], (qr + 1) * sizeof(Qtyp));
	else     memset(Qcol[c], 0, (qr + 1) * sizeof(Qtyp));
}
#endif

static apfcn_v MoveRow(int r1, int r2)
{
	int j;

	r2 = row[r2];
	if (r1 >= 0) {
		r1 = row[r1];
		for (j = Dm; j <= qc; j++) {
			Q_(r2, j) = Q_(r1, j);
		}
	}
	else for (j = Dm; j <= qc; j++) Q_(r2, j) = 0.0;
}

static apfcn_v MoveRoot(int s)
{
	/*Moves the network reference to the node corresponding to
	  logical column s of Q. Column s will then correspond to the
	  old reference node. The transformation is

	   Q0s := -Q0s,  Q0j :=  Q0j - Q0s (coordinates wrt root),
	   Qss :=  Qss,  Qsj :=  Qss - Qsj (column s),
	   Qij :=  Qss + Qij - Qis - Qsj   (for i,j not 0 or s).

	  The purpose of changing the reference node is to allow
	  a column (and possibly a row) of Q to be eliminated when
	  all vectors adjacent to the current reference have been
	  added. For good vector orderings this also tends to keep
	  the border node coordinates (the first Dm rows of Q)
	  from becoming large in magnitude.*/

	int i, j;
	Qptr Qcol_s, Qcol_j;

	if (s < Dm) return;

	Qcol_s = Qcol[s];
	ZZ = (Qcol_s[row[s]] / 2.0);

	/*zero row s of Q*/
	MoveRow(-1, s);

	for (i = 0; i < Dm; i++) P[i] = -Qcol_s[i];

#ifdef DELETES
	for (i = Dm; i <= qr; i++) {
		Qcol_j = Qcol_s + row[i];
		*Qcol_j = ZZ - *Qcol_j;
	}
#else
	Qcol_j = Qcol_s + Dm;
	for (i = Dm; i <= qr; i++, Qcol_j++) *Qcol_j = ZZ - *Qcol_j;
#endif

	for (j = Dm; j <= qc; j++) /*for each column not s --*/
		if (j != s) {
			Qcol_j = Qcol[j];
			M = Qcol_s[row[j]]; /*M=Qss/2-Qsj*/
#ifdef DELETES
			for (i = 0; i < Dm; i++) Qcol_j[i] += P[i];
			for (; i <= qr; i++) Qcol_j[row[i]] += M + Qcol_s[row[i]];
#else
			for (i = 0; i < Dm; i++) *Qcol_j++ += P[i];

			/* Qij = Qij + Qss - Qsj - Qsi */
			for (; i <= qr; i++) *Qcol_j++ += M + Qcol_s[i];
#endif
		}
	for (i = 0; i < Dm; i++) *Qcol_s++ = P[i];
	//for(;i<=qr;i++) *Qcol_s++ += ZZ;
	for (; i <= qr; i++, Qcol_s++) *Qcol_s += ZZ;

	ExchangeInt(&colnod[s], &colnod[-1]);
	nodcol[colnod[s]] = s;
	nodcol[colnod[-1]] = -1;
}

/*==============================================================*/
static apfcn_v extend(void)
{
	int i;
	Qptr fp;

	/* Adds a new node to the network. No other displacements and
	   covariances in Q are affected.*/

	if (v_t & Move1) { /*v_nod1 is resolved*/
		if (nodcol[v_nod1] < 0) MoveRoot(qc);
		DeleteCol(nodcol[v_nod1]);
		i = ++qc;
#ifdef DELETES
		if (!(v_t & Delete1)) {
			MoveRow(i, ++qr);
			nodcol[colnod[qr] = v_nod1] = qr;
		}
#else
		MoveRow(i, ++qr);
		nodcol[colnod[qr] = v_nod1] = qr;
#endif
	}
	else { /*from node stays in place*/
		i = nodcol[v_nod1];
		if (++qr > ++qc) {
			ExchangeInt(&row[qc], &row[qr]);
			nodcol[colnod[qr] = colnod[qc]] = qr;
		}
		MoveCol(i, qc);
		MoveRow(i, qc);
	}

	nodcol[colnod[qc] = v_nod2] = qc;
	fp = Qcol[qc];
	*(fp + row[qc]) = (i >= 0) ? *(fp + row[i]) + *v_v : *v_v;
	for (i = 0; i < Dm; i++) *fp++ += *++v_v;
}

static apfcn_v adjust(int es)
{
	/*Saved vectors and their variances in array V are revised by the
	  observed vector. If the vector's Nod_inet flag is set it will
	  from now on contain a current vector/variance estimate; otherwise, it
	  will have the estimate that was in effect prior to its addition,
	  which will remain unchanged.*/

	int i, e;

	sys_nc++;
	for (i = 0; i < Dm; i++) sys_ss[i] += M * Qcol_c[i] * Qcol_c[i];

	for (; es >= 0; es--) {
		e = jseq[es];
		if (!(jtask[e >> 1] & Nod_inet)) continue;
		if ((i = nodcol[jendp[e ^ 1]]) >= 0) Z = Qcol_c[row[i]];
		else Z = 0.0;
		if ((i = nodcol[jendp[e]]) >= 0) Z -= Qcol_c[row[i]];
		v_v = V + (e >> 1)*(Dm + 1);
		F = Z * M;
		*v_v -= (F * Z);
		for (i = 0; i < Dm; i++) *++v_v += (F * Qcol_c[i]);
	}
}

static apfcn_i update(void)
{
	/*Adds an edge between two existing nodes. This requires that
	  all elements in Q and the saved displacement/variances in
	  the vector array be revised. If Vst is the variance of an added
	  vector between nodes s and t, then Q is transformed as follows:

			  Qij  :=  Qij - (Qis-Qit) * M * (Qsj-Qtj) ,
	   where
			  M  =  1 / (Vst + Qss + Qtt - Qst - Qts) .

	  This can be accomplished with approximately one addition and
	  one multiplication per matrix element.*/

	int i, tcol, fcol;
	Qptr Qcol_j;

	fcol = nodcol[v_nod1];
	tcol = nodcol[v_nod2];
	if ((v_t&Move1) && qc >= Dm) {
		if ((v_t&Move2) && qc > Dm) {
			if (fcol < 0) MoveRoot(fcol = (tcol == qc) ? qc - 1 : qc);
			else if (tcol < 0) MoveRoot(tcol = (fcol == qc) ? qc - 1 : qc);
			DeleteCol(tcol);
		}
		else if (fcol < 0) MoveRoot(qc);
		DeleteCol(nodcol[v_nod1]);
		tcol = nodcol[v_nod2];
		fcol = i = nodcol[v_nod1];
	}
	else MoveCol(fcol, i = qc + 1);
	Qcol_c = Qcol[i];
	if (tcol >= 0) {
		Qcol_j = Qcol[tcol];
#ifdef DELETES
		for (i = 0; i <= qr; i++) Qcol_c[row[i]] -= Qcol_j[row[i]];
#else
		for (i = 0; i <= qr; i++) Qcol_c[i] -= Qcol_j[i];
#endif
		Z = -Qcol_c[row[tcol]];
	}
	else Z = 0.0;
	if (fcol >= 0) Z += Qcol_c[row[fcol]];

#ifdef DELETES
	if (v_t&Delete1) DeleteRow(fcol);
	if (v_t&Delete2) DeleteRow(tcol);
#endif

	if (v_t&Infvar) {
		*v_v = Z;
		for (i = 0; i < Dm; i++) *++v_v = -Qcol_c[i];
		sys_ni++;
		return(FALSE);
	}
	M = *v_v;  *v_v = Z;
	Z += M;
	if (Z <= 0.0) {
		/*This string is a bridge --*/
		sys_numzeroz++;
		jtask[v_s] &= ~Nod_inet;
		return(FALSE);
	}
	for (i = 0; i < Dm; i++) {
		M = *++v_v;
		*v_v = -Qcol_c[i];
		Qcol_c[i] += M;
	}
	M = (1.0 / Z);

	if (qc >= Dm) UpdateQ();  /*The real work is done here!*/
	return(TRUE);
}

apfcn_i str_get_varsum(double *psum)
{
	int rec, rec0;
	double sum = 0.0;

	rec = rec0 = dbf_Position(net_dbstr);
	do {
		if (net_dbstr_wflag&(NET_STR_CHNFLTXH << net_i)) sum += net_dbstrp->hv[net_i];
		rec = net_dbstrp->lnk_nxt;
		if (!rec) {
			return NET_ErrWork;
		}
		GOTO_STR(rec);
	} while (rec != rec0);
	if (!sum) sum = 1.0;
	*psum = sum;
	return 0;
}

apfcn_i str_apply_varsum(double varSum)
{
	double fdir, exy[2];
	double *pxy = net_dbstrp->e_xyz + 2 * net_i;
	double *pv = net_dbstrp->hv + net_i;
	int rec0, rec;
	char dir = (net_dbstr_wflag&NET_STR_LINKDIR);

#ifdef _DEBUG
	if ((net_dbstr_wflag&((NET_STR_FLOATH + NET_STR_CHNFLTXH) << net_i)) != ((NET_STR_FLOATH + NET_STR_CHNFLTXH) << net_i))
		return net_errno = NET_ErrWork;
#endif

	for (rec = 0; rec < 2 - net_i; rec++) exy[rec] = pxy[rec] - pxy[rec - 5];
	rec = rec0 = dbf_Position(net_dbstr);
	while (TRUE) {
		if (net_dbstr_wflag&(NET_STR_CHNFLTXH << net_i)) {
			net_dbstr_wflag |= (NET_STR_CHNFLTH << net_i);
			if (rec != rec0) net_dbstr_wflag &= ~(NET_STR_FLOATH << net_i);
			fdir = (dir == (net_dbstr_wflag&NET_STR_LINKDIR)) ? 1.0 : -1.0;
			for (rec = 0; rec < 2 - net_i; rec++) {
				pxy[rec] = pxy[rec - 5] + fdir * exy[rec] * (*pv / varSum);
			}
		}
		else {
			//This should already have been flagged a bridge --
#ifdef _DEBUG
			if (!(net_dbstr_wflag&(NET_STR_BRIDGEH << net_i))) return net_errno = NET_ErrWork;
			if (net_dbstr_wflag&(NET_STR_FLOATH << net_i)) return net_errno = NET_ErrWork;
#endif
			net_dbstr_wflag |= (NET_STR_BRGFLTH << net_i);
		}
		dbf_Mark(net_dbstr);
		if ((rec = net_dbstrp->lnk_nxt) == rec0) break;
		GOTO_STR(rec);
	}
	return 0;
}

static apfcn_i float_link(void)
{
	int i, infvarMask = ((F_InfvarH + InfvarH) << net_i);
	double varSum;

	for (i = 0; i < sys_numstr; i++) {
		if ((jtask[i] & infvarMask) != infvarMask) continue;
		/*This is the first (actual) floated fragment of a chain
		  with multiple floated fragments --*/
		GOTO_STR(sys_recfirst + i);
		if ((net_dbstr_wflag&((NET_STR_CHNFLTH + NET_STR_CHNFLTXH) << net_i)) == (NET_STR_CHNFLTH << net_i)) {
			/*Set CHNFLTX for the remaining unloated fragments --*/
			while (net_dbstrp->lnk_nxt != sys_recfirst + i) {
				GOTO_STR(net_dbstrp->lnk_nxt);
				net_dbstr_wflag |= (NET_STR_CHNFLTXH << net_i);
				dbf_Mark(net_dbstr);
			}
		}
		else {
			if (str_get_varsum(&varSum)) {
				return net_errno; //Get sum of variances for floated fragments
			}
			if (str_apply_varsum(varSum)) {
				return net_errno;
			}
		}
	}
	return 0;
}

static apfcn_i update_nts(void)
{
	int i, i_frag, infvarMask;
	double ss;
	dlnptr pxy = net_dbstrp->e_xyz + (2 * net_i);
	BOOL bLinkFloat;

	sys_nc += sys_ni;

	if (net_i) {
		i_frag = sys_numlinkfrag;
		ss = sys_ss[0];
	}
	else {
		i_frag = 0;
		ss = (sys_ss[0] + sys_ss[1]);
	}

	infvarMask = ((F_InfvarH + InfvarH) << net_i);
	bLinkFloat = FALSE;

	for (i = 0; i < sys_numstr; i++) {
		GOTO_STR(sys_recfirst + i);
		v_v = V + i * (Dm + 1);
		net_dbstrp->e_hv[net_i] = *v_v;

		if (*v_v == net_dbstrp->hv[net_i] && *v_v && !(net_dbstrp->flag&(NET_STR_FLOATH << net_i))) {
			/*A second floated chain fragment would satisfy the first two conditions.*/
			net_dbstr_wflag |= (NET_STR_BRIDGEH << net_i);
		}

		memcpy(pxy, v_v + 1, Dm * sizeof(Qtyp));

		net_dbstrp->sys_ss[net_i] = ss;
		net_dbstrp->sys_ni[net_i] = sys_ni;
		net_dbstrp->sys_nc = sys_nc;
		if (!sys_updating && !net_i && i_frag < sys_numlinkfrag && (sys_linkfrag[i_frag].rec >> 1) == i) {
			/*Set link direction flag --*/
			net_dbstrp->flag1 |= (NET_STR_LINKDIR * (sys_linkfrag[i_frag].rec & 1));
			/*update next link fragment field --*/
			net_dbstrp->lnk_nxt = sys_linkfrag[i_frag].nxtrec + sys_recfirst;
			++i_frag;
		}
		if (jtask[i] & Infvar) {
			if ((jtask[i] & infvarMask) == infvarMask) {
				/*At least two members of a multi-traverse chain are floated*/
				net_dbstr_wflag |= ((NET_STR_CHNFLTH + NET_STR_CHNFLTXH) << net_i);
				bLinkFloat = TRUE;
			}
			else if (net_dbstrp->lnk_nxt) {
				/*Only one member of this multi-traverse chain is floated,
				We'll need to set CHNFLT for this traverse and
				CHNFLTX for the remaining fragments --*/
				jtask[i] |= infvarMask;
				net_dbstr_wflag |= (NET_STR_CHNFLTH << net_i);
				bLinkFloat = TRUE;
			}
		}
		dbf_Mark(net_dbstr);
	}

	/*if necessary redistribute error accross multiple floated fragments --*/
	return bLinkFloat ? float_link() : 0;
}

static apfcn_i alloc_invert(void)
{
	/*In the original DOS version, memory for storing floating point
	  arrays were simply suballocated as 16-bit segment addresses from a
	  previously allocated large (>=128k) block. We then used 16-bit base
	  pointers to access elements of each array.

	  In this version, 80x86 segments cannot be so computed but must be
	  allocated as selectors, a limited resource in Windows 3.x. We can
	  either allocate separate selectors for V and the sys_bandw columns
	  of Q, or we can try to preserve selectors (and allocation calls) by
	  suballocating the arrays from a fewer number of global memory
	  blocks. Also, for compatibilty reasons we will not use base pointers
	  but will assume that the memory allocation function returns 32-bit
	  pointers whose offset portions are possibly nonzero. (We could
	  assume zero offsets with GlobalAlloc() in Windows 3.x.)

	  To avoid complexity that a 32-bit operating system will make
	  unnecessary anyway, we will simply use far pointers in a
	  conventional manner. Specifically, for workspace we will reuse a
	  static array (no allocation) or allocate just one selector (address)
	  if both V and Q can be stored in one 64k segment. Otherwise, we
	  allocate sys_bandw+1 separate selectors. To date (7/93), no system
	  in any real survey I've worked with has come close to requiring
	  multiple selectors.*/

	int i;
	int sizV = sys_numstr * (Dm + 1);
	int sizTotal = (sys_bandl*sys_bandw) + sizV;

	/*Note: For a single string system, sizTotal=(3*1)+4=7*/

	no_alloc = sizTotal <= ((MAX_SYSTEMS + 1) - (Dm + sys_bandw)) / (int)(sizeof(Qtyp) / sizeof(Qptr));

	if (no_alloc) V = (Qptr)&Qcol[sys_bandw + Dm];
	else {
		if ((V = (Qptr)dos_AllocFar(sizTotal * sizeof(Qtyp))) == NULL) return NET_ErrSysMem;
	}

	for (i = 0; i < sys_bandw; i++) Qcol[Dm + i] = V + (sizV + i * sys_bandl);
	return 0;
}

static apfcn_v free_invert(void)
{
	if (no_alloc) return;
	dos_FreeFar(V);
}

apfcn_i sys_invert(void)
{
	int i, e;
#ifdef _USE_CB_LEVEL
	int cb_level;
#endif

	/*Description of tasks:
	  AddNode - v_nod2 is a new node. Assign it a row and col of Q.
	  Movei   - endi becomes an interior node. Delete its col in Q.
	  Deletei - endi is no longer needed. Delete both row and col.
	  InfvarH - Hz Observation variance is infinite.
	  InfvarV - Vt Observation variance is infinite.
	  Nod_inet- Continue to update corresponding displacement/var.*/

	  /*Note: Hz and Vt components are processed separately: net_i = 0(Hz) or 1(Vt).
		Global variable Infvar is set to the appropriate component flag.*/

		/*Position V and Q arrays in far segment. V is restricted to size 64k
		  in this version, which limits the number of strings in one system
		  to 64k/(4*(Dm+1)) = 4k. This could be quadrupled by storing each
		  component as a separate column.*/

	Dm = 2 - net_i;  /*2 dimensions for Hz, 1 for Vt*/

	if (alloc_invert()) CHK_ABORT(NET_ErrSysMem); /*Consider fatal for now*/

	for (i = 0; i < sys_bandl; i++) row[i] = i;
	nodcol[colnod[-1] = jendp[*jseq ^ 1]] = -1;
	qc = qr = Dm - 1;

	/*Read vector data from NTS file -- */
	for (i = 0; i < sys_numstr; i++) {
		GOTO_STR(sys_recfirst + i);
		v_v = V + i * (Dm + 1);
		*v_v = (Qtyp)net_dbstrp->hv[net_i];
		memcpy(v_v + 1, net_dbstrp->xyz + (2 * net_i), Dm * sizeof(Qtyp));
	}

	/*Initialize residual statistics -- */
	sys_ni = sys_nc = sys_numzeroz = 0;
	for (i = 0; i < Dm; i++) sys_ss[i] = 0.0;

#ifdef _USE_CB_LEVEL
	cb_level = CB_LEVEL_INIT;
#endif

	/*Main loop to add observations -- */
	for (i = 0; i < sys_numstr; i++) {
		e = jseq[i];
		v_nod1 = jendp[e ^ 1];
		v_nod2 = jendp[e];
		v_s = e >> 1;
		v_t = jtask[v_s];
		v_v = V + v_s * (Dm + 1);
		if ((e & 1) == 0) reverse_vec(v_v);
		if (v_t & Addnode) extend();
		else if (update()) adjust(i);
#ifdef _USE_CB_LEVEL
		if (i == cb_level) {
			/*Check for user abort --*/
			net_pData->sys_id = i + net_i;
			if (net_pCB && NET_CB_RTN(NET_PostSolveNet)) {
				free_invert();
				return net_errno;
			}
			cb_level += CB_LEVEL_INC;
		}
#endif
	}

	for (i = 0; i < sys_numstr; i++) {
		e = jseq[i];
		if ((e & 1) == 0) reverse_vec(V + (e >> 1)*(Dm + 1));
	}

	/*Now update the string database with the corrected displacements and
	  final displacement variances. Also, each string record will carry
	  the system sum-of-squares (horiz. and vert.) and loop counts.*/

	update_nts(); /*sets net_errno upon .NTS file update error*/

	free_invert();

	return net_errno;
}

/*WNETDFS.C - Recursive routine - part of WALLNET4.DLL */

#include <limits.h>
#include "wnetlib.h"

/*================================================================*/
#ifndef _USE_DFSTACK
#pragma check_stack(on)

apfcn_i df_search(int n)
{

	/*This function implements a depth-first search algorithm
	  for finding the biconnected components of a connected graph. A
	  biconnected component is a maximal set of edges satisfying the
	  condition that for any chosen pair of edges there is a cycle
	  containing them both. In surveying, edges are observed vector
	  displacements and biconnected components are independent loop
	  systems.

	  This function also performs the operation of coelescing edges
	  into link traverses. For lack of a better name, "link" is what
	  I call a maximal set of edges statisfying the requirement that all
	  cycles containing any one member of the set must contain all
	  members. A link traverse is a maximal link subset whose edges
	  are connected end-to-end. In surveying, link traverses are
	  simply the traverses between loop junctions, and are what we need
	  to set up the observation matrix for least-squares estimation.

	  A biconnected component can be decomposed uniquely into links.
	  In our application of survey error analysis, these correspond to the
	  smallest survey components to which blunders can be isolated due
	  to inconsistency. We are concerned here only with link traverses,
	  however. They are referred to as "strings" in the code comments.

	  I may have gone overboard with comments below, but the method
	  involves the manipulation of some cryptic pointers and counters.
	  This function is called recursively, so to save stack space
	  I use statics or registers to hold temporary values that don't need
	  to be preserved across invocations. I've also made this a separate
	  module so that we can more conveniently hand optimize (or at least
	  check) the compiler's assembly output. The 16-bit version should
	  consume 10 bytes of stack space per call.*/

	register UINT p_adj;    /*Try preventing the compiler from making n a*/
	register int minlow;   /*register variable. This saves 2 bytes of stack.*/

	/*Temporary values thet need not be saved across recursive calls --*/
	static   UINT adj_sav;
	static   int low_sav;
	static   int Nxt_node, Low_val;

	/*Reuse p_adj to store current string index, renaming it for clarity*/
#define Cur_str p_adj

/*minlow keeps track of the smallest nodlow[] value assigned to any
  node adjacent to this node, except for the parent node (the previous
  node we just traversed from). We initialize it to the value just
  assigned to this node by the caller.*/

	minlow = nodlow[n];

	/*Depth-first search is implemented by exploring thoroughly, in
	  turn, each route leading from the current node. Backtracking will
	  occur only when previously traversed nodes (with nodlow[]>0) are
	  encountered. When a new node is found, it is initially assigned
	  a nodlow[] value of one greater than that of the node it was reached
	  from, then the search continues recursively from that point on.
	  When all routes from the current node are exhausted, nodlow[] is
	  reset to the smallest nodlow[] value encountered or assigned in
	  the search at or beyond this point. We then backtrack by returning
	  this value to the calling routine whose search is based at the
	  parent node.*/

	for (p_adj = net_node[n]; p_adj < net_node[n + 1]; p_adj++) {

		/*Skip routes already traversed.*/

		if (net_valist[p_adj] == (DWORD)-1) continue;

		/*Identify this string as a pointer to the node's corresponding
		  adjacency list entry.*/

		  /*This is now possible --*/
		if (net_numstr == net_maxstringvecs) {
			net_errno = NET_ErrMaxStrVecs;
			return 0;
		}

#ifdef _DEBUG
		if (net_numstr >= net_maxnumstr) net_maxnumstr = net_numstr;
#endif

		net_string[net_numstr] = p_adj;

		/*Obtain string endpoint node index. This node will have degree!=2.
		  unless we have found a one-string loop. extend_str() will set
		  the corresponding endpoint node's adjacency list entry to -1 so
		  the route cannot be retraced backwards. It also sets nodlow[]=-1
		  for each of the intermediate nodes. If a coordinate file is
		  being created, extend_str() also estalishes the next sequence
		  of vector endpoints to be printed or displayed.*/

		if ((Nxt_node = extend_str(net_valist[p_adj])) == -1) return 0;

		/*For now, fix net_vendp[] to identify string endpoints only. This
		  will facilitate merging of strings (when possible) after a
		  system is identified. The intermediate nodes (with nodlow[]==-1)
		  can be reattached later.*/

		net_vendp[net_valist[p_adj]] = Nxt_node;

		Cur_str = net_numstr++;
		Low_val = nodlow[Nxt_node];

		/*If the next nodes's Low_val is zero, it is a new string junction,
		  in which case we assign it a higher nodlow[] value and continue
		  the search recursively from that node by calling df_search().
		  The return value, Low_val, is then the smallest (nonzero) nodlow[]
		  value that was assigned from that point on.

		  Otherwise, if Low_val is nonzero, the current string is a closure
		  to a previously traversed system of nodes whose smallest assigned
		  nodlow[] value is Low_val. */

		if (!Low_val) {
			nodlow[Nxt_node] = nodlow[n] + 1;
			Low_val = df_search(Nxt_node);
			if (net_errno) return 0;
		}
		/*Increment closure count for a subsequent sanity check only --*/
		else if (Low_val != INT_MAX) net_closures++;

		/*At this point, Low_val is the smallest nodlow[] value reached
		  via the string just traversed. We must account for three
		  possible situations:

		  If Low_val<minlow, we have closed to nodes that were traversed
		  prior to first encountering this node, which means that the string
		  belongs to the same system as that of the string we came here by.
		  That system is still unresolved, and we set minlow=Low_val before
		  checking the next untraversed route.

		  Otherwise, if Low_val is the same as the nodlow[] value initially
		  assigned to this node, a new system is established containing the
		  string just traversed (Cur_str). This node is its attachment point
		  to the rest of the network. We call add_system() to move all
		  strings beyond Cur_str in array net_string[] to the sys[] array.

		  Finally, if Low_val is *greater* than the nodlow[] value assigned
		  to this node, the string is not part of a (biconnected) loop
		  system, but leads to other systems already resolved. We discard it
		  by resetting net_numstr, thereby removing it from the net_string[]
		  array.*/

		  /*Use static to recover and save adjacency list pointer, since
			p_adj is still in use (as Cur_str) and add_system(), if called,
			will modify net_string[Cur_str].*/

		adj_sav = net_string[Cur_str];

		if (Low_val < minlow) minlow = Low_val;
		else {
			if (Low_val == nodlow[n]) {
				low_sav = Low_val;        /*add_system() overwrites nodlow[n]*/
				net_stindex = Cur_str;    /*use global to pass starting string index*/
				add_system();           /*add strings to sysstr[] array*/
				nodlow[n] = low_sav;
				net_numstr = net_stindex; /*remove strings from net_string[] array*/
			}
			else if (Low_val > nodlow[n]) net_numstr = (int)Cur_str;
		}

		/*Restore the adjacency list pointer so that the next adjacent
		  string (if untraversed) can be explored.*/

		p_adj = adj_sav;
	}

	/*After exploring all routes from this node, set the node's nodlow[]
	value to the smallest one encountered beyond this point. Backtrack
	by returning to the parent node (calling routine) with this value.*/

	return(nodlow[n] = minlow);
}

#else

/*Non-recursive version --*/

apfcn_i df_search(int n)
{
	UINT p_adj;
	int minlow;
	UINT adj_sav;

	static int df_lowval;

#define Cur_str p_adj

	if (n < 0) {
		if (!df_ptr--) return 0;  /*Search finished*/
		n = df_stack[df_ptr].n;
		p_adj = df_stack[df_ptr].p_adj;
		minlow = df_stack[df_ptr].minlow;
		/*df_lowval set on previous "return -1" below*/
		goto _inc_end;
	}

	/*df_nxtnode set by caller, or before "return 1" below*/
	n = df_nxtnode;
	minlow = nodlow[n];
	p_adj = net_node[n];

_loopstart:

	if (p_adj >= net_node[n + 1]) goto _loopend;

	/*Skip routes already traversed.*/
	if (net_valist[p_adj] == (DWORD)-1) goto _cont;

	/*This is now possible --*/
	if (net_numstr == net_maxstringvecs) {
		net_errno = NET_ErrMaxStrVecs;
		return 0;
	}

#ifdef _DEBUG
	if (net_numstr >= net_maxnumstr) net_maxnumstr = net_numstr;
#endif

	net_string[net_numstr] = p_adj;

	if ((df_nxtnode = extend_str(net_valist[p_adj])) == -1) return 0;

	net_vendp[net_valist[p_adj]] = df_nxtnode;
	Cur_str = net_numstr++;

	if (df_lowval = nodlow[df_nxtnode]) {
		if (df_lowval != INT_MAX) net_closures++;
		goto _inc_end;
	}

	/*Recursive call equivalent is here - search to greater depth --*/
	nodlow[df_nxtnode] = nodlow[n] + 1;
	if (df_ptr == df_lenstack &&
		!(df_stack = (df_frame *)realloc(df_stack, (df_lenstack += DF_STACK_INCSIZE) * sizeof(df_frame))))
	{
		net_errno = NET_ErrNearHeap;
		return 0;
	}
	df_stack[df_ptr].n = n;
	df_stack[df_ptr].p_adj = p_adj;
	df_stack[df_ptr].minlow = minlow;
	df_ptr++;
#ifdef _DEBUG
	if (df_ptr > df_ptr_max) df_ptr_max = df_ptr;
#endif
	return 1;

_inc_end:
	adj_sav = net_string[Cur_str];

	if (df_lowval < minlow) minlow = df_lowval;
	else {
		if (df_lowval == nodlow[n]) {
			net_stindex = Cur_str;    /*use global to pass starting string index*/
			add_system();           /*add strings to sysstr[] array*/
			nodlow[n] = df_lowval;    /*add_system() overwrites nodlow[n]*/
			net_numstr = net_stindex; /*remove strings from net_string[] array*/
		}
		else if (df_lowval > nodlow[n]) net_numstr = Cur_str;
	}

	p_adj = adj_sav;

_cont:
	p_adj++;
	goto _loopstart;

_loopend:
	df_lowval = nodlow[n] = minlow;
	return -1;
}

#endif

/*
This code is described in "Computational Geometry in C" (Second Edition),
Chapter 3.  It is not written to be comprehensible without the
explanation in that book.

Input: 2n integer coordinates of points in the plane. 
Output: the convex hull, cw, in PostScript; other output precedes the PS.

NB: The original array storing the points is overwritten.

Compile: gcc -o graham graham.c  (or simply: make)

Written by Joseph O'Rourke.
Last modified: October 1997
Questions to orourke@cs.smith.edu.
--------------------------------------------------------------------
This code is Copyright 1998 by Joseph O'Rourke.  It may be freely
redistributed in its entirety provided that this copyright notice is
not removed.
--------------------------------------------------------------------
*/

#include   "stdafx.h"

/*----------Point(s) Structure-------------*/

#define PMAX    8               /* Max # of points */
#define SIZSTACK 20

typedef struct {
   int     vnum;
   POINT   v;
   BOOL    deleted;
} tsPoint;

typedef tsPoint *tPoint;

/*----------Stack Structure-------------*/
typedef struct tStackCell tsStack; /* Used on in NEW() */
typedef tsStack *tStack;
struct tStackCell {
   tPoint   p;
   tStack   next;
};

/* Global variables */
static tsPoint P[PMAX];
static int n;                         /* Actual # of points */
static BOOL ndelete;                   /* Number deleted */
static tsStack stackbuf[SIZSTACK];

/*---------------------------------------------------------------------
FindLowest finds the rightmost lowest point and swaps with 0-th.
The lowest point has the min y-coord, and amongst those, the
max x-coord: so it is rightmost among the lowest.
---------------------------------------------------------------------*/
static void FindLowest(void)
{
   int m=0;   /* Index of lowest so far. */
   tsPoint p;

   for (int i=1;i<n;i++) {
      if((P[i].v.y <  P[m].v.y) ||
          ((P[i].v.y == P[m].v.y) && (P[i].v.x > P[m].v.x))) m=i;
   }
   if(m) {
	   p=P[0];
	   P[0]=P[m]; /* Swap P[0] and P[m] */
	   P[m]=p;
   }
}

/*---------------------------------------------------------------------
Compare: returns -1,0,+1 if p1 < p2, =, or > respectively;
here "<" means smaller angle.  Follows the conventions of qsort.
---------------------------------------------------------------------*/
static int AreaSign( POINT a, POINT b, POINT c )
{
    double area2;

    area2 = ( b.x - a.x ) * (double)( c.y - a.y ) -
            ( c.x - a.x ) * (double)( b.y - a.y );

    /* The area should be an integer. */
    if      ( area2 >  0.5 ) return  1;
    else if ( area2 < -0.5 ) return -1;
    else                     return  0;
}

static int Compare( const void *tpi, const void *tpj )
{
   int a;             /* area */
   int x, y;          /* projections of ri & rj in 1st quadrant */
   tPoint pi, pj;
   pi = (tPoint)tpi;
   pj = (tPoint)tpj;

   a = AreaSign( P[0].v, pi->v, pj->v );
   if (a > 0)
      return -1;
   else if (a < 0)
      return 1;
   else { /* Collinear with P[0] */
      x =  abs( pi->v.x -  P[0].v.x ) - abs( pj->v.x -  P[0].v.x );
      y =  abs( pi->v.y -  P[0].v.y ) - abs( pj->v.y -  P[0].v.y );

      ndelete=TRUE;
      if ( (x < 0) || (y < 0) ) {
         pi->deleted = TRUE;
         return -1;
      }
      else if ( (x > 0) || (y > 0) ) {
         pj->deleted = TRUE;
         return 1;
      }
      else { /* points are coincident */
         if (pi->vnum > pj->vnum)
             pj->deleted = TRUE;
         else 
             pi->deleted = TRUE;
         return 0;
      }
   }
}


/*---------------------------------------------------------------------
Pops off top elment of stack s, frees up the cell, and returns new top.
---------------------------------------------------------------------*/
static tStack Pop( tStack s )
{
   s->p=NULL;
   return s->next;
}

/*---------------------------------------------------------------------
Get a new cell, fill it with p, and push it onto the stack.
Return pointer to new stack top.
---------------------------------------------------------------------*/
static tStack Push( tPoint p, tStack top )
{
   for(int i=0;i<SIZSTACK;i++) if(stackbuf[i].p==NULL) {
	   stackbuf[i].p = p;
	   stackbuf[i].next = top;
	   return &stackbuf[i];
   }
   ASSERT(FALSE);
   return NULL;
}

/*---------------------------------------------------------------------
Returns twice the signed area of the triangle determined by a,b,c.
The area is positive if a,b,c are oriented ccw, negative if cw,
and zero if the points are collinear.
---------------------------------------------------------------------*/
inline int Area2( POINT a, POINT b, POINT c )
{
   return
          (b.x - a.x) * (c.y - a.y) -
          (c.x - a.x) * (b.y - a.y);
}

/*---------------------------------------------------------------------
Performs the Graham scan on an array of angularly sorted points P.
---------------------------------------------------------------------*/
static tStack Graham()
{
   tStack   top;
   int i;
   tPoint p1, p2;  /* Top two points on stack. */

   /* Initialize stack. */
   memset(stackbuf,0,sizeof(stackbuf));

   top = Push ( &P[0], NULL );
   top = Push ( &P[1], top );

   /* Bottom two elements will never be removed. */
   i=2;

   while (i<n) {
      p1 = top->next->p;
      p2 = top->p;
	  //Area2 returns >0 iff c is strictly to the left of the directed
	  //line through a to b.

      if (Area2(p1->v,p2->v,P[i].v)>0) {
         top = Push ( &P[i], top );
         i++;
      }
	  else {
		  top = Pop(top);
		  ASSERT(top->next);
		  if(!top->next) return NULL;
	  }
   }
   return top;
}

static void Squash( void )
{
   int i,j;

   for(i=j=0;i<n;i++) if(!P[i].deleted) P[j++]=P[i];
   n = j;
}

UINT ElimDups(POINT *pt,UINT cnt)
{
	UINT i,j;
	POINT p;

	for(i=0;i<cnt;i++) {
		p=pt[i];
		for(j=i+1;j<cnt;j++) if(pt[j].x==p.x && pt[j].y==p.y) pt[j--]=pt[--cnt];
	}
	return cnt;
}

UINT GetConvexHull(POINT *pt,UINT ptCnt)
{
   tStack top;
   
   ptCnt=ElimDups(pt,ptCnt);

   if(ptCnt<=3) return ptCnt;

   for(n=0;n<(int)ptCnt;n++) {
	  P[n].v=pt[n];
      P[n].vnum = n;
      P[n].deleted = FALSE;
   }

   FindLowest();
   ndelete=FALSE;

//#ifdef _USE_QSORT
   qsort(
      &P[1],             /* pointer to 1st elem */
      n-1,               /* number of elems */
      sizeof( tsPoint ), /* size of each elem */
      Compare            /* -1,0,+1 compare function */
   );
//#endif

   if(ndelete) Squash();

   top = Graham();
   ptCnt=0;
   while(top) {
	   pt[ptCnt++]=top->p->v;
	   top=top->next;
   }
   return ptCnt;
}



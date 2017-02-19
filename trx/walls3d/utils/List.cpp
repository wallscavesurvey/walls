// <copyright>
// 
// Copyright (c) 1993
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
// </copyright>

//<file>
//
// Name:       list.C
//
// Purpose:    Double linked list
//
// Created:     1 Oct 91    Gerald Pani
//
// Modified:   21 Jul 93    Gerald Pani
//
//
//
// Description:
//
//
//</file>

#include "list.h"

#if defined(_MSC_VER) & defined(_DEBUG)
#include "../pcutil/debug.h"
#define new DEBUG_NEW
#endif

DLList::DLList() {
     head_.next_ = &tail_;
     tail_.prev_ = &head_;
     count_ = 0;
}

void DLList::addHead( DLListNode* n) {
     n->next_ = head_.next_;
     head_.next_ = n;
     n->next_->prev_ = n;
     n->prev_ = &head_;
     count_++;
}

void DLList::addTail( DLListNode* n) {
     n->next_ = &tail_;
     n->prev_ = tail_.prev_;
     tail_.prev_ = n;
     n->prev_->next_ = n;
     count_++;
}

DLListNode* DLList::removeHead() {
     if(head_.next_->next_ == nil)
	  return nil;
     DLListNode* n = head_.next_;
     head_.next_ = n->next_;
     head_.next_->prev_ = &head_;
     count_--;
     return n;
}

DLListNode* DLList::removeTail() {
     if(tail_.prev_->prev_ == nil)
	  return nil;
     DLListNode* n = tail_.prev_;
     tail_.prev_ = n->prev_;
     tail_.prev_->next_ = &tail_;
     count_--;
     return n;
}

void DLList::remove( DLListNode* n) {
     n->prev_->next_ = n->next_;
     n->next_->prev_ = n->prev_;
     count_--;
}

void DLList::insertAfter( DLListNode* ins, DLListNode* after) {
     ins->next_ = after->next_;
     after->next_ = ins;
     ins->next_->prev_ = ins;
     ins->prev_ = after;
     count_++;
}

void DLList::insertBefore( DLListNode* ins, DLListNode* before) {
     ins->next_ = before;
     ins->prev_ = before->prev_;
     before->prev_ = ins;
     ins->prev_->next_ = ins;
     count_++;
}

void DLList::free() {
     DLListNode* n;
     for(; n = (DLListNode*)removeHead();) {
	  delete n;
     }
     count_ = 0;
}

DLListNode * DLList::operator []( int i) {
     if ((i<0) || (i>=count()))
	  return nil;
     DLListNode* n;
     if (i > count()/2) {
	  n = tail_.prev_;
	  for (int j=count()-1; j!=i; j--, n = n->prev_);
     }
     else {
	  n = head_.next_;
	  for (int j=0; j!=i; j++, n = n->next_);
     }
     return n;
}

void DLList::insertAt( DLListNode* ins, int pos) {
     if (pos<0) {
	  addHead( ins);
     }
     else {
	  if (pos>=count())
	       addTail( ins);
	  else
	       insertBefore( ins, operator[]( pos));
     }
     return;
}

static inline void swap( DLListNode** heap, int a, int b) {
     DLListNode* help = heap[a];
     heap[a] = heap[b];
     heap[b] = help;
}

static void makeHeap( DLListNode** heap, int i, int n) {
     register int d = 2*i;
     if (d > n) 
	  return;
     if (*heap[d-1] < *heap[i-1]) {
	  if (d == n) {
	       ::swap( heap, i-1, d-1);
	       ::makeHeap( heap, d, n);
	  }
	  else {
	       if (*heap[d] < *heap[i-1]) {
		    if (*heap[d] < *heap[d-1]) {
			 ::swap( heap, i-1, d);
			 ::makeHeap( heap, d+1, n);
		    }
		    else {
			 ::swap( heap, i-1, d-1);
			 ::makeHeap( heap, d, n);
		    }
	       }
	       else {
		    ::swap( heap, i-1, d-1);
		    ::makeHeap( heap, d, n);
	       }
	  }
     }
     else {
	  if (d != n) {
	       if (*heap[d] < *heap[i-1]) {
		    ::swap( heap, i-1, d);
		    ::makeHeap( heap, d+1, n);
	       }
	  }
     }
}

static DLListNode* getRoot( DLListNode** heap, int n) {
     DLListNode* help = heap[0];
     heap[0] = heap[n-1];
     ::makeHeap( heap, 1, n-1);
     return help;
}

void DLList::sort() {
     // ---- allocate array for heap
     int n = count();
     if (n < 2) 
	  return;
     DLListNode** heap = new DLListNode*[n];
     
     boolean ok = 1;
     // ---- initialize array
     DLListNode** h = heap;
     DLListNode* prev = nil;
	 int i = 0;
     for (; i < n; i++, h++) {
	  *h = removeHead();
	  if (prev && **h < *prev)
	       ok = 0;
	  prev = *h;
     }
     
     if (ok) {
	  h = heap;
	  for (i = 0; i < n; i++)
	       addTail(*h++);
	  delete heap;
	  return;
     }

     // ---- build heap
     for (i = n/2; i > 0; i--) {
	  ::makeHeap( heap, i, n);
     }

     // ---- rebuild list
     h = heap;
     for (i = n; i > 0; i--) {
	  addTail( ::getRoot( heap, i));
     }
     
     delete heap;
}


// /* ******************* PStack ************************ */
// /* Pointer-Stack */
// 
// void PStack::push( void* p) {
//      addHead( new PNode( p));
// }
// 
// void* PStack::pop() {
//      PNode* n = (PNode*)removeHead();
//      void* v = nil;
//      if(n) {
// 	  v = n->pointer_;
// 	  delete n;
//      }
//      return v;
// }
// 
// boolean PStack::empty() {
//      return (getFirst()) ? false : true;
// }
// 
// void * PStack::getLastPushed() {
//      PNode * n = (PNode*)getFirst();
//      if(n)
// 	  return n->pointer_;
//      else
// 	  return nil;
// }


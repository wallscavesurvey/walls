// <copyright>
// 
// Copyright (c) 1993
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
// </copyright>

//<file>
//
// Name:       gentreap.C
//
// Purpose:    Generic Treap - balanced binary search tree
//
// Created:    22 Aug 91    Gerald Pani
//
// Modified:   21 Jul 93    Gerald Pani
//
//
//
// Description:
//
//
//</file>

//be sure to NOT include several Windows-header files !!
#define WIN32_LEAN_AND_MEAN

#include "gentreap.h"

///////// TreapNode
TreapNode::TreapNode() {
     p_ = ::rand();
     left_ = right_ = nil;
}

TreapNode::~TreapNode() {
     delete left_;
     delete right_;
}


/////////// Treap
void Treap::rotateRight( TreapNode*& y) {
     TreapNode* x = y->left_;
     y->left_ = x->right_;
     x->right_ = y;
     y = x;
}

void Treap::rotateLeft( TreapNode*& x) {
     TreapNode* y = x->right_;
     x->right_ = y->left_;
     y->left_ = x;
     x = y;
}

const TreapNode* Treap::maxKeyNode() const {
     if (!root_)
	  return nil;
     if (!root_->right_)
	  return root_;
     return maxKeyNode( root_->right_);
}

const TreapNode* Treap::minKeyNode() const {
     if (!root_)
	  return nil;
     if (!root_->left_)
	  return root_;
     return minKeyNode( root_->left_);
}

const TreapNode* Treap::maxKeyNode( const TreapNode* n) const {
     if (!n->right_)
	  return n;
     return maxKeyNode( n->right_);
}

const TreapNode* Treap::minKeyNode( const TreapNode* n) const {
     if (!n->left_)
	  return n;
     return minKeyNode( n->left_);
}

void Treap::count( const TreapNode* node, int idx) {
     histogram[idx++] += 1;
     if (node->left_)
	  count( node->left_, idx);
     if (node->right_)
	  count( node->right_, idx);
}

ostream& operator << ( ostream& s, Treap & t ) {
     int i;
     for (i = 0; i < MAXHIST; t.histogram[i++] = 0);
     t.count( t.root_, 0);
     s << "\nTreapNodes per Level\n====================\n";
     s << "\n     Level";  
     s <<   "   Maximum";
     s <<   "    Actual\n";
     int k = 0,j=0;
	 i=0;
     for (; t.histogram[i]; i++) {
	  j += t.histogram[i];
	  k += (i+1)*t.histogram[i];
	  s << dec;
	  s.width(10);
	  s.fill(' ');
	  s << i; 
	  s.width(10);
	  s << (1<<i);
	  s.width(10);
	  s << t.histogram[i];
	  s << endl;
     }
     s << "\nTotal:";
     s.width(24);
     s << j << endl;
     s << "\nMean # of compares:" << float(k)/j << endl;
     return s;
}
    

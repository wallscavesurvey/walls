// This may look like C code, but it is really -*- C++ -*-

//<copyright>
// 
// Copyright (c) 1993
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:       gentreap.h
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
// Macros for creating treaps of abstract datatypes.
//
//</file>

#ifndef hg_utils_gentreap_h
#define hg_utils_gentreap_h

#ifdef SYSV
#include <math.h>
#endif

#include <stdlib.h>

#ifdef LINUX
#undef rand
#define rand random
#endif

#ifdef __MSDOS__
#include <iomanip.h>
#endif

#include <iostream>
using namespace std;

#include "..\generic.h"
#include "types.h"

#define MAXHIST 100

//<class>
//
// Name:       TreapNode
//
// Purpose:    Base class for all TreapNodes
//
//
// Public Interface:
//
// - TreapNode()
//   Default constructor. Sets up a TreapNode with no children and a
//   random number for the heap order.
//
// - virtual ~TreapNode()
//   Destructor. Deletes ALL storage used by the TreapNode itself and
//   calls the destructor of its children.
//
//
// Description:
//
// 
//</class>

class TreapNode {
public:
	TreapNode();
	virtual ~TreapNode();

	long p_;
	TreapNode* left_;
	TreapNode* right_;
};

//<class>
//
// Name:       Treap
//
// Purpose:    Base class for all Treaps
//
//
// Public Interface:
// 
// - const TreapNode* maxKeyNode() const
//   Pointer to the node with the greatest key.
// 
// - const TreapNode* minKeyNode() const
//   Pointer to the node with the smallest key.
//
// - void free()
//   Empty the tree. Call the destructor of all members.
//
// - boolean deleteFlag()
//   Destructor deletes all nodes if true.
//
// - void setDeleteFlag( boolean v)
//   Set the delete flag.
//
// 
// Protected Interface:
//
// - Treap()
//   Default constructor. Sets up a empty Treap.
//
// - ~Treap()
//   Destructor. Deletes ALL storage used by the Treap itself and
//   calls the destructor of its members.
//   
// - void RotateRight( TreapNode*&)
// 
// - void RotateLeft( TreapNode*&)
// 
//
// Description:
//
// 
//</class>

class Treap {
public:
	const TreapNode* maxKeyNode() const;
	const TreapNode* minKeyNode() const;
	void free() { delete root_; root_ = nil; };
	boolean deleteFlag() const { return delete_; };
	void setDeleteFlag(boolean v) { delete_ = v; };
protected:
	Treap() : root_(nil), delete_(1) {};
	~Treap() { if (delete_) delete root_; };
	void rotateRight(TreapNode*&);
	void rotateLeft(TreapNode*&);

	TreapNode* root_;

private:
	const TreapNode* maxKeyNode(const TreapNode*) const;
	const TreapNode* minKeyNode(const TreapNode*) const;
	friend ostream& operator <<(ostream& s, Treap & t); // gives statistics about the Treap
	void count(const TreapNode* node, int idx);
	int histogram[MAXHIST];

	boolean delete_;		// should the destructor delete all nodes
};


//<class>
//
// Name:       TreapNameNode
//
// Purpose:    Generic TreapNode
//
//
// Public Interface:
// 
// - TreapNameNode( const KeyType& key, const DataType& data)
//   Default constructor. Sets up a TreapNameNode with key and data.
// 
// - ~TreapNameNode()
//   Destructor. Calls ~TreapNode.
// 
//
// Description:
//
// 
//</class>

//<class>
//
// Name:       TreapName
//
// Purpose:    Generic Treap
//
//
// Public Interface:
// 
// - boolean find( const KeyType& key, DataType& data)
//   Searches for key and if found return true and update data.
// 
// - void insert( const KeyType& key, const DataType& data)
//   Insert key and data. If key exists, call
//   DataType::mergeDataList( data, olddata).
// 
// - boolean remove( const KeyType& key, DataType& data)
//   Like find, but remove the node.
// 
// - boolean maxKeyData( KeyType& key, DataType& data) const
//   Like find with the greatest key, but return key too.
// 
// - boolean minKeyData( KeyType& key, DataType& data) const
//   Like find with the smallest key, but return key too.
// 
// - boolean remMaxKeyData( KeyType& key, DataType& data) const
//   Like maxKeyData, but remove the node.
// 
// - boolean remMinKeyData( KeyType& key, DataType& data) const
//   Like minKeyData, but remove the node.
// 
// - void dfs( class TreapNameAction* action)
//   Run through the tree in DFS-order and call at each node
//   action->actionBefore(key, data) (actionMiddle, actionAfter)
//   before visiting the left son (after visiting the left son, after
//   visiting the right son).
// 
// 
// Description:
//
// 
//</class>

//<class>
//
// Name:       TreapNameAction
//
// Purpose:    Generic TreapAction
//
//
// Public Interface:
// 
// - virtual void actionBefore( const KeyType& key, DataType& data) = 0
// 
// - virtual void actionMiddle( const KeyType& key, DataType& data) = 0
// 
// - virtual void actionAfter( const KeyType& key, DataType& data) = 0
// 
// 
// Description:
//
// 
//</class>
#ifdef __BCPLUSPLUS__
#define Treapdeclare(TreapName, KeyType, DataType) \
Treap2declare(TreapName,_Paste2(TreapName,_Node),KeyType, DataType)
#else
#define Treapdeclare(TreapName, KeyType, DataType) \
Treap2declare(TreapName,name2(TreapName,_Node),KeyType, DataType)
#endif

#define Treap2declare(TreapName, TreapNameNode, KeyType, DataType) \
class TreapNameNode : public TreapNode { \
  public: \
     TreapNameNode( const KeyType&, const DataType&); \
     ~TreapNameNode(); \
     KeyType 	key_; \
     DataType 	data_; \
}; \
class name3(TreapName,Act,ion); \
class TreapName : public Treap { \
  public: \
     boolean 	find( const KeyType&, DataType&); \
     void 	insert( const KeyType&, const DataType&); \
     boolean 	remove( const KeyType&, DataType&); \
     boolean    maxKeyData( KeyType&, DataType&) const; \
     boolean    minKeyData( KeyType&, DataType&) const; \
     boolean    remMaxKeyData( KeyType&, DataType&); \
     boolean    remMinKeyData( KeyType&, DataType&); \
     void dfs( name3(TreapName,Act,ion)*);	     \
  protected: \
     TreapNameNode* 	find( TreapNameNode*&, const KeyType&); \
     void 	insert( TreapNameNode*&, const KeyType&, const DataType&); \
     void	remove( TreapNameNode*&, const KeyType&, TreapNameNode*&); \
     void dfs( class name3(TreapName,Act,ion)*, TreapNameNode*);	      \
 };									      \
									      \
class name3(TreapName,Act,ion) {						      \
  protected:								      \
   friend class TreapName;						      \
     virtual void actionBefore( const KeyType&, DataType&) = 0;	   	      \
     virtual void actionMiddle( const KeyType&, DataType&) = 0;		      \
     virtual void actionAfter( const KeyType&, DataType&) = 0;		      \
};

// **********************************************************
// Treapimplement
// ----------------------------------------------------------
// Implementation of the Treap-classes
// **********************************************************
#ifdef __BCPLUSPLUS__
#define Treapimplement(TreapName, KeyType, DataType) 			\
Treap2implement(TreapName,_Paste2(TreapName,_Node),KeyType, DataType)
#else
#define Treapimplement(TreapName, KeyType, DataType) 			\
Treap2implement(TreapName,name2(TreapName,_Node),KeyType, DataType)
#endif

#define Treap2implement(TreapName, TreapNameNode, KeyType, DataType) 				\
TreapNameNode::TreapNameNode( const KeyType& key, const DataType& data) : TreapNode() { 	\
     key_ = key; 								       		\
     data_ = data; 										\
} 												\
 												\
TreapNameNode::~TreapNameNode() { 	     				       		\
} 									       		\
 									       		\
TreapNameNode* TreapName::find( TreapNameNode*& tree, const KeyType& key) {    	\
     TreapNameNode* found = nil; 						\
     if (tree == nil) 								\
	  return nil; 								\
     if (key < tree->key_) { 							\
	  if (tree->left_) { 							\
	       found = find( (TreapNameNode*&)(tree->left_), key); 		\
	       if (tree->p_ < tree->left_->p_) 					\
		    rotateRight( (TreapNode*&)tree); 				\
	  } 									\
	  else 									\
	       return nil; 							\
     } 										\
     else if (key == tree->key_) { 						\
	  found = tree; 							\
	  long p = ::rand(); 							\
	  if (p > tree->p_) 							\
	       tree->p_ = p; 							\
     } 										\
     else { 									\
	  if (tree->right_) { 							\
	       found = find( (TreapNameNode*&)(tree->right_), key); 		\
	       if (tree->p_ < tree->right_->p_) 					\
		    rotateLeft( (TreapNode*&)tree); 				\
	  } 									\
	  else 									\
	       return nil; 							\
     } 										\
     return found; 								\
} 										\
 										\
boolean TreapName::find( const KeyType& key, DataType& data) { 	\
     TreapNameNode* node = find( (TreapNameNode*&)root_, key); 	\
     if (node) { 						\
	  data = node->data_; 					\
	  return TRUE; 						\
     } 								\
     return FALSE; 						\
} 								\
 								\
void TreapName::insert( const KeyType& key, const DataType& data) { 	\
     insert( (TreapNameNode*&)root_, key, data); 				\
} 									\
 									\
void TreapName::insert( TreapNameNode*& tree, const KeyType& key, const DataType& data) {	\
     if (!tree) { 										\
	  TreapNameNode* node = new TreapNameNode( key, data); 					\
	  tree = node; 										\
     } 										\
     else if (key < tree->key_) { 						\
	  insert( (TreapNameNode*&)(tree->left_), key, data); 			\
	  if (tree->p_ < tree->left_->p_) 						\
	       rotateRight( (TreapNode*&)tree); 					\
     } 										\
     else if (key == tree->key_) { 						\
	  tree->data_ = DataType::mergeDataList( data, tree->data_); 		\
     } 										\
     else { 									\
	  insert( (TreapNameNode*&)(tree->right_), key, data); 			\
	  if (tree->p_ < tree->right_->p_) 					\
	       rotateLeft( (TreapNode*&)tree); 					\
     } 										\
     return; 									\
} 										\
 										\
boolean TreapName::remove( const KeyType& key, DataType& data) { 	\
     TreapNameNode* node; 						\
     remove( (TreapNameNode*&)root_, key, node); 				\
     if (node) { 							\
	  data = node->data_; 						\
	  delete node; 							\
	  return TRUE; 							\
     } 									\
     return FALSE; 							\
} 									\
 									\
void TreapName::remove( TreapNameNode*& tree, const KeyType& key, TreapNameNode*& nptr) { 	\
     if (!tree) { 										\
	  nptr = nil; 										\
	  return; 										\
     } 												\
     if (key == tree->key_) { 									\
	  if (tree->left_ == nil && tree->right_ == nil) { 					\
	       nptr = tree; 									\
	       tree = nil; 									\
	       return; 										\
	  } 											\
	  else if (tree->left_ == nil) { 							\
	       rotateLeft( (TreapNode*&)tree); 							\
	       remove( (TreapNameNode*&)(tree->left_), key, nptr); 				\
	  } 											\
	  else if (tree->right_ == nil) { 							\
	       rotateRight( (TreapNode*&)tree); 							\
	       remove( (TreapNameNode*&)(tree->right_), key, nptr); 				\
	  } 											\
	  else { 										\
	       if (((TreapNameNode*)(tree->left_))->key_ < ((TreapNameNode*)(tree->right_))->key_) {\
		    rotateLeft( (TreapNode*&)tree); 						\
		    remove( (TreapNameNode*&)(tree->left_), key, nptr); 				\
	       } 										\
	       else { 										\
		    rotateRight( (TreapNode*&)tree); 						\
		    remove( (TreapNameNode*&)(tree->right_), key, nptr); 				\
	       } 										\
	  } 											\
     } 												\
     else if (key < tree->key_) { 								\
	  remove( (TreapNameNode*&)(tree->left_), key, nptr); 					\
     } 												\
     else { 											\
	  remove( (TreapNameNode*&)(tree->right_), key, nptr); 					\
     } 												\
} 												\
 												\
boolean TreapName::maxKeyData( KeyType& key, DataType& data) const { 	 	 \
     const TreapNameNode* node = (const TreapNameNode*) maxKeyNode();	 \
     if (node) {							 \
	  key = node->key_;						 \
	  data = node->data_;						 \
	  return TRUE;							 \
     }									 \
     return FALSE;							 \
}									 \
									 \
boolean TreapName::minKeyData( KeyType& key, DataType& data) const {	\
     const TreapNameNode* node = (const TreapNameNode*) minKeyNode();	\
     if (node) {							\
	  key = node->key_;						\
	  data = node->data_;						\
	  return TRUE;							\
     }									\
     return FALSE;							\
}									\
									\
boolean TreapName::remMaxKeyData( KeyType& key, DataType& data) {	\
     if (maxKeyData( key,data)) {   	    				\
          TreapNameNode* node; 						\
	  remove( (TreapNameNode*&)root_, key, node);			\
	  delete node;							\
	  return TRUE;							\
     }									\
     return FALSE;	    						\
}									\
									\
boolean TreapName::remMinKeyData( KeyType& key, DataType& data) {	\
     if (minKeyData( key,data)) {					\
          TreapNameNode* node; 						\
	  remove( (TreapNameNode*&)root_, key, node);			\
	  delete node;							\
	  return TRUE;							\
     }									\
     return FALSE;	    						\
}									      \
									      \
void TreapName::dfs( name3(TreapName,Act,ion)* action, TreapNameNode* node) {  \
     action->actionBefore( node->key_, node->data_);	       		      \
     if (node->left_)							      \
	  dfs( action, (TreapNameNode*)node->left_);				      \
     action->actionMiddle( node->key_, node->data_);			      \
     if (node->right_)							      \
	  dfs( action, (TreapNameNode*)node->right_);				      \
     action->actionAfter( node->key_, node->data_);			      \
}									      \
									      \
void TreapName::dfs( name3(TreapName,Act,ion)* action) {			      \
     if (!root_)								      \
	  return;							      \
     dfs( action, (TreapNameNode*)root_);					      \
}


#endif














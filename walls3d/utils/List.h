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
// Name:       list.h
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
// Double linked list and macros for generic double linked list
//
//</file>

#ifndef hg_utils_list_h
#define hg_utils_list_h

#include "../generic.h"
#include "types.h"


//<class>
//
// Name:       DLListNode
//
// Purpose:    Base class for all DLListNodes
//
//
// Public Interface:
//
// - DLListNode()
//   Default constructor. Sets up a DLListNode with no next and no previous.
//
// - virtual ~DLListNode()
//   Destructor. Deletes ALL storage used by the DLListNode.
//
// - virtual boolean operator <( DLListNode& n)
//   Return false by default.
//
//
// Description:
//
// 
//</class>

class DLListNode {
  public:
     DLListNode();     
     virtual ~DLListNode() {};

     virtual boolean operator <( DLListNode&) { return 0; };
     virtual DLListNode* getNextNode() { return next_; };
     virtual DLListNode* getPrevNode() { return prev_; };

  private:
     DLListNode* next_;
     DLListNode* prev_;
 friend class DLList;
};

inline DLListNode::DLListNode() { 
     next_ = nil;
     prev_ = nil;

}

//<class>
//
// Name:       DLList
//
// Purpose:    Base class for all DLLists
//
//
// Public Interface:
//
// - DLList()
//   Default constructor. Sets up an empty list.
//
// - ~DLList()
//   Destructor. Deletes ALL storage used by the DLList and calls 
//   ~DLListNode for all nodes in the list.
//
// - void free()
//   Removes all nodes of the list and calls ~DLListNode for each of them.
//
// - void addHead( DLListNode* node)
//   Insert node at the head of the list.
//
// - void addTail( DLListNode* node)
//   Insert node at the tail of the list.
//
// - void insertBefore( DLListNode* ins, DLListNode* bef)
//   Inserts ins before bef.
//
// - void insertAfter( DLListNode* ins, DLListNode* aft)
//   Inserts ins after aft.
//
// - void insertAt( DLListNode* ins, int pos)
//   Inserts ins at position pos.
//
// - DLListNode* removeHead()
//   Removes the first node of the list and returns a pointer to it.
//   If the list is empty, the returned value is nil.
//
// - DLListNode* removeTail()
//   Removes the last node of the list and returns a pointer to it.
//   If the list is empty, the returned value is nil.
//
// - void remove( DLListNode* n)
//   Removes the node n from the list.
//
// - DLListNode* getFirst()
//   Returns a pointer to the first node or nil if the list is empty.
//
// - DLListNode* getLast()
//   Returns a pointer to the last node or nil if the list is empty.
//
// - DLListNode* getNext( DLListNode* n)
//   Returns a pointer to the successor of n or nil if there isn't any.
//
// - DLListNode* getPrev( DLListNode* n)
//   Returns a pointer to the predecessor of n or nil if there isn't
//   any.
//
// - DLListNode* operator []( int i)
//   Returns a pointer to the node at position i or nil if position
//   isn't valid. 
//
// - int count() const
//   Returns the number of nodes in the list.
//
// - void sort()
//   Heapsort of the list, using DLListNode::operator<().
//
// - const DLListNode* first() const
//   Returns a constant pointer to the first node or nil if the list
//   is empty. 
//
// - const DLListNode* next( const DLListNode* n) const
//   Returns a constant pointer to the successor of n or nil if there
//   isn't any. 
//
//
// Description:
//
// 
//</class>

class DLList {
     DLListNode head_;
     DLListNode tail_;
     int count_;
  public:
     DLList();
     ~DLList();
     void free();			 // remove all nodes of the list and delete them 
     void addHead( DLListNode*); 
     void addTail( DLListNode*);
     void insertBefore( DLListNode* ins, DLListNode* bef);
     void insertAfter( DLListNode* ins, DLListNode* aft);
     void insertAt( DLListNode* ins, int pos);
     DLListNode* removeHead();		 // remove the first/last node of the list and 
     DLListNode* removeTail();		 // returns a pointer to it. When the list is empty, return nil 
     void remove( DLListNode* n);	 // remove the node n 
     DLListNode* getFirst();		 // return a pointer to the first/last node of the list.
     DLListNode* getLast();		 // When the list is empty, return nil.
     DLListNode* getNext( DLListNode* n); // return a pointer to the successor of n
     DLListNode* getPrev( DLListNode* n); // return a pointer to the predecessor of n
     DLListNode* operator []( int i);
     int count() const;
     void sort();
     const DLListNode* first() const;		 // return a pointer to the first/last node of the list.
     const DLListNode* next( const DLListNode* n) const; // return a pointer to the successor of n
};

inline int DLList::count() const {
     return count_;
}

inline DLListNode* DLList::getFirst() {
     return (head_.next_->next_) ? (head_.next_) : (nil);
}

inline const DLListNode* DLList::first() const {
     return (head_.next_->next_) ? (head_.next_) : (nil);
}

inline DLListNode* DLList::getPrev(DLListNode* n) {
     return (n->prev_->prev_) ? (n->prev_) : nil;
}

inline DLListNode* DLList::getLast() {
     return (tail_.prev_->prev_) ? (tail_.prev_) : nil;
}

inline DLListNode* DLList::getNext(DLListNode* n) {
     return (n->next_->next_) ? (n->next_) : nil;
}

inline const DLListNode* DLList::next(const DLListNode* n) const {
     return (n->next_->next_) ? (n->next_) : nil;
}

inline DLList::~DLList() {
     free();
}

//<class>
//
// Name:       GenDLList
//
// Purpose:    Generic DLList
//
//
// Public Interface:
//
// - GenDLListNode* removeHead()
//   Removes the first node of the list and returns a pointer to it.
//   If the list is empty, the returned value is nil.
//
// - GenDLListNode* removeTail()
//   Removes the last node of the list and returns a pointer to it.
//   If the list is empty, the returned value is nil.
//
// - GenDLListNode* getFirst()
//   Returns a pointer to the first node or nil if the list is empty.
//
// - GenDLListNode* getLast()
//   Returns a pointer to the last node or nil if the list is empty.
//
// - GenDLListNode* getNext( DLListNode* n)
//   Returns a pointer to the successor of n or nil if there isn't any.
//
// - GenDLListNode* getPrev( DLListNode* n)
//   Returns a pointer to the predecessor of n or nil if there isn't
//   any.
//
// - GenDLListNode* operator []( int i)
//   Returns a pointer to the node at position i or nil if position
//   isn't valid. 
//
// - const GenDLListNode* first() const
//   Returns a constant pointer to the first node or nil if the list
//   is empty. 
//
// - const GenDLListNode* next( const DLListNode* n) const
//   Returns a constant pointer to the successor of n or nil if there
//   isn't any. 
//
//
// Description:
// No casting of returned pointers is necessary.
//
// 
//</class>

#define DLListdeclare(GenDLList,GenDLListNode)				      \
class GenDLList : public DLList {					      \
  public:								      \
     GenDLListNode* removeHead();					      \
     GenDLListNode* removeTail();					      \
     GenDLListNode* getFirst();					      \
     GenDLListNode* getLast();						      \
     GenDLListNode* getNext( DLListNode* n);				      \
     GenDLListNode* getPrev( DLListNode* n);				      \
     GenDLListNode* operator [](int i);				      \
     const GenDLListNode* first() const;				      \
     const GenDLListNode* next( const DLListNode* n) const;		      \
};									      \
									      \
inline GenDLListNode* GenDLList::getFirst() {				      \
     return (GenDLListNode*)(DLList::getFirst());			      \
}									      \
									      \
inline const GenDLListNode* GenDLList::first() const {			      \
     return (const GenDLListNode*)(DLList::first());			      \
}									      \
									      \
inline GenDLListNode* GenDLList::getPrev( DLListNode* n) {		      \
     return (GenDLListNode*)(DLList::getPrev( n));			      \
}									      \
									      \
inline GenDLListNode* GenDLList::getLast() {				      \
     return (GenDLListNode*)(DLList::getLast());			      \
}									      \
									      \
inline GenDLListNode* GenDLList::getNext( DLListNode* n) {		      \
     return (GenDLListNode*)(DLList::getNext( n));			      \
}									      \
									      \
inline const GenDLListNode* GenDLList::next( const DLListNode* n) const {    \
     return (const GenDLListNode*)(DLList::next( n));			      \
}									      \
									      \
inline GenDLListNode* GenDLList::removeHead() {			      \
     return (GenDLListNode*)(DLList::removeHead());			      \
}									      \
									      \
inline GenDLListNode* GenDLList::removeTail() {				      \
     return (GenDLListNode*)(DLList::removeTail());			      \
}									      \
									      \
inline GenDLListNode* GenDLList::operator []( int i) {			      \
     return (GenDLListNode*)(DLList::operator[]( i));			      \
}


// /* ******************* PNode ************************ */
// /* Listenknoten mit Pointer auf irgendetwas */
// 
// class PNode : public DLListNode {
//   public:
//      PNode(void*);
// 
//      void* pointer_;
// };
// 
// inline PNode::PNode( void* p) : DLListNode() {
//      pointer_ = p;
// } 
// 
// /* ******************* PStack ************************ */
// /* Pointer-Stack */
// class PStack : private DLList {	/* Stack mit PNodes */
//   public:
//      void push( void* p);	/* p ist der Pointer aus PNode */
//      void* pop();		/* Return nil, wenn Stack leer */
//      boolean empty();
//      void* getLastPushed();
// };
#endif






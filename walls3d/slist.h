/*
 * File    :  slist.h
 *
 * Author  :  Keith Andrews, IICM, TU Graz, Austria
 *
 * Changes :  Michael Hofer (until May 92) and Michael Pichler
 *
 * Changed :   7 Feb 95
 *
 */


#ifndef slist_h
#define slist_h


struct snode
{
	void* data;
	snode* next;
};


class slist
{
public:
	slist() { head = tail = curr = 0; }
	~slist() { clear(); }

	int isempty() const                // list empty?
	{
		return head == 0;
	}
	int isnotempty() const             // list not empty?
	{
		return head != 0;
	}

	void* first()                      // return pointer to first item
	{
		curr = head;  return current();
	}
	void* last()                       // return pointer to last item
	{
		curr = tail;  return current();
	}
	void* next()                       // return pointer to next item
	{
		curr = curr ? curr->next : 0;  return current();
	}
	void* current() const              // return pointer to current item
	{
		return curr ? curr->data : 0;
	}

	void put(void* item);              // put item at end of list

	void removeFirst();                // remove first item

	void clear();                      // Delete entire list structure

private:
	snode* head;                        // first node
	snode* tail;                        // last node
	snode* curr;                        // current node

};  // slist


#endif

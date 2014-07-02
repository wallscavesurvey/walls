/*
 * File    :  slist.C
 *
 * Author  :  Keith Andrews, IICM, TU Graz, Austria
 *
 * Changes :  Michael Hofer (until May 92) and Michael Pichler
 *
 * Changed :   7 Feb 95
 *
 */


#include "slist.h"
#include "memleak.h"


void slist::put (void *item)            // append item to end of list
{
  if (!item)  // ignore nil item pointer
    return ;

  register snode *tmp = New snode;
  tmp->data = item ;
  tmp->next = 0 ;

  if (tail) 
  { tail->next = tmp;
    tail = tmp;
  }
  else
    head = tail = tmp;
} // put


void slist::removeFirst ()              // remove first item
// NB: does not call delete on data
{
  if (head)
  {
    snode* oldhead = head;
    head = head->next;
    if (curr == oldhead)
      curr = head;
    if (tail == oldhead)
      tail = head;
    delete oldhead;
  }
} // removeHead


void slist::clear ()                    // Delete entire list structure
// NB: does not call delete on data
{
  snode *ptr = head;
  snode *tmp;

  while (ptr) 
  { tmp = ptr;
    ptr = ptr->next;
    Delete tmp;
  }

  head = tail = curr = 0;
} // clear

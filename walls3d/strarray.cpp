//<copyright>
// 
// Copyright (c) 1994
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        strarray.C
//
// Purpose:     implementation of class StringArray
//
// Created:     28 Feb 94   Michael Pichler
//
// Changed:      2 Aug 94   Michael Pichler
//
//
//</file>



#include "strarray.h"

#include <string.h>
#include <iostream>



// generate an array of strings by an item string,
// which contains the items separated by white space

StringArray::StringArray (const char* items)
{
  numitems_ = 0;

  if (!items)
  { data_ = 0;
    item_ = 0;
    return;
  }

  int slen = strlen (items);

  data_ = new char [slen+1];
  strcpy (data_, items);
  int firstchar = 1;  // flag: first char after WS counts

  // find individual strings and substitute WS by '\0' bytes
  int i = slen;
  char* sptr = data_;
  while (i--)
  {
    if (*sptr == ' ' || *sptr == '\t' || *sptr == '\n')
    { *sptr = '\0';
      firstchar = 1;
    }
    else
    { if (firstchar)
        numitems_++;
      firstchar = 0;
    }
    sptr++;
  }  

//cerr << numitems_ << " items for StringArray" << endl;

  item_ = (const char**) new char* [numitems_];

  // each item_ is pointer into data_
  i = 0;  // for all items
  sptr = data_;

  while (i < numitems_)
  {
    if (*sptr)
    { item_ [i++] = sptr;
      sptr += strlen (sptr);
    }
    else
      sptr++;
  }

  // assert: current_ always in range [-1..numitems_-1]
  // -1 to access first/last item first with first call next/previousItem
  current_ = -1;

} // StringArray



StringArray::~StringArray ()
{
  delete[] data_;
  delete[] item_;
}



void StringArray::printItems (ostream& os, char separator) const
{
  // print all items (dump format)
  for (int i = 0;  i < numitems_;  i++)
    os << item_ [i] << separator;
}



ostream& operator << (ostream& os, const StringArray& s)
{
  // print all items, list format: "item item item"

  int i, num = s.numitems_ - 1;  // index of last item

  for (i = 0;  i < num;  i++)  // allbutlast
    os << s.item_ [i] << ' ';

  if (s.numitems_)  // last
    os << s.item_ [i];

  return os;
}



int StringArray::contains (const char* item) const
{
  int i = numitems_;
  const char** items = item_;

  while (i--)
    if (!strcmp (item, *items++))
      return 1;

  return 0;
}



// nextItem () is not really a const member
// since the next call of nextItem will return the next item;
// nevertheless owners of a const StringArray* should be able to
// access nextItem without having rights to delete the class etc.
// returns next item (circular), initially begins with first item

const char* StringArray::nextItem () const
{
  if (!numitems_)
    return 0;

  StringArray* my = (StringArray*) this;

  if (++my->current_ == numitems_)
    my->current_ = 0;

  return item_ [current_];
}



// previousItem ()
// same remarks as for nextItem apply
// returns previous item (circular), initially begins with last item

const char* StringArray::previousItem () const
{
  if (!numitems_)
    return 0;

  StringArray* my = (StringArray*) this;

  if (--my->current_ < 0)
    my->current_ = numitems_-1;

  return item_ [current_];
}

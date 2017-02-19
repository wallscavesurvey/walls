// This may look like C code, but it is really -*- C++ -*-

//<copyright>
// 
// Copyright (c) 1994
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        strarray.h
//
// Purpose:     interface to simple array of strings
//
// Created:     28 Feb 94   Michael Pichler
//
// Changed:     14 Mar 94   Michael Pichler
//
//
//</file>



#ifndef hg_viewer_scene_strarray_h
#define hg_viewer_scene_strarray_h



//class ostream;
#include <iostream>
using namespace std;

class StringArray
{
  public:
    StringArray (const char* items);    // items are separated by whitespace
//  StringArray (const char* items, char separator);
    ~StringArray ();

    void printItems (ostream&, char separator = ' ') const;  // dump all items
    friend ostream& operator << (ostream&, const StringArray&);  // list output

    const char* item (int i) const;     // return item i (nil when out of range)
    int numItems () const  { return numitems_; }

    const char* nextItem () const;      // next item (circular)
    const char* previousItem () const;  // previous item (circular)

    int contains (const char* item) const;  // does string array contain item?

  private:
    char* data_;
    const char** item_;
    int numitems_;
    int current_;

    // prevent copying (declared, but not implemented)
    StringArray (const StringArray&);
    StringArray& operator = (const StringArray&);
};


inline const char* StringArray::item (int i) const
{
  return (i >= 0 && i < numitems_) ? item_ [i] : 0;
}


#endif

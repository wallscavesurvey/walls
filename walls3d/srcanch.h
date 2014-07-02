// This may look like C code, but it is really -*- C++ -*-

//<copyright>
// 
// Copyright (c) 1994,95
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        srcanch.h
//
// Purpose:     interface to source anchor (of a geometric 3D object)
//
// Created:      1 Jun 94   Michael Pichler
//
// Changed:     31 Jul 95   Michael Pichler
//
//
//</file>


#ifndef hg_viewer_scene_srcanch_h
#define hg_viewer_scene_srcanch_h


#include "utils/str.h"


class SourceAnchor
{
  public:
    SourceAnchor (                      // constructor
      long id,                          //   anchor id
      const RString& aobject,           //   anchor object
      const char* groupname             //   group name (if NULL, object anchor)
    );
    ~SourceAnchor ();

    const char* groupName () const      // get group name
    { return groupname_; }

    long id () const                    // get anchor id
    { return anchorid_; }

    const RString& object () const      // get anchor object
    { return anchorobj_; }

  private:
    char* groupname_;
    long anchorid_;
    RString anchorobj_;
};  // SourceAnchor



#endif

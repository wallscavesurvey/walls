//<copyright>
// 
// Copyright (c) 1994,95,96
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        scene3d.C (formerly scene.C)
//
// Purpose:     implementation of class scene3D, which manages SceneData
//
// Created:     18 Mar 92   Michael Hofer and Michael Pichler
//
// Changed:     23 Feb 96   Alexander Nussbaumer (editing)
// Changed:      1 Feb 96   Michael Pichler
//
// $Id: scene3d.C,v 1.22 1996/03/14 15:10:11 mpichler Exp mpichler $
//
//</file>


#ifndef __PC__
#define MOSAIC  /* define this for anchor/help support via xmosaic */
#define NETSCAPE  /* define this for anchor/help support via netscape */
#endif

#include "scene3d.h"
#include "memleak.h"

#include "sdfscene.h"
#include "geomobj.h"
#include "srcanch.h"
#include "camera.h"

#include "vrmlscene.h"
#include "qvlib/QvDB.h"
#include "qvlib/QvInput.h"
#include "qvlib/QvWWWAnchor.h"
#include "qvlib/QvString.h"

#include "vecutil.h"
#include "ge3d.h"

#include "hyperg/message.h"
#include "hyperg/verbose.h"
//#ifdef MOSAIC
//#include "hyperg/doccache/common/inetsocket.h"
//#endif
//#ifndef __PC__
//#include <hyperg/WWW/HTParse.h>
//#endif
#include "utils/str.h"

#include <ctype.h>
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#pragma warning(disable:4305)

// clipping planes
float Scene3D::clipFarFactor = 5.0;    // far clipping plane: 5 to 10 times scene size
float Scene3D::clipNearRatio = 100.0;  // ratio far/near (determines Z-buffer accuracy)
                                       //   (should be increased accroding to Z-buffer depth)
float Scene3D::clipNearMaximum = 1.0;  // upper bound for near clipping plane

// anchor highlighting intensities
float Scene3D::anchors_minbrightness = 0.7;
float Scene3D::nonanch_maxbrightness = 0.3;


// helpers for fetching data from URL and issuing browser commands

void fetchMosaic (const char* url);
// void mosaicCommand (const char* commandline);  // TODO
void fetchNetscape (const char* url);
void netscapeRemote (const char* cmd);
void netscapeCommand (const char* commandline);


/***** Mosaic *****/

void fetchMosaic (const char* url)
{
#ifdef MOSAIC
  const char* ccihint = "mosaic must be running and listening to a CCI port (menu File/CCI)";

  RString fname (getenv ("HOME"));
  fname += "/.mosaiccciport";
  FILE* ccifile = fopen (fname, "r");

  if (!ccifile)
  { cerr << "walls3d could not open $HOME/.mosaiccciport\n" << ccihint << endl;
    return;
  }
  char line[256];  *line = '\0';
  fgets (line, 256, ccifile);

  char* colpos = strchr (line, ':');
  if (!colpos)
  { cerr << "walls3d expected line 'HOSTNAME:PORTNUMBER' in $HOME/.mosaiccciport\n" << ccihint << endl;
    return;
  }
  *colpos++ = '\0';  // so line contains hostname and colpos portnumber
  int portnum = atoi (colpos);

  DEBUGNL ("opening CCI socket connection to xmosaic on host " << line << ", port " << portnum);
  INETSocket* socket = new INETSocket (line, portnum);
  if (!socket->ok ())
  { cerr << "walls3d socket connection to xmosaic failed on host " << line << ", port " << portnum << '\n';
    cerr << ccihint << endl;
    return;
  }

//     FILE* sockfile = fdopen (socket->fd (), "r");
//     fgets (line, 256, sockfile);
//     DEBUG ("mosaic CCI identification: " << line);  // includes '\n'

  char commandline [256];
  sprintf (commandline, "GET URL <%s> OUTPUT CURRENT\r\n", url);
  DEBUGNL ("walls3d issuing CCI request: " << commandline);

  // put the request
  socket->write (commandline, strlen (commandline));

// if this is activated for debugging, walls3d will be locked until the request is finished
//     fgets (line, 256, sockfile);
//     DEBUG ("mosaic CCI response: " << line);  // includes '\n'

  // (socket handled properly in its destructor)
#else
  cerr << "error: no support for mosaic compiled into this version of walls3d." << endl;
#endif
} // fetchMosaic


/***** Netscape *****/


void fetchNetscape (const char* url_in)
{
  RString url (url_in);
  // substitute special characters
  url.subst ("\"", "%22", false);  // secure netscape remote call
  url.subst ("'", "%27", false);   // and mailcap handling of URL
  url.subst (",", "%2c", false);   // netscape treats , as argument separator on remote call

  RString commandline = "netscape -remote 'openURL(" + url + ")'";
  // mpichler, 19960312: changed from " to ' around openURL:
  // $ etc. will no longer be interpreted by shell
  // no space between openURL and "("
  // sprintf (commandline, "netscape -remote \"openURL(%s)\"", url.string ());

  netscapeCommand (commandline.string ());
}


void netscapeRemote (const char* cmd)
{
  char commandline [256];
  sprintf (commandline, "netscape -remote \"%s\"", cmd);
  netscapeCommand (commandline);
}


void netscapeCommand (const char* commandline)
{
#ifdef NETSCAPE
  char line [256];

  // command gets parsed by a shell
  DEBUGNL ("walls3d issuing command: " << commandline);

  // could also use system() routine when output of command is not specially treated
  FILE* subprocess = popen (commandline, "r");
  if (!subprocess)
  { cerr << "walls3d could not execute '" << commandline << "'\n";
    cerr << "probably no netscape installed" << endl;
    // this only happens when netscape cannot be executed
    return;
  }

  while (fgets (line, 256, subprocess))  // will produce output only in case of errors
    fputs (line, stderr);

  int status = pclose (subprocess);
  if (status)
  { cerr << "walls3d could not execute '" << commandline << "'\n";
    cerr << "netscape must be running to handle remote requests" << endl;
  }
#else
  cerr << "error: no support for netscape compiled into this version of walls3d." << endl;
#endif
} // netscapeCommand



/***** Scene3D *****/


Scene3D::Scene3D ()
{ 
  data_ = 0;
  *m_FirstComment=0; //DMcK
  winaspect_ = 4.0/3.0;

  mode_ = ge3d_smooth_shading;
  modeInt_ = same_mode;
  interact_ = 0;
  twosidedpolys_ = twosided_auto;
  dolighting_ = lighting_auto;
  viewinglight_ = 0;  // automatically on for files without light
  texlighting_ = 1;  // default: do texture lighting (same as ge3d default)
  linksactive_ = 0;
  linkscolor_ = l_brightness;
  arbitraryrot_ = 1;  // default: allow arbitrary camera rotations
  colldetection_ = 1;  // default: coll. detection enabled
  scenemanipulated_ = 0;  // virgin scene is not edited (anuss)

  init3D (worldmin_, 0, 0, 0);
  init3D (worldmax_, 1, 1, 1);
  size_ = 1.0;
  selectedobj_ = NULL;
  selectedgroup_ = NULL;
  selectednode_ = NULL;
  selectedanchor_ = NULL;
  usemosaic_ = 0;  // default: netscape

  *pathtofiles_ = '\0';
  curfilename_ = 0;
  recenturl_ = 0;
  helpdir_ = new RString ();

  // default colours
  initRGB (col_background, 0.75, 0.75, 0.75);  // 75 % grey
  initRGB (col_hudisplay, 0.5, 0, 1.0);  // purple
  initRGB (col_anchorface, 1.0, 0.25, 0);  // red-brown
  initRGB (col_anchoredge, 1.0, 1.0, 0.25);  // yellow
  initRGB (col_viewinglight, 1.0, 1.0, 1.0);  // white

  // ge3d_initialise ();  // no window opened yet
} // Scene3D


Scene3D::~Scene3D ()
{
  clear ();
  delete helpdir_;
}


void Scene3D::clear ()
{
  if (data_)
  {
    // most recent URL not deleted for first scene to make -URL argument work
    delete recenturl_;
    recenturl_ = 0;

    delete data_;
    data_ = 0;
  }

  delete curfilename_;
  curfilename_ = 0;

  ge3dBackgroundColor (&col_background);  // user background

  if (selectedobj_ || selectedgroup_)
  {
    selectedobj_ = NULL;
    selectedgroup_ = NULL;
    selectionChanged ();
  }
  if (selectednode_ || selectedanchor_)
  {
    selectednode_ = NULL;
    selectedanchor_ = NULL;
    selectionChanged ();
  }

} // clear



// ***** readScene *****

// short form
// filepath should be the path and the name of the scene description file
// (if there is no extension present, .sdf is appended)
// returns 0 if no error
// returns errorcode otherwise

int Scene3D::readSceneFile (const char* filepath)
{
  char path [256], filename [64];

  strcpy (path, filepath);
  char* slpos = strrchr (path, '/');    // last slash in path

  if (slpos)  // "path/.../filename"
  { strcpy (filename, slpos + 1);
    *++slpos = '\0';  // path ends after '/'
  }
  else  // filename only: current directory
  { *path = '\0';
    strcpy (filename, filepath);
  }

  // char* ptpos = strrchr (filename, '.');  // search dot in filename (without extension)
  // if (!strchr (filename, '.'))
  //   strcat (filename, ".sdf");  // anachronism

  int retval = readScenePath (path, filename);

  delete curfilename_;
  curfilename_ = new RString (filepath);

  return retval;

} // readSceneFile


void Scene3D::setCurrentFilename (const char* name)
{
  delete curfilename_;
  curfilename_ = new RString (name);
}


// readScene - long form
// pathtofiles should be "" (current directory), or end with "/", or NULL (meaning that
// the directory of a previous call to readScene is used [pathtofiles_]).

int Scene3D::readScenePath (const char* pathtofiles, const char* fname)
{
//cerr << "*** reading scene {" << (long) this << "} ***" << endl;

  char filename [256];  // including path

  if (pathtofiles)  // save path
    strcpy (pathtofiles_, pathtofiles);

  strcpy (filename, pathtofiles_);
  strcat (filename, fname);

  FILE* file;  // open file

  if (!(file = fopen (filename, "r")))
  { cerr << "Scene Viewer. Error: scene file '" << filename << "' not found." << endl;
    return (100);  // file not found
  }

  int retval = readSceneFILE (file);
  fclose (file);

  return retval;

} // readScenePath



// readScene - from a stream (FILE*)
// caller responsible for closing the file
// caller responsible for decompression (OS dependent)

int Scene3D::readSceneFILE (FILE* file)
{
  // anuss
  if (scenemanipulated_ && !continueOnSaveState ())  // possibly save changed scene now
    return 0;  // cancel, no error

  // clear (delete) old data
  clear ();
  int errcode = 0;

  // check input format and create correct subclass of SceneData

  int chr = getc (file);
  ungetc (chr, file);

  if (chr == '#')  // evaluate header line
  {
    QvDB::init ();
    QvInput in;
    in.setFilePointer (file);

    if (in.getVersion ())  // check header
    {
	  strcpy(m_FirstComment,in.FirstComment); //DMcK
      DEBUGNL ("Scene Viewer: QvLib: valid VRML header");
      VRMLScene* vrmlscene = new VRMLScene (this);
      errcode = vrmlscene->readInput (in);
      data_ = vrmlscene;
    }
    else
    {
      DEBUGNL ("Scene Viewer: QvLib: invalid VRML header; assuming SDF file");
      SDFScene* sdfscene = new SDFScene (this);
      errcode = sdfscene->readFile (file);
      data_ = sdfscene;
    }
  }
  else if (chr == EOF)  // empty file
  { HgMessage::error ("empty scene file");
    return 0;  // no error
  }
  else  // no header line
  {
    HgMessage::message ("warning: scene file without header, assuming SDF");
    SDFScene* sdfscene = new SDFScene (this);
    errcode = sdfscene->readFile (file);
    data_ = sdfscene;
  }

  if (errcode)  // destroy invalid scenes immediately
  {
    delete data_;
    data_ = 0;
  }

  return errcode;
} // readSceneFILE


void Scene3D::saveVRMLSceneFile (ostream& os)
{ //anuss:
  if (data_->writeData (os, write_VRML))  // save current data as VRML
    scenemanipulated_ = 0;  // changes successfully saved
}


// formatName

const char* Scene3D::formatName () const
{
  if (data_)
    return data_->formatName ();
  return 0;
}


// mostRecentURL - return the URL most recently invoked
// or "" if unknown

const char* Scene3D::mostRecentURL () const
{
  if (recenturl_)
    return (const char*) *recenturl_;
  return (const char*) RString::lambda ();
}


// currentURL - the current URL to be returned by mostRecentURL

void Scene3D::currentURL (const char* url)
{
  delete recenturl_;
  recenturl_ = new RString (url);
}



// requestTextures
// stand-alone version requests all texture files to be loaded at once
// overridden by integrated Harmony Scene Viewer, which sends requests to database

void Scene3D::requestTextures ()
{
  if (data_)
    data_->loadTextures ();
}


void Scene3D::loadTextureFile (Material*, const char*)
{
// reading texture files (i.e. pixmaps) is better done by GUI libraries
}


// readInlineVRML (QvWWWInline*, const char*)
// pass the work over to readInlineVRML (FILE*)
// derived class should care for decompression

int Scene3D::readInlineVRMLFile (QvWWWInline* node, const char* filename)
{
/*  DEBUGNL ("Scene3D::readInlineVRML from file " << filename);

  FILE* file;
  if (!(file = fopen (filename, "r")))
  { HgMessage::error ("could not read temporary VRML WWWInline file");
    return 0;
  }

  int rval = readInlineVRML (node, file);

  fclose (file);

  return rval;*/
  return 0;
}


// readInlineVRML (QvWWWInline*, FILE*)
// read inline VRML data into node
// derived class should give progress feedback

int Scene3D::readInlineVRMLFILE (QvWWWInline* node, FILE* file)
{
/*  if (data_)
    return data_->readInlineVRML (node, file);
*/
  return 0;
}


void Scene3D::errorMessage (const char* str) const
{
  cerr << str;  // derived class may redirect them into a dialog
  // newline etc. should be part of string
}


// output internal scene data in write_... format

int Scene3D::writeScene (ostream& os, int format)
{
  if (!os || !data_)
    return -1;

  if (format != write_ORIGINAL)  // export current scene
    return data_->writeData (os, format);

  // save original input data
  return writeOriginalScene (os);
}


int Scene3D::writeOriginalScene (ostream& os)
{
  if (!curfilename_)  // don't know which file to save
    return -1;

  return copyFile (fopen (*curfilename_, "r"), os);
}


int Scene3D::supportsOutputformat (int format)
{
  DEBUG ("supportsOutputformat request for " << format);
  DEBUGNL ("; current filename: " << (curfilename_ ? (const char*) *curfilename_ : (const char*) "NONE"));

  if (format == write_ORIGINAL)  // can save original file when filname known
    return (curfilename_ != 0);

  // check possibility to export to desired format
  return (data_ && data_->supportsOutputformat (format));
}


int Scene3D::copyFile (FILE* infile, ostream& os)
{
  if (!infile || !os)
    return -1;

  int numchr;

  const int copybufsize = 4096;
  char copybuf [copybufsize];
  while ((numchr = fread (copybuf, 1, copybufsize, infile)) > 0)
    os.write (copybuf, numchr);

  fclose (infile);

  return 0;
}


// Scene3D::printInfo

void Scene3D::printInfo (int all)
{
  if (data_)
    data_->printInfo (all);
  else
    cout << "no scene loaded." << endl;
}


// get number of faces (polygons)

unsigned long Scene3D::getNumFaces () const
{
  if (data_)
    return data_->getNumFaces ();
  return 0L;
}

unsigned long Scene3D::getNumPrimitives () const
{
  if (data_)
    return data_->getNumPrimitives ();
  return 0L;
}


// find object by name

GeometricObject* Scene3D::findObject (const char* objname)
{
  if (data_)
    return data_->findObject (objname);
  return 0;
}



// find object by object number

GeometricObject* Scene3D::findObject (int objnum)
{
  if (data_)
    return data_->findObject (objnum);
  return 0;
}



Material* Scene3D::findMaterial (const char* matname)
{
  if (data_)
    return data_->findMaterial (matname);
  return 0;
}



// clearanchors
// clear all anchors of the scene

void Scene3D::clearAnchors ()
{
  if (data_)
    data_->clearAnchors ();
}



// defineAnchor
// associate anchor with id with a geometric object, optional group anchor

void Scene3D::defineAnchor (GeometricObject* obj, long id, const RString& aobj, const char* group)
{
  if (obj)
  { DEBUG ("Scene3D: anchor for object " << obj->name () << " with id " << id);
    DEBUGNL ((group ? " and group " : " (object anchor)") << (group ? group : ""));
    obj->setAnchor (id, aobj, group);
  }
  else
  { DEBUGNL ("Scene3D: anchor with id " << id
             << " for non-existent object (ignored)");
  }
}


// little helper function
// to compose the full anchor URL
// Gerbert, can you use this too?

void Scene3D::anchorURL (const QvWWWAnchor* anchor, RString& anchorurl)
{
/*  if (!anchor)
    return;

  RString url (anchor->name.value.getString ());

  const char* comma;
  if (usemosaic_)
    comma = ",";
  else  // netsace uses "," as remote argument separator; must escape it
    comma = "%2c";

  if (anchor->map.value == QvWWWAnchor::POINT)  // add "?x,y,z" to URL
  {
    const point3D& hitpoint = anchor->hitpoint_;
    char pointstr [64];
    // write "," as "%2c", because "," is used to separate arguments in netscape API
    sprintf (pointstr, "?%g%s%g%s%g", hitpoint.x, comma, hitpoint.y, comma, hitpoint.z);
    url += pointstr;
  }

  // convert local URLs to global ones for relative requests
  const char* parenturl = anchor->parentURL_.getString ();
  char* absurl = HTParse (url, parenturl, PARSE_ALL);

  anchorurl = absurl;  // string returned

  free (absurl);  // free string returned by HTParse
*/
} // anchorURL


// this function should be in main.C - it is here
// because it is currently used by both the Harmony and UNIX/WWW version of walls3d

// followLink (VRML)
int Scene3D::followLink (const QvWWWAnchor* anchor)
{
#ifndef __PC__
  if (!anchor)
    return 0;  // no redraw

  RString absurl;
  anchorURL (anchor, absurl);

  DEBUGNL ("walls3d following link to URL " << absurl
       << " (" << anchor->description.value.getString () << ")");

  if (usemosaic_)  // xmosaic
    fetchMosaic (absurl);
  else  // netscape
    fetchNetscape (absurl);
#endif

  return 0;  // no redraw
} // followLink



// requestInlineURL
// request a VRML scene by a URL that will be merged (inlined) into the current scene
// should be implemented by derived class
// soon dead code (hopefully)

void Scene3D::requestInlineURL (QvWWWInline*, const char* url, const char* docurl)
{
  cerr << "requesting inline URL " << url << " for document " << docurl << '\n'
       << "  (not supported by stand-alone scene viewer)" << endl;
}



// this function should be in main.C - it is here
// because it is currently used by both the Harmony and UNIX/WWW version of walls3d

void Scene3D::showHelp (const char* file)
{
#ifndef __PC__
  if (!helpdir_->length ())  // default: $HOME/.walls3d/help
  { *helpdir_ = getenv ("HOME");
    *helpdir_ += "/.walls3d/help";
  }

  RString url ("file:");
  url += *helpdir_;
  url += "/";

  if (file)
    url += file;
  else
    url += "walls3dhlp.html";

  DEBUGNL ("walls3d retrieving help from " << url);

  if (usemosaic_)  // xmosaic
    fetchMosaic (url);
  else  // netscape
    fetchNetscape (url);
#endif
} // showHelp


// pickObject (viewing coordinates)
// fx (horizontal): 0 = left, 1 = right margin; fy (vertical): 0 = bottom, 1 = top margin

void* Scene3D::pickObject (
  float fx, float fy,
  GeometricObject** gobj,
  QvNode** node,
  QvWWWAnchor** anchor,
  point3D* hitpoint, vector3D* normal,
  const StringArray** groups,
  float* hittime
)
{
  Camera* cam = data_ ? data_->getCamera () : 0;

  if (cam)
  {
    point3D A;
    vector3D b;
    float tnear, tfar;
    cam->viewingRay (fx, fy, A, b, tnear, tfar);

    return data_->pickObject (A, b, tnear, tfar, gobj, node, anchor, hitpoint, normal, groups, hittime);
  }
  return 0;
}


// pickObject (arbitrary direction)

void * Scene3D::pickObject (
  const point3D& A, const vector3D& b,
  float tnear, float tfar,
  GeometricObject** gobj,
  QvNode** node,
  QvWWWAnchor** anchor,
  point3D* hitpoint, vector3D* normal,
  const StringArray** groups,
  float* hittime
)
{
  if (data_)
    return data_->pickObject (A, b, tnear, tfar, gobj, node, anchor, hitpoint, normal, groups, hittime);
  return 0;
}


// selectObj - SDF version
// select a geometric object (which is specially highlighted),
// unless a NULL pointer is passed,
// always unselect the previously selected object and group
// returns non zero, iff the selection changed
// to select a group the object to which the group belongs must
// be given, because group names are local to geometric objects

int Scene3D::selectObj (GeometricObject* obj, const char* group)
{
  if (obj == selectedobj_ && group == selectedgroup_)
    return 0;  // no change

  if (selectedobj_)
    selectedobj_->unselect ();
    // no need to checkAnchorSelection because only selected object relevant for selected anchor

  if (obj)
  { obj->select (group);
    obj->checkAnchorSelection ();
  }

  selectedobj_ = obj;
  selectedgroup_ = group;

  selectionChanged ();
  return 1;  // change
}


// selectNode - VRML version
// select a node (which may be an anchor) unless NULL
// always remove old selection
// returns non zero, iff the selection changed

int Scene3D::selectNode (QvNode* node, QvWWWAnchor* anchor)
{
  if (node == selectednode_ && anchor == selectedanchor_)
    return 0;  // no change

/*
  // TODO: node/anchor highlighting
  if (selectednode_)
    selectednode_->unselect ();

  if (node)
    node->select ();
    // TODO: poss. need equivalent for checkAnchorSelection?
*/
  selectednode_ = node;
  selectedanchor_ = anchor;

  selectionChanged ();
  return 1;  // change
}


// setSourceAnchor
// define selected object as source anchor
// just prints the number of the selected object
// should be overridden by derived class

void Scene3D::setSourceAnchor ()
{
  if (selectedobj_)
  { cout << "Position=Object " << selectedobj_->getobj_num ();
    if (selectedgroup_)
      cout << '/' << selectedgroup_;
    cout << endl;
  }
  else
    cout << "no source object/group selected." << endl;
}


// deleteSourceAnchor
// delete selected source anchor
// does not delete the anchor from the internal data structure
// because assumes that anchors are redefined after this call

void Scene3D::deleteSourceAnchor ()
{
  if (selectedobj_)
    deleteSourceAnchor (selectedobj_->getCurrentAnchor ());
  else
  { DEBUGNL ("no object selected - no source anchor to delete.");
  }
}


// deleteSourceAnchor
// just prints the ID of the selected anchor
// should be overridden by derived class

void Scene3D::deleteSourceAnchor (const SourceAnchor* anchor)
{
  if (anchor)
    cout << "I would delete the selected anchor with id " << anchor->id () << endl;
  else
    cout << "object, but no source anchor selected to be deleted." << endl;
}


// setDestinationAnchor
// (just prints current view)
// should be overriden by derived class
// to define current view as destination anchor

void Scene3D::setDestinationAnchor ()
{
  Camera* cam = getCamera ();

  if (cam)
  {
    cout << "Position= ";
    point3D p;
    cam->getposition (p);
    cout << p << ' ';
    cam->getlookat (p);
    cout << p << endl;
  }
  else
    cout << "no camera active." << endl;
}


// setDefDestAnchor
// (does nothing)
// should be overriden by derived class
// to define default view as destination anchor

void Scene3D::setDefDestAnchor ()
{
}


// history functions: back, forward, history
// this code will move to main.C when followLink implementation moves there
// TODO: implement this also for mosaic (DOCOMMAND BACK/FORWARD 0) when available

void Scene3D::back ()
{
#ifdef __PC__
  cerr << "supposed to go back in history" << endl;
#else
  netscapeRemote ("back()");
#endif
}

void Scene3D::forward ()
{
#ifdef __PC__
  cerr << "supposed to go forward in history" << endl;
#else
  netscapeRemote ("forward()");
#endif
}

void Scene3D::history ()
{
#ifdef __PC__
  cerr << "supposed to show up history" << endl;
#else
  netscapeRemote ("history()");
#endif
}

// hold current document

void Scene3D::hold ()
{
  cerr << "supposed to hold current viewer" << endl;
}

// document or anchor information

void Scene3D::docinfo (int)
{
  cerr << "supposed to print document information" << endl;
}

// info on current anchor

void Scene3D::anchorinfo ()
{
  if (selectedobj_)
    anchorInfo (selectedobj_->getCurrentAnchor ());
}

// info on specific anchor

void Scene3D::anchorInfo (const SourceAnchor*)
{
  cerr << "supposed to print anchor attributes" << endl;
}


Camera* Scene3D::getCamera () const
{
  if (data_)
    return data_->getCamera ();
  return 0;
}


void Scene3D::storeCamera () const
{
  if (data_)
    data_->storeCamera ();
}


void Scene3D::restoreCamera () const
{
  if (data_)
    data_->restoreCamera ();
}


void Scene3D::activateCamera (
  const char* name,
  QvPerspectiveCamera* pcam, QvOrthographicCamera* ocam
)
{
  if (data_)
    data_->activateCamera (name, pcam, ocam);
}


// anachronisms getCameraPosition, getCameraLookat, setCameraParams removed
// (call the equivalent functions on the active camera)

void Scene3D::colorRebuild ()
{
  if (data_)
    data_->colorRebuild ();
}


// draw

void Scene3D::draw ()
{
  ge3dBackgroundColor(&col_background);
  ge3d_clearscreen ();

  if (data_)
    data_->draw (currentMode ());

} // Scene3D::draw

//<copyright>
// 
// Copyright (c) 1993,94,95,96
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        readscene.C
//
// Purpose:     implementation of reading routines for class SDFScene
//
// Created:     18 Mar 92   Michael Hofer and Michael Pichler
//
// Changed:     16 Jan 96   Michael Pichler
//
// $Id: readscene.C,v 1.3 1996/01/30 12:31:28 mpichler Exp $
//
//</file>



#include "sdfscene.h"
#include "scene3d.h"
#include "memleak.h"


#include "slist.h"
#include "geomobj.h"
#include "polyhed.h"
#include "sdfcam.h"
#include "light.h"
#include "dummyobj.h"
#include "material.h"

#include "ge3d.h"
#include "hyperg/message.h"
#include "hyperg/verbose.h"

#include <ctype.h>
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#pragma warning(disable:4305)

//#include <unistd.h>
// exec*, fork


/* reading files */

#define MAXLINELEN  255
// for actfile, camfile, mtlfile

#define FILE_SENTINEL '@'
// marker between logical file parts


// readActFile
// all other files are read after complete actor file is read
// returns 0 if no error
// returns errorcode >= 1 otherwise
// also computes the total number of channels

int SDFScene::readActFile (
  FILE* file,
  int& totalnoofchannels
)
{
  totalnoofchannels = 0;

  char line [MAXLINELEN];
  while (fgets (line, MAXLINELEN, file) && strncmp (line, "---", 3));

  // check syntax here (leading #, one line with @) !!!

  Polyhedron* polyhptr;
  SDFCamera* camptr;
  Light* lghtptr;
  DummyObject* dummyobjptr;

  int o_num,  // temporary values read from actfile
      o_parent,
      o_noofchannels;
  char o_name [64],
       o_filename [64],
       o_type [16],
       o_rotprior [4],
       o_tranprior [4],
       o_dispstat [8],
       o_color [64];


  while (fscanf (file, "%d %s %s %s %d %s %s %s %d %s\n",
         &o_num, o_name, o_filename, o_type, &o_parent, o_rotprior,
         o_tranprior, o_dispstat, &o_noofchannels, o_color) == 10)
  {
    if (!strcmp (o_dispstat, "OFF"))  // not displayed objects are dummies
      if (strcmp (o_type, "cam"))     // but store all cameras
        strcpy (o_type, "dum");

    if (!strcmp (o_type, "obj"))  // polyhedron
    { 
      Polyhedron* copyptr = 0;

      if (*o_filename == '&')  // copy of another polyhedron
      {
        int master = atoi (o_filename+1);
        // cerr << "Polyhedron " << o_num << " is a copy of Polyhedron " << master << '.' << endl;

        // search polyhedron master in gobjlist - copyptr
        for (GeometricObject* agobj = (GeometricObject*) gobjlist_.first ();  agobj;
                              agobj = (GeometricObject*) gobjlist_.next ())
          if (agobj->getobj_num () == master)
          { copyptr = (Polyhedron*) agobj;
            // cerr << "Master found." << endl;
            break;  // for
          }

        if (!copyptr)
          cerr << "hg3d. error: polyhedron " << o_num << " is copy of non-existent polyhedron " << master << endl;
      }
      // else  cerr << "Polyhedron " << o_num << " is a new one." << endl;

      polyhptr = New Polyhedron (o_num, o_parent, o_name, copyptr);
      strcpy (polyhptr->transfprior_, o_tranprior);
      strcpy (polyhptr->rotprior_, o_rotprior);
      polyhptr->no_of_channels_ = o_noofchannels;

      // set color: RGBCMY color coding if no material specified
      // could be made by e.g. calling readcolor ("", o_color)

//       rgbstruct* polyhcolor = polyhptr->color_;
//       switch (toupper (*o_color))
//       { case 'R':  polyhcolor->r = 1.0;  polyhcolor->g = 0.0;  polyhcolor->b = 0.0;  break;
//         case 'G':  polyhcolor->r = 0.0;  polyhcolor->g = 1.0;  polyhcolor->b = 0.0;  break;
//         case 'B':  polyhcolor->r = 0.0;  polyhcolor->g = 0.0;  polyhcolor->b = 1.0;  break;
//         case 'C':  polyhcolor->r = 0.0;  polyhcolor->g = 1.0;  polyhcolor->b = 1.0;  break;
//         case 'M':  polyhcolor->r = 1.0;  polyhcolor->g = 0.0;  polyhcolor->b = 1.0;  break;
//         case 'Y':  polyhcolor->r = 1.0;  polyhcolor->g = 1.0;  polyhcolor->b = 0.0;  break;
//         default:   polyhcolor->r = 1.0;  polyhcolor->g = 1.0;  polyhcolor->b = 1.0;  break;
//       }

      gobjlist_.put ((void*) polyhptr);
      allobjlist_.put ((void*) polyhptr);
      num_gobj_++;
    }
    else if (!strcmp (o_type, "cam"))  // camera
    { 
      camptr = New SDFCamera (o_num, o_parent, o_name);
      strcpy (camptr->transfprior_, o_tranprior);
      strcpy (camptr->rotprior_, o_rotprior);
      camptr->no_of_channels_ = o_noofchannels;
      camptr->dispstat_ = (tolower (o_dispstat [1]) != 'f'); 
      // set color
      camlist_.put ((void*) camptr);
      allobjlist_.put ((void*) camptr);
      num_cam_++;
    }
    else if (!strcmp (o_type, "lgt"))  // light
    { 
      lghtptr = New Light (o_num, o_parent, o_name);
      strcpy (lghtptr->transfprior_, o_tranprior);
      strcpy (lghtptr->rotprior_, o_rotprior);
      lghtptr->no_of_channels_ = o_noofchannels;
      // set color
      lightlist_.put ((void*) lghtptr);
      allobjlist_.put ((void*) lghtptr);
      num_light_++;
    }
    else  // all other objects are considered as dummies
    { 
      if (strcmp (o_type, "dum") && strcmp (o_type, "rfl"))
        cerr << "hg3d. Warning: object type '" << o_type
             << "' in actorfile unknown; treated as dummy." << endl;
      dummyobjptr = New DummyObject (o_num, o_parent, o_name);
      strcpy (dummyobjptr->transfprior_, o_tranprior);
      strcpy (dummyobjptr->rotprior_, o_rotprior);
      dummyobjptr->no_of_channels_ = o_noofchannels;
      allobjlist_.put ((void*) dummyobjptr);
    }

    num_objects_++;
     
    totalnoofchannels += o_noofchannels;
  }  // while

  // line with FILE_SENTINEL just read (hope so)

  if (!num_objects_)
  { cerr << "hg3d. syntax error in actor part (or empty)." << endl;
    fclose (file);
    return 1;
  }

  return 0;  // no error
}  // readActFile



// readPosFile
// returns 0 if no error
// returns errorcode >= 10 otherwise

int SDFScene::readPosFile (FILE* file, int maxlinelen)
{
  char* line1 = New char [maxlinelen];
  char* line1ptr = line1;

  int c;
  while ((c = fgetc (file)) != EOF && (char) c != '|');

  if (!fgets (line1, maxlinelen, file))
  { HgMessage::error ("unexpected EOF or syntax error (missing |) in position file part.");
    return (11);  // invalid posfile
  }

  while ((c = fgetc (file)) != EOF && (char) c != '\n');  //  ignore units of measurement

  // check for appropriate order

  while ((c = fgetc (file)) != EOF && (char) c != '|');

  Object3D* objptr;
  int i;
  int okay = 1;

  for (objptr = (Object3D*) allobjlist_.first (); objptr && okay;
       objptr = (Object3D*) allobjlist_.next ())
  {
    for (i = 1; i <= objptr->no_of_channels_ && okay; i++)
    { int val1, val2;
      if (fscanf (file, "%d/%d", &val1, &val2) != 2 ||
          val1 != objptr->obj_num_ || val2 != i)
        okay = 0;
    }
  } // for

  if (!okay)
    HgMessage::message ("warning: unexpected order of arguments in position data.");

  while ((c = fgetc (file)) != EOF && (char) c != '|');

  int channelno = 0;  // number of detected channel
  float channelvalue;

  for (objptr = (Object3D*) allobjlist_.first (); objptr;
       objptr = (Object3D*) allobjlist_.next ())
  {
    for (i = 1; i <= objptr->no_of_channels_; i++)
    {
      while (*line1ptr == ' ')  // ignore spaces
        line1ptr++;

      switch (*line1ptr++)
      {
        case 'S':  case 's':
          channelno = Object3D::ch_scale;
        break;

        case 'X':  case 'x':
          switch (*line1ptr++)
          { case 'T':  case 't':  channelno = Object3D::ch_xtran;   break;
            case 'S':  case 's':  channelno = Object3D::ch_xscale;  break;
            case 'R':  case 'r':  channelno = Object3D::ch_xrot;    break;
          }
        break;  // X

        case 'Y':  case 'y':
          switch (*line1ptr++)
          { case 'T':  case 't':  channelno = Object3D::ch_ytran;   break;
            case 'S':  case 's':  channelno = Object3D::ch_yscale;  break;
            case 'R':  case 'r':  channelno = Object3D::ch_yrot;    break;
          }
        break;  // Y

        case 'Z':  case 'z':
          switch (*line1ptr++)
          { case 'T':  case 't':  channelno = Object3D::ch_ztran;   break;
            case 'S':  case 's':  channelno = Object3D::ch_zscale;  break;
            case 'R':  case 'r':  channelno = Object3D::ch_zrot;    break;
          }
        break;  // Z

      }  // switch *line1ptr

      while (!isspace (*line1ptr))
        line1ptr++;

      fscanf (file, "%g", &channelvalue);

      objptr->channel_ [channelno] = channelvalue;  // setchannel (channelno, channelvalue);

      // cerr << "channel " << objptr->getobj_num () << '/' << i << " of kind "
      //      << channelno << " set to " << channelvalue << endl;

     }  // for channel i
  }  // for objptr

  Delete line1;

  // should ignore line with FILE_SENTINEL !!!

  return (0);  // no error

} // readPosFile




// readCamFile
// returns 0 if no error
// returns errorcode >= 30 otherwise

int SDFScene::readCamFile (FILE* file)
{
  char line [MAXLINELEN];
  while (fgets (line, MAXLINELEN, file) && strncmp (line, "---", 3));

  // report syntax error above !!!

  SDFCamera* camptr;
  int c_num;
  char c_name [32];
  char c_prjtype;
  float c_focallen,
        c_aper,
        c_aspect,
        c_left, c_top, c_right, c_bottom,
        c_hither, c_yon;

  while (fscanf (file, "%d %s %c %g %g %g %g %g %g %g %g %g\n",
                 &c_num, c_name, &c_prjtype, &c_focallen, &c_aper, &c_aspect,
                 &c_left, &c_right, &c_bottom, &c_top, &c_hither, &c_yon) == 12)
  { // scan the camera list for the appropriate camera c_num
    for (camptr = (SDFCamera*) camlist_.first ();  camptr && camptr->getobj_num () != c_num;
         camptr = (SDFCamera*) camlist_.next ());
    if (!camptr)
      cerr << "hg3d. Warning: camera no. " << c_num << " not found in camlist\n";
    else
    { camptr->projtype_ = c_prjtype;
      camptr->focallen_ = c_focallen;
      camptr->aspect_ = c_aspect;
      camptr->aper_ = c_aper / c_aspect;  // note: we use 'aper' for viewport heigt, but WF for width
      camptr->left_ = c_left;
      camptr->top_ = c_top;
      camptr->right_ = c_right;
      camptr->bottom_ = c_bottom;
      camptr->hither_ = c_hither;
      camptr->yon_ = c_yon;
    }  // else
  }  // while

  fgets (line, MAXLINELEN, file);  // ignore line with FILE_SENTINEL

  return 0;

} // readCamFile



// readMtlFile
// stores materials in materlist
// returns 0 if no error
// returns errorcode >= 50 otherwise

int SDFScene::readMtlFile (FILE* file)
{
  //cerr << "readMtlFile (" << filename << ")" << endl;

  char line [MAXLINELEN];

  while (fgets (line, MAXLINELEN, file) && *line != FILE_SENTINEL)  // until EOF or next file part
  {
    if (!strncmp (line, "newmtl", 6))  // start new material
    {
      float r = 0.7, g = 0.7, b = 0.7;
      char amaterialname [64];  *amaterialname = '\0';
      char texturename [64];  *texturename = '\0';

      sscanf (line, "newmtl %s", amaterialname);
      // cerr << "found material " << amaterialname << endl;      

//       do  // search Kd
//       { if (!fgets (line, MAXLINELEN, file) || *line == FILE_SENTINEL)
//           return 0;  // EOF or next file part
//       } while (strncmp (line, "Kd ", 3));

      for (;;)  // read all entries of this material
      {
        if (!fgets (line, MAXLINELEN, file))
          return 0;  // unexpected EOF

        if (*line == '\n' || *line == FILE_SENTINEL)  // emty line or separator
          break;  // material definition completed

        if (strstr (line, "map_Kd"))  // diffuse colour
          sscanf (line, "map_Kd %s\n", texturename);
        else if (strstr (line, "Kd"))  // texture map
          sscanf (line, "Kd %f %f %f\n", &r, &g, &b);
        // other entries currently ignored
      }

      materlist_.put ((void*) new Material (amaterialname, r, g, b, texturename));

      if (*line == FILE_SENTINEL)  // end of material part
        return 0;

    } // if

  } // while

  return 0;

} // readMtlFile



// readLgtFile
// on syntax error default light intensity is used, always returns 0

int Light::readLgtFile (FILE* file)
{
  char line [MAXLINELEN];
  int found = 0;

  while (fgets (line, MAXLINELEN, file) && *line != FILE_SENTINEL)  // until EOF or next file part
  {
    if (!strncmp (line, "intensity", 9))  // read intensity: r [g b]
    { if (sscanf (line, "intensity %g %g %g", &intensity_.R, &intensity_.G, &intensity_.B) == 1)
        intensity_.G = intensity_.B = intensity_.R;
      found = 1;
    }
    if (!strncmp (line, "directional", 11))  // directional (infinite) light
      positional_ = 0.0;  // spotlights are also treated as infinite light sources
  }

  if (!found)
    cerr << "hg3d. warning: could not find intensity for light '" << name () << "'." << endl;

  return 0;

}  // Light::readLgtFile




// readFile - read SDF file
// FILE* sdffile is assumened to point to an existing file
// report progress while reading
// caller responsible for closing file

int SDFScene::readFile (FILE* sdffile)
{
  int res = 0;
  int totalnoofchannels = 0;

  if (num_objects_)
  { HgMessage::error ("reading SDF scene over existent one (internal error).");
    return -1;
  }

  scene_->progress (0.0, Scene3D::progr_actfile);

  // DEBUGNL ("readScene: actor file part ");
  // ignore comment header and
  // read actor file to build gobjlist_, camlist_, lightlist_, allobjlist_
  res = readActFile (sdffile, totalnoofchannels);
  if (res)
  { cerr << "[hg3d. Error no. " << res << " on reading actor file.]\n";
    return (res);
  }
  scene_->progress (0.05, Scene3D::progr_posfile);

  // read channel values from positon file
  res = readPosFile (sdffile, totalnoofchannels * 10 + 16);
  if (res)
  { cerr << "[hg3d. Error no. " << res << " in readPosFile.]\n";
    return (res);
  }
  scene_->progress (0.10, Scene3D::progr_camfile);

  // read all cameras from camera file into camlist_
  res = readCamFile (sdffile);
  if (res)
  { cerr << "[hg3d. Error no. " << res << " in readCamFile.]\n";
    return (res);
  }
  scene_->progress (0.15, Scene3D::progr_mtlfile);

  // read all material definitions into materlist_
  res = readMtlFile (sdffile);
  if (res)
  { cerr << "[hg3d. Error no. " << res << " on reading materials.]\n";
    return (res);
  }
  scene_->progress (0.20, Scene3D::progr_lgtfile);

  // read all light sources (lightlist_)
  Light* alight;
  for (alight = (Light*) lightlist_.first ();  !res && alight;
       alight = (Light*) lightlist_.next ())
  { res = alight->readLgtFile (sdffile);
  }
  if (res)
  { cerr << "[hg3d. Error no. " << res << " on reading light source.]\n";
    return (res);
  }
  scene_->progress (0.24, Scene3D::progr_camprocessing);

  // active camera: first one with dispstat ON
  SDFCamera* activecam = NULL;
  for (activecam = (SDFCamera*) camlist_.first ();  
       activecam && !activecam->getdispstat ();  
       activecam = (SDFCamera*) camlist_.next () 
      );
  if (!activecam)
  { cerr << "hg3d. Warning: no active camera found. default: last one." << endl;
    activecam = (SDFCamera*) camlist_.last ();
  }

  // mark lights that depend on the camera
  if (activecam)
  { int actcamno = activecam->getobj_num ();
    for (alight = (Light*) lightlist_.first ();  alight;
         alight = (Light*) lightlist_.next ())
    {
      if (alight->getparent () == actcamno)
        alight->dependsOnCamera ();
    }
  }

  activecam_ = activecam;
  // now the camera is set, the scene *may* be drawn the first time

  scene_->progress (0.25, Scene3D::progr_objfile);

  // DEBUGNL ("readScene: geometric objects ");
  // read all geometric objects (gobjlist_)
  GeometricObject* agobj;
  int ngobjread = 1;
  for (agobj = (GeometricObject*) gobjlist_.first ();  !res && agobj;
       agobj = (GeometricObject*) gobjlist_.next (), ngobjread++)
  { res = agobj->readObjFile (sdffile, this);  // (needs access to materials)
    scene_->progress (0.25 + 0.75*ngobjread/num_gobj_, Scene3D::progr_unchanged);
    numfaces_ += agobj->numFaces ();
  }
  if (res)
  { cerr << "[hg3d. Error no. " << res << " on reading geometric object.]\n";
    return (res);
  }

  matbuilt_ = 0;  // note: cannot build matrices before window was opened.

  texturesloaded_ = 0;  // should be loaded when they are needed

  DEBUGNL ("readScene done.");
  return 0;

} // readFile (FILE*)

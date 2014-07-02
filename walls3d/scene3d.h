// This may look like C code, but it is really -*- C++ -*-

//<copyright>
// 
// Copyright (c) 1992,93,94,95,96
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        scene3d.h
//
// Purpose:     interface to 3D scene
//
// Created:     18 Mar 92   Michael Hofer and Michael Pichler
//
// Changed:     23 Feb 96   Alexander Nussbaumer (editing)
// Changed:     15 Mar 96   Michael Pichler
//
// $Id: scene3d.h,v 1.20 1996/03/12 10:21:50 mpichler Exp mpichler $
//
//</file>


#ifndef harmony_scene_scene3d_h
#define harmony_scene_scene3d_h

#include "vectors.h"
#include "color.h"

#pragma warning(disable: 4237 4244)

class GeometricObject;
class Camera;
class Material;
class RString;
class StringArray;
class SourceAnchor;
class QvNode;
class QvPerspectiveCamera;
class QvOrthographicCamera;
class QvWWWAnchor;
class QvWWWInline;

#include <stdio.h>
#include <iostream>
using namespace std;

class SceneData;

#define SIZ_FIRSTCOMMENT 80

class Scene3D
{ 
  public:
    Scene3D ();
    virtual ~Scene3D ();

    virtual void clear ();              // destroy scene data, free memory

    // *** input ***
    int readSceneFile (                 // read scene from file
      const char* filepath              //   "path/filename" or "filename"
    );
    int readScenePath (                 // read scene from file
      const char* path,                 //   "" (as "./"), "dirname/", or 0 (pathtofiles_)
      const char* filename              //   filename (appended to path)
    );
    const char* currentPath () const    // get path of last scene read
    { return pathtofiles_; }            //   non 0, ending with '/'

	char m_FirstComment[SIZ_FIRSTCOMMENT]; //DMcK

    virtual int readSceneFILE (         // read scene
      FILE* file                        //   from input stream
    );                                  //   determines file format, creates SceneData

    void saveVRMLSceneFile (ostream& os); //anuss

    const char* formatName () const;    // "SDF", "VRML" etc.

    virtual const char* mostRecentURL () const;  // should never return NULL ("" instead)
                                        // to handle relative URLs in anchors/inlines
    void currentURL (const char* url);  // will be returned by mostRecentURL

    const RString* currentFilename () const  // name of file from which scene was read
    { return curfilename_; }  // NULL if no scene loaded or got input directly from a FILE*
    void setCurrentFilename (const char*);   // normally set by readSceneFile

    enum { progr_actfile, progr_posfile, progr_camfile, progr_mtlfile,
           progr_lgtfile, progr_camprocessing, progr_objfile,
           progr_readvrml,
           progr_unchanged  // also number of progress id's
         };
    virtual void progress (float /*percentage*/, int /*labelid*/)
    { }  // progress of reading scene

    virtual void errorMessage (const char*) const;  // error message (e.g. during parsing)

    virtual void statusMessage (const char*)  { }   // message in status line

    virtual void showNumFaces ()  { }   // number of faces has changed

    virtual void requestTextures ();    // request all textures

    virtual void loadTextureFile (      // load a texture
      Material* material,               //   material  // TODO: abstract type
      const char* filename              //   from a file
    );

    virtual int readInlineVRMLFile (    // read inline VRML data
      QvWWWInline* node,                //   into node
      const char* filename              //   from a file
    );
    virtual int readInlineVRMLFILE (    // read inline VRML data
      QvWWWInline* node,                //   as above
      FILE* file                        //   from FILE*
    );

    // *** output ***
    enum { write_SDF, write_VRML, write_ORIGINAL = -1 };
    int writeScene (ostream& os,        // write SDF file to output stream
                    int format);        // format flag (write_...)
    int supportsOutputformat (int);     // support for specific output format
    virtual int writeOriginalScene (ostream& os);  // writeScene (write_ORIGINAL)
    int copyFile (FILE* infile, ostream& os);

//     enum { depth_PGMbin };
//     int writeDepth (ostream& os,        // write depth information of scene
//       int width, int height, int format);  // (format: depth_...)                   
//     requires reading back of Zbuffer, otherwise too slow

    void printInfo (int all = 1);       // print scene information

    unsigned long getNumFaces () const; // number of faces (polygons)
    unsigned long getNumPrimitives () const; // number of primitive objects

    // *** drawing ***
    void draw ();                       // draw whole scene (in current mode)

    void mode (int newmode)             // set drawing mode
    { mode_ = newmode; }
    int mode () const                   // get drawing mode
    { return mode_; }

    enum { same_mode = -1 };            // use same mode during interaction
    void modeInteract (int newmode)     // set drawing mode during interaction
    { modeInt_ = newmode; }
    int modeInteract () const           // get "       "    "      "
    { return modeInt_; }

    void interact (int flag)            // interaction (e.g. dragging)
    { interact_ = flag; }
    int interact () const               // get interaction flag
    { return interact_; }

    int interactRelevant () const;      // returns true if drawing mode changes on interaction
    int currentMode () const;           // return current mode with respect to interaction

    enum { twosided_always, twosided_auto, twosided_never };
    void twosidedpolys (int flag)       // set twosided polygons flag
    { twosidedpolys_ = flag; }
    int twosidedpolys () const          // get "        "        "
    { return twosidedpolys_; }

    enum { lighting_always, lighting_auto, lighting_never };
    void dolighting (int flag)          // set lighting calculation flag
    { dolighting_ = flag; }
    int dolighting () const             // get "        "           "
    { return dolighting_; }

    void viewingLight (int flag)        // set viewing light flag (headlight)
    { viewinglight_ = flag; }
    int viewingLight () const           // get "       "     "     "
    { return viewinglight_; }

    void textureLighting (int flag)     // set texture lighting flag
    { texlighting_ = flag; }
    int textureLighting () const        // get "       "        "
    { return texlighting_; }

    // *** size, center ***
    float size () const                 // get the "size" (of scene bounding box)
    { return size_; }
    void getCenter (point3D&) const;    // get the center (of scene bounding box)
    const point3D& getMin () const      // get scene bounding box (lower bounds)
    { return worldmin_; }
    const point3D& getMax () const      // get scene bounding box (upper bounds)
    { return worldmax_; }
    void setBoundings (                 // set bounding box of scene (world coords)
      const point3D& wmin, const point3D& wmax
    );

    // *** picking ***
    void* pickObject (                  // picking (full functionality; view coordinate system)
      float fx, float fy,               //   coordinates (fraction of window width and height)
      GeometricObject** gobj = 0,       //   GeometricObject hit (return)
      QvNode** node = 0,                //   node hit (return)
      QvWWWAnchor** anchor = 0,         //   anchor hit (return)
      point3D* hitpoint = 0,            //   optionally calculates hit point
      vector3D* normal = 0,             //   and face normal vector (normalized)
      const StringArray** groups = 0,   //   optionally determines groups hit
      float* hittime = 0                //   optionally returns the hit time
    );
    void* pickObject (                  // picking (full functionality; arbitrary direction)
      const point3D& A,                 //   ray start
      const vector3D& b,                //   ray direction
      float tnear,                      //   near distance
      float tfar,                       //   far distance
      GeometricObject** gobj = 0,       //   see above
      QvNode** node = 0,                //   "   "
      QvWWWAnchor** anchor = 0,         //   "   "
      point3D* hitpoint = 0,            //   "   "
      vector3D* normal = 0,             //   "   "
      const StringArray** groups = 0,   //   "   "
      float* hittime = 0                //   "   "
    );
    void* pickObject (                  // picking (shorthand for object info)
      float fx, float fy,               //   picking coordinates (as above)
      GeometricObject*& gobj,           //   GeometricObject hit
      QvNode*& node,                    //   node hit
      QvWWWAnchor*& anchor,             //   anchor hit
      const StringArray*& groups        //   determines groups hit
    );
    void* pickObject (                  // picking (shorthand for geometry info; view coordinates)
      float fx, float fy,               //   picking coordinates (as above)
      point3D& hitpoint,                //   hit point
      vector3D& normal,                 //   normal vector (normalized)
      float* hittime = 0                //   hit time (optional)
    );
    void* pickObject (                  // picking (shorthand for geometry info; arbitrary direction)
      const point3D& A,                 //   ray start
      const vector3D& b,                //   ray direction
      float tnear,                      //   near distance
      float tfar,                       //   far distance
      point3D& hitpoint,                //   hit point
      vector3D& normal,                 //   normal vector (normalized)
      float* hittime = 0                //   hit time (optional)
    );

    void activateLinks (int flag)       // highlight links or not
    { linksactive_ = flag; }
    int linksActive () const            // are links highlighted
    { return linksactive_; }

    // *** selection ***
    int selectObj (GeometricObject* obj,     // select SDF object, unselect previous
                   const char* group = 0);   //   optionally select group
    GeometricObject* selectedObj () const    // tell selected object (or nil)
    { return selectedobj_; }
    const char* selectedGroup () const       // tell selected group (or nil)
    { return selectedgroup_; }

    int selectNode (QvNode* node, QvWWWAnchor* = 0);  // select VRML node, unselect previous
    QvNode* selectedNode () const            // get selected VRML node (or NULL)
    { return selectednode_; }
    QvWWWAnchor* selectedAnchor () const     // get selected VRML anchor (or NULL)
    { return selectedanchor_; }

    virtual void selectionChanged ()  { }    // selected object has changed

    virtual void setSourceAnchor ();         // define selected object as source anchor
    void deleteSourceAnchor ();              // delete selected source anchor
    virtual void deleteSourceAnchor (const SourceAnchor*);  // delete a source anchor
    void do_deleteSourceAnchor ()  { deleteSourceAnchor (); }  // patch for goofy g++
    virtual void setDestinationAnchor ();    // define current view as destination anchor
    virtual void setDefDestAnchor ();        // define the default destination anchor

    // *** navigation ***
    virtual void back ();                    // history functions
    virtual void forward ();
    virtual void history ();
    virtual void hold ();                    // hold this viewer
    enum { info_references, info_annotations, info_parents, info_attributes, info_textures };
    virtual void docinfo (int);              // document information
    void anchorinfo ();                      // info on current anchor
    virtual void anchorInfo (const SourceAnchor*);

    enum { nav_flip, nav_walk };
    virtual void navigationHint (int)  { }   // navigation hint from file

    // *** find objects ***
    // TODO: abstract return types
    GeometricObject* findObject (const char* objname);
    GeometricObject* findObject (int objnum);
    Material* findMaterial (const char* matname);

    // *** anchors ***
    void clearAnchors ();               // clear all anchor definitions
    void defineAnchor (                 // store an anchor
      GeometricObject* obj,             //   geometric object for anchor
      long id,                          //   anchor id
      const RString& aobj,              //   anchor object
      const char* group                 //   opt. group name (NULL: object anchor)
    );
    void defineAnchor (                 // as above,
      int objnum,                       //   but object defined by its number
      long id, const RString& aobj, const char* group
    );
    void defineAnchor (                 // as above,
      const char* objname,              //   but object defined by its name
      long id, const RString& aobj, const char* group
    );

    virtual int followLink (            // link activation (SDF)
      const GeometricObject*, const SourceAnchor*
    )  { return 0; }                    // return value nonzero if redraw necessary
    virtual int followLink (            // link activation (VRML)
      const QvWWWAnchor*                //   anchor object hit
    );
    void anchorURL (                    // get the complete URL
      const QvWWWAnchor*,               //   of an anchor,
      RString& anchorurl                //   as absolute URL
    );
    void useMosaic (int flag)
    { usemosaic_ = flag; }

    virtual void requestInlineURL (     // URL-request for inline VRML
      QvWWWInline* node,                //   node (where attach inline data)
      const char* url,                  //   requested URL (of data to be fetched)
      const char* docurl                //   URL of requesting document (for local URLs)
    );

    virtual void showHelp (const char* topic = 0);  // show help
    void doShowHelp () { showHelp (); } // for callbacks

    enum { l_brightness = 1, l_boundingbox, l_diffcolor, l_coloredges, // link highlighting modes
           num_colors  // incl. natural color (index 0)
         };
    void linksColor (int type)          // set anchor highlighting mode (l_...)
    { linkscolor_ = type; }
    int linksColor () const             // how anchors are highlighted
    { return linkscolor_; }

    // *** camera ***
    Camera* getCamera () const;         // get active camera
    void storeCamera () const;          // store active camera
    void restoreCamera () const;        // reset camera (to one read from file or latest stored)

    virtual void registerCamera (       // add camera to viewpoint list
      const char*,
      QvPerspectiveCamera*,
      QvOrthographicCamera*
    )  { }                              // camera infos have to be cleared on clear
    void activateCamera (
      const char*,
      QvPerspectiveCamera*, QvOrthographicCamera*
    );

    // anachronisms getCameraPosition, getCameraLookat, setCameraParams removed
    // (call the equivalent functions on the active camera)

    void setWinAspect (float asp)       // set window aspect
    { winaspect_ = asp; }
    float getWinAspect () const         // (width/height)
    { return winaspect_; }

    void arbitraryRotations (int flag)  // set flag for arbitrary rotations
    { arbitraryrot_ = flag; }
    int arbitraryRotations () const     // get "    "   "         "
    { return arbitraryrot_; }

    void collisionDetection (int flag)  // set flag for collision detection
    { colldetection_ = flag; }
    int collisionDetection () const     // get "    "   "         "
    { return colldetection_; }

    // clipping planes
    static float clipFarFactor;  // multiples of scene size for far clipping plane
    static float clipNearRatio;  // ratio far/near clipping plane (determines z-buffer precision)
    static float clipNearMaximum;  // maximum for near clipping plane

    // *** colour ***
    colorRGB col_background;
    colorRGB col_hudisplay;
    colorRGB col_anchorface;
    colorRGB col_anchoredge;
    colorRGB col_viewinglight;
    void colorRebuild ();  // data structure may need a rebuild on anchor color change

    static float anchors_minbrightness; // brightness values for
    static float nonanch_maxbrightness; // anchor highlighting

    // *** scene data ***
    const SceneData* data () const      // managed data
    { return data_; }
    SceneData* editdata ()              // anuss: must be changeable during editing
    { return data_; }

    // help directory (will be moved to SceneWindow)
    RString* helpdir_;

    // scene editing (anuss)
    int sceneManipulated () const       // true, if the scene has been changed
    { return scenemanipulated_; }
    void setSceneManipulated (int state)
    { scenemanipulated_ = state; }
    virtual int continueOnSaveState ()  // chance to save current changes now
    { return 1; }  // return value tells whether to continue (scene will be cleared)

  private:
    SceneData* data_;

    int scenemanipulated_;  // anuss: if the scene has been edited

    char pathtofiles_ [256];  // (for standalone viewer)
    RString* curfilename_;    // current file name
    RString* recenturl_;      // URL most recently encountered

    int mode_;           // current drawing mode
    int modeInt_;        // drawing mode during interaction
    int interact_;       // interaction flag
    int twosidedpolys_;  // twosided polygon flag
    int dolighting_;     // lighting calculations flag
    int viewinglight_;   // viewing light flag
    int texlighting_;    // texture lighting flag
    int linksactive_;    // are links highlighted?
    int linkscolor_;     // how to highlight the links

    point3D worldmin_, worldmax_;       // bounding box in world coordinates
    float size_;                        // "size" of scene bounding box
    float winaspect_;
    int arbitraryrot_;                  // arbitrary rotations flag
    int colldetection_;                 // collision detection flag

    GeometricObject* selectedobj_;
    const char* selectedgroup_;
    QvNode* selectednode_;
    QvWWWAnchor* selectedanchor_;
    int usemosaic_;      // web browser

    // prevent copying (declared, but not implemented)
    Scene3D (const Scene3D&);
    Scene3D& operator = (const Scene3D&);

}; // Scene3D


inline int Scene3D::interactRelevant () const
{ return (modeInt_ >= 0 && modeInt_ != mode_);
}

inline int Scene3D::currentMode () const
{ return ((interact_ && modeInt_ >= 0) ? modeInt_ : mode_);
}

inline void Scene3D::getCenter (point3D& center) const
{ lcm3D (0.5, worldmin_, 0.5, worldmax_, center);
}

inline void Scene3D::setBoundings (const point3D& wmin, const point3D& wmax)
{
  worldmin_ = wmin;
  worldmax_ = wmax;
  // "size" of scene: average of x-, y-, and z-size
  size_ = (wmax.x-wmin.x + wmax.y-wmin.y + wmax.z-wmin.z) / 3;
}

inline void* Scene3D::pickObject (
  float fx, float fy, GeometricObject*& gobj, QvNode*& node, QvWWWAnchor*& anchor, const StringArray*& groups
) // shorthand for object picking
{ return pickObject (fx, fy, &gobj, &node, &anchor, 0, 0, &groups);
}

inline void* Scene3D::pickObject (
  float fx, float fy, point3D& hitpoint, vector3D& normal, float* hittime
) // shorthand for geometry picking, view coordinates
{ return pickObject (fx, fy, 0, 0, 0, &hitpoint, &normal, 0, hittime);
}

inline void* Scene3D::pickObject (
  const point3D& A, const vector3D& b, float tnear, float tfar, point3D& hitpoint, vector3D& normal, float* hittime
) // shorthand for geometry picking, arbitrary direction
{ return pickObject (A, b, tnear, tfar, 0, 0, 0, &hitpoint, &normal, 0, hittime);
}

inline void Scene3D::defineAnchor (int objnum, long id, const RString& aobj, const char* group)
{ defineAnchor (findObject (objnum), id, aobj, group);
}

inline void Scene3D::defineAnchor (const char* objname, long id, const RString& aobj, const char* group)
{ defineAnchor (findObject (objname), id, aobj, group);
}


#endif

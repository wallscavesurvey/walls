//<file>
//
// Name:        QvNode.h
//
// Purpose:     declaration basis for all QvNodes
//
// Created:     24 Apr 95   taken from QvSubNode.h
//
// Changed:      9 Nov 95   Michael Pichler
// Changed:      8 Feb 96   Alexander Nussbaumer
//
// $Id: QvNode.h,v 1.2 1995/11/14 14:28:03 mpichler Exp $
//
//</file>

#ifndef  _QV_NODE_
#define  _QV_NODE_

#include <iostream>

#include "QvString.h"

#include "../ge3d/vectors.h"

class QvChildList;
class QvDict;
class QvFieldData;
class QvInput;
class QvNodeList;
class QvState;

class Scene3D;  // mpichler
class VRMLScene;
//class ostream;

class QvNode {

  public:
    enum Stage {
	FIRST_INSTANCE,		// First real instance being constructed
	PROTO_INSTANCE,		// Prototype instance being constructed
	OTHER_INSTANCE		// Subsequent instance being constructed
    };

    QvFieldData	*fieldData;
    QvChildList	*children;
    QvBool	isBuiltIn;

    QvName		*objName;
    QvNode();
    virtual ~QvNode();

    // Reference counting:
    long	refCount;
    void	ref() const;		// Adds reference
    void	unref() const;		// Removes reference, deletes if now 0
    void	unrefNoDelete() const;	// Removes reference, never deletes

    const QvName &	getName() const;
    void		setName(const QvName &name);

    static void		init();
    static QvBool	read(QvInput *in, QvNode *&node);

    virtual QvFieldData *getFieldData() = 0;

    // traversal functions (mpichler)
    virtual void traverse(QvState *state) = 0;  // print node information

    virtual void build (QvState* state) = 0;  // preprocessing step

    virtual void draw () = 0;  // draw the node

    virtual void pick () = 0;  // picking

    virtual void save (QvDict* dict) = 0;  // saving (anuss, 19960118)
	static void saveAll (const QvNode *root, std::ostream& os, float version);

    // pointers back to the current (VRML)Scene (mpichler)
    static int curdrawmode_;            // current drawing mode
    static Scene3D* scene_;             // scene (management) class
    static VRMLScene* vrmlscene_;       // vrml scene (data)

    int hasextent_;                     // flag if extent (if not, wmin_/wmax_ unset)
    point3D wmin_, wmax_;               // bounding box (world coordinates)
    point3D omin_, omax_;               // bounding box (object coordinates)

  protected:
    virtual QvBool	readInstance(QvInput *in);

  private:
    static QvDict	*nameDict;
  
    static void		addName(QvNode *, const char *);
    static void		removeName(QvNode *, const char *);
    static QvNode *	readReference(QvInput *in);
    static QvBool	readNode(QvInput *in, QvName &className,QvNode *&node);
    static QvBool	readNodeInstance(QvInput *in, const QvName &className,
					 const QvName &refName, QvNode *&node);
    static QvNode *	createInstance(QvInput *in, const QvName &className);
    static QvNode *	createInstanceFromName(const QvName &className);
    static void		flushInput(QvInput *in);
};

#endif /* _QV_NODE_ */

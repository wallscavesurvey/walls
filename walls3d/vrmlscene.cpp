//<copyright>
// 
// Copyright (c) 1995,96
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        vrmlscene.C
//
// Purpose:     implementation of class VRMLScene
//
// Created:     24 Apr 95   Michael Pichler
//
// Changed:     23 Feb 96   Michael Pichler
//
// $Id: vrmlscene.C,v 1.19 1996/03/12 10:17:11 mpichler Exp $
//
//</file>


#include "vrmlscene.h"
#include "scene3d.h"
#include "vecutil.h"
#include "camera.h"

#include "ge3d.h"

#include "qvlib/QvDB.h"
#include "qvlib/QvInput.h"
#include "qvlib/QvDebugError.h"
#include "qvlib/QvReadError.h"
#include "qvlib/QvNode.h"
#include "qvlib/QvState.h"
#include "qvlib/QvSFString.h"
#include "qvlib/QvWWWInline.h"
#include "qvlib/QvPerspectiveCamera.h"
#include "qvlib/QvOrthographicCamera.h"

#include "hyperg/message.h"
#include "hyperg/verbose.h"

#include <math.h>



/*** VRMLErrorCallback ***/

struct VRMLErrorCallback  // used by VRMLScene only
{
	static Scene3D* scene_;  // assert: scene_ pointer set before callbacks called
	static void debugError(const char* method, const char* errormsg);
	static void readError(const char* error, const char* location);
};


Scene3D* VRMLErrorCallback::scene_ = 0;


void VRMLErrorCallback::debugError(const char* method, const char* errormsg)
{
	char buf[2500];
	sprintf(buf, "VRML error in %.1024s(): %.1024s\n", method, errormsg);
	scene_->errorMessage(buf);
}


void VRMLErrorCallback::readError(const char* error, const char* location)
{
	char buf[2500];
	sprintf(buf, "VRML read error: %.1024s\n%.1024s\n", error, location);
	scene_->errorMessage(buf);
}


/*** VRMLScene ***/


VRMLScene::VRMLScene(Scene3D* scene)
	: SceneData(scene)
{
	root_ = 0;
	matbuilt_ = 0;
	numfaces_ = 0;
	numprimitives_ = 0;
	hascamera_ = 0;
	haslight_ = 0;
	numlights_ = 0;
	camera_ = new Camera;  // default camera
	bakcamera_ = new Camera(*camera_);  // copy of original camera
	activepcam_ = 0;
	activeocam_ = 0;
	QvNode::scene_ = scene;
	QvNode::vrmlscene_ = this;

	// anuss
	vrmlversion_ = 0;  // set on reading
	selectedtransform_ = nil;

	// error callbacks
	VRMLErrorCallback::scene_ = scene;
	if (!QvDebugError::callback_)
		QvDebugError::callback_ = &VRMLErrorCallback::debugError;
	if (!QvReadError::callback_)
		QvReadError::callback_ = &VRMLErrorCallback::readError;
} // VRMLScene


VRMLScene::~VRMLScene()
{
	ge3dDefaultMaterial();

	delete root_;
	delete camera_;     // unlike SDFScene, both cameras are separate instances
	delete bakcamera_;  // and not pointers into the data structure
	QvNode::scene_ = 0;
	QvNode::vrmlscene_ = 0;

	//root_ = 0;
	//matbuilt_ = 0;
	//numfaces_ = 0;
	//numprimitives_ = 0;
	//hascamera_ = 0;
	//activepcam_ = 0;
	//activeocam_ = 0;
	//haslight_ = 0;
	//numlights_ = 0;
}


int VRMLScene::readInput(QvInput& in)
{
	if (root_)
	{
		HgMessage::error("reading VRML scene over existent one (internal error).");
		return -1;
	}

	scene_->progress(0.0, Scene3D::progr_readvrml);

	int ok = QvDB::read(&in, root_) && root_;

	if (!ok)
		HgMessage::error("invalid VRML scene");
	else
		vrmlversion_ = in.getVersion();  // anuss

	  // invalid scene should be destroyed immediately after reading
	return !ok;  // error code
}

// just added for %$%$% VC++ 4.0
int VRMLScene::readInlineVRMLFile(QvWWWInline* node, const char* filename)
{

	return 1;
}

int VRMLScene::readInlineVRMLFILE(QvWWWInline* node, FILE* file)
{
	if (!node || !file || !root_)
		return 0;

	// QVDB::init () not necessary
	QvInput in;
	in.setFilePointer(file);

	int ok = node->readChildren(&in);

	if (ok)
	{
		node->state_ = QvWWWInline::s_completed;
		DEBUGNL("VRMLScene::readInlineVRML: rebuild");

		// update data structures (much of the work done on first draw)
		ge3dLoadIdentity();
		QvState state;
		QvNode::scene_ = scene_;
		QvNode::vrmlscene_ = this;
		ge3dPushIdentity();  // top node need not be a separator
		root_->build(&state);  // node->build () not sufficient (global dependencies possible)
		ge3d_pop_matrix();
		if (root_->hasextent_)  // overall bounding box may change
			scene_->setBoundings(root_->wmin_, root_->wmax_);  // scene bounding box
		scene_->showNumFaces();  // number of faces probably increased

		// caller responsible for redraw
	}
	else
	{
		HgMessage::error("WWWInline: invalid VRML scene");
		node->state_ = QvWWWInline::s_failed;
	}

	// file will be closed by caller

	DEBUGNL("VRMLScene::readInlineVRML finished");
	return ok;  // redraw flag

} // readInlineVRML


//anuss
int VRMLScene::writeData(ostream& os, int format)
{
	if (format == Scene3D::write_SDF)
	{
		HgMessage::error("Cannot convert current VRML-Scene to SDF-Format");
		return 0;
	}

	if (!root_)
	{
		HgMessage::error("Cannot save empty Scene");
		return 0;
	}

	QvNode::saveAll(root_, os, vrmlversion_);

	return 1;  // success
}


void VRMLScene::printInfo(int /*all*/)
{
	QvState state;
	if (root_)
		root_->traverse(&state);
}


void VRMLScene::storeCamera()
{
	*bakcamera_ = *camera_;  // save current camera (default assignment operator)
}


void VRMLScene::restoreCamera()
{
	*camera_ = *bakcamera_;  // restore last saved camera (default assignment op.)
}


void VRMLScene::hasCamera(QvPerspectiveCamera* cam, const char* name)
{
	if (!hascamera_)
	{
		activatePCam(cam);
		hascamera_ = 1;
		activepcam_ = cam;
	}
	RString namestr(name);
	namestr.subst('_', ' ');

	scene_->registerCamera(namestr, cam, NULL);
}


void VRMLScene::hasCamera(QvOrthographicCamera* cam, const char* name)
{
	if (!hascamera_)
	{
		activateOCam(cam);
		hascamera_ = 1;
		activeocam_ = cam;
	}
	RString namestr(name);
	namestr.subst('_', ' ');

	scene_->registerCamera(namestr, NULL, cam);
}


void VRMLScene::activatePCam(QvPerspectiveCamera* cam)
{
	if (!camera_ || !cam)
		return;

	// cerr << "activated perspective camera" << endl;
	camera_->reset();  // position, orientation done by camera node
	camera_->perspectiveCam(cam->yangle_);  // radians
	storeCamera();
	cam->switchto();
}


void VRMLScene::activateOCam(QvOrthographicCamera* cam)
{
	if (!camera_ || !cam)
		return;

	// cerr << "activated orthographic camera" << endl;
	camera_->reset();
	camera_->orthographicCam(cam->height.value);  // vp.height
	storeCamera();
	cam->switchto();
}


void VRMLScene::activateCamera(
	const char* name,
	QvPerspectiveCamera* pcam, QvOrthographicCamera* ocam
)
{
	if (pcam)
		activatePCam(pcam);
	else
		activateOCam(ocam);

	scene_->statusMessage(name);

	if (pcam != activepcam_ || ocam != activeocam_)
	{
		activepcam_ = pcam;
		activeocam_ = ocam;

		DEBUGNL("VRMLScene: rebuild after camera change");

		QvState state;
		ge3dPushIdentity();  // top node need not be a separator
		root_->build(&state);  // rebuild for correct bounding boxes
		ge3d_pop_matrix();
	}
	// caller responsible for redraw
}


void VRMLScene::deactivateLights(int n)
{
	//cerr << "turning off lights " << n+1 << " to " << numlights_ << endl;
	// turn off all lights with index > n
	while (numlights_ > n)
		ge3d_switchlight(numlights_--, 0);
}


// void VRMLScene::colorRebuild (): see wrlbuild.C


void VRMLScene::draw(int curmode)
{
	static colorRGB white = { 1.0, 1.0, 1.0 };

	if (!root_)
		return;

	QvNode::scene_ = scene_;
	QvNode::vrmlscene_ = this;
	QvNode::curdrawmode_ = curmode;

	if (!matbuilt_)
	{
		matbuilt_ = 1;
		DEBUGNL("VRMLScene: preprocessing (first draw)");
		ge3d_init_();  // initialise ge3d library

		ge3dLoadIdentity();  // replace current matrix with identity

		QvState state;

		ge3dPushIdentity();  // top node need not be a separator

		root_->build(&state);  // preprocessing
		// done on first draw because graphics libraries require an opened window

		ge3d_pop_matrix();

		if (root_->hasextent_)
		{
			DEBUGNL("scene bounding box: " << root_->wmin_ << ", " << root_->wmax_);

			scene_->setBoundings(root_->wmin_, root_->wmax_);  // scene bounding box
			float clip = Scene3D::clipFarFactor * scene_->size();
			DEBUGNL("far clipping plane (" << Scene3D::clipFarFactor << " * size): " << clip);

			// prevent scene from being clipped off when camera is too far away from scene
			point3D center;  // by setBoundings
			scene_->getCenter(center);
			// currently camera at origin in world coordinates; later: subtract camera position
			DEBUGNL("scene center: " << center << ", size: " << scene_->size());

			float dist = norm3D(center) + 3 * scene_->size();
			if (clip < dist)
			{
				DEBUGNL("far clipping plane increased to " << dist);
				clip = dist;
			}

			// if (camera_->getyon () < clip)
			camera_->setyon(clip),
				bakcamera_->setyon(clip);

			clip /= Scene3D::clipNearRatio;  // near clipping plane
			DEBUGNL("near clipping plane (1/" << Scene3D::clipNearRatio << " of far; "
				"set to " << Scene3D::clipNearMaximum << " if larger): " << clip);
			if (clip > Scene3D::clipNearMaximum)
				clip = Scene3D::clipNearMaximum;
			camera_->sethither(clip),
				bakcamera_->sethither(clip);
		}

		scene_->showNumFaces();  // only available after build
		// may change after dynamic loading of inline scenes

		if (!hascamera_)  // define a default camera (based on bounding box)
		{
			point3D center;
			scene_->getCenter(center);

			point3D pos, look;
			center.z += 2 * scene_->size();  // rule of thumb (in accordance with cam angle)
			init3D(pos, center.x, center.y, center.z);
			init3D(look, center.x, center.y, center.z - 1);  // looking along z (to -z)

			DEBUGNL("setting default camera at pos. " << pos << ", looking to " << look);
			camera_->setposlookup(pos, look);  // default up vector (y)
			*bakcamera_ = *camera_;  // save this camera
		}

		DEBUGNL("VRMLScene: preprocessing finished");
	}

	ge3d_setmode(curmode);  // activate drawing mode

	ge3dFillColor(&white);  // default line/fill color

	ge3dLoadIdentity();  // replace current matrix with identity
	if (curmode == ge3d_texturing)
		ge3dLoadTextureIdentity();

	camera_->setCamera(scene_);
	// TODO: check side effects of viewing and scene camera to each other

	if (!haslight_ || scene_->viewingLight())  // default viewing light (headlight)
	{
		static const vector3D vwlgtdir = { 0, 0, 1 };  // towards camera
		ge3dSetLightSource(0, &scene_->col_viewinglight, &vwlgtdir, 0.0, 1);
		ge3d_switchlight(0, 1);
	}

	// light sources are activated within scene graph
	numlights_ = 0;

	// no backfaceculling unless never twosided
	ge3dHint(hint_backfaceculling, scene_->twosidedpolys() == Scene3D::twosided_never);
	// do lighting unless unless turned off globally
	ge3dHint(hint_lighting, scene_->dolighting() != Scene3D::lighting_never);
	// texture lighting flag
	ge3dHint(hint_texlighting, scene_->textureLighting());

	// TODO: request textues if necessary (will be done during draw traversal)

	drawVRML();  // draw the scene graph (see wrldraw.C)

  //   ge3d_setmode (ge3d_wireframe);  // test: show world bounding box
  //   ge3d_setlinecolor (1.0, 0.0, 0.0);
  //   ge3dCube (&root_->wmin_, &root_->wmax_);

	deactivateLights();  // deactivate remaining light sources

	ge3d_switchlight(0, 0);  // turn off default viewing light

	ge3dDefaultMaterial();  // some materials (e.g. emissive) interfere with wireframe GUI drawings
	ge3dDoTexturing(0);  // turn of texturing

	ge3dHint(hint_lighting, 1);

} // draw



// VRMLScene::pickObject implemented in wrlpick.C

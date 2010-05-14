//----------------------------------------------------------------------------------//
// OgreRecast Demo - A demonstration of integrating Recast Navigation Meshes		//
//					 with the Ogre3D rendering engine.								//
//																					//
//	This file was either Created by or Modified by :								//
//													Paul A Wilson 					//
//			All contents are Copyright (C) 2010 Paul A Wilson						//
//			Except where otherwise mentioned or where previous						//
//			copyright exists. In the case of pre-existing copyrights				//
//			all rights remain with the original Authors, and this is				//
//			to be considered a derivative work.										//
//																					//
//	Contact Email	:	paulwilson77@dodo.com.au									//
//																					//
// This 'SOFTWARE' is provided 'AS-IS', without any express or implied				//
// warranty.  In no event will the authors be held liable for any damages			//
// arising from the use of this software.											//
// Permission is granted to anyone to use this software for any purpose,			//
// including commercial applications, and to alter it and redistribute it			//
// freely, subject to the following restrictions:									//
// 1. The origin of this software must not be misrepresented; you must not			//
//    claim that you wrote the original software. If you use this software			//
//    in a product, an acknowledgment in the product documentation would be			//
//    appreciated but is not required.												//
// 2. Altered source versions must be plainly marked as such, and must not be		//
//    misrepresented as being the original software.								//
// 3. This notice may not be removed or altered from any source distribution.		//
//																					//
//----------------------------------------------------------------------------------//


#include "Ogre.h"
#include "OgreTemplate.h"
#include "SharedData.h"
#include "GUtility.h"
#include "InputGeom.h"

#include "Recast.h"
#include "RecastTimer.h"
#include "RecastDebugDraw.h"
#include "RecastDump.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"
#include "DetourDebugDraw.h"

#ifdef WIN32
#	define snprintf _snprintf
#endif

//----------------------------------------------------------------------------------------------
DebugDrawGL::DebugDrawGL():
	mYOffset(0.0f),
	mBeginCalledOnce(false),
	mActive(false)
{
	name = TemplateUtils::GetUniqueObjName("DebugDrawGL");
	mtl = TemplateUtils::GetUniqueMtlName("DebugDrawMTL");

	obj = SharedData::getSingleton().iSceneMgr->createManualObject(name);
	obj->setQueryFlags(STATIC_GEOMETRY_QUERY_MASK);

	createDefaultMaterial(mtl);
	
	SharedData::getSingleton().iSceneMgr->getRootSceneNode()->attachObject(obj); 

	// set to true here, used for a check for one-off initialization
	mActive = true;
}

//----------------------------------------------------------------------------------------------
DebugDrawGL::~DebugDrawGL()
{
	obj->detachFromParent();
	obj->clear();
	mMtrl->unload();
}

//----------------------------------------------------------------------------------------------
void DebugDrawGL::depthMask(bool state)
{
	mMtrl->getTechnique(0)->setDepthCheckEnabled(state);
	mMtrl->getTechnique(0)->setDepthWriteEnabled(state);
}

//----------------------------------------------------------------------------------------------
void DebugDrawGL::begin(duDebugDrawPrimitives prim, float size)
{
	switch (prim)
	{
	case DU_DRAW_POINTS:
		mMtrl->getTechnique(0)->setPointSize(size);
		obj->begin(mtl, Ogre::RenderOperation::OT_POINT_LIST);
		break;
	case DU_DRAW_LINES:
		mMtrl->getTechnique(0)->setPointSize(size);
		obj->begin(mtl, Ogre::RenderOperation::OT_LINE_LIST);
		break;
	case DU_DRAW_TRIS:
		obj->begin(mtl, Ogre::RenderOperation::OT_TRIANGLE_LIST);
		break;
	case DU_DRAW_QUADS:
		obj->begin(mtl, Ogre::RenderOperation::OT_POINT_LIST);
		break;
	case DU_DRAW_LINES_STRIP:
		mMtrl->getTechnique(0)->setPointSize(size);
		obj->begin(mtl, Ogre::RenderOperation::OT_LINE_STRIP);
		break;
	};

	mBeginCalledOnce = true;
}

//----------------------------------------------------------------------------------------------
void DebugDrawGL::vertex(const float* pos, unsigned int color)
{
	obj->position(pos[0], pos[1] + mYOffset, pos[2]);
	float *col = new float[3];
	duIntToCol(color, col);
	obj->colour(Ogre::ColourValue(col[0], col[1], col[2], 0));
	delete [] col;
}

//----------------------------------------------------------------------------------------------
void DebugDrawGL::vertex(const float x, const float y, const float z, unsigned int color)
{
	
	obj->position(x, y + mYOffset, z);
	float *col = new float[3];
	duIntToCol(color, col);
	obj->colour(Ogre::ColourValue(col[0], col[1], col[2], 0));
	delete [] col;
}

//----------------------------------------------------------------------------------------------
void DebugDrawGL::vertex(const float* pos, Ogre::ColourValue &color)
{
	obj->position(pos[0], pos[1] + mYOffset, pos[2]);
	obj->colour(color);
}

//----------------------------------------------------------------------------------------------
void DebugDrawGL::vertex(const float x, const float y, const float z, Ogre::ColourValue &color)
{

	obj->position(x, y + mYOffset, z);
	obj->colour(color);
}

//----------------------------------------------------------------------------------------------
void DebugDrawGL::end()
{
	obj->end();
}

//----------------------------------------------------------------------------------------------
void DebugDrawGL::clear()
{
	obj->clear();
	mBeginCalledOnce = false;
}

//----------------------------------------------------------------------------------------------
void DebugDrawGL::setMaterialScript(Ogre::String& matName)
{
	obj->clear();
	mMtrl->unload();
	mMtrl =  Ogre::MaterialManager::getSingleton().load(matName, "General");
	mtl = matName;
	mMtrl->_dirtyState();
}

//----------------------------------------------------------------------------------------------
void DebugDrawGL::createDefaultMaterial(Ogre::String& matName)
{
	if(mActive)
	{
		mMtrl->unload();
	}
	mMtrl = Ogre::MaterialManager::getSingleton().create(matName, "General"); 
	mMtrl->getTechnique(0)->getPass(0)->setDiffuse(1, 1, 1, 1); 
	mMtrl->getTechnique(0)->getPass(0)->setAmbient(1, 1, 1); 
	mMtrl->getTechnique(0)->getPass(0)->setSelfIllumination(1, 1, 1);

	mMtrl->getTechnique(0)->createPass();
	mMtrl->getTechnique(0)->getPass(1)->setDiffuse(1, 1, 1, 1); 
	mMtrl->getTechnique(0)->getPass(1)->setAmbient(1, 1, 1); 
	mMtrl->getTechnique(0)->getPass(1)->setSelfIllumination(1, 1, 1);
	mMtrl->getTechnique(0)->getPass(1)->setPolygonMode(Ogre::PolygonMode::PM_WIREFRAME);

	mtl = matName;
}

//----------------------------------------------------------------------------------------------
void DebugDrawGL::createDefaultMaterial(void)
{
	createDefaultMaterial(TemplateUtils::GetUniqueMtlName("DebugDrawMTL"));
}
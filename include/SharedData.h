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

// ORIGINALLY TAKEN FROM Ogre3D DeferredShading Sample

/*////////////////////////////////////////////////////////////////////////////////
//This Header is used to define any library wide structs and enums and defines
////////////////////////////////////////////////////////////////////////////////*/

#ifndef __H_SHAREDDATA_H__
#define __H_SHAREDDATA_H__

#include "Ogre.h"
#include "OIS.h"
#include "RecastLog.h"
#include "DetourNavMesh.h"


#define SDATA SharedData::getSingleton()
#define MAX_LAYERS_ALLOWED 6

// typedef for nodelist for navmesh
typedef std::vector<Ogre::SceneNode*> NavSceneNodeList;
// UTF String vector for use in the GUtility class.
typedef Ogre::vector<Ogre::UTFString>::type UTFStringVector;

/** This is the Query Flag for any other MovableObject */
const unsigned int QUERYFLAG_MOVABLE = 2;


// These are just sample areas to use consistent values across the samples.
// The use should specify these base on his needs.
enum SamplePolyAreas
{
	SAMPLE_POLYAREA_GROUND,
	SAMPLE_POLYAREA_WATER,
	SAMPLE_POLYAREA_ROAD,
	SAMPLE_POLYAREA_DOOR,
	SAMPLE_POLYAREA_GRASS,
	SAMPLE_POLYAREA_JUMP,
};
enum SamplePolyFlags
{
	SAMPLE_POLYFLAGS_WALK = 0x01,		// Ability to walk (ground, grass, road)
	SAMPLE_POLYFLAGS_SWIM = 0x02,		// Ability to swim (water).
	SAMPLE_POLYFLAGS_DOOR = 0x04,		// Ability to move through doors.
	SAMPLE_POLYFLAGS_JUMP = 0x08,		// Ability to jump.
	SAMPLE_POLYFLAGS_ALL = 0xffff		// All abilities.
};


// Tool types.
enum SampleToolType
{
	TOOL_NONE = 0,
	TOOL_TILE_EDIT,
	TOOL_TILE_HIGHLIGHT,
	TOOL_NAVMESH_TESTER,
	TOOL_OFFMESH_CONNECTION,
	TOOL_CONVEX_VOLUME,

};

enum DrawMode
{
	DRAWMODE_MESH,
	DRAWMODE_NAVMESH,
	DRAWMODE_NAVMESH_INVIS,
	DRAWMODE_NAVMESH_TRANS,
	DRAWMODE_NAVMESH_BVTREE,
	DRAWMODE_VOXELS,
	DRAWMODE_VOXELS_WALKABLE,
	DRAWMODE_COMPACT,
	DRAWMODE_COMPACT_DISTANCE,
	DRAWMODE_COMPACT_REGIONS,
	DRAWMODE_REGION_CONNECTIONS,
	DRAWMODE_RAW_CONTOURS,
	DRAWMODE_BOTH_CONTOURS,
	DRAWMODE_CONTOURS,
	DRAWMODE_POLYMESH_DETAIL,
	DRAWMODE_POLYMESH,
	MAX_DRAWMODE
};

enum ToolMode
{
	TOOLMODE_PATHFIND_ITER,
	TOOLMODE_PATHFIND_STRAIGHT,
	TOOLMODE_RAYCAST,
	TOOLMODE_DISTANCE_TO_WALL,
	TOOLMODE_FIND_POLYS_AROUND,
};

enum ApplicationMode
{
	APPMODE_NONE = 0,
	APPMODE_SOLOMESH,
	APPMODE_TILEMESH,
	APPMODE_TERRAINSCENE,
	APPMODE_MESHSCENE,
};

// struct holds the terrain layer blend info
struct CalcBlendData
{
	float hs;     /// Height Start
	float he;     /// Height End
	float hr;     /// Height Release
	float ss;     /// Slope Start
	float se;     /// Slope End
	float sr;     /// Slope Release
	float skw;    /// Skew Scale
	float skwazm; /// Skew Azimuth
	int layer;    /// Layer this structure's values reference
};
typedef std::vector<CalcBlendData*> BlendData;

class SharedData : public Ogre::Singleton<SharedData> 
{

public:

	SharedData()
		: iRoot(0),
		  iCamera(0),
		  iWindow(0),
		  mMLAnimState(0),
		  iMainLight(0),
		  m_toolMode(TOOLMODE_PATHFIND_ITER),
		  m_bidir(true),
		  m_areaType(SAMPLE_POLYAREA_GRASS),
		  m_boxHeight(6.0f),
		  m_boxDescent(1.0f),
		  m_AppMode(APPMODE_MESHSCENE)
	{
		iActivate = false;

		m_filter.includeFlags = SAMPLE_POLYFLAGS_ALL;
		m_filter.excludeFlags = 0;

		mDbgLog.clear();
		currentMeshName = "";
		mNavNodeList.resize(0);
		m_BlendList.resize(0);
	}

		~SharedData() {}


		//--------------------------------------------------------------------------
		// Ogre3D Specific Shared Data across the application

		
		/*  
		*	keeps track of the frame time of the previous frame
		*	this is updated in frameRenderingQueued ( in milliseconds )
		*/
		Ogre::Real							iLastFrameTime;
		/*
		*	Pointer to the Ogre3D Root object
		*/
		Ogre::Root*							iRoot;
		/*
		*	Pointer to the current render camera
		*/
		Ogre::Camera*						iCamera;
		/*
		*	Pointer to the render window of the application
		*/
		Ogre::RenderWindow*					iWindow;
		/*
		*	Pointer to the current Scene Manager
		*/
		Ogre::SceneManager*					iSceneMgr;
		/*
		*	Has this singleton been activated and setup
		*	This is called after the base Ogre3D data(scene manager, window etc)
		*	has finished being setup and configured.
		*	@ true - its safe to begin accessing data from the singleton
		*	@ false - the class is instanced, but data is not setup yet and unsafe to access
		*/
		bool								iActivate;
		/*
		*	Has this singleton been instanced
		*	This is called during this classes contructor.
		*	@ true - its safe to begin assigning data to the singleton
		*	@ false - the class is instanced, but data is not setup yet and unsafe to access
		*/
		bool								iGlobalActivate;
		/*
		*	Pointer to the current Input Manager for OIS
		*/
		OIS::InputManager* iInputManager;
		/*
		*	Pointer to the current Mouse device
		*/
		OIS::Mouse*    iMouse;
		/*
		*	Pointer to the current Keyboard device
		*/
		OIS::Keyboard* iKeyboard;


		// Animation state for light swarm
		Ogre::AnimationState*				mMLAnimState;

		// pointer to the scene's main light/current active light
		Ogre::Light*						iMainLight;

		// vector of Ogre Node pointers to lights
		Ogre::vector<Ogre::Node*>::type		mLightNodes;


		//--------------------------------------------------------------------------
		// Recast Specific Shared Data

		/*
		*	An std::vector of Ogre::SceneNode* pointers.
		*	Used to provide a list of geometry to Recast
		*/
		NavSceneNodeList					mNavNodeList;
		/*
		*	Global debug log - tied into class OgreConsole in OgreTemplate::frameStarted
		*/
		rcLog mDbgLog;
		/*
		*	Detour query filter for search data
		*/
		dtQueryFilter m_filter;

		/*
		*	Enumeration of current NavMesh Test Tool type
		*/
		ToolMode m_toolMode;
		/*
		*	Enumeration to tell the application which mode it is in
		*	Terrain mode requires different rendering etc, so must know
		*	when to allow and when not to allow terrain updating/rendering etc
		*/
		ApplicationMode m_AppMode;
		/*
		*	An std::vector of CalcBlendData structs
		*	Use : to hold information for terrain generation
		*/
		BlendData m_BlendList;

		bool m_bidir;

		int m_areaType;
		float m_boxHeight;
		float m_boxDescent;

		Ogre::String currentMeshName;

};

#endif // __H_SHAREDDATA_H__
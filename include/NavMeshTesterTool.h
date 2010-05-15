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

//
// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#ifndef NAVMESHTESTERTOOL_H
#define NAVMESHTESTERTOOL_H

#include "OgreTemplate.h"
#include "DetourNavMesh.h"
#include "SharedData.h"
#include <vector>

#include "Vector2D.h"
#include "CellSpacePartition.h"
#include "BaseOgreRecastEntity.h"
#include "OgreRecastEntityFunctionTemplate.h"



#define MAXIMUM_ENTITIES 15

enum EntityToolMode
{
	ENTITY_NONE = 0,
	ENTITY_IDLE,
	ENTITY_FINDPATH,
	ENTITY_AUTOMATED,
};

class SinbadCharacterController;
class MovableTextOverlayAttributes;
class Obstacle;
class Wall2D;
class Path;

typedef std::vector<BaseGameEntity*>::iterator  ObIt;
typedef std::vector<SinbadCharacterController*> CharacterList;


class NavMeshTesterTool : public SampleTool
{
	OgreTemplate* m_sample;

	dtNavMesh* m_navMesh;

	ToolMode m_toolMode;

	static const int MAX_POLYS = 2048; //512;
	static const int MAX_SMOOTH = 12096;

	dtPolyRef m_startRef;
	dtPolyRef m_endRef;
	dtPolyRef m_polys[MAX_POLYS];
	dtPolyRef m_parent[MAX_POLYS];
	int m_npolys;
	float m_straightPath[MAX_POLYS*3];
	unsigned char m_straightPathFlags[MAX_POLYS];
	dtPolyRef m_straightPathPolys[MAX_POLYS];
	int m_nstraightPath;
	float m_polyPickExt[3];
	float m_smoothPath[MAX_SMOOTH*3];
	int m_nsmoothPath;

	float m_spos[3];
	float m_epos[3];
	float m_hitPos[3];
	float m_hitNormal[3];
	bool m_hitResult;
	float m_distanceToWall;
	bool m_sposSet;
	bool m_eposSet;

	int m_pathIterNum;
	const dtPolyRef* m_pathIterPolys; 
	int m_pathIterPolyCount;
	float m_prevIterPos[3], m_iterPos[3], m_steerPos[3], m_targetPos[3];

	static const int MAX_STEER_POINTS = 100;
	float m_steerPoints[MAX_STEER_POINTS*3];
	int m_steerPointCount;

	DebugDrawGL* dd;
	DebugDrawGL* ddAgent;
	DebugDrawGL* ddPolys;

	DebugDrawGL* ddCrosshair;
	DebugDrawGL* ddCellAgentView;
	DebugDrawGL* ddCellAgentNeigh;

	EntityToolMode m_EntityMode;

	CharacterList m_EntityList;
	std::vector<GameObject*> m_GameObjectList;
	unsigned int mCurrentEntities;
	float mframeTimeCount;

	MovableTextOverlayAttributes* m_OverlayAttributes;

	std::vector<SinbadCharacterController*> m_Vehicles;
	std::vector<BaseGameEntity*>  m_Obstacles;
	std::vector<Wall2D>           m_Walls;
	CellSpacePartition<SinbadCharacterController*>* m_pCellSpace;
	Vector2D	m_offSetVec;

	bool                          m_bPaused;
	int                           m_cxClient, m_cyClient;
	int                           m_cxClientMin, m_cyClientMin;
	
	Vector2D                      m_vCrosshair;
	double                         m_dAvFrameTime;
	//flags to turn aids and obstacles etc on/off
	bool  m_bShowWalls;
	bool  m_bShowObstacles;
	bool  m_bShowPath;
	bool  m_bShowDetectionBox;
	bool  m_bShowWanderCircle;
	bool  m_bShowFeelers;
	bool  m_bShowSteeringForce;
	bool  m_bShowFPS;
	bool  m_bRenderNeighbors;
	bool  m_bViewKeys;
	bool  m_bShowCellSpaceInfo;
	bool  m_bShowEntityLabels;

	void CreateObstacles();
	void CreateWalls();

	Ogre::RaySceneQuery* mRaySceneQuery;
	Ogre::SceneNode* mCurrentObjectSelection;
	Ogre::SceneNode* mLastObjectSelection;


public:
	NavMeshTesterTool();
	~NavMeshTesterTool();

	virtual int type() { return TOOL_NAVMESH_TESTER; }
	virtual void init(OgreTemplate* sample);
	virtual void reset();
	virtual void handleMenu();
	virtual void handleClick(const float* p, bool shift);
	virtual void handleStep();
	virtual void handleRender(float _timeSinceLastFrame);
	virtual void handleKeyStrokes(const OIS::KeyEvent &arg);
	virtual void handleMouseClick();
	virtual void handleMouseRelease();
	virtual void handleMouseMove( const OIS::MouseEvent &arg );

	void recalc();
	void drawAgent(const float* pos, float r, float h, float c, const unsigned int col);

	void setToolMode(ToolMode _mode);
	void setEntityMode(int _entityMode);

	void setEntityLabelsVisibility(bool _vis);
	bool getEntityLabelsVisibility(void)const{return m_bShowEntityLabels;}
	void toggleEntityLabels(void);

	void removeLatestEntity(void);
	// TODO : implement member functions for this get/set
	// public so its accessible from the GUI
	dtQueryFilter m_filter;


	void  NonPenetrationContraint(SinbadCharacterController* v){EnforceNonPenetrationConstraint(v, m_Vehicles);}

	void  TagVehiclesWithinViewRange(BaseGameEntity* pVehicle, double range)
	{
		TagNeighbors(pVehicle, m_Vehicles, range);
	}

	void  TagObstaclesWithinViewRange(BaseGameEntity* pVehicle, double range)
	{
		TagNeighbors(pVehicle, m_Obstacles, range);
	}

	const std::vector<Wall2D>&							Walls(){return m_Walls;}                          
	CellSpacePartition<SinbadCharacterController*>*     CellSpace(){return m_pCellSpace;}
	const std::vector<BaseGameEntity*>&					Obstacles()const{return m_Obstacles;}
	const std::vector<SinbadCharacterController*>&      Agents(){return m_Vehicles;}

	void        TogglePause(){m_bPaused = !m_bPaused;}
	bool        Paused()const{return m_bPaused;}

	Vector2D    Crosshair()const{return m_vCrosshair;}
	void        SetCrosshair(POINTS p);
	void        SetCrosshair(Vector2D v){m_vCrosshair=v;}

	int			cxClient()const{return m_cxClient;}
	int			cyClient()const{return m_cyClient;}
	int			cxClientMin()const{return m_cxClientMin;}
	int			cyClientMin()const{return m_cyClientMin;}

	bool		RenderWalls()const{return m_bShowWalls;}
	bool		RenderObstacles()const{return m_bShowObstacles;}
	bool		RenderPath()const{return m_bShowPath;}
	bool		RenderDetectionBox()const{return m_bShowDetectionBox;}
	bool		RenderWanderCircle()const{return m_bShowWanderCircle;}
	bool		RenderFeelers()const{return m_bShowFeelers;}
	bool		RenderSteeringForce()const{return m_bShowSteeringForce;}

	bool		RenderFPS()const{return m_bShowFPS;}
	void		ToggleShowFPS(){m_bShowFPS = !m_bShowFPS;}

	void		ToggleRenderNeighbors(){m_bRenderNeighbors = !m_bRenderNeighbors;}
	bool		RenderNeighbors()const{return m_bRenderNeighbors;}

	void		ToggleViewKeys(){m_bViewKeys = !m_bViewKeys;}
	bool		ViewKeys()const{return m_bViewKeys;}
};

#endif // NAVMESHTESTERTOOL_H
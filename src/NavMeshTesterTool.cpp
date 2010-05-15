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

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "Ogre.h"
#include "NavMeshTesterTool.h"
#include "OgreTemplate.h"
#include "InputGeom.h"
#include "MeshLoaderObj.h"
#include "GUtility.h"
#include "GUIManager.h"
#include "Recast.h"
#include "RecastDebugDraw.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"
#include "DetourDebugDraw.h"
#include "SinbadController.h"
#include "MoveableTextOverlay.h"
#include "timesm.h"
#include "database.h"
#include "msgroute.h"
#include "debuglog.h"
#include "gameobject.h"
#include "statemch.h"

#include "SharedData.h"


#include "Geometry.h"
#include "OgreRecastWall3D.h"
#include "Transformations.h"
#include "OgreRecastSteeringBehaviour.h"
#include "OgreRecastSettingLoader.h"
#include "OgreRecastObstacle.h"

#ifdef WIN32
#	define snprintf _snprintf
#endif

using namespace Ogre;
// Uncomment this to dump all the requests in stdout.
//#define DUMP_REQS

inline bool inRange(const float* v1, const float* v2, const float r, const float h)
{
	const float dx = v2[0] - v1[0];
	const float dy = v2[1] - v1[1];
	const float dz = v2[2] - v1[2];
	return (dx*dx + dz*dz) < r*r && fabsf(dy) < h;
}

static bool getSteerTarget(dtNavMesh* navMesh, const float* startPos, const float* endPos,
						   const float minTargetDist,
						   const dtPolyRef* path, const int pathSize,
						   float* steerPos, unsigned char& steerPosFlag, dtPolyRef& steerPosRef,
						   float* outPoints = 0, int* outPointCount = 0)							 
{
	// Find steer target.
	static const int MAX_STEER_POINTS = 3;
	float steerPath[MAX_STEER_POINTS*3];
	unsigned char steerPathFlags[MAX_STEER_POINTS];
	dtPolyRef steerPathPolys[MAX_STEER_POINTS];
	int nsteerPath = navMesh->findStraightPath(startPos, endPos, path, pathSize,
		steerPath, steerPathFlags, steerPathPolys, MAX_STEER_POINTS);
	if (!nsteerPath)
		return false;

	if (outPoints && outPointCount)
	{
		*outPointCount = nsteerPath;
		for (int i = 0; i < nsteerPath; ++i)
			rcVcopy(&outPoints[i*3], &steerPath[i*3]);
	}


	// Find vertex far enough to steer to.
	int ns = 0;
	while (ns < nsteerPath)
	{
		// Stop at Off-Mesh link or when point is further than slop away.
		if ((steerPathFlags[ns] & DT_STRAIGHTPATH_OFFMESH_CONNECTION) ||
			!inRange(&steerPath[ns*3], startPos, minTargetDist, 1000.0f))
			break;
		ns++;
	}
	// Failed to find good point to steer to.
	if (ns >= nsteerPath)
		return false;

	rcVcopy(steerPos, &steerPath[ns*3]);
	steerPosFlag = steerPathFlags[ns];
	steerPosRef = steerPathPolys[ns];

	return true;
}

static void getPolyCenter(dtNavMesh* navMesh, dtPolyRef ref, float* center)
{
	const dtPoly* p = navMesh->getPolyByRef(ref);
	if (!p) return;
	const float* verts = navMesh->getPolyVertsByRef(ref);
	center[0] = 0;
	center[1] = 0;
	center[2] = 0;
	for (int i = 0; i < (int)p->vertCount; ++i)
	{
		const float* v = &verts[p->verts[i]*3];
		center[0] += v[0];
		center[1] += v[1];
		center[2] += v[2];
	}
	const float s = 1.0f / p->vertCount;
	center[0] *= s;
	center[1] *= s;
	center[2] *= s;
}

NavMeshTesterTool::NavMeshTesterTool() :
		m_sample(0), m_navMesh(0), m_toolMode(TOOLMODE_PATHFIND_ITER), m_startRef(0),
		m_endRef(0), m_npolys(0), m_nstraightPath(0), m_nsmoothPath(0), m_hitResult(false),
		m_distanceToWall(0), m_sposSet(false), m_eposSet(false), m_pathIterNum(0), m_steerPointCount(0),
		dd(0), ddAgent(0), ddPolys(0), m_EntityMode(ENTITY_IDLE), mCurrentEntities(0), mframeTimeCount(0),
		m_OverlayAttributes(0), m_bPaused(false), m_bShowWalls(true), m_bShowObstacles(true),m_bShowPath(true),
		m_bShowWanderCircle(true), m_bShowSteeringForce(true), m_bShowFeelers(true), m_bShowDetectionBox(true),
		m_bShowFPS(true), m_dAvFrameTime(0), m_bRenderNeighbors(true), m_bViewKeys(true), mLastObjectSelection(0),
		m_bShowCellSpaceInfo(true), m_vCrosshair(Vector2D(0, 0)), mCurrentObjectSelection(0),
		m_bShowEntityLabels(true), ddCrosshair(0), ddCellAgentView(0), ddCellAgentNeigh(0),
		m_offSetVec(Vector2D(0, 0))
{
	m_filter.includeFlags = SAMPLE_POLYFLAGS_ALL;
	m_filter.excludeFlags = 0;

	m_polyPickExt[0] = 10;
	m_polyPickExt[1] = 20;
	m_polyPickExt[2] = 10;

	m_EntityList.resize(0);
	m_GameObjectList.resize(0);
}

NavMeshTesterTool::~NavMeshTesterTool()
{
	if (m_sample)
	{
		unsigned char flags = DU_DRAWNAVMESH_CLOSEDLIST;
		if (m_navMesh)
			flags |= DU_DRAWNAVMESH_OFFMESHCONS;
		m_sample->setNavMeshDrawFlags(flags);
	}

	if(dd)
	{
		delete dd; 
		dd = NULL;
	}
	if(ddAgent)
	{
		delete ddAgent; 
		ddAgent = NULL;
	}
	if(ddPolys)
	{
		delete ddPolys;
		ddPolys = NULL;
	}
	if(ddCrosshair)
	{
		delete ddCrosshair;
		ddCrosshair = NULL;
	}
	if(ddCellAgentView)
	{
		delete ddCellAgentView;
		ddCellAgentView = NULL;
	}
	if(ddCellAgentNeigh)
	{
		delete ddCellAgentNeigh;
		ddCellAgentNeigh = NULL;
	}
	if(m_pCellSpace)
	{
		delete m_pCellSpace;
		m_pCellSpace = NULL;
	}

	if(m_OverlayAttributes)
	{
		delete m_OverlayAttributes;
	}
	
	for(unsigned int i = 0; i < m_GameObjectList.size(); ++i)
	{
		g_database.Remove(static_cast<GameObject*>(m_GameObjectList[i])->GetID());
		delete m_GameObjectList[i];
	}
	m_GameObjectList.resize(0);
	m_EntityList.resize(0);
	m_Vehicles.resize(0);

	SharedData::getSingleton().iSceneMgr->destroyQuery(mRaySceneQuery);
}

void NavMeshTesterTool::removeLatestEntity(void)
{
	// TODO : make sure that we are removing an entity we want to remove
	// add ability to remove from places other than end of list

	if(m_GameObjectList.size() > 0)
	{
		g_database.Remove(static_cast<GameObject*>(m_GameObjectList[(unsigned int)(m_GameObjectList.size() - 1)])->GetID());
		delete m_GameObjectList[(unsigned int)(m_GameObjectList.size() - 1)];
		m_GameObjectList.pop_back();
		m_EntityList.pop_back();
		m_Vehicles.pop_back();
		--mCurrentEntities;
		m_sample->getGUI()->setEntitiesCreatedInfo(mCurrentEntities);
	}
}

void NavMeshTesterTool::init(OgreTemplate* sample)
{
	m_sample = sample;
	m_navMesh = sample->getNavMesh();
	recalc();

	// setup the bounds for our steering agents
	const float* maxBound = m_sample->getBoundsMax();
	const float* minBound = m_sample->getBoundsMin();
	m_cxClient = maxBound[0];
	m_cyClient = maxBound[2];
	m_cxClientMin = minBound[0];
	m_cyClientMin = minBound[2];
	
	float cx = ((m_cxClientMin - m_cxClient) / -1);
	float cy = ((m_cyClientMin - m_cyClient) / -1);
	m_offSetVec.x = m_cxClientMin;
	m_offSetVec.y = m_cyClientMin;
	//setup the spatial subdivision class
	m_pCellSpace = new CellSpacePartition<SinbadCharacterController*>((double)cx, (double)cy, Prm.NumCellsX, Prm.NumCellsY, Prm.NumAgents, (m_cxClientMin / -1), (m_cyClientMin / -1));


	if (m_navMesh)
	{
		// Change costs.
		m_navMesh->setAreaCost(SAMPLE_POLYAREA_GROUND, 1.0f);
		m_navMesh->setAreaCost(SAMPLE_POLYAREA_WATER, 10.0f);
		m_navMesh->setAreaCost(SAMPLE_POLYAREA_ROAD, 1.0f);
		m_navMesh->setAreaCost(SAMPLE_POLYAREA_DOOR, 1.0f);
		m_navMesh->setAreaCost(SAMPLE_POLYAREA_GRASS, 2.0f);
		m_navMesh->setAreaCost(SAMPLE_POLYAREA_JUMP, 1.5f);
	}

	if(m_toolMode == TOOLMODE_PATHFIND_ITER || m_toolMode == TOOLMODE_PATHFIND_STRAIGHT)
	{
		unsigned char flags = DU_DRAWNAVMESH_CLOSEDLIST;
		if (m_navMesh)
			flags |= DU_DRAWNAVMESH_OFFMESHCONS;
		m_sample->setNavMeshDrawFlags(flags);
	}

	dd = new DebugDrawGL();
	dd->setMaterialScript(Ogre::String("NavMeshTestLines"));
	dd->setOffset(0.35f);

	ddAgent = new DebugDrawGL();
	ddAgent->getMaterial()->getTechnique(0)->getPass(0)->setDiffuse(1.0f, 1.0f, 1.0f, 0); 
	ddAgent->getMaterial()->getTechnique(0)->getPass(0)->setAmbient(1.0f, 1.0f, 1.0f); 
	ddAgent->getMaterial()->getTechnique(0)->getPass(0)->setSelfIllumination(1.0f, 1.0f, 1.0f);
	ddAgent->getMaterial()->getTechnique(0)->getPass(0)->setNormaliseNormals(true);
	ddAgent->getMaterial()->getTechnique(0)->getPass(0)->setPointSize(6.0f);
	ddAgent->setOffset(0.35f);

	ddPolys = new DebugDrawGL();
	ddPolys->setMaterialScript(Ogre::String("NavMeshTestPolys"));
	ddPolys->setOffset(0.30f);

	ddCrosshair = new DebugDrawGL();
	ddCrosshair->setMaterialScript(Ogre::String("EntityLinesORANGE"));
	ddCellAgentView = new DebugDrawGL();
	ddCellAgentView->setMaterialScript(Ogre::String("EntityLinesGREEN"));
	ddCellAgentNeigh = new DebugDrawGL();
	ddCellAgentNeigh->setMaterialScript(Ogre::String("EntityLines"));

	m_OverlayAttributes = new MovableTextOverlayAttributes("Attrs1", SharedData::getSingleton().iCamera, "EntityLabel", 14, ColourValue::White, "RedTransparent");

	m_GameObjectList.resize(0);
	m_EntityList.resize(0);
	m_Vehicles.resize(0);

	// Create RaySceneQuery
	mRaySceneQuery = SharedData::getSingleton().iSceneMgr->createRayQuery(Ogre::Ray());
}

void NavMeshTesterTool::handleMenu()
{
	if (m_toolMode == TOOLMODE_PATHFIND_ITER || m_toolMode == TOOLMODE_PATHFIND_STRAIGHT)
	{
		unsigned char flags = DU_DRAWNAVMESH_CLOSEDLIST;
		if (m_navMesh)
			flags |= DU_DRAWNAVMESH_OFFMESHCONS;
		m_sample->setNavMeshDrawFlags(flags);
	}
	else
	{
		unsigned char flags = 0;
		if (m_navMesh)
			flags |= DU_DRAWNAVMESH_OFFMESHCONS;
		m_sample->setNavMeshDrawFlags(flags);
	}
}

void NavMeshTesterTool::handleClick(const float* p, bool shift)
{
	if (shift)
	{
		m_sposSet = true;
		rcVcopy(m_spos, p);
		// place an entity if we are in Entity Mode and we haven't reached that max entities yet
		if(m_toolMode == TOOLMODE_ENTITY_DEMO)
		{
			for(unsigned int i = 0; i < m_EntityList.size(); ++i)
			{
				m_EntityList[i]->setIsSelected(false);
			}
			if(mCurrentEntities < MAXIMUM_ENTITIES)
			{	// cycle and set all selected to zero - then at end set newly made ent to selected state true
				GameObject* SinbadEntity = new GameObject( g_database.GetNewObjectID(), OBJECT_Enemy | OBJECT_Character, const_cast<char*>(TemplateUtils::GetUniqueName("SinbadControl_").c_str()));
				g_database.Store( *SinbadEntity );
				m_GameObjectList.push_back( SinbadEntity );
				SinbadCharacterController* chara = new SinbadCharacterController(m_sample, *SinbadEntity, this, Vector2D(m_spos[0],m_spos[2]),
					RandFloat()*TwoPi,  Vector2D(0,0), Prm.VehicleMass, Prm.MaxSteeringForce, Prm.MaxSpeed, Prm.MaxTurnRatePerSecond, Prm.VehicleScale );
				chara->setLabelAttributes(m_OverlayAttributes);
				chara->Initialize();
				if(SharedData::getSingleton().m_AppMode == APPMODE_TERRAINSCENE)
				{
					chara->GetBodyNode()->setScale(5.0f, 5.0f, 5.0f);
					chara->setCharHeightVal((5.0f * 5.0f));
					chara->SetScale(Vector2D(7, 10));
				}
				else
				{
					chara->GetBodyNode()->setScale(2.5f, 2.5f, 2.5f);
					chara->setCharHeightVal((5.0f * 2.5f));
					chara->SetScale(Vector2D(5, 6));
				}
				chara->GetBodyNode()->setPosition(m_spos[0], m_spos[1], m_spos[2]);
				chara->setGroundHeight(m_spos[1]);
				chara->setSinbadPosition(m_spos[0], m_spos[1], m_spos[2]);
				chara->setPathStart(Ogre::Vector3(m_spos[0], m_spos[1], m_spos[2]));
				chara->setInitialPosition(Ogre::Vector3(m_spos[0], m_spos[1], m_spos[2]));
				chara->setHasMoved(true);

				SinbadEntity->PushStateMachine(*chara);
				m_EntityList.push_back(chara);
				chara->setEntityMode(m_EntityMode);

				m_pCellSpace->AddEntity(chara);
				chara->Steering()->FollowPathOff();
				chara->Steering()->FlockingOff();
				chara->Steering()->SeparationOn();
				chara->Steering()->ObstacleAvoidanceOn();
				chara->Steering()->ToggleSpacePartitioningOnOff();
				chara->SmoothingOn();

				m_Vehicles.push_back(chara);

				chara->sendModeChangeMessage();
				++mCurrentEntities;
				m_sample->getGUI()->setEntitiesCreatedInfo(mCurrentEntities);
			}
		}
	}
	else
	{
		m_eposSet = true;
		rcVcopy(m_epos, p);
		m_vCrosshair.x = m_epos[0];
		m_vCrosshair.yUP = m_epos[1];
		m_vCrosshair.y = m_epos[2];
		handleMouseClick();
	}
	recalc();
}

void NavMeshTesterTool::handleStep()
{
	// TODO: merge separate to a path iterator. Use same code in recalc() too.
	if (m_toolMode != TOOLMODE_PATHFIND_ITER)
		return;

	if (!m_sposSet || !m_eposSet || !m_startRef || !m_endRef)
		return;

	static const float STEP_SIZE = 0.5f;
	static const float SLOP = 0.01f;

	if (m_pathIterNum == 0)
	{
		m_npolys = m_navMesh->findPath(m_startRef, m_endRef, m_spos, m_epos, &m_filter, m_polys, MAX_POLYS);
		m_nsmoothPath = 0;

		m_pathIterPolys = m_polys; 
		m_pathIterPolyCount = m_npolys;

		if (m_pathIterPolyCount)
		{
			// Iterate over the path to find smooth path on the detail mesh surface.

			m_navMesh->closestPointOnPolyBoundary(m_startRef, m_spos, m_iterPos);
			m_navMesh->closestPointOnPolyBoundary(m_pathIterPolys[m_pathIterPolyCount-1], m_epos, m_targetPos);

			m_nsmoothPath = 0;

			rcVcopy(&m_smoothPath[m_nsmoothPath*3], m_iterPos);
			m_nsmoothPath++;
		}
	}

	rcVcopy(m_prevIterPos, m_iterPos);

	m_pathIterNum++;

	if (!m_pathIterPolyCount)
		return;

	if (m_nsmoothPath >= MAX_SMOOTH)
		return;

	// Move towards target a small advancement at a time until target reached or
	// when ran out of memory to store the path.

	// Find location to steer towards.
	float steerPos[3];
	unsigned char steerPosFlag;
	dtPolyRef steerPosRef;

	if (!getSteerTarget(m_navMesh, m_iterPos, m_targetPos, SLOP,
		m_pathIterPolys, m_pathIterPolyCount, steerPos, steerPosFlag, steerPosRef,
		m_steerPoints, &m_steerPointCount))
		return;

	rcVcopy(m_steerPos, steerPos);

	bool endOfPath = (steerPosFlag & DT_STRAIGHTPATH_END) ? true : false;
	bool offMeshConnection = (steerPosFlag & DT_STRAIGHTPATH_OFFMESH_CONNECTION) ? true : false;

	// Find movement delta.
	float delta[3], len;
	rcVsub(delta, steerPos, m_iterPos);
	len = sqrtf(rcVdot(delta,delta));
	// If the steer target is end of path or off-mesh link, do not move past the location.
	if ((endOfPath || offMeshConnection) && len < STEP_SIZE)
		len = 1;
	else
		len = STEP_SIZE / len;
	float moveTgt[3];
	rcVmad(moveTgt, m_iterPos, delta, len);

	// Move
	float result[3];
	int n = m_navMesh->moveAlongPathCorridor(m_iterPos, moveTgt, result, m_pathIterPolys, m_pathIterPolyCount);
	float h = 0;
	m_navMesh->getPolyHeight(m_pathIterPolys[n], result, &h);
	result[1] = h;
	// Shrink path corridor if advanced.
	if (n)
	{
		m_pathIterPolys += n;
		m_pathIterPolyCount -= n;
	}
	// Update position.
	rcVcopy(m_iterPos, result);

	// Handle end of path and off-mesh links when close enough.
	if (endOfPath && inRange(m_iterPos, steerPos, SLOP, 1.0f))
	{
		// Reached end of path.
		rcVcopy(m_iterPos, m_targetPos);
		if (m_nsmoothPath < MAX_SMOOTH)
		{
			rcVcopy(&m_smoothPath[m_nsmoothPath*3], m_iterPos);
			m_nsmoothPath++;
		}
		return;
	}
	else if (offMeshConnection && inRange(m_iterPos, steerPos, SLOP, 1.0f))
	{
		// Reached off-mesh connection.
		float startPos[3], endPos[3];

		// Advance the path up to and over the off-mesh connection.
		dtPolyRef prevRef = 0, polyRef = m_pathIterPolys[0];
		while (m_pathIterPolyCount && polyRef != steerPosRef)
		{
			prevRef = polyRef;
			polyRef = m_pathIterPolys[0];
			m_pathIterPolys++;
			m_pathIterPolyCount--;
		}

		// Handle the connection.
		if (m_navMesh->getOffMeshConnectionPolyEndPoints(prevRef, polyRef, startPos, endPos))
		{
			if (m_nsmoothPath < MAX_SMOOTH)
			{
				rcVcopy(&m_smoothPath[m_nsmoothPath*3], startPos);
				m_nsmoothPath++;
				// Hack to make the dotted path not visible during off-mesh connection.
				if (m_nsmoothPath & 1)
				{
					rcVcopy(&m_smoothPath[m_nsmoothPath*3], startPos);
					m_nsmoothPath++;
				}
			}
			// Move position at the other side of the off-mesh link.
			rcVcopy(m_iterPos, endPos);
			float h;
			m_navMesh->getPolyHeight(m_pathIterPolys[0], m_iterPos, &h);
			m_iterPos[1] = h;
		}
	}

	// Store results.
	if (m_nsmoothPath < MAX_SMOOTH)
	{
		rcVcopy(&m_smoothPath[m_nsmoothPath*3], m_iterPos);
		m_nsmoothPath++;
	}

}

void NavMeshTesterTool::reset()
{
	if(dd)
	{
		delete dd; 
		dd = NULL;
	}
	if(ddAgent)
	{
		delete ddAgent; 
		ddAgent = NULL;
	}
	if(ddPolys)
	{
		delete ddPolys;
		ddPolys = NULL;
	}
	if(ddCrosshair)
	{
		delete ddCrosshair;
		ddCrosshair = NULL;
	}
	if(ddCellAgentView)
	{
		delete ddCellAgentView;
		ddCellAgentView = NULL;
	}
	if(ddCellAgentNeigh)
	{
		delete ddCellAgentNeigh;
		ddCellAgentNeigh = NULL;
	}
	if(m_pCellSpace)
	{
		delete m_pCellSpace;
		m_pCellSpace = NULL;
	}

	if(m_OverlayAttributes)
	{
		delete m_OverlayAttributes;
	}

	for(unsigned int i = 0; i < m_GameObjectList.size(); ++i)
	{
		g_database.Remove(static_cast<GameObject*>(m_GameObjectList[i])->GetID());
		delete m_GameObjectList[i];
	}
	m_GameObjectList.resize(0);
	m_EntityList.resize(0);
	m_Vehicles.resize(0);

	m_startRef = 0;
	m_endRef = 0;
	m_npolys = 0;
	m_nstraightPath = 0;
	m_nsmoothPath = 0;
	memset(m_hitPos, 0, sizeof(m_hitPos));
	memset(m_hitNormal, 0, sizeof(m_hitNormal));
	m_distanceToWall = 0;
	mframeTimeCount = 0.0f;
	mCurrentEntities = 0;

	SharedData::getSingleton().iSceneMgr->destroyQuery(mRaySceneQuery);
}


void NavMeshTesterTool::recalc()
{
	if (!m_navMesh)
		return;

	if (m_sposSet)
		m_startRef = m_navMesh->findNearestPoly(m_spos, m_polyPickExt, &m_filter, 0);
	else
		m_startRef = 0;

	if (m_eposSet)
		m_endRef = m_navMesh->findNearestPoly(m_epos, m_polyPickExt, &m_filter, 0);
	else
		m_endRef = 0;

	if (m_toolMode == TOOLMODE_PATHFIND_ITER)
	{
		m_pathIterNum = 0;
		if (m_sposSet && m_eposSet && m_startRef && m_endRef)
		{
#ifdef DUMP_REQS
			printf("pi  %f %f %f  %f %f %f  0x%x 0x%x\n",
				m_spos[0],m_spos[1],m_spos[2], m_epos[0],m_epos[1],m_epos[2],
				m_filter.includeFlags, m_filter.excludeFlags); 
			rcGetLog()->log(RC_LOG_PROGRESS, "pi  %f %f %f  %f %f %f  0x%x 0x%x\n",
				m_spos[0],m_spos[1],m_spos[2], m_epos[0],m_epos[1],m_epos[2],
				m_filter.includeFlags, m_filter.excludeFlags );
#endif

			m_npolys = m_navMesh->findPath(m_startRef, m_endRef, m_spos, m_epos, &m_filter, m_polys, MAX_POLYS);

			m_nsmoothPath = 0;

			if (m_npolys)
			{
				// Iterate over the path to find smooth path on the detail mesh surface.
				const dtPolyRef* polys = m_polys; 
				int npolys = m_npolys;

				float iterPos[3], targetPos[3];
				m_navMesh->closestPointOnPolyBoundary(m_startRef, m_spos, iterPos);
				m_navMesh->closestPointOnPolyBoundary(polys[npolys-1], m_epos, targetPos);

				static const float STEP_SIZE = 0.5f;
				static const float SLOP = 0.01f;

				m_nsmoothPath = 0;

				rcVcopy(&m_smoothPath[m_nsmoothPath*3], iterPos);
				m_nsmoothPath++;

				// Move towards target a small advancement at a time until target reached or
				// when ran out of memory to store the path.
				while (npolys && m_nsmoothPath < MAX_SMOOTH)
				{
					// Find location to steer towards.
					float steerPos[3];
					unsigned char steerPosFlag;
					dtPolyRef steerPosRef;

					if (!getSteerTarget(m_navMesh, iterPos, targetPos, SLOP,
						polys, npolys, steerPos, steerPosFlag, steerPosRef))
						break;

					bool endOfPath = (steerPosFlag & DT_STRAIGHTPATH_END) ? true : false;
					bool offMeshConnection = (steerPosFlag & DT_STRAIGHTPATH_OFFMESH_CONNECTION) ? true : false;

					// Find movement delta.
					float delta[3], len;
					rcVsub(delta, steerPos, iterPos);
					len = sqrtf(rcVdot(delta,delta));
					// If the steer target is end of path or off-mesh link, do not move past the location.
					if ((endOfPath || offMeshConnection) && len < STEP_SIZE)
						len = 1;
					else
						len = STEP_SIZE / len;
					float moveTgt[3];
					rcVmad(moveTgt, iterPos, delta, len);

					// Move
					float result[3];
					int n = m_navMesh->moveAlongPathCorridor(iterPos, moveTgt, result, polys, npolys);
					float h = 0;
					m_navMesh->getPolyHeight(polys[n], result, &h);
					result[1] = h;
					// Shrink path corridor if advanced.
					if (n)
					{
						polys += n;
						npolys -= n;
					}
					// Update position.
					rcVcopy(iterPos, result);

					// Handle end of path and off-mesh links when close enough.
					if (endOfPath && inRange(iterPos, steerPos, SLOP, 1.0f))
					{
						// Reached end of path.
						rcVcopy(iterPos, targetPos);
						if (m_nsmoothPath < MAX_SMOOTH)
						{
							rcVcopy(&m_smoothPath[m_nsmoothPath*3], iterPos);
							m_nsmoothPath++;
						}
						break;
					}
					else if (offMeshConnection && inRange(iterPos, steerPos, SLOP, 1.0f))
					{
						// Reached off-mesh connection.
						float startPos[3], endPos[3];

						// Advance the path up to and over the off-mesh connection.
						dtPolyRef prevRef = 0, polyRef = polys[0];
						while (npolys && polyRef != steerPosRef)
						{
							prevRef = polyRef;
							polyRef = polys[0];
							polys++;
							npolys--;
						}

						// Handle the connection.
						if (m_navMesh->getOffMeshConnectionPolyEndPoints(prevRef, polyRef, startPos, endPos))
						{
							if (m_nsmoothPath < MAX_SMOOTH)
							{
								rcVcopy(&m_smoothPath[m_nsmoothPath*3], startPos);
								m_nsmoothPath++;
								// Hack to make the dotted path not visible during off-mesh connection.
								if (m_nsmoothPath & 1)
								{
									rcVcopy(&m_smoothPath[m_nsmoothPath*3], startPos);
									m_nsmoothPath++;
								}
							}
							// Move position at the other side of the off-mesh link.
							rcVcopy(iterPos, endPos);
							float h;
							m_navMesh->getPolyHeight(polys[0], iterPos, &h);
							iterPos[1] = h;
						}
					}

					// Store results.
					if (m_nsmoothPath < MAX_SMOOTH)
					{
						rcVcopy(&m_smoothPath[m_nsmoothPath*3], iterPos);
						m_nsmoothPath++;
					}
				}
			}
		}
		else
		{
			m_npolys = 0;
			m_nsmoothPath = 0;
		}
	}
	else if (m_toolMode == TOOLMODE_PATHFIND_STRAIGHT)
	{
		if (m_sposSet && m_eposSet && m_startRef && m_endRef)
		{
#ifdef DUMP_REQS
			printf("ps  %f %f %f  %f %f %f  0x%x 0x%x\n",
				m_spos[0],m_spos[1],m_spos[2], m_epos[0],m_epos[1],m_epos[2],
				m_filter.includeFlags, m_filter.excludeFlags); 
			rcGetLog()->log(RC_LOG_PROGRESS, "ps  %f %f %f  %f %f %f  0x%x 0x%x\n",
				m_spos[0],m_spos[1],m_spos[2], m_epos[0],m_epos[1],m_epos[2],
				m_filter.includeFlags, m_filter.excludeFlags);
#endif
			m_npolys = m_navMesh->findPath(m_startRef, m_endRef, m_spos, m_epos, &m_filter, m_polys, MAX_POLYS);
			m_nstraightPath = 0;
			if (m_npolys)
			{
				m_nstraightPath = m_navMesh->findStraightPath(m_spos, m_epos, m_polys, m_npolys,
					m_straightPath, m_straightPathFlags,
					m_straightPathPolys, MAX_POLYS);
			}
		}
		else
		{
			m_npolys = 0;
			m_nstraightPath = 0;
		}
	}
	else if (m_toolMode == TOOLMODE_RAYCAST)
	{
		m_nstraightPath = 0;
		if (m_sposSet && m_eposSet && m_startRef)
		{
#ifdef DUMP_REQS
			printf("rc  %f %f %f  %f %f %f  0x%x 0x%x\n",
				m_spos[0],m_spos[1],m_spos[2], m_epos[0],m_epos[1],m_epos[2],
				m_filter.includeFlags, m_filter.excludeFlags); 
			rcGetLog()->log(RC_LOG_PROGRESS, "rc  %f %f %f  %f %f %f  0x%x 0x%x\n",
				m_spos[0],m_spos[1],m_spos[2], m_epos[0],m_epos[1],m_epos[2],
				m_filter.includeFlags, m_filter.excludeFlags);
#endif
			float t = 0;
			m_npolys = 0;
			m_nstraightPath = 2;
			m_straightPath[0] = m_spos[0];
			m_straightPath[1] = m_spos[1];
			m_straightPath[2] = m_spos[2];
			m_npolys = m_navMesh->raycast(m_startRef, m_spos, m_epos, &m_filter, t, m_hitNormal, m_polys, MAX_POLYS);
			if (t > 1)
			{
				// No hit
				rcVcopy(m_hitPos, m_epos);
				m_hitResult = false;
			}
			else
			{
				// Hit
				m_hitPos[0] = m_spos[0] + (m_epos[0] - m_spos[0]) * t;
				m_hitPos[1] = m_spos[1] + (m_epos[1] - m_spos[1]) * t;
				m_hitPos[2] = m_spos[2] + (m_epos[2] - m_spos[2]) * t;
				if (m_npolys)
				{
					float h = 0;
					m_navMesh->getPolyHeight(m_polys[m_npolys-1], m_hitPos, &h);
					m_hitPos[1] = h;
				}
				m_hitResult = true;
			}
			rcVcopy(&m_straightPath[3], m_hitPos);
		}
	}
	else if (m_toolMode == TOOLMODE_DISTANCE_TO_WALL)
	{
		m_distanceToWall = 0;
		if (m_sposSet && m_startRef)
		{
#ifdef DUMP_REQS
			printf("dw  %f %f %f  %f  0x%x 0x%x\n",
				m_spos[0],m_spos[1],m_spos[2], 100.0f,
				m_filter.includeFlags, m_filter.excludeFlags); 
			rcGetLog()->log(RC_LOG_PROGRESS, "dw  %f %f %f  %f  0x%x 0x%x\n",
				m_spos[0],m_spos[1],m_spos[2], 100.0f,
				m_filter.includeFlags, m_filter.excludeFlags); 
#endif
			m_distanceToWall = m_navMesh->findDistanceToWall(m_startRef, m_spos, 100.0f, &m_filter, m_hitPos, m_hitNormal);
		}
	}
	else if (m_toolMode == TOOLMODE_FIND_POLYS_AROUND)
	{
		if (m_sposSet && m_startRef && m_eposSet)
		{
			const float dx = m_epos[0] - m_spos[0];
			const float dz = m_epos[2] - m_spos[2];
			float dist = sqrtf(dx*dx + dz*dz);
#ifdef DUMP_REQS
			printf("fp  %f %f %f  %f  0x%x 0x%x\n",
				m_spos[0],m_spos[1],m_spos[2], dist,
				m_filter.includeFlags, m_filter.excludeFlags); 
			rcGetLog()->log(RC_LOG_PROGRESS, "fp  %f %f %f  %f  0x%x 0x%x\n",
				m_spos[0],m_spos[1],m_spos[2], dist,
				m_filter.includeFlags, m_filter.excludeFlags);
#endif
			m_npolys = m_navMesh->findPolysAround(m_startRef, m_spos, dist, &m_filter, m_polys, m_parent, 0, MAX_POLYS);
		}
	}
}

void NavMeshTesterTool::setToolMode(ToolMode _mode)
{
	m_toolMode = _mode;
	recalc();
}

void NavMeshTesterTool::setEntityMode(int _entityMode)
{
	switch(_entityMode)
	{
	case 0:
		m_EntityMode = ENTITY_NONE;
		for(unsigned int i = 0; i < m_EntityList.size(); ++i)
		{
			m_EntityList[i]->setEntityMode(ENTITY_NONE);
			//m_EntityList[i]->sendModeChangeMessage();
		}
		break;
	case 1:
		m_EntityMode = ENTITY_IDLE;
		for(unsigned int i = 0; i < m_EntityList.size(); ++i)
		{
			m_EntityList[i]->setEntityMode(ENTITY_IDLE);
			m_EntityList[i]->sendModeChangeMessage();
		}
		break;
	case 2:
		m_EntityMode = ENTITY_FINDPATH;
		for(unsigned int i = 0; i < m_EntityList.size(); ++i)
		{
			m_EntityList[i]->setEntityMode(ENTITY_FINDPATH);
			m_EntityList[i]->sendModeChangeMessage();
		}
		break;
	case 3:
		m_EntityMode = ENTITY_AUTOMATED;
		for(unsigned int i = 0; i < m_EntityList.size(); ++i)
		{
			m_EntityList[i]->setEntityMode(ENTITY_AUTOMATED);
			m_EntityList[i]->sendModeChangeMessage();
		}
		break;
	case 4:
	case 5:
		m_EntityMode = ENTITY_IDLE;
		break;
	default:
		m_EntityMode = ENTITY_IDLE;
	}
}

void NavMeshTesterTool::handleRender(float _timeSinceLastFrame)
{

	if(!dd || !ddAgent || !ddPolys || !ddCrosshair || !ddCellAgentView || !ddCellAgentNeigh)
		return;
	
	dd->clear();
	ddAgent->clear();
	ddPolys->clear();
	ddCrosshair->clear();
	ddCellAgentView->clear();
	ddCellAgentNeigh->clear();

	mframeTimeCount += _timeSinceLastFrame;

	if (m_toolMode == TOOLMODE_PATHFIND_ITER || m_toolMode == TOOLMODE_PATHFIND_STRAIGHT)
	{
		unsigned char flags = DU_DRAWNAVMESH_CLOSEDLIST;
		if (m_navMesh)
			flags |= DU_DRAWNAVMESH_OFFMESHCONS;
		m_sample->setNavMeshDrawFlags(flags);
	}
	else
	{
		unsigned char flags = 0;
		if (m_navMesh)
			flags |= DU_DRAWNAVMESH_OFFMESHCONS;
		m_sample->setNavMeshDrawFlags(flags);
	}


	static const unsigned int startCol = duRGBA(128,25,0,192);
	static const unsigned int endCol = duRGBA(51,102,0,129);
	static const unsigned int pathCol = duRGBA(0,0,0,64);

	const float agentRadius = m_sample->getAgentRadius();
	const float agentHeight = m_sample->getAgentHeight();
	const float agentClimb = m_sample->getAgentClimb();

	if (!m_navMesh)
	{
		return;
	}

	if(m_toolMode != TOOLMODE_ENTITY_DEMO)
	{

		ddAgent->depthMask(false);
		if (m_sposSet)
			drawAgent(m_spos, agentRadius, agentHeight, agentClimb, startCol);
		if (m_eposSet)
			drawAgent(m_epos, agentRadius, agentHeight, agentClimb, endCol);
		ddAgent->depthMask(true);
	}

	// HANDLE ENTITY UPDATING -----------------------------------------------------------
	if(m_toolMode == TOOLMODE_ENTITY_DEMO)
	{
		// TODO : enable GUI options to turn this drawing on and off
		// code already exists for turning on and off, just have to add
		// it to gui
		for(unsigned int i = 0; i < m_EntityList.size(); ++i)
		{
			m_EntityList[i]->addTime(_timeSinceLastFrame, SharedData::getSingleton().m_AppMode);

			if(mframeTimeCount >= 5.0f)
				m_EntityList[i]->sendThinkMessage();

			//render cell partitioning stuff
			if (m_pCellSpace && m_bShowCellSpaceInfo && i%3 == 0)
			{
				//gdi->HollowBrush();
				InvertedAABBox2D box(m_Vehicles[i]->Pos() - Vector2D(Prm.ViewDistance, Prm.ViewDistance),
									 m_Vehicles[i]->Pos() + Vector2D(Prm.ViewDistance, Prm.ViewDistance));
				box.Render(ddCellAgentView);

				//gdi->RedPen();
				ddCellAgentNeigh->begin(DU_DRAW_LINES, 5.0f);
				CellSpace()->CalculateNeighbors(m_Vehicles[i]->Pos(), Prm.ViewDistance);
				for (BaseGameEntity* pV = CellSpace()->begin();!CellSpace()->end();pV = CellSpace()->next())
				{
					duAppendCircle(ddCellAgentNeigh, pV->Pos().x, pV->Pos().yUP, pV->Pos().y, pV->BRadius(), (unsigned int)0);
				}
				ddCellAgentNeigh->end();

				ddCellAgentView->begin(DU_DRAW_LINES, 5.0f);
				duAppendCircle(ddCellAgentView, m_Vehicles[i]->Pos().x, m_Vehicles[i]->GetBodyNode()->getPosition().y, m_Vehicles[i]->Pos().y, Prm.ViewDistance, (unsigned int)0);
				ddCellAgentView->end();
			}
		}
		// render any steering walls
		for (unsigned int w=0; w<m_Walls.size(); ++w)
		{
			m_Walls[w].Render(true);  //true flag shows normals
		}
		//render any steering obstacles
		for (unsigned int ob=0; ob<m_Obstacles.size(); ++ob)
		{
			m_Obstacles[ob]->Render();
		}

		ddCrosshair->begin(DU_DRAW_LINES, 5.0f);
		duAppendCircle(ddCrosshair, m_vCrosshair.x, m_vCrosshair.yUP+5.0f, m_vCrosshair.y, 4, (unsigned int)0);
		duAppendCross(ddCrosshair, m_vCrosshair.x, m_vCrosshair.yUP+5.0f, m_vCrosshair.y, 8, (unsigned int)0);
		ddCrosshair->end();

		// render cellspace stuff
		if (m_bShowCellSpaceInfo)
		{
			if(m_pCellSpace)
				m_pCellSpace->RenderCells(dd, m_offSetVec.x, m_offSetVec.y);
		}
	}
	// RENDER ITERATIVE PATH -----------------------------------------------------------
	else if (m_toolMode == TOOLMODE_PATHFIND_ITER)
	{
		duDebugDrawNavMeshPoly(ddPolys, *m_navMesh, m_startRef, startCol);
		duDebugDrawNavMeshPoly(ddPolys, *m_navMesh, m_endRef, endCol);

		if (m_npolys)
		{
			for (int i = 1; i < m_npolys-1; ++i)
				duDebugDrawNavMeshPoly(ddPolys, *m_navMesh, m_polys[i], pathCol);
		}

		if (m_nsmoothPath)
		{
			dd->depthMask(false);
			const unsigned int pathCol = duRGBA(0,0,0,220);
			dd->begin(DU_DRAW_LINES, 3.0f);
			for (int i = 0; i < m_nsmoothPath; ++i)
				dd->vertex(m_smoothPath[i*3], m_smoothPath[i*3+1]+0.1f, m_smoothPath[i*3+2], pathCol);
			dd->end();
			dd->depthMask(true);
		}

		if (m_pathIterNum)
		{
			duDebugDrawNavMeshPoly(ddPolys, *m_navMesh, m_pathIterPolys[0], duRGBA(255,255,255,128));

			dd->depthMask(false);
			dd->begin(DU_DRAW_LINES, 1.0f);

			const unsigned int prevCol = duRGBA(255,192,0,220);
			const unsigned int curCol = duRGBA(255,255,255,220);
			const unsigned int steerCol = duRGBA(0,192,255,220);

			dd->vertex(m_prevIterPos[0],m_prevIterPos[1]-0.3f,m_prevIterPos[2], prevCol);
			dd->vertex(m_prevIterPos[0],m_prevIterPos[1]+0.3f,m_prevIterPos[2], prevCol);

			dd->vertex(m_iterPos[0],m_iterPos[1]-0.3f,m_iterPos[2], curCol);
			dd->vertex(m_iterPos[0],m_iterPos[1]+0.3f,m_iterPos[2], curCol);

			dd->vertex(m_prevIterPos[0],m_prevIterPos[1]+0.3f,m_prevIterPos[2], prevCol);
			dd->vertex(m_iterPos[0],m_iterPos[1]+0.3f,m_iterPos[2], prevCol);

			dd->vertex(m_prevIterPos[0],m_prevIterPos[1]+0.3f,m_prevIterPos[2], steerCol);
			dd->vertex(m_steerPos[0],m_steerPos[1]+0.3f,m_steerPos[2], steerCol);

			for (int i = 0; i < m_steerPointCount-1; ++i)
			{
				dd->vertex(m_steerPoints[i*3+0],m_steerPoints[i*3+1]+0.2f,m_steerPoints[i*3+2], duDarkenColor(steerCol));
				dd->vertex(m_steerPoints[(i+1)*3+0],m_steerPoints[(i+1)*3+1]+0.2f,m_steerPoints[(i+1)*3+2], duDarkenColor(steerCol));
			}

			dd->end();
			dd->depthMask(true);
		}
	}
	// RENDER STRAIGHT PATH -----------------------------------------------------------
	else if (m_toolMode == TOOLMODE_PATHFIND_STRAIGHT)
	{
		duDebugDrawNavMeshPoly(ddPolys, *m_navMesh, m_startRef, startCol);
		duDebugDrawNavMeshPoly(ddPolys, *m_navMesh, m_endRef, endCol);

		if (m_npolys)
		{
			for (int i = 1; i < m_npolys-1; ++i)
				duDebugDrawNavMeshPoly(ddPolys, *m_navMesh, m_polys[i], pathCol);
		}

		if (m_nstraightPath)
		{
			dd->depthMask(false);
			const unsigned int pathCol = duRGBA(64,16,0,220);
			const unsigned int offMeshCol = duRGBA(128,96,0,220);
			dd->begin(DU_DRAW_LINES, 2.0f);
			for (int i = 0; i < m_nstraightPath-1; ++i)
			{
				unsigned int col = 0;
				if (m_straightPathFlags[i] & DT_STRAIGHTPATH_OFFMESH_CONNECTION)
					col = offMeshCol;
				else
					col = pathCol;

				dd->vertex(m_straightPath[i*3], m_straightPath[i*3+1]+0.4f, m_straightPath[i*3+2], col);
				dd->vertex(m_straightPath[(i+1)*3], m_straightPath[(i+1)*3+1]+0.4f, m_straightPath[(i+1)*3+2], col);
			}
			dd->end();
			dd->begin(DU_DRAW_POINTS, 6.0f);
			for (int i = 0; i < m_nstraightPath; ++i)
			{
				unsigned int col = 0;
				if (m_straightPathFlags[i] & DT_STRAIGHTPATH_START)
					col = startCol;
				else if (m_straightPathFlags[i] & DT_STRAIGHTPATH_START)
					col = endCol;
				else if (m_straightPathFlags[i] & DT_STRAIGHTPATH_OFFMESH_CONNECTION)
					col = offMeshCol;
				else
					col = pathCol;
				dd->vertex(m_straightPath[i*3], m_straightPath[i*3+1]+0.4f, m_straightPath[i*3+2], pathCol);
			}
			dd->end();
			dd->depthMask(true);
		}
	}
	// RENDER RAY CAST TEST PATH -----------------------------------------------------------
	else if (m_toolMode == TOOLMODE_RAYCAST)
	{
		duDebugDrawNavMeshPoly(ddPolys, *m_navMesh, m_startRef, startCol);

		if (m_nstraightPath)
		{
			for (int i = 1; i < m_npolys; ++i)
				duDebugDrawNavMeshPoly(ddPolys, *m_navMesh, m_polys[i], pathCol);

			dd->depthMask(false);
			const unsigned int pathCol = m_hitResult ? duRGBA(64,16,0,220) : duRGBA(240,240,240,220);
			dd->begin(DU_DRAW_LINES, 2.0f);
			for (int i = 0; i < m_nstraightPath-1; ++i)
			{
				dd->vertex(m_straightPath[i*3], m_straightPath[i*3+1]+0.4f, m_straightPath[i*3+2], pathCol);
				dd->vertex(m_straightPath[(i+1)*3], m_straightPath[(i+1)*3+1]+0.4f, m_straightPath[(i+1)*3+2], pathCol);
			}
			dd->end();
			dd->begin(DU_DRAW_POINTS, 4.0f);
			for (int i = 0; i < m_nstraightPath; ++i)
				dd->vertex(m_straightPath[i*3], m_straightPath[i*3+1]+0.4f, m_straightPath[i*3+2], pathCol);
			dd->end();

			if (m_hitResult)
			{
				const unsigned int hitCol = duRGBA(0,0,0,128);
				dd->begin(DU_DRAW_LINES, 2.0f);
				dd->vertex(m_hitPos[0], m_hitPos[1] + 0.4f, m_hitPos[2], hitCol);
				dd->vertex(m_hitPos[0] + m_hitNormal[0]*agentRadius,
					m_hitPos[1] + 0.4f + m_hitNormal[1]*agentRadius,
					m_hitPos[2] + m_hitNormal[2]*agentRadius, hitCol);
				dd->end();
			}
			dd->depthMask(true);
		}
	}
	// RENDER DISTANCE TO WALL -----------------------------------------------------------
	else if (m_toolMode == TOOLMODE_DISTANCE_TO_WALL)
	{
		duDebugDrawNavMeshPoly(ddPolys, *m_navMesh, m_startRef, startCol);
		dd->depthMask(false);
		duDebugDrawCircle(dd, m_spos[0], m_spos[1]+agentHeight/2, m_spos[2], m_distanceToWall, duRGBA(64,16,0,220), 2.0f);
		dd->begin(DU_DRAW_LINES, 3.0f);
		dd->vertex(m_hitPos[0], m_hitPos[1] + 0.02f, m_hitPos[2], duRGBA(0,0,0,192));
		dd->vertex(m_hitPos[0], m_hitPos[1] + agentHeight, m_hitPos[2], duRGBA(0,0,0,192));
		dd->end();
		dd->depthMask(true);
	}
	// RENDER POLYGON CONNECTIONS -----------------------------------------------------------
	else if (m_toolMode == TOOLMODE_FIND_POLYS_AROUND)
	{
		for (int i = 0; i < m_npolys; ++i)
		{
			duDebugDrawNavMeshPoly(ddPolys, *m_navMesh, m_polys[i], pathCol);
			dd->depthMask(false);
			if (m_parent[i])
			{
				float p0[3], p1[3];
				dd->depthMask(false);
				getPolyCenter(m_navMesh, m_parent[i], p0);
				getPolyCenter(m_navMesh, m_polys[i], p1);
				duDebugDrawArc(dd, p0[0],p0[1],p0[2], p1[0],p1[1],p1[2], 0.25f, 0.0f, 0.4f, duRGBA(0,0,0,128), 2.0f);
				dd->depthMask(true);
			}
			dd->depthMask(true);
		}

		if (m_sposSet && m_eposSet)
		{
			dd->depthMask(false);
			const float dx = m_epos[0] - m_spos[0];
			const float dz = m_epos[2] - m_spos[2];
			const float dist = sqrtf(dx*dx + dz*dz);
			duDebugDrawCircle(dd, m_spos[0], m_spos[1]+agentHeight/2, m_spos[2], dist, duRGBA(64,16,0,220), 2.0f);
			dd->depthMask(true);
		}
	}

	if(mframeTimeCount >= 5.0f)
		mframeTimeCount = 0.0f;
}


void NavMeshTesterTool::drawAgent(const float* pos, float r, float h, float c, const unsigned int col)
{
	ddAgent->depthMask(false);

	// Agent dimensions.	
	duDebugDrawCylinderWire(ddAgent, pos[0]-r, pos[1]+0.02f, pos[2]-r, pos[0]+r, pos[1]+h, pos[2]+r, col, 2.0f);

	duDebugDrawCircle(ddAgent, pos[0],pos[1]+c,pos[2],r,duRGBA(0,0,0,64),1.0f);

	
	ddAgent->begin(DU_DRAW_LINES);
		ddAgent->vertex(pos[0], pos[1]-c, pos[2], Ogre::ColourValue(0, 0, 1));
		ddAgent->vertex(pos[0], pos[1]+c, pos[2], Ogre::ColourValue(0, 0, 1));
		ddAgent->vertex(pos[0]-r/2, pos[1]+0.02f, pos[2], Ogre::ColourValue(0, 0, 1));
		ddAgent->vertex(pos[0]+r/2, pos[1]+0.02f, pos[2], Ogre::ColourValue(0, 0, 1));
		ddAgent->vertex(pos[0], pos[1]+0.02f, pos[2]-r/2, Ogre::ColourValue(0, 0, 1));
		ddAgent->vertex(pos[0], pos[1]+0.02f, pos[2]+r/2, Ogre::ColourValue(0, 0, 1));
	ddAgent->end();
	

	ddAgent->depthMask(true);
}

//------------------------------------------------------------------------
// -- UNUSED - sets walls from base 0 - TODO : alter to create boundary for 
// nav mesh
void NavMeshTesterTool::CreateWalls()
{
	//create the walls  
	double bordersize = 20.0;
	double CornerSize = 0.2;
	double vDist = m_cyClient-2*bordersize;
	double hDist = m_cxClient-2*bordersize;

	const int NumWallVerts = 8;

	Vector2D walls[NumWallVerts] = {Vector2D(hDist*CornerSize+bordersize, bordersize),
		Vector2D(m_cxClient-bordersize-hDist*CornerSize, bordersize),
		Vector2D(m_cxClient-bordersize, bordersize+vDist*CornerSize),
		Vector2D(m_cxClient-bordersize, m_cyClient-bordersize-vDist*CornerSize),

		Vector2D(m_cxClient-bordersize-hDist*CornerSize, m_cyClient-bordersize),
		Vector2D(hDist*CornerSize+bordersize, m_cyClient-bordersize),
		Vector2D(bordersize, m_cyClient-bordersize-vDist*CornerSize),
		Vector2D(bordersize, bordersize+vDist*CornerSize)};

	for (int w=0; w<NumWallVerts-1; ++w)
	{
		m_Walls.push_back(Wall2D(walls[w], walls[w+1]));
	}

	m_Walls.push_back(Wall2D(walls[NumWallVerts-1], walls[0]));
}

//------------------------------------------------------------------------
// -- UNUSED - TODO : setup to create steering obstacles in the recast world, where recast
// sees obstacles, so that our steering entities are more accurate about where they can
// and cannot go, they have sensory memory so can tell if they are going to hit something or not
// the code for the memory is all there, it is not implemented atm
void NavMeshTesterTool::CreateObstacles()
{
	//create a number of randomly sized tiddlywinks
	for (int o=0; o<Prm.NumObstacles; ++o)
	{   
		bool bOverlapped = true;

		//keep creating tiddlywinks until we find one that doesn't overlap
		//any others.Sometimes this can get into an endless loop because the
		//obstacle has nowhere to fit. We test for this case and exit accordingly

		int NumTrys = 0; int NumAllowableTrys = 2000;

		while (bOverlapped)
		{
			NumTrys++;

			if (NumTrys > NumAllowableTrys) return;

			int radius = RandInt((int)Prm.MinObstacleRadius, (int)Prm.MaxObstacleRadius);

			const int border                 = 10;
			const int MinGapBetweenObstacles = 20;

			Obstacle* ob = new Obstacle(RandInt(radius+border, m_cxClient-radius-border),
				RandInt(radius+border, m_cyClient-radius-30-border),
				radius);

			if (!Overlapped(ob, m_Obstacles, MinGapBetweenObstacles))
			{
				//its not overlapped so we can add it
				m_Obstacles.push_back(ob);

				bOverlapped = false;
			}

			else
			{
				delete ob;
			}
		}
	}
}

//------------------------- Set Crosshair ------------------------------------
//-- UNUSED MOSTLY - used to set a place for flocks/moving entities to flee/seek/hide from/pursue/evade etc
void NavMeshTesterTool::SetCrosshair(POINTS p)
{
	Vector2D ProposedPosition((double)p.x, (double)p.y);

	//make sure it's not inside an obstacle
	for (ObIt curOb = m_Obstacles.begin(); curOb != m_Obstacles.end(); ++curOb)
	{
		if (PointInCircle((*curOb)->Pos(), (*curOb)->BRadius(), ProposedPosition))
		{
			return;
		}

	}
	m_vCrosshair.x = (double)p.x;
	m_vCrosshair.y = (double)p.y;
}

//------------------------------------------------------------------------------
// -- DEBUG HANDLER FOR KEYSTROKES - displayed in entity's label atm - messy
void NavMeshTesterTool::handleKeyStrokes(const OIS::KeyEvent &arg)
{
  if(m_toolMode == TOOLMODE_ENTITY_DEMO)
  {
	switch(arg.key)
	{
	case OIS::KC_INSERT:
		for(unsigned int i = 0; i < m_Vehicles.size(); ++i)
		{
			m_Vehicles[i]->SetMaxForce(m_Vehicles[i]->MaxForce() + 1000.0f*m_Vehicles[i]->TimeElapsed());
		}
		break;
	case OIS::KC_DELETE:
		for(unsigned int i = 0; i < m_Vehicles.size(); ++i)
		{
			if (m_Vehicles[i]->MaxForce() > 0.2f) 
				m_Vehicles[i]->SetMaxForce(m_Vehicles[i]->MaxForce() - 1000.0f*m_Vehicles[i]->TimeElapsed());
		}
		break;
	case OIS::KC_HOME:
		for(unsigned int i = 0; i < m_Vehicles.size(); ++i)
		{
			m_Vehicles[i]->SetMaxSpeed(m_Vehicles[i]->MaxSpeed() + 50.0f*m_Vehicles[i]->TimeElapsed());
		}
		break;
	case OIS::KC_END:
		for(unsigned int i = 0; i < m_Vehicles.size(); ++i)
		{
			if (m_Vehicles[i]->MaxSpeed() > 0.2f) 
				m_Vehicles[i]->SetMaxSpeed(m_Vehicles[i]->MaxSpeed() - 50.0f*m_Vehicles[i]->TimeElapsed());
		}
		break;
	case OIS::KC_P:
		for(unsigned int i = 0; i < m_Vehicles.size(); ++i)
		{
			m_Vehicles[i]->Steering()->setWaypointSeekDistance( (5.0 + m_Vehicles[i]->Steering()->WaypointSeekDistance()) );
		}
		break;
	case OIS::KC_L:
		for(unsigned int i = 0; i < m_Vehicles.size(); ++i)
		{
			if(m_Vehicles[i]->Steering()->WaypointSeekDistance() > 6.0)
			{
				m_Vehicles[i]->Steering()->setWaypointSeekDistance( (m_Vehicles[i]->Steering()->WaypointSeekDistance() - 5.0) );
			}
		}
		break;
	default:
		break;
	}

	for(unsigned int i = 0; i < m_Vehicles.size(); ++i)
	{
		if (m_Vehicles[i]->MaxForce() < 0) m_Vehicles[i]->SetMaxForce(0.0f);
		if (m_Vehicles[i]->MaxSpeed() < 0) m_Vehicles[i]->SetMaxSpeed(0.0f);

		Ogre::String capText = "MaxForce : " + Ogre::StringConverter::toString((float)m_Vehicles[i]->MaxForce()) + "\n" +
								"MaxSpeed : " + Ogre::StringConverter::toString((float)m_Vehicles[i]->MaxSpeed()) + "\n" + 
								"WP Seek  : " + Ogre::StringConverter::toString((float)m_Vehicles[i]->Steering()->WaypointSeekDistance());
		m_Vehicles[i]->setEntityLabelCaption(capText, 3, 15);
	}
  }
}

void NavMeshTesterTool::handleMouseClick()
{
	using namespace Ogre;

	if(m_toolMode == TOOLMODE_ENTITY_DEMO)
	{
		// cycle through objects and select one if we have mouse over one
		Ogre::Ray mouseRay = SharedData::getSingleton().iCamera->getCameraToViewportRay(m_sample->MouseCursorX(), m_sample->MouseCursorY()); 
		mRaySceneQuery->setRay(mouseRay);
		mRaySceneQuery->setSortByDistance(true, 1);
		mRaySceneQuery->setQueryMask(SINBAD_MASK); // only return results that are equal to SINBAD_MASK

		const RaySceneQueryResult& result = mRaySceneQuery->execute();
		mCurrentObjectSelection = NULL;
		mLastObjectSelection = NULL;
		for(unsigned int u = 0; u < m_Vehicles.size(); ++u)
		{
			m_Vehicles[u]->setIsSelected(false);
		}
		if (!result.empty())
		{
			RaySceneQueryResult::const_iterator i = result.begin();
			
			mRaySceneQuery->setSortByDistance (true, 1);//only one hit -- IS THIS NEEDED HERE OR BEFORE EXECUTE ?
			while((i != result.end()))
			{
				mCurrentObjectSelection = i->movable->getParentSceneNode();
				mLastObjectSelection = i->movable->getParentSceneNode();
				Ogre::String selName = mCurrentObjectSelection->getName();
				for(unsigned int u = 0; u < m_Vehicles.size(); ++u)
				{
					m_Vehicles[u]->setIsSelected(false);
					if(m_Vehicles[u]->GetBodyNode()->getName() == selName)
					{
						m_Vehicles[u]->setIsSelected(true);
						break;
					}
				}
				++i;
			}
		}
	}
}

void NavMeshTesterTool::handleMouseMove( const OIS::MouseEvent &arg )
{
	if(m_toolMode == TOOLMODE_ENTITY_DEMO)
	{
		if(m_sample->getMouseLeftState())
		{
		 if(mCurrentObjectSelection)
		 {
			// code to move current selected object
			if(SharedData::getSingleton().m_AppMode == APPMODE_TERRAINSCENE)
			{
					//mCurrentObjectSelection->setPosition(m_sample->getInputGeom()->getMesh()->mTerrainGroup->getHeightAtWorldPosition(m_sample->MouseCursorX(), 5000, m_sample->MouseCursorY()));
					Ogre::Ray mouseRay = SharedData::getSingleton().iCamera->getCameraToViewportRay(m_sample->MouseCursorX(), m_sample->MouseCursorY()); 
					TerrainGroup::RayResult rayResult = m_sample->getInputGeom()->getMesh()->mTerrainGroup->rayIntersects(mouseRay);
					if (rayResult.hit)
					{	
						mCurrentObjectSelection->setPosition(rayResult.position.x, rayResult.position.y+25.0f, rayResult.position.z);
						for(unsigned int v = 0; v < m_Vehicles.size(); ++v)
						{
							if( m_Vehicles[v]->getIsSelected() )
							{
								m_Vehicles[v]->SetPos(Vector2D(rayResult.position.x, rayResult.position.y+25.0f, rayResult.position.z));
								m_Vehicles[v]->SetVelocity(Vector2D(0.0, 0.0, 0.0));
							}
						}
					}
			}
			else
			{
				// use scenequery ? to place object at world position
					Ogre::Ray mouseRay = SharedData::getSingleton().iCamera->getCameraToViewportRay(m_sample->MouseCursorX(), m_sample->MouseCursorY()); 
					Ogre::Vector3 posHit = Ogre::Vector3::ZERO;
					float rayst[3];
					float rayen[3];
					float tt = 0;
					memset(rayst, 0, sizeof(rayst));
					memset(rayen, 0, sizeof(rayen));
					rayst[0] = (float)mouseRay.getOrigin().x; rayst[1] = (float)mouseRay.getOrigin().y; rayst[2] = (float)mouseRay.getOrigin().z;
					rayen[0] = (float)mouseRay.getPoint(10000.0).x; rayen[1] = (float)mouseRay.getPoint(10000.0).y; rayen[2] = (float)mouseRay.getPoint(10000.0).z;
					m_sample->getInputGeom()->raycastMesh(rayst, rayen, tt);
					posHit.x = rayst[0] + (rayen[0] - rayst[0])*tt;
					posHit.y = rayst[1] + (rayen[1] - rayst[1])*tt;
					posHit.z = rayst[2] + (rayen[2] - rayst[2])*tt;

					mCurrentObjectSelection->setPosition(posHit.x, posHit.y+12.5f, posHit.z);
					for(unsigned int v = 0; v < m_Vehicles.size(); ++v)
					{
						if( m_Vehicles[v]->getIsSelected())
						{
							m_Vehicles[v]->SetPos(Vector2D(posHit.x, posHit.y+12.5f, posHit.z));
							m_Vehicles[v]->SetVelocity(Vector2D(0.0, 0.0, 0.0));
						}
					}
			}
		 }
		}
	}
}

void NavMeshTesterTool::handleMouseRelease()
{
	if(m_toolMode == TOOLMODE_ENTITY_DEMO)
	{
		for(unsigned int i = 0; i < m_Vehicles.size(); ++i)
		{
			if( (m_Vehicles[i]->getIsSelected()) && (m_Vehicles[i]->getEntityMode() != ENTITY_MODE_IDLE) )
			{
				m_Vehicles[i]->sendFindNewPathMessage();
			}
			m_Vehicles[i]->setIsSelected(false);
		}
	}
}

void NavMeshTesterTool::setEntityLabelsVisibility(bool _vis)
{
	if(m_toolMode == TOOLMODE_ENTITY_DEMO)
	{
		m_bShowEntityLabels = _vis;
		toggleEntityLabels();
	}
}

void NavMeshTesterTool::toggleEntityLabels(void)
{
	if(m_toolMode == TOOLMODE_ENTITY_DEMO)
	{
		for(unsigned int i = 0; i < m_Vehicles.size(); ++i)
		{
			m_Vehicles[i]->setEntityLabelVisible(m_bShowEntityLabels);
		}
	}
}
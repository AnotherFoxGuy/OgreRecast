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
#include <float.h>

#include "Ogre.h"
#include "OgreTemplate.h"
#include "OffMeshConnectionTool.h"
#include "InputGeom.h"
#include "SharedData.h"
#include "Recast.h"
#include "RecastDebugDraw.h"
#include "DetourDebugDraw.h"

#include "GUIManager.h"

#ifdef WIN32
#	define snprintf _snprintf
#endif

OffMeshConnectionTool::OffMeshConnectionTool() :
	m_sample(0),
	m_hitPosSet(0),
	m_bidir(true),
	m_oldFlags(0),
	dd(0),
	ddOffMesh(0)
{
}

OffMeshConnectionTool::~OffMeshConnectionTool()
{
	if (m_sample)
	{
		m_sample->setNavMeshDrawFlags(m_oldFlags);
	}
	if(dd)
	{
		delete dd; 
		dd = NULL;
	}
	if(ddOffMesh)
	{
		delete ddOffMesh; 
		ddOffMesh = NULL;
	}
}

void OffMeshConnectionTool::init(OgreTemplate* sample)
{
	m_sample = sample;
	if (m_sample)
	{
		m_oldFlags = m_sample->getNavMeshDrawFlags();
		m_sample->setNavMeshDrawFlags(m_oldFlags & ~DU_DRAWNAVMESH_OFFMESHCONS);
	}

	dd = new DebugDrawGL();
	dd->setMaterialScript(Ogre::String("OffMeshConnection"));
	dd->setOffset(0.35f);

	ddOffMesh = new DebugDrawGL();
	ddOffMesh->setMaterialScript(Ogre::String("OffMeshConnection"));
	ddOffMesh->setOffset(0.35f);
}

void OffMeshConnectionTool::reset()
{
	m_hitPosSet = false;
	if(dd)
	{
		delete dd; 
		dd = NULL;
	}
	if(ddOffMesh)
	{
		delete ddOffMesh; 
		ddOffMesh = NULL;
	}
}

void OffMeshConnectionTool::handleMenu()
{
}

void OffMeshConnectionTool::handleClick(const float* p, bool shift)
{
	if (!m_sample) return;
	InputGeom* geom = m_sample->getInputGeom();
	if (!geom) return;

	if (shift)
	{
		// Delete
		// Find nearest link end-point
		float nearestDist = FLT_MAX;
		int nearestIndex = -1;
		const float* verts = geom->getOffMeshConnectionVerts();
		for (int i = 0; i < geom->getOffMeshConnectionCount()*2; ++i)
		{
			const float* v = &verts[i*3];
			float d = rcVdistSqr(p, v);
			if (d < nearestDist)
			{
				nearestDist = d;
				nearestIndex = i/2; // Each link has two vertices.
			}
		}
		// If end point close enough, delete it.
		if (nearestIndex != -1 &&
			sqrtf(nearestDist) < m_sample->getAgentRadius())
		{
			geom->deleteOffMeshConnection(nearestIndex);
		}
	}
	else
	{
		// Create	
		if (!m_hitPosSet)
		{
			rcVcopy(m_hitPos, p);
			m_hitPosSet = true;
		}
		else
		{
			const unsigned char area = SAMPLE_POLYAREA_JUMP;
			const unsigned short flags = SAMPLE_POLYFLAGS_JUMP; 
			geom->addOffMeshConnection(m_hitPos, p, m_sample->getAgentRadius(), m_bidir ? 1 : 0, area, flags);
			m_hitPosSet = false;
		}
	}
	
}

void OffMeshConnectionTool::handleStep()
{
}

void OffMeshConnectionTool::handleRender(float _timeSinceLastFrame)
{
	dd->clear();

	const float s = m_sample->getAgentRadius();

	if(m_bidir != SharedData::getSingleton().m_bidir)
		m_bidir = SharedData::getSingleton().m_bidir;

	if (!m_hitPosSet)
	{
		m_sample->getGUI()->setOffMeshText("Click to set connection start.");
	}
	else
	{
		m_sample->getGUI()->setOffMeshText("Click to set connection end.");
	}
	
	if (m_hitPosSet)
		duDebugDrawCross(dd, m_hitPos[0],m_hitPos[1]+0.1f,m_hitPos[2], s, duRGBA(0,0,0,128), 2.0f);

	InputGeom* geom = m_sample->getInputGeom();
	if (geom)
		geom->drawOffMeshConnections(ddOffMesh, true);
}
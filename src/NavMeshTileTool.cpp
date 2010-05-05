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

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <float.h>

#include "Ogre.h"
#include "OgreTemplate.h"
#include "NavMeshTileTool.h"
#include "InputGeom.h"
#include "SharedData.h"
#include "Recast.h"
#include "RecastDebugDraw.h"
#include "DetourDebugDraw.h"

#include "GUIManager.h"

#ifdef WIN32
#	define snprintf _snprintf
#endif


NavMeshTileTool::NavMeshTileTool():
		m_sample(0),
		m_hitPosSet(false),
		m_agentRadius(0),
		dd(0)
{
	m_hitPos[0] = m_hitPos[1] = m_hitPos[2] = 0;
}

NavMeshTileTool::~NavMeshTileTool()
{
	if(dd)
	{
		delete dd;
		dd = NULL;
	}
}


void NavMeshTileTool::init(OgreTemplate* _sample)
{
	m_sample = _sample; 

	dd = new DebugDrawGL();
	dd->setMaterialScript(Ogre::String("ActiveTiles"));
	dd->setOffset(0.20f);

}

void NavMeshTileTool::reset()
{
	if(dd)
	{
		delete dd;
		dd = NULL;
	}
}

void NavMeshTileTool::handleMenu()
{	
}

void NavMeshTileTool::handleClick(const float* p, bool shift)
{
	m_hitPosSet = true;
	rcVcopy(m_hitPos,p);
	if (m_sample)
	{
		if (shift)
			m_sample->removeTile(m_hitPos);
		else
			m_sample->buildTile(m_hitPos);
	}
}

void NavMeshTileTool::handleStep()
{

}

void NavMeshTileTool::handleRender()
{
	dd->clear();

	if (m_hitPosSet)
	{
		const float s = m_sample->getAgentRadius();
		//glColor4ub(0,0,0,128);
		dd->begin(DU_DRAW_LINES, 10.0f);
		dd->vertex(m_hitPos[0]-s, m_hitPos[1]+0.1f, m_hitPos[2], Ogre::ColourValue(0.1, 0.1, 0.1));
		dd->vertex(m_hitPos[0]+s, m_hitPos[1]+0.1f, m_hitPos[2], Ogre::ColourValue(0.1, 0.1, 0.1));
		dd->vertex(m_hitPos[0], m_hitPos[1]-s+0.1f, m_hitPos[2] , Ogre::ColourValue(0.1, 0.1, 0.1));
		dd->vertex(m_hitPos[0], m_hitPos[1]+s+0.1f, m_hitPos[2] , Ogre::ColourValue(0.1, 0.1, 0.1));
		dd->vertex(m_hitPos[0], m_hitPos[1]+0.1f, m_hitPos[2]-s , Ogre::ColourValue(0.1, 0.1, 0.1));
		dd->vertex(m_hitPos[0], m_hitPos[1]+0.1f, m_hitPos[2]+s , Ogre::ColourValue(0.1, 0.1, 0.1));
		dd->end();
		m_hitPosSet = false;
	}
	
}
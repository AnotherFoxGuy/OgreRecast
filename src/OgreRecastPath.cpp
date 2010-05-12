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

#include "OgreRecastPath.h"
#include "MiscUtils.h"
#include "Transformations.h"


//------------------------------------------------------------------------------------
// will have to create new methods based on this but working with the detour data
// so that we can generate valid paths randomly and efficiently - dont want to worry
// about selecting geom with raycasting and getting something outside the navmesh
std::list<Vector2D> Path::CreateRandomPath(int   NumWaypoints,
										   double MinX,
										   double MinY,
										   double MaxX,
										   double MaxY)
{
	m_WayPoints.clear();

	double midX = (MaxX+MinX)/2.0;
	double midY = (MaxY+MinY)/2.0;

	double smaller = std::min(midX, midY);

	double spacing = TwoPi/(double)NumWaypoints;

	for (int i=0; i<NumWaypoints; ++i)
	{
		double RadialDist = RandInRange(smaller*0.2f, smaller);

		Vector2D temp(RadialDist, 0.0f);

		Vec2DRotateAroundOrigin(temp, i*spacing);

		temp.x += midX; temp.y += midY;

		m_WayPoints.push_back(temp);

	}

	curWaypoint = m_WayPoints.begin();

	return m_WayPoints;
}

//------------------------------------------------------------------------------------
// -- DEBUG ONLY VISUALS
void Path::Render()const
{
	dd->clear();
	//gdi->OrangePen();

	std::list<Vector2D>::const_iterator it = m_WayPoints.begin();
	dd->depthMask(false);
	dd->begin(DU_DRAW_LINES_STRIP, 10);
	Vector2D wp = *it;

	while (it != m_WayPoints.end())
	{
		dd->vertex(wp.x, wp.yUP+10.0f, wp.y, (unsigned int)0);

		wp = *it++;
	}
	dd->vertex(wp.x, wp.yUP+10.0f, wp.y, (unsigned int)0);
	
	if (m_bLooped) 
		dd->vertex((*m_WayPoints.begin()).x, (*m_WayPoints.begin()).yUP+10.0f, (*m_WayPoints.begin()).y, (unsigned int)0);

	dd->depthMask(true);
	dd->end();
}

//------------------------------------------------------------------------------------
void Path::initDD(void)
{
	if(dd != NULL)
		delete dd; 
	dd = new DebugDrawGL();
	dd->setMaterialScript(Ogre::String("EntityLines"));
	dd->setOffset(0.35f);
}
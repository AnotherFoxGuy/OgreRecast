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

#ifndef __H_OGRERECASTPATH_H_
#define __H_OGRERECASTPATH_H_

#include <list>
#include <cassert>

#include "Vector2D.h"
#include "OgreTemplate.h"


class Path
{
private:

	std::list<Vector2D>            m_WayPoints;

	//points to the current waypoint
	std::list<Vector2D>::iterator  curWaypoint;

	//flag to indicate if the path should be looped
	//(The last waypoint connected to the first)
	bool                           m_bLooped;

	// flag to indicate if we have reached the end of the path
	bool						   m_bFinishedPath;
	// flag to indicate if we have had at least one complete run through of the path
	// used to tell us if we need to start looking for a new path
	bool						   m_bOneLoopDone;

	DebugDrawGL* dd;

public:

	Path():m_bLooped(false),m_bFinishedPath(false),m_bOneLoopDone(false),dd(0){ initDD(); }

	//constructor for creating a path with initial random waypoints. MinX/Y
	//& MaxX/Y define the bounding box of the path.
	Path(int    NumWaypoints,
		double MinX,
		double MinY,
		double MaxX,
		double MaxY,
		bool   looped):m_bLooped(looped),m_bFinishedPath(false),m_bOneLoopDone(false),dd(0)
	{
		CreateRandomPath(NumWaypoints, MinX, MinY, MaxX, MaxY);

		curWaypoint = m_WayPoints.begin();

		initDD();
	}

	~Path() { if(dd){delete dd; dd=0;} }
	// initialize the debug drawer object
	void initDD(void);
	//returns the current waypoint
	Vector2D    CurrentWaypoint()const
	{
		assert(*curWaypoint != Vector2D(0, 0)); 
		return *curWaypoint;
	}

	//returns true if the end of the list has been reached
	bool        Finished(){return !(curWaypoint != m_WayPoints.end());}

	//moves the iterator on to the next waypoint in the list
	inline void SetNextWaypoint();

	//creates a random path which is bound by rectangle described by
	//the min/max values
	std::list<Vector2D> CreateRandomPath(int    NumWaypoints,
		double MinX,
		double MinY,
		double MaxX,
		double MaxY);


	void LoopOn(){m_bLooped = true;}
	void LoopOff(){m_bLooped = false;}
	void SetLoop(bool _loop) { m_bLooped = _loop; }
	bool GetLoop(){return m_bLooped;}

	// returns true if the path has been finished, false otherwise
	// used in path following obviously, to avoid asserts on reaching end and waiting
	// for new path to be generated
	bool PathFinished(){ return m_bFinishedPath; }

	// returns true of we have done at least one run-through of the path, without loops
	// false if we have not dont at least one run though of the path yet.
	// used to indicate if we should start look for a new path
	bool GetPathRunThroughOnce(void) { return m_bOneLoopDone; }
	// used to set if we have been through the path once. called after we get a TRUE on GetPathRunThroughOnce()
	// and we have generated a new path.
	void SetPathRunThroughOnce(bool _onceThrough) { m_bOneLoopDone = _onceThrough; }

	//adds a waypoint to the end of the path
	void AddWayPoint(Vector2D new_point) { m_WayPoints.push_back(new_point); curWaypoint = m_WayPoints.begin(); /*curWaypoint++;*/ }

	//methods for setting the path with either another Path or a list of vectors
	void Set(std::list<Vector2D> new_path)
	{
		m_WayPoints.resize(0);
		m_WayPoints = new_path;
		curWaypoint = m_WayPoints.begin();
	}
	void Set(const Path& path)
	{
		m_WayPoints.resize(0);
		m_WayPoints=path.GetPath(); 
		curWaypoint = m_WayPoints.begin();
	}


	void Clear(){m_WayPoints.clear();}

	std::list<Vector2D> GetPath()const{return m_WayPoints;}

	//renders the path in orange
	void Render()const; 
};




//-------------------- Methods -------------------------------------------

inline void Path::SetNextWaypoint()
{
	assert (m_WayPoints.size() > 0);

	if(m_bFinishedPath == false)
	{
		if (++curWaypoint == m_WayPoints.end())
		{
			if (m_bLooped)
			{
				curWaypoint = m_WayPoints.begin(); 
			}
			else
			{
				m_bFinishedPath = true;
				m_bOneLoopDone = true;
				std::list<Vector2D> new_path;
				new_path.resize(0);
				
				std::list<Vector2D>::const_iterator it = m_WayPoints.begin();
				Vector2D wp = *it;
				while (it != m_WayPoints.end())
				{
					new_path.push_front(wp);
					wp = *it++;
				}
				m_WayPoints.resize(0);
				Set(new_path);
			}
		}
	}
	else if(m_bFinishedPath == true)
	{
		if (++curWaypoint == m_WayPoints.end())
		{
			if (m_bLooped)
			{
				curWaypoint = m_WayPoints.begin(); 
			}
			else
			{
				m_bFinishedPath = false;
				std::list<Vector2D> new_path;
				new_path.resize(0);

				std::list<Vector2D>::const_iterator it = m_WayPoints.begin();
				Vector2D wp = *it;
				while (it != m_WayPoints.end())
				{
					new_path.push_front(wp);
					wp = *it++;
				}
	
				m_WayPoints.resize(0);
				Set(new_path);
			}
		}
	}
}  



#endif // __H_OGRERECASTPATH_H_
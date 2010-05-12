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

#ifndef __H_OGRERECAST_ENTITY_FUNCTION_TEMPLATES_H_
#define __H_OGRERECAST_ENTITY_FUNCTION_TEMPLATES_H_

#include "BaseOgreRecastEntity.h"
#include "SinbadController.h"
#include "Geometry.h"
#include "MiscUtils.h"


class SinbadCharacterController;
class BaseGameEntity;
//////////////////////////////////////////////////////////////////////////
//
//  Some useful template functions
//
//////////////////////////////////////////////////////////////////////////

//------------------------- Overlapped -----------------------------------
//
//  tests to see if an entity is overlapping any of a number of entities
//  stored in a std container
//------------------------------------------------------------------------
template <class T, class conT>
bool Overlapped(const T* ob, const conT& conOb, double MinDistBetweenObstacles = 40.0)
{
	typename conT::const_iterator it;

	for (it=conOb.begin(); it != conOb.end(); ++it)
	{
		if (TwoCirclesOverlapped(ob->Pos(),
			ob->BRadius()+MinDistBetweenObstacles,                             
			(*it)->Pos(),
			(*it)->BRadius()))
		{
			return true;
		}
	}

	return false;
}

//----------------------- TagNeighbors ----------------------------------
//
//  tags any entities contained in a std container that are within the
//  radius of the single entity parameter
//------------------------------------------------------------------------
template <class T, class conT>
void TagNeighbors(const T& entity, conT& ContainerOfEntities, double radius)
{
	//iterate through all entities checking for range
	for (typename conT::iterator curEntity = ContainerOfEntities.begin();
		curEntity != ContainerOfEntities.end();
		++curEntity)
	{
		//first clear any current tag
		(*curEntity)->UnTag();

		Vector2D to = (*curEntity)->Pos() - entity->Pos();

		//the bounding radius of the other is taken into account by adding it 
		//to the range
		double range = radius + (*curEntity)->BRadius();

		//if entity within range, tag for further consideration. (working in
		//distance-squared space to avoid sqrts)
		if ( ((*curEntity) != entity) && (to.LengthSq() < range*range))
		{
			(*curEntity)->Tag();
		}

	}//next entity
}


//------------------- EnforceNonPenetrationConstraint ---------------------
//
//  Given a pointer to an entity and a std container of pointers to nearby
//  entities, this function checks to see if there is an overlap between
//  entities. If there is, then the entities are moved away from each
//  other
//------------------------------------------------------------------------
template <class T, class conT>
void EnforceNonPenetrationConstraint(const T&    entity, 
									 const conT& ContainerOfEntities)
{
	//iterate through all entities checking for any overlap of bounding radii
	for (typename conT::const_iterator curEntity =ContainerOfEntities.begin();
		curEntity != ContainerOfEntities.end();
		++curEntity)
	{
		//make sure we don't check against the individual
		if (*curEntity == entity) continue;

		//calculate the distance between the positions of the entities
		Vector2D ToEntity = entity->Pos() - (*curEntity)->Pos();

		double DistFromEachOther = ToEntity.Length();

		//if this distance is smaller than the sum of their radii then this
		//entity must be moved away in the direction parallel to the
		//ToEntity vector   
		double AmountOfOverLap = (*curEntity)->BRadius() + entity->BRadius() -
			DistFromEachOther;

		if (AmountOfOverLap >= 0)
		{
			//move the entity a distance away equivalent to the amount of overlap.
			entity->SetPos(entity->Pos() + (ToEntity/DistFromEachOther) *
				AmountOfOverLap);
		}
	}//next entity
}




//-------------------- GetEntityLineSegmentIntersections ----------------------
//
//  tests a line segment AB against a container of entities. First of all
//  a test is made to confirm that the entity is within a specified range of 
//  the one_to_ignore (positioned at A). If within range the intersection test
//  is made.
//
//  returns a list of all the entities that tested positive for intersection
//-----------------------------------------------------------------------------
template <class T, class conT>
std::list<T> GetEntityLineSegmentIntersections(const conT& entities,
											   int         the_one_to_ignore,
											   Vector2D    A,
											   Vector2D    B,
											   double       range = MaxDouble)
{
	typename conT::const_iterator it;

	std::list<T> hits;

	//iterate through all entities checking against the line segment AB
	for (it=entities.begin(); it != entities.end(); ++it)
	{
		//if not within range or the entity being checked is the_one_to_ignore
		//just continue with the next entity
		if ( ((*it)->ID() == the_one_to_ignore) ||
			(Vec2DDistanceSq((*it)->Pos(), A) > range*range) )
		{
			continue;
		}

		//if the distance to AB is less than the entities bounding radius then
		//there is an intersection so add it to hits
		if (DistToLineSegment(A, B, (*it)->Pos()) < (*it)->BRadius())
		{
			hits.push_back(*it);
		}

	}

	return hits;
}

//------------------------ GetClosestEntityLineSegmentIntersection ------------
//
//  tests a line segment AB against a container of entities. First of all
//  a test is made to confirm that the entity is within a specified range of 
//  the one_to_ignore (positioned at A). If within range the intersection test
//  is made.
//
//  returns the closest entity that tested positive for intersection or NULL
//  if none found
//-----------------------------------------------------------------------------

template <class T, class conT>
T* GetClosestEntityLineSegmentIntersection(const conT&	entities,
										   int			the_one_to_ignore,
										   Vector2D		A,
										   Vector2D		B,
										   double		range = MaxDouble)
{
	typename conT::const_iterator it;

	T* ClosestEntity = NULL;

	double ClosestDist = MaxDouble;

	//iterate through all entities checking against the line segment AB
	for (it=entities.begin(); it != entities.end(); ++it)
	{
		double distSq = Vec2DDistanceSq((*it)->Pos(), A);

		//if not within range or the entity being checked is the_one_to_ignore
		//just continue with the next entity
		if ( ((*it)->ID() == the_one_to_ignore) || (distSq > range*range) )
		{
			continue;
		}

		//if the distance to AB is less than the entities bounding radius then
		//there is an intersection so add it to hits
		if (DistToLineSegment(A, B, (*it)->Pos()) < (*it)->BRadius())
		{
			if (distSq < ClosestDist)
			{
				ClosestDist = distSq;

				ClosestEntity = *it;
			}
		}

	}

	return ClosestEntity;
}


#endif // __H_OGRERECAST_ENTITY_FUNCTION_TEMPLATES_H_
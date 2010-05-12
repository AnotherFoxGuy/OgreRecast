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

#include "BaseOgreRecastEntity.h"


int BaseGameEntity::m_iNextValidID = 0;

//------------------------------ ctor -----------------------------------------
//-----------------------------------------------------------------------------
BaseGameEntity::BaseGameEntity():
m_ID(GetNextValidID()),
m_dBoundingRadius(0.0),
m_vPos(Vector2D()),
m_vScale(Vector2D(1.0,1.0)),
m_EntityType(default_entity_type),
m_bTag(false)
{}


BaseGameEntity::BaseGameEntity(int ID):
m_dBoundingRadius(0.0),
m_vScale(Vector2D(1.0,1.0)),
m_EntityType(default_entity_type),
m_bTag(false)
{
	SetID(ID);
}

BaseGameEntity::BaseGameEntity(int entity_type, Vector2D pos):
m_ID(GetNextValidID()),
m_dBoundingRadius(0.0),
m_vPos(pos),
m_vScale(Vector2D(1.0,1.0)),
m_EntityType(entity_type),
m_bTag(false)
{}

BaseGameEntity::BaseGameEntity(int entity_type, Vector2D pos, double r):
m_vPos(pos),
m_dBoundingRadius(r),
m_ID(GetNextValidID()),
m_vScale(Vector2D(1.0,1.0)),
m_EntityType(entity_type),
m_bTag(false)
{}

//this can be used to create an entity with a 'forced' ID. It can be used
//when a previously created entity has been removed and deleted from the
//game for some reason. For example, The Raven map editor uses this ctor 
//in its undo/redo operations. 
//USE WITH CAUTION!
BaseGameEntity::BaseGameEntity(int entity_type, int ForcedID):
m_ID(ForcedID),
m_dBoundingRadius(0.0),
m_vPos(Vector2D()),
m_vScale(Vector2D(1.0,1.0)),
m_EntityType(entity_type),
m_bTag(false)
{}

//----------------------------- SetID -----------------------------------------
//
//  this must be called within each constructor to make sure the ID is set
//  correctly. It verifies that the value passed to the method is greater
//  or equal to the next valid ID, before setting the ID and incrementing
//  the next valid ID
//-----------------------------------------------------------------------------
void BaseGameEntity::SetID(int val)
{
	//make sure the val is equal to or greater than the next available ID
	assert ( (val >= m_iNextValidID) && "<BaseGameEntity::SetID>: invalid ID");

	m_ID = val;

	m_iNextValidID = m_ID + 1;
}

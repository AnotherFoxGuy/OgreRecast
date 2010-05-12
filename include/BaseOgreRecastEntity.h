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

#ifndef BASE_GAME_ENTITY_H
#define BASE_GAME_ENTITY_H

#include <vector>
#include <string>
#include "Vector2D.h"
#include "Geometry.h"
#include "MiscUtils.h"

struct Telegram;


class BaseGameEntity
{
public:

	enum {default_entity_type = -1};

private:

	//each entity has a unique ID
	int         m_ID;

	//every entity has a type associated with it (health, troll, ammo etc)
	int         m_EntityType;

	//this is a generic flag. 
	bool        m_bTag;

	//used by the constructor to give each entity a unique ID
	int NextValidID(){ return m_iNextValidID++;}

	//this is the next valid ID. Each time a BaseGameEntity is instantiated
	//this value is updated
	static int  m_iNextValidID;

	//this must be called within each constructor to make sure the ID is set
	//correctly. It verifies that the value passed to the method is greater
	//or equal to the next valid ID, before setting the ID and incrementing
	//the next valid ID
	void SetID(int val);


protected:

	//its location in the environment
	Vector2D m_vPos;

	Vector2D m_vScale;

	//the length of this object's bounding radius
	double    m_dBoundingRadius;


	BaseGameEntity();	
	BaseGameEntity(int ID);
	BaseGameEntity(int entity_type, Vector2D pos);
	BaseGameEntity(int entity_type, Vector2D pos, double r);

	//this can be used to create an entity with a 'forced' ID. It can be used
	//when a previously created entity has been removed and deleted from the
	//game for some reason. For example, The Raven map editor uses this ctor 
	//in its undo/redo operations. 
	//USE WITH CAUTION!
	BaseGameEntity(int entity_type, int ForcedID);



public:

	virtual ~BaseGameEntity(){}

	virtual void Update(double time_elapsed){}; 

	virtual void Render(){};

	virtual bool HandleMessage(const Telegram& msg){return false;}

	//entities should be able to read/write their data to a stream
	virtual void Write(std::ostream&  os)const{}
	virtual void Read (std::ifstream& is){}

	//use this to grab the next valid ID
	static int   GetNextValidID(){return m_iNextValidID;}

	//this can be used to reset the next ID
	static void  ResetNextValidID(){m_iNextValidID = 0;}

	Vector2D     Pos()const{return m_vPos;}
	void         SetPos(Vector2D new_pos){m_vPos = new_pos;}

	double        BRadius()const{return m_dBoundingRadius;}
	void         SetBRadius(double r){m_dBoundingRadius = r;}
	int          ID()const{return m_ID;}

	bool         IsTagged()const{return m_bTag;}
	void         Tag(){m_bTag = true;}
	void         UnTag(){m_bTag = false;}

	Vector2D     Scale()const{return m_vScale;}
	void         SetScale(Vector2D val){m_dBoundingRadius *= MaxOf(val.x, val.y)/MaxOf(m_vScale.x, m_vScale.y); m_vScale = val;}
	void         SetScale(double val){m_dBoundingRadius *= (val/MaxOf(m_vScale.x, m_vScale.y)); m_vScale = Vector2D(val, val);} 

	int          EntityType()const{return m_EntityType;}
	void         SetEntityType(int new_type){m_EntityType = new_type;}

};




#endif
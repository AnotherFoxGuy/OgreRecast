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

/* Copyright Steve Rabin, 2005. 
 * All rights reserved worldwide.
 *
 * This software is provided "as is" without express or implied
 * warranties. You may freely copy and compile this source into
 * applications you distribute provided that the copyright text
 * below is included in the resulting source code, for example:
 * "Portions Copyright Steve Rabin, 2005"
 */

#pragma once

#include <list>
#include "Ogre.h"
#include "SharedData.h"


//Add new object types here (bitfield mask - objects can be combinations of types)
#define OBJECT_Ignore_Type  (0)
#define OBJECT_Gameflow     (1<<1)
#define OBJECT_Character    (1<<2)
#define OBJECT_NPC          (1<<3)
#define OBJECT_Player       (1<<4)
#define OBJECT_Enemy        (1<<5)
#define OBJECT_Weapon       (1<<6)
#define OBJECT_Item         (1<<7)
#define OBJECT_Projectile   (1<<8)

#define GAME_OBJECT_MAX_NAME_SIZE 64

enum StateMachineChange {
	NO_STATE_MACHINE_CHANGE,
	STATE_MACHINE_RESET,
	STATE_MACHINE_REPLACE,
	STATE_MACHINE_QUEUE,
	STATE_MACHINE_REQUEUE,
	STATE_MACHINE_PUSH,
	STATE_MACHINE_POP
};

class StateMachine;

class GameObject
{
public:

	GameObject( objectID id, unsigned int type, char* name );
	~GameObject( void );

	objectID GetID( void )							{ return( m_id ); }
	unsigned int GetType( void )					{ return( m_type ); }
	char* GetName( void )							{ return( m_name ); }
	
	void Update( void );

	void MarkForDeletion( void )					{ m_markedForDeletion = true; }
	bool IsMarkedForDeletion( void )				{ return( m_markedForDeletion ); }
	
	//State Machine access
	StateMachine* GetStateMachine( void )			{ if( m_stateMachineList.empty() ) { return( 0 ); } else { return( m_stateMachineList.back() ); } }
	void RequestStateMachineChange( StateMachine * mch, StateMachineChange change );
	void ResetStateMachine( void );
	void ReplaceStateMachine( StateMachine & mch );
	void QueueStateMachine( StateMachine & mch );
	void RequeueStateMachine( void );
	void PushStateMachine( StateMachine & mch );
	void PopStateMachine( void );
	void DestroyStateMachineList( void );

	//Attribute access
	int GetHealth( void )							{ return( m_health ); }
	void SetHealth( int health )					{ if( health > 0 ) { m_health = health; } else { m_health = 0; } }
	bool IsAlive( void )							{ return( m_health > 0 ); }

	Ogre::Vector3 getObjectPosition(void)					{ return mObjectPosition; }
	void setObjectPosition(float x,float y,float z) { mObjectPosition.x = x; mObjectPosition.y = y; mObjectPosition.z = z; }
	void setObjectPosition(Ogre::Vector3 pos)		{ mObjectPosition = pos; }

private:

	typedef std::list<StateMachine*> stateMachineListContainer;
	
	objectID m_id;									//Unique id of object (safer than a pointer).
	unsigned int m_type;							//Type of object (can be combination).
	char m_name[GAME_OBJECT_MAX_NAME_SIZE];			//String name of object.

	bool m_markedForDeletion;						//Flag to delete this object (when it is safe to do so).

	stateMachineListContainer m_stateMachineList;	//List of state machines. Top one is active.

	StateMachineChange m_stateMachineChange;		//Directions for any pending state machine changes
	StateMachine * m_newStateMachine;				//A state machine that will be added to the queue later

	void ProcessStateMachineChangeRequests( void );


	//Player attributes (example used in unit tests, etc)
	int m_health;
	Ogre::Vector3 mObjectPosition;

};

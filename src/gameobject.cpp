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

#include "Ogre.h"
#include "gameobject.h"
#include "msgroute.h"
#include "statemch.h"


GameObject::GameObject( objectID id, unsigned int type, char* name )
: m_markedForDeletion(false),
  m_stateMachineChange(NO_STATE_MACHINE_CHANGE),
  m_newStateMachine(0),
  m_health(50),
  mObjectPosition(Ogre::Vector3::ZERO)
{
	m_id = id;
	m_type = type;
	
	if( strlen(name) < GAME_OBJECT_MAX_NAME_SIZE ) {
		strcpy( m_name, name );
	}
	else {
		strcpy( m_name, "invalid_name" );
		ASSERTMSG(0, "GameObject::GameObject - name is too long" );
	}
}

GameObject::~GameObject( void )
{
	DestroyStateMachineList();
}

/*---------------------------------------------------------------------------*
  Name:         Update

  Description:  Calls the update function of the currect state machine.

  Arguments:    None.

  Returns:      None.
 *---------------------------------------------------------------------------*/
void GameObject::Update( void )
{
	if( !m_stateMachineList.empty() ) {
		ProcessStateMachineChangeRequests();
		m_stateMachineList.back()->Update();
	}
}

/*---------------------------------------------------------------------------*
  Name:         ProcessStateMachineChangeRequests

  Description:  Checks if a state machine should be changed. It will loop
                until no more state machine change requests have been made.

  Arguments:    None.

  Returns:      None.
 *---------------------------------------------------------------------------*/
void GameObject::ProcessStateMachineChangeRequests( void )
{
	int safetyCount = 20;
	StateMachineChange change = m_stateMachineChange;
	StateMachine * tempStateMachine = m_newStateMachine;
	
	while( change != NO_STATE_MACHINE_CHANGE && (--safetyCount >= 0) )
	{
		ASSERTMSG( safetyCount > 0, "GameObject::ProcessStateMachineChangeRequests - State machiens are flip-flopping in an infinite loop." );

		m_stateMachineChange = NO_STATE_MACHINE_CHANGE;			//Reset
		m_newStateMachine = 0;									//Reset

		switch( change )
		{
			case STATE_MACHINE_RESET:
				g_msgroute.PurgeScopedMsg( m_id ); //Remove all delayed messages addressed to me that are scoped
				ResetStateMachine();
				break;
				
			case STATE_MACHINE_REPLACE:
				g_msgroute.PurgeScopedMsg( m_id ); //Remove all delayed messages addressed to me that are scoped
				ReplaceStateMachine( *tempStateMachine );
				break;
				
			case STATE_MACHINE_QUEUE:
				QueueStateMachine( *tempStateMachine );
				break;
				
			case STATE_MACHINE_REQUEUE:
				g_msgroute.PurgeScopedMsg( m_id ); //Remove all delayed messages addressed to me that are scoped
				RequeueStateMachine();
				break;
				
			case STATE_MACHINE_PUSH:
				g_msgroute.PurgeScopedMsg( m_id ); //Remove all delayed messages addressed to me that are scoped
				PushStateMachine( *tempStateMachine );
				break;
				
			case STATE_MACHINE_POP:
				g_msgroute.PurgeScopedMsg( m_id ); //Remove all delayed messages addressed to me that are scoped
				PopStateMachine();
				break;
				
			default:
				ASSERTMSG( 0, "GameObject::ProcessStateMachineChangeRequests - invalid StateMachineChange request." );
		}
				
		//Check if new change
		change = m_stateMachineChange;
		tempStateMachine = m_newStateMachine;
	}
}

/*---------------------------------------------------------------------------*
  Name:         RequestStateMachineChange

  Description:  Requests that a state machine change take place.

  Arguments:    mch    : the new state machine
                change : the change to take place

  Returns:      None.
 *---------------------------------------------------------------------------*/
void GameObject::RequestStateMachineChange( StateMachine * mch, StateMachineChange change )
{
	ASSERTMSG( m_stateMachineChange == NO_STATE_MACHINE_CHANGE, "GameObject::RequestStateMachineChange - Change already requested." );
	m_newStateMachine = mch;
	m_stateMachineChange = change;
}

/*---------------------------------------------------------------------------*
  Name:         ResetStateMachine

  Description:  Resets the current state machine.

  Arguments:    None.

  Returns:      None.
 *---------------------------------------------------------------------------*/
void GameObject::ResetStateMachine( void )
{
	ASSERTMSG( m_stateMachineList.size() > 0, "GameObject::ResetStateMachine - No existing state machine to reset." );
	if( m_stateMachineList.size() > 0 ) {
		StateMachine * mch = m_stateMachineList.back();
		mch->Reset();
	}
}

/*---------------------------------------------------------------------------*
  Name:         ReplaceStateMachine

  Description:  Replaces the current state machine with the provided one
                by popping off the current state machine and pushing the 
				new one.

  Arguments:    mch : the new state machine

  Returns:      None.
 *---------------------------------------------------------------------------*/
void GameObject::ReplaceStateMachine( StateMachine & mch )
{
	ASSERTMSG( m_stateMachineList.size() > 0, "GameObject::ReplaceStateMachine - No existing state machine to replace." );
	if( m_stateMachineList.size() > 0 ) {
		StateMachine * temp = m_stateMachineList.back();
		m_stateMachineList.pop_back();
		delete( temp );
	}
	PushStateMachine( mch );
}

/*---------------------------------------------------------------------------*
  Name:         QueueStateMachine

  Description:  Queues a state machine behind all others, except for the
                very last one. The last state machine is the "default" and
				should always remain the last.

  Arguments:    mch : the new state machine

  Returns:      None.
 *---------------------------------------------------------------------------*/
void GameObject::QueueStateMachine( StateMachine & mch )
{
	//Insert state machine one up from bottom
	if( m_stateMachineList.size() <= 1 ) {
		PushStateMachine( mch );
	}
	else {
		stateMachineListContainer::iterator i;
		i = m_stateMachineList.begin();
		i++;	//Move iterator past the first entry
		m_stateMachineList.insert(i, &mch);
		//Purposely do not "Reset" state machine until it is active
	}
}

/*---------------------------------------------------------------------------*
  Name:         RequeueStateMachine

  Description:  Requeues the current state machine behind all others, except
                for the very last one.

  Arguments:    None.

  Returns:      None.
 *---------------------------------------------------------------------------*/
void GameObject::RequeueStateMachine( void )
{
	ASSERTMSG( m_stateMachineList.size() > 0, "GameObject::RequeueStateMachine - No existing state machines to requeue." );
	if( m_stateMachineList.size() > 1 ) {
		StateMachine * mch = m_stateMachineList.back();
		QueueStateMachine( *mch );
		m_stateMachineList.pop_back();

		//Initialize new state machine
		mch = m_stateMachineList.back();
		mch->Reset();
	}
	else if( m_stateMachineList.size() == 1 ) {
		//Just reinitialize
		StateMachine * mch = m_stateMachineList.back();
		mch->Reset();	
	}
}

/*---------------------------------------------------------------------------*
  Name:         PushStateMachine

  Description:  Pushes a state machine onto the front of the state machine list.

  Arguments:    mch : the new state machine

  Returns:      None.
 *---------------------------------------------------------------------------*/
void GameObject::PushStateMachine( StateMachine & mch )
{
	m_stateMachineList.push_back( &mch );
	mch.Reset();
}

/*---------------------------------------------------------------------------*
  Name:         PopStateMachine

  Description:  Pops a state machine from the front of the state machine list.

  Arguments:    None.

  Returns:      None.
 *---------------------------------------------------------------------------*/
void GameObject::PopStateMachine( void )
{
	ASSERTMSG( m_stateMachineList.size() > 1, "GameObject::PopStateMachine - Can't pop last state machine." );
	if( m_stateMachineList.size() > 1 ) {
		StateMachine * mch = m_stateMachineList.back();
		m_stateMachineList.pop_back();
		delete( mch );
		
		//Initialize new state machine
		mch = m_stateMachineList.back();
		mch->Reset();
	}
}

/*---------------------------------------------------------------------------*
  Name:         DestroyStateMachineList

  Description:  Destroys all state machines in the state machine list.

  Arguments:    None.

  Returns:      None.
 *---------------------------------------------------------------------------*/
void GameObject::DestroyStateMachineList( void )
{
	while( m_stateMachineList.size() > 0 ) {
		StateMachine * mch = m_stateMachineList.back();
		m_stateMachineList.pop_back();
		delete( mch );
	}
}




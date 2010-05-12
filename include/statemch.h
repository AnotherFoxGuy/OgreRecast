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

#include <vector>
#include "gameobject.h"
#include "msg.h"
#include "debuglog.h"
#include "timesm.h"

#define MAX_STATE_NAME_SIZE 64
#define DEBUG_STATE_MACHINE_MACROS		//Comment out to get the release macros (no debug logging info)

//State Machine Language Macros (put the 9 keywords in the file USERTYPE.DAT in the same directory as MSDEV.EXE to get keyword highlighting)
#ifdef DEBUG_STATE_MACHINE_MACROS
	#define BeginStateMachine				StateName laststatedeclared; char eventbuffer[5]; if( state < 0 ) { const char statename[MAX_STATE_NAME_SIZE] = "STATE_Global"; const char substatename[MAX_STATE_NAME_SIZE] = ""; if( EVENT_Message == event && msg && MSG_CHANGE_STATE_DELAYED == msg->GetName() ) { ChangeState( static_cast<unsigned int>( msg->GetIntData() ) ); return( true ); } if( EVENT_Message == event && msg && MSG_CHANGE_SUBSTATE_DELAYED == msg->GetName() ) { ChangeSubstate( static_cast<unsigned int>( msg->GetIntData() ) );
	#define EndStateMachine					return( true ); } g_debuglog.LogStateMachineEvent( m_owner->GetID(), m_owner->GetName(), msg, statename, substatename, itoa(event, eventbuffer, 10), false ); return( false ); } ASSERTMSG( 0, "Invalid State" ); return( false );
	#define DeclareState(name)				return( true ); } g_debuglog.LogStateMachineEvent( m_owner->GetID(), m_owner->GetName(), msg, statename, substatename, itoa(event, eventbuffer, 10), false ); return( false ); } laststatedeclared = name; if( name == state && substate < 0 ) { const char statename[MAX_STATE_NAME_SIZE] = #name; const char substatename[MAX_STATE_NAME_SIZE] = ""; if( EVENT_Enter == event ) { SetCurrentStateName( #name ); } if(0) { 
	#define DeclareSubstate(name)			return( true ); } return( false ); } else if( laststatedeclared == state && name == substate ) { const char statename[MAX_STATE_NAME_SIZE] = ""; const char substatename[MAX_STATE_NAME_SIZE] = #name; if( EVENT_Enter == event ) { SetCurrentSubstateName( #name ); } SubstateName verifysubstatename = name; if(0) { 
	#define OnMsg(msgname)					return( true ); } else if( EVENT_Message == event && msg && msgname == msg->GetName() ) { VerifyMessageEnum( msgname ); g_debuglog.LogStateMachineEvent( m_owner->GetID(), m_owner->GetName(), msg, statename, substatename, #msgname, true );
	#define OnCCMsg(msgname)				return( true ); } else if( EVENT_CCMessage == event && msg && msgname == msg->GetName() ) { g_debuglog.LogStateMachineEvent( m_owner->GetID(), m_owner->GetName(), msg, statename, substatename, #msgname, true );
	#define ONEVENT_INTERNAL_HELPER(a)		return( true ); } else if( a == event ) { g_debuglog.LogStateMachineEvent( m_owner->GetID(), m_owner->GetName(), msg, statename, substatename, #a, true );
	#define OnUpdate						ONEVENT_INTERNAL_HELPER( EVENT_Update )
	#define OnEnter							ONEVENT_INTERNAL_HELPER( EVENT_Enter )
	#define OnExit							ONEVENT_INTERNAL_HELPER( EVENT_Exit )
#else
	#define BeginStateMachine				StateName laststatedeclared; if( state < 0 ) { if( EVENT_Message == event && msg && MSG_CHANGE_STATE_DELAYED == msg->GetName() ) { ChangeState( static_cast<unsigned int>( msg->GetIntData() ) ); return( true ); } if( EVENT_Message == event && msg && MSG_CHANGE_SUBSTATE_DELAYED == msg->GetName() ) { ChangeSubstate( static_cast<unsigned int>( msg->GetIntData() ) );
	#define EndStateMachine					return( true ); } return( false ); } ASSERTMSG( 0, "Invalid State" ); return( false );
	#define DeclareState(name)				return( true ); } return( false ); } laststatedeclared = name; if( name == state && substate < 0 ) { if(0) { 
	#define DeclareSubstate(name)			return( true ); } return( false ); } else if( laststatedeclared == state && name == substate ) { if(0) { 
	#define OnMsg(msgname)					return( true ); } else if( EVENT_Message == event && msg && msgname == msg->GetName() ) {
	#define OnCCMsg(msgname)				return( true ); } else if( EVENT_CCMessage == event && msg && msgname == msg->GetName() ) {
	#define ONEVENT_INTERNAL_HELPER(a)		return( true ); } else if( a == event ) {
	#define OnUpdate						ONEVENT_INTERNAL_HELPER( EVENT_Update )
	#define OnEnter							ONEVENT_INTERNAL_HELPER( EVENT_Enter )
	#define OnExit							ONEVENT_INTERNAL_HELPER( EVENT_Exit )
#endif



enum State_Machine_Event {
	EVENT_INVALID,
	EVENT_Update,
	EVENT_Message,
	EVENT_CCMessage,
	EVENT_Enter,
	EVENT_Exit
};


class StateMachine
{
public:

	StateMachine( GameObject & object );
	virtual ~StateMachine( void ) {}

	//Should only be called by GameObject
	void Update( void );
	void Reset( void );
	
	//Message Timers
	void SetTimer( float delay, MSG_Name name, Scope_Rule scope = SCOPE_TO_SUBSTATE );
	void StopTimer( MSG_Name name );

	//Access state and scope
	int GetState( void )						{ return( (int)m_currentState ); }
	unsigned int GetScopeState( void )			{ return( m_scopeState ); }
	unsigned int GetScopeSubstate( void )		{ return( m_scopeSubstate ); }
	
	//Main state machine code stored in here
	void Process( State_Machine_Event event, MSG_Object * msg );

	//Debug info
	char * GetCurrentStateNameString( void )	{ return( m_currentStateNameString ); }
	char * GetCurrentSubstateNameString( void )	{ return( m_currentSubstateNameString ); }


protected:

	GameObject * m_owner;						//GameObject that owns this state machine

	//Send Messages
	void SendMsg( MSG_Name name, objectID receiver, int data = 0 );
	void SendMsgToMe( MSG_Name name, Scope_Rule scope = SCOPE_TO_SUBSTATE, int data = 0 );
	void SendMsgDelayed( float delay, MSG_Name name, objectID receiver, int data = 0 );
	void SendMsgDelayedToMe( float delay, MSG_Name name, Scope_Rule scope = SCOPE_TO_SUBSTATE, int data = 0 );
	void SendMsgBroadcast( MSG_Name name, unsigned int type = OBJECT_Ignore_Type, int data = 0 );
	void SendMsgBroadcastToList( MSG_Name name, int data = 0 );

	//Broadcast List
	void BroadcastClearList( void );
	void BroadcastAddToList( objectID id );

	//CCing other objects
	void SetCCReceiver( objectID id )			{ m_ccMessagesToGameObject = id; }
	void ClearCCReceiver( void )				{ m_ccMessagesToGameObject = 0; }
	objectID GetCCReceiver( void )				{ return( m_ccMessagesToGameObject ); }

	//Controlling update events
	void EnableUpdateEvents( void )				{ m_updateEventEnabled = true; }
	void DisableUpdateEvents( void )			{ m_updateEventEnabled = false; }
	
	//Change State
	void ChangeState( unsigned int newState );
	void ChangeStateDelayed( float delay, unsigned int newState, Scope_Rule scope = SCOPE_TO_SUBSTATE );
	void PopState( void );
	void ChangeSubstate( unsigned int newSubstate );
	void ChangeSubstateDelayed( float delay, unsigned int newSubstate, Scope_Rule scope = SCOPE_TO_SUBSTATE );
	
	//Switch to another State Machine
	void ResetStateMachine( void );
	void ReplaceStateMachine( StateMachine & mch );
	void QueueStateMachine( StateMachine & mch );
	void RequeueStateMachine( void );
	void PushStateMachine( StateMachine & mch );
	void PopStateMachine( void );

	//Random generation for times
	float RandDelay( float min, float max );

	//Destroy game object
	void MarkForDeletion( void )				{ m_owner->MarkForDeletion(); }

	//Helper functions
	float GetTimeInState( void )				{ return( g_time.GetCurTime() - m_timeOnEnter ); }				

	//Used to verify proper message enums
	inline void VerifyMessageEnum( MSG_Name name ) {}

	//Used for debug to capture current state/substate name string
	void SetCurrentStateName( char * state )			{ strcpy( m_currentStateNameString, state ); m_currentSubstateNameString[0] = 0; }
	void SetCurrentSubstateName( char * substate )		{ strcpy( m_currentSubstateNameString, substate ); }


private:

	typedef std::vector<objectID> BroadcastListContainer;	//Container to hold game objects to broadcast to
	typedef std::list<unsigned int> StateListContainer;		//Container to hold past states

	enum State_Change {							//Possible state change requests
		NO_STATE_CHANGE,						//No change pending
		STATE_CHANGE,							//State change pending
		STATE_POP								//State pop pending
	};

	unsigned int m_scopeState;					//The current scope of the state
	unsigned int m_scopeSubstate;				//The current scope of the substate
	unsigned int m_currentState;				//Current state
	unsigned int m_nextState;					//Next state to switch to
	int m_currentSubstate;						//Current substate (-1 indicates not valid)
	int m_nextSubstate;							//Next substate (-1 indicates not valid)
	bool m_stateChangeAllowed;					//If a state change is allowed
	State_Change m_stateChange;					//If a state change is pending
	float m_timeOnEnter;						//Time since state was entered
	bool m_updateEventEnabled;					//Whether update events are sent to the state machine
	objectID m_ccMessagesToGameObject;			//A GameObject to CC messages to
	BroadcastListContainer m_broadcastList;		//List of GameObjects to broadcast to
	StateListContainer m_stack;					//Stack of past states (used for PopState)


	//Debug info
	char m_currentStateNameString[MAX_STATE_NAME_SIZE];		//Current state name string
	char m_currentSubstateNameString[MAX_STATE_NAME_SIZE];	//Current substate name string

	void Initialize( void );
	virtual bool States( State_Machine_Event event, MSG_Object * msg, int state, int substate ) = 0;
	void PerformStateChanges( void );
	void SendCCMsg( MSG_Name name, objectID receiver, MSG_Data data );
	void SendMsgDelayedToMeHelper( float delay, MSG_Name name, Scope_Rule scope, MSG_Data data, bool timer );

};

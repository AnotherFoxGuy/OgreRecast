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

#include "database.h"
#include "gameobject.h"


Database::Database( void )
: m_nextFreeID(1)
{

}

/*---------------------------------------------------------------------------*
  Name:         Update

  Description:  Calls the update function for all objects within the database.

  Arguments:    None.

  Returns:      None.
 *---------------------------------------------------------------------------*/
void Database::Update( void )
{
	dbContainer::iterator i = m_database.begin();
	while( i != m_database.end() )
	{
		(*i)->Update();
		
		if( (*i)->IsMarkedForDeletion() )
		{	//Destroy object
			delete( *i );
			i = m_database.erase( i );
		}
		else
		{
			++i;
		}
	}
}

/*---------------------------------------------------------------------------*
  Name:         Store

  Description:  Stores an object within the database.

  Arguments:    object : the game object

  Returns:      None.
 *---------------------------------------------------------------------------*/
void Database::Store( GameObject & object )
{
	if( Find( object.GetID() ) == 0 ) {
		m_database.push_back( &object );
	}
	else {
		ASSERTMSG( 0, "Database::Store - Object ID already represented in database." );
	}
}

/*---------------------------------------------------------------------------*
  Name:         Remove

  Description:  Removes an object from the database.

  Arguments:    id : the ID of the object

  Returns:      None.
 *---------------------------------------------------------------------------*/
void Database::Remove( objectID id )
{
	dbContainer::iterator i;
	for( i=m_database.begin(); i!=m_database.end(); ++i )
	{
		if( (*i)->GetID() == id ) {
			m_database.erase(i);	
			return;
		}
	}

	return;
}

/*---------------------------------------------------------------------------*
  Name:         Find

  Description:  Find an object given its id.

  Arguments:    id : the ID of the object

  Returns:      An object pointer. If object is not found, returns 0.
 *---------------------------------------------------------------------------*/
GameObject* Database::Find( objectID id )
{
	dbContainer::iterator i;
	for( i=m_database.begin(); i!=m_database.end(); ++i )
	{
		if( (*i)->GetID() == id ) {
			return( *i );
		}
	}

	return( 0 );

}

/*---------------------------------------------------------------------------*
  Name:         GetIDByName

  Description:  Get an object's id given its name.

  Arguments:    name : the name of the object

  Returns:      An ID. If object is not found, returns INVALID_OBJECT_ID.
 *---------------------------------------------------------------------------*/
objectID Database::GetIDByName( char* name )
{
	dbContainer::iterator i;
	for( i=m_database.begin(); i!=m_database.end(); ++i )
	{
		if( strcmp( (*i)->GetName(), name ) == 0 ) {
			return( (*i)->GetID() );
		}
	}

	return( INVALID_OBJECT_ID );
}

/*---------------------------------------------------------------------------*
  Name:         GetNewObjectID

  Description:  Get a fresh object ID.

  Arguments:    None.

  Returns:      An id.
 *---------------------------------------------------------------------------*/
objectID Database::GetNewObjectID( void )
{
	return( m_nextFreeID++ );

}

/*---------------------------------------------------------------------------*
  Name:         ComposeList

  Description:  Compose a list of objects given certain type.

  Arguments:    list   : the list to fill with the result of the operation
                type   : the type of object to add to the list (optional)

  Returns:      None. (The result is stored in the list argument.)
 *---------------------------------------------------------------------------*/
void Database::ComposeList( dbCompositionList & list, unsigned int type )
{
	//Find all objects of "type"
	dbContainer::iterator i;
	for( i=m_database.begin(); i!=m_database.end(); ++i )
	{
		if( type == OBJECT_Ignore_Type || (*i)->GetType() & type )
		{	//Type matches
			list.push_back(*i);
		}
	}
}


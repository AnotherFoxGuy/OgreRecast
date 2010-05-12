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

#pragma once

#include <assert.h>

namespace STM
{
//Singleton class as authored by Scott Bilas in the book Game Programming Gems

template <typename T>
class Singleton
{
public:
	Singleton( void )
	{
		assert( ms_Singleton == 0 && "Singleton constructor" );
		int offset = (int)(T*)1 - (int)(Singleton <T> *)(T*)1;
		ms_Singleton = (T*)((int)this + offset);
	}
	~Singleton( void )  {  assert( ms_Singleton != 0 && "Singleton destructor" );  ms_Singleton = 0;  }

	static T&   GetSingleton      ( void )  {  assert( ms_Singleton != 0 && "Singleton - GetSingleton" );  return ( *ms_Singleton );  }
	static T*   GetSingletonPtr   ( void )  {  return ( ms_Singleton );  }
	static bool DoesSingletonExist( void )  {  return ( ms_Singleton != 0 );  }

private:
	static T* ms_Singleton;

};

};

template <typename T> T* STM::Singleton <T>::ms_Singleton = 0;


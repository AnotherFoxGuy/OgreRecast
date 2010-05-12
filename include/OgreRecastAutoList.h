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

#ifndef __H_AUTOLIST_H_
#define __H_AUTOLIST_H_

//------------------------------------------------------------------------
//
//Name:   Autolist.h
//
//Desc:   Inherit from this class to automatically create lists of
//        similar objects. Whenever an object is created it will
//        automatically be added to the list. Whenever it is destroyed
//        it will automatically be removed.
//
//Author: Mat Buckland (fup@ai-junkie.com)
//
//------------------------------------------------------------------------
#include <list>


template <class T>
class AutoList
{
public:

	typedef std::list<T*> ObjectList;

private:

	static ObjectList m_Members;

protected:

	AutoList()
	{
		//cast this object to type T* and add it to the list
		m_Members.push_back(static_cast<T*>(this));
	}

	~AutoList()
	{
		m_Members.remove(static_cast<T*>(this));    
	}

public:


	static ObjectList& GetAllMembers(){return m_Members;}
};


template <class T>
std::list<T*> AutoList<T>::m_Members;



#endif // __H_AUTOLIST_H_
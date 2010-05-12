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

#ifndef __H_OGRERECASTOBSTACLE_H_
#define __H_OGRERECASTOBSTACLE_H_

#include "OgreTemplate.h"
#include "Vector2D.h"
#include "BaseOgreRecastEntity.h"
#include "MiscUtils.h"


#include <windows.h>


class Obstacle : public BaseGameEntity
{
	DebugDrawGL* dd;

public:

	Obstacle(double x,
		double y,
		double r):BaseGameEntity(0, Vector2D(x,y), r)
	{
		dd = new DebugDrawGL();
		dd->setMaterialScript(Ogre::String("EntityLines"));
	}

	Obstacle(Vector2D pos, double radius):BaseGameEntity(0, pos, radius)
	{
		dd = new DebugDrawGL();
		dd->setMaterialScript(Ogre::String("EntityLines"));
	}

	Obstacle(std::ifstream& in)
	{
		Read(in);
		dd = new DebugDrawGL();
		dd->setMaterialScript(Ogre::String("EntityLines"));
	}

	virtual ~Obstacle()
	{
		if(dd)
		{
			delete dd;
			dd = NULL;
		}
	}

	//this is defined as a pure virtual function in BasegameEntity so
	//it must be implemented
	void      Update(double time_elapsed){}

	void      Render();

	void      Write(std::ostream& os)const;
	void      Read(std::ifstream& in);
};



#endif // __H_OGRERECASTOBSTACLE_H_
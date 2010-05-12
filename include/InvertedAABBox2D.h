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

#ifndef __H_INVAABBOX2D_H_
#define __H_INVAABBOX2D_H_

#include "Vector2D.h"
#include "OgreTemplate.h"
#include "DebugDraw.h"

class InvertedAABBox2D
{
private:

	Vector2D  m_vTopLeft;
	Vector2D  m_vBottomRight;

	Vector2D  m_vCenter;
	DebugDrawGL* dd;

public:

	InvertedAABBox2D(Vector2D tl, Vector2D br):
		m_vTopLeft(tl),
		m_vBottomRight(br),
		m_vCenter((tl+br)/2.0)
	{
		dd = new DebugDrawGL();
		dd->setMaterialScript(Ogre::String("EntityLines"));
	}

	~InvertedAABBox2D()
	{
		if(dd)
		{
			delete dd;
			dd = NULL;
		}
	}

	//returns true if the bbox described by other intersects with this one
	bool isOverlappedWith(const InvertedAABBox2D& other)const
	{
		return !((other.Top() > this->Bottom()) ||
			(other.Bottom() < this->Top()) ||
			(other.Left() > this->Right()) ||
			(other.Right() < this->Left()));
	}


	Vector2D TopLeft()const{return m_vTopLeft;}
	Vector2D BottomRight()const{return m_vBottomRight;}

	double    Top()const{return m_vTopLeft.y;}
	double    Left()const{return m_vTopLeft.x;}
	double    Bottom()const{return m_vBottomRight.y;}
	double    Right()const{return m_vBottomRight.x;}
	Vector2D Center()const{return m_vCenter;}

	void     Render(bool RenderCenter = false)const
	{
		dd->clear();

		dd->begin(DU_DRAW_LINES_STRIP, 10.0f);
		duAppendBoxWire(dd, (float)Top(), (float)m_vTopLeft.yUP, (float)Left(), (float)Bottom(), (float)m_vBottomRight.yUP, (float)Right(), (unsigned int)0);

		if (RenderCenter)
		{
			duAppendCircle(dd, m_vCenter.x, m_vCenter.yUP, m_vCenter.y , 5, (unsigned int)0);
		}
		dd->end();
	}

};

#endif // __H_INVAABBOX2D_H_
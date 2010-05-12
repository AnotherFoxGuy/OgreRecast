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

#ifndef __H_WALLOBSTACLE_H_
#define __H_WALLOBSTACLE_H_

#include "Vector2D.h"
#include "OgreTemplate.h"
#include "DebugDraw.h"
#include <fstream>


class Wall2D 
{
protected:

	DebugDrawGL* dd;

	Vector2D    m_vA,
				m_vB,
				m_vN;

	void CalculateNormal()
	{
		Vector2D temp = Vec2DNormalize(m_vB - m_vA);

		m_vN.x = -temp.y;
		m_vN.y = temp.x;
		m_vN.yUP = 1.0;
	}

public:

	Wall2D()
	{
		dd = new DebugDrawGL();
		dd->setMaterialScript(Ogre::String("NavMeshTestLines"));
		dd->setOffset(0.35f);
	}

	Wall2D(Vector2D A, Vector2D B):m_vA(A), m_vB(B)
	{
		CalculateNormal();
		dd = new DebugDrawGL();
		dd->setMaterialScript(Ogre::String("NavMeshTestLines"));
		dd->setOffset(0.35f);
	}

	Wall2D(Vector2D A, Vector2D B, Vector2D N):m_vA(A), m_vB(B), m_vN(N)
	{
		dd = new DebugDrawGL();
		dd->setMaterialScript(Ogre::String("NavMeshTestLines"));
		dd->setOffset(0.35f);
	}

	Wall2D(std::ifstream& in)
	{
		Read(in);
		dd = new DebugDrawGL();
		dd->setMaterialScript(Ogre::String("NavMeshTestLines"));
		dd->setOffset(0.35f);
	}

	~Wall2D()
	{
		if(dd)
		{
			delete dd;
			dd = NULL;
		}
	}

	virtual void Render(bool RenderNormals = false)const
	{
		dd->clear();

		dd->begin(DU_DRAW_LINES_STRIP, 10.0f);
		
		duAppendBoxWire(dd, m_vA.x, m_vA.yUP, m_vA.y, m_vB.x, m_vB.yUP, m_vB.y, (unsigned int)0);
		//render the normals if needed
		if (RenderNormals)
		{
			int MidX = (int)((m_vA.x+m_vB.x)/2);
			int MidY = (int)((m_vA.y+m_vB.y)/2);
			int MidYUP = (int)((m_vA.yUP+m_vB.yUP)/2);
			duAppendBoxWire(dd, MidX, MidYUP, MidY, (int)(MidX+(m_vN.x * 5)), (int)(MidYUP+(m_vN.yUP * 5)), (int)(MidY+(m_vN.y * 5)), (unsigned int)0);
		}
		dd->end();
	}

	Vector2D From()const  {return m_vA;}
	void     SetFrom(Vector2D v){m_vA = v; CalculateNormal();}

	Vector2D To()const    {return m_vB;}
	void     SetTo(Vector2D v){m_vB = v; CalculateNormal();}

	Vector2D Normal()const{return m_vN;}
	void     SetNormal(Vector2D n){m_vN = n;}

	Vector2D Center()const{return (m_vA+m_vB)/2.0;}

	std::ostream& Wall2D::Write(std::ostream& os)const
	{
		os << std::endl;
		os << From() << ",";
		os << To() << ",";
		os << Normal();
		return os;
	}

	void Read(std::ifstream& in)
	{
		double x,y;

		in >> x >> y;
		SetFrom(Vector2D(x,y));

		in >> x >> y;
		SetTo(Vector2D(x,y));

		in >> x >> y;
		SetNormal(Vector2D(x,y));
	}

};

#endif // __H_WALLOBSTACLE_H_
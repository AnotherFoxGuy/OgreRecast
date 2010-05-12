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

#ifndef __H_MOVING_ENTITY_H_
#define __H_MOVING_ENTITY_H_
//------------------------------------------------------------------------
//
//  Name:   MovingEntity.h
//
//  Original Author: Mat Buckland 2003 (fup@ai-junkie.com)
//
//------------------------------------------------------------------------
#include <cassert>

#include "Vector2D.h"
#include "BaseOgreRecastEntity.h"



class MovingEntity : public BaseGameEntity
{
protected:

	Vector2D    m_vVelocity; 
	Vector2D    m_vHeading;
	Vector2D    m_vSide; 
	double       m_dMass;
	double       m_dMaxSpeed;
	double        m_dMaxForce;
	double       m_dMaxTurnRate;

public:


	MovingEntity(Vector2D position, double radius, Vector2D velocity, double max_speed,
		Vector2D heading, double mass, Vector2D scale,double turn_rate, double max_force):
			BaseGameEntity(0, position, radius), m_vHeading(heading), m_vVelocity(velocity),
							m_dMass(mass), m_vSide(m_vHeading.Perp()), m_dMaxSpeed(max_speed),
							m_dMaxTurnRate(turn_rate), m_dMaxForce(max_force)
			{
				m_vScale = scale;
			}

	virtual ~MovingEntity(){}

	//accessors
	Vector2D  Velocity()const{return m_vVelocity;}
	void      SetVelocity(const Vector2D& NewVel){m_vVelocity = NewVel;}

	double     Mass()const{return m_dMass;}

	Vector2D  Side()const{return m_vSide;}

	double     MaxSpeed()const{return m_dMaxSpeed;}                       
	void      SetMaxSpeed(double new_speed){m_dMaxSpeed = new_speed;}

	double     MaxForce()const{return m_dMaxForce;}
	void      SetMaxForce(double mf){m_dMaxForce = mf;}

	bool      IsSpeedMaxedOut()const{return m_dMaxSpeed*m_dMaxSpeed >= m_vVelocity.LengthSq();}
	double     Speed()const{return m_vVelocity.Length();}
	double     SpeedSq()const{return m_vVelocity.LengthSq();}

	Vector2D  Heading()const{return m_vHeading;}
	void      SetHeading(Vector2D new_heading);
	bool      RotateHeadingToFacePosition(Vector2D target);

	double     MaxTurnRate()const{return m_dMaxTurnRate;}
	void      SetMaxTurnRate(double val){m_dMaxTurnRate = val;}

};

//-----------------------------------------------------------------------------
inline bool MovingEntity::RotateHeadingToFacePosition(Vector2D target)
{
	Vector2D toTarget = Vec2DNormalize(target - m_vPos);

	//first determine the angle between the heading vector and the target
	double angle = acos(m_vHeading.Dot(toTarget));

	//return true if the player is facing the target
	if (angle < 0.00001) return true;

	//clamp the amount to turn to the max turn rate
	if (angle > m_dMaxTurnRate) angle = m_dMaxTurnRate;

	//The next few lines use a rotation matrix to rotate the player's heading
	//vector accordingly
	C2DMatrix RotationMatrix;

	//notice how the direction of rotation has to be determined when creating
	//the rotation matrix
	RotationMatrix.Rotate(angle * m_vHeading.Sign(toTarget));	
	RotationMatrix.TransformVector2Ds(m_vHeading);
	RotationMatrix.TransformVector2Ds(m_vVelocity);

	//finally recreate m_vSide
	m_vSide = m_vHeading.Perp();

	return false;
}

//-----------------------------------------------------------------------------
inline void MovingEntity::SetHeading(Vector2D new_heading)
{
	assert( (new_heading.LengthSq() - 1.0) < 0.00001);

	m_vHeading = new_heading;

	//the side vector must always be perpendicular to the heading
	m_vSide = m_vHeading.Perp();
}



#endif // __H_MOVING_ENTITY_H_
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

//------------------------------------------------------------------------
//
//  Name:   SteeringBehaviors.h
//
//  Desc:   class to encapsulate steering behaviors for a Vehicle
//
//  Original Author: Mat Buckland 2002 (fup@ai-junkie.com)
//
//------------------------------------------------------------------------

#ifndef __H_STEERINGBEHAVIORS_H_
#define __H_STEERINGBEHAVIORS_H_
#pragma warning (disable:4786)

#include <vector>
#include <windows.h>
#include <string>
#include <list>

#include "Vector2D.h"
#include "OgreTemplate.h"
#include "DebugDraw.h"
#include "OgreRecastPath.h"
#include "OgreRecastSettingLoader.h"


class SinbadCharacterController;
class Wall2D;
class BaseGameEntity;


//--------------------------- Constants ----------------------------------

//the radius of the constraining circle for the wander behavior
const double WanderRad    = 2.0;
//distance the wander circle is projected in front of the agent
const double WanderDist   = 2.0;
//the maximum amount of displacement along the circle each frame
const double WanderJitterPerSec = 80.0;

//used in path following
const double WaypointSeekDist   = 10;                                          



//------------------------------------------------------------------------

class SteeringBehavior
{
public:
  
  enum summing_method{weighted_average, prioritized, dithered};

private:

  enum behavior_type
  {
    none               = 0x00000,
    seek               = 0x00002,
    flee               = 0x00004,
    arrive             = 0x00008,
    wander             = 0x00010,
    cohesion           = 0x00020,
    separation         = 0x00040,
    allignment         = 0x00080,
    obstacle_avoidance = 0x00100,
    wall_avoidance     = 0x00200,
    follow_path        = 0x00400,
    pursuit            = 0x00800,
    evade              = 0x01000,
    interpose          = 0x02000,
    hide               = 0x04000,
    flock              = 0x08000,
    offset_pursuit     = 0x10000,
  };

private:

  
  //a pointer to the owner of this instance
  SinbadCharacterController*     m_pVehicle;   
  
  //the steering force created by the combined effect of all
  //the selected behaviors
  Vector2D    m_vSteeringForce;
 
  //these can be used to keep track of friends, pursuers, or prey
  SinbadCharacterController*     m_pTargetAgent1;
  SinbadCharacterController*     m_pTargetAgent2;

  //the current target
  Vector2D    m_vTarget;

  //length of the 'detection box' utilized in obstacle avoidance
  double                 m_dDBoxLength;


  //a vertex buffer to contain the feelers rqd for wall avoidance  
  std::vector<Vector2D> m_Feelers;
  
  //the length of the 'feeler/s' used in wall detection
  double                 m_dWallDetectionFeelerLength;



  //the current position on the wander circle the agent is
  //attempting to steer towards
  Vector2D     m_vWanderTarget; 

  //explained above
  double        m_dWanderJitter;
  double        m_dWanderRadius;
  double        m_dWanderDistance;


  //multipliers. These can be adjusted to effect strength of the  
  //appropriate behavior. Useful to get flocking the way you require
  //for example.
  double        m_dWeightSeparation;
  double        m_dWeightCohesion;
  double        m_dWeightAlignment;
  double        m_dWeightWander;
  double        m_dWeightObstacleAvoidance;
  double        m_dWeightWallAvoidance;
  double        m_dWeightSeek;
  double        m_dWeightFlee;
  double        m_dWeightArrive;
  double        m_dWeightPursuit;
  double        m_dWeightOffsetPursuit;
  double        m_dWeightInterpose;
  double        m_dWeightHide;
  double        m_dWeightEvade;
  double        m_dWeightFollowPath;

  //how far the agent can 'see'
  double        m_dViewDistance;

  //pointer to any current path
  Path*          m_pPath;

  //the distance (squared) a vehicle has to be from a path waypoint before
  //it starts seeking to the next waypoint
  double        m_dWaypointSeekDistSq;


  //any offset used for formations or offset pursuit
  Vector2D     m_vOffset;



  //binary flags to indicate whether or not a behavior should be active
  int           m_iFlags;

  
  //Arrive makes use of these to determine how quickly a vehicle
  //should decelerate to its target
  enum Deceleration{slow = 3, normal = 2, fast = 1};

  //default
  Deceleration m_Deceleration;

  //is cell space partitioning to be used or not?
  bool          m_bCellSpaceOn;
 
  //what type of method is used to sum any active behavior
  summing_method  m_SummingMethod;

  // Debug renderers for Ogre3D debug rendering
  DebugDrawGL* ddForce;
  DebugDrawGL* ddWanderCircle;
  DebugDrawGL* ddWanderTarget;
  DebugDrawGL* ddDetBox;
  DebugDrawGL* ddDetBoxHit;
  DebugDrawGL* ddFeelers;

  //this function tests if a specific bit of m_iFlags is set
  bool      On(behavior_type bt){return (m_iFlags & bt) == bt;}

  bool      AccumulateForce(Vector2D &sf, Vector2D ForceToAdd);

  //creates the antenna utilized by the wall avoidance behavior
  void      CreateFeelers();



   /* .......................................................

                    BEGIN BEHAVIOR DECLARATIONS

      .......................................................*/


  //this behavior moves the agent towards a target position
  Vector2D Seek(Vector2D TargetPos);

  //this behavior returns a vector that moves the agent away
  //from a target position
  Vector2D Flee(Vector2D TargetPos);

  //this behavior is similar to seek but it attempts to arrive 
  //at the target position with a zero velocity
  Vector2D Arrive(Vector2D     TargetPos,
                  Deceleration deceleration);

  //this behavior predicts where an agent will be in time T and seeks
  //towards that point to intercept it.
  Vector2D Pursuit(const SinbadCharacterController* agent);

  //this behavior maintains a position, in the direction of offset
  //from the target SinbadCharacterController
  Vector2D OffsetPursuit(const SinbadCharacterController* agent, const Vector2D offset);

  //this behavior attempts to evade a pursuer
  Vector2D Evade(const SinbadCharacterController* agent);

  //this behavior makes the agent wander about randomly
  Vector2D Wander();

  //this returns a steering force which will attempt to keep the agent 
  //away from any obstacles it may encounter
  Vector2D ObstacleAvoidance(const std::vector<BaseGameEntity*>& obstacles);

  //this returns a steering force which will keep the agent away from any
  //walls it may encounter
  Vector2D WallAvoidance(const std::vector<Wall2D> &walls);

  
  //given a series of Vector2Ds, this method produces a force that will
  //move the agent along the waypoints in order
  Vector2D FollowPath();

  //this results in a steering force that attempts to steer the vehicle
  //to the center of the vector connecting two moving agents.
  Vector2D Interpose(const SinbadCharacterController* VehicleA, const SinbadCharacterController* VehicleB);

  //given another agent position to hide from and a list of BaseGameEntitys this
  //method attempts to put an obstacle between itself and its opponent
  Vector2D Hide(const SinbadCharacterController* hunter, const std::vector<BaseGameEntity*>& obstacles);


  // -- Group Behaviors -- //

  Vector2D Cohesion(const std::vector<SinbadCharacterController*> &agents);
  
  Vector2D Separation(const std::vector<SinbadCharacterController*> &agents);

  Vector2D Alignment(const std::vector<SinbadCharacterController*> &agents);

  //the following three are the same as above but they use cell-space
  //partitioning to find the neighbors
  Vector2D CohesionPlus(const std::vector<SinbadCharacterController*> &agents);
  Vector2D SeparationPlus(const std::vector<SinbadCharacterController*> &agents);
  Vector2D AlignmentPlus(const std::vector<SinbadCharacterController*> &agents);

    /* .......................................................

                       END BEHAVIOR DECLARATIONS

      .......................................................*/

  //calculates and sums the steering forces from any active behaviors
  Vector2D CalculateWeightedSum();
  Vector2D CalculatePrioritized();
  Vector2D CalculateDithered();

  //helper method for Hide. Returns a position located on the other
  //side of an obstacle to the pursuer
  Vector2D GetHidingPosition(const Vector2D& posOb,
                              const double     radiusOb,
                              const Vector2D& posHunter);



  
  
public:

  SteeringBehavior(SinbadCharacterController* agent);

  virtual ~SteeringBehavior();

  //calculates and sums the steering forces from any active behaviors
  Vector2D Calculate();

  //calculates the component of the steering force that is parallel
  //with the SinbadCharacterController heading
  double    ForwardComponent();

  //calculates the component of the steering force that is perpendicuar
  //with the SinbadCharacterController heading
  double    SideComponent();



  //renders visual aids and info for seeing how each behavior is
  //calculated
  void      RenderAids();

  void      SetTarget(const Vector2D t){m_vTarget = t;}

  void      SetTargetAgent1(SinbadCharacterController* Agent){m_pTargetAgent1 = Agent;}
  void      SetTargetAgent2(SinbadCharacterController* Agent){m_pTargetAgent2 = Agent;}

  void      SetOffset(const Vector2D offset){m_vOffset = offset;}
  Vector2D  GetOffset()const{return m_vOffset;}

  void      SetPath(std::list<Vector2D> new_path){m_pPath->Set(new_path);}
  void		SetPathLoopOn() { m_pPath->LoopOn(); }
  void		SetPathLoopOff() { m_pPath->LoopOff(); }
  void		SetPathLoop(bool _loop) { m_pPath->SetLoop(_loop); }
  void      CreateRandomPath(int num_waypoints, int mx, int my, int cx, int cy)const
            {m_pPath->CreateRandomPath(num_waypoints, mx, my, cx, cy);}

  bool		PathDone() { return m_pPath->GetPathRunThroughOnce(); }
  void		SetPathDone(bool _doneOnce) { m_pPath->SetPathRunThroughOnce(_doneOnce); }

  Vector2D Force()const{return m_vSteeringForce;}

  void      ToggleSpacePartitioningOnOff(){m_bCellSpaceOn = !m_bCellSpaceOn;}
  bool      isSpacePartitioningOn()const{return m_bCellSpaceOn;}

  void      SetSummingMethod(summing_method sm){m_SummingMethod = sm;}


  void FleeOn(){m_iFlags |= flee;}
  void SeekOn(){m_iFlags |= seek;}
  void ArriveOn(){m_iFlags |= arrive;}
  void WanderOn(){m_iFlags |= wander;}
  void PursuitOn(SinbadCharacterController* v){m_iFlags |= pursuit; m_pTargetAgent1 = v;}
  void EvadeOn(SinbadCharacterController* v){m_iFlags |= evade; m_pTargetAgent1 = v;}
  void CohesionOn(){m_iFlags |= cohesion;}
  void SeparationOn(){m_iFlags |= separation;}
  void AlignmentOn(){m_iFlags |= allignment;}
  void ObstacleAvoidanceOn(){m_iFlags |= obstacle_avoidance;}
  void WallAvoidanceOn(){m_iFlags |= wall_avoidance;}
  void FollowPathOn(){m_iFlags |= follow_path;}
  void InterposeOn(SinbadCharacterController* v1, SinbadCharacterController* v2){m_iFlags |= interpose; m_pTargetAgent1 = v1; m_pTargetAgent2 = v2;}
  void HideOn(SinbadCharacterController* v){m_iFlags |= hide; m_pTargetAgent1 = v;}
  void OffsetPursuitOn(SinbadCharacterController* v1, const Vector2D offset){m_iFlags |= offset_pursuit; m_vOffset = offset; m_pTargetAgent1 = v1;}  
  void FlockingOn(){CohesionOn(); AlignmentOn(); SeparationOn(); WanderOn();}

  void FleeOff()  {if(On(flee))   m_iFlags ^=flee;}
  void SeekOff()  {if(On(seek))   m_iFlags ^=seek;}
  void ArriveOff(){if(On(arrive)) m_iFlags ^=arrive;}
  void WanderOff(){if(On(wander)) m_iFlags ^=wander;}
  void PursuitOff(){if(On(pursuit)) m_iFlags ^=pursuit;}
  void EvadeOff(){if(On(evade)) m_iFlags ^=evade;}
  void CohesionOff(){if(On(cohesion)) m_iFlags ^=cohesion;}
  void SeparationOff(){if(On(separation)) m_iFlags ^=separation;}
  void AlignmentOff(){if(On(allignment)) m_iFlags ^=allignment;}
  void ObstacleAvoidanceOff(){if(On(obstacle_avoidance)) m_iFlags ^=obstacle_avoidance;}
  void WallAvoidanceOff(){if(On(wall_avoidance)) m_iFlags ^=wall_avoidance;}
  void FollowPathOff(){if(On(follow_path)) m_iFlags ^=follow_path;}
  void InterposeOff(){if(On(interpose)) m_iFlags ^=interpose;}
  void HideOff(){if(On(hide)) m_iFlags ^=hide;}
  void OffsetPursuitOff(){if(On(offset_pursuit)) m_iFlags ^=offset_pursuit;}
  void FlockingOff(){CohesionOff(); AlignmentOff(); SeparationOff(); WanderOff();}

  bool isFleeOn(){return On(flee);}
  bool isSeekOn(){return On(seek);}
  bool isArriveOn(){return On(arrive);}
  bool isWanderOn(){return On(wander);}
  bool isPursuitOn(){return On(pursuit);}
  bool isEvadeOn(){return On(evade);}
  bool isCohesionOn(){return On(cohesion);}
  bool isSeparationOn(){return On(separation);}
  bool isAlignmentOn(){return On(allignment);}
  bool isObstacleAvoidanceOn(){return On(obstacle_avoidance);}
  bool isWallAvoidanceOn(){return On(wall_avoidance);}
  bool isFollowPathOn(){return On(follow_path);}
  bool isInterposeOn(){return On(interpose);}
  bool isHideOn(){return On(hide);}
  bool isOffsetPursuitOn(){return On(offset_pursuit);}

  double DBoxLength()const{return m_dDBoxLength;}
  const std::vector<Vector2D>& GetFeelers()const{return m_Feelers;}
  
  double WanderJitter()const{return m_dWanderJitter;}
  double WanderDistance()const{return m_dWanderDistance;}
  double WanderRadius()const{return m_dWanderRadius;}

  double SeparationWeight()const{return m_dWeightSeparation;}
  double AlignmentWeight()const{return m_dWeightAlignment;}
  double CohesionWeight()const{return m_dWeightCohesion;}

  double WaypointSeekDistance()const{return m_dWaypointSeekDistSq;}
  void setWaypointSeekDistance(double _waypointSeek) {m_dWaypointSeekDistSq = _waypointSeek;}

};

#endif // __H_STEERINGBEHAVIORS_H_
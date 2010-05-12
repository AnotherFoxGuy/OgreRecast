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

// * THIS CODE HAS BEEN TAKEN FROM THE OGRE3D SAMPLES
// *      Original Version's Author: 		Omnitor (ogre3d sdk samples)
// *		Email:							paulwilson77@dodo.com.au
 




#ifndef __SinbadController_H__
#define __SinbadController_H__

#include "Ogre.h"
#include "OIS.h"
#include "statemch.h"
#include <vector>
#include <list>
#include <string>
#include "Vector2D.h"
#include "MovingOgreRecastEntity.h"
#include "MiscUtils.h"

using namespace Ogre;

#define NUM_ANIMS 13           // number of animations the character has

enum StateName;
class OgreTemplate;
class MovableTextOverlay;
class MovableTextOverlayAttributes;
class SteeringBehavior;
class NavMeshTesterTool;
class Path;

enum EntityAIMode
{
	ENTITY_MODE_NONE = 0,
	ENTITY_MODE_IDLE,
	ENTITY_MODE_FINDPATH,
	ENTITY_MODE_AUTOMATED,
};

class SinbadCharacterController: public StateMachine, public MovingEntity
{
private:

	// all the animations our character has, and a null ID
	// some of these affect separate body parts and will be blended together
	enum AnimID
	{
		ANIM_IDLE_BASE,
		ANIM_IDLE_TOP,
		ANIM_RUN_BASE,
		ANIM_RUN_TOP,
		ANIM_HANDS_CLOSED,
		ANIM_HANDS_RELAXED,
		ANIM_DRAW_SWORDS,
		ANIM_SLICE_VERTICAL,
		ANIM_SLICE_HORIZONTAL,
		ANIM_DANCE,
		ANIM_JUMP_START,
		ANIM_JUMP_LOOP,
		ANIM_JUMP_END,
		ANIM_NONE
	};

	
public:
	
	// Altered by Paul Wilson 2010 to allow initialization to occur after instancing
	SinbadCharacterController(OgreTemplate* _m_sample, GameObject& object, NavMeshTesterTool* _tool, Vector2D position,
							  double rotation, Vector2D velocity, double mass, double max_force,
							  double max_speed, double max_turn_rate, double scale);
	~SinbadCharacterController(void);

	// DO NOT USE THIS CLASS UNTIL THIS FUNCTION HAS BEEN CALLED
	void Initialize();

	// get and set the initialized state of the controller so that we can control
	// if the engine is sending us input or not, this change made primarily to allow
	// for state changes in the app. eg. Loading times between levels etc.
	bool getInitialized() { return mIsInitialized; }
	void setInitialized(bool _setState) { mIsInitialized = _setState; }

	// public accessor for the body node object
	Ogre::SceneNode* GetBodyNode() const { return mBodyNode; }

	// updates the whole class
	void addTime(Real deltaTime, int _applicationMode);
	//updates the vehicle's position and orientation
	void        Update(double time_elapsed);

	// public accessors
	void setGroundHeight(float _gndHeight) { mGndHgt = _gndHeight; }
	float getGroundHeight(void) { return mGndHgt; }

	//---------------------------------------------------------------------------------------------------
	// Public Path Finding Stuff
	std::vector<Ogre::Vector3> mWalkList;
	Ogre::Vector3 mPathStart;
	Ogre::Vector3 mPathEnd;

	void setSinbadPosition(float x, float y, float z) { mCurrentPosition.x = x; mCurrentPosition.y = y; mCurrentPosition.z = z; }
	void setSinbadPosition(Ogre::Vector3 pos) { mCurrentPosition = pos; }
	Ogre::Vector3 getSinbadPosition(void) { return mCurrentPosition; }

	void sendThinkMessage(void);
	void sendModeChangeMessage(void);

	void setCharHeightVal(Ogre::Real _heightVal) { CHAR_HEIGHT = _heightVal; }
	Ogre::Real getCharHeightVal(void) { return CHAR_HEIGHT; }

	void setBodyUpdateTimerTotal(Ogre::Real _updateTotal) { mBodyUpdateTimerTotal = _updateTotal; }
	Ogre::Real getBodyUpdateTimerTotal(void) { return mBodyUpdateTimerTotal; }

	void setHasMoved(bool _hasMoved) { mHasMoved = _hasMoved; }
	bool getHasMoved(void) { return mHasMoved; }
	void toggleHasMoved(void) { mHasMoved = !mHasMoved; }

	void setPathStart(float x, float y, float z) { mPathStart.x = x;  mPathStart.y = y;  mPathStart.z = z; m_sposSet = true; }
	void setPathStart(Ogre::Vector3 pos) {  mPathStart = pos; m_sposSet = true; }
	void setPathStart(float* pos) {  mPathStart.x = pos[0];  mPathStart.y = pos[1];  mPathStart.z = pos[2]; m_sposSet = true; }
	Ogre::Vector3 getPathStart(void) { return mPathStart; }

	void setPathEnd(float x, float y, float z) { mPathEnd.x = x;  mPathEnd.y = y;  mPathEnd.z = z; m_eposSet = true; }
	void setPathEnd(Ogre::Vector3 pos) {  mPathEnd = pos; m_eposSet = true; }
	void setPathEnd(float* pos) { mPathEnd.x = pos[0];  mPathEnd.y = pos[1];  mPathEnd.z = pos[2]; m_eposSet = true; }
	Ogre::Vector3 getPathEnd(void) { return mPathEnd; }

	void setEntityMode(int _entMode);
	EntityAIMode getEntityMode(void) { return m_EntityAIMode; }

	void setLabelAttributes(MovableTextOverlayAttributes* _attr) { m_EntityLabelAttributes = _attr; }
	MovableTextOverlay* getLabel(void) { return m_EntityLabel; }

	void setInitialPosition(Ogre::Vector3 pos) { mInitialPosition = pos; }
	void setInitialPosition(float x, float y, float z) { mInitialPosition.x = x; mInitialPosition.y = y; mInitialPosition.z = z; }
	Ogre::Vector3 getInitialPosition(void) { return mInitialPosition; }

	////////////////////////////////////////////////////////////////////////////
	// -- PUBLIC VEHICLE AI STUFF

	//-------------------------------------------accessor methods
	SteeringBehavior*const  Steering()const{return m_pSteering;}
	NavMeshTesterTool*const         World()const{return m_tool;} 


	Vector2D    SmoothedHeading()const{return m_vSmoothedHeading;}

	bool        isSmoothingOn()const{return m_bSmoothingOn;}
	void        SmoothingOn(){m_bSmoothingOn = true;}
	void        SmoothingOff(){m_bSmoothingOn = false;}
	void        ToggleSmoothing(){m_bSmoothingOn = !m_bSmoothingOn;}

	double       TimeElapsed()const{return m_dTimeElapsed;}
	void sendFindNewPathMessage(void);
	void setEntityLabelCaption(Ogre::String _caption, int _numLines, int _maxWidth);


private:

	//disallow the copying of SinbadCharacterController types
	SinbadCharacterController(const SinbadCharacterController&);
	SinbadCharacterController& operator=(const SinbadCharacterController&);

	OgreTemplate* m_sample;
	NavMeshTesterTool* m_tool;

	void setupBody(SceneManager* sceneMgr);
	void setupAnimations();	
	
	void updateBody(Real deltaTime, int _applicationMode);
	void updateAnimations(Real deltaTime);	
	void fadeAnimations(Real deltaTime);
	
	void setBaseAnimation(AnimID id, bool reset = false);
	void setTopAnimation(AnimID id, bool reset = false);

	StateName selectIdleAnimset(void);
	void setDanceAnim(void);
	void setIdlingAnim(void);
	void setJumpingAnim(void);
	void setRunningAnimStart(void);
	void setRunningAnimEnd(void);
	void setDrawSwordsAnim(void);
	void setSwordSlashAnim(int _slashVertOrHoriz);

	bool						mIsIdling;
	bool						mIsDancing;

	float						mIdleTimerToChange;
	float						mIdleTimerCurrent;
	
	Camera*						mCamera;
	SceneNode*					mBodyNode;
	SceneNode*					mCameraPivot;
	SceneNode*					mCameraGoal;
	SceneNode*					mCameraNode;
	Real						mPivotPitch;
	Entity*						mBodyEnt;
	Entity*						mSword1;
	Entity*						mSword2;
	RibbonTrail*				mSwordTrail;
	AnimationState*				mAnims[NUM_ANIMS];    // master animation list
	AnimID						mBaseAnimID;                   // current base (full- or lower-body) animation
	AnimID						mTopAnimID;                    // current top (upper-body) animation
	bool						mFadingIn[NUM_ANIMS];            // which animations are fading in
	bool						mFadingOut[NUM_ANIMS];           // which animations are fading out
	bool						mSwordsDrawn;
	Vector3						mKeyDirection;      // player's local intended direction based on WASD keys
	Vector3						mGoalDirection;     // actual intended direction in world-space
	Real						mVerticalVelocity;     // for jumping
	Real						mTimer;                // general timer to see how long animations have been playing

	bool						mIsInitialized;		// this is for new startup procedure independant of sample framework
	Real						mCurGroundHeight;		// used for keeping character on the ground.
	Real						mGndHgt;
	Real						CHAR_HEIGHT;

	Real						mBodyUpdateTimerTotal;
	Real						mBodyUpdateTimerCurrent;
	bool						mHasMoved;

	//-- Names of entities and nodes etc
	Ogre::String				mBodyName;
	Ogre::String				mBodyEntity;
	Ogre::String				mRibbonTrail;
	Ogre::String				mLeftSword;
	Ogre::String				mRightSword;
	Ogre::String				mEntityLabelName;

	//---------------------------------------------------------------------------------------------------
	// Private Path Finding Stuff

	// this method represents the logic that drives the descisions that this entity makes
	// its a pretty basic state machine, made possible by the awesome work of Steve Rabin. You can
	// find more info about at the AIWisdom.com AI site.
	virtual bool States( State_Machine_Event event, MSG_Object * msg, int state, int substate );
	virtual void recalc(void);
	virtual void findStartEndPositions();

	objectID m_curTarget;
	GameObject* GetClosestPlayer( void );

	bool mHasPath;
	bool mFindingPath;
	int mCurrentPathPoint;
	float mStuckCounter;
	float mChangeRunAnimCount;
	float mDistLeft;
	float mLastDist;
	bool mNeedToWalk;
	bool mIsWalking;
	bool mNeedToTurn;
	bool mIsTurning;
	Ogre::Vector3 mCurrentPosition;
	Ogre::Vector3 mNextPosition;
	Ogre::Vector3 mInitialPosition;

	static const int MAX_POLYS = 2048; //512;
	static const int MAX_SMOOTH = 12096;

	dtPolyRef m_startRef;
	dtPolyRef m_endRef;
	dtPolyRef m_polys[MAX_POLYS];
	dtPolyRef m_parent[MAX_POLYS];
	int m_npolys;
	float m_straightPath[MAX_POLYS*3];
	unsigned char m_straightPathFlags[MAX_POLYS];
	dtPolyRef m_straightPathPolys[MAX_POLYS];
	int m_nstraightPath;
	float m_polyPickExt[3];
	float m_smoothPath[MAX_SMOOTH*3];
	int m_nsmoothPath;

	float m_spos[3];
	float m_epos[3];
	float m_hitPos[3];
	float m_hitNormal[3];
	bool m_hitResult;
	float m_distanceToWall;
	bool m_sposSet;
	bool m_eposSet;

	int m_pathIterNum;
	const dtPolyRef* m_pathIterPolys; 
	int m_pathIterPolyCount;
	float m_prevIterPos[3], m_iterPos[3], m_steerPos[3], m_targetPos[3];

	static const int MAX_STEER_POINTS = 200;
	float m_steerPoints[MAX_STEER_POINTS*3];
	int m_steerPointCount;
	dtQueryFilter m_filter;
	EntityAIMode m_EntityAIMode;
	
	// object to smooth out the framerate for animations and AI
	Smoother<double>*	m_pFrameSmoother;
	// this is managed completely from this class instance
	MovableTextOverlay* m_EntityLabel;

	// this pointer to an object managed in the NavMeshTesterTool class, do not delete this manually
	// just reset the pointer to zero
	MovableTextOverlayAttributes* m_EntityLabelAttributes; // DO NOT DELETE THIS MANUALLY

	//////////////////////////////////////////////////////////////////////////////////////////
	// -- VEHICLE AI

	//the steering behavior class
	SteeringBehavior*     m_pSteering;
	Smoother<Vector2D>*  m_pHeadingSmoother;
	Vector2D             m_vSmoothedHeading;
	bool                  m_bSmoothingOn;
	double                m_dTimeElapsed;

	Path*                         m_pPath;
	//buffer for the vehicle shape
	std::vector<Vector2D> m_vecVehicleVB;
	// object to draw to debug visuals for this vehicle
	DebugDrawGL* dd;
	void InitializeBuffer(void);
	void handleRenderDebug(void);
	void handleLabelUpdate(float _deltaTime);
};

#endif // __SinbadController_H_

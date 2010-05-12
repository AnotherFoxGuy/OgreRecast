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


#include "Ogre.h"
#include "OIS.h"
#include "OgreTemplate.h"
#include "SharedData.h"
#include "InputGeom.h"
#include "MeshLoaderObj.h"
#include "NavMeshTesterTool.h"
#include "GUtility.h"
#include "Recast.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"

#include "SinbadController.h"
#include "MoveableTextOverlay.h"
#include "RectLayoutManager.h"

#include "OgreTerrain.h"
#include "OgreTerrainGroup.h"

#include "database.h"

#include "OgreRecastSteeringBehaviour.h"
#include "C2DMatrix.h"
#include "Geometry.h"
#include "Transformations.h"
#include "CellSpacePartition.h"
#include "OgreRecastPath.h"

//------------------------------------------------------------------------------------
// TODO : Replace these properly with variables, setter and getters etc etc..
#define CAM_HEIGHT 5  //(12)         // height of camera above character's center of mass
#define RUN_SPEED 30           // character running speed in units per second
#define TURN_SPEED 720.0f      // character turning in degrees per second
#define ANIM_FADE_SPEED 7.5f   // animation crossfade speed in % of full weight per second
#define JUMP_ACCEL 20.0f       // character jump acceleration in upward units per squared second
#define GRAVITY 90.0f          // gravity in downward units per squared second
#define ZOOMIN 8			   // the clamp for zoom in
#define ZOOMOUT 55			   // the clamp for zooming out
#define CAM_SPEED 30.0f		   // turning speed of the camera

using namespace Ogre;

const float frameMultiplierTerrain = 3.5f;
const float frameMultiplierOriginal = 2.5f;

using std::vector;
using std::list;

//------------------------------------------------------------------------------------
// ENTITY STATES

//Add new states here
enum StateName 
{
	STATE_Initialize,	//Note: the first enum is the starting state
	STATE_Wander,
	STATE_Attack,
	STATE_Die,
	STATE_ModeChange,
	STATE_FindPath,
	STATE_WalkPath,
	STATE_Idle,
	STATE_Dance,
	STATE_Think,
	STATE_Shutdown,
};

//Add new substates here
enum SubstateName 
{
	SUBSTATE_WalkPath_Walk,
	SUBSTATE_WalkPath_Stop,
	SUBSTATE_WalkPath_Turn,
	SUBSTATE_WalkPath_Jump,
};

//------------------------------------------------------------------------------------
inline bool inRange(const float* v1, const float* v2, const float r, const float h)
{
	const float dx = v2[0] - v1[0];
	const float dy = v2[1] - v1[1];
	const float dz = v2[2] - v1[2];
	return (dx*dx + dz*dz) < r*r && fabsf(dy) < h;
}
//------------------------------------------------------------------------------------
static bool getSteerTarget(dtNavMesh* navMesh, const float* startPos, const float* endPos,
						   const float minTargetDist,
						   const dtPolyRef* path, const int pathSize,
						   float* steerPos, unsigned char& steerPosFlag, dtPolyRef& steerPosRef,
						   float* outPoints = 0, int* outPointCount = 0)							 
{
	// Find steer target.
	static const int MAX_STEER_POINTS = 3;
	float steerPath[MAX_STEER_POINTS*3];
	unsigned char steerPathFlags[MAX_STEER_POINTS];
	dtPolyRef steerPathPolys[MAX_STEER_POINTS];
	int nsteerPath = navMesh->findStraightPath(startPos, endPos, path, pathSize,
		steerPath, steerPathFlags, steerPathPolys, MAX_STEER_POINTS);
	if (!nsteerPath)
		return false;

	if (outPoints && outPointCount)
	{
		*outPointCount = nsteerPath;
		for (int i = 0; i < nsteerPath; ++i)
			rcVcopy(&outPoints[i*3], &steerPath[i*3]);
	}


	// Find vertex far enough to steer to.
	int ns = 0;
	while (ns < nsteerPath)
	{
		// Stop at Off-Mesh link or when point is further than slop away.
		if ((steerPathFlags[ns] & DT_STRAIGHTPATH_OFFMESH_CONNECTION) ||
			!inRange(&steerPath[ns*3], startPos, minTargetDist, 1000.0f))
			break;
		ns++;
	}
	// Failed to find good point to steer to.
	if (ns >= nsteerPath)
		return false;

	rcVcopy(steerPos, &steerPath[ns*3]);
	steerPosFlag = steerPathFlags[ns];
	steerPosRef = steerPathPolys[ns];

	return true;
}
//------------------------------------------------------------------------------------
static void getPolyCenter(dtNavMesh* navMesh, dtPolyRef ref, float* center)
{
	const dtPoly* p = navMesh->getPolyByRef(ref);
	if (!p) return;
	const float* verts = navMesh->getPolyVertsByRef(ref);
	center[0] = 0;
	center[1] = 0;
	center[2] = 0;
	for (int i = 0; i < (int)p->vertCount; ++i)
	{
		const float* v = &verts[p->verts[i]*3];
		center[0] += v[0];
		center[1] += v[1];
		center[2] += v[2];
	}
	const float s = 1.0f / p->vertCount;
	center[0] *= s;
	center[1] *= s;
	center[2] *= s;
}




// CONSTRUCTION / DESTRUCTION --------------------------------------------------------------------------

SinbadCharacterController::SinbadCharacterController(OgreTemplate* _m_sample, GameObject& object, NavMeshTesterTool* _tool, Vector2D position,
	double rotation, Vector2D velocity, double mass, double max_force,
	double max_speed, double max_turn_rate, double scale ):
StateMachine( object ), MovingEntity(position, scale, velocity, max_speed, Vector2D(sin(rotation),-cos(rotation)),
									 mass, Vector2D(scale,scale), max_turn_rate, max_force),
m_vSmoothedHeading(Vector2D(0,0)), m_bSmoothingOn(true), m_dTimeElapsed(0.0)
{
	mIsInitialized = false;
	mGndHgt = 0.0f;
	mCurGroundHeight = 0.0f;
	if(_m_sample)
		m_sample = _m_sample;
	if(_tool)
		m_tool = _tool;
	mHasPath = false;
	mPathEnd = Ogre::Vector3::ZERO;
	mPathStart = Ogre::Vector3::ZERO;
	mNextPosition = Ogre::Vector3::ZERO;
	mCurrentPosition = Ogre::Vector3::ZERO;
	mIsIdling = true;
	mIsDancing = false;
	mIdleTimerToChange = 100.0f;
	mIdleTimerCurrent = 0.0f;
	mKeyDirection = Ogre::Vector3::ZERO;
	CHAR_HEIGHT = 5.0f;
	mBodyUpdateTimerTotal = 2.0f;
	mBodyUpdateTimerCurrent = 0.0f;
	mHasMoved = false;
	m_EntityLabel = 0;
	m_EntityLabelAttributes = 0;
	mNeedToWalk = false;
	mIsTurning = false;
	mNeedToTurn = false;
	mIsWalking = false;
	mFindingPath = true;
	mStuckCounter = 0.0f;
	mChangeRunAnimCount = 0.0f;
	mDistLeft = 100.0f;
	mLastDist = 0.0f;

	m_startRef = 0;
	m_endRef = 0;
	m_npolys = 0;
	m_nstraightPath = 0;
	m_nsmoothPath = 0;
	m_hitResult = false;
	m_distanceToWall = 0;
	m_sposSet = false;
	m_eposSet = false;
	m_pathIterNum = 0;
	m_steerPointCount = 0;
	m_polyPickExt[0] = 20;
	m_polyPickExt[1] = 30;
	m_polyPickExt[2] = 20;
	m_EntityAIMode = ENTITY_MODE_IDLE;
	m_pPath = NULL;
	m_filter.includeFlags = SAMPLE_POLYFLAGS_ALL;
	m_filter.excludeFlags = 0;
	dd = 0;

	InitializeBuffer();

	//set up the steering behavior class
	m_pSteering = new SteeringBehavior(this);    
		
	//set up the smoother
	m_pHeadingSmoother = new Smoother<Vector2D>(Prm.NumSamplesForSmoothing, Vector2D(0.0, 0.0, 0.0));
	m_pFrameSmoother = new Smoother<double>(Prm.NumFrameSamplesForSmoothing, 0.0);
}
//------------------------------------------------------------------------------------
SinbadCharacterController::~SinbadCharacterController(void)
{
	delete m_pSteering;
	delete m_pHeadingSmoother;
	delete m_pFrameSmoother;

	if(mIsInitialized)
	{

		SharedData::getSingleton().iSceneMgr->getRootSceneNode()->detachObject(mSwordTrail);
		SharedData::getSingleton().iSceneMgr->destroyMovableObject((Ogre::MovableObject*)mSwordTrail);

		mBodyEnt->detachAllObjectsFromBone();

		SharedData::getSingleton().iSceneMgr->destroyEntity(mSword1);
		SharedData::getSingleton().iSceneMgr->destroyEntity(mSword2);

		mBodyEnt->detachFromParent();
		SharedData::getSingleton().iSceneMgr->destroyEntity(mBodyEnt);

		mBodyNode->detachAllObjects();
		SharedData::getSingleton().iSceneMgr->destroySceneNode(mBodyNode);
		
		if(m_EntityLabel)
			delete m_EntityLabel;

		if(dd)
		{
			delete dd;
			dd = NULL;
		}
		m_EntityLabelAttributes = 0;
	}
}


// PUBLIC MEMBERS ------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------
void SinbadCharacterController::Initialize()
{
	setupBody(SharedData::getSingleton().iSceneMgr);
	setupAnimations();

	// setup the movable text label for this entity
	mEntityLabelName = TemplateUtils::GetUniqueEntityLabelName("EntityLabel_");
	m_EntityLabel = new MovableTextOverlay(mEntityLabelName, mBodyEntity, mBodyEnt, m_EntityLabelAttributes);
	m_EntityLabel->enable(false); // make it invisible for now
	m_EntityLabel->setUpdateFrequency(0.0025);// set update frequency to 0.01 seconds
	mIsInitialized = true;
	mGoalDirection = Vector3::ZERO;

	dd = new DebugDrawGL();
	dd->setMaterialScript(Ogre::String("EntityPoints"));
	dd->setOffset(0.30f);

	Ogre::String capText = "MaxForce : " + Ogre::StringConverter::toString((float)MaxForce()) + "\n" +
		"MaxSpeed : " + Ogre::StringConverter::toString((float)MaxSpeed()) + "\n" + 
		"WP Seek  : " + Ogre::StringConverter::toString((float)Steering()->WaypointSeekDistance());
	setEntityLabelCaption(capText, 3, 15);
}

//------------------------------------------------------------------------------------
void SinbadCharacterController::addTime(Real deltaTime, int _applicationMode)
{
	mChangeRunAnimCount += deltaTime;
	
	float time_elapsed = m_pFrameSmoother->Update(deltaTime);

	Update((time_elapsed));
	updateBody(time_elapsed, _applicationMode);
	updateAnimations(time_elapsed);
	handleLabelUpdate(time_elapsed);
	handleRenderDebug();		
}

//------------------------------ Update ----------------------------------
//
//  Updates the vehicle's position from a series of steering behaviors
//------------------------------------------------------------------------
void SinbadCharacterController::Update(double time_elapsed)
{   
	Ogre::Vector3 OldBodyPos = mBodyNode->getPosition();
	if(!m_pSteering->PathDone() && !mFindingPath)
	{
		//update the time elapsed
		m_dTimeElapsed = time_elapsed;

		//keep a record of its old position so we can update its cell later
		//in this method
		Vector2D OldPos = Pos();

		Vector2D SteeringForce;

		//calculate the combined force from each steering behavior in the 
		//vehicle's list
		SteeringForce = m_pSteering->Calculate();

		//Acceleration = Force/Mass
		Vector2D acceleration = SteeringForce / m_dMass;

		//update velocity
		m_vVelocity += acceleration * time_elapsed; 

		//make sure vehicle does not exceed maximum velocity
		m_vVelocity.Truncate(m_dMaxSpeed);

		//update the position
		m_vPos += m_vVelocity * time_elapsed;

		//update the heading if the vehicle has a non zero velocity
		if (m_vVelocity.LengthSq() > 0.00000001)
		{    
			m_vHeading = Vec2DNormalize(m_vVelocity);
			m_vSide = m_vHeading.Perp();
		}
		//EnforceNonPenetrationConstraint(this, World()->Agents());

		//treat the screen as a toroid
		WrapAround(m_vPos, m_tool->cxClientMin(), m_tool->cyClientMin(), m_tool->cxClient(), m_tool->cyClient());

		//update the vehicle's current cell if space partitioning is turned on
		if (Steering()->isSpacePartitioningOn())
		{
			World()->CellSpace()->UpdateEntity(this, OldPos);
		}

		mBodyNode->setPosition(m_vPos.x, mBodyNode->getPosition().y, m_vPos.y);

		if (isSmoothingOn())
		{
			m_vSmoothedHeading = m_pHeadingSmoother->Update(Heading());
			mBodyNode->setDirection(Ogre::Vector3(m_vSmoothedHeading.x, 0, m_vSmoothedHeading.y), Ogre::Node::TransformSpace::TS_WORLD, Ogre::Vector3::UNIT_Z);
		}
		else
		{
			mBodyNode->setDirection(Ogre::Vector3(m_vHeading.x, 0, m_vHeading.y), Ogre::Node::TransformSpace::TS_WORLD, Ogre::Vector3::UNIT_Z);
		}

		mHasMoved = true;
	}
	else if(m_pSteering->PathDone() && !mFindingPath)
	{
		m_pSteering->FollowPathOff();
		m_vVelocity.Zero();
		mFindingPath = true;
		sendFindNewPathMessage();
	}
	else
	{
		m_vVelocity.Zero();
	}

	// -- RUN ANIMS LOGIC
	if(mBodyNode->getPosition() == OldBodyPos)
	{
		setRunningAnimEnd();
		mIsWalking = false;
	}
	else if(mBodyNode->getPosition() != OldBodyPos && !mIsWalking)
	{
		setRunningAnimStart();
		mIsWalking = true;
	}

}

// PRIVATE MEMBERS -------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------
void SinbadCharacterController::InitializeBuffer()
{
	const int NumVehicleVerts = 4;

	Vector2D vehicle[NumVehicleVerts] = {Vector2D(-1.0f,1.0f),
		Vector2D(1.0f,1.0f),
		Vector2D(1.0f,-1.0f),
		Vector2D(-1.0f,-1.0f)};

	//setup the vertex buffers and calculate the bounding radius
	for (int vtx=0; vtx<NumVehicleVerts; ++vtx)
	{
		m_vecVehicleVB.push_back(vehicle[vtx]);
	}
}

//------------------------------------------------------------------------------------
void SinbadCharacterController::setupBody(SceneManager* sceneMgr)
{
	// get unique names for the parts and store them in member vars for later use
	mBodyName = TemplateUtils::GetUniqueBodyNodeName("SinbadBodyNode_");
	mBodyEntity = TemplateUtils::GetUniqueBodyEntityName("SinbadBody_");
	mRibbonTrail = TemplateUtils::GetUniqueRibbonTrailName("RibbonTrail_");
	mLeftSword = TemplateUtils::GetUniqueLeftSwordName("SinbadSwordL_");
	mRightSword = TemplateUtils::GetUniqueRightSwordName("SinbadSwordR_");

	mBodyNode = sceneMgr->getRootSceneNode()->createChildSceneNode(mBodyName, Vector3::UNIT_Y * CHAR_HEIGHT);
	mBodyNode->showBoundingBox(true);
	mBodyEnt = sceneMgr->createEntity(mBodyEntity, "Sinbad.mesh");
	mBodyNode->attachObject(mBodyEnt);

	// create swords and attach to sheath
	mSword1 = sceneMgr->createEntity(mLeftSword, "Sword.mesh");
	mSword2 = sceneMgr->createEntity(mRightSword, "Sword.mesh");
	mBodyEnt->attachObjectToBone("Sheath.L", mSword1);
	mBodyEnt->attachObjectToBone("Sheath.R", mSword2);

	// create a couple of ribbon trails for the swords, just for fun
	NameValuePairList params;
	params["numberOfChains"] = "2";
	params["maxElements"] = "80";
	mSwordTrail = (RibbonTrail*)sceneMgr->createMovableObject(mRibbonTrail, "RibbonTrail", &params);
	mSwordTrail->setMaterialName("Examples/LightRibbonTrail");
	mSwordTrail->setTrailLength(20);
	mSwordTrail->setVisible(false);
	sceneMgr->getRootSceneNode()->attachObject(mSwordTrail);


	for (int i = 0; i < 2; i++)
	{
		mSwordTrail->setInitialColour(i, 1, 0.8f, 0);
		mSwordTrail->setColourChange(i, 0.75f, 1.25f, 1.25f, 1.25f);
		mSwordTrail->setWidthChange(i, 1);
		mSwordTrail->setInitialWidth(i, 0.5f);
	}

	mKeyDirection = Vector3::ZERO;
	mVerticalVelocity = 0;
}

//------------------------------------------------------------------------------------
void SinbadCharacterController::setupAnimations()
{
	// this is very important due to the nature of the exported animations
	mBodyEnt->getSkeleton()->setBlendMode(ANIMBLEND_CUMULATIVE);

	String animNames[] =
	{"IdleBase", "IdleTop", "RunBase", "RunTop", "HandsClosed", "HandsRelaxed", "DrawSwords",
	"SliceVertical", "SliceHorizontal", "Dance", "JumpStart", "JumpLoop", "JumpEnd"};

	// populate our animation list
	for (int i = 0; i < NUM_ANIMS; i++)
	{
		mAnims[i] = mBodyEnt->getAnimationState(animNames[i]);
		mAnims[i]->setLoop(true);
		mFadingIn[i] = false;
		mFadingOut[i] = false;
	}

	// start off in the idle state (top and bottom together)
	setBaseAnimation(ANIM_IDLE_BASE);
	setTopAnimation(ANIM_IDLE_TOP);

	// relax the hands since we're not holding anything
	mAnims[ANIM_HANDS_RELAXED]->setEnabled(true);

	mSwordsDrawn = false;
}

//------------------------------------------------------------------------------------
void SinbadCharacterController::updateBody(Real deltaTime, int _applicationMode)
{
	//mGoalDirection = Vector3::ZERO;   // we will calculate this

	mBodyUpdateTimerCurrent+= deltaTime;

	if((mBodyUpdateTimerCurrent >= mBodyUpdateTimerTotal) || mHasMoved)
	{		
		if(SharedData::getSingleton().m_AppMode == APPMODE_TERRAINSCENE)
		{
			mGndHgt = m_sample->getInputGeom()->getMesh()->mTerrainGroup->getHeightAtWorldPosition(Ogre::Vector3(mBodyNode->getPosition().x, mBodyNode->getPosition().y+10.0f, mBodyNode->getPosition().z));
		}
		
		if(SharedData::getSingleton().m_AppMode != APPMODE_TERRAINSCENE || mGndHgt < 0.0f)
		{
			Ogre::Ray ray;
			if(mBodyNode->getPosition().y <= -20.0f)
				ray.setOrigin(Vector3(mBodyNode->getPosition().x, mBodyNode->getPosition().y+20.0f, mBodyNode->getPosition().z));
			else
				ray.setOrigin(Vector3(mBodyNode->getPosition().x, mBodyNode->getPosition().y, mBodyNode->getPosition().z));
			ray.setDirection(Vector3::NEGATIVE_UNIT_Y);

			float tt = 0;
			float rays[3];
			float raye[3];
			float pos[3];
			memset(pos,  0, sizeof(pos));
			memset(rays, 0, sizeof(rays));
			memset(raye, 0, sizeof(raye));
			rays[0] = (float)ray.getOrigin().x; rays[1] = (float)ray.getOrigin().y; rays[2] = (float)ray.getOrigin().z;
			raye[0] = (float)ray.getPoint(50.0).x; raye[1] = (float)ray.getPoint(50.0).y; raye[2] = (float)ray.getPoint(50.0).z;

			if (m_sample->getInputGeom()->raycastMesh(rays, raye, tt))
			{
				pos[0] = rays[0] + (raye[0] - rays[0])*tt;
				pos[1] = rays[1] + (raye[1] - rays[1])*tt;
				pos[2] = rays[2] + (raye[2] - rays[2])*tt;
			}
			mGndHgt = pos[1];
		}

		if(mBodyNode->getPosition().y != (mGndHgt + CHAR_HEIGHT))
			if(mBaseAnimID != ANIM_JUMP_LOOP)
				mBodyNode->setPosition(mBodyNode->getPosition().x, (mGndHgt + CHAR_HEIGHT), mBodyNode->getPosition().z);
	mBodyUpdateTimerCurrent = 0.0f;
	mHasMoved = false;
	}

	if (mBaseAnimID == ANIM_JUMP_LOOP)
	{
		Vector3 bodyPos = mBodyNode->getPosition();
		// if we're jumping, add a vertical offset too, and apply gravity
		mBodyNode->translate(0, mVerticalVelocity * deltaTime, 0, Node::TS_LOCAL);
		mVerticalVelocity -= GRAVITY * deltaTime;

		Vector3 pos = mBodyNode->getPosition();
		if (pos.y <= (mGndHgt + CHAR_HEIGHT))
		{
			// if we've hit the ground, change to landing state
			pos.y = (mGndHgt + CHAR_HEIGHT);
			mBodyNode->setPosition(pos);
			setBaseAnimation(ANIM_JUMP_END, true);
			mTimer = 0;
		}
	}
}

//------------------------------------------------------------------------------------
void SinbadCharacterController::updateAnimations(Real deltaTime)
{
	Real baseAnimSpeed = 1;
	Real topAnimSpeed = 1;

	mTimer += deltaTime;

	if (mTopAnimID == ANIM_DRAW_SWORDS)
	{
		// flip the draw swords animation if we need to put it back
		topAnimSpeed = Real(mSwordsDrawn ? -1 : 1);

		// half-way through the animation is when the hand grasps the handles...
		if (mTimer >= mAnims[mTopAnimID]->getLength() / 2 &&
			mTimer - deltaTime < mAnims[mTopAnimID]->getLength() / 2)
		{
			// so transfer the swords from the sheaths to the hands
			mBodyEnt->detachAllObjectsFromBone();
			mBodyEnt->attachObjectToBone(mSwordsDrawn ? "Sheath.L" : "Handle.L", mSword1);
			mBodyEnt->attachObjectToBone(mSwordsDrawn ? "Sheath.R" : "Handle.R", mSword2);
			// change the hand state to grab or let go
			mAnims[ANIM_HANDS_CLOSED]->setEnabled(!mSwordsDrawn);
			mAnims[ANIM_HANDS_RELAXED]->setEnabled(mSwordsDrawn);

			// toggle sword trails
			if (mSwordsDrawn)
			{
				mSwordTrail->setVisible(false);
				mSwordTrail->removeNode(mSword1->getParentNode());
				mSwordTrail->removeNode(mSword2->getParentNode());
			}
			else
			{
				mSwordTrail->setVisible(true);
				mSwordTrail->addNode(mSword1->getParentNode());
				mSwordTrail->addNode(mSword2->getParentNode());
			}
		}

		if (mTimer >= mAnims[mTopAnimID]->getLength())
		{
			// animation is finished, so return to what we were doing before
			if (mBaseAnimID == ANIM_IDLE_BASE) setTopAnimation(ANIM_IDLE_TOP);
			else
			{
				setTopAnimation(ANIM_RUN_TOP);
				mAnims[ANIM_RUN_TOP]->setTimePosition(mAnims[ANIM_RUN_BASE]->getTimePosition());
			}
			mSwordsDrawn = !mSwordsDrawn;
		}
	}
	else if (mTopAnimID == ANIM_SLICE_VERTICAL || mTopAnimID == ANIM_SLICE_HORIZONTAL)
	{
		if (mTimer >= mAnims[mTopAnimID]->getLength())
		{
			// animation is finished, so return to what we were doing before
			if (mBaseAnimID == ANIM_IDLE_BASE) setTopAnimation(ANIM_IDLE_TOP);
			else
			{
				setTopAnimation(ANIM_RUN_TOP);
				mAnims[ANIM_RUN_TOP]->setTimePosition(mAnims[ANIM_RUN_BASE]->getTimePosition());
			}
		}

		// don't sway hips from side to side when slicing. that's just embarrasing.
		if (mBaseAnimID == ANIM_IDLE_BASE) baseAnimSpeed = 0;
	}
	else if (mBaseAnimID == ANIM_JUMP_START)
	{
		if (mTimer >= mAnims[mBaseAnimID]->getLength())
		{
			// takeoff animation finished, so time to leave the ground!
			setBaseAnimation(ANIM_JUMP_LOOP, true);
			// apply a jump acceleration to the character
			mVerticalVelocity = JUMP_ACCEL;
		}
	}
	else if (mBaseAnimID == ANIM_JUMP_END)
	{
		if (mTimer >= mAnims[mBaseAnimID]->getLength())
		{
			// safely landed, so go back to running or idling
			if (mKeyDirection == Vector3::ZERO)
			{
				setBaseAnimation(ANIM_IDLE_BASE);
				setTopAnimation(ANIM_IDLE_TOP);
			}
			else
			{
				setBaseAnimation(ANIM_RUN_BASE, true);
				setTopAnimation(ANIM_RUN_TOP, true);
			}
		}
	}

	// increment the current base and top animation times
	if (mBaseAnimID != ANIM_NONE) mAnims[mBaseAnimID]->addTime(deltaTime * baseAnimSpeed);
	if (mTopAnimID != ANIM_NONE) mAnims[mTopAnimID]->addTime(deltaTime * topAnimSpeed);

	// apply smooth transitioning between our animations
	fadeAnimations(deltaTime);
}

//------------------------------------------------------------------------------------
void SinbadCharacterController::fadeAnimations(Real deltaTime)
{
	for (int i = 0; i < NUM_ANIMS; i++)
	{
		if (mFadingIn[i])
		{
			// slowly fade this animation in until it has full weight
			Real newWeight = mAnims[i]->getWeight() + deltaTime * ANIM_FADE_SPEED;
			mAnims[i]->setWeight(Math::Clamp<Real>(newWeight, 0, 1));
			if (newWeight >= 1) mFadingIn[i] = false;
		}
		else if (mFadingOut[i])
		{
			// slowly fade this animation out until it has no weight, and then disable it
			Real newWeight = mAnims[i]->getWeight() - deltaTime * ANIM_FADE_SPEED;
			mAnims[i]->setWeight(Math::Clamp<Real>(newWeight, 0, 1));
			if (newWeight <= 0)
			{
				mAnims[i]->setEnabled(false);
				mFadingOut[i] = false;
			}
		}
	}
}

//------------------------------------------------------------------------------------
void SinbadCharacterController::setBaseAnimation(AnimID id, bool reset)
{
	if (mBaseAnimID >= 0 && mBaseAnimID < NUM_ANIMS)
	{
		// if we have an old animation, fade it out
		mFadingIn[mBaseAnimID] = false;
		mFadingOut[mBaseAnimID] = true;
	}

	mBaseAnimID = id;

	if (id != ANIM_NONE)
	{
		// if we have a new animation, enable it and fade it in
		mAnims[id]->setEnabled(true);
		mAnims[id]->setWeight(0);
		mFadingOut[id] = false;
		mFadingIn[id] = true;
		if (reset) mAnims[id]->setTimePosition(0);
	}
}

//------------------------------------------------------------------------------------
void SinbadCharacterController::setTopAnimation(AnimID id, bool reset)
{
	if (mTopAnimID >= 0 && mTopAnimID < NUM_ANIMS)
	{
		// if we have an old animation, fade it out
		mFadingIn[mTopAnimID] = false;
		mFadingOut[mTopAnimID] = true;
	}

	mTopAnimID = id;

	if (id != ANIM_NONE)
	{
		// if we have a new animation, enable it and fade it in
		mAnims[id]->setEnabled(true);
		mAnims[id]->setWeight(0);
		mFadingOut[id] = false;
		mFadingIn[id] = true;
		if (reset) mAnims[id]->setTimePosition(0);
	}
}

//------------------------------------------------------------------------------------
// STATE MACHINE LOGIC START
//------------------------------------------------------------------------------------

bool SinbadCharacterController::States( State_Machine_Event event, MSG_Object * msg, int state, int substate )
{
	BeginStateMachine
	///////////////////////////////////////////////////////////////
	//Global message responses
		OnMsg( MSG_Damaged )
			m_owner->SetHealth( m_owner->GetHealth() - (int)msg->GetIntData() );
				if( m_owner->GetHealth() == 0 )
				{
					ChangeState( STATE_Die );
				}

		OnMsg( MSG_ModeChange )
			ChangeState( STATE_ModeChange );

		OnMsg( MSG_FindPath )
			ChangeState( STATE_FindPath );

		OnMsg( MSG_Think )
			// CURRENTLY UNUSED - very low call cycle - for low level state changes, sensory memory updating etc
			

	///////////////////////////////////////////////////////////////
	DeclareState( STATE_Initialize )

		OnEnter

			if(m_owner->GetHealth() <= 49) m_owner->SetHealth(50);
			ChangeStateDelayed( 2.0f, STATE_ModeChange );

		OnUpdate

	///////////////////////////////////////////////////////////////
	DeclareState( STATE_Wander ) /* we should never get here yet */

		OnEnter
			SendMsgDelayedToMe( RandDelay( 2.0f, 5.0f ), MSG_Attack );

		OnUpdate
			//Wander around

		OnMsg( MSG_Attack )
			GameObject* target = GetClosestPlayer();
			if( target && target->IsAlive() )
			{
				m_curTarget = target->GetID();
				ChangeState( STATE_Attack );
			}
			else
			{
				SendMsgDelayedToMe( RandDelay( 2.0f, 5.0f ), MSG_Attack );
			}

	///////////////////////////////////////////////////////////////
	DeclareState( STATE_Attack ) /* we should never get here yet */

		OnEnter
			SendMsgToMe( MSG_Attack );

		OnMsg( MSG_Attack )
			GameObject* target = g_database.Find(m_curTarget);
			if( target && target->IsAlive() )
			{
				SendMsg( MSG_Damaged, m_curTarget, (rand()%20) ); 
				SendMsgDelayedToMe( RandDelay( 3.0f, 6.0f ), MSG_Attack, SCOPE_TO_STATE );
			}
			else
			{
				ChangeState( STATE_Wander );
			}

	///////////////////////////////////////////////////////////////
	DeclareState( STATE_Die ) /* we shoud NEVER EVER get here*/ 

		OnEnter
			//Play die animation
			ChangeStateDelayed( 10.0f, STATE_Initialize );	//Respawn after 10 seconds

		OnMsg( MSG_Damaged )
			//Do nothing

	///////////////////////////////////////////////////////////////
	DeclareState( STATE_ModeChange )

		OnEnter
			if(m_EntityAIMode == ENTITY_MODE_IDLE)
			{
				ChangeState(STATE_Idle);
			}
			else if(m_EntityAIMode == ENTITY_MODE_FINDPATH)
			{
				ChangeState(STATE_FindPath);
			}
			else if(m_EntityAIMode == ENTITY_MODE_AUTOMATED)
			{

			}
			else if(m_EntityAIMode == ENTITY_MODE_NONE)
			{
				ChangeState(STATE_Idle);
			}
						

	///////////////////////////////////////////////////////////////
	DeclareState( STATE_FindPath )

		OnEnter
			setIdlingAnim();
			mFindingPath = true;
			m_pSteering->FollowPathOff();
			m_vVelocity.Zero();
			findStartEndPositions();

		OnUpdate
			mFindingPath = true;
			m_vVelocity.Zero();
			findStartEndPositions();

		OnExit
			m_vVelocity.Zero();

	///////////////////////////////////////////////////////////////
	DeclareState( STATE_WalkPath )

		OnEnter

		OnUpdate

		OnMsg( MSG_FindPath )
			ChangeState( STATE_FindPath );

		OnExit

	///////////////////////////////////////////////////////////////
	DeclareState( STATE_Idle )

		OnEnter
			setIdlingAnim();
			m_pSteering->FollowPathOff();
			m_vVelocity.Zero();
			mIdleTimerToChange = RandDelay(5.0f, 20.0f);
			SendMsgDelayedToMe( mIdleTimerToChange, MSG_IdleChange, SCOPE_TO_STATE);

		OnUpdate
			if((RandDelay(0.0f, 100.0f) <= 1.0f) && (mBaseAnimID != ANIM_JUMP_LOOP))
				setJumpingAnim();

		OnMsg( MSG_IdleChange )
			ChangeStateDelayed(0.25f, selectIdleAnimset());

		OnExit
			m_vVelocity.Zero();

	///////////////////////////////////////////////////////////////
	DeclareState( STATE_Dance )

		OnEnter
			setDanceAnim();
			m_pSteering->FollowPathOff();
			m_vVelocity.Zero();
			mIdleTimerToChange = RandDelay(7.0f, 15.0f)	;
			SendMsgDelayedToMe( mIdleTimerToChange, MSG_IdleChange, SCOPE_TO_STATE);

		OnUpdate

		OnMsg( MSG_IdleChange )
			ChangeStateDelayed(0.25f, selectIdleAnimset());

		OnExit
			m_vVelocity.Zero();

	///////////////////////////////////////////////////////////////
	DeclareState( STATE_Think )

		OnEnter


	///////////////////////////////////////////////////////////////
	EndStateMachine
}
//------------------------------------------------------------------------------------
// STATE MACHINE LOGIC END
//------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------
void SinbadCharacterController::setEntityMode(int _entMode) 
{ 
	switch(_entMode)
	{
	case 0:
		m_EntityAIMode = ENTITY_MODE_NONE;
		break;
	case 1:
		m_EntityAIMode = ENTITY_MODE_IDLE;
		break;
	case 2:
		m_EntityAIMode = ENTITY_MODE_FINDPATH;
		break;
	case 3:
		m_EntityAIMode = ENTITY_MODE_AUTOMATED;
		break;
	case 4:
	case 5:
	case 6:
		break;
	default:
		m_EntityAIMode = ENTITY_MODE_IDLE;
	}
}

//------------------------------------------------------------------------------------
void SinbadCharacterController::sendThinkMessage(void)
{
	SendMsgToMe( MSG_Think );
}

//------------------------------------------------------------------------------------
void SinbadCharacterController::sendModeChangeMessage(void)
{
	SendMsgToMe( MSG_ModeChange );
}

//------------------------------------------------------------------------------------
void SinbadCharacterController::sendFindNewPathMessage(void)
{
	SendMsgToMe( MSG_FindPath );
}

//------------------------------------------------------------------------------------
StateName SinbadCharacterController::selectIdleAnimset(void)
{
	if(RandDelay( 0.0f, 10.0f ) >= 7.0f)
	{
		return STATE_Idle;
	}
	
	return STATE_Dance;
}

//------------------------------------------------------------------------------------
void SinbadCharacterController::setDanceAnim(void)
{
	if (mTopAnimID == ANIM_IDLE_TOP || mTopAnimID == ANIM_RUN_TOP)
	{
		// start dancing
		setBaseAnimation(ANIM_DANCE, true);
		setTopAnimation(ANIM_NONE);
		// disable hand animation because the dance controls hands
		mAnims[ANIM_HANDS_RELAXED]->setEnabled(false);
	}
}

//------------------------------------------------------------------------------------
void SinbadCharacterController::setIdlingAnim(void)
{
	if (mKeyDirection.isZeroLength() && mBaseAnimID == ANIM_RUN_BASE)
	{
		// stop running if already moving and the player doesn't want to move
		setBaseAnimation(ANIM_IDLE_BASE);
		if (mTopAnimID == ANIM_RUN_TOP) setTopAnimation(ANIM_IDLE_TOP);
	}
	else
	{
		// stop dancing
		setBaseAnimation(ANIM_IDLE_BASE);
		setTopAnimation(ANIM_IDLE_TOP);
		// re-enable hand animation
		mAnims[ANIM_HANDS_RELAXED]->setEnabled(true);
	}
}

//------------------------------------------------------------------------------------
void SinbadCharacterController::setDrawSwordsAnim(void)
{
	if ((mTopAnimID == ANIM_IDLE_TOP || mTopAnimID == ANIM_RUN_TOP))
	{
		// take swords out (or put them back, since it's the same animation but reversed)
		setTopAnimation(ANIM_DRAW_SWORDS, true);
		mTimer = 0;
	}
}

//------------------------------------------------------------------------------------
void SinbadCharacterController::setSwordSlashAnim(int _slashVertOrHoriz)
{
	if (mSwordsDrawn && (mTopAnimID == ANIM_IDLE_TOP || mTopAnimID == ANIM_RUN_TOP))
	{
		// if swords are out, and character's not doing something weird, then SLICE!
		switch(_slashVertOrHoriz)
		{
		case 0:
			setTopAnimation(ANIM_SLICE_VERTICAL, true);
			mTimer = 0;
			break;
		case 1:
			setTopAnimation(ANIM_SLICE_HORIZONTAL, true);
			mTimer = 0;
			break;
		default:
			setTopAnimation(ANIM_SLICE_VERTICAL, true);
			mTimer = 0;
			break;
		}
	}
}

//------------------------------------------------------------------------------------
void SinbadCharacterController::setJumpingAnim(void)
{
	if(mTopAnimID == ANIM_IDLE_TOP || mTopAnimID == ANIM_RUN_TOP)
	{
		// jump if on ground
		setBaseAnimation(ANIM_JUMP_START, true);
		setTopAnimation(ANIM_NONE);
		mTimer = 0;
	}
}

//------------------------------------------------------------------------------------
void SinbadCharacterController::setRunningAnimStart(void)
{
	if(mBaseAnimID == ANIM_IDLE_BASE)
	{
		// start running if not already moving and the player wants to move
		setBaseAnimation(ANIM_RUN_BASE, true);
		if (mTopAnimID == ANIM_IDLE_TOP) setTopAnimation(ANIM_RUN_TOP, true);
	}
}

//------------------------------------------------------------------------------------
void SinbadCharacterController::setRunningAnimEnd(void)
{
	if(mBaseAnimID == ANIM_RUN_BASE)
	{
		// stop running if already moving and the player doesn't want to move
		setBaseAnimation(ANIM_IDLE_BASE);
		if (mTopAnimID == ANIM_RUN_TOP) setTopAnimation(ANIM_IDLE_TOP);
	}
}

//------------------------------------------------------------------------------------
void SinbadCharacterController::findStartEndPositions()
{
	//srand(g_time.GetCurTime());
	// randomly generate an endpoint for a path from present location
	setPathStart(mBodyNode->getPosition().x, mBodyNode->getPosition().y, mBodyNode->getPosition().z);

	int negpos = RandDelay(0.0f, 10.0f);

	if(negpos > 5)
		negpos = 1;
	else
		negpos = -1;

	float XVal = 0.0f;
	float ZVal = 0.0f;

	if(SharedData::getSingleton().m_AppMode == APPMODE_TERRAINSCENE)
	{
		setPathStart(mBodyNode->getPosition().x, mGndHgt, mBodyNode->getPosition().z);

		XVal = (mBodyNode->getPosition().x + (RandDelay(100.0f, 5500.0f) / negpos));
		ZVal = (mBodyNode->getPosition().z + (RandDelay(100.0f, 5500.0f) / negpos));
		
		float gndAtEnd = m_sample->getInputGeom()->getMesh()->mTerrainGroup->getHeightAtWorldPosition(Ogre::Vector3(XVal, 5000, ZVal));
		
		setPathEnd(XVal, gndAtEnd, ZVal);
	}
	
	
	if( (SharedData::getSingleton().m_AppMode != APPMODE_TERRAINSCENE) )
	{
		XVal = (mBodyNode->getPosition().x + (RandDelay(7.5f, 1000.0f) / negpos));
		ZVal = (mBodyNode->getPosition().z + (RandDelay(7.5f, 1000.0f) / negpos));
		
		Ogre::Ray ray;
		ray.setOrigin(Vector3(XVal, 5000, ZVal));
		ray.setDirection(Vector3::NEGATIVE_UNIT_Y);

		float tt = 0;
		float rays[3];
		float raye[3];
		float pos[3];
		memset(pos,  0, sizeof(pos));
		memset(rays, 0, sizeof(rays));
		memset(raye, 0, sizeof(raye));
		rays[0] = (float)ray.getOrigin().x; rays[1] = (float)ray.getOrigin().y; rays[2] = (float)ray.getOrigin().z;
		raye[0] = (float)ray.getPoint(5000.0).x; raye[1] = (float)ray.getPoint(5000.0).y; raye[2] = (float)ray.getPoint(5000.0).z;

		if (m_sample->getInputGeom()->raycastMesh(rays, raye, tt))
		{
			pos[0] = rays[0] + (raye[0] - rays[0])*tt;
			pos[1] = rays[1] + (raye[1] - rays[1])*tt;
			pos[2] = rays[2] + (raye[2] - rays[2])*tt;
		}
	
		setPathEnd(XVal, pos[1], ZVal);

	}

	recalc();
	// if path valid change state STATE_WalkPath and handle walking the path
	if(m_nstraightPath > 1)
	{
		mHasPath = true;
		mFindingPath = false;
		ChangeState( STATE_WalkPath );
	}
	else
	{
		mHasPath = false;
		mFindingPath = true;
	}
	// if path invalid wait for update and try again till we get a valid one
}

//------------------------------------------------------------------------------------
GameObject* SinbadCharacterController::GetClosestPlayer( void )
{
	dbCompositionList list;
	g_database.ComposeList( list, OBJECT_Enemy | OBJECT_Character );

	if( list.empty() )
	{
		return 0;
	}
	else
	{
		for(dbCompositionList::iterator it = list.begin(); it != list.end(); ++it)
		{
			// TODO : add proximity check code for closest check here
		}
		return( list.front() );
	}
}


//------------------------------------------------------------------------------------
void SinbadCharacterController::recalc(void)
{
	if (!m_sample->getNavMesh())
		return;

	mWalkList.resize(0);
	if (m_sposSet)
	{
		m_spos[0] = mPathStart.x;
		m_spos[1] = mPathStart.y;
		m_spos[2] = mPathStart.z;
		m_startRef = m_sample->getNavMesh()->findNearestPoly(m_spos, m_polyPickExt, &m_filter, 0);
	}
	else
		m_startRef = 0;

	if (m_eposSet)
	{
		m_epos[0] = mPathEnd.x;
		m_epos[1] = mPathEnd.y;
		m_epos[2] = mPathEnd.z;
		m_endRef = m_sample->getNavMesh()->findNearestPoly(m_epos, m_polyPickExt, &m_filter, 0);
	}
	else
		m_endRef = 0;

	if (m_sposSet && m_eposSet && m_startRef && m_endRef)
	{
		if(m_pPath)
			delete m_pPath;
		m_pPath = new Path();
		m_npolys = m_sample->getNavMesh()->findPath(m_startRef, m_endRef, m_spos, m_epos, &m_filter, m_polys, MAX_POLYS);
		m_nstraightPath = 0;
		if (m_npolys)
		{
			m_nstraightPath = m_sample->getNavMesh()->findStraightPath(m_spos, m_epos, m_polys, m_npolys,
				m_straightPath, m_straightPathFlags,
				m_straightPathPolys, MAX_POLYS);
			if(m_nstraightPath)
			{
				for(unsigned int i = 0; i < m_nstraightPath; ++i)
				{
					m_pPath->AddWayPoint(Vector2D(m_straightPath[i * 3], m_straightPath[i * 3 + 1],  m_straightPath[i * 3 + 2]));
					mWalkList.push_back(Ogre::Vector3(m_straightPath[i * 3], m_straightPath[i * 3 + 1], m_straightPath[i * 3 + 2]));
				}
				m_pSteering->SetPath(m_pPath->GetPath());
				m_pSteering->SetPathLoopOff();
				m_pSteering->SetPathDone(false);
				m_pSteering->ArriveOff();
				m_pSteering->FollowPathOn();
			}	
		}
		m_sposSet = false;
		m_eposSet = false;
	}
	else
	{
		m_npolys = 0;
		m_nstraightPath = 0;
	}
}

//------------------------------------------------------------------------------------
void SinbadCharacterController::setEntityLabelCaption(Ogre::String _caption, int _numLines, int _maxWidth)
{
	m_EntityLabel->setCaption(_caption);
	m_EntityLabel->setCaptionLines(_numLines, _maxWidth);
}

//------------------------------------------------------------------------------------
void SinbadCharacterController::handleRenderDebug(void)
{
	dd->clear();
	//a vector to hold the transformed vertices
	static std::vector<Vector2D>  m_vecVehicleVBTrans;

	if (isSmoothingOn())
	{ 
		m_vecVehicleVBTrans = WorldTransform(m_vecVehicleVB,
			Pos(),
			SmoothedHeading(),
			SmoothedHeading().Perp(),
			Scale());
	}

	else
	{
		m_vecVehicleVBTrans = WorldTransform(m_vecVehicleVB,
			Pos(),
			Heading(),
			Side(),
			Scale());
	}

	dd->begin(DU_DRAW_POINTS, 20.0);
	for(unsigned int i = 0; i < m_vecVehicleVBTrans.size(); ++i)
	{
		dd->vertex(m_vecVehicleVBTrans[i].x, mBodyNode->getPosition().y, m_vecVehicleVBTrans[i].y, (unsigned int)0);
	}
	dd->end();
	dd->begin(DU_DRAW_LINES, 20.0);
	for(unsigned int i = 0; i < m_vecVehicleVBTrans.size(); ++i)
	{
		dd->vertex(m_vecVehicleVBTrans[i].x, mBodyNode->getPosition().y, m_vecVehicleVBTrans[i].y, (unsigned int)0);
	}
	dd->end();

	Steering()->RenderAids();
}

//------------------------------------------------------------------------------------
void SinbadCharacterController::handleLabelUpdate(float _deltaTime)
{
	if(m_EntityLabel)
	{
		RectLayoutManager m(0, 0, SharedData::getSingleton().iCamera->getViewport()->getActualWidth(),
			SharedData::getSingleton().iCamera->getViewport()->getActualHeight());
		m.setDepth(0);
		m_EntityLabel->update(_deltaTime);
		if (m_EntityLabel->isOnScreen())
		{
			RectLayoutManager::Rect r(m_EntityLabel->getPixelsLeft(), m_EntityLabel->getPixelsTop(),
				m_EntityLabel->getPixelsRight(), m_EntityLabel->getPixelsBottom());
			RectLayoutManager::RectList::iterator it = m.addData(r);
			if (it != m.getListEnd())
			{
				m_EntityLabel->setPixelsTop((*it).getTop());
				m_EntityLabel->enable(true);
			}
			else
			{
				m_EntityLabel->enable(false);
			}
		}
		else
		{
			m_EntityLabel->enable(false);
		}
	}
}
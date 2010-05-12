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


#include "OgreRecastSettingLoader.h"
#include "Ogre.h"

using namespace Ogre;

ParamLoader* ParamLoader::Instance()
{
	static ParamLoader instance;

	return &instance;
}

void ParamLoader::load(Ogre::String _cfgfile)
{
	// todo check if exists in resource system and create from default values otherwise
	mConfigFile.loadFromResourceSystem(_cfgfile, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, "=", true);

	NumAgents               = Ogre::StringConverter::parseUnsignedInt(mConfigFile.getSetting("NumAgents"));
	NumObstacles            = Ogre::StringConverter::parseUnsignedInt(mConfigFile.getSetting("NumObstacles"));
	MinObstacleRadius       = Ogre::StringConverter::parseReal(mConfigFile.getSetting("MinObstacleRadius"));
	MaxObstacleRadius       = Ogre::StringConverter::parseReal(mConfigFile.getSetting("MaxObstacleRadius"));

	NumCellsX               = Ogre::StringConverter::parseUnsignedInt(mConfigFile.getSetting("NumCellsX"));
	NumCellsY               = Ogre::StringConverter::parseUnsignedInt(mConfigFile.getSetting("NumCellsY"));

	NumSamplesForSmoothing  = Ogre::StringConverter::parseUnsignedInt(mConfigFile.getSetting("NumSamplesForSmoothing"));
	NumFrameSamplesForSmoothing = Ogre::StringConverter::parseUnsignedInt(mConfigFile.getSetting("NumFrameSamplesForSmoothing"));

	SteeringForceTweaker    = Ogre::StringConverter::parseReal(mConfigFile.getSetting("SteeringForceTweaker"));
	MaxSteeringForce        = Ogre::StringConverter::parseReal(mConfigFile.getSetting("MaxSteeringForce")) * SteeringForceTweaker;
	MaxSpeed                = Ogre::StringConverter::parseReal(mConfigFile.getSetting("MaxSpeed"));
	VehicleMass             = Ogre::StringConverter::parseReal(mConfigFile.getSetting("VehicleMass"));
	VehicleScale            = Ogre::StringConverter::parseReal(mConfigFile.getSetting("VehicleScale"));

	SeparationWeight        = Ogre::StringConverter::parseReal(mConfigFile.getSetting("SeparationWeight")) * SteeringForceTweaker;
	AlignmentWeight         = Ogre::StringConverter::parseReal(mConfigFile.getSetting("AlignmentWeight")) * SteeringForceTweaker;
	CohesionWeight          = Ogre::StringConverter::parseReal(mConfigFile.getSetting("CohesionWeight")) * SteeringForceTweaker;
	ObstacleAvoidanceWeight = Ogre::StringConverter::parseReal(mConfigFile.getSetting("ObstacleAvoidanceWeight")) * SteeringForceTweaker;
	WallAvoidanceWeight     = Ogre::StringConverter::parseReal(mConfigFile.getSetting("WallAvoidanceWeight")) * SteeringForceTweaker;
	WanderWeight            = Ogre::StringConverter::parseReal(mConfigFile.getSetting("WanderWeight")) * SteeringForceTweaker;
	SeekWeight              = Ogre::StringConverter::parseReal(mConfigFile.getSetting("SeekWeight")) * SteeringForceTweaker;
	FleeWeight              = Ogre::StringConverter::parseReal(mConfigFile.getSetting("FleeWeight")) * SteeringForceTweaker;
	ArriveWeight            = Ogre::StringConverter::parseReal(mConfigFile.getSetting("ArriveWeight")) * SteeringForceTweaker;
	PursuitWeight           = Ogre::StringConverter::parseReal(mConfigFile.getSetting("PursuitWeight")) * SteeringForceTweaker;
	OffsetPursuitWeight     = Ogre::StringConverter::parseReal(mConfigFile.getSetting("OffsetPursuitWeight")) * SteeringForceTweaker;
	InterposeWeight         = Ogre::StringConverter::parseReal(mConfigFile.getSetting("InterposeWeight")) * SteeringForceTweaker;
	HideWeight              = Ogre::StringConverter::parseReal(mConfigFile.getSetting("HideWeight")) * SteeringForceTweaker;
	EvadeWeight             = Ogre::StringConverter::parseReal(mConfigFile.getSetting("EvadeWeight")) * SteeringForceTweaker;
	FollowPathWeight        = Ogre::StringConverter::parseReal(mConfigFile.getSetting("FollowPathWeight")) * SteeringForceTweaker;

	ViewDistance            = Ogre::StringConverter::parseReal(mConfigFile.getSetting("ViewDistance"));
	MinDetectionBoxLength   = Ogre::StringConverter::parseReal(mConfigFile.getSetting("MinDetectionBoxLength"));
	WallDetectionFeelerLength=Ogre::StringConverter::parseReal(mConfigFile.getSetting("WallDetectionFeelerLength"));

	prWallAvoidance         = Ogre::StringConverter::parseReal(mConfigFile.getSetting("prWallAvoidance"));
	prObstacleAvoidance     = Ogre::StringConverter::parseReal(mConfigFile.getSetting("prObstacleAvoidance"));  
	prSeparation            = Ogre::StringConverter::parseReal(mConfigFile.getSetting("prSeparation"));
	prAlignment             = Ogre::StringConverter::parseReal(mConfigFile.getSetting("prAlignment"));
	prCohesion              = Ogre::StringConverter::parseReal(mConfigFile.getSetting("prCohesion"));
	prWander                = Ogre::StringConverter::parseReal(mConfigFile.getSetting("prWander"));
	prSeek                  = Ogre::StringConverter::parseReal(mConfigFile.getSetting("prSeek"));
	prFlee                  = Ogre::StringConverter::parseReal(mConfigFile.getSetting("prFlee"));
	prEvade                 = Ogre::StringConverter::parseReal(mConfigFile.getSetting("prEvade"));
	prHide                  = Ogre::StringConverter::parseReal(mConfigFile.getSetting("prHide"));
	prArrive                = Ogre::StringConverter::parseReal(mConfigFile.getSetting("prArrive"));

	MaxTurnRatePerSecond    = Pi;

}
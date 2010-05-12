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

// Based on code Originally authored by Mat Buckland - fup@AIJunkie.com

#ifndef __H_OGRERECASTPARAMLOADER_H_
#define __H_OGRERECASTPARAMLOADER_H_

#include "Ogre.h"
#include "MiscUtils.h"



#define Prm (*ParamLoader::Instance())
#define CONFIG_FILENAME Ogre::String("OgreRecastAISettings.cfg")


class ParamLoader
{
private:

	Ogre::ConfigFile mConfigFile;
	Ogre::String mConfigFileName;

	ParamLoader()
	{
		mConfigFileName = CONFIG_FILENAME;
		load(mConfigFileName);
	}

	void load(Ogre::String _cfgfile);

public:

	static ParamLoader* Instance();

	void setConfigFilename(Ogre::String _cfgfile) { mConfigFileName = _cfgfile; }
	Ogre::String getConfigFilename(void) { return mConfigFileName; }

	int	NumAgents;
	int	NumObstacles;
	double MinObstacleRadius;
	double MaxObstacleRadius;

	//number of horizontal cells used for spatial partitioning
	int   NumCellsX;
	//number of vertical cells used for spatial partitioning
	int   NumCellsY;

	//how many samples the smoother will use to average a value
	int   NumSamplesForSmoothing;

	//how many samples the smoother will use to average the AI an animation values
	int	  NumFrameSamplesForSmoothing;

	//used to tweak the combined steering force (simply altering the MaxSteeringForce
	//will NOT work!This tweaker affects all the steering force multipliers
	//too).
	double SteeringForceTweaker;

	double MaxSteeringForce;
	double MaxSpeed;
	double VehicleMass;

	double VehicleScale;
	double MaxTurnRatePerSecond;

	double SeparationWeight;
	double AlignmentWeight;
	double CohesionWeight;
	double ObstacleAvoidanceWeight;
	double WallAvoidanceWeight;
	double WanderWeight;
	double SeekWeight;
	double FleeWeight;
	double ArriveWeight;
	double PursuitWeight;
	double OffsetPursuitWeight;
	double InterposeWeight;
	double HideWeight;
	double EvadeWeight;
	double FollowPathWeight;

	//how close a neighbour must be before an agent perceives it (considers it
	//to be within its neighborhood)
	double ViewDistance;

	//used in obstacle avoidance
	double MinDetectionBoxLength;

	//used in wall avoidance
	double WallDetectionFeelerLength;

	//these are the probabilities that a steering behavior will be used
	//when the prioritized dither calculate method is used
	double prWallAvoidance;
	double prObstacleAvoidance;
	double prSeparation;
	double prAlignment;
	double prCohesion;
	double prWander;
	double prSeek;
	double prFlee;
	double prEvade;
	double prHide;
	double prArrive;

};

#endif // __H_OGRERECASTPARAMLOADER_H_
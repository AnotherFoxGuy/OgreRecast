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
// warranty.  In no event will the author/s be held liable for any damages			//
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

/////////////////////////////////////////////////////////////////////////////////////
// ORIGINALLY PART OF RECAST DEMO APPLICATION - MODIFIED VERSION			       //
/////////////////////////////////////////////////////////////////////////////////////

#include "MeshLoaderObj.h"
#include "SharedData.h"
#include "GUtility.h"

#include "OgreTerrain.h"
#include "OgreTerrainGroup.h"
#include "OgreTerrainQuadTreeNode.h"
#include "OgreTerrainMaterialGeneratorA.h"
#include "OgreTerrainPaging.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>

using namespace Ogre;

// CURRENTLY UNUSED DEFINES ( mostly )
// can use these to allow system to page in initially, though 
// recast isnt aware, so if u try to generate a mesh without all
// terrain loaded you will end up only generating a partial mesh
#define TERRAIN_PAGE_MIN_X -1
#define TERRAIN_PAGE_MIN_Y -1
#define TERRAIN_PAGE_MAX_X 0
#define TERRAIN_PAGE_MAX_Y 0

// heightmap from PLSM2 originally
#define TERRAIN_HEIGHTMAP_FULLPATH_NAME Ogre::String("../../media/_ORIG_Heightmaps/ps_height_1k.png")
#define TERRAIN_HEIGHTMAP_ORIGINAL String("ps_height_1k.png")
#define TERRAIN_FILE_PREFIX String("TestPage")
#define TERRAIN_FILE_SUFFIX String("dat")
#define TERRAIN_WORLD_SIZE 6000
#define TERRAIN_SIZE 513


rcMeshLoaderObj::rcMeshLoaderObj() :
	m_verts(0),	m_tris(0), m_normals(0), m_vertCount(0), m_triCount(0),
	myManualObjectMaterial(0), obj(0), mMatsLoaded(false), ntris(0),
	tris(0), verts(0), nverts(0), numEnt(0), mTerrainGroup(0), mTerrainPaging(0),
	mTerrainGlobals(0), mPageManager(0), mFly(true), mFallVelocity(0), mMode(MODE_NORMAL),
	mShadowMode(SHADOWS_DEPTH), mLayerEdit(1), mBrushSizeTerrainSpace(0.02f), mHeightUpdateCountDown(0),
	mTerrainPos(0,0,0), mTerrainsImported(false), mHousesLoaded(false), mSceneMgr(0), mLayerCount(0),
	mPagesTotal(0)
{
	for(int i=0;i<3;++i)
	{
		bmin[i] = 0;
		bmax[i] = 0;
	}
	// Update terrain at max 20fps
	mHeightUpdateRate = 1.0f / 20.0f;

	mSceneMgr = SharedData::getSingleton().iSceneMgr;
}

rcMeshLoaderObj::~rcMeshLoaderObj()
{
	delete [] m_verts;
	delete [] m_normals;
	delete [] m_tris;
	delete [] tris;
	delete [] verts;
	
	for(unsigned int i = 0; i < numEnt; ++i)
	{
		SharedData::getSingleton().iSceneMgr->getSceneNode(m_entNames[i])->detachAllObjects();
		SharedData::getSingleton().iSceneMgr->destroySceneNode(m_entNames[i]);
		SharedData::getSingleton().iSceneMgr->destroyEntity(m_entNames[i]);
	}
	SharedData::getSingleton().mNavNodeList.resize(0);

	if (mTerrainPaging)
	{
		OGRE_DELETE mTerrainPaging;
		OGRE_DELETE mPageManager;
	}
	else
	{ 
		if(mTerrainGroup)
			OGRE_DELETE mTerrainGroup;
	}

	if(mTerrainGlobals)
		OGRE_DELETE mTerrainGlobals;

	if(!mPSSMSetup.isNull())
		mPSSMSetup.setNull();
	
}

void rcMeshLoaderObj::addVertex(float x, float y, float z, int& cap)
{
	if (m_vertCount+1 > cap)
	{	
		cap = !cap ? 8 : cap*2;
		float* nv = new float[cap*3];
		if (m_vertCount)
			memcpy(nv, m_verts, m_vertCount*3*sizeof(float));
		delete [] m_verts;
		m_verts = nv;
	}
	float* dst = &m_verts[m_vertCount*3];
	*dst++ = x;
	*dst++ = y;
	*dst++ = z;
	m_vertCount++;
	//TODO : add normal calculation after position is called - we need to add the Normal to the manual object
	//		 in the order that we create the verts, indices can be done seperately, but not the vertex decl data
	//obj->position(x, y, z);
}

void rcMeshLoaderObj::addTriangle(int a, int b, int c, int& cap)
{
	if (m_triCount+1 > cap)
	{
		cap = !cap ? 8 : cap*2;
		int* nv = new int[cap*3];
		if (m_triCount)
			memcpy(nv, m_tris, m_triCount*3*sizeof(int));
		delete [] m_tris;
		m_tris = nv;
	}
	int* dst = &m_tris[m_triCount*3];
	*dst++ = a;
	*dst++ = b;
	*dst++ = c;
	m_triCount++;
	//obj->triangle(a, b, c);
}

static char* parseRow(char* buf, char* bufEnd, char* row, int len)
{
	bool cont = false;
	bool start = true;
	bool done = false;
	int n = 0;
	while (!done && buf < bufEnd)
	{
		char c = *buf;
		buf++;
		// multirow
		switch (c)
		{
		case '\\':
			cont = true; // multirow
			break;
		case '\n':
			if (start) break;
			done = true;
			break;
		case '\r':
			break;
		case '\t':
		case ' ':
			if (start) break;
		default:
			start = false;
			cont = false;
			row[n++] = c;
			if (n >= len-1)
				done = true;
			break;
		}
	}
	row[n] = '\0';
	return buf;
}

static int parseFace(char* row, int* data, int n, int vcnt)
{
	int j = 0;
	while (*row != '\0')
	{
		// Skip initial white space
		while (*row != '\0' && (*row == ' ' || *row == '\t'))
			row++;
		char* s = row;
		// Find vertex delimiter and terminated the string there for conversion.
		while (*row != '\0' && *row != ' ' && *row != '\t')
		{
			if (*row == '/') *row = '\0';
			row++;
		}
		if (*s == '\0')
			continue;
		int vi = atoi(s);
		data[j++] = vi < 0 ? vi+vcnt : vi-1;
		if (j >= n) return j;
	}
	return j;
}

// PARTS OF THE FOLLOWING METHOD WERE TAKEN FROM AN OGRE3D FORUM POST ABOUT RECAST
bool rcMeshLoaderObj::load(Ogre::StringVector entNames, Ogre::StringVector fileNames)
{
	// check to make sure we have the same amount of filenames and entity names
	if(entNames.size() != fileNames.size())
		return false;

	m_entNames = entNames;

	// number of entities we need to load
	numEnt = fileNames.size();
	float offsetX = 0.0f;
	float offsetZ = 0.0f;
	
	for(int i = 0; i < numEnt; ++i)
	{
		Ogre::Entity* ent = SharedData::getSingleton().iSceneMgr->createEntity(entNames[i], fileNames[i]);
		Ogre::SceneNode* lvlNode = SharedData::getSingleton().iSceneMgr->getRootSceneNode()->createChildSceneNode(entNames[i]);
 		lvlNode->attachObject(ent);
		lvlNode->setPosition(offsetX, 0, offsetZ);
		SharedData::getSingleton().mNavNodeList.push_back(lvlNode);
		offsetX += 70.0f;
		offsetZ += 70.0f;
	}

		//get all vertices and triangles
		// mesh data to retrieve
		const int numNodes = SharedData::getSingleton().mNavNodeList.size();
		size_t *meshVertexCount = new size_t[numNodes];
		size_t *meshIndexCount = new size_t[numNodes];
		Ogre::Vector3 **meshVertices = new Ogre::Vector3*[numNodes];
		unsigned long **meshIndices = new unsigned long*[numNodes]; 



		for (int i = 0 ; i < numNodes ; i++)
		{
			Ogre::Entity *ent = (Ogre::Entity*)SharedData::getSingleton().mNavNodeList[i]->getAttachedObject(0);
			TemplateUtils::getMeshInformation(ent->getMesh(), meshVertexCount[i], meshVertices[i], meshIndexCount[i], meshIndices[i]);

			//total number of verts
			nverts += meshVertexCount[i];
			//total number of indices
			ntris += meshIndexCount[i];
		}


		verts = new float[nverts*3];// *3 as verts holds x,y,&z for each verts in the array
		tris = new int[ntris];// tris in recast is really indices like ogre

		//convert index count into tri count
		ntris = ntris/3; //although the tris array are indices the ntris is actual number of triangles, eg. indices/3;

		//set the reference node
		Ogre::SceneNode *referenceNode;
			referenceNode = SharedData::getSingleton().iSceneMgr->getRootSceneNode();

		//copy all meshes verticies into single buffer and transform to world space relative to parentNode
		int vertsIndex = 0;
		int prevVerticiesCount = 0;
		int prevIndexCountTotal = 0;
		for (uint i = 0 ; i < SharedData::getSingleton().mNavNodeList.size() ; i++)
		{
			//find the transform between the reference node and this node
			Ogre::Matrix4 transform = referenceNode->_getFullTransform().inverse() * SharedData::getSingleton().mNavNodeList[i]->_getFullTransform();
			Ogre::Vector3 vertexPos;
			for (uint j = 0 ; j < meshVertexCount[i] ; j++)
			{
				vertexPos = transform*meshVertices[i][j];
				verts[vertsIndex] = vertexPos.x;
				verts[vertsIndex+1] = vertexPos.y;
				verts[vertsIndex+2] = vertexPos.z;
				vertsIndex+=3;
			}

			for (uint j = 0 ; j < meshIndexCount[i] ; j++)
			{
				tris[prevIndexCountTotal+j] = meshIndices[i][j]+prevVerticiesCount;
			}
			prevIndexCountTotal += meshIndexCount[i];
			prevVerticiesCount = meshVertexCount[i];
		}
		//delete tempory arrays 
		//TODO These probably could member varibles, this would increase performance slightly
		for (int i = 0 ; i < numNodes ; i++)
		{
			delete [] meshVertices[i];
			delete [] meshIndices[i];
		}
		delete [] meshVertices;
		delete [] meshVertexCount;
		delete [] meshIndices;
		delete [] meshIndexCount;


		// calculate normals data for Recast - im not 100% sure where this is required
		// but it is used, Ogre handles its own Normal data for rendering, this is not related
		// to Ogre at all ( its also not correct lol )
		// TODO : fix this
	 	m_normals = new float[ntris*3];
	 	for (int i = 0; i < ntris*3; i += 3)
	 	{
	 		const float* v0 = &verts[tris[i]*3];
	 		const float* v1 = &verts[tris[i+1]*3];
	 		const float* v2 = &verts[tris[i+2]*3];
	 		float e0[3], e1[3];
	 		for (int j = 0; j < 3; ++j)
	 		{
	 			e0[j] = (v1[j] - v0[j]);
	 			e1[j] = (v2[j] - v0[j]);
	 		}
	 		float* n = &m_normals[i];
	  		n[0] = ((e0[1]*e1[2]) - (e0[2]*e1[1]));
	  		n[1] = ((e0[2]*e1[0]) - (e0[0]*e1[2]));
	  		n[2] = ((e0[0]*e1[1]) - (e0[1]*e1[0]));
	 		
	 		float d = sqrtf(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
	 		if (d > 0)
	 		{
	 			d = 1.0f/d;
	 			n[0] *= d;
	 			n[1] *= d;
	 			n[2] *= d;
	 		}	
	 	}


	return true;
}

//-------------------------------------------------------------------------------
// PARTS OF THE FOLLOWING CODE WERE TAKEN AND MODIFIED FROM AN OGRE3D FORUM POST
bool rcMeshLoaderObj::load()
{
	
	setupContent();

	const int numNodes = SharedData::getSingleton().mNavNodeList.size();
	const int totalMeshes = numNodes + mPagesTotal;

	nverts = 0;
	ntris = 0;
	size_t *meshVertexCount = new size_t[totalMeshes];
	size_t *meshIndexCount = new size_t[totalMeshes];
	Ogre::Vector3 **meshVertices = new Ogre::Vector3*[totalMeshes];
	unsigned long **meshIndices = new unsigned long*[totalMeshes]; 

	//---------------------------------------------------------------------------------
	// TERRAIN DATA BUILDING
	TerrainGroup::TerrainIterator ti = mTerrainGroup->getTerrainIterator();
	Terrain* trn;
	size_t trnCount = 0;
	while(ti.hasMoreElements())
	{
		 trn = ti.getNext()->instance;

	// get height data, world size, map size
	float *mapptr = trn->getHeightData();
	float WorldSize = trn->getWorldSize();
	int MapSize = trn->getSize();
	// calculate where we need to move/place our vertices
	float DeltaPos = (WorldSize / 2.0f); 

	float DeltaX = 0;
	float DeltaZ = 0;
	switch(trnCount)
	{
	case 0:
		DeltaX = -3000;
		DeltaZ = 3000;
		break;
	case 1:
		DeltaX = -3000;
		DeltaZ = -3000;
		break;
	case 2:
		DeltaX = 3000;
		DeltaZ = 3000;
		break;
	case 3:
		DeltaX = 3000;
		DeltaZ = -3000;
		break;
	default:
		DeltaX = 0;
		DeltaZ = 0;
	}

	
	float Scale = WorldSize / (float)(MapSize - 1);

	//////////////////////////////
	// THIS CODE WAS TAKEN FROM
	// AN OGRE FORUMS THREAD IN THE
	// NEW TERRAIN SCREENSHOTS THREAD
	// IN THE SHOWCASE FORUMS - I ONLY MODIFIED IT
	// TO BE ABLE TO WORK FOR RECAST AND IN THE CONTEXT OF 
	// THIS DEMO APPLICATION

	// build vertices
	meshVertices[trnCount] = new Ogre::Vector3[(MapSize*MapSize)];

	int i = 0;
	int u = 0;
	int max = MapSize; // i think i needed this for something before but now it's obviously redundant
	int z = 0;
	for(int x = 0;; ++x)
	{
		// if we've reached the right edge, start over on the next line
		if(x == max)
		{
			x = 0;
			++z;
		}
		// if we reached the bottom/end, we're done
		if(z == max)
			break;

		// add the vertex to the buffer
		meshVertices[trnCount][u] = Ogre::Vector3((Scale * x) + DeltaX, mapptr[(MapSize * z) + x], (Scale * -z) + DeltaZ);		

		i += 3;
		++u;
	}


	size_t size = ((MapSize*MapSize)-(MapSize*2)) * 6;
	meshIndices[trnCount] = new unsigned long[size];
	// i will point to the 'indices' index to insert at, x points to the vertex # to use
	i = 0;
	for(int x = 0;;++x)
	{
		// skip rightmost vertices
		if((x+1)%MapSize == 0)
		{
			++x;
		}

		// make a square of 2 triangles
		meshIndices[trnCount][i] = x;
		meshIndices[trnCount][i+1] = x + 1;
		meshIndices[trnCount][i+2] = x + MapSize; 

		meshIndices[trnCount][i+3] = x + 1;
		meshIndices[trnCount][i+4] = x + 1 + MapSize;
		meshIndices[trnCount][i+5] = x + MapSize;

		// if we just did the final square, we're done
		if(x+1+MapSize == (MapSize*MapSize)-1)
			break;

		i += 6;
	}
	
	meshVertexCount[trnCount] = trn->getSize()*trn->getSize();
	meshIndexCount[trnCount] = size;

	nverts += meshVertexCount[trnCount];
	ntris += meshIndexCount[trnCount];

	if(trnCount < mPagesTotal)
		++trnCount;
	}

	//-----------------------------------------------------------------------------------------
	// ENTITY DATA BUILDING

	for (uint i = 0 ; i < numNodes ; i++)
	{
		int ind = mPagesTotal + i;
		Ogre::Entity *ent = (Ogre::Entity*)SharedData::getSingleton().mNavNodeList[i]->getAttachedObject(0);
		TemplateUtils::getMeshInformation(ent->getMesh(), meshVertexCount[ind], meshVertices[ind], meshIndexCount[ind], meshIndices[ind]);

		//total number of verts
		nverts += meshVertexCount[ind];
		//total number of indices
		ntris += meshIndexCount[ind];
	}


	//-----------------------------------------------------------------------------------------
	// DECLARE RECAST DATA BUFFERS USING THE INFO WE GRABBED ABOVE

	verts = new float[nverts*3];// *3 as verts holds x,y,&z for each verts in the array
	tris = new int[ntris];// tris in recast is really indices like ogre

	//convert index count into tri count
	ntris = ntris/3; //although the tris array are indices the ntris is actual number of triangles, eg. indices/3;


	//-----------------------------------------------------------------------------------------
	// RECAST TERRAIN DATA BUILDING

	//set the reference node
	Ogre::SceneNode *referenceNode;
	referenceNode = SharedData::getSingleton().iSceneMgr->getRootSceneNode();

	//copy all meshes verticies into single buffer and transform to world space relative to parentNode
	int vertsIndex = 0;
	int prevVerticiesCount = 0;
	int prevIndexCountTotal = 0;
	
	for (uint i = 0 ; i < mPagesTotal ; ++i)
	{
		//find the transform between the reference node and this node
		//Ogre::Matrix4 transform = referenceNode->_getFullTransform().inverse(); // dont need to transform terrain verts, already in world space
		Ogre::Vector3 vertexPos;
		for (uint j = 0 ; j < meshVertexCount[i] ; ++j)
		{
			vertexPos = meshVertices[i][j];
			verts[vertsIndex] = vertexPos.x;
			verts[vertsIndex+1] = vertexPos.y;
			verts[vertsIndex+2] = vertexPos.z;
			vertsIndex+=3;
		}

		for (uint j = 0 ; j < meshIndexCount[i] ; j++)
		{
			tris[prevIndexCountTotal+j] = meshIndices[i][j]+prevVerticiesCount;
		}
		prevIndexCountTotal += meshIndexCount[i];
		prevVerticiesCount += meshVertexCount[i];

	}

	//-----------------------------------------------------------------------------------------
	// RECAST TERRAIN ENTITY DATA BUILDING

	//copy all meshes verticies into single buffer and transform to world space relative to parentNode
	// DO NOT RESET THESE VALUES
	// we need to keep the vert/index offset we have from the terrain generation above to make sure
	// we start referencing recast's buffers from the correct place, otherwise we just end up
	// overwriting our terrain data, which really is a pain ;)

	// int vertsIndex = 0;
	// int prevVerticiesCount = 0;
	// int prevIndexCountTotal = 0;

	for (uint i = 0 ; i < SharedData::getSingleton().mNavNodeList.size() ; i++)
	{
		int ind = mPagesTotal + i;
		//find the transform between the reference node and this node
		Ogre::Matrix4 transform = referenceNode->_getFullTransform().inverse() * SharedData::getSingleton().mNavNodeList[i]->_getFullTransform();
		Ogre::Vector3 vertexPos;
		for (uint j = 0 ; j < meshVertexCount[ind] ; j++)
		{
			vertexPos = transform * meshVertices[ind][j];
			verts[vertsIndex] = vertexPos.x;
			verts[vertsIndex+1] = vertexPos.y;
			verts[vertsIndex+2] = vertexPos.z;
			vertsIndex+=3;
		}

		for (uint j = 0 ; j < meshIndexCount[ind] ; j++)
		{
			tris[prevIndexCountTotal+j] = meshIndices[ind][j]+prevVerticiesCount;
		}
		prevIndexCountTotal += meshIndexCount[ind];
		prevVerticiesCount += meshVertexCount[ind];
	}


	//delete tempory arrays 
	//TODO These probably could member varibles, this would increase performance slightly
	for(uint i = 0; i < totalMeshes; ++i)
	{
		delete [] meshVertices[i];
		
	}
	// first 4 were created differently, without getMeshInformation();
	// throws an exception if we delete the first 4
	// TODO - FIX THIS MEMORY LEAK - its only small, but its still not good
	for(uint i  = mPagesTotal; i < totalMeshes; ++i)
	{
		delete [] meshIndices[i];
	}
	
	delete [] meshVertices;
	delete [] meshVertexCount;
	delete [] meshIndices;
	delete [] meshIndexCount;



	//---------------------------------------------------------------------------------------------
	// RECAST **ONLY** NORMAL CALCS ( These are not used anywhere other than internally by recast)

	m_normals = new float[ntris*3];
	for (int i = 0; i < ntris*3; i += 3)
	{
		const float* v0 = &verts[tris[i]*3];
		const float* v1 = &verts[tris[i+1]*3];
		const float* v2 = &verts[tris[i+2]*3];
		float e0[3], e1[3];
		for (int j = 0; j < 3; ++j)
		{
			e0[j] = (v1[j] - v0[j]);
			e1[j] = (v2[j] - v0[j]);
		}
		float* n = &m_normals[i];
		n[0] = ((e0[1]*e1[2]) - (e0[2]*e1[1]));
		n[1] = ((e0[2]*e1[0]) - (e0[0]*e1[2]));
		n[2] = ((e0[0]*e1[1]) - (e0[1]*e1[0]));

		float d = sqrtf(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
		if (d > 0)
		{
			d = 1.0f/d;
			n[0] *= d;
			n[1] *= d;
			n[2] *= d;
		}	
	}

	return true;
}

void rcMeshLoaderObj::saveTerrains(bool onlyIfModified)
{
	mTerrainGroup->saveAllTerrains(onlyIfModified);
}

void rcMeshLoaderObj::defineTerrain(long x, long y, bool flat/* = false */)
{
		// if a file is available, use it
		// if not, generate file from import

		// Usually in a real project you'll know whether the compact terrain data is
		// available or not; I'm doing it this way to save distribution size

		if (flat)
		{
			mTerrainGroup->defineTerrain(x, y, 0.0f);
		}
		else
		{
			String filename = mTerrainGroup->generateFilename(x, y);
			if (ResourceGroupManager::getSingleton().resourceExists(mTerrainGroup->getResourceGroup(), filename))
			{
				mTerrainGroup->defineTerrain(x, y);
			}
			else
			{
				Image img;
				getTerrainImage(x, y, x % 2 != 0, y % 2 != 0, img);
				//getTerrainImage(x, y, false, false, img);
				mTerrainGroup->defineTerrain(x, y, &img);
				mTerrainsImported = true;
			}

		}
}


void rcMeshLoaderObj::getTerrainImage(long x, long y, bool flipX, bool flipY, Image& img)
{
	Ogre::String baseName = "gcanyon_height";
	Ogre::String extName = ".png";
	long xx = (x / -1);
	Ogre::String xname = Ogre::StringConverter::toString(xx);
	Ogre::String yname = Ogre::StringConverter::toString(y);

	Ogre::String loadName = baseName + "." + xname + "." + yname + extName;
	Ogre::String saveName = "../../media/heightmaps/" + baseName + "." + xname + "." + yname + "_flipped" + extName;

	
	img.load(loadName, ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
		
		if (flipX)
		{
			img.flipAroundY();
		}
		if (flipY)
		{
			img.flipAroundX();
		}
}

void rcMeshLoaderObj::initBlendMaps(Terrain* terrain)
{
		TerrainLayerBlendMap* blendMap0 = terrain->getLayerBlendMap(1);
		TerrainLayerBlendMap* blendMap1 = terrain->getLayerBlendMap(2);
		Real minHeight0 = 70;
		Real fadeDist0 = 40;
		Real minHeight1 = 70;
		Real fadeDist1 = 15;
		float* pBlend1 = blendMap1->getBlendPointer();
		for (Ogre::uint16 y = 0; y < terrain->getLayerBlendMapSize(); ++y)
		{
			for (Ogre::uint16 x = 0; x < terrain->getLayerBlendMapSize(); ++x)
			{
				Real tx, ty;

				blendMap0->convertImageToTerrainSpace(x, y, &tx, &ty);
				Real height = terrain->getHeightAtTerrainPosition(tx, ty);
				Real val = (height - minHeight0) / fadeDist0;
				val = Math::Clamp(val, (Real)0, (Real)1);
				//*pBlend0++ = val;

				val = (height - minHeight1) / fadeDist1;
				val = Math::Clamp(val, (Real)0, (Real)1);
				*pBlend1++ = val;


			}
		}
		blendMap0->dirty();
		blendMap1->dirty();
		//blendMap0->loadImage("blendmap1.png", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		blendMap0->update();
		blendMap1->update();

		// set up a colour map
		/*
		if (!terrain->getGlobalColourMapEnabled())
		{
			terrain->setGlobalColourMapEnabled(true);
			Image colourMap;
			colourMap.load("testcolourmap.jpg", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
			terrain->getGlobalColourMap()->loadImage(colourMap);
		}
		*/

}

void rcMeshLoaderObj::configureTerrainDefaults(Light* l)
{
		// Configure global
		mTerrainGlobals->setMaxPixelError(3);
		// testing composite map
		mTerrainGlobals->setCompositeMapDistance(5000);
		//mTerrainGlobals->setUseRayBoxDistanceCalculation(true);
		//mTerrainGlobals->getDefaultMaterialGenerator()->setDebugLevel(1);
		mTerrainGlobals->setLightMapSize(2048);
		mTerrainGlobals->setLayerBlendMapSize(2048);

		//matProfile->setLightmapEnabled(false);
		// Important to set these so that the terrain knows what to use for derived (non-realtime) data
		mTerrainGlobals->setLightMapDirection(l->getDerivedDirection());
		mTerrainGlobals->setCompositeMapAmbient(mSceneMgr->getAmbientLight());
		//mTerrainGlobals->setCompositeMapAmbient(ColourValue::Red);
		mTerrainGlobals->setCompositeMapDiffuse(l->getDiffuseColour());

		// Configure default import settings for if we use imported image
		Terrain::ImportData& defaultimp = mTerrainGroup->getDefaultImportSettings();
		defaultimp.terrainSize = TERRAIN_SIZE;
		defaultimp.worldSize = TERRAIN_WORLD_SIZE;
		defaultimp.inputScale = 1024;
		defaultimp.minBatchSize = 33;
		defaultimp.maxBatchSize = 129;
		// textures
		mLayerCount = 5;
		defaultimp.layerList.resize(6);
		defaultimp.layerList[0].worldSize = 50;
		defaultimp.layerList[0].textureNames.push_back("dirt_grayrocky_diffusespecular.dds");
		defaultimp.layerList[0].textureNames.push_back("dirt_grayrocky_normalheight.dds");
		defaultimp.layerList[1].worldSize = 50;
		defaultimp.layerList[1].textureNames.push_back("jungle_1_diffusespecular.dds");
		defaultimp.layerList[1].textureNames.push_back("jungle_1_normalheight.dds");
		defaultimp.layerList[2].worldSize = 50;
		defaultimp.layerList[2].textureNames.push_back("jungle_2_diffusespecular.dds");
		defaultimp.layerList[2].textureNames.push_back("jungle_2_normalheight.dds");
		defaultimp.layerList[3].worldSize = 50;
		defaultimp.layerList[3].textureNames.push_back("jungle_4_diffusespecular.dds");
		defaultimp.layerList[3].textureNames.push_back("jungle_4_normalheight.dds");
		defaultimp.layerList[4].worldSize = 50;
		defaultimp.layerList[4].textureNames.push_back("jungle_5_diffusespecular.dds");
		defaultimp.layerList[4].textureNames.push_back("jungle_5_normalheight.dds");
		defaultimp.layerList[5].worldSize = 50;
		defaultimp.layerList[5].textureNames.push_back("jungle_6_diffusespecular.dds");
		defaultimp.layerList[5].textureNames.push_back("jungle_6_normalheight.dds");

}

MaterialPtr rcMeshLoaderObj::buildDepthShadowMaterial(const String& textureName)
{
		String matName = "DepthShadows/" + textureName;

		MaterialPtr ret = MaterialManager::getSingleton().getByName(matName);
		if (ret.isNull())
		{
			MaterialPtr baseMat = MaterialManager::getSingleton().getByName("Ogre/shadow/depth/integrated/pssm");
			ret = baseMat->clone(matName);
			Pass* p = ret->getTechnique(0)->getPass(0);
			p->getTextureUnitState("diffuse")->setTextureName(textureName);

			Vector4 splitPoints;
			const PSSMShadowCameraSetup::SplitPointList& splitPointList = 
				static_cast<PSSMShadowCameraSetup*>(mPSSMSetup.get())->getSplitPoints();
			for (int i = 0; i < 3; ++i)
			{
				splitPoints[i] = splitPointList[i];
			}
			p->getFragmentProgramParameters()->setNamedConstant("pssmSplitPoints", splitPoints);


		}

		return ret;
}

void rcMeshLoaderObj::changeShadows()
{
	configureShadows(mShadowMode != SHADOWS_NONE, mShadowMode == SHADOWS_DEPTH);
}

void rcMeshLoaderObj::configureShadows(bool enabled, bool depthShadows)
{
	Ogre::Camera* mCamera = SharedData::getSingleton().iCamera;

		TerrainMaterialGeneratorA::SM2Profile* matProfile = 
			static_cast<TerrainMaterialGeneratorA::SM2Profile*>(mTerrainGlobals->getDefaultMaterialGenerator()->getActiveProfile());
		matProfile->setReceiveDynamicShadowsEnabled(enabled);
#ifdef SHADOWS_IN_LOW_LOD_MATERIAL
		matProfile->setReceiveDynamicShadowsLowLod(true);
#else
		matProfile->setReceiveDynamicShadowsLowLod(false);
#endif

		// Default materials
		for (EntityList::iterator i = mHouseList.begin(); i != mHouseList.end(); ++i)
		{
			(*i)->setMaterialName("Examples/TudorHouse");
		}

		if (enabled)
		{
			// General scene setup
			mSceneMgr->setShadowTechnique(SHADOWTYPE_TEXTURE_ADDITIVE_INTEGRATED);
			mSceneMgr->setShadowFarDistance(3000);

			// 3 textures per directional light (PSSM)
			mSceneMgr->setShadowTextureCountPerLightType(Ogre::Light::LT_DIRECTIONAL, 3);

			if (mPSSMSetup.isNull())
			{
				// shadow camera setup
				PSSMShadowCameraSetup* pssmSetup = new PSSMShadowCameraSetup();
				pssmSetup->setSplitPadding(mCamera->getNearClipDistance());
				pssmSetup->calculateSplitPoints(3, mCamera->getNearClipDistance(), mSceneMgr->getShadowFarDistance());
				pssmSetup->setOptimalAdjustFactor(0, 2);
				pssmSetup->setOptimalAdjustFactor(1, 1);
				pssmSetup->setOptimalAdjustFactor(2, 0.5);

				mPSSMSetup.bind(pssmSetup);

			}
			mSceneMgr->setShadowCameraSetup(mPSSMSetup);

			// TODO : add extra house/chapel and windmill entities to this material build list
			if (depthShadows)
			{
				mSceneMgr->setShadowTextureCount(3);
				mSceneMgr->setShadowTextureConfig(0, 2048, 2048, PF_FLOAT32_R);
				mSceneMgr->setShadowTextureConfig(1, 1024, 1024, PF_FLOAT32_R);
				mSceneMgr->setShadowTextureConfig(2, 1024, 1024, PF_FLOAT32_R);
				mSceneMgr->setShadowTextureSelfShadow(true);
				mSceneMgr->setShadowCasterRenderBackFaces(true);
				mSceneMgr->setShadowTextureCasterMaterial("PSSM/shadow_caster");

				MaterialPtr houseMat = buildDepthShadowMaterial("fw12b.jpg");
				for (EntityList::iterator i = mHouseList.begin(); i != mHouseList.end(); ++i)
				{
					(*i)->setMaterial(houseMat);
				}

			}
			else
			{
				mSceneMgr->setShadowTextureCount(3);
				mSceneMgr->setShadowTextureConfig(0, 2048, 2048, PF_X8B8G8R8);
				mSceneMgr->setShadowTextureConfig(1, 1024, 1024, PF_X8B8G8R8);
				mSceneMgr->setShadowTextureConfig(2, 1024, 1024, PF_X8B8G8R8);
				mSceneMgr->setShadowTextureSelfShadow(false);
				mSceneMgr->setShadowCasterRenderBackFaces(false);
				mSceneMgr->setShadowTextureCasterMaterial(StringUtil::BLANK);
			}

			matProfile->setReceiveDynamicShadowsDepth(depthShadows);
			matProfile->setReceiveDynamicShadowsPSSM(static_cast<PSSMShadowCameraSetup*>(mPSSMSetup.get()));

			//addTextureShadowDebugOverlay(TL_RIGHT, 3);


		}
		else
		{
			mSceneMgr->setShadowTechnique(SHADOWTYPE_NONE);
		}


}

void rcMeshLoaderObj::setupView()
{
}

void rcMeshLoaderObj::setupContent()
{
		bool blankTerrain = false;

		mTerrainGlobals = OGRE_NEW TerrainGlobalOptions();

		 // setup some of the application's shared data
		SDATA.iActivate = true;
			
		mTerrainGroup = OGRE_NEW TerrainGroup(mSceneMgr, Terrain::ALIGN_X_Z, TERRAIN_SIZE, TERRAIN_WORLD_SIZE);
		mTerrainGroup->setFilenameConvention(TERRAIN_FILE_PREFIX, TERRAIN_FILE_SUFFIX);
		mTerrainGroup->setOrigin(mTerrainPos);

		configureTerrainDefaults(SharedData::getSingleton().iMainLight);

#ifdef PAGING
		// Paging setup
		mPageManager = OGRE_NEW PageManager();
		// Since we're not loading any pages from .page files, we need a way just 
		// to say we've loaded them without them actually being loaded
		mPageManager->setPageProvider(&mDummyPageProvider);
		mPageManager->addCamera(mCamera);
		mTerrainPaging = OGRE_NEW TerrainPaging(mPageManager);
		PagedWorld* world = mPageManager->createWorld();
		mTerrainPaging->createWorldSection(world, mTerrainGroup, 1024, 2048, 
			TERRAIN_PAGE_MIN_X, TERRAIN_PAGE_MIN_Y, 
			TERRAIN_PAGE_MAX_X, TERRAIN_PAGE_MAX_Y);
#else
		// sync load since we want everything in place when we start
		importFullTerrainFromHeightMap();
		mTerrainGroup->loadAllTerrains(true);
#endif

		if (mTerrainsImported)
		{
			TerrainGroup::TerrainIterator ti = mTerrainGroup->getTerrainIterator();
			while(ti.hasMoreElements())
			{
				Terrain* t = ti.getNext()->instance;			
				calculateBlendMap(t);
			}
		}
		mTerrainGroup->freeTemporaryResources();

		// this is only here as we KNOW all terrains will be loaded by this point,
		// otherwise see "frameRender" method and its loadHouses routine
		loadHouses();
		mHousesLoaded = true;
}

// moved entity loading here due to paging... when terrain is paging in, you cannot 
// use height data to place things on the terrain, it just returns 0, so we wait
// until the terrain loads(checked in this class's frameRender method)
void rcMeshLoaderObj::loadHouses()
{
	if(mTerrainGroup->getHeightAtWorldPosition(Vector3(2043, 50, 1715)) <= 0.0f)
	{
		mHousesLoaded = false;
		return;
	}
	// create a few entities on the terrain
		Entity* e = mSceneMgr->createEntity("tudorhouse.mesh");
		Vector3 entPos(mTerrainPos.x + 2043, 0, mTerrainPos.z + 1715);
		Quaternion rot;
		entPos.y = mTerrainGroup->getHeightAtWorldPosition(entPos) + 60.5 + mTerrainPos.y;
		rot.FromAngleAxis(Degree(Math::RangeRandom(-180, 180)), Vector3::UNIT_Y);
		SceneNode* sn = mSceneMgr->getRootSceneNode()->createChildSceneNode(entPos, rot);
		sn->setScale(Vector3(0.12, 0.12, 0.12));
		sn->attachObject(e);
		mHouseList.push_back(e);
		SharedData::getSingleton().mNavNodeList.push_back(sn);

		e = mSceneMgr->createEntity("tudorhouse.mesh");
		entPos = Vector3(mTerrainPos.x + 1850, 0, mTerrainPos.z + 1478);
		entPos.y = mTerrainGroup->getHeightAtWorldPosition(entPos) + 60.5 + mTerrainPos.y;
		rot.FromAngleAxis(Degree(Math::RangeRandom(-180, 180)), Vector3::UNIT_Y);
		sn = mSceneMgr->getRootSceneNode()->createChildSceneNode(entPos, rot);
		sn->setScale(Vector3(0.12, 0.12, 0.12));
		sn->attachObject(e);
		mHouseList.push_back(e);
		SharedData::getSingleton().mNavNodeList.push_back(sn);

		e = mSceneMgr->createEntity("tudorhouse.mesh");
		entPos = Vector3(mTerrainPos.x + 1970, 0, mTerrainPos.z + 2180);
		entPos.y = mTerrainGroup->getHeightAtWorldPosition(entPos) + 60.5 + mTerrainPos.y;
		rot.FromAngleAxis(Degree(Math::RangeRandom(-180, 180)), Vector3::UNIT_Y);
		sn = mSceneMgr->getRootSceneNode()->createChildSceneNode(entPos, rot);
		sn->setScale(Vector3(0.12, 0.12, 0.12));
		sn->attachObject(e);
		mHouseList.push_back(e);
		SharedData::getSingleton().mNavNodeList.push_back(sn);

		////////////////////////////////////////////////////////////////////
		e = mSceneMgr->createEntity("HouseMesh.mesh");
		entPos = Vector3(mTerrainPos.x + 970, 0, mTerrainPos.z + 580);
		entPos.y = mTerrainGroup->getHeightAtWorldPosition(entPos) + mTerrainPos.y - 2.0f;
		rot.FromAngleAxis(Degree(Math::RangeRandom(-180, 180)), Vector3::UNIT_Y);
		sn = mSceneMgr->getRootSceneNode()->createChildSceneNode(entPos, rot);
		sn->setScale(Vector3(0.12, 0.12, 0.12));
		sn->attachObject(e);
		mHouseList.push_back(e);
		SharedData::getSingleton().mNavNodeList.push_back(sn);

		e = mSceneMgr->createEntity("HouseMesh.mesh");
		entPos = Vector3(mTerrainPos.x + 4070, 0, mTerrainPos.z + -280);
		entPos.y = mTerrainGroup->getHeightAtWorldPosition(entPos) + mTerrainPos.y - 2.0f;
		rot.FromAngleAxis(Degree(Math::RangeRandom(-180, 180)), Vector3::UNIT_Y);
		sn = mSceneMgr->getRootSceneNode()->createChildSceneNode(entPos, rot);
		sn->setScale(Vector3(0.12, 0.12, 0.12));
		sn->attachObject(e);
		mHouseList.push_back(e);
		SharedData::getSingleton().mNavNodeList.push_back(sn);

		e = mSceneMgr->createEntity("HouseMesh.mesh");
		entPos = Vector3(mTerrainPos.x + 4270, 0, mTerrainPos.z + -480);
		entPos.y = mTerrainGroup->getHeightAtWorldPosition(entPos) + mTerrainPos.y - 2.0f;
		rot.FromAngleAxis(Degree(Math::RangeRandom(-180, 180)), Vector3::UNIT_Y);
		sn = mSceneMgr->getRootSceneNode()->createChildSceneNode(entPos, rot);
		sn->setScale(Vector3(0.12, 0.12, 0.12));
		sn->attachObject(e);
		mHouseList.push_back(e);
		SharedData::getSingleton().mNavNodeList.push_back(sn);

		e = mSceneMgr->createEntity("HouseMesh.mesh");
		entPos = Vector3(mTerrainPos.x + 4470, 0, mTerrainPos.z + -680);
		entPos.y = mTerrainGroup->getHeightAtWorldPosition(entPos) + mTerrainPos.y - 2.0f;
		rot.FromAngleAxis(Degree(Math::RangeRandom(-180, 180)), Vector3::UNIT_Y);
		sn = mSceneMgr->getRootSceneNode()->createChildSceneNode(entPos, rot);
		sn->setScale(Vector3(0.12, 0.12, 0.12));
		sn->attachObject(e);
		mHouseList.push_back(e);
		SharedData::getSingleton().mNavNodeList.push_back(sn);

		e = mSceneMgr->createEntity("HouseMesh.mesh");
		entPos = Vector3(mTerrainPos.x + 2470, 0, mTerrainPos.z + -2500);
		entPos.y = mTerrainGroup->getHeightAtWorldPosition(entPos) + mTerrainPos.y - 2.0f;
		rot.FromAngleAxis(Degree(Math::RangeRandom(-180, 180)), Vector3::UNIT_Y);
		sn = mSceneMgr->getRootSceneNode()->createChildSceneNode(entPos, rot);
		sn->setScale(Vector3(0.12, 0.12, 0.12));
		sn->attachObject(e);
		mHouseList.push_back(e);
		SharedData::getSingleton().mNavNodeList.push_back(sn);

		e = mSceneMgr->createEntity("HouseMesh.mesh");
		entPos = Vector3(mTerrainPos.x + 2370, 0, mTerrainPos.z + -2200);
		entPos.y = mTerrainGroup->getHeightAtWorldPosition(entPos) + mTerrainPos.y - 2.0f;
		rot.FromAngleAxis(Degree(Math::RangeRandom(-180, 180)), Vector3::UNIT_Y);
		sn = mSceneMgr->getRootSceneNode()->createChildSceneNode(entPos, rot);
		sn->setScale(Vector3(0.12, 0.12, 0.12));
		sn->attachObject(e);
		mHouseList.push_back(e);
		SharedData::getSingleton().mNavNodeList.push_back(sn);

		e = mSceneMgr->createEntity("HouseMesh.mesh");
		entPos = Vector3(mTerrainPos.x + 2170, 0, mTerrainPos.z + -2000);
		entPos.y = mTerrainGroup->getHeightAtWorldPosition(entPos) + mTerrainPos.y - 2.0f;
		rot.FromAngleAxis(Degree(Math::RangeRandom(-180, 180)), Vector3::UNIT_Y);
		sn = mSceneMgr->getRootSceneNode()->createChildSceneNode(entPos, rot);
		sn->setScale(Vector3(0.12, 0.12, 0.12));
		sn->attachObject(e);
		mHouseList.push_back(e);
		SharedData::getSingleton().mNavNodeList.push_back(sn);

		e = mSceneMgr->createEntity("ChapelMesh.mesh");
		entPos = Vector3(mTerrainPos.x + 2450, 0, mTerrainPos.z + -750);
		entPos.y = mTerrainGroup->getHeightAtWorldPosition(entPos) + mTerrainPos.y - 2.0f;
		rot.FromAngleAxis(Degree(Math::RangeRandom(-180, 180)), Vector3::UNIT_Y);
		sn = mSceneMgr->getRootSceneNode()->createChildSceneNode(entPos, rot);
		sn->setScale(Vector3(0.12, 0.12, 0.12));
		sn->attachObject(e);
		mHouseList.push_back(e);
		SharedData::getSingleton().mNavNodeList.push_back(sn);

		e = mSceneMgr->createEntity("ChapelMesh.mesh");
		entPos = Vector3(mTerrainPos.x + 1650, 0, mTerrainPos.z + -1150);
		entPos.y = mTerrainGroup->getHeightAtWorldPosition(entPos) + mTerrainPos.y - 2.0f;
		rot.FromAngleAxis(Degree(Math::RangeRandom(-180, 180)), Vector3::UNIT_Y);
		sn = mSceneMgr->getRootSceneNode()->createChildSceneNode(entPos, rot);
		sn->setScale(Vector3(0.12, 0.12, 0.12));
		sn->attachObject(e);
		mHouseList.push_back(e);
		SharedData::getSingleton().mNavNodeList.push_back(sn);

		e = mSceneMgr->createEntity("WindmillMesh.mesh");
		entPos = Vector3(mTerrainPos.x + 1850, 0, mTerrainPos.z + -1350);
		entPos.y = mTerrainGroup->getHeightAtWorldPosition(entPos) + mTerrainPos.y - 2.0f;
		rot.FromAngleAxis(Degree(Math::RangeRandom(-180, 180)), Vector3::UNIT_Y);
		sn = mSceneMgr->getRootSceneNode()->createChildSceneNode(entPos, rot);
		sn->setScale(Vector3(0.12, 0.12, 0.12));
		sn->attachObject(e);
		mHouseList.push_back(e);
		SharedData::getSingleton().mNavNodeList.push_back(sn);

		e = mSceneMgr->createEntity("WindmillMesh.mesh");
		entPos = Vector3(mTerrainPos.x + 3450, 0, mTerrainPos.z + -950);
		entPos.y = mTerrainGroup->getHeightAtWorldPosition(entPos) + mTerrainPos.y - 2.0f;
		rot.FromAngleAxis(Degree(Math::RangeRandom(-180, 180)), Vector3::UNIT_Y);
		sn = mSceneMgr->getRootSceneNode()->createChildSceneNode(entPos, rot);
		sn->setScale(Vector3(0.12, 0.12, 0.12));
		sn->attachObject(e);
		mHouseList.push_back(e);
		SharedData::getSingleton().mNavNodeList.push_back(sn);

		e = mSceneMgr->createEntity("WindmillMesh.mesh");
		entPos = Vector3(mTerrainPos.x + 2250, 0, mTerrainPos.z + -1550);
		entPos.y = mTerrainGroup->getHeightAtWorldPosition(entPos) + mTerrainPos.y - 2.0f;
		rot.FromAngleAxis(Degree(Math::RangeRandom(-180, 180)), Vector3::UNIT_Y);
		sn = mSceneMgr->getRootSceneNode()->createChildSceneNode(entPos, rot);
		sn->setScale(Vector3(0.12, 0.12, 0.12));
		sn->attachObject(e);
		mHouseList.push_back(e);
		SharedData::getSingleton().mNavNodeList.push_back(sn);


}


void rcMeshLoaderObj::importFullTerrainFromHeightMap()
{
	mTerrainsImported = true;

	int mMapSize = 513;
	Ogre::Real fScale = 1.0f;
    Ogre::Real fBias = 0.0f;

    float *data = 0;

	Ogre::String filename = TERRAIN_HEIGHTMAP_FULLPATH_NAME;
    Ogre::String namePart = TemplateUtils::ExtractFileName(filename);
    namePart.erase(0, namePart.find("."));

    int imgW = 0;
    int imgH = 0;

	// TODO : add try {} catch {} to make sure we dont throw exception on bad image data
    if(namePart == ".png")
    {
        std::fstream fstr(filename.c_str(), std::ios::in|std::ios::binary);
        Ogre::DataStreamPtr stream = Ogre::DataStreamPtr(OGRE_NEW Ogre::FileStreamDataStream(&fstr, false));
			
        Ogre::Image img;
		img.load(stream);
	
        data = OGRE_ALLOC_T(float, img.getWidth() * img.getHeight(), Ogre::MEMCATEGORY_GEOMETRY);
        Ogre::PixelBox pb(img.getWidth(), img.getHeight(), 1, Ogre::PF_FLOAT32_R, data);
        Ogre::PixelUtil::bulkPixelConversion(img.getPixelBox(), pb);
	
        imgW = img.getWidth();
        imgH = img.getHeight();
        
        img.freeMemory();
        stream.setNull();
    }
	else
	{
		return;
	}

	int msize = mMapSize - 1;
	int XCount = (imgW - 1) / msize;
	int YCount = (imgH - 1) / msize;

	mPagesTotal = (XCount * YCount);
	
	dataptr = OGRE_ALLOC_T(float, mMapSize * mMapSize, Ogre::MEMCATEGORY_GEOMETRY);

	for(int y = 0;y < YCount;y++)
	{
		for(int x = 0;x < XCount;x++)
		{
			Ogre::Vector3 position = Ogre::Vector3::ZERO;
			mTerrainGroup->convertTerrainSlotToWorldPosition(x - (XCount / 2), y - (YCount / 2), &position);

			float cval;

			for(int iy = 0;iy <= msize;iy++)
			{
				for(int ix = 0;ix <= msize;ix++)
				{
					cval = data[(((y * msize) + iy) * imgW) + (x * msize) + ix];
					dataptr[(iy * (msize + 1)) + ix] = fBias + (cval * fScale);					
				}
			}

			String filename = mTerrainGroup->generateFilename(x, y);
			if (ResourceGroupManager::getSingleton().resourceExists(mTerrainGroup->getResourceGroup(), filename))
			{
				mTerrainGroup->defineTerrain(x, y);
				mTerrainsImported = false;
			}
			else
			{
					mTerrainGroup->defineTerrain(x, y, dataptr);
			}
		}
	}
	OGRE_FREE(data, Ogre::MEMCATEGORY_GEOMETRY);
	OGRE_FREE(dataptr, Ogre::MEMCATEGORY_GEOMETRY);
}


bool rcMeshLoaderObj::keyPressed( const OIS::KeyEvent &arg )
{
	if (arg.key == OIS::KC_0)   // toggle scenenode debug renderables
	{
		mSceneMgr->setDisplaySceneNodes(!mSceneMgr->getDisplaySceneNodes());
	}
	
	switch (arg.key)
		{
		case OIS::KC_S:
			// CTRL-S to save
			if (SharedData::getSingleton().iKeyboard->isKeyDown(OIS::KC_LCONTROL) || 
				SharedData::getSingleton().iKeyboard->isKeyDown(OIS::KC_RCONTROL))
			{
				//saveTerrains(true); // save only if modified since last save
				saveTerrains(false); // save ALL terrain not matter what
			}
			else
				return true;
			break;
		case OIS::KC_F10:
			// dump
			{
				TerrainGroup::TerrainIterator ti = mTerrainGroup->getTerrainIterator();
				while (ti.hasMoreElements())
				{
					Ogre::uint32 tkey = ti.peekNextKey();
					TerrainGroup::TerrainSlot* ts = ti.getNext();
					if (ts->instance && ts->instance->isLoaded())
					{
						ts->instance->_dumpTextures("terrain_" + StringConverter::toString(tkey), ".png");
					}
				}
			}
			break;
			/*
		case OIS::KC_F7:
			// change terrain size
			if (mTerrainGroup->getTerrainSize() == 513)
				mTerrainGroup->setTerrainSize(1025);
			else
				mTerrainGroup->setTerrainSize(513);
			break;
		case OIS::KC_F8:
			// change terrain world size
			if (mTerrainGroup->getTerrainWorldSize() == TERRAIN_WORLD_SIZE)
				mTerrainGroup->setTerrainWorldSize(TERRAIN_WORLD_SIZE * 2);
			else
				mTerrainGroup->setTerrainWorldSize(TERRAIN_WORLD_SIZE);
			break;
			*/
		default:
			return true;
		}

	return true;
}

bool rcMeshLoaderObj::frameRender(const Ogre::FrameEvent& evt)
{
	if (mHeightUpdateCountDown > 0)
		{
			mHeightUpdateCountDown -= evt.timeSinceLastFrame;
			if (mHeightUpdateCountDown <= 0)
			{
				mTerrainGroup->update();
				mHeightUpdateCountDown = 0;

			}

		}

		if (mTerrainGroup->isDerivedDataUpdateInProgress())
		{
			/*if (mTerrainsImported)
			{
				mInfoLabel->setCaption("Building terrain, please wait...");
			}
			else
			{
				mInfoLabel->setCaption("Updating textures, patience...");
			}*/

		}
		else
		{
			if(!mHousesLoaded)
			{
				mHousesLoaded = true;
				loadHouses();
			}
			if (mTerrainsImported)
			{
				saveTerrains(true);
				mTerrainsImported = false;
			}
		}

		return true;
}





/*/////////////////////////////////////////////////////////////////////////////////
/// An
///    ___   ____ ___ _____ ___  ____
///   / _ \ / ___|_ _|_   _/ _ \|  _ \
///  | | | | |  _ | |  | || | | | |_) |
///  | |_| | |_| || |  | || |_| |  _ <
///   \___/ \____|___| |_| \___/|_| \_\
///                              File
///
/// Copyright (c) 2008-2010 Ismail TARIM <ismail@royalspor.com> and the Ogitor Team
//
/// The MIT License
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
/// THE SOFTWARE.
////////////////////////////////////////////////////////////////////////////////*/



// ORIGINALLY OGITOR CODE MODIFIED TO NOT HAVE QT DEPS OR A GUI AT ALL ;)
// All of the following code until the end of file have been directly used
// from Ogitor, or Modified from the original Ogitor source. But, all the following
// can be considered to be derived directly from Ogitor at the least.
//-----------------------------------------------------------------------------------------
/* EXAMPLE Values 
TYPE | HS-HE  |SS-SE|SKW / SKWAZM|HREL-SREL
--------------------------------------------
grass| 30-150 | 0-20|  0 / 90	 |	10-10
soil | 50-160 |20-60| -1 / 90	 |	10-10
rock |150-200 | 0-60| -1 / 90	 |	10-10
steep|150-400 |30-90|  0 / 90	 |	10-10
snow |200-400 | 0-60| -3 / 90	 |	10-10
*/
bool rcMeshLoaderObj::setupBlendData(Ogre::NameValuePairList &params)
{
	int layerCount = 5;

	// create layer data holding objects and place in vector
	for(uint i = 0; i < layerCount; ++i)
	{
		CalcBlendData* blendData = new CalcBlendData();
		SharedData::getSingleton().m_BlendList.push_back(blendData);
	}

	// layer1
	SharedData::getSingleton().m_BlendList[0]->hs = 15;
	SharedData::getSingleton().m_BlendList[0]->he = 150;
	SharedData::getSingleton().m_BlendList[0]->ss = 15;
	SharedData::getSingleton().m_BlendList[0]->se = 90;
	SharedData::getSingleton().m_BlendList[0]->skw = -2;
	SharedData::getSingleton().m_BlendList[0]->skwazm = 90;
	SharedData::getSingleton().m_BlendList[0]->hr = 15;
	SharedData::getSingleton().m_BlendList[0]->sr = 15;
	SharedData::getSingleton().m_BlendList[0]->layer = 1;

	// layer 2
	SharedData::getSingleton().m_BlendList[1]->hs = 30;
	SharedData::getSingleton().m_BlendList[1]->he = 375;
	SharedData::getSingleton().m_BlendList[1]->ss = 0;
	SharedData::getSingleton().m_BlendList[1]->se = 40;
	SharedData::getSingleton().m_BlendList[1]->skw = 0;
	SharedData::getSingleton().m_BlendList[1]->skwazm = 90;
	SharedData::getSingleton().m_BlendList[1]->hr = 15;
	SharedData::getSingleton().m_BlendList[1]->sr = 15;
	SharedData::getSingleton().m_BlendList[1]->layer = 2;

	// layer 3
	SharedData::getSingleton().m_BlendList[2]->hs = 100;
	SharedData::getSingleton().m_BlendList[2]->he = 450;
	SharedData::getSingleton().m_BlendList[2]->ss = 0;
	SharedData::getSingleton().m_BlendList[2]->se = 60;
	SharedData::getSingleton().m_BlendList[2]->skw = -1;
	SharedData::getSingleton().m_BlendList[2]->skwazm = 90;
	SharedData::getSingleton().m_BlendList[2]->hr = 15;
	SharedData::getSingleton().m_BlendList[2]->sr = 15;
	SharedData::getSingleton().m_BlendList[2]->layer = 3;

	// layer 4
	SharedData::getSingleton().m_BlendList[3]->hs = 350;
	SharedData::getSingleton().m_BlendList[3]->he = 650;
	SharedData::getSingleton().m_BlendList[3]->ss = 30;
	SharedData::getSingleton().m_BlendList[3]->se = 90;
	SharedData::getSingleton().m_BlendList[3]->skw = 0;
	SharedData::getSingleton().m_BlendList[3]->skwazm = 90;
	SharedData::getSingleton().m_BlendList[3]->hr = 15;
	SharedData::getSingleton().m_BlendList[3]->sr = 15;
	SharedData::getSingleton().m_BlendList[3]->layer = 4;

	// layer 5
	SharedData::getSingleton().m_BlendList[4]->hs = 450;
	SharedData::getSingleton().m_BlendList[4]->he = 700;
	SharedData::getSingleton().m_BlendList[4]->ss = 0;
	SharedData::getSingleton().m_BlendList[4]->se = 60;
	SharedData::getSingleton().m_BlendList[4]->skw = -3;
	SharedData::getSingleton().m_BlendList[4]->skwazm = 90;
	SharedData::getSingleton().m_BlendList[4]->hr = 15;
	SharedData::getSingleton().m_BlendList[4]->sr = 15;
	SharedData::getSingleton().m_BlendList[4]->layer = 5;

	// setup Ogre::NameValuePairList params as temp measure for blending data

	// LAYER 1
	int position = 1;
	
	Ogre::String keyst = Ogre::StringConverter::toString(position) + "::";

	params[keyst + "img"] = "jungle_1"; //_diffusespecular.dds";
	params[keyst + "hs"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[0]->hs);
	params[keyst + "he"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[0]->he);
	params[keyst + "ss"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[0]->ss);
	params[keyst + "se"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[0]->se);
	params[keyst + "skw"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[0]->skw);
	params[keyst + "skwazm"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[0]->skwazm);
	params[keyst + "hr"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[0]->hr);
	params[keyst + "sr"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[0]->sr);

	// LAYER 2
	++position;

	keyst = Ogre::StringConverter::toString(position) + "::";

	params[keyst + "img"] = "jungle_2"; //_diffusespecular.dds";
	params[keyst + "hs"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[1]->hs);
	params[keyst + "he"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[1]->he);
	params[keyst + "ss"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[1]->ss);
	params[keyst + "se"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[1]->se);
	params[keyst + "skw"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[1]->skw);
	params[keyst + "skwazm"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[1]->skwazm);
	params[keyst + "hr"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[1]->hr);
	params[keyst + "sr"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[1]->sr);

	// LAYER 3
	++position;

	keyst = Ogre::StringConverter::toString(position) + "::";

	params[keyst + "img"] = "jungle_4"; //_diffusespecular.dds";
	params[keyst + "hs"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[2]->hs);
	params[keyst + "he"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[2]->he);
	params[keyst + "ss"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[2]->ss);
	params[keyst + "se"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[2]->se);
	params[keyst + "skw"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[2]->skw);
	params[keyst + "skwazm"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[2]->skwazm);
	params[keyst + "hr"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[2]->hr);
	params[keyst + "sr"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[2]->sr);

	// LAYER 4
	++position;

	keyst = Ogre::StringConverter::toString(position) + "::";

	params[keyst + "img"] = "jungle_5"; //_diffusespecular.dds";
	params[keyst + "hs"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[3]->hs);
	params[keyst + "he"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[3]->he);
	params[keyst + "ss"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[3]->ss);
	params[keyst + "se"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[3]->se);
	params[keyst + "skw"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[3]->skw);
	params[keyst + "skwazm"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[3]->skwazm);
	params[keyst + "hr"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[3]->hr);
	params[keyst + "sr"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[3]->sr);

	// LAYER 5
	++position;

	keyst = Ogre::StringConverter::toString(position) + "::";

	params[keyst + "img"] = "jungle_6"; //_diffusespecular.dds";
	params[keyst + "hs"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[4]->hs);
	params[keyst + "he"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[4]->he);
	params[keyst + "ss"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[4]->ss);
	params[keyst + "se"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[4]->se);
	params[keyst + "skw"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[4]->skw);
	params[keyst + "skwazm"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[4]->skwazm);
	params[keyst + "hr"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[4]->hr);
	params[keyst + "sr"] = Ogre::StringConverter::toString(SharedData::getSingleton().m_BlendList[4]->sr);

	return (position > 0);
}

//-----------------------------------------------------------------------------------------
/* EXAMPLE Values 
TYPE | HS-HE  |SS-SE|SKW / SKWAZM|HREL-SREL
--------------------------------------------
grass| 30-150 | 0-20|  0 / 90	 |	10-10
soil | 50-160 |20-60| -1 / 90	 |	10-10
rock |150-200 | 0-60| -1 / 90	 |	10-10
steep|150-400 |30-90|  0 / 90	 |	10-10
snow |200-400 | 0-60| -3 / 90	 |	10-10
*/
inline float _calculateBlendFactor(float h, Ogre::Vector3 &normal, CalcBlendData& data )
{
    //slope of current point (the y value of the normal)
    float slope=normal.y;

    //the skew denominator
    float skewDenom = normal.x * normal.x + normal.z * normal.z;

    //are we to do skewing... 
        //if it is a flat terrain there is no need
    bool doSkew;

    //if skew denominator is "almost" 0 then terrain is flat
    if (skewDenom > 0.00001) 
    {
        //turn the skewing on and calculate the denominator
        doSkew = true;
        skewDenom = 1.0f / sqrt(skewDenom);
    }
    else 
        doSkew=false;

    //factor for this material (for clarity)
    float factor=1.0; 

    //first check - elevation
    float elv_max = data.he;
    float elv_min = data.hs;

    //are we to do skewing ?
    if (doSkew) 
    {
        //calculate 2D skew vector
        float skx = cos(data.skwazm * Ogre::Math::PI / 180.0f);
        float sky = sin(data.skwazm * Ogre::Math::PI / 180.0f);

        //skew scale value
        float scale = ((normal.x * skx) + (normal.y * sky)) * skewDenom;
            
        //adjust elevation limits
        elv_max += data.skw * scale;
        elv_min += data.skw * scale;
    }
    
    //current elevation release
    float rel = data.hr; //RELEASE AMOUNT

    //if outside limit elevation AND release skip this one
    if ((elv_max + rel) < h) 
        return 0.0f;
    if ((elv_min - rel) > h) 
        return 0.0f;
        
    if (elv_max < h) 
    {
        // we are in release compute the factor
        factor = 1.0f - (h - elv_max) / rel;
    }
        
    if (elv_min > h) 
    {
        // we are in release compute the factor
        factor = 1.0f - (elv_min - h) / rel;
    }

    //now check the slopes...

    //slope release
    float srel = cos((Ogre::Math::PI / 2.0f) - (data.sr * Ogre::Math::PI / 180.f));

    //calculate min and max slope
    float minslope = cos(data.ss * Ogre::Math::PI / 180.0f);
    float maxslope = cos(data.se * Ogre::Math::PI / 180.0f);

    //reverse?
    if (minslope > maxslope) 
    {
        float t = maxslope;
        maxslope = minslope;
        minslope = t;
    }

    //this slope is not supported for this type
    if (slope < (minslope - srel)) 
        return 0.0f;
    if (slope > (maxslope + srel)) 
        return 0.0f;

    //release?
    if (slope > maxslope) 
        factor *= 1.0f - ( slope - maxslope) / srel;
    if (slope < minslope) 
        factor *= 1.0f - (minslope - slope) / srel;

    return factor;
}






//-----------------------------------------------------------------------------------------
bool rcMeshLoaderObj::calculateBlendMap(Ogre::Terrain* mHandle)
{
			// return FALSE if we have no terrain instance or its not loaded
			// paging can cause this, need to implement a way to re-call
			// this method once the terrain IS loaded. - possibly isDerivedDataUpdateInProgress()
			// may help here
			if(!mHandle || !mHandle->isLoaded())
					return false;

			// store the number of blending layers we have in a member var for later use
			mLayerCount = mHandle->getLayerCount();

			Ogre::NameValuePairList params;

			if(setupBlendData(params))
			{
				int i;
				for(i = 1;i < mLayerCount;i++)
				{			
					mHandle->removeLayer(1);
				}
			
				mLayerCount = 1;
			
				std::vector<CalcBlendData> layerdata;
			
				for(i = 1;i < 6;i++)
				{
					Ogre::String ids = Ogre::StringConverter::toString(i) + "::";
					if(!(params[ids + "img"].empty()))
					{
						// configure our diffuse/normal texture names
						Ogre::String mTextureDiffuse = params[ids + "img"] + "_diffusespecular.dds";
						Ogre::String mTextureNormal = params[ids + "img"] + "_normalheight.dds";
						
						_createNewLayer(mHandle, mTextureDiffuse, mTextureNormal, true);
			
						CalcBlendData data;
						// pass data to structure responsible for it
						data.hs = Ogre::StringConverter::parseReal(params[ids + "hs"]);
						data.he = Ogre::StringConverter::parseReal(params[ids + "he"]);
						data.hr = Ogre::StringConverter::parseReal(params[ids + "hr"]);
						data.ss = Ogre::StringConverter::parseReal(params[ids + "ss"]);
						data.se = Ogre::StringConverter::parseReal(params[ids + "se"]);
						data.sr = Ogre::StringConverter::parseReal(params[ids + "sr"]);
						data.skw = Ogre::StringConverter::parseReal(params[ids + "skw"]);
						data.skwazm = Ogre::StringConverter::parseReal(params[ids + "skwazm"]);
			
						if(data.hs > data.he)
							std::swap(data.hs, data.he);
			
						if(data.ss > data.se)
							std::swap(data.ss, data.se);
			
						layerdata.push_back(data);
			
					}
					else
						break;
				}
		
				if(layerdata.size() > 0)
				{
					int blendSize = mHandle->getLayerBlendMapSize();
					Ogre::Real steppingWorld = (Ogre::Real)mHandle->getWorldSize() / ((Ogre::Real)blendSize * 2.0f);
					Ogre::Real stepping = 1.0f / (Ogre::Real)blendSize;
					Ogre::Real halfStepping = stepping / 2.0f;
					Ogre::Real quarterStepping = stepping / 4.0f;
			
					float influence_sum[6];
					influence_sum[0] = 0.0f;
			
					Ogre::TerrainLayerBlendMap *mBlendMaps[5];
					float *mBlendDatas[5];
			
					for(unsigned int l = 0;l < layerdata.size();l++)
					{
						mBlendMaps[l] = mHandle->getLayerBlendMap(l + 1);
						mBlendDatas[l] = mBlendMaps[l]->getBlendPointer();
					}
			
					for(int y = 0;y < blendSize;y++)
					{
						int blendPositionY = (blendSize - y - 1) * blendSize;
						for(int x = 0;x < blendSize;x++)
						{
							Ogre::Real centerH = mHandle->getHeightAtTerrainPosition(((Ogre::Real)x * stepping) + halfStepping, ((Ogre::Real)y * stepping) + halfStepping);
							Ogre::Real topH = mHandle->getHeightAtTerrainPosition(((Ogre::Real)x * stepping) + halfStepping, ((Ogre::Real)y * stepping) + halfStepping - quarterStepping);
							Ogre::Real bottomH = mHandle->getHeightAtTerrainPosition(((Ogre::Real)x * stepping) + halfStepping, ((Ogre::Real)y * stepping) + halfStepping + quarterStepping);
							Ogre::Real leftH = mHandle->getHeightAtTerrainPosition(((Ogre::Real)x * stepping) + halfStepping - quarterStepping, ((Ogre::Real)y * stepping) + halfStepping);
							Ogre::Real rightH = mHandle->getHeightAtTerrainPosition(((Ogre::Real)x * stepping) + halfStepping + quarterStepping, ((Ogre::Real)y * stepping) + halfStepping);
		
							Ogre::Vector3 dv1(0,topH - bottomH, steppingWorld);
							Ogre::Vector3 dv2(steppingWorld, leftH - rightH, 0);
							dv1 = dv1.crossProduct(dv2).normalisedCopy();
			
							for(unsigned int current_layer = 0;current_layer < layerdata.size();current_layer++)
							{
								float infl = _calculateBlendFactor(centerH, dv1, layerdata[current_layer]);
								influence_sum[current_layer + 1] = influence_sum[current_layer] + infl;
								infl = infl / std::max(influence_sum[current_layer + 1], 0.001f);
								influence_sum[current_layer + 1] = influence_sum[current_layer] + infl;
								mBlendDatas[current_layer][blendPositionY + x] = infl;
							}
						}
					}
			
					Ogre::Rect maprect(0,0, blendSize, blendSize);
			
					for(unsigned int l = 0;l < layerdata.size();l++)
					{
						mBlendMaps[l]->dirtyRect(maprect);
						mBlendMaps[l]->update();
					}
				}		
				return true;
			}
		
	

	return false;
}

//-----------------------------------------------------------------------------------------
int rcMeshLoaderObj::_getEmptyLayer(Ogre::Terrain* mHandle)
{
	bool isFull;
	unsigned int mBlendMapArea = mHandle->getLayerBlendMapSize() * mHandle->getLayerBlendMapSize();
	for(int i = 1;i < mLayerCount;i++)
	{
		float *ptr = mHandle->getLayerBlendMap(i)->getBlendPointer();
		isFull = false;
		for(unsigned int j = 0;j < mBlendMapArea;j++)
		{
			if(ptr[j] > 0.0f)
			{
				isFull = true;
				break;
			}
		}
		if(!isFull)
			return i;
	}
	return -1;
}
//-----------------------------------------------------------------------------------------
int rcMeshLoaderObj::_createNewLayer(Ogre::Terrain* mHandle, Ogre::String &texture,  Ogre::String& normal, bool donotuseempty)
{
	int layerID = -1;
	unsigned int i;

	if(!donotuseempty)
	{
		layerID = _getEmptyLayer(mHandle);

		if(layerID != -1)
		{
			mHandle->setLayerTextureName(layerID, 0, texture);
			mHandle->setLayerTextureName(layerID, 1, normal);
			for(i = 2;i < mHandle->getLayerDeclaration().samplers.size();i++)
				mHandle->setLayerTextureName(layerID, i, "");

			return layerID;
		}
	}

	if(mLayerCount == MAX_LAYERS_ALLOWED)
		return -1;

	Ogre::StringVector vTextures;
	vTextures.push_back(texture);
	vTextures.push_back(normal);
	for(i = 2;i < mHandle->getLayerDeclaration().samplers.size();i++)
		vTextures.push_back("");

	mHandle->addLayer(70, &vTextures);
	layerID = mLayerCount;
	mLayerCount = (layerID + 1);

	return layerID;
}
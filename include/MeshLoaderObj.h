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


//
// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//


#ifndef MESHLOADER_OBJ
#define MESHLOADER_OBJ

#include "Ogre.h"
#include "OIS.h"
#include "OgreTerrain.h"
#include "OgreTerrainGroup.h"
#include "OgreTerrainPaging.h"

#include "SharedData.h"



using namespace Ogre;

class rcMeshLoaderObj
{
public:
	rcMeshLoaderObj();
	~rcMeshLoaderObj();

	bool load(Ogre::StringVector entNames, Ogre::StringVector fileNames);
	bool load();

	inline const float* getVerts() const { return verts; }
	inline const float* getNormals() const { return m_normals; }
	inline const int* getTris() const { return tris; }
	inline int getVertCount() const { return nverts; }
	inline int getTriCount() const { return ntris; }
	inline const char* getFileName() const { return m_filename; }

	inline Ogre::Entity* getEntity() const { return ent; }
	inline Ogre::SceneNode* getSceneNode() const { return lvlNode; }


	void saveTerrains(bool onlyIfModified);
	void defineTerrain(long x, long y, bool flat = false);
	void getTerrainImage(long x, long y, bool flipX, bool flipY, Image& img);
	void initBlendMaps(Terrain* terrain);
	void configureTerrainDefaults(Light* l);
	MaterialPtr buildDepthShadowMaterial(const String& textureName);
	void changeShadows();
	void configureShadows(bool enabled, bool depthShadows);
	void setupView();
	void setupContent();

	bool frameRender(const Ogre::FrameEvent& evt);
	bool keyPressed( const OIS::KeyEvent &arg );

	bool calculateBlendMap(Ogre::Terrain* mHandle);

	// this shouldnt be public but im lazy.. user is responsible for not misusing this member
	TerrainGroup* mTerrainGroup;

	// DEBUG METHOD FOR TERRAIN BLENDMAP GENERATION - HARDCODED VALUES
	bool setupBlendData(Ogre::NameValuePairList &params);

private:

	Ogre::SceneManager* mSceneMgr;

	void addVertex(float x, float y, float z, int& cap);
	void addTriangle(int a, int b, int c, int& cap);

	Ogre::MaterialPtr myManualObjectMaterial;
	Ogre::ManualObject *obj;
	Ogre::MeshPtr mesh;
	Ogre::Entity* ent;
	Ogre::SceneNode* lvlNode;
	bool mMatsLoaded;

	char m_filename[260];

	float* m_verts;
	int* m_tris;
	float* m_normals;
	int m_vertCount;
	int m_triCount;

	float bmin[3];
	float bmax[3];
	int ntris;//number of total triangles
	int *tris;//list of trinagles
	float *verts;//list of verticies
	int nverts;//number of verticies
	unsigned int numEnt;
	Ogre::StringVector m_entNames;

	void importFullTerrainFromHeightMap();

	float *dataptr;
	
	void loadHouses();
	bool mHousesLoaded;

	TerrainGlobalOptions* mTerrainGlobals;
	bool mPaging;
	TerrainPaging* mTerrainPaging;
	PageManager* mPageManager;
#ifdef PAGING
	/// This class just pretends to provide prcedural page content to avoid page loading
	class DummyPageProvider : public PageProvider
	{
	public:
		bool prepareProceduralPage(Page* page, PagedWorldSection* section) { return true; }
		bool loadProceduralPage(Page* page, PagedWorldSection* section) { return true; }
		bool unloadProceduralPage(Page* page, PagedWorldSection* section) { return true; }
		bool unprepareProceduralPage(Page* page, PagedWorldSection* section) { return true; }
	};
	DummyPageProvider mDummyPageProvider;
#endif
	// unused - TODO : refactor out unused stuff
	bool mFly;
	Real mFallVelocity;

 	enum Mode
 	{
 		MODE_NORMAL = 0,
 		MODE_EDIT_HEIGHT = 1,
 		MODE_EDIT_BLEND = 2,
 		MODE_COUNT = 3
 	};

	enum ShadowMode
	{
		SHADOWS_NONE = 0,
		SHADOWS_COLOUR = 1,
		SHADOWS_DEPTH = 2,
		SHADOWS_COUNT = 3
	};
	Mode mMode;
	ShadowMode mShadowMode;

	Ogre::uint8 mLayerEdit;
	Real mBrushSizeTerrainSpace;
	SceneNode* mEditNode;
	Entity* mEditMarker;
	Real mHeightUpdateCountDown;
	Real mHeightUpdateRate;
	Vector3 mTerrainPos;
	bool mTerrainsImported;
	ShadowCameraSetupPtr mPSSMSetup;

	typedef std::list<Entity*> EntityList;
	EntityList mHouseList;

	int mLayerCount;
	int _createNewLayer(Ogre::Terrain* mHandle, Ogre::String &texture,  Ogre::String& normal, bool donotuseempty);
	int _getEmptyLayer(Ogre::Terrain* mHandle);
	int mPagesTotal;

};

#endif // MESHLOADER_OBJ
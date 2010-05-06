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

#ifndef _H_GUIMANAGER_H_
#define _H_GUIMANAGER_H_

#include <vector>
#include "Ogre.h"
#include "OIS.h"
#include <CEGUI.h> 
#include <RendererModules/Ogre/CEGUIOgreRenderer.h>

using namespace Ogre;

class OgreTemplate;

//---------------------------------------------------------------------------------------------
// HELP TOPIC HELPER CLASS
//---------------------------------------------------------------------------------------------

#define NUMBER_HELP_TOPICS 2

class GUIHelpTopic
{
	CEGUI::String mTopicTitle;
	CEGUI::String mTopicText;
	bool mIsDisplayed;
	bool mIsInitialised;

public:
	GUIHelpTopic()
	{
		mTopicTitle = "";
		mTopicText = "";
		mIsInitialised = true;
	}
	GUIHelpTopic(CEGUI::String& topicTitle)
	{
		mTopicTitle = topicTitle;
		mTopicText = "";
		mIsInitialised = true;
	}
	~GUIHelpTopic()
	{

	}


	void setTopicTitle(CEGUI::String& topicTitle)
	{
		mTopicTitle = topicTitle;
	}

	void setTopicText(CEGUI::String& topicText)
	{
		mTopicText = topicText;
	}

	CEGUI::String getTopicText(void)
	{
		return mTopicText;
	}

	CEGUI::String getTopicTitle(void)
	{
		return mTopicTitle;
	}


	bool getIsInitialised(void)
	{
		return mIsInitialised;
	}
};
typedef std::vector<GUIHelpTopic*> GUIHelpTopicList;


class GUIManager : public FrameListener, public Ogre::Singleton<GUIManager> 
{
public:
	
	GUIManager();
	~GUIManager();
	
	void init();
	void Shutdown();
	void Startup(Ogre::StringVector &meshNames, OgreTemplate* _sample); 
	
	bool frameStarted(const FrameEvent& evt);
	bool frameRenderingQueued(const FrameEvent& evt);
	bool frameEnded(const FrameEvent& evt);

	bool keyPressed( const OIS::KeyEvent &arg );
	bool keyReleased( const OIS::KeyEvent &arg );
	bool mouseMoved( const OIS::MouseEvent &arg );
	bool mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
	bool mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id );

	bool GetShuttingDown( void ) { return mShuttingDown; }
	CEGUI::OgreRenderer* GetRenderer() const { return mRenderer; }

	bool GetInitialisedState() { return mIsInitialised; }
	void SetInitialisedState(bool _initState) { mIsInitialised = _initState; }
	bool GetLeftButtonState() { return mLeftButton; }
	bool GetMiddleButtonState() { return mMiddleButton; }
	bool GetRightButtonState() { return mRightButton; }

	void setStatusText(const CEGUI::String& pText);
	void setStatusText(const CEGUI::String& pText, const CEGUI::Window& pWindow);
	void setStatusDBGText(const CEGUI::String& pText);
	void setInfoPanelText(const CEGUI::String& pText);
	void setOffMeshText(const CEGUI::String& pText);
	void setTileToolText(const CEGUI::String& pText);

	void ShowMouseCur() { CEGUI::MouseCursor::getSingleton().show(); }
	void HideMouseCur() { CEGUI::MouseCursor::getSingleton().hide(); }

	void hideAllTools(void);
	void updateDebugRB(void) { configureDebugRBEx(d_wm->getWindow("Root/DDFrame/DDScrollPane")); }

	CEGUI::Combobox* getMeshSelection(void) { return d_meshSelect; }
	bool getIntermediateResultCheck(void) { return mKeepIntResult; }
	bool getBuildAllTilesCheck(void) { return mBuildAllTiles; }

	bool getBuildToggled(void) { return mBuildToggled; }
	void setBuildToggled(bool _setToggled) { mBuildToggled = _setToggled; }

	bool getClearToggled(void) { return mClearToggled; }
	void setClearToggled(bool _setToggled) { mClearToggled = _setToggled; }

	bool getSaveToggled(void) { return mSaveToggled; }
	void setSaveToggled(bool _setToggled) { mSaveToggled = _setToggled; }

	bool getLoadToggled(void) { return mLoadToggled; }
	void setLoadToggled(bool _setToggled) { mLoadToggled = _setToggled; }

	void resetNavTest(void) 
	{ 
		resetNavTestCB(d_wm->getWindow("Root/NavTestFrame/IncFlagHold"), true);
		resetNavTestCB(d_wm->getWindow("Root/NavTestFrame/ExcFlagHold"), false);
	}

protected:

	void configureMenu(CEGUI::Window* pParent, const bool& pMenubar);
	void configureBuildSliders(CEGUI::Window* pParent);
	void configureDebugRB(CEGUI::Window* pParent);
	void configureDebugRBEx(CEGUI::Window* pParent);
	void configureNavTestRB(CEGUI::Window* pParent);
	void configureNavTestCB(CEGUI::Window* pParent);
	void configureConvexVolumeRB(CEGUI::Window* pParent);
	void configureConvexSliders(CEGUI::Window* pParent);
	void configureFrameWindowMouseEvents(CEGUI::Window* pParent);

	
	void resetNavTestCB(CEGUI::Window* pParent, bool _selected);

	bool onMouseEntersMenuItem(const CEGUI::EventArgs& e);
	bool onMouseLeavesMenuItem(const CEGUI::EventArgs& e);
	bool onMouseLeavesPopupMenuItem(const CEGUI::EventArgs& e);
	bool onMenuItemClicked(const CEGUI::EventArgs& e);
	bool onMenuKey(const CEGUI::EventArgs& e);
	bool onPopupMenu(const CEGUI::EventArgs& e);
	bool hideOptionsWindow(const CEGUI::EventArgs& e);
	bool hideAboutbox(const CEGUI::EventArgs& e);

	// helper to change mouse button enums from OIS format to CEGUI format
	CEGUI::MouseButton convertButton(OIS::MouseButtonID buttonID);

	bool handleToolFrameCB(const CEGUI::EventArgs &e);
	bool handleToolFrameRB(const CEGUI::EventArgs &e);
	bool handleToolFrameBTHideTool(const CEGUI::EventArgs &e);
	bool handleToolFrameBTHideAll(const CEGUI::EventArgs &e);
	bool handleToolFrameBTHelp(const CEGUI::EventArgs &e);
	bool handleBuildSliders(const CEGUI::EventArgs &e);
	bool handleDebugRB(const CEGUI::EventArgs &e);

	bool handleBuildButton(const CEGUI::EventArgs &e);
	bool mBuildToggled;
	bool handleClearButton(const CEGUI::EventArgs &e);
	bool mClearToggled;
	bool handleSaveButton(const CEGUI::EventArgs &e);
	bool mSaveToggled;
	bool handleLoadButton(const CEGUI::EventArgs &e);
	bool mLoadToggled;
	bool handleKeepIntResultCB(const CEGUI::EventArgs &e);
	bool mKeepIntResult;
	bool handleBuildAllTilesCB(const CEGUI::EventArgs &e);
	bool mBuildAllTiles;
	bool handleHelpTopicSelectionCMB(const CEGUI::EventArgs &e);
	bool handleHelpTopicCloseButon(const CEGUI::EventArgs &e);
	bool handleMeshSelectionCMB(const CEGUI::EventArgs &e);
	bool hideInfoWindow(const CEGUI::EventArgs &e);
	bool quitApplication(const CEGUI::EventArgs &e); // callback for cegui to quit app
	bool mShuttingDown;
	
	bool handleNavMeshTestRB(const CEGUI::EventArgs &e);
	bool handleNavMeshTestCB(const CEGUI::EventArgs &e);

	bool handleOffMeshConnRB(const CEGUI::EventArgs &e);

	bool handleConvexVolumeSliders(const CEGUI::EventArgs &e);
	bool handleConvexVolumeRB(const CEGUI::EventArgs &e);
	bool handleConvexVolumeBT(const CEGUI::EventArgs &e);

	bool handleMouseEntersFrame(const CEGUI::EventArgs &e);
	bool handleMouseLeavesFrame(const CEGUI::EventArgs &e);

	bool handleTileToolCreateBT(const CEGUI::EventArgs &e);
	bool handleTileToolRemoveBT(const CEGUI::EventArgs &e);

	// callback handler for all the global hotkeys
	bool								hotkeysHandler(const CEGUI::EventArgs& e);

	// MEMBER VARIABLES ------------------------------------------------------------------------

	CEGUI::OgreRenderer*				mRenderer; // CEGUI renderer


	bool								mIsInitialised; // has gui manager been started ?
	// for camera/character input
	bool								mLeftButton;
	bool								mMiddleButton;
	bool								mRightButton;

	// member data
	CEGUI::WindowManager*				d_wm; // we will use the window manager alot
	CEGUI::System*						d_system;    // the gui system
	CEGUI::Window*						d_root;      // the gui sheet
	CEGUI::Font*						d_font;        // the font we use
	CEGUI::ScrollablePane*				d_pane; // the scrollable pane.
	CEGUI::Combobox*					d_meshSelect; // combobox to select the mesh to use for input.
	CEGUI::Combobox*					d_helpTopicSelect; // comboxbox to select the help topic to display

	OgreTemplate* m_sample;

	GUIHelpTopicList					mHelpTopicList;
	void setupHelpTopics(void);

};

// taken from CEGUI sample7
class MyListItem : public CEGUI::ListboxTextItem
{
public:
	MyListItem(const CEGUI::String& text) : ListboxTextItem(text)
	{
		setSelectionBrushImage("TaharezLook", "MultiListSelectionBrush");
		itemName = text;
	}
	CEGUI::String itemName;
};


#endif // _H_GUIMANAGER_H_
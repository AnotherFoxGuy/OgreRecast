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
#include "OgreTemplate.h"
#include "NavMeshTesterTool.h"
#include "OffMeshConnectionTool.h"
#include "ConvexVolumeTool.h"
#include "GUIManager.h"
#include "SharedData.h"

using namespace Ogre;
//---------------------------------------------------------------------------------------------
// CLASS GLOBALS
//---------------------------------------------------------------------------------------------

template<> GUIManager* Ogre::Singleton<GUIManager>::ms_Singleton = 0;




//---------------------------------------------------------------------------------------------
// CONSTRUCTION / DESTRUCTION
//---------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------
GUIManager::GUIManager():
	mIsInitialised(false), mLeftButton(false), mMiddleButton(false), mRightButton(false),
	mShuttingDown(false), mRenderer(0), d_wm(0), d_system(0), d_root(0), d_font(0), d_pane(0),
	d_meshSelect(0), mKeepIntResult(true), mBuildToggled(false), mSaveToggled(false), 
	mLoadToggled(false), mClearToggled(false), m_sample(0), mBuildAllTiles(true), d_helpTopicSelect(0)
{
	mHelpTopicList.resize(0);
}

//---------------------------------------------------------------------------------------------
GUIManager::~GUIManager()
{
	for(uint i = 0; i < mHelpTopicList.size(); ++i)
	{
		delete mHelpTopicList[i];
	}
	mHelpTopicList.resize(0);
}



//---------------------------------------------------------------------------------------------
// PUBLIC MEMBERS
//---------------------------------------------------------------------------------------------


//---------------------------------------------------------------------------------------------
void GUIManager::init()
{
	mShuttingDown = false;

	using namespace CEGUI;

	SharedData::getSingleton().iRoot->getSingleton().addFrameListener(this);

		// init cegui renderer
	mRenderer = &CEGUI::OgreRenderer::bootstrapSystem();

	// init, set and load cegui resources, matched to resources.cfg in .exe folder
	CEGUI::Imageset::setDefaultResourceGroup("Imagesets");
	CEGUI::Font::setDefaultResourceGroup("Fonts");
	CEGUI::Scheme::setDefaultResourceGroup("Schemes");
	CEGUI::WidgetLookManager::setDefaultResourceGroup("LookNFeel");
	CEGUI::WindowManager::setDefaultResourceGroup("Layouts");

	// set scheme to use for windows
	CEGUI::SchemeManager::getSingleton().create("WindowsLook.scheme");
	CEGUI::SchemeManager::getSingleton().create("TaharezLook.scheme");

	// load the default font
	d_font = &CEGUI::FontManager::getSingleton().create("DejaVuSans-10.font");
	d_font->setProperty("AutoScaled", "false");

	d_font = &CEGUI::FontManager::getSingleton().create("fkp-16.font");
	d_font->setProperty("AutoScaled", "false");

	// to look more like a real application, we override the autoscale setting
	// for both skin and font
	Imageset& wndlook = ImagesetManager::getSingleton().get("WindowsLook");
	wndlook.setAutoScalingEnabled(false);
	

	
	// set the mouse cursor
	d_system = System::getSingletonPtr();
	CEGUI::System::getSingleton().setDefaultMouseCursor("WindowsLook", "MouseArrow");
	CEGUI::System::getSingleton().injectMousePosition(0, 0);
	
	
	// set the default tooltip type
	d_system->setDefaultTooltip("WindowsLook/Tooltip");

	mIsInitialised = true;
}

//---------------------------------------------------------------------------------------------
void GUIManager::Shutdown()
{
	if(mRenderer != NULL)
		CEGUI::OgreRenderer::destroySystem();

	SharedData::getSingleton().iRoot->getSingleton().removeFrameListener(this);
	mIsInitialised = false;
}

//---------------------------------------------------------------------------------------------
void GUIManager::Startup(Ogre::StringVector &meshNames, OgreTemplate* _sample)
{
	m_sample = _sample;

	using namespace CEGUI;

	CEGUI::Window *guiRoot = CEGUI::WindowManager::getSingleton().loadWindowLayout("OgreRecastGUI.layout"); 
	CEGUI::System::getSingleton().setGUISheet(guiRoot);

	// We need the window manager to set up the test interface :)
	d_wm = WindowManager::getSingletonPtr();
	d_root = d_wm->getWindow("Root");

	// root window will take care of hotkeys
	d_root->subscribeEvent(Window::EventKeyDown, Event::Subscriber(&GUIManager::hotkeysHandler, this));

	//-------------------------------------------------------------------------------------------------
	// configure callbacks / setup GUI interactiveness  - could move this to individual methods
	// for tidiness, but atm, this is Proof of Concept so it doesnt matter, i will tidy code up 
	// before releasing.


	// setup the application quit button on the general options dialog
	CEGUI::Window *quit = d_wm->getWindow("Root/GeneralOptFrame/QuitButton");
	quit->subscribeEvent(CEGUI::PushButton::EventClicked,
						 CEGUI::Event::Subscriber(&GUIManager::quitApplication, this));

	// setup the callback for the Options Frame's OK button
	CEGUI::Window* optOK = d_wm->getWindow("Root/GeneralOptFrame/BTOk");
	optOK->subscribeEvent(CEGUI::PushButton::EventClicked,
								  CEGUI::Event::Subscriber(&GUIManager::hideOptionsWindow, this));

	// general options radio button config
	configureOptionsSkyboxRB(d_wm->getWindow("Root/GeneralOptFrame/SetSkyboxHold/SkyboxHoldRB"));
	configureOptionsPolygonModeRB(d_wm->getWindow("Root/GeneralOptFrame/DrawModeHold"));
	configureOptionsTextureFilterRB(d_wm->getWindow("Root/GeneralOptFrame/TextureFilterHold"));
	CEGUI::Window* btPreset = d_wm->getWindow("Root/GeneralOptFrame/BuildValuePresetHold/BTOriginalRecast");
	btPreset->subscribeEvent(CEGUI::PushButton::EventClicked,
							 CEGUI::Event::Subscriber(&GUIManager::handleOptionsPresetButtons, this));
	btPreset = d_wm->getWindow("Root/GeneralOptFrame/BuildValuePresetHold/BTOgreTerrain");
	btPreset->subscribeEvent(CEGUI::PushButton::EventClicked,
							 CEGUI::Event::Subscriber(&GUIManager::handleOptionsPresetButtons, this));
	CEGUI::Slider* optSL = static_cast<Slider*>(d_wm->getWindow("Root/GeneralOptFrame/GUIAlphaHold/SLGUIAlpha"));
	optSL->setCurrentValue(0.9f);
	optSL->subscribeEvent(CEGUI::Slider::EventValueChanged, Event::Subscriber(&GUIManager::handleOptionsAlphaSL, this));

	// setup the callback for the Information Window "OK" button
	CEGUI::Window* ok = d_wm->getWindow("Root/InfoFrame/BTOk");
	ok->subscribeEvent(CEGUI::PushButton::EventClicked,
					   CEGUI::Event::Subscriber(&GUIManager::hideInfoWindow, this));

	// setup callback for help topic window close button
	CEGUI::Window* closeHelp = d_wm->getWindow("Root/HelpWindowFrame/BTClose");
	closeHelp->subscribeEvent(CEGUI::PushButton::EventClicked,
							  CEGUI::Event::Subscriber(&GUIManager::handleHelpTopicCloseButon, this));

	// set the Help Window
	mHelpTopicList.clear();
	mHelpTopicList.resize(0);
	setupHelpTopics();
	d_helpTopicSelect = static_cast<Combobox*>(d_wm->getWindow("Root/HelpWindowFrame/HelpTopicSelectHold/HelpTopicSelectCMB"));
	unsigned int numTopics = mHelpTopicList.size();
	for(unsigned int i = 0; i < numTopics; ++i)
	{
		MyListItem* lstItem = new MyListItem(mHelpTopicList[i]->getTopicTitle());
		lstItem->setTextColours(CEGUI::colour(0.0f, 0.0f, 0.0f));
		d_helpTopicSelect->addItem(lstItem);
	}
	d_helpTopicSelect->setItemSelectState((unsigned int)0, true);
	d_helpTopicSelect->setFont("");
	d_helpTopicSelect->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, 
									  CEGUI::Event::Subscriber(&GUIManager::handleHelpTopicSelectionCMB, this));
	// set the Help window's initial text to be the general usage help
	d_wm->getWindow("Root/HelpWindowFrame/HelpTopicTextHold/HelpTopicTextScrollPane/HelpTopicText")->setText(mHelpTopicList[0]->getTopicText());

	// setup the mesh selection combobox
	d_meshSelect = static_cast<Combobox*>(d_wm->getWindow("Root/BuildFrame/BuildButtons/CMBMeshSelect"));
	unsigned int numMesh = meshNames.size();
	for(unsigned int i = 0; i < numMesh; ++i)
	{
		MyListItem* lstItem = new MyListItem(meshNames[i]);
		lstItem->setTextColours(CEGUI::colour(0.0f, 0.0f, 0.0f));
		d_meshSelect->addItem(lstItem);
	}
	d_meshSelect->setItemSelectState((numMesh-numMesh), true);
	d_meshSelect->setFont("");
	d_meshSelect->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, 
								 CEGUI::Event::Subscriber(&GUIManager::handleMeshSelectionCMB, this));

	

	// setup the tools frame Check Boxes to hide/show the tools panels
	CEGUI::Window *cb = d_wm->getWindow("Root/ToolFrame/ViewOptionsHolder/CBShowGenOptions");
	cb->subscribeEvent(Checkbox::EventCheckStateChanged, Event::Subscriber(&GUIManager::handleToolFrameCB, this));

	cb = d_wm->getWindow("Root/ToolFrame/ViewOptionsHolder/CBShowDDOptions");
	cb->subscribeEvent(Checkbox::EventCheckStateChanged, Event::Subscriber(&GUIManager::handleToolFrameCB, this));

	cb = d_wm->getWindow("Root/ToolFrame/ViewOptionsHolder/CBShowBuildOptions");
	cb->subscribeEvent(Checkbox::EventCheckStateChanged, Event::Subscriber(&GUIManager::handleToolFrameCB, this));


	// setup the tools frame Radio Boxes to show hide the tools panels
	CEGUI::Window* rb = d_wm->getWindow("Root/ToolFrame/ToolTypeHolder/TestHolder/RBNavMeshTest");
	rb->subscribeEvent(RadioButton::EventSelectStateChanged, Event::Subscriber(&GUIManager::handleToolFrameRB, this));

	rb = d_wm->getWindow("Root/ToolFrame/ToolTypeHolder/TestHolder/RBOffMeshConTest1");
	rb->subscribeEvent(RadioButton::EventSelectStateChanged, Event::Subscriber(&GUIManager::handleToolFrameRB, this));

	rb = d_wm->getWindow("Root/ToolFrame/ToolTypeHolder/TestHolder/RBConvexVolumeTool1");
	rb->subscribeEvent(RadioButton::EventSelectStateChanged, Event::Subscriber(&GUIManager::handleToolFrameRB, this));

	rb = d_wm->getWindow("Root/ToolFrame/ToolTypeHolder/RBTileTool");
	rb->subscribeEvent(RadioButton::EventSelectStateChanged, Event::Subscriber(&GUIManager::handleToolFrameRB, this));


	// setup callback for Hide Tools button
	CEGUI::Window* pb = d_wm->getWindow("Root/ToolFrame/BTHideTool");
	pb->subscribeEvent(CEGUI::PushButton::EventClicked, Event::Subscriber(&GUIManager::handleToolFrameBTHideTool, this));


	// setup callback for Hide All button
	pb = d_wm->getWindow("Root/ToolFrame/BTHideAll");
	pb->subscribeEvent(CEGUI::PushButton::EventClicked, Event::Subscriber(&GUIManager::handleToolFrameBTHideAll, this));

	// setup callback for Help Window button
	pb = d_wm->getWindow("Root/ToolFrame/BTHelp");
	pb->subscribeEvent(CEGUI::PushButton::EventClicked, Event::Subscriber(&GUIManager::handleToolFrameBTHelp, this));

	// setupcallbacks for Build Panel Sliders
	CEGUI::Slider* sl = static_cast<Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder1/SLCellSize"));
	sl->setCurrentValue(0.3f);
	d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder1/EBCellSize")->
		  setText(Ogre::StringConverter::toString(sl->getCurrentValue()));
	m_sample->setBuildCellSize(sl->getCurrentValue());

	sl = static_cast<Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder2/SLCellHeight"));
	sl->setCurrentValue(0.2f);
	d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder2/EBCellHeight")->
		  setText(Ogre::StringConverter::toString(sl->getCurrentValue()));
	m_sample->setBuildCellHeight(sl->getCurrentValue());

	sl = static_cast<Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder3/SLAgentHeight"));
	sl->setCurrentValue(2.0f);
	d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder3/EBAgentHeight")->
		  setText(Ogre::StringConverter::toString(sl->getCurrentValue()));
	m_sample->setBuildAgentHeight(sl->getCurrentValue());

	sl = static_cast<Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder4/SLAgentRadius"));
	sl->setCurrentValue(0.6f);
	d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder4/EBAgentRadius")->
		  setText(Ogre::StringConverter::toString(sl->getCurrentValue()));
	m_sample->setBuildAgentRadius(sl->getCurrentValue());

	sl = static_cast<Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder5/SLMaxClimb"));
	sl->setCurrentValue(0.9f);
	d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder5/EBMaxClimb")->
		  setText(Ogre::StringConverter::toString(sl->getCurrentValue()));
	m_sample->setBuildAgentMaxClimb(sl->getCurrentValue());

	sl = static_cast<Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder6/SLMaxSlope"));
	sl->setCurrentValue(45.0f);
	d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder6/EBMaxSlope")->
		  setText(Ogre::StringConverter::toString(sl->getCurrentValue()));
	m_sample->setBuildAgentMaxSlope(sl->getCurrentValue());

	sl = static_cast<Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder7/SLMinRegion"));
	sl->setCurrentValue(50);
	d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder7/EBMinRegion")->
		  setText(Ogre::StringConverter::toString(sl->getCurrentValue()));
	m_sample->setBuildRegionSize(sl->getCurrentValue());

	sl = static_cast<Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder8/SLMergedSize"));
	sl->setCurrentValue(20);
	d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder8/EBMergedSize")->
		  setText(Ogre::StringConverter::toString(sl->getCurrentValue()));
	m_sample->setBuildRegionMerge(sl->getCurrentValue());

	sl = static_cast<Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder9/SLMaxEdge"));
	sl->setCurrentValue(12);
	d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder9/EBMaxEdge")->
		setText(Ogre::StringConverter::toString(sl->getCurrentValue()));
	m_sample->setBuildEdgeLength(sl->getCurrentValue());

	sl = static_cast<Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder10/SLMaxEdgeError"));
	sl->setCurrentValue(1.3f);
	d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder10/EBMaxEdgeError")->
		  setText(Ogre::StringConverter::toString(sl->getCurrentValue()));
	m_sample->setBuildEdgeError(sl->getCurrentValue());

	sl = static_cast<Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder11/SLVertPerPoly"));
	sl->setCurrentValue(6);
	d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder11/EBVertPerPoly")->
		  setText(Ogre::StringConverter::toString(sl->getCurrentValue()));
	m_sample->setBuildVertPerPoly(sl->getCurrentValue());

	sl = static_cast<Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder12/SLSampleDist"));
	sl->setCurrentValue(6);
	d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder12/EBSampleDist")->
		  setText(Ogre::StringConverter::toString(sl->getCurrentValue()));
	m_sample->setBuildSampleDist(sl->getCurrentValue());

	sl = static_cast<Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder13/SLMaxSampErr"));
	sl->setCurrentValue(1);
	d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder13/EBMaxSampErr")->
		  setText(Ogre::StringConverter::toString(sl->getCurrentValue()));
	m_sample->setBuildSampleError(sl->getCurrentValue());

	sl = static_cast<Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder14/SLTileSize"));
	sl->setCurrentValue(32);
	d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder14/EBTileSize")->
		setText(Ogre::StringConverter::toString(sl->getCurrentValue()));
	m_sample->setBuildTileSize(sl->getCurrentValue());

	configureBuildSliders(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane"));

	// setup callbacks for Build Panel Buttons and checkbox
	cb = d_wm->getWindow("Root/BuildFrame/BuildButtons/CBKeepInterResults");
	cb->subscribeEvent(Checkbox::EventCheckStateChanged, Event::Subscriber(&GUIManager::handleKeepIntResultCB, this));

	cb = d_wm->getWindow("Root/BuildFrame/BuildButtons/CBBuildAllTiles");
	cb->subscribeEvent(Checkbox::EventCheckStateChanged, Event::Subscriber(&GUIManager::handleBuildAllTilesCB, this));
	
	cb = d_wm->getWindow("Root/BuildFrame/BuildButtons/BTBuildMesh");
	cb->subscribeEvent(PushButton::EventClicked, Event::Subscriber(&GUIManager::handleBuildButton, this));

	cb = d_wm->getWindow("Root/BuildFrame/BuildButtons/BTClearMesh");
	cb->subscribeEvent(PushButton::EventClicked, Event::Subscriber(&GUIManager::handleClearButton, this));

	cb = d_wm->getWindow("Root/BuildFrame/BuildButtons/BTSaveMesh");
	cb->subscribeEvent(PushButton::EventClicked, Event::Subscriber(&GUIManager::handleSaveButton, this));

	cb = d_wm->getWindow("Root/BuildFrame/BuildButtons/BTLoadMesh");
	cb->subscribeEvent(PushButton::EventClicked, Event::Subscriber(&GUIManager::handleLoadButton, this));

	// setup the callbacks for the Debug Drawing Radio Buttons
	configureDebugRB(d_wm->getWindow("Root/DDFrame/DDScrollPane"));

	// setup callback for NavTest Radio Buttons and Check Boxes
	configureNavTestRB(d_wm->getWindow("Root/NavTestFrame/NavTestTypeHold"));
	configureNavTestCB(d_wm->getWindow("Root/NavTestFrame"));
	//configureNavTestCB(d_wm->getWindow("Root/NavTestFrame/ExcFlagHold"));
	CEGUI::Window* rbEntity = d_wm->getWindow("Root/NavTestFrame/EntityDemoHold/RBIdle");
	rbEntity->subscribeEvent(CEGUI::RadioButton::EventSelectStateChanged, Event::Subscriber(&GUIManager::handleNavMeshTestEntityRB, this));
	rbEntity = d_wm->getWindow("Root/NavTestFrame/EntityDemoHold/RBFindPath");
	rbEntity->subscribeEvent(CEGUI::RadioButton::EventSelectStateChanged, Event::Subscriber(&GUIManager::handleNavMeshTestEntityRB, this));
	
	CEGUI::Window* pbEntity = d_wm->getWindow("Root/NavTestFrame/EntityDemoHold/BTRemoveEntity");
	pbEntity->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIManager::handleNavMeshTestEntityRemoveBT, this));

	// setup callbacks for OffMesh Connection tools RadioButtons
	CEGUI::Window* omRB = d_wm->getWindow("Root/OffMeshTestFrame/OffMeshRBHold/RBOneWay");
	omRB->subscribeEvent(CEGUI::RadioButton::EventSelectStateChanged, Event::Subscriber(&GUIManager::handleOffMeshConnRB, this));
	omRB = d_wm->getWindow("Root/OffMeshTestFrame/OffMeshRBHold/RBTwoWay");
	omRB->subscribeEvent(CEGUI::RadioButton::EventSelectStateChanged, Event::Subscriber(&GUIManager::handleOffMeshConnRB, this));

	// setup callbacks for ConvexVolume Sliders, RadioButtons and PushButton
	CEGUI::Window* cvPB = d_wm->getWindow("Root/ConvexToolsFrame/ConvexOPHold/BTClearConvexShape");
	cvPB->subscribeEvent(PushButton::EventClicked, Event::Subscriber(&GUIManager::handleConvexVolumeBT, this));

	configureConvexVolumeRB(d_wm->getWindow("Root/ConvexToolsFrame/ConvexOPHold/ConvTypeHold"));
	
	CEGUI::Slider* cvSL = static_cast<Slider*>(d_wm->getWindow("Root/ConvexToolsFrame/ConvexOPHold/SLHeightHold/SLConvHeight"));
	cvSL->setCurrentValue(6.0f);
	cvSL->subscribeEvent(CEGUI::Slider::EventValueChanged, Event::Subscriber(&GUIManager::handleConvexVolumeSliders, this));
	d_wm->getWindow("Root/ConvexToolsFrame/ConvexOPHold/SLHeightHold/EBConvHeight")->
		  setText(Ogre::StringConverter::toString(cvSL->getCurrentValue()));

	cvSL = static_cast<Slider*>(d_wm->getWindow("Root/ConvexToolsFrame/ConvexOPHold/SLDescenttHold/SLConvDescent"));
	cvSL->setCurrentValue(1.0f);
	cvSL->subscribeEvent(CEGUI::Slider::EventValueChanged, Event::Subscriber(&GUIManager::handleConvexVolumeSliders, this));
	d_wm->getWindow("Root/ConvexToolsFrame/ConvexOPHold/SLDescentHold/EBConvDescent")->
		  setText(Ogre::StringConverter::toString(cvSL->getCurrentValue()));

	// setup the frame windows to fire mouse events for enter/leave event
	configureFrameWindowMouseEvents(d_wm->getWindow("Root"));

	// set the callbacks for the tile tool buttons
	CEGUI::Window* tlbt = d_wm->getWindow("Root/TileToolFrame/TileToolButHolder/BTCreateAll");
	tlbt->subscribeEvent(PushButton::EventClicked, Event::Subscriber(&GUIManager::handleTileToolCreateBT, this));

	tlbt = d_wm->getWindow("Root/TileToolFrame/TileToolButHolder/BTRemoveAll");
	tlbt->subscribeEvent(PushButton::EventClicked, Event::Subscriber(&GUIManager::handleTileToolRemoveBT, this));

	//---------------------------------------------------------------------------------------------------
	// setup GUI initial visibility - hide / show relevant windows.
	d_wm->getWindow("Root/InfoFrame")->hide();
	d_wm->getWindow("Root/GeneralOptFrame")->hide();
	d_wm->getWindow("Root/ConvexToolsFrame")->hide();
	d_wm->getWindow("Root/OffMeshTestFrame")->hide();
	d_wm->getWindow("Root/NavTestEntityPanel")->hide();
	d_wm->getWindow("Root/NavTestFrame")->hide();
	d_wm->getWindow("Root/DDFrame")->hide();
	d_wm->getWindow("Root/TileToolFrame")->hide();
	d_wm->getWindow("Root/HelpWindowFrame")->hide();

}

//---------------------------------------------------------------------------------------------
bool GUIManager::frameStarted(const FrameEvent& evt)
{
	CEGUI::System::getSingleton().injectTimePulse(static_cast<float>(evt.timeSinceLastFrame));

	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::frameRenderingQueued(const FrameEvent& evt)
{
	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::frameEnded(const FrameEvent& evt)
{
	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::keyPressed( const OIS::KeyEvent &arg )
{
	CEGUI::System &sys = CEGUI::System::getSingleton();
	sys.injectKeyDown(arg.key);
	sys.injectChar(arg.text);


	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::keyReleased( const OIS::KeyEvent &arg )
{
	CEGUI::System::getSingleton().injectKeyUp(arg.key);

	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::mouseMoved( const OIS::MouseEvent &arg )
{
	CEGUI::System::getSingleton().injectMouseMove(Real(arg.state.X.rel), Real(arg.state.Y.rel));
	CEGUI::System::getSingleton().injectMouseWheelChange(arg.state.Z.rel * 0.03f);
	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
{
	CEGUI::System::getSingleton().injectMouseButtonDown(convertButton(id));

	switch(id)
	{
	case OIS::MB_Left:
		mLeftButton = true;
		break;
	case OIS::MB_Right:
		mRightButton = true;
		CEGUI::MouseCursor::getSingleton().hide();
		break;
	case OIS::MB_Middle:
		mMiddleButton = true;
		break;
	}

	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
{
	CEGUI::System::getSingleton().injectMouseButtonUp(convertButton(id));

	switch(id)
	{
	case OIS::MB_Left:
		mLeftButton = false;
		break;
	case OIS::MB_Right:
		mRightButton = false;
		CEGUI::MouseCursor::getSingleton().show();
		break;
	case OIS::MB_Middle:
		mMiddleButton = false;
		break;
	}

	return true;
}



//---------------------------------------------------------------------------------------------
// PRIVATE MEMBERS
//---------------------------------------------------------------------------------------------


//---------------------------------------------------------------------------------------------
bool GUIManager::handleToolFrameCB(const CEGUI::EventArgs &e)
{
	using namespace CEGUI;

	CEGUI::Checkbox* cb = static_cast<Checkbox*>(static_cast<const WindowEventArgs&>(e).window); 
	
	if(cb->getName() == "Root/ToolFrame/ViewOptionsHolder/CBShowGenOptions")
	{
		if(cb->isSelected())
		{
			d_wm->getWindow("Root/GeneralOptFrame")->show();
		}
		else
		{
			d_wm->getWindow("Root/GeneralOptFrame")->hide();
		}
	}
	else if(cb->getName() == "Root/ToolFrame/ViewOptionsHolder/CBShowDDOptions")
	{
		configureDebugRBEx(d_wm->getWindow("Root/DDFrame/DDScrollPane"));
		if(cb->isSelected())
		{
			d_wm->getWindow("Root/DDFrame")->show();
		}
		else
		{
			d_wm->getWindow("Root/DDFrame")->hide();
		}
	}
	else if(cb->getName() == "Root/ToolFrame/ViewOptionsHolder/CBShowBuildOptions")
	{
		if(cb->isSelected())
		{
			d_wm->getWindow("Root/BuildFrame")->show();
		}
		else
		{
			d_wm->getWindow("Root/BuildFrame")->hide();
		}
	}

	return true;
}


//---------------------------------------------------------------------------------------------
bool GUIManager::handleToolFrameRB(const CEGUI::EventArgs &e)
{
	using namespace CEGUI;

	CEGUI::RadioButton* rb = static_cast<CEGUI::RadioButton*>(static_cast<const WindowEventArgs&>(e).window); 

	if(rb->getName() == "Root/ToolFrame/ToolTypeHolder/TestHolder/RBNavMeshTest")
	{
		if(rb->isSelected())
		{
			d_wm->getWindow("Root/NavTestFrame")->show();
			m_sample->setSampleToolType(TOOL_NAVMESH_TESTER);
		}
		else
		{
			d_wm->getWindow("Root/NavTestFrame")->hide();
		}
	}
	else if(rb->getName() == "Root/ToolFrame/ToolTypeHolder/TestHolder/RBOffMeshConTest1")
	{
		if(rb->isSelected())
		{
			d_wm->getWindow("Root/OffMeshTestFrame")->show();
			m_sample->setSampleToolType(TOOL_OFFMESH_CONNECTION);
		}
		else
		{
			d_wm->getWindow("Root/OffMeshTestFrame")->hide();
		}
	}
	else if(rb->getName() == "Root/ToolFrame/ToolTypeHolder/TestHolder/RBConvexVolumeTool1")
	{
		if(rb->isSelected())
		{
			d_wm->getWindow("Root/ConvexToolsFrame")->show();
			m_sample->setSampleToolType(TOOL_CONVEX_VOLUME);
		}
		else
		{
			d_wm->getWindow("Root/ConvexToolsFrame")->hide();
		}
	}
	else if(rb->getName() == "Root/ToolFrame/ToolTypeHolder/RBTileTool")
	{
		if(rb->isSelected())
		{
			d_wm->getWindow("Root/TileToolFrame")->show();
			m_sample->setSampleToolType(TOOL_TILE_EDIT);
		}
		else
		{
			d_wm->getWindow("Root/TileToolFrame")->hide();
		}
	}

	return true;
}


//---------------------------------------------------------------------------------------------
bool GUIManager::handleToolFrameBTHideTool(const CEGUI::EventArgs &e)
{
	using namespace CEGUI;

	CEGUI::RadioButton* rb = static_cast<RadioButton*>(d_wm->getWindow("Root/ToolFrame/ToolTypeHolder/TestHolder/RBNavMeshTest"));
	rb->setSelected(false);

	rb = static_cast<RadioButton*>(d_wm->getWindow("Root/ToolFrame/ToolTypeHolder/TestHolder/RBOffMeshConTest1"));
	rb->setSelected(false);

	rb = static_cast<RadioButton*>(d_wm->getWindow("Root/ToolFrame/ToolTypeHolder/TestHolder/RBConvexVolumeTool1"));
	rb->setSelected(false);

	rb = static_cast<RadioButton*>(d_wm->getWindow("Root/ToolFrame/ToolTypeHolder/RBTileTool"));
	rb->setSelected(false);

	m_sample->setSampleToolType(TOOL_NONE);

	d_wm->getWindow("Root/NavTestFrame")->hide();
	d_wm->getWindow("Root/OffMeshTestFrame")->hide();
	d_wm->getWindow("Root/ConvexToolsFrame")->hide();
	d_wm->getWindow("Root/TileToolFrame")->hide();
	d_wm->getWindow("Root/NavTestEntityPanel")->hide();

	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::handleToolFrameBTHideAll(const CEGUI::EventArgs &e)
{
	using namespace CEGUI;

	CEGUI::RadioButton* rb = static_cast<RadioButton*>(d_wm->getWindow("Root/ToolFrame/ToolTypeHolder/TestHolder/RBNavMeshTest"));
	rb->setSelected(false);

	rb = static_cast<RadioButton*>(d_wm->getWindow("Root/ToolFrame/ToolTypeHolder/TestHolder/RBOffMeshConTest1"));
	rb->setSelected(false);

	rb = static_cast<RadioButton*>(d_wm->getWindow("Root/ToolFrame/ToolTypeHolder/TestHolder/RBConvexVolumeTool1"));
	rb->setSelected(false);

	rb = static_cast<RadioButton*>(d_wm->getWindow("Root/ToolFrame/ToolTypeHolder/RBTileTool"));
	rb->setSelected(false);


	m_sample->setSampleToolType(TOOL_NONE);

	CEGUI::Checkbox* cb = static_cast<Checkbox*>(d_wm->getWindow("Root/ToolFrame/ViewOptionsHolder/CBShowGenOptions"));
	cb->setSelected(false);

	cb = static_cast<Checkbox*>(d_wm->getWindow("Root/ToolFrame/ViewOptionsHolder/CBShowDDOptions"));
	cb->setSelected(false);

	cb = static_cast<Checkbox*>(d_wm->getWindow("Root/ToolFrame/ViewOptionsHolder/CBShowBuildOptions"));
	cb->setSelected(false);

	d_wm->getWindow("Root/NavTestFrame")->hide();
	d_wm->getWindow("Root/OffMeshTestFrame")->hide();
	d_wm->getWindow("Root/ConvexToolsFrame")->hide();
	d_wm->getWindow("Root/NavTestEntityPanel")->hide();
	d_wm->getWindow("Root/InfoFrame")->hide();
	d_wm->getWindow("Root/GeneralOptFrame")->hide();
	d_wm->getWindow("Root/DDFrame")->hide();
	d_wm->getWindow("Root/BuildFrame")->hide();
	d_wm->getWindow("Root/TileToolFrame")->hide();
	d_wm->getWindow("Root/HelpWindowFrame")->hide();

	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::handleToolFrameBTHelp(const CEGUI::EventArgs &e)
{
	if(d_wm->getWindow("Root/HelpWindowFrame")->isVisible())
	{
		d_wm->getWindow("Root/HelpWindowFrame")->hide();
	}
	else if(!d_wm->getWindow("Root/HelpWindowFrame")->isVisible())
	{
		d_wm->getWindow("Root/HelpWindowFrame")->show();
	}
	return true;
}


//---------------------------------------------------------------------------------------------
bool GUIManager::handleBuildSliders(const CEGUI::EventArgs &e)
{
	using namespace CEGUI;

	CEGUI::Slider* sl = static_cast<CEGUI::Slider*>(static_cast<const WindowEventArgs&>(e).window);

	if(sl->getName() == "Root/BuildFrame/BuildSliderSPane/SliderHolder1/SLCellSize")
	{
		Window* eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder1/EBCellSize"));
		eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));
		m_sample->setBuildCellSize(sl->getCurrentValue());
	}
	else if(sl->getName() == "Root/BuildFrame/BuildSliderSPane/SliderHolder2/SLCellHeight")
	{
		Window* eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder2/EBCellHeight"));
		eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));
		m_sample->setBuildCellHeight(sl->getCurrentValue());
	}
	else if(sl->getName() == "Root/BuildFrame/BuildSliderSPane/SliderHolder3/SLAgentHeight")
	{
		Window* eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder3/EBAgentHeight"));
		eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));
		m_sample->setBuildAgentHeight(sl->getCurrentValue());
	}
	else if(sl->getName() == "Root/BuildFrame/BuildSliderSPane/SliderHolder4/SLAgentRadius")
	{
		Window* eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder4/EBAgentRadius"));
		eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));
		m_sample->setBuildAgentRadius(sl->getCurrentValue());
	}
	else if(sl->getName() == "Root/BuildFrame/BuildSliderSPane/SliderHolder5/SLMaxClimb")
	{
		Window* eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder5/EBMaxClimb"));
		eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));
		m_sample->setBuildAgentMaxClimb(sl->getCurrentValue());
	}
	else if(sl->getName() == "Root/BuildFrame/BuildSliderSPane/SliderHolder6/SLMaxSlope")
	{
		Window* eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder6/EBMaxSlope"));
		eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));
		m_sample->setBuildAgentMaxSlope(sl->getCurrentValue());
	}
	else if(sl->getName() == "Root/BuildFrame/BuildSliderSPane/SliderHolder7/SLMinRegion")
	{
		Window* eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder7/EBMinRegion"));
		eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));
		m_sample->setBuildRegionSize(sl->getCurrentValue());
	}
	else if(sl->getName() == "Root/BuildFrame/BuildSliderSPane/SliderHolder8/SLMergedSize")
	{
		Window* eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder8/EBMergedSize"));
		eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));
		m_sample->setBuildRegionMerge(sl->getCurrentValue());
	}
	else if(sl->getName() == "Root/BuildFrame/BuildSliderSPane/SliderHolder9/SLMaxEdge")
	{
		Window* eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder9/EBMaxEdge"));
		eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));
		m_sample->setBuildEdgeLength(sl->getCurrentValue());
	}
	else if(sl->getName() == "Root/BuildFrame/BuildSliderSPane/SliderHolder10/SLMaxEdgeError")
	{
		Window* eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder10/EBMaxEdgeError"));
		eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));
		m_sample->setBuildEdgeError(sl->getCurrentValue());
	}
	else if(sl->getName() == "Root/BuildFrame/BuildSliderSPane/SliderHolder11/SLVertPerPoly")
	{
		Window* eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder11/EBVertPerPoly"));
		eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));
		m_sample->setBuildVertPerPoly(sl->getCurrentValue());
	}
	else if(sl->getName() == "Root/BuildFrame/BuildSliderSPane/SliderHolder12/SLSampleDist")
	{
		Window* eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder12/EBSampleDist"));
		eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));
		m_sample->setBuildSampleDist(sl->getCurrentValue());
	}
	else if(sl->getName() == "Root/BuildFrame/BuildSliderSPane/SliderHolder13/SLMaxSampErr")
	{
		Window* eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder13/EBMaxSampErr"));
		eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));
		m_sample->setBuildSampleError(sl->getCurrentValue());
	}
	else if(sl->getName() == "Root/BuildFrame/BuildSliderSPane/SliderHolder14/SLTileSize")
	{
		Window* eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder14/EBTileSize"));
		int curVal = sl->getCurrentValue();
		if(curVal < 16)
		{
			curVal = 16;
			sl->setCurrentValue(curVal);
		}
		else if(curVal < 1008)
		{	// this is HACKY - will replace when cleaning up code
			// should be using proper methods to pull the decimal part of the float
			// out of it rather than simply casting from float to int types
			// works on VC compiler but may not on other OS's /Release settings
			float newVal = curVal / 16;
			int Val = newVal;
			curVal = Val * 16;
		}
		else if(curVal > 1008)
		{
			curVal = 1024;
			sl->setCurrentValue(curVal);
		}
		eb->setText(Ogre::StringConverter::toString(curVal));
		m_sample->setBuildTileSize(curVal);
	}
		
	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::handleBuildButton(const CEGUI::EventArgs &e)
{
	mBuildToggled = true;
	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::handleClearButton(const CEGUI::EventArgs &e)
{
	mClearToggled = true;
	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::handleSaveButton(const CEGUI::EventArgs &e)
{
	mSaveToggled = true;
	//d_wm->getWindow("Root/InfoFrame/InfoPanel")->setText("Sorry but the Save Mesh feature is not implemented yet. Click OK to continue.");
	//d_wm->getWindow("Root/InfoFrame")->show();
	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::handleLoadButton(const CEGUI::EventArgs &e)
{
	mLoadToggled = true;
	//d_wm->getWindow("Root/InfoFrame/InfoPanel")->setText("Sorry but the Load Mesh feature is not implemented yet. Click OK to continue.");
	//d_wm->getWindow("Root/InfoFrame")->show();
	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::handleKeepIntResultCB(const CEGUI::EventArgs &e)
{
	using namespace CEGUI;
	CEGUI::Checkbox* cb = static_cast<Checkbox*>(static_cast<const WindowEventArgs&>(e).window); 

	mKeepIntResult = cb->isSelected();

	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::handleBuildAllTilesCB(const CEGUI::EventArgs &e)
{
	using namespace CEGUI;
	CEGUI::Checkbox* cb = static_cast<Checkbox*>(static_cast<const WindowEventArgs&>(e).window); 

	mBuildAllTiles = cb->isSelected();

	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::handleDebugRB(const CEGUI::EventArgs &e)
{
	using namespace CEGUI;

	CEGUI::RadioButton* rb = static_cast<CEGUI::RadioButton*>(static_cast<const WindowEventArgs&>(e).window); 

	configureDebugRBEx(d_wm->getWindow("Root/DDFrame/DDScrollPane"));

	if(rb->getName() == "Root/DDFrame/DDScrollPane/RBInputMesh")
	{
		if(rb->isSelected())
		{
			if(m_sample->valid[DRAWMODE_MESH])
			{
				m_sample->setDebugDrawMode(DRAWMODE_MESH);
			}
		}
		else
		{
			
		}
	}
	else if(rb->getName() == "Root/DDFrame/DDScrollPane/RBNavMesh")
	{
		if(rb->isSelected())
		{
			if(m_sample->valid[DRAWMODE_NAVMESH])
			{
				m_sample->setDebugDrawMode(DRAWMODE_NAVMESH);
			}
		}
		else
		{
			
		}
	}
	else if(rb->getName() == "Root/DDFrame/DDScrollPane/RBNavMeshInv")
	{
		if(rb->isSelected())
		{
			if(m_sample->valid[DRAWMODE_NAVMESH_INVIS])
			{
				m_sample->setDebugDrawMode(DRAWMODE_NAVMESH_INVIS);
			}
		}
		else
		{
			
		}
	}
	else if(rb->getName() == "Root/DDFrame/DDScrollPane/RBNavMeshTrans")
	{
		if(rb->isSelected())
		{
			if(m_sample->valid[DRAWMODE_NAVMESH_TRANS])
			{
				m_sample->setDebugDrawMode(DRAWMODE_NAVMESH_TRANS);
			}
		}
		else
		{

		}
	}
	else if(rb->getName() == "Root/DDFrame/DDScrollPane/RBNavMeshBV")
	{
		if(rb->isSelected())
		{
			if(m_sample->valid[DRAWMODE_NAVMESH_BVTREE])
			{
				m_sample->setDebugDrawMode(DRAWMODE_NAVMESH_BVTREE);
			}
		}
		else
		{

		}
	}
	else if(rb->getName() == "Root/DDFrame/DDScrollPane/RBVoxel")
	{
		if(rb->isSelected())
		{
			if(m_sample->valid[DRAWMODE_VOXELS])
			{	
				m_sample->setDebugDrawMode(DRAWMODE_VOXELS);
			}
		}
		else
		{

		}
	}
	else if(rb->getName() == "Root/DDFrame/DDScrollPane/RBWalkVoxel")
	{
		if(rb->isSelected())
		{
			if(m_sample->valid[DRAWMODE_VOXELS_WALKABLE])
			{	
				m_sample->setDebugDrawMode(DRAWMODE_VOXELS_WALKABLE);
			}
		}
		else
		{

		}
	}
	else if(rb->getName() == "Root/DDFrame/DDScrollPane/RBCompact")
	{
		if(rb->isSelected())
		{
			if(m_sample->valid[DRAWMODE_COMPACT])
			{
				m_sample->setDebugDrawMode(DRAWMODE_COMPACT);
			}
		}
		else
		{

		}
	}
	else if(rb->getName() == "Root/DDFrame/DDScrollPane/RBCompactDist")
	{
		if(rb->isSelected())
		{
			if(m_sample->valid[DRAWMODE_COMPACT_DISTANCE])
			{
				m_sample->setDebugDrawMode(DRAWMODE_COMPACT_DISTANCE);
			}
		}
		else
		{

		}
	}
	else if(rb->getName() == "Root/DDFrame/DDScrollPane/RBCompactRegion")
	{
		if(rb->isSelected())
		{
			if(m_sample->valid[DRAWMODE_COMPACT_REGIONS])
			{
				m_sample->setDebugDrawMode(DRAWMODE_COMPACT_REGIONS);
			}
		}
		else
		{

		}
	}
	else if(rb->getName() == "Root/DDFrame/DDScrollPane/RBRegionConnect")
	{
		if(rb->isSelected())
		{
			if(m_sample->valid[DRAWMODE_REGION_CONNECTIONS])
			{
				m_sample->setDebugDrawMode(DRAWMODE_REGION_CONNECTIONS);
			}
		}
		else
		{

		}
	}
	else if(rb->getName() == "Root/DDFrame/DDScrollPane/RBContoursRaw")
	{
		if(rb->isSelected())
		{
			if(m_sample->valid[DRAWMODE_RAW_CONTOURS])
			{
				m_sample->setDebugDrawMode(DRAWMODE_RAW_CONTOURS);
			}
		}
		else
		{

		}
	}
	else if(rb->getName() == "Root/DDFrame/DDScrollPane/RBContoursBoth")
	{
		if(rb->isSelected())
		{
			if(m_sample->valid[DRAWMODE_BOTH_CONTOURS])
			{	
				m_sample->setDebugDrawMode(DRAWMODE_BOTH_CONTOURS);
			}
		}
		else
		{

		}
	}
	else if(rb->getName() == "Root/DDFrame/DDScrollPane/RBContours")
	{
		if(rb->isSelected())
		{
			if(m_sample->valid[DRAWMODE_CONTOURS])
			{
				m_sample->setDebugDrawMode(DRAWMODE_CONTOURS);
			}
		}
		else
		{

		}
	}
	else if(rb->getName() == "Root/DDFrame/DDScrollPane/RBPolyMeshDetail")
	{
		if(rb->isSelected())
		{
			if(m_sample->valid[DRAWMODE_POLYMESH_DETAIL])
			{
				m_sample->setDebugDrawMode(DRAWMODE_POLYMESH_DETAIL);
			}
		}
		else
		{

		}
	}
	else if(rb->getName() == "Root/DDFrame/DDScrollPane/RBPolyMesh")
	{
		if(rb->isSelected())
		{
			if(m_sample->valid[DRAWMODE_POLYMESH])
			{
				m_sample->setDebugDrawMode(DRAWMODE_POLYMESH);
			}
		}
		else
		{

		}
	}

	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::handleNavMeshTestEntityRB(const CEGUI::EventArgs &e)
{
	using namespace CEGUI;
	
	CEGUI::RadioButton* rb = static_cast<CEGUI::RadioButton*>(static_cast<const WindowEventArgs&>(e).window); 

	if(rb->getName() == "Root/NavTestFrame/EntityDemoHold/RBFindPath")
	{
		static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->setEntityMode(ENTITY_FINDPATH);
	}
	else if(rb->getName() == "Root/NavTestFrame/EntityDemoHold/RBIdle")
	{
		static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->setEntityMode(ENTITY_IDLE);
	}

	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::handleNavMeshTestEntityRemoveBT(const CEGUI::EventArgs &e)
{
	using namespace CEGUI;

	static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->removeLatestEntity();

	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::handleNavMeshTestRB(const CEGUI::EventArgs &e)
{
	using namespace CEGUI;

	CEGUI::RadioButton* rb = static_cast<CEGUI::RadioButton*>(static_cast<const WindowEventArgs&>(e).window); 

	if(rb->getName() == "Root/NavTestFrame/NavTestTypeHold/RBPathFindIter")
	{
		if(rb->isSelected())
		{
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->setToolMode(TOOLMODE_PATHFIND_ITER);
		}
		else
		{

		}
	}
	else if(rb->getName() == "Root/NavTestFrame/NavTestTypeHold/RBPathFindStraight")
	{
		if(rb->isSelected())
		{
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->setToolMode(TOOLMODE_PATHFIND_STRAIGHT);
		}
		else
		{

		}
	}
	else if(rb->getName() == "Root/NavTestFrame/NavTestTypeHold/RBDistToWall")
	{
		if(rb->isSelected())
		{
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->setToolMode(TOOLMODE_DISTANCE_TO_WALL);
		}
		else
		{

		}
	}
	else if(rb->getName() == "Root/NavTestFrame/NavTestTypeHold/RBRaycast")
	{
		if(rb->isSelected())
		{
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->setToolMode(TOOLMODE_RAYCAST);
		}
		else
		{

		}
	}
	else if(rb->getName() == "Root/NavTestFrame/NavTestTypeHold/RBFindPoly")
	{
		if(rb->isSelected())
		{
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->setToolMode(TOOLMODE_FIND_POLYS_AROUND);
		}
		else
		{

		}
	}
	else if(rb->getName() == "Root/NavTestFrame/NavTestTypeHold/RBEntityDemo")
	{
		if(rb->isSelected())
		{
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->setToolMode(TOOLMODE_ENTITY_DEMO);
			d_wm->getWindow("Root/NavTestEntityPanel")->show();
		}
		else
		{
			d_wm->getWindow("Root/NavTestEntityPanel")->hide();
		}
	}

	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::handleNavMeshTestCB(const CEGUI::EventArgs &e)
{
	using namespace CEGUI;

	CEGUI::Checkbox* cb = static_cast<CEGUI::Checkbox*>(static_cast<const WindowEventArgs&>(e).window); 

	// include flags
	if(cb->getName() == "Root/NavTestFrame/IncFlagHold/CBIncWalk")
	{
		if(cb->isSelected())
		{
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->m_filter.includeFlags ^= SAMPLE_POLYFLAGS_WALK;
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->recalc();
		}
		else
		{
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->m_filter.includeFlags ^= SAMPLE_POLYFLAGS_WALK;
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->recalc();
		}
	}
	else if(cb->getName() == "Root/NavTestFrame/IncFlagHold/CBIncSwim")
	{
		if(cb->isSelected())
		{
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->m_filter.includeFlags ^= SAMPLE_POLYFLAGS_SWIM;
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->recalc();
		}
		else
		{
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->m_filter.includeFlags ^= SAMPLE_POLYFLAGS_SWIM;
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->recalc();
		}
	}
	else if(cb->getName() == "Root/NavTestFrame/IncFlagHold/CBIncDoor")
	{
		if(cb->isSelected())
		{
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->m_filter.includeFlags ^= SAMPLE_POLYFLAGS_DOOR;
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->recalc();
		}
		else
		{
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->m_filter.includeFlags ^= SAMPLE_POLYFLAGS_DOOR;
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->recalc();
		}
	}
	else if(cb->getName() == "Root/NavTestFrame/IncFlagHold/CBIncJump")
	{
		if(cb->isSelected())
		{
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->m_filter.includeFlags ^= SAMPLE_POLYFLAGS_JUMP;
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->recalc();
		}
		else
		{
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->m_filter.includeFlags ^= SAMPLE_POLYFLAGS_JUMP;
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->recalc();
		}
	}
	// exclude flags
	else if(cb->getName() == "Root/NavTestFrame/ExcFlagHold/CBExcWalk")
	{
		if(cb->isSelected())
		{
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->m_filter.excludeFlags ^= SAMPLE_POLYFLAGS_WALK;
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->recalc();
		}
		else
		{
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->m_filter.excludeFlags ^= SAMPLE_POLYFLAGS_WALK;
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->recalc();
		}
	}
	else if(cb->getName() == "Root/NavTestFrame/ExcFlagHold/CBExcSwim")
	{
		if(cb->isSelected())
		{
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->m_filter.excludeFlags ^= SAMPLE_POLYFLAGS_SWIM;
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->recalc();
		}
		else
		{
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->m_filter.excludeFlags ^= SAMPLE_POLYFLAGS_SWIM;
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->recalc();
		}
	}
	else if(cb->getName() == "Root/NavTestFrame/ExcFlagHold/CBExcDoor")
	{
		if(cb->isSelected())
		{
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->m_filter.excludeFlags ^= SAMPLE_POLYFLAGS_DOOR;			
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->recalc();
		}
		else
		{
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->m_filter.excludeFlags ^= SAMPLE_POLYFLAGS_DOOR;			
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->recalc();
		}
	}
	else if(cb->getName() == "Root/NavTestFrame/ExcFlagHold/CBExcJump")
	{
		if(cb->isSelected())
		{
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->m_filter.excludeFlags ^= SAMPLE_POLYFLAGS_JUMP;
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->recalc();
		}
		else
		{
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->m_filter.excludeFlags ^= SAMPLE_POLYFLAGS_JUMP;
			static_cast<NavMeshTesterTool*>(m_sample->getCurrentTool())->recalc();
		}
	}

	return true;
}


//---------------------------------------------------------------------------------------------
bool GUIManager::handleOffMeshConnRB(const CEGUI::EventArgs &e)
{
	using namespace CEGUI;

	CEGUI::RadioButton* rb = static_cast<CEGUI::RadioButton*>(static_cast<const WindowEventArgs&>(e).window); 

	if(rb->getName() == "Root/OffMeshTestFrame/OffMeshRBHold/RBTwoWay")
	{
		if(rb->isSelected())
			static_cast<OffMeshConnectionTool*>(m_sample->getCurrentTool())->setBiDirection(true);
	}
	else if(rb->getName() == "Root/OffMeshTestFrame/OffMeshRBHold/RBOneWay")
	{
		if(rb->isSelected())
			static_cast<OffMeshConnectionTool*>(m_sample->getCurrentTool())->setBiDirection(false);
	}

	return true;
}


//---------------------------------------------------------------------------------------------
bool GUIManager::handleConvexVolumeSliders(const CEGUI::EventArgs &e)
{
	using namespace CEGUI;

	CEGUI::Slider* sl = static_cast<CEGUI::Slider*>(static_cast<const WindowEventArgs&>(e).window);

	if(sl->getName() == "Root/ConvexToolsFrame/ConvexOPHold/SLHeightHold/SLConvHeight")
	{
		Window* eb = (d_wm->getWindow("Root/ConvexToolsFrame/ConvexOPHold/SLHeightHold/EBConvHeight"));
		eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));
		static_cast<ConvexVolumeTool*>(m_sample->getCurrentTool())->setBoxHeight(sl->getCurrentValue());
	}
	else if(sl->getName() == "Root/ConvexToolsFrame/ConvexOPHold/SLDescenttHold/SLConvDescent")
	{
		Window* eb = (d_wm->getWindow("Root/ConvexToolsFrame/ConvexOPHold/SLDescentHold/EBConvDescent"));
		eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));
		static_cast<ConvexVolumeTool*>(m_sample->getCurrentTool())->setBoxDescent(sl->getCurrentValue());
	}

	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::handleConvexVolumeRB(const CEGUI::EventArgs &e)
{

	using namespace CEGUI;

	CEGUI::RadioButton* rb = static_cast<CEGUI::RadioButton*>(static_cast<const WindowEventArgs&>(e).window); 

	if(rb->getName() == "Root/ConvexToolsFrame/ConvexOPHold/ConvTypeHold/ConvTypeGrass")
	{
		if(rb->isSelected())
		{
			static_cast<ConvexVolumeTool*>(m_sample->getCurrentTool())->setAreaType(SAMPLE_POLYAREA_GRASS);
		}
		else
		{

		}
	}
	else if(rb->getName() == "Root/ConvexToolsFrame/ConvexOPHold/ConvTypeHold/ConvTypeRoad")
	{
		if(rb->isSelected())
		{
			static_cast<ConvexVolumeTool*>(m_sample->getCurrentTool())->setAreaType(SAMPLE_POLYAREA_ROAD);
		}
		else
		{

		}
	}
	else if(rb->getName() == "Root/ConvexToolsFrame/ConvexOPHold/ConvTypeHold/ConvTypeWater")
	{
		if(rb->isSelected())
		{
			static_cast<ConvexVolumeTool*>(m_sample->getCurrentTool())->setAreaType(SAMPLE_POLYAREA_WATER);
		}
		else
		{

		}
	}
	else if(rb->getName() == "Root/ConvexToolsFrame/ConvexOPHold/ConvTypeHold/ConvTypeDoor")
	{
		if(rb->isSelected())
		{
			static_cast<ConvexVolumeTool*>(m_sample->getCurrentTool())->setAreaType(SAMPLE_POLYAREA_DOOR);
		}
		else
		{

		}
	}

	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::handleConvexVolumeBT(const CEGUI::EventArgs &e)
{
	static_cast<ConvexVolumeTool*>(m_sample->getCurrentTool())->setClearShape();
	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::handleHelpTopicCloseButon(const CEGUI::EventArgs &e)
{
	d_wm->getWindow("Root/HelpWindowFrame")->hide();
	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::handleHelpTopicSelectionCMB(const CEGUI::EventArgs &e)
{
	using namespace CEGUI;
	
	Combobox* dm = static_cast<Combobox*>(d_wm->getWindow("Root/HelpWindowFrame/HelpTopicSelectHold/HelpTopicSelectCMB"));

	if(dm->getSelectedItem()->getText() == "Base Application Interface")
	{
		d_wm->getWindow("Root/HelpWindowFrame/HelpTopicTextHold/HelpTopicTextScrollPane/HelpTopicText")->setText(mHelpTopicList[0]->getTopicText());
	}
	else if(dm->getSelectedItem()->getText() == "Application Tool Notes")
	{
		d_wm->getWindow("Root/HelpWindowFrame/HelpTopicTextHold/HelpTopicTextScrollPane/HelpTopicText")->setText(mHelpTopicList[1]->getTopicText());
	}
	else if(dm->getSelectedItem()->getText() == "NavMesh Test Tool Notes")
	{
		d_wm->getWindow("Root/HelpWindowFrame/HelpTopicTextHold/HelpTopicTextScrollPane/HelpTopicText")->setText(mHelpTopicList[2]->getTopicText());
	}
	else if(dm->getSelectedItem()->getText() == "NavMesh Test Tool Entity Notes")
	{
		d_wm->getWindow("Root/HelpWindowFrame/HelpTopicTextHold/HelpTopicTextScrollPane/HelpTopicText")->setText(mHelpTopicList[3]->getTopicText());
	}
	else if(dm->getSelectedItem()->getText() == "ConvexVolume Tool Notes")
	{
		d_wm->getWindow("Root/HelpWindowFrame/HelpTopicTextHold/HelpTopicTextScrollPane/HelpTopicText")->setText(mHelpTopicList[4]->getTopicText());
	}
	else if(dm->getSelectedItem()->getText() == "Off-Mesh Connection Tool Notes")
	{
		d_wm->getWindow("Root/HelpWindowFrame/HelpTopicTextHold/HelpTopicTextScrollPane/HelpTopicText")->setText(mHelpTopicList[5]->getTopicText());
	}
	
	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::handleMeshSelectionCMB(const CEGUI::EventArgs &e)
{
	using namespace CEGUI;

	Combobox* dm = static_cast<Combobox*>(d_wm->getWindow("Root/BuildFrame/BuildButtons/CMBMeshSelect"));
	m_sample->setCurrentMeshName(dm->getSelectedItem()->getText().c_str());
	
	return true;
}


//---------------------------------------------------------------------------------------------
bool GUIManager::handleMouseEntersFrame(const CEGUI::EventArgs &e)
{
	using namespace CEGUI;

	static_cast<CEGUI::Window*>(static_cast<const WindowEventArgs&>(e).window)->activate();
	m_sample->setCastRays(false);

	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::handleMouseLeavesFrame(const CEGUI::EventArgs &e)
{
	using namespace CEGUI;
	
	static_cast<CEGUI::Window*>(static_cast<const WindowEventArgs&>(e).window)->deactivate();
	m_sample->setCastRays(true);

	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::handleTileToolCreateBT(const CEGUI::EventArgs &e)
{
	m_sample->buildAllTiles();
	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::handleTileToolRemoveBT(const CEGUI::EventArgs &e)
{
	m_sample->removeAllTiles();
	return true;
}


//---------------------------------------------------------------------------------------------
bool GUIManager::handleOptionsSkyboxRB(const CEGUI::EventArgs &e)
{
	using namespace CEGUI;

	CEGUI::RadioButton* rb = static_cast<CEGUI::RadioButton*>(static_cast<const WindowEventArgs&>(e).window); 

	if(rb->getName() == "Root/GeneralOptFrame/SetSkyboxHold/SkyboxHoldRB/RBSkyboxNone")
	{
		m_sample->setCurrentSkybox(SKYBOX_NONE);
	}
	else if(rb->getName() == "Root/GeneralOptFrame/SetSkyboxHold/SkyboxHoldRB/RBSkyboxMorning")
	{
		m_sample->setCurrentSkybox(SKYBOX_MORNING);
	}
	else if(rb->getName() == "Root/GeneralOptFrame/SetSkyboxHold/SkyboxHoldRB/RBSkyboxEvening")
	{
		m_sample->setCurrentSkybox(SKYBOX_EVENING);
	}
	else if(rb->getName() == "Root/GeneralOptFrame/SetSkyboxHold/SkyboxHoldRB/RBSkyboxDawn")
	{
		m_sample->setCurrentSkybox(SKYBOX_DAWN);
	}

	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::handleOptionsTextureFilteringRB(const CEGUI::EventArgs &e)
{

	using namespace CEGUI;

	CEGUI::RadioButton* rb = static_cast<CEGUI::RadioButton*>(static_cast<const WindowEventArgs&>(e).window); 

	if(rb->getName() == "Root/GeneralOptFrame/TextureFilterHold/TextureFilterNoneRB")
	{
		Ogre::MaterialManager::getSingleton().setDefaultTextureFiltering(Ogre::TFO_NONE);
		Ogre::MaterialManager::getSingleton().setDefaultAnisotropy(1);
	}
	else if(rb->getName() == "Root/GeneralOptFrame/TextureFilterHold/TextureFilterBilinearRB")
	{
		Ogre::MaterialManager::getSingleton().setDefaultTextureFiltering(Ogre::TFO_BILINEAR);
		Ogre::MaterialManager::getSingleton().setDefaultAnisotropy(1);
	}
	else if(rb->getName() == "Root/GeneralOptFrame/TextureFilterHold/TextureFilterTrilinearRB")
	{
		Ogre::MaterialManager::getSingleton().setDefaultTextureFiltering(Ogre::TFO_TRILINEAR);
		Ogre::MaterialManager::getSingleton().setDefaultAnisotropy(1);
	}
	else if(rb->getName() == "Root/GeneralOptFrame/TextureFilterHold/TextureFilterAnisotropicRB")
	{
		Ogre::MaterialManager::getSingleton().setDefaultTextureFiltering(Ogre::TFO_ANISOTROPIC);
		Ogre::MaterialManager::getSingleton().setDefaultAnisotropy(8);
	}

	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::handleOptionsPolygonModeRB(const CEGUI::EventArgs &e)
{
	using namespace CEGUI;

	CEGUI::RadioButton* rb = static_cast<CEGUI::RadioButton*>(static_cast<const WindowEventArgs&>(e).window); 

	if(rb->getName() == "Root/GeneralOptFrame/DrawModeHold/DrawModeSolidRB")
	{
		SharedData::getSingleton().iCamera->setPolygonMode(Ogre::PM_SOLID);
	}
	else if(rb->getName() == "Root/GeneralOptFrame/DrawModeHold/DrawModeWireframeRB")
	{
		SharedData::getSingleton().iCamera->setPolygonMode(Ogre::PM_WIREFRAME);
	}
	else if(rb->getName() == "Root/GeneralOptFrame/DrawModeHold/DrawModePointsRB")
	{
		SharedData::getSingleton().iCamera->setPolygonMode(Ogre::PM_POINTS);
	}

	return true;
}

//---------------------------------------------------------------------------------------------
void GUIManager::setPolygonMode(int _polyMode)
{
	using namespace CEGUI;

	switch(_polyMode)
	{
	case Ogre::PM_SOLID:
		static_cast<CEGUI::RadioButton*>(d_wm->getWindow("Root/GeneralOptFrame/DrawModeHold/DrawModeSolidRB"))->setSelected(true);
		break;
	case Ogre::PM_WIREFRAME:
		static_cast<CEGUI::RadioButton*>(d_wm->getWindow("Root/GeneralOptFrame/DrawModeHold/DrawModeWireframeRB"))->setSelected(true);
		break;
	case Ogre::PM_POINTS:
		static_cast<CEGUI::RadioButton*>(d_wm->getWindow("Root/GeneralOptFrame/DrawModeHold/DrawModePointsRB"))->setSelected(true);
		break;
	}
}

//---------------------------------------------------------------------------------------------
bool GUIManager::handleOptionsPresetButtons(const CEGUI::EventArgs &e)
{
	using namespace CEGUI;

	CEGUI::PushButton* pb = static_cast<CEGUI::PushButton*>(static_cast<const WindowEventArgs&>(e).window); 

	if(pb->getName() == "Root/GeneralOptFrame/BuildValuePresetHold/BTOriginalRecast")
	{
		setPresetRecastOriginal();
	}
	else if(pb->getName() == "Root/GeneralOptFrame/BuildValuePresetHold/BTOgreTerrain")
	{
		setPresetOgreTerrain();
	}

	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::handleOptionsAlphaSL(const CEGUI::EventArgs &e)
{
	// Taken from CEGUI Sample Number 7
	using namespace CEGUI;

	// get the current slider value
	float val = static_cast<CEGUI::Slider*>(static_cast<const WindowEventArgs&>(e).window)->getCurrentValue();
	if(val < 0.4f)
	{
		val = 0.4f;
	}
	else if(val > 1.0f)
	{
		val = 1.0f;
	}

	// set the alpha on the window containing all the controls.
	d_wm->getWindow("Root")->setAlpha(val);

	// event was handled.
	return true;
}


//---------------------------------------------------------------------------------------------
void GUIManager::configureMenu(CEGUI::Window* pParent, const bool& pMenubar)
{

}

//---------------------------------------------------------------------------------------------
void GUIManager::setInfoPanelText(const CEGUI::String& pText)
{
	d_wm->getWindow("Root/InfoFrame/InfoPanel")->setText(pText);
	d_wm->getWindow("Root/InfoFrame")->show();
}

//---------------------------------------------------------------------------------------------
void GUIManager::setStatusDBGText(const CEGUI::String& pText)
{
	d_wm->getWindow("Root/InfoText/DBGInfo")->setText(pText);
}

//---------------------------------------------------------------------------------------------
void GUIManager::setStatusText(const CEGUI::String& pText)
{
	d_wm->getWindow("Root/InfoText")->setText(pText);
}

//---------------------------------------------------------------------------------------------
void GUIManager::setOffMeshText(const CEGUI::String& pText)
{
	d_wm->getWindow("Root/OffMeshTestFrame/OffMeshInfo")->setText(pText);
}

//---------------------------------------------------------------------------------------------
void GUIManager::setTileToolText(const CEGUI::String& pText)
{
	d_wm->getWindow("Root/TileToolFrame/TileToolInfo")->setText(pText);
}

//---------------------------------------------------------------------------------------------
void GUIManager::setEntitiesCreatedInfo(int _entCreated)
{
	Ogre::String entCreated = Ogre::StringConverter::toString(_entCreated);
	d_wm->getWindow("Root/NavTestFrame/EntityDemoHold/EntitiesCreated")->setText(entCreated.c_str());
}

//---------------------------------------------------------------------------------------------
bool GUIManager::onMouseEntersMenuItem(const CEGUI::EventArgs& e)
{
	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::onMouseLeavesMenuItem(const CEGUI::EventArgs& e)
{
	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::onMouseLeavesPopupMenuItem(const CEGUI::EventArgs& e)
{
	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::onMenuItemClicked(const CEGUI::EventArgs& e)
{
	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::onMenuKey(const CEGUI::EventArgs& e)
{
	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::onPopupMenu(const CEGUI::EventArgs& e)
{
	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::hideAboutbox(const CEGUI::EventArgs& e)
{
	return true;
}

//---------------------------------------------------------------------------------------------
CEGUI::MouseButton GUIManager::convertButton(OIS::MouseButtonID buttonID)
{
	switch (buttonID)
	{
	case OIS::MB_Left:
		return CEGUI::LeftButton;

	case OIS::MB_Right:
		return CEGUI::RightButton;

	case OIS::MB_Middle:
		return CEGUI::MiddleButton;

	default:
		return CEGUI::LeftButton;
	}
}

//---------------------------------------------------------------------------------------------
bool GUIManager::quitApplication(const CEGUI::EventArgs &e)
{
	mShuttingDown = true;
	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::hideInfoWindow(const CEGUI::EventArgs &e)
{
	d_wm->getWindow("Root/InfoFrame")->hide();
	d_wm->getWindow("Root/InfoFrame/InfoPanel")->setText("");
	return true;
}

//---------------------------------------------------------------------------------------------
bool GUIManager::hotkeysHandler(const CEGUI::EventArgs& e)
{
	using namespace CEGUI;
	const KeyEventArgs& k = static_cast<const KeyEventArgs&>(e);

	switch (k.scancode)
	{
		// space is a hotkey for demo -> new dialog
	case Key::Space:
		// this handler does not use the event args at all so this is fine :)
		// though maybe a bit hackish...
		//    demoNewDialog(e);
		break;

		// no hotkey found? event not used...
	default:
		return false;
	}

	return true;
}


//---------------------------------------------------------------------------------------------
void GUIManager::configureBuildSliders(CEGUI::Window* pParent)
{
	// Recursively subscribe every Build Panel slider to it's callback
	size_t childCount = pParent->getChildCount(); 
	for(size_t childIdx = 0; childIdx < childCount; childIdx++) 
	{ 
		if(pParent->getChildAtIdx(childIdx)->testClassName("Slider"))
		{ 
			pParent->getChildAtIdx(childIdx)->subscribeEvent(CEGUI::Slider::EventValueChanged, CEGUI::Event::Subscriber(&GUIManager::handleBuildSliders, this)); 			
		} 
		configureBuildSliders(pParent->getChildAtIdx(childIdx)); 
	} 
}

//---------------------------------------------------------------------------------------------
void GUIManager::configureDebugRB(CEGUI::Window* pParent)
{
	// Recursively subscribe every menu item to the mouse enters/leaves/clicked events
	size_t childCount = pParent->getChildCount(); 
	for(size_t childIdx = 0; childIdx < childCount; childIdx++) 
	{ 
		if(pParent->getChildAtIdx(childIdx)->testClassName("RadioButton"))
		{ 
			pParent->getChildAtIdx(childIdx)->subscribeEvent(CEGUI::RadioButton::EventSelectStateChanged, CEGUI::Event::Subscriber(&GUIManager::handleDebugRB, this)); 
			if(!m_sample->valid[pParent->getChildAtIdx(childIdx)->getID()])
			{
				pParent->getChildAtIdx(childIdx)->setEnabled(false);
				pParent->getChildAtIdx(childIdx)->setAlpha(0.25f);
			}
			else
			{
				pParent->getChildAtIdx(childIdx)->setEnabled(true);
			}
		} 
		configureDebugRB(pParent->getChildAtIdx(childIdx)); 
	} 
}

//---------------------------------------------------------------------------------------------
void GUIManager::configureDebugRBEx(CEGUI::Window* pParent)
{
	// Recursively subscribe every menu item to the mouse enters/leaves/clicked events
	size_t childCount = pParent->getChildCount(); 
	for(size_t childIdx = 0; childIdx < childCount; childIdx++) 
	{ 
		if(pParent->getChildAtIdx(childIdx)->testClassName("RadioButton"))
		{ 			
			if(!m_sample->valid[pParent->getChildAtIdx(childIdx)->getID()])
			{
				pParent->getChildAtIdx(childIdx)->setEnabled(false);
				pParent->getChildAtIdx(childIdx)->setAlpha(0.25f);
			}
			else
			{
				pParent->getChildAtIdx(childIdx)->setEnabled(true);
				pParent->getChildAtIdx(childIdx)->setAlpha(1.00f);
			}
		} 
		configureDebugRBEx(pParent->getChildAtIdx(childIdx)); 
	} 
}

//---------------------------------------------------------------------------------------------
void GUIManager::configureNavTestRB(CEGUI::Window* pParent)
{
	// Recursively subscribe every Build Panel slider to it's callback
	size_t childCount = pParent->getChildCount(); 
	for(size_t childIdx = 0; childIdx < childCount; childIdx++) 
	{ 
		if(pParent->getChildAtIdx(childIdx)->testClassName("RadioButton"))
		{ 
			pParent->getChildAtIdx(childIdx)->subscribeEvent(CEGUI::RadioButton::EventSelectStateChanged, CEGUI::Event::Subscriber(&GUIManager::handleNavMeshTestRB, this)); 			
		} 
		configureNavTestRB(pParent->getChildAtIdx(childIdx)); 
	} 
}

//---------------------------------------------------------------------------------------------
void GUIManager::configureNavTestCB(CEGUI::Window* pParent)
{
	// Recursively subscribe every Build Panel slider to it's callback
	size_t childCount = pParent->getChildCount(); 
	for(size_t childIdx = 0; childIdx < childCount; childIdx++) 
	{ 
		if(pParent->getChildAtIdx(childIdx)->testClassName("Checkbox"))
		{ 
			pParent->getChildAtIdx(childIdx)->subscribeEvent(CEGUI::Checkbox::EventCheckStateChanged, CEGUI::Event::Subscriber(&GUIManager::handleNavMeshTestCB, this)); 			
		} 
		configureNavTestCB(pParent->getChildAtIdx(childIdx)); 
	} 
}

//---------------------------------------------------------------------------------------------
void GUIManager::configureConvexVolumeRB(CEGUI::Window* pParent)
{
	// Recursively subscribe every Build Panel slider to it's callback
	size_t childCount = pParent->getChildCount(); 
	for(size_t childIdx = 0; childIdx < childCount; childIdx++) 
	{ 
		if(pParent->getChildAtIdx(childIdx)->testClassName("RadioButton"))
		{ 
			pParent->getChildAtIdx(childIdx)->subscribeEvent(CEGUI::RadioButton::EventSelectStateChanged, CEGUI::Event::Subscriber(&GUIManager::handleConvexVolumeRB, this)); 			
		} 
		configureNavTestRB(pParent->getChildAtIdx(childIdx)); 
	} 
}

//---------------------------------------------------------------------------------------------
void GUIManager::configureOptionsSkyboxRB(CEGUI::Window* pParent)
{
	// Recursively subscribe every Build Panel slider to it's callback
	size_t childCount = pParent->getChildCount(); 
	for(size_t childIdx = 0; childIdx < childCount; childIdx++) 
	{ 
		if(pParent->getChildAtIdx(childIdx)->testClassName("RadioButton"))
		{ 
			pParent->getChildAtIdx(childIdx)->subscribeEvent(CEGUI::RadioButton::EventSelectStateChanged, CEGUI::Event::Subscriber(&GUIManager::handleOptionsSkyboxRB, this)); 			
		} 
		configureNavTestRB(pParent->getChildAtIdx(childIdx)); 
	} 
}

//---------------------------------------------------------------------------------------------
void GUIManager::configureOptionsPolygonModeRB(CEGUI::Window* pParent)
{
	// Recursively subscribe every Build Panel slider to it's callback
	size_t childCount = pParent->getChildCount(); 
	for(size_t childIdx = 0; childIdx < childCount; childIdx++) 
	{ 
		if(pParent->getChildAtIdx(childIdx)->testClassName("RadioButton"))
		{ 
			pParent->getChildAtIdx(childIdx)->subscribeEvent(CEGUI::RadioButton::EventSelectStateChanged, CEGUI::Event::Subscriber(&GUIManager::handleOptionsPolygonModeRB, this)); 			
		} 
		configureNavTestRB(pParent->getChildAtIdx(childIdx)); 
	} 
}

//---------------------------------------------------------------------------------------------
void GUIManager::configureOptionsTextureFilterRB(CEGUI::Window* pParent)
{
	// Recursively subscribe every Build Panel slider to it's callback
	size_t childCount = pParent->getChildCount(); 
	for(size_t childIdx = 0; childIdx < childCount; childIdx++) 
	{ 
		if(pParent->getChildAtIdx(childIdx)->testClassName("RadioButton"))
		{ 
			pParent->getChildAtIdx(childIdx)->subscribeEvent(CEGUI::RadioButton::EventSelectStateChanged, CEGUI::Event::Subscriber(&GUIManager::handleOptionsTextureFilteringRB, this)); 			
		} 
		configureNavTestRB(pParent->getChildAtIdx(childIdx)); 
	} 
}

//---------------------------------------------------------------------------------------------
void GUIManager::configureConvexSliders(CEGUI::Window* pParent)
{
	// Recursively subscribe every Build Panel slider to it's callback
	size_t childCount = pParent->getChildCount(); 
	for(size_t childIdx = 0; childIdx < childCount; childIdx++) 
	{ 
		if(pParent->getChildAtIdx(childIdx)->testClassName("Slider"))
		{ 
			pParent->getChildAtIdx(childIdx)->subscribeEvent(CEGUI::Slider::EventValueChanged, CEGUI::Event::Subscriber(&GUIManager::handleConvexVolumeSliders, this)); 			
		} 
		configureBuildSliders(pParent->getChildAtIdx(childIdx)); 
	} 
}


//---------------------------------------------------------------------------------------------
void GUIManager::configureFrameWindowMouseEvents(CEGUI::Window* pParent)
{
	size_t childCount = pParent->getChildCount(); 
	for(size_t childIdx = 0; childIdx < childCount; childIdx++) 
	{
		pParent->getChildAtIdx(childIdx)->subscribeEvent(CEGUI::Window::EventMouseEnters, CEGUI::Event::Subscriber(&GUIManager::handleMouseEntersFrame, this)); 			
		pParent->getChildAtIdx(childIdx)->subscribeEvent(CEGUI::Window::EventMouseLeaves, CEGUI::Event::Subscriber(&GUIManager::handleMouseLeavesFrame, this)); 			

		configureFrameWindowMouseEvents(pParent->getChildAtIdx(childIdx)); 
	}
}


//---------------------------------------------------------------------------------------------
void GUIManager::resetNavTestCB(CEGUI::Window* pParent, bool _selected)
{
	// Recursively subscribe every Build Panel slider to it's callback
	size_t childCount = pParent->getChildCount(); 
	for(size_t childIdx = 0; childIdx < childCount; childIdx++) 
	{ 
		if(pParent->getChildAtIdx(childIdx)->testClassName("Checkbox"))
		{ 
			static_cast<CEGUI::Checkbox*>(pParent->getChildAtIdx(childIdx))->setSelected(_selected);
		} 
		resetNavTestCB(pParent->getChildAtIdx(childIdx), _selected); 
	} 
}

//---------------------------------------------------------------------------------------------
void GUIManager::resetNavTestRB(CEGUI::Window* pParent, bool _selected)
{
	// Recursively subscribe every Build Panel slider to it's callback
	size_t childCount = pParent->getChildCount(); 
	for(size_t childIdx = 0; childIdx < childCount; childIdx++) 
	{ 
		if(pParent->getChildAtIdx(childIdx)->testClassName("RadioButton"))
		{ 
			static_cast<CEGUI::Checkbox*>(pParent->getChildAtIdx(childIdx))->setSelected(_selected);
		} 
		resetNavTestRB(pParent->getChildAtIdx(childIdx), _selected); 
	} 
}

//---------------------------------------------------------------------------------------------
void GUIManager::hideAllTools(void)
{
	using namespace CEGUI;

	CEGUI::RadioButton* rb = static_cast<RadioButton*>(d_wm->getWindow("Root/ToolFrame/ToolTypeHolder/TestHolder/RBNavMeshTest"));
	rb->setSelected(false);

	rb = static_cast<RadioButton*>(d_wm->getWindow("Root/ToolFrame/ToolTypeHolder/TestHolder/RBOffMeshConTest1"));
	rb->setSelected(false);

	rb = static_cast<RadioButton*>(d_wm->getWindow("Root/ToolFrame/ToolTypeHolder/TestHolder/RBConvexVolumeTool1"));
	rb->setSelected(false);

	rb = static_cast<RadioButton*>(d_wm->getWindow("Root/ToolFrame/ToolTypeHolder/RBTileTool"));
	rb->setSelected(false);

	//m_sample->setSampleToolType(TOOL_NONE);

	
	d_wm->getWindow("Root/NavTestFrame")->hide();
	d_wm->getWindow("Root/OffMeshTestFrame")->hide();
	d_wm->getWindow("Root/ConvexToolsFrame")->hide();
	d_wm->getWindow("Root/TileToolFrame")->hide();
	d_wm->getWindow("Root/NavTestEntityPanel")->hide();
	

	m_sample->setSampleToolType(TOOL_NONE);
}

//---------------------------------------------------------------------------------------------
bool GUIManager::hideOptionsWindow(const CEGUI::EventArgs& e)
{
	d_wm->getWindow("Root/GeneralOptFrame")->hide();
	static_cast<CEGUI::Checkbox*>(d_wm->getWindow("Root/ToolFrame/ViewOptionsHolder/CBShowGenOptions"))->setSelected(false);
	return true;
}


//---------------------------------------------------------------------------------------------
void GUIManager::setStatusText(const CEGUI::String& pText, const CEGUI::Window& pWindow)
{
	//pWindow.setText(pText);
}	

//---------------------------------------------------------------------------------------------
void GUIManager::setHelpWindowWithKey(void)
{
	if(d_wm->getWindow("Root/HelpWindowFrame")->isVisible())
	{
		d_wm->getWindow("Root/HelpWindowFrame")->hide();
	}
	else if(!d_wm->getWindow("Root/HelpWindowFrame")->isVisible())
	{
		d_wm->getWindow("Root/HelpWindowFrame")->show();
	}
}

//---------------------------------------------------------------------------------------------
void GUIManager::setupHelpTopics(void)
{
	using namespace CEGUI;

	// setup help topic titles
	CEGUI::String title1 = "Base Application Interface";
	CEGUI::String title2 = "Application Tool Notes";
	CEGUI::String title3 = "NavMesh Test Tool Notes";
	CEGUI::String title4 = "NavMesh Test Tool Entity Notes";
	CEGUI::String title5 = "ConvexVolume Tool Notes";
	CEGUI::String title6 = "Off-Mesh Connection Tool Notes";
	
	// setup help topic 1 - "Base Application Interface"
	CEGUI::String txt1 = "OgreRecast User Interface Help Notes\n \n";
	CEGUI::String txt2 = "Basic Controls\n  W - move camera forward\n  S - move camera backward \n";
	CEGUI::String txt3 = "  A - move camera left\n  D - move camera right\n \n  Right Mouse Button - adjust camera angle \n";
	CEGUI::String txt4 = "  Left Mouse Button  - select current interface option or use current tool\n";
	CEGUI::String txt5 = "  Shift Left Mouse Button - remove nearest Convex Volume/Off-Mesh Connection/NavMesh Tile as per tool selection\n \n";
	CEGUI::String txt6 = "  Left Mouse Button(NavMesh Test Tool) - Place path ending point and Recalc the path \n";
	CEGUI::String txt7 = "  Shift Left Mouse Button(NavMesh Test Tool) - Place path starting point \n \n";
	CEGUI::String txt8 = "  Space Bar(NavMesh Test Tool) - Step the Path in increments. See source code.";
	CEGUI::String text1 = (txt1 + txt2 + txt3 + txt4 + txt5 + txt6 + txt7 + txt8);

	GUIHelpTopic* mTopic1 = new GUIHelpTopic(title1);
	mTopic1->setTopicText(text1);
	mHelpTopicList.push_back(mTopic1);

	// setup help topic 2 - "Application Tool Notes"
	CEGUI::String btxt1 = "OgreRecast Application Tools Basic Notes\n \n ";
	CEGUI::String btxt2 = "All of the basic tools, that is the NavMesh Tester(without Entity), the ConvexVolume and the OffMesh Connection tool,\n";
	CEGUI::String btxt3 = " share most of their basic usage features, in that they are mouse input controlled. For specific help with\n";
	CEGUI::String btxt4 = " a tool please see the Tool's help topic.\n \n";
	CEGUI::String btxt5 = "Basic Tool Use\n \n";
	CEGUI::String btxt6 = " Left Click - Use primary tool property, place a point.\n";
	CEGUI::String btxt7 = " Shift-Left Click - Use secondary tool property, remove a point, place end point.\n \n";
	CEGUI::String btxt8 = "Basic tool functionality and options are controlled through the User Interface.";
	CEGUI::String text2 = (btxt1 + btxt2 + btxt3 + btxt4 + btxt5 + btxt6 + btxt7 + btxt8);

	GUIHelpTopic* mTopic2 = new GUIHelpTopic(title2);
	mTopic2->setTopicText(text2);
	mHelpTopicList.push_back(mTopic2);
	
	// setup help topic 3 - "NavMesh Test Tool Notes"
	CEGUI::String ctxt1 = "Navigation Mesh Testing Tool Notes\n \n";
	CEGUI::String ctxt2 = "This section will contain basic help for the navmesh tester tool.";
	CEGUI::String text3 = (ctxt1 + ctxt2);

	GUIHelpTopic* mTopic3 = new GUIHelpTopic(title3);
	mTopic3->setTopicText(text3);
	mHelpTopicList.push_back(mTopic3);
		
	// setup help topic 4 - "NavMesh Test Tool Entity Notes"
	CEGUI::String dtxt1 = "Navigation Mesh Entity Testing Tool Notes\n \n";
	CEGUI::String dtxt2 = "This section will contain basic help for the navmesh test entity panel";
	CEGUI::String text4 = (dtxt1 + dtxt2);

	GUIHelpTopic* mTopic4 = new GUIHelpTopic(title4);
	mTopic4->setTopicText(text4);
	mHelpTopicList.push_back(mTopic4);

	// setup help topic 5 - "ConvexVolume Tool Notes"
	CEGUI::String etxt1 = "Convex Volume Tool Notes\n \n";
	CEGUI::String etxt2 = "This section will contain basic help for the convex volume creation tool";
	CEGUI::String text5 = (etxt1 + etxt2);

	GUIHelpTopic* mTopic5 = new GUIHelpTopic(title5);
	mTopic5->setTopicText(text5);
	mHelpTopicList.push_back(mTopic5);

	// setup help topic 6 - "Off-Mesh Connection Tool Notes"
	CEGUI::String ftxt1 = "Off-Mesh Connection Tool Notes\n \n";
	CEGUI::String ftxt2 = "This section will contain basic help for the off mesh connection tool";
	CEGUI::String text6 = (ftxt1 + ftxt2);

	GUIHelpTopic* mTopic6 = new GUIHelpTopic(title6);
	mTopic6->setTopicText(text6);
	mHelpTopicList.push_back(mTopic6);
	
}

//---------------------------------------------------------------------------------------------
void GUIManager::setPresetRecastOriginal(void)
{
	using namespace CEGUI;

	m_sample->setBuildCellSize(1.0f);
	m_sample->setBuildCellHeight(0.6f);
	m_sample->setBuildAgentHeight(7.0f);
	m_sample->setBuildAgentRadius(5.0f);
	m_sample->setBuildAgentMaxClimb(2.0f);
	m_sample->setBuildAgentMaxSlope(60.0f);
	m_sample->setBuildRegionSize(50);
	m_sample->setBuildRegionMerge(20);
	m_sample->setBuildEdgeLength(12.0f);
	m_sample->setBuildEdgeError(1.3f);
	m_sample->setBuildVertPerPoly(6.0f);
	m_sample->setBuildSampleDist(6.0f);
	m_sample->setBuildSampleError(1.0f);
	m_sample->setBuildTileSize(64.0f);



	CEGUI::Slider* sl = static_cast<CEGUI::Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder1/SLCellSize"));
	sl->setCurrentValue(1.0f);
	Window* eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder1/EBCellSize"));
	eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));

	sl = static_cast<CEGUI::Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder2/SLCellHeight"));
	sl->setCurrentValue(0.6f);
	eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder2/EBCellHeight"));
	eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));

	sl = static_cast<CEGUI::Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder3/SLAgentHeight"));
	sl->setCurrentValue(7.0f);
	eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder3/EBAgentHeight"));
	eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));

	sl = static_cast<CEGUI::Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder4/SLAgentRadius"));
	sl->setCurrentValue(5.0f);
	eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder4/EBAgentRadius"));
	eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));

	sl = static_cast<CEGUI::Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder5/SLMaxClimb"));
	sl->setCurrentValue(2.0f);
	eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder5/EBMaxClimb"));
	eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));

	sl = static_cast<CEGUI::Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder6/SLMaxSlope"));
	sl->setCurrentValue(60.0f);
	eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder6/EBMaxSlope"));
	eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));

	sl = static_cast<CEGUI::Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder7/SLMinRegion"));
	sl->setCurrentValue(50.0f);
	eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder7/EBMinRegion"));
	eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));

	sl = static_cast<CEGUI::Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder8/SLMergedSize"));		
	sl->setCurrentValue(20.0f);
	eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder8/EBMergedSize"));
	eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));

	sl = static_cast<CEGUI::Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder9/SLMaxEdge"));		
	sl->setCurrentValue(12.0f);
	eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder9/EBMaxEdge"));
	eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));

	sl = static_cast<CEGUI::Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder10/SLMaxEdgeError"));		
	sl->setCurrentValue(1.3f);
	eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder10/EBMaxEdgeError"));
	eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));

	sl = static_cast<CEGUI::Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder11/SLVertPerPoly"));
	sl->setCurrentValue(6.0f);
	eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder11/EBVertPerPoly"));
	eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));

	sl = static_cast<CEGUI::Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder12/SLSampleDist"));
	sl->setCurrentValue(6.0f);
	eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder12/EBSampleDist"));
	eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));

	sl = static_cast<CEGUI::Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder13/SLMaxSampErr"));
	sl->setCurrentValue(1.0f);
	eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder13/EBMaxSampErr"));
	eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));

	sl = static_cast<CEGUI::Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder14/SLTileSize"));
	sl->setCurrentValue(64.0f);
	eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder14/EBTileSize"));
	eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));
}

//---------------------------------------------------------------------------------------------
void GUIManager::setPresetOgreTerrain(void)
{
	using namespace CEGUI;

	m_sample->setBuildCellSize(2.5f);
	m_sample->setBuildCellHeight(1.5f);
	m_sample->setBuildAgentHeight(15.0f);
	m_sample->setBuildAgentRadius(10.0f);
	m_sample->setBuildAgentMaxClimb(2.0f);
	m_sample->setBuildAgentMaxSlope(60.0f);
	m_sample->setBuildRegionSize(100);
	m_sample->setBuildRegionMerge(70);
	m_sample->setBuildEdgeLength(32.0f);
	m_sample->setBuildEdgeError(1.5f);
	m_sample->setBuildVertPerPoly(6.0f);
	m_sample->setBuildSampleDist(16.0f);
	m_sample->setBuildSampleError(2.0f);
	m_sample->setBuildTileSize(128.0f);

	CEGUI::Slider* sl = static_cast<CEGUI::Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder1/SLCellSize"));
	sl->setCurrentValue(2.5f);
	Window* eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder1/EBCellSize"));
	eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));

	sl = static_cast<CEGUI::Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder2/SLCellHeight"));
	sl->setCurrentValue(1.5f);
	eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder2/EBCellHeight"));
	eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));

	sl = static_cast<CEGUI::Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder3/SLAgentHeight"));
	sl->setCurrentValue(15.0f);
	eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder3/EBAgentHeight"));
	eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));

	sl = static_cast<CEGUI::Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder4/SLAgentRadius"));
	sl->setCurrentValue(10.0f);
	eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder4/EBAgentRadius"));
	eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));

	sl = static_cast<CEGUI::Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder5/SLMaxClimb"));
	sl->setCurrentValue(2.0f);
	eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder5/EBMaxClimb"));
	eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));

	sl = static_cast<CEGUI::Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder6/SLMaxSlope"));
	sl->setCurrentValue(60.0f);
	eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder6/EBMaxSlope"));
	eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));

	sl = static_cast<CEGUI::Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder7/SLMinRegion"));
	sl->setCurrentValue(100.0f);
	eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder7/EBMinRegion"));
	eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));

	sl = static_cast<CEGUI::Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder8/SLMergedSize"));		
	sl->setCurrentValue(70.0f);
	eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder8/EBMergedSize"));
	eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));

	sl = static_cast<CEGUI::Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder9/SLMaxEdge"));		
	sl->setCurrentValue(32.0f);
	eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder9/EBMaxEdge"));
	eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));

	sl = static_cast<CEGUI::Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder10/SLMaxEdgeError"));		
	sl->setCurrentValue(1.5f);
	eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder10/EBMaxEdgeError"));
	eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));

	sl = static_cast<CEGUI::Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder11/SLVertPerPoly"));
	sl->setCurrentValue(6.0f);
	eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder11/EBVertPerPoly"));
	eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));

	sl = static_cast<CEGUI::Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder12/SLSampleDist"));
	sl->setCurrentValue(16.0f);
	eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder12/EBSampleDist"));
	eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));

	sl = static_cast<CEGUI::Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder13/SLMaxSampErr"));
	sl->setCurrentValue(2.0f);
	eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder13/EBMaxSampErr"));
	eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));

	sl = static_cast<CEGUI::Slider*>(d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder14/SLTileSize"));
	sl->setCurrentValue(128.0f);
	eb = (d_wm->getWindow("Root/BuildFrame/BuildSliderSPane/SliderHolder14/EBTileSize"));
	eb->setText(Ogre::StringConverter::toString(sl->getCurrentValue()));
}

//---------------------------------------------------------------------------------------------
// END OF FILE
//---------------------------------------------------------------------------------------------

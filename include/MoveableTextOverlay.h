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

// ORIGINAL CODE TAKEN FROM Ogre3D Wiki.

#ifndef __MovableTextOverlay_H__
#define __MovableTextOverlay_H__

#include "Ogre.h"
#include "OgreFont.h"
#include "OgreFontManager.h"

using namespace Ogre;

class MovableTextOverlayAttributes
{
public:
	MovableTextOverlayAttributes(const Ogre::String & name, const Ogre::Camera *cam,
		const Ogre::String & fontName = "EntityLabel", int charHeight = 16, const Ogre::ColourValue & color = Ogre::ColourValue::White, const Ogre::String & materialName = "");
	~MovableTextOverlayAttributes();

	void setFontName(const Ogre::String & fontName);
	void setMaterialName(const Ogre::String & materialName);
	void setColor(const Ogre::ColourValue & color);
	void setCharacterHeight(unsigned int height);

	const Ogre::String& getName() const {return mName;}
	const Ogre::Camera* getCamera() const {return mpCam;}
	const Ogre::Font* getFont() const {return mpFont;}
	const Ogre::String& getFontName() const {return mFontName;}
	const Ogre::String& getMaterialName() const {return mMaterialName;}
	const Ogre::ColourValue& getColor() const {return mColor;}
	const Ogre::Real getCharacterHeight() const {return mCharHeight;}

	const Ogre::String mName;
	const Ogre::Camera *mpCam;

	Ogre::Font* mpFont;
	Ogre::String mFontName;
	Ogre::String mMaterialName;
	Ogre::ColourValue mColor;
	Ogre::Real mCharHeight;
};

class MovableTextOverlay {
public:
	MovableTextOverlay(const Ogre::String & name, const Ogre::String & caption,
		const Ogre::MovableObject *mov, MovableTextOverlayAttributes *attrs);

	virtual ~MovableTextOverlay();

	void setCaption(const Ogre::String & caption);
	void setCaptionLines(int _captionNumLines, int _charsInLongestLine) 
	{ 
		if(_captionNumLines > 0 && _captionNumLines < 5)
			mCaptionLines = _captionNumLines;
		else
			mCaptionLines = 1;
		if(_charsInLongestLine > 2 && _charsInLongestLine < 25)
			mCharInLongestLine = _charsInLongestLine;
		else
			mCharInLongestLine = 10;

		mCaptionWidthCalc = "";
		for(unsigned int i = 0; i < mCharInLongestLine; ++i)
		{
			mCaptionWidthCalc += "X";
		}
	}
	void setUpdateFrequency(Ogre::Real updateFrequency) {mUpdateFrequency = updateFrequency;}
	void setAttributes(MovableTextOverlayAttributes *attrs)
	{
		mAttrs = attrs;
		_updateOverlayAttrs();
	}

	const Ogre::String&	getName() const {return mName;}
	const Ogre::String&	getCaption() const {return mCaption;}
	const Ogre::Real getUpdateFrequency() const {return mUpdateFrequency;}
	const bool isOnScreen() const {return mOnScreen;}
	const bool isEnabled() const {return mEnabled;}
	const MovableTextOverlayAttributes* getAttributes() const {return mAttrs;}

	void enable(bool enable);
	void update(Ogre::Real timeSincelastFrame);

	// Needed for RectLayoutManager.
	int getPixelsTop() {return Ogre::OverlayManager::getSingleton().getViewportHeight() * (mpOvContainer->getTop());}
	int getPixelsBottom() {return Ogre::OverlayManager::getSingleton().getViewportHeight() * (mpOvContainer->getTop() + mpOvContainer->getHeight());}
	int getPixelsLeft() {return Ogre::OverlayManager::getSingleton().getViewportWidth() * mpOvContainer->getLeft();}
	int getPixelsRight() {return Ogre::OverlayManager::getSingleton().getViewportWidth() * (mpOvContainer->getLeft() + mpOvContainer->getWidth());}

	void setPixelsTop(int px) {mpOvContainer->setTop((Ogre::Real)px / Ogre::OverlayManager::getSingleton().getViewportHeight());}
	// end

protected:
	void _computeTextWidth();
	void _updateOverlayAttrs();
	void _getMinMaxEdgesOfTopAABBIn2D(Ogre::Real& MinX, Ogre::Real& MinY, Ogre::Real& MaxX, Ogre::Real& MaxY);
	void _getScreenCoordinates(const Ogre::Vector3& position, Ogre::Real& x, Ogre::Real& y);

	const Ogre::String mName;
	const Ogre::MovableObject* mpMov;

	Ogre::Overlay* mpOv;
	Ogre::OverlayContainer* mpOvContainer;
	Ogre::OverlayElement* mpOvText;

	// true if mpOvContainer is visible, false otherwise
	bool mEnabled;

	// true if mTextWidth needs to be recalculated
	bool mNeedUpdate;

	// Text width in pixels
	Ogre::Real mTextWidth;

	// the Text
	Ogre::String mCaption;
	// the text used to calculate the width, this should be equal to the longest line in the
	// caption
	Ogre::String mCaptionWidthCalc;

	// the number of lines the caption contains - split a single string with \n escape
	// characters for multiple lines
	int mCaptionLines;
	int mCharInLongestLine;

	// true if the upper vertices projections of the MovableObject are on screen
	bool mOnScreen;

	// the update frequency in seconds
	// mpOvContainer coordinates get updated each mUpdateFrequency seconds.
	Ogre::Real mUpdateFrequency;

	// the Font/Material/Color text attributes
	MovableTextOverlayAttributes *mAttrs;
};
#endif /* __MovableTextOverlay_H__ */
// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "UIButton.h"

#include "UISkin.h"
#include "UIFont.h"

#include "UserInterface.h"

#include "Graphic/Renderer/Renderer.h"
#include "Core/OS/OS.h"

//! constructor
UIButton::UIButton(BaseUI* ui, int id, RectangleBase<2, int> rectangle)
:	BaseUIButton(id, rectangle), mSpriteBank(0), mOverrideFont(0), mUI(ui),
	mImage(0), mPressedImage(0), mClickTime(0), mHoverTime(0), mFocusTime(0), mPushButton(false), 
	mPressed(false), mUseAlphaChannel(false), mDrawBorder(true), mScaleImage(false)
{
	#ifdef _DEBUG
	//setDebugName("GUIButton");
	#endif
}


//! destructor
UIButton::~UIButton()
{

}

//! initialize button
void UIButton::OnInit(bool noclip)
{
	SetNotClipped(noclip);

	// Initialize the sprites.
	for (unsigned int i=0; i<BS_COUNT; ++i)
		mButtonSprites[i].Index = -1;
	
	// This element can be tabbed.
	SetTabStop(true);
	SetTabOrder(-1);
}

//! Sets if the images should be scaled to fit the button
void UIButton::SetScaleImage(bool scaleImage)
{
	mScaleImage = scaleImage;
}


//! Returns whether the button scale the used images
bool UIButton::IsScalingImage() const
{
	return mScaleImage;
}


//! Sets if the button should use the skin to draw its border
void UIButton::SetDrawBorder(bool border)
{
	mDrawBorder = border;
}


void UIButton::SetSpriteBank(const eastl::shared_ptr<BaseUISpriteBank>& sprites)
{
	mSpriteBank = sprites;
}


void UIButton::SetSprite(UIButtonState state, int index, eastl::array<float, 4> color, bool loop)
{
	if (mSpriteBank)
	{
		mButtonSprites[(unsigned int)state].Index	= index;
		mButtonSprites[(unsigned int)state].Color	= color;
		mButtonSprites[(unsigned int)state].Loop	= loop;
	}
	else
	{
		mButtonSprites[(unsigned int)state].Index = -1;
	}
}


//! called if an event happened.
bool UIButton::OnEvent(const Event& ev)
{
	if (!IsEnabled())
		return BaseUIElement::OnEvent(ev);

	switch(ev.mEventType)
	{
	case ET_KEY_INPUT_EVENT:
		if (ev.mKeyInput.mPressedDown &&
			(ev.mKeyInput.mKey == KEY_RETURN || 
			ev.mKeyInput.mKey == KEY_SPACE))
		{
			if (!mPushButton)
				SetPressed(true);
			else
				SetPressed(!mPressed);

			return true;
		}
		if (mPressed && !mPushButton && 
			ev.mKeyInput.mPressedDown && 
			ev.mKeyInput.mKey == KEY_ESCAPE)
		{
			SetPressed(false);
			return true;
		}
		else
		if (!ev.mKeyInput.mPressedDown && mPressed &&
			(ev.mKeyInput.mKey == KEY_RETURN || 
			ev.mKeyInput.mKey == KEY_SPACE))
		{

			if (!mPushButton)
				SetPressed(false);

			if (mParent)
			{
				Event newEvent;
				newEvent.mEventType = ET_UI_EVENT;
				newEvent.mUIEvent.mCaller = this;
				newEvent.mUIEvent.mElement = 0;
				newEvent.mUIEvent.mEventType = UIEVT_BUTTON_CLICKED;
				mParent->OnEvent(newEvent);
			}
			return true;
		}
		break;
	case ET_UI_EVENT:
		if (ev.mUIEvent.mCaller == this)
		{
			if (ev.mUIEvent.mEventType == UIEVT_ELEMENT_FOCUS_LOST)
			{
				if (!mPushButton)
					SetPressed(false);
				mFocusTime = Timer::GetTime();
			}
			else if (ev.mUIEvent.mEventType == UIEVT_ELEMENT_FOCUSED)
			{
				mFocusTime = Timer::GetTime();
			}
			else if (ev.mUIEvent.mEventType == UIEVT_ELEMENT_HOVERED || 
					ev.mUIEvent.mEventType == UIEVT_ELEMENT_LEFT)
			{
				mHoverTime = Timer::GetTime();
			}
		}
		break;
	case ET_MOUSE_INPUT_EVENT:
		if (ev.mMouseInput.mEvent == MIE_LMOUSE_PRESSED_DOWN)
		{	/*
			if (mUI->HasFocus(shared_from_this()) &&
				!mAbsoluteClippingRect.IsPointInside(
					Vector2<int>{ev.mMouseInput.X, ev.mMouseInput.Y}))
			{
				mUI->RemoveFocus(shared_from_this());
				return false;
			}
			*/
			if (!mPushButton)
				SetPressed(true);

			mUI->SetFocus(shared_from_this());
			return true;
		}
		else
		if (ev.mMouseInput.mEvent == MIE_LMOUSE_LEFT_UP)
		{
			bool wasPressed = mPressed;
			/*
			if (!mAbsoluteClippingRect.IsPointInside(
				Vector2<int>{ev.mMouseInput.X, ev.mMouseInput.Y }))
			{
				if (!mPushButton)
					SetPressed(false);
				return true;
			}
			*/
			if (!mPushButton)
				SetPressed(false);
			else
				SetPressed(!mPressed);

			if ((!mPushButton && wasPressed && mParent) ||
				(mPushButton && wasPressed != mPressed))
			{
				Event newEvent;
				newEvent.mEventType = ET_UI_EVENT;
				newEvent.mUIEvent.mCaller = this;
				newEvent.mUIEvent.mElement = 0;
				newEvent.mUIEvent.mEventType = UIEVT_BUTTON_CLICKED;
				mParent->OnEvent(newEvent);
			}

			return true;
		}
		break;
	default:
		break;
	}

	return mParent ? mParent->OnEvent(ev) : false;
}


//! draws the element and its children
void UIButton::Draw( )
{
	if (!mVisible)
		return;

	const eastl::shared_ptr<BaseUISkin>& skin = mUI->GetSkin();

	// todo:	move sprite up and text down if the pressed state has a sprite
	const Vector2<int> spritePos = mAbsoluteRect.center;

	if (!mPressed)
	{
		if (mDrawBorder)
			skin->Draw3DButtonPaneStandard(
				shared_from_this(), mAbsoluteRect, &mAbsoluteClippingRect);

		if (mImage)
		{
			Vector2<int> pos = spritePos;
			pos[0] -= mImageRect.extent[0]/ 2;
			pos[1] -= mImageRect.extent[1] / 2;
			/*
			Renderer::Get()->Draw2DImage(mImage.get(), mScaleImage ? mAbsoluteRect : 
				RectangleBase<2, int>{ pos, mImageRect.axis, mImageRect.extent },
				mImageRect, &mAbsoluteClippingRect, 0, mUseAlphaChannel);
			*/
		}
	}
	else
	{
		if (mDrawBorder)
			skin->Draw3DButtonPanePressed(shared_from_this(), mAbsoluteRect, &mAbsoluteClippingRect);

		if (mPressedImage)
		{
			Vector2<int> pos = spritePos;
			pos[0] -= mPressedImageRect.extent[0] / 2;
			pos[1] -= mPressedImageRect.extent[1] / 2;

			if (mImage == mPressedImage && mPressedImageRect == mImageRect)
			{
				pos[0] += skin->GetSize(DS_BUTTON_PRESSED_IMAGE_OFFSET_X);
				pos[1] += skin->GetSize(DS_BUTTON_PRESSED_IMAGE_OFFSET_Y);
			}
			/*
			Renderer::Get()->Draw2DImage(mPressedImage.get(), mScaleImage ? mAbsoluteRect : 
				RectangleBase<2, int>{ pos, mPressedImageRect.axis, mPressedImageRect.extent },
				mPressedImageRect, &mAbsoluteClippingRect, 0, mUseAlphaChannel);
			*/
		}
	}

	if (mSpriteBank)
	{
		// pressed / unpressed animation
		unsigned int state = mPressed ? (unsigned int)BS_BUTTON_DOWN : (unsigned int)BS_BUTTON_UP;
		if (mButtonSprites[state].Index != -1)
		{
			/*
			mSpriteBank->Draw2DSprite(mButtonSprites[state].Index, spritePos,
			 	&mAbsoluteClippingRect, mButtonSprites[state].Color, mClickTime, Timer::GetTime(),
				mButtonSprites[state].Loop, true);
			*/
		}

		// Focused / unFocused animation
		state = mUI->HasFocus(shared_from_this()) ? 
			(unsigned int)BS_BUTTON_Focused : (unsigned int)BS_BUTTON_NOT_Focused;
		if (mButtonSprites[state].Index != -1)
		{
			/*
			mSpriteBank->Draw2DSprite(mButtonSprites[state].Index, spritePos,
			 	&mAbsoluteClippingRect, mButtonSprites[state].Color, mFocusTime, Timer::GetTime(),
				mButtonSprites[state].Loop, true);
			*/
		}

		// mouse over / off animation
		if (IsEnabled())
		{
			state = mUI->GetHovered() == shared_from_this() ? 
				(unsigned int)BS_BUTTON_MOUSE_OVER : (unsigned int)BS_BUTTON_MOUSE_OFF;
			if (mButtonSprites[state].Index != -1)
			{
				/*
				mSpriteBank->Draw2DSprite(mButtonSprites[state].Index, spritePos,
				 	&mAbsoluteClippingRect, mButtonSprites[state].Color, mHoverTime, Timer::GetTime(),
					mButtonSprites[state].Loop, true);
				*/
			}
		}
	}

	if (mText.size())
	{
		const eastl::shared_ptr<BaseUIFont>& font = GetActiveFont();

		RectangleBase<2, int> rect = mAbsoluteRect;
		if (mPressed)
		{
			rect.center[0] += skin->GetSize(DS_BUTTON_PRESSED_TEXT_OFFSET_X);
			rect.center[1] += skin->GetSize(DS_BUTTON_PRESSED_TEXT_OFFSET_Y);
		}

		if (font)
			font->Draw(mText.c_str(), rect,
				skin->GetColor(IsEnabled() ? DC_BUTTON_TEXT : DC_GRAY_TEXT),
				true, true, &mAbsoluteClippingRect);
	}

	BaseUIElement::Draw();
}


//! sets another skin independent font. if this is set to zero, the button uses the font of the skin.
void UIButton::SetOverrideFont(const eastl::shared_ptr<BaseUIFont>& font)
{
	if (mOverrideFont == font)
		return;

	mOverrideFont = font ;
}

//! Gets the override font (if any)
const eastl::shared_ptr<BaseUIFont>& UIButton::GetOverrideFont() const
{
	return mOverrideFont;
}

//! Get the font which is used right now for drawing
eastl::shared_ptr<BaseUIFont> UIButton::GetActiveFont() const
{
	if ( mOverrideFont )
		return mOverrideFont;
	
	return eastl::shared_ptr<BaseUIFont>();// BaseUIElement::GetFont();
}

//! Sets an image which should be displayed on the button when it is in normal state.
void UIButton::SetImage(const eastl::shared_ptr<Texture>& image)
{
	mImage = image;

	if (image)
	{
		mImageRect = RectangleBase<2, int>();
		mImageRect.extent[0] = image->GetDimension(0);
		mImageRect.extent[1] = image->GetDimension(1);
	}

	if (!mPressedImage)
		SetPressedImage(mImage);
}


//! Sets the image which should be displayed on the button when it is in its normal state.
void UIButton::SetImage(const eastl::shared_ptr<Texture>& image, const RectangleBase<2, int>& pos)
{
	SetImage(image);
	mImageRect = pos;
}


//! Sets an image which should be displayed on the button when it is in pressed state.
void UIButton::SetPressedImage(const eastl::shared_ptr<Texture>& image)
{
	mPressedImage = image;

	if (image)
	{
		mPressedImageRect = RectangleBase<2, int>();
		mPressedImageRect.extent[0] = image->GetDimension(0);
		mPressedImageRect.extent[1] = image->GetDimension(1);
	}
}


//! Sets the image which should be displayed on the button when it is in its pressed state.
void UIButton::SetPressedImage(const eastl::shared_ptr<Texture>& image, const RectangleBase<2, int>& pos)
{
	SetPressedImage(image);
	mPressedImageRect = pos;
}


//! Sets if the button should behave like a push button. Which means it
//! can be in two states: Normal or Pressed. With a click on the button,
//! the user can change the state of the button.
void UIButton::SetPushButton(bool pushButton)
{
	mPushButton = pushButton;
}


//! Returns if the button is currently pressed
bool UIButton::IsPressed() const
{
	return mPressed;
}


//! Sets the pressed state of the button if this is a pushbutton
void UIButton::SetPressed(bool pressed)
{
	if (mPressed != pressed)
	{
		mClickTime = Timer::GetTime();
		mPressed = pressed;
	}
}


//! Returns whether the button is a push button
bool UIButton::IsPushButton() const
{
	return mPushButton;
}


//! Sets if the alpha channel should be used for drawing images on the button (default is false)
void UIButton::SetUseAlphaChannel(bool useAlphaChannel)
{
	mUseAlphaChannel = useAlphaChannel;
}


//! Returns if the alpha channel should be used for drawing images on the button
bool UIButton::IsAlphaChannelUsed() const
{
	return mUseAlphaChannel;
}


bool UIButton::IsDrawingBorder() const
{
	return mDrawBorder;
}
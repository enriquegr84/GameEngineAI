// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "UIStaticText.h"

#include "UISkin.h"
#include "UIFont.h"

#include "Graphic/Renderer/Renderer.h"
#include "UserInterface.h"

//! constructor
UIStaticText::UIStaticText(BaseUI* ui, int id, const wchar_t* text, 
	bool border, const RectangleBase<2, int>& rectangle, bool background)
:	BaseUIStaticText(id, rectangle), mHAlign(UIA_UPPERLEFT), mVAlign(UIA_UPPERLEFT), 
	mUI(ui), mBorder(border), mOverrideColorEnabled(false), mOverrideBGColorEnabled(false), 
	mWordWrap(false), mBackground(background), mRestrainTextInside(true), mRightToLeft(false), 
	mOverrideColor(eastl::array<float, 4>{101 / 255.f, 255 / 255.f, 255 / 255.f, 255 / 255.f}), 
	mBGColor(eastl::array<float, 4>{101 / 255.f, 210 / 255.f, 210 / 255.f, 210 / 255.f}),
	mOverrideFont(0), mLastBreakFont(0)
{
	#ifdef _DEBUG
	//setDebugName("GUIStaticText");
	#endif

	mText = text;
	if (mUI && mUI->GetSkin())
	{
		mBGColor = mUI->GetSkin()->GetColor(DC_3D_FACE);
	}
}


//! destructor
UIStaticText::~UIStaticText()
{
}


//! draws the element and its children
void UIStaticText::Draw( )
{
	if (!mVisible)
		return;

	const eastl::shared_ptr<BaseUISkin>& skin = mUI->GetSkin();
	if (!skin)
		return;

	RectangleBase<2, int> frameRect(mAbsoluteRect);

	// draw background

	if (mBackground)
	{
		if ( !mOverrideBGColorEnabled )	// skin-colors can change
			mBGColor = skin->GetColor(DC_3D_FACE);

		//Renderer::Get()->Draw2DRectangle(mBGColor, frameRect, &mAbsoluteClippingRect);
	}

	// draw the border

	if (mBorder)
	{
		skin->Draw3DSunkenPane(shared_from_this(), eastl::array<float, 4>(), true, false, frameRect, &mAbsoluteClippingRect);
		frameRect.extent[0] += skin->GetSize(DS_TEXT_DISTANCE_X);
	}

	// draw the text
	if (mText.size())
	{
		const eastl::shared_ptr<BaseUIFont>& font = GetActiveFont();

		if (font)
		{
			if (!mWordWrap)
			{
				if (mVAlign == UIA_LOWERRIGHT)
				{
					frameRect.extent[1] = frameRect.extent[1] -
						font->GetDimension(L"A")[1] - font->GetKerningHeight();
				}
				if (mHAlign == UIA_LOWERRIGHT)
				{
					frameRect.extent[0] = frameRect.extent[0] -
						font->GetDimension(mText.c_str())[0];
				}

				font->Draw(mText.c_str(), frameRect,
					mOverrideColorEnabled ? mOverrideColor : skin->GetColor(IsEnabled() ? DC_BUTTON_TEXT : DC_GRAY_TEXT),
					mHAlign == UIA_CENTER, mVAlign == UIA_CENTER, (mRestrainTextInside ? &mAbsoluteClippingRect : NULL));
			}
			else
			{
				if (font != mLastBreakFont)
					BreakText();

				RectangleBase<2, int> r = frameRect;
				int height = font->GetDimension(L"A")[1] + font->GetKerningHeight();
				int totalHeight = height * mBrokenText.size();
				if (mVAlign == UIA_CENTER)
				{
					r.extent[1] = r.center[1] - (totalHeight / 2);
				}
				else if (mVAlign == UIA_LOWERRIGHT)
				{
					r.extent[1] = r.extent[1] - totalHeight;
				}

				for (unsigned int i=0; i<mBrokenText.size(); ++i)
				{
					if (mHAlign == UIA_LOWERRIGHT)
					{
						r.extent[0] = frameRect.extent[0] -
							font->GetDimension(mBrokenText[i].c_str())[0];
					}

					font->Draw(mBrokenText[i].c_str(), r,
						mOverrideColorEnabled ? mOverrideColor : skin->GetColor(IsEnabled() ? DC_BUTTON_TEXT : DC_GRAY_TEXT),
						mHAlign == UIA_CENTER, false, (mRestrainTextInside ? &mAbsoluteClippingRect : NULL));

					r.extent[1] += height;
					r.extent[1] += height;
				}
			}
		}
	}

	BaseUIElement::Draw();
}


//! Sets another skin independent font.
void UIStaticText::SetOverrideFont(const eastl::shared_ptr<BaseUIFont>& font)
{
	if (mOverrideFont == font)
		return;

	mOverrideFont = font;

	BreakText();
}

//! Gets the override font (if any)
const eastl::shared_ptr<BaseUIFont>& UIStaticText::GetOverrideFont() const
{
	return mOverrideFont;
}

//! Get the font which is used right now for drawing
eastl::shared_ptr<BaseUIFont> UIStaticText::GetActiveFont() const
{
	if ( mOverrideFont )
		return mOverrideFont;
	
	return eastl::shared_ptr<BaseUIFont>();
}

//! Sets another color for the text.
void UIStaticText::SetOverrideColor(eastl::array<float, 4> color)
{
	mOverrideColor = color;
	mOverrideColorEnabled = true;
}


//! Sets another color for the text.
void UIStaticText::SetBackgroundColor(eastl::array<float, 4> color)
{
	mBGColor = color;
	mOverrideBGColorEnabled = true;
	mBackground = true;
}


//! Sets whether to draw the background
void UIStaticText::SetDrawBackground(bool draw)
{
	mBackground = draw;
}


//! Gets the background color
eastl::array<float, 4> UIStaticText::GetBackgroundColor() const
{
	return mBGColor;
}


//! Checks if background drawing is enabled
bool UIStaticText::IsDrawBackgroundEnabled() const
{
	return mBackground;
}


//! Sets whether to draw the border
void UIStaticText::SetDrawBorder(bool draw)
{
	mBorder = draw;
}


//! Checks if border drawing is enabled
bool UIStaticText::IsDrawBorderEnabled() const
{
	return mBorder;
}


void UIStaticText::SetTextRestrainedInside(bool restrainTextInside)
{
	mRestrainTextInside = restrainTextInside;
}


bool UIStaticText::IsTextRestrainedInside() const
{
	return mRestrainTextInside;
}


void UIStaticText::SetTextAlignment(UIAlignment horizontal, UIAlignment vertical)
{
	mHAlign = horizontal;
	mVAlign = vertical;
}


eastl::array<float, 4> UIStaticText::GetOverrideColor() const
{
	return mOverrideColor;
}


//! Sets if the static text should use the overide color or the
//! color in the gui skin.
void UIStaticText::EnableOverrideColor(bool enable)
{
	mOverrideColorEnabled = enable;
}


bool UIStaticText::IsOverrideColorEnabled() const
{
	return mOverrideColorEnabled;
}


//! Enables or disables word wrap for using the static text as
//! multiline text control.
void UIStaticText::SetWordWrap(bool enable)
{
	mWordWrap = enable;
	BreakText();
}


bool UIStaticText::IsWordWrapEnabled() const
{
	return mWordWrap;
}


void UIStaticText::SetRightToLeft(bool rtl)
{
	if (mRightToLeft != rtl)
	{
		mRightToLeft = rtl;
		BreakText();
	}
}


bool UIStaticText::IsRightToLeft() const
{
	return mRightToLeft;
}


//! Breaks the single text line.
void UIStaticText::BreakText()
{
	if (!mWordWrap)
		return;

	mBrokenText.clear();

	const eastl::shared_ptr<BaseUISkin>& skin = mUI->GetSkin();
	const eastl::shared_ptr<BaseUIFont>& font = GetActiveFont();
	if (!font)
		return;

	mLastBreakFont = font;

	eastl::wstring line;
	eastl::wstring word;
	eastl::wstring whitespace;
	int size = mText.size();
	int length = 0;
	int elWidth = mRelativeRect.extent[0];
	if (mBorder)
		elWidth -= 2*skin->GetSize(DS_TEXT_DISTANCE_X);
	wchar_t c;

	// We have to deal with right-to-left and left-to-right differently
	// However, most parts of the following code is the same, it's just
	// some order and boundaries which change.
	if (!mRightToLeft)
	{
		// regular (left-to-right)
		for (int i=0; i<size; ++i)
		{
			c = mText[i];
			bool lineBreak = false;

			if (c == L'\r') // Mac or Windows breaks
			{
				lineBreak = true;
				if (mText[i+1] == L'\n') // Windows breaks
				{
					mText.erase(i+1);
					--size;
				}
				c = '\0';
			}
			else if (c == L'\n') // Unix breaks
			{
				lineBreak = true;
				c = '\0';
			}

			bool isWhitespace = (c == L' ' || c == 0);
			if ( !isWhitespace )
			{
				// part of a word
				word += c;
			}

			if ( isWhitespace || i == (size-1))
			{
				if (word.size())
				{
					// here comes the next whitespace, look if
					// we must break the last word to the next line.
					const int whitelgth = font->GetDimension(whitespace.c_str())[0];
					const int wordlgth = font->GetDimension(word.c_str())[0];

					if (wordlgth > elWidth)
					{
						// This word is too long to fit in the available space, look for
						// the Unicode Soft HYphen (SHY / 00AD) character for a place to
						// break the word at
						int where = word.find_first_of( wchar_t(0x00AD) );
						if (where != -1)
						{
							eastl::wstring first  = word.substr(0, where);
							eastl::wstring second = word.substr(where, word.size() - where);
							mBrokenText.push_back(line + first + L"-");
							const int secondLength = font->GetDimension(second.c_str())[0];

							length = secondLength;
							line = second;
						}
						else
						{
							// No soft hyphen found, so there's nothing more we can do
							// break to next line
							if (length)
								mBrokenText.push_back(line);
							length = wordlgth;
							line = word;
						}
					}
					else if (length && (length + wordlgth + whitelgth > elWidth))
					{
						// break to next line
						mBrokenText.push_back(line);
						length = wordlgth;
						line = word;
					}
					else
					{
						// add word to line
						line += whitespace;
						line += word;
						length += whitelgth + wordlgth;
					}

					word = L"";
					whitespace = L"";
				}

				if ( isWhitespace )
				{
					whitespace += c;
				}

				// compute line break
				if (lineBreak)
				{
					line += whitespace;
					line += word;
					mBrokenText.push_back(line);
					line = L"";
					word = L"";
					whitespace = L"";
					length = 0;
				}
			}
		}

		line += whitespace;
		line += word;
		mBrokenText.push_back(line);
	}
	else
	{
		// right-to-left
		for (int i=size; i>=0; --i)
		{
			c = mText[i];
			bool lineBreak = false;

			if (c == L'\r') // Mac or Windows breaks
			{
				lineBreak = true;
				if ((i>0) && mText[i-1] == L'\n') // Windows breaks
				{
					mText.erase(i-1);
					--size;
				}
				c = '\0';
			}
			else if (c == L'\n') // Unix breaks
			{
				lineBreak = true;
				c = '\0';
			}

			if (c==L' ' || c==0 || i==0)
			{
				if (word.size())
				{
					// here comes the next whitespace, look if
					// we must break the last word to the next line.
					const int whitelgth = font->GetDimension(whitespace.c_str())[0];
					const int wordlgth = font->GetDimension(word.c_str())[0];

					if (length && (length + wordlgth + whitelgth > elWidth))
					{
						// break to next line
						mBrokenText.push_back(line);
						length = wordlgth;
						line = word;
					}
					else
					{
						// add word to line
						line = whitespace + line;
						line = word + line;
						length += whitelgth + wordlgth;
					}

					word = L"";
					whitespace = L"";
				}

				if (c != 0)
					whitespace = eastl::wstring(&c, 1) + whitespace;

				// compute line break
				if (lineBreak)
				{
					line = whitespace + line;
					line = word + line;
					mBrokenText.push_back(line);
					line = L"";
					word = L"";
					whitespace = L"";
					length = 0;
				}
			}
			else
			{
				// yippee this is a word..
				word = eastl::wstring(&c, 1) + word;
			}
		}

		line = whitespace + line;
		line = word + line;
		mBrokenText.push_back(line);
	}
}


//! Sets the new caption of this element.
void UIStaticText::SetText(const wchar_t* text)
{
	BaseUIElement::SetText(text);
	BreakText();
}


void UIStaticText::UpdateAbsoluteTransformation()
{
	BaseUIElement::UpdateAbsoluteTransformation();
	BreakText();
}


//! Returns the height of the text in pixels when it is drawn.
int UIStaticText::GetTextHeight() const
{
	const eastl::shared_ptr<BaseUIFont>& font = GetActiveFont();
	if (!font)
		return 0;

	int height = font->GetDimension(L"A")[1] + font->GetKerningHeight();

	if (mWordWrap)
		height *= mBrokenText.size();

	return height;
}


int UIStaticText::GetTextWidth() const
{
	const eastl::shared_ptr<BaseUIFont>& font = GetActiveFont();
	if(!font)
		return 0;

	if(mWordWrap)
	{
		int widest = 0;

		for(unsigned int line = 0; line < mBrokenText.size(); ++line)
		{
			int width = font->GetDimension(mBrokenText[line].c_str())[0];

			if(width > widest)
				widest = width;
		}

		return widest;
	}
	else
	{
		return font->GetDimension(mText.c_str())[0];
	}
}
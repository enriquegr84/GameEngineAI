//========================================================================
// GameOptions.h : Defines utility functions for game options
//
//
// Part of the GameEngine Application
//
// GameEngine is the sample application that encapsulates much of the source code
// discussed in "Game Coding Complete - 4th Edition" by Mike McShaffry and David
// "Rez" Graham, published by Charles River Media. 
// ISBN-10: 1133776574 | ISBN-13: 978-1133776574
//
// If this source code has found it's way to you, and you think it has helped you
// in any way, do the authors a favor and buy a new copy of the book - there are 
// detailed explanations in it that compliment this code well. Buy a copy at Amazon.com
// by clicking here: 
//    http://www.amazon.com/gp/product/1133776574/ref=olp_product_details?ie=UTF8&me=&seller=
//
// There's a companion web site at http://www.mcshaffry.com/GameCode/
// 
// The source code is managed and maintained through Google Code: 
//    http://code.google.com/p/GameEngine/
//
// (c) Copyright 2012 Michael L. McShaffry and David Graham
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser GPL v3
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See 
// http://www.gnu.org/licenses/lgpl-3.0.txt for more details.
//
// You should have received a copy of the GNU Lesser GPL v3
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//========================================================================

#ifndef GAMEOPTION_H
#define GAMEOPTION_H

#include "GameEngineStd.h"

#include "Mathematic/Algebra/Vector2.h"

struct GameOption
{
	// Level option
	eastl::string m_Level;

	// Rendering options
	eastl::string m_Renderer;
	Vector2<int> m_ScreenSize;

	// Sound options
	float m_soundEffectsVolume;			
	float m_musicVolume;				

	// Multiplayer options
	int m_expectedPlayers;				
	int m_listenPort;					
	eastl::string m_gameHost;				
	int m_numAIs;
	int m_maxAIs;
	int m_maxPlayers;

	// XMLElement - look at this to find other options added by the developer
	XMLElement *m_pRoot;

	GameOption();
	~GameOption() { }

	void Init(const wchar_t* xmlFilePath);
};


#endif
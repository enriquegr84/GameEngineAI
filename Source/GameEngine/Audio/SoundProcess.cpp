//========================================================================
// SoundProcess.cpp : Defines sound processes.
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

#include "Audio.h"
#include "SoundProcess.h"
#include "SoundResource.h"

#include "Application/GameApplication.h"

//////////////////////////////////////////////////////////////////////
// SoundProcess Implementation
//////////////////////////////////////////////////////////////////////

//
// SoundProcess::SoundProcess				- Chapter 13, page 428
//
SoundProcess::SoundProcess(eastl::shared_ptr<ResHandle> resource, int volume, bool looping) :
	m_handle(resource),
	m_Volume(volume),
	m_isLooping(looping)
{
	InitializeVolume();
}


//
// SoundProcess::~SoundProcess				- Chapter 13, page 428
//
SoundProcess::~SoundProcess()
{
    if (IsPlaying())
        Stop();

	if (m_AudioBuffer)
		g_pAudio->ReleaseAudioBuffer(m_AudioBuffer.get());
}


void SoundProcess::InitializeVolume()
{
	// FUTURE WORK: Somewhere set an adjusted volume based on game options
	// m_volume = g_GraphicalApp->GetVolume(typeOfSound);
}

//
// SoundProcess::GetLengthMilli					- Chapter 13, page 430
//
int SoundProcess::GetLengthMilli()
{
	if ( m_handle && m_handle->GetExtra())
	{
		eastl::shared_ptr<SoundResourceExtraData> extra = 
			eastl::static_pointer_cast<SoundResourceExtraData>(m_handle->GetExtra());
		return extra->GetLengthMilli();
	}
	else
    {
		return 0;
    }
}

/*
	SoundProcess::OnInit
	One important concept to understand about sounds is that the code might create a sound 
	process long before the sound should be played or even loaded. Since sound effects tend 
	to require a lot of data, it�s a good idea to be careful about when you instantiate sound
	effects. After all, you don�t want the game to stutter or suffer wacky pauses.
*/
void SoundProcess::OnInit()
{
    Process::OnInit();

	//If the sound has never been... you know... then Play it for the very first time
	if ( m_handle == NULL || m_handle->GetExtra() == NULL)
		return;

	//This sound will manage it's own handle in the other thread
	BaseAudioBuffer *buffer = g_pAudio->InitAudioBuffer(m_handle);

	if (!buffer)
	{
        Fail();
		return;
	}

	m_AudioBuffer.reset(buffer);	

	Play(m_Volume, m_isLooping);
}

/*
	SoundProcess::OnUpdate
	The OnUpdate method monitors the sound effect as it�s being played. Once it is
	finished, it kills the process and releases the audio buffer. If the sound is 
	looping, it will play until some external call kills the process.
*/
void SoundProcess::OnUpdate(unsigned long deltaMs)
{
    if (!IsPlaying())
    {
        Succeed();
    }
}

//
// SoundProcess::IsPlaying						- Chapter 13, page 429
//
bool SoundProcess::IsPlaying()
{
	if ( ! m_handle || ! m_AudioBuffer )
		return false;
	
	return m_AudioBuffer->IsPlaying();
}

//
// SoundProcess::SetVolume						- Chapter 13, page 430
//
void SoundProcess::SetVolume(int volume)
{
	if(m_AudioBuffer==NULL)
	{
		return;
	}

	LogAssert(volume>=0 && volume<=100, "Volume must be a number between 0 and 100");
	m_Volume = volume;
	m_AudioBuffer->SetVolume(volume);
}

//
// SoundProcess::GetVolume						- Chapter 13, page 430
//
int SoundProcess::GetVolume()
{
	if(m_AudioBuffer==NULL)
	{
		return 0;
	}

	m_Volume = m_AudioBuffer->GetVolume();
	return m_Volume;
}

//
// SoundProcess::PauseSound						- Chapter 13, page 430
//   NOTE: This is called TogglePause in te book
//
void SoundProcess::PauseSound()
{
	if (m_AudioBuffer)
		m_AudioBuffer->TogglePause();
}

//
// SoundProcess::Play							- Chapter 13, page 431
//
void SoundProcess::Play(const int volume, const bool looping)
{
	LogAssert(volume>=0 && volume<=100, "Volume must be a number between 0 and 100");

	if(!m_AudioBuffer)
	{
		return;
	}
	
	m_AudioBuffer->Play(volume, looping);
}

//
// SoundProcess::Stop							- Chapter 13, page 431
//
void SoundProcess::Stop()
{
	if (m_AudioBuffer)
	{
		m_AudioBuffer->Stop();
	}
}

//
// SoundProcess::GetProgress					- Chapter 13, page 431
//
float SoundProcess::GetProgress()
{
	if (m_AudioBuffer)
	{
		return m_AudioBuffer->GetProgress();
	}

	return 0.0f;
}

//
// ExplosionProcess::OnInit					- Chapter 13, page 433
//
void ExplosionProcess::OnInit()
{
	Process::OnInit();

	BaseResource resource(L"explosion.wav");

	GameApplication* gameApp = (GameApplication*)Application::App;
	eastl::shared_ptr<ResHandle> srh = gameApp->mResCache->GetHandle(&resource);
	m_Sound.reset(new SoundProcess(srh));

	// Imagine cool explosion graphics setup code here!!!!
	//
	//
	//
}

//
// ExplosionProcess::OnUpdate					- Chapter 13, page 433
//
void ExplosionProcess::OnUpdate(unsigned long deltaMs)
{
	// Since the sound is the real pacing mechanism - we ignore deltaMilliseconds
	float progress = m_Sound->GetProgress();
	
	switch (m_Stage)
	{
		case 0:
        {
			if (progress > 0.55f)
			{
				++m_Stage;
				// Imagine secondary explosion effect launch right here!
			}
			break;
        }

		case 1:
        {
			if (progress > 0.75f)
			{
				++m_Stage;
				// Imagine tertiary explosion effect launch right here!
			}
			break;
        }

		default:
        {
			break;
        }
	}
}

/////////////////////////////////////////////////////////////////////////////
// 
// FadeProcess Implementation 
//
//////////////////////////////////////////////////////////////////////

//
// FadeProcess::FadeProcess						- Chapter 13, page 435
//
FadeProcess::FadeProcess(eastl::shared_ptr<SoundProcess> sound, int fadeTime, int endVolume)
{
	m_Sound = sound;
	m_TotalFadeTime = fadeTime;
	m_StartVolume = sound->GetVolume();
	m_EndVolume = endVolume;
	m_ElapsedTime = 0;

	OnUpdate(0);
}

//
// FadeProcess::OnUpdate						- Chapter 13, page 435
//
void FadeProcess::OnUpdate(unsigned long deltaMs)
{
    m_ElapsedTime += deltaMs;

	if (m_Sound->IsDead())
		Succeed();

	float cooef = (float)m_ElapsedTime / m_TotalFadeTime;
	if (cooef>1.0f)
		cooef = 1.0f;
	if (cooef<0.0f)
		cooef = 0.0f;

	int newVolume = m_StartVolume + (int)( float(m_EndVolume - m_StartVolume) * cooef);

	if (m_ElapsedTime >= m_TotalFadeTime)
	{
		newVolume = m_EndVolume;
		Succeed();
	}

	m_Sound->SetVolume(newVolume);
}
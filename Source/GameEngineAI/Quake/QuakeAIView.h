/*******************************************************
 * Copyright (C) GameEngineAI - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Enrique González Rodríguez <enriquegr84@hotmail.es>, 2019-2020
 *******************************************************/

#ifndef QUAKEAIVIEW_H
#define QUAKEAIVIEW_H

#include "QuakeStd.h"

#include "Core/Event/EventManager.h"

#include "Game/View/GameView.h"

#include "AI/Pathing.h"

class QuakeAIView : public BaseGameView 
{

public:
	QuakeAIView();
	virtual ~QuakeAIView();

	virtual bool OnRestore() { return true; }
	virtual void OnRender(double time, float elapsedTime) {}
	virtual bool OnLostDevice() { return true; }
	virtual GameViewType GetType() { return GV_AI; }
	virtual GameViewId GetId() const { return mViewId; }
	virtual ActorId GetActorId() const { return mPlayerId; }
	virtual void OnAttach(GameViewId vid, ActorId actorId);
	virtual bool OnMsgProc( const Event& event ) {	return false; }
	virtual void OnUpdate(unsigned int timeMs, unsigned long deltaMs);

protected:

	void Stationary(unsigned long deltaMs);
	void Avoidance(unsigned long deltaMs);
	void Smooth(unsigned long deltaMs);
	void Cliff();

	// Movement Controls
	int mOrientation;
	unsigned long mStationaryTime;

	// Orientation Controls
	float mYaw;
	float mYawSmooth;
	float mYawSmoothTime;
	float mPitch;
	float mPitchTarget;

	// Speed Controls
	float mMaxFallSpeed;
	float mMaxJumpSpeed;
	float mMaxRotateSpeed;
	float mFallSpeed;
	float mJumpSpeed;
	float mJumpMoveSpeed;
	float mMoveSpeed;
	float mRotateSpeed;

	GameViewId	mViewId;
	ActorId mPlayerId;

	Transform mAbsoluteTransform;

private:

	float mCurrentActionTime;
	unsigned int mCurrentAction;

	ActorId mCurrentActor;
	PathingArcVec mCurrentPlan;
	int mCurrentPlanId;

	PathingNode* mGoalNode;
	PathingArc* mCurrentArc;
	PathingNode* mCurrentNode;
	eastl::shared_ptr<PathingGraph> mPathingGraph;
};

#endif
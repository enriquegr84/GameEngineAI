/*******************************************************
 * Copyright (C) GameEngineAI - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Enrique González Rodríguez <enriquegr84@hotmail.es>, 2019-2020
 *******************************************************/

#ifndef QUAKEAIPROCESS_H
#define QUAKEAIPROCESS_H

#include "Core/Process/RealtimeProcess.h"
#include "Core/Event/EventManager.h"

#include "QuakeAIManager.h"


//
// class QuakeAIProcess
//
class QuakeAIProcess : public RealtimeProcess
{
public:

	QuakeAIProcess();
	~QuakeAIProcess();

	virtual void ThreadProc(void);

protected:

	void Visibility(
		PathingNode* playerNode, PathingArcVec& playerPathPlan,
		PathingNode* otherPlayerNode, PathingArcVec& otherPlayerPathPlan,
		float* visibleTime, float* visibleDistance, float* visibleHeight,
		float* otherVisibleTime, float* otherVisibleDistance, float* otherVisibleHeight);
	void ConstructPath(NodeState& playerState,
		PathingCluster* playerCluster, PathingArcVec& playerPathPlan);
	void ConstructActorPath(NodeState& playerState,
		PathingCluster* playerCluster, PathingArcVec& playerActorPlan);
	void Simulation(
		NodeState& playerState, PathingArcVec& playerPathPlan,
		NodeState& otherPlayerState, PathingArcVec& otherPlayerPathPlan);
	void EvaluatePlayers(
		NodeState& playerState, NodeState& otherPlayerState);

private:

	NodeState mPlayerState, mOtherPlayerState;

	eastl::map<ActorId, float> mExcludeActors;

	QuakeAIManager*	mAIManager;
};

#endif
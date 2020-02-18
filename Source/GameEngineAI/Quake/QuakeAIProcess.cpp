//========================================================================
// QuakeAIProcess.cpp : Defines ai process that can run in a thread
//
// GameCode4 is the sample application that encapsulates much of the source code
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
//    http://code.google.com/p/gamecode4/
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

#include "Quake.h"
#include "QuakeApp.h"
#include "QuakeEvents.h"
#include "QuakeAIProcess.h"

#include "Core/OS/OS.h"

QuakeAIProcess::QuakeAIProcess() : RealtimeProcess()
{
	mAIManager = dynamic_cast<QuakeAIManager*>(GameLogic::Get()->GetAIManager());
}

QuakeAIProcess::~QuakeAIProcess(void)
{

}

void QuakeAIProcess::Visibility(
	PathingNode* playerNode, PathingArcVec& playerPathPlan, 
	PathingNode* otherPlayerNode, PathingArcVec& otherPlayerPathPlan,
	float* visibleTime, float* visibleDistance, float* visibleHeight,
	float* otherVisibleTime, float* otherVisibleDistance, float* otherVisibleHeight)
{
	float totalTime = 0.f, totalArcTime = 0.f;
	unsigned int index = 0, otherIndex = 0, otherPathIndex = 0;

	PathingNode* currentNode = playerNode;
	PathingTransition* currentTransition = NULL;
	PathingNodeVec transitionNodes;

	PathingNode* otherCurrentNode = otherPlayerNode;
	PathingTransition* otherCurrentTransition =
		otherCurrentNode->FindTransition(otherPlayerPathPlan[otherPathIndex]->GetId());
	PathingNodeVec otherTransitionNodes = otherCurrentTransition->GetNodes();

	//lets calculate the visibility for simultaneous path travelling.
	for (PathingArc* currentArc : playerPathPlan)
	{
		index = 0;
		currentTransition = currentNode->FindTransition(currentArc->GetId());
		transitionNodes = currentTransition->GetNodes();
		for (; index < transitionNodes.size(); index++)
		{
			float visibleWeight = currentTransition->GetWeights()[index];
			if (mAIManager->GetPathingGraph()->IsVisibleCluster(
				transitionNodes[index]->GetCluster(), otherTransitionNodes[otherIndex]->GetCluster()))
			{
				if (transitionNodes[index]->IsVisibleNode(otherTransitionNodes[otherIndex]))
				{
					(*visibleDistance) += Length(
						otherCurrentTransition->GetConnections()[otherIndex] -
						currentTransition->GetConnections()[index]) * visibleWeight;
					(*visibleHeight) +=
						(currentTransition->GetConnections()[index][2] -
						otherCurrentTransition->GetConnections()[otherIndex][2]) * visibleWeight;
					(*visibleTime) += visibleWeight;

					(*otherVisibleDistance) += Length(
						otherCurrentTransition->GetConnections()[otherIndex] -
						currentTransition->GetConnections()[index]) * visibleWeight;
					(*otherVisibleHeight) +=
						(otherCurrentTransition->GetConnections()[otherIndex][2] -
						currentTransition->GetConnections()[index][2]) * visibleWeight;
					(*otherVisibleTime) += visibleWeight;
				}
			}

			while (totalArcTime <= totalTime)
			{
				totalArcTime += otherCurrentTransition->GetWeights()[otherIndex];
				if (otherIndex + 1 >= otherTransitionNodes.size())
				{
					if (otherPathIndex + 1 < otherPlayerPathPlan.size())
					{
						otherIndex = 0;
						otherCurrentNode = otherPlayerPathPlan[otherPathIndex]->GetNode();

						otherPathIndex++;
						otherCurrentTransition = otherCurrentNode->FindTransition(
							otherPlayerPathPlan[otherPathIndex]->GetId());
						otherTransitionNodes = otherCurrentTransition->GetNodes();
					}
					else break;
				}
				else otherIndex++;
			}
			totalTime += visibleWeight;
		}
		currentNode = currentArc->GetNode();
	}
	
	//average
	if ((*visibleTime) > 0.f)
	{
		//lets put a minimum of visible time if their ending nodes are visible
		if (totalTime < 0.6f)
		{
			if (mAIManager->GetPathingGraph()->IsVisibleCluster(
				playerPathPlan.back()->GetNode()->GetCluster(),
				otherPlayerPathPlan.back()->GetNode()->GetCluster()))
			{
				if (playerPathPlan.back()->GetNode()->IsVisibleNode(
					otherPlayerPathPlan.back()->GetNode()))
				{
					float visibleWeight = (*visibleTime) / totalTime;
					visibleWeight = (0.6f - totalTime) * visibleWeight;

					(*visibleDistance) += Length(
						otherPlayerPathPlan.back()->GetNode()->GetPos() -
						playerPathPlan.back()->GetNode()->GetPos()) * visibleWeight;
					(*visibleHeight) +=
						(playerPathPlan.back()->GetNode()->GetPos()[2] -
						otherPlayerPathPlan.back()->GetNode()->GetPos()[2]) * visibleWeight;
					(*visibleTime) += visibleWeight;
				}
			}
		}

		(*visibleDistance) /= (*visibleTime);
		(*visibleHeight) /= (*visibleTime);
	}

	if ((*otherVisibleTime) > 0.f)
	{
		//lets put a minimum of visible time if their ending nodes are visible
		if (totalTime < 0.6f)
		{
			if (mAIManager->GetPathingGraph()->IsVisibleCluster(
				playerPathPlan.back()->GetNode()->GetCluster(),
				otherPlayerPathPlan.back()->GetNode()->GetCluster()))
			{
				if (playerPathPlan.back()->GetNode()->IsVisibleNode(
					otherPlayerPathPlan.back()->GetNode()))
				{
					float otherVisibleWeight = (*otherVisibleTime) / totalTime;
					otherVisibleWeight = (0.6f - totalTime) * otherVisibleWeight;

					(*otherVisibleDistance) += Length(
						otherPlayerPathPlan.back()->GetNode()->GetPos() -
						playerPathPlan.back()->GetNode()->GetPos()) * otherVisibleWeight;
					(*otherVisibleHeight) +=
						(otherPlayerPathPlan.back()->GetNode()->GetPos()[2] -
						playerPathPlan.back()->GetNode()->GetPos()[2]) * otherVisibleWeight;
					(*otherVisibleTime) += otherVisibleWeight;
				}
			}
		}

		(*otherVisibleDistance) /= (*otherVisibleTime);
		(*otherVisibleHeight) /= (*otherVisibleTime);
	}
}

void QuakeAIProcess::Simulation(
	NodeState& playerState, PathingArcVec& playerPathPlan,
	NodeState& otherPlayerState, PathingArcVec& otherPlayerPathPlan)
{
	PathingNode* playerNode = playerState.plan.node;
	PathingNode* otherPlayerNode = otherPlayerState.plan.node;
	eastl::map<PathingArcVec, eastl::map<PathingArcVec, NodeState>> playerStates, otherPlayerStates;

	float pathPlanWeight = 0.f;
	eastl::map<ActorId, float> actors;
	if (playerNode->GetActorId() != INVALID_ACTOR_ID)
		actors[playerNode->GetActorId()] = pathPlanWeight;

	PathingNode* currentNode = playerNode;
	for (PathingArc* playerArc : playerPathPlan)
	{
		pathPlanWeight += playerArc->GetWeight();
		if (playerArc->GetNode()->GetActorId() != INVALID_ACTOR_ID)
			if (actors.find(playerArc->GetNode()->GetActorId()) == actors.end())
				actors[playerArc->GetNode()->GetActorId()] = pathPlanWeight;

		currentNode = playerArc->GetNode();
	}

	float otherPathPlanWeight = 0.f;
	eastl::map<ActorId, float> otherActors;
	if (otherPlayerNode->GetActorId() != INVALID_ACTOR_ID)
		otherActors[otherPlayerNode->GetActorId()] = otherPathPlanWeight;

	currentNode = otherPlayerNode;
	for (PathingArc* otherPlayerArc : otherPlayerPathPlan)
	{
		otherPathPlanWeight += otherPlayerArc->GetWeight();
		if (otherPlayerArc->GetNode()->GetActorId() != INVALID_ACTOR_ID)
			if (otherActors.find(otherPlayerArc->GetNode()->GetActorId()) == otherActors.end())
				otherActors[otherPlayerArc->GetNode()->GetActorId()] = otherPathPlanWeight;

		currentNode = otherPlayerArc->GetNode();
	}

	float visibleTime = 0, otherVisibleTime = 0;
	float visibleHeight = 0, otherVisibleHeight = 0;
	float visibleDistance = 0, otherVisibleDistance = 0;
	if (pathPlanWeight > otherPathPlanWeight)
	{
		Visibility(
			playerNode, playerPathPlan, otherPlayerNode, otherPlayerPathPlan,
			&visibleTime, &visibleDistance, &visibleHeight, 
			&otherVisibleTime, &otherVisibleDistance, &otherVisibleHeight);
	}
	else
	{
		Visibility(
			otherPlayerNode, otherPlayerPathPlan, playerNode, playerPathPlan,
			&otherVisibleTime, &otherVisibleDistance, &otherVisibleHeight, 
			&visibleTime, &visibleDistance, &visibleHeight);
	}

	//calculate damage
	NodeState playerNodeState(playerState);
	playerNodeState.plan.AddPlanPath(playerPathPlan);
	mAIManager->CalculateDamage(playerNodeState, visibleTime, visibleDistance, visibleHeight);

	eastl::map<ActorId, float> pathActors;
	eastl::map<ActorId, float>::iterator itActor;
	for (itActor = actors.begin(); itActor != actors.end(); itActor++)
	{
		if (mAIManager->CanItemBeGrabbed(
			(*itActor).first, (*itActor).second, playerState, mExcludeActors))
		{
			if (otherActors.find((*itActor).first) != otherActors.end())
			{
				if (!mAIManager->CanItemBeGrabbed(
					(*itActor).first, (*itActor).second, otherPlayerState, mExcludeActors) ||
					otherActors[(*itActor).first] > (*itActor).second)
				{
					pathActors[(*itActor).first] = (*itActor).second;
				}
			}
			else pathActors[(*itActor).first] = (*itActor).second;
		}
	}
	mAIManager->PickupItems(playerNodeState, pathActors, mExcludeActors);

	NodeState otherPlayerNodeState(otherPlayerState);
	otherPlayerNodeState.plan.AddPlanPath(otherPlayerPathPlan);
	mAIManager->CalculateDamage(otherPlayerNodeState, otherVisibleTime, otherVisibleDistance, otherVisibleHeight);
			
	eastl::map<ActorId, float> otherPathActors;
	eastl::map<ActorId, float>::iterator itOtherActor;
	for (itOtherActor = otherActors.begin(); itOtherActor != otherActors.end(); itOtherActor++)
	{
		if (mAIManager->CanItemBeGrabbed(
			(*itOtherActor).first, (*itOtherActor).second, otherPlayerState, mExcludeActors))
		{
			if (actors.find((*itOtherActor).first) != actors.end())
			{
				if (!mAIManager->CanItemBeGrabbed(
					(*itOtherActor).first, (*itOtherActor).second, playerState, mExcludeActors) ||
					actors[(*itOtherActor).first] > (*itOtherActor).second)
				{
					otherPathActors[(*itOtherActor).first] = (*itOtherActor).second;
				}
			}
			else otherPathActors[(*itOtherActor).first] = (*itOtherActor).second;
		}
	}
	mAIManager->PickupItems(otherPlayerNodeState, otherPathActors, mExcludeActors);

	//we calculate the heuristic
	mAIManager->CalculateHeuristic(playerNodeState, otherPlayerNodeState);

	playerState.Copy(playerNodeState);
	otherPlayerState.Copy(otherPlayerNodeState);
}

void QuakeAIProcess::ConstructPath(NodeState& playerState,
	PathingCluster* playerCluster, PathingArcVec& playerPathPlan)
{
	PathingNode* currentNode = playerState.plan.node;
	while (currentNode != playerCluster->GetTarget())
	{
		PathingCluster* currentCluster = currentNode->FindCluster(
			playerCluster->GetType(), playerCluster->GetTarget());
		PathingArc* currentArc = currentNode->FindArc(currentCluster->GetNode());
		playerPathPlan.push_back(currentArc);

		currentNode = currentArc->GetNode();
	}
}

void QuakeAIProcess::ConstructActorPath(NodeState& playerState,
	PathingCluster* playerCluster, PathingArcVec& playerActorPlan)
{
	float maxPathWeight = 0.f;
	PathingNode* currentNode = playerState.plan.node;
	while (currentNode != playerCluster->GetTarget())
	{
		PathingCluster* currentCluster = currentNode->FindCluster(
			playerCluster->GetType(), playerCluster->GetTarget());
		PathingArc* currentArc = currentNode->FindArc(currentCluster->GetNode());

		currentNode = currentArc->GetNode();
		maxPathWeight += currentArc->GetWeight();
	}
	//add extra time
	maxPathWeight += maxPathWeight * 0.15f;
	mAIManager->FindPath(playerState, playerCluster, playerActorPlan, mExcludeActors, maxPathWeight);
}

void QuakeAIProcess::EvaluatePlayers(NodeState& playerState, NodeState& otherPlayerState)
{
	eastl::map<PathingCluster*, PathingArcVec> playerPathPlans, otherPlayerPathPlans;

	//search player surrounding clusters
	PathingClusterVec playerClusters;
	playerState.plan.node->GetClusters(GAT_JUMP, playerClusters, 60);

	/*
		player path construction in two phases:
		- actor path planning
		- path planning
	*/
	unsigned int playerClusterIdx = 0;
	unsigned int clusterSize = playerClusters.size();
	mAIManager->PrintLogInformationDetails("\n player actors ");
	//construct path based on closest actors to each cluster pathway
	for (; playerClusterIdx < clusterSize; playerClusterIdx++)
	{
		PathingCluster* playerCluster = playerClusters[playerClusterIdx];

		PathingArcVec playerActorPlan;
		ConstructActorPath(playerState, playerCluster, playerActorPlan);
		if (!playerActorPlan.empty())
		{
			//construct path
			playerPathPlans[playerCluster] = playerActorPlan;
		}
	}

	//construct path based on cluster pathway
	for (playerClusterIdx = 0; playerClusterIdx < clusterSize; playerClusterIdx++)
	{
		PathingCluster* playerCluster = playerClusters[playerClusterIdx];
		if (playerPathPlans.find(playerCluster) != playerPathPlans.end())
			continue;

		PathingArcVec playerPathPlan;
		ConstructPath(playerState, playerCluster, playerPathPlan);
		if (!playerPathPlan.empty())
		{
			//construct path
			playerPathPlans[playerCluster] = playerPathPlan;
		}
	}

	//search other player surrounding clusters
	PathingClusterVec otherPlayerClusters;
	otherPlayerState.plan.node->GetClusters(GAT_JUMP, otherPlayerClusters, 60);

	//other player path construction
	unsigned int otherPlayerClusterIdx = 0;
	unsigned int otherClusterSize = otherPlayerClusters.size();
	mAIManager->PrintLogInformationDetails("\n other player actors ");
	for (; otherPlayerClusterIdx < otherClusterSize; otherPlayerClusterIdx++)
	{
		PathingCluster* otherPlayerCluster = otherPlayerClusters[otherPlayerClusterIdx];

		PathingArcVec otherPlayerActorPlan;
		ConstructActorPath(otherPlayerState, otherPlayerCluster, otherPlayerActorPlan);
		if (!otherPlayerActorPlan.empty())
		{
			//construct path
			otherPlayerPathPlans[otherPlayerCluster] = otherPlayerActorPlan;
		}
	}

	for (otherPlayerClusterIdx = 0; otherPlayerClusterIdx < otherClusterSize; otherPlayerClusterIdx++)
	{
		PathingCluster* otherPlayerCluster = otherPlayerClusters[otherPlayerClusterIdx];
		if (otherPlayerPathPlans.find(otherPlayerCluster) != otherPlayerPathPlans.end())
			continue;

		PathingArcVec otherPlayerPathPlan;
		ConstructPath(otherPlayerState, otherPlayerCluster, otherPlayerPathPlan);
		if (otherPlayerPathPlan.size())
		{
			//construct path
			otherPlayerPathPlans[otherPlayerCluster] = otherPlayerPathPlan;
		}
	}

	eastl::map<PathingCluster*, eastl::map<PathingCluster*, NodeState>> playerClustersStates, otherPlayerClustersStates;
	for (playerClusterIdx = 0; playerClusterIdx < clusterSize; playerClusterIdx++)
	{
		PathingCluster* playerCluster = playerClusters[playerClusterIdx];

		for (otherPlayerClusterIdx = 0; otherPlayerClusterIdx < otherClusterSize; otherPlayerClusterIdx++)
		{
			PathingCluster* otherPlayerCluster = otherPlayerClusters[otherPlayerClusterIdx];

			NodeState state(playerState);
			NodeState otherState(otherPlayerState);
			Simulation(state, playerPathPlans[playerCluster], otherState, otherPlayerPathPlans[otherPlayerCluster]);

			if (state.valid && otherState.valid)
			{
				playerClustersStates[playerCluster][otherPlayerCluster] = state;
				otherPlayerClustersStates[otherPlayerCluster][playerCluster] = otherState;
			}
		}
	}

	eastl::map<PathingCluster*, NodeState> currentClusterStates, otherCurrentClusterPlanStates;
	if (otherPlayerState.valid && otherPlayerState.plan.path.size())
	{
		for (playerClusterIdx = 0; playerClusterIdx < clusterSize; playerClusterIdx++)
		{
			PathingCluster* playerCluster = playerClusters[playerClusterIdx];

			NodeState state(playerState);
			NodeState otherState(otherPlayerState);

			otherState.current = true;
			Simulation(state, playerPathPlans[playerCluster], otherState, otherPlayerState.plan.path);

			if (state.valid && otherState.valid)
			{
				currentClusterStates[playerCluster] = state;
				otherCurrentClusterPlanStates[playerCluster] = otherState;
			}
		}
	}

	eastl::map<PathingCluster*, NodeState> otherCurrentClusterStates, currentClusterPlanStates;
	if (playerState.valid && playerState.plan.path.size())
	{
		for (otherPlayerClusterIdx = 0; otherPlayerClusterIdx < otherClusterSize; otherPlayerClusterIdx++)
		{
			PathingCluster* otherPlayerCluster = otherPlayerClusters[otherPlayerClusterIdx];

			NodeState state(playerState);
			NodeState otherState(otherPlayerState);

			state.current = true;
			Simulation(state, playerState.plan.path, otherState, otherPlayerPathPlans[otherPlayerCluster]);

			if (state.valid && otherState.valid)
			{
				otherCurrentClusterStates[otherPlayerCluster] = otherState;
				currentClusterPlanStates[otherPlayerCluster] = state;
			}
		}
	}

	NodeState currentPlanState, otherCurrentPlanState;
	if (playerState.valid && otherPlayerState.valid &&
		playerState.plan.path.size() && otherPlayerState.plan.path.size())
	{
		NodeState state(playerState);
		NodeState otherState(otherPlayerState);

		state.current = true;
		otherState.current = true;
		Simulation(state, playerState.plan.path, otherState, otherPlayerState.plan.path);

		if (state.valid && otherState.valid)
		{
			otherCurrentPlanState = otherState;
			currentPlanState = state;
		}
	}

	mOtherPlayerState.Copy(otherPlayerState);
	mOtherPlayerState.heuristic = FLT_MAX;
	mOtherPlayerState.valid = false;
	mAIManager->PrintLogInformationDetails("\n\n minimax other player \n");

	mAIManager->PrintLogInformation("\n other player old guess plan path id " +
		eastl::to_string(mOtherPlayerState.plan.id) + " : ");
	for (PathingArc* pathArc : mOtherPlayerState.plan.path)
		mAIManager->PrintLogInformation(eastl::to_string(pathArc->GetNode()->GetId()) + " ");

	PathingCluster* otherPlayerCluster = NULL;
	for (auto otherPlayerClustersState : otherPlayerClustersStates)
	{
		mAIManager->PrintLogInformationDetails("\n other player cluster : " +
			eastl::to_string(otherPlayerClustersState.first->GetTarget()->GetCluster()) + " ");

		NodeState otherPlayerNodeState(otherPlayerState);
		otherPlayerNodeState.heuristic = -FLT_MAX;
		otherPlayerNodeState.valid = false;
		for (auto otherPlayerClusterState : otherPlayerClustersState.second)
		{
			if (otherPlayerClusterState.second.heuristic > otherPlayerNodeState.heuristic)
			{
				otherPlayerNodeState = otherPlayerClusterState.second;
			}

			mAIManager->PrintLogInformationDetails("player cluster : " +
				eastl::to_string(otherPlayerClusterState.first->GetTarget()->GetCluster()) + " ");
			mAIManager->PrintLogInformationDetails("heuristic : " +
				eastl::to_string(otherPlayerClusterState.second.heuristic) + " ");

			if (otherPlayerClusterState.second.weapon != WP_NONE)
			{
				mAIManager->PrintLogInformationDetails("weapon : " +
					eastl::to_string(otherPlayerClusterState.second.weapon) + " ");
				mAIManager->PrintLogInformationDetails("damage : " +
					eastl::to_string(otherPlayerClusterState.second.damage[otherPlayerClusterState.second.weapon - 1]) + " ");
			}

			if (!otherPlayerClusterState.second.items.empty())
				mAIManager->PrintLogInformationDetails("actors : ");
			for (eastl::shared_ptr<Actor> pItemActor : otherPlayerClusterState.second.items)
			{
				if (pItemActor->GetType() == "Weapon")
				{
					eastl::shared_ptr<WeaponPickup> pWeaponPickup =
						pItemActor->GetComponent<WeaponPickup>(WeaponPickup::Name).lock();
					mAIManager->PrintLogInformationDetails("weapon " + eastl::to_string(pWeaponPickup->GetCode()) + " ");
				}
				else if (pItemActor->GetType() == "Ammo")
				{
					eastl::shared_ptr<AmmoPickup> pAmmoPickup =
						pItemActor->GetComponent<AmmoPickup>(AmmoPickup::Name).lock();
					mAIManager->PrintLogInformationDetails("ammo " + eastl::to_string(pAmmoPickup->GetCode()) + " ");
				}
				else if (pItemActor->GetType() == "Armor")
				{
					eastl::shared_ptr<ArmorPickup> pArmorPickup =
						pItemActor->GetComponent<ArmorPickup>(ArmorPickup::Name).lock();
					mAIManager->PrintLogInformationDetails("armor " + eastl::to_string(pArmorPickup->GetCode()) + " ");
				}
				else if (pItemActor->GetType() == "Health")
				{
					eastl::shared_ptr<HealthPickup> pHealthPickup =
						pItemActor->GetComponent<HealthPickup>(HealthPickup::Name).lock();
					mAIManager->PrintLogInformationDetails("health " + eastl::to_string(pHealthPickup->GetCode()) + " ");
				}
			}
		}

		if (otherCurrentClusterStates.find(otherPlayerClustersState.first) != otherCurrentClusterStates.end())
		{
			if (otherCurrentClusterStates[otherPlayerClustersState.first].heuristic > otherPlayerNodeState.heuristic)
			{
				otherPlayerNodeState = otherCurrentClusterStates[otherPlayerClustersState.first];
			}

			mAIManager->PrintLogInformationDetails("player cluster : " +
				eastl::to_string(otherPlayerClustersState.first->GetTarget()->GetCluster()) + " ");
			mAIManager->PrintLogInformationDetails("heuristic : " +
				eastl::to_string(otherCurrentClusterStates[otherPlayerClustersState.first].heuristic) + " ");

			if (otherCurrentClusterStates[otherPlayerClustersState.first].weapon != WP_NONE)
			{
				mAIManager->PrintLogInformationDetails("weapon : " +
					eastl::to_string(otherCurrentClusterStates[otherPlayerClustersState.first].weapon) + " ");
				mAIManager->PrintLogInformationDetails("damage : " + eastl::to_string(
					otherCurrentClusterStates[otherPlayerClustersState.first].damage[
					otherCurrentClusterStates[otherPlayerClustersState.first].weapon - 1]) + " ");
			}

			if (!otherCurrentClusterStates[otherPlayerClustersState.first].items.empty())
				mAIManager->PrintLogInformationDetails("actors : ");
			for (eastl::shared_ptr<Actor> pItemActor : otherCurrentClusterStates[otherPlayerClustersState.first].items)
			{
				if (pItemActor->GetType() == "Weapon")
				{
					eastl::shared_ptr<WeaponPickup> pWeaponPickup =
						pItemActor->GetComponent<WeaponPickup>(WeaponPickup::Name).lock();
					mAIManager->PrintLogInformationDetails("weapon " + eastl::to_string(pWeaponPickup->GetCode()) + " ");
				}
				else if (pItemActor->GetType() == "Ammo")
				{
					eastl::shared_ptr<AmmoPickup> pAmmoPickup =
						pItemActor->GetComponent<AmmoPickup>(AmmoPickup::Name).lock();
					mAIManager->PrintLogInformationDetails("ammo " + eastl::to_string(pAmmoPickup->GetCode()) + " ");
				}
				else if (pItemActor->GetType() == "Armor")
				{
					eastl::shared_ptr<ArmorPickup> pArmorPickup =
						pItemActor->GetComponent<ArmorPickup>(ArmorPickup::Name).lock();
					mAIManager->PrintLogInformationDetails("armor " + eastl::to_string(pArmorPickup->GetCode()) + " ");
				}
				else if (pItemActor->GetType() == "Health")
				{
					eastl::shared_ptr<HealthPickup> pHealthPickup =
						pItemActor->GetComponent<HealthPickup>(HealthPickup::Name).lock();
					mAIManager->PrintLogInformationDetails("health " + eastl::to_string(pHealthPickup->GetCode()) + " ");
				}
			}
		}

		if (otherPlayerNodeState.valid)
		{
			mAIManager->PrintLogInformationDetails("\n other player heuristic : " +
				eastl::to_string(otherPlayerNodeState.heuristic) + " ");

			if (otherPlayerNodeState.weapon != WP_NONE)
			{
				mAIManager->PrintLogInformationDetails("weapon : " +
					eastl::to_string(otherPlayerNodeState.weapon) + " ");
				mAIManager->PrintLogInformationDetails("damage : " +
					eastl::to_string(otherPlayerNodeState.damage[otherPlayerNodeState.weapon - 1]) + " ");
			}

			if (!otherPlayerNodeState.items.empty())
				mAIManager->PrintLogInformationDetails("actors : ");
			for (eastl::shared_ptr<Actor> pItemActor : otherPlayerNodeState.items)
			{
				if (pItemActor->GetType() == "Weapon")
				{
					eastl::shared_ptr<WeaponPickup> pWeaponPickup =
						pItemActor->GetComponent<WeaponPickup>(WeaponPickup::Name).lock();
					mAIManager->PrintLogInformationDetails("weapon " + eastl::to_string(pWeaponPickup->GetCode()) + " ");
				}
				else if (pItemActor->GetType() == "Ammo")
				{
					eastl::shared_ptr<AmmoPickup> pAmmoPickup =
						pItemActor->GetComponent<AmmoPickup>(AmmoPickup::Name).lock();
					mAIManager->PrintLogInformationDetails("ammo " + eastl::to_string(pAmmoPickup->GetCode()) + " ");
				}
				else if (pItemActor->GetType() == "Armor")
				{
					eastl::shared_ptr<ArmorPickup> pArmorPickup =
						pItemActor->GetComponent<ArmorPickup>(ArmorPickup::Name).lock();
					mAIManager->PrintLogInformationDetails("armor " + eastl::to_string(pArmorPickup->GetCode()) + " ");
				}
				else if (pItemActor->GetType() == "Health")
				{
					eastl::shared_ptr<HealthPickup> pHealthPickup =
						pItemActor->GetComponent<HealthPickup>(HealthPickup::Name).lock();
					mAIManager->PrintLogInformationDetails("health " + eastl::to_string(pHealthPickup->GetCode()) + " ");
				}
			}

			if (otherPlayerNodeState.heuristic < mOtherPlayerState.heuristic)
			{
				mOtherPlayerState.Copy(otherPlayerNodeState);
				otherPlayerCluster = otherPlayerClustersState.first;
			}
		}
	}

	if (otherPlayerState.valid)
	{
		NodeState otherPlayerNodeState(otherPlayerState);
		otherPlayerNodeState.heuristic = -FLT_MAX;
		otherPlayerNodeState.valid = false;
		for (auto otherCurrentClusterPlanState : otherCurrentClusterPlanStates)
		{
			if (otherCurrentClusterPlanState.second.heuristic > otherPlayerNodeState.heuristic)
			{
				otherPlayerNodeState = otherCurrentClusterPlanState.second;
			}
		}

		if (otherCurrentPlanState.valid)
		{
			if (otherCurrentPlanState.heuristic > otherPlayerNodeState.heuristic)
			{
				otherPlayerNodeState = otherCurrentPlanState;
			}
		}

		//we keep the current plan if the heuristic is less or close to the best player heuristic
		if (otherPlayerNodeState.valid)
		{
			if (abs(otherPlayerNodeState.heuristic - mOtherPlayerState.heuristic) < 0.02f)
			{
				mOtherPlayerState.Copy(otherPlayerNodeState);
				otherPlayerCluster = NULL;
			}
			else if (otherPlayerNodeState.heuristic < mOtherPlayerState.heuristic)
			{
				mOtherPlayerState.Copy(otherPlayerNodeState);
				otherPlayerCluster = NULL;
			}
		}
	}

	//In game theory for simultaneous decision making, it is stated that the best way to play 
	//is always considering the opponent�s decision-making. The way we have dealt with it is 
	//by taking the minimax result as what the player think is the best possible play for the 
	//opponent and react against it by choosing the course of actions that best counter the opponent�s play.
	if (playerState.valid)
	{
		PathingCluster* playerCluster = NULL;
		if (otherPlayerCluster != NULL)
		{
			mAIManager->PrintLogInformation("\n other player decision tree target cluster : " +
				eastl::to_string(otherPlayerCluster->GetTarget()->GetCluster()) + " ");
			mAIManager->PrintLogInformationDetails("\n\n other player decision tree target cluster : " +
				eastl::to_string(otherPlayerCluster->GetTarget()->GetCluster()) + " ");

			mPlayerState.Copy(playerState);
			mPlayerState.heuristic = -FLT_MAX;
			mPlayerState.valid = false;
			for (auto playerClustersState : playerClustersStates)
			{
				if (playerClustersState.second[otherPlayerCluster].heuristic > mPlayerState.heuristic)
				{
					mPlayerState.Copy(playerClustersState.second[otherPlayerCluster]);
					playerCluster = playerClustersState.first;
				}
				else if (abs(playerClustersState.second[otherPlayerCluster].heuristic - mPlayerState.heuristic) < GE_ROUNDING_ERROR)
				{
					//lets take the longest path for trying to avoid wandering situations
					if (playerClustersState.second[otherPlayerCluster].plan.weight > mPlayerState.plan.weight)
					{
						mPlayerState.Copy(playerClustersState.second[otherPlayerCluster]);
						playerCluster = playerClustersState.first;
					}
				}

				mAIManager->PrintLogInformationDetails("\n player cluster : " +
					eastl::to_string(playerClustersState.first->GetTarget()->GetCluster()) + " ");
				mAIManager->PrintLogInformationDetails("heuristic : " +
					eastl::to_string(playerClustersState.second[otherPlayerCluster].heuristic) + " ");

				if (playerClustersState.second[otherPlayerCluster].weapon != WP_NONE)
				{
					mAIManager->PrintLogInformationDetails("weapon : " +
						eastl::to_string(playerClustersState.second[otherPlayerCluster].weapon) + " ");
					mAIManager->PrintLogInformationDetails("damage : " +
						eastl::to_string(
							playerClustersState.second[otherPlayerCluster].damage[
							playerClustersState.second[otherPlayerCluster].weapon - 1]) + " ");
				}

				if (!playerClustersState.second[otherPlayerCluster].items.empty())
					mAIManager->PrintLogInformationDetails("actors : ");
				for (eastl::shared_ptr<Actor> pItemActor : playerClustersState.second[otherPlayerCluster].items)
				{
					if (pItemActor->GetType() == "Weapon")
					{
						eastl::shared_ptr<WeaponPickup> pWeaponPickup =
							pItemActor->GetComponent<WeaponPickup>(WeaponPickup::Name).lock();
						mAIManager->PrintLogInformationDetails("weapon " + eastl::to_string(pWeaponPickup->GetCode()) + " ");
					}
					else if (pItemActor->GetType() == "Ammo")
					{
						eastl::shared_ptr<AmmoPickup> pAmmoPickup =
							pItemActor->GetComponent<AmmoPickup>(AmmoPickup::Name).lock();
						mAIManager->PrintLogInformationDetails("ammo " + eastl::to_string(pAmmoPickup->GetCode()) + " ");
					}
					else if (pItemActor->GetType() == "Armor")
					{
						eastl::shared_ptr<ArmorPickup> pArmorPickup =
							pItemActor->GetComponent<ArmorPickup>(ArmorPickup::Name).lock();
						mAIManager->PrintLogInformationDetails("armor " + eastl::to_string(pArmorPickup->GetCode()) + " ");
					}
					else if (pItemActor->GetType() == "Health")
					{
						eastl::shared_ptr<HealthPickup> pHealthPickup =
							pItemActor->GetComponent<HealthPickup>(HealthPickup::Name).lock();
						mAIManager->PrintLogInformationDetails("health " + eastl::to_string(pHealthPickup->GetCode()) + " ");
					}
				}
			}

			if (currentClusterPlanStates.find(otherPlayerCluster) != currentClusterPlanStates.end())
			{
				//we keep the current plan if the heuristic is less or close to the best player heuristic
				if (abs(currentClusterPlanStates[otherPlayerCluster].heuristic - mPlayerState.heuristic) < 0.02f)
				{
					mPlayerState.Copy(currentClusterPlanStates[otherPlayerCluster]);
					playerCluster = NULL;
				}
				else if (currentClusterPlanStates[otherPlayerCluster].heuristic > mPlayerState.heuristic)
				{
					mPlayerState.Copy(currentClusterPlanStates[otherPlayerCluster]);
					playerCluster = NULL;
				}

				mAIManager->PrintLogInformationDetails("\n current player heuristic : " +
					eastl::to_string(currentClusterPlanStates[otherPlayerCluster].heuristic) + " ");

				if (currentClusterPlanStates[otherPlayerCluster].weapon != WP_NONE)
				{
					mAIManager->PrintLogInformationDetails("weapon : " +
						eastl::to_string(currentClusterPlanStates[otherPlayerCluster].weapon) + " ");
					mAIManager->PrintLogInformationDetails("damage : " +
						eastl::to_string(
							currentClusterPlanStates[otherPlayerCluster].damage[
							currentClusterPlanStates[otherPlayerCluster].weapon - 1]) + " ");
				}

				if (!currentClusterPlanStates[otherPlayerCluster].items.empty())
					mAIManager->PrintLogInformationDetails("actors : ");
				for (eastl::shared_ptr<Actor> pItemActor : currentClusterPlanStates[otherPlayerCluster].items)
				{
					if (pItemActor->GetType() == "Weapon")
					{
						eastl::shared_ptr<WeaponPickup> pWeaponPickup =
							pItemActor->GetComponent<WeaponPickup>(WeaponPickup::Name).lock();
						mAIManager->PrintLogInformationDetails("weapon " + eastl::to_string(pWeaponPickup->GetCode()) + " ");
					}
					else if (pItemActor->GetType() == "Ammo")
					{
						eastl::shared_ptr<AmmoPickup> pAmmoPickup =
							pItemActor->GetComponent<AmmoPickup>(AmmoPickup::Name).lock();
						mAIManager->PrintLogInformationDetails("ammo " + eastl::to_string(pAmmoPickup->GetCode()) + " ");
					}
					else if (pItemActor->GetType() == "Armor")
					{
						eastl::shared_ptr<ArmorPickup> pArmorPickup =
							pItemActor->GetComponent<ArmorPickup>(ArmorPickup::Name).lock();
						mAIManager->PrintLogInformationDetails("armor " + eastl::to_string(pArmorPickup->GetCode()) + " ");
					}
					else if (pItemActor->GetType() == "Health")
					{
						eastl::shared_ptr<HealthPickup> pHealthPickup =
							pItemActor->GetComponent<HealthPickup>(HealthPickup::Name).lock();
						mAIManager->PrintLogInformationDetails("health " + eastl::to_string(pHealthPickup->GetCode()) + " ");
					}
				}
			}
		}
		else
		{
			mAIManager->PrintLogInformation("\n same other player decision tree target cluster");
			mAIManager->PrintLogInformationDetails("\n\n same other player decision tree target cluster");

			mPlayerState.Copy(playerState);
			mPlayerState.heuristic = -FLT_MAX;
			mPlayerState.valid = false;
			for (auto currentClusterState : currentClusterStates)
			{
				if (currentClusterState.second.heuristic > mPlayerState.heuristic)
				{
					mPlayerState.Copy(currentClusterState.second);
					playerCluster = currentClusterState.first;
				}
				else if (abs(currentClusterState.second.heuristic - mPlayerState.heuristic) < GE_ROUNDING_ERROR)
				{
					//lets take the longest path for trying to avoid wandering situations
					if (currentClusterState.second.plan.weight > mPlayerState.plan.weight)
					{
						mPlayerState.Copy(currentClusterState.second);
						playerCluster = currentClusterState.first;
					}
				}

				mAIManager->PrintLogInformationDetails("\n player cluster : " +
					eastl::to_string(currentClusterState.first->GetTarget()->GetCluster()) + " ");
				mAIManager->PrintLogInformationDetails("heuristic : " +
					eastl::to_string(currentClusterState.second.heuristic) + " ");

				if (currentClusterState.second.weapon != WP_NONE)
				{
					mAIManager->PrintLogInformationDetails("weapon : " +
						eastl::to_string(currentClusterState.second.weapon) + " ");
					mAIManager->PrintLogInformationDetails("damage : " +
						eastl::to_string(
							currentClusterState.second.damage[
							currentClusterState.second.weapon - 1]) + " ");
				}

				if (!currentClusterState.second.items.empty())
					mAIManager->PrintLogInformationDetails("actors : ");
				for (eastl::shared_ptr<Actor> pItemActor : currentClusterState.second.items)
				{
					if (pItemActor->GetType() == "Weapon")
					{
						eastl::shared_ptr<WeaponPickup> pWeaponPickup =
							pItemActor->GetComponent<WeaponPickup>(WeaponPickup::Name).lock();
						mAIManager->PrintLogInformationDetails("weapon " + eastl::to_string(pWeaponPickup->GetCode()) + " ");
					}
					else if (pItemActor->GetType() == "Ammo")
					{
						eastl::shared_ptr<AmmoPickup> pAmmoPickup =
							pItemActor->GetComponent<AmmoPickup>(AmmoPickup::Name).lock();
						mAIManager->PrintLogInformationDetails("ammo " + eastl::to_string(pAmmoPickup->GetCode()) + " ");
					}
					else if (pItemActor->GetType() == "Armor")
					{
						eastl::shared_ptr<ArmorPickup> pArmorPickup =
							pItemActor->GetComponent<ArmorPickup>(ArmorPickup::Name).lock();
						mAIManager->PrintLogInformationDetails("armor " + eastl::to_string(pArmorPickup->GetCode()) + " ");
					}
					else if (pItemActor->GetType() == "Health")
					{
						eastl::shared_ptr<HealthPickup> pHealthPickup =
							pItemActor->GetComponent<HealthPickup>(HealthPickup::Name).lock();
						mAIManager->PrintLogInformationDetails("health " + eastl::to_string(pHealthPickup->GetCode()) + " ");
					}
				}
			}

			if (currentPlanState.valid)
			{
				//we keep the current plan if the heuristic is less or close to the best player heuristic
				if (abs(currentPlanState.heuristic - mPlayerState.heuristic) < 0.02f)
				{
					mPlayerState.Copy(currentPlanState);
					playerCluster = NULL;
				}
				else if (currentPlanState.heuristic > mPlayerState.heuristic)
				{
					mPlayerState.Copy(currentPlanState);
					playerCluster = NULL;
				}
			}
		}

		if (playerCluster != NULL)
		{
			mPlayerState.plan.id = mAIManager->GetNewPlanID();

			mAIManager->PrintLogInformation("\n player current position : " +
				eastl::to_string(playerState.plan.node->GetPos()[0]) + " " +
				eastl::to_string(playerState.plan.node->GetPos()[1]) + " " +
				eastl::to_string(playerState.plan.node->GetPos()[2]) + " ");
			mAIManager->PrintLogInformation("\n player current cluster : " +
				eastl::to_string(playerState.plan.node->GetCluster()) + " ");
			mAIManager->PrintLogInformation("\n player target position : " +
				eastl::to_string(playerCluster->GetTarget()->GetPos()[0]) + " " +
				eastl::to_string(playerCluster->GetTarget()->GetPos()[1]) + " " +
				eastl::to_string(playerCluster->GetTarget()->GetPos()[2]) + " ");
			mAIManager->PrintLogInformation("\n player target cluster : " +
				eastl::to_string(playerCluster->GetTarget()->GetCluster()) + " ");

			for (auto playerClusterState : playerClustersStates[playerCluster])
			{
				mAIManager->PrintLogInformation("other player cluster : " +
					eastl::to_string(playerClusterState.first->GetTarget()->GetCluster()) + " ");
				mAIManager->PrintLogInformation("heuristic : " +
					eastl::to_string(playerClusterState.second.heuristic) + " ");

				if (playerClusterState.second.weapon != WP_NONE)
				{
					mAIManager->PrintLogInformation("weapon : " +
						eastl::to_string(playerClusterState.second.weapon) + " ");
					mAIManager->PrintLogInformation("damage : " +
						eastl::to_string(playerClusterState.second.damage[playerClusterState.second.weapon - 1]) + " ");
				}

				if (!playerClusterState.second.items.empty())
					mAIManager->PrintLogInformation("actors : ");
				for (eastl::shared_ptr<Actor> pItemActor : playerClusterState.second.items)
				{
					if (pItemActor->GetType() == "Weapon")
					{
						eastl::shared_ptr<WeaponPickup> pWeaponPickup =
							pItemActor->GetComponent<WeaponPickup>(WeaponPickup::Name).lock();
						mAIManager->PrintLogInformation("weapon " + eastl::to_string(pWeaponPickup->GetCode()) + " ");
					}
					else if (pItemActor->GetType() == "Ammo")
					{
						eastl::shared_ptr<AmmoPickup> pAmmoPickup =
							pItemActor->GetComponent<AmmoPickup>(AmmoPickup::Name).lock();
						mAIManager->PrintLogInformation("ammo " + eastl::to_string(pAmmoPickup->GetCode()) + " ");
					}
					else if (pItemActor->GetType() == "Armor")
					{
						eastl::shared_ptr<ArmorPickup> pArmorPickup =
							pItemActor->GetComponent<ArmorPickup>(ArmorPickup::Name).lock();
						mAIManager->PrintLogInformation("armor " + eastl::to_string(pArmorPickup->GetCode()) + " ");
					}
					else if (pItemActor->GetType() == "Health")
					{
						eastl::shared_ptr<HealthPickup> pHealthPickup =
							pItemActor->GetComponent<HealthPickup>(HealthPickup::Name).lock();
						mAIManager->PrintLogInformation("health " + eastl::to_string(pHealthPickup->GetCode()) + " ");
					}
				}
			}
		}
		else
		{
			mAIManager->PrintLogInformation("\n player same position : " +
				eastl::to_string(playerState.plan.node->GetPos()[0]) + " " +
				eastl::to_string(playerState.plan.node->GetPos()[1]) + " " +
				eastl::to_string(playerState.plan.node->GetPos()[2]) + " ");
			mAIManager->PrintLogInformation("\n player same plan cluster : " +
				eastl::to_string(playerState.plan.node->GetCluster()) + " ");

			for (auto currentClusterPlanState : currentClusterPlanStates)
			{
				mAIManager->PrintLogInformation("other player cluster : " +
					eastl::to_string(currentClusterPlanState.first->GetTarget()->GetCluster()) + " ");
				mAIManager->PrintLogInformation("heuristic : " +
					eastl::to_string(currentClusterPlanState.second.heuristic) + " ");

				if (currentClusterPlanState.second.weapon != WP_NONE)
				{
					mAIManager->PrintLogInformation("weapon : " +
						eastl::to_string(currentClusterPlanState.second.weapon) + " ");
					mAIManager->PrintLogInformation("damage : " + eastl::to_string(
						currentClusterPlanState.second.damage[currentClusterPlanState.second.weapon - 1]) + " ");
				}

				if (!currentClusterPlanState.second.items.empty())
					mAIManager->PrintLogInformation("actors : ");
				for (eastl::shared_ptr<Actor> pItemActor : currentClusterPlanState.second.items)
				{
					if (pItemActor->GetType() == "Weapon")
					{
						eastl::shared_ptr<WeaponPickup> pWeaponPickup =
							pItemActor->GetComponent<WeaponPickup>(WeaponPickup::Name).lock();
						mAIManager->PrintLogInformation("weapon " + eastl::to_string(pWeaponPickup->GetCode()) + " ");
					}
					else if (pItemActor->GetType() == "Ammo")
					{
						eastl::shared_ptr<AmmoPickup> pAmmoPickup =
							pItemActor->GetComponent<AmmoPickup>(AmmoPickup::Name).lock();
						mAIManager->PrintLogInformation("ammo " + eastl::to_string(pAmmoPickup->GetCode()) + " ");
					}
					else if (pItemActor->GetType() == "Armor")
					{
						eastl::shared_ptr<ArmorPickup> pArmorPickup =
							pItemActor->GetComponent<ArmorPickup>(ArmorPickup::Name).lock();
						mAIManager->PrintLogInformation("armor " + eastl::to_string(pArmorPickup->GetCode()) + " ");
					}
					else if (pItemActor->GetType() == "Health")
					{
						eastl::shared_ptr<HealthPickup> pHealthPickup =
							pItemActor->GetComponent<HealthPickup>(HealthPickup::Name).lock();
						mAIManager->PrintLogInformation("health " + eastl::to_string(pHealthPickup->GetCode()) + " ");
					}
				}
			}
		}

		mAIManager->PrintLogInformation("\n player path id " + eastl::to_string(mPlayerState.plan.id) + " : ");
		for (PathingArc* pathArc : mPlayerState.plan.path)
			mAIManager->PrintLogInformation(eastl::to_string(pathArc->GetNode()->GetId()) + " ");
		mAIManager->PrintLogInformation("\n player heuristic : " + eastl::to_string(mPlayerState.heuristic) + " ");

		if (mPlayerState.weapon != WP_NONE)
		{
			mAIManager->PrintLogInformation("weapon : " +
				eastl::to_string(mPlayerState.weapon) + " ");
			mAIManager->PrintLogInformation("damage : " +
				eastl::to_string(mPlayerState.damage[mPlayerState.weapon - 1]) + " ");
		}

		if (!mPlayerState.items.empty())
			mAIManager->PrintLogInformation("actors : ");
		for (eastl::shared_ptr<Actor> pItemActor : mPlayerState.items)
		{
			if (pItemActor->GetType() == "Weapon")
			{
				eastl::shared_ptr<WeaponPickup> pWeaponPickup =
					pItemActor->GetComponent<WeaponPickup>(WeaponPickup::Name).lock();
				mAIManager->PrintLogInformation("weapon " + eastl::to_string(pWeaponPickup->GetCode()) + " ");
			}
			else if (pItemActor->GetType() == "Ammo")
			{
				eastl::shared_ptr<AmmoPickup> pAmmoPickup =
					pItemActor->GetComponent<AmmoPickup>(AmmoPickup::Name).lock();
				mAIManager->PrintLogInformation("ammo " + eastl::to_string(pAmmoPickup->GetCode()) + " ");
			}
			else if (pItemActor->GetType() == "Armor")
			{
				eastl::shared_ptr<ArmorPickup> pArmorPickup =
					pItemActor->GetComponent<ArmorPickup>(ArmorPickup::Name).lock();
				mAIManager->PrintLogInformation("armor " + eastl::to_string(pArmorPickup->GetCode()) + " ");
			}
			else if (pItemActor->GetType() == "Health")
			{
				eastl::shared_ptr<HealthPickup> pHealthPickup =
					pItemActor->GetComponent<HealthPickup>(HealthPickup::Name).lock();
				mAIManager->PrintLogInformation("health " + eastl::to_string(pHealthPickup->GetCode()) + " ");
			}
		}
	}

	if (mOtherPlayerState.valid)
	{
		if (otherPlayerCluster != NULL)
		{
			mOtherPlayerState.plan.id = mAIManager->GetNewPlanID();

			mAIManager->PrintLogInformation("\n other player current position : " +
				eastl::to_string(otherPlayerState.plan.node->GetPos()[0]) + " " +
				eastl::to_string(otherPlayerState.plan.node->GetPos()[1]) + " " +
				eastl::to_string(otherPlayerState.plan.node->GetPos()[2]) + " ");
			mAIManager->PrintLogInformation("\n other player current cluster : " +
				eastl::to_string(otherPlayerState.plan.node->GetCluster()) + " ");
			mAIManager->PrintLogInformation("\n other player target position : " +
				eastl::to_string(otherPlayerCluster->GetTarget()->GetPos()[0]) + " " +
				eastl::to_string(otherPlayerCluster->GetTarget()->GetPos()[1]) + " " +
				eastl::to_string(otherPlayerCluster->GetTarget()->GetPos()[2]) + " ");
			mAIManager->PrintLogInformation("\n other player target cluster : " +
				eastl::to_string(otherPlayerCluster->GetTarget()->GetCluster()) + " ");

			for (auto otherPlayerClusterState : otherPlayerClustersStates[otherPlayerCluster])
			{
				mAIManager->PrintLogInformation("player cluster : " +
					eastl::to_string(otherPlayerClusterState.first->GetTarget()->GetCluster()) + " ");
				mAIManager->PrintLogInformation("heuristic : " +
					eastl::to_string(otherPlayerClusterState.second.heuristic) + " ");

				if (otherPlayerClusterState.second.weapon != WP_NONE)
				{
					mAIManager->PrintLogInformation("weapon : " +
						eastl::to_string(otherPlayerClusterState.second.weapon) + " ");
					mAIManager->PrintLogInformation("damage : " + eastl::to_string(
						otherPlayerClusterState.second.damage[otherPlayerClusterState.second.weapon - 1]) + " ");
				}

				if (!otherPlayerClusterState.second.items.empty())
					mAIManager->PrintLogInformation("actors : ");
				for (eastl::shared_ptr<Actor> pItemActor : otherPlayerClusterState.second.items)
				{
					if (pItemActor->GetType() == "Weapon")
					{
						eastl::shared_ptr<WeaponPickup> pWeaponPickup =
							pItemActor->GetComponent<WeaponPickup>(WeaponPickup::Name).lock();
						mAIManager->PrintLogInformation("weapon " + eastl::to_string(pWeaponPickup->GetCode()) + " ");
					}
					else if (pItemActor->GetType() == "Ammo")
					{
						eastl::shared_ptr<AmmoPickup> pAmmoPickup =
							pItemActor->GetComponent<AmmoPickup>(AmmoPickup::Name).lock();
						mAIManager->PrintLogInformation("ammo " + eastl::to_string(pAmmoPickup->GetCode()) + " ");
					}
					else if (pItemActor->GetType() == "Armor")
					{
						eastl::shared_ptr<ArmorPickup> pArmorPickup =
							pItemActor->GetComponent<ArmorPickup>(ArmorPickup::Name).lock();
						mAIManager->PrintLogInformation("armor " + eastl::to_string(pArmorPickup->GetCode()) + " ");
					}
					else if (pItemActor->GetType() == "Health")
					{
						eastl::shared_ptr<HealthPickup> pHealthPickup =
							pItemActor->GetComponent<HealthPickup>(HealthPickup::Name).lock();
						mAIManager->PrintLogInformation("health " + eastl::to_string(pHealthPickup->GetCode()) + " ");
					}
				}
			}
		}
		else
		{
			mAIManager->PrintLogInformation("\n other player same position : " +
				eastl::to_string(otherPlayerState.plan.node->GetPos()[0]) + " " +
				eastl::to_string(otherPlayerState.plan.node->GetPos()[1]) + " " +
				eastl::to_string(otherPlayerState.plan.node->GetPos()[2]) + " ");
			mAIManager->PrintLogInformation("\n other player same plan cluster : " +
				eastl::to_string(otherPlayerState.plan.node->GetCluster()) + " ");

			for (auto otherCurrentClusterPlanState : otherCurrentClusterPlanStates)
			{
				mAIManager->PrintLogInformation("player cluster : " +
					eastl::to_string(otherCurrentClusterPlanState.first->GetTarget()->GetCluster()) + " ");
				mAIManager->PrintLogInformation("heuristic : " +
					eastl::to_string(otherCurrentClusterPlanState.second.heuristic) + " ");

				if (otherCurrentClusterPlanState.second.weapon != WP_NONE)
				{
					mAIManager->PrintLogInformation("weapon : " +
						eastl::to_string(otherCurrentClusterPlanState.second.weapon) + " ");
					mAIManager->PrintLogInformation("damage : " + eastl::to_string(
						otherCurrentClusterPlanState.second.damage[otherCurrentClusterPlanState.second.weapon - 1]) + " ");
				}

				if (!otherCurrentClusterPlanState.second.items.empty())
					mAIManager->PrintLogInformation("actors : ");
				for (eastl::shared_ptr<Actor> pItemActor : otherCurrentClusterPlanState.second.items)
				{
					if (pItemActor->GetType() == "Weapon")
					{
						eastl::shared_ptr<WeaponPickup> pWeaponPickup =
							pItemActor->GetComponent<WeaponPickup>(WeaponPickup::Name).lock();
						mAIManager->PrintLogInformation("weapon " + eastl::to_string(pWeaponPickup->GetCode()) + " ");
					}
					else if (pItemActor->GetType() == "Ammo")
					{
						eastl::shared_ptr<AmmoPickup> pAmmoPickup =
							pItemActor->GetComponent<AmmoPickup>(AmmoPickup::Name).lock();
						mAIManager->PrintLogInformation("ammo " + eastl::to_string(pAmmoPickup->GetCode()) + " ");
					}
					else if (pItemActor->GetType() == "Armor")
					{
						eastl::shared_ptr<ArmorPickup> pArmorPickup =
							pItemActor->GetComponent<ArmorPickup>(ArmorPickup::Name).lock();
						mAIManager->PrintLogInformation("armor " + eastl::to_string(pArmorPickup->GetCode()) + " ");
					}
					else if (pItemActor->GetType() == "Health")
					{
						eastl::shared_ptr<HealthPickup> pHealthPickup =
							pItemActor->GetComponent<HealthPickup>(HealthPickup::Name).lock();
						mAIManager->PrintLogInformation("health " + eastl::to_string(pHealthPickup->GetCode()) + " ");
					}
				}
			}
		}

		mAIManager->PrintLogInformation("\n other player path id " +
			eastl::to_string(mOtherPlayerState.plan.id) + " : ");
		for (PathingArc* pathArc : mOtherPlayerState.plan.path)
			mAIManager->PrintLogInformation(eastl::to_string(pathArc->GetNode()->GetId()) + " ");
		mAIManager->PrintLogInformation("\n other player heuristic : " +
			eastl::to_string(mOtherPlayerState.heuristic) + " ");

		if (mOtherPlayerState.weapon != WP_NONE)
		{
			mAIManager->PrintLogInformation("weapon : " +
				eastl::to_string(mOtherPlayerState.weapon) + " ");
			mAIManager->PrintLogInformation("damage : " +
				eastl::to_string(mOtherPlayerState.damage[mOtherPlayerState.weapon - 1]) + " ");
		}

		if (!mOtherPlayerState.items.empty())
			mAIManager->PrintLogInformation("actors : ");
		for (eastl::shared_ptr<Actor> pItemActor : mOtherPlayerState.items)
		{
			if (pItemActor->GetType() == "Weapon")
			{
				eastl::shared_ptr<WeaponPickup> pWeaponPickup =
					pItemActor->GetComponent<WeaponPickup>(WeaponPickup::Name).lock();
				mAIManager->PrintLogInformation("weapon " + eastl::to_string(pWeaponPickup->GetCode()) + " ");
			}
			else if (pItemActor->GetType() == "Ammo")
			{
				eastl::shared_ptr<AmmoPickup> pAmmoPickup =
					pItemActor->GetComponent<AmmoPickup>(AmmoPickup::Name).lock();
				mAIManager->PrintLogInformation("ammo " + eastl::to_string(pAmmoPickup->GetCode()) + " ");
			}
			else if (pItemActor->GetType() == "Armor")
			{
				eastl::shared_ptr<ArmorPickup> pArmorPickup =
					pItemActor->GetComponent<ArmorPickup>(ArmorPickup::Name).lock();
				mAIManager->PrintLogInformation("armor " + eastl::to_string(pArmorPickup->GetCode()) + " ");
			}
			else if (pItemActor->GetType() == "Health")
			{
				eastl::shared_ptr<HealthPickup> pHealthPickup =
					pItemActor->GetComponent<HealthPickup>(HealthPickup::Name).lock();
				mAIManager->PrintLogInformation("health " + eastl::to_string(pHealthPickup->GetCode()) + " ");
			}
		}
	}
}

void QuakeAIProcess::ThreadProc( )
{
	unsigned int iteration = 0;

	while (true)
	{
		if (GameLogic::Get()->GetState() == BGS_RUNNING)
		{
			eastl::map<GameViewType, eastl::vector<ActorId>> players;

			GameApplication* gameApp = (GameApplication*)Application::App;
			const GameViewList& gameViews = gameApp->GetGameViews();
			for (auto it = gameViews.begin(); it != gameViews.end(); ++it)
			{
				eastl::shared_ptr<BaseGameView> pView = *it;
				if (pView->GetActorId() != INVALID_ACTOR_ID)
					players[pView->GetType()].push_back(pView->GetActorId());
			}

			if (players.find(GV_HUMAN) != players.end() && players.find(GV_AI) != players.end())
			{
				eastl::vector<ActorId>::iterator itAIPlayer;
				for (itAIPlayer = players[GV_AI].begin(); itAIPlayer != players[GV_AI].end(); )
				{
					bool removeAIPlayer = false;
					for (ActorId player : players[GV_HUMAN])
						if (player == (*itAIPlayer))
							removeAIPlayer = true;

					if (removeAIPlayer)
						itAIPlayer = players[GV_AI].erase(itAIPlayer);
					else
						itAIPlayer++;
				}

				iteration++;
				//printf("\n ITERATION %u", iteration);
				Timer::RealTimeDate realTime = Timer::GetRealTimeAndDate();
				eastl::string info = "\n\n ITERATION " + 
					eastl::to_string(iteration) + " time " + 
					eastl::to_string(realTime.Hour) + ":" + 
					eastl::to_string(realTime.Minute) + ":" + 
					eastl::to_string(realTime.Second) + "\n";
				mAIManager->PrintLogInformation(info);
				mAIManager->PrintLogInformationDetails(info);

				info = "\n blue player ai guessing (red)";
				mAIManager->PrintLogInformation(info);
				mAIManager->PrintLogInformationDetails(info);
				for (ActorId player : players[GV_HUMAN])
				{
					eastl::shared_ptr<PlayerActor> pHumanPlayer =
						eastl::dynamic_shared_pointer_cast<PlayerActor>(
						GameLogic::Get()->GetActor(player).lock());

					NodeState playerState(pHumanPlayer);
					playerState.weapon = mAIManager->GetPlayerWeapon(player);
					playerState.weaponTarget = mAIManager->GetPlayerWeaponTarget(player);
					mAIManager->GetPlayerPlan(player, playerState.plan);
					
					if (playerState.plan.path.size() > 1)
					{
						//search for the starting node closest to the actual player position
						eastl::shared_ptr<TransformComponent> pTransformComponent(
							pHumanPlayer->GetComponent<TransformComponent>(TransformComponent::Name).lock());
						if (pTransformComponent)
						{
							Vector3<float> playerPosition = pTransformComponent->GetPosition();
							if (Length(playerState.plan.path[0]->GetNode()->GetPos() - playerPosition) >
								Length(playerState.plan.path[1]->GetNode()->GetPos() - playerPosition))
							{
								PathingNode* playerNode = playerState.plan.path[0]->GetNode();
								unsigned int playerAction = playerState.plan.path[0]->GetType();
								if (playerAction != GAT_PUSH && playerAction != GAT_TELEPORT)
								{
									playerState.plan.node = playerNode;
									playerState.plan.path.erase(playerState.plan.path.begin());
								}
							}
						}
					}

					for (ActorId aiPlayer : players[GV_AI])
					{
						NodeState aiPlayerState;
						mAIManager->GetPlayerGuessState(aiPlayer, aiPlayerState);
						mAIManager->GetPlayerGuessPlan(aiPlayer, aiPlayerState.plan);
						aiPlayerState.ResetItems();

						mExcludeActors.clear();
						mAIManager->GetPlayerGuessItems(aiPlayerState.player, mExcludeActors);
						EvaluatePlayers(playerState, aiPlayerState);
					}
				}

				if (!mExcludeActors.empty())
				{
					info = "\n exclude actors : ";
					mAIManager->PrintLogInformation(info);
					mAIManager->PrintLogInformationDetails(info);
				}

				for (auto excludeActor : mExcludeActors)
				{
					eastl::shared_ptr<Actor> pItemActor(
						GameLogic::Get()->GetActor(excludeActor.first).lock());
					if (pItemActor->GetType() == "Weapon")
					{
						eastl::shared_ptr<WeaponPickup> pWeaponPickup =
							pItemActor->GetComponent<WeaponPickup>(WeaponPickup::Name).lock();

						info = "weapon " + eastl::to_string(pWeaponPickup->GetCode()) + " ";
						mAIManager->PrintLogInformation(info);
						mAIManager->PrintLogInformationDetails(info);
					}
					else if (pItemActor->GetType() == "Ammo")
					{
						eastl::shared_ptr<AmmoPickup> pAmmoPickup =
							pItemActor->GetComponent<AmmoPickup>(AmmoPickup::Name).lock();

						info = "ammo " + eastl::to_string(pAmmoPickup->GetCode()) + " ";
						mAIManager->PrintLogInformation(info);
						mAIManager->PrintLogInformationDetails(info);
					}
					else if (pItemActor->GetType() == "Armor")
					{
						eastl::shared_ptr<ArmorPickup> pArmorPickup =
							pItemActor->GetComponent<ArmorPickup>(ArmorPickup::Name).lock();

						info = "armor " + eastl::to_string(pArmorPickup->GetCode()) + " ";
						mAIManager->PrintLogInformation(info);
						mAIManager->PrintLogInformationDetails(info);
					}
					else if (pItemActor->GetType() == "Health")
					{
						eastl::shared_ptr<HealthPickup> pHealthPickup =
							pItemActor->GetComponent<HealthPickup>(HealthPickup::Name).lock();

						info = "health " + eastl::to_string(pHealthPickup->GetCode()) + " ";
						mAIManager->PrintLogInformation(info);
						mAIManager->PrintLogInformationDetails(info);
					}
				}

				info = "\n blue player heuristic " + eastl::to_string(mPlayerState.heuristic) + " ";
				mAIManager->PrintLogInformation(info);
				mAIManager->PrintLogInformationDetails(info);
				if (!mPlayerState.items.empty())
				{
					info = " actors : ";
					mAIManager->PrintLogInformation(info);
					mAIManager->PrintLogInformationDetails(info);
				}
				for (eastl::shared_ptr<Actor> pItemActor : mPlayerState.items)
				{
					if (pItemActor->GetType() == "Weapon")
					{
						eastl::shared_ptr<WeaponPickup> pWeaponPickup =
							pItemActor->GetComponent<WeaponPickup>(WeaponPickup::Name).lock();

						info = "weapon " + eastl::to_string(pWeaponPickup->GetCode()) + " ";
						mAIManager->PrintLogInformation(info);
						mAIManager->PrintLogInformationDetails(info);
					}
					else if (pItemActor->GetType() == "Ammo")
					{
						eastl::shared_ptr<AmmoPickup> pAmmoPickup =
							pItemActor->GetComponent<AmmoPickup>(AmmoPickup::Name).lock();

						info = "ammo " + eastl::to_string(pAmmoPickup->GetCode()) + " ";
						mAIManager->PrintLogInformation(info);
						mAIManager->PrintLogInformationDetails(info);
					}
					else if (pItemActor->GetType() == "Armor")
					{
						eastl::shared_ptr<ArmorPickup> pArmorPickup =
							pItemActor->GetComponent<ArmorPickup>(ArmorPickup::Name).lock();

						info = "armor " + eastl::to_string(pArmorPickup->GetCode()) + " ";
						mAIManager->PrintLogInformation(info);
						mAIManager->PrintLogInformationDetails(info);
					}
					else if (pItemActor->GetType() == "Health")
					{
						eastl::shared_ptr<HealthPickup> pHealthPickup =
							pItemActor->GetComponent<HealthPickup>(HealthPickup::Name).lock();

						info = "health " + eastl::to_string(pHealthPickup->GetCode()) + " ";
						mAIManager->PrintLogInformation(info);
						mAIManager->PrintLogInformationDetails(info);
					}
				}
				if (mPlayerState.weapon != WP_NONE)
				{
					info = "weapon : " +  eastl::to_string(mPlayerState.weapon) + " ";
					mAIManager->PrintLogInformation(info);
					mAIManager->PrintLogInformationDetails(info);

					info = "damage : " +
						eastl::to_string(mPlayerState.damage[mPlayerState.weapon - 1]) + " ";
					mAIManager->PrintLogInformation(info);
					mAIManager->PrintLogInformationDetails(info);
				}
				else
				{
					info = "weapon : 0 ";
					mAIManager->PrintLogInformation(info);
					mAIManager->PrintLogInformationDetails(info);

					info = "damage : 0 ";
					mAIManager->PrintLogInformation(info);
					mAIManager->PrintLogInformationDetails(info);
				}

				mAIManager->SetPlayerGuessState(mOtherPlayerState.player, mOtherPlayerState);
				mAIManager->SetPlayerGuessUpdated(mOtherPlayerState.player, true);

				mAIManager->SetPlayerState(mPlayerState.player, mPlayerState);
				mAIManager->SetPlayerUpdated(mPlayerState.player, true);

				info = "\n\n red player ai guessing (blue)";
				mAIManager->PrintLogInformation(info);
				mAIManager->PrintLogInformationDetails(info);
				for (ActorId aiPlayer : players[GV_AI])
				{
					eastl::shared_ptr<PlayerActor> pAIPlayer =
						eastl::dynamic_shared_pointer_cast<PlayerActor>(
						GameLogic::Get()->GetActor(aiPlayer).lock());

					NodeState aiPlayerState(pAIPlayer);
					aiPlayerState.weapon = mAIManager->GetPlayerWeapon(aiPlayer);
					aiPlayerState.weaponTarget = mAIManager->GetPlayerWeaponTarget(aiPlayer);
					mAIManager->GetPlayerPlan(aiPlayer, aiPlayerState.plan);

					if (aiPlayerState.plan.path.size() > 1)
					{
						//search for the starting node closest to the actual ai player position
						eastl::shared_ptr<TransformComponent> pTransformComponent(
							pAIPlayer->GetComponent<TransformComponent>(TransformComponent::Name).lock());
						if (pTransformComponent)
						{
							Vector3<float> aiPlayerPosition = pTransformComponent->GetPosition();
							if (Length(aiPlayerState.plan.path[0]->GetNode()->GetPos() - aiPlayerPosition) >
								Length(aiPlayerState.plan.path[1]->GetNode()->GetPos() - aiPlayerPosition))
							{
								PathingNode* aiPlayerNode = aiPlayerState.plan.path[0]->GetNode();
								unsigned int aiPlayerAction = aiPlayerState.plan.path[0]->GetType();
								if (aiPlayerAction != GAT_PUSH && aiPlayerAction != GAT_TELEPORT)
								{
									aiPlayerState.plan.node = aiPlayerNode;
									aiPlayerState.plan.path.erase(aiPlayerState.plan.path.begin());
								}
							}
						}
					}

					for (ActorId player : players[GV_HUMAN])
					{
						NodeState playerState;
						mAIManager->GetPlayerGuessState(player, playerState);
						mAIManager->GetPlayerGuessPlan(player, playerState.plan);
						playerState.ResetItems();

						mExcludeActors.clear();
						mAIManager->GetPlayerGuessItems(playerState.player, mExcludeActors);
						EvaluatePlayers(aiPlayerState, playerState);
					}
				}

				if (!mExcludeActors.empty())
				{
					info = "\n exclude actors : ";
					mAIManager->PrintLogInformation(info);
					mAIManager->PrintLogInformationDetails(info);
				}

				for (auto excludeActor : mExcludeActors)
				{
					eastl::shared_ptr<Actor> pItemActor(
						GameLogic::Get()->GetActor(excludeActor.first).lock());
					if (pItemActor->GetType() == "Weapon")
					{
						eastl::shared_ptr<WeaponPickup> pWeaponPickup =
							pItemActor->GetComponent<WeaponPickup>(WeaponPickup::Name).lock();

						info = "weapon " + eastl::to_string(pWeaponPickup->GetCode()) + " ";
						mAIManager->PrintLogInformation(info);
						mAIManager->PrintLogInformationDetails(info);
					}
					else if (pItemActor->GetType() == "Ammo")
					{
						eastl::shared_ptr<AmmoPickup> pAmmoPickup =
							pItemActor->GetComponent<AmmoPickup>(AmmoPickup::Name).lock();

						info = "ammo " + eastl::to_string(pAmmoPickup->GetCode()) + " ";
						mAIManager->PrintLogInformation(info);
						mAIManager->PrintLogInformationDetails(info);
					}
					else if (pItemActor->GetType() == "Armor")
					{
						eastl::shared_ptr<ArmorPickup> pArmorPickup =
							pItemActor->GetComponent<ArmorPickup>(ArmorPickup::Name).lock();

						info = "armor " + eastl::to_string(pArmorPickup->GetCode()) + " ";
						mAIManager->PrintLogInformation(info);
						mAIManager->PrintLogInformationDetails(info);
					}
					else if (pItemActor->GetType() == "Health")
					{
						eastl::shared_ptr<HealthPickup> pHealthPickup =
							pItemActor->GetComponent<HealthPickup>(HealthPickup::Name).lock();

						info = "health " + eastl::to_string(pHealthPickup->GetCode()) + " ";
						mAIManager->PrintLogInformation(info);
						mAIManager->PrintLogInformationDetails(info);
					}
				}

				info = "\n red player heuristic " + eastl::to_string(mOtherPlayerState.heuristic) + " ";
				mAIManager->PrintLogInformation(info);
				mAIManager->PrintLogInformationDetails(info);
				if (!mOtherPlayerState.items.empty())
				{
					info = " actors : ";
					mAIManager->PrintLogInformation(info);
					mAIManager->PrintLogInformationDetails(info);
				}
				for (eastl::shared_ptr<Actor> pItemActor : mOtherPlayerState.items)
				{
					if (pItemActor->GetType() == "Weapon")
					{
						eastl::shared_ptr<WeaponPickup> pWeaponPickup =
							pItemActor->GetComponent<WeaponPickup>(WeaponPickup::Name).lock();

						info = "weapon " + eastl::to_string(pWeaponPickup->GetCode()) + " ";
						mAIManager->PrintLogInformation(info);
						mAIManager->PrintLogInformationDetails(info);
					}
					else if (pItemActor->GetType() == "Ammo")
					{
						eastl::shared_ptr<AmmoPickup> pAmmoPickup =
							pItemActor->GetComponent<AmmoPickup>(AmmoPickup::Name).lock();

						info = "ammo " + eastl::to_string(pAmmoPickup->GetCode()) + " ";
						mAIManager->PrintLogInformation(info);
						mAIManager->PrintLogInformationDetails(info);
					}
					else if (pItemActor->GetType() == "Armor")
					{
						eastl::shared_ptr<ArmorPickup> pArmorPickup =
							pItemActor->GetComponent<ArmorPickup>(ArmorPickup::Name).lock();

						info = "armor " + eastl::to_string(pArmorPickup->GetCode()) + " ";
						mAIManager->PrintLogInformation(info);
						mAIManager->PrintLogInformationDetails(info);
					}
					else if (pItemActor->GetType() == "Health")
					{
						eastl::shared_ptr<HealthPickup> pHealthPickup =
							pItemActor->GetComponent<HealthPickup>(HealthPickup::Name).lock();

						info = "health " + eastl::to_string(pHealthPickup->GetCode()) + " ";
						mAIManager->PrintLogInformation(info);
						mAIManager->PrintLogInformationDetails(info);
					}
				}
				if (mOtherPlayerState.weapon != WP_NONE)
				{
					info = "weapon : " + eastl::to_string(mOtherPlayerState.weapon) + " ";
					mAIManager->PrintLogInformation(info);
					mAIManager->PrintLogInformationDetails(info);

					info = "damage : " + eastl::to_string(mOtherPlayerState.damage[mOtherPlayerState.weapon - 1]) + " ";
					mAIManager->PrintLogInformation(info);
					mAIManager->PrintLogInformationDetails(info);
				}
				else
				{
					info = "weapon : 0 ";
					mAIManager->PrintLogInformation(info);
					mAIManager->PrintLogInformationDetails(info);

					info = "damage : 0 ";
					mAIManager->PrintLogInformation(info);
					mAIManager->PrintLogInformationDetails(info);
				}

				mAIManager->SetPlayerGuessState(mOtherPlayerState.player, mOtherPlayerState);
				mAIManager->SetPlayerGuessUpdated(mOtherPlayerState.player, true);

				mAIManager->SetPlayerState(mPlayerState.player, mPlayerState);
				mAIManager->SetPlayerUpdated(mPlayerState.player, true);
			}

			mAIManager->SetEnable(true);
		}
	}

	Succeed();
}
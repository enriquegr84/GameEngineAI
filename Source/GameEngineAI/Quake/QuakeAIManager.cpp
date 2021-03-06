/*******************************************************
 * Copyright (C) GameEngineAI - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Enrique Gonz�lez Rodr�guez <enriquegr84@hotmail.es>, 2019-2020
 *******************************************************/

#include "QuakeAIManager.h"

#include "Core/OS/OS.h"
#include "Core/Logger/Logger.h"
#include "Core/IO/XmlResource.h"
#include "Core/Event/EventManager.h"
#include "Core/Event/Event.h"

#include "Physic/PhysicEventListener.h"

#include "QuakeEvents.h"
#include "QuakeView.h"
#include "QuakeApp.h"
#include "Quake.h"

#include <cereal/types/vector.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/archives/binary.hpp>
#include <fstream>

namespace CerealTypes
{
	struct Vec3
	{
		short x, y, z;

		template <class Archive>
		void serialize(Archive & ar)
		{
			ar(x, y, z);
		}
	};

	struct VisibleNode
	{
		unsigned short id;

		template <class Archive>
		void serialize(Archive & ar)
		{
			ar(id);
		}
	};

	struct ClusterNode
	{
		unsigned short type;
		unsigned short actor;
		unsigned short nodeid;
		unsigned short targetid;

		template <class Archive>
		void serialize(Archive & ar)
		{
			ar(type, actor, nodeid, targetid);
		}
	};

	struct ArcNode
	{
		int id;
		unsigned short type;
		unsigned short nodeid;
		float weight;

		template <class Archive>
		void serialize(Archive & ar)
		{
			ar(id, type, nodeid, weight);
		}
	};

	struct TransitionNode
	{
		int id;
		unsigned short type;
		std::vector<float> weights;
		std::vector<unsigned short> nodes;
		std::vector<Vec3> connections;

		template <class Archive>
		void save(Archive & ar) const
		{
			ar(id, type, weights, nodes, connections);
		}

		template <class Archive>
		void load(Archive & ar)
		{
			ar(id, type, weights, nodes, connections);
		}
	};

	struct GraphNode
	{
		unsigned short id;
		unsigned short actorid;
		unsigned short clusterid;
		float tolerance;
		Vec3 position;
		std::vector<ArcNode> arcs;
		std::vector<ClusterNode> clusters;
		std::vector<TransitionNode> transitions;
		std::vector<VisibleNode> visibles;

		template <class Archive>
		void save(Archive & ar) const
		{
			ar(id, actorid, clusterid, tolerance, position, arcs, clusters, transitions, visibles);
		}

		template <class Archive>
		void load(Archive & ar)
		{
			ar(id, actorid, clusterid, tolerance, position, arcs, clusters, transitions, visibles);
		}
	};

	struct Graph
	{
		std::vector<GraphNode> nodes;

		template <class Archive>
		void save(Archive & ar) const
		{
			ar(nodes);
		}

		template <class Archive>
		void load(Archive & ar)
		{
			ar(nodes);
		}
	};
}

QuakeAIManager::QuakeAIManager() : AIManager()
{
	mEnable = false;

	mLastArcId = 0;
	mLastNodeId = 0;
	mLastPlanId = 0;

	mMaxJumpSpeed = 3.4f;
	mMaxFallSpeed = 240.0f;
	mMaxRotateSpeed = 180.0f;
	mMoveSpeed = 6.0f;
	mJumpSpeed = 2.6f;
	mJumpMoveSpeed = 7.2f;
	mFallSpeed = 0.0f;
	mRotateSpeed = 0.0f;

	mLogError = std::ofstream("error.txt", std::ios::out);
	mLogInformation = std::ofstream("info.txt", std::ios::out);
	mLogInformationDetails = std::ofstream("infodetails.txt", std::ios::out);
	mLogPathingInformation = std::ofstream("pathinginfo.txt", std::ios::out);
	mLogGuessInformation = std::ofstream("guessinfo.txt", std::ios::out);
}   // QuakeAIManager

//-----------------------------------------------------------------------------

QuakeAIManager::~QuakeAIManager()
{
	mLogError.close();
	mLogInformation.close();
	mLogInformationDetails.close();
	mLogPathingInformation.close();
	mLogGuessInformation.close();
}   // ~QuakeAIManager

/////////////////////////////////////////////////////////////////////////////
// QuakeAIManager::SavePathingGraph
//
//    Saves the AI pathing graph information to an XML file
//
void QuakeAIManager::SavePathingGraph(const eastl::string& path)
{
	//set data
	CerealTypes::Graph data;

	for (PathingNode* pathNode : mPathingGraph->GetNodes())
	{
		CerealTypes::GraphNode node;

		node.id = pathNode->GetId();
		node.actorid = pathNode->GetActorId();
		node.clusterid = pathNode->GetCluster();
		node.tolerance = pathNode->GetTolerance();
		node.position.x = (short)round(pathNode->GetPos()[0]);
		node.position.y = (short)round(pathNode->GetPos()[1]);
		node.position.z = (short)round(pathNode->GetPos()[2]);

		for (auto visibilityNode : pathNode->GetVisibileNodes())
		{
			CerealTypes::VisibleNode visibleNode;
			visibleNode.id = visibilityNode.first->GetId();

			node.visibles.push_back(visibleNode);
		}

		for (PathingArc* pathArc : pathNode->GetArcs())
		{
			CerealTypes::ArcNode arcNode;
			arcNode.id = pathArc->GetId();
			arcNode.type = pathArc->GetType();
			arcNode.nodeid = pathArc->GetNode()->GetId();
			arcNode.weight = pathArc->GetWeight();

			node.arcs.push_back(arcNode);
		}

		for (PathingCluster* pathCluster : pathNode->GetClusters())
		{
			CerealTypes::ClusterNode clusterNode;
			clusterNode.type = pathCluster->GetType();
			clusterNode.actor = pathCluster->GetActor();
			clusterNode.nodeid = pathCluster->GetNode()->GetId();
			clusterNode.targetid = pathCluster->GetTarget()->GetId();

			node.clusters.push_back(clusterNode);
		}

		for (PathingTransition* pathTransition : pathNode->GetTransitions())
		{
			CerealTypes::TransitionNode transitionNode;
			transitionNode.id = pathTransition->GetId();
			transitionNode.type = pathTransition->GetType();

			for (PathingNode* pNode : pathTransition->GetNodes())
			{
				transitionNode.nodes.push_back(pNode->GetId());
			}
			for (float weight : pathTransition->GetWeights())
			{
				transitionNode.weights.push_back(weight);
			}
			for (Vector3<float> connection : pathTransition->GetConnections())
			{
				transitionNode.connections.push_back(CerealTypes::Vec3{
						(short)round(connection[0]), 
						(short)round(connection[1]), 
						(short)round(connection[2]) });
			}

			node.transitions.push_back(transitionNode);
		}

		data.nodes.push_back(node);
	}

	std::ofstream os(path.c_str(), std::ios::binary);
	cereal::BinaryOutputArchive archive(os);
	archive(data);
}


/////////////////////////////////////////////////////////////////////////////
// QuakeAIManager::LoadPathingGraph
//
//    Loads the AI pathing graph information from an XML file
//
void QuakeAIManager::LoadPathingGraph(const eastl::wstring& path)
{
	//set data
	CerealTypes::Graph data;

	std::ifstream is(path.c_str(), std::ios::binary);
	if (is.fail())
	{
		LogError(strerror(errno));
	}
	cereal::BinaryInputArchive archive(is);
	archive(data);

	mLastArcId = 0;
	mLastNodeId = 0;

	mPathingGraph = eastl::make_shared<PathingGraph>();

	eastl::map<unsigned int, PathingNode*> pathingNodeGraph;
	for (CerealTypes::GraphNode node : data.nodes)
	{
		unsigned short clusterId = node.clusterid;
		unsigned short pathNodeId = node.id;
		ActorId actorId = node.actorid;
		float tolerance = node.tolerance;
		Vector3<float> position{ 
			(float)node.position.x, (float)node.position.y, (float)node.position.z };
		if (mLastNodeId < pathNodeId) mLastNodeId = pathNodeId;

		PathingNode* pathNode = new PathingNode(pathNodeId, actorId, position, tolerance);
		pathNode->SetCluster(clusterId);
		mPathingGraph->InsertNode(pathNode);

		pathingNodeGraph[pathNodeId] = pathNode;
	}

	for (CerealTypes::GraphNode node : data.nodes)
	{
		unsigned short pathNodeId = node.id;
		PathingNode* pathNode = pathingNodeGraph[pathNodeId];

		for (CerealTypes::VisibleNode visibleNode : node.visibles)
		{
			PathingNode* visibilityNode = pathingNodeGraph[visibleNode.id];
			pathNode->AddVisibleNode(
				visibilityNode, Length(visibilityNode->GetPos() - pathNode->GetPos()));

			mPathingGraph->InsertVisibleCluster(pathNode->GetCluster(), visibilityNode->GetCluster());
		}

		for (CerealTypes::ArcNode arc : node.arcs)
		{
			unsigned int arcId = arc.id;
			unsigned short arcType = arc.type;
			int arcNode = arc.nodeid;
			float weight = arc.weight;
			if (mLastArcId < arcId) mLastArcId = arcId;

			PathingArc* pathArc = new PathingArc(arcId, arcType, pathingNodeGraph[arcNode], weight);
			mPathingGraph->InsertArc(pathArc);

			pathNode->AddArc(pathArc);
		}

		for (CerealTypes::ClusterNode cluster : node.clusters)
		{
			int clusterType = cluster.type;
			int clusterActor = cluster.actor;
			int clusterNode = cluster.nodeid;
			int clusterTarget = cluster.targetid;

			PathingCluster* pathCluster = new PathingCluster(clusterType, clusterActor);
			pathCluster->LinkClusters(pathingNodeGraph[clusterNode], pathingNodeGraph[clusterTarget]);
			mPathingGraph->InsertCluster(pathCluster);

			pathNode->AddCluster(pathCluster);
			if (clusterActor != INVALID_ACTOR_ID)
				pathNode->AddClusterActor(pathCluster);
		}

		for (CerealTypes::TransitionNode transition : node.transitions)
		{
			unsigned int transitionId = transition.id;
			unsigned short transitionType = transition.type;

			eastl::vector<float> weights;
			eastl::vector<PathingNode*> nodes;
			eastl::vector<Vector3<float>> connections;
			for (int nodeid : transition.nodes)
			{
				nodes.push_back(pathingNodeGraph[nodeid]);
			}
			for (float weight : transition.weights)
			{
				weights.push_back(weight);
			}
			for (CerealTypes::Vec3 connection : transition.connections)
			{
				connections.push_back(Vector3<float>{
					(float)connection.x, (float)connection.y, (float)connection.z});
			}

			PathingTransition* pathTransition = new PathingTransition(
				transitionId, transitionType, nodes, weights, connections);
			pathNode->AddTransition(pathTransition);
		}
	}
}


//--------------------------------------------------------------------------------------------------------
// AIPlanNode
//--------------------------------------------------------------------------------------------------------
AIPlanNode::AIPlanNode(PathingNode* pNode, AIPlanNode* pPrevNode, 
	PathingCluster* pGoalCluster, float weight, float heuristic)
{
	LogAssert(pNode, "Invalid node");

	mPathingNode = pNode;
	mPrevNode = pPrevNode;  // NULL is a valid value, though it should only be NULL for the start node
	mGoalCluster = pGoalCluster;
	mWeight = weight;
	mHeuristic = heuristic;
	mClosed = false;
}

void AIPlanNode::GetPlanActors(eastl::map<ActorId, float>& planActors)
{
	AIPlanNode* pNode = this;
	while (pNode)
	{
		if (pNode->GetGoalCluster())
		{
			if (pNode->GetGoalCluster()->GetActor() != INVALID_ACTOR_ID)
				planActors[pNode->GetGoalCluster()->GetActor()] = pNode->GetWeight();
		}
		pNode = pNode->GetPrev();
	}
}

void AIPlanNode::UpdateNode(PathingNode* pNode, AIPlanNode* pPrev,
	PathingCluster* pGoalCluster, float weight, float heuristic)
{
	LogAssert(pPrev, "Invalid node");
	mPathingNode = pNode;
	mPrevNode = pPrev;
	mGoalCluster = pGoalCluster;
	mWeight = weight;
	mHeuristic = heuristic;
}


//--------------------------------------------------------------------------------------------------------
// AIFinder
//--------------------------------------------------------------------------------------------------------
AIFinder::AIFinder(void)
{
	mNodeState = NodeState();
	mGoalCluster = NULL;
}

AIFinder::~AIFinder(void)
{
	Destroy();
}

void AIFinder::Destroy(void)
{
	// destroy all the AIPlanNode objects and clear the map
	for (AIPlanNodeVector::iterator it = mNodes.begin(); it != mNodes.end(); ++it)
		delete (*it);
	mNodes.clear();

	// clear the open set
	mOpenSet.clear();

	// clear the node state & goal cluster
	mNodeState = NodeState();
	mGoalCluster = NULL;
}

//
// AIFinder::operator()
//
void AIFinder::operator()(NodeState& pNodeState, 
	PathingCluster* pGoalCluster, PathingArcVec& planPath, 
	eastl::map<ActorId, float>& excludeActors, float threshold)
{
	LogAssert(pNodeState.valid, "Invalid node");
	LogAssert(pGoalCluster, "Invalid cluster");

	QuakeAIManager* aiManager = dynamic_cast<QuakeAIManager*>(GameLogic::Get()->GetAIManager());
	LogAssert(aiManager, "Invalid ai manager");

	// if the start and end nodes are the same, we're close enough to b-line to the goal
	if (pNodeState.plan.node == pGoalCluster->GetTarget())
		return;

	// set our members
	mNodeState = pNodeState;
	mGoalCluster = pGoalCluster;

	// The open set is a priority queue of the nodes to be evaluated.  If it's ever empty, it means 
	// we couldn't find a path to the goal. The start node is the only node that is initially in the open set.
	AddToOpenSet(mNodeState.plan.node, NULL, NULL, 0.f, 0.f);

	AIPlanNode* bestPlanNode = NULL;
	while (!mOpenSet.empty())
	{
		// grab the most likely candidate
		AIPlanNode* planNode = mOpenSet.front();
		PathingNode* pathingNode = planNode->GetGoalCluster() ?
			planNode->GetGoalCluster()->GetTarget() : planNode->GetPathingNode();

		// we're processing this node so remove it from the open set and add it to the closed set
		mOpenSet.pop_front();
		AddToClosedSet(planNode);

		eastl::map<unsigned int, PathingCluster*> targetClusters;
		pathingNode->GetClusters(mGoalCluster->GetTarget()->GetCluster(), targetClusters);
		if (!targetClusters.size())
			continue;

		PathingCluster* targetCluster = (*targetClusters.begin()).second;
		if (targetClusters.find(mGoalCluster->GetType()) != targetClusters.end())
			targetCluster = targetClusters[mGoalCluster->GetType()];

		PathingNode* pNode = pathingNode;
		float targetWeight = planNode->GetWeight();
		while (pNode != targetCluster->GetTarget())
		{
			PathingCluster* pCluster = pNode->FindCluster(
				targetCluster->GetType(), targetCluster->GetTarget());
			PathingArc* pArc = pNode->FindArc(pCluster->GetNode());
			targetWeight += pArc->GetWeight();

			pNode = pArc->GetNode();
		}
		if (targetWeight >= threshold)
			continue;

		// lets find out if we successfully found a plan.
		if (bestPlanNode)
		{
			if (planNode->GetHeuristic() > bestPlanNode->GetHeuristic())
				bestPlanNode = planNode;
		}
		else if (planNode->GetHeuristic() > 0.f)
		{
			bestPlanNode = planNode;
		}

		// get traversed actors
		eastl::map<ActorId, float> planActors;
		planNode->GetPlanActors(planActors);

		float pathingWeight = planNode->GetWeight();
		while (pathingNode != targetCluster->GetTarget())
		{
			// get the neighboring actor clusters
			PathingClusterVec neighbors;
			pathingNode->GetClusterActors(GAT_JUMP, neighbors);

			// loop though all the neighboring nodes and evaluate each one
			for (PathingClusterVec::iterator it = neighbors.begin(); it != neighbors.end(); ++it)
			{
				PathingCluster* pClusterToEvaluate = (*it);
				if (planActors.find(pClusterToEvaluate->GetActor()) != planActors.end())
					continue;

				// figure out the cost for this route through the node
				float weight = pathingWeight;
				PathingNode* pNeighborNode = pathingNode;
				while (pNeighborNode != pClusterToEvaluate->GetTarget())
				{
					PathingCluster* pNeighborCluster = pNeighborNode->FindClusterActor(
						pClusterToEvaluate->GetType(), pClusterToEvaluate->GetTarget());
					PathingArc* pNeighborArc = pNeighborNode->FindArc(pNeighborCluster->GetNode());
					weight += pNeighborArc->GetWeight();

					pNeighborNode = pNeighborArc->GetNode();
				}
				if (weight >= threshold)
					continue;

				if (!aiManager->CanItemBeGrabbed(pClusterToEvaluate->GetActor(), weight, mNodeState, excludeActors))
					continue;

				eastl::map<ActorId, float> planClusterActors;
				planNode->GetPlanActors(planClusterActors);
				planClusterActors[pNeighborNode->GetActorId()] = weight;
				planActors[pNeighborNode->GetActorId()] = weight;

				NodeState nodeState(mNodeState);
				aiManager->PickupItems(nodeState, planClusterActors, excludeActors);
				float nodeHeuristic = aiManager->CalculateHeuristicItems(nodeState);
				AddToOpenSet(pathingNode, planNode, pClusterToEvaluate, weight, nodeHeuristic);
			}

			PathingCluster* pathingCluster = 
				pathingNode->FindCluster(targetCluster->GetType(), targetCluster->GetTarget());
			PathingArc* pathingArc = pathingNode->FindArc(pathingCluster->GetNode());

			pathingWeight += pathingArc->GetWeight();
			pathingNode = pathingArc->GetNode();
		}
	}
	RebuildPath(bestPlanNode, planPath);
}

AIPlanNode* AIFinder::AddToOpenSet(PathingNode* pNode, AIPlanNode* pPrevNode,
	PathingCluster* pGoalCluster, float weight, float heuristic)
{
	LogAssert(pNode, "Invalid node");

	AIPlanNode* pThisNode = new AIPlanNode(pNode, pPrevNode, pGoalCluster, weight, heuristic);
	mNodes.push_back(pThisNode);

	// now insert it into the priority queue
	InsertNode(pThisNode);

	return pThisNode;
}

void AIFinder::AddToClosedSet(AIPlanNode* pNode)
{
	LogAssert(pNode, "Invalid node");
	pNode->SetClosed();
}

//
// AIFinder::InsertNode					- Chapter 17, page 636
//
void AIFinder::InsertNode(AIPlanNode* pNode)
{
	LogAssert(pNode, "Invalid node");

	// just add the node if the open set is empty
	if (mOpenSet.empty())
	{
		mOpenSet.push_back(pNode);
		return;
	}

	// otherwise, perform an insertion sort	
	AIPlanNodeList::iterator it = mOpenSet.begin();
	AIPlanNode* pCompare = *it;
	while (pCompare->IsBetterChoiceThan(pNode))
	{
		++it;

		if (it != mOpenSet.end())
			pCompare = *it;
		else
			break;
	}
	mOpenSet.insert(it, pNode);
}

void AIFinder::RebuildPath(AIPlanNode* pGoalNode, PathingArcVec& planPath)
{
	AIPlanNodeVector planNodes;
	AIPlanNode* pPlanNode = pGoalNode;
	PathingNode* pPathingNode = NULL;
	while (pPlanNode)
	{
		if (pPlanNode->GetGoalCluster())
			planNodes.insert(planNodes.begin(), pPlanNode);
		else 
			pPathingNode = pPlanNode->GetPathingNode();

		pPlanNode = pPlanNode->GetPrev();
	}
	if (pPathingNode)
	{
		for (AIPlanNode* pNode : planNodes)
		{
			if (pPathingNode != pNode->GetPathingNode())
			{
				eastl::map<unsigned int, PathingCluster*> targetClusters;
				pPathingNode->GetClusters(mGoalCluster->GetTarget()->GetCluster(), targetClusters);

				PathingCluster* pTargetCluster = (*targetClusters.begin()).second;
				if (targetClusters.find(mGoalCluster->GetType()) != targetClusters.end())
					pTargetCluster = targetClusters[mGoalCluster->GetType()];

				while (pPathingNode != pNode->GetPathingNode())
				{
					PathingCluster* pPathingCluster = pPathingNode->FindCluster(
						pTargetCluster->GetType(), pTargetCluster->GetTarget());
					PathingArc* pPathingArc = pPathingNode->FindArc(pPathingCluster->GetNode());
					planPath.push_back(pPathingArc);

					pPathingNode = pPathingArc->GetNode();
				}
			}

			pPathingNode = pNode->GetPathingNode();
			if (pNode->GetGoalCluster())
			{
				while (pPathingNode != pNode->GetGoalCluster()->GetTarget())
				{
					PathingCluster* pPathingCluster = pPathingNode->FindClusterActor(
						pNode->GetGoalCluster()->GetType(), pNode->GetGoalCluster()->GetTarget());
					PathingArc* pPathingArc = pPathingNode->FindArc(pPathingCluster->GetNode());
					planPath.push_back(pPathingArc);

					pPathingNode = pPathingArc->GetNode();
				}
			}
		}

		eastl::map<unsigned int, PathingCluster*> targetClusters;
		pPathingNode->GetClusters(mGoalCluster->GetTarget()->GetCluster(), targetClusters);

		PathingCluster* pTargetCluster = (*targetClusters.begin()).second;
		if (targetClusters.find(mGoalCluster->GetType()) != targetClusters.end())
			pTargetCluster = targetClusters[mGoalCluster->GetType()];

		while (pPathingNode != pTargetCluster->GetTarget())
		{
			PathingCluster* pPathingCluster = pPathingNode->FindCluster(
				pTargetCluster->GetType(), pTargetCluster->GetTarget());
			PathingArc* pPathingArc = pPathingNode->FindArc(pPathingCluster->GetNode());
			planPath.push_back(pPathingArc);

			pPathingNode = pPathingArc->GetNode();
		}

		QuakeAIManager* aiManager = dynamic_cast<QuakeAIManager*>(GameLogic::Get()->GetAIManager());

		eastl::map<ActorId, float> planActors;
		pGoalNode->GetPlanActors(planActors);

		aiManager->PrintLogInformationDetails("\n actor heuristic " +
			eastl::to_string(pGoalNode->GetHeuristic()) +  " cluster " +
			eastl::to_string(pTargetCluster->GetTarget()->GetCluster()) + " : ");
		for (auto planActor : planActors)
		{
			eastl::shared_ptr<Actor> pItemActor(
				GameLogic::Get()->GetActor(planActor.first).lock());
			if (pItemActor->GetType() == "Weapon")
			{
				eastl::shared_ptr<WeaponPickup> pWeaponPickup =
					pItemActor->GetComponent<WeaponPickup>(WeaponPickup::Name).lock();
				aiManager->PrintLogInformationDetails(
					"weapon " + eastl::to_string(pWeaponPickup->GetCode()) + " ");
			}
			else if (pItemActor->GetType() == "Ammo")
			{
				eastl::shared_ptr<AmmoPickup> pAmmoPickup =
					pItemActor->GetComponent<AmmoPickup>(AmmoPickup::Name).lock();
				aiManager->PrintLogInformationDetails(
					"ammo " + eastl::to_string(pAmmoPickup->GetCode()) + " ");
			}
			else if (pItemActor->GetType() == "Armor")
			{
				eastl::shared_ptr<ArmorPickup> pArmorPickup =
					pItemActor->GetComponent<ArmorPickup>(ArmorPickup::Name).lock();
				aiManager->PrintLogInformationDetails(
					"armor " + eastl::to_string(pArmorPickup->GetCode()) + " ");
			}
			else if (pItemActor->GetType() == "Health")
			{
				eastl::shared_ptr<HealthPickup> pHealthPickup =
					pItemActor->GetComponent<HealthPickup>(HealthPickup::Name).lock();
				aiManager->PrintLogInformationDetails(
					"health " + eastl::to_string(pHealthPickup->GetCode()) + " ");
			}
		}
	}
}

ActorId QuakeAIManager::GetPlayerWeaponTarget(ActorId player)
{
	ActorId playerWeaponTarget = INVALID_ACTOR_ID;

	mMutex.lock();
	if (mPlayerStates.find(player) != mPlayerStates.end())
		playerWeaponTarget = mPlayerStates[player].weaponTarget;
	mMutex.unlock();

	return playerWeaponTarget;
}

WeaponType QuakeAIManager::GetPlayerWeapon(ActorId player)
{
	WeaponType weaponType = WP_NONE;

	mMutex.lock();
	if (mPlayerStates.find(player) != mPlayerStates.end())
		weaponType = mPlayerStates[player].weapon;
	mMutex.unlock();

	return weaponType;
}

void QuakeAIManager::GetPlayerState(ActorId player, NodeState& playerState)
{
	mMutex.lock();
	if (mPlayerStates.find(player) != mPlayerStates.end())
		playerState = NodeState(mPlayerStates[player]);
	mMutex.unlock();
}

void QuakeAIManager::GetPlayerPlan(ActorId player, NodePlan& playerPlan)
{
	mMutex.lock();
	if (mPlayerPlans.find(player) != mPlayerPlans.end())
		playerPlan = mPlayerPlans[player];
	mMutex.unlock();
}

bool QuakeAIManager::IsPlayerUpdated(ActorId player)
{
	bool playerUpdated = false;

	mMutex.lock();
	if (mPlayers.find(player) != mPlayers.end())
		playerUpdated = mPlayers[player];
	mMutex.unlock();

	return playerUpdated;
}

void QuakeAIManager::SetPlayerState(ActorId player, eastl::shared_ptr<PlayerActor> playerActor)
{
	mMutex.lock();
	mPlayerStates[player] = NodeState(playerActor);
	mMutex.unlock();
}

void QuakeAIManager::SetPlayerState(ActorId player, NodeState& playerState)
{
	mMutex.lock();
	mPlayerStates[player] = NodeState(playerState);
	mMutex.unlock();
}

void QuakeAIManager::SetPlayerPlan(ActorId player, NodePlan& playerPlan)
{
	mMutex.lock();
	mPlayerPlans[player] = NodePlan(playerPlan);
	mMutex.unlock();
}

void QuakeAIManager::SetPlayerUpdated(ActorId player, bool update)
{
	mMutex.lock();
	mPlayers[player] = update;
	mMutex.unlock();
}

void QuakeAIManager::RemovePlayerGuessItems(ActorId player)
{
	mMutex.lock();
	mPlayerGuessItems[player].clear();
	mMutex.unlock();
}

void QuakeAIManager::SetPlayerGuessItems(ActorId player, eastl::map<ActorId, float>& guessItems)
{
	mMutex.lock();
	mPlayerGuessItems[player].clear();
	for (auto guessItem : guessItems)
		mPlayerGuessItems[player][guessItem.first] = guessItem.second;
	mMutex.unlock();
}

void QuakeAIManager::GetPlayerGuessItems(ActorId player, eastl::map<ActorId, float>& guessItems)
{
	mMutex.lock();
	eastl::map<ActorId, float> playerGuessItems = mPlayerGuessItems[player];
	for (auto guessItem : playerGuessItems)
		guessItems[guessItem.first] = guessItem.second;
	mMutex.unlock();
}

void QuakeAIManager::GetPlayerGuessState(ActorId player, NodeState& state)
{
	mMutex.lock();
	if (mPlayerGuessStates.find(player) != mPlayerGuessStates.end())
		state = NodeState(mPlayerGuessStates[player]);
	mMutex.unlock();
}

void QuakeAIManager::GetPlayerGuessPlan(ActorId player, NodePlan& playerPlan)
{
	mMutex.lock();
	if (mPlayerGuessPlans.find(player) != mPlayerGuessPlans.end())
		playerPlan = mPlayerGuessPlans[player];
	mMutex.unlock();
}

bool QuakeAIManager::IsPlayerGuessUpdated(ActorId player)
{
	bool guessUpdated = false;

	mMutex.lock();
	if (mPlayerGuess.find(player) != mPlayerGuess.end())
		guessUpdated = mPlayerGuess[player];
	mMutex.unlock();

	return guessUpdated;
}

void QuakeAIManager::SetPlayerGuessState(ActorId player, eastl::shared_ptr<PlayerActor> playerActor)
{
	mMutex.lock();
	mPlayerGuessStates[player] = NodeState(playerActor);
	mMutex.unlock();
}

void QuakeAIManager::SetPlayerGuessState(ActorId player, NodeState& playerState)
{
	mMutex.lock();
	mPlayerGuessStates[player] = NodeState(playerState);
	mMutex.unlock();
}

void QuakeAIManager::SetPlayerGuessPlan(ActorId player, NodePlan& playerPlan)
{
	mMutex.lock();
	mPlayerGuessPlans[player] = NodePlan(playerPlan);
	mMutex.unlock();
}

void QuakeAIManager::SetPlayerGuessUpdated(ActorId player, bool update)
{
	mMutex.lock();
	mPlayerGuess[player] = update;
	mMutex.unlock();
}

void QuakeAIManager::SpawnActor(ActorId playerId)
{
	GameApplication* gameApp = (GameApplication*)Application::App;
	QuakeLogic* game = static_cast<QuakeLogic *>(GameLogic::Get());

	eastl::shared_ptr<PlayerActor> pPlayerActor(
		eastl::dynamic_shared_pointer_cast<PlayerActor>(
		GameLogic::Get()->GetActor(playerId).lock()));
	if (pPlayerActor)
	{
		eastl::shared_ptr<TransformComponent> pPlayerTransform(
			pPlayerActor->GetComponent<TransformComponent>(TransformComponent::Name).lock());
		if (pPlayerTransform)
		{
			SetPlayerState(pPlayerActor->GetId(), pPlayerActor);

			PathingNode* spawnNode = mPathingGraph->FindClosestNode(pPlayerTransform->GetPosition());
			NodePlan playerPlan(spawnNode, PathingArcVec());
			SetPlayerPlan(pPlayerActor->GetId(), playerPlan);
		}

		if (mPlayerPlanTime.find(pPlayerActor->GetId()) == mPlayerPlanTime.end())
			mPlayerPlanTime[pPlayerActor->GetId()] = 0.f;

		//no idea where the player is located take any random spawn position
		eastl::shared_ptr<Actor> spawnSpot;
		game->SelectRandomSpawnPoint(spawnSpot);

		eastl::shared_ptr<TransformComponent> pSpawnTransform(
			spawnSpot->GetComponent<TransformComponent>(TransformComponent::Name).lock());
		if (pSpawnTransform)
		{
			SetPlayerGuessItems(pPlayerActor->GetId(), eastl::map<ActorId, float>());
			SetPlayerGuessState(pPlayerActor->GetId(), pPlayerActor);

			PathingNode* spawnNode = mPathingGraph->FindClosestNode(pSpawnTransform->GetPosition());
			NodePlan playerGuessPlan(spawnNode, PathingArcVec());
			SetPlayerGuessPlan(pPlayerActor->GetId(), playerGuessPlan);
			SetPlayerGuessUpdated(pPlayerActor->GetId(), false);
		}

		eastl::vector<eastl::shared_ptr<PlayerActor>> playerActors;
		game->GetPlayerActors(playerActors);
		for (eastl::shared_ptr<PlayerActor> pOtherPlayerActor : playerActors)
		{
			if (pPlayerActor->GetId() == pOtherPlayerActor->GetId())
				continue;

			mPlayerPlanTime[pOtherPlayerActor->GetId()] = 0.f;
		}
	}
}

void QuakeAIManager::DetectActor(eastl::shared_ptr<PlayerActor> pPlayerActor, eastl::shared_ptr<Actor> pItemActor)
{
	GameApplication* gameApp = (GameApplication*)Application::App;
	QuakeLogic* game = static_cast<QuakeLogic *>(GameLogic::Get());

	eastl::vector<eastl::shared_ptr<PlayerActor>> playerActors;
	game->GetPlayerActors(playerActors);
	for (eastl::shared_ptr<PlayerActor> pOtherPlayerActor : playerActors)
	{
		if (pOtherPlayerActor->GetId() != pPlayerActor->GetId())
		{
			// take into consideration within a certain radius
			eastl::shared_ptr<TransformComponent> pItemTransform(
				pItemActor->GetComponent<TransformComponent>(TransformComponent::Name).lock());
			eastl::shared_ptr<TransformComponent> pOtherPlayerTransform(
				pOtherPlayerActor->GetComponent<TransformComponent>(TransformComponent::Name).lock());
			if (Length(pItemTransform->GetPosition() - pOtherPlayerTransform->GetPosition()) > 700.f)
				continue;

			mPlayerPlanTime[pOtherPlayerActor->GetId()] = 0.f;

			RemovePlayerGuessItems(pPlayerActor->GetId());

			NodePlan playerGuessPlan;
			GetPlayerPlan(pPlayerActor->GetId(), playerGuessPlan);
			playerGuessPlan = NodePlan(playerGuessPlan.node, PathingArcVec());

			SetPlayerGuessPlan(pPlayerActor->GetId(), playerGuessPlan);
			SetPlayerGuessState(pPlayerActor->GetId(), pPlayerActor);
			SetPlayerGuessUpdated(pPlayerActor->GetId(), false);
		}
	}
}

void QuakeAIManager::PrintLogError(eastl::string error)
{
	mLogError << error.c_str();
	mLogError.flush();
}

void QuakeAIManager::PrintLogInformation(eastl::string info)
{
	mLogInformation << info.c_str();
	mLogInformation.flush();
}

void QuakeAIManager::PrintLogInformationDetails(eastl::string info)
{
	//mLogInformationDetails << info.c_str();
	//mLogInformationDetails.flush();
}

void QuakeAIManager::PrintLogPathingInformation(eastl::string info)
{
	//mLogPathingInformation << info.c_str();
	//mLogPathingInformation.flush();
}

void QuakeAIManager::PrintLogGuessInformation(eastl::string info)
{
	//mLogGuessInformation << info.c_str();
	//mLogGuessInformation.flush();
}

float QuakeAIManager::CalculateHeuristicItems(NodeState& playerState)
{
	float heuristicFactor = 0.f;
	float heuristic = 0.f;
	float maxWeight = 6.0f;
	float weight = 0.f;
	int maxAmmo = 0;
	int ammo = 0;

	//heuristic from picked up items
	for (eastl::shared_ptr<Actor> item : playerState.items)
	{
		if (item->GetType() == "Weapon")
		{
			eastl::shared_ptr<WeaponPickup> pWeaponPickup =
				item->GetComponent<WeaponPickup>(WeaponPickup::Name).lock();

			switch (pWeaponPickup->GetCode())
			{
				case WP_LIGHTNING:
					maxAmmo = 60;

					//lets calculate the item heuristicFactor based on the player status
					if (playerState.stats[STAT_WEAPONS] & (1 << pWeaponPickup->GetCode()))
					{
						heuristicFactor = 0.1f;
						if (playerState.ammo[pWeaponPickup->GetCode()] > 80)
							heuristicFactor *= 0.5f;
					}
					else heuristicFactor = 0.2f;

					weight = (playerState.itemWeight[item] < maxWeight) ?
						playerState.itemWeight[item] : maxWeight;
					ammo = (playerState.itemAmount[item] < maxAmmo) ?
						playerState.itemAmount[item] : maxAmmo;

					//relation based on amount gained and heuristicFactor travelled
					heuristic += (ammo / (float)maxAmmo) * (1.0f - (weight / (float)maxWeight)) * heuristicFactor;
					break;
				case WP_SHOTGUN:
					maxAmmo = 10;

					//lets calculate the item heuristicFactor based on the player status
					if (playerState.stats[STAT_WEAPONS] & (1 << pWeaponPickup->GetCode()))
					{
						heuristicFactor = 0.1f;
						if (playerState.ammo[pWeaponPickup->GetCode()] > 10)
							heuristicFactor *= 0.5f;
					}
					else heuristicFactor = 0.2f;

					weight = (playerState.itemWeight[item] < maxWeight) ?
						playerState.itemWeight[item] : maxWeight;
					ammo = (playerState.itemAmount[item] < maxAmmo) ?
						playerState.itemAmount[item] : maxAmmo;

					//relation based on amount gained and heuristicFactor travelled
					heuristic += (ammo / (float)maxAmmo) * (1.0f - (weight / (float)maxWeight)) * heuristicFactor;
					break;
				case WP_MACHINEGUN:
					maxAmmo = 50;

					//lets calculate the item heuristicFactor based on the player status
					if (playerState.stats[STAT_WEAPONS] & (1 << pWeaponPickup->GetCode()))
					{
						heuristicFactor = 0.05f;
						if (playerState.ammo[pWeaponPickup->GetCode()] > 80)
							heuristicFactor *= 0.5f;
					}
					else heuristicFactor = 0.1f;

					weight = (playerState.itemWeight[item] < maxWeight) ?
						playerState.itemWeight[item] : maxWeight;
					ammo = (playerState.itemAmount[item] < maxAmmo) ?
						playerState.itemAmount[item] : maxAmmo;

					//relation based on amount gained and heuristicFactor travelled
					heuristic += (ammo / (float)maxAmmo) * (1.0f - (weight / (float)maxWeight)) * heuristicFactor;
					break;
				case WP_PLASMAGUN:
					maxAmmo = 30;

					//lets calculate the item heuristicFactor based on the player status
					if (playerState.stats[STAT_WEAPONS] & (1 << pWeaponPickup->GetCode()))
					{
						heuristicFactor = 0.05f;
						if (playerState.ammo[pWeaponPickup->GetCode()] > 60)
							heuristicFactor *= 0.5f;
					}
					else heuristicFactor = 0.1f;

					weight = (playerState.itemWeight[item] < maxWeight) ?
						playerState.itemWeight[item] : maxWeight;
					ammo = (playerState.itemAmount[item] < maxAmmo) ?
						playerState.itemAmount[item] : maxAmmo;

					//relation based on amount gained and heuristicFactor travelled
					heuristic += (ammo / (float)maxAmmo) * (1.0f - (weight / (float)maxWeight)) * heuristicFactor;
					break;
				case WP_GRENADE_LAUNCHER:
					maxAmmo = 5;
					
					heuristicFactor = 0.f;

					weight = (playerState.itemWeight[item] < maxWeight) ?
						playerState.itemWeight[item] : maxWeight;
					ammo = (playerState.itemAmount[item] < maxAmmo) ?
						playerState.itemAmount[item] : maxAmmo;

					//relation based on amount gained and heuristicFactor travelled
					heuristic += (ammo / (float)maxAmmo) * (1.0f - (weight / (float)maxWeight)) * heuristicFactor;
					break;
				case WP_ROCKET_LAUNCHER:
					maxAmmo = 5;

					//lets calculate the item heuristicFactor based on the player status
					if (playerState.stats[STAT_WEAPONS] & (1 << pWeaponPickup->GetCode()))
					{
						heuristicFactor = 0.1f;
						if (playerState.ammo[pWeaponPickup->GetCode()] > 10)
							heuristicFactor *= 0.5f;
					}
					else heuristicFactor = 0.2f;

					weight = (playerState.itemWeight[item] < maxWeight) ?
						playerState.itemWeight[item] : maxWeight;
					ammo = (playerState.itemAmount[item] < maxAmmo) ?
						playerState.itemAmount[item] : maxAmmo;

					//relation based on amount gained and heuristicFactor travelled
					heuristic += (ammo / (float)maxAmmo) * (1.0f - (weight / (float)maxWeight)) * heuristicFactor;
					break;
				case WP_RAILGUN:
					maxAmmo = 10;

					//lets calculate the item heuristicFactor based on the player status
					if (playerState.stats[STAT_WEAPONS] & (1 << pWeaponPickup->GetCode()))
					{
						heuristicFactor = 0.1f;
						if (playerState.ammo[pWeaponPickup->GetCode()] > 10)
							heuristicFactor *= 0.5f;
					}
					else heuristicFactor = 0.2f;

					weight = (playerState.itemWeight[item] < maxWeight) ?
						playerState.itemWeight[item] : maxWeight;
					ammo = (playerState.itemAmount[item] < maxAmmo) ?
						playerState.itemAmount[item] : maxAmmo;

					//relation based on amount gained and heuristicFactor travelled
					heuristic += (ammo / (float)maxAmmo) * (1.0f - (weight / (float)maxWeight)) * heuristicFactor;
					break;
			}
		}
		else if (item->GetType() == "Ammo")
		{
			eastl::shared_ptr<AmmoPickup> pAmmoPickup =
				item->GetComponent<AmmoPickup>(AmmoPickup::Name).lock();

			switch (pAmmoPickup->GetCode())
			{
				case WP_LIGHTNING:
					maxAmmo = 60;
					if (playerState.stats[STAT_WEAPONS] & (1 << pAmmoPickup->GetCode()))
					{
						//lets calculate the item heuristicFactor based on the player status
						heuristicFactor = 0.1f;
						if (playerState.ammo[pAmmoPickup->GetCode()] > 80)
							heuristicFactor *= 0.5f;

						weight = (playerState.itemWeight[item] < maxWeight) ?
							playerState.itemWeight[item] : maxWeight;
						ammo = (playerState.itemAmount[item] < maxAmmo) ?
							playerState.itemAmount[item] : maxAmmo;

						//relation based on amount gained and heuristicFactor travelled
						heuristic += (ammo / (float)maxAmmo) * (1.0f - (weight / (float)maxWeight)) * heuristicFactor;
					}
					break;
				case WP_SHOTGUN:
					maxAmmo = 10;
					if (playerState.stats[STAT_WEAPONS] & (1 << pAmmoPickup->GetCode()))
					{
						//lets calculate the item heuristicFactor based on the player status
						heuristicFactor = 0.1f;
						if (playerState.ammo[pAmmoPickup->GetCode()] > 10)
							heuristicFactor *= 0.5f;

						weight = (playerState.itemWeight[item] < maxWeight) ?
							playerState.itemWeight[item] : maxWeight;
						ammo = (playerState.itemAmount[item] < maxAmmo) ?
							playerState.itemAmount[item] : maxAmmo;

						//relation based on amount gained and heuristicFactor travelled
						heuristic += (ammo / (float)maxAmmo) * (1.0f - (weight / (float)maxWeight)) * heuristicFactor;
					}
					break;
				case WP_MACHINEGUN:
					maxAmmo = 50;
					if (playerState.stats[STAT_WEAPONS] & (1 << pAmmoPickup->GetCode()))
					{
						//lets calculate the item heuristicFactor based on the player status
						heuristicFactor = 0.05f;
						if (playerState.ammo[pAmmoPickup->GetCode()] > 80)
							heuristicFactor *= 0.5f;

						weight = (playerState.itemWeight[item] < maxWeight) ?
							playerState.itemWeight[item] : maxWeight;
						ammo = (playerState.itemAmount[item] < maxAmmo) ?
							playerState.itemAmount[item] : maxAmmo;

						//relation based on amount gained and heuristicFactor travelled
						heuristic += (ammo / (float)maxAmmo) * (1.0f - (weight / (float)maxWeight)) * heuristicFactor;
					}
					break;
				case WP_PLASMAGUN:
					maxAmmo = 30;
					if (playerState.stats[STAT_WEAPONS] & (1 << pAmmoPickup->GetCode()))
					{
						//lets calculate the item heuristicFactor based on the player status
						heuristicFactor = 0.05f;
						if (playerState.ammo[pAmmoPickup->GetCode()] > 60)
							heuristicFactor *= 0.5f;

						weight = (playerState.itemWeight[item] < maxWeight) ?
							playerState.itemWeight[item] : maxWeight;
						ammo = (playerState.itemAmount[item] < maxAmmo) ?
							playerState.itemAmount[item] : maxAmmo;

						//relation based on amount gained and heuristicFactor travelled
						heuristic += (ammo / (float)maxAmmo) * (1.0f - (weight / (float)maxWeight)) * heuristicFactor;
					}
					break;
				case WP_GRENADE_LAUNCHER:
					maxAmmo = 5;
					if (playerState.stats[STAT_WEAPONS] & (1 << pAmmoPickup->GetCode()))
					{
						heuristicFactor = 0.f;

						weight = (playerState.itemWeight[item] < maxWeight) ?
							playerState.itemWeight[item] : maxWeight;
						ammo = (playerState.itemAmount[item] < maxAmmo) ?
							playerState.itemAmount[item] : maxAmmo;

						//relation based on amount gained and heuristicFactor travelled
						heuristic += (ammo / (float)maxAmmo) * (1.0f - (weight / (float)maxWeight)) * heuristicFactor;
					}
					break;
				case WP_ROCKET_LAUNCHER:
					maxAmmo = 5;
					if (playerState.stats[STAT_WEAPONS] & (1 << pAmmoPickup->GetCode()))
					{
						//lets calculate the item heuristicFactor based on the player status
						heuristicFactor = 0.1f;
						if (playerState.ammo[pAmmoPickup->GetCode()] > 10)
							heuristicFactor *= 0.5f;

						weight = (playerState.itemWeight[item] < maxWeight) ?
							playerState.itemWeight[item] : maxWeight;
						ammo = (playerState.itemAmount[item] < maxAmmo) ?
							playerState.itemAmount[item] : maxAmmo;

						//relation based on amount gained and heuristicFactor travelled
						heuristic += (ammo / (float)maxAmmo) * (1.0f - (weight / (float)maxWeight)) * heuristicFactor;
					}
					break;
				case WP_RAILGUN:
					maxAmmo = 10;
					if (playerState.stats[STAT_WEAPONS] & (1 << pAmmoPickup->GetCode()))
					{
						//lets calculate the item heuristicFactor based on the player status
						heuristicFactor = 0.1f;
						if (playerState.ammo[pAmmoPickup->GetCode()] > 10)
							heuristicFactor *= 0.5f;

						weight = (playerState.itemWeight[item] < maxWeight) ?
							playerState.itemWeight[item] : maxWeight;
						ammo = (playerState.itemAmount[item] < maxAmmo) ?
							playerState.itemAmount[item] : maxAmmo;

						//relation based on amount gained and heuristicFactor travelled
						heuristic += (ammo / (float)maxAmmo) * (1.0f - (weight / (float)maxWeight)) * heuristicFactor;
					}
					break;
			}
		}
		else if (item->GetType() == "Armor")
		{
			eastl::shared_ptr<ArmorPickup> pArmorPickup =
				item->GetComponent<ArmorPickup>(ArmorPickup::Name).lock();

			int armor = 0;
			int maxArmor = 100;

			//lets calculate the item heuristicFactor based on the player status
			heuristicFactor = 0.3f;
			if (playerState.stats[STAT_ARMOR] > 80)
				heuristicFactor *= 0.25f;

			weight = (playerState.itemWeight[item] < maxWeight) ?
				playerState.itemWeight[item] : maxWeight;
			armor = (playerState.itemAmount[item] < maxArmor) ?
				playerState.itemAmount[item] : maxArmor;

			//relation based on amount gained and heuristicFactor travelled
			heuristic += (armor / (float)maxArmor) * (1.0f - (weight / (float)maxWeight)) * heuristicFactor;
		}
		else if (item->GetType() == "Health")
		{
			eastl::shared_ptr<HealthPickup> pHealthPickup =
				item->GetComponent<HealthPickup>(HealthPickup::Name).lock();

			int health = 0;
			int maxHealth = 100;

			//lets calculate the item heuristicFactor based on the player status
			heuristicFactor = 0.3f;
			if (playerState.stats[STAT_HEALTH] > 80)
				heuristicFactor *= 0.25f;

			weight = (playerState.itemWeight[item] < maxWeight) ?
				playerState.itemWeight[item] : maxWeight;
			health = (playerState.itemAmount[item] < maxHealth) ?
				playerState.itemAmount[item] : maxHealth;

			//relation based on amount gained and heuristicFactor travelled
			heuristic += (health / (float)maxHealth) * (1.0f - (weight / (float)maxWeight)) * heuristicFactor;
		}
	}

	return heuristic;
}

void QuakeAIManager::CalculateHeuristic(NodeState& playerState, NodeState& otherPlayerState)
{
	float heuristic = 0.f;

	//heuristic from picked up items
	heuristic += CalculateHeuristicItems(playerState);
	heuristic -= CalculateHeuristicItems(otherPlayerState);

	//heuristic from damage dealing
	playerState.weaponTarget = INVALID_ACTOR_ID;
	otherPlayerState.weaponTarget = INVALID_ACTOR_ID;
	int playerMaxDamage = 0, otherPlayerMaxDamage = 0;
	for (int weapon = 1; weapon <= MAX_WEAPONS; weapon++)
	{
		if (playerState.current)
		{
			if (playerState.weapon == (WeaponType)weapon)
			{
				if (playerState.damage[weapon - 1] > playerMaxDamage)
				{
					playerState.weaponTarget = otherPlayerState.player;
					playerMaxDamage = playerState.damage[weapon - 1];
				}
			}
		}
		else
		{
			if (playerState.damage[weapon - 1] > playerMaxDamage)
			{
				playerState.weaponTarget = otherPlayerState.player;
				playerState.weapon = (WeaponType)weapon;
				playerMaxDamage = playerState.damage[weapon - 1];
			}
		}

		if (otherPlayerState.current)
		{
			if (otherPlayerState.weapon == (WeaponType)weapon)
			{
				if (otherPlayerState.damage[weapon - 1] > otherPlayerMaxDamage)
				{
					otherPlayerState.weaponTarget = playerState.player;
					otherPlayerMaxDamage = otherPlayerState.damage[weapon - 1];
				}
			}
		}
		else
		{
			if (otherPlayerState.damage[weapon - 1] > otherPlayerMaxDamage)
			{
				otherPlayerState.weaponTarget = playerState.player;
				otherPlayerState.weapon = (WeaponType)weapon;
				otherPlayerMaxDamage = otherPlayerState.damage[weapon - 1];
			}
		}
	}

	//damage heuristic
	if (playerMaxDamage > 0 || otherPlayerMaxDamage > 0)
	{
		//health & armor status
		unsigned int maxHealth = 200;
		unsigned int maxArmor = 200;
		heuristic += (playerState.stats[STAT_HEALTH] / (float)maxHealth) * 0.1f;
		heuristic += (playerState.stats[STAT_ARMOR] / (float)maxArmor) * 0.1f;

		heuristic -= (otherPlayerState.stats[STAT_HEALTH] / (float)maxHealth) * 0.1f;
		heuristic -= (otherPlayerState.stats[STAT_ARMOR] / (float)maxArmor) * 0.1f;

		//damage
		int maxDamage = 300;
		heuristic += (playerMaxDamage / (float)maxDamage) * 0.4f;
		heuristic -= (otherPlayerMaxDamage / (float)maxDamage) * 0.4f;
	}

	playerState.heuristic = heuristic;
	otherPlayerState.heuristic = heuristic;
}

void QuakeAIManager::CalculateDamage(NodeState& state, 
	float visibleTime, float visibleDistance, float visibleHeight)
{
	for (int weapon = 1; weapon <= MAX_WEAPONS; weapon++)
	{
		int shotCount = visibleTime >= 0.3f ? 1 : 0;

		if (weapon != WP_GAUNTLET)
		{
			if (state.ammo[weapon] && (state.stats[STAT_WEAPONS] & (1 << weapon)))
			{
				int damage = 0;
				float fireTime = 0.f;
				float rangeDistance = 0;
				switch (weapon)
				{
					case WP_LIGHTNING:
						damage = 6;
						fireTime = 0.05f;
						state.damage[weapon - 1] = 0;
						if (visibleTime > fireTime)
							shotCount = (int)round(visibleTime / fireTime);
						shotCount = shotCount > state.ammo[weapon] ? state.ammo[weapon] : shotCount;
						if (visibleDistance <= 800)
							state.damage[weapon - 1] = damage * shotCount;
						break;
					case WP_SHOTGUN:
						damage = 100;
						fireTime = 1.0f;
						rangeDistance = visibleDistance > 500 ? visibleDistance : 500;
						if (visibleTime > fireTime)
							shotCount = (int)round(visibleTime / fireTime);
						shotCount = shotCount > state.ammo[weapon] ? state.ammo[weapon] : shotCount;
						state.damage[weapon - 1] = (int)round(damage *
							(1.f - (visibleDistance / rangeDistance)) * shotCount);
						break;
					case WP_MACHINEGUN:
						damage = 4;
						fireTime = 0.1f;
						rangeDistance = visibleDistance > 300 ? visibleDistance : 300;
						if (visibleTime > fireTime)
							shotCount = (int)round(visibleTime / fireTime);
						shotCount = shotCount > state.ammo[weapon] ? state.ammo[weapon] : shotCount;
						state.damage[weapon - 1] = (int)round(damage *
							(1.f - (visibleDistance / rangeDistance)) * shotCount);
						break;
					case WP_GRENADE_LAUNCHER:
						damage = 80;
						fireTime = 0.8f;
						state.damage[weapon - 1] = 0;
						break;
					case WP_ROCKET_LAUNCHER:
						damage = 80;
						fireTime = 0.8f;
						if (visibleHeight <= 30.f)
							rangeDistance = visibleDistance > 500 ? visibleDistance : 500;
						else
							rangeDistance = visibleDistance > 700 ? visibleDistance : 700;

						if (visibleTime > fireTime)
							shotCount = (int)round(visibleTime / fireTime);
						shotCount = shotCount > state.ammo[weapon] ? state.ammo[weapon] : shotCount;
						state.damage[weapon - 1] = (int)round(damage *
							(1.f - (visibleDistance / rangeDistance)) * shotCount);
						break;
					case WP_PLASMAGUN:
						damage = 6;
						fireTime = 0.1f;
						rangeDistance = visibleDistance > 300 ? visibleDistance : 300;
						if (visibleTime > fireTime)
							shotCount = (int)round(visibleTime / fireTime);
						shotCount = shotCount > state.ammo[weapon] ? state.ammo[weapon] : shotCount;
						state.damage[weapon - 1] = (int)round(damage *
							(1.f - (visibleDistance / rangeDistance)) * shotCount);
						break;
					case WP_RAILGUN:
						damage = 100;
						fireTime = 1.5f;
						if (visibleTime > fireTime)
							shotCount = (int)round(visibleTime / fireTime);
						shotCount = shotCount > state.ammo[weapon] ? state.ammo[weapon] : shotCount;
						state.damage[weapon - 1] = damage * shotCount;
						break;
				}
			}
		}
		else
		{
			if (visibleDistance <= 20.f)
			{
				int damage = 50;
				float fireTime = 1.5f;
				if (visibleTime > fireTime)
					shotCount = (int)round(visibleTime / fireTime);
				state.damage[weapon - 1] = damage * shotCount;
			}
		}
	}
}

/*
CanItemBeGrabbed
Returns false if the item should not be picked up.
*/
bool QuakeAIManager::CanItemBeGrabbed(ActorId itemId, float itemTime, 
	NodeState& playerState, eastl::map<ActorId, float>& excludeActors)
{
	eastl::shared_ptr<Actor> pItemActor(GameLogic::Get()->GetActor(itemId).lock());
	if (pItemActor)
	{
		if (pItemActor->GetType() == "Weapon")
		{
			eastl::shared_ptr<WeaponPickup> pWeaponPickup =
				pItemActor->GetComponent<WeaponPickup>(WeaponPickup::Name).lock();
			float respawnItemTime = pWeaponPickup->mRespawnTime / 1000.f;
			if (excludeActors.find(itemId) != excludeActors.end())
			{
				if (excludeActors[itemId] - itemTime > 0)
					return false;
			}
			else
			{
				if (respawnItemTime - itemTime > 0)
					return false;
			}

			if (playerState.ammo[pWeaponPickup->GetCode()] >= 200)
				return false;		// can't hold any more

			return true;
		}
		else if (pItemActor->GetType() == "Ammo")
		{
			eastl::shared_ptr<AmmoPickup> pAmmoPickup =
				pItemActor->GetComponent<AmmoPickup>(AmmoPickup::Name).lock();
			float respawnItemTime = pAmmoPickup->mRespawnTime / 1000.f;
			if (excludeActors.find(itemId) != excludeActors.end())
			{
				if (excludeActors[itemId] - itemTime > 0)
					return false;
			}
			else
			{
				if (respawnItemTime - itemTime > 0)
					return false;
			}

			if (playerState.ammo[pAmmoPickup->GetCode()] >= 200)
				return false;		// can't hold any more

			return true;
		}
		else if (pItemActor->GetType() == "Armor")
		{
			eastl::shared_ptr<ArmorPickup> pArmorPickup =
				pItemActor->GetComponent<ArmorPickup>(ArmorPickup::Name).lock();
			float respawnItemTime = pArmorPickup->mRespawnTime / 1000.f;
			if (excludeActors.find(itemId) != excludeActors.end())
			{
				if (excludeActors[itemId] - itemTime > 0)
					return false;
			}
			else
			{
				if (respawnItemTime - itemTime > 0)
					return false;
			}

			if (playerState.stats[STAT_ARMOR] >= playerState.stats[STAT_MAX_HEALTH] * 2)
				return false;		// can't hold any more

			return true;
		}
		else if (pItemActor->GetType() == "Health")
		{
			eastl::shared_ptr<HealthPickup> pHealthPickup =
				pItemActor->GetComponent<HealthPickup>(HealthPickup::Name).lock();
			float respawnItemTime = pHealthPickup->mRespawnTime / 1000.f;
			if (excludeActors.find(itemId) != excludeActors.end())
			{
				if (excludeActors[itemId] - itemTime > 0)
					return false;
			}
			else
			{
				if (respawnItemTime - itemTime > 0)
					return false;
			}

			// small and mega healths will go over the max, otherwise
			// don't pick up if already at max
			if (pHealthPickup->GetAmount() == 5 || pHealthPickup->GetAmount() == 100)
			{
				if (playerState.stats[STAT_HEALTH] >= playerState.stats[STAT_MAX_HEALTH] * 2)
					return false;		// can't hold any more

				return true;
			}

			if (playerState.stats[STAT_HEALTH] >= playerState.stats[STAT_MAX_HEALTH])
				return false;		// can't hold any more

			return true;
		}
	}
	return false;
}

void QuakeAIManager::PickupItems(NodeState& playerState, 
	eastl::map<ActorId, float>& actors, eastl::map<ActorId, float>& excludeActors)
{
	for (auto actor : actors)
	{
		eastl::shared_ptr<Actor> pItemActor(
			GameLogic::Get()->GetActor(actor.first).lock());
		if (pItemActor)
		{
			if (pItemActor->GetType() == "Weapon")
			{
				eastl::shared_ptr<WeaponPickup> pWeaponPickup =
					pItemActor->GetComponent<WeaponPickup>(WeaponPickup::Name).lock();
				float respawnTime = pWeaponPickup->mRespawnTime / 1000.f;
				if (excludeActors.find(actor.first) != excludeActors.end())
				{
					if (excludeActors[actor.first] - actor.second > 0)
						continue;
				}
				else
				{
					if (respawnTime - actor.second > 0)
						continue;
				}

				// add ammo
				int ammo = playerState.ammo[pWeaponPickup->GetCode()] + pWeaponPickup->GetAmmo();
				if (ammo > 200)
				{
					//add amount and weight
					playerState.items.push_back(pItemActor);
					playerState.itemWeight[pItemActor] = actor.second;
					playerState.itemAmount[pItemActor] = pWeaponPickup->GetAmmo() - (ammo - 200);
				}
				else
				{
					//add amount and weight
					playerState.items.push_back(pItemActor);
					playerState.itemWeight[pItemActor] = actor.second;
					playerState.itemAmount[pItemActor] = pWeaponPickup->GetAmmo();
				}
			}
			else if (pItemActor->GetType() == "Ammo")
			{
				eastl::shared_ptr<AmmoPickup> pAmmoPickup =
					pItemActor->GetComponent<AmmoPickup>(AmmoPickup::Name).lock();
				float respawnTime = pAmmoPickup->mRespawnTime / 1000.f;
				if (excludeActors.find(actor.first) != excludeActors.end())
				{
					if (excludeActors[actor.first] - actor.second > 0)
						continue;
				}
				else
				{
					if (respawnTime - actor.second > 0)
						continue;
				}

				int ammo = playerState.ammo[pAmmoPickup->GetCode()] + pAmmoPickup->GetAmount();
				if (ammo > 200)
				{
					//add ammunt and weight
					playerState.items.push_back(pItemActor);
					playerState.itemWeight[pItemActor] = actor.second;
					playerState.itemAmount[pItemActor] = pAmmoPickup->GetAmount() - (ammo - 200);
				}
				else
				{
					//add amount and weight
					playerState.items.push_back(pItemActor);
					playerState.itemWeight[pItemActor] = actor.second;
					playerState.itemAmount[pItemActor] = pAmmoPickup->GetAmount();
				}
			}
			else if (pItemActor->GetType() == "Armor")
			{
				eastl::shared_ptr<ArmorPickup> pArmorPickup =
					pItemActor->GetComponent<ArmorPickup>(ArmorPickup::Name).lock();
				float respawnTime = pArmorPickup->mRespawnTime / 1000.f;
				if (excludeActors.find(actor.first) != excludeActors.end())
				{
					if (excludeActors[actor.first] - actor.second > 0)
						continue;
				}
				else
				{
					if (respawnTime - actor.second > 0)
						continue;
				}

				int armor = playerState.stats[STAT_ARMOR] + pArmorPickup->GetAmount();
				if (armor > playerState.stats[STAT_MAX_HEALTH] * 2)
				{
					//add ammount and weight
					playerState.items.push_back(pItemActor);
					playerState.itemWeight[pItemActor] = actor.second;
					playerState.itemAmount[pItemActor] = pArmorPickup->GetAmount() -
						(armor - playerState.stats[STAT_MAX_HEALTH] * 2);
					if (playerState.itemAmount[pItemActor] < 0)
						playerState.itemAmount[pItemActor] = 0;
				}
				else
				{
					//add ammount and weight
					playerState.items.push_back(pItemActor);
					playerState.itemWeight[pItemActor] = actor.second;
					playerState.itemAmount[pItemActor] = pArmorPickup->GetAmount();
				}

			}
			else if (pItemActor->GetType() == "Health")
			{
				eastl::shared_ptr<HealthPickup> pHealthPickup =
					pItemActor->GetComponent<HealthPickup>(HealthPickup::Name).lock();
				float respawnTime = pHealthPickup->mRespawnTime / 1000.f;
				if (excludeActors.find(actor.first) != excludeActors.end())
				{
					if (excludeActors[actor.first] - actor.second > 0)
						continue;
				}
				else
				{
					if (respawnTime - actor.second > 0)
						continue;
				}

				int max;
				if (pHealthPickup->GetAmount() != 5 && pHealthPickup->GetAmount() != 100)
					max = playerState.stats[STAT_MAX_HEALTH];
				else
					max = playerState.stats[STAT_MAX_HEALTH] * 2;

				int health = playerState.stats[STAT_HEALTH] + pHealthPickup->GetAmount();
				if (health > max)
				{
					//add ammount and weight
					playerState.items.push_back(pItemActor);
					playerState.itemWeight[pItemActor] = actor.second;
					playerState.itemAmount[pItemActor] = pHealthPickup->GetAmount() - (health - max);
					if (playerState.itemAmount[pItemActor] < 0)
						playerState.itemAmount[pItemActor] = 0;
				}
				else
				{
					//add ammount and weight
					playerState.items.push_back(pItemActor);
					playerState.itemWeight[pItemActor] = actor.second;
					playerState.itemAmount[pItemActor] = pHealthPickup->GetAmount();
				}
			}
		}
	}
}

void QuakeAIManager::FindPath(NodeState& pNodeState, 
	PathingCluster* pGoalCluster, PathingArcVec& planPath, 
	eastl::map<ActorId, float>& excludeActors, float threshold)
{
	// find the best path using an A* search algorithm
	AIFinder aiFinder;
	aiFinder(pNodeState, pGoalCluster, planPath, excludeActors, threshold);
}

void QuakeAIManager::OnUpdate(unsigned long deltaMs)
{
	//update ai guessing system
	Timer::RealTimeDate realTime = Timer::GetRealTimeAndDate();
	eastl::string info = "\n\n guessing system update time " +
		eastl::to_string(realTime.Hour) + ":" +
		eastl::to_string(realTime.Minute) + ":" +
		eastl::to_string(realTime.Second) + "\n";
	PrintLogGuessInformation(info);

	GameApplication* gameApp = (GameApplication*)Application::App;
	QuakeLogic* game = static_cast<QuakeLogic *>(GameLogic::Get());

	eastl::vector<eastl::shared_ptr<PlayerActor>> playerActors;
	game->GetPlayerActors(playerActors);
	for (eastl::shared_ptr<PlayerActor> pPlayerActor : playerActors)
	{
		mPlayerPlanTime[pPlayerActor->GetId()] += deltaMs / 1000.f;

		info = "\n\n player id " + eastl::to_string(pPlayerActor->GetId()) + 
			" planning time " + eastl::to_string(mPlayerPlanTime[pPlayerActor->GetId()]);
		PrintLogGuessInformation(info);

		for (eastl::shared_ptr<PlayerActor> pOtherPlayerActor : playerActors)
		{
			if (pPlayerActor->GetId() == pOtherPlayerActor->GetId())
				continue;

			info = "\n other player id " + eastl::to_string(pOtherPlayerActor->GetId());
			PrintLogGuessInformation(info);

			NodeState otherPlayerGuessState;
			GetPlayerGuessState(pOtherPlayerActor->GetId(), otherPlayerGuessState);

			info = "\n minimax guess state ";
			PrintLogGuessInformation(info);
			if (otherPlayerGuessState.plan.node)
			{
				PrintLogGuessInformation("\n other player position : " +
					eastl::to_string(otherPlayerGuessState.plan.node->GetPos()[0]) + " " +
					eastl::to_string(otherPlayerGuessState.plan.node->GetPos()[1]) + " " +
					eastl::to_string(otherPlayerGuessState.plan.node->GetPos()[2]) + " ");
				PrintLogGuessInformation("\n other player plan cluster : " +
					eastl::to_string(otherPlayerGuessState.plan.node->GetCluster()) + " ");
			}
			PrintLogGuessInformation("\n other player path id " + eastl::to_string(otherPlayerGuessState.plan.id) + " : ");
			for (PathingArc* pathArc : otherPlayerGuessState.plan.path)
				PrintLogGuessInformation(eastl::to_string(pathArc->GetNode()->GetId()) + " ");
			PrintLogGuessInformation("\n other player heuristic : " + eastl::to_string(otherPlayerGuessState.heuristic) + " ");
			if (otherPlayerGuessState.weapon != WP_NONE)
			{
				info = "weapon : " + eastl::to_string(otherPlayerGuessState.weapon) + " ";
				PrintLogGuessInformation(info);

				info = "damage : " +
					eastl::to_string(otherPlayerGuessState.damage[otherPlayerGuessState.weapon - 1]) + " ";
				PrintLogGuessInformation(info);
			}
			if (!otherPlayerGuessState.items.empty())
				PrintLogGuessInformation("actors : ");
			for (eastl::shared_ptr<Actor> pItemActor : otherPlayerGuessState.items)
			{
				if (pItemActor->GetType() == "Weapon")
				{
					eastl::shared_ptr<WeaponPickup> pWeaponPickup =
						pItemActor->GetComponent<WeaponPickup>(WeaponPickup::Name).lock();
					PrintLogGuessInformation("weapon " + eastl::to_string(pWeaponPickup->GetCode()) + " ");
				}
				else if (pItemActor->GetType() == "Ammo")
				{
					eastl::shared_ptr<AmmoPickup> pAmmoPickup =
						pItemActor->GetComponent<AmmoPickup>(AmmoPickup::Name).lock();
					PrintLogGuessInformation("ammo " + eastl::to_string(pAmmoPickup->GetCode()) + " ");
				}
				else if (pItemActor->GetType() == "Armor")
				{
					eastl::shared_ptr<ArmorPickup> pArmorPickup =
						pItemActor->GetComponent<ArmorPickup>(ArmorPickup::Name).lock();
					PrintLogGuessInformation("armor " + eastl::to_string(pArmorPickup->GetCode()) + " ");
				}
				else if (pItemActor->GetType() == "Health")
				{
					eastl::shared_ptr<HealthPickup> pHealthPickup =
						pItemActor->GetComponent<HealthPickup>(HealthPickup::Name).lock();
					PrintLogGuessInformation("health " + eastl::to_string(pHealthPickup->GetCode()) + " ");
				}
			}

			NodePlan otherPlayerGuessPlan;
			GetPlayerGuessPlan(pOtherPlayerActor->GetId(), otherPlayerGuessPlan);

			info = "\n current guess plan ";
			PrintLogGuessInformation(info);
			if (otherPlayerGuessPlan.node)
			{
				PrintLogGuessInformation("\n other player position : " +
					eastl::to_string(otherPlayerGuessPlan.node->GetPos()[0]) + " " +
					eastl::to_string(otherPlayerGuessPlan.node->GetPos()[1]) + " " +
					eastl::to_string(otherPlayerGuessPlan.node->GetPos()[2]) + " ");
				PrintLogGuessInformation("\n other player plan cluster : " +
					eastl::to_string(otherPlayerGuessPlan.node->GetCluster()) + " ");
			}
			PrintLogGuessInformation("\n other player path id " + eastl::to_string(otherPlayerGuessPlan.id) + " : ");
			for (PathingArc* pathArc : otherPlayerGuessPlan.path)
				PrintLogGuessInformation(eastl::to_string(pathArc->GetNode()->GetId()) + " ");

			eastl::map<ActorId, float> guessItems;
			GetPlayerGuessItems(pOtherPlayerActor->GetId(), guessItems);
			if (!guessItems.empty())
			{
				info = "\n current guess items to exclude : ";
				PrintLogGuessInformation(info);
			}
			for (auto guessItem : guessItems)
			{
				eastl::shared_ptr<Actor> pItemActor(GameLogic::Get()->GetActor(guessItem.first).lock());

				if (pItemActor->GetType() == "Weapon")
				{
					eastl::shared_ptr<WeaponPickup> pWeaponPickup =
						pItemActor->GetComponent<WeaponPickup>(WeaponPickup::Name).lock();

					info = "weapon " + eastl::to_string(pWeaponPickup->GetCode()) + " ";
					PrintLogGuessInformation(info);
				}
				else if (pItemActor->GetType() == "Ammo")
				{
					eastl::shared_ptr<AmmoPickup> pAmmoPickup =
						pItemActor->GetComponent<AmmoPickup>(AmmoPickup::Name).lock();

					info = "ammo " + eastl::to_string(pAmmoPickup->GetCode()) + " ";
					PrintLogGuessInformation(info);
				}
				else if (pItemActor->GetType() == "Armor")
				{
					eastl::shared_ptr<ArmorPickup> pArmorPickup =
						pItemActor->GetComponent<ArmorPickup>(ArmorPickup::Name).lock();

					info = "armor " + eastl::to_string(pArmorPickup->GetCode()) + " ";
					PrintLogGuessInformation(info);
				}
				else if (pItemActor->GetType() == "Health")
				{
					eastl::shared_ptr<HealthPickup> pHealthPickup =
						pItemActor->GetComponent<HealthPickup>(HealthPickup::Name).lock();

					info = "health " + eastl::to_string(pHealthPickup->GetCode()) + " ";
					PrintLogGuessInformation(info);
				}
			}

			if (IsPlayerGuessUpdated(pOtherPlayerActor->GetId()))
			{
				if (!otherPlayerGuessState.plan.path.empty() &&
					otherPlayerGuessPlan.id != otherPlayerGuessState.plan.id)
				{
					PathingNode* guessNode = otherPlayerGuessState.plan.node;
					otherPlayerGuessPlan = otherPlayerGuessState.plan;

					PathingArcVec::iterator itArc = otherPlayerGuessPlan.path.begin();
					PathingArcVec::iterator itPathArc = otherPlayerGuessPlan.path.begin();
					for (; itArc != otherPlayerGuessPlan.path.end(); itArc++)
					{
						if ((*itArc)->GetNode() == otherPlayerGuessPlan.node)
						{
							guessNode = otherPlayerGuessPlan.node;
							itPathArc = itArc;
							itPathArc++;
							break;
						}
					}

					PathingArcVec guessPath;
					for (; itPathArc != otherPlayerGuessPlan.path.end(); itPathArc++)
						guessPath.push_back((*itPathArc));

					otherPlayerGuessPlan.id = otherPlayerGuessState.plan.id;
					otherPlayerGuessPlan.path = guessPath;
					otherPlayerGuessPlan.node = guessNode;

					mPlayerPlanTime[pPlayerActor->GetId()] = 0;
					SetPlayerGuessPlan(pOtherPlayerActor->GetId(), otherPlayerGuessPlan);
					SetPlayerGuessUpdated(pOtherPlayerActor->GetId(), false);
				}
			}

			if (otherPlayerGuessPlan.path.size())
			{
				PathingArc* guessPath = (*otherPlayerGuessPlan.path.begin());
				if (mPlayerPlanTime[pPlayerActor->GetId()] >= guessPath->GetWeight())
				{
					eastl::vector<ActorId> actors;
					PathingTransition* guessTransition = 
						otherPlayerGuessPlan.node->FindTransition(guessPath->GetId());
					for (PathingNode* nodeTransition : guessTransition->GetNodes())
						if (nodeTransition->GetActorId() != INVALID_ACTOR_ID)
							actors.push_back(nodeTransition->GetActorId());
					if (otherPlayerGuessPlan.node->GetActorId() != INVALID_ACTOR_ID)
						actors.push_back(otherPlayerGuessPlan.node->GetActorId());

					otherPlayerGuessPlan.node = guessPath->GetNode();
					mPlayerPlanTime[pPlayerActor->GetId()] -= guessPath->GetWeight();
					otherPlayerGuessPlan.path.erase(otherPlayerGuessPlan.path.begin());

					for (ActorId actor : actors)
					{
						eastl::vector<eastl::shared_ptr<Actor>>::iterator itItem = 
							otherPlayerGuessState.items.begin();
						for (; itItem != otherPlayerGuessState.items.end(); itItem++)
						{
							eastl::shared_ptr<Actor> pItemActor = (*itItem);
							if (pItemActor->GetId() == actor)
							{
								if (pItemActor->GetType() == "Weapon")
								{
									eastl::shared_ptr<WeaponPickup> pWeaponPickup =
										pItemActor->GetComponent<WeaponPickup>(WeaponPickup::Name).lock();
									//printf("\n current guess item weapon %u ", pWeaponPickup->GetCode());

									// add the weapon
									otherPlayerGuessState.stats[STAT_WEAPONS] |= (1 << pWeaponPickup->GetCode());

									// add ammo
									otherPlayerGuessState.ammo[pWeaponPickup->GetCode()] += 
										otherPlayerGuessState.itemAmount[pItemActor];

									guessItems[actor] = (float)pWeaponPickup->GetWait() / 1000.f;
								}
								else if (pItemActor->GetType() == "Ammo")
								{
									eastl::shared_ptr<AmmoPickup> pAmmoPickup =
										pItemActor->GetComponent<AmmoPickup>(AmmoPickup::Name).lock();
									//printf("\n current guess item ammo %u ", pAmmoPickup->GetCode());

									otherPlayerGuessState.ammo[pAmmoPickup->GetCode()] += 
										otherPlayerGuessState.itemAmount[pItemActor];

									guessItems[actor] = (float)pAmmoPickup->GetWait() / 1000.f;
								}
								else if (pItemActor->GetType() == "Armor")
								{
									eastl::shared_ptr<ArmorPickup> pArmorPickup =
										pItemActor->GetComponent<ArmorPickup>(ArmorPickup::Name).lock();
									//printf("\n current guess item armor %u ", pArmorPickup->GetCode());

									otherPlayerGuessState.stats[STAT_ARMOR] += 
										otherPlayerGuessState.itemAmount[pItemActor];

									guessItems[actor] = (float)pArmorPickup->GetWait() / 1000.f;
								}
								else if (pItemActor->GetType() == "Health")
								{
									eastl::shared_ptr<HealthPickup> pHealthPickup =
										pItemActor->GetComponent<HealthPickup>(HealthPickup::Name).lock();
									//printf("\n current guess item health %u ", pHealthPickup->GetCode());

									otherPlayerGuessState.stats[STAT_HEALTH] += 
										otherPlayerGuessState.itemAmount[pItemActor];

									guessItems[actor] = (float)pHealthPickup->GetWait() / 1000.f;
								}

								otherPlayerGuessState.items.erase(itItem);
								otherPlayerGuessState.itemAmount.erase(pItemActor);
								otherPlayerGuessState.itemWeight.erase(pItemActor);
								break;
							}
						}
					}

					//printf("\n current guess node %u ", guessNode->GetId());
					SetPlayerGuessState(pOtherPlayerActor->GetId(), otherPlayerGuessState);
					SetPlayerGuessPlan(pOtherPlayerActor->GetId(), otherPlayerGuessPlan);
					SetPlayerGuessItems(pOtherPlayerActor->GetId(), guessItems);
				}
			}

			eastl::shared_ptr<TransformComponent> pTransformComponent(
				pPlayerActor->GetComponent<TransformComponent>(TransformComponent::Name).lock());
			if (pTransformComponent)
			{
				bool resetGuessPlayer = false;
				eastl::vector<ActorId> removeGuessItems;

				Vector3<float> playerPos = pTransformComponent->GetTransform().GetTranslation();
				playerPos += Vector3<float>::Unit(YAW) * (float)pPlayerActor->GetState().viewHeight;
				for (auto guessItem : guessItems)
				{
					guessItems[guessItem.first] -= deltaMs / 1000.f;

					eastl::shared_ptr<Actor> pItemActor(GameLogic::Get()->GetActor(guessItem.first).lock());
					eastl::shared_ptr<TransformComponent> pItemTransform(
						pItemActor->GetComponent<TransformComponent>(TransformComponent::Name).lock());
					Vector3<float> itemPos = pItemTransform->GetTransform().GetTranslation();

					eastl::vector<ActorId> collisionActors;
					eastl::vector<Vector3<float>> collisions, collisionNormals;
					GameLogic::Get()->GetGamePhysics()->CastRay(
						playerPos, itemPos, collisionActors, collisions, collisionNormals);

					ActorId closestCollisionId = INVALID_ACTOR_ID;
					Vector3<float> closestCollision = itemPos;
					for (unsigned int i = 0; i < collisionActors.size(); i++)
					{
						if (collisionActors[i] != pPlayerActor->GetId())
						{
							if (closestCollision != NULL)
							{
								if (Length(closestCollision - playerPos) > Length(collisions[i] - playerPos))
								{
									closestCollisionId = collisionActors[i];
									closestCollision = collisions[i];
								}
							}
							else
							{
								closestCollisionId = collisionActors[i];
								closestCollision = collisions[i];
							}
						}
					}

					if (closestCollisionId == pItemActor->GetId())
					{
						//check if the item is visible
						if (pItemActor->GetType() == "Weapon")
						{
							eastl::shared_ptr<WeaponPickup> pWeaponPickup =
								pItemActor->GetComponent<WeaponPickup>(WeaponPickup::Name).lock();

							if (pWeaponPickup->mRespawnTime <= 0.f)
							{
								//distrust the guessing plan and reset guess player status
								resetGuessPlayer = true;
								break;
							}
						}
						else if (pItemActor->GetType() == "Ammo")
						{
							eastl::shared_ptr<AmmoPickup> pAmmoPickup =
								pItemActor->GetComponent<AmmoPickup>(AmmoPickup::Name).lock();

							if (pAmmoPickup->mRespawnTime <= 0.f)
							{
								//distrust the guessing plan and reset guess player status
								resetGuessPlayer = true;
								break;
							}
						}
						else if (pItemActor->GetType() == "Armor")
						{
							eastl::shared_ptr<ArmorPickup> pArmorPickup =
								pItemActor->GetComponent<ArmorPickup>(ArmorPickup::Name).lock();

							if (pArmorPickup->mRespawnTime <= 0.f)
							{
								//distrust the guessing plan and reset guess player status
								resetGuessPlayer = true;
								break;
							}
						}
						else if (pItemActor->GetType() == "Health")
						{
							eastl::shared_ptr<HealthPickup> pHealthPickup =
								pItemActor->GetComponent<HealthPickup>(HealthPickup::Name).lock();

							if (pHealthPickup->mRespawnTime <= 0.f)
							{
								//distrust the guessing plan and reset guess player status
								resetGuessPlayer = true;
								break;
							}
						}
					}

					if (guessItems[guessItem.first] <= 0)
						removeGuessItems.push_back(guessItem.first);
				}

				Vector3<float> currentPosition = pTransformComponent->GetPosition();
				PathingNode* currentNode = mPathingGraph->FindClosestNode(currentPosition);

				eastl::shared_ptr<TransformComponent> pOtherPlayerTransformComponent(
					pOtherPlayerActor->GetComponent<TransformComponent>(TransformComponent::Name).lock());
				if (pOtherPlayerTransformComponent)
				{
					PathingNode* otherPlayerNode =
						mPathingGraph->FindClosestNode(pOtherPlayerTransformComponent->GetPosition());
					if (currentNode->IsVisibleNode(otherPlayerNode))
					{
						//update the player with the current status
						guessItems.clear();
						removeGuessItems.clear();

						//what i am guessing about the other player
						NodePlan otherPlayerGuessPlan;
						GetPlayerPlan(pOtherPlayerActor->GetId(), otherPlayerGuessPlan);
						otherPlayerGuessPlan = NodePlan(otherPlayerGuessPlan.node, PathingArcVec());

						SetPlayerGuessPlan(pOtherPlayerActor->GetId(), otherPlayerGuessPlan);
						SetPlayerGuessState(pOtherPlayerActor->GetId(), pOtherPlayerActor);
						SetPlayerGuessUpdated(pOtherPlayerActor->GetId(), false);

						mPlayerPlanTime[pPlayerActor->GetId()] = 0.f;
					}
					else if (currentNode->IsVisibleNode(otherPlayerGuessPlan.node))
					{
						//distrust the guessing plan and reset guess player status
						resetGuessPlayer = true;
					}
				}

				if (resetGuessPlayer)
				{
					RemovePlayerGuessItems(pOtherPlayerActor->GetId());

					//what i am guessing about the other player
					NodePlan otherPlayerGuessPlan;
					GetPlayerPlan(pOtherPlayerActor->GetId(), otherPlayerGuessPlan);
					otherPlayerGuessPlan = NodePlan(otherPlayerGuessPlan.node, PathingArcVec());

					SetPlayerGuessPlan(pOtherPlayerActor->GetId(), otherPlayerGuessPlan);
					SetPlayerGuessState(pOtherPlayerActor->GetId(), pOtherPlayerActor);
					SetPlayerGuessUpdated(pOtherPlayerActor->GetId(), false);

					mPlayerPlanTime[pPlayerActor->GetId()] = 0.f;
				}
				else
				{
					for (ActorId removeGuessItem : removeGuessItems)
						guessItems.erase(removeGuessItem);
					SetPlayerGuessItems(pOtherPlayerActor->GetId(), guessItems);
				}
			}

			GetPlayerGuessState(pOtherPlayerActor->GetId(), otherPlayerGuessState);

			info = "\n new minimax guess state ";
			PrintLogGuessInformation(info);
			if (otherPlayerGuessState.plan.node)
			{
				PrintLogGuessInformation("\n other player position : " +
					eastl::to_string(otherPlayerGuessState.plan.node->GetPos()[0]) + " " +
					eastl::to_string(otherPlayerGuessState.plan.node->GetPos()[1]) + " " +
					eastl::to_string(otherPlayerGuessState.plan.node->GetPos()[2]) + " ");
				PrintLogGuessInformation("\n other player plan cluster : " +
					eastl::to_string(otherPlayerGuessState.plan.node->GetCluster()) + " ");
			}
			PrintLogGuessInformation("\n other player path id " + eastl::to_string(otherPlayerGuessState.plan.id) + " : ");
			for (PathingArc* pathArc : otherPlayerGuessState.plan.path)
				PrintLogGuessInformation(eastl::to_string(pathArc->GetNode()->GetId()) + " ");
			PrintLogGuessInformation("\n other player heuristic : " + eastl::to_string(otherPlayerGuessState.heuristic) + " ");
			if (otherPlayerGuessState.weapon != WP_NONE)
			{
				info = "weapon : " + eastl::to_string(otherPlayerGuessState.weapon) + " ";
				PrintLogGuessInformation(info);

				info = "damage : " +
					eastl::to_string(otherPlayerGuessState.damage[otherPlayerGuessState.weapon - 1]) + " ";
				PrintLogGuessInformation(info);
			}
			if (!otherPlayerGuessState.items.empty())
				PrintLogGuessInformation("actors : ");
			for (eastl::shared_ptr<Actor> pItemActor : otherPlayerGuessState.items)
			{
				if (pItemActor->GetType() == "Weapon")
				{
					eastl::shared_ptr<WeaponPickup> pWeaponPickup =
						pItemActor->GetComponent<WeaponPickup>(WeaponPickup::Name).lock();
					PrintLogGuessInformation("weapon " + eastl::to_string(pWeaponPickup->GetCode()) + " ");
				}
				else if (pItemActor->GetType() == "Ammo")
				{
					eastl::shared_ptr<AmmoPickup> pAmmoPickup =
						pItemActor->GetComponent<AmmoPickup>(AmmoPickup::Name).lock();
					PrintLogGuessInformation("ammo " + eastl::to_string(pAmmoPickup->GetCode()) + " ");
				}
				else if (pItemActor->GetType() == "Armor")
				{
					eastl::shared_ptr<ArmorPickup> pArmorPickup =
						pItemActor->GetComponent<ArmorPickup>(ArmorPickup::Name).lock();
					PrintLogGuessInformation("armor " + eastl::to_string(pArmorPickup->GetCode()) + " ");
				}
				else if (pItemActor->GetType() == "Health")
				{
					eastl::shared_ptr<HealthPickup> pHealthPickup =
						pItemActor->GetComponent<HealthPickup>(HealthPickup::Name).lock();
					PrintLogGuessInformation("health " + eastl::to_string(pHealthPickup->GetCode()) + " ");
				}
			}

			GetPlayerGuessPlan(pOtherPlayerActor->GetId(), otherPlayerGuessPlan);

			info = "\n new guess plan ";
			PrintLogGuessInformation(info);
			if (otherPlayerGuessPlan.node)
			{
				PrintLogGuessInformation("\n other player position : " +
					eastl::to_string(otherPlayerGuessPlan.node->GetPos()[0]) + " " +
					eastl::to_string(otherPlayerGuessPlan.node->GetPos()[1]) + " " +
					eastl::to_string(otherPlayerGuessPlan.node->GetPos()[2]) + " ");
				PrintLogGuessInformation("\n other player plan cluster : " +
					eastl::to_string(otherPlayerGuessPlan.node->GetCluster()) + " ");
			}
			PrintLogGuessInformation("\n other player path id " + eastl::to_string(otherPlayerGuessPlan.id) + " : ");
			for (PathingArc* pathArc : otherPlayerGuessPlan.path)
				PrintLogGuessInformation(eastl::to_string(pathArc->GetNode()->GetId()) + " ");

			GetPlayerGuessItems(pOtherPlayerActor->GetId(), guessItems);
			if (!guessItems.empty())
			{
				info = "\n new guess items to exclude : ";
				PrintLogGuessInformation(info);
			}
			for (auto guessItem : guessItems)
			{
				eastl::shared_ptr<Actor> pItemActor(GameLogic::Get()->GetActor(guessItem.first).lock());

				if (pItemActor->GetType() == "Weapon")
				{
					eastl::shared_ptr<WeaponPickup> pWeaponPickup =
						pItemActor->GetComponent<WeaponPickup>(WeaponPickup::Name).lock();

					info = "weapon " + eastl::to_string(pWeaponPickup->GetCode()) + " ";
					PrintLogGuessInformation(info);
				}
				else if (pItemActor->GetType() == "Ammo")
				{
					eastl::shared_ptr<AmmoPickup> pAmmoPickup =
						pItemActor->GetComponent<AmmoPickup>(AmmoPickup::Name).lock();

					info = "ammo " + eastl::to_string(pAmmoPickup->GetCode()) + " ";
					PrintLogGuessInformation(info);
				}
				else if (pItemActor->GetType() == "Armor")
				{
					eastl::shared_ptr<ArmorPickup> pArmorPickup =
						pItemActor->GetComponent<ArmorPickup>(ArmorPickup::Name).lock();

					info = "armor " + eastl::to_string(pArmorPickup->GetCode()) + " ";
					PrintLogGuessInformation(info);
				}
				else if (pItemActor->GetType() == "Health")
				{
					eastl::shared_ptr<HealthPickup> pHealthPickup =
						pItemActor->GetComponent<HealthPickup>(HealthPickup::Name).lock();

					info = "health " + eastl::to_string(pHealthPickup->GetCode()) + " ";
					PrintLogGuessInformation(info);
				}
			}
		}
	}
}

//map generation via physics simulation
void QuakeAIManager::CreateMap(ActorId playerId)
{
	GameApplication* gameApp = (GameApplication*)Application::App;
	QuakeLogic* game = static_cast<QuakeLogic *>(GameLogic::Get());
	Level* level = game->GetLevelManager()->GetLevel(
		ToWideString(gameApp->mOption.mLevel.c_str()));

	mPlayerActor = eastl::dynamic_shared_pointer_cast<PlayerActor>(
		GameLogic::Get()->GetActor(playerId).lock());

	game->RemoveAllDelegates();

	/*
	mPathingGraph = eastl::make_shared<PathingGraph>();

	eastl::vector<eastl::shared_ptr<Actor>> actors;
	game->GetAmmoActors(actors);
	game->GetWeaponActors(actors);
	game->GetHealthActors(actors);
	game->GetArmorActors(actors);
	game->GetTargetActors(actors);
	for (eastl::shared_ptr<Actor> actor : actors)
	{
		eastl::shared_ptr<TransformComponent> pTransformComponent(
			actor->GetComponent<TransformComponent>(TransformComponent::Name).lock());
		if (pTransformComponent)
			SimulateActorPosition(actor->GetId(), pTransformComponent->GetPosition());
	}

	// we create the waypoint according to the character controller physics system. Every
	// simulation step, it will be generated new waypoints from different actions such as
	// movement, jumping or falling and its conections
	SimulateWaypoint();

	// we obtain visibility information from pathing graph 
	SimulateVisibility();
	*/
	// we group the graph nodes in clusters
	CreateClusters();

	eastl::wstring levelPath = L"ai/quake/" + level->GetName() + L".bin";
	GameLogic::Get()->GetAIManager()->SavePathingGraph(
		FileSystem::Get()->GetPath(ToString(levelPath.c_str())));

	game->RegisterAllDelegates();
}

void QuakeAIManager::SimulateVisibility()
{
	eastl::shared_ptr<BaseGamePhysic> gamePhysics = GameLogic::Get()->GetGamePhysics();

	Transform transform;

	// each arc in the graph has a set of transition nodes that we can�t realistically process for 
	// visibility since there are hundred of millions of pair transition combinations depending 
	// on the size of the map which will take forever to simulate visibility. Thats why we have to 
	// make an aproximation by associating every transition position to its neareast node

	// first we get visibility info from every node by raycasting
	for (PathingNode* pathNode : mPathingGraph->GetNodes())
	{
		//set muzzle location relative to pivoting eye
		Vector3<float> muzzle = pathNode->GetPos();
		muzzle[2] += mPlayerActor->GetState().viewHeight;
		muzzle -= Vector3<float>::Unit(ROLL) * 11.f;

		for (PathingNode* visibleNode : mPathingGraph->GetNodes())
		{
			Vector3<float> end = visibleNode->GetPos() +
				(float)mPlayerActor->GetState().viewHeight * Vector3<float>::Unit(YAW);

			eastl::vector<ActorId> collisionActors;
			eastl::vector<Vector3<float>> collisions, collisionNormals;
			gamePhysics->CastRay(muzzle, end, collisionActors, collisions, collisionNormals);

			Vector3<float> collision = NULL;
			for (unsigned int i = 0; i < collisionActors.size(); i++)
				if (collisionActors[i] == INVALID_ACTOR_ID)
					collision = collisions[i];

			if (collision == NULL)
				pathNode->AddVisibleNode(visibleNode, Length(visibleNode->GetPos() - pathNode->GetPos()));
		}
	}

	//next we recreate each transition to its nearest node position
	eastl::map<Vector3<float>, PathingNode*> pathingNodes;
	for (PathingNode* pathNode : mPathingGraph->GetNodes())
	{
		pathingNodes[pathNode->GetPos()] = pathNode;
		for (PathingTransition* pathTransition : pathNode->GetTransitions())
		{
			for (Vector3<float> pathConnection : pathTransition->GetConnections())
				pathingNodes[pathConnection] = mPathingGraph->FindClosestNode(pathConnection);
		}
	}

	for (PathingNode* pathNode : mPathingGraph->GetNodes())
	{
		for (PathingArc* pathArc : pathNode->GetArcs())
		{
			PathingTransition* pathTransition = pathNode->FindTransition(pathArc->GetId());
			if (pathTransition)
			{
				PathingNodeVec nodes;
				eastl::map<PathingNode*, float> weights;
				eastl::map<PathingNode*, Vector3<float>> positions;
				for (unsigned int idx = 0; idx < pathTransition->GetConnections().size(); idx++)
				{
					PathingNode* pathConnection = pathingNodes[pathTransition->GetConnections()[idx]];
					if (eastl::find(nodes.begin(), nodes.end(), pathConnection) == nodes.end())
					{
						weights[pathConnection] = 0;
						nodes.push_back(pathConnection);
					}

					weights[pathConnection] += pathTransition->GetWeights()[idx];
					positions[pathConnection] = pathTransition->GetConnections()[idx];
				}

				eastl::vector<float> nodeWeights;
				eastl::vector<Vector3<float>> nodePositions;
				for (PathingNode* node : nodes)
				{
					nodeWeights.push_back(weights[node]);
					nodePositions.push_back(positions[node]);
				}

				pathNode->RemoveTransition(pathTransition->GetId());
				pathNode->AddTransition(new PathingTransition(
					pathArc->GetId(), pathArc->GetType(), nodes, nodeWeights, nodePositions));
			}
			else
			{
				pathNode->AddTransition(new PathingTransition(pathArc->GetId(), pathArc->GetType(), 
					{ pathArc->GetNode() }, { pathArc->GetWeight() }, { pathArc->GetNode()->GetPos() }));
			}
		}
	}
}

void QuakeAIManager::SimulateWaypoint()
{
	eastl::shared_ptr<BaseGamePhysic> gamePhysics = GameLogic::Get()->GetGamePhysics();

	RegisterAllDelegates();

	mActorNodes.clear();
	while (!mOpenSet.empty())
	{
		// grab the candidate
		PathingNode* pNode = mOpenSet.front();
		SimulateMovement(pNode);

		// we have processed this node so remove it from the open set
		mClosedSet.push_back(pNode);
		mOpenSet.erase(mOpenSet.begin());
	}

	RemoveAllDelegates();

	// process the item actors which we have met
	eastl::map<PathingNode*, ActorId>::iterator itActorNode;
	for (itActorNode = mActorNodes.begin(); itActorNode != mActorNodes.end(); itActorNode++)
	{
		eastl::shared_ptr<Actor> pItemActor(
			GameLogic::Get()->GetActor(itActorNode->second).lock());
		PathingNode* pNode = itActorNode->first;
		if (pNode != NULL)
		{
			pNode->RemoveTransitions();
			pNode->RemoveArcs();

			if (pItemActor->GetComponent<PushTrigger>(PushTrigger::Name).lock())
			{
				eastl::shared_ptr<PushTrigger> pPushTrigger =
					pItemActor->GetComponent<PushTrigger>(PushTrigger::Name).lock();

				Vector3<float> targetPosition = pPushTrigger->GetTarget().GetTranslation();
				SimulateTriggerPush(pNode, targetPosition);
			}
			else if (pItemActor->GetComponent<TeleporterTrigger>(TeleporterTrigger::Name).lock())
			{
				eastl::shared_ptr<TeleporterTrigger> pTeleporterTrigger =
					pItemActor->GetComponent<TeleporterTrigger>(TeleporterTrigger::Name).lock();

				Vector3<float> targetPosition = pTeleporterTrigger->GetTarget().GetTranslation();
				SimulateTriggerTeleport(pNode, targetPosition);
			}
		}
	}

	while (!mClosedSet.empty())
	{
		// grab the candidate
		PathingNode* pNode = mClosedSet.front();

		// if the node is a trigger we don't simulate it
		if (mActorNodes.find(pNode) == mActorNodes.end())
			SimulateJump(pNode);

		// we have processed this node so remove it from the closed set
		mClosedSet.erase(mClosedSet.begin());
	}

	mActorNodes.clear();
}

void QuakeAIManager::CreateClusters()
{
	eastl::vector<Point> points;
	for (PathingNode* pathNode : mPathingGraph->GetNodes())
	{
		eastl::vector<float> pos{
			pathNode->GetPos()[0], pathNode->GetPos()[1], pathNode->GetPos()[2] };
		Point point(pathNode->GetId(), pos);
		points.push_back(point);
	}

	//Running K-Means Clustering
	unsigned int iters = 100;
	KMeans kmeans(200, iters);
	kmeans.Run(points);

	for (Point point : points)
	{
		PathingNode* pathNode = mPathingGraph->FindNode(point.GetId());
		pathNode->SetCluster(point.GetCluster());
	}

	eastl::vector<unsigned short> searchClusters;
	for (Clustering kCluster : kmeans.GetClusters())
		searchClusters.push_back(kCluster.GetId());

	for (PathingNode* pathNode : mPathingGraph->GetNodes())
	{
		ClusterPlanMap clusterPlans;

		//add cluster transitions with jumps and moves
		mPathingGraph->FindPlans(pathNode, searchClusters, clusterPlans);
		for (auto clusterPlan : clusterPlans)
		{
			PathingArcVec pathArcs = clusterPlan.second->GetArcs();
			if (pathArcs.size())
			{
				PathingNode* pathTarget = pathArcs.back()->GetNode();

				if (pathTarget != pathNode)
				{
					PathingNode* currentNode = pathNode;
					for (PathingArc* pArc : pathArcs)
					{
						bool addCluster = true;
						PathingClusterVec clusters;
						currentNode->GetClusters(GAT_JUMP, clusters);
						for (PathingCluster* cluster : clusters)
						{
							if (cluster->GetTarget() == pathTarget)
							{
								addCluster = false;
								break;
							}
						}

						if (addCluster)
						{
							PathingCluster* pathCluster = 
								new PathingCluster(GAT_JUMP, INVALID_ACTOR_ID);
							pathCluster->LinkClusters(pArc->GetNode(), pathTarget);
							mPathingGraph->InsertCluster(pathCluster);

							currentNode->AddCluster(pathCluster);
						}
						else break;

						currentNode = pArc->GetNode();
					}
				}
			}
			delete clusterPlan.second;
		}
		clusterPlans.clear();

		//add cluster transitions only with moves
		mPathingGraph->FindPlans(pathNode, searchClusters, clusterPlans, GAT_JUMP);
		for (auto clusterPlan : clusterPlans)
		{
			PathingArcVec pathArcs = clusterPlan.second->GetArcs();
			if (pathArcs.size())
			{
				PathingNode* pathTarget = pathArcs.back()->GetNode();
				if (pathTarget != pathNode)
				{
					PathingNode* currentNode = pathNode;
					for (PathingArc* pArc : pathArcs)
					{
						bool addCluster = true;
						PathingClusterVec clusters;
						currentNode->GetClusters(GAT_MOVE, clusters);
						for (PathingCluster* cluster : clusters)
						{
							if (cluster->GetTarget() == pathTarget)
							{
								addCluster = false;
								break;
							}
						}

						if (addCluster)
						{
							PathingCluster* pathCluster =
								new PathingCluster(GAT_MOVE, INVALID_ACTOR_ID);
							pathCluster->LinkClusters(pArc->GetNode(), pathTarget);
							mPathingGraph->InsertCluster(pathCluster);

							currentNode->AddCluster(pathCluster);
						}
						else break;

						currentNode = pArc->GetNode();
					}
				}
			}

			delete clusterPlan.second;
		}
	}

	GameApplication* gameApp = (GameApplication*)Application::App;
	QuakeLogic* game = static_cast<QuakeLogic *>(GameLogic::Get());

	eastl::vector<eastl::shared_ptr<Actor>> searchActors;
	game->GetAmmoActors(searchActors);
	game->GetWeaponActors(searchActors);
	game->GetHealthActors(searchActors);
	game->GetArmorActors(searchActors);
	for (PathingNode* pathNode : mPathingGraph->GetNodes())
	{
		ActorPlanMap actorPlans;

		//add cluster transitions with jumps and moves
		mPathingGraph->FindPlans(pathNode, searchActors, actorPlans, -1, 1.0f);
		for (auto actorPlan : actorPlans)
		{
			PathingArcVec pathArcs = actorPlan.second->GetArcs();
			if (pathArcs.size())
			{
				PathingNode* pathTarget = pathArcs.back()->GetNode();
				if (pathTarget != pathNode)
				{
					PathingNode* currentNode = pathNode;
					for (PathingArc* pArc : pathArcs)
					{
						bool addCluster = true;
						PathingClusterVec clusters;
						currentNode->GetClusterActors(GAT_JUMP, clusters);
						for (PathingCluster* cluster : clusters)
						{
							if (cluster->GetTarget() == pathTarget)
							{
								addCluster = false;
								break;
							}
						}

						if (addCluster)
						{
							PathingCluster* pathCluster =
								new PathingCluster(GAT_JUMP, pathTarget->GetActorId());
							pathCluster->LinkClusters(pArc->GetNode(), pathTarget);
							mPathingGraph->InsertCluster(pathCluster);

							currentNode->AddCluster(pathCluster);
							currentNode->AddClusterActor(pathCluster);
						}
						else break;

						currentNode = pArc->GetNode();
					}
				}
			}

			delete actorPlan.second;
		}

		actorPlans.clear();

		//add cluster transitions only with moves
		mPathingGraph->FindPlans(pathNode, searchActors, actorPlans, GAT_JUMP, 1.0f);
		for (auto actorPlan : actorPlans)
		{
			PathingArcVec pathArcs = actorPlan.second->GetArcs();
			if (pathArcs.size())
			{
				PathingNode* pathTarget = pathArcs.back()->GetNode();
				if (pathTarget != pathNode)
				{
					PathingNode* currentNode = pathNode;
					for (PathingArc* pArc : pathArcs)
					{
						bool addCluster = true;
						PathingClusterVec clusters;
						currentNode->GetClusterActors(GAT_MOVE, clusters);
						for (PathingCluster* cluster : clusters)
						{
							if (cluster->GetTarget() == pathTarget)
							{
								addCluster = false;
								break;
							}
						}

						if (addCluster)
						{
							PathingCluster* pathCluster =
								new PathingCluster(GAT_MOVE, pathTarget->GetActorId());
							pathCluster->LinkClusters(pArc->GetNode(), pathTarget);
							mPathingGraph->InsertCluster(pathCluster);

							currentNode->AddCluster(pathCluster);
							currentNode->AddClusterActor(pathCluster);
						}
						else break;

						currentNode = pArc->GetNode();
					}
				}
			}

			delete actorPlan.second;
		}
		actorPlans.clear();
	}

	for (PathingNode* pathNode : mPathingGraph->GetNodes())
		pathNode->OrderClusters();
}

void QuakeAIManager::SimulateActorPosition(ActorId actorId, const Vector3<float>& position)
{
	eastl::shared_ptr<BaseGamePhysic> gamePhysics = GameLogic::Get()->GetGamePhysics();

	Transform transform;
	transform.SetTranslation(position);
	gamePhysics->SetTransform(mPlayerActor->GetId(), transform);
	gamePhysics->OnUpdate(0.02f);

	transform = gamePhysics->GetTransform(mPlayerActor->GetId());
	PathingNode* pNewNode = new PathingNode(
		GetNewNodeID(), actorId, transform.GetTranslation());
	mPathingGraph->InsertNode(pNewNode);
	mOpenSet.push_back(pNewNode);
}

void QuakeAIManager::SimulateTriggerTeleport(PathingNode* pNode, const Vector3<float>& target)
{
	eastl::shared_ptr<BaseGamePhysic> gamePhysics = GameLogic::Get()->GetGamePhysics();

	Vector3<float> direction = Vector3<float>::Unit(YAW); //up vector

	Transform transform;
	transform.SetTranslation(target);
	gamePhysics->SetTransform(mPlayerActor->GetId(), transform);
	gamePhysics->OnUpdate(0.02f);

	// gravity falling simulation
	transform = gamePhysics->GetTransform(mPlayerActor->GetId());

	float totalTime = 0.f, fallSpeed = 0.f;
	eastl::vector<Vector3<float>> nodePositions;
	while (!gamePhysics->OnGround(mPlayerActor->GetId()) && totalTime <= 10.f)
	{
		nodePositions.push_back(transform.GetTranslation());

		float jumpSpeed = gamePhysics->GetJumpSpeed(mPlayerActor->GetId());

		totalTime += 0.02f;
		fallSpeed += (20.f / (jumpSpeed * 0.5f));
		if (fallSpeed > mMaxFallSpeed) fallSpeed = mMaxFallSpeed;

		Normalize(direction);
		direction[PITCH] *= jumpSpeed * (fallSpeed / 4.f);
		direction[ROLL] *= jumpSpeed * (fallSpeed / 4.f);
		direction[YAW] = -jumpSpeed * fallSpeed;

		gamePhysics->FallDirection(mPlayerActor->GetId(), direction);
		gamePhysics->OnUpdate(0.02f);

		transform = gamePhysics->GetTransform(mPlayerActor->GetId());
	}

	if (totalTime >= 10.f) return;
	totalTime += 0.02f;

	Vector3<float> position = transform.GetTranslation();
	PathingNode* pEndNode = mPathingGraph->FindClosestNode(position);
	if (pNode != pEndNode)
	{
		if (pEndNode != NULL && pNode->FindArc(GAT_TELEPORT, pEndNode) == NULL)
		{
			if (pEndNode->GetActorId() == INVALID_ACTOR_ID ||
				pNode->GetActorId() != pEndNode->GetActorId())
			{
				PathingArc* pArc = new PathingArc(GetNewArcID(), GAT_TELEPORT, pEndNode, totalTime);
				pNode->AddArc(pArc);

				//lets interpolate transitions from the already created arc
				float deltaTime = 0.f;
				Vector3<float> position = pNode->GetPos();

				eastl::vector<float> weights;
				eastl::vector<PathingNode*> nodes;
				eastl::vector<Vector3<float>> positions;
				for (unsigned int idx = 0; idx < nodePositions.size(); idx++)
				{
					deltaTime += 0.02f;

					if (Length(nodePositions[idx] - position) >= 16.f)
					{
						nodes.push_back(pNode);
						weights.push_back(deltaTime);
						positions.push_back(nodePositions[idx]);

						deltaTime = 0.f;
					}
				}

				if (!nodes.empty())
				{
					PathingTransition* pTransition = new PathingTransition(
						pArc->GetId(), GAT_TELEPORT, nodes, weights, positions);
					pNode->AddTransition(pTransition);
				}
			}
		}
	}
}

void QuakeAIManager::SimulateTriggerPush(PathingNode* pNode, const Vector3<float>& target)
{
	eastl::shared_ptr<BaseGamePhysic> gamePhysics = GameLogic::Get()->GetGamePhysics();

	Vector3<float> direction = target - pNode->GetPos();
	float push = Length(target - pNode->GetPos());
	Normalize(direction);

	direction[PITCH] *= push / 90.f;
	direction[ROLL] *= push / 90.f;
	direction[YAW] = push / 30.f;

	Transform transform;
	transform.SetTranslation(pNode->GetPos());
	gamePhysics->SetTransform(mPlayerActor->GetId(), transform);
	gamePhysics->WalkDirection(mPlayerActor->GetId(), direction);
	gamePhysics->Jump(mPlayerActor->GetId(), direction);
	gamePhysics->OnUpdate(0.02f);

	// gravity falling simulation
	transform = gamePhysics->GetTransform(mPlayerActor->GetId());

	float totalTime = 0.f, fallSpeed = 0.f;
	eastl::vector<Vector3<float>> nodePositions;
	while (!gamePhysics->OnGround(mPlayerActor->GetId()) && totalTime <= 10.f)
	{
		nodePositions.push_back(transform.GetTranslation());

		float jumpSpeed = gamePhysics->GetJumpSpeed(mPlayerActor->GetId());

		totalTime += 0.02f;
		fallSpeed += (20.f / (jumpSpeed * 0.5f));
		if (fallSpeed > mMaxFallSpeed) fallSpeed = mMaxFallSpeed;

		Normalize(direction);
		direction[PITCH] *= jumpSpeed * (fallSpeed / 4.f);
		direction[ROLL] *= jumpSpeed * (fallSpeed / 4.f);
		direction[YAW] = -jumpSpeed * fallSpeed;

		gamePhysics->FallDirection(mPlayerActor->GetId(), direction);
		gamePhysics->OnUpdate(0.02f);

		transform = gamePhysics->GetTransform(mPlayerActor->GetId());
	}

	if (totalTime >= 10.f) return;
	totalTime += 0.02f;

	//we store the jump if we find a landing node
	Vector3<float> position = transform.GetTranslation();
	PathingNode* pEndNode = mPathingGraph->FindClosestNode(position);
	if (pNode != pEndNode)
	{
		if (pEndNode != NULL && pNode->FindArc(GAT_PUSH, pEndNode) == NULL)
		{
			if (pEndNode->GetActorId() == INVALID_ACTOR_ID ||
				pNode->GetActorId() != pEndNode->GetActorId())
			{
				PathingArc* pArc = new PathingArc(GetNewArcID(), GAT_PUSH, pEndNode, totalTime);
				pNode->AddArc(pArc);

				//lets interpolate transitions from the already created arc
				float deltaTime = 0.f;
				Vector3<float> position = pNode->GetPos();

				eastl::vector<float> weights;
				eastl::vector<PathingNode*> nodes;
				eastl::vector<Vector3<float>> positions;
				for (unsigned int idx = 0; idx < nodePositions.size(); idx++)
				{
					deltaTime += 0.02f;

					if (Length(nodePositions[idx] - position) >= 16.f)
					{
						nodes.push_back(pNode);
						weights.push_back(deltaTime);
						positions.push_back(nodePositions[idx]);

						deltaTime = 0.f;
					}
				}

				if (!nodes.empty())
				{
					PathingTransition* pTransition = new PathingTransition(
						pArc->GetId(), GAT_PUSH, nodes, weights, positions);
					pNode->AddTransition(pTransition);
				}
			}
		}
	}
}

float FindClosestMovement(eastl::vector<Vector3<float>>& movements, const Vector3<float>& pos)
{
	float length = FLT_MAX;
	eastl::vector<Vector3<float>>::iterator it;
	for (it = movements.begin(); it != movements.end(); it++)
	{
		Vector3<float> diff = pos - (*it);
		if (Length(diff) < length)
			length = Length(diff);
	}

	return length;
}


// Cliff control
bool Cliff(const Vector3<float>& translation)
{
	for (int angle = 0; angle < 360; angle += 5)
	{
		Matrix4x4<float> rotation = Rotation<4, float>(
			AxisAngle<4, float>(Vector4<float>::Unit(YAW), angle * (float)GE_C_DEG_TO_RAD));

		// This will give us the "look at" vector 
		// in world space - we'll use that to move.
		Vector4<float> atWorld = Vector4<float>::Unit(PITCH); // forward vector
#if defined(GE_USE_MAT_VEC)
		atWorld = rotation * atWorld;
#else
		atWorld = atWorld * rotation;
#endif

		Vector3<float> position = translation + HProject(atWorld * 10.f);

		Transform start;
		start.SetRotation(rotation);
		start.SetTranslation(position);

		Transform end;
		end.SetRotation(rotation);
		end.SetTranslation(position - Vector3<float>::Unit(YAW) * 300.f);

		Vector3<float> collision, collisionNormal;
		collision = end.GetTranslation();
		ActorId actorId = GameLogic::Get()->GetGamePhysics()->CastRay(
			start.GetTranslation(), end.GetTranslation(), collision, collisionNormal);

		//Check whether we are close to a cliff
		//printf("weight cliff %f \n", abs(collision[2] - position[2]));
		if (abs(collision[2] - position[2]) > 60.f)
			return true;
	}
	return false;
}

void QuakeAIManager::SimulateMovement(PathingNode* pNode)
{
	eastl::shared_ptr<BaseGamePhysic> gamePhysics = GameLogic::Get()->GetGamePhysics();

	Transform transform;
	transform.SetTranslation(pNode->GetPos());
	gamePhysics->SetTransform(mPlayerActor->GetId(), transform);
	gamePhysics->SetVelocity(mPlayerActor->GetId(), Vector3<float>::Zero());

	// nodes closed to falling position
	for (int angle = 0; angle < 360; angle += 5)
	{
		Matrix4x4<float> rotation = Rotation<4, float>(
			AxisAngle<4, float>(Vector4<float>::Unit(YAW), angle * (float)GE_C_DEG_TO_RAD));

		transform.SetTranslation(pNode->GetPos());
		transform.SetRotation(rotation);

		//create movements on the ground
		eastl::vector<Vector3<float>> movements;
		eastl::map<Vector3<float>, bool> groundMovements;

		PathingNode* pCurrentNode = pNode;
		Vector3<float> position = pNode->GetPos();

		do
		{
			if (!gamePhysics->OnGround(mPlayerActor->GetId()))
			{
				float totalTime = 0.f;
				float fallSpeed = 0.f;
				do
				{
					groundMovements[position] = false;
					movements.push_back(position);

					float jumpSpeed = gamePhysics->GetJumpSpeed(mPlayerActor->GetId());

					totalTime += 0.02f;
					fallSpeed += (20.f / (jumpSpeed * 0.5f));
					if (fallSpeed > mMaxFallSpeed) fallSpeed = mMaxFallSpeed;

					Vector3<float> direction; // forward vector
#if defined(GE_USE_MAT_VEC)
					direction = HProject(rotation * Vector4<float>::Unit(PITCH));
#else
					direction = HProject(Vector4<float>::Unit(PITCH) * rotation);
#endif
					direction[PITCH] *= jumpSpeed * (fallSpeed / 4.f);
					direction[ROLL] *= jumpSpeed * (fallSpeed / 4.f);
					direction[YAW] = -jumpSpeed * fallSpeed;

					gamePhysics->FallDirection(mPlayerActor->GetId(), direction);
					gamePhysics->OnUpdate(0.02f);

					transform = gamePhysics->GetTransform(mPlayerActor->GetId());
					position = transform.GetTranslation();
				} while (!gamePhysics->OnGround(mPlayerActor->GetId()) && totalTime <= 10.f);

				if (totalTime >= 10.f)
					break;
			}

			groundMovements[position] = true;
			movements.push_back(position);

			PathingNode* pClosestNode = mPathingGraph->FindClosestNode(position, false);
			if (pCurrentNode != pClosestNode)
			{
				Vector3<float> diff = pClosestNode->GetPos() - position;
				if (Length(diff) <= PATHING_DEFAULT_NODE_TOLERANCE)
				{
					//if we find any node close to our current position we stop
					break;
				}
			}

			Vector3<float> direction; // forward vector
#if defined(GE_USE_MAT_VEC)
			direction = HProject(rotation * Vector4<float>::Unit(PITCH));
#else
			direction = HProject(Vector4<float>::Unit(PITCH) * rotation);
#endif

			gamePhysics->SetTransform(mPlayerActor->GetId(), transform);
			gamePhysics->WalkDirection(mPlayerActor->GetId(), direction * mMoveSpeed);
			gamePhysics->OnUpdate(0.02f);

			transform = gamePhysics->GetTransform(mPlayerActor->GetId());
			position = transform.GetTranslation();

		} while (FindClosestMovement(movements, position) >= 4.f); // stalling is a break condition

		if (!movements.empty())
		{
			float deltaTime = 0.f, totalTime = 0.f;
			eastl::vector<Vector3<float>>::iterator itMove = movements.begin();
			eastl::vector<Vector3<float>> positions{ (*itMove) };
			eastl::vector<float> times{ 0 };
			itMove++;
			bool onGround = true;
			for (; itMove != movements.end(); itMove++)
			{
				totalTime += 0.02f;
				deltaTime += 0.02f;
				if (Length((*itMove) - positions.back()) >= 16.f)
				{
					times.push_back(deltaTime);
					positions.push_back((*itMove));

					deltaTime = 0.f;
				}

				if (groundMovements[(*itMove)])
				{
					PathingNode* pClosestNode = mPathingGraph->FindClosestNode((*itMove), false);
					Vector3<float> diff = pClosestNode->GetPos() - (*itMove);
					if (Length(diff) >= 16.f)
					{
						if (!Cliff(*itMove))
						{
							Vector3<float> scale = gamePhysics->GetScale(mPlayerActor->GetId()) / 2.f;

							Transform start;
							start.SetTranslation(pCurrentNode->GetPos() + scale[YAW] * Vector3<float>::Unit(YAW));
							Transform end;
							end.SetTranslation((*itMove) + scale[YAW] * Vector3<float>::Unit(YAW));

							Vector3<float> collision, collisionNormal;
							ActorId actorId = gamePhysics->ConvexSweep(
								mPlayerActor->GetId(), start, end, collision, collisionNormal);
							if (collision == NULL || actorId != INVALID_ACTOR_ID)
							{
								unsigned int arcType = onGround ? GAT_MOVE : GAT_FALL;
								PathingNode* pNewNode = new PathingNode(GetNewNodeID(), INVALID_ACTOR_ID, (*itMove));
								PathingArc* pArc = new PathingArc(GetNewArcID(), arcType, pNewNode, totalTime);
								pCurrentNode->AddArc(pArc);

								//lets interpolate transitions from the already created arc
								if (!positions.empty())
								{
									PathingTransition* pTransition = new PathingTransition(
										pArc->GetId(), arcType, { pCurrentNode }, times, positions);
									pCurrentNode->AddTransition(pTransition);
								}

								mPathingGraph->InsertNode(pNewNode);
								mOpenSet.push_back(pNewNode);
								pCurrentNode = pNewNode;

								deltaTime = 0.f;
								totalTime = 0.f;

								onGround = true;
								positions = { pNewNode->GetPos() };
								times = { 0 };
							}
							else
							{
								break; //we stop processing movements if we find collision
							}
						}
						else
						{
							break; //we stop processing movements if any point don't pass the cliff control
						}
					}
					else if (pCurrentNode != pClosestNode)
					{
						if (Length(diff) <= PATHING_DEFAULT_NODE_TOLERANCE)
						{
							unsigned int arcType = onGround ? GAT_MOVE : GAT_FALL;
							if (pCurrentNode->FindArc(arcType, pClosestNode) == NULL)
							{
								Vector3<float> scale = gamePhysics->GetScale(mPlayerActor->GetId()) / 2.f;

								Transform start;
								start.SetTranslation(pCurrentNode->GetPos() + scale[YAW] * Vector3<float>::Unit(YAW));
								Transform end;
								end.SetTranslation(pClosestNode->GetPos() + scale[YAW] * Vector3<float>::Unit(YAW));

								Vector3<float> collision, collisionNormal;
								ActorId actorId = gamePhysics->ConvexSweep(
									mPlayerActor->GetId(), start, end, collision, collisionNormal);
								if (collision == NULL || actorId != INVALID_ACTOR_ID)
								{
									PathingArc* pArc = new PathingArc(GetNewArcID(), arcType, pClosestNode, totalTime);
									pCurrentNode->AddArc(pArc);

									//lets interpolate transitions from the already created arc
									if (!positions.empty())
									{
										PathingTransition* pTransition = new PathingTransition(
											pArc->GetId(), arcType, { pCurrentNode }, times, positions);
										pCurrentNode->AddTransition(pTransition);
									}
								}
								else
								{
									break; //we stop processing movements if we find collision
								}
							}
							pCurrentNode = pClosestNode;

							deltaTime = 0.f;
							totalTime = 0.f;

							onGround = true;
							positions = { pClosestNode->GetPos() };
							times = { 0 };
						}
					}
				}
				else onGround = false;
			}
		}
	}
}

void QuakeAIManager::SimulateJump(PathingNode* pNode)
{
	eastl::shared_ptr<BaseGamePhysic> gamePhysics = GameLogic::Get()->GetGamePhysics();

	Transform transform;
	Vector3<float> direction;
	for (int angle = 0; angle < 360; angle += 5)
	{
		Matrix4x4<float> rotation = Rotation<4, float>(
			AxisAngle<4, float>(Vector4<float>::Unit(YAW), angle * (float)GE_C_DEG_TO_RAD));

		// forward vector
#if defined(GE_USE_MAT_VEC)
		direction = HProject(rotation * Vector4<float>::Unit(PITCH));
#else
		direction = HProject(Vector4<float>::Unit(PITCH) * rotation);
#endif
		direction[PITCH] *= mJumpMoveSpeed;
		direction[ROLL] *= mJumpMoveSpeed;
		direction[YAW] = mJumpSpeed;

		transform.SetTranslation(pNode->GetPos());
		transform.SetRotation(rotation);
		gamePhysics->SetTransform(mPlayerActor->GetId(), transform);
		gamePhysics->WalkDirection(mPlayerActor->GetId(), direction);
		gamePhysics->Jump(mPlayerActor->GetId(), direction);
		gamePhysics->OnUpdate(0.02f);

		transform = gamePhysics->GetTransform(mPlayerActor->GetId());

		float fallSpeed = 0.f, totalTime = 0.f;
		PathingNode* pFallingNode = pNode;

		// gravity falling simulation
		eastl::vector<Vector3<float>> nodePositions;
		while (!gamePhysics->OnGround(mPlayerActor->GetId()) && totalTime <= 10.f)
		{
			nodePositions.push_back(transform.GetTranslation());
			float jumpSpeed = gamePhysics->GetJumpSpeed(mPlayerActor->GetId());

			totalTime += 0.02f;
			fallSpeed += (20.f / (jumpSpeed * 0.5f));
			if (fallSpeed > mMaxFallSpeed) fallSpeed = mMaxFallSpeed;

			// forward vector
#if defined(GE_USE_MAT_VEC)
			direction = HProject(rotation * Vector4<float>::Unit(PITCH));
#else
			direction = HProject(Vector4<float>::Unit(PITCH) * rotation);
#endif
			direction[PITCH] *= jumpSpeed * (fallSpeed / 4.f);
			direction[ROLL] *= jumpSpeed * (fallSpeed / 4.f);
			direction[YAW] = -jumpSpeed * fallSpeed;

			gamePhysics->FallDirection(mPlayerActor->GetId(), direction);
			gamePhysics->OnUpdate(0.02f);

			transform = gamePhysics->GetTransform(mPlayerActor->GetId());
		}

		if (totalTime > 10.f) continue;
		totalTime += 0.02f;

		//we store the jump if we find a landing node
		Vector3<float> position = transform.GetTranslation();
		PathingNode* pEndNode = mPathingGraph->FindClosestNode(position);
		if (pNode != pEndNode)
		{
			if (pEndNode != NULL && pNode->FindArc(GAT_JUMP, pEndNode) == NULL)
			{
				//check if we have done a clean jump (no collisions)
				if (Length(pEndNode->GetPos() - pNode->GetPos()) >= 300.f ||
					pEndNode->GetPos()[2] - pNode->GetPos()[2] >= 20.f)
				{
					Vector3<float> diff = pEndNode->GetPos() - position;
					if (Length(diff) <= PATHING_DEFAULT_NODE_TOLERANCE)
					{
						PathingArc* pArc = new PathingArc(GetNewArcID(), GAT_JUMP, pEndNode, totalTime);
						pNode->AddArc(pArc);

						//lets interpolate transitions from the already created arc
						float deltaTime = 0.f;
						Vector3<float> position = pNode->GetPos();

						eastl::vector<float> weights;
						eastl::vector<PathingNode*> nodes;
						eastl::vector<Vector3<float>> positions;
						for (unsigned int idx = 0; idx < nodePositions.size(); idx++)
						{
							deltaTime += 0.02f;

							if (Length(nodePositions[idx] - position) >= 16.f)
							{
								nodes.push_back(pNode);
								weights.push_back(deltaTime);
								positions.push_back(nodePositions[idx]);

								deltaTime = 0.f;
							}
						}

						if (!nodes.empty())
						{
							PathingTransition* pTransition = new PathingTransition(
								pArc->GetId(), GAT_JUMP, nodes, weights, positions);
							pNode->AddTransition(pTransition);
						}
					}
				}
			}
		}
	}
}

void QuakeAIManager::PhysicsTriggerEnterDelegate(BaseEventDataPtr pEventData)
{
	eastl::shared_ptr<EventDataPhysTriggerEnter> pCastEventData =
		eastl::static_pointer_cast<EventDataPhysTriggerEnter>(pEventData);

	eastl::shared_ptr<PlayerActor> pPlayerActor(
		eastl::dynamic_shared_pointer_cast<PlayerActor>(
		GameLogic::Get()->GetActor(pCastEventData->GetOtherActor()).lock()));
	if (!pPlayerActor) return;

	eastl::shared_ptr<Actor> pItemActor(
		GameLogic::Get()->GetActor(pCastEventData->GetTriggerId()).lock());

	if (mPlayerActor && mPlayerActor->GetId() == pPlayerActor->GetId())
	{
		eastl::shared_ptr<PhysicComponent> pPhysicComponent(
			mPlayerActor->GetComponent<PhysicComponent>(PhysicComponent::Name).lock());
		if (pPhysicComponent->OnGround())
		{
			if (pItemActor->GetType() == "Weapon" ||
				pItemActor->GetType() == "Ammo" ||
				pItemActor->GetType() == "Armor" ||
				pItemActor->GetType() == "Health")
			{
				Vector3<float> position = pPhysicComponent->GetTransform().GetTranslation();
				PathingNode* pClosestNode = mPathingGraph->FindClosestNode(position, false);
				if (pClosestNode != NULL)
				{
					Vector3<float> diff = pClosestNode->GetPos() - position;
					if (Length(diff) <= PATHING_DEFAULT_NODE_TOLERANCE)
						pClosestNode->SetActorId(pItemActor->GetId());
				}
			}
		}
	}
}


void QuakeAIManager::PhysicsTriggerLeaveDelegate(BaseEventDataPtr pEventData)
{
	eastl::shared_ptr<EventDataPhysTriggerLeave> pCastEventData =
		eastl::static_pointer_cast<EventDataPhysTriggerLeave>(pEventData);
}


void QuakeAIManager::PhysicsCollisionDelegate(BaseEventDataPtr pEventData)
{
	eastl::shared_ptr<EventDataPhysCollision> pCastEventData =
		eastl::static_pointer_cast<EventDataPhysCollision>(pEventData);

	eastl::shared_ptr<Actor> pGameActorA(
		GameLogic::Get()->GetActor(pCastEventData->GetActorA()).lock());
	eastl::shared_ptr<Actor> pGameActorB(
		GameLogic::Get()->GetActor(pCastEventData->GetActorB()).lock());
	if (pGameActorA && pGameActorB)
	{
		eastl::shared_ptr<Actor> pItemActor;
		eastl::shared_ptr<PlayerActor> pPlayerActor;
		if (eastl::dynamic_shared_pointer_cast<PlayerActor>(pGameActorA) &&
			!eastl::dynamic_shared_pointer_cast<PlayerActor>(pGameActorB))
		{
			pPlayerActor = eastl::dynamic_shared_pointer_cast<PlayerActor>(pGameActorA);
			pItemActor = pGameActorB;

		}
		else if (!eastl::dynamic_shared_pointer_cast<PlayerActor>(pGameActorA) &&
			eastl::dynamic_shared_pointer_cast<PlayerActor>(pGameActorB))
		{
			pPlayerActor = eastl::dynamic_shared_pointer_cast<PlayerActor>(pGameActorB);
			pItemActor = pGameActorA;
		}
		else
		{
			return;
		}

		if (mPlayerActor && mPlayerActor->GetId() == pPlayerActor->GetId())
		{
			eastl::shared_ptr<PhysicComponent> pPhysicComponent(
				mPlayerActor->GetComponent<PhysicComponent>(PhysicComponent::Name).lock());
			if (pPhysicComponent->OnGround())
			{
				if (pItemActor->GetType() == "Trigger")
				{
					Vector3<float> position = pPhysicComponent->GetTransform().GetTranslation();
					PathingNode* pClosestNode = mPathingGraph->FindClosestNode(position, false);
					if (pClosestNode != NULL)
					{
						Vector3<float> diff = pClosestNode->GetPos() - position;
						if (Length(diff) <= PATHING_DEFAULT_NODE_TOLERANCE)
						{
							pClosestNode->SetActorId(pItemActor->GetId());
							mActorNodes[pClosestNode] = pItemActor->GetId();
						}
					}
				}
			}
		}
	}
}

void QuakeAIManager::PhysicsSeparationDelegate(BaseEventDataPtr pEventData)
{
	eastl::shared_ptr<EventDataPhysSeparation> pCastEventData =
		eastl::static_pointer_cast<EventDataPhysSeparation>(pEventData);

}

void QuakeAIManager::RegisterAllDelegates(void)
{
	BaseEventManager* pGlobalEventManager = BaseEventManager::Get();
	pGlobalEventManager->AddListener(
		MakeDelegate(this, &QuakeAIManager::PhysicsTriggerEnterDelegate),
		EventDataPhysTriggerEnter::skEventType);
	pGlobalEventManager->AddListener(
		MakeDelegate(this, &QuakeAIManager::PhysicsTriggerLeaveDelegate),
		EventDataPhysTriggerLeave::skEventType);
	pGlobalEventManager->AddListener(
		MakeDelegate(this, &QuakeAIManager::PhysicsCollisionDelegate),
		EventDataPhysCollision::skEventType);
	pGlobalEventManager->AddListener(
		MakeDelegate(this, &QuakeAIManager::PhysicsSeparationDelegate),
		EventDataPhysSeparation::skEventType);
}

void QuakeAIManager::RemoveAllDelegates(void)
{
	BaseEventManager* pGlobalEventManager = BaseEventManager::Get();
	pGlobalEventManager->RemoveListener(
		MakeDelegate(this, &QuakeAIManager::PhysicsTriggerEnterDelegate),
		EventDataPhysTriggerEnter::skEventType);
	pGlobalEventManager->RemoveListener(
		MakeDelegate(this, &QuakeAIManager::PhysicsTriggerLeaveDelegate),
		EventDataPhysTriggerLeave::skEventType);
	pGlobalEventManager->RemoveListener(
		MakeDelegate(this, &QuakeAIManager::PhysicsCollisionDelegate),
		EventDataPhysCollision::skEventType);
	pGlobalEventManager->RemoveListener(
		MakeDelegate(this, &QuakeAIManager::PhysicsSeparationDelegate),
		EventDataPhysSeparation::skEventType);
}
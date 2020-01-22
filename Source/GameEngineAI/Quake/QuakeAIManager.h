//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#ifndef QUAKEAIMANAGER_H
#define QUAKEAIMANAGER_H

#include "QuakeStd.h"

#include "AI/AIManager.h"

#include "Actors/PlayerActor.h"
#include "Core/Event/EventManager.h"

#include "Physic/PhysicEventListener.h"
#include "Mathematic/Algebra/Matrix4x4.h"

#include <mutex>

class AIPlanNode;

typedef eastl::list<AIPlanNode*> AIPlanNodeList;
typedef eastl::vector<AIPlanNode*> AIPlanNodeVector;

enum GameActionType
{
	GAT_MOVE = 0x0000000,
	GAT_PUSH = 0x0000002,
	GAT_TELEPORT = 0x0000006,
	GAT_FALL = 0x000000A,
	GAT_JUMP = 0x00000E
};

//
// struct NodePlan
//
struct NodePlan
{
	NodePlan()
	{
		id = -1;
		node = NULL;
		weight = 0.f;
	}

	NodePlan(PathingNode* playerNode, PathingArcVec& path)
	{
		id = -1;
		node = playerNode;

		weight = 0.f;
		for (PathingArc* pathArc : path)
		{
			weight += pathArc->GetWeight();
			path.push_back(pathArc);
		}
	}

	NodePlan(const NodePlan& plan) :
		id(plan.id), node(plan.node)
	{
		weight = 0.f;
		for (PathingArc* pathArc : plan.path)
		{
			weight += pathArc->GetWeight();
			path.push_back(pathArc);
		}
	}

	~NodePlan()
	{

	}

	void AddPlanPath(const PathingArcVec& planPath)
	{
		weight = 0.f;
		path.clear();
		for (PathingArc* pathArc : planPath)
		{
			weight += pathArc->GetWeight();
			path.push_back(pathArc);
		}
	}

	int id;
	float weight;
	PathingNode* node;
	PathingArcVec path;
};

//
// struct NodeState
//
struct NodeState
{
	NodeState()
	{
		valid = false;
		current = false;
		player = INVALID_ACTOR_ID;

		weapon = WP_NONE;
		weaponTarget = INVALID_ACTOR_ID;
		heuristic = 0.f;
		for (unsigned int i = 0; i < MAX_STATS; i++)
		{
			stats[i] = 0;
		}

		for (unsigned int i = 0; i < MAX_WEAPONS; i++)
		{
			ammo[i] = 0;
			damage[i] = 0;
		}
	}

	NodeState(eastl::shared_ptr<PlayerActor> playerActor)
	{
		valid = true;
		current = false;
		player = playerActor->GetId();

		weapon = WP_NONE;
		weaponTarget = INVALID_ACTOR_ID;
		heuristic = 0.f;
		for (unsigned int i = 0; i < MAX_STATS; i++)
		{
			stats[i] = playerActor->GetState().stats[i];
		}

		for (unsigned int i = 0; i < MAX_WEAPONS; i++)
		{
			ammo[i] = playerActor->GetState().ammo[i];
			damage[i] = 0;
		}
	}

	NodeState(const NodeState& state) : valid(state.valid),
		player(state.player), weaponTarget(state.weaponTarget),
		weapon(state.weapon), heuristic(state.heuristic)
	{
		current = false;

		plan.id = state.plan.id;
		plan.node = state.plan.node;
		plan.AddPlanPath(state.plan.path);

		for (unsigned int i = 0; i < MAX_STATS; i++)
		{
			stats[i] = state.stats[i];
		}

		for (unsigned int i = 0; i < MAX_WEAPONS; i++)
		{
			ammo[i] = state.ammo[i];
			damage[i] = state.damage[i];
		}

		for (eastl::shared_ptr<Actor> item : state.items)
		{
			items.push_back(item);
			itemAmount[item] = state.itemAmount.at(item);
			itemWeight[item] = state.itemWeight.at(item);
		}
	}

	~NodeState()
	{

	}

	void NodeState::Copy(NodeState& state)
	{
		current = false;

		valid = state.valid;
		player = state.player;

		weapon = state.weapon;
		weaponTarget = state.weaponTarget;
		heuristic = state.heuristic;

		plan.id = state.plan.id;
		plan.node = state.plan.node;
		plan.AddPlanPath(state.plan.path);

		for (unsigned int i = 0; i < MAX_STATS; i++)
		{
			stats[i] = state.stats[i];
		}

		for (unsigned int i = 0; i < MAX_WEAPONS; i++)
		{
			ammo[i] = state.ammo[i];
			damage[i] = state.damage[i];
		}

		items.clear();
		itemAmount.clear();
		itemWeight.clear();
		for (eastl::shared_ptr<Actor> item : state.items)
		{
			items.push_back(item);
			itemAmount[item] = state.itemAmount[item];
			itemWeight[item] = state.itemWeight[item];
		}
	}

	void NodeState::CopyItems(NodeState& state)
	{
		items.clear();
		itemAmount.clear();
		itemWeight.clear();
		for (eastl::shared_ptr<Actor> item : state.items)
		{
			items.push_back(item);
			itemAmount[item] = state.itemAmount[item];
			itemWeight[item] = state.itemWeight[item];
		}
	}

	void NodeState::ResetItems()
	{
		items.clear();
		itemAmount.clear();
		itemWeight.clear();
	}

	bool valid;
	bool current;
	ActorId player;

	NodePlan plan;

	float heuristic;
	WeaponType weapon;
	ActorId weaponTarget;

	int stats[MAX_STATS];
	int ammo[MAX_WEAPONS];
	int damage[MAX_WEAPONS];

	eastl::vector<eastl::shared_ptr<Actor>> items;
	eastl::map<eastl::shared_ptr<Actor>, int> itemAmount;
	eastl::map<eastl::shared_ptr<Actor>, float> itemWeight;
};

//--------------------------------------------------------------------------------------------------------
// class AIPlanNode
// This class is a helper used in AIManager::FindPath().
//--------------------------------------------------------------------------------------------------------
class AIPlanNode
{
	AIPlanNode* mPrevNode;  // node we just came from
	PathingNode* mPathingNode;  // pointer to the pathing node from the pathing graph
	PathingCluster* mGoalCluster;  // pointer to the goal cluster
	bool mClosed;  // the node is closed if it's already been processed
	float mWeight;  // weight of the entire path up to this point
	float mHeuristic;  // heuristic of the entire path up to this point

public:
	explicit AIPlanNode(PathingNode* pNode, AIPlanNode* pPrevNode, 
		PathingCluster* pGoalCluster, float weight, float heuristic);
	AIPlanNode* GetPrev(void) const { return mPrevNode; }
	PathingNode* GetPathingNode(void) const { return mPathingNode; }
	PathingCluster* GetGoalCluster(void) const { return mGoalCluster; }
	void GetPlanActors(eastl::map<ActorId, float>& planActors);
	bool IsClosed(void) const { return mClosed; }
	float GetHeuristic(void) const { return mHeuristic; }
	float GetWeight(void) const { return mWeight; }

	void UpdateNode(PathingNode* pNode, AIPlanNode* pPrev,
		PathingCluster* pGoalCluster, float weight, float heuristic);
	void SetClosed(bool toClose = true) { mClosed = toClose; }
	bool IsBetterChoiceThan(AIPlanNode* pRight) { return (mHeuristic > pRight->GetHeuristic()); }
};


//--------------------------------------------------------------------------------------------------------
// class AIFinder
// This class implements the AIFinder algorithm.
//--------------------------------------------------------------------------------------------------------
class AIFinder
{

public:
	AIFinder(void);
	~AIFinder(void);
	void Destroy(void);

	void operator()(NodeState& pNodeState, 
		PathingCluster* pGoalCluster, PathingArcVec& planPath, 
		eastl::map<ActorId, float>& excludeActors, float threshold = FLT_MAX);

protected:

	AIPlanNodeVector mNodes;
	AIPlanNodeList mOpenSet;

	NodeState mNodeState;
	PathingCluster* mGoalCluster;

private:

	AIPlanNode* AddToOpenSet(PathingNode* pNode, AIPlanNode* pPrevNode,
		PathingCluster* pGoalCluster, float weight, float heuristic);
	void AddToClosedSet(AIPlanNode* pNode);
	void InsertNode(AIPlanNode* pNode);
	void RebuildPath(AIPlanNode* pGoalNode, PathingArcVec& planPath);
};

class QuakeAIManager : public AIManager
{
	friend class AIFinder;
	friend class QuakeAIView;
	friend class QuakeAIProcess;

public:
	QuakeAIManager();
	~QuakeAIManager();

	virtual void CreateMap(ActorId playerId);

	virtual void SavePathingGraph(const eastl::string& path);
	virtual void LoadPathingGraph(const eastl::wstring& path);

	virtual void OnUpdate(unsigned long deltaMs);

	bool IsEnable() { return mEnable; }
	void SetEnable(bool enable) { mEnable = enable; }

	ActorId GetPlayerWeaponTarget(ActorId player);
	WeaponType GetPlayerWeapon(ActorId player);
	void GetPlayerState(ActorId player, NodeState& playerState);
	void GetPlayerPlan(ActorId player, NodePlan& playerPlan);
	bool IsPlayerUpdated(ActorId player);

	void SetPlayerState(ActorId player, NodeState& playerState);
	void SetPlayerState(ActorId player, eastl::shared_ptr<PlayerActor> playerActor);
	void SetPlayerPlan(ActorId player, NodePlan& playerPlan);
	void SetPlayerUpdated(ActorId player, bool update);

	void RemovePlayerGuessItems(ActorId player);
	void SetPlayerGuessItems(ActorId player, eastl::map<ActorId, float>& guessItems);
	void GetPlayerGuessItems(ActorId player, eastl::map<ActorId, float>& guessItems);

	void GetPlayerGuessState(ActorId player, NodeState& playerState);
	void GetPlayerGuessPlan(ActorId player, NodePlan& playerPlan);
	bool IsPlayerGuessUpdated(ActorId player);

	void SetPlayerGuessState(ActorId player, NodeState& playerState);
	void SetPlayerGuessState(ActorId player, eastl::shared_ptr<PlayerActor> playerActor);
	void SetPlayerGuessPlan(ActorId player, NodePlan& playerPlan);
	void SetPlayerGuessUpdated(ActorId player, bool update);

	void SpawnActor(ActorId playerId);
	void DetectActor(eastl::shared_ptr<PlayerActor> playerActor, eastl::shared_ptr<Actor> item);

protected:

	int GetNewPlanID(void)
	{
		return ++mLastPlanId;
	}

	void PrintLogError(eastl::string log);
	void PrintLogInformation(eastl::string log);
	void PrintLogInformationDetails(eastl::string log);

	float CalculateHeuristicItems(NodeState& playerState);
	void CalculateHeuristic(NodeState& playerState, NodeState& otherPlayerState);
	void CalculateDamage(NodeState& state, 
		float visibleTime, float visibleDistance, float visibleHeight);

	bool CanItemBeGrabbed(ActorId itemId, float itemTime, NodeState& playerState);
	void PickupItems(NodeState& playerState, 
		eastl::map<ActorId, float>& actors, eastl::map<ActorId, float>& excludeActors);

	void FindPath(NodeState& pNodeState, 
		PathingCluster* pGoalCluster, PathingArcVec& planPath, 
		eastl::map<ActorId, float>& excludeActors, float threshold = FLT_MAX);

private:

	std::ofstream mLogError;
	std::ofstream mLogInformation;
	std::ofstream mLogInformationDetails;

	void SimulateJump(PathingNode* pNode);
	void SimulateMovement(PathingNode* pNode);
	void SimulateTriggerPush(PathingNode* pNode, const Vector3<float>& target);
	void SimulateTriggerTeleport(PathingNode* pNode, const Vector3<float>& target);
	void SimulateActorPosition(ActorId actorId, const Vector3<float>& position);

	void SimulateWaypoint();
	void SimulateVisibility();

	void CreateClusters();

	unsigned int GetNewArcID(void)
	{
		return ++mLastArcId;
	}

	unsigned int GetNewNodeID(void)
	{
		return ++mLastNodeId;
	}

	// event delegates
	void PhysicsTriggerEnterDelegate(BaseEventDataPtr pEventData);
	void PhysicsTriggerLeaveDelegate(BaseEventDataPtr pEventData);
	void PhysicsCollisionDelegate(BaseEventDataPtr pEventData);
	void PhysicsSeparationDelegate(BaseEventDataPtr pEventData);

	void RegisterAllDelegates(void);
	void RemoveAllDelegates(void);

	bool mEnable;
	std::mutex mMutex;

	unsigned int mLastArcId;
	unsigned int mLastNodeId;
	int mLastPlanId;

	//set of nodes to be analized from the ground
	eastl::vector<PathingNode*> mOpenSet, mClosedSet;

	//pathing nodes which contains actors from game
	eastl::map<PathingNode*, ActorId> mActorNodes;

	//player ai states
	eastl::map<ActorId, float> mPlayerTime;
	eastl::map<ActorId, float> mPlayerPlanTime;

	eastl::map<ActorId, bool> mPlayers;
	eastl::map<ActorId, NodePlan> mPlayerPlans;
	eastl::map<ActorId, NodeState> mPlayerStates;
	eastl::map<ActorId, PathingNode*> mPlayerNodes;

	eastl::map<ActorId, eastl::map<ActorId, unsigned short>> mPlayerTargets;
	eastl::map<ActorId, eastl::map<ActorId, float>> mPlayerHeuristics;

	eastl::map<ActorId, bool> mPlayerGuess;
	eastl::map<ActorId, NodePlan> mPlayerGuessPlans;
	eastl::map<ActorId, NodeState> mPlayerGuessStates;
	eastl::map<ActorId, PathingNode*> mPlayerGuessNodes;

	eastl::map<ActorId, eastl::map<ActorId, unsigned short>> mPlayerGuessTargets;
	eastl::map<ActorId, eastl::map<ActorId, float>> mPlayerGuessHeuristics;

	eastl::map<ActorId, eastl::map<ActorId, float>> mPlayerGuessItems;

	// Orientation Controls
	float mYaw;
	float mPitch;
	float mPitchTarget;
	float mPitchOnDown;
	float mYawOnDown;

	// Speed Controls
	float mMaxFallSpeed;
	float mMaxJumpSpeed;
	float mMaxRotateSpeed;
	float mFallSpeed;
	float mJumpSpeed;
	float mJumpMoveSpeed;
	float mMoveSpeed;
	float mRotateSpeed;

	eastl::shared_ptr<PlayerActor> mPlayerActor;

};   // QuakeAIManager

#endif

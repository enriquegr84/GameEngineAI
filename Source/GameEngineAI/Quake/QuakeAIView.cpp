/*******************************************************
 * Copyright (C) GameEngineAI - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Enrique Gonz�lez Rodr�guez <enriquegr84@hotmail.es>, 2019-2020
 *******************************************************/

#include "QuakeStd.h"

#include "Core/OS/OS.h"
#include "Core/Logger/Logger.h"
#include "Core/Event/EventManager.h"

#include "Physic/PhysicEventListener.h"

#include "AI/AIManager.h"

#include "QuakePlayerController.h"
#include "QuakeAIManager.h"
#include "QuakeAIView.h"
#include "QuakeEvents.h"
#include "QuakeApp.h"
#include "Quake.h"

///////////////////////////////////////////////////////////////////////////////
//
// QuakeAIView::QuakeAIView
//

QuakeAIView::QuakeAIView()
	: BaseGameView()
{
	mYaw = 0.0f;
	mYawSmooth = 0.0f;
	mYawSmoothTime = 0.0f;
	mPitchTarget = 0.0f;

	mOrientation = 1;
	mStationaryTime = 0;

	mMaxJumpSpeed = 3.4f;
	mMaxFallSpeed = 240.0f;
	mMaxRotateSpeed = 180.0f;
	mMoveSpeed = 6.0f;
	mJumpSpeed = 3.4f;
	mJumpMoveSpeed = 10.0f;
	mFallSpeed = 0.0f;
	mRotateSpeed = 0.0f;

	mCurrentAction = 0;
	mCurrentActionTime = 0.f;
	mCurrentActor = INVALID_ACTOR_ID;
	mCurrentPlanId = -1;

	mCurrentNode = NULL;
	mCurrentArc = NULL;
	mGoalNode= NULL;

	mViewId = INVALID_GAME_VIEW_ID;
	mPlayerId = INVALID_ACTOR_ID;
}

//
// QuakeAIView::~QuakeAIView
//
QuakeAIView::~QuakeAIView(void)
{
	//LogInformation("AI Destroying QuakeAIView");
}

//  class QuakeAIView::OnAttach
void QuakeAIView::OnAttach(GameViewId vid, ActorId actorId)
{
	mViewId = vid;
	mPlayerId = actorId;

	mPathingGraph = GameLogic::Get()->GetAIManager()->GetPathingGraph();
	eastl::shared_ptr<Actor> pGameActor(GameLogic::Get()->GetActor(mPlayerId).lock());
	eastl::shared_ptr<TransformComponent> pTransformComponent(
		pGameActor->GetComponent<TransformComponent>(TransformComponent::Name).lock());
	if (pTransformComponent)
	{
		EulerAngles<float> yawPitchRoll;
		yawPitchRoll.mAxis[1] = 1;
		yawPitchRoll.mAxis[2] = 2;
		pTransformComponent->GetTransform().GetRotation(yawPitchRoll);

		mYaw = mYawSmooth = yawPitchRoll.mAngle[YAW] * (float)GE_C_RAD_TO_DEG;
		mPitchTarget = yawPitchRoll.mAngle[ROLL] * (float)GE_C_RAD_TO_DEG;

		mAbsoluteTransform.SetRotation(pTransformComponent->GetRotation());
		mAbsoluteTransform.SetTranslation(pTransformComponent->GetPosition());
	}
}

//Stationary movement
void QuakeAIView::Stationary(unsigned long deltaMs)
{
	Vector3<float> position = mAbsoluteTransform.GetTranslation();
	Matrix4x4<float> rotation = Rotation<4, float>(
		AxisAngle<4, float>(Vector4<float>::Unit(YAW), mYaw * (float)GE_C_DEG_TO_RAD));

	// This will give us the "look at" vector 
	// in world space - we'll use that to move.
	Vector4<float> atWorld = Vector4<float>::Unit(PITCH); // forward vector
#if defined(GE_USE_MAT_VEC)
	atWorld = rotation * atWorld;
#else
	atWorld = atWorld * rotation;
#endif

	Vector3<float> scale =
		GameLogic::Get()->GetGamePhysics()->GetScale(mPlayerId) / 2.f;

	Transform start;
	start.SetRotation(rotation);
	start.SetTranslation(mAbsoluteTransform.GetTranslationW1() 
		+ scale[YAW] * Vector4<float>::Unit(YAW));

	Transform end;
	end.SetRotation(rotation);
	end.SetTranslation(mAbsoluteTransform.GetTranslationW1() +
		atWorld * 500.f + scale[YAW] * Vector4<float>::Unit(YAW));

	Vector3<float> collision, collisionNormal;
	collision = end.GetTranslation();
	ActorId actorId = GameLogic::Get()->GetGamePhysics()->ConvexSweep(
		mPlayerId, start, end, collision, collisionNormal);
	if (Length(collision - position) < 50.f)
	{
		mStationaryTime += deltaMs;
		if (mStationaryTime > 100)
		{
			//Choose randomly which way too look for obstacles
			int sign = Randomizer::Rand() % 2 ? 1 : -1;
			mYaw += 130.f * sign;
		}
	}
	else mStationaryTime = 0;
}

// Cliff control
void QuakeAIView::Cliff()
{
	Matrix4x4<float> rotation = Rotation<4, float>(
		AxisAngle<4, float>(Vector4<float>::Unit(YAW), mYaw * (float)GE_C_DEG_TO_RAD));

	// This will give us the "look at" vector 
	// in world space - we'll use that to move.
	Vector4<float> atWorld = Vector4<float>::Unit(PITCH); // forward vector
#if defined(GE_USE_MAT_VEC)
	atWorld = rotation * atWorld;
#else
	atWorld = atWorld * rotation;
#endif

	Vector3<float> position = HProject(
		mAbsoluteTransform.GetTranslationW1() + atWorld * 10.f);

	Transform start;
	start.SetRotation(rotation);
	start.SetTranslation(position);

	Transform end;
	end.SetRotation(rotation);
	end.SetTranslation(mAbsoluteTransform.GetTranslationW1() + 
		atWorld * 10.f - Vector4<float>::Unit(YAW) * 300.f);

	Vector3<float> collision, collisionNormal;
	collision = end.GetTranslation();
	ActorId actorId = GameLogic::Get()->GetGamePhysics()->CastRay(
		start.GetTranslation(), end.GetTranslation(), collision, collisionNormal);

	//Check whether we are close to a cliff
	if (abs(collision[2] - position[2]) > 60.f)
	{
		//Choose randomly which way too look for getting out the cliff
		int sign = Randomizer::Rand() % 2 ? 1 : -1;

		// Smoothly turn 110� and check raycasting until we meet a minimum distance
		for (int angle = 1; angle <= 110; angle++)
		{
			rotation = Rotation<4, float>(
				AxisAngle<4, float>(Vector4<float>::Unit(YAW),
				(mYaw + angle * sign) * (float)GE_C_DEG_TO_RAD));

			atWorld = Vector4<float>::Unit(PITCH); // forward vector
#if defined(GE_USE_MAT_VEC)
			atWorld = rotation * atWorld;
#else
			atWorld = atWorld * rotation;
#endif

			start.SetRotation(rotation);
			end.SetRotation(rotation);
			end.SetTranslation(mAbsoluteTransform.GetTranslationW1() +
				atWorld * 100.f - Vector4<float>::Unit(YAW) * 300.f);

			collision = end.GetTranslation();
			ActorId actorId = GameLogic::Get()->GetGamePhysics()->CastRay(
				start.GetTranslation(), end.GetTranslation(), collision, collisionNormal);
			if (abs(collision[2] - position[2]) <= 60.f)
			{
				mOrientation = Randomizer::Rand() % 2 ? 1 : -1;
				mYaw += angle * sign;
				return;
			}
		}

		//If we haven't find a way out we proceed exactly the same but in the opposite direction
		sign *= -1;
		for (int angle = 1; angle <= 110; angle++)
		{
			rotation = Rotation<4, float>(
				AxisAngle<4, float>(Vector4<float>::Unit(YAW),
				(mYaw + angle * sign) * (float)GE_C_DEG_TO_RAD));

			atWorld = Vector4<float>::Unit(PITCH); // forward vector
#if defined(GE_USE_MAT_VEC)
			atWorld = rotation * atWorld;
#else
			atWorld = atWorld * rotation;
#endif

			start.SetRotation(rotation);
			end.SetRotation(rotation);
			end.SetTranslation(mAbsoluteTransform.GetTranslationW1() +
				atWorld * 100.f - Vector4<float>::Unit(YAW) * 300.f);

			collision = end.GetTranslation();
			ActorId actorId = GameLogic::Get()->GetGamePhysics()->CastRay(
				start.GetTranslation(), end.GetTranslation(), collision, collisionNormal);
			if (abs(collision[2] - position[2]) <= 60.f)
			{
				mOrientation = Randomizer::Rand() % 2 ? 1 : -1;
				mYaw += angle * sign;
				return;
			}
		}

		//if we couldnt find any way out then we make a hard turn
		mOrientation = Randomizer::Rand() % 2 ? 1 : -1;
		mYaw += 130.f * sign;
	}
}

//Avoidance
void QuakeAIView::Avoidance(unsigned long deltaMs)
{
	Vector3<float> position = mAbsoluteTransform.GetTranslation();
	Matrix4x4<float> rotation = Rotation<4, float>(
		AxisAngle<4, float>(Vector4<float>::Unit(YAW), mYaw * (float)GE_C_DEG_TO_RAD));

	// This will give us the "look at" vector 
	// in world space - we'll use that to move.
	Vector4<float> atWorld = Vector4<float>::Unit(PITCH); // forward vector
#if defined(GE_USE_MAT_VEC)
	atWorld = rotation * atWorld;
#else
	atWorld = atWorld * rotation;
#endif

	Vector3<float> scale =
		GameLogic::Get()->GetGamePhysics()->GetScale(mPlayerId) / 2.f;

	Transform start;
	start.SetRotation(rotation);
	start.SetTranslation(mAbsoluteTransform.GetTranslationW1() + 
		scale[YAW] * Vector4<float>::Unit(YAW));

	Transform end;
	end.SetRotation(rotation);
	end.SetTranslation(mAbsoluteTransform.GetTranslationW1() +
		atWorld * 500.f + scale[YAW] * Vector4<float>::Unit(YAW));

	Vector3<float> collision, collisionNormal;
	collision = end.GetTranslation();
	ActorId actorId = GameLogic::Get()->GetGamePhysics()->ConvexSweep(
		mPlayerId, start, end, collision, collisionNormal);
	if (Length(collision - position) < 50.f)
	{
		//Choose randomly which way too look for obstacles
		int sign = mOrientation;

		// Smoothly turn 90� and check raycasting until we meet a minimum distance
		for (int angle = 1; angle <= 90; angle++)
		{
			rotation = Rotation<4, float>(
				AxisAngle<4, float>(Vector4<float>::Unit(YAW),
				(mYaw + angle * sign) * (float)GE_C_DEG_TO_RAD));

			atWorld = Vector4<float>::Unit(PITCH); // forward vector
#if defined(GE_USE_MAT_VEC)
			atWorld = rotation * atWorld;
#else
			atWorld = atWorld * rotation;
#endif

			start.SetRotation(rotation);
			end.SetRotation(rotation);
			end.SetTranslation(mAbsoluteTransform.GetTranslationW1() +
				atWorld * 500.f + scale[YAW] * Vector4<float>::Unit(YAW));

			collision = end.GetTranslation();
			actorId = GameLogic::Get()->GetGamePhysics()->ConvexSweep(
				mPlayerId, start, end, collision, collisionNormal);
			if (Length(collision - position) > 50.f)
			{
				mYaw += angle * sign;
				return;
			}
		}

		//If we haven't find a way out we proceed exactly the same but in the opposite direction
		sign *= -1;
		for (int angle = 1; angle <= 90; angle++)
		{
			rotation = Rotation<4, float>(
				AxisAngle<4, float>(Vector4<float>::Unit(YAW),
				(mYaw + angle * sign) * (float)GE_C_DEG_TO_RAD));

			atWorld = Vector4<float>::Unit(PITCH); // forward vector
#if defined(GE_USE_MAT_VEC)
			atWorld = rotation * atWorld;
#else
			atWorld = atWorld * rotation;
#endif

			start.SetRotation(rotation);
			end.SetRotation(rotation);
			end.SetTranslation(mAbsoluteTransform.GetTranslationW1() +
				atWorld * 500.f + scale[YAW] * Vector4<float>::Unit(YAW));

			collision = end.GetTranslation();
			actorId = GameLogic::Get()->GetGamePhysics()->ConvexSweep(
				mPlayerId, start, end, collision, collisionNormal);
			if (Length(collision - position) > 50.f)
			{
				mYaw += angle * sign;
				return;
			}
		}
	}
}

//Smooth movement
void QuakeAIView::Smooth(unsigned long deltaMs)
{
	Vector3<float> position = mAbsoluteTransform.GetTranslation();
	Matrix4x4<float> rotation = Rotation<4, float>(
		AxisAngle<4, float>(Vector4<float>::Unit(YAW), mYaw * (float)GE_C_DEG_TO_RAD));

	// This will give us the "look at" vector 
	// in world space - we'll use that to move.
	Vector4<float> atWorld = Vector4<float>::Unit(PITCH); // forward vector
#if defined(GE_USE_MAT_VEC)
	atWorld = rotation * atWorld;
#else
	atWorld = atWorld * rotation;
#endif

	Vector3<float> scale = 
		GameLogic::Get()->GetGamePhysics()->GetScale(mPlayerId) / 2.f;

	Transform start;
	start.SetRotation(rotation);
	start.SetTranslation(mAbsoluteTransform.GetTranslationW1() 
		+ scale[YAW] * Vector4<float>::Unit(YAW));

	Transform end;
	end.SetRotation(rotation);
	end.SetTranslation(mAbsoluteTransform.GetTranslationW1() + 
		atWorld * 500.f + scale[YAW] * Vector4<float>::Unit(YAW));

	Vector3<float> collision, collisionNormal;
	collision = end.GetTranslation();
	ActorId actorId = GameLogic::Get()->GetGamePhysics()->ConvexSweep(
		mPlayerId, start, end, collision, collisionNormal);
	if (Length(collision - position) < 80.f)
	{
		//Choose randomly which way too look for obstacles
		int sign = Randomizer::Rand() % 2 ? 1 : -1;

		// Smoothly turn 90� and check raycasting until we meet a minimum distance
		for (int angle = 1; angle <= 90; angle++)
		{
			rotation = Rotation<4, float>(
				AxisAngle<4, float>(Vector4<float>::Unit(YAW),
				(mYaw + angle * sign) * (float)GE_C_DEG_TO_RAD));

			atWorld = Vector4<float>::Unit(PITCH); // forward vector
#if defined(GE_USE_MAT_VEC)
			atWorld = rotation * atWorld;
#else
			atWorld = atWorld * rotation;
#endif

			start.SetRotation(rotation);
			end.SetRotation(rotation);
			end.SetTranslation(mAbsoluteTransform.GetTranslationW1() + 
				atWorld * 500.f + scale[YAW] * Vector4<float>::Unit(YAW));

			collision = end.GetTranslation();
			actorId = GameLogic::Get()->GetGamePhysics()->ConvexSweep(
				mPlayerId, start, end, collision, collisionNormal);
			if (Length(collision - position) > 80.f)
			{
				mOrientation = Randomizer::Rand() % 2 ? 1 : -1;
				mYaw += angle * sign;
				return;
			}
		}

		//If we haven't find a way out we proceed exactly the same but in the opposite direction
		sign *= -1;
		for (int angle = 1; angle <= 90; angle++)
		{
			rotation = Rotation<4, float>(
				AxisAngle<4, float>(Vector4<float>::Unit(YAW),
				(mYaw + angle * sign) * (float)GE_C_DEG_TO_RAD));

			atWorld = Vector4<float>::Unit(PITCH); // forward vector
#if defined(GE_USE_MAT_VEC)
			atWorld = rotation * atWorld;
#else
			atWorld = atWorld * rotation;
#endif

			start.SetRotation(rotation);
			end.SetRotation(rotation);
			end.SetTranslation(mAbsoluteTransform.GetTranslationW1() + 
				atWorld * 500.f + scale[YAW] * Vector4<float>::Unit(YAW));

			collision = end.GetTranslation();
			actorId = GameLogic::Get()->GetGamePhysics()->ConvexSweep(
				mPlayerId, start, end, collision, collisionNormal);
			if (Length(collision - position) > 80.f)
			{
				mOrientation = Randomizer::Rand() % 2 ? 1 : -1;
				mYaw += angle * sign;
				return;
			}
		}

		//if we couldnt find any way out the stationary function will take care of it.
		mOrientation = Randomizer::Rand() % 2 ? 1 : -1;
	}
	else
	{
		mYaw += 0.03f * deltaMs * mOrientation;
	}
}


//  class QuakeAIView::OnUpdate			- Chapter 10, page 283
void QuakeAIView::OnUpdate(unsigned int timeMs, unsigned long deltaMs)
{
	QuakeAIManager* aiManager =
		dynamic_cast<QuakeAIManager*>(GameLogic::Get()->GetAIManager());
	if (!aiManager->IsEnable()) return;

	mYawSmoothTime += deltaMs / 1000.f;
	mCurrentActionTime -= deltaMs / 1000.f;

	eastl::shared_ptr<PlayerActor> pPlayerActor(
		eastl::dynamic_shared_pointer_cast<PlayerActor>(
		GameLogic::Get()->GetActor(mPlayerId).lock()));
	if (!pPlayerActor) return;

	eastl::shared_ptr<TransformComponent> pTransformComponent(
		pPlayerActor->GetComponent<TransformComponent>(TransformComponent::Name).lock());
	if (pTransformComponent)
	{
		if (pPlayerActor->GetAction().triggerPush != INVALID_ACTOR_ID ||
			pPlayerActor->GetAction().triggerTeleporter != INVALID_ACTOR_ID)
		{
			Vector3<float> currentPosition = pTransformComponent->GetPosition();
			if (mCurrentPlan.size())
			{
				bool isValid = false;
				PathingArcVec::iterator itArc = mCurrentPlan.begin();
				do
				{
					mCurrentArc = (*itArc);
					mCurrentAction = mCurrentArc->GetType();
					mCurrentNode = mCurrentArc->GetNode();
					mCurrentActionTime = mCurrentArc->GetWeight() + 1.5f;
					Vector3<float> direction = mCurrentNode->GetPos() - currentPosition;
					Normalize(direction);
					mYaw = atan2(direction[1], direction[0]) * (float)GE_C_RAD_TO_DEG;

					if (pPlayerActor->GetAction().triggerPush != INVALID_ACTOR_ID &&
						pPlayerActor->GetAction().triggerPush == mCurrentNode->GetActorId())
					{
						isValid = true;
						break;
					}
					else if (pPlayerActor->GetAction().triggerTeleporter != INVALID_ACTOR_ID &&
						pPlayerActor->GetAction().triggerTeleporter == mCurrentNode->GetActorId())
					{
						isValid = true;
						break;
					}
					if (mCurrentAction == GAT_PUSH || mCurrentAction == GAT_TELEPORT)
					{
						isValid = true;
						break;
					}
					itArc++;
				} while (itArc != mCurrentPlan.end());

				itArc = mCurrentPlan.begin();
				if (isValid)
				{
					while (!mCurrentPlan.empty())
					{
						if ((*itArc)->GetType() == GAT_PUSH || (*itArc)->GetType() == GAT_TELEPORT)
						{
							itArc++;
							break;
						}
							
						itArc = mCurrentPlan.erase(mCurrentPlan.begin());
					}
					if (itArc != mCurrentPlan.end())
					{
						mCurrentArc = (*itArc);
						mCurrentAction = mCurrentArc->GetType();
						mCurrentNode = mCurrentArc->GetNode();

						Vector3<float> direction = mCurrentNode->GetPos() - currentPosition;
						Normalize(direction);
						mYaw = atan2(direction[1], direction[0]) * (float)GE_C_RAD_TO_DEG;
					}
					else
					{
						mGoalNode = NULL;
						mCurrentArc = NULL;
						mCurrentNode = NULL;
						mCurrentPlanId = -1;

						mCurrentActor = INVALID_ACTOR_ID;
						mCurrentActionTime = 0.f;
						mCurrentAction = 0;
						mCurrentPlan.clear();
					}
				}
				else
				{
					mCurrentArc = (*itArc);
					mCurrentNode = NULL;
					mCurrentPlanId = -1;

					mCurrentActor = INVALID_ACTOR_ID;
					mCurrentActionTime = 0.f;
					mCurrentAction = 0;

					mGoalNode = NULL;
				}
			}
			else if (mCurrentNode != NULL && mGoalNode != NULL)
			{
				do
				{
					PathingCluster* currentCluster = mCurrentNode->FindCluster(GAT_MOVE, mGoalNode);
					if (currentCluster != NULL)
					{
						PathingArc* clusterArc = mCurrentNode->FindArc(currentCluster->GetNode());
						PathingNode* clusterNode = clusterArc->GetNode();
						unsigned int clusterArcType = clusterArc->GetType();

						mCurrentArc = clusterArc;
						mCurrentAction = clusterArcType;
						mCurrentNode = clusterArc->GetNode();
						mCurrentActionTime = clusterArc->GetWeight() + 1.5f;
						Vector3<float> direction = clusterNode->GetPos() - currentPosition;
						Normalize(direction);
						mYaw = atan2(direction[1], direction[0]) * (float)GE_C_RAD_TO_DEG;
					}
					else mCurrentNode = mGoalNode;

					if (mCurrentAction == GAT_PUSH || mCurrentAction == GAT_TELEPORT)
						break;
				} while (mCurrentNode != mGoalNode);

				PathingNode* currentNode = mCurrentNode;
				if (currentNode != mGoalNode)
				{
					//printf("\n random trigger nodes %u : ", mPlayerId);
					do
					{
						PathingCluster* currentCluster = currentNode->FindCluster(GAT_MOVE, mGoalNode);
						PathingArc* clusterArc = currentNode->FindArc(currentCluster->GetNode());
						currentNode = clusterArc->GetNode();
						//printf("%u ", currentNode->GetId());
					} while (currentNode != mGoalNode);
				}
			}
		}

		if (pPlayerActor->GetAction().triggerTeleporter != INVALID_ACTOR_ID)
		{
			eastl::shared_ptr<Actor> pItemActor(
				eastl::dynamic_shared_pointer_cast<Actor>(
				GameLogic::Get()->GetActor(pPlayerActor->GetAction().triggerTeleporter).lock()));
			eastl::shared_ptr<TeleporterTrigger> pTeleporterTrigger =
				pItemActor->GetComponent<TeleporterTrigger>(TeleporterTrigger::Name).lock();

			EulerAngles<float> yawPitchRoll;
			yawPitchRoll.mAxis[1] = 1;
			yawPitchRoll.mAxis[2] = 2;
			pTeleporterTrigger->GetTarget().GetRotation(yawPitchRoll);
			mYaw = yawPitchRoll.mAngle[YAW] * (float)GE_C_RAD_TO_DEG;
			mPitchTarget = -yawPitchRoll.mAngle[ROLL] * (float)GE_C_RAD_TO_DEG;

			EventManager::Get()->TriggerEvent(
				eastl::make_shared<QuakeEventDataTeleportActor>(mPlayerId));
		}
		else
		{
			eastl::shared_ptr<PhysicComponent> pPhysicComponent(
				pPlayerActor->GetComponent<PhysicComponent>(PhysicComponent::Name).lock());
			if (pPhysicComponent)
			{
				pPlayerActor->GetAction().actionType = 0;
				pPlayerActor->GetAction().actionType |= ACTION_MOVEFORWARD;

				if (aiManager->GetPlayerWeapon(mPlayerId) != WP_NONE)
					pPlayerActor->ChangeWeapon(aiManager->GetPlayerWeapon(mPlayerId));

				Vector4<float> velocity = Vector4<float>::Zero();
				if (pPhysicComponent->OnGround())
				{
					mFallSpeed = 0.0f;

					if (pPlayerActor->GetAction().triggerPush != INVALID_ACTOR_ID)
					{
						float push;
						Vector3<float> direction;
						eastl::shared_ptr<Actor> pItemActor(
							eastl::dynamic_shared_pointer_cast<Actor>(
							GameLogic::Get()->GetActor(pPlayerActor->GetAction().triggerPush).lock()));
						eastl::shared_ptr<PushTrigger> pPushTrigger =
							pItemActor->GetComponent<PushTrigger>(PushTrigger::Name).lock();

						Vector3<float> targetPosition = pPushTrigger->GetTarget().GetTranslation();
						Vector3<float> playerPosition = pTransformComponent->GetPosition();
						direction = targetPosition - playerPosition;
						push = Length(direction);
						Normalize(direction);

						direction[PITCH] *= push / 90.f;
						direction[ROLL] *= push / 90.f;
						direction[YAW] = push / 30.f;
						velocity = HLift(direction, 0.f);

						pPlayerActor->GetAction().actionType |= ACTION_JUMP;
					}
					else
					{
						mPitchTarget = 0.f;
						mPitchTarget = eastl::max(-85.f, eastl::min(85.f, mPitchTarget));
						mPitch = 90 * ((mPitchTarget + 85.f) / 170.f) - 45.f;

						if (mPathingGraph)
						{
							Vector3<float> currentPosition = pTransformComponent->GetPosition();
							PathingNode* currentNode = aiManager->GetPathingGraph()->FindClosestNode(currentPosition);
							if (mCurrentNode == NULL)
								mCurrentNode = currentNode;

							bool searchNode = false;
							if (mCurrentNode->GetActorId() != INVALID_ACTOR_ID)
							{
								eastl::shared_ptr<Actor> pItemActor(
									eastl::dynamic_shared_pointer_cast<Actor>(
									GameLogic::Get()->GetActor(mCurrentNode->GetActorId()).lock()));
								if (pItemActor->GetType() != "Trigger")
								{
									if (mCurrentActor != mCurrentNode->GetActorId())
									{
										eastl::shared_ptr<TransformComponent> pTransform =
											pItemActor->GetComponent<TransformComponent>(TransformComponent::Name).lock();
										Vector3<float> diff = pTransform->GetPosition() - currentPosition;
										diff[YAW] = 0.f;

										if (Length(diff) <= 6.0f)
										{
											searchNode = true;
											if (!mCurrentPlan.empty())
												mCurrentPlan.erase(mCurrentPlan.begin());
											mCurrentActor = mCurrentNode->GetActorId();
										}
									}
									else
									{
										Vector3<float> diff = mCurrentNode->GetPos() - currentPosition;
										if (Length(diff) <= 6.0f)
										{
											searchNode = true;
											if (!mCurrentPlan.empty())
												mCurrentPlan.erase(mCurrentPlan.begin());
										}
									}
								}
							}
							else
							{
								Vector3<float> diff = mCurrentNode->GetPos() - currentPosition;
								if (Length(diff) <= 6.0f)
								{
									searchNode = true;
									if (!mCurrentPlan.empty())
										mCurrentPlan.erase(mCurrentPlan.begin());
								}
							}

							if (mCurrentActionTime <= 0.f)
							{
								printf("\n current decision making player %u : ", mPlayerId);
								for (PathingArc* pathArc : mCurrentPlan)
									printf("%u ", pathArc->GetNode()->GetId());

								if (mCurrentArc != NULL)
								{
									printf("\n fail arc id %u type %u node %u \n ",
										mCurrentArc->GetId(), mCurrentArc->GetType(), mCurrentArc->GetNode()->GetId());

									eastl::string error("fail arc id " +
										eastl::to_string(mCurrentArc->GetId()) + " type " +
										eastl::to_string(mCurrentArc->GetType()) + " node " +
										eastl::to_string(mCurrentArc->GetNode()->GetId()) + "\n");
									aiManager->PrintLogError(error);
									LogInformation(error);
								}
								else
								{
									printf("\n no arc \n ");
									LogInformation("no arc");
								}

								mCurrentPlanId = -1;
								mCurrentPlan.clear();
								mCurrentNode = currentNode;

								mCurrentActor = INVALID_ACTOR_ID;
								mCurrentActionTime = 0.f;
								mCurrentAction = 0;

								mCurrentArc = NULL;
								mGoalNode = NULL;
								searchNode = true;
							}

							if (searchNode)
							{
								NodeState playerState;
								PathingArcVec playerPathPlan;
								if (aiManager->IsPlayerUpdated(mPlayerId))
								{
									aiManager->GetPlayerState(mPlayerId, playerState);
									playerPathPlan = playerState.plan.path;
								}

								if (playerPathPlan.size() && playerState.plan.id != mCurrentPlanId)
								{
									Timer::RealTimeDate realTime = Timer::GetRealTimeAndDate();
									aiManager->PrintLogPathingInformation("\n\n player " +
										eastl::to_string(mPlayerId) + " time " +
										eastl::to_string(realTime.Hour) + ":" +
										eastl::to_string(realTime.Minute) + ":" +
										eastl::to_string(realTime.Second) + "\n");
									aiManager->PrintLogPathingInformation("\n old path : ");
									for (PathingArc* pathArc : mCurrentPlan)
									{
										aiManager->PrintLogPathingInformation(
											eastl::to_string(pathArc->GetNode()->GetId()) + " ");
									}
									aiManager->PrintLogPathingInformation("\n new plan path : ");
									for (PathingArc* pathArc : playerPathPlan)
									{
										aiManager->PrintLogPathingInformation(
											eastl::to_string(pathArc->GetNode()->GetId()) + " ");
									}

									PathingNodeVec searchNodes;
									for (PathingArc* pathArc : playerPathPlan)
										searchNodes.push_back(pathArc->GetNode());

									PathPlan* plan = 
										aiManager->GetPathingGraph()->FindPath(mCurrentNode, searchNodes, GAT_JUMP, 2.0f);
									if (plan)
									{
										plan->ResetPath();

										PathingArcVec path;
										if (!plan->CheckForEnd())
										{
											PathingNode* node = plan->GetArcs().back()->GetNode();
											for (PathingArc* planArc : plan->GetArcs())
												path.push_back(planArc);

											PathingArcVec::iterator itArc;
											PathingArcVec::iterator itPathArc = playerPathPlan.begin();
											for (itArc = playerPathPlan.begin(); itArc != playerPathPlan.end(); itArc++)
												if ((*itArc)->GetNode() == node)
													itPathArc = itArc;

											for (itPathArc++; itPathArc != playerPathPlan.end(); itPathArc++)
												path.push_back((*itPathArc));

											aiManager->PrintLogPathingInformation("\n fusion plan path : ");
											for (PathingArc* pathArc : path)
											{
												aiManager->PrintLogPathingInformation(
													eastl::to_string(pathArc->GetNode()->GetId()) + " ");
											}

											//printf("\n found new plan %u : ", mPlayerId);
											mCurrentPlanId = playerState.plan.id;
											aiManager->SetPlayerUpdated(mPlayerId, false);
										}
										else if (eastl::find(searchNodes.begin(), searchNodes.end(), mCurrentNode))
										{
											PathingArcVec::iterator itArc;
											PathingArcVec::iterator itPathArc = playerPathPlan.begin();
											for (itArc = playerPathPlan.begin(); itArc != playerPathPlan.end(); itArc++)
												if ((*itArc)->GetNode() == mCurrentNode)
													itPathArc = itArc;

											for (itPathArc++; itPathArc != playerPathPlan.end(); itPathArc++)
												path.push_back((*itPathArc));

											aiManager->PrintLogPathingInformation("\n fusion search path : ");
											for (PathingArc* pathArc : path)
											{
												aiManager->PrintLogPathingInformation(
													eastl::to_string(pathArc->GetNode()->GetId()) + " ");
											}

											//printf("\n new plan %u : ", mPlayerId);
											mCurrentPlanId = playerState.plan.id;
											aiManager->SetPlayerUpdated(mPlayerId, false);
										}
										else if (!mCurrentPlan.empty())
										{
											aiManager->PrintLogPathingInformation("\n same old path");

											path = mCurrentPlan;
										}

										mCurrentPlan = path;
										delete plan;
									}
								}

								if (!mCurrentPlan.empty())
								{
									PathingArcVec::iterator itArc = mCurrentPlan.begin();
									mCurrentArc = (*itArc);

									if (mCurrentNode->FindArc(mCurrentArc->GetId()) != NULL)
									{
										NodePlan playerPlan;
										playerPlan.id = mCurrentPlanId;
										playerPlan.node = mCurrentNode;
										playerPlan.path = mCurrentPlan;
										aiManager->SetPlayerPlan(mPlayerId, playerPlan);
									}

									mCurrentAction = mCurrentArc->GetType();
									if (mCurrentAction != GAT_PUSH && mCurrentAction != GAT_TELEPORT)
										mCurrentNode = mCurrentArc->GetNode();
									mCurrentActionTime = mCurrentArc->GetWeight() + 1.0f;
									Vector3<float> direction = mCurrentNode->GetPos() - currentPosition;
									Normalize(direction);
									mYaw = atan2(direction[1], direction[0]) * (float)GE_C_RAD_TO_DEG;

									mGoalNode = NULL;
								}
								else
								{
									mCurrentNode = currentNode;
									NodePlan playerPlan(mCurrentNode, mCurrentPlan);
									aiManager->SetPlayerPlan(mPlayerId, playerPlan);

									Timer::RealTimeDate realTime = Timer::GetRealTimeAndDate();
									aiManager->PrintLogPathingInformation("\n player " +
										eastl::to_string(mPlayerId) + " time " +
										eastl::to_string(realTime.Hour) + ":" +
										eastl::to_string(realTime.Minute) + ":" +
										eastl::to_string(realTime.Second));
									aiManager->PrintLogPathingInformation("\n random path ");

									if (mGoalNode == NULL || mGoalNode == currentNode)
									{
										//printf("\n random node %u : ", mPlayerId);
										PathingClusterVec clusterNodes;
										currentNode->GetClusters(GAT_MOVE, clusterNodes);

										// choose a random cluster
										do
										{
											if (!clusterNodes.empty())
											{
												unsigned int cluster = Randomizer::Rand() % clusterNodes.size();
												mGoalNode = clusterNodes[cluster]->GetTarget();
											}
											else
											{
												mGoalNode = NULL;
												break;
											}
										} while (currentNode == mGoalNode || mGoalNode->GetArcs().empty());
									}

									if (mGoalNode != NULL)
									{
										float minPosDiff = FLT_MAX;
										PathingCluster* currentCluster = currentNode->FindCluster(GAT_MOVE, mGoalNode);
										if (currentCluster != NULL)
										{
											PathingArc* clusterArc = currentNode->FindArc(currentCluster->GetNode());
											PathingNode* clusterNode = clusterArc->GetNode();
											unsigned int clusterArcType = clusterArc->GetType();

											mCurrentArc = clusterArc;
											mCurrentAction = clusterArcType;
											if (mCurrentAction != GAT_PUSH && mCurrentAction != GAT_TELEPORT)
												mCurrentNode = clusterArc->GetNode();
											mCurrentActionTime = clusterArc->GetWeight() + 1.0f;
											Vector3<float> direction = clusterNode->GetPos() - currentPosition;
											Normalize(direction);
											mYaw = atan2(direction[1], direction[0]) * (float)GE_C_RAD_TO_DEG;

											//printf("\n new plan %u : ", mPlayerId);
											do
											{
												currentCluster = currentNode->FindCluster(GAT_MOVE, mGoalNode);
												clusterArc = currentNode->FindArc(currentCluster->GetNode());
												currentNode = clusterArc->GetNode();
												//printf("%u ", currentNode->GetId());
											} while (currentNode != mGoalNode);
										}
										else
										{
											mGoalNode = NULL;
											mCurrentArc = NULL;
											mCurrentNode = NULL;
											mCurrentPlanId = -1;

											mCurrentActor = INVALID_ACTOR_ID;
											mCurrentActionTime = 0.f;
											mCurrentAction = 0;
											mCurrentPlan.clear();
										}
									}
									else
									{
										Stationary(deltaMs);
										Smooth(deltaMs);
										Cliff();
									}
								}
							}
							else
							{
								Vector3<float> direction;
								if (mCurrentAction == GAT_JUMP || mCurrentAction == GAT_PUSH || mCurrentAction == GAT_TELEPORT)
								{
									eastl::shared_ptr<Actor> pItemActor(
										eastl::dynamic_shared_pointer_cast<Actor>(
										GameLogic::Get()->GetActor(mCurrentNode->GetActorId()).lock()));
									if (pItemActor && pItemActor->GetType() == "Trigger")
									{
										eastl::shared_ptr<TransformComponent> pTriggerTransform =
											pItemActor->GetComponent<TransformComponent>(TransformComponent::Name).lock();

										direction = pTriggerTransform->GetPosition() - currentPosition;
									}
									else
									{
										if (mCurrentAction == GAT_JUMP)
										{
											pPlayerActor->GetAction().actionType |= ACTION_JUMP;
											mCurrentAction = 0;
										}

										direction = mCurrentNode->GetPos() - currentPosition;
									}

									Normalize(direction);
									mYaw = atan2(direction[1], direction[0]) * (float)GE_C_RAD_TO_DEG;
								}
								else
								{
									if (mCurrentNode->GetActorId() != INVALID_ACTOR_ID)
									{
										eastl::shared_ptr<Actor> pItemActor(
											eastl::dynamic_shared_pointer_cast<Actor>(
											GameLogic::Get()->GetActor(mCurrentNode->GetActorId()).lock()));

										if (mCurrentActor != mCurrentNode->GetActorId())
										{
											eastl::shared_ptr<TransformComponent> pTransform =
												pItemActor->GetComponent<TransformComponent>(TransformComponent::Name).lock();

											direction = pTransform->GetPosition() - currentPosition;
										}
										else direction = mCurrentNode->GetPos() - currentPosition;
									}
									else direction = mCurrentNode->GetPos() - currentPosition;

									Normalize(direction);
									mYaw = atan2(direction[1], direction[0]) * (float)GE_C_RAD_TO_DEG;

									Vector3<float> scale =
										GameLogic::Get()->GetGamePhysics()->GetScale(mPlayerId) / 2.f;

									Transform start;
									start.SetTranslation(currentPosition + scale[YAW] * Vector3<float>::Unit(YAW));
									Transform end;
									end.SetTranslation(currentPosition + direction * 40.f + scale[YAW] * Vector3<float>::Unit(YAW));

									Vector3<float> collision, collisionNormal;
									ActorId actorId = GameLogic::Get()->GetGamePhysics()->ConvexSweep(
										mPlayerId, start, end, collision, collisionNormal);
									if (actorId != INVALID_ACTOR_ID)
									{
										eastl::shared_ptr<Actor> pActor(
											eastl::dynamic_shared_pointer_cast<Actor>(
											GameLogic::Get()->GetActor(actorId).lock()));
										if (pActor && pActor->GetType() == "Player")
										{
											//dynamic avoidance
											Stationary(deltaMs);
											Avoidance(deltaMs);
										}
									}
								}
							}
						}
						else
						{
							Stationary(deltaMs);
							Smooth(deltaMs);
							Cliff();
						}

						// Calculate the new rotation matrix from the camera
						// yaw and pitch (zrotate and xrotate).
						Matrix4x4<float> yawRotation = Rotation<4, float>(
							AxisAngle<4, float>(Vector4<float>::Unit(YAW), mYaw * (float)GE_C_DEG_TO_RAD));
						Matrix4x4<float> rotation = yawRotation;
						Matrix4x4<float> pitchRotation = Rotation<4, float>(
							AxisAngle<4, float>(Vector4<float>::Unit(ROLL), mPitch * (float)GE_C_DEG_TO_RAD));

						//smoothing rotation
						if (abs(mYawSmooth - mYaw) < 90)
						{
							if (mYaw - mYawSmooth > 180)
								mYawSmooth--;
							else if (mYaw - mYawSmooth < -180)
								mYawSmooth++;
							else if (mYaw > mYawSmooth)
								mYawSmooth++;
							else if (mYaw < mYawSmooth)
								mYawSmooth--;
						}
						else if (mYawSmoothTime >= 0.5f)
						{
							mYawSmooth = mYaw;
							mYawSmoothTime = 0.f;
						}

						yawRotation = Rotation<4, float>(
							AxisAngle<4, float>(Vector4<float>::Unit(YAW), mYawSmooth * (float)GE_C_DEG_TO_RAD));

						mAbsoluteTransform.SetRotation(yawRotation * pitchRotation);
						mAbsoluteTransform.SetTranslation(pTransformComponent->GetPosition());

						// This will give us the "look at" vector 
						// in world space - we'll use that to move.
						Vector4<float> atWorld = Vector4<float>::Unit(PITCH); // forward vector
#if defined(GE_USE_MAT_VEC)
						atWorld = rotation * atWorld;
#else
						atWorld = atWorld * rotation;
#endif
						Normalize(atWorld);

						if (pPlayerActor->GetAction().actionType & ACTION_JUMP)
						{
							Vector4<float> upWorld = Vector4<float>::Unit(YAW);
							Vector4<float> direction = atWorld + upWorld;
							Normalize(direction);

							direction[PITCH] *= mJumpMoveSpeed;
							direction[ROLL] *= mJumpMoveSpeed;
							direction[YAW] *= mJumpSpeed;
							velocity = direction;
						}
						else
						{
							atWorld *= mMoveSpeed;
							velocity = atWorld;
						}

						ActorId playerTarget = aiManager->GetPlayerWeaponTarget(mPlayerId);
						if (playerTarget != INVALID_ACTOR_ID)
						{
							eastl::shared_ptr<PlayerActor> pPlayerTarget(
								eastl::dynamic_shared_pointer_cast<PlayerActor>(
								GameLogic::Get()->GetActor(playerTarget).lock()));

							if (pPlayerTarget->GetState().stats[STAT_HEALTH] > 0)
							{
								//set muzzle location relative to pivoting eye
								Vector3<float> playerPos = pTransformComponent->GetTransform().GetTranslation();
								playerPos += Vector3<float>::Unit(YAW) * (float)pPlayerActor->GetState().viewHeight;

								eastl::shared_ptr<TransformComponent> pTargetTransform(
									pPlayerTarget->GetComponent<TransformComponent>(TransformComponent::Name).lock());

								Vector3<float> targetPos = pTargetTransform->GetTransform().GetTranslation();
								//targetPos += Vector3<float>::Unit(YAW) * (float)pPlayerTarget->GetState().viewHeight;

								eastl::vector<ActorId> collisionActors;
								eastl::vector<Vector3<float>> collisions, collisionNormals;
								GameLogic::Get()->GetGamePhysics()->CastRay(
									playerPos, targetPos, collisionActors, collisions, collisionNormals);

								ActorId closestCollisionId = INVALID_ACTOR_ID;
								Vector3<float> closestCollision = targetPos;
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

								if (closestCollisionId == pPlayerTarget->GetId())
								{
									Vector3<float> direction = closestCollision - playerPos;
									float scale = Length(direction);
									Normalize(direction);

									mYaw = mYawSmooth = atan2(direction[1], direction[0]) * (float)GE_C_RAD_TO_DEG;
									mPitchTarget = -asin(direction[2]) * (float)GE_C_RAD_TO_DEG;

									mPitchTarget = eastl::max(-85.f, eastl::min(85.f, mPitchTarget));
									mPitch = 90 * ((mPitchTarget + 85.f) / 170.f) - 45.f;

									yawRotation = Rotation<4, float>(
										AxisAngle<4, float>(Vector4<float>::Unit(YAW), mYaw * (float)GE_C_DEG_TO_RAD));
									pitchRotation = Rotation<4, float>(
										AxisAngle<4, float>(Vector4<float>::Unit(ROLL), mPitch * (float)GE_C_DEG_TO_RAD));
									mAbsoluteTransform.SetRotation(yawRotation * pitchRotation);

									pPlayerActor->GetAction().actionType |= ACTION_ATTACK;
								}
							}
						}

						// update node rotation matrix
						pitchRotation = Rotation<4, float>(
							AxisAngle<4, float>(Vector4<float>::Unit(ROLL), mPitchTarget * (float)GE_C_DEG_TO_RAD));
						pTransformComponent->SetRotation(yawRotation * pitchRotation);
					}
					pPlayerActor->GetAction().actionType |= ACTION_RUN;
				}
				else
				{
					mFallSpeed += deltaMs / (pPhysicComponent->GetJumpSpeed() * 0.5f);
					if (mFallSpeed > mMaxFallSpeed) mFallSpeed = mMaxFallSpeed;

					// Calculate the new rotation matrix from the camera
					// yaw and pitch (zrotate and xrotate).
					Matrix4x4<float> yawRotation = Rotation<4, float>(
						AxisAngle<4, float>(Vector4<float>::Unit(YAW), mYaw * (float)GE_C_DEG_TO_RAD));
					Matrix4x4<float> rotation = yawRotation;
					Matrix4x4<float> pitchRotation = Rotation<4, float>(
						AxisAngle<4, float>(Vector4<float>::Unit(ROLL), mPitch * (float)GE_C_DEG_TO_RAD));

					// This will give us the "look at" vector 
					// in world space - we'll use that to move.
					Vector4<float> atWorld = Vector4<float>::Unit(PITCH); // forward vector
#if defined(GE_USE_MAT_VEC)
					atWorld = rotation * atWorld;
#else
					atWorld = atWorld * rotation;
#endif
					Normalize(atWorld);

					Vector4<float> upWorld = -Vector4<float>::Unit(YAW);
					Vector4<float> direction = atWorld + upWorld;
					Normalize(direction);

					direction[PITCH] *= pPhysicComponent->GetJumpSpeed() * (mFallSpeed / 4.f);
					direction[ROLL] *= pPhysicComponent->GetJumpSpeed() * (mFallSpeed / 4.f);
					direction[YAW] = -pPhysicComponent->GetJumpSpeed() * mFallSpeed;
					velocity = direction;

					ActorId playerTarget = aiManager->GetPlayerWeaponTarget(mPlayerId);
					if (playerTarget != INVALID_ACTOR_ID)
					{
						eastl::shared_ptr<PlayerActor> pPlayerTarget(
							eastl::dynamic_shared_pointer_cast<PlayerActor>(
							GameLogic::Get()->GetActor(playerTarget).lock()));

						if (pPlayerTarget->GetState().stats[STAT_HEALTH] > 0)
						{
							//set muzzle location relative to pivoting eye
							Vector3<float> playerPos = pTransformComponent->GetTransform().GetTranslation();
							playerPos += Vector3<float>::Unit(YAW) * (float)pPlayerActor->GetState().viewHeight;

							eastl::shared_ptr<TransformComponent> pTargetTransform(
								pPlayerTarget->GetComponent<TransformComponent>(TransformComponent::Name).lock());

							Vector3<float> targetPos = pTargetTransform->GetTransform().GetTranslation();
							//targetPos += Vector3<float>::Unit(YAW) * (float)pPlayerTarget->GetState().viewHeight;

							eastl::vector<ActorId> collisionActors;
							eastl::vector<Vector3<float>> collisions, collisionNormals;
							GameLogic::Get()->GetGamePhysics()->CastRay(
								playerPos, targetPos, collisionActors, collisions, collisionNormals);

							ActorId closestCollisionId = INVALID_ACTOR_ID;
							Vector3<float> closestCollision = targetPos;
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

							if (closestCollisionId == pPlayerTarget->GetId())
							{
								Vector3<float> direction = closestCollision - playerPos;
								float scale = Length(direction);
								Normalize(direction);

								mYaw = mYawSmooth = atan2(direction[1], direction[0]) * (float)GE_C_RAD_TO_DEG;
								mPitchTarget = -asin(direction[2]) * (float)GE_C_RAD_TO_DEG;

								mPitchTarget = eastl::max(-85.f, eastl::min(85.f, mPitchTarget));
								mPitch = 90 * ((mPitchTarget + 85.f) / 170.f) - 45.f;

								yawRotation = Rotation<4, float>(
									AxisAngle<4, float>(Vector4<float>::Unit(YAW), mYaw * (float)GE_C_DEG_TO_RAD));
								pitchRotation = Rotation<4, float>(
									AxisAngle<4, float>(Vector4<float>::Unit(ROLL), mPitch * (float)GE_C_DEG_TO_RAD));
								mAbsoluteTransform.SetRotation(yawRotation * pitchRotation);

								pPlayerActor->GetAction().actionType |= ACTION_ATTACK;

								// update node rotation matrix
								pitchRotation = Rotation<4, float>(
									AxisAngle<4, float>(Vector4<float>::Unit(ROLL), mPitchTarget * (float)GE_C_DEG_TO_RAD));
								pTransformComponent->SetRotation(yawRotation * pitchRotation);
							}
						}
					}

					pPlayerActor->GetAction().actionType |= ACTION_FALLEN;
				}

				if (pPlayerActor->GetState().moveType == PM_DEAD)
				{
					mGoalNode = NULL;
					mCurrentArc = NULL;
					mCurrentNode = NULL;
					mCurrentPlanId = -1;

					mCurrentActor = INVALID_ACTOR_ID;
					mCurrentActionTime = 0.f;
					mCurrentAction = 0;
					mCurrentPlan.clear();

					pPlayerActor->PlayerSpawn();
					pPlayerActor->GetAction().actionType = 0;
				}
				else
				{
					EventManager::Get()->TriggerEvent(
						eastl::make_shared<QuakeEventDataRotateActor>(mPlayerId, mAbsoluteTransform));

					pPlayerActor->UpdateTimers(deltaMs);
					pPlayerActor->UpdateWeapon(deltaMs);
					pPlayerActor->UpdateMovement(HProject(velocity));
				}
			}
		}
	}
}
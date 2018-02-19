// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "ParticleSystemNode.h"

#include "Graphic/Renderer/Renderer.h"
#include "Graphic/Effect/Material.h"

#include "Core/OS/OS.h"

#include "Graphic/Scene/Scene.h"

#include "ParticleAnimatedMeshNodeEmitter.h"

//#include "Utilities/ViewFrustum.h"

//! constructor
ParticleSystemNode::ParticleSystemNode(const ActorId actorId, PVWUpdater& updater,
	WeakBaseRenderComponentPtr renderComponent, bool createDefaultEmitter)
:	Node(actorId, renderComponent, RP_TRANSPARENT, NT_PARTICLE_SYSTEM),
	mEmitter(0), mParticleSize(Vector2<float>{5.f, 5.f}), mLastEmitTime(0),
	mMaxParticles(0xffff), mBuffer(0), mParticlesAreGlobal(true)
{
	#ifdef _DEBUG
	//setDebugName("CParticleSystemSceneNode");
	#endif
	mPVWUpdater = updater;
	mBuffer = eastl::make_shared<MeshBuffer<float>>();
	if (createDefaultEmitter)
		SetEmitter(eastl::make_shared<BaseParticleEmitter>(CreateBoxEmitter()));
}


//! destructor
ParticleSystemNode::~ParticleSystemNode()
{
	RemoveAllAffectors();
}


//! prerender
bool ParticleSystemNode::PreRender(Scene *pScene)
{
	DoParticleSystem(Timer::GetTime());

	if (IsVisible() && (mParticles.size() != 0))
	{
		int transparentCount = 0;
		int solidCount = 0;

		// count transparent and solid materials in this scene node
		for (unsigned int i = 0; i < GetMaterialCount(); ++i)
		{
			if (GetMaterial(i).IsTransparent())
				++transparentCount;
			else
				++solidCount;

			if (solidCount && transparentCount)
				break;
		}

		// register according to material types counted
		if (!pScene->IsCulled(this))
		{
			if (solidCount)
				pScene->AddToRenderQueue(RP_SOLID, shared_from_this());

			if (transparentCount)
				pScene->AddToRenderQueue(RP_TRANSPARENT, shared_from_this());
		}
	}

	return true;
}

//
// LightSceneNode::Render
//
bool ParticleSystemNode::Render(Scene *pScene)
{
	Transform toWorld, fromWorld;
	//Transform(&toWorld, &fromWorld);

	const eastl::shared_ptr<CameraNode>& node = pScene->GetActiveCamera();

	if (!node || !Renderer::Get())
		return false;

	const Matrix4x4<float> &m = node->GetCamera()->GetViewMatrix();
	const Vector3<float> view{ -m[2], -m[6] , -m[10] };

	// reallocate arrays, if they are too small
	ReallocateBuffers();

	// create particle vertex data
	int idx = 0;
	for (unsigned int i=0; i<mParticles.size(); ++i)
	{
		const Particle& particle = mParticles[i];

		float f = 0.5f * particle.mSize[0];
		const Vector3<float> horizontal{ m[0] * f, m[4] * f, m[8] * f };

		f = -0.5f * particle.mSize[1];
		const Vector3<float> vertical{ m[1] * f, m[5] * f, m[9] * f };

		mBuffer->mVertices[0+idx].mPos = particle.mPos + horizontal + vertical;
		mBuffer->mVertices[0+idx].mColor = particle.mColor;
		mBuffer->mVertices[0+idx].mNormal = view;

		mBuffer->mVertices[1+idx].mPos = particle.mPos + horizontal - vertical;
		mBuffer->mVertices[1+idx].mColor = particle.mColor;
		mBuffer->mVertices[1+idx].mNormal = view;

		mBuffer->mVertices[2+idx].mPos = particle.mPos - horizontal - vertical;
		mBuffer->mVertices[2+idx].mColor = particle.mColor;
		mBuffer->mVertices[2+idx].mNormal = view;

		mBuffer->mVertices[3+idx].mPos = particle.mPos - horizontal + vertical;
		mBuffer->mVertices[3+idx].mColor = particle.mColor;
		mBuffer->mVertices[3+idx].mNormal = view;

		idx +=4;
	}

	// render all
	Transform mat;
	if (!mParticlesAreGlobal)
		mat.SetTranslation(toWorld.GetTranslation());
	//Renderer::Get()->SetTransform(TS_WORLD, mat);

	//Renderer::Get()->SetMaterial(mBuffer->mMaterial);

	Renderer::Get()->DrawVertexPrimitiveList(mBuffer->GetVertices(), mParticles.size()*4,
		mBuffer->GetIndices(), mParticles.size()*2, VT_STANDARD, PT_TRIANGLES, mBuffer->GetIndexType());

	// for debug purposes only:
	if (DebugDataVisible() & DS_BBOX )
	{
		Renderer::Get()->SetTransform(TS_WORLD, toWorld);
		Material debM;
		debM.mLighting = false;
		Renderer::Get()->SetMaterial(debM);
		Renderer::Get()->Draw3DBox(mBuffer->mBoundingBox, eastl::array<float, 4>{0.f, 255.f, 255.f, 255.f});
	}

	return Node::Render(pScene);
}


void ParticleSystemNode::DoParticleSystem(unsigned int time)
{
	Matrix4x4<float> toWorld, fromWorld;
	//Transform(&toWorld, &fromWorld);

	if (mLastEmitTime==0)
	{
		mLastEmitTime = time;
		return;
	}

	unsigned int now = time;
	unsigned int timediff = time - mLastEmitTime;
	mLastEmitTime = time;

	// run emitter

	if (mEmitter && IsVisible())
	{
		Particle* array = 0;
		int newParticles = mEmitter->Emitt(now, timediff, array);

		if (newParticles && array)
		{
			int j = mParticles.size();
			if (newParticles > 16250-j)
				newParticles=16250-j;

			mParticles.resize(j+newParticles);
			for (int i=j; i<j+newParticles; ++i)
			{
				mParticles[i]=array[i-j];
				toWorld.RotateVect(mParticles[i].mStartVector);
				if (mParticlesAreGlobal)
					toWorld.TransformVect(mParticles[i].mPos);
			}
		}
	}

	// run affectors
	eastl::list<eastl::shared_ptr<BaseParticleAffector>>::iterator ait = mAffectorList.begin();
	for (; ait != mAffectorList.end(); ++ait)
		(*ait)->Affect(now, mParticles.data(), mParticles.size());

	if (mParticlesAreGlobal)
		mBuffer->mBoundingBox.Reset(toWorld.GetTranslation());
	else
		mBuffer->mBoundingBox.Reset(Vector3<float>{0.f, 0.f, 0.f});

	// animate all particles
	float scale = (float)timediff;

	for (unsigned int i=0; i<mParticles.size();)
	{
		// erase is pretty expensive!
		if (now > mParticles[i].mEndTime)
		{
			// Particle order does not seem to matter.
			// So we can delete by switching with last particle and deleting that one.
			// This is a lot faster and speed is very important here as the erase otherwise
			// can cause noticable freezes.
			mParticles[i] = mParticles[mParticles.size()-1];
			mParticles.pop_back();
		}
		else
		{
			mParticles[i].mPos += (mParticles[i].mVector * scale);
			mBuffer->mBoundingBox.AddInternalPoint(mParticles[i].mPos);
			++i;
		}
	}

	const float m = (mParticleSize[0] > mParticleSize[1] ? mParticleSize[0] : mParticleSize[1]) * 0.5f;
	mBuffer->mBoundingBox.MaxEdge.X += m;
	mBuffer->mBoundingBox.MaxEdge.Y += m;
	mBuffer->mBoundingBox.MaxEdge.Z += m;

	mBuffer->mBoundingBox.MinEdge.X -= m;
	mBuffer->mBoundingBox.MinEdge.Y -= m;
	mBuffer->mBoundingBox.MinEdge.Z -= m;

	if (mParticlesAreGlobal)
	{
		//Matrix4x4<float> absinv( toWorld, Matrix4x4<float>::EM4CONST_INVERSE );
		fromWorld.TransformBoxEx(mBuffer->mBoundingBox);
	}
}

//! Sets if the particles should be global. If it is, the particles are affected by
//! the movement of the particle system scene node too, otherwise they completely
//! ignore it. Default is true.
void ParticleSystemNode::SetParticlesAreGlobal(bool global)
{
	mParticlesAreGlobal = global;
}

//! Remove all currently visible particles
void ParticleSystemNode::ClearParticles()
{
	mParticles.clear();
}

void ParticleSystemNode::ReallocateBuffers()
{
	if (mParticles.size() * 4 > mBuffer->GetVertexCount() ||
		mParticles.size() * 6 > mBuffer->GetIndexCount())
	{
		unsigned int oldSize = mBuffer->GetVertexCount();
		mBuffer->mVertices.resize(mParticles.size() * 4);

		unsigned int i;

		// fill remaining vertices
		for (i=oldSize; i<mBuffer->mVertices.size(); i+=4)
		{
			mBuffer->mVertices[0+i].mTCoords.set(0.0f, 0.0f);
			mBuffer->mVertices[1+i].mTCoords.set(0.0f, 1.0f);
			mBuffer->mVertices[2+i].mTCoords.set(1.0f, 1.0f);
			mBuffer->mVertices[3+i].mTCoords.set(1.0f, 0.0f);
		}

		// fill remaining indices
		unsigned int oldIdxSize = mBuffer->GetIndexCount();
		unsigned int oldvertices = oldSize;
		mBuffer->mIndices.resize(mParticles.size() * 6);

		for (i=oldIdxSize; i<mBuffer->mIndices.size(); i+=6)
		{
			mBuffer->mIndices[0+i] = (unsigned int)0+oldvertices;
			mBuffer->mIndices[1+i] = (unsigned int)2+oldvertices;
			mBuffer->mIndices[2+i] = (unsigned int)1+oldvertices;
			mBuffer->mIndices[3+i] = (unsigned int)0+oldvertices;
			mBuffer->mIndices[4+i] = (unsigned int)3+oldvertices;
			mBuffer->mIndices[5+i] = (unsigned int)2+oldvertices;
			oldvertices += 4;
		}
	}
}

//! Gets the particle emitter, which creates the particles.
eastl::shared_ptr<BaseParticleEmitter> ParticleSystemNode::GetEmitter()
{
	return mEmitter;
}


//! Sets the particle emitter, which creates the particles.
void ParticleSystemNode::SetEmitter(const eastl::shared_ptr<BaseParticleEmitter>& emitter)
{
	mEmitter = emitter;
}


//! Adds new particle effector to the particle system.
void ParticleSystemNode::AddAffector(const eastl::shared_ptr<BaseParticleAffector>& affector)
{
	mAffectorList.push_back(affector);
}

//! Get a list of all particle affectors.
const eastl::list<eastl::shared_ptr<BaseParticleAffector>>& ParticleSystemNode::GetAffectors() const
{
	return mAffectorList;
}

//! Removes all particle affectors in the particle system.
void ParticleSystemNode::RemoveAllAffectors()
{
	eastl::list<eastl::shared_ptr<BaseParticleAffector>>::iterator it = mAffectorList.begin();
	while (it != mAffectorList.end())
		it = mAffectorList.erase(it);
}


//! Returns the material based on the zero based index i.
eastl::shared_ptr<Material> const& ParticleSystemNode::GetMaterial(unsigned int i)
{
	return mBuffer->mMaterial;
}


//! Returns amount of materials used by this scene node.
unsigned int ParticleSystemNode::GetMaterialCount() const
{
	return 1;
}


//! Creates a particle emitter for an animated mesh scene node
ParticleAnimatedMeshNodeEmitter* ParticleSystemNode::CreateAnimatedMeshNodeEmitter(const ActorId actorId,
	const eastl::shared_ptr<AnimatedMeshNode>& node, bool useNormalDirection, const Vector3<float>& direction, 
	float normalDirectionModifier, int mbNumber, bool everyMeshVertex, unsigned int minParticlesPerSecond, 
	unsigned int maxParticlesPerSecond, const eastl::array<float, 4>& minStartColor, const eastl::array<float, 4>& maxStartColor, 
	unsigned int lifeTimeMin, unsigned int lifeTimeMax, int maxAngleDegrees, 
	const Vector2<float>& minStartSize, const Vector2<float>& maxStartSize )
{
	
	return new ParticleAnimatedMeshNodeEmitter(actorId, WeakBaseRenderComponentPtr(),
		node, useNormalDirection, direction, normalDirectionModifier, mbNumber, everyMeshVertex, 
		minParticlesPerSecond, maxParticlesPerSecond, minStartColor, maxStartColor,
		lifeTimeMin, lifeTimeMax, maxAngleDegrees, minStartSize, maxStartSize );
}


//! Creates a box particle emitter.
ParticleBoxEmitter* ParticleSystemNode::CreateBoxEmitter(const AlignedBox3<float>& box,
	const Vector3<float>& direction, unsigned int minParticlesPerSecond, unsigned int maxParticlesPerSecond, 
	const eastl::array<float, 4>& minStartColor, const eastl::array<float, 4>& maxStartColor, unsigned int lifeTimeMin, 
	unsigned int lifeTimeMax, int maxAngleDegrees, const Vector2<float>& minStartSize, const Vector2<float>& maxStartSize )
{
	return new ParticleBoxEmitter(box, direction, minParticlesPerSecond, maxParticlesPerSecond, 
		minStartColor, maxStartColor, lifeTimeMin, lifeTimeMax, maxAngleDegrees, minStartSize, maxStartSize );
}


//! Creates a particle emitter for emitting from a cylinder
ParticleCylinderEmitter* ParticleSystemNode::CreateCylinderEmitter(
	const Vector3<float>& center, float radius, const Vector3<float>& normal, float length, bool outlineOnly, 
	const Vector3<float>& direction, unsigned int minParticlesPerSecond, unsigned int maxParticlesPerSecond,
	const eastl::array<float, 4>& minStartColor, const eastl::array<float, 4>& maxStartColor, unsigned int lifeTimeMin, 
	unsigned int lifeTimeMax, int maxAngleDegrees, const Vector2<float>& minStartSize, const Vector2<float>& maxStartSize )
{
	return new ParticleCylinderEmitter( center, radius, normal, length, outlineOnly, direction, minParticlesPerSecond, 
		maxParticlesPerSecond, minStartColor, maxStartColor, lifeTimeMin, lifeTimeMax, maxAngleDegrees, minStartSize, maxStartSize );
}


//! Creates a mesh particle emitter.
ParticleMeshEmitter* ParticleSystemNode::CreateMeshEmitter(
	const eastl::shared_ptr<BaseMesh>& mesh, bool useNormalDirection, const Vector3<float>& direction, 
	float normalDirectionModifier, int mbNumber, bool everyMeshVertex, unsigned int minParticlesPerSecond, 
	unsigned int maxParticlesPerSecond, const eastl::array<float, 4>& minStartColor, const eastl::array<float, 4>& maxStartColor, 
	unsigned int lifeTimeMin, unsigned int lifeTimeMax, int maxAngleDegrees, const Vector2<float>& minStartSize, 
	const Vector2<float>& maxStartSize)
{
	return new ParticleMeshEmitter( mesh, useNormalDirection, direction, normalDirectionModifier, mbNumber, everyMeshVertex, 
		minParticlesPerSecond, maxParticlesPerSecond, minStartColor, maxStartColor, lifeTimeMin, lifeTimeMax, maxAngleDegrees, 
		minStartSize, maxStartSize );
}


//! Creates a point particle emitter.
ParticlePointEmitter* ParticleSystemNode::CreatePointEmitter(
	const Vector3<float>& direction, unsigned int minParticlesPerSecond, unsigned int maxParticlesPerSecond,
	const eastl::array<float, 4>& minStartColor, const eastl::array<float, 4>& maxStartColor, unsigned int lifeTimeMin, 
	unsigned int lifeTimeMax, int maxAngleDegrees, const Vector2<float>& minStartSize, const Vector2<float>& maxStartSize )
{
	return new ParticlePointEmitter(direction, minParticlesPerSecond, maxParticlesPerSecond, 
		minStartColor, maxStartColor, lifeTimeMin, lifeTimeMax, maxAngleDegrees, minStartSize, maxStartSize );
}


//! Creates a ring particle emitter.
ParticleRingEmitter* ParticleSystemNode::CreateRingEmitter(
	const Vector3<float>& center, float radius, float ringThickness, const Vector3<float>& direction,
	unsigned int minParticlesPerSecond, unsigned int maxParticlesPerSecond, const eastl::array<float, 4>& minStartColor, 
	const eastl::array<float, 4>& maxStartColor, unsigned int lifeTimeMin, unsigned int lifeTimeMax, int maxAngleDegrees, 
	const Vector2<float>& minStartSize, const Vector2<float>& maxStartSize )
{
	return new ParticleRingEmitter(center, radius, ringThickness, direction, minParticlesPerSecond, maxParticlesPerSecond, 
		minStartColor, maxStartColor, lifeTimeMin, lifeTimeMax, maxAngleDegrees, minStartSize, maxStartSize );
}


//! Creates a sphere particle emitter.
ParticleSphereEmitter* ParticleSystemNode::CreateSphereEmitter(
	const Vector3<float>& center, float radius, const Vector3<float>& direction, unsigned int minParticlesPerSecond, 
	unsigned int maxParticlesPerSecond, const eastl::array<float, 4>& minStartColor, const eastl::array<float, 4>& maxStartColor,
	unsigned int lifeTimeMin, unsigned int lifeTimeMax, int maxAngleDegrees, const Vector2<float>& minStartSize,
	const Vector2<float>& maxStartSize )
{
	return new ParticleSphereEmitter(center, radius, direction, minParticlesPerSecond, maxParticlesPerSecond, minStartColor, 
		maxStartColor, lifeTimeMin, lifeTimeMax, maxAngleDegrees, minStartSize, maxStartSize );
}


//! Creates a point attraction affector. This affector modifies the positions of the
//! particles and attracts them to a specified point at a specified speed per second.
ParticleAttractionAffector* ParticleSystemNode::CreateAttractionAffector(
	const Vector3<float>& point, float speed, bool attract, bool affectX, bool affectY, bool affectZ )
{
	return new ParticleAttractionAffector(point, speed, attract, affectX, affectY, affectZ );
}

//! Creates a scale particle affector.
BaseParticleAffector* ParticleSystemNode::CreateScaleParticleAffector(const Vector2<float>& scaleTo)
{
	return new ParticleScaleAffector(scaleTo);
}


//! Creates a fade out particle affector.
ParticleFadeOutAffector* ParticleSystemNode::CreateFadeOutParticleAffector(
	const eastl::array<float, 4>& targetColor, unsigned int timeNeededToFadeOut)
{
	return new ParticleFadeOutAffector(targetColor, timeNeededToFadeOut);
}


//! Creates a gravity affector.
ParticleGravityAffector* ParticleSystemNode::CreateGravityAffector(
	const Vector3<float>& gravity, unsigned int timeForceLost)
{
	return new ParticleGravityAffector(gravity, timeForceLost);
}


//! Creates a rotation affector. This affector rotates the particles around a specified pivot
//! point.  The speed represents Degrees of rotation per second.
ParticleRotationAffector* ParticleSystemNode::CreateRotationAffector(
	const Vector3<float>& speed, const Vector3<float>& pivotPoint )
{
	return new ParticleRotationAffector(speed, pivotPoint);
}

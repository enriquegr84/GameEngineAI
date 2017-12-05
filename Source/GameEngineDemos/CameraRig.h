// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.0 (2016/06/19)

#ifndef CAMERARIG_H
#define CAMERARIG_H

#include "Graphic/Scene/Hierarchy/Camera.h"
#include "Graphic/Resource/Buffer/ConstantBuffer.h"

class CameraRig
{
public:
    // Construction.  The camera rig is designed for moving the camera around
    // in a world coordinate system.  This object is used in the sample 3D
    // applications.
    virtual ~CameraRig();
    CameraRig();
    CameraRig(eastl::shared_ptr<Camera> const& camera, float translationSpeed, float rotationSpeed);

    // Member access.  The functions are for deferred construction after
    // a default construction of a camera rig.
    void Set(eastl::shared_ptr<Camera> const& camera, float translationSpeed, float rotationSpeed);

    // TODO:  The camera vectors are the world axes?  (INVARIANT)
    void ComputeWorldAxes();

    inline eastl::shared_ptr<Camera> const& GetCamera() const;
    inline void SetTranslationSpeed(float translationSpeed);
    inline float GetTranslationSpeed() const;
    inline void SetRotationSpeed(float rotationSpeed);
    inline float GetRotationSpeed() const;

    // Control of camera motion.  If the camera moves, subscribers to the
    // pvw-matrix update will have the system memory and GPU memory of the
    // constant buffers updated.  Only one motion may be active at a single
    // time.  When a motion is active, a call to Move() will execute that
    // motion.

    // The motion is controlled directly by calling SetDirect*().
    inline void SetDirectMoveForward();
    inline void SetDirectMoveBackward();
    inline void SetDirectMoveUp();
    inline void SetDirectMoveDown();
    inline void SetDirectMoveRight();
    inline void SetDirectMoveLeft();
    inline void SetDirectTurnRight();
    inline void SetDirectTurnLeft();
    inline void SetDirectLookUp();
    inline void SetDirectLookDown();
    inline void SetDirectRollClockwise();
    inline void SetDirectRollCounterclockwise();

    // The motion is controlled indirectly.  TheRegister* calls map the
    // 'trigger' to the function specified by the *-suffix.  If trigger >= 0,
    // the function is added to a map.  If trigger < 0 in the Register*
    // function, the corresponding function is removed from the map.  A call
    // to PushMotion(trigger) will set the active motion if the trigger
    // is currently mapped; the Boolean return is 'true' iff the trigger is
    // mapped.  A call to PopMotion(trigger) will disable the motion if the
    // trigger is currently mapped; the Boolean return it 'true' iff the
    // trigger is currently mapped.
    inline void RegisterMoveForward(int trigger);
    inline void RegisterMoveBackward(int trigger);
    inline void RegisterMoveUp(int trigger);
    inline void RegisterMoveDown(int trigger);
    inline void RegisterMoveRight(int trigger);
    inline void RegisterMoveLeft(int trigger);
    inline void RegisterTurnRight(int trigger);
    inline void RegisterTurnLeft(int trigger);
    inline void RegisterLookUp(int trigger);
    inline void RegisterLookDown(int trigger);
    inline void RegisterRollClockwise(int trigger);
    inline void RegisterRollCounterclockwise(int trigger);
    bool PushMotion(int trigger);
    bool PopMotion(int trigger);

    bool Move();
    void ClearMotions();

protected:
    // Camera motion.  These are called based on the state set in the
    // public interface.
    enum { MAX_ACTIVE_MOTIONS = 12 };
    virtual void MoveForward();
    virtual void MoveBackward();
    virtual void MoveUp();
    virtual void MoveDown();
    virtual void MoveRight();
    virtual void MoveLeft();
    virtual void TurnRight();
    virtual void TurnLeft();
    virtual void LookUp();
    virtual void LookDown();
    virtual void RollClockwise();
    virtual void RollCounterclockwise();

    typedef void(CameraRig::*MoveFunction)(void);
    void Register(int trigger, MoveFunction function);

    bool SetActive(MoveFunction function);
    bool SetInactive(MoveFunction function);

	eastl::shared_ptr<Camera> mCamera;
    float mTranslationSpeed, mRotationSpeed;

    // Maintain the world coordinate system.
    Vector4<float> mWorldAxis[3];

    // Move via direct or indirect triggers.
    MoveFunction mMotion;
	eastl::map<int, MoveFunction> mIndirectMap;
    int mNumActiveMotions;
	eastl::array<MoveFunction, MAX_ACTIVE_MOTIONS> mActiveMotions;
};


inline eastl::shared_ptr<Camera> const& CameraRig::GetCamera() const
{
    return mCamera;
}

inline void CameraRig::SetTranslationSpeed(float translationSpeed)
{
    mTranslationSpeed = translationSpeed;
}

inline float CameraRig::GetTranslationSpeed() const
{
    return mTranslationSpeed;
}

inline void CameraRig::SetRotationSpeed(float rotationSpeed)
{
    mRotationSpeed = rotationSpeed;
}

inline float CameraRig::GetRotationSpeed() const
{
    return mRotationSpeed;
}

inline void CameraRig::SetDirectMoveForward()
{
    mMotion = &CameraRig::MoveForward;
}

inline void CameraRig::SetDirectMoveBackward()
{
    mMotion = &CameraRig::MoveBackward;
}

inline void CameraRig::SetDirectMoveUp()
{
    mMotion = &CameraRig::MoveUp;
}

inline void CameraRig::SetDirectMoveDown()
{
    mMotion = &CameraRig::MoveDown;
}

inline void CameraRig::SetDirectMoveRight()
{
    mMotion = &CameraRig::MoveRight;
}

inline void CameraRig::SetDirectMoveLeft()
{
    mMotion = &CameraRig::MoveLeft;
}

inline void CameraRig::SetDirectTurnRight()
{
    mMotion = &CameraRig::TurnRight;
}

inline void CameraRig::SetDirectTurnLeft()
{
    mMotion = &CameraRig::TurnLeft;
}

inline void CameraRig::SetDirectLookUp()
{
    mMotion = &CameraRig::LookUp;
}

inline void CameraRig::SetDirectLookDown()
{
    mMotion = &CameraRig::LookDown;
}

inline void CameraRig::SetDirectRollClockwise()
{
    mMotion = &CameraRig::RollClockwise;
}

inline void CameraRig::SetDirectRollCounterclockwise()
{
    mMotion = &CameraRig::RollCounterclockwise;
}

inline void CameraRig::RegisterMoveForward(int trigger)
{
    Register(trigger, &CameraRig::MoveForward);
}

inline void CameraRig::RegisterMoveBackward(int trigger)
{
    Register(trigger, &CameraRig::MoveBackward);
}

inline void CameraRig::RegisterMoveUp(int trigger)
{
    Register(trigger, &CameraRig::MoveUp);
}

inline void CameraRig::RegisterMoveDown(int trigger)
{
    Register(trigger, &CameraRig::MoveDown);
}

inline void CameraRig::RegisterMoveRight(int trigger)
{
    Register(trigger, &CameraRig::MoveRight);
}

inline void CameraRig::RegisterMoveLeft(int trigger)
{
    Register(trigger, &CameraRig::MoveLeft);
}

inline void CameraRig::RegisterTurnRight(int trigger)
{
    Register(trigger, &CameraRig::TurnRight);
}

inline void CameraRig::RegisterTurnLeft(int trigger)
{
    Register(trigger, &CameraRig::TurnLeft);
}

inline void CameraRig::RegisterLookUp(int trigger)
{
    Register(trigger, &CameraRig::LookUp);
}

inline void CameraRig::RegisterLookDown(int trigger)
{
    Register(trigger, &CameraRig::LookDown);
}

inline void CameraRig::RegisterRollClockwise(int trigger)
{
    Register(trigger, &CameraRig::RollClockwise);
}

inline void CameraRig::RegisterRollCounterclockwise(int trigger)
{
    Register(trigger, &CameraRig::RollCounterclockwise);
}

#endif
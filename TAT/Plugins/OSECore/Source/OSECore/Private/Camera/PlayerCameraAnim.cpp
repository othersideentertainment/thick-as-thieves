// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Camera/PlayerCameraAnim.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PlayerCameraAnim)

FPlayerCameraSocket::FPlayerCameraSocket(
   const FName SocketName,
   const FRotator& RelRotation /*= FRotator::ZeroRotator*/,
   const FVector& RelLocation /*= FVector::ZeroVector*/)
   : Name(SocketName)
   , Location(RelLocation)
   , Rotation(RelRotation)
{

}

FPlayerCameraAnimation::FPlayerCameraAnimation(float InPosWeight, float InRotWeight, float InDirOverride, float InCenterAimOverride)
   : PositionWeight(InPosWeight)
   , RotationWeight(InRotWeight)
   , ForwardDirectionOverride(InDirOverride)
   , CenterAimOverride(InCenterAimOverride)
{

}


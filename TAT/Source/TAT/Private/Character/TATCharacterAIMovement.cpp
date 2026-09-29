// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Character/TATCharacterAIMovement.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCharacterAIMovement)

void UTATCharacterAIMovement::SetOverrideBaseMaxWalkSpeed(float baseMaxSpeed)
{
   checkf(baseMaxSpeed >= 0.0f, TEXT("%s | SetOverrideBaseMaxWalkSpeed called with a negative baseMaxSpeed!"), *GetOwner()->GetActorNameOrLabel());
   _baseMaxWalkSpeedOverride = baseMaxSpeed;
}

float UTATCharacterAIMovement::GetOverrideBaseMaxWalkSpeed() const
{
   return _baseMaxWalkSpeedOverride.Get(-1.0f);
}

void UTATCharacterAIMovement::ClearOverrideBaseMaxWalkSpeed()
{
   _baseMaxWalkSpeedOverride.Reset();
}

float UTATCharacterAIMovement::GetBaseMaxSpeed() const
{
   if (_baseMaxWalkSpeedOverride.IsSet() && (_onlyOverrideSpeedIfMovingOnFloor == false || IsMovingOnGround()))
   {
      return _baseMaxWalkSpeedOverride.GetValue();
   }

   return Super::GetBaseMaxSpeed();
}

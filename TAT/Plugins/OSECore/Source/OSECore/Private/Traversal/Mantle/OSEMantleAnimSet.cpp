// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Traversal/Mantle/OSEMantleAnimSet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEMantleAnimSet)


namespace MantleCVars
{
   static float HeightCompareBuffer = 5.0f;
   FAutoConsoleVariableRef CVarHeightCompareBuffer(
      TEXT("OSE.Mantle.HeightCompareBuffer"),
      HeightCompareBuffer,
      TEXT("Buffer to apply to height values before comparing to the animation set."),
      ECVF_Default);
}


bool UOSEMantleAnimSet::FindAnimation(FOSEMantleAnim& outAnim, const FOSEMantleAnimSearchParams& params) const
{
   outAnim = FOSEMantleAnim();

   if (Animations.Num() == 0)
      return false;

   switch (RequiredMode)
   {
   case EOSEMantleAnimMovement::MovingOnGround:
   {
      if ((params.Movement != EMovementMode::MOVE_Walking) && (params.Movement != EMovementMode::MOVE_NavWalking))
         return false;
   }
   break;

   case EOSEMantleAnimMovement::Falling:
   {
      if (params.Movement != EMovementMode::MOVE_Falling)
         return false;
   }
   break;

   case EOSEMantleAnimMovement::Scrambling:
   {
      if (params.CustomType != ECustomMovementType::Scramble)
         return false;
   }
   break;

   case EOSEMantleAnimMovement::WallClimb:
   {
      if (params.CustomType != ECustomMovementType::WallClimb)
         return false;
   }
   break;

   default:
      break;
   }

   // Should this be a bool rather than a value in the enum?
   if ((RequiredMode == EOSEMantleAnimMovement::Crouching) != params.IsCrouching)
   {
      return false;
   }

   if (params.Speed < RequiredSpeed)
      return false;

   TArray<FOSEMantleAnim> sortedAnims = Animations;
   sortedAnims.Sort([](const FOSEMantleAnim& A, const FOSEMantleAnim& B) { return A.Height > B.Height; });

   // Sorted min/max height with buffer
   const float setMaxHeight = +MantleCVars::HeightCompareBuffer + sortedAnims[0].Height;
   const float setMinHeight = -MantleCVars::HeightCompareBuffer + sortedAnims.Last().Height;
   if ((params.Height > setMaxHeight) || (params.Height < setMinHeight))
      return false;

   for (const auto& item : sortedAnims)
   {
      if (!item.IsValid())
         continue;

      if ((params.Height + MantleCVars::HeightCompareBuffer) >= item.Height)
      {
         outAnim = item;
         return true;
      }
   }

   return false;
}


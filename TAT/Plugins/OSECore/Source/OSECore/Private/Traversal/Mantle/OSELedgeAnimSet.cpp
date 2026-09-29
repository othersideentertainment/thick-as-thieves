// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Traversal/Mantle/OSELedgeAnimSet.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(OSELedgeAnimSet)

namespace LedgeCVars
{
   static float HeightCompareBuffer = 5.0f;
   FAutoConsoleVariableRef CVarHeightCompareBuffer(
      TEXT("OSE.Ledge.HeightCompareBuffer"),
      HeightCompareBuffer,
      TEXT("Buffer to apply to height values before comparing to the animation set."),
      ECVF_Default);
}


bool UOSELedgeAnimSet::FindAnimation(FOSELedgeAnim& outAnim,
   EMovementMode inMovement, ECustomMovementType inCustomType,
   float inSpeed, float ledgeHeight, bool allowEject) const
{
   outAnim = FOSELedgeAnim();

   if (Animations.Num() == 0)
      return false;

   switch (RequiredMode)
   {
   case EOSELedgeAnimMovement::MovingOnGround:
   {
      if ((inMovement != EMovementMode::MOVE_Walking) && (inMovement != EMovementMode::MOVE_NavWalking))
         return false;
   }
   break;

   case EOSELedgeAnimMovement::Falling:
   {
      if (inMovement != EMovementMode::MOVE_Falling)
         return false;
   }
   break;

   case EOSELedgeAnimMovement::Scrambling:
   {
      if (inCustomType != ECustomMovementType::Scramble)
         return false;
   }
   break;
   case EOSELedgeAnimMovement::Ledge:
   {
      if (inCustomType != ECustomMovementType::Mantle)
         return false;
   }
   break;
   case EOSELedgeAnimMovement::WallClimb:
   {
      if (inCustomType != ECustomMovementType::WallClimb)
         return false;
   }
   break;
   default:
      break;
   }

   if (inSpeed < RequiredSpeed)
      return false;

   TArray<FOSELedgeAnim> sortedAnims = Animations;
   sortedAnims.Sort([](const FOSELedgeAnim& A, const FOSELedgeAnim& B) { return A.HeightRange.Max > B.HeightRange.Max; });

   // Sorted min/max height with buffer
   const float setMaxHeight = sortedAnims[0].HeightRange.Max;
   const float setMinHeight = sortedAnims.Last().HeightRange.Min;
   if ((ledgeHeight > setMaxHeight) || (ledgeHeight < setMinHeight))
      return false;

   for (const auto& item : sortedAnims)
   {
      if (!item.IsValid())
         continue;
      if(item.AutoEject && !allowEject)
         continue;

      if (item.HeightRange.Contains(ledgeHeight))
      {
         outAnim = item;
         return true;
      }
   }

   return false;
}

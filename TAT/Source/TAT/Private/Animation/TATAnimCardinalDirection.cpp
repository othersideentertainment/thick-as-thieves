// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Animation/TATAnimCardinalDirection.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAnimCardinalDirection)

ETATAnimCardinalDirection UTATAnimCardinalDirectionUtils::SelectCardinalDirectionFromAngle(float angle, float deadZone,
   ETATAnimCardinalDirection currentDirection, bool favorCurrentDirection)
{
   float forwardDeadZone = deadZone;
   float backwardDeadZone = deadZone;

   if(favorCurrentDirection)
   {
      if(currentDirection == ETATAnimCardinalDirection::Forward)
      {
         // If moving Fwd, double the Fwd dead zone.
         // It should be harder to leave Fwd when moving Fwd.
         // When moving Left/Right, the dead zone will be smaller so we don't rapidly toggle between directions.
         forwardDeadZone *= 2;
      }
      else if(currentDirection == ETATAnimCardinalDirection::Backward)
      {
         backwardDeadZone *= 2;
      }
   }

   float absAngle = FMath::Abs(angle);
   if(absAngle <= 45 + forwardDeadZone)
   {
      return ETATAnimCardinalDirection::Forward;
   }
   else if(absAngle >= 135 - backwardDeadZone)
   {
      return ETATAnimCardinalDirection::Backward;
   }
   else if(angle > 0)
   {
      return ETATAnimCardinalDirection::Right;
   }
   else
   {
      return ETATAnimCardinalDirection::Left;
   }
}

ETATAnimCardinalDirection UTATAnimCardinalDirectionUtils::GetOppositeDirection(ETATAnimCardinalDirection direction)
{
   switch (direction)
   {
      case ETATAnimCardinalDirection::Forward:
         return ETATAnimCardinalDirection::Backward;
      case ETATAnimCardinalDirection::Backward:
         return ETATAnimCardinalDirection::Forward;
      case ETATAnimCardinalDirection::Left:
         return ETATAnimCardinalDirection::Right;
      case ETATAnimCardinalDirection::Right:
         return ETATAnimCardinalDirection::Left;
      default:
         return ETATAnimCardinalDirection::Forward;
   }
}

// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Traversal/OSEDistanceConstraint.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEDistanceConstraint)

EDistanceConstraintDelta FOSEDistanceConstraint::GetDeltaTo(const FOSEDistanceConstraint& other) const
{
   const bool coreDiffers =
      (other.Position != Position) ||
      (other.Distance != Distance) ||
      (other.SlackAmount != SlackAmount) ||
      (other.SlackUsed != SlackUsed) ||
      (other.Enabled != Enabled);
   const bool configDiffers =
      (other.MinLength != MinLength) ||
      (other.ContractSpeed != ContractSpeed) ||
      (other.SlackVelocityAir != SlackVelocityAir) ||
      (other.SlackVelocityGround != SlackVelocityGround) ||
      (other.BrakingDeceleration != BrakingDeceleration);

   if (!coreDiffers && !configDiffers)
   {
      return EDistanceConstraintDelta::None;
   }
   else if (!configDiffers)
   {
      return EDistanceConstraintDelta::Partial;
   }
   else
   {
      return EDistanceConstraintDelta::Full;
   }
}

void FOSEDistanceConstraint::Apply(EDistanceConstraintDelta type, const FOSEDistanceConstraint& other)
{
   switch (type)
   {
   case EDistanceConstraintDelta::Full:
      *this = other;
      break;
   case EDistanceConstraintDelta::Partial:
      Enabled = other.Enabled;
      Position = other.Position;
      Distance = other.Distance;
      SlackAmount = other.SlackAmount;
      SlackUsed = other.SlackUsed;
      break;
   }
}

void FOSEDistanceConstraint::SerializeDelta(EDistanceConstraintDelta type, FArchive& ar)
{
   switch (type)
   {
   case EDistanceConstraintDelta::Full:
      SerializeFull(ar);
      break;
   case EDistanceConstraintDelta::Partial:
      SerializePartial(ar);
      break;
   }
}

void FOSEDistanceConstraint::SerializeFull(FArchive& ar)
{
   SerializePartial(ar);
   if (Enabled)
   {
      ar << MinLength;
      ar << ContractSpeed;
      ar << SlackVelocityAir;
      ar << SlackVelocityGround;
      ar << BrakingDeceleration;
   }
}

void FOSEDistanceConstraint::SerializePartial(FArchive& ar)
{
   ar << Enabled;
   if (Enabled)
   {
      ar << Position;
      ar << Distance;
      ar << SlackAmount;
      ar << SlackUsed;
   }
}


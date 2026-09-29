// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// OSE
#include "Character/OSECharacterMovementReplication.h"
#include "Character/OSECharacterMovement.h"

void FOSECharacterNetworkMoveData::ClientFillNetworkMoveData(const FSavedMove_Character& clientMove, ENetworkMoveType moveType)
{
   Super::ClientFillNetworkMoveData(clientMove, moveType);

   const auto& oseMove = static_cast<const FOSESavedMove_Character&>(clientMove);
   bExtendingDistanceConstraint = oseMove.bExtendingDistanceConstraint;
   bWantsToContractDistanceConstraint = oseMove.bWantsToContractDistanceConstraint;
   MaxSpeedMultiplier = oseMove.MaxSpeedMultiplier;
   DistanceConstraintDelta = oseMove.DistanceConstraintDelta;
   DistanceConstraint = oseMove.DistanceConstraint;
   ExtraGravityScale = oseMove.ExtraGravityScale;
   bWantsToWallClimb = oseMove.bWantsToWallClimb;
}

bool FOSECharacterNetworkMoveData::Serialize(UCharacterMovementComponent& characterMovement, FArchive& ar, UPackageMap* packageMap, ENetworkMoveType moveType)
{
   Super::Serialize(characterMovement, ar, packageMap, moveType);

   const bool isLoading = ar.IsLoading();

   // Distance constraint changes should be rare most of the time, so only use 1 bit
   // if there is none, at the expense of the extra bit if there are some
   bool anyDistanceConstraintChanges = DistanceConstraintDelta != EDistanceConstraintDelta::None || bWantsToContractDistanceConstraint || bExtendingDistanceConstraint;
   ar.SerializeBits(&anyDistanceConstraintChanges, 1);
   if (anyDistanceConstraintChanges)
   {
      // extending and controlling are mutually exclusive, so only use 1 bit if neither is active
      bool mutatingDistanceConstraint = bWantsToContractDistanceConstraint || bExtendingDistanceConstraint;
      ar.SerializeBits(&mutatingDistanceConstraint, 1);
      if (mutatingDistanceConstraint)
      {
         uint8 wantsToContactDistanceConstraint = bWantsToContractDistanceConstraint;
         ar.SerializeBits(&wantsToContactDistanceConstraint, 1);
         bWantsToContractDistanceConstraint = wantsToContactDistanceConstraint & 1;
         bExtendingDistanceConstraint = !bWantsToContractDistanceConstraint;
      }
      else
      {
         bWantsToContractDistanceConstraint = false;
         bExtendingDistanceConstraint = false;
      }

      // Serialize distance constraint as either full or partial
      bool hasDelta = DistanceConstraintDelta != EDistanceConstraintDelta::None;
      ar.SerializeBits(&hasDelta, 1);
      if (hasDelta)
      {
         bool isFull = DistanceConstraintDelta == EDistanceConstraintDelta::Full;
         ar.SerializeBits(&isFull, 1);
         DistanceConstraintDelta = isFull ? EDistanceConstraintDelta::Full : EDistanceConstraintDelta::Partial;
         DistanceConstraint.SerializeDelta(DistanceConstraintDelta, ar);
      }
      else if (isLoading)
      {
         DistanceConstraintDelta = EDistanceConstraintDelta::None;
      }
   }
   else if (isLoading)
   {
      DistanceConstraintDelta = EDistanceConstraintDelta::None;
      bWantsToContractDistanceConstraint = false;
      bExtendingDistanceConstraint = false;
   }

   SerializeOptionalValue<float>(!isLoading, ar, MaxSpeedMultiplier, 1.f);
   SerializeOptionalValue<float>(!isLoading, ar, ExtraGravityScale, 1.f);

   uint8 bTempWantsToWallClimb = bWantsToWallClimb ? 1 : 0;
   ar.SerializeBits(&bTempWantsToWallClimb, 1);
   bWantsToWallClimb = (bTempWantsToWallClimb != 0);

   return !ar.IsError();
}

void FOSECharacterMoveResponseDataContainer::ServerFillResponseData(const UCharacterMovementComponent& characterMovement, const FClientAdjustment& pendingAdjustment)
{
   Super::ServerFillResponseData(characterMovement, pendingAdjustment);

   if (!pendingAdjustment.bAckGoodMove)
   {
      if (auto oseMovement = Cast<UOSECharacterMovement>(&characterMovement))
      {
         bDistanceConstraintCorrection = oseMovement->GetDistanceConstraintEnabled();
         if (bDistanceConstraintCorrection)
         {
            const FOSEDistanceConstraint& distanceConstraint = oseMovement->GetDistanceConstraint();
            DistanceConstraintPosition = distanceConstraint.Position;
            DistanceConstraintDistance = distanceConstraint.Distance;
         }
      }
   }
}

bool FOSECharacterMoveResponseDataContainer::Serialize(UCharacterMovementComponent& characterMovement, FArchive& ar, UPackageMap* packageMap)
{
   if (!Super::Serialize(characterMovement, ar, packageMap))
   {
      return false;
   }

   bool localSuccess = true;
   if (IsCorrection())
   {
      ar.SerializeBits(&bDistanceConstraintCorrection, 1);
      if (bDistanceConstraintCorrection)
      {
         DistanceConstraintPosition.NetSerialize(ar, packageMap, localSuccess);
         ar << DistanceConstraintDistance;
      }
   }

   return !ar.IsError();
}

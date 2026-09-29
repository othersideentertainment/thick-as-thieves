// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Traversal/OSERootMotion.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSERootMotion)


FOSEConstrainDistanceRootMotion::FOSEConstrainDistanceRootMotion()
   : TargetLocation(ForceInitToZero)
   , MaxDistance(0.0f)
{
}

FRootMotionSource* FOSEConstrainDistanceRootMotion::Clone() const
{
   FOSEConstrainDistanceRootMotion* CopyPtr = new FOSEConstrainDistanceRootMotion(*this);
   return CopyPtr;
}

bool FOSEConstrainDistanceRootMotion::Matches(const FRootMotionSource* Other) const
{
   if (!FRootMotionSource::Matches(Other))
   {
      return false;
   }

   // We can cast safely here since in FRootMotionSource::Matches() we ensured ScriptStruct equality
   const FOSEConstrainDistanceRootMotion* OtherCast = static_cast<const FOSEConstrainDistanceRootMotion*>(Other);

   // If these values can be changed dynamically, relying on them for identity is flawed, since they cannot stay exactly in sync
   return true;
   //return TargetLocation.Equals(OtherCast->TargetLocation, 0.1f)
   //   && FMath::IsNearlyEqual(MaxDistance, OtherCast->MaxDistance, 0.1f);
}

bool FOSEConstrainDistanceRootMotion::MatchesAndHasSameState(const FRootMotionSource* Other) const
{
   // Check that it matches
   if (!FRootMotionSource::MatchesAndHasSameState(Other))
   {
      return false;
   }

   return true; // MoveToForce has no unique state
}

bool FOSEConstrainDistanceRootMotion::UpdateStateFrom(const FRootMotionSource* SourceToTakeStateFrom, bool bMarkForSimulatedCatchup)
{
   if (!FRootMotionSource::UpdateStateFrom(SourceToTakeStateFrom, bMarkForSimulatedCatchup))
   {
      return false;
   }

   return true; // MoveToForce has no unique state other than Time which is handled by FRootMotionSource
}

void FOSEConstrainDistanceRootMotion::SetTime(float NewTime)
{
   FRootMotionSource::SetTime(NewTime);

   // TODO-RootMotionSource: Check if reached destination?
}

void FOSEConstrainDistanceRootMotion::PrepareRootMotion(
   float SimulationTime,
   float MovementTickTime,
   const ACharacter& Character,
   const UCharacterMovementComponent& MoveComponent)
{
   RootMotionParams.Clear();
   FVector Force = FVector::ZeroVector;

   if ((MovementTickTime > SMALL_NUMBER) && (MaxDistance > KINDA_SMALL_NUMBER))
   {
      // Current location
      const FVector CurrLocation = Character.GetActorLocation();
      const FVector CurrToTarget = TargetLocation - CurrLocation;

      // Future location based on velocity
      const FVector NextLocation = CurrLocation + (MoveComponent.Velocity * MovementTickTime);
      const FVector NextToTarget = TargetLocation - NextLocation;

      // Use whichever is the further distance
      const float CurrDistSqr = CurrToTarget.SizeSquared();
      const float NextDistSqr = NextToTarget.SizeSquared();
      const float DistSqr = FMath::Max(CurrDistSqr, NextDistSqr);

      // Only change things if we're beyond the distance
      const float MaxDistSqr = MaxDistance * MaxDistance;
      if (DistSqr > MaxDistSqr)
      {
         // The distance we'll need to move
         const float Dist = FMath::Sqrt(DistSqr);
         const float DistDelta = FMath::Max(0.0f, Dist - MaxDistance);

         // New position is on the sphere now
         // Don't modify the Z when walking
         FVector NewPos = CurrLocation + (CurrToTarget.GetSafeNormal() * DistDelta);
         if (MoveComponent.IsMovingOnGround())
            NewPos.Z = CurrLocation.Z;

         // Force to get us to the new position
         Force = (NewPos - CurrLocation) / MovementTickTime;
      }
   }

   FTransform NewTransform(Force);
   RootMotionParams.Set(NewTransform);
   SetTime(GetTime() + SimulationTime);
}

bool FOSEConstrainDistanceRootMotion::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
   if (!FRootMotionSource::NetSerialize(Ar, Map, bOutSuccess))
   {
      return false;
   }

   // TODO-RootMotionSource: Quantization
   Ar << TargetLocation;
   Ar << MaxDistance;

   bOutSuccess = true;
   return true;
}

UScriptStruct* FOSEConstrainDistanceRootMotion::GetScriptStruct() const
{
   return FOSEConstrainDistanceRootMotion::StaticStruct();
}

FString FOSEConstrainDistanceRootMotion::ToSimpleString() const
{
   return FString::Printf(TEXT("[ID:%u]FOSEConstrainDistanceRootMotion %s"), LocalID, *InstanceName.GetPlainNameString());
}

FOSERootMotionSource_ConstantVerticalForce::FOSERootMotionSource_ConstantVerticalForce()
   : VerticalForce(0)
   , StrengthOverTime(nullptr)
   , LateralDamping(0)
   , LateralMovementCoefficient(0)
{
   // Disable Partial End Tick for Constant Forces.
   // Otherwise we end up with very inconsistent velocities on the last frame.
   // This ensures that the ending velocity is maintained and consistent.
   Settings.SetFlag(ERootMotionSourceSettingsFlags::DisablePartialEndTick);
}

FRootMotionSource* FOSERootMotionSource_ConstantVerticalForce::Clone() const
{
   return new FOSERootMotionSource_ConstantVerticalForce(*this);
}

bool FOSERootMotionSource_ConstantVerticalForce::Matches(const FRootMotionSource* Other) const
{
   if (!Super::Matches(Other))
   {
      return false;
   }


   // We can cast safely here since in FRootMotionSource::Matches() we ensured ScriptStruct equality
   const FOSERootMotionSource_ConstantVerticalForce* otherCast = static_cast<const FOSERootMotionSource_ConstantVerticalForce*>(Other);

   return FMath::IsNearlyEqual(VerticalForce, otherCast->VerticalForce) &&
      FMath::IsNearlyEqual(LateralDamping, otherCast->LateralDamping) &&
      FMath::IsNearlyEqual(LateralMovementCoefficient, otherCast->LateralMovementCoefficient) &&
      StrengthOverTime == otherCast->StrengthOverTime;
}

bool FOSERootMotionSource_ConstantVerticalForce::MatchesAndHasSameState(const FRootMotionSource* Other) const
{
   return Super::MatchesAndHasSameState(Other);
}

bool FOSERootMotionSource_ConstantVerticalForce::UpdateStateFrom(const FRootMotionSource* SourceToTakeStateFrom, bool bMarkForSimulatedCatchup)
{
   if (!FRootMotionSource::UpdateStateFrom(SourceToTakeStateFrom, bMarkForSimulatedCatchup))
   {
      return false;
   }

   return true;
}

void FOSERootMotionSource_ConstantVerticalForce::PrepareRootMotion(float SimulationTime, float MovementTickTime, const ACharacter& Character, const UCharacterMovementComponent& MoveComponent)
{
   RootMotionParams.Clear();

   float verticalVelocity = VerticalForce;

   // Scale strength of force over time
   if (StrengthOverTime)
   {
      const float TimeValue = Duration > 0.f ? FMath::Clamp(GetTime() / Duration, 0.f, 1.f) : GetTime();
      const float TimeFactor = StrengthOverTime->GetFloatValue(TimeValue);
      verticalVelocity *= VerticalForce;
   }

   const float Multiplier = (MovementTickTime > SMALL_NUMBER) ? (SimulationTime / MovementTickTime) : 1.f;
   verticalVelocity *= Multiplier;

   // This damping probably isn't stable, but the engine doesn't allow move combining for RMSs
   // So possibly fine if not too much packet loss (and corrections include velocity)
   FVector newVelocity = MoveComponent.Velocity;
   newVelocity *= FMath::Max(1 - MovementTickTime * LateralDamping, 0.f);
   newVelocity.Z = verticalVelocity;

   FVector acceleration = MoveComponent.GetCurrentAcceleration();
   acceleration.Z = 0;
   acceleration *= MovementTickTime * LateralMovementCoefficient;
   newVelocity += acceleration;

   FTransform newTransform(newVelocity);
   RootMotionParams.Set(newTransform);

   SetTime(GetTime() + SimulationTime);
}

bool FOSERootMotionSource_ConstantVerticalForce::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
   if (!Super::NetSerialize(Ar, Map, bOutSuccess))
   {
      return false;
   }

   Ar << VerticalForce;
   Ar << StrengthOverTime;
   Ar << LateralDamping;
   Ar << LateralMovementCoefficient;

   bOutSuccess = true;
   return true;
}

UScriptStruct* FOSERootMotionSource_ConstantVerticalForce::GetScriptStruct() const
{
   return FOSERootMotionSource_ConstantVerticalForce::StaticStruct();
}

FString FOSERootMotionSource_ConstantVerticalForce::ToSimpleString() const
{
   return FString::Printf(TEXT("[ID:%u]FOSERootMotionSource_ConstantVerticalForce %s"), LocalID, *InstanceName.GetPlainNameString());
}

void FOSERootMotionSource_ConstantVerticalForce::AddReferencedObjects(class FReferenceCollector& Collector)
{
   Collector.AddReferencedObject(StrengthOverTime);
   Super::AddReferencedObjects(Collector);
}


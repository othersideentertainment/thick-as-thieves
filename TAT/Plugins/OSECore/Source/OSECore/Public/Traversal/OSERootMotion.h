// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/RootMotionSource.h"
#include "OSERootMotion.generated.h"



USTRUCT()
struct OSECORE_API FOSEConstrainDistanceRootMotion : public FRootMotionSource
{
   GENERATED_BODY()

public:

   FOSEConstrainDistanceRootMotion();
   virtual ~FOSEConstrainDistanceRootMotion() {}

   UPROPERTY()
   FVector TargetLocation;

   UPROPERTY()
   float MaxDistance;

   virtual FRootMotionSource* Clone() const override;

   virtual bool Matches(const FRootMotionSource* Other) const override;

   virtual bool MatchesAndHasSameState(const FRootMotionSource* Other) const override;

   virtual bool UpdateStateFrom(const FRootMotionSource* SourceToTakeStateFrom, bool bMarkForSimulatedCatchup = false) override;

   virtual void SetTime(float NewTime) override;

   virtual void PrepareRootMotion(
      float SimulationTime, 
      float MovementTickTime,
      const ACharacter& Character, 
      const UCharacterMovementComponent& MoveComponent
      ) override;

   virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override;

   virtual UScriptStruct* GetScriptStruct() const override;

   virtual FString ToSimpleString() const override;
};

template<>
struct TStructOpsTypeTraits< FOSEConstrainDistanceRootMotion > : public TStructOpsTypeTraitsBase2< FOSEConstrainDistanceRootMotion >
{
   enum
   {
      WithNetSerializer = true,
      WithCopy = true
   };
};

USTRUCT()
struct OSECORE_API FOSERootMotionSource_ConstantVerticalForce : public FRootMotionSource
{
   GENERATED_BODY()

public:

   FOSERootMotionSource_ConstantVerticalForce();
   virtual ~FOSERootMotionSource_ConstantVerticalForce() {}

   UPROPERTY()
   float VerticalForce;

   UPROPERTY()
   TObjectPtr<UCurveFloat> StrengthOverTime;

   UPROPERTY()
   float LateralDamping;

   UPROPERTY()
   float LateralMovementCoefficient;

   virtual FRootMotionSource* Clone() const override;

   virtual bool Matches(const FRootMotionSource* Other) const override;

   virtual bool MatchesAndHasSameState(const FRootMotionSource* Other) const override;

   virtual bool UpdateStateFrom(const FRootMotionSource* SourceToTakeStateFrom, bool bMarkForSimulatedCatchup = false) override;

   virtual void PrepareRootMotion(
      float SimulationTime,
      float MovementTickTime,
      const ACharacter& Character,
      const UCharacterMovementComponent& MoveComponent
   ) override;

   virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override;

   virtual UScriptStruct* GetScriptStruct() const override;

   virtual FString ToSimpleString() const override;

   virtual void AddReferencedObjects(class FReferenceCollector& Collector) override;
};

template<>
struct TStructOpsTypeTraits< FOSERootMotionSource_ConstantVerticalForce > : public TStructOpsTypeTraitsBase2< FOSERootMotionSource_ConstantVerticalForce >
{
   enum
   {
      WithNetSerializer = true,
      WithCopy = true
   };
};

// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Perception/StimInfo.h"

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SmartObjectSubsystem.h"

// self
#include "UtilityAIStateTarget.generated.h"

class USmartObjectComponent;

UENUM(BlueprintType)
enum class EBehaviorTargetType : uint8
{
   Invalid,
   Actor,
   Stim,
   ActorComponent,
   SmartObjectRequest,
};

USTRUCT(BlueprintType)
struct OSEAI_API FUtilityStateTarget
{
   GENERATED_BODY()

public:

   static const FUtilityStateTarget Invalid;

   FUtilityStateTarget() {}

   explicit FUtilityStateTarget(AActor* actor);

   explicit FUtilityStateTarget(UActorComponent* component);

   explicit FUtilityStateTarget(const FStimInfo& stim);

   explicit FUtilityStateTarget(const FSmartObjectRequestResult& smartObjectResult, UWorld* world, USmartObjectComponent* smartObjectComponent);

   bool IsValid() const { return TargetType != EBehaviorTargetType::Invalid; }

   bool PointsAt(AActor* actor) const;
   bool PointsAt(UActorComponent* component) const;
   bool PointsAt(const FStimInfo& stim) const;
   bool PointsAt(const FSmartObjectRequestResult& smartObjectResult) const;

   UObject* GetTargetUObject() const;
   FVector GetTargetWorldLocation() const;

   FString ToString() const;

   bool operator==(const FUtilityStateTarget& other) const;
   bool operator!=(const FUtilityStateTarget& other) const;

   UPROPERTY(BlueprintReadOnly)
   FStimInfo Stim;

   UPROPERTY(BlueprintReadOnly)
   TWeakObjectPtr<AActor> Actor;

   UPROPERTY(BlueprintReadOnly)
   TWeakObjectPtr<UActorComponent> Component;
   
   UPROPERTY(BlueprintReadOnly)
   FSmartObjectRequestResult SmartObjectRequestTarget;

   UPROPERTY(BlueprintReadOnly)
   EBehaviorTargetType TargetType = EBehaviorTargetType::Invalid;

   TWeakObjectPtr<UWorld> World;
};

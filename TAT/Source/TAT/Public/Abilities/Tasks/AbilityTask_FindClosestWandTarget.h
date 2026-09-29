// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "GameplayTagContainer.h"
#include "Abilities/Tasks/AbilityTask.h"

#include "AbilityTask_FindClosestWandTarget.generated.h"

UCLASS()
class TAT_API UAbilityTask_FindClosestWandTarget : public UAbilityTask
{
   GENERATED_BODY()
public:
   UAbilityTask_FindClosestWandTarget();

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClosestWandTargetChanged, AActor*, newTarget);

   UPROPERTY(BlueprintAssignable)
   FOnClosestWandTargetChanged OnClosestWandTargetChanged;

   /// If `actor` is not passed in, it will default to AvatarActor in Activate()
   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks|TAT", meta = (HidePin = "owningAbility", DefaultToSelf = "owningAbility"))
   static UAbilityTask_FindClosestWandTarget* FindClosestWandTarget(
      UGameplayAbility* owningAbility,
      AActor* actor,
      float maxRange,
      float maxDegreesFromForward,
      float pollFrequency,
      FCollisionProfileName traceProfile,
      const FGameplayTagContainer& blockedTags,
      bool debugDraw = false
   );

   virtual void Activate() override;

   virtual void OnDestroy(bool abilityIsEnding) override;

private:

   void _RefreshTarget();

   FTimerHandle _timerHandle;

   UPROPERTY(Transient)
   AActor* _currentTarget = nullptr;

   float _maxRange = 400.0;
   float _maxDegreesFromForward = 60.0;
   float _pollFrequency = 0.5;
   FCollisionProfileName _traceProfile;
   FGameplayTagContainer _blockedTags;
   bool _debugDraw = false;

   UPROPERTY(Transient)
   AActor* _sourceActor;
};

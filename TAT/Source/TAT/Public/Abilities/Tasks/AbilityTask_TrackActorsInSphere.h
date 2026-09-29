// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "Abilities/Tasks/AbilityTask.h"
#include "Abilities/GameplayAbilityTargetDataFilter.h"

#include "AbilityTask_TrackActorsInSphere.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTrackActorsInSphereDelegate, AActor*, foundActor);

UCLASS()
class TAT_API UAbilityTask_TrackActorsInSphere : public UAbilityTask
{
   GENERATED_BODY()
   
   UPROPERTY(BlueprintAssignable)
   FTrackActorsInSphereDelegate  OnActorEnter;

   UPROPERTY(BlueprintAssignable)
   FTrackActorsInSphereDelegate  OnActorLeave;


   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks|TAT", meta = (HidePin = "owningAbility", DefaultToSelf = "owningAbility", BlueprintInternalUseOnly = "TRUE"))
   static UAbilityTask_TrackActorsInSphere* TrackActorsInSphere(
         UGameplayAbility* owningAbility,
         float sphereRadius,
         float extraRemoveBuffer,
         float pollInterval,
         FCollisionProfileName collisionProfile,
         FGameplayTargetDataFilterHandle targetFilter,
         bool drawDebug = false);


   virtual void Activate() override;

protected:
   virtual void OnDestroy(bool abilityIsEnding) override;

   void _Update();
   void _AddActorsInRange(AActor* avatar);
   void _RemoveActorsOutOfRange(AActor* avatar);
   void _OnActorTracked(AActor* trackedActor);
   void _OnActorRemoved(AActor* removedActor);

protected:
   float _sphereRadius;
   float _extraRemoveDistance;
   float _pollInterval;
   FCollisionProfileName _collisionProfile;
   FGameplayTargetDataFilterHandle _targetFilter;
   bool _drawDebug;

   UPROPERTY(Transient)
   TArray<AActor*> _trackedActors;

   FTimerHandle _timerHandle;
};

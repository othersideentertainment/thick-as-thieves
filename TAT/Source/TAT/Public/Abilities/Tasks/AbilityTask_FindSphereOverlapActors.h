// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_FindSphereOverlapActors.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFindSphereOverlapActorsDelegate, const TArray<AActor*>&, FoundActors);

UCLASS()
class TAT_API UAbilityTask_FindSphereOverlapActors : public UAbilityTask
{
   GENERATED_BODY()

   UPROPERTY(BlueprintAssignable)
   FFindSphereOverlapActorsDelegate   FoundActors;

public:
   UAbilityTask_FindSphereOverlapActors(const FObjectInitializer& objectInitializer);

   /// If `actor` is not passed in, it will default to AvatarActor in Activate()
   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks|TAT", meta = (HidePin = "owningAbility", DefaultToSelf = "owningAbility", BlueprintInternalUseOnly = "TRUE"))
   static UAbilityTask_FindSphereOverlapActors* FindSphereOverlapActors(
      UGameplayAbility* owningAbility,
      AActor* actor,
      float sphereRadius,
      float timeBetweenActions,
      TArray<TEnumAsByte<EObjectTypeQuery>> objectTypes,
      bool drawDebugSphere = false
   );

   virtual void TickTask(float deltaTime) override;

   virtual void Activate() override;

   virtual void OnDestroy(bool abilityIsEnding) override;

   UFUNCTION(BlueprintCallable)
   void CancelTask();


private:
   void _FindActors();

   // Handle for _FindActors timer - See `AbilityTask_Repeat.h` for reference.
   FTimerHandle _timerHandle_FindActors;

   UPROPERTY(Transient)
   AActor* _actor = nullptr;

   float _timeBetweenActions = 0.5;
   float _sphereRadius = 400.0;

   bool _drawDebugSphere = false;

   UPROPERTY(Transient)
   TArray<AActor*> _ignore;
   TArray<TEnumAsByte<EObjectTypeQuery>> _overlapObjectTypes;
   
   UPROPERTY(Transient)
   TArray<AActor*> _outActors;
};

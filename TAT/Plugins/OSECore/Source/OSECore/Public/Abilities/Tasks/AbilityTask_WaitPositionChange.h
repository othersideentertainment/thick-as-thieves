// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_WaitPositionChange.generated.h"

class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWaitPositionChangeDelegate);

UCLASS()
class OSECORE_API UAbilityTask_WaitPositionChange : public UAbilityTask
{
   GENERATED_UCLASS_BODY()

   UPROPERTY(BlueprintAssignable)
   FWaitPositionChangeDelegate OnPositionChange;

   virtual void TickTask(float DeltaTime) override;

   /// Wait for the actor to move away from the specified position by more than the specified magnitude
   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks|OSE", meta = (DisplayName = "Wait Position Change", HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
   static UAbilityTask_WaitPositionChange* CreateWaitPositionChange(UGameplayAbility* owningAbility, FVector position, float minimumDistance);

   /// Wait for the actor to move away from the specified actor by more than the specified magnitude
   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks|OSE", meta = (DisplayName = "Wait Position Change From Actor", HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
   static UAbilityTask_WaitPositionChange* CreateWaitPositionChangeFromActor(UGameplayAbility* owningAbility, AActor* targetActor, FVector worldPosition, float minimumDistance);

   virtual void Activate() override;

private:
   UPROPERTY()
   AActor* _cachedActor;

   UPROPERTY()
   AActor* _targetActor;

   float   _minimumDistance;
   FVector _position;
};

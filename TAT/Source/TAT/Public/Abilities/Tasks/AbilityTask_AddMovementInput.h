// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_AddMovementInput.generated.h"


UCLASS()
class TAT_API UAbilityTask_AddMovementInput : public UAbilityTask
{
   GENERATED_UCLASS_BODY()

   // Apply the given movement input every frame
   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks|TAT", meta = (HidePin = "owningAbility", DefaultToSelf = "owningAbility", BlueprintInternalUseOnly = "TRUE"))
   static UAbilityTask_AddMovementInput* ApplyContinuousMovementInput(UGameplayAbility* owningAbility, FVector localDirection, float magnitude);


   virtual void TickTask(float deltaTime) override;

   virtual void Activate() override;

private:
   FVector _localDirection;
   float _magnitude;
   
   UPROPERTY()
   APawn* _pawn;
};

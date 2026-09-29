// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"

#include "AbilityTask_InterpControlRotationTo.generated.h"

/**
 *  Force the control rotation to the target rotation over a duration.
 */
UCLASS()
class TAT_API UAbilityTask_InterpControlRotationTo : public UAbilityTask
{
   GENERATED_BODY()
public:
   
   UAbilityTask_InterpControlRotationTo(const FObjectInitializer& objectInitializer);
   // Apply the given movement input every frame
   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks|TAT", meta = (HidePin = "owningAbility", DefaultToSelf = "owningAbility", BlueprintInternalUseOnly = "TRUE"))
   static UAbilityTask_InterpControlRotationTo* InterpControlRotationTo(UGameplayAbility* owningAbility, FRotator targetRotation, float duration);
   
   virtual void Activate() override;
   virtual void OnDestroy(bool bInOwnerFinished) override;
   virtual void TickTask(float deltaTime) override;
   
private:
   FRotator _currentRotation { FRotator::ZeroRotator };
   FRotator _targetRotation { FRotator::ZeroRotator };
   
   float _duration { 0.f };
   float _startTime { 0.f };
   float _endTime { 0.f };

   UPROPERTY()
   AController* _owningController { nullptr };
};

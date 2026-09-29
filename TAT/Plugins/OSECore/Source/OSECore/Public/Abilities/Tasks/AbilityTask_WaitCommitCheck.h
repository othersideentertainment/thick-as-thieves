// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_WaitCommitCheck.generated.h"

class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCommitCheckSuccess);

UCLASS()
class OSECORE_API UAbilityTask_WaitCommitCheck : public UAbilityTask
{
   GENERATED_UCLASS_BODY()

   UPROPERTY(BlueprintAssignable)
   FOnCommitCheckSuccess OnCommitCheckSuccess;

   virtual void TickTask(float deltaTime) override;

   /** Wait for the actor to move away from the specified position by more than the specified magnitude */
   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks|OSE", meta = (DisplayName = "Wait For Commit Check", HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
   static UAbilityTask_WaitCommitCheck* CreateWaitCommitCheck(UGameplayAbility* owningAbility);

protected:
   virtual void Activate() override;

private:
   void _CheckCommitSuccess();
};

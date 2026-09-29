// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "Abilities/Tasks/AbilityTask.h"

#include "AbilityTask_PossessNewPawn.generated.h"

class APawn;
class AController;

UCLASS()
class TAT_API UAbilityTask_PossessNewPawn : public UAbilityTask
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Ability|Tasks|TAT", meta = (HidePin = "owningAbility", DefaultToSelf = "owningAbility", BlueprintInternalUseOnly = "TRUE"))
   static UAbilityTask_PossessNewPawn* AuthorityPossessNewPawn(
      UGameplayAbility* owningAbility,
      APawn* newPawnToPossess);

protected:
   virtual void Activate() override;
   virtual void OnDestroy(bool abilityIsEnding) override;

private:
   UPROPERTY(Transient)
   AController* _oldController = nullptr;
   UPROPERTY(Transient)
   APawn* _oldPawn = nullptr;
   UPROPERTY(Transient)
   APawn* _newPawn = nullptr;
};

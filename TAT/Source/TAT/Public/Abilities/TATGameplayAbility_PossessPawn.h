// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

//ose
#include "Abilities/OSEGameplayAbility.h"

#include "TATGameplayAbility_PossessPawn.generated.h"

class APawn;
class AController;

UCLASS()
class TAT_API UTATGameplayAbility_PossessPawn : public UOSEGameplayAbility
{
   GENERATED_BODY()

public:
   UTATGameplayAbility_PossessPawn();

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
   void AuthoritySwitchPossessionToNewPawn(APawn* pawn);

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
   void AuthoritySwitchPossessionBackToPreviousPawn();

private:
   bool _isPossessingNewPawn = false;

   UPROPERTY(Transient)
   AController* _oldController = nullptr;
   UPROPERTY(Transient)
   APawn* _oldPawn = nullptr;
   UPROPERTY(Transient)
   APawn* _newPawn = nullptr;
};

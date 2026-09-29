// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/Traversal/OSEGameplayAbility_Character.h"

#include "TATGameplayAbility_ContractDistanceContraint.generated.h"


UCLASS()
class TAT_API UTATGameplayAbility_ContractDistanceContraint : public UOSEGameplayAbility_Character
{
   GENERATED_BODY()
   
public:
   UTATGameplayAbility_ContractDistanceContraint();


protected:
   virtual bool CanActivateAbility(ACharacter* character) const override;
   virtual void ActivateAbility(ACharacter* character) override;
   virtual void CancelAbility(ACharacter* character) override;
};

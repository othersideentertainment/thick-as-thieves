// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/Traversal/OSEGameplayAbility_Character.h"

#include "TATGameplayAbility_ExtendDistanceContraint.generated.h"


UCLASS()
class TAT_API UTATGameplayAbility_ExtendDistanceContraint : public UOSEGameplayAbility_Character
{
   GENERATED_BODY()
   
public:
   UTATGameplayAbility_ExtendDistanceContraint();


protected:
   virtual bool CanActivateAbility(ACharacter* character) const override;
   virtual void ActivateAbility(ACharacter* character) override;
   virtual void CancelAbility(ACharacter* character) override;
};

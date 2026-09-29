// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "GameplayBehaviorConfig_Animation.h"

#include "TATGameplayBehaviorConfig_Animation.generated.h"

UCLASS()
class TAT_API UTATGameplayBehaviorConfig_Animation : public UGameplayBehaviorConfig_Animation
{
   GENERATED_BODY()
public:
   TSoftObjectPtr<UAnimMontage> GetAnimMontage() const { return AnimMontage; }
};

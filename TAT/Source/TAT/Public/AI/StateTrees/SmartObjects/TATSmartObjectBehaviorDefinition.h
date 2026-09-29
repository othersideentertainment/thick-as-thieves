// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "SmartObjectDefinition.h"
#include "StateTreeReference.h"

#include "TATSmartObjectBehaviorDefinition.generated.h"

UCLASS()
class TAT_API UTATSmartObjectBehaviorDefinition : public USmartObjectBehaviorDefinition
{
   GENERATED_BODY()
public:
   UPROPERTY(EditDefaultsOnly, Category="", meta=(Schema="/Script/TAT.TATSmartObjectStateTreeSchema"))
   FStateTreeReference StateTreeReference;
};

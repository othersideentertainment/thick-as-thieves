// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_CheckIfInForcedState.generated.h"

UCLASS()
class TAT_API UBTDecorator_CheckIfInForcedState : public UBTDecorator
{
   GENERATED_BODY()
public:
   UBTDecorator_CheckIfInForcedState(const FObjectInitializer& objectInitializer);
   virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory) const override;
};

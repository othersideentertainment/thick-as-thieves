// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Utility/UtilityAIState.h"

// ue4
#include "CoreMinimal.h"

#include "UtilityAIGoal.generated.h"

/// A goal for the utility AI
UCLASS(BlueprintType, Blueprintable)
class OSEAI_API UUtilityAIGoal : public UUtilityAIStateBase
{
   GENERATED_BODY()

public:

   UUtilityAIGoal();

   // from UUtilityAIStateBase
   virtual void Init(UUtilityAIComponent& utilityAIComponent) override;
   virtual void Reset() override;
   virtual void Enter() override;
   virtual void Exit() override;
   virtual bool IsInterruptible() const override { return true; }
   virtual bool IsComplete() const override { return false; }
};

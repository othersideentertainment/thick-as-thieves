// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "StateTreePropertyRef.h"
#include "StateTreeTaskBase.h"

// ose
#include "Abilities/OSEAbilityInputBinds.h"

#include "TATStateTreeTaskApplyGameplayEffect.generated.h"

class AAIController;

USTRUCT()
struct FTATStateTreeTaskApplyGameplayEffectData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category = Context)
   TObjectPtr<AAIController> AIController = nullptr;

   UPROPERTY(EditAnywhere, Category = "In")
   TSubclassOf<UGameplayEffect> GameplayEffectClass = nullptr;

   UPROPERTY(EditAnywhere, Category = "In")
   float Level = 0.0f;

   FActiveGameplayEffectHandle AppliedEffectHandle;
   
   FDelegateHandle BoundDelegateHandle;

   bool IsFinished = false;
};

// Applies a gameplay effect on EnterState that is cleared on ExitState.
// The lifespan of the effect is also tracked on Tick which, if it is
// found to be removed, this state will end.
USTRUCT(meta = (DisplayName = "Apply Gameplay Effect", Category = "TAT|Gameplay Effect"))
struct TAT_API FTATStateTreeTaskApplyGameplayEffect : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeTaskApplyGameplayEffectData;
	
   FTATStateTreeTaskApplyGameplayEffect() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
   virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& context, const float deltaTime) const override;
   virtual void ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
};

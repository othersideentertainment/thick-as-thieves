// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/Utility/TATUtilityAIBehaviorComponent.h"
#include "AI/Utility/UtilityAITypes.h"

// ue
#include "Engine/DataAsset.h"

#include "TATEscalationModifier.generated.h"


class ATATAIController;
class UGameplayEffect;

UCLASS(Abstract, Const, DefaultToInstanced, EditInlineNew, CollapseCategories)
class TAT_API UTATEscalationModifier : public UObject
{
   GENERATED_BODY()

public:
   virtual void Apply(ATATAIController* aiController) const {};
   virtual void Revert(ATATAIController* aiController) const {};
};

UCLASS(BlueprintType)
class TAT_API UTATEscalationModifier_GameplayEffect : public UTATEscalationModifier
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere)
   TArray<TSubclassOf<UGameplayEffect>> EffectsToApply;

public:
   virtual void Apply(ATATAIController* aiController) const override;
   virtual void Revert(ATATAIController* aiController) const override;
};

UCLASS(BlueprintType)
class TAT_API UTATEscalationModifier_Behaviors : public UTATEscalationModifier
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere)
   FGameplayTag BehaviorInjectionTag;
   
   UPROPERTY(EditAnywhere)
   TArray<TObjectPtr<UUtilityBehaviorSet>> BehaviorsToAdd;

public:
   virtual void Apply(ATATAIController* aiController) const override;
   virtual void Revert(ATATAIController* aiController) const override;
};

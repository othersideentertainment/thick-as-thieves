// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

// tat
#include "Modifiers/TATEscalationModifier.h"
#include "TATEscalationDataAsset.generated.h"

class UGameplayAbility;
class ATATAIController;

UCLASS()
class TAT_API UTATEscalationDataAsset : public UDataAsset
{
   GENERATED_BODY()
public:
   void ApplyModifiers(ATATAIController* aiController) const;
   void RevertModifiers(ATATAIController* aiController) const;

   void ActivateStateGameplayAbility(const ATATAIController* aiController) const;
   void DeactivateStateGameplayAbility(const ATATAIController* aiController) const;
protected:
   UPROPERTY(EditDefaultsOnly, Category="State")
   TSubclassOf<UGameplayAbility> StateAbility;
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced, Category = "Modifiers", meta = (DisplayName = "Escalation Modifiers", ShowOnlyInnerProperties, DisplayPriority = 0))
   TArray<TObjectPtr<UTATEscalationModifier>> EscalationModifiers;
};
